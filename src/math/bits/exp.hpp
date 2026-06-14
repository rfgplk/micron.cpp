//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// exp exp2 exp10 expm1 live here
// Tang table (32-entry f64, 16-entry f32) + cody/waite ln2 split + Remez/Taylor polynomial in the res

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits.hpp"
#include "../constants.hpp"
#include "../dd64.hpp"
#include "../ieee.hpp"
#include "coeff/exp_f32.hpp"
#include "coeff/exp_f64.hpp"
#include "impl.hpp"
#include "manip.hpp"
#include "round.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("-fno-unsafe-math-optimizations", "-fno-associative-math", "-fno-reciprocal-math")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace exp_ns
{

[[nodiscard, gnu::flatten]] inline constexpr f64
exp_f64(f64 x) noexcept
{
  using namespace coeff::exp_f64_data;

  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x > 0x1.62e42fefa39efp+9 ) [[unlikely]]      // 709.7827128933840: largest x with finite exp(x)
    return ieee::inf_v<f64>(0);
  if ( x < -745.13 ) [[unlikely]]
    return 0.0;
  if ( x == 0.0 ) [[unlikely]]
    return 1.0;

  // N = round(x * 32 / ln2)
  f64 fN = round_ns::rint<f64>(x * inv_ln2_32);
  i64 N = i64(fN);
  f64 r = hw::fmadd_sd(-fN, ln2_32_hi, x);
  r = hw::fmadd_sd(-fN, ln2_32_lo, r);

  // 1 + r + r**2*(c0 + r*(c1 + r*(c2 + r*(c3 + r*c4))))
  f64 r2 = r * r;
  f64 p = hw::fmadd_sd(rem[4], r, rem[3]);
  p = hw::fmadd_sd(p, r, rem[2]);
  p = hw::fmadd_sd(p, r, rem[1]);
  p = hw::fmadd_sd(p, r, rem[0]);
  f64 exp_r = hw::fmadd_sd(p, r2, r) + 1.0;

  // 2^(N>>5) * 2^((N&31)/32) * exp_r
  i64 j = N & 31;
  i32 k = i32(N >> 5);
  f64 tw = twoN[j];
  f64 r_full = tw * exp_r;
  return manip::ldexp<f64>(r_full, int(k));
}

[[nodiscard, gnu::flatten]] inline constexpr f32
exp_f32(f32 x) noexcept
{
  using namespace coeff::exp_f32_data;

  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x > 88.72283935546875f ) [[unlikely]]      // first f32 above this overflows; kernel yields inf for it
    return ieee::inf_v<f32>(0);
  if ( x < -103.972f ) [[unlikely]]
    return 0.0f;
  if ( x == 0.0f ) [[unlikely]]
    return 1.0f;

  f32 fN = round_ns::rint<f32>(x * inv_ln2_16);
  i32 N = i32(fN);
  f32 r = (x - fN * ln2_16_hi) - fN * ln2_16_lo;

  f32 r2 = r * r;
  f32 p = hw::fmadd_ss(rem[1], r, rem[0]);
  f32 exp_r = hw::fmadd_ss(p, r2, r) + 1.0f;

  i32 j = N & 15;
  i32 k = N >> 4;
  return manip::ldexp<f32>(twoN[j] * exp_r, int(k));
}

