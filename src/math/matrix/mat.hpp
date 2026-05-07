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
#include "../quants/vec.hpp"
#include "bits.hpp"

namespace micron
{
namespace math
{

template <arith_scalar T, usize R, usize C>
  requires(R >= 1 && C >= 1 && R * C * sizeof(T) <= 1024)
struct mat : public micron::int_matrix_base_avx<T, C, R> {
  using base_type = micron::int_matrix_base_avx<T, C, R>;
  using value_type = T;
  static constexpr usize rows = R;
  static constexpr usize cols = C;

  using base_type::base_type;

  [[nodiscard, gnu::always_inline]] constexpr T &
  at(usize r, usize c) noexcept
  {
    return this->data[r * C + c];
  }

  [[nodiscard, gnu::always_inline]] constexpr const T &
  at(usize r, usize c) const noexcept
  {
    return this->data[r * C + c];
  }

  [[nodiscard, gnu::always_inline]] constexpr T &
  operator[](usize i) noexcept
  {
    return this->data[i];
  }

  [[nodiscard, gnu::always_inline]] constexpr const T &
  operator[](usize i) const noexcept
  {
    return this->data[i];
  }

  using base_type::operator[];

  [[nodiscard]] static constexpr mat
  identity() noexcept
  {
    mat m{};
    for ( usize i = 0; i < R * C; ++i ) m.data[i] = T(0);
    constexpr usize K = (R < C) ? R : C;
    for ( usize i = 0; i < K; ++i ) m.data[i * C + i] = T(1);
    return m;
  }

  [[nodiscard]] static constexpr mat
  zero() noexcept
  {
    mat m{};
    for ( usize i = 0; i < R * C; ++i ) m.data[i] = T(0);
    return m;
  }
};

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr bool
operator==(const mat<T, R, C> &a, const mat<T, R, C> &b) noexcept
{
  for ( usize i = 0; i < R * C; ++i )
    if ( a.data[i] != b.data[i] ) return false;
  return true;
}

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr bool
operator!=(const mat<T, R, C> &a, const mat<T, R, C> &b) noexcept
{
  return !(a == b);
}

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, R, C>
operator+(const mat<T, R, C> &a, const mat<T, R, C> &b) noexcept
{
  mat<T, R, C> r{};
  for ( usize i = 0; i < R * C; ++i ) r.data[i] = a.data[i] + b.data[i];
  return r;
}

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, R, C>
operator-(const mat<T, R, C> &a, const mat<T, R, C> &b) noexcept
{
  mat<T, R, C> r{};
  for ( usize i = 0; i < R * C; ++i ) r.data[i] = a.data[i] - b.data[i];
  return r;
}

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, R, C>
operator*(const mat<T, R, C> &a, T s) noexcept
{
  mat<T, R, C> r{};
  for ( usize i = 0; i < R * C; ++i ) r.data[i] = a.data[i] * s;
  return r;
}

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, R, C>
operator*(T s, const mat<T, R, C> &a) noexcept
{
  return a * s;
}

};     // namespace math
};     // namespace micron
