//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../linalg/ops.hpp"
#include "../quants/vec.hpp"
#include "../sqrt.hpp"
#include "hyperplane.hpp"

namespace micron
{
namespace math
{
namespace geometry
{

// L(t) = origin + t * direction
template<ieee754_floating F, usize Dim>
  requires(Dim >= 2 && Dim <= 16)
struct parametrized_line {
  vec<F, Dim> origin;
  vec<F, Dim> direction;

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  point_at(F t) const noexcept
  {
    vec<F, Dim> r{};
    for ( usize i = 0; i < Dim; ++i ) r.data[i] = origin.data[i] + t * direction.data[i];
    return r;
  }

  // perpendicular distance from p to the line
  [[nodiscard, gnu::always_inline]] constexpr F
  distance(const vec<F, Dim> &p) const noexcept
  {
    F dot = F(0);
    F po_sq = F(0);
    for ( usize i = 0; i < Dim; ++i ) {
      F po = p.data[i] - origin.data[i];
      dot += po * direction.data[i];
      po_sq += po * po;
    }
    F sq = po_sq - dot * dot;
    if ( sq < F(0) ) sq = F(0);
    return math::fsqrt(sq);
  }

  // t such that L(t) is on the hyperplane, 0 if parallel
  [[nodiscard, gnu::always_inline]] constexpr F
  intersection_param(const hyperplane<F, Dim> &h) const noexcept
  {
    F denom = F(0);
    for ( usize i = 0; i < Dim; ++i ) denom += h.normal.data[i] * direction.data[i];
    if ( denom == F(0) ) return F(0);
    F num = -h.offset;
    for ( usize i = 0; i < Dim; ++i ) num -= h.normal.data[i] * origin.data[i];
    return num / denom;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  intersection_point(const hyperplane<F, Dim> &h) const noexcept
  {
    return point_at(intersection_param(h));
  }
};

};      // namespace geometry
};      // namespace math
};      // namespace micron
