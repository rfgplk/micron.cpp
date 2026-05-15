//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"

#include "../quants/vec.hpp"

#include "mat.hpp"
#include "views.hpp"

namespace micron
{
namespace math
{

template<arith_scalar T> struct dynmat {
  using value_type = T;
  using storage_type = micron::vector<T, micron::allocator_serial<>, false>;

  storage_type buf;
  usize rows{ 0 };
  usize cols{ 0 };
  usize ld{ 0 };

  dynmat() noexcept = default;

  dynmat(usize r, usize c) : buf(r * c), rows(r), cols(c), ld(c) { }

  dynmat(usize r, usize c, const T &fill) : buf(r * c, fill), rows(r), cols(c), ld(c) { }

  dynmat(const dynmat &) = default;
  dynmat(dynmat &&) noexcept = default;
  dynmat &operator=(const dynmat &) = default;
  dynmat &operator=(dynmat &&) noexcept = default;

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
  at(usize r, usize c) noexcept
  {
    return buf.data()[r * ld + c];
  }

  [[nodiscard, gnu::always_inline]] const T &
  at(usize r, usize c) const noexcept
  {
    return buf.data()[r * ld + c];
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

  [[nodiscard]] static dynmat
  zero(usize r, usize c)
  {
    return dynmat(r, c, T(0));
  }

  [[nodiscard]] static dynmat
  with_ld(usize r, usize c, usize lda)
  {
    dynmat m;
    m.buf = storage_type(r * lda);
    m.rows = r;
    m.cols = c;
    m.ld = lda;
    return m;
  }

  [[nodiscard]] static dynmat
  filled(usize r, usize c, const T &v)
  {
    return dynmat(r, c, v);
  }

  [[nodiscard]] static dynmat
  identity(usize n)
  {
    dynmat m(n, n, T(0));
    for ( usize i = 0; i < n; ++i ) m.buf.data()[i * n + i] = T(1);
    return m;
  }

  template<usize R, usize C>
  [[nodiscard]] static dynmat
  from(const mat<T, R, C> &fm)
  {
    dynmat m(R, C);
    for ( usize i = 0; i < R * C; ++i ) m.buf.data()[i] = fm.data[i];
    return m;
  }
};

template<arith_scalar T>
[[nodiscard, gnu::always_inline]] inline matrix::row_view<T>
as_row_view(dynmat<T> &m) noexcept
{
  return matrix::row_view<T>{ m.data(), m.rows, m.cols, m.ld };
}

template<arith_scalar T>
[[nodiscard, gnu::always_inline]] inline matrix::row_view<const T>
as_row_view(const dynmat<T> &m) noexcept
{
  return matrix::row_view<const T>{ m.data(), m.rows, m.cols, m.ld };
}

template<arith_scalar T>
[[nodiscard, gnu::always_inline]] inline matrix::row_view<T>
submat_view(dynmat<T> &m, usize r0, usize c0, usize nr, usize nc) noexcept
{
  return matrix::row_view<T>{ m.data() + r0 * m.ld + c0, nr, nc, m.ld };
}

template<arith_scalar T>
[[nodiscard, gnu::always_inline]] inline matrix::row_view<const T>
submat_view(const dynmat<T> &m, usize r0, usize c0, usize nr, usize nc) noexcept
{
  return matrix::row_view<const T>{ m.data() + r0 * m.ld + c0, nr, nc, m.ld };
}

};      // namespace math
};      // namespace micron
