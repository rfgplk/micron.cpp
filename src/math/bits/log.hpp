//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// log log2 log10 log1p live here

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits.hpp"
#include "../constants.hpp"
#include "../dd64.hpp"
#include "../ieee.hpp"
#include "coeff/log_f32.hpp"
#include "coeff/log_f64.hpp"
#include "impl.hpp"
#include "manip.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace log_ns
{

[[nodiscard, gnu::flatten]] inline constexpr f64
log_f64(f64 x) noexcept
{
  using namespace coeff::log_f64_data;

  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x < 0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( x == 0 ) [[unlikely]]
    return ieee::inf_v<f64>(1);
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;

  int k;
  f64 m = manip::frexp<f64>(x, &k);
  if ( m < 0x1.6a09e667f3bcdp-1 ) {
    m = m + m;
    k -= 1;
  }

  f64 f = m - 1.0;
  f64 s = f / (2.0 + f);
  f64 z = s * s;
  f64 w = z * z;

  // odd
  f64 t1 = hw::fmadd_sd(Lg6, w, Lg4);
  t1 = hw::fmadd_sd(t1, w, Lg2);
  t1 = t1 * w;
  // even
  f64 t2 = hw::fmadd_sd(Lg7, w, Lg5);
  t2 = hw::fmadd_sd(t2, w, Lg3);
  t2 = hw::fmadd_sd(t2, w, Lg1);
  t2 = t2 * z;
  f64 R = t1 + t2;

  f64 hfsq = 0.5 * f * f;
  f64 log_m = s * (hfsq + R) + (f - hfsq);

  // log(x) = k * ln2 + log_m
  f64 fk = f64(k);
  return hw::fmadd_sd(fk, ln2_hi, hw::fmadd_sd(fk, ln2_lo, log_m));
}

[[nodiscard, gnu::flatten]] inline constexpr f32
log_f32(f32 x) noexcept
{
  using namespace coeff::log_f32_data;

  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x < 0 ) [[unlikely]]
    return ieee::qnan_v<f32>();
  if ( x == 0 ) [[unlikely]]
    return ieee::inf_v<f32>(1);
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;

  int k;
  f32 m = manip::frexp<f32>(x, &k);
  if ( m < 0x1.6a09e6p-1f ) {
    m = m + m;
    k -= 1;
  }

  f32 f = m - 1.0f;
  f32 s = f / (2.0f + f);
  f32 z = s * s;
  f32 w = z * z;
  f32 t1 = hw::fmadd_ss(Lg4, w, Lg2);
  t1 = t1 * w;
  f32 t2 = hw::fmadd_ss(Lg3, w, Lg1);
  t2 = t2 * z;
  f32 R = t1 + t2;
  f32 hfsq = 0.5f * f * f;
  f32 log_m = s * (hfsq + R) + (f - hfsq);

  f32 fk = f32(k);
  return hw::fmadd_ss(fk, ln2_hi, hw::fmadd_ss(fk, ln2_lo, log_m));
}

[[nodiscard, gnu::flatten]] inline constexpr f64
log1p_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x == -1.0 ) [[unlikely]]
    return ieee::inf_v<f64>(1);
  if ( x < -1.0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;
  if ( manip::fabs(x) < 0x1.0p-54 ) [[unlikely]]
    return x;
  f64 u = 1.0 + x;
  f64 c = (u - 1.0) - x;
  return log_f64(u) - c / u;
}

