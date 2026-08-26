//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tensor-product B-spline and NURBS surfaces/manifolds

#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../matrix/mat.hpp"
#include "../quants/vec.hpp"
#include "bspline_helpers.hpp"
#include "curve_extensions.hpp"
#include "policies.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<ieee754_floating F, typename V, usize P>
  requires(P >= 2 && P <= 4)
struct tensor_bspline {
  u32 degree[P]{};
  vector<F> knots[P];
  vector<F> __basis_inverse[P];
  usize shape[P]{};
  vector<V> ctrl;
  mutable usize last_hit[P]{};
  extension_mode mode[P]{};
};

template<ieee754_floating F, typename V, usize P>
  requires(P >= 2 && P <= 4)
struct tensor_nurbs {
  u32 degree[P]{};
  vector<F> knots[P];
  vector<F> __basis_inverse[P];
  usize shape[P]{};
  vector<V> ctrl;
  vector<F> weights;
  mutable usize last_hit[P]{};
  extension_mode mode[P]{};
};

template<ieee754_floating F, typename V, usize P>
inline void
rebuild_basis_cache(tensor_bspline<F, V, P> &s) noexcept
{
  for ( usize axis = 0; axis < P; ++axis )
    __impl_bspline::prepare_basis_inverse<F>(s.knots[axis].data(), s.shape[axis], s.degree[axis], s.__basis_inverse[axis]);
}

template<ieee754_floating F, typename V, usize P>
inline void
rebuild_basis_cache(tensor_nurbs<F, V, P> &s) noexcept
{
  for ( usize axis = 0; axis < P; ++axis )
    __impl_bspline::prepare_basis_inverse<F>(s.knots[axis].data(), s.shape[axis], s.degree[axis], s.__basis_inverse[axis]);
}

template<ieee754_floating F, typename V, usize P>
inline void
invalidate_basis_cache(tensor_bspline<F, V, P> &s) noexcept
{
  for ( usize axis = 0; axis < P; ++axis ) s.__basis_inverse[axis].set_size(0);
}

template<ieee754_floating F, typename V, usize P>
inline void
invalidate_basis_cache(tensor_nurbs<F, V, P> &s) noexcept
{
  for ( usize axis = 0; axis < P; ++axis ) s.__basis_inverse[axis].set_size(0);
}

