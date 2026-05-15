//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "tags.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

template<typename M> using point_t = typename traits<M>::point_type;
template<typename M> using tangent_t = typename traits<M>::tangent_type;
template<typename M> using scalar_t = typename traits<M>::scalar_type;
template<typename M> using category_t = typename traits<M>::category;

};      // namespace manifolds
};      // namespace math
};      // namespace micron