[[nodiscard, gnu::flatten]] inline constexpr f64
expm1_f64(f64 x) noexcept
{
  using namespace coeff::exp_f64_data;
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x > 0x1.62e42fefa39efp+9 ) [[unlikely]]      // 709.7827128933840: largest x with finite exp(x)
    return ieee::inf_v<f64>(0);
  if ( x < -38.0 ) [[unlikely]]
    return -1.0;
  f64 ax = manip::fabs(x);
  if ( ax < 0x1.0p-7 ) {
    f64 x2 = x * x;
    f64 p = hw::fmadd_sd(rem[4], x, rem[3]);
    p = hw::fmadd_sd(p, x, rem[2]);
    p = hw::fmadd_sd(p, x, rem[1]);
    p = hw::fmadd_sd(p, x, rem[0]);
    return hw::fmadd_sd(p, x2, x);
  }
  if ( ax < 0.5 ) {
    f64 fN = round_ns::rint<f64>(x * inv_ln2_32);
    i64 N = i64(fN);
    f64 r = (x - fN * ln2_32_hi) - fN * ln2_32_lo;
    f64 r2 = r * r;
    f64 p = hw::fmadd_sd(rem[4], r, rem[3]);
    p = hw::fmadd_sd(p, r, rem[2]);
    p = hw::fmadd_sd(p, r, rem[1]);
    p = hw::fmadd_sd(p, r, rem[0]);
    f64 exp_r_minus_1 = hw::fmadd_sd(p, r2, r);
    i64 j = N & 31;
    i32 k = i32(N >> 5);
    f64 t = twoN[j];
    f64 twoN_m1 = t - 1.0;
    f64 part = twoN_m1 + t * exp_r_minus_1;
    if ( k == 0 ) return part;
    return manip::ldexp<f64>(part + 1.0, int(k)) - 1.0;
  }
  return exp_f64(x) - 1.0;
}

[[nodiscard, gnu::flatten]] inline constexpr f32
expm1_f32(f32 x) noexcept
{
  using namespace coeff::exp_f32_data;
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x > 88.72283935546875f ) [[unlikely]]      // first f32 above this overflows
    return ieee::inf_v<f32>(0);
  if ( x < -20.0f ) [[unlikely]]
    return -1.0f;
  f32 ax = manip::fabs(x);
  if ( ax < 0x1.0p-3f ) {
    // expm1(x) = x + x^2*(1/2 + x*(1/6 + x*(1/24))); the 1/24 term keeps this <=1 ULP near 0.1
    f32 x2 = x * x;
    f32 p = hw::fmadd_ss(0x1.555556p-5f, x, rem[1]);
    p = hw::fmadd_ss(p, x, rem[0]);
    return hw::fmadd_ss(p, x2, x);
  }
  return exp_f32(x) - 1.0f;
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
exp2_f64(f64 x) noexcept
{
  using namespace coeff::exp_f64_data;
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x >= 1024.0 ) [[unlikely]]
    return ieee::inf_v<f64>(0);
  if ( x <= -1075.0 ) [[unlikely]]
    return 0.0;

  // M = round(x * 32)
  const f64 fM = round_ns::rint<f64>(x * 32.0);
  const i64 M = i64(fM);
  const i32 j = i32(M & 31);
  const i32 k = i32(M >> 5);
  // rho = x - M/32
  const f64 rho = x - fM * 0x1.0p-5;
  // r = rgo * ln2
  constexpr f64 ln2_hi = 0x1.62e42fefa39efp-1;
  constexpr f64 ln2_lo = 0x1.abc9e3b39803fp-56;
  const f64 r = hw::fmadd_sd(rho, ln2_hi, rho * ln2_lo);
  // exp(r) = 1 + r + r**2 x P(r)
  f64 P = hw::fmadd_sd(r, rem[4], rem[3]);
  P = hw::fmadd_sd(r, P, rem[2]);
  P = hw::fmadd_sd(r, P, rem[1]);
  P = hw::fmadd_sd(r, P, rem[0]);
  const f64 r2 = r * r;
  const f64 expr = hw::fmadd_sd(r2, P, 1.0 + r);
  // 2^x = ldexp(twoN[j] * exp(r), k)
  return manip::ldexp<f64>(twoN[j] * expr, k);
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
exp2_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x >= 128.0f ) [[unlikely]]
    return ieee::inf_v<f32>(0);
  if ( x <= -150.0f ) [[unlikely]]
    return 0.0f;
  constexpr f32 ln2_hi = 0x1.62e430p-1f;
  constexpr f32 ln2_lo = -0x1.b9e3afp-26f;
  const f32 fN = round_ns::rint<f32>(x);
  const f32 r = x - fN;
  const f32 y = hw::fmadd_ss(r, ln2_hi, r * ln2_lo);
  return manip::ldexp<f32>(exp_f32(y), int(i32(fN)));
}

