//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/branchless.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace bl = micron::math::branchless;

namespace
{

// the oracles are the language's own operators at a width that cannot wrap
constexpr i64
omin(i64 a, i64 b) noexcept
{
  return a < b ? a : b;
}

constexpr i64
omax(i64 a, i64 b) noexcept
{
  return a > b ? a : b;
}

constexpr i64
oclamp(i64 x, i64 lo, i64 hi) noexcept
{
  return omin(omax(x, lo), hi);
}

// xorshift64, fixed seed -- never time-based
u64 __rng_state = 0xC0FFEE1234567891ull;

u64
next(void) noexcept
{
  __rng_state ^= __rng_state << 13;
  __rng_state ^= __rng_state >> 7;
  __rng_state ^= __rng_state << 17;
  return __rng_state;
}

constexpr i64 kI32Edge[]
    = { -2147483647ll - 1, -2000000000ll, -1073741825ll, -70000ll, -1ll, 0ll, 1ll, 70000ll, 1073741824ll, 2000000000ll, 2147483647ll };
constexpr i64 kI64Edge[]
    = { -9223372036854775807ll - 1, -5000000000000000000ll, -4611686018427387905ll, -2147483648ll, -1ll, 0ll, 1ll, 2147483647ll,
        4611686018427387904ll,      5000000000000000000ll,  9223372036854775807ll };
constexpr usize kI32EdgeN = sizeof(kI32Edge) / sizeof(kI32Edge[0]);
constexpr usize kI64EdgeN = sizeof(kI64Edge) / sizeof(kI64Edge[0]);

}      // namespace

// ^^^^ these are the six that must survive a constant expression ^^^^
// mod_pow2_32/64 built their mask as (1 << p) - 1, which is an overflow -- and
// therefore a hard error, not a wrap -- at the top of each width
static_assert(bl::min8(-128, 1) == -128);
static_assert(bl::max8(-128, 1) == 1);
static_assert(bl::min16(20000, -20000) == -20000);
static_assert(bl::max16(20000, -20000) == 20000);
static_assert(bl::min32(2000000000, -2000000000) == -2000000000);
static_assert(bl::max32(2000000000, -2000000000) == 2000000000);
static_assert(bl::min64(5000000000000000000ll, -5000000000000000000ll) == -5000000000000000000ll);
static_assert(bl::max64(5000000000000000000ll, -5000000000000000000ll) == 5000000000000000000ll);
static_assert(bl::clamp8(50, -100, 100) == 50);
static_assert(bl::clamp16(20000, -20000, 30000) == 20000);
static_assert(bl::lt8(-128, 1) == 1);
static_assert(bl::gt8(1, -128) == 1);
static_assert(bl::le8(1, -128) == 0);
static_assert(bl::ge8(-128, 1) == 0);
static_assert(bl::lt16(20000, -20000) == 0);
static_assert(bl::lt32(2000000000, -2000000000) == 0);
static_assert(bl::ge32(2000000000, -2000000000) == 1);
static_assert(bl::lt64(5000000000000000000ll, -5000000000000000000ll) == 0);
static_assert(bl::mod7(100) == 2);
static_assert(bl::mod7(1000) == 6);
static_assert(bl::div_const(100u, 1u) == 100u);
static_assert(bl::div_const(2147483649u, 2u) == 1073741824u);
static_assert(bl::mul_const(5, -1) == -5);
static_assert(bl::mod_pow2_8(123, 7) == 123);
static_assert(bl::mod_pow2_16(12345, 15) == 12345);
static_assert(bl::mod_pow2_32(123, 31) == 123);
static_assert(bl::mod_pow2_64(123, 63) == 123);
static_assert(bl::mod_pow2_32(-1, 31) == 2147483647);
static_assert(bl::mod_pow2_64(-1, 63) == 9223372036854775807ll);

