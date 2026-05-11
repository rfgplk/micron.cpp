//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CORDIC shift+add trig kernel
//   -> rotation mode for sin/cos/sincos/tan
//   -> vectoring mode for atan/atan2
//   -> Q2.62 fixed-point for f64 path (62 iterations)
//   -> Q2.30 fixed-point for f32 path (28 iterations)
//   -> Cody-Waite pi/2 range reduction (Payne-Hanek delegated to trig.hpp)

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "manip.hpp"
#include "round.hpp"
#include "trig.hpp"

#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace cordic_ns
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// constants & tables
inline constexpr long double K_INF_LD = 0.6072529350088812561694475L;

inline constexpr int N_F64 = 62;
inline constexpr int N_F32 = 28;

inline constexpr long double Q62_SCALE_LD = 4611686018427387904.0L;     // 2^62
inline constexpr long double Q30_SCALE_LD = 1073741824.0L;              // 2^30
inline constexpr f64 Q62_TO_F64 = 0x1.0p-62;
inline constexpr f32 Q30_TO_F32 = 0x1.0p-30f;
inline constexpr f64 F64_TO_Q62 = 0x1.0p+62;
inline constexpr f32 F32_TO_Q30 = 0x1.0p+30f;

inline constexpr f64 F64_TO_Q61 = 0x1.0p+61;
inline constexpr f32 F32_TO_Q29 = 0x1.0p+29f;

inline constexpr long double ATAN_REF_LD[63] = {
  0.7853981633974483096156608458198757210L,     //  0
  0.4636476090008061162142562314612144020L,     //  1
  0.2449786631268641541720824812112758109L,     //  2
  0.1243549945467614350313548491638710395L,     //  3
  0.0624188099959573484739791129855051136L,     //  4
  0.0312398334302682762537117448924909770L,     //  5
  0.0156237286204768308028015212565703189L,     //  6
  0.0078123410601011112964633918421992816L,     //  7
  0.0039062301319669718276286653114993615L,     //  8
  0.0019531225164788186851214780264478263L,     //  9
  9.7656218955931943045456767291187054e-4L,     // 10
  4.8828121119489828932766246594243358e-4L,     // 11
  2.4414062014936176401640090641870030e-4L,     // 12
  1.2207031189367020423905807943895442e-4L,     // 13
  6.1035156174208772682133954812269e-5L,        // 14
  3.0517578115526096861022508977444e-5L,        // 15
  1.5258789061315761542377552945110e-5L,        // 16
  7.629394531219739048773234802357e-6L,         // 17
  3.814697265606496242526260015022e-6L,         // 18
  1.907348632810187935788000976254e-6L,         // 19
  9.53674316406385849808790115413e-7L,          // 20
  4.76837158203074335175571117507e-7L,          // 21
  2.38418579101557912342076908267e-7L,          // 22
  1.19209289550780685311051119134e-7L,          // 23
  5.96046447753905522713914361060e-8L,          // 24
  2.98023223876953036767246464956e-8L,          // 25
  1.49011611938476552800721650283e-8L,          // 26
  7.45058059692382759952426226155e-9L,          // 27
  3.72529029846191404596574039283e-9L,          // 28
  1.86264514923095703031444202086e-9L,          // 29
  9.3132257461547851562500000000e-10L,          // 30: 2^-30
  4.6566128730773925781250000000e-10L,          // 31
  2.3283064365386962890625000000e-10L,          // 32
  1.1641532182693481445312500000e-10L,          // 33
  5.8207660913467407226562500000e-11L,          // 34
  2.9103830456733703613281250000e-11L,          // 35
  1.4551915228366851806640625000e-11L,          // 36
  7.2759576141834259033203125000e-12L,          // 37
  3.6379788070917129516601562500e-12L,          // 38
  1.8189894035458564758300781250e-12L,          // 39
  9.0949470177292823791503906250e-13L,          // 40
  4.5474735088646411895751953125e-13L,          // 41
  2.2737367544323205947875976562e-13L,          // 42
  1.1368683772161602973937988281e-13L,          // 43
  5.6843418860808014869689941406e-14L,          // 44
  2.8421709430404007434844970703e-14L,          // 45
  1.4210854715202003717422485352e-14L,          // 46
  7.1054273576010018587112426758e-15L,          // 47
  3.5527136788005009293556213379e-15L,          // 48
  1.7763568394002504646778106689e-15L,          // 49
  8.8817841970012523233890533447e-16L,          // 50
  4.4408920985006261616945266724e-16L,          // 51
  2.2204460492503130808472633362e-16L,          // 52
  1.1102230246251565404236316681e-16L,          // 53
  5.5511151231257827021181583404e-17L,          // 54
  2.7755575615628913510590791702e-17L,          // 55
  1.3877787807814456755295395851e-17L,          // 56
  6.9388939039072283776476979256e-18L,          // 57
  3.4694469519536141888238489628e-18L,          // 58
  1.7347234759768070944119244814e-18L,          // 59
  8.6736173798840354720596224070e-19L,          // 60
  4.3368086899420177360298112035e-19L,          // 61
  2.1684043449710088680149056017e-19L           // 62
};

