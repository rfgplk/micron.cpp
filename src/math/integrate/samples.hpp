//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// discrete-data quadrature for measurement series
//
//  -> integrate_samples(xs, ys, n)
//  -> cum_trapezoid(xs, ys, out, n)
//  -> cum_simpson(xs, ys, out, n)
//  -> sampled_romberg(y, n, dx)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "bits/kernels.hpp"
#include "common.hpp"
#include "simpson.hpp"
#include "trapezoid.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template<ieee754_floating F>
[[nodiscard]] inline F
integrate_samples(const F *xs, const F *ys, usize n) noexcept
{
  return trapezoid<F>(xs, ys, n);
}

template<is_iterable_container C, is_iterable_container D>
  requires ieee754_floating<typename C::value_type>
[[nodiscard]] inline typename C::value_type
integrate_samples(const C &xs, const D &ys) noexcept
{
  return trapezoid<typename C::value_type>(xs.cbegin(), ys.cbegin(), xs.size());
}

template<ieee754_floating F>
inline void
cum_trapezoid(const F *xs, const F *ys, F *out, usize n) noexcept
{
  if ( n == 0 ) return;
  F y_left = ys[0];
  out[0] = F(0);
  for ( usize i = 1; i < n; ++i ) {
    const F y_right = ys[i];
    const F dx = xs[i] - xs[i - 1];
    out[i] = math::fma<F>(F(0.5) * dx, y_right + y_left, out[i - 1]);
    y_left = y_right;
  }
}

template<ieee754_floating F>
inline void
cum_trapezoid(const F *ys, F *out, usize n, F dx) noexcept
{
  if ( ys != out ) {
    __integrate_arch::cum_trapezoid_uniform(ys, out, n, dx);
    return;
  }
  if ( n == 0 ) return;
  F y_left = ys[0];
  out[0] = F(0);
  for ( usize i = 1; i < n; ++i ) {
    const F y_right = ys[i];
    out[i] = math::fma<F>(F(0.5) * dx, y_right + y_left, out[i - 1]);
    y_left = y_right;
  }
}

template<is_iterable_container X, is_iterable_container Y, is_iterable_container O>
  requires ieee754_floating<typename X::value_type>
inline void
cum_trapezoid(const X &xs, const Y &ys, O &out) noexcept
{
  const usize n = xs.size() < ys.size() ? xs.size() : ys.size();
  cum_trapezoid<typename X::value_type>(xs.cbegin(), ys.cbegin(), out.begin(), n < out.size() ? n : out.size());
}

template<is_iterable_container Y, is_iterable_container O>
  requires ieee754_floating<typename Y::value_type>
inline void
cum_trapezoid(const Y &ys, O &out, typename Y::value_type dx) noexcept
{
  const usize n = ys.size() < out.size() ? ys.size() : out.size();
  cum_trapezoid<typename Y::value_type>(ys.cbegin(), out.begin(), n, dx);
}

namespace __impl_samples
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
irregular_simpson_panel(F x0, F x1, F x2, F y0, F y1, F y2) noexcept
{
  const F h0 = x1 - x0;
  const F h1 = x2 - x1;
  if ( h0 == F(0) || h1 == F(0) ) return F(0);
  const F h = h0 + h1;
  return (h / F(6)) * ((F(2) - h1 / h0) * y0 + (h * h / (h0 * h1)) * y1 + (F(2) - h0 / h1) * y2);
}

};      // namespace __impl_samples

template<ieee754_floating F, accumulation_policy Policy = accumulation_policy::fast>
[[nodiscard]] inline F
simpson(const F *xs, const F *ys, usize n) noexcept
{
  if ( n < 2 ) return F(0);
  __impl_accumulate::sum<F, Policy> total{};
  usize i = 0;
  for ( ; i + 2 < n; i += 2 )
    total.add(__impl_samples::irregular_simpson_panel<F>(xs[i], xs[i + 1], xs[i + 2], ys[i], ys[i + 1], ys[i + 2]));
  if ( i + 1 < n ) total.add(F(0.5) * (xs[i + 1] - xs[i]) * (ys[i] + ys[i + 1]));
  return total.get();
}

template<accumulation_policy Policy = accumulation_policy::fast, is_iterable_container X, is_iterable_container Y>
  requires ieee754_floating<typename X::value_type>
