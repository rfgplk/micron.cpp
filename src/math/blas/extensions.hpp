//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../matrix/views.hpp"
#include "../quants/views.hpp"
#include "bits/impl.hpp"
#include "concepts.hpp"
#include "level1.hpp"
#include "level2.hpp"
#include "level3.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace blas
{
namespace ext
{

using micron::math::matrix::col_view;
using micron::math::matrix::row_view;
using micron::math::quants::vec_view;

template<op::op_tag Op = op::none, blas_scalar T, typename VA, typename VB>
  requires(mat_view_like<VA> and mat_view_like<VB>)
[[gnu::flatten]] inline constexpr void
omatcopy(T alpha, const VA &A, VB B) noexcept
{
  constexpr bool tr = micron::same_as<Op, op::trans> or micron::same_as<Op, op::conj_trans>;
  const usize m = tr ? A.cols : A.rows;
  const usize n = tr ? A.rows : A.cols;
  const ssize_t rs_A = level3::__impl_level3::rs_of(A);
  const ssize_t cs_A = level3::__impl_level3::cs_of(A);
  const ssize_t rs_B = level3::__impl_level3::rs_of(B);
  const ssize_t cs_B = level3::__impl_level3::cs_of(B);
  for ( usize i = 0; i < m; ++i )
    for ( usize j = 0; j < n; ++j )
      bits::mat_at(B.data, i, j, rs_B, cs_B)
          = alpha * (tr ? bits::mat_at(A.data, j, i, rs_A, cs_A) : bits::mat_at(A.data, i, j, rs_A, cs_A));
}

template<op::op_tag Op = op::none, blas_scalar T, typename VA>
  requires(mat_view_like<VA>)
[[gnu::flatten]] inline constexpr void
imatcopy(T alpha, VA A) noexcept
{
  const ssize_t rs = level3::__impl_level3::rs_of(A);
  const ssize_t cs = level3::__impl_level3::cs_of(A);
  if constexpr ( micron::same_as<Op, op::none> ) {
    for ( usize i = 0; i < A.rows; ++i )
      for ( usize j = 0; j < A.cols; ++j ) bits::mat_at(A.data, i, j, rs, cs) *= alpha;
  } else {
    for ( usize i = 0; i < A.rows; ++i ) {
      bits::mat_at(A.data, i, i, rs, cs) *= alpha;
      for ( usize j = i + 1; j < A.cols; ++j ) {
        T &aij = bits::mat_at(A.data, i, j, rs, cs);
        T &aji = bits::mat_at(A.data, j, i, rs, cs);
        const T t = alpha * aji;
        aji = alpha * aij;
        aij = t;
      }
    }
  }
}

template<uplo::uplo_tag U, op::op_tag OpA = op::none, op::op_tag OpB = op::none, blas_scalar T, typename VA, typename VB, typename VC>
  requires(mat_view_like<VA> and mat_view_like<VB> and mat_view_like<VC>)
[[gnu::flatten]] inline constexpr void
gemmt(T alpha, const VA &A, const VB &B, T beta, VC C) noexcept
{
  constexpr bool trA = micron::same_as<OpA, op::trans> or micron::same_as<OpA, op::conj_trans>;
  constexpr bool trB = micron::same_as<OpB, op::trans> or micron::same_as<OpB, op::conj_trans>;
  constexpr bool upper = micron::same_as<U, uplo::upper>;
  const usize n = trA ? A.cols : A.rows;      // C is n × n
  const usize k = trA ? A.rows : A.cols;
  const ssize_t rs_A = level3::__impl_level3::rs_of(A);
  const ssize_t cs_A = level3::__impl_level3::cs_of(A);
  const ssize_t rs_B = level3::__impl_level3::rs_of(B);
  const ssize_t cs_B = level3::__impl_level3::cs_of(B);
  const ssize_t rs_C = level3::__impl_level3::rs_of(C);
  const ssize_t cs_C = level3::__impl_level3::cs_of(C);
  for ( usize i = 0; i < n; ++i ) {
    const usize j_lo = upper ? i : 0;
    const usize j_hi = upper ? n : (i + 1);
    for ( usize j = j_lo; j < j_hi; ++j ) {
      T acc{};
      for ( usize p = 0; p < k; ++p ) {
        const T a = trA ? bits::mat_at(A.data, p, i, rs_A, cs_A) : bits::mat_at(A.data, i, p, rs_A, cs_A);
        const T b = trB ? bits::mat_at(B.data, j, p, rs_B, cs_B) : bits::mat_at(B.data, p, j, rs_B, cs_B);
        acc = bits::fma_acc<T>(a, b, acc);
      }
      T &cij = bits::mat_at(C.data, i, j, rs_C, cs_C);
      cij = bits::fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

template<blas_scalar T>
[[gnu::flatten]] inline void
axpby_batched(usize batch_count, T alpha, const vec_view<T> *xs, T beta, vec_view<T> *ys) noexcept
{
  for ( usize b = 0; b < batch_count; ++b ) level1::axpby<T>(alpha, xs[b], beta, ys[b]);
}

template<blas_scalar T>
[[gnu::flatten]] inline void
dot_batched(usize batch_count, const vec_view<T> *xs, const vec_view<T> *ys, T *out) noexcept
{
  for ( usize b = 0; b < batch_count; ++b ) out[b] = level1::dot<T>(xs[b], ys[b]);
}

template<op::op_tag OpA = op::none, op::op_tag OpB = op::none, blas_scalar T, typename VA, typename VB, typename VC>
  requires(mat_view_like<VA> and mat_view_like<VB> and mat_view_like<VC>)
[[gnu::flatten]] inline void
gemm_batched(usize batch_count, T alpha, const VA *As, const VB *Bs, T beta, VC *Cs) noexcept
{
  for ( usize b = 0; b < batch_count; ++b ) level3::gemm<OpA, OpB, T>(alpha, As[b], Bs[b], beta, Cs[b]);
}

template<blas_scalar T, typename VA>
  requires(mat_view_like<VA>)
[[gnu::flatten]] inline void
ger_batched(usize batch_count, T alpha, const vec_view<T> *xs, const vec_view<T> *ys, VA *As) noexcept
{
  for ( usize b = 0; b < batch_count; ++b ) level2::ger<T>(alpha, xs[b], ys[b], As[b]);
}

};      // namespace ext
};      // namespace blas
};      // namespace math
};      // namespace micron