struct atan_q62_table_t {
  i64 v[63];
};

struct atan_q30_table_t {
  i32 v[31];
};

struct shift_pow_f64_table_t {
  f64 v[64];
};

struct shift_pow_f32_table_t {
  f32 v[32];
};

struct atan_f64_table_t {
  f64 v[64];
};

struct atan_f32_table_t {
  f32 v[32];
};

inline constexpr atan_q62_table_t ATAN_Q62 = [] {
  atan_q62_table_t t{};
  for ( int i = 0; i < 63; ++i ) t.v[i] = static_cast<i64>(ATAN_REF_LD[i] * Q62_SCALE_LD);
  return t;
}();

inline constexpr atan_q30_table_t ATAN_Q30 = [] {
  atan_q30_table_t t{};
  for ( int i = 0; i < 31; ++i ) t.v[i] = static_cast<i32>(ATAN_REF_LD[i] * Q30_SCALE_LD);
  return t;
}();

inline constexpr shift_pow_f64_table_t SHIFT_POW_F64 = [] {
  shift_pow_f64_table_t t{};
  long double v = 1.0L;
  for ( int i = 0; i < 64; ++i ) {
    t.v[i] = f64(v);
    v *= 0.5L;
  }
  return t;
}();

inline constexpr shift_pow_f32_table_t SHIFT_POW_F32 = [] {
  shift_pow_f32_table_t t{};
  long double v = 1.0L;
  for ( int i = 0; i < 32; ++i ) {
    t.v[i] = f32(v);
    v *= 0.5L;
  }
  return t;
}();

inline constexpr atan_f64_table_t ATAN_F64 = [] {
  atan_f64_table_t t{};
  for ( int i = 0; i < 64; ++i ) t.v[i] = (i < 63) ? f64(ATAN_REF_LD[i]) : 0.0;
  return t;
}();

inline constexpr atan_f32_table_t ATAN_F32 = [] {
  atan_f32_table_t t{};
  for ( int i = 0; i < 32; ++i ) t.v[i] = (i < 63) ? f32(ATAN_REF_LD[i]) : 0.0f;
  return t;
}();

inline constexpr i64 K_Q62 = static_cast<i64>(K_INF_LD * Q62_SCALE_LD);
inline constexpr i32 K_Q30 = static_cast<i32>(K_INF_LD * Q30_SCALE_LD);
inline constexpr f64 K_F64 = f64(K_INF_LD);
inline constexpr f32 K_F32 = f32(K_INF_LD);