namespace __impl_tensor_splines
{

template<typename V, ieee754_floating F>
[[gnu::always_inline]] inline void
scaled_add(V &out, const V &value, F scale) noexcept
{
  out += value * scale;
}

template<ieee754_floating F>
[[gnu::always_inline]] inline void
scaled_add(F &out, F value, F scale) noexcept
{
  out = math::fma<F>(value, scale, out);
}

template<typename V, ieee754_floating F>
[[gnu::always_inline]] inline void
scale_in_place(V &out, F scale) noexcept
{
  out *= scale;
}

template<ieee754_floating F>
[[gnu::always_inline]] inline void
scale_in_place(F &out, F scale) noexcept
{
  out *= scale;
}

template<ieee754_floating F, usize P>
[[nodiscard]] inline bool
validate_axes(const raw_slice<const F> (&knots)[P], const usize (&shape)[P], const u32 (&degree)[P], usize &total) noexcept
{
  total = 1;
  for ( usize axis = 0; axis < P; ++axis ) {
    if ( !__impl_bspline_helpers::valid_knots<F>(knots[axis], shape[axis], degree[axis]) ) return false;
    if ( shape[axis] != 0 && total > usize(-1) / shape[axis] ) return false;
    total *= shape[axis];
  }
  return true;
}

template<ieee754_floating F, usize P>
[[nodiscard]] inline bool
prepare_basis(const u32 (&degree)[P], const vector<F> (&knots)[P], const usize (&shape)[P], const extension_mode (&mode)[P],
              const vector<F> (&inverse)[P], usize (&last)[P], const F *coords, const u32 *orders, usize (&span)[P],
              F (&values)[P][bspline_max_degree + 1]) noexcept
{
  for ( usize axis = 0; axis < P; ++axis ) {
    const F original = coords[axis];
    const F lo = knots[axis][degree[axis]];
    const F hi = knots[axis][shape[axis]];
    const bool outside = __impl_curve_extensions::outside_domain<F>(original, lo, hi);
    const u32 requested_order = orders ? orders[axis] : 0;
    if ( outside && mode[axis] == extension_mode::zero ) return false;
    if ( outside && mode[axis] == extension_mode::clamp && requested_order != 0 ) {
      for ( u32 j = 0; j <= degree[axis]; ++j ) values[axis][j] = F(0);
      span[axis] = original < lo ? degree[axis] : shape[axis] - 1;
      continue;
    }
    const F x = outside ? __impl_curve_extensions::map_parameter<F>(original, lo, hi, mode[axis]) : original;
    span[axis] = __impl_bspline::bspline_span<F>(knots[axis].data(), shape[axis], degree[axis], x, last[axis]);
    if ( outside && mode[axis] == extension_mode::linear ) {
      if ( requested_order > 1 ) {
        for ( u32 j = 0; j <= degree[axis]; ++j ) values[axis][j] = F(0);
        continue;
      }
      F derivatives[bspline_max_degree + 1][bspline_max_degree + 1]{};
      __impl_bspline_helpers::local_basis_derivatives<F>(knots[axis].data(), span[axis], degree[axis], x, 1, derivatives);
      for ( u32 j = 0; j <= degree[axis]; ++j )
        values[axis][j] = requested_order == 1 ? derivatives[1][j] : math::fma<F>(original - x, derivatives[1][j], derivatives[0][j]);
    } else if ( requested_order != 0 ) {
      F derivatives[bspline_max_degree + 1][bspline_max_degree + 1]{};
      const u32 order = requested_order <= degree[axis] ? requested_order : degree[axis];
      __impl_bspline_helpers::local_basis_derivatives<F>(knots[axis].data(), span[axis], degree[axis], x, order, derivatives);
      const F sign = mode[axis] == extension_mode::reflect
                         ? __impl_curve_extensions::reflection_derivative_sign<F>(original, lo, hi, requested_order)
                         : F(1);
      for ( u32 j = 0; j <= degree[axis]; ++j )
        values[axis][j] = requested_order <= degree[axis] ? sign * derivatives[requested_order][j] : F(0);
    } else {
      __impl_bspline::bspline_basis_cached<F>(knots[axis].data(), shape[axis], span[axis], degree[axis], x, inverse[axis], values[axis]);
    }
  }
  return true;
}

template<ieee754_floating F, usize P>
[[nodiscard, gnu::always_inline]] inline bool
prepare_value_basis(const u32 (&degree)[P], const vector<F> (&knots)[P], const usize (&shape)[P], const extension_mode (&mode)[P],
                    const vector<F> (&inverse)[P], usize (&last)[P], const F *coords, usize (&span)[P],
                    F (&values)[P][bspline_max_degree + 1]) noexcept
{
  for ( usize axis = 0; axis < P; ++axis ) {
    const F lo = knots[axis][degree[axis]];
    const F hi = knots[axis][shape[axis]];
    if ( coords[axis] < lo || coords[axis] > hi )
      return prepare_basis<F, P>(degree, knots, shape, mode, inverse, last, coords, nullptr, span, values);
  }
  for ( usize axis = 0; axis < P; ++axis ) {
    const F x = coords[axis];
    span[axis] = __impl_bspline::bspline_span<F>(knots[axis].data(), shape[axis], degree[axis], x, last[axis]);
    __impl_bspline::bspline_basis_cached<F>(knots[axis].data(), shape[axis], span[axis], degree[axis], x, inverse[axis], values[axis]);
  }
  return true;
}

template<typename V, ieee754_floating F, usize P>
[[nodiscard]] inline V
contract(const u32 (&degree)[P], const usize (&shape)[P], const usize (&span)[P], const F (&values)[P][bspline_max_degree + 1],
         const V *ctrl, const F *weights, F *weight_sum = nullptr) noexcept
{
  V out{};
  const usize base0 = span[0] - degree[0];
  const usize base1 = span[1] - degree[1];
  if constexpr ( P == 2 ) {
    for ( u32 i = 0; i <= degree[0]; ++i ) {
      const F c0 = values[0][i];
      const usize row = (base0 + i) * shape[1] + base1;
      for ( u32 j = 0; j <= degree[1]; ++j ) {
        const usize index = row + j;
        F coefficient = c0 * values[1][j];
        if ( weights ) coefficient *= weights[index];
        scaled_add<V, F>(out, ctrl[index], coefficient);
        if ( weight_sum ) *weight_sum += coefficient;
      }
    }
  } else if constexpr ( P == 3 ) {
    const usize base2 = span[2] - degree[2];
    const usize stride0 = shape[1] * shape[2];
    for ( u32 i = 0; i <= degree[0]; ++i ) {
      const usize slab = (base0 + i) * stride0;
      const F c0 = values[0][i];
      for ( u32 j = 0; j <= degree[1]; ++j ) {
        const usize row = slab + (base1 + j) * shape[2] + base2;
        const F c01 = c0 * values[1][j];
        for ( u32 k = 0; k <= degree[2]; ++k ) {
          const usize index = row + k;
          F coefficient = c01 * values[2][k];
          if ( weights ) coefficient *= weights[index];
          scaled_add<V, F>(out, ctrl[index], coefficient);
          if ( weight_sum ) *weight_sum += coefficient;
        }
      }
    }
  } else {
    const usize base2 = span[2] - degree[2];
    const usize base3 = span[3] - degree[3];
    const usize stride1 = shape[2] * shape[3];
    const usize stride0 = shape[1] * stride1;
    for ( u32 i = 0; i <= degree[0]; ++i ) {
      const usize block0 = (base0 + i) * stride0;
      const F c0 = values[0][i];
      for ( u32 j = 0; j <= degree[1]; ++j ) {
        const usize block1 = block0 + (base1 + j) * stride1;
        const F c01 = c0 * values[1][j];
        for ( u32 k = 0; k <= degree[2]; ++k ) {
          const usize row = block1 + (base2 + k) * shape[3] + base3;
          const F c012 = c01 * values[2][k];
          for ( u32 l = 0; l <= degree[3]; ++l ) {
            const usize index = row + l;
            F coefficient = c012 * values[3][l];
            if ( weights ) coefficient *= weights[index];
            scaled_add<V, F>(out, ctrl[index], coefficient);
            if ( weight_sum ) *weight_sum += coefficient;
          }
        }
      }
    }
  }
  return out;
}

};      // namespace __impl_tensor_splines

