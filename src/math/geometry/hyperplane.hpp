//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// hyperplanes

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

template <ieee754_floating F, usize Dim>
  requires(Dim >= 2 && Dim <= 16)
struct hyperplane {
  vec<F, Dim> normal;
  F offset;

  // plane through point p with unit normal n
  [[nodiscard]] static constexpr hyperplane
  through(const vec<F, Dim> &p, const vec<F, Dim> &n) noexcept
  {
    hyperplane r{ n, F(0) };
    F off = F(0);
    for ( usize i = 0; i < Dim; ++i ) off -= n.data[i] * p.data[i];
    r.offset = off;
    return r;
  }

  // 3D plane through three points
  [[nodiscard]] static constexpr hyperplane
  through3(const vec<F, 3> &a, const vec<F, 3> &b, const vec<F, 3> &c) noexcept
    requires(Dim == 3)
  {
    vec<F, 3> ab{};
    vec<F, 3> ac{};
    for ( usize i = 0; i < 3; ++i ) {
      ab.data[i] = b.data[i] - a.data[i];
      ac.data[i] = c.data[i] - a.data[i];
    }
    auto n = linalg::ops::cross<F>(ab, ac);
    F nrm = math::fsqrt(n.data[0] * n.data[0] + n.data[1] * n.data[1] + n.data[2] * n.data[2]);
    if ( nrm > F(0) ) {
      F inv = F(1) / nrm;
      for ( usize i = 0; i < 3; ++i ) n.data[i] *= inv;
    }
    return through(a, n);
  }

  [[nodiscard, gnu::always_inline]] constexpr F
  signed_distance(const vec<F, Dim> &p) const noexcept
  {
    F s = offset;
    for ( usize i = 0; i < Dim; ++i ) s += normal.data[i] * p.data[i];
    return s;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  projection(const vec<F, Dim> &p) const noexcept
  {
    F d = signed_distance(p);
    vec<F, Dim> r{};
    for ( usize i = 0; i < Dim; ++i ) r.data[i] = p.data[i] - d * normal.data[i];
    return r;
  }
};

};     // namespace geometry
};     // namespace math
};     // namespace micron
