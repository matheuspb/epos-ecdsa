#ifndef __machine_cortex_ecdsa_h
#define __machine_cortex_ecdsa_h

#include <pka.h>
#include <utility/hwbignum.h>

__BEGIN_UTIL

struct SecP256Info {
    static const uint8_t size = 8;
    static const uint32_t prime[];
    static const uint32_t n[];
    static const uint32_t a[];
    static const uint32_t b[];
    static const uint32_t x[];
    static const uint32_t y[];
};

const uint32_t SecP256Info::prime[] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF };
const uint32_t SecP256Info::n[] = {
    0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
    0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF };
const uint32_t SecP256Info::a[] = {
    0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF };
const uint32_t SecP256Info::b[] = {
    0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
    0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8 };
const uint32_t SecP256Info::x[] = {
    0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
    0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2 };
const uint32_t SecP256Info::y[] = {
    0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
    0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2 };

struct SecP192Info {
    static const uint8_t size = 6;
    static const uint32_t prime[];
    static const uint32_t n[];
    static const uint32_t a[];
    static const uint32_t b[];
    static const uint32_t x[];
    static const uint32_t y[];
};

const uint32_t SecP192Info::prime[] = {
    0xffffffff, 0xffffffff, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff };
const uint32_t SecP192Info::n[] = {
    0xb4d22831, 0x146bc9b1, 0x99def836, 0xffffffff, 0xffffffff, 0xffffffff };
const uint32_t SecP192Info::a[] = {
    0xfffffffc, 0xffffffff, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff };
const uint32_t SecP192Info::b[] = {
    0xc146b9b1, 0xfeb8deec, 0x72243049, 0x0fa7e9ab, 0xe59c80e7, 0x64210519 };
const uint32_t SecP192Info::x[] = {
    0x82ff1012, 0xf4ff0afd, 0x43a18800, 0x7cbf20eb, 0xb03090f6, 0x188da80e };
const uint32_t SecP192Info::y[] = {
    0x1e794811, 0x73f977a1, 0x6b24cdd5, 0x631011ed, 0xffc8da78, 0x07192b95 };

struct SecP256k1Info {
    static const uint8_t size = 8;
    static const uint32_t prime[];
    static const uint32_t n[];
    static const uint32_t a[];
    static const uint32_t b[];
    static const uint32_t x[];
    static const uint32_t y[];
};

const uint32_t SecP256k1Info::prime[] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFC2F };
const uint32_t SecP256k1Info::n[] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE,
    0xBAAEDCE6, 0xAF48A03B, 0xBFD25E8C, 0xD0364141 };
const uint32_t SecP256k1Info::a[] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000 };
const uint32_t SecP256k1Info::b[] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000007 };
const uint32_t SecP256k1Info::x[] = {
    0x79BE667E, 0xF9DCBBAC, 0x55A06295, 0xCE870B07,
    0x029BFCDB, 0x2DCE28D9, 0x59F2815B, 0x16F81798 };
const uint32_t SecP256k1Info::y[] = {
    0x483ADA77, 0x26A3C465, 0x5DA4FBFC, 0x0E1108A8,
    0xFD17B448, 0xA6855419, 0x9C47D08F, 0xFB10D4B8 };


template <typename> class ECDSA;

template <typename Curve>
class ECPoint {

    template<typename> friend class ECDSA;
    typedef uint8_t PKAStatus;
    typedef HWBignum Scalar;

public:

    ECPoint() {
        memcpy(x, Curve::x, Curve::size * sizeof(*Curve::x));
        memcpy(y, Curve::y, Curve::size * sizeof(*Curve::y));
    }

    ECPoint(const ECPoint& other) {
        memcpy(x, other.x, Curve::size * sizeof(*Curve::x));
        memcpy(y, other.y, Curve::size * sizeof(*Curve::y));
    }

    ECPoint& operator=(const ECPoint& other) {
        memcpy(x, other.x, Curve::size * sizeof(*Curve::x));
        memcpy(y, other.y, Curve::size * sizeof(*Curve::y));
        return *this;
    }

    ECPoint& operator*=(const Scalar& x) {
        uint32_t res_addr = 0;
        ecc_mul_start(x, res_addr);
        PKAStatus op_status = 0;
        do {
            op_status = ecc_mul_result(res_addr);
        } while (op_status == PKA_STATUS_OPERATION_INPRG);
        return *this;
    }

