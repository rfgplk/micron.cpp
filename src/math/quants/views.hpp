//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// NOTE: these are all used by the BLAS routines

#include "../../concepts.hpp"
#include "../../slice.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{
namespace math
{
namespace quants
{

template <typename T> struct vec_view {
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;

  T *data{ nullptr };
  usize n{ 0 };
  ssize_t inc{ 1 };

  constexpr vec_view() noexcept = default;

  constexpr vec_view(T *p, usize len, ssize_t stride = 1) noexcept : data(p), n(len), inc(stride) {}

  constexpr vec_view(const raw_slice<T> &s) noexcept : data(const_cast<T *>(s.ptr)), n(s.len), inc(1) {}

  constexpr vec_view(raw_slice<T> &s) noexcept : data(s.ptr), n(s.len), inc(1) {}

  [[nodiscard]] static constexpr vec_view
  from(T *p, usize len, ssize_t stride = 1) noexcept
  {
    return vec_view{ p, len, stride };
  }

  [[nodiscard]] static constexpr vec_view
  from(const raw_slice<T> &s) noexcept
  {
    return vec_view{ const_cast<T *>(s.ptr), s.len, 1 };
  }

  template <typename C>
    requires(requires(C &c) {
      { c.data() } -> micron::convertible_to<T *>;
      { c.size() } -> micron::convertible_to<usize>;
    })
  [[nodiscard]] static constexpr vec_view
  from(C &c) noexcept
  {
    return vec_view{ c.data(), c.size(), 1 };
  }

  template <typename C>
    requires(requires(const C &c) {
      { c.data() } -> micron::convertible_to<const T *>;
      { c.size() } -> micron::convertible_to<usize>;
    })
  [[nodiscard]] static constexpr vec_view<const T>
  from(const C &c) noexcept
  {
    return vec_view<const T>{ c.data(), c.size(), 1 };
  }

  [[nodiscard]] static constexpr vec_view
  strided(T *p, usize len, ssize_t stride) noexcept
  {
    return vec_view{ p, len, stride };
  }

  [[nodiscard, gnu::always_inline]] constexpr T &
  operator[](usize i) noexcept
  {
    return data[ssize_t(i) * inc];
  }

  [[nodiscard, gnu::always_inline]] constexpr const T &
  operator[](usize i) const noexcept
  {
    return data[ssize_t(i) * inc];
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  size() const noexcept
  {
    return n;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  is_unit_stride() const noexcept
  {
    return inc == 1;
  }
};

template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr T *
addr(const vec_view<T> &v, usize i) noexcept
{
  return v.data + ssize_t(i) * v.inc;
}

};     // namespace quants
};     // namespace math
};     // namespace micron
