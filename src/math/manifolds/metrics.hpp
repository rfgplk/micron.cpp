//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

struct euclidean_metric {
};

struct affine_invariant_metric {
};

struct log_euclidean_metric {
};

struct bures_metric {
};

template<typename T>
concept metric_tag = micron::is_same_v<T, euclidean_metric> || micron::is_same_v<T, affine_invariant_metric>
                     || micron::is_same_v<T, log_euclidean_metric> || micron::is_same_v<T, bures_metric>;

};      // namespace manifolds
};      // namespace math
};      // namespace micron
