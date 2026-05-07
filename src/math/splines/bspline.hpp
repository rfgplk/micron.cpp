//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// B-splines of arbitrary degree
// (uses De Boors algorithm)

#include "../../concepts.hpp"
#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../linalg/banded.hpp"
#include "bits/impl.hpp"
#include "concepts.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace splines
{

inline constexpr u32 bspline_max_degree = 16;

template <ieee754_floating F> struct bspline {
  u32 degree{ 0 };
  vector<F> knots;     // size = n_ctrl + degree + 1
  vector<F> ctrl;      // n_ctrl scalar control points
  mutable usize last_hit{ 0 };
};

template <ieee754_floating F>
[[nodiscard]] inline bspline<F>
make_bspline_from_ctrl(raw_slice<const F> knots, raw_slice<const F> ctrl, u32 degree, build_info<F> *info = nullptr) noexcept
{
  bspline<F> s{};
  s.degree = degree;
  if ( degree == 0 || degree > bspline_max_degree ) {
    if ( info ) info->status = build_status::invalid_argument;
    return s;
  }
  if ( knots.size() != ctrl.size() + degree + 1 ) {
    if ( info ) info->status = build_status::size_mismatch;
    return s;
  }
  if ( ctrl.size() < degree + 1 ) {
    if ( info ) info->status = build_status::too_few_points;
    return s;
  }
  for ( usize i = 1; i < knots.size(); ++i )
    if ( knots[i] < knots[i - 1] ) {
      if ( info ) info->status = build_status::non_monotonic_x;
      return s;
    }

  s.knots.reserve(knots.size());
  s.ctrl.reserve(ctrl.size());
  for ( usize i = 0; i < knots.size(); ++i ) s.knots.emplace_back(knots[i]);
  for ( usize i = 0; i < ctrl.size(); ++i ) s.ctrl.emplace_back(ctrl[i]);

  if ( info ) info->status = build_status::ok;
  return s;
}

template <ieee754_floating F>
[[nodiscard]] inline vector<F>
make_uniform_clamped_knots(usize n_ctrl, u32 degree, F t_min, F t_max) noexcept
{
  vector<F> u;
  if ( degree == 0 || n_ctrl < degree + 1 ) return u;
  const usize m = n_ctrl + degree + 1;
  u.reserve(m);
  for ( u32 k = 0; k <= degree; ++k ) u.emplace_back(t_min);

  const usize n_int = n_ctrl - degree - 1;
  for ( usize i = 1; i <= n_int; ++i ) {
    const F t = t_min + (t_max - t_min) * F(i) / F(n_int + 1);
    u.emplace_back(t);
  }
  for ( u32 k = 0; k <= degree; ++k ) u.emplace_back(t_max);
  return u;
}

namespace __impl_bspline
{

template <ieee754_floating F>
[[gnu::always_inline]] inline void
bspline_basis(const F *__restrict__ u, usize k, u32 p, F x, F *__restrict__ B) noexcept
{
  F left[bspline_max_degree + 1];
  F right[bspline_max_degree + 1];
  B[0] = F(1);
  for ( u32 j = 1; j <= p; ++j ) {
    left[j] = x - u[k + 1 - j];
    right[j] = u[k + j] - x;
    F saved = F(0);
    for ( u32 r = 0; r < j; ++r ) {
      const F denom = right[r + 1] + left[j - r];
      F temp = F(0);
      if ( denom > F(0) ) temp = B[r] / denom;
      B[r] = saved + right[r + 1] * temp;
      saved = left[j - r] * temp;
    }
    B[j] = saved;
  }
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline usize
bspline_span(const F *u, usize n_ctrl, u32 degree, F x, usize &last) noexcept
{
  if ( x >= u[n_ctrl] ) {
    last = n_ctrl - 1;
    return last;
  }
  if ( x <= u[degree] ) {
    last = degree;
    return last;
  }
  if ( last >= degree && last < n_ctrl ) {
    if ( x >= u[last] && x < u[last + 1] ) return last;
    if ( last + 1 < n_ctrl && x >= u[last + 1] && x < u[last + 2] ) {
      ++last;
      return last;
    }
  }
  usize lo = degree;
  usize hi = n_ctrl;
  while ( hi - lo > 1 ) {
    const usize mid = lo + ((hi - lo) >> 1);
    const bool right = u[mid] <= x;
    lo = right ? mid : lo;
    hi = right ? hi : mid;
  }
  last = lo;
  return lo;
}

};     // namespace __impl_bspline

// De Boor's algorithm
template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
evaluate(const bspline<F> &s, F x) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 ) return F(0);
  if ( s.degree == 0 ) return F(0);

  const u32 p = s.degree;
  const F *__restrict__ u = s.knots.data();
  const F *__restrict__ P = s.ctrl.data();
  if ( x < u[p] ) x = u[p];
  if ( x > u[n_ctrl] ) x = u[n_ctrl];
  const usize k = __impl_bspline::bspline_span<F>(u, n_ctrl, p, x, s.last_hit);

  F d[bspline_max_degree + 1];
  for ( u32 j = 0; j <= p; ++j ) d[j] = P[k - p + j];

  for ( u32 r = 1; r <= p; ++r ) {
    for ( u32 j = p; j >= r; --j ) {
      const F denom = u[k + 1 + j - r] - u[k + j - p];
      F alpha = F(0);
      if ( denom > F(0) ) alpha = (x - u[k + j - p]) / denom;
      d[j] = math::fma<F>(alpha, d[j] - d[j - 1], d[j - 1]);
    }
  }
  return d[p];
}