// exp(x * ln10)
[[nodiscard, gnu::always_inline]] inline constexpr f64
exp10_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  if ( x > 308.25 ) return ieee::inf_v<f64>(0);
  if ( x < -323.3 ) return 0.0;
  // ln10 dd64 split
  constexpr f64 ln10_hi = 0x1.26bb1bbb55516p+1;
  constexpr f64 ln10_lo = -0x1.f48ad494ea3e9p-53;
  f64 y = hw::fmadd_sd(x, ln10_hi, x * ln10_lo);
  return exp_f64(y);
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
exp10_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x > 38.53f ) [[unlikely]]
    return ieee::inf_v<f32>(0);
  if ( x < -45.0f ) [[unlikely]]
    return 0.0f;
  constexpr f32 ln10_hi = 0x1.26bb1cp+1f;
  constexpr f32 ln10_lo = -0x1.f48ad4p-25f;
  const f32 y = hw::fmadd_ss(x, ln10_hi, x * ln10_lo);
  return exp_f32(y);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(exp_f32(f32(x)));
  else
    return F(exp_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp2(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(exp2_f32(f32(x)));
  else
    return F(exp2_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp10(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(exp10_f32(f32(x)));
  else
    return F(exp10_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
expm1(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(expm1_f32(f32(x)));
  else
    return F(expm1_f64(f64(x)));
}

[[nodiscard, gnu::flatten]] inline constexpr dd64
exp_dd_kernel_f64(f64 x) noexcept
{
  using namespace coeff::exp_f64_data;

  const f64 fN = round_ns::rint<f64>(x * inv_ln2_32);
  const i64 N = i64(fN);
  const f64 r_hi = hw::fmadd_sd(-fN, ln2_32_hi, x);
  const f64 r_corr = -fN * ln2_32_lo;
  const dd64 r_dd = dd::add(dd64{ r_hi, 0.0 }, r_corr);

  dd64 p = dd64{ rem[4], 0.0 };
  p = dd::add(dd::mul(p, r_dd), rem[3]);
  p = dd::add(dd::mul(p, r_dd), rem[2]);
  p = dd::add(dd::mul(p, r_dd), rem[1]);
  p = dd::add(dd::mul(p, r_dd), rem[0]);

  const dd64 r_sq = dd::mul(r_dd, r_dd);
  const dd64 one_plus_r = dd::add(dd64{ 1.0, 0.0 }, r_dd);
  const dd64 r2P = dd::mul(r_sq, p);
  const dd64 exp_r = dd::add(one_plus_r, r2P);

  const i64 j = N & 31;
  const i32 k = i32(N >> 5);
  const dd64 t = dd::mul(dd64{ twoN[j], 0.0 }, exp_r);

  const f64 hi = manip::ldexp<f64>(t.hi, int(k));
  const f64 lo = manip::ldexp<f64>(t.lo, int(k));
  return dd64{ hi, lo };
}

[[nodiscard, gnu::flatten]] inline constexpr dd64
exp_dd_f64_with_special(f64 x, bool *handled) noexcept
{
  if ( ieee::is_nan(x) ) {
    *handled = true;
    return dd64{ x, 0.0 };
  }
  if ( x > 0x1.62e42fefa39efp+9 ) {      // 709.7827128933840: largest x with finite exp(x)
    *handled = true;
    return dd64{ ieee::inf_v<f64>(0), 0.0 };
  }
  if ( x < -745.13 ) {
    *handled = true;
    return dd64{ 0.0, 0.0 };
  }
  if ( x == 0.0 ) {
    *handled = true;
    return dd64{ 1.0, 0.0 };
  }
  *handled = false;
  return exp_dd_kernel_f64(x);
}

};      // namespace exp_ns
};      // namespace mkbits
};      // namespace math
};      // namespace micron

#pragma GCC pop_options
