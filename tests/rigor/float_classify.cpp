//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: float classification under -Ofast (duck's default).
//
// -Ofast implies -ffinite-math-only, which tells gcc NaN and Inf cannot occur. It then folds
// __builtin_isnan / isinf_sign / isfinite / isnormal to constants, and they answer WRONGLY for
// runtime values -- measured on amd64: isnan(NaN) = 0, isinf(Inf) = 0, isfinite(Inf) = 1. It also
// implies -fno-signed-zeros, under which __builtin_signbit may lose the sign of -0.0.
//
// math/generic.hpp was reworked onto raw bits for exactly this; math/ieee.hpp and math/bits.hpp
// were not, so is_nan/is_inf/is_finite/is_normal/sign_bit kept answering wrongly -- and their
// callers (ieee::next_up/next_down/ulp_distance, log_sum_exp, matfunc_schur) kept taking the wrong
// branch. This file is the gate for the whole family.
//
// WARNING: every input here goes through opaque(), a noinline volatile round-trip. A test that
// classifies a LITERAL proves nothing: the compiler folds the call at compile time, where the
// builtins are still correct. The bug only exists for values it cannot see.

#include "../../src/math/bits.hpp"
#include "../../src/math/generic.hpp"
#include "../../src/math/ieee.hpp"
#include "../../src/math/log.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{

[[gnu::noinline]] f64
opaque(u64 b) noexcept
{
  volatile u64 v = b;
  return __builtin_bit_cast(f64, static_cast<u64>(v));
}

[[gnu::noinline]] f32
opaque32(u32 b) noexcept
{
  volatile u32 v = b;
  return __builtin_bit_cast(f32, static_cast<u32>(v));
}

// the oracle: the definition of IEEE-754 classification, on bits, in this file, with nothing the
// optimizer is allowed to assume about it
constexpr u64 e64 = 0x7ff0000000000000ull;
constexpr u64 m64 = 0x000fffffffffffffull;
constexpr u64 s64 = 0x8000000000000000ull;
constexpr u32 e32 = 0x7f800000u;
constexpr u32 m32 = 0x007fffffu;
constexpr u32 s32 = 0x80000000u;

bool
o_nan(u64 b) noexcept
{
  return (b & e64) == e64 && (b & m64) != 0;
}
bool
o_inf(u64 b) noexcept
{
  return (b & e64) == e64 && (b & m64) == 0;
}
bool
o_fin(u64 b) noexcept
{
  return (b & e64) != e64;
}
bool
o_norm(u64 b) noexcept
{
  return (b & e64) != 0 && (b & e64) != e64;
}
bool
o_sign(u64 b) noexcept
{
  return (b & s64) != 0;
}

bool
o_nan32(u32 b) noexcept
{
  return (b & e32) == e32 && (b & m32) != 0;
}
bool
o_inf32(u32 b) noexcept
{
  return (b & e32) == e32 && (b & m32) == 0;
}
bool
o_fin32(u32 b) noexcept
{
  return (b & e32) != e32;
}
bool
o_norm32(u32 b) noexcept
{
  return (b & e32) != 0 && (b & e32) != e32;
}

// fixed seed, never time-based
u64 g_state = 0xa3f1c9d27e5b4081ull;

u64
next_bits() noexcept
{
  g_state ^= g_state << 13;
  g_state ^= g_state >> 7;
  g_state ^= g_state << 17;
  return g_state;
}

int FAILS = 0;

void
check_f64(u64 b) noexcept
{
  const f64 x = opaque(b);
  if ( micron::math::isnan(x) != (o_nan(b) ? 1 : 0) ) ++FAILS;
  if ( micron::math::isinf(x) != (o_inf(b) ? 1 : 0) ) ++FAILS;
  if ( micron::math::isfinite(x) != (o_fin(b) ? 1 : 0) ) ++FAILS;
  if ( micron::math::isnormal(x) != (o_norm(b) ? 1 : 0) ) ++FAILS;
  if ( (micron::math::signbit(x) != 0) != o_sign(b) ) ++FAILS;

  if ( micron::math::ieee::is_nan(x) != o_nan(b) ) ++FAILS;
  if ( micron::math::ieee::is_inf(x) != o_inf(b) ) ++FAILS;
  if ( micron::math::ieee::is_finite(x) != o_fin(b) ) ++FAILS;
  if ( micron::math::ieee::is_normal(x) != o_norm(b) ) ++FAILS;
  if ( (micron::math::bits::sign_bit(x) != 0) != o_sign(b) ) ++FAILS;

  const int want_sign = o_inf(b) ? (o_sign(b) ? -1 : 1) : 0;
  if ( micron::math::ieee::inf_sign(x) != want_sign ) ++FAILS;
}

void
check_f32(u32 b) noexcept
{
  const f32 x = opaque32(b);
  if ( micron::math::isnan(x) != (o_nan32(b) ? 1 : 0) ) ++FAILS;
  if ( micron::math::isinf(x) != (o_inf32(b) ? 1 : 0) ) ++FAILS;
  if ( micron::math::isfinite(x) != (o_fin32(b) ? 1 : 0) ) ++FAILS;
  if ( micron::math::isnormal(x) != (o_norm32(b) ? 1 : 0) ) ++FAILS;
  if ( (micron::math::signbit(x) != 0) != ((b & s32) != 0) ) ++FAILS;

  if ( micron::math::ieee::is_nan(x) != o_nan32(b) ) ++FAILS;
  if ( micron::math::ieee::is_inf(x) != o_inf32(b) ) ++FAILS;
  if ( micron::math::ieee::is_finite(x) != o_fin32(b) ) ++FAILS;
  if ( micron::math::ieee::is_normal(x) != o_norm32(b) ) ++FAILS;
  if ( (micron::math::bits::sign_bit(x) != 0) != ((b & s32) != 0) ) ++FAILS;
}