// Number of CORDIC iterations needed in the floating-point SIMD path to reach near f64/f32 precision
inline constexpr int N_FP_F64 = 53;
inline constexpr int N_FP_F32 = 24;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pure shift+add kernels (rotation mode)
[[gnu::flatten, gnu::always_inline]] inline constexpr void
sincos_kernel_q62(i64 z, i64 *c_out, i64 *s_out) noexcept
{
  i64 x = K_Q62;
  i64 y = 0;
  for ( int i = 0; i < N_F64; ++i ) {
    const i64 nm = z >> 63;     // 0 if z >= 0, -1 if z < 0
    const i64 ys = y >> i;
    const i64 xs = x >> i;
    const i64 ai = ATAN_Q62.v[i];
    const i64 x_d = (ys ^ nm) - nm;
    const i64 y_d = (xs ^ nm) - nm;
    const i64 z_d = (ai ^ nm) - nm;
    x -= x_d;
    y += y_d;
    z -= z_d;
  }
  *c_out = x;
  *s_out = y;
}

[[gnu::flatten, gnu::always_inline]] inline constexpr void
sincos_kernel_q30(i32 z, i32 *c_out, i32 *s_out) noexcept
{
  i32 x = K_Q30;
  i32 y = 0;
  for ( int i = 0; i < N_F32; ++i ) {
    const i32 nm = z >> 31;
    const i32 ys = y >> i;
    const i32 xs = x >> i;
    const i32 ai = ATAN_Q30.v[i];
    const i32 x_d = (ys ^ nm) - nm;
    const i32 y_d = (xs ^ nm) - nm;
    const i32 z_d = (ai ^ nm) - nm;
    x -= x_d;
    y += y_d;
    z -= z_d;
  }
  *c_out = x;
  *s_out = y;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// vectoring mode kernel (atan2)
[[gnu::flatten, gnu::always_inline]] inline constexpr i64
atan2_kernel_q62(i64 x, i64 y) noexcept
{
  i64 z = 0;
  for ( int i = 0; i < N_F64; ++i ) {
    const i64 ym = y >> 63;
    const i64 pm = ~ym;     // 0 if y<0, -1 if y>=0
    const i64 ys = y >> i;
    const i64 xs = x >> i;
    const i64 ai = ATAN_Q62.v[i];
    const i64 x_d = (ys ^ pm) - pm;
    const i64 y_d = (xs ^ pm) - pm;
    const i64 z_d = (ai ^ pm) - pm;
    // sigma=+1 path: x -= ys, y += xs, z -= ai (pm=0)
    // sigma=-1 path: x += ys, y -= xs, z += ai (pm=-1)
    x -= x_d;
    y += y_d;
    z -= z_d;
  }
  return z;
}

[[gnu::flatten, gnu::always_inline]] inline constexpr i32
atan2_kernel_q30(i32 x, i32 y) noexcept
{
  i32 z = 0;
  for ( int i = 0; i < N_F32; ++i ) {
    const i32 ym = y >> 31;
    const i32 pm = ~ym;
    const i32 ys = y >> i;
    const i32 xs = x >> i;
    const i32 ai = ATAN_Q30.v[i];
    const i32 x_d = (ys ^ pm) - pm;
    const i32 y_d = (xs ^ pm) - pm;
    const i32 z_d = (ai ^ pm) - pm;
    x -= x_d;
    y += y_d;
    z -= z_d;
  }
  return z;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// range reduction
namespace __rr
{

[[nodiscard, gnu::always_inline]] inline constexpr int
reduce_pio2(f64 x, f64 *r) noexcept
{
  return trig_ns::__rr::reduce_pio2(x, r);
}

[[nodiscard, gnu::always_inline]] inline constexpr int
reduce_pio2(f32 x, f32 *r) noexcept
{
  f64 rd;
  const int q = trig_ns::__rr::reduce_pio2(f64(x), &rd);
  *r = f32(rd);
  return q;
}

};     // namespace __rr

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// f64 entry points
[[gnu::flatten]] inline constexpr void
sincos_f64(f64 x, f64 *s_out, f64 *c_out) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]] {
    *s_out = ieee::qnan_v<f64>();
    *c_out = ieee::qnan_v<f64>();
    return;
  }
  if ( manip::fabs(x) < 0x1.0p-26 ) [[unlikely]] {
    *s_out = x;
    *c_out = 1.0;
    return;
  }
  f64 r;
  const int q = __rr::reduce_pio2(x, &r);
  // r is in (-π/2, π/2). Convert to Q2.62.
  const i64 z = i64(r * F64_TO_Q62);
  i64 ci, si;
  sincos_kernel_q62(z, &ci, &si);
  const f64 c = f64(ci) * Q62_TO_F64;
  const f64 s = f64(si) * Q62_TO_F64;
  switch ( q ) {
  case 0 :
    *s_out = s;
    *c_out = c;
    return;
  case 1 :
    *s_out = c;
    *c_out = -s;
    return;
  case 2 :
    *s_out = -s;
    *c_out = -c;
    return;
  default :
    *s_out = -c;
    *c_out = s;
    return;
  }
}

