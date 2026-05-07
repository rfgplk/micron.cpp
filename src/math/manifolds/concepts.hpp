//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "tags.hpp"
#include "tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

template <typename M>
concept manifold = requires {
  typename traits<M>::point_type;
  typename traits<M>::tangent_type;
  typename traits<M>::scalar_type;
  typename traits<M>::category;
  { traits<M>::dim } -> micron::convertible_to<usize>;
  { traits<M>::ambient_dim } -> micron::convertible_to<usize>;
};

template <typename M>
concept lie_group = manifold<M> && requires(const point_t<M> &g, const point_t<M> &h, const tangent_t<M> &X) {
  { M::identity() } -> micron::same_as<point_t<M>>;
  { M::compose(g, h) } -> micron::same_as<point_t<M>>;
  { M::inverse(g) } -> micron::same_as<point_t<M>>;
  { M::exp_map(X) } -> micron::same_as<point_t<M>>;
  { M::log_map(g) } -> micron::same_as<tangent_t<M>>;
};

template <typename M>
concept riemannian = manifold<M> && requires(const point_t<M> &p, const tangent_t<M> &u, const tangent_t<M> &v) {
  { M::inner(p, u, v) } -> micron::same_as<scalar_t<M>>;
};

};     // namespace manifolds
};     // namespace math
};     // namespace micron
