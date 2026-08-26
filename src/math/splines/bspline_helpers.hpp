//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// additive B-spline construction, calculus, refinement, and fitting

#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../ieee.hpp"
#include "../sparse/csr.hpp"
#include "../sqrt.hpp"
#include "bspline.hpp"
#include "policies.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
make_unclamped_uniform_knots(usize n_ctrl, u32 degree, F t_min, F t_max) noexcept
{
  vector<F> out;
  if ( degree == 0 || n_ctrl < degree + 1 || !(t_max > t_min) ) return out;
  const usize count = n_ctrl + degree + 1;
  const F spacing = (t_max - t_min) / F(n_ctrl - degree);
  out.reserve(count);
  for ( usize i = 0; i < count; ++i ) out.emplace_back(t_min + (F(i) - F(degree)) * spacing);
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
make_periodic_uniform_knots(usize n_ctrl, u32 degree, F t_min, F t_max) noexcept
{
  return make_unclamped_uniform_knots<F>(n_ctrl, degree, t_min, t_max);
}

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
greville_abscissae(raw_slice<const F> knots, u32 degree) noexcept
{
  vector<F> out;
  if ( degree == 0 || knots.size() < usize(2 * degree + 2) ) return out;
  const usize n_ctrl = knots.size() - degree - 1;
  out.reserve(n_ctrl);
  for ( usize i = 0; i < n_ctrl; ++i ) {
    F sum = F(0);
    for ( u32 j = 1; j <= degree; ++j ) sum += knots[i + j];
    out.emplace_back(sum / F(degree));
  }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
make_knots(knot_style style, usize n_ctrl, u32 degree, F t_min, F t_max, raw_slice<const F> samples = {}) noexcept
{
  if ( style == knot_style::clamped_uniform ) return make_uniform_clamped_knots<F>(n_ctrl, degree, t_min, t_max);
  if ( style == knot_style::unclamped_uniform ) return make_unclamped_uniform_knots<F>(n_ctrl, degree, t_min, t_max);
  if ( style == knot_style::periodic_uniform ) return make_periodic_uniform_knots<F>(n_ctrl, degree, t_min, t_max);
  if ( samples.size() != n_ctrl ) return {};
  return make_averaged_clamped_knots<F>(samples.ptr, samples.size(), degree);
}

template<ieee754_floating F>
inline void
basis(const bspline<F> &s, F x, F *__restrict__ out, usize out_count) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  for ( usize i = 0; i < out_count; ++i ) out[i] = F(0);
  if ( n_ctrl == 0 || out_count < n_ctrl || s.degree == 0 ) return;
  const u32 p = s.degree;
  const F *u = s.knots.data();
  if ( x < u[p] ) x = u[p];
  if ( x > u[n_ctrl] ) x = u[n_ctrl];
  usize last = s.last_hit;
  const usize span = __impl_bspline::bspline_span<F>(u, n_ctrl, p, x, last);
  F local[bspline_max_degree + 1];
  __impl_bspline::bspline_basis<F>(u, span, p, x, local);
  for ( u32 j = 0; j <= p; ++j ) out[span - p + j] = local[j];
}

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
basis(const bspline<F> &s, F x) noexcept
{
  vector<F> out(s.ctrl.size(), F(0));
  out.set_size(s.ctrl.size());
  basis<F>(s, x, out.data(), out.size());
  return out;
}

namespace __impl_bspline_helpers
{

template<ieee754_floating F>
inline void
local_basis_derivatives(const F *u, usize span, u32 degree, F x, u32 order, F out[bspline_max_degree + 1][bspline_max_degree + 1]) noexcept
{
  F ndu[bspline_max_degree + 1][bspline_max_degree + 1]{};
  F left[bspline_max_degree + 1]{};
  F right[bspline_max_degree + 1]{};
  ndu[0][0] = F(1);
  for ( u32 j = 1; j <= degree; ++j ) {
    left[j] = x - u[span + 1 - j];
    right[j] = u[span + j] - x;
    F saved = F(0);
    for ( u32 r = 0; r < j; ++r ) {
      ndu[j][r] = right[r + 1] + left[j - r];
      const F temp = ndu[j][r] != F(0) ? ndu[r][j - 1] / ndu[j][r] : F(0);
      ndu[r][j] = saved + right[r + 1] * temp;
      saved = left[j - r] * temp;
    }
    ndu[j][j] = saved;
  }
  for ( u32 j = 0; j <= degree; ++j ) out[0][j] = ndu[j][degree];

  F a[2][bspline_max_degree + 1]{};
  for ( u32 r = 0; r <= degree; ++r ) {
    u32 s1 = 0, s2 = 1;
    a[0][0] = F(1);
    for ( u32 k = 1; k <= order; ++k ) {
      F d = F(0);
      const i32 rk = i32(r) - i32(k);
      const i32 pk = i32(degree) - i32(k);
      if ( r >= k ) {
        const F denom = ndu[pk + 1][rk];
        a[s2][0] = denom != F(0) ? a[s1][0] / denom : F(0);
        d = a[s2][0] * ndu[rk][pk];
      }
      const i32 j1 = rk >= -1 ? 1 : -rk;
      const i32 j2 = i32(r) - 1 <= pk ? i32(k) - 1 : i32(degree) - i32(r);
      for ( i32 j = j1; j <= j2; ++j ) {
        const F denom = ndu[pk + 1][rk + j];
        a[s2][j] = denom != F(0) ? (a[s1][j] - a[s1][j - 1]) / denom : F(0);
        d += a[s2][j] * ndu[rk + j][pk];
      }
      if ( i32(r) <= pk ) {
        const F denom = ndu[pk + 1][r];
        a[s2][k] = denom != F(0) ? -a[s1][k - 1] / denom : F(0);
        d += a[s2][k] * ndu[r][pk];
      }
      out[k][r] = d;
      const u32 swap = s1;
      s1 = s2;
      s2 = swap;
    }
  }
  F scale = F(degree);
  for ( u32 k = 1; k <= order; ++k ) {
    for ( u32 j = 0; j <= degree; ++j ) out[k][j] *= scale;
    scale *= F(degree - k);
  }
}

template<ieee754_floating F>
[[nodiscard]] inline bool
valid_knots(raw_slice<const F> knots, usize n_ctrl, u32 degree) noexcept
{
  if ( degree == 0 || degree > bspline_max_degree || n_ctrl < degree + 1 ) return false;
  if ( knots.size() != n_ctrl + degree + 1 ) return false;
  for ( usize i = 1; i < knots.size(); ++i )
    if ( knots[i] < knots[i - 1] ) return false;
  return knots[degree] < knots[n_ctrl];
}

template<ieee754_floating F>
[[nodiscard]] inline bool
banded_qr_solve(raw_slice<const F> xs, raw_slice<const F> ys, raw_slice<const F> weights, raw_slice<const F> knots, usize n_ctrl,
                u32 degree, F *solution) noexcept
{
  const usize width = usize(degree + 1);
  vector<F> R(n_ctrl * width, F(0));
  R.set_size(n_ctrl * width);
  vector<F> transformed(n_ctrl, F(0));
  transformed.set_size(n_ctrl);
  usize last = degree;
  for ( usize sample = 0; sample < xs.size(); ++sample ) {
    const usize span = __impl_bspline::bspline_span<F>(knots.ptr, n_ctrl, degree, xs[sample], last);
    const usize first = span - degree;
    F row[bspline_max_degree + 1]{};
    __impl_bspline::bspline_basis<F>(knots.ptr, span, degree, xs[sample], row);
    const F scale = weights.size() ? F(math::fsqrt(weights[sample])) : F(1);
    for ( u32 j = 0; j <= degree; ++j ) row[j] *= scale;
    F rhs = scale * ys[sample];
    for ( usize column = first; column <= span; ++column ) {
      const usize local_column = column - first;
      const F a = R[column * width];
      const F b = row[local_column];
      if ( b == F(0) ) continue;
      const F radius = F(math::fsqrt(a * a + b * b));
      if ( !(radius > F(0)) ) continue;
      const F inverse_radius = F(1) / radius;
      const F cosine = a * inverse_radius;
      const F sine = b * inverse_radius;
      const usize last_column = span < column + degree ? span : column + degree;
      for ( usize j = column; j <= last_column; ++j ) {
        const usize r_index = column * width + j - column;
        const usize row_index = j - first;
        const F old_r = R[r_index];
        const F old_row = row[row_index];
        R[r_index] = math::fma<F>(sine, old_row, cosine * old_r);
        row[row_index] = math::fma<F>(cosine, old_row, -sine * old_r);
      }
      const F old_rhs = transformed[column];
      transformed[column] = math::fma<F>(sine, rhs, cosine * old_rhs);
      rhs = math::fma<F>(cosine, rhs, -sine * old_rhs);
    }
  }
  const F tiny = sizeof(F) <= 4 ? F(1e-12) : F(1e-28);
  for ( usize ii = n_ctrl; ii-- > 0; ) {
    F rhs = transformed[ii];
    const usize end = ii + degree < n_ctrl - 1 ? ii + degree : n_ctrl - 1;
    for ( usize j = ii + 1; j <= end; ++j ) rhs -= R[ii * width + j - ii] * solution[j];
    const F diagonal = R[ii * width];
    if ( !(math::fabs(diagonal) > tiny) ) return false;
    solution[ii] = rhs / diagonal;
  }
  return true;
}

template<ieee754_floating F>
[[nodiscard]] inline bool
banded_normal_solve(raw_slice<const F> xs, raw_slice<const F> ys, raw_slice<const F> weights, raw_slice<const F> knots, usize n_ctrl,
                    u32 degree, F *solution) noexcept
{
  const usize width = usize(degree + 1);
  vector<F> lower(n_ctrl * width, F(0));
  lower.set_size(n_ctrl * width);
  for ( usize i = 0; i < n_ctrl; ++i ) solution[i] = F(0);
  usize last = degree;
  for ( usize sample = 0; sample < xs.size(); ++sample ) {
    const usize span = __impl_bspline::bspline_span<F>(knots.ptr, n_ctrl, degree, xs[sample], last);
    const usize first = span - degree;
    F row[bspline_max_degree + 1]{};
    __impl_bspline::bspline_basis<F>(knots.ptr, span, degree, xs[sample], row);
    const F weight = weights.size() ? weights[sample] : F(1);
    for ( u32 j = 0; j <= degree; ++j ) {
      const usize column = first + j;
      solution[column] = math::fma<F>(weight * row[j], ys[sample], solution[column]);
      for ( u32 k = 0; k <= j; ++k ) {
        const usize other = first + k;
        lower[column * width + column - other] = math::fma<F>(weight * row[j], row[k], lower[column * width + column - other]);
      }
    }
  }
  for ( usize i = 0; i < n_ctrl; ++i ) {
    const usize begin = i > degree ? i - degree : 0;
    for ( usize j = begin; j <= i; ++j ) {
      F value = lower[i * width + i - j];
      usize k_begin = i > degree ? i - degree : 0;
      const usize j_begin = j > degree ? j - degree : 0;
      if ( j_begin > k_begin ) k_begin = j_begin;
      for ( usize k = k_begin; k < j; ++k ) value -= lower[i * width + i - k] * lower[j * width + j - k];
      if ( i == j ) {
        if ( !(value > F(0)) ) return false;
        lower[i * width] = F(math::fsqrt(value));
      } else {
        lower[i * width + i - j] = value / lower[j * width];
      }
    }
  }
  for ( usize i = 0; i < n_ctrl; ++i ) {
    F value = solution[i];
    const usize begin = i > degree ? i - degree : 0;
    for ( usize j = begin; j < i; ++j ) value -= lower[i * width + i - j] * solution[j];
    solution[i] = value / lower[i * width];
  }
  for ( usize ii = n_ctrl; ii-- > 0; ) {
    F value = solution[ii];
    const usize end = ii + degree < n_ctrl - 1 ? ii + degree : n_ctrl - 1;
    for ( usize j = ii + 1; j <= end; ++j ) value -= lower[j * width + j - ii] * solution[j];
    solution[ii] = value / lower[ii * width];
  }
  return true;
}

};      // namespace __impl_bspline_helpers

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
basis_derivatives(const bspline<F> &s, F x, u32 order) noexcept
{
  vector<F> out;
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || s.degree == 0 ) return out;
  if ( order > s.degree ) order = s.degree;
  out = vector<F>(usize(order + 1) * n_ctrl, F(0));
  out.set_size(usize(order + 1) * n_ctrl);
  const F *u = s.knots.data();
  if ( x < u[s.degree] ) x = u[s.degree];
  if ( x > u[n_ctrl] ) x = u[n_ctrl];
  usize last = s.last_hit;
  const usize span = __impl_bspline::bspline_span<F>(u, n_ctrl, s.degree, x, last);
  F local[bspline_max_degree + 1][bspline_max_degree + 1]{};
  __impl_bspline_helpers::local_basis_derivatives<F>(u, span, s.degree, x, order, local);
  for ( u32 k = 0; k <= order; ++k )
    for ( u32 j = 0; j <= s.degree; ++j ) out[usize(k) * n_ctrl + span - s.degree + j] = local[k][j];
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline sparse::csr<F, u32>
design_matrix(const bspline<F> &s, raw_slice<const F> x) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  sparse::csr<F, u32> out(x.size(), n_ctrl);
  if ( n_ctrl == 0 || s.degree == 0 ) return out;
  const usize per_row = usize(s.degree + 1);
  out.inner.reserve(x.size() * per_row);
  out.values.reserve(x.size() * per_row);
  out.outer[0] = 0;
  usize last = s.last_hit;
  for ( usize i = 0; i < x.size(); ++i ) {
    F value = x[i];
    if ( value < s.knots[s.degree] ) value = s.knots[s.degree];
    if ( value > s.knots[n_ctrl] ) value = s.knots[n_ctrl];
    const usize span = __impl_bspline::bspline_span<F>(s.knots.data(), n_ctrl, s.degree, value, last);
    F local[bspline_max_degree + 1];
    __impl_bspline::bspline_basis<F>(s.knots.data(), span, s.degree, value, local);
    for ( u32 j = 0; j <= s.degree; ++j ) {
      out.inner.emplace_back(u32(span - s.degree + j));
      out.values.emplace_back(local[j]);
    }
    out.outer[i + 1] = u32(out.values.size());
  }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline bspline<F>
antiderivative_spline(const bspline<F> &s, F constant = F(0)) noexcept
{
  bspline<F> out{};
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || s.degree >= bspline_max_degree ) return out;
  const u32 p = s.degree;
  out.degree = p + 1;
  out.knots.reserve(s.knots.size() + 2);
  out.knots.emplace_back(s.knots[0]);
  for ( usize i = 0; i < s.knots.size(); ++i ) out.knots.emplace_back(s.knots[i]);
  out.knots.emplace_back(s.knots[s.knots.size() - 1]);
  out.ctrl.reserve(n_ctrl + 1);
  F value = constant;
  out.ctrl.emplace_back(value);
  for ( usize i = 0; i < n_ctrl; ++i ) {
    value += (s.knots[i + p + 1] - s.knots[i]) * s.ctrl[i] / F(p + 1);
    out.ctrl.emplace_back(value);
  }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline F
integral(const bspline<F> &s, F a, F b) noexcept
{
  const auto anti = antiderivative_spline<F>(s);
  return evaluate<F>(anti, b) - evaluate<F>(anti, a);
}

template<ieee754_floating F>
[[nodiscard]] inline bspline<F>
insert_knot(const bspline<F> &s, F knot, u32 repetitions = 1, build_info<F> *info = nullptr) noexcept
{
  bspline<F> current = s;
  if ( s.ctrl.size() == 0 || repetitions == 0 ) {
    if ( info ) info->status = build_status::ok;
    return current;
  }
  for ( u32 repeat = 0; repeat < repetitions; ++repeat ) {
    const u32 p = current.degree;
    const usize n_ctrl = current.ctrl.size();
    const F lo = current.knots[p];
    const F hi = current.knots[n_ctrl];
    if ( knot < lo || knot > hi ) {
      if ( info ) info->status = build_status::invalid_argument;
      return {};
    }
    usize last = current.last_hit;
    const usize span = __impl_bspline::bspline_span<F>(current.knots.data(), n_ctrl, p, knot, last);
    u32 multiplicity = 0;
    for ( usize i = 0; i < current.knots.size(); ++i )
      if ( current.knots[i] == knot ) ++multiplicity;
    const u32 limit = (knot == lo || knot == hi) ? p + 1 : p;
    if ( multiplicity >= limit ) {
      if ( info ) info->status = build_status::degenerate;
      return {};
    }
    bspline<F> next{};
    next.degree = p;
    next.knots.reserve(current.knots.size() + 1);
    for ( usize i = 0; i <= span; ++i ) next.knots.emplace_back(current.knots[i]);
    next.knots.emplace_back(knot);
    for ( usize i = span + 1; i < current.knots.size(); ++i ) next.knots.emplace_back(current.knots[i]);
    next.ctrl.reserve(n_ctrl + 1);
    for ( usize i = 0; i <= span - p; ++i ) next.ctrl.emplace_back(current.ctrl[i]);
    for ( usize i = span - p + 1; i <= span - multiplicity; ++i ) {
      const F denom = current.knots[i + p] - current.knots[i];
      const F alpha = denom > F(0) ? (knot - current.knots[i]) / denom : F(0);
      next.ctrl.emplace_back(math::fma<F>(alpha, current.ctrl[i] - current.ctrl[i - 1], current.ctrl[i - 1]));
    }
    for ( usize i = span - multiplicity; i < n_ctrl; ++i ) next.ctrl.emplace_back(current.ctrl[i]);
    current = micron::move(next);
  }
  if ( info ) info->status = build_status::ok;
  return current;
}

template<ieee754_floating F>
[[nodiscard]] inline bspline<F>
refine_knots(const bspline<F> &s, raw_slice<const F> knots, build_info<F> *info = nullptr) noexcept
{
  bspline<F> out = s;
  for ( usize i = 0; i < knots.size(); ++i ) {
    build_info<F> local{};
    out = insert_knot<F>(out, knots[i], 1, &local);
    if ( local.status != build_status::ok ) {
      if ( info ) info->status = local.status;
      return {};
    }
  }
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F> struct bspline_split {
  bspline<F> left;
  bspline<F> right;
};

template<ieee754_floating F>
[[nodiscard]] inline bspline_split<F>
split(const bspline<F> &s, F knot, build_info<F> *info = nullptr) noexcept
{
  bspline_split<F> out{};
  if ( s.ctrl.size() == 0 || !(knot > s.knots[s.degree] && knot < s.knots[s.ctrl.size()]) ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  u32 multiplicity = 0;
  for ( usize i = 0; i < s.knots.size(); ++i )
    if ( s.knots[i] == knot ) ++multiplicity;
  const u32 insertions = multiplicity < s.degree ? s.degree - multiplicity : 0;
  auto refined = insert_knot<F>(s, knot, insertions, info);
  if ( refined.ctrl.size() == 0 ) return out;
  usize first = 0;
  while ( first < refined.knots.size() && refined.knots[first] < knot ) ++first;
  const usize left_ctrl = first;
  // The p-fold knot is shared by both children.  Its shared control point is
  // immediately before the first copy of the split knot.
  const usize right_start = first - 1;

  out.left.degree = refined.degree;
  out.left.ctrl.reserve(left_ctrl);
  for ( usize i = 0; i < left_ctrl; ++i ) out.left.ctrl.emplace_back(refined.ctrl[i]);
  out.left.knots.reserve(left_ctrl + refined.degree + 1);
  for ( usize i = 0; i < first; ++i ) out.left.knots.emplace_back(refined.knots[i]);
  for ( u32 i = 0; i <= refined.degree; ++i ) out.left.knots.emplace_back(knot);

  out.right.degree = refined.degree;
  out.right.ctrl.reserve(refined.ctrl.size() - right_start);
  for ( usize i = right_start; i < refined.ctrl.size(); ++i ) out.right.ctrl.emplace_back(refined.ctrl[i]);
  out.right.knots.reserve(out.right.ctrl.size() + refined.degree + 1);
  for ( u32 i = 0; i <= refined.degree; ++i ) out.right.knots.emplace_back(knot);
  for ( usize i = first + refined.degree; i < refined.knots.size(); ++i ) out.right.knots.emplace_back(refined.knots[i]);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline bspline<F>
make_lsq_bspline(raw_slice<const F> xs, raw_slice<const F> ys, raw_slice<const F> knots, u32 degree, raw_slice<const F> weights = {},
                 lsq_method method = lsq_method::householder_qr, build_info<F> *info = nullptr) noexcept
{
  bspline<F> out{};
  out.degree = degree;
  if ( xs.size() != ys.size() || (weights.size() != 0 && weights.size() != xs.size()) ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  if ( knots.size() <= degree + 1 ) {
    if ( info ) info->status = build_status::too_few_points;
    return out;
  }
  const usize n_ctrl = knots.size() - degree - 1;
  if ( xs.size() < n_ctrl || !__impl_bspline_helpers::valid_knots<F>(knots, n_ctrl, degree) ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(xs.ptr, xs.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return out;
  }
  out.knots.reserve(knots.size());
  for ( usize i = 0; i < knots.size(); ++i ) out.knots.emplace_back(knots[i]);
  vector<F> solution(n_ctrl, F(0));
  solution.set_size(n_ctrl);
  for ( usize i = 0; i < xs.size(); ++i ) {
    if ( xs[i] < knots[degree] || xs[i] > knots[n_ctrl] || (weights.size() && !(weights[i] > F(0))) ) {
      if ( info ) info->status = build_status::invalid_argument;
      return {};
    }
  }
  const bool solved = method == lsq_method::householder_qr
                          ? __impl_bspline_helpers::banded_qr_solve<F>(xs, ys, weights, knots, n_ctrl, degree, solution.data())
                          : __impl_bspline_helpers::banded_normal_solve<F>(xs, ys, weights, knots, n_ctrl, degree, solution.data());
  if ( !solved ) {
    if ( info ) info->status = build_status::singular_system;
    return {};
  }
  out.ctrl.reserve(n_ctrl);
  for ( usize i = 0; i < n_ctrl; ++i ) out.ctrl.emplace_back(solution[i]);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
inline void
evaluate(const bspline<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
}

};      // namespace splines
};      // namespace math
};      // namespace micron
