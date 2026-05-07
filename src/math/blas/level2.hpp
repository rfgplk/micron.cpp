//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// BLAS level 2 routines, quadratic time
//
//  gemv
//  symv
//  trmv
//  trsv
//  ger
//  syr
//  syr2

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../matrix/views.hpp"
#include "../quants/views.hpp"
#include "bits/impl.hpp"
#include "concepts.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace blas
{
namespace level2
{

using micron::math::matrix::col_view;
using micron::math::matrix::row_view;
using micron::math::matrix::sym_col_view;
using micron::math::matrix::sym_row_view;
using micron::math::matrix::tri_col_view;
using micron::math::matrix::tri_row_view;
using micron::math::quants::vec_view;

template <op::op_tag Op = op::none, blas_scalar T>
[[gnu::flatten]] inline constexpr void
gemv(T alpha, const row_view<T> &A, const vec_view<T> &x, T beta, vec_view<T> y) noexcept
{
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  bits::gemv_kernel<T>(tr, A.rows, A.cols, alpha, A.data, ssize_t(A.ld), 1, x.data, x.inc, beta, y.data, y.inc);
}

template <op::op_tag Op = op::none, blas_scalar T>
[[gnu::flatten]] inline constexpr void
gemv(T alpha, const col_view<T> &A, const vec_view<T> &x, T beta, vec_view<T> y) noexcept
{
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  bits::gemv_kernel<T>(tr, A.rows, A.cols, alpha, A.data, 1, ssize_t(A.ld), x.data, x.inc, beta, y.data, y.inc);
}

// row major
template <blas_scalar T>
[[gnu::flatten]] inline constexpr void
gemv_row(bool tr, usize m, usize n, T alpha, const T *A, usize lda, const T *x, ssize_t incx, T beta, T *y, ssize_t incy) noexcept
{
  bits::gemv_kernel<T>(tr, m, n, alpha, A, ssize_t(lda), 1, x, incx, beta, y, incy);
}

// col major
template <blas_scalar T>
[[gnu::flatten]] inline constexpr void
gemv_col(bool tr, usize m, usize n, T alpha, const T *A, usize lda, const T *x, ssize_t incx, T beta, T *y, ssize_t incy) noexcept
{
  bits::gemv_kernel<T>(tr, m, n, alpha, A, 1, ssize_t(lda), x, incx, beta, y, incy);
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
symv(T alpha, const sym_row_view<T, U> &A, const vec_view<T> &x, T beta, vec_view<T> y) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  bits::symv_kernel<T>(upper, A.rows, alpha, A.data, ssize_t(A.ld), 1, x.data, x.inc, beta, y.data, y.inc);
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
symv(T alpha, const sym_col_view<T, U> &A, const vec_view<T> &x, T beta, vec_view<T> y) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  bits::symv_kernel<T>(upper, A.rows, alpha, A.data, 1, ssize_t(A.ld), x.data, x.inc, beta, y.data, y.inc);
}

template <op::op_tag Op = op::none, blas_scalar T, uplo::uplo_tag U, diag::diag_tag D>
[[gnu::flatten]] inline constexpr void
trmv(const tri_row_view<T, U, D> &A, vec_view<T> x) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  constexpr bool unit_diag = micron::same_as<D, diag::unit>;
  bits::trmv_kernel<T>(upper, tr, unit_diag, A.rows, A.data, ssize_t(A.ld), 1, x.data, x.inc);
}

template <op::op_tag Op = op::none, blas_scalar T, uplo::uplo_tag U, diag::diag_tag D>
[[gnu::flatten]] inline constexpr void
trmv(const tri_col_view<T, U, D> &A, vec_view<T> x) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  constexpr bool unit_diag = micron::same_as<D, diag::unit>;
  bits::trmv_kernel<T>(upper, tr, unit_diag, A.rows, A.data, 1, ssize_t(A.ld), x.data, x.inc);
}

template <op::op_tag Op = op::none, blas_scalar T, uplo::uplo_tag U, diag::diag_tag D>
[[gnu::flatten]] inline constexpr void
trsv(const tri_row_view<T, U, D> &A, vec_view<T> b) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  constexpr bool unit_diag = micron::same_as<D, diag::unit>;
  bits::trsv_kernel<T>(upper, tr, unit_diag, A.rows, A.data, ssize_t(A.ld), 1, b.data, b.inc);
}

template <op::op_tag Op = op::none, blas_scalar T, uplo::uplo_tag U, diag::diag_tag D>
[[gnu::flatten]] inline constexpr void
trsv(const tri_col_view<T, U, D> &A, vec_view<T> b) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  constexpr bool unit_diag = micron::same_as<D, diag::unit>;
  bits::trsv_kernel<T>(upper, tr, unit_diag, A.rows, A.data, 1, ssize_t(A.ld), b.data, b.inc);
}

template <blas_scalar T>
[[gnu::flatten]] inline constexpr void
ger(T alpha, const vec_view<T> &x, const vec_view<T> &y, row_view<T> A) noexcept
{
  bits::ger_kernel<T>(A.rows, A.cols, alpha, x.data, x.inc, y.data, y.inc, A.data, ssize_t(A.ld), 1);
}

template <blas_scalar T>
[[gnu::flatten]] inline constexpr void
ger(T alpha, const vec_view<T> &x, const vec_view<T> &y, col_view<T> A) noexcept
{
  bits::ger_kernel<T>(A.rows, A.cols, alpha, x.data, x.inc, y.data, y.inc, A.data, 1, ssize_t(A.ld));
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
syr(T alpha, const vec_view<T> &x, sym_row_view<T, U> A) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  bits::syr_kernel<T>(upper, A.rows, alpha, x.data, x.inc, A.data, ssize_t(A.ld), 1);
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
syr(T alpha, const vec_view<T> &x, sym_col_view<T, U> A) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  bits::syr_kernel<T>(upper, A.rows, alpha, x.data, x.inc, A.data, 1, ssize_t(A.ld));
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
syr2(T alpha, const vec_view<T> &x, const vec_view<T> &y, sym_row_view<T, U> A) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  bits::syr2_kernel<T>(upper, A.rows, alpha, x.data, x.inc, y.data, y.inc, A.data, ssize_t(A.ld), 1);
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
syr2(T alpha, const vec_view<T> &x, const vec_view<T> &y, sym_col_view<T, U> A) noexcept
{
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  bits::syr2_kernel<T>(upper, A.rows, alpha, x.data, x.inc, y.data, y.inc, A.data, 1, ssize_t(A.ld));
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
hemv(T alpha, const sym_row_view<T, U> &A, const vec_view<T> &x, T beta, vec_view<T> y) noexcept
{
  symv<T, U>(alpha, A, x, beta, y);
}

template <blas_scalar T, uplo::uplo_tag U>
[[gnu::flatten]] inline constexpr void
hemv(T alpha, const sym_col_view<T, U> &A, const vec_view<T> &x, T beta, vec_view<T> y) noexcept
{
  symv<T, U>(alpha, A, x, beta, y);
}

};     // namespace level2
};     // namespace blas
};     // namespace math
};     // namespace micron
