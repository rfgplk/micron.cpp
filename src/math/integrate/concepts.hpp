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

namespace micron
{
namespace math
{
namespace integrate
{

template<typename Fn, typename F>
concept callable_real = requires(Fn fn, F x) {
  { fn(x) } -> micron::convertible_to<F>;
};

template<typename Fn, typename F, usize D>
concept callable_real_d = requires(Fn fn, const F (&x)[D]) {
  { fn(x) } -> micron::convertible_to<F>;
};

};      // namespace integrate
};      // namespace math
};      // namespace micron
