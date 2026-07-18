//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"

namespace micron
{
namespace math
{
// soleus artifact, consider porting over to vec_types
template<ieee754_floating F> struct alignas(vec_align_v<F, 4>) quat {
  F data[4];

  using value_type = F;
  static constexpr usize length = 4;

  [[nodiscard, gnu::always_inline]] constexpr F &
  x() noexcept
  {
    return data[0];
  }

  [[nodiscard, gnu::always_inline]] constexpr F &
  y() noexcept
  {
    return data[1];
  }

  [[nodiscard, gnu::always_inline]] constexpr F &
  z() noexcept
  {
    return data[2];
  }

  [[nodiscard, gnu::always_inline]] constexpr F &
  w() noexcept
  {
    return data[3];
  }

  [[nodiscard, gnu::always_inline]] constexpr const F &
  x() const noexcept
  {
    return data[0];
  }

  [[nodiscard, gnu::always_inline]] constexpr const F &
  y() const noexcept
  {
    return data[1];
  }

  [[nodiscard, gnu::always_inline]] constexpr const F &
  z() const noexcept
  {
    return data[2];
  }

  [[nodiscard, gnu::always_inline]] constexpr const F &
  w() const noexcept
  {
    return data[3];
  }

  [[nodiscard, gnu::always_inline]] constexpr F &
  operator[](usize i) noexcept
  {
    return data[i];
  }

  [[nodiscard, gnu::always_inline]] constexpr const F &
  operator[](usize i) const noexcept
  {
    return data[i];
  }

  [[nodiscard]] static constexpr quat
  identity() noexcept
  {
    return { F(0), F(0), F(0), F(1) };
  }
};

};      // namespace math
};      // namespace micron
