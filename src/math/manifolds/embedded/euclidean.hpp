//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// flat Euclidean mf

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../linalg/ops.hpp"
#include "../../quants/vec.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

template<ieee754_floating F, usize N>
  requires(N >= 2 && N <= 16)
struct euclidean {
  using value_type = F;
  static constexpr usize length = N;

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  identity() noexcept
  {
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = F(0);
    return r;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  compose(const vec<F, N> &a, const vec<F, N> &b) noexcept
  {
    return a + b;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  inverse(const vec<F, N> &p) noexcept
  {
    return -p;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  exp_map(const vec<F, N> &v) noexcept
  {
    return v;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  exp_map(const vec<F, N> &p, const vec<F, N> &v) noexcept
  {
    return p + v;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  log_map(const vec<F, N> &p) noexcept
  {
    return p;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  log_map(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    return q - p;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  retract(const vec<F, N> &p, const vec<F, N> &v) noexcept
  {
    return p + v;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  inverse_retract(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    return q - p;
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

  [[nodiscard, gnu::always_inline]] static F
  distance(const vec<F, N> &p, const vec<F, N> &q) noexcept
  {
    return linalg::ops::norm(q - p);
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

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  project_to_tangent(const vec<F, N> &, const vec<F, N> &v) noexcept
  {
    return v;
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  project_to_manifold(const vec<F, N> &x) noexcept
  {
    return x;
  }
};

template<ieee754_floating F, usize N> struct traits<euclidean<F, N>> {
  using point_type = vec<F, N>;
  using tangent_type = vec<F, N>;
  using scalar_type = F;
  using category = euclidean_tag;
  static constexpr usize dim = N;
  static constexpr usize ambient_dim = N;
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
