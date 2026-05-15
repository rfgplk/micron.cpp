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
struct translation {
  vec<F, Dim> t;

  [[nodiscard, gnu::always_inline]] constexpr translation
  inverse() const noexcept
  {
    translation r{};
    for ( usize i = 0; i < Dim; ++i ) r.t.data[i] = -t.data[i];
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  apply(const vec<F, Dim> &p) const noexcept
  {
    vec<F, Dim> r{};
    for ( usize i = 0; i < Dim; ++i ) r.data[i] = p.data[i] + t.data[i];
    return r;
  }
};

template<ieee754_floating F, usize Dim>
[[nodiscard, gnu::always_inline]] inline constexpr translation<F, Dim>
operator*(const translation<F, Dim> &a, const translation<F, Dim> &b) noexcept
{
  translation<F, Dim> r{};
  for ( usize i = 0; i < Dim; ++i ) r.t.data[i] = a.t.data[i] + b.t.data[i];
  return r;
}

};      // namespace geometry
};      // namespace math
};      // namespace micron
