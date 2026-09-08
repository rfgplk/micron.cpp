//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// __multf3 / __divtf3 at the underflow boundary.
//
// The shim documents one deviation from IEEE binary128: a SUBNORMAL result flushes to signed zero.
// A result that rounds UP to 2^-16382 is not a subnormal -- it is the smallest NORMAL -- so it is
// outside that deviation and must be delivered. Every golden value below was produced by exact
// rational arithmetic with an explicit round-to-nearest-even, not by a soft-float implementation.

#include "../../src/math/__gcc_fp128_syms.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::test_case;

#if defined(__SIZEOF_INT128__) && defined(__FLT128_MANT_DIG__)

__micron_diagnostic_push __micron_diagnostic_ignored("-Wpedantic")

    namespace
{

  using rep = unsigned __int128;

  constexpr int mantbits = 112;
  constexpr rep implicit_bit = static_cast<rep>(1) << mantbits;      // also the smallest normal, 2^-16382
  constexpr rep signbit = static_cast<rep>(1) << 127;

  constexpr rep
  bits(u64 hi, u64 lo) noexcept
  {
    return (static_cast<rep>(hi) << 64) | lo;
  }

  struct tcase {
    u64 ahi, alo, bhi, blo;
    char op;
    u64 whi, wlo;
  };

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // exact product/quotient rounds (RNE) to the smallest normal
  constexpr tcase boundary[] = {
    { 0x0001000000000000, 0x0000000000000000, 0x3ffeffffffffffff, 0xffffffffffffffff, '*', 0x0001000000000000,
      0x0000000000000000 },      // exact tie: 2^-16382 * (1 - 2^-113)
    { 0x0001ffffffffffff, 0xffffffffffffffff, 0x4000000000000000, 0x0000000000000000, '/', 0x0001000000000000,
      0x0000000000000000 },      // exact tie: (2 - 2^-112) * 2^-16382 / 2
    { 0x3ffe000000000000, 0x0000000000000000, 0x0001ffffffffffff, 0xffffffffffffffff, '*', 0x0001000000000000,
      0x0000000000000000 },      // exact tie: 0.5 * (2 - 2^-112) * 2^-16382
    { 0x13d8a56c5a561ede, 0x3817b0a84c80ca13, 0x2c273705b0db5e2b, 0x3a7808a0dad34ffe, '*', 0x0001000000000000,
      0x0000000000000000 },      // above the tie: guard = 1, sticky = 1
    { 0x96ee019ec5653870, 0xc35ba691cd0ed0b0, 0x2911fcc7acc7c20b, 0x84d33e264b526565, '*', 0x8001000000000000,
      0x0000000000000000 },      // the same, negative
    { 0x02d413e443520034, 0x3747297aefaabafd, 0x3d2bdb15c5fb282a, 0x0e7fc69f847f4fc1, '*', 0x0001000000000000,
      0x0000000000000000 },      // guard = 0, sticky = 1
    { 0x828601c080134a65, 0xb3f00ac4bd7581d1, 0x3d79fc8518abf513, 0xcef665bf9820a786, '*', 0x8001000000000000,
      0x0000000000000000 },      // the same, negative
    { 0x0007ffffffffffff, 0xffffffffffffffff, 0x4006000000000000, 0x0000000000000000, '/', 0x0001000000000000,
      0x0000000000000000 },
    { 0xa329ffffffffffff, 0xffffffffffffffff, 0x6328000000000000, 0x0000000000000000, '/', 0x8001000000000000,
      0x0000000000000000 },
    { 0x3fffffffffffffff, 0xffffffffffffffff, 0x7ffe000000000000, 0x0000000000000000, '/', 0x0001000000000000,
      0x0000000000000000 },
  };

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // one grid step down: the exact result IS a subnormal, so the documented FTZ owns it
  constexpr tcase flushed[] = {
    { 0x03426f8804e8b6ea, 0x3fb4c0d483a9133e, 0x3cbd64a0cb807aca, 0x1374d6d6f45412a3, '*', 0x0000000000000000,
      0x0000000000000000 },
    { 0x8238b81240a48c59, 0x6ed19b45dd57dac3, 0x3dc729d7b7a2be59, 0xc172fa02c26940f4, '*', 0x8000000000000000,
      0x0000000000000000 },
    { 0x02f3eea0971fc71c, 0xb058a16cd4e2debf, 0x3d0c08fdcebcbab1, 0xbb8ff2db2ab5d935, '*', 0x0000000000000000,
      0x0000000000000000 },
    { 0x892dc62c0f8f9439, 0xc6ceb6cfafa5cd41, 0x36d220986eaa657c, 0xe5b41e5f2a57581f, '*', 0x8000000000000000,
      0x0000000000000000 },
    { 0x000bffffffffffff, 0xfffffffffffffffe, 0x400a000000000000, 0x0000000000000000, '/', 0x0000000000000000,
      0x0000000000000000 },      // one ulp short of the tie
    { 0x1388ffffffffffff, 0xfffffffffffffffe, 0x5387000000000000, 0x0000000000000000, '/', 0x0000000000000000,
      0x0000000000000000 },
    { 0x0001ffffffffffff, 0xffffffffffffffff, 0x4001000000000000, 0x0000000000000000, '/', 0x0000000000000000,
      0x0000000000000000 },      // exp == -1: half the above, nothing can reach the boundary
  };

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // the ordinary path, unchanged
  constexpr tcase normal[] = {
    { 0x3fff000000000000, 0x0000000000000000, 0x4000800000000000, 0x0000000000000000, '*', 0x4000800000000000,
      0x0000000000000000 },      // 1 * 3
    { 0x4000800000000000, 0x0000000000000000, 0x4000000000000000, 0x0000000000000000, '/', 0x3fff800000000000,
      0x0000000000000000 },      // 3 / 2
    { 0x3fff000000000000, 0x0000000000000000, 0x4000a00000000000, 0x0000000000000000, '/', 0x3ffd3b13b13b13b1,
      0x3b13b13b13b13b14 },      // 1 / 3.25
    { 0x4002400000000000, 0x0000000000000000, 0x4002400000000000, 0x0000000000000000, '*', 0x4005900000000000,
      0x0000000000000000 },      // 10 * 10
  };

