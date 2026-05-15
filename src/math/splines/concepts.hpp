//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../quants/vec.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<typename Fn, typename F, usize D>
concept callable_real_curve = requires(Fn fn, F t) {
  { fn(t) } -> micron::convertible_to<vec<F, D>>;
};

};      // namespace splines
};      // namespace math
};      // namespace micron
