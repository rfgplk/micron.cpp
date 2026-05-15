//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// N-D parametric curves

#include "../../concepts.hpp"
#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../linalg/banded.hpp"
#include "../quants/vec.hpp"
#include "bits/impl.hpp"
#include "concepts.hpp"
#include "cubic_1d.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<ieee754_floating F, usize D>
  requires(D >= 2 && D <= 16)
struct alignas(vec_align_v<F, D>) curve_seg {
  vec<F, D> a, b, c, d;
};

template<ieee754_floating F, usize D>
  requires(D >= 2 && D <= 16)
struct nearest_curve_nd {
  vector<F> ts;
  vector<vec<F, D>> pts;
  mutable usize last_hit{ 0 };
  extrap mode{ extrap::clamp_to_endpoints };
};

template<ieee754_floating F, usize D>
[[nodiscard]] inline nearest_curve_nd<F, D>
make_nearest_curve(raw_slice<const F> ts, const vec<F, D> *points, usize n, build_info<F> *info = nullptr) noexcept
{
  nearest_curve_nd<F, D> c{};
  if ( ts.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return c;
  }
  if ( n < 2 ) {
    if ( info ) info->status = build_status::too_few_points;
    return c;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(ts.ptr, n) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return c;
  }
  c.ts.reserve(n);
  c.pts.reserve(n);
  for ( usize i = 0; i < n; ++i ) {
    c.ts.emplace_back(ts[i]);
    c.pts.emplace_back(points[i]);
  }
  if ( info ) info->status = build_status::ok;
  return c;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate(const nearest_curve_nd<F, D> &c, F t) noexcept
{
  const usize n = c.ts.size();
  const F *__restrict__ ts = c.ts.data();
  const vec<F, D> *__restrict__ pts = c.pts.data();
  if ( n == 0 ) return vec<F, D>{};
  if ( n == 1 ) return pts[0];
  if ( t <= ts[0] ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    return pts[0];
  }
  if ( t >= ts[n - 1] ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    return pts[n - 1];
  }
  const usize i = __impl_splines_bits::locate_segment<F>(ts, n, t, c.last_hit);
  return ((t + t) < (ts[i] + ts[i + 1])) ? pts[i] : pts[i + 1];
}

template<ieee754_floating F, usize D>
  requires(D >= 2 && D <= 16)
struct linear_curve_nd {
  vector<F> ts;
  vector<vec<F, D>> pts;
  mutable usize last_hit{ 0 };
  extrap mode{ extrap::linear_continue };
};

template<ieee754_floating F, usize D>
[[nodiscard]] inline linear_curve_nd<F, D>
make_linear_curve(raw_slice<const F> ts, const vec<F, D> *points, usize n, build_info<F> *info = nullptr) noexcept
{
  linear_curve_nd<F, D> c{};
  if ( ts.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return c;
  }
  if ( n < 2 ) {
    if ( info ) info->status = build_status::too_few_points;
    return c;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(ts.ptr, n) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return c;
  }
  c.ts.reserve(n);
  c.pts.reserve(n);
  for ( usize i = 0; i < n; ++i ) {
    c.ts.emplace_back(ts[i]);
    c.pts.emplace_back(points[i]);
  }
  if ( info ) info->status = build_status::ok;
  return c;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate(const linear_curve_nd<F, D> &c, F t) noexcept
{
  const usize n = c.ts.size();
  const F *__restrict__ ts = c.ts.data();
  const vec<F, D> *__restrict__ pts = c.pts.data();
  if ( n == 0 ) return vec<F, D>{};
  if ( n == 1 ) return pts[0];

  if ( t <= ts[0] ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    if ( c.mode == extrap::clamp_to_endpoints ) return pts[0];

    const F u = (t - ts[0]) / (ts[1] - ts[0]);
    vec<F, D> r{};
    for ( usize d = 0; d < D; ++d ) r.data[d] = math::fma<F>(u, pts[1].data[d] - pts[0].data[d], pts[0].data[d]);
    return r;
  }
  if ( t >= ts[n - 1] ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    if ( c.mode == extrap::clamp_to_endpoints ) return pts[n - 1];
    const F u = (t - ts[n - 2]) / (ts[n - 1] - ts[n - 2]);
    vec<F, D> r{};
    for ( usize d = 0; d < D; ++d ) r.data[d] = math::fma<F>(u, pts[n - 1].data[d] - pts[n - 2].data[d], pts[n - 2].data[d]);
    return r;
  }
  const usize i = __impl_splines_bits::locate_segment<F>(ts, n, t, c.last_hit);
  const F u = (t - ts[i]) / (ts[i + 1] - ts[i]);
  vec<F, D> r{};
  for ( usize d = 0; d < D; ++d ) r.data[d] = math::fma<F>(u, pts[i + 1].data[d] - pts[i].data[d], pts[i].data[d]);
  return r;
}

template<ieee754_floating F, usize D>
  requires(D >= 2 && D <= 16)
struct cubic_curve_nd {
  vector<F> ts;
  vector<curve_seg<F, D>> seg;
  mutable usize last_hit{ 0 };
  bc_kind bc{ bc_kind::natural };
  extrap mode{ extrap::linear_continue };
};

namespace __impl_curve_nd
{

template<ieee754_floating F, usize D>
inline bool
build_cubic_curve_segments(const F *ts, const vec<F, D> *pts, usize n, bc_kind bc, const vec<F, D> &left_slope,
                           const vec<F, D> &right_slope, curve_seg<F, D> *seg_out) noexcept
{

  vector<F> M(n, F(0));
  M.set_size(n);
  vector<F> ys(n, F(0));
  ys.set_size(n);
  vector<poly_coeffs<F, 3>> seg_axis(n - 1, poly_coeffs<F, 3>{});
  seg_axis.set_size(n - 1);

  for ( usize i = 0; i + 1 < n; ++i ) {
    for ( usize d = 0; d < D; ++d ) {
      seg_out[i].a.data[d] = F(0);
      seg_out[i].b.data[d] = F(0);
      seg_out[i].c.data[d] = F(0);
      seg_out[i].d.data[d] = F(0);
    }
  }

  for ( usize d = 0; d < D; ++d ) {
    for ( usize i = 0; i < n; ++i ) ys[i] = pts[i].data[d];
    if ( !__impl_cubic_1d::solve_cubic_M<F>(ts, ys.data(), n, bc, left_slope.data[d], right_slope.data[d], M.data()) ) return false;
    __impl_splines_bits::build_cubic_segments<F>(ts, ys.data(), M.data(), seg_axis.data(), n);
    for ( usize i = 0; i + 1 < n; ++i ) {
      seg_out[i].a.data[d] = seg_axis[i].data[0];
      seg_out[i].b.data[d] = seg_axis[i].data[1];
      seg_out[i].c.data[d] = seg_axis[i].data[2];
      seg_out[i].d.data[d] = seg_axis[i].data[3];
    }
  }
  return true;
}

};      // namespace __impl_curve_nd

template<ieee754_floating F, usize D>
[[nodiscard]] inline cubic_curve_nd<F, D>
make_cubic_curve(raw_slice<const F> ts, const vec<F, D> *points, usize n, bc_kind bc = bc_kind::natural, vec<F, D> left_slope = vec<F, D>{},
                 vec<F, D> right_slope = vec<F, D>{}, build_info<F> *info = nullptr) noexcept
{
  cubic_curve_nd<F, D> c{};
  c.bc = bc;
  if ( ts.size() != n ) {
    if ( info ) info->status = build_status::size_mismatch;
    return c;
  }
  if ( n < 2 ) {
    if ( info ) info->status = build_status::too_few_points;
    return c;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(ts.ptr, n) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return c;
  }

  c.ts.reserve(n);
  for ( usize i = 0; i < n; ++i ) c.ts.emplace_back(ts[i]);
  c.seg.reserve(n - 1);
  curve_seg<F, D> zero{};
  for ( usize i = 0; i + 1 < n; ++i ) c.seg.emplace_back(zero);
  if ( !__impl_curve_nd::build_cubic_curve_segments<F, D>(ts.ptr, points, n, bc, left_slope, right_slope, c.seg.data()) ) {
    if ( info ) info->status = build_status::singular_system;
    return c;
  }
  if ( info ) info->status = build_status::ok;
  return c;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate(const cubic_curve_nd<F, D> &c, F t) noexcept
{
  const usize n = c.ts.size();
  if ( n < 2 ) return vec<F, D>{};
  const F *__restrict__ ts = c.ts.data();
  const auto *__restrict__ seg = c.seg.data();
  if ( t <= ts[0] ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    if ( c.mode == extrap::clamp_to_endpoints ) return seg[0].a;

    vec<F, D> r{};
    const F dt = t - ts[0];
    for ( usize d = 0; d < D; ++d ) r.data[d] = math::fma<F>(seg[0].b.data[d], dt, seg[0].a.data[d]);
    return r;
  }
  if ( t >= ts[n - 1] ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    const auto &p = seg[n - 2];
    const F t_end = ts[n - 1] - ts[n - 2];

    vec<F, D> y_end{};
    vec<F, D> slope_end{};
    for ( usize d = 0; d < D; ++d ) {
      F r = math::fma<F>(p.d.data[d], t_end, p.c.data[d]);
      r = math::fma<F>(r, t_end, p.b.data[d]);
      y_end.data[d] = math::fma<F>(r, t_end, p.a.data[d]);
      F s = math::fma<F>(F(3) * p.d.data[d], t_end, F(2) * p.c.data[d]);
      slope_end.data[d] = math::fma<F>(s, t_end, p.b.data[d]);
    }
    if ( c.mode == extrap::clamp_to_endpoints ) return y_end;
    const F dt = t - ts[n - 1];
    vec<F, D> r{};
    for ( usize d = 0; d < D; ++d ) r.data[d] = math::fma<F>(slope_end.data[d], dt, y_end.data[d]);
    return r;
  }
  const usize i = __impl_splines_bits::locate_segment<F>(ts, n, t, c.last_hit);
  const F u = t - ts[i];
  const auto &p = seg[i];
  vec<F, D> r{};
  for ( usize d = 0; d < D; ++d ) {
    F v = math::fma<F>(p.d.data[d], u, p.c.data[d]);
    v = math::fma<F>(v, u, p.b.data[d]);
    r.data[d] = math::fma<F>(v, u, p.a.data[d]);
  }
  return r;
}

template<ieee754_floating F, usize D>
  requires(D >= 2 && D <= 16)
struct regular_cubic_curve_nd {
  F t0{ 0 };
  F dt{ 1 };
  vector<curve_seg<F, D>> seg;
  bc_kind bc{ bc_kind::natural };
  extrap mode{ extrap::linear_continue };
};

template<ieee754_floating F, usize D>
[[nodiscard]] inline regular_cubic_curve_nd<F, D>
make_regular_cubic_curve(F t0, F dt, const vec<F, D> *points, usize n, bc_kind bc = bc_kind::natural, vec<F, D> left_slope = vec<F, D>{},
                         vec<F, D> right_slope = vec<F, D>{}, build_info<F> *info = nullptr) noexcept
{
  regular_cubic_curve_nd<F, D> c{};
  c.t0 = t0;
  c.dt = dt;
  c.bc = bc;
  if ( n < 2 ) {
    if ( info ) info->status = build_status::too_few_points;
    return c;
  }
  if ( !(dt > F(0)) ) {
    if ( info ) info->status = build_status::degenerate;
    return c;
  }

  vector<F> ts(n, F(0));
  ts.set_size(n);
  for ( usize i = 0; i < n; ++i ) ts[i] = t0 + dt * F(i);
  c.seg.reserve(n - 1);
  curve_seg<F, D> zero{};
  for ( usize i = 0; i + 1 < n; ++i ) c.seg.emplace_back(zero);
  if ( !__impl_curve_nd::build_cubic_curve_segments<F, D>(ts.data(), points, n, bc, left_slope, right_slope, c.seg.data()) ) {
    if ( info ) info->status = build_status::singular_system;
    return c;
  }
  if ( info ) info->status = build_status::ok;
  return c;
}

template<ieee754_floating F, usize D>
[[nodiscard, gnu::flatten]] inline vec<F, D>
evaluate(const regular_cubic_curve_nd<F, D> &c, F t) noexcept
{
  const usize n_seg = c.seg.size();
  if ( n_seg == 0 ) return vec<F, D>{};
  const auto *__restrict__ seg = c.seg.data();

  const F u_full = (t - c.t0) / c.dt;

  if ( !(u_full > F(0)) ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    if ( c.mode == extrap::clamp_to_endpoints ) return seg[0].a;
    vec<F, D> r{};
    for ( usize d = 0; d < D; ++d ) r.data[d] = math::fma<F>(seg[0].b.data[d], t - c.t0, seg[0].a.data[d]);
    return r;
  }
  if ( !(u_full < F(n_seg)) ) {
    if ( c.mode == extrap::error_value ) return vec<F, D>{};
    const auto &p = seg[n_seg - 1];
    const F t_end = c.dt;
    vec<F, D> y_end{};
    vec<F, D> slope_end{};
    for ( usize d = 0; d < D; ++d ) {
      F r = math::fma<F>(p.d.data[d], t_end, p.c.data[d]);
      r = math::fma<F>(r, t_end, p.b.data[d]);
      y_end.data[d] = math::fma<F>(r, t_end, p.a.data[d]);
      F s = math::fma<F>(F(3) * p.d.data[d], t_end, F(2) * p.c.data[d]);
      slope_end.data[d] = math::fma<F>(s, t_end, p.b.data[d]);
    }
    if ( c.mode == extrap::clamp_to_endpoints ) return y_end;
    const F dtq = t - (c.t0 + c.dt * F(n_seg));
    vec<F, D> r{};
    for ( usize d = 0; d < D; ++d ) r.data[d] = math::fma<F>(slope_end.data[d], dtq, y_end.data[d]);
    return r;
  }
  const usize i = static_cast<usize>(u_full);
  const F u = t - (c.t0 + c.dt * F(i));
  const auto &p = seg[i];
  vec<F, D> r{};
  for ( usize d = 0; d < D; ++d ) {
    F v = math::fma<F>(p.d.data[d], u, p.c.data[d]);
    v = math::fma<F>(v, u, p.b.data[d]);
    r.data[d] = math::fma<F>(v, u, p.a.data[d]);
  }
  return r;
}

};      // namespace splines
};      // namespace math
};      // namespace micron
