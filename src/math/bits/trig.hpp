//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sin cos sincos tan asin acos atan atan2 live here
//   -> cody waite 2-word range reduction for abs(x) <= 2^20
//   -> payne hanek 396-bit reduction for abs(x) > 2^20
//   -> Remez kernel polynomials
//   -> atan 11-coefficient minimax + 4-entry table
//   -> atan2 9-case special table
//   -> asin/acos 2 region Remez

#include "../../bits.hpp"
#include "../../bits/__int128.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits.hpp"
#include "../constants.hpp"
#include "../dd64.hpp"
#include "../ieee.hpp"
#include "coeff/atan_f64.hpp"
#include "coeff/sin_f32.hpp"
#include "coeff/sin_f64.hpp"
#include "manip.hpp"
#include "round.hpp"
#include "sqrt.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace trig_ns
{

// range reductions

namespace __rr
{

inline constexpr f64 pio2_hi = 0x1.921fb54400000p+0;
inline constexpr f64 pio2_mid = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_lo = 0x1.9880000000000p-77;
inline constexpr f64 inv_pio2 = 0x1.45f306dc9c883p-1;

// cody waite reduction: r = x - N * pi/2
[[nodiscard, gnu::always_inline]] inline constexpr int
cody_waite(f64 x, f64 *r) noexcept
{
  // N = round(x * 2/pi)
  f64 fN = round_ns::rint<f64>(x * inv_pio2);
  i64 N = i64(fN);
  f64 t = x - fN * pio2_hi;
  t = t - fN * pio2_mid;
  t = t - fN * pio2_lo;
  *r = t;
  return int(N & 3);
}

[[nodiscard, gnu::always_inline]] inline constexpr int
iter_cody_waite(f64 x, f64 *r) noexcept
{
  int N_total = 0;
  f64 t = x;
  for ( int pass = 0; pass < 2 && manip::fabs(t) >= 0x1.0p20; ++pass ) {
    f64 fN = round_ns::rint<f64>(t * inv_pio2);
    if ( fN > 0x1.0p20 )
      fN = 0x1.0p20;
    else if ( fN < -0x1.0p20 )
      fN = -0x1.0p20;
    i64 N = i64(fN);
    t = t - fN * pio2_hi;
    t = t - fN * pio2_mid;
    t = t - fN * pio2_lo;
    N_total += int(N);
  }
  f64 fN = round_ns::rint<f64>(t * inv_pio2);
  i64 N = i64(fN);
  t = t - fN * pio2_hi;
  t = t - fN * pio2_mid;
  t = t - fN * pio2_lo;
  N_total += int(N);
  *r = t;
  return N_total & 3;
}

// payne hanek pi/2 reductions
// = mantissa * 2^(ex-52)

namespace __paynehanek
{

inline constexpr u32 ipio2[66] = {
  0xA2F983u, 0x6E4E44u, 0x1529FCu, 0x2757D1u, 0xF534DDu, 0xC0DB62u, 0x95993Cu, 0x439041u, 0xFE5163u, 0xABDEBBu, 0xC561B7u,
  0x246E3Au, 0x424DD2u, 0xE00649u, 0x2EEA09u, 0xD1921Cu, 0xFE1DEBu, 0x1CB129u, 0xA73EE8u, 0x8235F5u, 0x2EBB44u, 0x84E99Cu,
  0x7026B4u, 0x5F7E41u, 0x3991D6u, 0x398353u, 0x39F49Cu, 0x845F8Bu, 0xBDF928u, 0x3B1FF8u, 0x97FFDEu, 0x05980Fu, 0xEF2F11u,
  0x8B5A0Au, 0x6D1F6Du, 0x367ECFu, 0x27CB09u, 0xB74F46u, 0x3F669Eu, 0x5FEA2Du, 0x7527BAu, 0xC7EBE5u, 0xF17B3Du, 0x0739F7u,
  0x8A5292u, 0xEA6BFBu, 0x5FB11Fu, 0x8D5D08u, 0x560330u, 0x46FC7Bu, 0x6BABF0u, 0xCFBC20u, 0x9AF436u, 0x1DA9E3u, 0x91615Eu,
  0xE61B08u, 0x659985u, 0x5F14A0u, 0x68408Du, 0xFFD880u, 0x4D7327u, 0x310606u, 0x1556CAu, 0x73A8C9u, 0x60E27Bu, 0xC08C6Bu,
};

inline constexpr f64 pio2_w0 = 0x1.921fb54400000p+0;     // pi/2 to 33 bits
inline constexpr f64 pio2_w1 = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_w2 = 0x1.9880000000000p-77;

[[nodiscard, gnu::flatten]] inline constexpr int
payne_hanek(f64 x, f64 *r) noexcept
{
  using TR = ieee::traits<f64>;
  using u128 = uint128_t;

  const u64 b = ieee::to_bits(x);
  const bool neg = (b & TR::sign_mask) != 0;
  const u64 m = (b & TR::mant_mask) | TR::implicit_one;
  const int e = int((b & TR::exp_mask) >> TR::mant_bits) - TR::exp_bias;

  const int k0 = (e + 24) / 24;
  u128 acc = u128{ 0 };
  constexpr int anchor = 119;
  for ( int k = 0; k < 6; ++k ) {
    const int idx = k0 + k;
    if ( idx < 0 || idx >= 66 ) continue;
    const u128 prod = u128{ m } * u128{ u64(ipio2[idx]) };     // 77-bit
    const int shift = anchor - 24 * k - 76;
    if ( shift >= 0 && shift < 128 ) {
      acc = acc + (prod << shift);
    } else if ( shift > -128 && shift < 0 ) {
      acc = acc + (prod >> (-shift));
    }
  }

  // bits 120 through 127 of acc hold the integer part
  const u64 acc_hi = u64(acc >> 64);
  const u32 int_part = u32(acc_hi >> 56);
  int q = int(int_part & 3u);

  u128 frac = acc - (u128{ u64(int_part) } << 120);
  const u64 frac_hi = u64(frac >> 64);
  const u64 frac_mid = u64(frac);

  f64 f_hi = f64(frac_hi) * 0x1.0p-56;
  f64 f_lo = f64(frac_mid) * 0x1.0p-120;
  f64 f = f_hi + f_lo;

  if ( f >= 0.5 ) {
    f -= 1.0;
    q = (q + 1) & 3;
  }

  f64 t = f * pio2_w0;
  t = hw::fmadd_sd(f, pio2_w1, t);
  t = hw::fmadd_sd(f, pio2_w2, t);

  if ( neg ) {
    q = (4 - q) & 3;
    t = -t;
  }
  *r = t;
  return q;
}

};     // namespace __paynehanek

[[nodiscard, gnu::always_inline]] inline constexpr int
reduce_pio2(f64 x, f64 *r) noexcept
{
  const f64 ax = manip::fabs(x);
  if ( ax < 0x1.0p20 ) return cody_waite(x, r);
  if ( ax < 0x1.0p33 ) return iter_cody_waite(x, r);
  return __paynehanek::payne_hanek(x, r);
}

[[nodiscard, gnu::flatten]] inline constexpr int
cody_waite_dd(f64 x, dd64 *r) noexcept
{
  const f64 fN = round_ns::rint<f64>(x * inv_pio2);
  const i64 N = i64(fN);
  const f64 t0 = x - fN * pio2_hi;
  const dd64 p_mid = dd::two_prod(fN, pio2_mid);
  const dd64 p_lo = dd::two_prod(fN, pio2_lo);
  dd64 t = dd::sub(dd64{ t0, 0.0 }, p_mid);
  t = dd::sub(t, p_lo);
  *r = t;
  return int(N & 3);
}

[[nodiscard, gnu::flatten]] inline constexpr int
reduce_pio2_dd(f64 x, dd64 *r) noexcept
{
  const f64 ax = manip::fabs(x);
  if ( ax < 0x1.0p20 ) return cody_waite_dd(x, r);
  f64 rf;
  int q;
  if ( ax < 0x1.0p33 )
    q = iter_cody_waite(x, &rf);
  else
    q = __paynehanek::payne_hanek(x, &rf);
  *r = dd64{ rf, 0.0 };
  return q;
}

};     // namespace __rr

[[nodiscard, gnu::always_inline]] inline constexpr f64
ksin_f64(f64 r) noexcept
{
  using namespace coeff::sin_f64_data;
  f64 z = r * r;
  f64 r3 = z * r;
  f64 p = hw::fmadd_sd(S6, z, S5);
  p = hw::fmadd_sd(p, z, S4);
  p = hw::fmadd_sd(p, z, S3);
  p = hw::fmadd_sd(p, z, S2);
  p = hw::fmadd_sd(p, z, S1);
  return r + r3 * p;
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
kcos_f64(f64 r) noexcept
{
  using namespace coeff::sin_f64_data;
  f64 z = r * r;
  f64 hz = 0.5 * z;
  f64 p = hw::fmadd_sd(C6, z, C5);
  p = hw::fmadd_sd(p, z, C4);
  p = hw::fmadd_sd(p, z, C3);
  p = hw::fmadd_sd(p, z, C2);
  p = hw::fmadd_sd(p, z, C1);
  return (1.0 - hz) + z * z * p;
}

[[nodiscard, gnu::flatten]] inline constexpr dd64
ksin_dd_f64(dd64 r) noexcept
{
  using namespace coeff::sin_f64_data;
  const dd64 z = dd::mul(r, r);
  const dd64 r3 = dd::mul(z, r);
  dd64 p{ S6, 0.0 };
  p = dd::add(dd::mul(p, z), S5);
  p = dd::add(dd::mul(p, z), S4);
  p = dd::add(dd::mul(p, z), S3);
  p = dd::add(dd::mul(p, z), S2);
  p = dd::add(dd::mul(p, z), S1);
  return dd::add(r, dd::mul(r3, p));
}

[[nodiscard, gnu::flatten]] inline constexpr dd64
kcos_dd_f64(dd64 r) noexcept
{
  using namespace coeff::sin_f64_data;
  const dd64 z = dd::mul(r, r);
  const dd64 z2 = dd::mul(z, z);
  const dd64 hz = dd::mul(z, dd64{ 0.5, 0.0 });
  dd64 p{ C6, 0.0 };
  p = dd::add(dd::mul(p, z), C5);
  p = dd::add(dd::mul(p, z), C4);
  p = dd::add(dd::mul(p, z), C3);
  p = dd::add(dd::mul(p, z), C2);
  p = dd::add(dd::mul(p, z), C1);
  const dd64 one_minus_hz = dd::sub(dd64{ 1.0, 0.0 }, hz);
  return dd::add(one_minus_hz, dd::mul(z2, p));
}

[[nodiscard, gnu::flatten]] inline constexpr f64
sin_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( manip::fabs(x) < 0x1.0p-26 ) [[unlikely]]
    return x;     // return x if small enough
  f64 r;
  int q = __rr::reduce_pio2(x, &r);
  switch ( q ) {
  case 0 :
    return ksin_f64(r);
  case 1 :
    return kcos_f64(r);
  case 2 :
    return -ksin_f64(r);
  default :
    return -kcos_f64(r);
  }
}

[[nodiscard, gnu::flatten]] inline constexpr f64
cos_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( manip::fabs(x) < 0x1.0p-26 ) [[unlikely]]
    return 1.0;
  f64 r;
  int q = __rr::reduce_pio2(x, &r);
  switch ( q ) {
  case 0 :
    return kcos_f64(r);
  case 1 :
    return -ksin_f64(r);
  case 2 :
    return -kcos_f64(r);
  default :
    return ksin_f64(r);
  }
}

