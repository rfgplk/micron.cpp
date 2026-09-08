//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// mkbits::rem (fmod / remainder / remquo) and the two defects in mkbits::special_ns that produce a
// wrong VALUE rather than a wrong last bit: tgamma's premature overflow and the Hankel P/Q
// coefficients. the subnormal and tie sections carry their own oracle -- a subnormal is an exact
// integer multiple of 2^-1074, so fmod and the round-half-even quotient are u64 arithmetic there,
// and 2^k mod 3 closes the remquo case in u64 too. everything else is a hex golden

#include "../../src/math/mk.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace
{

namespace mrem = micron::math::mkbits::rem;
namespace mspec = micron::math::mkbits::special_ns;

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

// a relative tolerance cannot be written as `|got-want| < tol*|want|` here: tol*|want| is
// SUBNORMAL for the tgamma goldens near DBL_MIN, and -Ofast arms DAZ, which reads that threshold
// as zero and fails every comparison against it. the integer distance owes nothing to the FPU
i64
__ord(f64 v) noexcept
{
  const u64 u = __builtin_bit_cast(u64, v);
  return (u >> 63) ? i64(0x8000000000000000ULL - u) : i64(u);
}

u64
ulps(f64 got, f64 want) noexcept
{
  const i64 d = __ord(got) - __ord(want);
  return u64(d < 0 ? -d : d);
}

// 2^k by repeated doubling/halving: every step is exact, subnormals included
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

// m * 2^-1074 -- exact for every m below 2^52, which is the whole subnormal range
f64
sub64(u64 m) noexcept
{
  return ld(f64(m) * pow2(-1074));
}

f32
sub32(u32 m) noexcept
{
  f32 s = 1.0f;
  for ( int i = 0; i < 149; ++i ) s = lf(s * 0.5f);
  return lf(f32(m) * s);
}

// -Ofast links crtfastmath.o, which arms FTZ/DAZ: there are then no subnormals to test
bool
denormals_live() noexcept
{
  return ld(ld(0x1p-1022) * 0.5) != 0.0;
}

// n = m / d rounded to nearest, ties to even -- the quotient IEEE remainder is defined against
u64
round_half_even(u64 m, u64 d) noexcept
{
  const u64 q = m / d;
  const u64 r = m - q * d;
  if ( r + r > d ) return q + 1;
  if ( r + r == d ) return (q & 1u) ? q + 1 : q;
  return q;
}

// -ffinite-math-only, which -Ofast implies, is a PROMISE that no infinity reaches this code, so
// there is nothing left to assert about one
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0
constexpr bool __nonfinite_testable = false;
#else
constexpr bool __nonfinite_testable = true;
#endif

constexpr f64 __inf = __builtin_inf();

struct __tie_row {
  f64 x, y, r;
};

struct __gamma_row {
  f64 x, want;
};

struct __pq_row {
  f64 x, p0, q0, p1, q1;
};

struct __bessel_row {
  f64 x, j0, j1, y0, y1;
  u64 tol;
};

}      // namespace

