//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: the scalar front-ends -- math/{mk,sqrt,constants,cr,dd64,ieee}.hpp.
//
// Five defects, each of which this file fails on if it comes back:
//
//  1. micron::math::{round,ceil,floor,sqrt} were AMBIGUOUS. mk.hpp exported a
//     template<ieee754_floating F> F NAME(F) beside generic.hpp:475/486/500's
//     template<T> requires is_floating_point_v<T> T NAME(T) and sqrt.hpp:107's
//     template<T> requires is_arithmetic_v<T> T sqrt(T). Same T(T) signature, so partial ordering
//     cannot break the tie, and the two constraints are distinct atomic expressions, so neither
//     subsumes. round(double) was ambiguous on ALL FOUR arches; ceil/floor/sqrt of an f64 on
//     amd64, where f64 is _Float64 and no non-template is an exact match. THE CALLS BELOW ARE THE
//     GATE -- this file does not compile at all against a tree carrying that defect.
//
//  2. cr::round_dd_to_f32's round-to-odd stepped AWAY FROM ZERO (`to_bits(s) | 1`) instead of
//     toward the value. Round to odd has to land on the neighbour the residual points at; setting
//     the low bit picks the far one whenever the residual is negative for a positive s, and the
//     f32 then rounds to the wrong side of the boundary. Half of every boundary input misrounded.
//
//  3. dd::two_prod's consteval arm was `(a * b) - p` with `p = a * b` two lines above -- an
//     identity zero -- on any compiler whose __builtin_fma is not a constant expression, i.e.
//     clang. Every constant-evaluated double-double silently degraded to a plain double there.
//
//  4. dd::two_sum / fast_two_sum / two_prod carried no fast-math guard, so -ffast-math (which
//     -Ofast implies, and duck defaults to -Ofast) cancelled the error term to zero and collapsed
//     every dd64 path to a plain double.
//
//  5. pi_t<T>() and default_eps<T>() are declared-only primaries; on amd64+gcc+C++23 f32/f64/f128
//     are _Float32/_Float64/_Float128, distinct types no specialization covered, so the calls
//     compiled and failed at LINK.
//
// WARNING: every classifier and EFT input below goes through opaque(), a noinline volatile
// round-trip. A literal proves nothing -- these defects only exist for values the optimizer
// cannot see.

#include "../../src/math/constants.hpp"
#include "../../src/math/cr.hpp"
#include "../../src/math/dd64.hpp"
#include "../../src/math/ieee.hpp"
#include "../../src/math/mk.hpp"
#include "../../src/math/sqrt.hpp"
#include "../../src/math/trig.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{

[[gnu::noinline]] f64
opaque(f64 x) noexcept
{
  volatile f64 v = x;
  return v;
}

[[gnu::noinline]] f64
opaque_bits(u64 b) noexcept
{
  volatile u64 v = b;
  return __builtin_bit_cast(f64, static_cast<u64>(v));
}

[[gnu::noinline]] u32
bits32(f32 x) noexcept
{
  volatile f32 v = x;
  return __builtin_bit_cast(u32, static_cast<f32>(v));
}

[[gnu::noinline]] u64
bits64(f64 x) noexcept
{
  volatile f64 v = x;
  return __builtin_bit_cast(u64, static_cast<f64>(v));
}

// fixed seed, never time-based
u32 g_state = 0x9e3779b9u;