[[gnu::flatten]] inline constexpr void
sincos_f64(f64 x, f64 *s, f64 *c) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]] {
    *s = ieee::qnan_v<f64>();
    *c = ieee::qnan_v<f64>();
    return;
  }
  if ( manip::fabs(x) < 0x1.0p-26 ) [[unlikely]] {
    *s = x;
    *c = 1.0;
    return;
  }
  f64 r;
  int q = __rr::reduce_pio2(x, &r);
  f64 ks = ksin_f64(r);
  f64 kc = kcos_f64(r);
  switch ( q ) {
  case 0 :
    *s = ks;
    *c = kc;
    return;
  case 1 :
    *s = kc;
    *c = -ks;
    return;
  case 2 :
    *s = -ks;
    *c = -kc;
    return;
  default :
    *s = -kc;
    *c = ks;
    return;
  }
}

[[nodiscard, gnu::flatten]] inline constexpr f64
tan_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( manip::fabs(x) < 0x1.0p-26 ) [[unlikely]]
    return x;
  f64 r;
  const int q = __rr::reduce_pio2(x, &r);
  const f64 s = ksin_f64(r);
  const f64 c = kcos_f64(r);
  return (q & 1) ? -c / s : s / c;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
atan_f64(f64 x) noexcept
{
  using namespace coeff::atan_f64_data;
  if ( ieee::is_nan(x) ) return x;
  f64 ax = manip::fabs(x);
  if ( ax >= 0x1.0p+61 ) return manip::copysign<f64>(0x1.921fb54442d18p+0, x);
  if ( ax < 0x1.0p-29 ) return x;     // tiny

  int idx;
  f64 t;
  if ( ax < 0.4375 ) {
    idx = 0;
    t = ax;
  } else if ( ax < 0.6875 ) {
    idx = 1;
    t = (2.0 * ax - 1.0) / (2.0 + ax);
  } else if ( ax < 1.1875 ) {
    idx = 2;
    t = (ax - 1.0) / (ax + 1.0);
  } else if ( ax < 2.4375 ) {
    idx = 3;
    t = (ax - 1.5) / (1.0 + 1.5 * ax);
  } else {
    idx = 4;
    t = -1.0 / ax;
  }

  f64 z = t * t;
  f64 w = z * z;
  f64 s1 = hw::fmadd_sd(aT[10], w, aT[8]);
  s1 = hw::fmadd_sd(s1, w, aT[6]);
  s1 = hw::fmadd_sd(s1, w, aT[4]);
  s1 = hw::fmadd_sd(s1, w, aT[2]);
  s1 = hw::fmadd_sd(s1, w, aT[0]);
  f64 s2 = hw::fmadd_sd(aT[9], w, aT[7]);
  s2 = hw::fmadd_sd(s2, w, aT[5]);
  s2 = hw::fmadd_sd(s2, w, aT[3]);
  s2 = hw::fmadd_sd(s2, w, aT[1]);
  f64 P = s1 + z * s2;
  f64 atan_t = t - t * z * P;
  f64 r = (idx == 0) ? atan_t : f64(atan_lo[idx] + atan_t);
  return manip::signbit(x) ? -r : r;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
atan2_f64(f64 y, f64 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_nan(y) ) [[unlikely]]
    return ieee::qnan_v<f64>();

  constexpr f64 pi = 0x1.921fb54442d18p+1;
  constexpr f64 pi_half = 0x1.921fb54442d18p+0;
  constexpr f64 pi_quarter = 0x1.921fb54442d18p-1;
  constexpr f64 three_pi_q = 0x1.2d97c7f3321d2p+1;

  const bool y_neg = manip::signbit(y);
  const bool x_neg = manip::signbit(x);

  if ( y == 0 ) [[unlikely]] {
    if ( !x_neg ) return manip::copysign<f64>(0.0, y);
    return y_neg ? -pi : pi;
  }
  if ( x == 0 ) [[unlikely]]
    return y_neg ? -pi_half : pi_half;
  if ( ieee::is_inf(x) ) [[unlikely]] {
    if ( ieee::is_inf(y) ) {
      if ( !x_neg ) return y_neg ? -pi_quarter : pi_quarter;
      return y_neg ? -three_pi_q : three_pi_q;
    }
    if ( !x_neg ) return manip::copysign<f64>(0.0, y);
    return y_neg ? -pi : pi;
  }
  if ( ieee::is_inf(y) ) [[unlikely]]
    return y_neg ? -pi_half : pi_half;

  const f64 z = atan_f64(y / x);
  if ( !x_neg ) [[likely]]
    return z;
  return y_neg ? f64(z - pi) : f64(z + pi);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
asin_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  f64 ax = manip::fabs(x);
  if ( ax > 1.0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( ax < 0x1.0p-27 ) [[unlikely]]
    return x;
  constexpr f64 pS0 = 1.66666666666666657415e-01;
  constexpr f64 pS1 = -3.25565818622400915405e-01;
  constexpr f64 pS2 = 2.01212532134862925881e-01;
  constexpr f64 pS3 = -4.00555345006794114027e-02;
  constexpr f64 pS4 = 7.91534994289814532176e-04;
  constexpr f64 pS5 = 3.47933107596021167570e-05;
  constexpr f64 qS1 = -2.40339491173441421878e+00;
  constexpr f64 qS2 = 2.02094576023350569471e+00;
  constexpr f64 qS3 = -6.88283971605453293030e-01;
  constexpr f64 qS4 = 7.70381505559019352791e-02;
  constexpr f64 pio2_hi = 1.57079632679489655800e+00;
  constexpr f64 pio2_lo = 6.12323399573676603587e-17;

  if ( ax < 0.5 ) {
    f64 t = x * x;
    f64 p = t * (pS0 + t * (pS1 + t * (pS2 + t * (pS3 + t * (pS4 + t * pS5)))));
    f64 q = 1.0 + t * (qS1 + t * (qS2 + t * (qS3 + t * qS4)));
    return x + x * (p / q);
  }
  f64 w = 1.0 - ax;
  f64 t = w * 0.5;
  f64 p = t * (pS0 + t * (pS1 + t * (pS2 + t * (pS3 + t * (pS4 + t * pS5)))));
  f64 q = 1.0 + t * (qS1 + t * (qS2 + t * (qS3 + t * qS4)));
  f64 s = sqrt_ns::sqrt<f64>(t);

  f64 result;
  if ( ax >= 0.975 ) {
    f64 wq = p / q;
    result = pio2_hi - (2.0 * (s + s * wq) - pio2_lo);
  } else {
    using T = ieee::traits<f64>;
    using U = T::uint_type;
    U sb = ieee::to_bits(s) & 0xffffffff00000000ULL;
    f64 ws = ieee::from_bits<f64>(sb);
    f64 c = (t - ws * ws) / (s + ws);
    f64 r = p / q;
    f64 pp = 2.0 * s * r - (pio2_lo - 2.0 * c);
    f64 qq = pio2_hi - 2.0 * ws;
    result = qq - pp;
  }
  return manip::signbit(x) ? -result : result;
}

[[nodiscard, gnu::flatten]] inline constexpr f64
acos_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  f64 ax = manip::fabs(x);
  if ( ax > 1.0 ) return ieee::qnan_v<f64>();
  constexpr f64 pio2_hi = 0x1.921fb54442d18p+0;
  constexpr f64 pio2_lo = 0x1.1a62633145c07p-54;
  if ( ax < 0.5 ) {
    if ( ax < 0x1.0p-57 ) return pio2_hi + pio2_lo;
    return pio2_hi - (asin_f64(x) - pio2_lo);
  }
  if ( manip::signbit(x) ) {
    constexpr f64 pi_hi = 0x1.921fb54442d18p+1;
    constexpr f64 pi_lo = 0x1.1a62633145c07p-53;
    f64 w = (1.0 + x) * 0.5;
    f64 s = sqrt_ns::sqrt<f64>(w);
    f64 r = asin_f64(s);
    return pi_hi - (2.0 * r - pi_lo);
  }
  f64 w = (1.0 - x) * 0.5;
  f64 s = sqrt_ns::sqrt<f64>(w);
  return 2.0 * asin_f64(s);
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
ksin_f32(f32 r) noexcept
{
  using namespace coeff::sin_f32_data;
  const f32 z = r * r;
  const f32 r3 = z * r;
  f32 p = hw::fmadd_ss(S4, z, S3);
  p = hw::fmadd_ss(p, z, S2);
  p = hw::fmadd_ss(p, z, S1);
  return r + r3 * p;
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
kcos_f32(f32 r) noexcept
{
  using namespace coeff::sin_f32_data;
  const f32 z = r * r;
  const f32 hz = 0.5f * z;
  f32 p = hw::fmadd_ss(C4, z, C3);
  p = hw::fmadd_ss(p, z, C2);
  p = hw::fmadd_ss(p, z, C1);
  return (1.0f - hz) + z * z * p;
}

[[nodiscard, gnu::flatten]] inline constexpr f32
sin_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f32>();
  if ( manip::fabs(x) < 0x1.0p-12f ) [[unlikely]]
    return x;
  f64 rd;
  const int q = __rr::reduce_pio2(f64(x), &rd);
  const f32 r = f32(rd);
  switch ( q ) {
  case 0 :
    return ksin_f32(r);
  case 1 :
    return kcos_f32(r);
  case 2 :
    return -ksin_f32(r);
  default :
    return -kcos_f32(r);
  }
}

[[nodiscard, gnu::flatten]] inline constexpr f32
cos_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f32>();
  if ( manip::fabs(x) < 0x1.0p-12f ) [[unlikely]]
    return 1.0f;
  f64 rd;
  const int q = __rr::reduce_pio2(f64(x), &rd);
  const f32 r = f32(rd);
  switch ( q ) {
  case 0 :
    return kcos_f32(r);
  case 1 :
    return -ksin_f32(r);
  case 2 :
    return -kcos_f32(r);
  default :
    return ksin_f32(r);
  }
}

