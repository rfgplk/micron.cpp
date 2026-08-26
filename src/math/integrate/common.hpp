//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

enum class accumulation_policy : u32 { fast = 0, accurate = 1 };

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
machine_epsilon() noexcept
{
  if constexpr ( sizeof(F) <= 4 )
    return F(1.1920928955078125e-7L);
  else
    return F(2.220446049250313080847263336181640625e-16L);
}

namespace __impl_accumulate
{

template<ieee754_floating F, accumulation_policy Policy> struct sum {
  F value{ 0 };
  F correction{ 0 };

  [[gnu::always_inline]] constexpr void
  add(F x) noexcept
  {
    if constexpr ( Policy == accumulation_policy::fast ) {
      value += x;
    } else {
      const F next = value + x;
      if ( mk::manip::fabs<F>(value) >= mk::manip::fabs<F>(x) )
        correction += (value - next) + x;
      else
        correction += (x - next) + value;
      value = next;
    }
  }

  [[nodiscard, gnu::always_inline]] constexpr F
  get() const noexcept
  {
    if constexpr ( Policy == accumulation_policy::fast )
      return value;
    else
      return value + correction;
  }
};

template<ieee754_floating F> struct runtime_sum {
  F value{ 0 };
  F correction{ 0 };
  accumulation_policy policy{ accumulation_policy::fast };

  constexpr explicit runtime_sum(accumulation_policy p = accumulation_policy::fast) noexcept : policy(p) { }

  [[gnu::always_inline]] constexpr void
  add(F x) noexcept
  {
    if ( policy == accumulation_policy::fast ) {
      value += x;
      return;
    }
    const F next = value + x;
    if ( mk::manip::fabs<F>(value) >= mk::manip::fabs<F>(x) )
      correction += (value - next) + x;
    else
      correction += (x - next) + value;
    value = next;
  }

  [[nodiscard, gnu::always_inline]] constexpr F
  get() const noexcept
  {
    return policy == accumulation_policy::accurate ? value + correction : value;
  }
};

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
max(F a, F b) noexcept
{
  return a > b ? a : b;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
min(F a, F b) noexcept
{
  return a < b ? a : b;
}

};      // namespace __impl_accumulate

};      // namespace integrate
};      // namespace math
};      // namespace micron
