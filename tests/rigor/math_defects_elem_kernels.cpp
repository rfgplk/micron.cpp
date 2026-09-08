//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// the scalar elementary-function kernels and the constant tables they read. every golden below is a
// hex literal produced from MPFR at 400 bits, so the file carries its own oracle and links nothing.
//
// what each case pins, and why it is not an accuracy preference:
//   cbrt        -- the halley step used to form a**3 against the RAW argument, so it overflowed to
//                  inf/inf = NaN above |x| ~ 6.6e230 and flushed to 0 below ~8.8e-244
//   cody_waite_dd -- the leading x - N*(pi/2)_hi was formed in one f64 where the product needs 66
//                  bits, so sin/cos/tan lost 8 digits for 2^20 <= |x| < 2^33 while both neighbouring
//                  bands stayed at 2 ulp
//   ATAN_F64    -- the cordic table's own invariant is entry i == atan(2^-i); two entries were not
//   classify_integer -- e == mant_bits was reported EVEN, dropping the sign of pow(-x, odd)
//   exp_f32 twoN -- entry i must be the nearest f32 to 2^(i/16); one entry was not

#include "../../src/math/bits/cordic.hpp"
#include "../../src/math/bits/exp.hpp"
#include "../../src/math/bits/pow.hpp"
#include "../../src/math/bits/sqrt.hpp"
#include "../../src/math/bits/trig.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace mk = micron::math::mkbits;