int
main(void)
{
  // the i8 domain is 65536 pairs, so the oracle sweep is exhaustive and free.
  // it runs first on purpose: mul_const's own case cannot be reached on a tree
  // that still spins on a negative multiplier
  test_case("min8/max8 over the whole i8 x i8 domain");
  {
    bool ok = true;
    for ( int a = -128; a < 128; ++a ) {
      for ( int b = -128; b < 128; ++b ) {
        const i8 A = i8(a), B = i8(b);
        if ( i64(bl::min8(A, B)) != omin(a, b) ) ok = false;
        if ( i64(bl::max8(A, B)) != omax(a, b) ) ok = false;
      }
    }
    require_true(ok);
  }
  end_test_case();

  test_case("lt8/gt8/le8/ge8 over the whole i8 x i8 domain");
  {
    bool ok = true;
    for ( int a = -128; a < 128; ++a ) {
      for ( int b = -128; b < 128; ++b ) {
        const i8 A = i8(a), B = i8(b);
        if ( bl::lt8(A, B) != i32(a < b) ) ok = false;
        if ( bl::gt8(A, B) != i32(a > b) ) ok = false;
        if ( bl::le8(A, B) != i32(a <= b) ) ok = false;
        if ( bl::ge8(A, B) != i32(a >= b) ) ok = false;
      }
    }
    require_true(ok);
  }
  end_test_case();

  test_case("clamp8 over every (x, lo, hi) with lo <= hi");
  {
    bool ok = true;
    for ( int lo = -128; lo < 128; ++lo ) {
      for ( int hi = lo; hi < 128; ++hi ) {
        for ( int x = -128; x < 128; ++x ) {
          if ( i64(bl::clamp8(i8(x), i8(lo), i8(hi))) != oclamp(x, lo, hi) ) ok = false;
        }
      }
    }
    require_true(ok);
  }
  end_test_case();

  test_case("the i16 family across the difference-wrap boundary");
  {
    bool ok = true;
    for ( int a = -32768; a < 32768; a += 61 ) {
      for ( int b = -32768; b < 32768; b += 67 ) {
        const i16 A = i16(a), B = i16(b);
        if ( i64(bl::min16(A, B)) != omin(a, b) ) ok = false;
        if ( i64(bl::max16(A, B)) != omax(a, b) ) ok = false;
        if ( bl::lt16(A, B) != i32(a < b) ) ok = false;
        if ( bl::ge16(A, B) != i32(a >= b) ) ok = false;
        const i16 lo = i16(omin(a, b)), hi = i16(omax(a, b));
        if ( i64(bl::clamp16(i16(30000), lo, hi)) != oclamp(30000, omin(a, b), omax(a, b)) ) ok = false;
      }
    }
    if ( bl::min16(20000, -20000) != -20000 ) ok = false;
    if ( bl::clamp16(20000, -20000, 30000) != 20000 ) ok = false;
    require_true(ok);
  }
  end_test_case();

  test_case("the i32 family at and around the 32-bit wrap");
  {
    bool ok = true;
    for ( usize i = 0; i < kI32EdgeN; ++i ) {
      for ( usize j = 0; j < kI32EdgeN; ++j ) {
        const i32 a = i32(kI32Edge[i]), b = i32(kI32Edge[j]);
        if ( i64(bl::min32(a, b)) != omin(kI32Edge[i], kI32Edge[j]) ) ok = false;
        if ( i64(bl::max32(a, b)) != omax(kI32Edge[i], kI32Edge[j]) ) ok = false;
        if ( bl::lt32(a, b) != i32(kI32Edge[i] < kI32Edge[j]) ) ok = false;
        if ( bl::le32(a, b) != i32(kI32Edge[i] <= kI32Edge[j]) ) ok = false;
      }
    }
    for ( usize n = 0; n < 400000; ++n ) {
      const i32 a = i32(u32(next())), b = i32(u32(next()));
      if ( i64(bl::min32(a, b)) != omin(i64(a), i64(b)) ) ok = false;
      if ( bl::lt32(a, b) != i32(i64(a) < i64(b)) ) ok = false;
      const i32 x = i32(u32(next()));
      if ( i64(bl::clamp32(x, i32(omin(a, b)), i32(omax(a, b)))) != oclamp(i64(x), omin(a, b), omax(a, b)) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("the i64 family at and around the 64-bit wrap");
  {
    bool ok = true;
    for ( usize i = 0; i < kI64EdgeN; ++i ) {
      for ( usize j = 0; j < kI64EdgeN; ++j ) {
        const i64 a = kI64Edge[i], b = kI64Edge[j];
        if ( bl::min64(a, b) != omin(a, b) ) ok = false;
        if ( bl::max64(a, b) != omax(a, b) ) ok = false;
        if ( bl::lt64(a, b) != i32(a < b) ) ok = false;
        if ( bl::gt64(a, b) != i32(a > b) ) ok = false;
        if ( bl::le64(a, b) != i32(a <= b) ) ok = false;
        if ( bl::ge64(a, b) != i32(a >= b) ) ok = false;
      }
    }
    for ( usize n = 0; n < 400000; ++n ) {
      const i64 a = i64(next()), b = i64(next()), x = i64(next());
      if ( bl::min64(a, b) != omin(a, b) ) ok = false;
      if ( bl::max64(a, b) != omax(a, b) ) ok = false;
      if ( bl::lt64(a, b) != i32(a < b) ) ok = false;
      if ( bl::clamp64(x, omin(a, b), omax(a, b)) != oclamp(x, omin(a, b), omax(a, b)) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("mod3/mod5/mod7 against the hardware remainder");
  {
    bool ok = true;
    for ( u32 a = 0; a < 400000u; ++a ) {
      if ( bl::mod3(a) != a % 3u ) ok = false;
      if ( bl::mod5(a) != a % 5u ) ok = false;
      if ( bl::mod7(a) != a % 7u ) ok = false;
    }
    for ( u32 a = 0xFFF00000u; a != 0u; ++a ) {
      if ( bl::mod3(a) != a % 3u ) ok = false;
      if ( bl::mod5(a) != a % 5u ) ok = false;
      if ( bl::mod7(a) != a % 7u ) ok = false;
    }
    for ( usize n = 0; n < 400000; ++n ) {
      const u32 a = u32(next());
      if ( bl::mod3(a) != a % 3u ) ok = false;
      if ( bl::mod5(a) != a % 5u ) ok = false;
      if ( bl::mod7(a) != a % 7u ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("div_const over the u32 range, including d == 1 and the halfway point");
  {
    bool ok = true;
    const u32 ds[] = { 1u, 2u, 3u, 4u, 5u, 7u, 10u, 100u, 1000u, 65535u, 65536u, 2147483647u, 2147483648u, 4294967295u };
    for ( usize k = 0; k < sizeof(ds) / sizeof(ds[0]); ++k ) {
      const u32 d = ds[k];
      for ( u32 a = 0; a < 20000u; ++a )
        if ( bl::div_const(a, d) != a / d ) ok = false;
      for ( u32 a = 2147483548u; a < 2147483748u; ++a )
        if ( bl::div_const(a, d) != a / d ) ok = false;
      for ( u32 a = 0xFFFF0000u; a != 0u; ++a )
        if ( bl::div_const(a, d) != a / d ) ok = false;
      for ( usize n = 0; n < 20000; ++n ) {
        const u32 a = u32(next());
        if ( bl::div_const(a, d) != a / d ) ok = false;
      }
    }
    for ( usize n = 0; n < 400000; ++n ) {
      const u32 a = u32(next());
      u32 d = u32(next());
      if ( d == 0u ) d = 1u;
      if ( bl::div_const(a, d) != a / d ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("mod_pow2_* masks the low p bits over each width's own domain");
  {
    bool ok = true;
    for ( int p = 0; p < 8; ++p )
      for ( int x = -128; x < 128; ++x )
        if ( bl::mod_pow2_8(i8(x), p) != i8(u8(x) & u8(~(~u32(0) << p))) ) ok = false;
    for ( int p = 0; p < 16; ++p )
      for ( int x = -32768; x < 32768; ++x )
        if ( bl::mod_pow2_16(i16(x), p) != i16(u16(x) & u16(~(~u32(0) << p))) ) ok = false;
    for ( int p = 0; p < 32; ++p )
      for ( usize n = 0; n < 20000; ++n ) {
        const i32 x = i32(u32(next()));
        if ( bl::mod_pow2_32(x, p) != i32(u32(x) & ~(~u32(0) << p)) ) ok = false;
      }
    for ( int p = 0; p < 64; ++p )
      for ( usize n = 0; n < 10000; ++n ) {
        const i64 x = i64(next());
        if ( bl::mod_pow2_64(x, p) != i64(u64(x) & ~(~u64(0) << p)) ) ok = false;
      }
    require_true(ok);
  }
  end_test_case();

  // last: on a tree that still shifts a signed multiplier this loop never
  // returns, so every case above has to have run first
  test_case("mul_const terminates for a negative multiplier and wraps like the hardware");
  {
    bool ok = true;
    if ( bl::mul_const(5, -1) != -5 ) ok = false;
    if ( bl::mul_const(-7, -9) != 63 ) ok = false;
    if ( bl::mul_const(5, 3) != 15 ) ok = false;
    if ( bl::mul_const(3, -2147483647 - 1) != i32(u32(3) * u32(-2147483647 - 1)) ) ok = false;
    if ( bl::mul_const(-2147483647 - 1, -1) != i32(u32(-2147483647 - 1) * u32(-1)) ) ok = false;
    for ( usize n = 0; n < 400000; ++n ) {
      const i32 x = i32(u32(next())), k = i32(u32(next()));
      if ( bl::mul_const(x, k) != i32(u32(x) * u32(k)) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
