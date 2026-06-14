//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../bits/impl.hpp"
#include "../../ieee.hpp"

namespace micron
{
namespace math
{
namespace splines
{
namespace __impl_splines_bits
{

// monotonicity checks
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
strictly_increasing(const F *__restrict__ x, usize n) noexcept
{
  if ( n < 2 ) return true;
  for ( usize i = 1; i < n; ++i )
    if ( !(x[i] > x[i - 1]) ) return false;
  return true;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline usize
locate_segment(const F *__restrict__ xs, usize n, F x, usize &last) noexcept
{
  const usize last_seg = (n >= 2) ? (n - 2) : 0;

  if ( !(x > xs[0]) ) {
    last = 0;
    return 0;
  }
  if ( !(x < xs[n - 1]) ) {
    last = last_seg;
    return last_seg;
  }

  const usize i = last <= last_seg ? last : last_seg;
  if ( x >= xs[i] && x <= xs[i + 1] ) {
    last = i;
    return i;
  }
  if ( i + 1 <= last_seg && x >= xs[i + 1] && x <= xs[i + 2] ) {
    last = i + 1;
    return i + 1;
  }

  usize lo = 0;
  usize hi = n - 1;
  while ( hi - lo > 1 ) {
    const usize mid = lo + ((hi - lo) >> 1);
    const bool right = xs[mid] <= x;
    lo = right ? mid : lo;
    hi = right ? hi : mid;
  }
  last = lo;
  return lo;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
eval_cubic_local(const poly_coeffs<F, 3> &p, F t) noexcept
{
  F r = math::fma<F>(p.data[3], t, p.data[2]);
  r = math::fma<F>(r, t, p.data[1]);
  r = math::fma<F>(r, t, p.data[0]);
  return r;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
eval_cubic_deriv1_local(const poly_coeffs<F, 3> &p, F t) noexcept
{
  F r = math::fma<F>(F(3) * p.data[3], t, F(2) * p.data[2]);
  r = math::fma<F>(r, t, p.data[1]);
  return r;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
eval_cubic_deriv2_local(const poly_coeffs<F, 3> &p, F t) noexcept
{
  return math::fma<F>(F(6) * p.data[3], t, F(2) * p.data[2]);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
eval_cubic_antideriv_local(const poly_coeffs<F, 3> &p, F t) noexcept
{
  // via horner
  F r = math::fma<F>(p.data[3] * F(0.25), t, p.data[2] * (F(1) / F(3)));
  r = math::fma<F>(r, t, p.data[1] * F(0.5));
  r = math::fma<F>(r, t, p.data[0]);
  return r * t;
}

template<ieee754_floating F>
inline void
build_cubic_segments(const F *__restrict__ xs, const F *__restrict__ ys, const F *__restrict__ M, poly_coeffs<F, 3> *__restrict__ seg,
                     usize n) noexcept
{
  const F sixth = F(1) / F(6);
  for ( usize i = 0; i + 1 < n; ++i ) {
    const F h = xs[i + 1] - xs[i];
    const F dy = ys[i + 1] - ys[i];
    const F Mi = M[i];
    const F Mi1 = M[i + 1];
    seg[i].data[0] = ys[i];
    seg[i].data[1] = dy / h - h * (F(2) * Mi + Mi1) * sixth;
    seg[i].data[2] = F(0.5) * Mi;
    seg[i].data[3] = (Mi1 - Mi) / (F(6) * h);
  }
}

template<ieee754_floating F>
inline void
build_cubic_segments_from_slopes(const F *__restrict__ xs, const F *__restrict__ ys, const F *__restrict__ m,
                                 poly_coeffs<F, 3> *__restrict__ seg, usize n) noexcept
{
  for ( usize i = 0; i + 1 < n; ++i ) {
    const F h = xs[i + 1] - xs[i];
    const F inv_h = F(1) / h;
    const F delta = (ys[i + 1] - ys[i]) * inv_h;
    const F mi = m[i];
    const F mi1 = m[i + 1];
    seg[i].data[0] = ys[i];
    seg[i].data[1] = mi;
    seg[i].data[2] = (F(3) * delta - F(2) * mi - mi1) * inv_h;
    seg[i].data[3] = (mi + mi1 - F(2) * delta) * (inv_h * inv_h);
  }
}

template<ieee754_floating F>
inline void
pchip_slopes(const F *__restrict__ xs, const F *__restrict__ ys, F *__restrict__ m, usize n) noexcept
{
  if ( n == 0 ) return;
  if ( n == 1 ) {
    m[0] = F(0);
    return;
  }
  if ( n == 2 ) {
    const F s = (ys[1] - ys[0]) / (xs[1] - xs[0]);
    m[0] = s;
    m[1] = s;
    return;
  }

  for ( usize i = 1; i + 1 < n; ++i ) {
    const F h0 = xs[i] - xs[i - 1];
    const F h1 = xs[i + 1] - xs[i];
    const F s0 = (ys[i] - ys[i - 1]) / h0;
    const F s1 = (ys[i + 1] - ys[i]) / h1;
    if ( s0 * s1 <= F(0) ) {
      m[i] = F(0);
    } else {
      const F w0 = F(2) * h1 + h0;
      const F w1 = h1 + F(2) * h0;
      m[i] = (w0 + w1) / (w0 / s0 + w1 / s1);
    }
  }

  {
    const F h0 = xs[1] - xs[0];
    const F h1 = xs[2] - xs[1];
    const F s0 = (ys[1] - ys[0]) / h0;
    const F s1 = (ys[2] - ys[1]) / h1;
    F t = ((F(2) * h0 + h1) * s0 - h0 * s1) / (h0 + h1);
    if ( t * s0 <= F(0) )
      t = F(0);
    else if ( s0 * s1 <= F(0) && (t < F(0) ? -t : t) > F(3) * (s0 < F(0) ? -s0 : s0) )
      t = F(3) * s0;
    m[0] = t;
  }
  {
    const F h0 = xs[n - 2] - xs[n - 3];
    const F h1 = xs[n - 1] - xs[n - 2];
    const F s0 = (ys[n - 2] - ys[n - 3]) / h0;
    const F s1 = (ys[n - 1] - ys[n - 2]) / h1;
    F t = ((F(2) * h1 + h0) * s1 - h1 * s0) / (h0 + h1);
    if ( t * s1 <= F(0) )
      t = F(0);
    else if ( s0 * s1 <= F(0) && (t < F(0) ? -t : t) > F(3) * (s1 < F(0) ? -s1 : s1) )
      t = F(3) * s1;
    m[n - 1] = t;
  }
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline bool
is_sorted_nondecreasing(const F *__restrict__ x, usize n, usize probe = 8) noexcept
{
  (void)probe;
  if ( n < 2 ) return true;
  for ( usize i = 1; i < n; ++i )
    if ( x[i] < x[i - 1] ) return false;
  return true;
}

};      // namespace __impl_splines_bits
};      // namespace splines
};      // namespace math
};      // namespace micron