constexpr u64 CORNERS64[] = {
  0x0000000000000000ull,      // +0
  0x8000000000000000ull,      // -0
  0x0000000000000001ull,      // smallest subnormal
  0x800fffffffffffffull,      // largest negative subnormal
  0x0010000000000000ull,      // smallest normal
  0x7fefffffffffffffull,      // largest normal
  0x3ff0000000000000ull,      // 1.0
  0xbff0000000000000ull,      // -1.0
  0x7ff0000000000000ull,      // +inf
  0xfff0000000000000ull,      // -inf
  0x7ff8000000000000ull,      // quiet NaN
  0xfff8000000000000ull,      // negative quiet NaN
  0x7ff0000000000001ull,      // signalling NaN
  0x7fffffffffffffffull,      // NaN, all payload
};

constexpr u32 CORNERS32[] = {
  0x00000000u, 0x80000000u, 0x00000001u, 0x807fffffu, 0x00800000u, 0x7f7fffffu, 0x3f800000u,
  0xbf800000u, 0x7f800000u, 0xff800000u, 0x7fc00000u, 0xffc00000u, 0x7f800001u, 0x7fffffffu,
};

}      // namespace

int
main(int, char **)
{
  sb::print("=== FLOAT CLASSIFICATION RIGOR (runtime values, -Ofast safe) ===");
  sb::check_callback([]() { ++FAILS; });

  test_case("every classifier agrees with the bit oracle on the f64 corners");
  {
    for ( u64 b : CORNERS64 ) check_f64(b);
    require(FAILS == 0);
  }
  end_test_case();

  test_case("every classifier agrees with the bit oracle on the f32 corners");
  {
    for ( u32 b : CORNERS32 ) check_f32(b);
    require(FAILS == 0);
  }
  end_test_case();

  test_case("and on 200k seeded random bit patterns, specials over-represented");
  {
    for ( u32 i = 0; i < 200000; ++i ) {
      u64 b = next_bits();
      // steer a quarter of the draws into the exponent-all-ones band, where the folding bites
      if ( (i & 3u) == 0u ) b = (b & ~e64) | e64;
      check_f64(b);
      u32 c = static_cast<u32>(next_bits() >> 17);
      if ( (i & 3u) == 1u ) c = (c & ~e32) | e32;
      check_f32(c);
    }
    require(FAILS == 0);
  }
  end_test_case();

  test_case("the ordered-comparison family stays NaN-aware");
  {
    const f64 nan = opaque(0x7ff8000000000000ull);
    const f64 one = opaque(0x3ff0000000000000ull);
    const f64 two = opaque(0x4000000000000000ull);

    require(micron::math::isunordered(nan, one) != 0);
    require(micron::math::isunordered(one, two) == 0);
    require(micron::math::isgreater(two, one) != 0);
    require(micron::math::isgreater(nan, one) == 0);
    require(micron::math::isless(one, two) != 0);
    require(micron::math::isless(nan, one) == 0);
    require(micron::math::isgreaterequal(one, one) != 0);
    require(micron::math::isgreaterequal(nan, nan) == 0);
    require(micron::math::islessequal(one, one) != 0);
    require(micron::math::islessequal(nan, one) == 0);
    require(micron::math::islessgreater(one, two) != 0);
    require(micron::math::islessgreater(nan, two) == 0);
  }
  end_test_case();

  test_case("ieee helpers that branch on the classifiers still short-circuit correctly");
  {
    const f64 pinf = opaque(0x7ff0000000000000ull);
    const f64 ninf = opaque(0xfff0000000000000ull);
    const f64 nan = opaque(0x7ff8000000000000ull);
    const f64 one = opaque(0x3ff0000000000000ull);

    // next_up(+inf) == +inf, next_down(-inf) == -inf, next_up(NaN) == NaN
    require(micron::math::ieee::is_inf(micron::math::ieee::next_up(pinf)));
    require(micron::math::ieee::is_inf(micron::math::ieee::next_down(ninf)));
    require(micron::math::ieee::is_nan(micron::math::ieee::next_up(nan)));
    require(micron::math::ieee::is_nan(micron::math::ieee::next_down(nan)));

    // and it still steps for finite values
    require(micron::math::ieee::next_up(one) > one);
    require(micron::math::ieee::next_down(one) < one);

    require(micron::math::ieee::ulp_distance(nan, one) == -1);
    require(micron::math::ieee::ulp_distance(one, one) == 0);
    require(micron::math::ieee::ulp_distance(one, micron::math::ieee::next_up(one)) == 1);

    // log_sum_exp's -inf identity is a classifier branch
    require(micron::math::log_sum_exp(ninf, one) == one);
    require(micron::math::log_sum_exp(one, ninf) == one);
  }
  end_test_case();

  require(FAILS == 0);
  sb::print("=== FLOAT CLASSIFICATION PASSED ===");
  return 1;
}