template<ieee754_floating F, typename V, usize P>
[[nodiscard]] inline tensor_bspline<F, V, P>
make_tensor_bspline(const raw_slice<const F> (&knots)[P], const usize (&shape)[P], const u32 (&degree)[P], raw_slice<const V> ctrl,
                    build_info<F> *info = nullptr) noexcept
{
  tensor_bspline<F, V, P> out{};
  usize total = 0;
  if ( !__impl_tensor_splines::validate_axes<F, P>(knots, shape, degree, total) ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  if ( ctrl.size() != total ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  for ( usize axis = 0; axis < P; ++axis ) {
    out.degree[axis] = degree[axis];
    out.shape[axis] = shape[axis];
    out.mode[axis] = extension_mode::clamp;
    out.knots[axis].reserve(knots[axis].size());
    for ( usize i = 0; i < knots[axis].size(); ++i ) out.knots[axis].emplace_back(knots[axis][i]);
    __impl_bspline::prepare_basis_inverse<F>(out.knots[axis].data(), shape[axis], degree[axis], out.__basis_inverse[axis]);
  }
  out.ctrl.reserve(total);
  for ( usize i = 0; i < total; ++i ) out.ctrl.emplace_back(ctrl[i]);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, typename V, usize P>
[[nodiscard]] inline tensor_nurbs<F, V, P>
make_tensor_nurbs(const raw_slice<const F> (&knots)[P], const usize (&shape)[P], const u32 (&degree)[P], raw_slice<const V> ctrl,
                  raw_slice<const F> weights, build_info<F> *info = nullptr) noexcept
{
  tensor_nurbs<F, V, P> out{};
  usize total = 0;
  if ( !__impl_tensor_splines::validate_axes<F, P>(knots, shape, degree, total) ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  if ( ctrl.size() != total || weights.size() != total ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  for ( usize i = 0; i < total; ++i )
    if ( !(weights[i] > F(0)) ) {
      if ( info ) info->status = build_status::invalid_argument;
      return out;
    }
  for ( usize axis = 0; axis < P; ++axis ) {
    out.degree[axis] = degree[axis];
    out.shape[axis] = shape[axis];
    out.mode[axis] = extension_mode::clamp;
    out.knots[axis].reserve(knots[axis].size());
    for ( usize i = 0; i < knots[axis].size(); ++i ) out.knots[axis].emplace_back(knots[axis][i]);
    __impl_bspline::prepare_basis_inverse<F>(out.knots[axis].data(), shape[axis], degree[axis], out.__basis_inverse[axis]);
  }
  out.ctrl.reserve(total);
  out.weights.reserve(total);
  for ( usize i = 0; i < total; ++i ) {
    out.ctrl.emplace_back(ctrl[i]);
    out.weights.emplace_back(weights[i]);
  }
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F, typename V, usize P>
[[nodiscard, gnu::flatten]] inline V
evaluate(const tensor_bspline<F, V, P> &s, const F *coords) noexcept
{
  if ( s.ctrl.size() == 0 || !coords ) return {};
  usize span[P]{};
  F values[P][bspline_max_degree + 1]{};
  if ( !__impl_tensor_splines::prepare_value_basis<F, P>(s.degree, s.knots, s.shape, s.mode, s.__basis_inverse, s.last_hit, coords, span,
                                                         values) )
    return {};
  return __impl_tensor_splines::contract<V, F, P>(s.degree, s.shape, span, values, s.ctrl.data(), nullptr);
}

template<ieee754_floating F, typename V, usize P>
[[nodiscard, gnu::flatten]] inline V
evaluate(const tensor_nurbs<F, V, P> &s, const F *coords) noexcept
{
  if ( s.ctrl.size() == 0 || !coords ) return {};
  usize span[P]{};
  F values[P][bspline_max_degree + 1]{};
  if ( !__impl_tensor_splines::prepare_value_basis<F, P>(s.degree, s.knots, s.shape, s.mode, s.__basis_inverse, s.last_hit, coords, span,
                                                         values) )
    return {};
  F denominator = F(0);
  V out = __impl_tensor_splines::contract<V, F, P>(s.degree, s.shape, span, values, s.ctrl.data(), s.weights.data(), &denominator);
  if ( denominator != F(0) ) __impl_tensor_splines::scale_in_place<V, F>(out, F(1) / denominator);
  return out;
}

template<ieee754_floating F, typename V, usize P>
inline void
evaluate(const tensor_bspline<F, V, P> &s, const F *__restrict__ coords, V *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F, V, P>(s, coords + i * P);
}

template<ieee754_floating F, typename V, usize P>
inline void
evaluate(const tensor_nurbs<F, V, P> &s, const F *__restrict__ coords, V *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = evaluate<F, V, P>(s, coords + i * P);
}

template<ieee754_floating F, typename V, usize P>
[[nodiscard]] inline V
partial_derivative(const tensor_bspline<F, V, P> &s, const F *coords, const u32 *orders) noexcept
{
  if ( s.ctrl.size() == 0 || !coords || !orders ) return {};
  usize span[P]{};
  F values[P][bspline_max_degree + 1]{};
  if ( !__impl_tensor_splines::prepare_basis<F, P>(s.degree, s.knots, s.shape, s.mode, s.__basis_inverse, s.last_hit, coords, orders, span,
                                                   values) )
    return {};
  return __impl_tensor_splines::contract<V, F, P>(s.degree, s.shape, span, values, s.ctrl.data(), nullptr);
}

template<ieee754_floating F, typename V, usize P>
[[nodiscard]] inline V
partial_derivative(const tensor_nurbs<F, V, P> &s, const F *coords, usize axis) noexcept
{
  if ( s.ctrl.size() == 0 || !coords || axis >= P ) return {};
  usize span[P]{};
  F base[P][bspline_max_degree + 1]{};
  F deriv[P][bspline_max_degree + 1]{};
  u32 orders[P]{};
  if ( !__impl_tensor_splines::prepare_basis<F, P>(s.degree, s.knots, s.shape, s.mode, s.__basis_inverse, s.last_hit, coords, nullptr, span,
                                                   base) )
    return {};
  orders[axis] = 1;
  if ( !__impl_tensor_splines::prepare_basis<F, P>(s.degree, s.knots, s.shape, s.mode, s.__basis_inverse, s.last_hit, coords, orders, span,
                                                   deriv) )
    return {};
  F weight = F(0), weight_deriv = F(0);
  V value = __impl_tensor_splines::contract<V, F, P>(s.degree, s.shape, span, base, s.ctrl.data(), s.weights.data(), &weight);
  V value_deriv = __impl_tensor_splines::contract<V, F, P>(s.degree, s.shape, span, deriv, s.ctrl.data(), s.weights.data(), &weight_deriv);
  if ( weight == F(0) ) return {};
  __impl_tensor_splines::scale_in_place<V, F>(value_deriv, F(1) / weight);
  __impl_tensor_splines::scaled_add<V, F>(value_deriv, value, -weight_deriv / (weight * weight));
  return value_deriv;
}

template<ieee754_floating F, usize P>
[[nodiscard]] inline vec<F, P>
gradient(const tensor_bspline<F, F, P> &s, const F *coords) noexcept
{
  vec<F, P> out{};
  for ( usize axis = 0; axis < P; ++axis ) {
    u32 orders[P]{};
    orders[axis] = 1;
    out[axis] = partial_derivative<F, F, P>(s, coords, orders);
  }
  return out;
}

template<ieee754_floating F, usize P>
[[nodiscard]] inline mat<F, P, P>
hessian(const tensor_bspline<F, F, P> &s, const F *coords) noexcept
{
  mat<F, P, P> out{};
  for ( usize row = 0; row < P; ++row )
    for ( usize col = row; col < P; ++col ) {
      u32 orders[P]{};
      ++orders[row];
      ++orders[col];
      const F value = partial_derivative<F, F, P>(s, coords, orders);
      out.at(row, col) = value;
      out.at(col, row) = value;
    }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline vec<F, 3>
surface_normal(const tensor_bspline<F, vec<F, 3>, 2> &s, const F *coords) noexcept
{
  const u32 du[2] = { 1, 0 };
  const u32 dv[2] = { 0, 1 };
  const auto a = partial_derivative<F, vec<F, 3>, 2>(s, coords, du);
  const auto b = partial_derivative<F, vec<F, 3>, 2>(s, coords, dv);
  vec<F, 3> out{ a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
  const F squared = out[0] * out[0] + out[1] * out[1] + out[2] * out[2];
  if ( squared > F(0) ) out *= F(1) / F(math::fsqrt(squared));
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline vec<F, 3>
surface_normal(const tensor_nurbs<F, vec<F, 3>, 2> &s, const F *coords) noexcept
{
  const auto a = partial_derivative<F, vec<F, 3>, 2>(s, coords, 0);
  const auto b = partial_derivative<F, vec<F, 3>, 2>(s, coords, 1);
  vec<F, 3> out{ a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
  const F squared = out[0] * out[0] + out[1] * out[1] + out[2] * out[2];
  if ( squared > F(0) ) out *= F(1) / F(math::fsqrt(squared));
  return out;
}

};      // namespace splines
};      // namespace math
};      // namespace micron