  // the table is read through a volatile pointer so no operand can be constant-folded away and the
  // shim is actually entered
  rep
  run(const tcase &t) noexcept
  {
    volatile const tcase *v = &t;
    const _Float128 a = __builtin_bit_cast(_Float128, bits(v->ahi, v->alo));
    const _Float128 b = __builtin_bit_cast(_Float128, bits(v->bhi, v->blo));
    return __builtin_bit_cast(rep, v->op == '*' ? __multf3(a, b) : __divtf3(a, b));
  }

  template<usize N>
  bool
  all_match(const tcase (&ts)[N]) noexcept
  {
    for ( usize i = 0; i < N; ++i )
      if ( run(ts[i]) != bits(ts[i].whi, ts[i].wlo) ) return false;
    return true;
  }

  // a / 2 and 2^-16382 * b over significands 2^113-1-k: k == 0 is the tie that rounds to the
  // smallest normal, every k >= 1 lands strictly inside the subnormals
  tcase
  swept(int k, char op) noexcept
  {
    const rep sig = ((implicit_bit << 1) - 1) - static_cast<rep>(k);
    const rep operand = (static_cast<rep>(op == '/' ? 1 : 16382) << mantbits) | (sig & (implicit_bit - 1));
    const rep other = static_cast<rep>(op == '/' ? 16384 : 1) << mantbits;
    const rep want = k == 0 ? implicit_bit : 0;
    tcase t;
    t.ahi = static_cast<u64>((op == '/' ? operand : other) >> 64);
    t.alo = static_cast<u64>(op == '/' ? operand : other);
    t.bhi = static_cast<u64>((op == '/' ? other : operand) >> 64);
    t.blo = static_cast<u64>(op == '/' ? other : operand);
    t.op = op;
    t.whi = static_cast<u64>(want >> 64);
    t.wlo = static_cast<u64>(want);
    return t;
  }

}      // namespace

#endif

int
main()
{
#if defined(__SIZEOF_INT128__) && defined(__FLT128_MANT_DIG__)
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("a product that rounds up to the smallest normal is delivered, not flushed");
  {
    bool ok = true;
    for ( usize i = 0; i < sizeof(boundary) / sizeof(boundary[0]); ++i ) {
      if ( boundary[i].op != '*' ) continue;
      const rep got = run(boundary[i]);
      if ( got != bits(boundary[i].whi, boundary[i].wlo) ) ok = false;
      if ( (got & ~signbit) == 0 ) ok = false;      // the defect: signed zero
    }
    require(ok);
  }
  end_test_case();

  test_case("a quotient that rounds up to the smallest normal is delivered, not flushed");
  {
    bool ok = true;
    for ( usize i = 0; i < sizeof(boundary) / sizeof(boundary[0]); ++i ) {
      if ( boundary[i].op != '/' ) continue;
      const rep got = run(boundary[i]);
      if ( got != bits(boundary[i].whi, boundary[i].wlo) ) ok = false;
      if ( (got & ~signbit) == 0 ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  // the boundary is one grid step wide: this is what an over-eager repair breaks
  test_case("a result that really is subnormal still flushes to signed zero");
  {
    require(all_match(flushed));
  }
  end_test_case();

  test_case("the normal range is untouched");
  {
    require(all_match(normal));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // the sweep carries its own oracle: only the all-ones significand reaches 2^112 under RNE
  test_case("k == 0 rounds to the smallest normal, k in [1,64] flushes -- mul and div");
  {
    bool ok = true;
    for ( int k = 0; k <= 64 && ok; ++k ) {
      const tcase m = swept(k, '*'), d = swept(k, '/');
      if ( run(m) != bits(m.whi, m.wlo) ) ok = false;
      if ( run(d) != bits(d.whi, d.wlo) ) ok = false;
    }
    require(ok);
  }
  end_test_case();
#else
  sb::skip("no binary128 on this target: __SIZEOF_INT128__ / __FLT128_MANT_DIG__ absent");
#endif

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}

#if defined(__SIZEOF_INT128__) && defined(__FLT128_MANT_DIG__)
__micron_diagnostic_pop
#endif
