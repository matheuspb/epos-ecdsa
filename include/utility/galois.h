// EPOS Galois Fields Module, used on BCH

#include <system/config.h>

#ifndef __galois_h
#define __galois_h

__BEGIN_SYS

typedef unsigned short uint16_t;

class Galois
{
  public:
    static uint16_t galois_mulx(uint16_t a)
    {
      uint16_t r = a << 1;

      if (r & 8192)
        r ^= 0x201b;

      return r;
    }

    static uint16_t galois_mul(uint16_t a, uint16_t b)
    {
      unsigned int i;
      uint16_t r = 0;

      for (i = 0; i < 13; i++) {
        r = galois_mulx(r);

        if (b & 4096)
          r ^= a;

        b <<= 1;
      }

      return r;
    }

    static uint16_t galois_div(uint16_t a, uint16_t b)
    {
      unsigned int i;
      uint16_t r = 1;

      for (i = 0; i < 12; i++) {
        r = galois_mul(r, r);
        r = galois_mul(r, b);
      }

      r = galois_mul(r, r);
      return galois_mul(a, r);
    }
};
__END_SYS

#endif
