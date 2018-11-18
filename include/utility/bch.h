// EPOS BCH Correction Code Module

#ifndef __bch_h
#define __bch_h

#define MAX_POLY 16

#include <system/config.h>
#include <utility/string.h>
#include <utility/galois.h>

__BEGIN_SYS

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef uint32_t bch_poly_t;


struct BCH_Def {
    int syns; /* Number of errors to compute, when decoding */
    int degree; /* Generator degree */
    int ecc_bytes; /* Number of ECC bytes */
    bch_poly_t generator; /* Generator polynominal for BCH. LSB is highest-order term */
};

class BCH
{
  public:
    BCH() {}

    /* Verify the page. If it needs to be corrected, returns -1. Otherwise,
     * returns 0 on success
     */
    int verify(const struct BCH_Def *bch, const uint8_t *chunk, uint32_t len, const uint8_t *ecc);

    /* Repair errors on the input signal */
    void repair(const struct BCH_Def *bch, uint8_t *chunk, uint32_t len, uint8_t *ecc);

    /* Generates parity bits */
    void bch_generate(const struct BCH_Def *bch, const uint8_t *chunk, uint32_t len, uint8_t *ecc);
  private:
    bch_poly_t unpack_poly(const struct BCH_Def *def, const uint8_t *ecc);
    void poly_add(uint16_t *dst, const uint16_t *src, uint16_t c, int shift);
    void pack_poly(const struct BCH_Def *def, bch_poly_t poly, uint8_t *ecc);
    uint16_t poly_eval(const uint16_t *s, uint16_t x);
    bch_poly_t chunk_remainder(const struct BCH_Def *def, const uint8_t *chunk, uint32_t len);
    uint16_t syndrome(const struct BCH_Def *bch, const uint8_t *chunk, uint32_t len, bch_poly_t remainder, uint16_t x);
    void berlekamp_massey(const uint16_t *s, int N, uint16_t *sigma);
};

__END_SYS

#endif
