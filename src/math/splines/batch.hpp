//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// explicit sorted, stateless, cursor, and common query helpers

#include "../../types.hpp"
#include "bits/impl.hpp"
#include "bits/kernels.hpp"
#include "bspline.hpp"
#include "cubic_1d.hpp"
#include "curve_nd.hpp"
#include "linear_1d.hpp"
#include "nearest_1d.hpp"
#include "policies.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<typename S>
  requires requires(const S &s) {
    s.xs.size();
    s.xs[0];
  }
[[nodiscard]] inline auto
domain(const S &s) noexcept
{
  using F = typename decltype(s.xs)::value_type;
  return s.xs.size() ? spline_domain<F>{ s.xs[0], s.xs[s.xs.size() - 1], true } : spline_domain<F>{};
}

template<typename S>
  requires requires(const S &s) {
    s.ts.size();
    s.ts[0];
  }
[[nodiscard]] inline auto
domain(const S &s) noexcept
{
  using F = typename decltype(s.ts)::value_type;
  return s.ts.size() ? spline_domain<F>{ s.ts[0], s.ts[s.ts.size() - 1], true } : spline_domain<F>{};
}

template<ieee754_floating F>
[[nodiscard]] inline spline_domain<F>
domain(const bspline<F> &s) noexcept
{
  return s.ctrl.size() && s.knots.size() ? spline_domain<F>{ s.knots[s.degree], s.knots[s.ctrl.size()], true } : spline_domain<F>{};
}

template<typename S>
  requires requires(const S &s) { s.xs.size(); }
[[nodiscard, gnu::always_inline]] inline usize
knot_count(const S &s) noexcept
{
  return s.xs.size();
}

template<typename S>
  requires requires(const S &s) { s.ts.size(); }
[[nodiscard, gnu::always_inline]] inline usize
knot_count(const S &s) noexcept
{
  return s.ts.size();
}

template<typename S>
  requires requires(const S &s) { s.knots.size(); }
[[nodiscard, gnu::always_inline]] inline usize
knot_count(const S &s) noexcept
{
  return s.knots.size();
}

template<typename S>
  requires requires(const S &s) { s.ctrl.size(); }
[[nodiscard, gnu::always_inline]] inline usize
control_count(const S &s) noexcept
{
  return s.ctrl.size();
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const nearest_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 || x <= s.xs[0] || x >= s.xs[n - 1] ) return evaluate<F>(s, x);
  const usize segment = __impl_splines_bits::locate_segment<F>(s.xs.data(), n, x, cursor.segment);
  return ((x + x) < (s.xs[segment] + s.xs[segment + 1])) ? s.ys[segment] : s.ys[segment + 1];
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const linear_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 || x <= s.xs[0] || x >= s.xs[n - 1] ) return evaluate<F>(s, x);
  const usize segment = __impl_splines_bits::locate_segment<F>(s.xs.data(), n, x, cursor.segment);
  const F fraction = (x - s.xs[segment]) / (s.xs[segment + 1] - s.xs[segment]);
  return math::fma<F>(fraction, s.ys[segment + 1] - s.ys[segment], s.ys[segment]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const cubic_spline_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 || x <= s.xs[0] || x >= s.xs[n - 1] ) return evaluate<F>(s, x);
  const usize segment = __impl_splines_bits::locate_segment<F>(s.xs.data(), n, x, cursor.segment);
  return __impl_splines_bits::eval_cubic_local<F>(s.seg[segment], x - s.xs[segment]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const bspline<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || s.degree == 0 ) return F(0);
  if ( x < s.knots[s.degree] ) x = s.knots[s.degree];
  if ( x > s.knots[n_ctrl] ) x = s.knots[n_ctrl];
  const usize span = __impl_bspline::bspline_span<F>(s.knots.data(), n_ctrl, s.degree, x, cursor.segment);
  switch ( s.degree ) {
  case 1:
    return __impl_bspline::deboor_fixed<1, F>(s.knots.data(), s.ctrl.data(), span, x);
  case 2:
    return __impl_bspline::deboor_fixed<2, F>(s.knots.data(), s.ctrl.data(), span, x);
  case 3:
    return __impl_bspline::deboor_fixed<3, F>(s.knots.data(), s.ctrl.data(), span, x);
  case 4:
    return __impl_bspline::deboor_fixed<4, F>(s.knots.data(), s.ctrl.data(), span, x);
  case 5:
    return __impl_bspline::deboor_fixed<5, F>(s.knots.data(), s.ctrl.data(), span, x);
  default:
    break;
  }
  F d[bspline_max_degree + 1];
  for ( u32 j = 0; j <= s.degree; ++j ) d[j] = s.ctrl[span - s.degree + j];
  for ( u32 r = 1; r <= s.degree; ++r )
    for ( u32 j = s.degree; j >= r; --j ) {
      const F denom = s.knots[span + 1 + j - r] - s.knots[span + j - s.degree];
      const F alpha = denom > F(0) ? (x - s.knots[span + j - s.degree]) / denom : F(0);
      d[j] = math::fma<F>(alpha, d[j] - d[j - 1], d[j - 1]);
    }
  return d[s.degree];
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const cubic_curve_nd<F, D> &c, F t, spline_cursor &cursor) noexcept
{
  const usize n = c.ts.size();
  if ( n < 2 || t <= c.ts[0] || t >= c.ts[n - 1] ) return evaluate<F, D>(c, t);
  const usize segment = __impl_splines_bits::locate_segment<F>(c.ts.data(), n, t, cursor.segment);
  const F u = t - c.ts[segment];
  vec<F, D> out{};
  __spline_arch::curve_horner(c.seg[segment].a.data, c.seg[segment].b.data, c.seg[segment].c.data, c.seg[segment].d.data, u, out.data, D);
  return out;
}