[[nodiscard]] inline typename X::value_type
simpson(const X &xs, const Y &ys) noexcept
{
  const usize n = xs.size() < ys.size() ? xs.size() : ys.size();
  return simpson<typename X::value_type, Policy>(xs.cbegin(), ys.cbegin(), n);
}

template<ieee754_floating F>
inline void
cum_simpson(const F *xs, const F *ys, F *out, usize n) noexcept
{
  if ( n == 0 ) return;
  out[0] = F(0);
  if ( n == 1 ) return;

  F y0 = ys[0];
  F y1 = ys[1];
  out[1] = F(0.5) * (xs[1] - xs[0]) * (y0 + y1);
  usize i = 2;
  for ( ; i < n; i += 2 ) {
    const F y2 = ys[i];
    out[i] = out[i - 2] + __impl_samples::irregular_simpson_panel<F>(xs[i - 2], xs[i - 1], xs[i], y0, y1, y2);
    if ( i + 1 < n ) {
      const F y3 = ys[i + 1];
      out[i + 1] = out[i] + F(0.5) * (xs[i + 1] - xs[i]) * (y2 + y3);
      y0 = y2;
      y1 = y3;
    }
  }
}

template<ieee754_floating F>
inline void
cum_simpson(const F *ys, F *out, usize n, F dx) noexcept
{
  if ( n == 0 ) return;
  out[0] = F(0);
  if ( n == 1 ) return;

  F y0 = ys[0];
  F y1 = ys[1];
  out[1] = F(0.5) * dx * (y0 + y1);
  usize i = 2;
  for ( ; i < n; i += 2 ) {
    const F y2 = ys[i];
    out[i] = out[i - 2] + (dx / F(3)) * (y0 + F(4) * y1 + y2);
    if ( i + 1 < n ) {
      const F y3 = ys[i + 1];
      out[i + 1] = out[i] + F(0.5) * dx * (y2 + y3);
      y0 = y2;
      y1 = y3;
    }
  }
}

template<is_iterable_container X, is_iterable_container Y, is_iterable_container O>
  requires ieee754_floating<typename X::value_type>
inline void
cum_simpson(const X &xs, const Y &ys, O &out) noexcept
{
  const usize xy = xs.size() < ys.size() ? xs.size() : ys.size();
  const usize n = xy < out.size() ? xy : out.size();
  cum_simpson<typename X::value_type>(xs.cbegin(), ys.cbegin(), out.begin(), n);
}

template<is_iterable_container Y, is_iterable_container O>
  requires ieee754_floating<typename Y::value_type>
inline void
cum_simpson(const Y &ys, O &out, typename Y::value_type dx) noexcept
{
  const usize n = ys.size() < out.size() ? ys.size() : out.size();
  cum_simpson<typename Y::value_type>(ys.cbegin(), out.begin(), n, dx);
}

template<ieee754_floating F>
[[nodiscard]] inline F
sampled_romberg(const F *y, usize n, F dx) noexcept
{
  if ( n < 2 ) return F(0);
  const usize panels = n - 1;
  if ( (panels & (panels - 1)) != 0 ) return F(0);

  constexpr usize cap = sizeof(usize) * 8;
  F row[cap]{};
  usize levels = 1;
  for ( usize p = panels; p > 1; p >>= 1 ) ++levels;

  for ( usize level = 0; level < levels; ++level ) {
    const usize stride = panels >> level;
    const usize count = usize(1) << level;
    F t = F(0.5) * (y[0] + y[panels]);
    for ( usize j = 1; j < count; ++j ) t += y[j * stride];
    row[level] = dx * F(stride) * t;
    F factor = F(4);
    for ( usize j = level; j > 0; --j ) {
      row[j - 1] = row[j] + (row[j] - row[j - 1]) / (factor - F(1));
      factor *= F(4);
    }
  }
  return row[0];
}

template<ieee754_floating F>
[[nodiscard]] inline F
romberg(const F *y, usize n, F dx) noexcept
{
  return sampled_romberg<F>(y, n, dx);
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard]] inline typename C::value_type
sampled_romberg(const C &values, typename C::value_type dx) noexcept
{
  return sampled_romberg<typename C::value_type>(values.cbegin(), values.size(), dx);
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
