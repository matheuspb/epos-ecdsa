// EPOS BCH Correction Code Implementation

#include <utility/bch.h>

__BEGIN_SYS

/* Private Methods */
bch_poly_t BCH::unpack_poly(const struct BCH_Def *def, const uint8_t *ecc)
{
  bch_poly_t poly = 0;

  for (int  i = def->ecc_bytes - 1; i >= 0; i--) {
    poly <<= 8;
    poly |= ecc[i] ^ 0xff;
  }

  poly &= ((1LL << def->degree) - 1);

  return poly;
}

void BCH::poly_add(uint16_t *dst, const uint16_t *src, uint16_t c, int shift)
{
    for (unsigned int i = 0; i < MAX_POLY; i++) {
        int p = i + shift;
        uint16_t v = src[i];

        if (p < 0 || p >= MAX_POLY || !v)
            continue;

        dst[p] ^= Galois::galois_mul(v, c);
    }
}

void BCH::pack_poly(const struct BCH_Def *def, bch_poly_t poly, uint8_t *ecc)
{
	for (int i = 0; i < def->ecc_bytes; i++) {
		ecc[i] = ~poly;
		poly >>= 8;
	}
}

bch_poly_t BCH::chunk_remainder(const struct BCH_Def *def, const uint8_t *chunk, uint32_t len)
{
  bch_poly_t remain = 0;

  for (unsigned int i = 0; i < len; i++) {
    remain ^= chunk[i] ^ 0xff;
    for (unsigned int j = 0; j < 8; j++) {
      if (remain & 1)
        remain ^= def->generator;
      remain >>= 1;
    }
  }

  return remain;
}

uint16_t BCH::syndrome(const struct BCH_Def *bch, const uint8_t *chunk, uint32_t len, bch_poly_t remainder, uint16_t x)
{
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

  for (int j = 0; j < bch->degree; j++) {
    if (remainder & 1)
      y ^= t;

    remainder >>= 1;
    t = Galois::galois_mul(t, x);
  }

  return y;
}

void BCH::berlekamp_massey(const uint16_t *s, int N, uint16_t *sigma)
{
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

uint16_t BCH::poly_eval(const uint16_t *s, uint16_t x)
{
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


/* Public Methods */
int BCH::verify(const struct BCH_Def *bch, const uint8_t *chunk, uint32_t len, const uint8_t *ecc)
{
	return (chunk_remainder(bch, chunk, len) == unpack_poly(bch, ecc)) ? 0 : -1;
}

void BCH::repair(const struct BCH_Def *bch, uint8_t *chunk, uint32_t len, uint8_t *ecc)
{
  const bch_poly_t remainder = unpack_poly(bch, ecc);
  const int chunk_bits = len << 3;
  uint16_t syns[8];
  uint16_t sigma[MAX_POLY];
  uint16_t x;

  /* Compute syndrome vector */
  x = 2;

  for (int i = 0; i < bch->syns; i++) {
    syns[i] = syndrome(bch, chunk, len, remainder, x);
    x = Galois::galois_mulx(x);
  }

  /* Compute sigma */
  berlekamp_massey(syns, bch->syns, sigma);

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
  for (int i = 0; i < bch->degree; i++) {
    if (!poly_eval(sigma, x))
      ecc[i >> 3] ^= 1 << (i & 7);
    x = Galois::galois_mul(x, 0x100d);
  }
}


void BCH::bch_generate(const struct BCH_Def *bch, const uint8_t *chunk, uint32_t len, uint8_t *ecc)
{
	pack_poly(bch, chunk_remainder(bch, chunk, len), ecc);
}
__END_SYS
