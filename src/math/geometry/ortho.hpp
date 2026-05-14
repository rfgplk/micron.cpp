//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// orthogonal vector fns

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../linalg/ops.hpp"
#include "../quants/vec.hpp"
#include "../sqrt.hpp"

namespace micron
{
namespace math
{
namespace geometry
{

// TODO: skeleton, expand later

// unit vector perpendicular to v
template <ieee754_floating F, usize N>
  requires(N >= 2 && N <= 16)
[[nodiscard]] inline constexpr vec<F, N>
unit_orthogonal(const vec<F, N> &v) noexcept
{
  if constexpr ( N == 3 ) {
    F ax = math::fabs(v.data[0]);
    F ay = math::fabs(v.data[1]);
    F az = math::fabs(v.data[2]);
    vec<F, 3> axis{};
    if ( ax <= ay && ax <= az ) {
      axis.data[0] = F(1);
    } else if ( ay <= az ) {
      axis.data[1] = F(1);
    } else {
      axis.data[2] = F(1);
    }
    auto c = linalg::ops::cross<F>(v, axis);
    F nrm = math::fsqrt(c.data[0] * c.data[0] + c.data[1] * c.data[1] + c.data[2] * c.data[2]);
    if ( nrm > F(0) ) {
      F inv = F(1) / nrm;
      for ( usize i = 0; i < 3; ++i ) c.data[i] *= inv;
    }
    return c;
  } else {
    usize idx_min = 0;
    F amin = math::fabs(v.data[0]);
    for ( usize i = 1; i < N; ++i ) {
      F a = math::fabs(v.data[i]);
      if ( a < amin ) {
        amin = a;
        idx_min = i;
      }
    }
    usize idx_a = (idx_min + 1) % N;
    usize idx_b = (idx_min + 2) % N;
    vec<F, N> u{};
    for ( usize i = 0; i < N; ++i ) u.data[i] = F(0);
    u.data[idx_a] = -v.data[idx_b];
    u.data[idx_b] = v.data[idx_a];
    F nrm_sq = u.data[idx_a] * u.data[idx_a] + u.data[idx_b] * u.data[idx_b];
    F nrm = math::fsqrt(nrm_sq);
    if ( nrm > F(0) ) {
      F inv = F(1) / nrm;
      for ( usize i = 0; i < N; ++i ) u.data[i] *= inv;
    }
    return u;
  }
}

};     // namespace geometry
};     // namespace math
};     // namespace micron
