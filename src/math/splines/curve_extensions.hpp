//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// additive B-spline, NURBS, Bezier, closed, and packed curves

#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../ieee.hpp"
#include "../quants/vec.hpp"
#include "../sqrt.hpp"
#include "bits/kernels.hpp"
#include "bspline_helpers.hpp"
#include "curve_nd.hpp"
#include "piecewise_1d.hpp"
#include "policies.hpp"
#include "smoothing.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<ieee754_floating F, usize D>
  requires(D >= 2 && D <= 16)
struct bspline_curve_nd {
  u32 degree{ 0 };
  vector<F> knots;
  vector<vec<F, D>> ctrl;
  vector<F> __basis_inverse;
  mutable usize last_hit{ 0 };
  extension_mode mode{ extension_mode::clamp };
};

template<ieee754_floating F, usize D>
  requires(D >= 2 && D <= 16)
struct nurbs_curve_nd {
  u32 degree{ 0 };
  vector<F> knots;
  vector<vec<F, D>> ctrl;
  vector<F> weights;
  vector<F> __basis_inverse;
  mutable usize last_hit{ 0 };
  extension_mode mode{ extension_mode::clamp };
};

template<ieee754_floating F, usize D>
inline void
rebuild_basis_cache(bspline_curve_nd<F, D> &s) noexcept
{
  __impl_bspline::prepare_basis_inverse<F>(s.knots.data(), s.ctrl.size(), s.degree, s.__basis_inverse);
}

template<ieee754_floating F, usize D>
inline void
rebuild_basis_cache(nurbs_curve_nd<F, D> &s) noexcept
{
  __impl_bspline::prepare_basis_inverse<F>(s.knots.data(), s.ctrl.size(), s.degree, s.__basis_inverse);
}

template<ieee754_floating F, usize D>
inline void
invalidate_basis_cache(bspline_curve_nd<F, D> &s) noexcept
{
  s.__basis_inverse.set_size(0);
}

