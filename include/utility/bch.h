// EPOS BCH Correction Code Module

#ifndef __bch_h
#define __bch_h

#define MAX_POLY 16

#include <utility/string.h>
#include <utility/galois.h>
#include <persistent_storage.h>
#include <riffs.h>

__BEGIN_SYS

typedef uint32_t bch_poly_t;

struct BCH_Standard_Def {
    const static int syns             = Traits<BCH>::SYNS_SIZE;
    const static int degree           = Traits<BCH>::POLY_DEGREE;
    const static int ecc_bytes        = Traits<BCH>::POLY_DEGREE;
    const static bch_poly_t generator = Traits<BCH>::POLY_DEGREE;
};

template<typename BCH_Def>
class BCH_Wrapper {

public:
    BCH_Wrapper() = default;

    /* Verify the page. If it needs to be corrected, returns -1. Otherwise,
     * returns 0 on success
     */
    static int verify(const uint8_t *chunk, uint32_t len, const uint8_t *ecc) {
        return (chunk_remainder(chunk, len) == unpack_poly(ecc)) ? 0 : -1;
    }

    /* Repair errors on the input signal */
    static void repair(uint8_t *chunk, uint32_t len, uint8_t *ecc) {
        const bch_poly_t remainder = unpack_poly(ecc);
        const int chunk_bits = len << 3;
        uint16_t syns[8];
        uint16_t sigma[MAX_POLY];
        uint16_t x;

        /* Compute syndrome vector */
        x = 2;

        for (int i = 0; i < BCH_Def::syns; i++) {
            syns[i] = syndrome(chunk, len, remainder, x);
            x = Galois::galois_mulx(x);
        }

        /* Compute sigma */
        berlekamp_massey(syns, BCH_Def::syns, sigma);

        /* Each root of sigma corresponds to an error location. Correct
         * errors in the chunk data first.
         */
        x = 1;
        for (int i = 0; i < chunk_bits; i++) {
            if (!poly_eval(sigma, x))
                chunk[i >> 3] ^= 1 << (i & 7);
            x = Galois::galois_mul(x, 0x100d);
        }

        /* Then correct errors in the ECC data */
        for (int i = 0; i < BCH_Def::degree; i++) {
            if (!poly_eval(sigma, x))
                ecc[i >> 3] ^= 1 << (i & 7);
            x = Galois::galois_mul(x, 0x100d);
        }
    }

    /* Generates parity bits */
    static void bch_generate(const uint8_t *chunk, uint32_t len, uint8_t *ecc) {
        pack_poly(chunk_remainder(chunk, len), ecc);
    }

private:
    static bch_poly_t unpack_poly(const uint8_t *ecc) {
        bch_poly_t poly = 0;

        for (int  i = BCH_Def::ecc_bytes - 1; i >= 0; i--) {
            poly <<= 8;
            poly |= ecc[i] ^ 0xff;
        }

        poly &= ((1LL << BCH_Def::degree) - 1);

        return poly;
    }

    static void poly_add(uint16_t *dst, const uint16_t *src, uint16_t c, int shift) {
        for (unsigned int i = 0; i < MAX_POLY; i++) {
            int p = i + shift;
            uint16_t v = src[i];

            if (p < 0 || p >= MAX_POLY || !v)
                continue;

            dst[p] ^= Galois::galois_mul(v, c);
        }
    }

    static void pack_poly(bch_poly_t poly, uint8_t *ecc) {
        for (int i = 0; i < BCH_Def::ecc_bytes; i++) {
            ecc[i] = ~poly;
            poly >>= 8;
        }
    }

    static uint16_t poly_eval(const uint16_t *s, uint16_t x) {
        uint16_t sum = 0;
        uint16_t t = x;

        for (unsigned int i = 0; i < MAX_POLY; i++) {
            const uint16_t c = s[i];

            if (c)
                sum ^= Galois::galois_mul(c, t);

            t = Galois::galois_mul(t, x);
        }

        return sum;
    }

    static bch_poly_t chunk_remainder(const uint8_t *chunk, uint32_t len) {
        bch_poly_t remain = 0;

        for (unsigned int i = 0; i < len; i++) {
            remain ^= chunk[i] ^ 0xff;
            for (unsigned int j = 0; j < 8; j++) {
                if (remain & 1)
                    remain ^= BCH_Def::generator;
                remain >>= 1;
            }
        }

        return remain;
    }