template <ieee754_floating F>
[[nodiscard]] inline bspline<F>
derivative_spline(const bspline<F> &s) noexcept
{
  bspline<F> out{};
  if ( s.degree == 0 ) return out;
  const u32 p = s.degree;
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl < 2 ) return out;
  out.degree = p - 1;
  out.knots.reserve(s.knots.size() - 2);
  for ( usize i = 1; i + 1 < s.knots.size(); ++i ) out.knots.emplace_back(s.knots[i]);
  out.ctrl.reserve(n_ctrl - 1);
  const F *u = s.knots.data();
  const F *P = s.ctrl.data();
  for ( usize i = 0; i + 1 < n_ctrl; ++i ) {
    const F denom = u[i + p + 1] - u[i + 1];
    F q = F(0);
    if ( denom > F(0) ) q = F(p) * (P[i + 1] - P[i]) / denom;
    out.ctrl.emplace_back(q);
  }
  return out;
}

template <ieee754_floating F>
[[nodiscard]] inline vector<F>
make_averaged_clamped_knots(const F *xs, usize n, u32 degree) noexcept
{
  vector<F> u;
  if ( degree == 0 || n < degree + 1 ) return u;
  const usize m = n + degree + 1;
  u.reserve(m);
  for ( u32 k = 0; k <= degree; ++k ) u.emplace_back(xs[0]);

  for ( usize i = 1; i + degree < n; ++i ) {
    F s = F(0);
    for ( u32 k = 0; k < degree; ++k ) s += xs[i + k];
    u.emplace_back(s / F(degree));
  }
  for ( u32 k = 0; k <= degree; ++k ) u.emplace_back(xs[n - 1]);
  return u;
}

template <ieee754_floating F>
[[nodiscard]] inline bspline<F>
make_bspline_interpolating(raw_slice<const F> xs, raw_slice<const F> ys, u32 degree, build_info<F> *info = nullptr) noexcept
{
  bspline<F> s{};
  s.degree = degree;
  if ( degree == 0 || degree > bspline_max_degree ) {
    if ( info ) info->status = build_status::invalid_argument;
    return s;
  }
  if ( xs.size() != ys.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return s;
  }
  if ( xs.size() < degree + 1 ) {
    if ( info ) info->status = build_status::too_few_points;
    return s;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(xs.ptr, xs.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return s;
  }

  const usize n = xs.size();
  const u32 p = degree;
  const usize kl = p, ku = p;
  const usize nb = kl + ku + 1;

  s.knots = make_averaged_clamped_knots<F>(xs.ptr, n, p);
  if ( s.knots.size() != n + p + 1 ) {
    if ( info ) info->status = build_status::degenerate;
    return s;
  }
  const F *__restrict__ u = s.knots.data();

  vector<F> AB(nb * n, F(0));
  AB.set_size(nb * n);
  for ( usize i = 0; i < nb * n; ++i ) AB[i] = F(0);

  vector<F> rhs(n, F(0));
  rhs.set_size(n);

  usize last_hit = 0;
  F basis[bspline_max_degree + 1];
  for ( usize i = 0; i < n; ++i ) {
    const F xi = xs[i];
    const usize k = __impl_bspline::bspline_span<F>(u, n, p, xi, last_hit);
    __impl_bspline::bspline_basis<F>(u, k, p, xi, basis);
    for ( u32 j = 0; j <= p; ++j ) {
      const usize col = k - p + j;
      const usize band = (i + ku) - col;
      AB[band * n + col] = basis[j];
    }
    rhs[i] = ys[i];
  }

  if ( !linalg::gband_lu_solve<F>(AB.data(), kl, ku, n, rhs.data()) ) {
    if ( info ) info->status = build_status::singular_system;
    s.knots.set_size(0);
    return s;
  }

  s.ctrl.reserve(n);
  for ( usize i = 0; i < n; ++i ) s.ctrl.emplace_back(rhs[i]);
  if ( info ) info->status = build_status::ok;
  return s;
}

};     // namespace splines
};     // namespace math
};     // namespace micron
