//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// flat N-torus

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
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
  requires(N >= 1 && N <= 16)
struct torus {
  using value_type = F;
  static constexpr usize length = N;

  [[nodiscard, gnu::flatten]] static F
  canonical(F angle) noexcept
  {
    return math::atan2<F>(math::sin<F>(angle), math::cos<F>(angle));
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  project_to_manifold(const vec<F, N> &x) noexcept
  {
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = canonical(x.data[i]);
    return r;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  project_to_tangent(const vec<F, N> &, const vec<F, N> &v) noexcept
  {
    return v;
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  exp_map(const vec<F, N> &p, const vec<F, N> &v) noexcept
  {
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = canonical(p.data[i] + v.data[i]);
    return r;
  }

  [[nodiscard, gnu::flatten]] static vec<F, N>
  log_map(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = canonical(q.data[i] - p.data[i]);
    return r;
  }

  [[nodiscard, gnu::always_inline]] static vec<F, N>
  retract(const vec<F, N> &p, const vec<F, N> &v) noexcept
  {
    return exp_map(p, v);
  }

  [[nodiscard, gnu::always_inline]] static vec<F, N>
  inverse_retract(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    return log_map(p, q);
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

  [[nodiscard, gnu::flatten]] static F
  distance(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    return linalg::ops::norm(log_map(p, q));
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  parallel_transport(const vec<F, N> &, const vec<F, N> &, const vec<F, N> &v) noexcept
  {
    return v;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  vector_transport(const vec<F, N> &, const vec<F, N> &, const vec<F, N> &v) noexcept
  {
    return v;
  }
};

template <ieee754_floating F, usize N> struct traits<torus<F, N>> {
  using point_type = vec<F, N>;
  using tangent_type = vec<F, N>;
  using scalar_type = F;
  using category = torus_tag;
  static constexpr usize dim = N;
  static constexpr usize ambient_dim = N;
};

};     // namespace manifolds
};     // namespace math
};     // namespace micron