template<ieee754_floating F, usize D>
inline void
invalidate_basis_cache(nurbs_curve_nd<F, D> &s) noexcept
{
  s.__basis_inverse.set_size(0);
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline spline_domain<F>
domain(const bspline_curve_nd<F, D> &s) noexcept
{
  return s.ctrl.size() ? spline_domain<F>{ s.knots[s.degree], s.knots[s.ctrl.size()], true } : spline_domain<F>{};
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline spline_domain<F>
domain(const nurbs_curve_nd<F, D> &s) noexcept
{
  return s.ctrl.size() ? spline_domain<F>{ s.knots[s.degree], s.knots[s.ctrl.size()], true } : spline_domain<F>{};
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline bspline_curve_nd<F, D>
make_bspline_curve_from_ctrl(raw_slice<const F> knots, const vec<F, D> *ctrl, usize n_ctrl, u32 degree,
                             build_info<F> *info = nullptr) noexcept
{
  bspline_curve_nd<F, D> out{};
  out.degree = degree;
  if ( !ctrl || !__impl_bspline_helpers::valid_knots<F>(knots, n_ctrl, degree) ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  out.knots.reserve(knots.size());
  out.ctrl.reserve(n_ctrl);
  for ( usize i = 0; i < knots.size(); ++i ) out.knots.emplace_back(knots[i]);
  for ( usize i = 0; i < n_ctrl; ++i ) out.ctrl.emplace_back(ctrl[i]);
  rebuild_basis_cache(out);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline nurbs_curve_nd<F, D>
make_nurbs_curve(raw_slice<const F> knots, const vec<F, D> *ctrl, raw_slice<const F> weights, usize n_ctrl, u32 degree,
                 build_info<F> *info = nullptr) noexcept
{
  nurbs_curve_nd<F, D> out{};
  out.degree = degree;
  if ( !ctrl || weights.size() != n_ctrl || !__impl_bspline_helpers::valid_knots<F>(knots, n_ctrl, degree) ) {
    if ( info ) info->status = weights.size() != n_ctrl ? build_status::size_mismatch : build_status::invalid_argument;
    return out;
  }
  for ( usize i = 0; i < n_ctrl; ++i )
    if ( !(weights[i] > F(0)) ) {
      if ( info ) info->status = build_status::invalid_argument;
      return out;
    }
  out.knots.reserve(knots.size());
  out.ctrl.reserve(n_ctrl);
  out.weights.reserve(n_ctrl);
  for ( usize i = 0; i < knots.size(); ++i ) out.knots.emplace_back(knots[i]);
  for ( usize i = 0; i < n_ctrl; ++i ) {
    out.ctrl.emplace_back(ctrl[i]);
    out.weights.emplace_back(weights[i]);
  }
  rebuild_basis_cache(out);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline bspline_curve_nd<F, D>
make_closed_bspline_curve(const vec<F, D> *ctrl, usize n_ctrl, u32 degree, build_info<F> *info = nullptr) noexcept
{
  bspline_curve_nd<F, D> out{};
  if ( !ctrl || degree == 0 || degree > bspline_max_degree || n_ctrl < degree + 1 ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  const usize extended = n_ctrl + degree;
  out.degree = degree;
  out.mode = extension_mode::periodic;
  out.knots = make_periodic_uniform_knots<F>(extended, degree, F(0), F(n_ctrl));
  out.ctrl.reserve(extended);
  for ( usize i = 0; i < extended; ++i ) out.ctrl.emplace_back(ctrl[i % n_ctrl]);
  rebuild_basis_cache(out);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline nurbs_curve_nd<F, D>
make_closed_nurbs_curve(const vec<F, D> *ctrl, raw_slice<const F> weights, usize n_ctrl, u32 degree, build_info<F> *info = nullptr) noexcept
{
  nurbs_curve_nd<F, D> out{};
  if ( !ctrl || weights.size() != n_ctrl || degree == 0 || degree > bspline_max_degree || n_ctrl < degree + 1 ) {
    if ( info ) info->status = weights.size() != n_ctrl ? build_status::size_mismatch : build_status::invalid_argument;
    return out;
  }
  for ( usize i = 0; i < n_ctrl; ++i )
    if ( !(weights[i] > F(0)) ) {
      if ( info ) info->status = build_status::invalid_argument;
      return out;
    }
  const usize extended = n_ctrl + degree;
  out.degree = degree;
  out.mode = extension_mode::periodic;
  out.knots = make_periodic_uniform_knots<F>(extended, degree, F(0), F(n_ctrl));
  out.ctrl.reserve(extended);
  out.weights.reserve(extended);
  for ( usize i = 0; i < extended; ++i ) {
    out.ctrl.emplace_back(ctrl[i % n_ctrl]);
    out.weights.emplace_back(weights[i % n_ctrl]);
  }
  rebuild_basis_cache(out);
  if ( info ) info->status = build_status::ok;
  return out;
}

namespace __impl_curve_extensions
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
map_parameter(F value, F lo, F hi, extension_mode mode) noexcept
{
  if ( mode == extension_mode::periodic ) return __impl_piecewise_1d::map_periodic<F>(value, lo, hi);
  if ( mode == extension_mode::reflect ) return __impl_piecewise_1d::map_reflect<F>(value, lo, hi);
  if ( mode == extension_mode::polynomial ) return value;
  if ( value < lo ) return lo;
  if ( value > hi ) return hi;
  return value;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline bool
outside_domain(F value, F lo, F hi) noexcept
{
  return value < lo || value > hi;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
reflection_derivative_sign(F value, F lo, F hi, u32 order) noexcept
{
  if ( (order & 1u) == 0 || (value >= lo && value <= hi) ) return F(1);
  const F width = hi - lo;
  if ( !(width > F(0)) ) return F(1);
  const F cell = math::mk::round_ns::floor<F>((value - lo) / width);
  const F half = math::mk::round_ns::floor<F>(cell * F(0.5));
  return cell - F(2) * half != F(0) ? F(-1) : F(1);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
binomial(u32 n, u32 k) noexcept
{
  if ( k > n ) return F(0);
  if ( k > n - k ) k = n - k;
  F value = F(1);
  for ( u32 i = 1; i <= k; ++i ) value = value * F(n - k + i) / F(i);
  return value;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate_value_unextended(const bspline_curve_nd<F, D> &s, F t, usize &last) noexcept
{
  vec<F, D> out{};
  const usize n_ctrl = s.ctrl.size();
  const usize span = __impl_bspline::bspline_span<F>(s.knots.data(), n_ctrl, s.degree, t, last);
  F local[bspline_max_degree + 1];
  __impl_bspline::bspline_basis_cached<F>(s.knots.data(), n_ctrl, span, s.degree, t, s.__basis_inverse, local);
  for ( u32 j = 0; j <= s.degree; ++j ) {
    const auto &point = s.ctrl[span - s.degree + j];
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(local[j], point[d], out[d]);
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate_value_unextended(const nurbs_curve_nd<F, D> &s, F t, usize &last) noexcept
{
  vec<F, D> out{};
  const usize n_ctrl = s.ctrl.size();
  const usize span = __impl_bspline::bspline_span<F>(s.knots.data(), n_ctrl, s.degree, t, last);
  F local[bspline_max_degree + 1];
  __impl_bspline::bspline_basis_cached<F>(s.knots.data(), n_ctrl, span, s.degree, t, s.__basis_inverse, local);
  F denominator = F(0);
  for ( u32 j = 0; j <= s.degree; ++j ) {
    const usize index = span - s.degree + j;
    const F coefficient = local[j] * s.weights[index];
    denominator += coefficient;
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(coefficient, s.ctrl[index][d], out[d]);
  }
  if ( denominator != F(0) ) {
    const F inverse = F(1) / denominator;
    for ( usize d = 0; d < D; ++d ) out[d] *= inverse;
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate_unextended(const bspline_curve_nd<F, D> &s, F t, u32 order, usize &last) noexcept
{
  vec<F, D> out{};
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || order > s.degree ) return out;
  const usize span = __impl_bspline::bspline_span<F>(s.knots.data(), n_ctrl, s.degree, t, last);
  F local[bspline_max_degree + 1]{};
  if ( order == 0 ) {
    __impl_bspline::bspline_basis_cached<F>(s.knots.data(), n_ctrl, span, s.degree, t, s.__basis_inverse, local);
  } else {
    F derivatives[bspline_max_degree + 1][bspline_max_degree + 1]{};
    __impl_bspline_helpers::local_basis_derivatives<F>(s.knots.data(), span, s.degree, t, order, derivatives);
    for ( u32 j = 0; j <= s.degree; ++j ) local[j] = derivatives[order][j];
  }
  for ( u32 j = 0; j <= s.degree; ++j ) {
    const auto &point = s.ctrl[span - s.degree + j];
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(local[j], point[d], out[d]);
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate_unextended(const nurbs_curve_nd<F, D> &s, F t, u32 order, usize &last) noexcept
{
  vec<F, D> zero{};
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || order > s.degree ) return zero;
  const usize span = __impl_bspline::bspline_span<F>(s.knots.data(), n_ctrl, s.degree, t, last);
  F derivatives[bspline_max_degree + 1][bspline_max_degree + 1]{};
  if ( order == 0 ) {
    __impl_bspline::bspline_basis_cached<F>(s.knots.data(), n_ctrl, span, s.degree, t, s.__basis_inverse, derivatives[0]);
  } else {
    __impl_bspline_helpers::local_basis_derivatives<F>(s.knots.data(), span, s.degree, t, order, derivatives);
  }
  F weight_derivative[bspline_max_degree + 1]{};
  vec<F, D> numerator_derivative[bspline_max_degree + 1]{};
  vec<F, D> curve_derivative[bspline_max_degree + 1]{};
  for ( u32 k = 0; k <= order; ++k ) {
    for ( u32 j = 0; j <= s.degree; ++j ) {
      const usize index = span - s.degree + j;
      const F coefficient = derivatives[k][j] * s.weights[index];
      weight_derivative[k] += coefficient;
      for ( usize d = 0; d < D; ++d ) numerator_derivative[k][d] = math::fma<F>(coefficient, s.ctrl[index][d], numerator_derivative[k][d]);
    }
  }
  if ( weight_derivative[0] == F(0) ) return zero;
  const F inverse_weight = F(1) / weight_derivative[0];
  for ( u32 k = 0; k <= order; ++k ) {
    curve_derivative[k] = numerator_derivative[k];
    for ( u32 i = 1; i <= k; ++i ) {
      const F factor = binomial<F>(k, i) * weight_derivative[i];
      for ( usize d = 0; d < D; ++d ) curve_derivative[k][d] -= factor * curve_derivative[k - i][d];
    }
    for ( usize d = 0; d < D; ++d ) curve_derivative[k][d] *= inverse_weight;
  }
  return curve_derivative[order];
}

template<ieee754_floating F>
[[nodiscard]] inline bool
factor_gband(F *__restrict__ matrix, usize lower, usize upper, usize n) noexcept
{
  const usize bands = lower + upper + 1;
  auto at = [&](usize band, usize column) -> F & { return matrix[band * n + column]; };
  for ( usize pivot = 0; pivot + 1 < n; ++pivot ) {
    const F diagonal = at(upper, pivot);
    if ( diagonal == F(0) ) return false;
    const F inverse = F(1) / diagonal;
    const usize row_end = pivot + lower < n - 1 ? pivot + lower : n - 1;
    const usize column_end = pivot + upper < n - 1 ? pivot + upper : n - 1;
    for ( usize row = pivot + 1; row <= row_end; ++row ) {
      const usize lower_band = row + upper - pivot;
      const F multiplier = at(lower_band, pivot) * inverse;
      at(lower_band, pivot) = multiplier;
      for ( usize column = pivot + 1; column <= column_end; ++column ) {
        if ( column > row + upper ) continue;
        const usize target_band = row + upper - column;
        if ( target_band < bands ) at(target_band, column) -= multiplier * at(pivot + upper - column, column);
      }
    }
  }
  return n == 0 || at(upper, n - 1) != F(0);
}

template<ieee754_floating F, usize D>
inline void
solve_gband_factored(const F *__restrict__ matrix, usize lower, usize upper, usize n, vec<F, D> *__restrict__ rhs) noexcept
{
  auto at = [&](usize band, usize column) -> const F & { return matrix[band * n + column]; };
  for ( usize row = 0; row < n; ++row ) {
    const usize begin = row > lower ? row - lower : 0;
    for ( usize column = begin; column < row; ++column ) {
      const F multiplier = at(row + upper - column, column);
      for ( usize d = 0; d < D; ++d ) rhs[row][d] = math::fma<F>(-multiplier, rhs[column][d], rhs[row][d]);
    }
  }
  for ( usize row = n; row-- > 0; ) {
    const usize end = row + upper < n - 1 ? row + upper : n - 1;
    for ( usize column = row + 1; column <= end; ++column ) {
      const F multiplier = at(row + upper - column, column);
      for ( usize d = 0; d < D; ++d ) rhs[row][d] = math::fma<F>(-multiplier, rhs[column][d], rhs[row][d]);
    }
    const F inverse = F(1) / at(upper, row);
    for ( usize d = 0; d < D; ++d ) rhs[row][d] *= inverse;
  }
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline bool
lsq_curve_qr(raw_slice<const F> xs, const vec<F, D> *points, raw_slice<const F> weights, raw_slice<const F> knots, usize n_ctrl, u32 degree,
             vec<F, D> *solution) noexcept
{
  const usize width = usize(degree + 1);
  vector<F> upper(n_ctrl * width, F(0));
  upper.set_size(n_ctrl * width);
  vector<vec<F, D>> transformed(n_ctrl, vec<F, D>{});
  transformed.set_size(n_ctrl);
  usize last = degree;
  for ( usize sample = 0; sample < xs.size(); ++sample ) {
    const usize span = __impl_bspline::bspline_span<F>(knots.ptr, n_ctrl, degree, xs[sample], last);
    const usize first = span - degree;
    F row[bspline_max_degree + 1]{};
    __impl_bspline::bspline_basis<F>(knots.ptr, span, degree, xs[sample], row);
    const F scale = weights.size() ? F(math::fsqrt(weights[sample])) : F(1);
    for ( u32 j = 0; j <= degree; ++j ) row[j] *= scale;
    vec<F, D> rhs{};
    for ( usize d = 0; d < D; ++d ) rhs[d] = scale * points[sample][d];
    for ( usize column = first; column <= span; ++column ) {
      const usize local = column - first;
      const F a = upper[column * width];
      const F b = row[local];
      if ( b == F(0) ) continue;
      const F radius = F(math::fsqrt(a * a + b * b));
      if ( !(radius > F(0)) ) continue;
      const F inverse_radius = F(1) / radius;
      const F cosine = a * inverse_radius;
      const F sine = b * inverse_radius;
      const usize end = span < column + degree ? span : column + degree;
      for ( usize j = column; j <= end; ++j ) {
        const usize upper_index = column * width + j - column;
        const usize row_index = j - first;
        const F old_upper = upper[upper_index];
        const F old_row = row[row_index];
        upper[upper_index] = math::fma<F>(sine, old_row, cosine * old_upper);
        row[row_index] = math::fma<F>(cosine, old_row, -sine * old_upper);
      }
      for ( usize d = 0; d < D; ++d ) {
        const F old_transformed = transformed[column][d];
        transformed[column][d] = math::fma<F>(sine, rhs[d], cosine * old_transformed);
        rhs[d] = math::fma<F>(cosine, rhs[d], -sine * old_transformed);
      }
    }
  }
  const F tiny = sizeof(F) <= 4 ? F(1e-12) : F(1e-28);
  for ( usize row = n_ctrl; row-- > 0; ) {
    solution[row] = transformed[row];
    const usize end = row + degree < n_ctrl - 1 ? row + degree : n_ctrl - 1;
    for ( usize column = row + 1; column <= end; ++column ) {
      const F multiplier = upper[row * width + column - row];
      for ( usize d = 0; d < D; ++d ) solution[row][d] = math::fma<F>(-multiplier, solution[column][d], solution[row][d]);
    }
    const F diagonal = upper[row * width];
    if ( !(math::fabs(diagonal) > tiny) ) return false;
    const F inverse = F(1) / diagonal;
    for ( usize d = 0; d < D; ++d ) solution[row][d] *= inverse;
  }
  return true;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline bool
lsq_curve_normal(raw_slice<const F> xs, const vec<F, D> *points, raw_slice<const F> weights, raw_slice<const F> knots, usize n_ctrl,
                 u32 degree, vec<F, D> *solution) noexcept
{
  const usize width = usize(degree + 1);
  vector<F> lower(n_ctrl * width, F(0));
  lower.set_size(n_ctrl * width);
  for ( usize i = 0; i < n_ctrl; ++i ) solution[i] = {};
  usize last = degree;
  for ( usize sample = 0; sample < xs.size(); ++sample ) {
    const usize span = __impl_bspline::bspline_span<F>(knots.ptr, n_ctrl, degree, xs[sample], last);
    const usize first = span - degree;
    F row[bspline_max_degree + 1]{};
    __impl_bspline::bspline_basis<F>(knots.ptr, span, degree, xs[sample], row);
    const F weight = weights.size() ? weights[sample] : F(1);
    for ( u32 j = 0; j <= degree; ++j ) {
      const usize column = first + j;
      const F projected = weight * row[j];
      for ( usize d = 0; d < D; ++d ) solution[column][d] = math::fma<F>(projected, points[sample][d], solution[column][d]);
      for ( u32 k = 0; k <= j; ++k ) {
        const usize other = first + k;
        lower[column * width + column - other] = math::fma<F>(projected, row[k], lower[column * width + column - other]);
      }
    }
  }
  for ( usize row = 0; row < n_ctrl; ++row ) {
    const usize begin = row > degree ? row - degree : 0;
    for ( usize column = begin; column <= row; ++column ) {
      F value = lower[row * width + row - column];
      usize k_begin = row > degree ? row - degree : 0;
      const usize column_begin = column > degree ? column - degree : 0;
      if ( column_begin > k_begin ) k_begin = column_begin;
      for ( usize k = k_begin; k < column; ++k ) value -= lower[row * width + row - k] * lower[column * width + column - k];
      if ( row == column ) {
        if ( !(value > F(0)) ) return false;
        lower[row * width] = F(math::fsqrt(value));
      } else {
        lower[row * width + row - column] = value / lower[column * width];
      }
    }
  }
  for ( usize row = 0; row < n_ctrl; ++row ) {
    const usize begin = row > degree ? row - degree : 0;
    for ( usize column = begin; column < row; ++column ) {
      const F multiplier = lower[row * width + row - column];
      for ( usize d = 0; d < D; ++d ) solution[row][d] = math::fma<F>(-multiplier, solution[column][d], solution[row][d]);
    }
    const F inverse = F(1) / lower[row * width];
    for ( usize d = 0; d < D; ++d ) solution[row][d] *= inverse;
  }
  for ( usize row = n_ctrl; row-- > 0; ) {
    const usize end = row + degree < n_ctrl - 1 ? row + degree : n_ctrl - 1;
    for ( usize column = row + 1; column <= end; ++column ) {
      const F multiplier = lower[column * width + column - row];
      for ( usize d = 0; d < D; ++d ) solution[row][d] = math::fma<F>(-multiplier, solution[column][d], solution[row][d]);
    }
    const F inverse = F(1) / lower[row * width];
    for ( usize d = 0; d < D; ++d ) solution[row][d] *= inverse;
  }
  return true;
}

};      // namespace __impl_curve_extensions

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate(const bspline_curve_nd<F, D> &s, F t) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || s.degree == 0 ) return {};
  const F lo = s.knots[s.degree];
  const F hi = s.knots[n_ctrl];
  const bool outside = __impl_curve_extensions::outside_domain<F>(t, lo, hi);
  if ( !outside ) return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, s.last_hit);
  if ( outside && s.mode == extension_mode::zero ) return {};
  if ( outside && s.mode == extension_mode::linear ) {
    const F edge = t < lo ? lo : hi;
    auto out = __impl_curve_extensions::evaluate_value_unextended<F, D>(s, edge, s.last_hit);
    const auto slope = __impl_curve_extensions::evaluate_unextended<F, D>(s, edge, 1, s.last_hit);
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(slope[d], t - edge, out[d]);
    return out;
  }
  t = __impl_curve_extensions::map_parameter<F>(t, lo, hi, s.mode);
  return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, s.last_hit);
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate(const nurbs_curve_nd<F, D> &s, F t) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || s.degree == 0 ) return {};
  const F lo = s.knots[s.degree];
  const F hi = s.knots[n_ctrl];
  const bool outside = __impl_curve_extensions::outside_domain<F>(t, lo, hi);
  if ( !outside ) return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, s.last_hit);
  if ( outside && s.mode == extension_mode::zero ) return {};
  if ( outside && s.mode == extension_mode::linear ) {
    const F edge = t < lo ? lo : hi;
    auto out = __impl_curve_extensions::evaluate_value_unextended<F, D>(s, edge, s.last_hit);
    const auto slope = __impl_curve_extensions::evaluate_unextended<F, D>(s, edge, 1, s.last_hit);
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(slope[d], t - edge, out[d]);
    return out;
  }
  t = __impl_curve_extensions::map_parameter<F>(t, lo, hi, s.mode);
  return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, s.last_hit);
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const bspline_curve_nd<F, D> &s, F t, spline_cursor &cursor) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || s.degree == 0 ) return {};
  const F lo = s.knots[s.degree];
  const F hi = s.knots[n_ctrl];
  const bool outside = __impl_curve_extensions::outside_domain<F>(t, lo, hi);
  if ( !outside ) return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, cursor.segment);
  if ( outside && s.mode == extension_mode::zero ) return {};
  if ( outside && s.mode == extension_mode::linear ) {
    const F edge = t < lo ? lo : hi;
    auto out = __impl_curve_extensions::evaluate_value_unextended<F, D>(s, edge, cursor.segment);
    const auto slope = __impl_curve_extensions::evaluate_unextended<F, D>(s, edge, 1, cursor.segment);
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(slope[d], t - edge, out[d]);
    return out;
  }
  t = __impl_curve_extensions::map_parameter<F>(t, lo, hi, s.mode);
  return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, cursor.segment);
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const nurbs_curve_nd<F, D> &s, F t, spline_cursor &cursor) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || s.degree == 0 ) return {};
  const F lo = s.knots[s.degree];
  const F hi = s.knots[n_ctrl];
  const bool outside = __impl_curve_extensions::outside_domain<F>(t, lo, hi);
  if ( !outside ) return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, cursor.segment);
  if ( outside && s.mode == extension_mode::zero ) return {};
  if ( outside && s.mode == extension_mode::linear ) {
    const F edge = t < lo ? lo : hi;
    auto out = __impl_curve_extensions::evaluate_value_unextended<F, D>(s, edge, cursor.segment);
    const auto slope = __impl_curve_extensions::evaluate_unextended<F, D>(s, edge, 1, cursor.segment);
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(slope[d], t - edge, out[d]);
    return out;
  }
  t = __impl_curve_extensions::map_parameter<F>(t, lo, hi, s.mode);
  return __impl_curve_extensions::evaluate_value_unextended<F, D>(s, t, cursor.segment);
}

template<ieee754_floating F, usize D>
inline void
evaluate(const bspline_curve_nd<F, D> &s, const F *__restrict__ tq, vec<F, D> *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F, D>(s, tq[i]);
}

template<ieee754_floating F, usize D>
inline void
evaluate(const nurbs_curve_nd<F, D> &s, const F *__restrict__ tq, vec<F, D> *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F, D>(s, tq[i]);
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline bspline_curve_nd<F, D>
derivative_spline(const bspline_curve_nd<F, D> &s) noexcept
{
  bspline_curve_nd<F, D> out{};
  if ( s.degree == 0 || s.ctrl.size() < 2 ) return out;
  const u32 p = s.degree;
  out.degree = p - 1;
  out.mode = s.mode;
  out.knots.reserve(s.knots.size() - 2);
  for ( usize i = 1; i + 1 < s.knots.size(); ++i ) out.knots.emplace_back(s.knots[i]);
  out.ctrl.reserve(s.ctrl.size() - 1);
  for ( usize i = 0; i + 1 < s.ctrl.size(); ++i ) {
    vec<F, D> point{};
    const F denom = s.knots[i + p + 1] - s.knots[i + 1];
    if ( denom > F(0) )
      for ( usize d = 0; d < D; ++d ) point[d] = F(p) * (s.ctrl[i + 1][d] - s.ctrl[i][d]) / denom;
    out.ctrl.emplace_back(point);
  }
  rebuild_basis_cache(out);
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
derivative(const bspline_curve_nd<F, D> &s, F t, u32 order = 1) noexcept
{
  if ( order == 0 ) return evaluate<F, D>(s, t);
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || order > s.degree ) return {};
  const F lo = s.knots[s.degree];
  const F hi = s.knots[n_ctrl];
  const bool outside = __impl_curve_extensions::outside_domain<F>(t, lo, hi);
  if ( !outside ) return __impl_curve_extensions::evaluate_unextended<F, D>(s, t, order, s.last_hit);
  if ( outside && (s.mode == extension_mode::zero || s.mode == extension_mode::clamp) ) return {};
  if ( outside && s.mode == extension_mode::linear )
    return order == 1 ? __impl_curve_extensions::evaluate_unextended<F, D>(s, t < lo ? lo : hi, 1, s.last_hit) : vec<F, D>{};
  const F sign = s.mode == extension_mode::reflect ? __impl_curve_extensions::reflection_derivative_sign<F>(t, lo, hi, order) : F(1);
  t = __impl_curve_extensions::map_parameter<F>(t, lo, hi, s.mode);
  auto out = __impl_curve_extensions::evaluate_unextended<F, D>(s, t, order, s.last_hit);
  if ( sign != F(1) )
    for ( usize d = 0; d < D; ++d ) out[d] = -out[d];
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
derivative(const nurbs_curve_nd<F, D> &s, F t, u32 order = 1) noexcept
{
  const usize n_ctrl = s.ctrl.size();
  if ( n_ctrl == 0 || order > s.degree ) return order == 0 ? evaluate<F, D>(s, t) : vec<F, D>{};
  if ( order == 0 ) return evaluate<F, D>(s, t);
  const F lo = s.knots[s.degree];
  const F hi = s.knots[n_ctrl];
  const bool outside = __impl_curve_extensions::outside_domain<F>(t, lo, hi);
  if ( !outside ) return __impl_curve_extensions::evaluate_unextended<F, D>(s, t, order, s.last_hit);
  if ( outside && (s.mode == extension_mode::zero || s.mode == extension_mode::clamp) ) return {};
  if ( outside && s.mode == extension_mode::linear )
    return order == 1 ? __impl_curve_extensions::evaluate_unextended<F, D>(s, t < lo ? lo : hi, 1, s.last_hit) : vec<F, D>{};
  const F sign = s.mode == extension_mode::reflect ? __impl_curve_extensions::reflection_derivative_sign<F>(t, lo, hi, order) : F(1);
  t = __impl_curve_extensions::map_parameter<F>(t, lo, hi, s.mode);
  auto out = __impl_curve_extensions::evaluate_unextended<F, D>(s, t, order, s.last_hit);
  if ( sign != F(1) )
    for ( usize d = 0; d < D; ++d ) out[d] = -out[d];
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline bspline_curve_nd<F, D>
make_bspline_curve_interpolating(raw_slice<const F> ts, const vec<F, D> *points, usize n, u32 degree,
                                 build_info<F> *info = nullptr) noexcept
{
  bspline_curve_nd<F, D> out{};
  out.degree = degree;
  if ( degree == 0 || degree > bspline_max_degree ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  if ( !points || ts.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  if ( n < degree + 1 ) {
    if ( info ) info->status = build_status::too_few_points;
    return out;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(ts.ptr, n) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return out;
  }
  out.knots = make_averaged_clamped_knots<F>(ts.ptr, n, degree);
  if ( out.knots.size() != n + degree + 1 ) {
    if ( info ) info->status = build_status::degenerate;
    return out;
  }
  const usize width = usize(2 * degree + 1);
  vector<F> matrix(width * n, F(0));
  matrix.set_size(width * n);
  usize last = degree;
  F local[bspline_max_degree + 1];
  for ( usize row = 0; row < n; ++row ) {
    const usize span = __impl_bspline::bspline_span<F>(out.knots.data(), n, degree, ts[row], last);
    __impl_bspline::bspline_basis<F>(out.knots.data(), span, degree, ts[row], local);
    for ( u32 j = 0; j <= degree; ++j ) {
      const usize column = span - degree + j;
      matrix[(row + degree - column) * n + column] = local[j];
    }
  }
  if ( !__impl_curve_extensions::factor_gband<F>(matrix.data(), degree, degree, n) ) {
    if ( info ) info->status = build_status::singular_system;
    out.knots.set_size(0);
    return out;
  }
  out.ctrl.reserve(n);
  for ( usize i = 0; i < n; ++i ) out.ctrl.emplace_back(points[i]);
  __impl_curve_extensions::solve_gband_factored<F, D>(matrix.data(), degree, degree, n, out.ctrl.data());
  rebuild_basis_cache(out);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline bspline_curve_nd<F, D>
make_lsq_bspline_curve(raw_slice<const F> ts, const vec<F, D> *points, usize n, raw_slice<const F> knots, u32 degree,
                       raw_slice<const F> weights = {}, lsq_method method = lsq_method::householder_qr,
                       build_info<F> *info = nullptr) noexcept
{
  bspline_curve_nd<F, D> out{};
  if ( !points || ts.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  const usize n_ctrl = knots.size() > degree + 1 ? knots.size() - degree - 1 : 0;
  if ( weights.size() != 0 && weights.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  if ( n < n_ctrl || !__impl_bspline_helpers::valid_knots<F>(knots, n_ctrl, degree) ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(ts.ptr, n) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return out;
  }
  for ( usize i = 0; i < n; ++i ) {
    if ( ts[i] < knots[degree] || ts[i] > knots[n_ctrl] || (weights.size() && !(weights[i] > F(0))) ) {
      if ( info ) info->status = build_status::invalid_argument;
      return {};
    }
  }
  out.degree = degree;
  out.knots.reserve(knots.size());
  for ( usize i = 0; i < knots.size(); ++i ) out.knots.emplace_back(knots[i]);
  out.ctrl.reserve(n_ctrl);
  for ( usize i = 0; i < n_ctrl; ++i ) out.ctrl.emplace_back(vec<F, D>{});
  const bool solved = method == lsq_method::householder_qr
                          ? __impl_curve_extensions::lsq_curve_qr<F, D>(ts, points, weights, knots, n_ctrl, degree, out.ctrl.data())
                          : __impl_curve_extensions::lsq_curve_normal<F, D>(ts, points, weights, knots, n_ctrl, degree, out.ctrl.data());
  if ( !solved ) {
    if ( info ) info->status = build_status::singular_system;
    return {};
  }
  rebuild_basis_cache(out);
  if ( info ) info->status = build_status::ok;
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Bezier curves

template<ieee754_floating F, usize D> struct bezier_curve_nd {
  vector<vec<F, D>> ctrl;
  extension_mode mode{ extension_mode::clamp };
};

template<ieee754_floating F, usize D> struct rational_bezier_curve_nd {
  vector<vec<F, D>> ctrl;
  vector<F> weights;
  extension_mode mode{ extension_mode::clamp };
};

template<ieee754_floating F, usize D>
[[nodiscard]] inline bezier_curve_nd<F, D>
make_bezier_curve(const vec<F, D> *ctrl, usize n_ctrl, build_info<F> *info = nullptr) noexcept
{
  bezier_curve_nd<F, D> out{};
  if ( !ctrl || n_ctrl < 2 || n_ctrl > bspline_max_degree + 1 ) {
    if ( info ) info->status = n_ctrl < 2 ? build_status::too_few_points : build_status::invalid_argument;
    return out;
  }
  out.ctrl.reserve(n_ctrl);
  for ( usize i = 0; i < n_ctrl; ++i ) out.ctrl.emplace_back(ctrl[i]);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline rational_bezier_curve_nd<F, D>
make_rational_bezier_curve(const vec<F, D> *ctrl, raw_slice<const F> weights, usize n_ctrl, build_info<F> *info = nullptr) noexcept
{
  rational_bezier_curve_nd<F, D> out{};
  if ( !ctrl || n_ctrl < 2 || n_ctrl > bspline_max_degree + 1 || weights.size() != n_ctrl ) {
    if ( info ) info->status = weights.size() != n_ctrl ? build_status::size_mismatch : build_status::invalid_argument;
    return out;
  }
  out.ctrl.reserve(n_ctrl);
  out.weights.reserve(n_ctrl);
  for ( usize i = 0; i < n_ctrl; ++i ) {
    if ( !(weights[i] > F(0)) ) {
      if ( info ) info->status = build_status::invalid_argument;
      return {};
    }
    out.ctrl.emplace_back(ctrl[i]);
    out.weights.emplace_back(weights[i]);
  }
  if ( info ) info->status = build_status::ok;
  return out;
}

namespace __impl_curve_extensions
{

template<ieee754_floating F, usize D>
[[nodiscard, gnu::always_inline]] inline vec<F, D>
evaluate_bezier_unextended(const bezier_curve_nd<F, D> &s, F t) noexcept
{
  vec<F, D> scratch[bspline_max_degree + 1]{};
  const usize n = s.ctrl.size();
  for ( usize i = 0; i < n; ++i ) scratch[i] = s.ctrl[i];
  for ( usize level = 1; level < n; ++level )
    for ( usize i = 0; i + level < n; ++i )
      for ( usize d = 0; d < D; ++d ) scratch[i][d] = math::fma<F>(t, scratch[i + 1][d] - scratch[i][d], scratch[i][d]);
  return scratch[0];
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::always_inline]] inline vec<F, D>
evaluate_bezier_unextended(const rational_bezier_curve_nd<F, D> &s, F t) noexcept
{
  vec<F, D> points[bspline_max_degree + 1]{};
  F weights[bspline_max_degree + 1]{};
  const usize n = s.ctrl.size();
  for ( usize i = 0; i < n; ++i ) {
    weights[i] = s.weights[i];
    for ( usize d = 0; d < D; ++d ) points[i][d] = s.ctrl[i][d] * weights[i];
  }
  for ( usize level = 1; level < n; ++level )
    for ( usize i = 0; i + level < n; ++i ) {
      weights[i] = math::fma<F>(t, weights[i + 1] - weights[i], weights[i]);
      for ( usize d = 0; d < D; ++d ) points[i][d] = math::fma<F>(t, points[i + 1][d] - points[i][d], points[i][d]);
    }
  if ( weights[0] != F(0) ) {
    const F inverse = F(1) / weights[0];
    for ( usize d = 0; d < D; ++d ) points[0][d] *= inverse;
  }
  return points[0];
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::noinline]] vec<F, D>
evaluate_bezier_extended(const bezier_curve_nd<F, D> &s, F t) noexcept
{
  const usize n = s.ctrl.size();
  if ( s.mode == extension_mode::zero ) return {};
  if ( s.mode == extension_mode::linear ) {
    const usize edge = t < F(0) ? 0 : n - 1;
    vec<F, D> out = s.ctrl[edge];
    if ( n > 1 ) {
      const usize neighbor = edge == 0 ? 1 : edge - 1;
      const F direction = edge == 0 ? F(1) : F(-1);
      const F delta = t - (edge == 0 ? F(0) : F(1));
      for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(F(n - 1) * direction * (s.ctrl[neighbor][d] - s.ctrl[edge][d]), delta, out[d]);
    }
    return out;
  }
  return evaluate_bezier_unextended<F, D>(s, map_parameter<F>(t, F(0), F(1), s.mode));
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::noinline]] vec<F, D>
evaluate_bezier_extended(const rational_bezier_curve_nd<F, D> &s, F t) noexcept
{
  const usize n = s.ctrl.size();
  if ( s.mode == extension_mode::zero ) return {};
  if ( s.mode == extension_mode::linear ) {
    const usize edge = t < F(0) ? 0 : n - 1;
    vec<F, D> out = s.ctrl[edge];
    if ( n > 1 && s.weights[edge] != F(0) ) {
      const usize neighbor = edge == 0 ? 1 : edge - 1;
      const F direction = edge == 0 ? F(1) : F(-1);
      const F scale = F(n - 1) * direction * s.weights[neighbor] / s.weights[edge];
      const F delta = t - (edge == 0 ? F(0) : F(1));
      for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(scale * (s.ctrl[neighbor][d] - s.ctrl[edge][d]), delta, out[d]);
    }
    return out;
  }
  return evaluate_bezier_unextended<F, D>(s, map_parameter<F>(t, F(0), F(1), s.mode));
}

};      // namespace __impl_curve_extensions

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const bezier_curve_nd<F, D> &s, F t) noexcept
{
  const usize n = s.ctrl.size();
  if ( n == 0 ) return {};
  if ( t >= F(0) && t <= F(1) ) return __impl_curve_extensions::evaluate_bezier_unextended<F, D>(s, t);
  return __impl_curve_extensions::evaluate_bezier_extended<F, D>(s, t);
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const rational_bezier_curve_nd<F, D> &s, F t) noexcept
{
  const usize n = s.ctrl.size();
  if ( n == 0 ) return {};
  if ( t >= F(0) && t <= F(1) ) return __impl_curve_extensions::evaluate_bezier_unextended<F, D>(s, t);
  return __impl_curve_extensions::evaluate_bezier_extended<F, D>(s, t);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// closed and packed cubic curves

template<ieee754_floating F, usize D> struct closed_cubic_curve_nd {
  cubic_curve_nd<F, D> curve;
  F period{ F(0) };
};

template<ieee754_floating F, usize D>
[[nodiscard]] inline spline_domain<F>
domain(const closed_cubic_curve_nd<F, D> &s) noexcept
{
  return s.curve.ts.size() ? spline_domain<F>{ s.curve.ts[0], s.curve.ts[s.curve.ts.size() - 1], true } : spline_domain<F>{};
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline closed_cubic_curve_nd<F, D>
make_closed_cubic_curve(raw_slice<const F> ts, const vec<F, D> *points, usize n, build_info<F> *info = nullptr) noexcept
{
  closed_cubic_curve_nd<F, D> out{};
  if ( !points || ts.size() != n || n < 4 || !__impl_splines_bits::strictly_increasing<F>(ts.ptr, n) ) {
    if ( info ) info->status = ts.size() != n ? build_status::size_mismatch : build_status::invalid_argument;
    return out;
  }
  out.curve.ts.reserve(n);
  out.curve.seg.reserve(n - 1);
  for ( usize i = 0; i < n; ++i ) out.curve.ts.emplace_back(ts[i]);
  const curve_seg<F, D> zero{};
  for ( usize i = 1; i < n; ++i ) out.curve.seg.emplace_back(zero);
  for ( usize d = 0; d < D; ++d ) {
    const F scale = F(1) + __impl_piecewise_1d::abs_value<F>(points[0][d]) + __impl_piecewise_1d::abs_value<F>(points[n - 1][d]);
    const F tolerance = (sizeof(F) <= 4 ? F(1e-5) : F(1e-12)) * scale;
    if ( __impl_piecewise_1d::abs_value<F>(points[0][d] - points[n - 1][d]) > tolerance ) {
      if ( info ) info->status = build_status::degenerate;
      return {};
    }
  }

  const usize m = n - 1;
  vector<F> lower(m - 1, F(0)), diagonal(m, F(0)), upper(m - 1, F(0));
  lower.set_size(m - 1);
  diagonal.set_size(m);
  upper.set_size(m - 1);
  vector<vec<F, D>> rhs(m, vec<F, D>{});
  rhs.set_size(m);
  for ( usize i = 0; i < m; ++i ) {
    const F hp = i == 0 ? ts[n - 1] - ts[n - 2] : ts[i] - ts[i - 1];
    const F hn = ts[i + 1] - ts[i];
    diagonal[i] = F(2) * (hp + hn);
    if ( i > 0 ) lower[i - 1] = hp;
    if ( i + 1 < m ) upper[i] = hn;
    for ( usize d = 0; d < D; ++d ) {
      const F previous = i == 0 ? (points[n - 1][d] - points[n - 2][d]) / hp : (points[i][d] - points[i - 1][d]) / hp;
      const F next = (points[i + 1][d] - points[i][d]) / hn;
      rhs[i][d] = F(6) * (next - previous);
    }
  }
  const F alpha = ts[n - 1] - ts[n - 2];
  const F beta = alpha;
  const F gamma = -diagonal[0];
  diagonal[0] -= gamma;
  diagonal[m - 1] -= alpha * beta / gamma;

  constexpr F tiny = default_eps<F>() * F(4);
  if ( !(math::fabs(diagonal[0]) > tiny) ) {
    if ( info ) info->status = build_status::singular_system;
    return {};
  }
  diagonal[0] = F(1) / diagonal[0];
  upper[0] *= diagonal[0];
  for ( usize i = 1; i < m; ++i ) {
    const F pivot = diagonal[i] - lower[i - 1] * upper[i - 1];
    if ( !(math::fabs(pivot) > tiny) ) {
      if ( info ) info->status = build_status::singular_system;
      return {};
    }
    diagonal[i] = F(1) / pivot;
    if ( i + 1 < m ) upper[i] *= diagonal[i];
  }
  auto solve_factored = [&](auto *values) noexcept {
    values[0] *= diagonal[0];
    for ( usize i = 1; i < m; ++i ) values[i] = (values[i] - values[i - 1] * lower[i - 1]) * diagonal[i];
    for ( usize i = m - 1; i-- > 0; ) values[i] -= values[i + 1] * upper[i];
  };
  solve_factored(rhs.data());
  vector<F> z(m, F(0));
  z.set_size(m);
  z[0] = gamma;
  z[m - 1] = alpha;
  solve_factored(z.data());
  for ( usize d = 0; d < D; ++d ) {
    const F correction = (rhs[0][d] + beta * rhs[m - 1][d] / gamma) / (F(1) + z[0] + beta * z[m - 1] / gamma);
    for ( usize i = 0; i < m; ++i ) rhs[i][d] -= correction * z[i];
  }

  const F sixth = F(1) / F(6);
  for ( usize i = 0; i < m; ++i ) {
    const F h = ts[i + 1] - ts[i];
    const F inverse_h = F(1) / h;
    const vec<F, D> &moment0 = rhs[i];
    const vec<F, D> &moment1 = i + 1 < m ? rhs[i + 1] : rhs[0];
    for ( usize d = 0; d < D; ++d ) {
      out.curve.seg[i].a[d] = points[i][d];
      out.curve.seg[i].b[d] = (points[i + 1][d] - points[i][d]) * inverse_h - h * (F(2) * moment0[d] + moment1[d]) * sixth;
      out.curve.seg[i].c[d] = F(0.5) * moment0[d];
      out.curve.seg[i].d[d] = (moment1[d] - moment0[d]) * inverse_h * sixth;
    }
  }
  out.period = ts[n - 1] - ts[0];
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const closed_cubic_curve_nd<F, D> &s, F t) noexcept
{
  if ( s.curve.ts.size() < 2 ) return {};
  return evaluate<F, D>(s.curve, __impl_piecewise_1d::map_periodic<F>(t, s.curve.ts[0], s.curve.ts[s.curve.ts.size() - 1]));
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const closed_cubic_curve_nd<F, D> &s, F t, spline_cursor &cursor) noexcept
{
  vec<F, D> out{};
  const usize n = s.curve.ts.size();
  if ( n < 2 ) return out;
  const F mapped = __impl_piecewise_1d::map_periodic<F>(t, s.curve.ts[0], s.curve.ts[n - 1]);
  if ( mapped <= s.curve.ts[0] ) return s.curve.seg[0].a;
  const usize segment = __impl_splines_bits::locate_segment<F>(s.curve.ts.data(), n, mapped, cursor.segment);
  const auto &polynomial = s.curve.seg[segment];
  __spline_arch::curve_horner(polynomial.a.data, polynomial.b.data, polynomial.c.data, polynomial.d.data, mapped - s.curve.ts[segment],
                              out.data, D);
  return out;
}

template<ieee754_floating F, usize D> struct packed_cubic_curve_nd {
  vector<F> ts;
  vector<F> coeff;
  mutable usize last_hit{ 0 };
  extrap mode{ extrap::linear_continue };
};

template<ieee754_floating F, usize D>
[[nodiscard]] inline spline_domain<F>
domain(const packed_cubic_curve_nd<F, D> &s) noexcept
{
  return s.ts.size() ? spline_domain<F>{ s.ts[0], s.ts[s.ts.size() - 1], true } : spline_domain<F>{};
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline packed_cubic_curve_nd<F, D>
pack(const cubic_curve_nd<F, D> &source) noexcept
{
  packed_cubic_curve_nd<F, D> out{};
  out.mode = source.mode;
  out.ts.reserve(source.ts.size());
  out.coeff.reserve(source.seg.size() * 4 * D);
  for ( usize i = 0; i < source.ts.size(); ++i ) out.ts.emplace_back(source.ts[i]);
  for ( usize i = 0; i < source.seg.size(); ++i ) {
    for ( usize d = 0; d < D; ++d ) out.coeff.emplace_back(source.seg[i].a[d]);
    for ( usize d = 0; d < D; ++d ) out.coeff.emplace_back(source.seg[i].b[d]);
    for ( usize d = 0; d < D; ++d ) out.coeff.emplace_back(source.seg[i].c[d]);
    for ( usize d = 0; d < D; ++d ) out.coeff.emplace_back(source.seg[i].d[d]);
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline packed_cubic_curve_nd<F, D>
make_packed_cubic_curve(raw_slice<const F> ts, const vec<F, D> *points, usize n, bc_kind bc = bc_kind::natural, vec<F, D> left_slope = {},
                        vec<F, D> right_slope = {}, build_info<F> *info = nullptr) noexcept
{
  auto curve = make_cubic_curve<F, D>(ts, points, n, bc, left_slope, right_slope, info);
  return pack<F, D>(curve);
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate(const packed_cubic_curve_nd<F, D> &s, F t) noexcept
{
  vec<F, D> out{};
  const usize n = s.ts.size();
  if ( n < 2 ) return out;
  const F *ts = s.ts.data();
  usize segment = 0;
  if ( t <= ts[0] ) {
    if ( s.mode == extrap::error_value ) return out;
    if ( s.mode == extrap::clamp_to_endpoints ) {
      for ( usize d = 0; d < D; ++d ) out[d] = s.coeff[d];
      return out;
    }
    const F dt = t - ts[0];
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(s.coeff[D + d], dt, s.coeff[d]);
    return out;
  } else if ( t >= ts[n - 1] ) {
    segment = n - 2;
    const F *p = s.coeff.data() + segment * 4 * D;
    const F h = ts[n - 1] - ts[n - 2];
    __spline_arch::packed_curve_horner(p, h, out.data, D);
    if ( s.mode == extrap::error_value ) return {};
    if ( s.mode == extrap::clamp_to_endpoints ) return out;
    const F dt = t - ts[n - 1];
    for ( usize d = 0; d < D; ++d ) {
      const F slope = math::fma<F>(F(3) * p[3 * D + d], h * h, math::fma<F>(F(2) * p[2 * D + d], h, p[D + d]));
      out[d] = math::fma<F>(slope, dt, out[d]);
    }
    return out;
  } else {
    segment = __impl_splines_bits::locate_segment<F>(ts, n, t, s.last_hit);
  }
  const F *p = s.coeff.data() + segment * 4 * D;
  const F u = t - ts[segment];
  __spline_arch::packed_curve_horner(p, u, out.data, D);
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
evaluate(const packed_cubic_curve_nd<F, D> &s, F t, spline_cursor &cursor) noexcept
{
  vec<F, D> out{};
  const usize n = s.ts.size();
  if ( n < 2 ) return out;
  const F *ts = s.ts.data();
  if ( t <= ts[0] ) {
    if ( s.mode == extrap::error_value ) return out;
    if ( s.mode == extrap::clamp_to_endpoints ) {
      for ( usize d = 0; d < D; ++d ) out[d] = s.coeff[d];
      return out;
    }
    const F dt = t - ts[0];
    for ( usize d = 0; d < D; ++d ) out[d] = math::fma<F>(s.coeff[D + d], dt, s.coeff[d]);
    return out;
  }
  if ( t >= ts[n - 1] ) {
    if ( s.mode == extrap::error_value ) return out;
    const usize segment = n - 2;
    const F *polynomial = s.coeff.data() + segment * 4 * D;
    const F width = ts[n - 1] - ts[segment];
    __spline_arch::packed_curve_horner(polynomial, width, out.data, D);
    if ( s.mode == extrap::clamp_to_endpoints ) return out;
    const F dt = t - ts[n - 1];
    for ( usize d = 0; d < D; ++d ) {
      const F slope
          = math::fma<F>(F(3) * polynomial[3 * D + d], width * width, math::fma<F>(F(2) * polynomial[2 * D + d], width, polynomial[D + d]));
      out[d] = math::fma<F>(slope, dt, out[d]);
    }
    return out;
  }
  const usize segment = __impl_splines_bits::locate_segment<F>(ts, n, t, cursor.segment);
  __spline_arch::packed_curve_horner(s.coeff.data() + segment * 4 * D, t - ts[segment], out.data, D);
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// curve differential and parameter helpers

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
derivative(const cubic_curve_nd<F, D> &c, F t, u32 order = 1) noexcept
{
  vec<F, D> out{};
  const usize n = c.ts.size();
  if ( n < 2 || order > 3 ) return out;
  if ( order == 0 ) return evaluate<F, D>(c, t);
  const F lo = c.ts[0];
  const F hi = c.ts[n - 1];
  const bool below = t < lo;
  const bool above = t > hi;
  if ( below || above ) {
    if ( c.mode != extrap::linear_continue || order != 1 ) return out;
    const usize endpoint_segment = above ? n - 2 : 0;
    const F endpoint_offset = above ? hi - c.ts[endpoint_segment] : F(0);
    const auto &endpoint = c.seg[endpoint_segment];
    for ( usize d = 0; d < D; ++d )
      out[d] = math::fma<F>(math::fma<F>(F(3) * endpoint.d[d], endpoint_offset, F(2) * endpoint.c[d]), endpoint_offset, endpoint.b[d]);
    return out;
  }
  usize i = t >= hi ? n - 2 : 0;
  if ( t > c.ts[0] && t < c.ts[n - 1] ) i = __impl_splines_bits::locate_segment<F>(c.ts.data(), n, t, c.last_hit);
  const F u = t - c.ts[i];
  const auto &p = c.seg[i];
  for ( usize d = 0; d < D; ++d ) {
    if ( order == 1 )
      out[d] = math::fma<F>(math::fma<F>(F(3) * p.d[d], u, F(2) * p.c[d]), u, p.b[d]);
    else if ( order == 2 )
      out[d] = math::fma<F>(F(6) * p.d[d], u, F(2) * p.c[d]);
    else
      out[d] = F(6) * p.d[d];
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline F
speed(const cubic_curve_nd<F, D> &c, F t) noexcept
{
  const auto d = derivative<F, D>(c, t, 1);
  F sum = F(0);
  for ( usize i = 0; i < D; ++i ) sum = math::fma<F>(d[i], d[i], sum);
  return F(math::fsqrt(sum));
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vec<F, D>
tangent(const cubic_curve_nd<F, D> &c, F t) noexcept
{
  auto out = derivative<F, D>(c, t, 1);
  F squared = F(0);
  for ( usize i = 0; i < D; ++i ) squared = math::fma<F>(out[i], out[i], squared);
  if ( squared > F(0) ) {
    const F inverse = F(1) / F(math::fsqrt(squared));
    for ( usize i = 0; i < D; ++i ) out[i] *= inverse;
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline F
curvature(const cubic_curve_nd<F, D> &c, F t) noexcept
{
  const auto d1 = derivative<F, D>(c, t, 1);
  const auto d2 = derivative<F, D>(c, t, 2);
  F a = F(0), b = F(0), cross2 = F(0);
  for ( usize i = 0; i < D; ++i ) {
    a = math::fma<F>(d1[i], d1[i], a);
    b = math::fma<F>(d1[i], d2[i], b);
  }
  for ( usize i = 0; i < D; ++i ) {
    const F value = a * d2[i] - b * d1[i];
    cross2 = math::fma<F>(value, value, cross2);
  }
  if ( !(a > F(0)) ) return F(0);
  return F(math::fsqrt(cross2)) / (a * a);
}

template<ieee754_floating F>
[[nodiscard]] inline F
torsion(const cubic_curve_nd<F, 3> &c, F t) noexcept
{
  const auto a = derivative<F, 3>(c, t, 1);
  const auto b = derivative<F, 3>(c, t, 2);
  const auto d = derivative<F, 3>(c, t, 3);
  const F cx = a[1] * b[2] - a[2] * b[1];
  const F cy = a[2] * b[0] - a[0] * b[2];
  const F cz = a[0] * b[1] - a[1] * b[0];
  const F denom = cx * cx + cy * cy + cz * cz;
  return denom > F(0) ? (cx * d[0] + cy * d[1] + cz * d[2]) / denom : F(0);
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline F
arc_length(const cubic_curve_nd<F, D> &c, F a, F b, usize intervals = 128) noexcept
{
  if ( a == b || c.ts.size() < 2 ) return F(0);
  F sign = F(1);
  if ( a > b ) {
    const F swap = a;
    a = b;
    b = swap;
    sign = F(-1);
  }
  if ( intervals < 2 ) intervals = 2;
  if ( intervals & 1 ) ++intervals;
  const F h = (b - a) / F(intervals);
  F sum = speed<F, D>(c, a) + speed<F, D>(c, b);
  for ( usize i = 1; i < intervals; ++i ) sum += (i & 1 ? F(4) : F(2)) * speed<F, D>(c, a + h * F(i));
  return sign * h * sum / F(3);
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vector<F>
parameterize(const vec<F, D> *points, usize n, curve_parameterization method = curve_parameterization::chord_length) noexcept
{
  vector<F> out;
  if ( !points || n == 0 ) return out;
  out.reserve(n);
  out.emplace_back(F(0));
  for ( usize i = 1; i < n; ++i ) {
    F squared = F(0);
    for ( usize d = 0; d < D; ++d ) {
      const F delta = points[i][d] - points[i - 1][d];
      squared = math::fma<F>(delta, delta, squared);
    }
    F step = F(1);
    if ( method != curve_parameterization::uniform ) step = F(math::fsqrt(squared));
    if ( method == curve_parameterization::centripetal ) step = F(math::fsqrt(step));
    out.emplace_back(out[i - 1] + step);
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline vector<vec<F, D>>
sample_uniform_arc_length(const cubic_curve_nd<F, D> &c, usize count, usize integration_intervals = 128) noexcept
{
  vector<vec<F, D>> out;
  if ( count == 0 || c.ts.size() < 2 ) return out;
  out.reserve(count);
  const F lo = c.ts[0];
  const F hi = c.ts[c.ts.size() - 1];
  if ( count == 1 ) {
    out.emplace_back(evaluate<F, D>(c, lo));
    return out;
  }
  if ( integration_intervals < 2 ) integration_intervals = 2;
  vector<F> cumulative;
  vector<F> node_speed;
  cumulative.reserve(integration_intervals + 1);
  node_speed.reserve(integration_intervals + 1);
  cumulative.emplace_back(F(0));
  const F step = (hi - lo) / F(integration_intervals);
  F left_speed = speed<F, D>(c, lo);
  node_speed.emplace_back(left_speed);
  for ( usize interval = 0; interval < integration_intervals; ++interval ) {
    const F left = math::fma<F>(step, F(interval), lo);
    const F right = interval + 1 == integration_intervals ? hi : left + step;
    const F right_speed = speed<F, D>(c, right);
    const F midpoint_speed = speed<F, D>(c, F(0.5) * (left + right));
    const F length = (right - left) * (left_speed + F(4) * midpoint_speed + right_speed) / F(6);
    cumulative.emplace_back(cumulative[interval] + length);
    node_speed.emplace_back(right_speed);
    left_speed = right_speed;
  }
  const F total = cumulative[integration_intervals];
  for ( usize i = 0; i < count; ++i ) {
    if ( i == 0 ) {
      out.emplace_back(evaluate<F, D>(c, lo));
      continue;
    }
    if ( i + 1 == count ) {
      out.emplace_back(evaluate<F, D>(c, hi));
      continue;
    }
    const F target = total * F(i) / F(count - 1);
    usize first = 1, last = integration_intervals;
    while ( first < last ) {
      const usize middle = first + ((last - first) >> 1);
      if ( cumulative[middle] < target )
        first = middle + 1;
      else
        last = middle;
    }
    const usize interval = first - 1;
    const F left = math::fma<F>(step, F(interval), lo);
    const F right = interval + 1 == integration_intervals ? hi : left + step;
    const F local_target = target - cumulative[interval];
    const F interval_length = cumulative[interval + 1] - cumulative[interval];
    F parameter = interval_length > F(0) ? math::fma<F>(right - left, local_target / interval_length, left) : left;
    const F speed_left = node_speed[interval];
    for ( u32 iteration = 0; iteration < 3; ++iteration ) {
      const F speed_parameter = speed<F, D>(c, parameter);
      if ( !(speed_parameter > F(0)) ) break;
      const F width = parameter - left;
      const F partial = width * (speed_left + F(4) * speed<F, D>(c, F(0.5) * (left + parameter)) + speed_parameter) / F(6);
      F candidate = parameter - (partial - local_target) / speed_parameter;
      if ( candidate < left ) candidate = F(0.5) * (left + parameter);
      if ( candidate > right ) candidate = F(0.5) * (parameter + right);
      parameter = candidate;
    }
    out.emplace_back(evaluate<F, D>(c, parameter));
  }
  return out;
}

template<ieee754_floating F, usize D>
[[nodiscard]] inline cubic_curve_nd<F, D>
make_smoothing_curve(raw_slice<const F> ts, const vec<F, D> *points, usize n, raw_slice<const F> weights, F lambda,
                     build_info<F> *info = nullptr) noexcept
{
  cubic_curve_nd<F, D> out{};
  if ( !points || ts.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  if ( n < 4 ) {
    if ( info ) info->status = build_status::too_few_points;
    return out;
  }
  if ( weights.size() != 0 && weights.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  for ( usize i = 0; i < weights.size(); ++i )
    if ( !(weights[i] > F(0)) ) {
      if ( info ) info->status = build_status::invalid_argument;
      return out;
    }
  if ( !__impl_splines_bits::strictly_increasing<F>(ts.ptr, n) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return out;
  }
  if ( lambda == F(0) ) return make_cubic_curve<F, D>(ts, points, n, bc_kind::natural, {}, {}, info);

  const bool uniform_weights = weights.size() == 0;
  vector<F> values(n, F(0));
  values.set_size(n);
  for ( usize i = 0; i < n; ++i ) values[i] = points[i][0];
  __impl_smoothing::reinsch_workspace<F> workspace;
  __impl_smoothing::reinsch_precompute<F>(workspace, ts.ptr, values.data(), weights.ptr, uniform_weights, n);

  F smoothing = lambda;
  if ( lambda < F(0) ) {
    const F phi = (math::fsqrt(F(5)) - F(1)) * F(0.5);
    F lo = F(-12), hi = F(12);
    F left = hi - phi * (hi - lo);
    F right = lo + phi * (hi - lo);
    auto score = [&](F log_lambda) noexcept -> F {
      const F candidate = math::mk::exp_ns::exp10<F>(log_lambda);
      if ( !__impl_smoothing::reinsch_factor_lambda<F>(workspace, n, candidate) ) return F(1e30);
      F total_rss = F(0);
      for ( usize d = 0; d < D; ++d ) {
        for ( usize i = 0; i < n; ++i ) values[i] = points[i][d];
        __impl_smoothing::reinsch_update_rhs<F>(workspace, values.data(), n);
        F rss = F(0);
        __impl_smoothing::reinsch_apply_factored<F>(workspace, values.data(), n, candidate, &rss);
        total_rss += rss;
      }
      const F trace = __impl_smoothing::reinsch_trace_inverse_m<F>(workspace, n);
      return __impl_smoothing::gcv_score<F>(total_rss, candidate, trace, n);
    };
    F left_score = score(left);
    F right_score = score(right);
    usize iterations = 0;
    for ( ; iterations < 60 && hi - lo >= F(1e-3); ++iterations ) {
      if ( left_score < right_score ) {
        hi = right;
        right = left;
        right_score = left_score;
        left = hi - phi * (hi - lo);
        left_score = score(left);
      } else {
        lo = left;
        left = right;
        left_score = right_score;
        right = lo + phi * (hi - lo);
        right_score = score(right);
      }
    }
    smoothing = math::mk::exp_ns::exp10<F>(F(0.5) * (lo + hi));
    if ( info ) info->n_iterations = iterations + 2;
  }

  if ( !__impl_smoothing::reinsch_factor_lambda<F>(workspace, n, smoothing) ) {
    if ( info ) info->status = build_status::singular_system;
    return out;
  }
  vector<vec<F, D>> smoothed(n, vec<F, D>{});
  smoothed.set_size(n);
  F total_rss = F(0);
  for ( usize d = 0; d < D; ++d ) {
    for ( usize i = 0; i < n; ++i ) values[i] = points[i][d];
    __impl_smoothing::reinsch_update_rhs<F>(workspace, values.data(), n);
    F rss = F(0);
    __impl_smoothing::reinsch_apply_factored<F>(workspace, values.data(), n, smoothing, &rss);
    total_rss += rss;
    for ( usize i = 0; i < n; ++i ) smoothed[i][d] = workspace.y_hat[i];
  }
  build_info<F> cubic_info{};
  out = make_cubic_curve<F, D>(ts, smoothed.data(), n, bc_kind::natural, {}, {}, &cubic_info);
  if ( info ) {
    info->status = cubic_info.status;
    info->residual = lambda < F(0) ? smoothing : total_rss;
    if ( cubic_info.status != build_status::ok ) {
      info->n_iterations = 0;
    }
  }
  return out;
}

};      // namespace splines
};      // namespace math
};      // namespace micron
