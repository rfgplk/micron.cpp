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

// axis-aligned bounding box
template <ieee754_floating F, usize Dim>
  requires(Dim >= 2 && Dim <= 16)
struct aligned_box {
  vec<F, Dim> min_corner;
  vec<F, Dim> max_corner;

  [[nodiscard]] static constexpr aligned_box
  empty() noexcept
  {
    aligned_box r{};
    // empty box
    for ( usize i = 0; i < Dim; ++i ) {
      r.min_corner.data[i] = F(1);
      r.max_corner.data[i] = F(-1);
    }
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  is_empty() const noexcept
  {
    for ( usize i = 0; i < Dim; ++i )
      if ( min_corner.data[i] > max_corner.data[i] ) return true;
    return false;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  contains(const vec<F, Dim> &p) const noexcept
  {
    for ( usize i = 0; i < Dim; ++i ) {
      if ( p.data[i] < min_corner.data[i] ) return false;
      if ( p.data[i] > max_corner.data[i] ) return false;
    }
    return true;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  contains(const aligned_box &other) const noexcept
  {
    if ( other.is_empty() ) return true;
    for ( usize i = 0; i < Dim; ++i ) {
      if ( other.min_corner.data[i] < min_corner.data[i] ) return false;
      if ( other.max_corner.data[i] > max_corner.data[i] ) return false;
    }
    return true;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  intersects(const aligned_box &other) const noexcept
  {
    if ( is_empty() || other.is_empty() ) return false;
    for ( usize i = 0; i < Dim; ++i ) {
      if ( min_corner.data[i] > other.max_corner.data[i] ) return false;
      if ( max_corner.data[i] < other.min_corner.data[i] ) return false;
    }
    return true;
  }

  constexpr aligned_box &
  extend(const vec<F, Dim> &p) noexcept
  {
    if ( is_empty() ) {
      for ( usize i = 0; i < Dim; ++i ) {
        min_corner.data[i] = p.data[i];
        max_corner.data[i] = p.data[i];
      }
      return *this;
    }
    for ( usize i = 0; i < Dim; ++i ) {
      if ( p.data[i] < min_corner.data[i] ) min_corner.data[i] = p.data[i];
      if ( p.data[i] > max_corner.data[i] ) max_corner.data[i] = p.data[i];
    }
    return *this;
  }

  constexpr aligned_box &
  extend(const aligned_box &other) noexcept
  {
    if ( other.is_empty() ) return *this;
    if ( is_empty() ) {
      *this = other;
      return *this;
    }
    for ( usize i = 0; i < Dim; ++i ) {
      if ( other.min_corner.data[i] < min_corner.data[i] ) min_corner.data[i] = other.min_corner.data[i];
      if ( other.max_corner.data[i] > max_corner.data[i] ) max_corner.data[i] = other.max_corner.data[i];
    }
    return *this;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  center() const noexcept
  {
    vec<F, Dim> r{};
    for ( usize i = 0; i < Dim; ++i ) r.data[i] = (min_corner.data[i] + max_corner.data[i]) * F(0.5);
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  diagonal() const noexcept
  {
    vec<F, Dim> r{};
    for ( usize i = 0; i < Dim; ++i ) r.data[i] = max_corner.data[i] - min_corner.data[i];
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr F
  volume() const noexcept
  {
    if ( is_empty() ) return F(0);
    F v = F(1);
    for ( usize i = 0; i < Dim; ++i ) v *= (max_corner.data[i] - min_corner.data[i]);
    return v;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  corner(usize i) const noexcept
  {
    vec<F, Dim> r{};
    for ( usize a = 0; a < Dim; ++a ) {
      r.data[a] = ((i >> a) & 1ULL) ? max_corner.data[a] : min_corner.data[a];
    }
    return r;
  }
};

};     // namespace geometry
};     // namespace math
};     // namespace micron
