//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

struct manifold_tag {
};

struct euclidean_tag: manifold_tag {
};

struct lie_group_tag: manifold_tag {
};

struct sphere_tag: manifold_tag {
};

struct torus_tag: manifold_tag {
};

struct stiefel_tag: manifold_tag {
};

struct grassmann_tag: manifold_tag {
};

struct spd_tag: manifold_tag {
};

struct hyperbolic_tag: manifold_tag {
};

template<typename M> struct traits;

};      // namespace manifolds
};      // namespace math
};      // namespace micron
