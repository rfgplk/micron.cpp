//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 1-D nearest-neighbour interpolant

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

template<ieee754_floating F> struct nearest_1d {
  vector<F> xs;
  vector<F> ys;
  mutable usize last_hit{ 0 };
  extrap mode{ extrap::clamp_to_endpoints };
};

template<ieee754_floating F>
[[nodiscard]] inline nearest_1d<F>
make_nearest(raw_slice<const F> xs, raw_slice<const F> ys, build_info<F> *info = nullptr) noexcept
{
  nearest_1d<F> s{};
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
evaluate(const nearest_1d<F> &s, F x) noexcept
{
  const usize n = s.xs.size();
  const F *__restrict__ xs = s.xs.data();
  const F *__restrict__ ys = s.ys.data();
  if ( n == 0 ) return F(0);
  if ( n == 1 ) return ys[0];

  if ( x <= xs[0] ) {
    if ( s.mode == extrap::error_value ) return F(0);
    return ys[0];
  }
  if ( x >= xs[n - 1] ) {
    if ( s.mode == extrap::error_value ) return F(0);
    return ys[n - 1];
  }

  const usize i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  return ((x + x) < (xs[i] + xs[i + 1])) ? ys[i] : ys[i + 1];
}

template<ieee754_floating F>
inline void
evaluate(const nearest_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  if ( !__impl_splines_bits::is_sorted_nondecreasing<F>(xq, n) || s.xs.size() < 2 ) {
    for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }
  const usize ns = s.xs.size();
  const F *xs = s.xs.data();
  const F *ys = s.ys.data();
  usize segment = 0;
  for ( usize i = 0; i < n; ++i ) {
    const F x = xq[i];
    if ( x <= xs[0] ) {
      out[i] = s.mode == extrap::error_value ? F(0) : ys[0];
      continue;
    }
    if ( x >= xs[ns - 1] ) {
      out[i] = s.mode == extrap::error_value ? F(0) : ys[ns - 1];
      segment = ns - 2;
      continue;
    }
    while ( segment + 1 < ns - 1 && x > xs[segment + 1] ) ++segment;
    out[i] = ((x + x) < (xs[segment] + xs[segment + 1])) ? ys[segment] : ys[segment + 1];
  }
  if ( n ) s.last_hit = segment;
}

};      // namespace splines
};      // namespace math
};      // namespace micron