[[nodiscard, gnu::flatten]] inline constexpr f32
log1p_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x == -1.0f ) [[unlikely]]
    return ieee::inf_v<f32>(1);
  if ( x < -1.0f ) [[unlikely]]
    return ieee::qnan_v<f32>();
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;
  if ( manip::fabs(x) < 0x1.0p-25f ) [[unlikely]]
    return x;
  f32 u = 1.0f + x;
  f32 c = (u - 1.0f) - x;
  return log_f32(u) - c / u;
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
log2_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x < 0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( x == 0 ) [[unlikely]]
    return ieee::inf_v<f64>(1);
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;

  constexpr f64 inv_ln2_hi = 0x1.71547652b82fep+0;
  constexpr f64 inv_ln2_lo = 0x1.777d0ffda0d24p-56;
  f64 lx = log_f64(x);
  return hw::fmadd_sd(lx, inv_ln2_hi, lx * inv_ln2_lo);
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
log2_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x < 0 ) [[unlikely]]
    return ieee::qnan_v<f32>();
  if ( x == 0 ) [[unlikely]]
    return ieee::inf_v<f32>(1);
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;

  constexpr f32 inv_ln2_hi = 0x1.715476p+0f;
  constexpr f32 inv_ln2_lo = 0x1.4ae0bep-26f;
  const f32 lx = log_f32(x);
  return hw::fmadd_ss(lx, inv_ln2_hi, lx * inv_ln2_lo);
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
log10_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x < 0 ) [[unlikely]]
    return ieee::qnan_v<f64>();
  if ( x == 0 ) [[unlikely]]
    return ieee::inf_v<f64>(1);
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;

  constexpr f64 inv_ln10_hi = 0x1.bcb7b1526e50ep-2;
  constexpr f64 inv_ln10_lo = 0x1.95355baaafad3p-57;
  f64 lx = log_f64(x);
  return hw::fmadd_sd(lx, inv_ln10_hi, lx * inv_ln10_lo);
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
log10_f32(f32 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  if ( x < 0 ) [[unlikely]]
    return ieee::qnan_v<f32>();
  if ( x == 0 ) [[unlikely]]
    return ieee::inf_v<f32>(1);
  if ( ieee::is_inf(x) ) [[unlikely]]
    return x;

  constexpr f32 inv_ln10_hi = 0x1.bcb7b2p-2f;
  constexpr f32 inv_ln10_lo = -0x1.5b235ep-29f;
  const f32 lx = log_f32(x);
  return hw::fmadd_ss(lx, inv_ln10_hi, lx * inv_ln10_lo);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(log_f32(f32(x)));
  else
    return F(log_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log2(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(log2_f32(f32(x)));
  else
    return F(log2_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log10(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(log10_f32(f32(x)));
  else
    return F(log10_f64(f64(x)));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log1p(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(log1p_f32(f32(x)));
  else
    return F(log1p_f64(f64(x)));
}

[[nodiscard, gnu::flatten]] inline constexpr dd64
log_dd_kernel_f64(f64 x) noexcept
{
  using namespace coeff::log_f64_data;

  int k;
  f64 m = manip::frexp<f64>(x, &k);
  if ( m < 0x1.6a09e667f3bcdp-1 ) {
    m = m + m;
    k -= 1;
  }
  // sterbenz f and q
  const f64 f = m - 1.0;
  const f64 q = 2.0 + f;

  const dd64 s = dd::div(dd64{ f, 0.0 }, dd64{ q, 0.0 });

  // f**2/2 exactly
  const dd64 ff = dd::two_prod(f, f);
  const dd64 hfsq{ ff.hi * 0.5, ff.lo * 0.5 };

  // s**2 in dd64 for the polynomial argument
  const dd64 z_dd = dd::mul(s, s);
  const f64 z = z_dd.hi;
  const f64 w = z * z;

  // Remez polynomial in z, w
  f64 t1 = hw::fmadd_sd(Lg6, w, Lg4);
  t1 = hw::fmadd_sd(t1, w, Lg2);
  t1 = t1 * w;
  f64 t2 = hw::fmadd_sd(Lg7, w, Lg5);
  t2 = hw::fmadd_sd(t2, w, Lg3);
  t2 = hw::fmadd_sd(t2, w, Lg1);
  t2 = t2 * z;
  const f64 R = t1 + t2;

  // log(1+f) = (f - f**2/2) + s x (f**2/2 + R)
  const dd64 hfsq_plus_R = dd::add(hfsq, R);
  const dd64 corr = dd::mul(s, hfsq_plus_R);
  const dd64 f_minus_hfsq = dd::sub(dd64{ f, 0.0 }, hfsq);
  const dd64 log_m_dd = dd::add(f_minus_hfsq, corr);

  // k x ln2
  const dd64 ln2_dd{ ln2_hi, ln2_lo };
  const dd64 k_ln2 = dd::mul(ln2_dd, dd64{ f64(k), 0.0 });

  return dd::add(k_ln2, log_m_dd);
}

[[nodiscard, gnu::flatten]] inline constexpr dd64
log_dd_f64_with_special(f64 x, bool *handled) noexcept
{

  if ( ieee::is_nan(x) ) {
    *handled = true;
    return dd64{ x, 0.0 };
  }
  if ( x < 0 ) {
    *handled = true;
    return dd64{ ieee::qnan_v<f64>(), 0.0 };
  }
  if ( x == 0 ) {
    *handled = true;
    return dd64{ ieee::inf_v<f64>(1), 0.0 };
  }
  if ( ieee::is_inf(x) ) {
    *handled = true;
    return dd64{ x, 0.0 };
  }
  *handled = false;
  return log_dd_kernel_f64(x);
}

};     // namespace log_ns
};     // namespace mkbits
};     // namespace math
};     // namespace micron

#pragma GCC pop_options
