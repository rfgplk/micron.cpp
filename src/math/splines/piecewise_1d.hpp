//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// additive one-dimensional piecewise-polynomial spline families

#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../linalg/banded.hpp"
#include "../mk.hpp"
#include "bits/impl.hpp"
#include "cubic_1d.hpp"
#include "policies.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace splines
{

namespace __impl_piecewise_1d
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
abs_value(F x) noexcept
{
  return x < F(0) ? -x : x;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
map_periodic(F x, F lo, F hi) noexcept
{
  const F span = hi - lo;
  if ( !(span > F(0)) ) return lo;
  const F turns = math::mk::round_ns::floor<F>((x - lo) / span);
  F out = x - turns * span;
  if ( out < lo ) out += span;
  if ( !(out < hi) ) out = lo;
  return out;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
map_reflect(F x, F lo, F hi) noexcept
{
  const F span = hi - lo;
  if ( !(span > F(0)) ) return lo;
  const F period = span + span;
  const F turns = math::mk::round_ns::floor<F>((x - lo) / period);
  F out = x - turns * period;
  if ( out < lo ) out += period;
  if ( out > hi ) out = hi - (out - hi);
  return out;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
reflection_derivative_sign(F x, F lo, F hi, u32 order) noexcept
{
  if ( (order & 1u) == 0 || (x >= lo && x <= hi) ) return F(1);
  const F width = hi - lo;
  if ( !(width > F(0)) ) return F(1);
  const F cell = math::mk::round_ns::floor<F>((x - lo) / width);
  const F half = math::mk::round_ns::floor<F>(cell * F(0.5));
  return cell - F(2) * half != F(0) ? F(-1) : F(1);
}

template<ieee754_floating F>
[[nodiscard]] inline bool
validate_xy(raw_slice<const F> xs, raw_slice<const F> ys, usize minimum, build_info<F> *info) noexcept
{
  if ( xs.size() != ys.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return false;
  }
  if ( xs.size() < minimum ) {
    if ( info ) info->status = build_status::too_few_points;
    return false;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(xs.ptr, xs.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return false;
  }
  return true;
}

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_from_slopes(raw_slice<const F> xs, raw_slice<const F> ys, const F *slopes, build_info<F> *info) noexcept
{
  cubic_spline_1d<F> out{};
  out.bc = bc_kind::clamped;
  const usize n = xs.size();
  out.xs.reserve(n);
  out.seg.reserve(n - 1);
  for ( usize i = 0; i < n; ++i ) out.xs.emplace_back(xs[i]);
  const poly_coeffs<F, 3> zero{ { F(0), F(0), F(0), F(0) } };
  for ( usize i = 1; i < n; ++i ) out.seg.emplace_back(zero);
  __impl_splines_bits::build_cubic_segments_from_slopes<F>(xs.ptr, ys.ptr, slopes, out.seg.data(), n);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
eval_power(const F *coeff, u32 degree, F x) noexcept
{
  F value = coeff[degree];
  for ( u32 k = degree; k-- > 0; ) value = math::fma<F>(value, x, coeff[k]);
  return value;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
eval_power_derivative(const F *coeff, u32 degree, F x, u32 order) noexcept
{
  if ( order == 0 ) return eval_power<F>(coeff, degree, x);
  if ( order > degree ) return F(0);
  F value = F(1);
  for ( u32 j = 0; j < order; ++j ) value *= F(degree - j);
  value *= coeff[degree];
  for ( u32 k = degree; k-- > order; ) {
    F scale = F(1);
    for ( u32 j = 0; j < order; ++j ) scale *= F(k - j);
    value = math::fma<F>(value, x, scale * coeff[k]);
  }
  return value;
}

};      // namespace __impl_piecewise_1d

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// constant_1d

template<ieee754_floating F> struct constant_1d {
  vector<F> xs;
  vector<F> ys;
  mutable usize last_hit{ 0 };
  constant_side side{ constant_side::nearest };
  extension_mode mode{ extension_mode::clamp };
};

template<ieee754_floating F>
[[nodiscard]] inline constant_1d<F>
make_constant(raw_slice<const F> xs, raw_slice<const F> ys, constant_side side = constant_side::nearest,
              build_info<F> *info = nullptr) noexcept
{
  constant_1d<F> out{};
  out.side = side;
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 2, info) ) return out;
  out.xs.reserve(xs.size());
  out.ys.reserve(ys.size());
  for ( usize i = 0; i < xs.size(); ++i ) {
    out.xs.emplace_back(xs[i]);
    out.ys.emplace_back(ys[i]);
  }
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
evaluate(const constant_1d<F> &s, F x) noexcept
{
  const usize n = s.xs.size();
  if ( n == 0 ) return F(0);
  if ( n == 1 ) return s.ys[0];
  const F *xs = s.xs.data();
  const F *ys = s.ys.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  if ( x <= xs[0] ) return s.mode == extension_mode::zero && x < xs[0] ? F(0) : ys[0];
  if ( x >= xs[n - 1] ) return s.mode == extension_mode::zero && x > xs[n - 1] ? F(0) : ys[n - 1];
  const usize i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  if ( x == xs[i] ) return ys[i];
  if ( x == xs[i + 1] ) return ys[i + 1];
  if ( s.side == constant_side::previous ) return ys[i];
  if ( s.side == constant_side::next ) return ys[i + 1];
  return ((x + x) < (xs[i] + xs[i + 1])) ? ys[i] : ys[i + 1];
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const constant_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) return n ? s.ys[0] : F(0);
  const F *xs = s.xs.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  if ( x <= xs[0] ) return s.mode == extension_mode::zero && x < xs[0] ? F(0) : s.ys[0];
  if ( x >= xs[n - 1] ) return s.mode == extension_mode::zero && x > xs[n - 1] ? F(0) : s.ys[n - 1];
  const usize segment = __impl_splines_bits::locate_segment<F>(xs, n, x, cursor.segment);
  if ( x == xs[segment] ) return s.ys[segment];
  if ( x == xs[segment + 1] ) return s.ys[segment + 1];
  if ( s.side == constant_side::previous ) return s.ys[segment];
  if ( s.side == constant_side::next ) return s.ys[segment + 1];
  return ((x + x) < (xs[segment] + xs[segment + 1])) ? s.ys[segment] : s.ys[segment + 1];
}

template<ieee754_floating F>
inline void
evaluate(const constant_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
derivative(const constant_1d<F> &s, F x, u32 order = 1) noexcept
{
  return order == 0 ? evaluate<F>(s, x) : F(0);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// quadratic_spline_1d

template<ieee754_floating F> struct quadratic_spline_1d {
  vector<F> xs;
  vector<poly_coeffs<F, 2>> seg;
  mutable usize last_hit{ 0 };
  quadratic_boundary boundary{ quadratic_boundary::minimum_curvature };
  extension_mode mode{ extension_mode::linear };
};

template<ieee754_floating F>
[[nodiscard]] inline quadratic_spline_1d<F>
make_quadratic(raw_slice<const F> xs, raw_slice<const F> ys, quadratic_boundary boundary = quadratic_boundary::minimum_curvature,
               F boundary_slope = F(0), build_info<F> *info = nullptr) noexcept
{
  quadratic_spline_1d<F> out{};
  out.boundary = boundary;
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 2, info) ) return out;
  const usize n = xs.size();
  vector<F> delta;
  delta.reserve(n - 1);
  for ( usize i = 0; i + 1 < n; ++i ) delta.emplace_back((ys[i + 1] - ys[i]) / (xs[i + 1] - xs[i]));

  F first_slope = boundary_slope;
  if ( boundary == quadratic_boundary::right_slope ) {
    F a = (n & 1) ? F(1) : F(-1);
    F b = F(0);
    for ( usize i = 0; i + 1 < n; ++i ) b = F(2) * delta[i] - b;
    first_slope = (boundary_slope - b) / a;
  } else if ( boundary == quadratic_boundary::minimum_curvature ) {
    F numerator = F(0);
    F denominator = F(0);
    F a = F(1);
    F b = F(0);
    for ( usize i = 0; i + 1 < n; ++i ) {
      const F inv_h = F(1) / (xs[i + 1] - xs[i]);
      numerator += a * (delta[i] - b) * inv_h;
      denominator += inv_h;
      b = F(2) * delta[i] - b;
      a = -a;
    }
    first_slope = denominator > F(0) ? numerator / denominator : delta[0];
  }

  out.xs.reserve(n);
  out.seg.reserve(n - 1);
  for ( usize i = 0; i < n; ++i ) out.xs.emplace_back(xs[i]);
  F slope = first_slope;
  for ( usize i = 0; i + 1 < n; ++i ) {
    const F inv_h = F(1) / (xs[i + 1] - xs[i]);
    poly_coeffs<F, 2> p{ { ys[i], slope, (delta[i] - slope) * inv_h } };
    out.seg.emplace_back(p);
    slope = F(2) * delta[i] - slope;
  }
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
evaluate(const quadratic_spline_1d<F> &s, F x) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) return F(0);
  const F *xs = s.xs.data();
  const auto *seg = s.seg.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  if ( x <= xs[0] ) {
    if ( s.mode == extension_mode::zero && x < xs[0] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return seg[0][0];
    const F t = x - xs[0];
    if ( s.mode == extension_mode::linear ) return math::fma<F>(seg[0][1], t, seg[0][0]);
    return math::horner<F, 2>(seg[0], t);
  }
  if ( x >= xs[n - 1] ) {
    const auto &p = seg[n - 2];
    const F h = xs[n - 1] - xs[n - 2];
    const F y = math::horner<F, 2>(p, h);
    if ( s.mode == extension_mode::zero && x > xs[n - 1] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return y;
    if ( s.mode == extension_mode::linear ) return math::fma<F>(p[1] + F(2) * p[2] * h, x - xs[n - 1], y);
    return math::horner<F, 2>(p, x - xs[n - 2]);
  }
  const usize i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  return math::horner<F, 2>(seg[i], x - xs[i]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const quadratic_spline_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) return F(0);
  const F *xs = s.xs.data();
  const auto *segments = s.seg.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  if ( x <= xs[0] ) {
    if ( s.mode == extension_mode::zero && x < xs[0] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return segments[0][0];
    const F local = x - xs[0];
    return s.mode == extension_mode::linear ? math::fma<F>(segments[0][1], local, segments[0][0]) : math::horner<F, 2>(segments[0], local);
  }
  if ( x >= xs[n - 1] ) {
    const auto &polynomial = segments[n - 2];
    const F width = xs[n - 1] - xs[n - 2];
    const F value = math::horner<F, 2>(polynomial, width);
    if ( s.mode == extension_mode::zero && x > xs[n - 1] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return value;
    if ( s.mode == extension_mode::linear ) return math::fma<F>(polynomial[1] + F(2) * polynomial[2] * width, x - xs[n - 1], value);
    return math::horner<F, 2>(polynomial, x - xs[n - 2]);
  }
  const usize segment = __impl_splines_bits::locate_segment<F>(xs, n, x, cursor.segment);
  return math::horner<F, 2>(segments[segment], x - xs[segment]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
derivative(const quadratic_spline_1d<F> &s, F x, u32 order = 1) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 || order > 2 ) return F(0);
  if ( order == 0 ) return evaluate<F>(s, x);
  const F *xs = s.xs.data();
  const bool outside = x < xs[0] || x > xs[n - 1];
  if ( outside && (s.mode == extension_mode::zero || s.mode == extension_mode::clamp) ) return F(0);
  if ( outside && s.mode == extension_mode::linear ) {
    if ( order != 1 ) return F(0);
    if ( x < xs[0] ) return s.seg[0][1];
    const F width = xs[n - 1] - xs[n - 2];
    return math::fma<F>(F(2) * s.seg[n - 2][2], width, s.seg[n - 2][1]);
  }
  const F sign = s.mode == extension_mode::reflect ? __impl_piecewise_1d::reflection_derivative_sign<F>(x, xs[0], xs[n - 1], order) : F(1);
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  usize i = 0;
  if ( x >= xs[n - 1] )
    i = n - 2;
  else if ( x > xs[0] )
    i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  const F t = x - xs[i];
  const F value = order == 1 ? math::fma<F>(F(2) * s.seg[i][2], t, s.seg[i][1]) : F(2) * s.seg[i][2];
  return sign * value;
}

template<ieee754_floating F>
inline void
evaluate(const quadratic_spline_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cubic slope builders

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_cubic_hermite(raw_slice<const F> xs, raw_slice<const F> ys, raw_slice<const F> dydx, build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> out{};
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 2, info) ) return out;
  if ( dydx.size() != xs.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  return __impl_piecewise_1d::make_from_slopes<F>(xs, ys, dydx.ptr, info);
}

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_akima(raw_slice<const F> xs, raw_slice<const F> ys, akima_kind kind = akima_kind::akima, build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> out{};
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 2, info) ) return out;
  const usize n = xs.size();
  vector<F> slopes(n, F(0));
  slopes.set_size(n);
  if ( n == 2 ) {
    const F value = (ys[1] - ys[0]) / (xs[1] - xs[0]);
    slopes[0] = slopes[1] = value;
    return __impl_piecewise_1d::make_from_slopes<F>(xs, ys, slopes.data(), info);
  }

  vector<F> ext(n + 3, F(0));
  ext.set_size(n + 3);
  for ( usize i = 0; i + 1 < n; ++i ) ext[i + 2] = (ys[i + 1] - ys[i]) / (xs[i + 1] - xs[i]);
  ext[1] = F(2) * ext[2] - ext[3];
  ext[0] = F(2) * ext[1] - ext[2];
  ext[n + 1] = F(2) * ext[n] - ext[n - 1];
  ext[n + 2] = F(2) * ext[n + 1] - ext[n];
  for ( usize i = 0; i < n; ++i ) {
    F w1 = __impl_piecewise_1d::abs_value<F>(ext[i + 3] - ext[i + 2]);
    F w2 = __impl_piecewise_1d::abs_value<F>(ext[i + 1] - ext[i]);
    if ( kind == akima_kind::makima ) {
      w1 += F(0.5) * __impl_piecewise_1d::abs_value<F>(ext[i + 3] + ext[i + 2]);
      w2 += F(0.5) * __impl_piecewise_1d::abs_value<F>(ext[i + 1] + ext[i]);
    }
    const F sum = w1 + w2;
    slopes[i] = sum > F(0) ? (w1 * ext[i + 1] + w2 * ext[i + 2]) / sum : F(0.5) * (ext[i + 1] + ext[i + 2]);
  }
  return __impl_piecewise_1d::make_from_slopes<F>(xs, ys, slopes.data(), info);
}

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_cardinal(raw_slice<const F> xs, raw_slice<const F> ys, F tension = F(0), build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> out{};
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 2, info) ) return out;
  if ( tension < F(0) || tension > F(1) ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  const usize n = xs.size();
  vector<F> slopes(n, F(0));
  slopes.set_size(n);
  const F scale = F(1) - tension;
  slopes[0] = scale * (ys[1] - ys[0]) / (xs[1] - xs[0]);
  for ( usize i = 1; i + 1 < n; ++i ) slopes[i] = scale * (ys[i + 1] - ys[i - 1]) / (xs[i + 1] - xs[i - 1]);
  slopes[n - 1] = scale * (ys[n - 1] - ys[n - 2]) / (xs[n - 1] - xs[n - 2]);
  return __impl_piecewise_1d::make_from_slopes<F>(xs, ys, slopes.data(), info);
}

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_catmull_rom(raw_slice<const F> xs, raw_slice<const F> ys, build_info<F> *info = nullptr) noexcept
{
  return make_cardinal<F>(xs, ys, F(0), info);
}

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_steffen(raw_slice<const F> xs, raw_slice<const F> ys, build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> out{};
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 2, info) ) return out;
  const usize n = xs.size();
  vector<F> slopes(n, F(0));
  slopes.set_size(n);
  __impl_splines_bits::pchip_slopes<F>(xs.ptr, ys.ptr, slopes.data(), n);
  for ( usize i = 1; i + 1 < n; ++i ) {
    const F h0 = xs[i] - xs[i - 1];
    const F h1 = xs[i + 1] - xs[i];
    const F d0 = (ys[i] - ys[i - 1]) / h0;
    const F d1 = (ys[i + 1] - ys[i]) / h1;
    if ( d0 * d1 <= F(0) ) {
      slopes[i] = F(0);
      continue;
    }
    const F p = (d0 * h1 + d1 * h0) / (h0 + h1);
    F limit = __impl_piecewise_1d::abs_value<F>(d0);
    const F ad1 = __impl_piecewise_1d::abs_value<F>(d1);
    const F ap2 = F(0.5) * __impl_piecewise_1d::abs_value<F>(p);
    if ( ad1 < limit ) limit = ad1;
    if ( ap2 < limit ) limit = ap2;
    slopes[i] = (d0 > F(0) ? F(2) : F(-2)) * limit;
  }
  return __impl_piecewise_1d::make_from_slopes<F>(xs, ys, slopes.data(), info);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// quintic_spline_1d

template<ieee754_floating F> struct quintic_spline_1d {
  vector<F> xs;
  vector<poly_coeffs<F, 5>> seg;
  mutable usize last_hit{ 0 };
  extension_mode mode{ extension_mode::linear };
};

template<ieee754_floating F>
[[nodiscard]] inline quintic_spline_1d<F>
make_quintic_hermite(raw_slice<const F> xs, raw_slice<const F> ys, raw_slice<const F> dydx, raw_slice<const F> d2ydx2,
                     build_info<F> *info = nullptr) noexcept
{
  quintic_spline_1d<F> out{};
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 2, info) ) return out;
  if ( dydx.size() != xs.size() || d2ydx2.size() != xs.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  const usize n = xs.size();
  out.xs.reserve(n);
  out.seg.reserve(n - 1);
  for ( usize i = 0; i < n; ++i ) out.xs.emplace_back(xs[i]);
  for ( usize i = 0; i + 1 < n; ++i ) {
    const F h = xs[i + 1] - xs[i];
    const F h2 = h * h;
    const F inv_h = F(1) / h;
    const F inv_h2 = inv_h * inv_h;
    const F inv_h3 = inv_h2 * inv_h;
    const F A = ys[i + 1] - (ys[i] + dydx[i] * h + F(0.5) * d2ydx2[i] * h2);
    const F B = dydx[i + 1] - (dydx[i] + d2ydx2[i] * h);
    const F C = d2ydx2[i + 1] - d2ydx2[i];
    poly_coeffs<F, 5> p{};
    p[0] = ys[i];
    p[1] = dydx[i];
    p[2] = F(0.5) * d2ydx2[i];
    p[3] = (F(10) * A - F(4) * B * h + F(0.5) * C * h2) * inv_h3;
    p[4] = (-F(15) * A + F(7) * B * h - C * h2) * inv_h3 * inv_h;
    p[5] = (F(6) * A - F(3) * B * h + F(0.5) * C * h2) * inv_h3 * inv_h2;
    out.seg.emplace_back(p);
  }
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
evaluate(const quintic_spline_1d<F> &s, F x) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) return F(0);
  const F *xs = s.xs.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  usize i = 0;
  if ( x <= xs[0] ) {
    if ( s.mode == extension_mode::zero && x < xs[0] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return s.seg[0][0];
    if ( s.mode == extension_mode::linear ) return math::fma<F>(s.seg[0][1], x - xs[0], s.seg[0][0]);
  } else if ( x >= xs[n - 1] ) {
    i = n - 2;
    const F h = xs[n - 1] - xs[i];
    const F y = math::horner<F, 5>(s.seg[i], h);
    if ( s.mode == extension_mode::zero && x > xs[n - 1] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return y;
    if ( s.mode == extension_mode::linear ) {
      const F slope = __impl_piecewise_1d::eval_power_derivative<F>(s.seg[i].data, 5, h, 1);
      return math::fma<F>(slope, x - xs[n - 1], y);
    }
  } else {
    i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  }
  return math::horner<F, 5>(s.seg[i], x - xs[i]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const quintic_spline_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) return F(0);
  const F *xs = s.xs.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  if ( x <= xs[0] ) {
    if ( s.mode == extension_mode::zero && x < xs[0] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return s.seg[0][0];
    if ( s.mode == extension_mode::linear ) return math::fma<F>(s.seg[0][1], x - xs[0], s.seg[0][0]);
    return math::horner<F, 5>(s.seg[0], x - xs[0]);
  }
  if ( x >= xs[n - 1] ) {
    const usize segment = n - 2;
    const F width = xs[n - 1] - xs[segment];
    const F value = math::horner<F, 5>(s.seg[segment], width);
    if ( s.mode == extension_mode::zero && x > xs[n - 1] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return value;
    if ( s.mode == extension_mode::linear ) {
      const F slope = __impl_piecewise_1d::eval_power_derivative<F>(s.seg[segment].data, 5, width, 1);
      return math::fma<F>(slope, x - xs[n - 1], value);
    }
    return math::horner<F, 5>(s.seg[segment], x - xs[segment]);
  }
  const usize segment = __impl_splines_bits::locate_segment<F>(xs, n, x, cursor.segment);
  return math::horner<F, 5>(s.seg[segment], x - xs[segment]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
derivative(const quintic_spline_1d<F> &s, F x, u32 order = 1) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) return F(0);
  if ( order == 0 ) return evaluate<F>(s, x);
  const F *xs = s.xs.data();
  const bool outside = x < xs[0] || x > xs[n - 1];
  if ( outside && (s.mode == extension_mode::zero || s.mode == extension_mode::clamp) ) return F(0);
  if ( outside && s.mode == extension_mode::linear ) {
    if ( order != 1 ) return F(0);
    if ( x < xs[0] ) return s.seg[0][1];
    const usize segment = n - 2;
    return __impl_piecewise_1d::eval_power_derivative<F>(s.seg[segment].data, 5, xs[n - 1] - xs[segment], 1);
  }
  const F sign = s.mode == extension_mode::reflect ? __impl_piecewise_1d::reflection_derivative_sign<F>(x, xs[0], xs[n - 1], order) : F(1);
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, xs[0], xs[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, xs[0], xs[n - 1]);
  usize i = x >= xs[n - 1] ? n - 2 : 0;
  if ( x > xs[0] && x < xs[n - 1] ) i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  return sign * __impl_piecewise_1d::eval_power_derivative<F>(s.seg[i].data, 5, x - xs[i], order);
}

template<ieee754_floating F>
inline void
evaluate(const quintic_spline_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// piecewise_polynomial_1d

template<ieee754_floating F> struct piecewise_polynomial_1d {
  u32 degree{ 0 };
  vector<F> breaks;
  vector<F> coeff;
  mutable usize last_hit{ 0 };
  extension_mode mode{ extension_mode::polynomial };
};

template<ieee754_floating F>
[[nodiscard]] inline spline_domain<F>
domain(const piecewise_polynomial_1d<F> &s) noexcept
{
  return s.breaks.size() ? spline_domain<F>{ s.breaks[0], s.breaks[s.breaks.size() - 1], true } : spline_domain<F>{};
}

template<ieee754_floating F>
[[nodiscard]] inline piecewise_polynomial_1d<F>
make_piecewise_polynomial(raw_slice<const F> breaks, raw_slice<const F> coeff, u32 degree, build_info<F> *info = nullptr) noexcept
{
  piecewise_polynomial_1d<F> out{};
  out.degree = degree;
  if ( degree > 16 ) {
    if ( info ) info->status = build_status::invalid_argument;
    return out;
  }
  if ( breaks.size() < 2 ) {
    if ( info ) info->status = build_status::too_few_points;
    return out;
  }
  if ( coeff.size() != (breaks.size() - 1) * usize(degree + 1) ) {
    if ( info ) info->status = build_status::size_mismatch;
    return out;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(breaks.ptr, breaks.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return out;
  }
  out.breaks.reserve(breaks.size());
  out.coeff.reserve(coeff.size());
  for ( usize i = 0; i < breaks.size(); ++i ) out.breaks.emplace_back(breaks[i]);
  for ( usize i = 0; i < coeff.size(); ++i ) out.coeff.emplace_back(coeff[i]);
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline piecewise_polynomial_1d<F>
to_power_basis(const cubic_spline_1d<F> &s) noexcept
{
  piecewise_polynomial_1d<F> out{};
  out.degree = 3;
  out.mode = s.mode == extrap::clamp_to_endpoints ? extension_mode::clamp
             : s.mode == extrap::linear_continue  ? extension_mode::linear
                                                  : extension_mode::zero;
  out.breaks.reserve(s.xs.size());
  out.coeff.reserve(s.seg.size() * 4);
  for ( usize i = 0; i < s.xs.size(); ++i ) out.breaks.emplace_back(s.xs[i]);
  for ( usize i = 0; i < s.seg.size(); ++i )
    for ( usize k = 0; k < 4; ++k ) out.coeff.emplace_back(s.seg[i][k]);
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline piecewise_polynomial_1d<F>
to_power_basis(const quadratic_spline_1d<F> &s) noexcept
{
  piecewise_polynomial_1d<F> out{};
  out.degree = 2;
  out.mode = s.mode;
  out.breaks.reserve(s.xs.size());
  out.coeff.reserve(s.seg.size() * 3);
  for ( usize i = 0; i < s.xs.size(); ++i ) out.breaks.emplace_back(s.xs[i]);
  for ( usize i = 0; i < s.seg.size(); ++i )
    for ( usize k = 0; k < 3; ++k ) out.coeff.emplace_back(s.seg[i][k]);
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline piecewise_polynomial_1d<F>
to_power_basis(const quintic_spline_1d<F> &s) noexcept
{
  piecewise_polynomial_1d<F> out{};
  out.degree = 5;
  out.mode = s.mode;
  out.breaks.reserve(s.xs.size());
  out.coeff.reserve(s.seg.size() * 6);
  for ( usize i = 0; i < s.xs.size(); ++i ) out.breaks.emplace_back(s.xs[i]);
  for ( usize i = 0; i < s.seg.size(); ++i )
    for ( usize k = 0; k < 6; ++k ) out.coeff.emplace_back(s.seg[i][k]);
  return out;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
evaluate(const piecewise_polynomial_1d<F> &s, F x) noexcept
{
  const usize n = s.breaks.size();
  if ( n < 2 ) return F(0);
  const F *breaks = s.breaks.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, breaks[0], breaks[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, breaks[0], breaks[n - 1]);
  usize i = 0;
  if ( x <= breaks[0] ) {
    if ( s.mode == extension_mode::zero && x < breaks[0] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return s.coeff[0];
    if ( s.mode == extension_mode::linear ) return math::fma<F>(s.degree ? s.coeff[1] : F(0), x - breaks[0], s.coeff[0]);
  } else if ( x >= breaks[n - 1] ) {
    i = n - 2;
    const F h = breaks[n - 1] - breaks[i];
    const F *p = s.coeff.data() + i * usize(s.degree + 1);
    const F y = __impl_piecewise_1d::eval_power<F>(p, s.degree, h);
    if ( s.mode == extension_mode::zero && x > breaks[n - 1] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return y;
    if ( s.mode == extension_mode::linear )
      return math::fma<F>(__impl_piecewise_1d::eval_power_derivative<F>(p, s.degree, h, 1), x - breaks[n - 1], y);
  } else {
    i = __impl_splines_bits::locate_segment<F>(breaks, n, x, s.last_hit);
  }
  return __impl_piecewise_1d::eval_power<F>(s.coeff.data() + i * usize(s.degree + 1), s.degree, x - breaks[i]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const piecewise_polynomial_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  const usize n = s.breaks.size();
  if ( n < 2 ) return F(0);
  const F *breaks = s.breaks.data();
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, breaks[0], breaks[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, breaks[0], breaks[n - 1]);
  if ( x <= breaks[0] ) {
    if ( s.mode == extension_mode::zero && x < breaks[0] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return s.coeff[0];
    if ( s.mode == extension_mode::linear ) return math::fma<F>(s.degree ? s.coeff[1] : F(0), x - breaks[0], s.coeff[0]);
    return __impl_piecewise_1d::eval_power<F>(s.coeff.data(), s.degree, x - breaks[0]);
  }
  if ( x >= breaks[n - 1] ) {
    const usize segment = n - 2;
    const F width = breaks[n - 1] - breaks[segment];
    const F *polynomial = s.coeff.data() + segment * usize(s.degree + 1);
    const F value = __impl_piecewise_1d::eval_power<F>(polynomial, s.degree, width);
    if ( s.mode == extension_mode::zero && x > breaks[n - 1] ) return F(0);
    if ( s.mode == extension_mode::clamp ) return value;
    if ( s.mode == extension_mode::linear )
      return math::fma<F>(__impl_piecewise_1d::eval_power_derivative<F>(polynomial, s.degree, width, 1), x - breaks[n - 1], value);
    return __impl_piecewise_1d::eval_power<F>(polynomial, s.degree, x - breaks[segment]);
  }
  const usize segment = __impl_splines_bits::locate_segment<F>(breaks, n, x, cursor.segment);
  return __impl_piecewise_1d::eval_power<F>(s.coeff.data() + segment * usize(s.degree + 1), s.degree, x - breaks[segment]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
derivative(const piecewise_polynomial_1d<F> &s, F x, u32 order = 1) noexcept
{
  const usize n = s.breaks.size();
  if ( n < 2 ) return F(0);
  if ( order == 0 ) return evaluate<F>(s, x);
  const F *breaks = s.breaks.data();
  const bool outside = x < breaks[0] || x > breaks[n - 1];
  if ( outside && (s.mode == extension_mode::zero || s.mode == extension_mode::clamp) ) return F(0);
  if ( outside && s.mode == extension_mode::linear ) {
    if ( order != 1 ) return F(0);
    if ( x < breaks[0] ) return s.degree ? s.coeff[1] : F(0);
    const usize segment = n - 2;
    const F *polynomial = s.coeff.data() + segment * usize(s.degree + 1);
    return __impl_piecewise_1d::eval_power_derivative<F>(polynomial, s.degree, breaks[n - 1] - breaks[segment], 1);
  }
  const F sign
      = s.mode == extension_mode::reflect ? __impl_piecewise_1d::reflection_derivative_sign<F>(x, breaks[0], breaks[n - 1], order) : F(1);
  if ( s.mode == extension_mode::periodic ) x = __impl_piecewise_1d::map_periodic<F>(x, breaks[0], breaks[n - 1]);
  if ( s.mode == extension_mode::reflect ) x = __impl_piecewise_1d::map_reflect<F>(x, breaks[0], breaks[n - 1]);
  usize i = x >= breaks[n - 1] ? n - 2 : 0;
  if ( x > breaks[0] && x < breaks[n - 1] ) i = __impl_splines_bits::locate_segment<F>(breaks, n, x, s.last_hit);
  return sign * __impl_piecewise_1d::eval_power_derivative<F>(s.coeff.data() + i * usize(s.degree + 1), s.degree, x - breaks[i], order);
}

template<ieee754_floating F>
inline void
evaluate(const piecewise_polynomial_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
}

template<ieee754_floating F>
[[nodiscard]] inline piecewise_polynomial_1d<F>
derivative_spline(const piecewise_polynomial_1d<F> &s, u32 order = 1) noexcept
{
  piecewise_polynomial_1d<F> out{};
  if ( s.breaks.size() < 2 ) return out;
  out.degree = order > s.degree ? 0 : s.degree - order;
  out.mode = s.mode;
  out.breaks.reserve(s.breaks.size());
  for ( usize i = 0; i < s.breaks.size(); ++i ) out.breaks.emplace_back(s.breaks[i]);
  const usize width = usize(out.degree + 1);
  out.coeff.reserve((s.breaks.size() - 1) * width);
  for ( usize i = 0; i + 1 < s.breaks.size(); ++i ) {
    const F *p = s.coeff.data() + i * usize(s.degree + 1);
    for ( u32 k = 0; k <= out.degree; ++k ) {
      F scale = F(1);
      for ( u32 j = 0; j < order; ++j ) scale *= F(k + order - j);
      out.coeff.emplace_back(order > s.degree ? F(0) : scale * p[k + order]);
    }
  }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline piecewise_polynomial_1d<F>
antiderivative_spline(const piecewise_polynomial_1d<F> &s) noexcept
{
  piecewise_polynomial_1d<F> out{};
  if ( s.breaks.size() < 2 || s.degree >= 16 ) return out;
  out.degree = s.degree + 1;
  out.mode = extension_mode::polynomial;
  out.breaks.reserve(s.breaks.size());
  for ( usize i = 0; i < s.breaks.size(); ++i ) out.breaks.emplace_back(s.breaks[i]);
  out.coeff.reserve((s.breaks.size() - 1) * usize(out.degree + 1));
  F carry = F(0);
  for ( usize i = 0; i + 1 < s.breaks.size(); ++i ) {
    const F *p = s.coeff.data() + i * usize(s.degree + 1);
    out.coeff.emplace_back(carry);
    for ( u32 k = 0; k <= s.degree; ++k ) out.coeff.emplace_back(p[k] / F(k + 1));
    const F h = s.breaks[i + 1] - s.breaks[i];
    carry = __impl_piecewise_1d::eval_power<F>(out.coeff.data() + i * usize(out.degree + 1), out.degree, h);
  }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline F
integral(const piecewise_polynomial_1d<F> &s, F a, F b) noexcept
{
  if ( a == b ) return F(0);
  const auto anti = antiderivative_spline<F>(s);
  return evaluate<F>(anti, b) - evaluate<F>(anti, a);
}

template<ieee754_floating F>
[[nodiscard]] inline F
integral(const quadratic_spline_1d<F> &s, F a, F b) noexcept
{
  return integral<F>(to_power_basis<F>(s), a, b);
}

template<ieee754_floating F>
[[nodiscard]] inline F
integral(const quintic_spline_1d<F> &s, F a, F b) noexcept
{
  return integral<F>(to_power_basis<F>(s), a, b);
}

namespace __impl_piecewise_roots
{

template<ieee754_floating F>
inline void
append_unique(vector<F> &out, F value, F tolerance) noexcept
{
  if ( out.size() == 0 || __impl_piecewise_1d::abs_value<F>(out[out.size() - 1] - value) > tolerance ) out.emplace_back(value);
}

template<ieee754_floating F>
inline void
polynomial_roots_interval(const F *coeff, u32 degree, F lo, F hi, F tolerance, vector<F> &out) noexcept
{
  while ( degree > 0 && __impl_piecewise_1d::abs_value<F>(coeff[degree]) <= tolerance ) --degree;
  if ( degree == 0 ) return;
  if ( degree == 1 ) {
    const F root = -coeff[0] / coeff[1];
    if ( root >= lo - tolerance && root <= hi + tolerance ) append_unique<F>(out, root < lo ? lo : (root > hi ? hi : root), tolerance);
    return;
  }

  F deriv[17]{};
  for ( u32 i = 1; i <= degree; ++i ) deriv[i - 1] = F(i) * coeff[i];
  vector<F> stationary;
  polynomial_roots_interval<F>(deriv, degree - 1, lo, hi, tolerance, stationary);
  F left = lo;
  F f_left = __impl_piecewise_1d::eval_power<F>(coeff, degree, left);
  if ( __impl_piecewise_1d::abs_value<F>(f_left) <= tolerance ) append_unique<F>(out, left, tolerance);
  for ( usize boundary = 0; boundary <= stationary.size(); ++boundary ) {
    const F right = boundary < stationary.size() ? stationary[boundary] : hi;
    const F f_right = __impl_piecewise_1d::eval_power<F>(coeff, degree, right);
    if ( f_left * f_right < F(0) ) {
      F a = left, b = right, fa = f_left;
      for ( u32 iteration = 0; iteration < 72; ++iteration ) {
        const F middle = F(0.5) * (a + b);
        const F fm = __impl_piecewise_1d::eval_power<F>(coeff, degree, middle);
        if ( fa * fm <= F(0) )
          b = middle;
        else {
          a = middle;
          fa = fm;
        }
      }
      append_unique<F>(out, F(0.5) * (a + b), tolerance);
    }
    if ( __impl_piecewise_1d::abs_value<F>(f_right) <= tolerance ) append_unique<F>(out, right, tolerance);
    left = right;
    f_left = f_right;
  }
}

};      // namespace __impl_piecewise_roots

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
roots(const piecewise_polynomial_1d<F> &s, F target = F(0), F tolerance = sizeof(F) <= 4 ? F(1e-5) : F(1e-12)) noexcept
{
  vector<F> out;
  if ( s.breaks.size() < 2 || !(tolerance > F(0)) ) return out;
  F coeff[17]{};
  for ( usize segment = 0; segment + 1 < s.breaks.size(); ++segment ) {
    const F *source = s.coeff.data() + segment * usize(s.degree + 1);
    for ( u32 k = 0; k <= s.degree; ++k ) coeff[k] = source[k];
    coeff[0] -= target;
    vector<F> local;
    const F width = s.breaks[segment + 1] - s.breaks[segment];
    __impl_piecewise_roots::polynomial_roots_interval<F>(coeff, s.degree, F(0), width, tolerance, local);
    for ( usize i = 0; i < local.size(); ++i ) __impl_piecewise_roots::append_unique<F>(out, s.breaks[segment] + local[i], tolerance);
  }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
solve(const piecewise_polynomial_1d<F> &s, F target, F tolerance = sizeof(F) <= 4 ? F(1e-5) : F(1e-12)) noexcept
{
  return roots<F>(s, target, tolerance);
}

template<ieee754_floating F>
[[nodiscard]] inline vector<F>
roots(const cubic_spline_1d<F> &s, F target = F(0), F tolerance = sizeof(F) <= 4 ? F(1e-5) : F(1e-12)) noexcept
{
  return roots<F>(to_power_basis<F>(s), target, tolerance);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// periodic cubic wrapper

template<ieee754_floating F> struct periodic_cubic_spline_1d {
  cubic_spline_1d<F> spline;
  F period{ F(0) };
};

template<ieee754_floating F>
[[nodiscard]] inline periodic_cubic_spline_1d<F>
make_periodic_cubic(raw_slice<const F> xs, raw_slice<const F> ys, build_info<F> *info = nullptr) noexcept
{
  periodic_cubic_spline_1d<F> out{};
  if ( !__impl_piecewise_1d::validate_xy<F>(xs, ys, 4, info) ) return out;
  const usize n = xs.size();
  const F scale = F(1) + __impl_piecewise_1d::abs_value<F>(ys[0]) + __impl_piecewise_1d::abs_value<F>(ys[n - 1]);
  const F tolerance = (sizeof(F) <= 4 ? F(1e-5) : F(1e-12)) * scale;
  if ( __impl_piecewise_1d::abs_value<F>(ys[0] - ys[n - 1]) > tolerance ) {
    if ( info ) info->status = build_status::degenerate;
    return out;
  }

  const usize m = n - 1;
  vector<F> a(m - 1, F(0)), b(m, F(0)), c(m - 1, F(0)), rhs(m, F(0));
  a.set_size(m - 1);
  b.set_size(m);
  c.set_size(m - 1);
  rhs.set_size(m);
  for ( usize i = 0; i < m; ++i ) {
    const usize prev = i == 0 ? m - 1 : i - 1;
    const usize next = i + 1 == m ? 0 : i + 1;
    const F hp = i == 0 ? xs[n - 1] - xs[n - 2] : xs[i] - xs[i - 1];
    const F hn = xs[i + 1] - xs[i];
    const F dp = i == 0 ? (ys[n - 1] - ys[n - 2]) / hp : (ys[i] - ys[i - 1]) / hp;
    const F dn = (ys[i + 1] - ys[i]) / hn;
    (void)prev;
    (void)next;
    b[i] = F(2) * (hp + hn);
    rhs[i] = F(6) * (dn - dp);
    if ( i > 0 ) a[i - 1] = hp;
    if ( i + 1 < m ) c[i] = hn;
  }
  const F alpha = xs[n - 1] - xs[n - 2];
  const F beta = alpha;
  const F gamma = -b[0];
  b[0] -= gamma;
  b[m - 1] -= alpha * beta / gamma;

  vector<F> aa = a, bb = b, cc = c, solution = rhs;
  linalg::tridiag_solve<F>(aa.data(), bb.data(), cc.data(), solution.data(), m);
  vector<F> z(m, F(0));
  z.set_size(m);
  z[0] = gamma;
  z[m - 1] = alpha;
  aa = a;
  bb = b;
  cc = c;
  linalg::tridiag_solve<F>(aa.data(), bb.data(), cc.data(), z.data(), m);
  const F factor = (solution[0] + beta * solution[m - 1] / gamma) / (F(1) + z[0] + beta * z[m - 1] / gamma);
  for ( usize i = 0; i < m; ++i ) solution[i] -= factor * z[i];

  out.spline.bc = bc_kind::natural;
  out.spline.mode = extrap::clamp_to_endpoints;
  out.spline.xs.reserve(n);
  out.spline.seg.reserve(n - 1);
  for ( usize i = 0; i < n; ++i ) out.spline.xs.emplace_back(xs[i]);
  const poly_coeffs<F, 3> zero{ { F(0), F(0), F(0), F(0) } };
  for ( usize i = 1; i < n; ++i ) out.spline.seg.emplace_back(zero);
  vector<F> moments(n, F(0));
  moments.set_size(n);
  for ( usize i = 0; i < m; ++i ) moments[i] = solution[i];
  moments[n - 1] = moments[0];
  __impl_splines_bits::build_cubic_segments<F>(xs.ptr, ys.ptr, moments.data(), out.spline.seg.data(), n);
  out.period = xs[n - 1] - xs[0];
  if ( info ) info->status = build_status::ok;
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const periodic_cubic_spline_1d<F> &s, F x) noexcept
{
  if ( s.spline.xs.size() < 2 ) return F(0);
  const F mapped = __impl_piecewise_1d::map_periodic<F>(x, s.spline.xs[0], s.spline.xs[s.spline.xs.size() - 1]);
  return evaluate<F>(s.spline, mapped);
}

template<ieee754_floating F>
[[nodiscard]] inline F
evaluate(const periodic_cubic_spline_1d<F> &s, F x, spline_cursor &cursor) noexcept
{
  if ( s.spline.xs.size() < 2 ) return F(0);
  const F mapped = __impl_piecewise_1d::map_periodic<F>(x, s.spline.xs[0], s.spline.xs[s.spline.xs.size() - 1]);
  if ( mapped <= s.spline.xs[0] ) return s.spline.seg[0][0];
  const usize segment = __impl_splines_bits::locate_segment<F>(s.spline.xs.data(), s.spline.xs.size(), mapped, cursor.segment);
  return __impl_splines_bits::eval_cubic_local<F>(s.spline.seg[segment], mapped - s.spline.xs[segment]);
}

template<ieee754_floating F>
[[nodiscard]] inline F
derivative(const periodic_cubic_spline_1d<F> &s, F x, u32 order = 1) noexcept
{
  if ( s.spline.xs.size() < 2 ) return F(0);
  const F mapped = __impl_piecewise_1d::map_periodic<F>(x, s.spline.xs[0], s.spline.xs[s.spline.xs.size() - 1]);
  return derivative<F>(s.spline, mapped, order);
}

template<ieee754_floating F>
inline void
evaluate(const periodic_cubic_spline_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
}

};      // namespace splines
};      // namespace math
};      // namespace micron