namespace
{

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
finite64(f64 x) noexcept
{
  return (__builtin_bit_cast(u64, x) & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL;
}

bool
finite32(f32 x) noexcept
{
  return (__builtin_bit_cast(u32, x) & 0x7f800000u) != 0x7f800000u;
}

// ordinal distance, the usual monotone ulp metric
u64
ulps64(f64 a, f64 b) noexcept
{
  if ( !finite64(a) || !finite64(b) ) return 0xFFFFFFFFFFFFFFFFULL;
  i64 x = __builtin_bit_cast(i64, a);
  i64 y = __builtin_bit_cast(i64, b);
  if ( x < 0 ) x = i64(0x8000000000000000ULL) - x;
  if ( y < 0 ) y = i64(0x8000000000000000ULL) - y;
  return u64(x > y ? x - y : y - x);
}

u32
ulps32(f32 a, f32 b) noexcept
{
  if ( !finite32(a) || !finite32(b) ) return 0xFFFFFFFFu;
  i32 x = __builtin_bit_cast(i32, a);
  i32 y = __builtin_bit_cast(i32, b);
  if ( x < 0 ) x = i32(0x80000000u) - x;
  if ( y < 0 ) y = i32(0x80000000u) - y;
  return u32(x > y ? x - y : y - x);
}

f64
absd(f64 x) noexcept
{
  return x < 0 ? -x : x;
}

bool
signbit64(f64 x) noexcept
{
  return (__builtin_bit_cast(u64, x) & 0x8000000000000000ULL) != 0;
}

// -Ofast links crtfastmath.o, which arms FTZ/DAZ: a subnormal argument then compares equal to zero
// before cbrt is even entered, and the early-out returns it unchanged
bool
denormals_live() noexcept
{
  return ld(ld(0x1p-1022) * 0.5) != 0.0;
}

// -fno-signed-zeros, implied by -Ofast, is a promise that the sign of a zero is not observable, and
// gcc acts on it: measured, the -0.0 pow returns folds away in the comparison, not in the callee
#if defined(__NO_SIGNED_ZEROS__) || defined(__FAST_MATH__)
constexpr bool __signed_zero_testable = false;
#else
constexpr bool __signed_zero_testable = true;
#endif

struct pair64 {
  f64 x, g;
};

struct pair32 {
  f32 x, g;
};

struct trio64 {
  f64 x, s, c;
};

// atan(2^-i), nearest f64, i = 0..62 -- the invariant ATAN_F64/ATAN_REF_LD are built on
constexpr f64 __atan_ref[63] = {
  0x1.921fb54442d18p-1,  0x1.dac670561bb4fp-2,  0x1.f5b75f92c80ddp-3,  0x1.fd5ba9aac2f6ep-4,  0x1.ff55bb72cfdeap-5,  0x1.ffd55bba97625p-6,
  0x1.fff555bbb729bp-7,  0x1.fffd555bbba97p-8,  0x1.ffff5555bbbb7p-9,  0x1.ffffd5555bbbcp-10, 0x1.fffff55555bbcp-11, 0x1.fffffd55555bcp-12,
  0x1.ffffff555555cp-13, 0x1.ffffffd555556p-14, 0x1.fffffff555555p-15, 0x1.fffffffd55555p-16, 0x1.ffffffff55555p-17, 0x1.ffffffffd5555p-18,
  0x1.fffffffff5555p-19, 0x1.fffffffffd555p-20, 0x1.ffffffffff555p-21, 0x1.ffffffffffd55p-22, 0x1.fffffffffff55p-23, 0x1.fffffffffffd5p-24,
  0x1.ffffffffffff5p-25, 0x1.ffffffffffffdp-26, 0x1.fffffffffffffp-27, 0x1.0000000000000p-27, 0x1.0000000000000p-28, 0x1.0000000000000p-29,
  0x1.0000000000000p-30, 0x1.0000000000000p-31, 0x1.0000000000000p-32, 0x1.0000000000000p-33, 0x1.0000000000000p-34, 0x1.0000000000000p-35,
  0x1.0000000000000p-36, 0x1.0000000000000p-37, 0x1.0000000000000p-38, 0x1.0000000000000p-39, 0x1.0000000000000p-40, 0x1.0000000000000p-41,
  0x1.0000000000000p-42, 0x1.0000000000000p-43, 0x1.0000000000000p-44, 0x1.0000000000000p-45, 0x1.0000000000000p-46, 0x1.0000000000000p-47,
  0x1.0000000000000p-48, 0x1.0000000000000p-49, 0x1.0000000000000p-50, 0x1.0000000000000p-51, 0x1.0000000000000p-52, 0x1.0000000000000p-53,
  0x1.0000000000000p-54, 0x1.0000000000000p-55, 0x1.0000000000000p-56, 0x1.0000000000000p-57, 0x1.0000000000000p-58, 0x1.0000000000000p-59,
  0x1.0000000000000p-60, 0x1.0000000000000p-61, 0x1.0000000000000p-62,
};

// 2^(i/16), nearest f32 -- the invariant coeff::exp_f32_data::twoN is built on
constexpr f32 __two_p16[16] = {
  0x1.000000p+0f, 0x1.0b5586p+0f, 0x1.172b84p+0f, 0x1.2387a6p+0f, 0x1.306fe0p+0f, 0x1.3dea64p+0f, 0x1.4bfdaep+0f, 0x1.5ab07ep+0f,
  0x1.6a09e6p+0f, 0x1.7a1148p+0f, 0x1.8ace54p+0f, 0x1.9c4918p+0f, 0x1.ae89fap+0f, 0x1.c199bep+0f, 0x1.d5818ep+0f, 0x1.ea4afap+0f,
};

constexpr f64 __two52_plus1 = 0x1.0000000000001p+52;      // 4503599627370497, an ODD integer
constexpr f64 __two52 = 0x1.0000000000000p+52;            // 4503599627370496, an even one
constexpr f64 __two53 = 0x1.0000000000000p+53;            // 9007199254740992, e > mant_bits

};      // namespace

int
main()
{
  print("=== SCALAR ELEMENTARY-KERNEL DEFECT TESTS ===");

  test_case("cbrt stays finite over the whole f64 exponent range and is <= 8 ulp");
  {
    // the top three entries returned NaN and the bottom three returned +0 before the binade scaling
    constexpr pair64 t[] = {
      { 0x1.7e43c8800759cp+996, 0x1.249ad2594c37dp+332 },        // 1e300
      { -0x1.7e43c8800759cp+996, -0x1.249ad2594c37dp+332 },      // -1e300
      { 0x1.b9ea11d82d1bfp+766, 0x1.82ea5708d811dp+255 },        // 6.7e230, just past the old NaN edge
      { 0x1.0000000000000p-1022, 0x1.428a2f98d728bp-341 },       // DBL_MIN, a NORMAL number
      { 0x1.56e1fc2f8f359p-997, 0x1.bff2ee48e0530p-333 },        // 1e-300
      { 0x0.0000000000001p-1022, 0x1.0000000000000p-358 },       // the smallest subnormal
      { 0x1.b4feb7eb212cdp-808, 0x1.e5aacf2156838p-270 },        // 1e-243, 94% wrong before
      { -0x1.b000000000000p+4, -0x1.8000000000000p+1 },          // -27
      { 0x1.0000000000000p+3, 0x1.0000000000000p+1 },            // 8
    };
    const bool sub = denormals_live();
    if ( !sub ) sb::skip("FTZ/DAZ armed: the subnormal rows compare equal to zero on the way in");
    bool ok = true;
    for ( const auto &e : t ) {
      if ( !sub && ulps64(e.x, 0.0) < (1ULL << 52) ) continue;
      const f64 r = mk::sqrt_ns::cbrt<f64>(ld(e.x));
      if ( !finite64(r) || r == 0.0 )
        ok = false;
      else if ( ulps64(r, e.g) > 8 )
        ok = false;
      if ( signbit64(r) != signbit64(e.x) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("cbrt f32 stays finite over the whole f32 exponent range");
  {
    constexpr pair32 t[] = {
      { 0x1.93e594p+99f, 0x1.2a05f2p+33f },       // 1e30f, NaN before
      { 0x1.000000p-149f, 0x1.428a30p-50f },      // the smallest f32 subnormal, +0 before
      { 0x1.4484c0p-100f, 0x1.b7cdfep-34f },      // 1e-30f, 33 ulp before
      { -0x1.000000p+3f, -0x1.000000p+1f },       // -8
    };
    const bool sub = denormals_live();
    if ( !sub ) sb::skip("FTZ/DAZ armed: the subnormal row compares equal to zero on the way in");
    bool ok = true;
    for ( const auto &e : t ) {
      if ( !sub && ulps32(e.x, 0.0f) < (1u << 23) ) continue;
      const f32 r = mk::sqrt_ns::cbrt<f32>(lf(e.x));
      if ( !finite32(r) || r == 0.0f )
        ok = false;
      else if ( ulps32(r, e.g) > 8 )
        ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("sin/cos hold 1e-14 absolute for 2^20 <= |x| < 2^33 (cody_waite_dd)");
  {
    // before the leading two_prod these ran to 4.8e-07 absolute -- ~2e11 ulp -- while |x| < 2^20 and
    // |x| >= 2^33 both stayed at 2 ulp
    constexpr trio64 t[] = {
      { 0x1.dcd6500000000p+29, 0x1.1778cae83c69bp-1, 0x1.acff8c7364234p-1 },       // 1e9
      { 0x1.7d78400000000p+26, 0x1.dcffca623a20bp-1, -0x1.741b388a8c029p-2 },      // 1e8
      { 0x1.3d3e8d17ce1d1p+32, 0x1.067ccc94bb954p-8, 0x1.fffef2dc09ab9p-1 },
      { 0x1.0fa4b0d8b0ccfp+32, -0x1.81b888e6a1d98p-1, 0x1.50b1e2e5363e1p-1 },
      { 0x1.0000180000000p+20, 0x1.edfd304b953b9p-1, -0x1.0d3669fe6c521p-2 },      // just inside
      { 0x1.ffffffff80000p+32, 0x1.c7082d2332cbfp-2, 0x1.caac2a7d977a0p-1 },       // just below 2^33
      { 0x1.0000000080000p+31, -0x1.c3b9eb5603fcdp-1, 0x1.e206cbac83762p-2 },
    };
    bool ok = true;
    for ( const auto &e : t ) {
      if ( absd(mk::trig_ns::sin_f64(ld(e.x)) - e.s) > 1.0e-14 ) ok = false;
      if ( absd(mk::trig_ns::cos_f64(ld(e.x)) - e.c) > 1.0e-14 ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("payne-hanek band keeps its 1e-14 absolute accuracy (control, |x| >= 2^33)");
  {
    // this band was NOT the defect: it holds ~1 ulp(1) ABSOLUTE and nothing more. asserting only
    // the absolute bound is deliberate -- the relative error near a zero of sin is inherent to
    // collapsing the reduced argument into one f64, and is recorded, not fixed
    constexpr trio64 t[] = {
      { 0x1.ea679870c8a06p+993, -0x1.ffffffff74f2dp-1, -0x1.79582d944a5c7p-17 },
      { -0x1.4043f665d8839p+59, -0x1.50b925d905f65p-15, -0x1.fffffff91465ep-1 },
      { 0x1.d6329f1c35ca5p+132, 0x1.4b27597db33cep-1, -0x1.867d0a5330fd0p-1 },
    };
    bool ok = true;
    for ( const auto &e : t ) {
      if ( absd(mk::trig_ns::sin_f64(ld(e.x)) - e.s) > 1.0e-14 ) ok = false;
      if ( absd(mk::trig_ns::cos_f64(ld(e.x)) - e.c) > 1.0e-14 ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("ATAN_F64[i] == atan(2^-i) for every entry");
  {
    // entry 17 was 543 Q2.62 lsbs high and entry 20 was 2 high -- and entry 20 EXCEEDED 2^-20,
    // which atan(y) < y forbids outright
    bool ok = true;
    for ( int i = 0; i < 63; ++i ) {
      if ( ulps64(mk::cordic_ns::ATAN_F64.v[i], __atan_ref[i]) > 2 ) ok = false;
      if ( i >= 1 && f64(mk::cordic_ns::ATAN_REF_LD[i]) > mk::manip::ldexp<f64>(1.0, -i) ) ok = false;
    }
    if ( mk::cordic_ns::ATAN_F64.v[63] != 0.0 ) ok = false;
    require_true(ok);
  }
  end_test_case();

  test_case("cordic atan/sin carry no table bias");
  {
    // the whole cordic accumulator subtracted the bad entry 17 on every call, so every result was
    // ~1.19e-16 out no matter how small the argument
    bool ok = true;
    if ( absd(mk::cordic_ns::atan_f64(ld(0x1.0c6f7a0b5ed8dp-20)) - 0x1.0c6f7a0b5e767p-20) > 3.0e-17 ) ok = false;
    if ( absd(mk::cordic_ns::atan_f64(ld(0x1.0624dd2f1a9fcp-10)) - 0x1.0624d77516e16p-10) > 3.0e-17 ) ok = false;
    if ( absd(mk::cordic_ns::sin_f64(ld(0x1.0624dd2f1a9fcp-10)) - 0x1.0624da5218a62p-10) > 3.0e-17 ) ok = false;
    if ( absd(mk::cordic_ns::atan_f64(ld(0x1.0000000000000p-1)) - 0x1.dac670561bb4fp-2) > 3.0e-17 ) ok = false;
    // tan just below the pole came out with the WRONG SIGN off the biased table
    if ( mk::cordic_ns::tan_f64(ld(0x1.921fb54442d18p+0)) <= 0.0 ) ok = false;
    require_true(ok);
  }
  end_test_case();

  test_case("classify_integer reads the parity bit at e == mant_bits");
  {
    bool ok = true;
    if ( mk::pow_ns::__impl::classify_integer<f64>(__two52_plus1) != 2 ) ok = false;        // odd
    if ( mk::pow_ns::__impl::classify_integer<f64>(__two52) != 1 ) ok = false;              // even
    if ( mk::pow_ns::__impl::classify_integer<f64>(__two53) != 1 ) ok = false;              // e > mant_bits
    if ( mk::pow_ns::__impl::classify_integer<f32>(0x1.000002p+23f) != 2 ) ok = false;      // 8388609
    if ( mk::pow_ns::__impl::classify_integer<f32>(0x1.000000p+23f) != 1 ) ok = false;      // 8388608
    require_true(ok);
  }
  end_test_case();

  test_case("pow keeps the sign of a negative base under an odd exponent in [2^52,2^53)");
  {
    bool ok = true;
    if ( mk::pow_ns::pow<f64>(ld(-1.0), ld(__two52_plus1)) != -1.0 ) ok = false;
    if ( mk::pow_ns::pow<f64>(ld(-1.0), ld(__two52)) != 1.0 ) ok = false;
    const f64 big = mk::pow_ns::pow<f64>(ld(-2.0), ld(__two52_plus1));
    if ( finite64(big) || !signbit64(big) ) ok = false;      // -inf, was +inf
    if constexpr ( __signed_zero_testable ) {
      const f64 tiny = mk::pow_ns::pow<f64>(ld(-0.5), ld(__two52_plus1));
      if ( tiny != 0.0 || !signbit64(tiny) ) ok = false;      // -0, was +0
    }
    if ( mk::pow_ns::pow<f32>(lf(-1.0f), lf(0x1.000002p+23f)) != -1.0f ) ok = false;
    require_true(ok);
  }
  end_test_case();

  test_case("pow_int stays EXACT for small integer exponents");
  {
    // the recorded accuracy complaint against the pow_int path is that its error grows like the bit
    // count of n. this is the property that pays for it -- rerouting integer y at log/exp loses it
    bool ok = true;
    if ( mk::pow_ns::pow<f64>(ld(3.0), ld(10.0)) != 59049.0 ) ok = false;
    if ( mk::pow_ns::pow<f64>(ld(2.0), ld(30.0)) != 1073741824.0 ) ok = false;
    if ( mk::pow_ns::pow<f64>(ld(-3.0), ld(5.0)) != -243.0 ) ok = false;
    require_true(ok);
  }
  end_test_case();

  test_case("exp_f32 twoN[i] == 2^(i/16)");
  {
    bool ok = true;
    for ( int i = 0; i < 16; ++i )
      if ( ulps32(micron::math::mkbits::coeff::exp_f32_data::twoN[i], __two_p16[i]) > 1 ) ok = false;
    require_true(ok);
  }
  end_test_case();

  test_case("exp_f64 twoN[i] == 2^(i/32) (control -- this table was already right)");
  {
    bool ok = true;
    for ( int i = 0; i < 16; ++i )
      if ( ulps64(micron::math::mkbits::coeff::exp_f64_data::twoN[2 * i], f64(__two_p16[i])) > (1ULL << 30) ) ok = false;
    // the f32 table is the f64 one at every other index, rounded
    for ( int i = 0; i < 16; ++i ) {
      const f32 down = f32(micron::math::mkbits::coeff::exp_f64_data::twoN[2 * i]);
      if ( ulps32(down, __two_p16[i]) > 1 ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  print("=== ALL TESTS PASSED ===");
  return 1;
}