template<typename S, ieee754_floating F>
  requires requires(const S &s, F x, spline_cursor &cursor) { evaluate(s, x, cursor); }
[[nodiscard]] inline auto
evaluate_stateless(const S &s, F x) noexcept
{
  spline_cursor cursor{};
  return evaluate(s, x, cursor);
}

template<ieee754_floating F>
inline void
evaluate_sorted(const cubic_spline_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize count) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) {
    for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }
  usize query = 0;
  while ( query < count && xq[query] <= s.xs[0] ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }
  usize segment = 0;
  if ( query < count && xq[query] < s.xs[n - 1] ) segment = __impl_splines_bits::locate_segment<F>(s.xs.data(), n, xq[query], segment);
  while ( query < count && xq[query] < s.xs[n - 1] ) {
    while ( segment + 1 < n - 1 && xq[query] > s.xs[segment + 1] ) ++segment;
    const usize begin = query;
    while ( query < count && xq[query] < s.xs[n - 1] && xq[query] <= s.xs[segment + 1] ) ++query;
    __spline_arch::cubic_horner_batch(s.seg[segment], s.xs[segment], xq + begin, out + begin, query - begin);
  }
  while ( query < count ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }
  if ( count ) {
    if ( xq[count - 1] <= s.xs[0] )
      s.last_hit = 0;
    else if ( xq[count - 1] >= s.xs[n - 1] )
      s.last_hit = n - 2;
    else
      s.last_hit = segment;
  }
}

template<ieee754_floating F>
inline void
evaluate_sorted(const linear_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize count) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) {
    for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }
  usize query = 0;
  while ( query < count && xq[query] <= s.xs[0] ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }
  usize segment = 0;
  if ( query < count && xq[query] < s.xs[n - 1] ) segment = __impl_splines_bits::locate_segment<F>(s.xs.data(), n, xq[query], segment);
  while ( query < count && xq[query] < s.xs[n - 1] ) {
    while ( segment + 1 < n - 1 && xq[query] > s.xs[segment + 1] ) ++segment;
    const usize begin = query;
    while ( query < count && xq[query] < s.xs[n - 1] && xq[query] <= s.xs[segment + 1] ) ++query;
    const F slope = (s.ys[segment + 1] - s.ys[segment]) / (s.xs[segment + 1] - s.xs[segment]);
    __spline_arch::linear_batch(s.xs[segment], s.ys[segment], slope, xq + begin, out + begin, query - begin);
  }
  while ( query < count ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }
  if ( count ) s.last_hit = xq[count - 1] >= s.xs[n - 1] ? n - 2 : segment;
}

template<ieee754_floating F>
inline void
evaluate_sorted(const nearest_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize count) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) {
    for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }
  usize segment = 0;
  for ( usize i = 0; i < count; ++i ) {
    if ( xq[i] <= s.xs[0] || xq[i] >= s.xs[n - 1] ) {
      out[i] = evaluate<F>(s, xq[i]);
      if ( xq[i] >= s.xs[n - 1] ) segment = n - 2;
      continue;
    }
    while ( segment + 1 < n - 1 && xq[i] > s.xs[segment + 1] ) ++segment;
    out[i] = ((xq[i] + xq[i]) < (s.xs[segment] + s.xs[segment + 1])) ? s.ys[segment] : s.ys[segment + 1];
  }
  if ( count ) s.last_hit = segment;
}