[[nodiscard, gnu::flatten]] inline constexpr f64
sin_f64(f64 x) noexcept
{
  f64 s, c;
  sincos_f64(x, &s, &c);
  return s;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
cos_f64(f64 x) noexcept
{
  f64 s, c;
  sincos_f64(x, &s, &c);
  return c;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
tan_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f64>();
  f64 s, c;
  sincos_f64(x, &s, &c);
  return s / c;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
atan_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return manip::copysign<f64>(0x1.921fb54442d18p+0, x);     // ±π/2
  const bool inv = manip::fabs(x) > 1.0;
  const f64 t = inv ? (1.0 / x) : x;
  const i64 x0 = i64(F64_TO_Q61);
  const i64 y0 = i64(t * F64_TO_Q61);
  const i64 z = atan2_kernel_q62(x0, y0);
  const f64 a = f64(z) * Q62_TO_F64;
  if ( !inv ) return a;
  constexpr f64 pio2 = 0x1.921fb54442d18p+0;
  return manip::signbit(x) ? f64(-pio2 - a) : f64(pio2 - a);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
atan2_f64(f64 y, f64 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_nan(y) ) [[unlikely]]
    return ieee::qnan_v<f64>();
  constexpr f64 pi = 0x1.921fb54442d18p+1;
  constexpr f64 pi_half = 0x1.921fb54442d18p+0;
  const bool y_neg = manip::signbit(y);
  const bool x_neg = manip::signbit(x);
  if ( y == 0 ) [[unlikely]] {
    if ( !x_neg ) return manip::copysign<f64>(0.0, y);
    return y_neg ? -pi : pi;
  }
  if ( x == 0 ) [[unlikely]]
    return y_neg ? -pi_half : pi_half;
  if ( ieee::is_inf(x) || ieee::is_inf(y) ) [[unlikely]] {
    constexpr f64 pi_quarter = 0x1.921fb54442d18p-1;
    constexpr f64 three_pi_q = 0x1.2d97c7f3321d2p+1;
    if ( ieee::is_inf(x) && ieee::is_inf(y) ) {
      if ( !x_neg ) return y_neg ? -pi_quarter : pi_quarter;
      return y_neg ? -three_pi_q : three_pi_q;
    }
    if ( ieee::is_inf(y) ) return y_neg ? -pi_half : pi_half;
    if ( !x_neg ) return manip::copysign<f64>(0.0, y);
    return y_neg ? -pi : pi;
  }
  const f64 a = atan_f64(y / x);
  if ( !x_neg ) [[likely]]
    return a;
  return y_neg ? f64(a - pi) : f64(a + pi);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
asin_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  const f64 ax = manip::fabs(x);
  if ( ax > 1.0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( ax == 1.0 ) return manip::copysign<f64>(0x1.921fb54442d18p+0, x);
  // asin(x) = atan(x / sqrt(1 - x^2))
  const f64 d = hw::sqrt_sd(1.0 - x * x);
  return atan_f64(x / d);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
acos_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  const f64 ax = manip::fabs(x);
  if ( ax > 1.0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  // acos(x) = atan2(sqrt(1 - x^2), x)
  const f64 d = hw::sqrt_sd((1.0 - x) * (1.0 + x));
  return atan2_f64(d, x);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// f32 entry points
[[gnu::flatten]] inline constexpr void
sincos_f32(f32 x, f32 *s_out, f32 *c_out) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]] {
    *s_out = ieee::qnan_v<f32>();
    *c_out = ieee::qnan_v<f32>();
    return;
  }
  if ( manip::fabs(x) < 0x1.0p-12f ) [[unlikely]] {
    *s_out = x;
    *c_out = 1.0f;
    return;
  }
  f32 r;
  const int q = __rr::reduce_pio2(x, &r);
  const i32 z = i32(f32(r) * F32_TO_Q30);
  i32 ci, si;
  sincos_kernel_q30(z, &ci, &si);
  const f32 c = f32(ci) * Q30_TO_F32;
  const f32 s = f32(si) * Q30_TO_F32;
  switch ( q ) {
  case 0 :
    *s_out = s;
    *c_out = c;
    return;
  case 1 :
    *s_out = c;
    *c_out = -s;
    return;
  case 2 :
    *s_out = -s;
    *c_out = -c;
    return;
  default :
    *s_out = -c;
    *c_out = s;
    return;
  }
}

[[nodiscard, gnu::flatten]] inline constexpr f32
sin_f32(f32 x) noexcept
{
  f32 s, c;
  sincos_f32(x, &s, &c);
  return s;
}

[[nodiscard, gnu::flatten]] inline constexpr f32
cos_f32(f32 x) noexcept
{
  f32 s, c;
  sincos_f32(x, &s, &c);
  return c;
}

[[nodiscard, gnu::flatten]] inline constexpr f32
tan_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f32>();
  f32 s, c;
  sincos_f32(x, &s, &c);
  return s / c;
}