u32
next_bits() noexcept
{
  g_state ^= g_state << 13;
  g_state ^= g_state >> 17;
  g_state ^= g_state << 5;
  return g_state;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the round-to-odd oracle, which needs no arithmetic at all: hi is EXACTLY the midpoint between
// two adjacent f32, so a residual of either sign names the correctly rounded one outright

int
roundodd_failures() noexcept
{
  int bad = 0;
  for ( int k = 0; k < 60000; ++k ) {
    u32 b = next_bits() & 0x7fffffffu;
    const u32 e = (b >> 23) & 0xffu;
    if ( e == 0 || e >= 0xfdu ) continue;
    if ( next_bits() & 1u ) b |= 0x80000000u;
    const u32 bu = b + 1u;
    const f32 lo32 = __builtin_bit_cast(f32, b);
    const f32 up32 = __builtin_bit_cast(f32, bu);
    const f64 d = f64(lo32);
    const f64 du = f64(up32);
    const f64 mid = d + (du - d) * 0.5;      // exact: du - d is a power of two
    const f64 step = (du > d) ? (du - d) : (d - du);
    const f64 tiny = step * 0x1.0p-40;      // far under half an ulp of mid
    const f32 lower = (lo32 < up32) ? lo32 : up32;
    const f32 upper = (lo32 < up32) ? up32 : lo32;
    if ( bits32(micron::math::cr::round_dd_to_f32(micron::math::dd64{ mid, tiny })) != bits32(upper) ) ++bad;
    if ( bits32(micron::math::cr::round_dd_to_f32(micron::math::dd64{ mid, -tiny })) != bits32(lower) ) ++bad;
  }
  return bad;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the 256-bit vsqrt overload set tracks AVX, not merely x86

#if defined(__micron_arch_x86_any) && !defined(__micron_simd_generic)
// the 256-bit vsqrt block was gated on bare __micron_arch_x86_any, so below --isa v3 it was
// DECLARED, could not be called (target specific option mismatch at codegen) and emitted
// -Wpsabi in every TU that merely included math/sqrt.hpp. the box is what makes the requirement
// dependent -- a non-dependent one is a hard error rather than an unsatisfied constraint
struct v256_box {
  micron::simd::d256 d;
  micron::simd::f256 f;
};

template<typename B>
concept has_vsqrt_256 = requires(B b) {
  micron::math::vsqrt(b.d);
  micron::math::vsqrt(b.f);
};

#if defined(__micron_x86_avx)
static_assert(has_vsqrt_256<v256_box>, "the 256-bit vsqrt forms must exist where AVX does");
#else
static_assert(!has_vsqrt_256<v256_box>, "the 256-bit vsqrt forms must not exist below AVX");
#endif
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// constant-evaluated double-double, against values baked as hex floats

constexpr micron::math::dd64 k_pi_e = micron::math::dd::two_prod(0x1.921fb54442d18p+1, 0x1.5bf0a8b145769p+1);
constexpr micron::math::dd64 k_eps2 = micron::math::dd::two_prod(1.0 + 0x1.0p-52, 1.0 + 0x1.0p-52);
constexpr micron::math::dd64 k_tenth = micron::math::dd::two_prod(0x1.999999999999ap-4, 0x1.999999999999ap-4);

};      // namespace

int
main()
{
  sb::print("=== MATH FRONT-END DEFECT RIGOR ===");

  test_case("micron::math::{round,ceil,floor,sqrt} resolve to exactly one overload");
  {
    // the compile gate. every one of these was ambiguous, and f64/f32 are _Float64/_Float32 on
    // amd64+gcc, which is where the ceil/floor/sqrt half of it shows
    require(micron::math::round(opaque(2.5)) == 3.0);
    require(micron::math::round(opaque(-2.5)) == -3.0);
    require(micron::math::ceil(opaque(1.2)) == 2.0);
    require(micron::math::floor(opaque(1.8)) == 1.0);
    require(micron::math::trunc(opaque(-1.7)) == -1.0);
    require(micron::math::sqrt(opaque(2.0)) == 0x1.6a09e667f3bcdp+0);

    require(f64(micron::math::round(f64(opaque(2.5)))) == 3.0);
    require(f64(micron::math::ceil(f64(opaque(1.2)))) == 2.0);
    require(f64(micron::math::floor(f64(opaque(1.8)))) == 1.0);
    require(f64(micron::math::sqrt(f64(opaque(2.0)))) == 0x1.6a09e667f3bcdp+0);

    require(micron::math::round(2.5f) == 3.0f);
    require(f32(micron::math::sqrt(f32(2.0f))) == 0x1.6a09e6p+0f);
    require(micron::math::round(2.5L) == 3.0L);
    require(micron::math::sqrt(9) == 3);

    // and the constant-evaluated forms still fold. NOTE: sqrt is deliberately absent -- its
    // consteval path is hw::__constexpr_sqrt on clang and __builtin_sqrt on gcc, and the two
    // disagree by 1 ulp, which belongs to math/__asm/hw.hpp and not here
    constexpr f64 cr = micron::math::round(1.5);
    constexpr f64 cc = micron::math::ceil(1.2);
    constexpr f64 cf = micron::math::floor(1.8);
    static_assert(cr == 2.0 && cc == 2.0 && cf == 1.0);
  }
  end_test_case();

  test_case("cr::round_dd_to_f32 rounds to odd TOWARD the residual, not away from zero");
  {
    // the case the defect fails: hi is the midpoint between 1.0f and its successor, the residual
    // is negative, so the correctly rounded f32 is 1.0f. `to_bits(s) | 1` answered 1.00000012f
    require(bits32(micron::math::cr::round_dd_to_f32(micron::math::dd64{ 1.0 + 0x1.0p-24, -0x1.0p-60 })) == 0x3f800000u);
    require(bits32(micron::math::cr::round_dd_to_f32(micron::math::dd64{ 1.0 + 0x1.0p-24, 0x1.0p-60 })) == 0x3f800001u);
    // and the mirror, where the wrong direction is a POSITIVE residual
    require(bits32(micron::math::cr::round_dd_to_f32(micron::math::dd64{ -(1.0 + 0x1.0p-24), 0x1.0p-60 })) == 0xbf800000u);
    require(bits32(micron::math::cr::round_dd_to_f32(micron::math::dd64{ -(1.0 + 0x1.0p-24), -0x1.0p-60 })) == 0xbf800001u);
    // an exact pair must not be perturbed at all
    require(bits32(micron::math::cr::round_dd_to_f32(micron::math::dd64{ 1.0, 0.0 })) == 0x3f800000u);

    require(roundodd_failures() == 0);
  }
  end_test_case();

  test_case("dd:: error-free transformations stay exact -- consteval and under -ffast-math");
  {
    require(k_pi_e.hi == 0x1.114580b45d474p+3);
    require(k_pi_e.lo == 0x1.679e124a69b6p-52);
    require(k_eps2.hi == 0x1.0000000000002p+0);
    require(k_eps2.lo == 0x1.0p-104);
    require(k_tenth.hi == 0x1.47ae147ae147cp-7);
    require(k_tenth.lo == -0x1.eb851eb851eb8p-61);

    // the same products at runtime must agree bit for bit with the constant-evaluated pairs
    const micron::math::dd64 r_pi_e = micron::math::dd::two_prod(opaque(0x1.921fb54442d18p+1), opaque(0x1.5bf0a8b145769p+1));
    const micron::math::dd64 r_eps2 = micron::math::dd::two_prod(opaque(1.0 + 0x1.0p-52), opaque(1.0 + 0x1.0p-52));
    require(r_pi_e.hi == k_pi_e.hi && r_pi_e.lo == k_pi_e.lo);
    require(r_eps2.hi == k_eps2.hi && r_eps2.lo == k_eps2.lo);

    // -ffast-math is licensed to cancel these to zero; the barrier is what stops it
    const micron::math::dd64 s = micron::math::dd::two_sum(opaque(1.0), opaque(0x1.79ca10c924223p-67));
    const micron::math::dd64 fs = micron::math::dd::fast_two_sum(opaque(1.0), opaque(0x1.79ca10c924223p-67));
    require(s.hi == 1.0 && s.lo == 0x1.79ca10c924223p-67);
    require(fs.hi == 1.0 && fs.lo == 0x1.79ca10c924223p-67);
  }
  end_test_case();

  test_case("ieee:: classification survives -ffinite-math-only");
  {
    const f64 nan = opaque_bits(0x7ff8000000000000ull);
    const f64 pinf = opaque_bits(0x7ff0000000000000ull);
    const f64 ninf = opaque_bits(0xfff0000000000000ull);
    const f64 one = opaque_bits(0x3ff0000000000000ull);

    require(micron::math::ieee::is_nan(nan));
    require(!micron::math::ieee::is_nan(one));
    require(micron::math::ieee::is_inf(pinf) && micron::math::ieee::is_inf(ninf));
    require(!micron::math::ieee::is_inf(one));
    require(!micron::math::ieee::is_finite(pinf) && !micron::math::ieee::is_finite(nan));
    require(micron::math::ieee::is_finite(one));
    require(!micron::math::ieee::is_normal(pinf) && micron::math::ieee::is_normal(one));
    require(micron::math::ieee::is_special(pinf) && micron::math::ieee::is_special(nan));
    require(micron::math::ieee::inf_sign(pinf) == 1 && micron::math::ieee::inf_sign(ninf) == -1);
    require(micron::math::ieee::inf_sign(one) == 0);
    require(micron::math::ieee::ulp_distance(nan, one) == -1);

    // qnan_v/inf_v were NEVER folded -- this pins that, so nobody "fixes" what was already right.
    // they go through bits64 for the reason in the banner: clang folds a COMPILE-TIME inf away
    // under -ffinite-math-only whatever the producer does, and that is the flag, not the header
    require(bits64(micron::math::ieee::qnan_v<f64>()) == 0x7ff8000000000001ull);
    require(bits64(micron::math::ieee::inf_v<f64>(0)) == 0x7ff0000000000000ull);

    // and the guard the whole family exists to serve
    require(micron::math::ieee::is_nan(micron::math::mk::pow_ns::hypot<f64>(f64(opaque(3.0)), f64(nan))));
    require(micron::math::ieee::is_inf(micron::math::mk::pow_ns::hypot<f64>(f64(opaque(3.0)), f64(pinf))));
  }
  end_test_case();

  test_case("pi_t / default_eps link for every f32/f64/f128 spelling");
  {
    require(f64(micron::math::pi_t<f32>()) == f64(0x1.921fb6p+1f));
    require(f64(micron::math::pi_t<f64>()) == 0x1.921fb54442d18p+1);
    require(f64(micron::math::pi_t<f128>()) == 0x1.921fb54442d18p+1);
    require(micron::math::pi_t<float>() == 0x1.921fb6p+1f);
    require(micron::math::pi_t<double>() == 0x1.921fb54442d18p+1);

    require(f64(micron::math::default_eps<f32>()) > 0.0);
    require(f64(micron::math::default_eps<f64>()) > 0.0);
    require(f64(micron::math::default_eps<f128>()) > 0.0);
    require(f64(micron::math::default_eps<f64>()) < f64(micron::math::default_eps<f32>()));
  }
  end_test_case();

  sb::print("=== MATH FRONT-END DEFECTS PASSED ===");
  return 1;
}
