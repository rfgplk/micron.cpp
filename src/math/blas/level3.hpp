//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// BLAS level 3 (cubic time)
//
//  gemm
//  symm
//  syrk
//  syr2k
//  trmm
//  trsm

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../matrix/dynmat.hpp"
#include "../matrix/pack.hpp"
#include "../matrix/views.hpp"
#include "bits/impl.hpp"
#include "concepts.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace blas
{
namespace level3
{

using micron::math::matrix::col_view;
using micron::math::matrix::row_view;
using micron::math::matrix::sym_col_view;
using micron::math::matrix::sym_row_view;
using micron::math::matrix::tri_col_view;
using micron::math::matrix::tri_row_view;

namespace __impl_level3
{

template <typename V>
[[nodiscard, gnu::always_inline]] inline constexpr ssize_t
rs_of(const V &v) noexcept
{
  if constexpr ( row_view_like<V> )
    return ssize_t(v.ld);
  else
    return 1;
}

template <typename V>
[[nodiscard, gnu::always_inline]] inline constexpr ssize_t
cs_of(const V &v) noexcept
{
  if constexpr ( row_view_like<V> )
    return 1;
  else
    return ssize_t(v.ld);
}

template <typename Va, typename Vb>
[[nodiscard, gnu::always_inline]] inline bool
view_overlaps(const Va &a, const Vb &b) noexcept
{
  if ( a.rows == 0 || a.cols == 0 || b.rows == 0 || b.cols == 0 ) return false;
  const ssize_t a_rs = rs_of(a);
  const ssize_t a_cs = cs_of(a);
  const ssize_t b_rs = rs_of(b);
  const ssize_t b_cs = cs_of(b);
  const ssize_t a_sz = ssize_t(sizeof(typename Va::value_type));
  const ssize_t b_sz = ssize_t(sizeof(typename Vb::value_type));
  const auto *a_lo = reinterpret_cast<const unsigned char *>(a.data);
  const auto *b_lo = reinterpret_cast<const unsigned char *>(b.data);
  const auto *a_hi = a_lo + (ssize_t(a.rows - 1) * a_rs + ssize_t(a.cols - 1) * a_cs) * a_sz + a_sz;
  const auto *b_hi = b_lo + (ssize_t(b.rows - 1) * b_rs + ssize_t(b.cols - 1) * b_cs) * b_sz + b_sz;
  return (a_lo < b_hi) && (b_lo < a_hi);
}

};     // namespace __impl_level3

template <op::op_tag OpA = op::none, op::op_tag OpB = op::none, blas_scalar T, typename VA, typename VB, typename VC>
  requires(mat_view_like<VA> and mat_view_like<VB> and mat_view_like<VC> and micron::same_as<typename VA::value_type, T>
           and micron::same_as<typename VB::value_type, T> and micron::same_as<typename VC::value_type, T>)
[[gnu::flatten]] inline constexpr void
gemm(T alpha, const VA &A, const VB &B, T beta, VC C) noexcept
{
  constexpr bool trA = micron::same_as<OpA, op::trans> or micron::same_as<OpA, op::conj_trans>;
  constexpr bool trB = micron::same_as<OpB, op::trans> or micron::same_as<OpB, op::conj_trans>;
  const usize m = trA ? A.cols : A.rows;
  const usize k = trA ? A.rows : A.cols;
  const usize n = trB ? B.rows : B.cols;

  if !consteval {
    if ( __impl_level3::view_overlaps(C, A) || __impl_level3::view_overlaps(C, B) ) {
      micron::math::dynmat<T> tmp(m, n);
      bits::gemm_kernel<T>(trA, trB, m, n, k, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), B.data,
                           __impl_level3::rs_of(B), __impl_level3::cs_of(B), T(0), tmp.data(), ssize_t(tmp.ld), ssize_t(1));
      const ssize_t rs_C = __impl_level3::rs_of(C);
      const ssize_t cs_C = __impl_level3::cs_of(C);
      for ( usize i = 0; i < m; ++i ) {
        for ( usize j = 0; j < n; ++j ) {
          T &cij = C.data[ssize_t(i) * rs_C + ssize_t(j) * cs_C];
          cij = beta * cij + tmp.data()[i * tmp.ld + j];
        }
      }
      return;
    }
  }

  bits::gemm_kernel<T>(trA, trB, m, n, k, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), B.data, __impl_level3::rs_of(B),
                       __impl_level3::cs_of(B), beta, C.data, __impl_level3::rs_of(C), __impl_level3::cs_of(C));
}

template <blas_scalar T>
[[gnu::flatten]] inline constexpr void
gemm_row(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, usize lda, const T *B, usize ldb, T beta, T *C,
         usize ldc) noexcept
{
  bits::gemm_kernel<T>(trA, trB, m, n, k, alpha, A, ssize_t(lda), 1, B, ssize_t(ldb), 1, beta, C, ssize_t(ldc), 1);
}

template <blas_scalar T>
[[gnu::flatten]] inline constexpr void
gemm_col(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, usize lda, const T *B, usize ldb, T beta, T *C,
         usize ldc) noexcept
{
  bits::gemm_kernel<T>(trA, trB, m, n, k, alpha, A, 1, ssize_t(lda), B, 1, ssize_t(ldb), beta, C, 1, ssize_t(ldc));
}

template <side::side_tag S = side::left, blas_scalar T, typename VA, typename VB, typename VC>
  requires(mat_view_like<VB> and mat_view_like<VC>)