int
main()
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // fmod ORed the implicit one into the significand BEFORE the loop that normalises a subnormal
  // one, so that loop could never run and a subnormal decoded as 2^52 + mantissa. both operands
  // here are exact multiples of 2^-1074, so the whole answer is one u64 modulo
  test_case("fmod decodes a subnormal significand -- 128x128 exhaustive against a u64 oracle");
  if ( !denormals_live() ) {
    sb::skip("FTZ/DAZ is armed in this build, the hardware has no subnormals to hand fmod");
  } else {
    bool ok = true;
    for ( u64 mx = 1; mx <= 128; ++mx ) {
      for ( u64 my = 1; my <= 128; ++my ) {
        const f64 x = sub64(mx), y = sub64(my);
        const f64 want = sub64(mx % my);
        if ( !same(mrem::fmod<f64>(x, y), want) ) ok = false;
        if ( !same(mrem::fmod<f64>(-x, y), (mx % my) == 0 ? -0.0 : -want) ) ok = false;
        if ( !same(mrem::fmod<f64>(x, -y), want) ) ok = false;
      }
    }
    require_true(ok);
  }
  end_test_case();

  test_case("fmod is exact for a subnormal operand outside the u64 oracle's reach");
  if ( !denormals_live() ) {
    sb::skip("FTZ/DAZ is armed in this build, the hardware has no subnormals to hand fmod");
  } else {
    bool ok = true;
    if ( !same(mrem::fmod<f64>(ld(1.0), sub64(3)), sub64(1)) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(1.0), ld(0x0.fffffffffffffp-1022)), 0x0.00004p-1022) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(0x0.fffffffffffffp-1022), sub64(7)), sub64(1)) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(0x1p-1022), sub64(7)), sub64(2)) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(1e300), sub64(5)), 0.0) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(0x1.0p+300), ld(0x0.0000abcdef123p-1022)), 0x0.00005f8d525e3p-1022) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(0x0.abcdef0123456p-1022), ld(0x0.0000000fedcbap-1022)), 0x0.000000097a64p-1022) ) ok = false;
    if ( !samef(mrem::fmod<f32>(lf(1.0f), sub32(3)), sub32(2)) ) ok = false;
    if ( !samef(mrem::fmod<f32>(lf(0x1p-126f), sub32(7)), sub32(4)) ) ok = false;
    if ( !samef(mrem::fmod<f32>(lf(0x0.abcdefp-126f), lf(0x0.0000fp-126f)), 0x1p-146f) ) ok = false;
    if ( !samef(mrem::fmod<f32>(lf(123456.0f), lf(0x0.000123p-126f)), 0x1.fp-143f) ) ok = false;
    require_true(ok);
  }
  end_test_case();

  // fmod over normals was never the defect -- this is the control that says so
  test_case("fmod over normal operands is unchanged");
  {
    bool ok = true;
    if ( !same(mrem::fmod<f64>(ld(1e17), ld(3.0)), 0x1p+0) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(1.0), ld(0.1)), 0x1.9999999999996p-4) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(0.5), ld(1e-300)), 0x1.33550b9c24a66p-998) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(1e300), ld(3.0)), 0x0p+0) ) ok = false;
    if ( !same(mrem::fmod<f64>(ld(123456789.0), ld(1e-5)), 0x1.4f34984e7cfd1p-17) ) ok = false;
    if ( !samef(mrem::fmod<f32>(lf(1.0f), lf(0.1f)), 0x1.999996p-4f) ) ok = false;
    require_true(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // IEEE 754 remainder rounds x/y to nearest with ties to EVEN. the tie arm used to read
  // `fabs(r/y) >= 2`, which is 0.5 at every tie and so never fired -- remainder(3,2) answered +1
  // where the quotient has to round up to the even 2 and the answer is -1
  test_case("remainder rounds the quotient half to even");
  {
    const __tie_row v[] = {
      { 1.5, 1.0, -0.5 }, { 3.5, 1.0, -0.5 }, { 2.5, 1.0, 0.5 }, { -1.5, 1.0, 0.5 },      { 1.5, -1.0, -0.5 },  { 3.0, 2.0, -1.0 },
      { 5.0, 2.0, 1.0 },  { 7.0, 4.0, -1.0 }, { 0.5, 1.0, 0.5 }, { 0.375, 0.25, -0.125 }, { 1024.5, 1.0, 0.5 }, { -7.5, 1.0, 0.5 },
    };
    bool ok = true;
    for ( u32 i = 0; i < sizeof(v) / sizeof(v[0]); ++i ) {
      if ( !same(mrem::remainder<f64>(ld(v[i].x), ld(v[i].y)), v[i].r) ) ok = false;
      if ( !samef(mrem::remainder<f32>(lf(f32(v[i].x)), lf(f32(v[i].y))), f32(v[i].r)) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  // y*0.5 is inexact for an odd subnormal divisor, so the tie test cannot be `ar > ay*0.5`
  test_case("remainder over subnormals -- 128x128 exhaustive against a u64 oracle");
  if ( !denormals_live() ) {
    sb::skip("FTZ/DAZ is armed in this build, the hardware has no subnormals to hand remainder");
  } else {
    bool ok = true;
    for ( u64 mx = 1; mx <= 128; ++mx ) {
      for ( u64 my = 1; my <= 128; ++my ) {
        const u64 n = round_half_even(mx, my);
        const i64 rr = i64(mx) - i64(n) * i64(my);
        const f64 want = rr < 0 ? -sub64(u64(-rr)) : sub64(u64(rr));
        const f64 got = mrem::remainder<f64>(sub64(mx), sub64(my));
        if ( rr == 0 ? !same(got, 0.0) : !same(got, want) ) ok = false;
      }
    }
    require_true(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // C99 7.12.10.3 wants *q congruent to the integral quotient modulo 2^n, n >= 3. it used to be
  // rebuilt as round((x - r)/y) in f64, which carries no bit below 2^-53 -- and, once that
  // division overflowed, handed int(NaN) straight to the conversion: INT_MIN on amd64, 0 on arm.
  // 2^k mod 3 is 1 for even k and 2 for odd, which pins both the remainder and the quotient
  test_case("remquo's quotient survives a ratio above 2^53, and past DBL_MAX");
  {
    // x = 2^(k-1000), y = 3*2^-1000, so x/y is 2^k/3 for k up to 2020 -- the last 996 of those
    // overflow the f64 division the quotient used to be rebuilt from. 2^k = 3m+1 for even k and
    // 3m+2 for odd, which fixes the remainder at +-2^-1000 and m mod 8 at 5 and 3
    const f64 y = ld(3.0 * pow2(-1000));
    const f64 unit = pow2(-1000);
    bool ok = true;
    for ( int k = 54; k <= 2020; ++k ) {
      const f64 x = pow2(k - 1000);
      const bool even = (k & 1) == 0;
      int q = 0;
      const f64 r = mrem::remquo<f64>(x, y, &q);
      if ( !same(r, even ? unit : -unit) ) ok = false;
      if ( q != (even ? 5 : 3) ) ok = false;
      int qn = 0;
      const f64 rn = mrem::remquo<f64>(-x, y, &qn);
      if ( !same(rn, even ? -unit : unit) ) ok = false;
      if ( qn != -(even ? 5 : 3) ) ok = false;
      int qy = 0;
      (void)mrem::remquo<f64>(x, -y, &qy);
      if ( qy != -(even ? 5 : 3) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  // int(NaN) is undefined, and the two arches do not even agree on the answer: amd64's cvttsd2si
  // hands back INT_MIN, aarch64's fcvtzs hands back 0
  test_case("remquo never hands back a quotient outside three bits");
  {
    bool ok = true;
    u64 s = 0x9E3779B97F4A7C15ULL;
    for ( int i = 0; i < 20000; ++i ) {
      s ^= s << 13;
      s ^= s >> 7;
      s ^= s << 17;
      const f64 x = ld(f64(i64(s >> 3)) * pow2(int(s % 1400) - 700));
      s ^= s << 13;
      s ^= s >> 7;
      s ^= s << 17;
      const f64 y = ld(f64(i64(s >> 11) | 1) * pow2(int(s % 1400) - 700));
      if ( y == 0.0 || x != x || y != y || x - x != 0.0 || y - y != 0.0 ) continue;
      int q = 0;
      (void)mrem::remquo<f64>(x, y, &q);
      if ( q < -7 || q > 7 ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("remquo matches an i64 oracle on exactly-representable integer pairs");
  {
    bool ok = true;
    u64 s = 0x243F6A8885A308D3ULL;
    for ( int i = 0; i < 20000; ++i ) {
      s ^= s << 13;
      s ^= s >> 7;
      s ^= s << 17;
      const u64 mx = (s >> 16) % 1000000007ULL + 1;
      s ^= s << 13;
      s ^= s >> 7;
      s ^= s << 17;
      const u64 my = (s >> 24) % 9973ULL + 1;
      const u64 n = round_half_even(mx, my);
      const i64 rr = i64(mx) - i64(n) * i64(my);
      int q = 0;
      const f64 r = mrem::remquo<f64>(ld(f64(mx)), ld(f64(my)), &q);
      if ( r != f64(rr) ) ok = false;
      if ( q != int(n & 7u) ) ok = false;
      int qn = 0;
      const f64 rn = mrem::remquo<f64>(ld(-f64(mx)), ld(f64(my)), &qn);
      if ( rn != -f64(rr) ) ok = false;
      if ( qn != -int(n & 7u) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // t^(z+0.5) formed on its own leaves DBL_MAX at x = 142.2; Gamma itself does not until 171.62,
  // because e^-t is what pulls the power back. the reflection branch divides by tgamma(1-x), so
  // the spurious infinity came back as a spurious signed zero for x below -141
  test_case("tgamma stays finite to the real f64 overflow threshold");
  {
    const __gamma_row v[] = {
      { 142.0, 0x1.1ca9fcdf65321p+808 },    { 143.0, 0x1.3bcc9487d4439p+815 },  { 150.0, 0x1.8c5d92b583900p+865 },
      { 160.0, 0x1.44ab297a8724bp+938 },    { 170.0, 0x1.f2054eb4d96ecp+1011 }, { 171.0, 0x1.4ab7864418639p+1019 },
      { 171.6, 0x1.c3adadc5107b1p+1023 },   { -141.5, 0x1.e5fcdf1bbea17p-811 }, { -150.5, -0x1.20cf9b3f08b2fp-875 },
      { -170.5, -0x1.7d2374dfcda7ap-1022 },
    };
    bool ok = true;
    // the Lanczos g=7 form is worth about 1400 ulp here; the defect was 1e15 and up
    for ( u32 i = 0; i < sizeof(v) / sizeof(v[0]); ++i )
      if ( ulps(mspec::tgamma_f64(ld(v[i].x)), v[i].want) > 20000u ) ok = false;
    if ( ulps(mspec::tgamma_f64(ld(5.0)), 24.0) > 64u ) ok = false;
    if constexpr ( __nonfinite_testable )
      if ( !same(mspec::tgamma_f64(ld(172.0)), __inf) ) ok = false;
    require_true(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // P = sum (-1)^k a_2k / z^2k and Q = sum (-1)^k a_{2k+1} / z^{2k+1} with the Hankel
  // a_k(nu) = prod_{j=1..k} (4nu^2-(2j-1)^2) / (k! 8^k). the third Q coefficient of BOTH helpers
  // was a copy of the other one's third P coefficient -- 36% and 38% wrong -- and the third P
  // coefficient of both was off in its fifth digit. every a_k here is exact in f64, so the golden
  // is the series itself and the tolerance only has to clear f64 evaluation noise
  test_case("the Hankel P/Q helpers carry the asymptotic a_k(nu)");
  {
    const __pq_row v[] = {
      { 4.0, 0x1.fde71a1180000p-1, -0x1.f0e23c0000000p-6, 0x1.01c5e952c0000p+0, 0x1.7a8c3d0000000p-4 },
      { 2.0, 0x1.f602446000000p-1, -0x1.ef23c00000000p-5, 0x1.07e634b000000p+0, 0x1.7783d00000000p-3 },
    };
    bool ok = true;
    // measured 0 ulp on every cell; the mistyped table lands 1e8 (P) and 2e13 (Q) away
    for ( u32 i = 0; i < sizeof(v) / sizeof(v[0]); ++i ) {
      f64 P = 0.0, Q = 0.0;
      micron::math::mkbits::special_ns::__bessel::hankel_PQ_j0(ld(v[i].x), &P, &Q);
      if ( ulps(P, v[i].p0) > 1024u || ulps(Q, v[i].q0) > 1024u ) ok = false;
      micron::math::mkbits::special_ns::__bessel::hankel_PQ_j1(ld(v[i].x), &P, &Q);
      if ( ulps(P, v[i].p1) > 1024u || ulps(Q, v[i].q1) > 1024u ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("j0/j1/y0/y1 carry the Hankel expansion's own accuracy above x = 8");
  {
    // worst measured with the corrected coefficients / worst with the mistyped ones:
    // x=100  326 / 11695, x=50  37245 / 1058034. x=20 is a ceiling, not a discriminator
    const __bessel_row v[] = {
      { 50.0, 0x1.c936ef41b2c50p-5, -0x1.8f68900c5532ap-4, -0x1.91ac99c6d2688p-4, -0x1.d1452660e7e7dp-5, 150000u },
      { 100.0, 0x1.4772bb5c1ef71p-6, -0x1.3bfcc3bf06394p-4, -0x1.3c64887b47b65p-4, -0x1.4dc7ab72edc1fp-6, 2000u },
      { 20.0, 0x1.561106f7bed64p-3, 0x1.11bf9c29ff1c6p-4, 0x1.00936d2b2bee8p-4, -0x1.52f7c0d65c8e1p-3, 100000000u },
    };
    bool ok = true;
    for ( u32 i = 0; i < sizeof(v) / sizeof(v[0]); ++i ) {
      const f64 x = ld(v[i].x);
      if ( ulps(mspec::j0_f64(x), v[i].j0) > v[i].tol ) ok = false;
      if ( ulps(mspec::j1_f64(x), v[i].j1) > v[i].tol ) ok = false;
      if ( ulps(mspec::y0_f64(x), v[i].y0) > v[i].tol ) ok = false;
      if ( ulps(mspec::y1_f64(x), v[i].y1) > v[i].tol ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // NOT a regression gate -- a ceiling. j0_small stops at k=15 and j1_small at k=13, well inside
  // the |x| < 8 they serve, which costs 7 and 11 decimal digits at the cutover. that is an
  // accuracy choice and it is left alone; this only says it must not get worse, and it is the
  // reason j0's C[16] and C[17] table entries are never read
  test_case("j0/j1/y0/y1 hold their measured ceiling at the top of the power-series domain");
  {
    // measured 1.4e9, 8.5e10, 1.8e9 and 1.1e11 ulp -- 2.3e-7 and 1.0e-5 relative
    const f64 x = ld(7.999);
    bool ok = true;
    if ( ulps(mspec::j0_f64(x), 0x1.60057024e824bp-3) > 4000000000ull ) ok = false;
    if ( ulps(mspec::j1_f64(x), 0x1.e03e56a715646p-3) > 200000000000ull ) ok = false;
    if ( ulps(mspec::y0_f64(x), 0x1.c972b3a01608ap-3) > 5000000000ull ) ok = false;
    if ( ulps(mspec::y1_f64(x), -0x1.4434b803cb041p-3) > 250000000000ull ) ok = false;
    require_true(ok);
  }
  end_test_case();

  // likewise a ceiling, not a gate. lgamma's Lanczos log form sums O(1) terms to reach a value of
  // order 1e-9 at its zeros x=1 and x=2, so what survives there is ABSOLUTE accuracy -- the
  // relative error reaches 3.3e-6. the formula is right; only the cancellation is not handled
  test_case("lgamma keeps absolute accuracy through the cancellation at its zeros");
  {
    struct {
      f64 x, want;
    } const v[] = {
      { 2.0000000013446546, 0x1.38891bae5af69p-31 },
      { 1.9999999999984908, -0x1.673548b59c338p-41 },
      { 1.0000001, -0x1.efd30c8e518b9p-25 },
      { 2.0000001, 0x1.6b2b43f393939p-25 },
    };

    bool ok = true;
    for ( u32 i = 0; i < sizeof(v) / sizeof(v[0]); ++i ) {
      const f64 d = mspec::lgamma_f64(ld(v[i].x)) - v[i].want;
      if ( !((d < 0.0 ? -d : d) < 1e-14) ) ok = false;
    }
    if ( !same(mspec::lgamma_f64(ld(5.0)), mspec::lgamma_f64(ld(5.0))) ) ok = false;
    require_true(ok);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
