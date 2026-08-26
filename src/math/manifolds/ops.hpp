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

template<lie_group M>
[[nodiscard, gnu::always_inline]] inline constexpr point_t<M>
identity() noexcept
{
  return M::identity();
}

template<lie_group M>
[[nodiscard, gnu::always_inline]] inline constexpr point_t<M>
compose(const point_t<M> &a, const point_t<M> &b) noexcept
{
  return M::compose(a, b);
}

template<lie_group M>
[[nodiscard, gnu::always_inline]] inline constexpr point_t<M>
inverse(const point_t<M> &g) noexcept
{
  return M::inverse(g);
}

template<lie_group M>
[[nodiscard, gnu::always_inline]] inline constexpr point_t<M>
between(const point_t<M> &a, const point_t<M> &b) noexcept
{
  if constexpr ( requires { M::between(a, b); } )
    return M::between(a, b);
  else
    return M::compose(M::inverse(a), b);
}

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

template<riemannian M>
[[nodiscard, gnu::always_inline]] inline constexpr scalar_t<M>
squared_norm(const point_t<M> &p, const tangent_t<M> &v) noexcept
{
  return M::inner(p, v, v);
}

template<manifold M>
[[nodiscard, gnu::always_inline]] inline scalar_t<M>
squared_distance(const point_t<M> &p, const point_t<M> &q) noexcept
{
  if constexpr ( requires { M::squared_distance(p, q); } )
    return M::squared_distance(p, q);
  else {
    const scalar_t<M> d = M::distance(p, q);
    return d * d;
  }
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
  requires(micron::is_convertible_v<U, scalar_t<M>> && requires(const point_t<M> &p, const tangent_t<M> &v) { M::exp_map(p, v); })
[[nodiscard, gnu::always_inline]] inline constexpr auto
geodesic(const point_t<M> &p, const tangent_t<M> &v, U t) noexcept
{
  return M::exp_map(p, v * static_cast<scalar_t<M>>(t));
}

template<manifold M, typename U>
  requires(!lie_group<M> && micron::is_convertible_v<U, scalar_t<M>>
           && requires(const point_t<M> &p, const point_t<M> &q, const tangent_t<M> &v) {
                M::log_map(p, q);
                M::exp_map(p, v);
              })
[[nodiscard, gnu::always_inline]] inline constexpr point_t<M>
interpolate(const point_t<M> &p, const point_t<M> &q, U t) noexcept
{
  return M::exp_map(p, M::log_map(p, q) * static_cast<scalar_t<M>>(t));
}

template<lie_group M, typename U>
  requires(micron::is_convertible_v<U, scalar_t<M>>)
[[nodiscard, gnu::always_inline]] inline constexpr point_t<M>
interpolate(const point_t<M> &a, const point_t<M> &b, U t) noexcept
{
  if constexpr ( requires { M::interpolate(a, b, static_cast<scalar_t<M>>(t)); } )
    return M::interpolate(a, b, static_cast<scalar_t<M>>(t));
  else {
    const auto delta = M::log_map(between<M>(a, b));
    return M::compose(a, M::exp_map(delta * static_cast<scalar_t<M>>(t)));
  }
}

template<manifold M, typename V>
  requires requires(const point_t<M> &g, const V &v) { M::act(g, v); }
[[nodiscard, gnu::always_inline]] inline constexpr auto
act(const point_t<M> &g, const V &v) noexcept
{
  return M::act(g, v);
}

};      // namespace manifolds
};      // namespace math
};      // namespace micron
