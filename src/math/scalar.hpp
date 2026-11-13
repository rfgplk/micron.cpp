//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../types.hpp"
#include "ieee.hpp"

namespace micron::math::scalar
{

template<ieee754_floating F>
[[nodiscard]] constexpr F
add(F left, F right) noexcept
{
  return left + right;
}

template<ieee754_floating F>
[[nodiscard]] constexpr F
sub(F left, F right) noexcept
{
  return left - right;
}

template<ieee754_floating F>
[[nodiscard]] constexpr F
mul(F left, F right) noexcept
{
  return left * right;
}

template<ieee754_floating F>
[[nodiscard]] constexpr F
div(F left, F right) noexcept
{
  return left / right;
}

template<ieee754_floating F>
[[nodiscard]] constexpr F
neg(F value) noexcept
{
  return -value;
}

};      // namespace micron::math::scalar
