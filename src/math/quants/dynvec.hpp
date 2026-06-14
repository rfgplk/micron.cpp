//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// runtime-sized dense vector

#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"

#include "vec.hpp"
#include "views.hpp"

namespace micron
{
namespace math
{

template<arith_scalar T> struct dynvec {
  using value_type = T;
  using storage_type = micron::vector<T, micron::allocator_serial<>, false>;

  storage_type buf;

  dynvec() noexcept = default;

  explicit dynvec(usize n) : buf(n) { }

  dynvec(usize n, const T &fill) : buf(n, fill) { }

  dynvec(const dynvec &) = default;
  dynvec(dynvec &&) noexcept = default;
  dynvec &operator=(const dynvec &) = default;
  dynvec &operator=(dynvec &&) noexcept = default;

  template<usize N>
  [[nodiscard]] static dynvec
  from(const vec<T, N> &v)
  {
    dynvec r(N);
    for ( usize i = 0; i < N; ++i ) r.buf.data()[i] = v.data[i];
    return r;
  }

  [[nodiscard, gnu::always_inline]] T *
  data() noexcept
  {
    return buf.data();
  }

  [[nodiscard, gnu::always_inline]] const T *
  data() const noexcept
  {
    return buf.data();
  }

  [[nodiscard, gnu::always_inline]] usize
  size() const noexcept
  {
    return buf.size();
  }

  [[nodiscard, gnu::always_inline]] bool
  empty() const noexcept
  {
    return buf.empty();
  }

  [[nodiscard, gnu::always_inline]] T &
  operator[](usize i) noexcept
  {
    return buf.data()[i];
  }

  [[nodiscard, gnu::always_inline]] const T &
  operator[](usize i) const noexcept
  {
    return buf.data()[i];
  }

  [[nodiscard, gnu::always_inline]] T &
  at(usize i)
  {
    if ( i >= buf.size() ) exc<except::library_error>("micron::math::dynvec at() out of bounds");
    return buf.data()[i];
  }

  [[nodiscard, gnu::always_inline]] const T &
  at(usize i) const
  {
    if ( i >= buf.size() ) exc<except::library_error>("micron::math::dynvec at() out of bounds");
    return buf.data()[i];
  }

  [[nodiscard]] static dynvec
  zero(usize n)
  {
    return dynvec(n, T(0));
  }

  [[nodiscard]] static dynvec
  filled(usize n, const T &v)
  {
    return dynvec(n, v);
  }
};

template<arith_scalar T>
[[nodiscard, gnu::always_inline]] inline quants::vec_view<T>
as_view(dynvec<T> &v) noexcept
{
  return quants::vec_view<T>{ v.data(), v.size(), 1 };
}

template<arith_scalar T>
[[nodiscard, gnu::always_inline]] inline quants::vec_view<const T>
as_view(const dynvec<T> &v) noexcept
{
  return quants::vec_view<const T>{ v.data(), v.size(), 1 };
}

};      // namespace math
};      // namespace micron