[[gnu::flatten]] inline constexpr void
symm(T alpha, const VA &A, const VB &B, T beta, VC C) noexcept
{
  constexpr bool upper = micron::same_as<typename VA::uplo_type, uplo::upper>;
  if constexpr ( micron::same_as<S, side::left> ) {
    bits::symm_left_kernel<T>(upper, B.rows, B.cols, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), B.data,
                              __impl_level3::rs_of(B), __impl_level3::cs_of(B), beta, C.data, __impl_level3::rs_of(C),
                              __impl_level3::cs_of(C));
  } else {
    bits::symm_right_kernel<T>(upper, B.rows, B.cols, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), B.data,
                               __impl_level3::rs_of(B), __impl_level3::cs_of(B), beta, C.data, __impl_level3::rs_of(C),
                               __impl_level3::cs_of(C));
  }
}

template <side::side_tag S = side::left, blas_scalar T, typename VA, typename VB, typename VC>
[[gnu::flatten]] inline constexpr void
hemm(T alpha, const VA &A, const VB &B, T beta, VC C) noexcept
{
  symm<S, T>(alpha, A, B, beta, C);
}

template <op::op_tag OpA = op::none, blas_scalar T, typename VA, typename VC>
  requires(mat_view_like<VA>)
[[gnu::flatten]] inline constexpr void
syrk(T alpha, const VA &A, T beta, VC C) noexcept
{
  constexpr bool trA = micron::same_as<OpA, op::trans> or micron::same_as<OpA, op::conj_trans>;
  constexpr bool upper = micron::same_as<typename VC::uplo_type, uplo::upper>;
  const usize n = trA ? A.cols : A.rows;
  const usize k = trA ? A.rows : A.cols;
  bits::syrk_kernel<T>(upper, trA, n, k, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), beta, C.data,
                       __impl_level3::rs_of(C), __impl_level3::cs_of(C));
}

template <op::op_tag OpA = op::none, blas_scalar T, typename VA, typename VC>
[[gnu::flatten]] inline constexpr void
herk(T alpha, const VA &A, T beta, VC C) noexcept
{
  syrk<OpA, T>(alpha, A, beta, C);
}

template <op::op_tag OpA = op::none, blas_scalar T, typename VA, typename VB, typename VC>
  requires(mat_view_like<VA> and mat_view_like<VB>)
[[gnu::flatten]] inline constexpr void
syr2k(T alpha, const VA &A, const VB &B, T beta, VC C) noexcept
{
  constexpr bool trA = micron::same_as<OpA, op::trans> or micron::same_as<OpA, op::conj_trans>;
  constexpr bool upper = micron::same_as<typename VC::uplo_type, uplo::upper>;
  const usize n = trA ? A.cols : A.rows;
  const usize k = trA ? A.rows : A.cols;
  bits::syr2k_kernel<T>(upper, trA, n, k, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), B.data, __impl_level3::rs_of(B),
                        __impl_level3::cs_of(B), beta, C.data, __impl_level3::rs_of(C), __impl_level3::cs_of(C));
}

template <op::op_tag OpA = op::none, blas_scalar T, typename VA, typename VB, typename VC>
[[gnu::flatten]] inline constexpr void
her2k(T alpha, const VA &A, const VB &B, T beta, VC C) noexcept
{
  syr2k<OpA, T>(alpha, A, B, beta, C);
}

template <side::side_tag S = side::left, op::op_tag Op = op::none, blas_scalar T, typename VA, typename VB>
  requires(mat_view_like<VA> and mat_view_like<VB>)
[[gnu::flatten]] inline constexpr void
trmm(T alpha, const VA &A, VB B) noexcept
{
  constexpr bool upper = micron::same_as<typename VA::uplo_type, uplo::upper>;
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  constexpr bool unit_diag = micron::same_as<typename VA::diag_type, diag::unit>;
  if constexpr ( micron::same_as<S, side::left> ) {
    bits::trmm_left_kernel<T>(upper, tr, unit_diag, B.rows, B.cols, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), B.data,
                              __impl_level3::rs_of(B), __impl_level3::cs_of(B));
  } else {
    bits::trmm_right_kernel<T>(upper, tr, unit_diag, B.rows, B.cols, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A),
                               B.data, __impl_level3::rs_of(B), __impl_level3::cs_of(B));
  }
}

template <side::side_tag S = side::left, op::op_tag Op = op::none, blas_scalar T, typename VA, typename VB>
  requires(mat_view_like<VA> and mat_view_like<VB>)
[[gnu::flatten]] inline constexpr void
trsm(T alpha, const VA &A, VB B) noexcept
{
  constexpr bool upper = micron::same_as<typename VA::uplo_type, uplo::upper>;
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  constexpr bool unit_diag = micron::same_as<typename VA::diag_type, diag::unit>;
  if constexpr ( micron::same_as<S, side::left> ) {
    bits::trsm_left_kernel<T>(upper, tr, unit_diag, B.rows, B.cols, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A), B.data,
                              __impl_level3::rs_of(B), __impl_level3::cs_of(B));
  } else {
    bits::trsm_right_kernel<T>(upper, tr, unit_diag, B.rows, B.cols, alpha, A.data, __impl_level3::rs_of(A), __impl_level3::cs_of(A),
                               B.data, __impl_level3::rs_of(B), __impl_level3::cs_of(B));
  }
}

};     // namespace level3
};     // namespace blas
};     // namespace math
};     // namespace micron
