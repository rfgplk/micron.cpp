//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Fritsch-Carlson monotone cubic

#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "bits/impl.hpp"
#include "cubic_1d.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template <ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_pchip(raw_slice<const F> xs, raw_slice<const F> ys, build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> s{};
  s.bc = bc_kind::clamped;

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

  vector<F> m(n, F(0));
  m.set_size(n);
  __impl_splines_bits::pchip_slopes<F>(xs.ptr, ys.ptr, m.data(), n);

  s.xs.reserve(n);
  s.seg.reserve(n - 1);
  for ( usize i = 0; i < n; ++i ) s.xs.emplace_back(xs[i]);
  poly_coeffs<F, 3> zero{};
  zero.data[0] = zero.data[1] = zero.data[2] = zero.data[3] = F(0);
  for ( usize i = 0; i + 1 < n; ++i ) s.seg.emplace_back(zero);
  __impl_splines_bits::build_cubic_segments_from_slopes<F>(xs.ptr, ys.ptr, m.data(), s.seg.data(), n);

  if ( info ) info->status = build_status::ok;
  return s;
}

};     // namespace splines
};     // namespace math
};     // namespace micron
