// regex_truffle.cpp
// Parity test for the truffle SIMD character-class scanner: over many random
// 256-member classes and random byte buffers (including >32-byte buffers that
// exercise the AVX2 / NEON blocks and the high-bit-7 table), the SIMD result
// must equal the scalar charreach reference. Exercises the new
// _mm256_shuffle_epi8 (amd64) / vqtbl1q_u8 (arm64) intrinsics.
//
// snowball convention: exit 1 == success; judge by the banner.

#include "../../src/regex/classscan.hpp"

#include "../snowball/snowball.hpp"

using sb::print;
using sb::require_true;

namespace rgx = micron::rgx;

static unsigned int RNG = 2463534242u;

static unsigned int
xr()
{
  RNG ^= RNG << 13;
  RNG ^= RNG >> 17;
  RNG ^= RNG << 5;
  return RNG;
}

static usize
scalar_first(const char *p, usize n, const rgx::charreach &cls)
{
  for ( usize i = 0; i < n; ++i )
    if ( cls.test((unsigned char)p[i]) ) return i;
  return n;
}

int
main()
{
  print("=== REGEX TRUFFLE (SIMD class scan) PARITY ===");

  char buf[128];
  int mismatches = 0;
  int total = 0;

  for ( int trial = 0; trial < 200000; ++trial ) {
    // random class of a random density
    rgx::charreach cls;
    unsigned dsel = xr() % 4;
    for ( int c = 0; c < 256; ++c ) {
      unsigned r = xr() & 0xff;
      bool set = (dsel == 0)   ? (r < 8)         // sparse
                 : (dsel == 1) ? (r < 64)        // ~25%
                 : (dsel == 2) ? (r < 200)       // dense
                               : (r < 250);      // near-full
      if ( set ) cls.set((unsigned char)c);
    }
    rgx::truffle_masks t = rgx::truffle_build(cls);

    usize n = (usize)(xr() % 80);      // 0..79: spans tails + full SIMD blocks
    for ( usize i = 0; i < n; ++i ) buf[i] = (char)(unsigned char)(xr() & 0xff);

    usize got = rgx::truffle_find_first(buf, n, t);
    usize want = scalar_first(buf, n, cls);
    ++total;
    if ( got != want ) {
      if ( mismatches < 10 ) print("  MISMATCH");
      ++mismatches;
    }
  }

  print("=== truffle parity trials done ===");
  require_true(mismatches == 0);
  print("=== ALL REGEX TRUFFLE TESTS PASSED ===");
  return 1;
}
