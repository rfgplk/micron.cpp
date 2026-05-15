//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

//   row_view<T> -> row-major
//   col_view<T> -> col-major
//   sym_row_view -> row-major decorated
//   sym_col_view -> col-major decorated
//   tri_row_view -> row-major triangular
//   tri_col_view -> col-major triangular

#include "../../concepts.hpp"
#include "../../slice.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "bits.hpp"
#include "mat.hpp"

namespace micron
{
namespace math
{
namespace matrix
{

template<typename T> struct row_view {
  using row_major_tag = void;
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;

  T *data{ nullptr };
  usize rows{ 0 };
  usize cols{ 0 };
  usize ld{ 0 };

  constexpr row_view() noexcept = default;

  constexpr row_view(T *p, usize r, usize c) noexcept : data(p), rows(r), cols(c), ld(c) { }

  constexpr row_view(T *p, usize r, usize c, usize lda) noexcept : data(p), rows(r), cols(c), ld(lda) { }

  [[nodiscard]] static constexpr row_view
  from(T *p, usize r, usize c) noexcept
  {
    return row_view{ p, r, c, c };
  }

  [[nodiscard]] static constexpr row_view
  from(T *p, usize r, usize c, usize lda) noexcept
  {
    return row_view{ p, r, c, lda };
  }

  [[nodiscard]] static constexpr row_view
  from(const raw_slice<T> &s, usize r, usize c) noexcept
  {
    return row_view{ const_cast<T *>(s.ptr), r, c, c };
  }

  template<typename C>
    requires(requires(C &c) {
      { c.data() } -> micron::convertible_to<T *>;
    })
  [[nodiscard]] static constexpr row_view
  from(C &c, usize r, usize cols_) noexcept
  {
    return row_view{ c.data(), r, cols_, cols_ };
  }

  template<usize R, usize Cn>
  [[nodiscard]] static constexpr row_view
  from(mat<T, R, Cn> &m) noexcept
  {
    return row_view{ m.data, R, Cn, Cn };
  }

  template<usize Cn, usize R>
  [[nodiscard]] static constexpr row_view
  from(int_matrix_base<T, Cn, R> &m) noexcept
  {
    return row_view{ m.data, R, Cn, Cn };
  }

  [[nodiscard]] constexpr row_view
  submat(usize r0, usize c0, usize nr, usize nc) const noexcept
  {
    return row_view{ data + r0 * ld + c0, nr, nc, ld };
  }

  [[nodiscard, gnu::always_inline]] constexpr T &
  at(usize r, usize c) noexcept
  {
    return data[r * ld + c];
  }

  [[nodiscard, gnu::always_inline]] constexpr const T &
  at(usize r, usize c) const noexcept
  {
    return data[r * ld + c];
  }
};

template<typename T> struct col_view {
  using col_major_tag = void;
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;

  T *data{ nullptr };
  usize rows{ 0 };
  usize cols{ 0 };
  usize ld{ 0 };

  constexpr col_view() noexcept = default;

  constexpr col_view(T *p, usize r, usize c) noexcept : data(p), rows(r), cols(c), ld(r) { }

  constexpr col_view(T *p, usize r, usize c, usize lda) noexcept : data(p), rows(r), cols(c), ld(lda) { }

  [[nodiscard]] static constexpr col_view
  from(T *p, usize r, usize c) noexcept
  {
    return col_view{ p, r, c, r };
  }

  [[nodiscard]] static constexpr col_view
  from(T *p, usize r, usize c, usize lda) noexcept
  {
    return col_view{ p, r, c, lda };
  }

  [[nodiscard]] static constexpr col_view
  from(const raw_slice<T> &s, usize r, usize c) noexcept
  {
    return col_view{ const_cast<T *>(s.ptr), r, c, r };
  }

  template<typename C>
    requires(requires(C &c) {
      { c.data() } -> micron::convertible_to<T *>;
    })
  [[nodiscard]] static constexpr col_view
  from(C &c, usize r, usize cols_) noexcept
  {
    return col_view{ c.data(), r, cols_, r };
  }

  [[nodiscard]] constexpr col_view
  submat(usize r0, usize c0, usize nr, usize nc) const noexcept
  {
    return col_view{ data + c0 * ld + r0, nr, nc, ld };
  }

  [[nodiscard, gnu::always_inline]] constexpr T &
  at(usize r, usize c) noexcept
  {
    return data[c * ld + r];
  }

  [[nodiscard, gnu::always_inline]] constexpr const T &
  at(usize r, usize c) const noexcept
  {
    return data[c * ld + r];
  }
};

template<typename T>
[[nodiscard, gnu::always_inline]] inline constexpr col_view<T>
transpose_view(const row_view<T> &v) noexcept
{
  return col_view<T>{ v.data, v.cols, v.rows, v.ld };
}

template<typename T>
[[nodiscard, gnu::always_inline]] inline constexpr row_view<T>
transpose_view(const col_view<T> &v) noexcept
{
  return row_view<T>{ v.data, v.cols, v.rows, v.ld };
}

template<typename T, typename Uplo> struct sym_row_view: row_view<T> {
  using uplo_type = Uplo;

  constexpr sym_row_view() noexcept = default;

  constexpr sym_row_view(const row_view<T> &v) noexcept : row_view<T>(v) { }

  constexpr sym_row_view(T *p, usize r, usize c, usize lda) noexcept : row_view<T>(p, r, c, lda) { }
};

template<typename T, typename Uplo, typename Diag> struct tri_row_view: row_view<T> {
  using uplo_type = Uplo;
  using diag_type = Diag;

  constexpr tri_row_view() noexcept = default;

  constexpr tri_row_view(const row_view<T> &v) noexcept : row_view<T>(v) { }

  constexpr tri_row_view(T *p, usize r, usize c, usize lda) noexcept : row_view<T>(p, r, c, lda) { }
};

template<typename T, typename Uplo> struct sym_col_view: col_view<T> {
  using uplo_type = Uplo;

  constexpr sym_col_view() noexcept = default;

  constexpr sym_col_view(const col_view<T> &v) noexcept : col_view<T>(v) { }

  constexpr sym_col_view(T *p, usize r, usize c, usize lda) noexcept : col_view<T>(p, r, c, lda) { }
};

template<typename T, typename Uplo, typename Diag> struct tri_col_view: col_view<T> {
  using uplo_type = Uplo;
  using diag_type = Diag;

  constexpr tri_col_view() noexcept = default;

  constexpr tri_col_view(const col_view<T> &v) noexcept : col_view<T>(v) { }

  constexpr tri_col_view(T *p, usize r, usize c, usize lda) noexcept : col_view<T>(p, r, c, lda) { }
};

};      // namespace matrix
};      // namespace math
};      // namespace micron