    ECPoint operator+(const ECPoint& p) {
        uint32_t res_addr = 0;
        ecc_add_start(p, res_addr);
        PKAStatus op_status = 0;
        ECPoint result;
        do {
            op_status = ecc_add_result(result, res_addr);
        } while (op_status == PKA_STATUS_OPERATION_INPRG);
        return result;
    }

    bool operator==(const ECPoint& other) {
        if (memcmp(x, other.x, Curve::size * sizeof(*Curve::x)) != 0)
            return false;
        if (memcmp(y, other.y, Curve::size * sizeof(*Curve::y)) != 0)
            return false;
        return true;
    }

    friend OStream& operator<<(OStream& out, const ECPoint& p) {
        out << "x = [";
        for (uint8_t i = 0; i < Curve::size; i++) {
            out << p.x[i];
            out << ", ";
        }
        out << "], y = [";
        for (uint8_t i = 0; i < Curve::size; i++) {
            out << p.y[i];
            out << ", ";
        }
        out << "]";
    }

private:

    PKAStatus ecc_mul_start(const Scalar& scalar, uint32_t& result_vector) {
        /* Calculate the extra buffer requirement. */
        uint8_t extra_buf_size = 2 + Curve::size % 2, i;
        uint32_t offset = 0;

        /* Make sure no PKA operation is in progress. */
        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        /* Update the A ptr with the offset address of the PKA RAM location
         * where the scalar will be stored. */
        HWREG(PKA_APTR) = offset >> 2;

        /* Load the scalar in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = scalar._data[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + (Curve::size % 2));

        /* Update the B ptr with the offset address of the PKA RAM location
         * where the curve parameters will be stored. */
        HWREG(PKA_BPTR) = offset >> 2;

        /* Write curve parameter 'p' as 1st part of vector B immediately
         * following vector A at PKA RAM */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::prime[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Copy curve parameter 'a' in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::a[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Copy curve parameter 'b' in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::b[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Update the C ptr with the offset address of the PKA RAM location
         * where the x, y will be stored. */
        HWREG(PKA_CPTR) = offset >> 2;

        /* Write elliptic curve point.x co-ordinate value. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->x[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Write elliptic curve point.y co-ordinate value. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->y[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Update the result location. */
        result_vector = PKA_RAM_BASE + offset;

        /* Load D ptr with the result location in PKA RAM. */
        HWREG(PKA_DPTR) = offset >> 2;

        /* Load length registers. */
        HWREG(PKA_ALENGTH) = Curve::size;
        HWREG(PKA_BLENGTH) = Curve::size;

        /* set the PKA function to ECC-MULT and start the operation. */
        HWREG(PKA_FUNCTION) = 0x0000D000;

        return PKA_STATUS_SUCCESS;
    }

    PKAStatus ecc_mul_result(uint32_t result_vector) {
        /* Check for the arguments. */
        assert(result_vector > PKA_RAM_BASE);
        assert(result_vector < (PKA_RAM_BASE + PKA_RAM_SIZE));

        /* Verify that the operation is completed. */
        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        if(HWREG(PKA_SHIFT) == 0x00000000) {
            /* Get the MSW register value. */
            uint32_t regMSWVal = HWREG(PKA_MSW);

            /* Check to make sure that the result vector is not all zeroes. */
            if(regMSWVal & PKA_MSW_RESULT_IS_ZERO) {
                return PKA_STATUS_RESULT_0;
            }

            /* Get the length of the result */
            uint32_t len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1)
                - ((result_vector - PKA_RAM_BASE) >> 2);

            /* copy the x co-ordinate value of the result from vector D into
             * the \e ec_point. */
            uint32_t addr = result_vector;
            uint8_t i;
            for(i = 0; i < len; i++) {
                this->x[i] = HWREG(addr + 4 * i);
            }

            /* copy the y co-ordinate value of the result from vector D into
             * the \e ec_point. */
            addr += 4 * (i + 2 + len % 2);
            for(i = 0; i < len; i++) {
                this->y[i] = HWREG(addr + 4 * i);
            }

            return PKA_STATUS_SUCCESS;
        } else {
            return PKA_STATUS_FAILURE;
        }
    }

    PKAStatus ecc_add_start(const ECPoint& p, uint32_t& result_vector) {
        /* Calculate the extra buffer requirement. */
        uint8_t extra_buf_size = 2 + Curve::size % 2, i;
        uint32_t offset = 0;

        /* Make sure no operation is in progress. */
        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        /* Update the A ptr with the offset address of the PKA RAM location
         * where the first ecPt will be stored. */
        HWREG(PKA_APTR) = offset >> 2;

        /* Load the x co-ordinate value of the first EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->x[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Load the y co-ordinate value of the first EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->y[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Update the B ptr with the offset address of the PKA RAM location
         * where the curve parameters will be stored. */
        HWREG(PKA_BPTR) = offset >> 2;

        /* Write curve parameter 'p' as 1st part of vector B */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::prime[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Write curve parameter 'a'. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::a[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Update the C ptr with the offset address of the PKA RAM location
         * where the ecPt2 will be stored. */
        HWREG(PKA_CPTR) = offset >> 2;

        /* Load the x co-ordinate value of the second EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = p.x[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Load the y co-ordinate value of the second EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = p.y[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extra_buf_size);

        /* Copy the result vector location. */
        result_vector = PKA_RAM_BASE + offset;

        /* Load D ptr with the result location in PKA RAM. */
        HWREG(PKA_DPTR) = offset >> 2;

        /* Load length registers. */
        HWREG(PKA_BLENGTH) = Curve::size;

        /* Set the PKA Function to ECC-ADD and start the operation. */
        HWREG(PKA_FUNCTION) = 0x0000B000;

        return PKA_STATUS_SUCCESS;
    }

    PKAStatus ecc_add_result(ECPoint& result, uint32_t result_vector) {
        /* Check for the arguments. */
        assert(result_vector > PKA_RAM_BASE);
        assert(result_vector < (PKA_RAM_BASE + PKA_RAM_SIZE));

        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        if(HWREG(PKA_SHIFT) == 0x00000000) {
            /* Get the MSW register value. */
            uint32_t regMSWVal = HWREG(PKA_MSW);

            /* Check to make sure that the result vector is not all zeroes. */
            if(regMSWVal & PKA_MSW_RESULT_IS_ZERO) {
                return PKA_STATUS_RESULT_0;
            }

            /* Get the length of the result. */
            uint32_t len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1)
                - ((result_vector - PKA_RAM_BASE) >> 2);

            /* Copy the x co-ordinate value of result from vector D into the
             * the output EC Point. */
            uint32_t addr = result_vector;
            uint8_t i;
            for(i = 0; i < len; i++) {
                result.x[i] = HWREG(addr + 4 * i);
            }

            /* Copy the y co-ordinate value of result from vector D into the
             * the output EC Point. */
            addr += 4 * (i + 2 + len % 2);
            for(i = 0; i < len; i++) {
                result.y[i] = HWREG(addr + 4 * i);
            }

            return PKA_STATUS_SUCCESS;
        } else {
            return PKA_STATUS_FAILURE;
        }
    }

    uint32_t x[Curve::size];
    uint32_t y[Curve::size];

};

template<typename Curve>
class ECDSA : public ECDSA_Common {

    typedef ECPoint<Curve> Point;
    typedef HWBignum Scalar;
    static const uint32_t zero[Curve::size];

public:

    struct Signature {
        Signature(const Scalar& _r, const Scalar& _s): r(_r), s(_s) {}
        Scalar r;
        Scalar s;
    };

    ECDSA():
        d(zero, Curve::size*4),
        n(Curve::n, Curve::size*4)
    {}

    void gen_key_pair() {
        d.random_same_size();
        q *= d;
    }

    Signature sign(const Scalar& z) {
        Scalar k(zero, Curve::size*4);
        k.random_same_size();
        k = k % n;

        Point g;
        g *= k;
        Scalar x_1(g.x, Curve::size*4);

        Scalar r = x_1 % n;
        Scalar s = (k.inv_mod(n) * (z + r*d)) % n;

        return Signature(r, s);
    }

    bool verify(const Scalar& z, const Signature& sig) {
        Scalar w = sig.s.inv_mod(n);

        Scalar u_1 = (z*w) % n;
        Scalar u_2 = (sig.r*w) % n;

        Point g;
        g *= u_1;
        Point q_1(q);
        q_1 *= u_2;

        Point result = g + q_1;
        Scalar x_1(result.x, Curve::size*4);
        x_1 = x_1 % n;

        return Scalar::cmp(&x_1, &(sig.r)) == 0;
    }

private:

    Scalar d;  // Private key
    Point  q;  // Public key
    Scalar n;  // Order of the curve generator

};

template <typename Curve> const uint32_t ECDSA<Curve>::zero[Curve::size] = {0};

__END_UTIL

#endif
