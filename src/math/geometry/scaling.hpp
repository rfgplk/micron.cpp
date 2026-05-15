//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../quants/vec.hpp"

namespace micron
{
namespace math
{
namespace geometry
{

template<ieee754_floating F, usize Dim>
  requires(Dim >= 2 && Dim <= 16)
struct scaling {
  vec<F, Dim> s;

  [[nodiscard]] static constexpr scaling
  uniform(F k) noexcept
  {
    scaling r{};
    for ( usize i = 0; i < Dim; ++i ) r.s.data[i] = k;
    return r;
  }

  // undefined when any component is zero
  [[nodiscard, gnu::always_inline]] constexpr scaling
  inverse() const noexcept
  {
    scaling r{};
    for ( usize i = 0; i < Dim; ++i ) r.s.data[i] = F(1) / s.data[i];
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  apply(const vec<F, Dim> &p) const noexcept
  {
    vec<F, Dim> r{};
    for ( usize i = 0; i < Dim; ++i ) r.data[i] = p.data[i] * s.data[i];
    return r;
  }
};

template<ieee754_floating F, usize Dim>
[[nodiscard, gnu::always_inline]] inline constexpr scaling<F, Dim>
operator*(const scaling<F, Dim> &a, const scaling<F, Dim> &b) noexcept
{
  scaling<F, Dim> r{};
  for ( usize i = 0; i < Dim; ++i ) r.s.data[i] = a.s.data[i] * b.s.data[i];
  return r;
}

};      // namespace geometry
};      // namespace math
};      // namespace micron