    static uint16_t syndrome(const uint8_t *chunk, uint32_t len, bch_poly_t remainder, uint16_t x) {
        uint16_t y = 0;
        uint16_t t = 1;

        for (unsigned int i = 0; i < len; i++) {
            uint8_t c = chunk[i] ^ 0xff;

            for (unsigned int j = 0; j < 8; j++) {
                if (c & 1)
                    y ^= t;

                c >>= 1;
                t = Galois::galois_mul(t, x);
            }
        }

        for (int j = 0; j < BCH_Def::degree; j++) {
            if (remainder & 1)
                y ^= t;

            remainder >>= 1;
            t = Galois::galois_mul(t, x);
        }

        return y;
    }

    static void berlekamp_massey(const uint16_t *s, int N, uint16_t *sigma) {
        uint16_t C[MAX_POLY];
        uint16_t B[MAX_POLY];
        int L = 0;
        int m = 1;
        uint16_t b = 1;

        memset(sigma, 0, MAX_POLY * sizeof(sigma[0]));
        memset(B, 0, sizeof(B));
        memset(C, 0, sizeof(C));
        B[0] = 1;
        C[0] = 1;

        for (int n = 0; n < N; n++) {
            uint16_t d = s[n];
            uint16_t mult;

            for (int i = 1; i <= L; i++) {
                if (!(C[i] && s[n - i]))
                    continue;

                d ^= Galois::galois_mul(C[i], s[n - i]);
            }

            mult = Galois::galois_div(d, b);

            if (!d) {
                m++;
            } else if (L * 2 <= n) {
                uint16_t T[MAX_POLY];

                memcpy(T, C, sizeof(T));
                poly_add(C, B, mult, m);
                memcpy(B, T, sizeof(B));
                L = n + 1 - L;
                b = d;
                m = 1;
            } else {
                poly_add(C, B, mult, m);
                m++;
            }
        }

        memcpy(sigma, C, MAX_POLY);
    }

};

class PUF {

public:
    static void bootstrap() {
        char* SRAM_BEG = reinterpret_cast<char*>(Traits<Machine>::PUF_BASE);
        char* SRAM_END = reinterpret_cast<char*>(Traits<Machine>::PUF_END);

        unsigned int FLASH_END = Traits<Machine>::FLASH_STORAGE_TOP;

        uint16_t bch_size = Traits<BCH>::TEST_CHUNK_SIZE;
        uint16_t input_size = Traits<BCH>::CHUNK_SIZE;

        uint8_t block[bch_size];

        int index = 0;
        while(SRAM_BEG != SRAM_END) {
            block[index] = *SRAM_BEG++;
            index++;
        }

        BCH_Wrapper<BCH_Standard_Def>::bch_generate(block, input_size, block + input_size);

        Persistent_Storage::write((Persistent_Storage::SIZE - 3) - (sizeof(Persistent_Storage::Word) * 18), block, bch_size);
    }

    static uint8_t* read_sram() {
        char* SRAM_BEG = reinterpret_cast<char*>(Traits<Machine>::PUF_BASE);
        char* SRAM_END = reinterpret_cast<char*>(Traits<Machine>::PUF_END);

        uint16_t bch_size = (Traits<BCH>::TEST_CHUNK_SIZE);
        uint16_t input_size = (Traits<BCH>::CHUNK_SIZE);
        uint8_t* block = new uint8_t[bch_size];

        unsigned int FLASH_END = Traits<Machine>::FLASH_STORAGE_TOP;
        unsigned int ECC_FLASH_BEG = (Persistent_Storage::SIZE - 3) - (sizeof(Persistent_Storage::Word) * 2);

        for (uint16_t i = input_size; i < bch_size; i++) {
            Persistent_Storage::read(ECC_FLASH_BEG++, &block[i], sizeof(uint8_t));
        }

        int index = 0;
        while(SRAM_BEG != SRAM_END) {
            block[index] = *SRAM_BEG++;
            index++;
        }

        BCH_Wrapper<BCH_Standard_Def>::repair(block, input_size, block + input_size);
        bool test = BCH_Wrapper<BCH_Standard_Def>::verify(block, input_size, block + input_size);

        if (test)
            return block;
        else
            return 0;
    }

};

__END_SYS

#endif
