//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%
// unit sphere

#include "../../../concepts.hpp"
#include "../../../except.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../linalg/ops.hpp"
#include "../../mk.hpp"
#include "../../quants/vec.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

template <ieee754_floating F, usize N>
  requires(N >= 2 && N <= 16)
struct sphere {
  using value_type = F;
  static constexpr usize length = N;

  [[nodiscard, gnu::flatten]] static vec<F, N>
  project(const vec<F, N> &x) noexcept
  {
    return linalg::ops::normalize(x);
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  project_to_manifold(const vec<F, N> &x) noexcept
  {
    return linalg::ops::normalize(x);
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, N>
  project_to_tangent(const vec<F, N> &p, const vec<F, N> &v) noexcept
  {
    const F d = linalg::ops::dot(p, v);
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = v.data[i] - d * p.data[i];
    return r;
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  exp_map(const vec<F, N> &p, const vec<F, N> &v) noexcept
  {
    const F theta = linalg::ops::norm(v);
    if ( theta < math::default_eps<F>() ) {
      vec<F, N> r{};
      for ( usize i = 0; i < N; ++i ) r.data[i] = p.data[i] + v.data[i];
      return linalg::ops::normalize(r);
    }
    const F c = math::cos<F>(theta);
    const F s = math::sin<F>(theta);
    const F k = s / theta;
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = c * p.data[i] + k * v.data[i];
    return r;
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  log_map(const vec<F, N> &p, const vec<F, N> &q)
  {
    F dot_pq = linalg::ops::dot(p, q);
    if ( dot_pq > F(1) ) dot_pq = F(1);
    if ( dot_pq < F(-1) ) dot_pq = F(-1);
    if ( dot_pq <= F(-1) + math::default_eps<F>() ) exc<except::domain_error>("sphere::log_map: antipodal points have no unique geodesic");
    if ( dot_pq >= F(1) - math::default_eps<F>() ) {
      vec<F, N> r{};
      for ( usize i = 0; i < N; ++i ) r.data[i] = q.data[i] - p.data[i];
      return project_to_tangent(p, r);
    }
    const F theta = math::acos<F>(dot_pq);
    const F sin_theta = math::sin<F>(theta);
    const F k = theta / sin_theta;
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = (q.data[i] - dot_pq * p.data[i]) * k;
    return r;
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  retract(const vec<F, N> &p, const vec<F, N> &v) noexcept
  {
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = p.data[i] + v.data[i];
    return linalg::ops::normalize(r);
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  inverse_retract(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    vec<F, N> diff{};
    for ( usize i = 0; i < N; ++i ) diff.data[i] = q.data[i] - p.data[i];
    return project_to_tangent(p, diff);
  }

  [[nodiscard, gnu::always_inline]] static F
  distance(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    F dot_pq = linalg::ops::dot(p, q);
    if ( dot_pq > F(1) ) dot_pq = F(1);
    if ( dot_pq < F(-1) ) dot_pq = F(-1);
    return math::acos<F>(dot_pq);
  }

  [[nodiscard, gnu::always_inline]] static constexpr F
  inner(const vec<F, N> &, const vec<F, N> &u, const vec<F, N> &v) noexcept
  {
    return linalg::ops::dot(u, v);
  }

  [[nodiscard, gnu::always_inline]] static F
  norm(const vec<F, N> &, const vec<F, N> &v) noexcept
  {
    return linalg::ops::norm(v);
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  parallel_transport(const vec<F, N> &p, const vec<F, N> &q, const vec<F, N> &v) noexcept
  {
    const F denom = F(1) + linalg::ops::dot(p, q);
    if ( denom < math::default_eps<F>() ) return project_to_tangent(q, v);
    const F qv = linalg::ops::dot(q, v);
    const F k = qv / denom;
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = v.data[i] - k * (p.data[i] + q.data[i]);
    return r;
  }

  [[nodiscard, gnu::always_inline]] static vec<F, N>
  vector_transport(const vec<F, N> &, const vec<F, N> &q, const vec<F, N> &v) noexcept
  {
    return project_to_tangent(q, v);
  }
};

template <ieee754_floating F, usize N> struct traits<sphere<F, N>> {
  using point_type = vec<F, N>;
  using tangent_type = vec<F, N>;
  using scalar_type = F;
  using category = sphere_tag;
  static constexpr usize dim = N - 1;
  static constexpr usize ambient_dim = N;
};

};     // namespace manifolds
};     // namespace math
};     // namespace micron
