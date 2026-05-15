//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "concepts.hpp"
#include "tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

template<manifold M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
exp_map(const point_t<M> &p, const tangent_t<M> &v) noexcept
{
  return M::exp_map(p, v);
}

template<lie_group M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
exp_map(const tangent_t<M> &X) noexcept
{
  return M::exp_map(X);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
log_map(const point_t<M> &p, const point_t<M> &q) noexcept
{
  return M::log_map(p, q);
}

template<lie_group M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
log_map(const point_t<M> &g) noexcept
{
  return M::log_map(g);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
retract(const point_t<M> &p, const tangent_t<M> &v) noexcept
{
  return M::retract(p, v);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
inverse_retract(const point_t<M> &p, const point_t<M> &q) noexcept
{
  return M::inverse_retract(p, q);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline auto
parallel_transport(const point_t<M> &p, const point_t<M> &q, const tangent_t<M> &v) noexcept
{
  return M::parallel_transport(p, q, v);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline auto
vector_transport(const point_t<M> &p, const point_t<M> &q, const tangent_t<M> &v) noexcept
{
  return M::vector_transport(p, q, v);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline auto
distance(const point_t<M> &p, const point_t<M> &q) noexcept
{
  return M::distance(p, q);
}

template<riemannian M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
inner(const point_t<M> &p, const tangent_t<M> &u, const tangent_t<M> &v) noexcept
{
  return M::inner(p, u, v);
}

template<riemannian M>
[[nodiscard, gnu::always_inline]] inline auto
norm(const point_t<M> &p, const tangent_t<M> &v) noexcept
{
  return M::norm(p, v);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
project_to_tangent(const point_t<M> &p, const tangent_t<M> &v) noexcept
{
  return M::project_to_tangent(p, v);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline constexpr auto
project_to_manifold(const point_t<M> &x) noexcept
{
  return M::project_to_manifold(x);
}

template<manifold M, typename U>
  requires(micron::is_convertible_v<U, scalar_t<M>>)
[[nodiscard, gnu::always_inline]] inline constexpr auto
geodesic(const point_t<M> &p, const tangent_t<M> &v, U t) noexcept
{
  return M::exp_map(p, v * static_cast<scalar_t<M>>(t));
}

};      // namespace manifolds
};      // namespace math
};      // namespace micron
