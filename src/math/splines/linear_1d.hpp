//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "bits/impl.hpp"
#include "concepts.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<ieee754_floating F> struct linear_1d {
  vector<F> xs;
  vector<F> ys;
  mutable usize last_hit{ 0 };
  extrap mode{ extrap::linear_continue };
};

template<ieee754_floating F>
[[nodiscard]] inline linear_1d<F>
make_linear(raw_slice<const F> xs, raw_slice<const F> ys, build_info<F> *info = nullptr) noexcept
{
  linear_1d<F> s{};
  if ( xs.size() != ys.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return s;
  }
  if ( xs.size() < 2 ) {
    if ( info ) info->status = build_status::too_few_points;
    return s;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(xs.ptr, xs.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return s;
  }
  const usize n = xs.size();
  s.xs.reserve(n);
  s.ys.reserve(n);
  for ( usize i = 0; i < n; ++i ) {
    s.xs.emplace_back(xs[i]);
    s.ys.emplace_back(ys[i]);
  }
  if ( info ) info->status = build_status::ok;
  return s;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
evaluate(const linear_1d<F> &s, F x) noexcept
{
  const usize n = s.xs.size();
  const F *__restrict__ xs = s.xs.data();
  const F *__restrict__ ys = s.ys.data();
  if ( n == 0 ) return F(0);
  if ( n == 1 ) return ys[0];

  if ( x <= xs[0] ) {
    if ( s.mode == extrap::error_value ) return F(0);
    if ( s.mode == extrap::clamp_to_endpoints ) return ys[0];

    const F slope = (ys[1] - ys[0]) / (xs[1] - xs[0]);
    return math::fma<F>(slope, x - xs[0], ys[0]);
  }
  if ( x >= xs[n - 1] ) {
    if ( s.mode == extrap::error_value ) return F(0);
    if ( s.mode == extrap::clamp_to_endpoints ) return ys[n - 1];
    const F slope = (ys[n - 1] - ys[n - 2]) / (xs[n - 1] - xs[n - 2]);
    return math::fma<F>(slope, x - xs[n - 1], ys[n - 1]);
  }

  const usize i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  const F t = (x - xs[i]) / (xs[i + 1] - xs[i]);
  return math::fma<F>(t, ys[i + 1] - ys[i], ys[i]);
}

template<ieee754_floating F>
inline void
evaluate(const linear_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  if ( !__impl_splines_bits::is_sorted_nondecreasing<F>(xq, n) ) {
    for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }

  const usize ns = s.xs.size();
  const F *__restrict__ xs = s.xs.data();
  const F *__restrict__ ys = s.ys.data();
  if ( ns < 2 ) {
    for ( usize i = 0; i < n; ++i ) out[i] = (ns == 0) ? F(0) : ys[0];
    return;
  }

  usize idx = s.last_hit <= ns - 2 ? s.last_hit : 0;
  for ( usize i = 0; i < n; ++i ) {
    const F x = xq[i];
    if ( x <= xs[0] ) {
      if ( s.mode == extrap::error_value ) {
        out[i] = F(0);
      } else if ( s.mode == extrap::clamp_to_endpoints ) {
        out[i] = ys[0];
      } else {
        const F slope = (ys[1] - ys[0]) / (xs[1] - xs[0]);
        out[i] = math::fma<F>(slope, x - xs[0], ys[0]);
      }
      continue;
    }
    if ( x >= xs[ns - 1] ) {
      if ( s.mode == extrap::error_value ) {
        out[i] = F(0);
      } else if ( s.mode == extrap::clamp_to_endpoints ) {
        out[i] = ys[ns - 1];
      } else {
        const F slope = (ys[ns - 1] - ys[ns - 2]) / (xs[ns - 1] - xs[ns - 2]);
        out[i] = math::fma<F>(slope, x - xs[ns - 1], ys[ns - 1]);
      }
      continue;
    }
    while ( idx + 1 < ns - 1 && x > xs[idx + 1] ) ++idx;
    __builtin_prefetch(&xs[idx + 2]);
    __builtin_prefetch(&ys[idx + 2]);
    const F t = (x - xs[idx]) / (xs[idx + 1] - xs[idx]);
    out[i] = math::fma<F>(t, ys[idx + 1] - ys[idx], ys[idx]);
  }
  s.last_hit = idx;
}

};      // namespace splines
};      // namespace math
};      // namespace micron