[[gnu::flatten]] inline constexpr void
sincos_f32(f32 x, f32 *s, f32 *c) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]] {
    *s = ieee::qnan_v<f32>();
    *c = ieee::qnan_v<f32>();
    return;
  }
  if ( manip::fabs(x) < 0x1.0p-12f ) [[unlikely]] {
    *s = x;
    *c = 1.0f;
    return;
  }
  f64 rd;
  const int q = __rr::reduce_pio2(f64(x), &rd);
  const f32 r = f32(rd);
  const f32 ks = ksin_f32(r);
  const f32 kc = kcos_f32(r);
  switch ( q ) {
  case 0 :
    *s = ks;
    *c = kc;
    return;
  case 1 :
    *s = kc;
    *c = -ks;
    return;
  case 2 :
    *s = -ks;
    *c = -kc;
    return;
  default :
    *s = -kc;
    *c = ks;
    return;
  }
}

[[nodiscard, gnu::flatten]] inline constexpr f32
tan_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) [[unlikely]]
    return ieee::qnan_v<f32>();
  if ( manip::fabs(x) < 0x1.0p-12f ) [[unlikely]]
    return x;
  f64 rd;
  const int q = __rr::reduce_pio2(f64(x), &rd);
  const f32 r = f32(rd);
  const f32 ks = ksin_f32(r);
  const f32 kc = kcos_f32(r);
  return (q & 1) ? -kc / ks : ks / kc;
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
atan_f32(f32 x) noexcept
{
  return f32(atan_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
atan2_f32(f32 y, f32 x) noexcept
{
  return f32(atan2_f64(f64(y), f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
asin_f32(f32 x) noexcept
{
  return f32(asin_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
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

[[nodiscard, gnu::flatten]] inline constexpr dd64
sin_dd_f64_with_special(f64 x, bool *handled) noexcept
{
  if ( ieee::is_nan(x) ) {
    *handled = true;
    return dd64{ x, 0.0 };
  }
  if ( ieee::is_inf(x) ) {
    *handled = true;
    return dd64{ ieee::qnan_v<f64>(), 0.0 };
  }
  if ( manip::fabs(x) < 0x1.0p-26 ) {
    *handled = true;
    return dd64{ x, 0.0 };
  }
  *handled = false;
  dd64 r;
  const int q = __rr::reduce_pio2_dd(x, &r);
  switch ( q ) {
  case 0 :
    return ksin_dd_f64(r);
  case 1 :
    return kcos_dd_f64(r);
  case 2 : {
    dd64 v = ksin_dd_f64(r);
    return dd64{ -v.hi, -v.lo };
  }
  default : {
    dd64 v = kcos_dd_f64(r);
    return dd64{ -v.hi, -v.lo };
  }
  }
}

[[nodiscard, gnu::flatten]] inline constexpr dd64
cos_dd_f64_with_special(f64 x, bool *handled) noexcept
{
  if ( ieee::is_nan(x) ) {
    *handled = true;
    return dd64{ x, 0.0 };
  }
  if ( ieee::is_inf(x) ) {
    *handled = true;
    return dd64{ ieee::qnan_v<f64>(), 0.0 };
  }
  if ( manip::fabs(x) < 0x1.0p-26 ) {
    *handled = true;
    return dd64{ 1.0, 0.0 };
  }
  *handled = false;
  dd64 r;
  const int q = __rr::reduce_pio2_dd(x, &r);
  switch ( q ) {
  case 0 :
    return kcos_dd_f64(r);
  case 1 : {
    dd64 v = ksin_dd_f64(r);
    return dd64{ -v.hi, -v.lo };
  }
  case 2 : {
    dd64 v = kcos_dd_f64(r);
    return dd64{ -v.hi, -v.lo };
  }
  default :
    return ksin_dd_f64(r);
  }
}

};     // namespace trig_ns
};     // namespace mkbits
};     // namespace math
};     // namespace micron

#pragma GCC pop_options
