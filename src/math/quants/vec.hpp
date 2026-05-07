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

template <typename T>
concept arith_scalar = micron::is_arithmetic_v<T>;

template <arith_scalar T, usize N>
  requires(N >= 2 && N <= 16)
struct alignas(vec_align_v<__conditional_t<micron::is_floating_point_v<T>, T, double>, N>) vec {
  T data[N];

  using value_type = T;
  static constexpr usize length = N;

  static_assert(sizeof(T) * N <= 128, "vec<> exceeding 128 B; prefer mat<> for large vectors");

  [[nodiscard, gnu::always_inline]] constexpr T &
  operator[](usize i) noexcept
  {
    return data[i];
  }

  [[nodiscard, gnu::always_inline]] constexpr const T &
  operator[](usize i) const noexcept
  {
    return data[i];
  }

  template <usize... Is>
  [[nodiscard, gnu::always_inline]] constexpr vec<T, sizeof...(Is)>
  swizzle() const noexcept
  {
    static_assert(((Is < N) && ...), "swizzle index out of range");
    return vec<T, sizeof...(Is)>{ data[Is]... };
  }

  [[gnu::always_inline]] constexpr vec &
  operator+=(const vec &o) noexcept
  {
    for ( usize i = 0; i < N; ++i ) data[i] += o.data[i];
    return *this;
  }

  [[gnu::always_inline]] constexpr vec &
  operator-=(const vec &o) noexcept
  {
    for ( usize i = 0; i < N; ++i ) data[i] -= o.data[i];
    return *this;
  }

  [[gnu::always_inline]] constexpr vec &
  operator*=(const vec &o) noexcept
  {
    for ( usize i = 0; i < N; ++i ) data[i] *= o.data[i];
    return *this;
  }

  [[gnu::always_inline]] constexpr vec &
  operator*=(T s) noexcept
  {
    for ( usize i = 0; i < N; ++i ) data[i] *= s;
    return *this;
  }

  [[gnu::always_inline]] constexpr vec &
  operator/=(T s) noexcept
  {
    for ( usize i = 0; i < N; ++i ) data[i] /= s;
    return *this;
  }
};

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
operator+(const vec<T, N> &a, const vec<T, N> &b) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = a.data[i] + b.data[i];
  return r;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
operator-(const vec<T, N> &a, const vec<T, N> &b) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = a.data[i] - b.data[i];
  return r;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
operator-(const vec<T, N> &a) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = -a.data[i];
  return r;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
operator*(const vec<T, N> &a, T s) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = a.data[i] * s;
  return r;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
operator*(T s, const vec<T, N> &a) noexcept
{
  return a * s;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
operator/(const vec<T, N> &a, T s) noexcept
{
  vec<T, N> r{};
  T inv = T(1) / s;
  for ( usize i = 0; i < N; ++i ) r.data[i] = a.data[i] * inv;
  return r;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr bool
operator==(const vec<T, N> &a, const vec<T, N> &b) noexcept
{
  for ( usize i = 0; i < N; ++i )
    if ( a.data[i] != b.data[i] ) return false;
  return true;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr bool
operator!=(const vec<T, N> &a, const vec<T, N> &b) noexcept
{
  return !(a == b);
}

};     // namespace math
};     // namespace micron