[[nodiscard, gnu::flatten]] inline constexpr f32
atan_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return manip::copysign<f32>(0x1.921fb6p+0f, x);
  const bool inv = manip::fabs(x) > 1.0f;
  const f32 t = inv ? (1.0f / x) : x;
  const i32 x0 = i32(F32_TO_Q29);
  const i32 y0 = i32(t * F32_TO_Q29);
  const i32 z = atan2_kernel_q30(x0, y0);
  const f32 a = f32(z) * Q30_TO_F32;
  if ( !inv ) return a;
  constexpr f32 pio2 = 0x1.921fb6p+0f;
  return manip::signbit(x) ? f32(-pio2 - a) : f32(pio2 - a);
}

[[nodiscard, gnu::flatten]] inline constexpr f32
atan2_f32(f32 y, f32 x) noexcept
{
  return f32(atan2_f64(f64(y), f64(x)));
}

[[nodiscard, gnu::flatten]] inline constexpr f32
asin_f32(f32 x) noexcept
{
  return f32(asin_f64(f64(x)));
}

[[nodiscard, gnu::flatten]] inline constexpr f32
acos_f32(f32 x) noexcept
{
  return f32(acos_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sin(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(sin_f32(f32(x)));
  else
    return F(sin_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
cos(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(cos_f32(f32(x)));
  else
    return F(cos_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
tan(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(tan_f32(f32(x)));
  else
    return F(tan_f64(f64(x)));
}

template <ieee754_floating F>
[[gnu::always_inline]] inline constexpr void
sincos(F x, F &s, F &c) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) ) {
    f32 ss, cc;
    sincos_f32(f32(x), &ss, &cc);
    s = F(ss);
    c = F(cc);
  } else {
    f64 ss, cc;
    sincos_f64(f64(x), &ss, &cc);
    s = F(ss);
    c = F(cc);
  }
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
atan(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(atan_f32(f32(x)));
  else
    return F(atan_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
atan2(F y, F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(atan2_f32(f32(y), f32(x)));
  else
    return F(atan2_f64(f64(y), f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
asin(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(asin_f32(f32(x)));
  else
    return F(asin_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
acos(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(acos_f32(f32(x)));
  else
    return F(acos_f64(f64(x)));
}

};     // namespace cordic_ns
};     // namespace mkbits
};     // namespace math
};     // namespace micron

#pragma GCC pop_options