template<ieee754_floating F>
inline void
evaluate_sorted(const bspline<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize count) noexcept
{
  spline_cursor cursor{};
  cursor.segment = s.degree;
  for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F>(s, xq[i], cursor);
  if ( count ) s.last_hit = cursor.segment;
}

template<ieee754_floating F>
inline void
evaluate_streaming(const cubic_spline_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize count) noexcept
{
  if ( !__impl_splines_bits::is_sorted_nondecreasing<F>(xq, count) || s.xs.size() < 2 ) {
    for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }
  const usize n = s.xs.size();
  usize query = 0;
  while ( query < count && xq[query] <= s.xs[0] ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }
  usize segment = 0;
  if ( query < count && xq[query] < s.xs[n - 1] ) segment = __impl_splines_bits::locate_segment<F>(s.xs.data(), n, xq[query], segment);
  while ( query < count && xq[query] < s.xs[n - 1] ) {
    while ( segment + 1 < n - 1 && xq[query] > s.xs[segment + 1] ) ++segment;
    const usize begin = query;
    while ( query < count && xq[query] < s.xs[n - 1] && xq[query] <= s.xs[segment + 1] ) ++query;
    __spline_arch::cubic_horner_stream_batch(s.seg[segment], s.xs[segment], xq + begin, out + begin, query - begin);
  }
  while ( query < count ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }
  if ( count * sizeof(F) >= 64 ) __spline_arch::spline_store_fence();
}

template<ieee754_floating F>
inline void
evaluate_streaming(const linear_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize count) noexcept
{
  if ( !__impl_splines_bits::is_sorted_nondecreasing<F>(xq, count) || s.xs.size() < 2 ) {
    for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }
  const usize n = s.xs.size();
  const F left_slope = (s.ys[1] - s.ys[0]) / (s.xs[1] - s.xs[0]);
  const F right_slope = (s.ys[n - 1] - s.ys[n - 2]) / (s.xs[n - 1] - s.xs[n - 2]);
  usize query = 0;
  while ( query < count && xq[query] <= s.xs[0] ) {
    const F x = xq[query];
    out[query] = s.mode == extrap::error_value
                     ? F(0)
                     : (s.mode == extrap::clamp_to_endpoints ? s.ys[0] : math::fma<F>(left_slope, x - s.xs[0], s.ys[0]));
    ++query;
  }
  usize segment = 0;
  if ( query < count && xq[query] < s.xs[n - 1] ) segment = __impl_splines_bits::locate_segment<F>(s.xs.data(), n, xq[query], segment);
  while ( query < count && xq[query] < s.xs[n - 1] ) {
    while ( segment + 1 < n - 1 && xq[query] > s.xs[segment + 1] ) ++segment;
    const usize begin = query;
    while ( query < count && xq[query] < s.xs[n - 1] && xq[query] <= s.xs[segment + 1] ) ++query;
    const F slope = (s.ys[segment + 1] - s.ys[segment]) / (s.xs[segment + 1] - s.xs[segment]);
    __spline_arch::linear_stream_batch(s.xs[segment], s.ys[segment], slope, xq + begin, out + begin, query - begin);
  }
  while ( query < count ) {
    const F x = xq[query];
    out[query] = s.mode == extrap::error_value
                     ? F(0)
                     : (s.mode == extrap::clamp_to_endpoints ? s.ys[n - 1] : math::fma<F>(right_slope, x - s.xs[n - 1], s.ys[n - 1]));
    ++query;
  }
  if ( count * sizeof(F) >= 64 ) __spline_arch::spline_store_fence();
}

template<ieee754_floating F, usize D>
inline void
evaluate(const nearest_curve_nd<F, D> &s, const F *__restrict__ tq, vec<F, D> *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F, D>(s, tq[i]);
}

template<ieee754_floating F, usize D>
inline void
evaluate(const linear_curve_nd<F, D> &s, const F *__restrict__ tq, vec<F, D> *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F, D>(s, tq[i]);
}

template<ieee754_floating F, usize D>
inline void
evaluate(const cubic_curve_nd<F, D> &s, const F *__restrict__ tq, vec<F, D> *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F, D>(s, tq[i]);
}

template<ieee754_floating F, usize D>
inline void
evaluate(const regular_cubic_curve_nd<F, D> &s, const F *__restrict__ tq, vec<F, D> *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F, D>(s, tq[i]);
}

template<ieee754_floating F>
inline void
derivative(const cubic_spline_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize count, u32 order = 1) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = derivative<F>(s, xq[i], order);
}

};      // namespace splines
};      // namespace math
};      // namespace micron
