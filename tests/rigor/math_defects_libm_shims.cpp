//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// the freestanding libm shims: ceil/floor/trunc, fmod, hypot, fma. the float sibling of
// gcc_int_syms.cpp. every golden below is a hex literal, so the file carries its own oracle

#include "../../src/math/__gcc_math_syms.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::test_case;

namespace
{

// the shims are weak extern "C"; taking the address binds the definition THIS tu carries, and a
// volatile pointer keeps the optimizer from folding the call into its own idea of the builtin
double (*volatile __p_ceil)(double) = &::ceil;
double (*volatile __p_floor)(double) = &::floor;
double (*volatile __p_trunc)(double) = &::trunc;
double (*volatile __p_fmod)(double, double) = &::fmod;
float (*volatile __p_fmodf)(float, float) = &::fmodf;
double (*volatile __p_hypot)(double, double) = &::hypot;
float (*volatile __p_hypotf)(float, float) = &::hypotf;
double (*volatile __p_fma)(double, double, double) = &::fma;
float (*volatile __p_fmaf)(float, float, float) = &::fmaf;

volatile f64 __sink64 = 0.0;
volatile f32 __sink32 = 0.0f;

f64
ld(f64 x) noexcept
{
  __sink64 = x;
  return __sink64;
}

f32
lf(f32 x) noexcept
{
  __sink32 = x;
  return __sink32;
}

bool
same(f64 a, f64 b) noexcept
{
  return __builtin_bit_cast(u64, a) == __builtin_bit_cast(u64, b);
}

bool
samef(f32 a, f32 b) noexcept
{
  return __builtin_bit_cast(u32, a) == __builtin_bit_cast(u32, b);
}

bool
is_nan(f64 x) noexcept
{
  return (__builtin_bit_cast(u64, x) & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL;
}

// xorshift64, fixed seed
u64 __seed = 0x5DEECE66D0F3C1A7ULL;

u64
next() noexcept
{
  __seed ^= __seed << 13;
  __seed ^= __seed >> 7;
  __seed ^= __seed << 17;
  return __seed;
}

// 2^k by repeated doubling: exact, and owes nothing to the code under test
f64
pow2(int k) noexcept
{
  f64 r = 1.0;
  if ( k >= 0 )
    for ( int i = 0; i < k; ++i ) r = ld(r * 2.0);
  else
    for ( int i = 0; i < -k; ++i ) r = ld(r * 0.5);
  return r;
}

// -Ofast links crtfastmath.o, which arms FTZ/DAZ: there are then no subnormals to test
bool
denormals_live() noexcept
{
  return ld(ld(0x1p-1022) * 0.5) != 0.0;
}

// -ffinite-math-only, which -Ofast implies, is a PROMISE that no NaN or infinity reaches this
// code, and clang acts on it: measured, ieee::is_nan answers false there for a real NaN bit
// pattern loaded out of memory, and ieee::qnan_v comes back as +inf. nothing to assert in such a
// build -- and no runtime probe can detect it either, since the probe folds the same way
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0
constexpr bool __nonfinite_testable = false;
#else
constexpr bool __nonfinite_testable = true;
#endif

// -fno-signed-zeros, likewise implied by -Ofast, is a promise that the sign of a zero is not
// observable. measured on aarch64 -Ofast: the literal -0.0 in THIS file loads as +0.0, so the
// expectation dies before the function under test is even called
#if defined(__NO_SIGNED_ZEROS__) || defined(__FAST_MATH__)
constexpr bool __signed_zero_testable = false;
#else
constexpr bool __signed_zero_testable = true;
#endif

constexpr f64 __inf = __builtin_inf();
constexpr f64 __nan = __builtin_nan("");
constexpr f64 __dbl_max = 0x1.fffffffffffffp+1023;

}      // namespace

int
main()
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // every double of magnitude >= 2^52 is already an integer, so all three must hand it back
  // untouched. the i64 round trip these used to take saturates instead, from 2^63 upward
  test_case("ceil/floor/trunc are the identity on every already-integral magnitude");
  {
    const f64 v[] = { 0x1p52, -0x1p52, 0x1p53, -0x1p53,   0x1p63,     -0x1p63, 0x1p64,   -0x1p64, 1e17,
                      -1e17,  1e300,   -1e300, __dbl_max, -__dbl_max, 0x1p100, -0x1p100, 0x1p999, 9007199254740993.0 * 2.0 };
    bool ok = true;
    for ( u32 i = 0; i < sizeof(v) / sizeof(v[0]); ++i ) {
      const f64 x = ld(v[i]);
      if ( !same(__p_ceil(x), x) ) ok = false;
      if ( !same(__p_floor(x), x) ) ok = false;
      if ( !same(__p_trunc(x), x) ) ok = false;
      if ( !same(micron::math::ceil(x), x) ) ok = false;
      if ( !same(micron::math::floor(x), x) ) ok = false;
      if ( !same(micron::math::ftrunc(x), x) ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  test_case("ceil/floor/trunc pass +-inf through and propagate NaN");
  if constexpr ( !__nonfinite_testable ) {
    sb::skip("-ffinite-math-only: this build promises no NaN or infinity operand");
  } else {
    bool ok = true;
    if ( !same(__p_ceil(ld(__inf)), __inf) ) ok = false;
    if ( !same(__p_floor(ld(__inf)), __inf) ) ok = false;
    if ( !same(__p_trunc(ld(__inf)), __inf) ) ok = false;
    if ( !same(__p_ceil(ld(-__inf)), -__inf) ) ok = false;
    if ( !same(__p_floor(ld(-__inf)), -__inf) ) ok = false;
    if ( !same(__p_trunc(ld(-__inf)), -__inf) ) ok = false;
    if ( !is_nan(__p_ceil(ld(__nan))) ) ok = false;
    if ( !is_nan(__p_floor(ld(__nan))) ) ok = false;
    if ( !is_nan(__p_trunc(ld(__nan))) ) ok = false;
    if ( !is_nan(micron::math::ceil(ld(__nan))) ) ok = false;
    if ( !is_nan(micron::math::floor(ld(__nan))) ) ok = false;
    require(ok);
  }
  end_test_case();

  // C99 7.12.9.1: ceil of a negative fraction is -0.0, and the sign is observable
  test_case("ceil(-0.5) is -0.0 and trunc(-0.5) is -0.0, sign bit and all");
  if constexpr ( !__signed_zero_testable ) {
    sb::skip("-fno-signed-zeros: this build promises the sign of a zero is not observable");
  } else {
    bool ok = true;
    if ( !same(__p_ceil(ld(-0.5)), -0.0) ) ok = false;
    if ( !same(__p_trunc(ld(-0.5)), -0.0) ) ok = false;
    if ( !same(__p_ceil(ld(-0.0)), -0.0) ) ok = false;
    if ( !same(__p_floor(ld(-0.0)), -0.0) ) ok = false;
    if ( !same(micron::math::ceil(ld(-0.5)), -0.0) ) ok = false;
    if ( !same(micron::math::ftrunc(ld(-0.5)), -0.0) ) ok = false;
    require(ok);
  }
  end_test_case();

  // integer oracle over a dense grid of sixteenths
  test_case("ceil/floor/trunc match an integer oracle over k/16, k in [-4096, 4096]");
  {
    bool ok = true;
    for ( int k = -4096; k <= 4096 && ok; ++k ) {
      const f64 x = ld(f64(k) / 16.0);
      const int q = k / 16;
      const int r = k % 16;
      const f64 wt = f64(q);
      const f64 wc = f64(r > 0 ? q + 1 : q);
      const f64 wf = f64(r < 0 ? q - 1 : q);
      if ( __p_ceil(x) != wc ) ok = false;
      if ( __p_floor(x) != wf ) ok = false;
      if ( __p_trunc(x) != wt ) ok = false;
      if ( micron::math::ceil(x) != wc ) ok = false;
      if ( micron::math::floor(x) != wf ) ok = false;
      if ( micron::math::ftrunc(x) != wt ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // fmod(x,y) = x - n*y with n = trunc(x/y) computed EXACTLY. rounding x/y to a double loses n
  // the moment the true quotient needs more than 53 bits, which 1e17/3 does
  test_case("fmod is exact where the quotient overflows 53 bits");
  {
    bool ok = true;
    if ( !same(__p_fmod(ld(1e17), ld(3.0)), 0x1p+0) ) ok = false;
    if ( !same(__p_fmod(ld(1.0), ld(0.1)), 0x1.9999999999996p-4) ) ok = false;
    if ( !same(__p_fmod(ld(0.5), ld(1e-300)), 0x1.33550b9c24a66p-998) ) ok = false;
    if ( !same(__p_fmod(ld(1e300), ld(3.0)), 0x0p+0) ) ok = false;
    if ( !same(__p_fmod(ld(123456789.0), ld(1e-5)), 0x1.4f34984e7cfd1p-17) ) ok = false;
    if ( !same(__p_fmod(ld(__dbl_max), ld(3.0)), 0x1p+1) ) ok = false;
    if ( !samef(__p_fmodf(lf(1.0f), lf(0.1f)), 0x1.999996p-4f) ) ok = false;
    require(ok);
  }
  end_test_case();

  // the quotient here runs to 62 bits, and the oracle is integer arithmetic
  test_case("fmod matches an i64 remainder over 20000 exactly-representable integer pairs");
  {
    bool ok = true;
    for ( int i = 0; i < 20000 && ok; ++i ) {
      const u64 m = (next() & ((1ULL << 52) - 1)) + 1;
      const int sh = int(next() % 10);
      const i64 xi = i64(m << sh);
      const i64 yi = i64((next() & ((1ULL << 26) - 1)) + 1);
      const i64 want = xi % yi;
      const f64 x = ld(f64(xi));
      const f64 y = ld(f64(yi));
      if ( __p_fmod(x, y) != f64(want) ) ok = false;
      if ( __p_fmod(-x, y) != -f64(want) ) ok = false;
      if ( __p_fmod(x, -y) != f64(want) ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  // |r| < |y| is a postcondition, not an accuracy target. fmod(0.5, 1e-300) = 0.5 broke it by 300
  // decades
  test_case("fmod honours |r| < |y| and the sign of x over 30000 random pairs");
  {
    bool ok = true;
    for ( int i = 0; i < 30000 && ok; ++i ) {
      const f64 x = __builtin_bit_cast(f64, next());
      const f64 y = __builtin_bit_cast(f64, next());
      const u64 bx = __builtin_bit_cast(u64, x) & 0x7ff0000000000000ULL;
      const u64 by = __builtin_bit_cast(u64, y) & 0x7ff0000000000000ULL;
      // a subnormal y is filtered out here only because -Ofast arms FTZ and this block would then
      // be comparing values the hardware has already flushed; the block below is where it is tested
      if ( bx == 0x7ff0000000000000ULL || by == 0x7ff0000000000000ULL || by == 0 ) continue;
      const f64 r = __p_fmod(ld(x), ld(y));
      if ( is_nan(r) ) ok = false;
      const f64 ar = r < 0.0 ? -r : r;
      const f64 ay = y < 0.0 ? -y : y;
      if ( !(ar < ay) ) ok = false;
      if ( r != 0.0 && ((r < 0.0) != (x < 0.0)) ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  // a subnormal divisor is the one operand mkbits::rem::fmod cannot normalise (its
  // subnormal-normalisation loop at rem.hpp:49-51 is dead code); the shim lifts it out of that
  // range before delegating, so these have to come out exact
  test_case("fmod is exact for subnormal operands");
  if ( !denormals_live() ) {
    sb::skip("FTZ/DAZ is armed in this build, the hardware has no subnormals to hand fmod");
  } else {
    const f64 s = 0x0.0000000000001p-1022;      // 2^-1074, the smallest subnormal
    bool ok = true;
    if ( !same(__p_fmod(ld(1.0), ld(3.0 * s)), s) ) ok = false;
    if ( !same(__p_fmod(ld(3.0 * s), ld(2.0 * s)), s) ) ok = false;
    if ( !same(__p_fmod(ld(0x1p-1022), ld(7.0 * s)), 2.0 * s) ) ok = false;
    if ( !same(__p_fmod(ld(1e300), ld(5.0 * s)), 0.0) ) ok = false;
    if ( !same(__p_fmod(ld(-3.3482466981047688e-308), ld(1.8504240176743228e-308)), -0x0.ac53fa1d3ef2dp-1022) ) ok = false;
    if ( !same(__p_fmod(ld(1.4551276266212226e-308), ld(1.3557423756954236e-308)), 0x0.0b6f3be60e3aep-1022) ) ok = false;
    if ( !samef(__p_fmodf(lf(1.0f), lf(3.0f * 0x1p-149f)), 0x1p-148f) ) ok = false;
    if ( !samef(__p_fmodf(lf(0x1p-126f), lf(7.0f * 0x1p-149f)), 0x1p-147f) ) ok = false;
    require(ok);
  }
  end_test_case();

  test_case("fmod(x, 0) is NaN, fmod(x, +-inf) is x, fmod(+-0, y) keeps its sign");
  {
    bool ok = true;
    if constexpr ( __nonfinite_testable ) {
      if ( !is_nan(__p_fmod(ld(5.0), ld(0.0))) ) ok = false;
      if ( !is_nan(__p_fmod(ld(__inf), ld(3.0))) ) ok = false;
      if ( !same(__p_fmod(ld(3.0), ld(__inf)), 3.0) ) ok = false;
      if ( !same(__p_fmod(ld(-3.0), ld(-__inf)), -3.0) ) ok = false;
    }
    if constexpr ( __signed_zero_testable ) {
      if ( !same(__p_fmod(ld(-0.0), ld(5.0)), -0.0) ) ok = false;
      if ( !same(__p_fmod(ld(-1.0), ld(1.0)), -0.0) ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // x*x overflows above ~1.34e154 and flushes to zero below ~1.5e-162, across a band where the
  // result is an ordinary finite number. 3-4-5 scaled by 2^k is exact in every one of those decades
  test_case("hypot(3*2^k, 4*2^k) == 5*2^k exactly for k in [-1000, 1000]");
  {
    bool ok = true;
    for ( int k = -1000; k <= 1000 && ok; ++k ) {
      const f64 s = pow2(k);
      if ( __p_hypot(ld(3.0 * s), ld(4.0 * s)) != 5.0 * s ) ok = false;
      if ( __p_hypot(ld(4.0 * s), ld(3.0 * s)) != 5.0 * s ) ok = false;
      if ( __p_hypot(ld(-3.0 * s), ld(4.0 * s)) != 5.0 * s ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  test_case("hypot neither overflows nor flushes to zero where the result is finite");
  {
    bool ok = true;
    if ( !same(__p_hypot(ld(1e200), ld(1e200)), 0x1.d8f9811335b57p+664) ) ok = false;
    if ( !same(__p_hypot(ld(1e-200), ld(1e-200)), 0x1.151f68876f41p-664) ) ok = false;
    if ( !same(__p_hypot(ld(1e300), ld(1.0)), 0x1.7e43c8800759cp+996) ) ok = false;
    if ( !same(__p_hypot(ld(1e-200), ld(3e-200)), 0x1.35d5244b69495p-663) ) ok = false;
    if ( !samef(__p_hypotf(lf(1e20f), lf(1e20f)), 0x1.eaa766p+66f) ) ok = false;
    require(ok);
  }
  end_test_case();

  // C99 F.10.4.3: hypot(+-inf, y) is +inf even when y is NaN. x*x + y*y cannot honour that
  test_case("hypot(+-inf, NaN) is +inf");
  if constexpr ( !__nonfinite_testable ) {
    sb::skip("-ffinite-math-only: this build promises no NaN or infinity operand");
  } else {
    bool ok = true;
    if ( !same(__p_hypot(ld(__inf), ld(__nan)), __inf) ) ok = false;
    if ( !same(__p_hypot(ld(__nan), ld(__inf)), __inf) ) ok = false;
    if ( !same(__p_hypot(ld(-__inf), ld(__nan)), __inf) ) ok = false;
    if ( !same(__p_hypot(ld(0.0), ld(0.0)), 0.0) ) ok = false;
    if ( !same(__p_hypot(ld(-5.0), ld(0.0)), 5.0) ) ok = false;
    require(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // fma forms a*b exactly and rounds ONCE. a*b+c rounds the product to the format first, which
  // destroys exactly the low half the function exists to deliver
  test_case("fma keeps the low half of the product: one rounding, not two");
  {
    bool ok = true;
    if ( !same(__p_fma(ld(1.0 + 0x1p-52), ld(1.0 - 0x1p-52), ld(-1.0)), -0x1p-104) ) ok = false;
    if ( !same(__p_fma(ld(1.0 + 0x1p-52), ld(1.0 + 0x1p-52), ld(-(1.0 + 0x1p-51))), 0x1p-104) ) ok = false;
    if ( !same(__p_fma(ld(3.141592653589793), ld(2.718281828459045), ld(-8.539734222673566)), 0x1.679e124a69b6p-52) ) ok = false;
    if ( !same(__p_fma(ld(0x1.fp60), ld(0x1.dp60), ld(-0x1.bp121)), 0x1.18p+117) ) ok = false;
    if ( !samef(__p_fmaf(lf(1.0f + 0x1p-23f), lf(1.0f - 0x1p-23f), lf(-1.0f)), -0x1p-46f) ) ok = false;
    if ( !samef(__p_fmaf(lf(1.0f + 0x1p-23f), lf(1.0f + 0x1p-23f), lf(-(1.0f + 0x1p-22f))), 0x1p-46f) ) ok = false;
    require(ok);
  }
  end_test_case();

  // the error-free transform itself: fma(a, b, -(a*b)) IS the rounding error of a*b. a*b + c
  // rounds the product first, so it answers 0 for every pair below -- the count is the assertion
  test_case("fma(a, b, -(a*b)) recovers the product's rounding error over 20000 random pairs");
  {
    bool ok = true;
    int nonzero = 0;
    for ( int i = 0; i < 20000 && ok; ++i ) {
      // significands in [1,2), exponents kept well inside the band so the product is ordinary
      const f64 a = ld(ld(1.0 + f64(next() >> 12) / 4503599627370496.0) * pow2(int(next() % 101) - 50));
      const f64 b = ld(ld(1.0 + f64(next() >> 12) / 4503599627370496.0) * pow2(int(next() % 101) - 50));
      const f64 p = ld(a * b);
      const f64 e = __p_fma(a, b, ld(-p));
      if ( is_nan(e) ) ok = false;
      const f64 ae = e < 0.0 ? -e : e;
      const f64 ap = p < 0.0 ? -p : p;
      if ( !(ae <= ap * 0x1p-52) ) ok = false;
      // p and e do not overlap: p + e rounds straight back to p
      if ( ld(p + e) != p ) ok = false;
      if ( e != 0.0 ) ++nonzero;
    }
    if ( nonzero < 19000 ) ok = false;
    require(ok);
  }
  end_test_case();

  test_case("fma passes non-finite operands through unchanged");
  if constexpr ( !__nonfinite_testable ) {
    sb::skip("-ffinite-math-only: this build promises no NaN or infinity operand");
  } else {
    bool ok = true;
    if ( !is_nan(__p_fma(ld(__inf), ld(0.0), ld(1.0))) ) ok = false;
    if ( !is_nan(__p_fma(ld(__nan), ld(1.0), ld(1.0))) ) ok = false;
    if ( !same(__p_fma(ld(__inf), ld(2.0), ld(1.0)), __inf) ) ok = false;
    if ( !same(__p_fma(ld(3.0), ld(7.0), ld(-21.0)), 0.0) ) ok = false;
    if ( !same(__p_fma(ld(-0.0), ld(1.0), ld(0.0)), 0.0) ) ok = false;
    require(ok);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
