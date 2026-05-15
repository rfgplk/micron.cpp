//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// sparse products:
//   spmv   = sparse-mat * dense-vec
//   spmm   = sparse-mat * dense-mat
//   spgemm = sparse-mat * sparse-mat (Gustavson algorithm)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../ieee.hpp"
#include "../matrix/dynmat.hpp"
#include "../quants/dynvec.hpp"
#include "csc.hpp"
#include "csr.hpp"
#include "simd_kernels.hpp"

namespace micron
{
namespace math
{
namespace sparse
{

// y := alpha * A * x + beta * y
// csc native: walk columns, scatter contributions into y
template<arith_scalar T, micron::integral I>
inline void
spmv(T alpha, const csc<T, I> &A, const dynvec<T> &x, T beta, dynvec<T> &y) noexcept
{
  // y := beta * y
  if ( beta == T(0) ) {
    for ( usize i = 0; i < y.size(); ++i ) y[i] = T(0);
  } else if ( beta != T(1) ) {
    scal_values<T>(beta, y.data(), y.size());
  }
  if ( alpha == T(0) ) return;

  const I *__restrict__ outer = A.outer.data();
  const I *__restrict__ inner = A.inner.data();
  const T *__restrict__ vals = A.values.data();
  T *__restrict__ yp = y.data();

  for ( usize j = 0; j < A.cols; ++j ) {
    const T xj = x[j];
    if ( xj == T(0) ) continue;
    const usize a = static_cast<usize>(outer[j]);
    const usize b = static_cast<usize>(outer[j + 1]);
    spmv_col_scatter<T, I>(alpha, xj, vals + a, inner + a, yp, b - a);
  }
}

// CSC transpose: walk columns as dot products into y[j]
template<arith_scalar T, micron::integral I>
inline void
spmv_transposed(T alpha, const csc<T, I> &A, const dynvec<T> &x, T beta, dynvec<T> &y) noexcept
{
  if ( beta == T(0) ) {
    for ( usize i = 0; i < y.size(); ++i ) y[i] = T(0);
  } else if ( beta != T(1) ) {
    scal_values<T>(beta, y.data(), y.size());
  }
  if ( alpha == T(0) ) return;

  const I *__restrict__ outer = A.outer.data();
  const I *__restrict__ inner = A.inner.data();
  const T *__restrict__ vals = A.values.data();
  const T *__restrict__ xp = x.data();

  for ( usize j = 0; j < A.cols; ++j ) {
    const usize a = static_cast<usize>(outer[j]);
    const usize b = static_cast<usize>(outer[j + 1]);
    T s = T(0);
    for ( usize k = a; k < b; ++k ) s = math::fma<T>(vals[k], xp[inner[k]], s);
    y[j] = math::fma<T>(alpha, s, y[j]);
  }
}

// row-wise inner products
template<arith_scalar T, micron::integral I>
inline void
spmv(T alpha, const csr<T, I> &A, const dynvec<T> &x, T beta, dynvec<T> &y) noexcept
{
  if ( beta == T(0) ) {
    for ( usize i = 0; i < y.size(); ++i ) y[i] = T(0);
  } else if ( beta != T(1) ) {
    scal_values<T>(beta, y.data(), y.size());
  }
  if ( alpha == T(0) ) return;

  const I *__restrict__ outer = A.outer.data();
  const I *__restrict__ inner = A.inner.data();
  const T *__restrict__ vals = A.values.data();
  const T *__restrict__ xp = x.data();

  for ( usize i = 0; i < A.rows; ++i ) {
    const usize a = static_cast<usize>(outer[i]);
    const usize b = static_cast<usize>(outer[i + 1]);
    T s = T(0);
    for ( usize k = a; k < b; ++k ) s = math::fma<T>(vals[k], xp[inner[k]], s);
    y[i] = math::fma<T>(alpha, s, y[i]);
  }
}

// y := alpha * A * X + beta * Y  (A sparse, X/Y dense)
template<arith_scalar T, micron::integral I>
inline void
spmm(T alpha, const csc<T, I> &A, const dynmat<T> &X, T beta, dynmat<T> &Y) noexcept
{
  // Y is rows×nrhs
  const usize nrhs = X.cols;
  dynvec<T> xj(X.rows, T(0));
  dynvec<T> yj(Y.rows, T(0));
  for ( usize j = 0; j < nrhs; ++j ) {
    for ( usize i = 0; i < X.rows; ++i ) xj[i] = X.at(i, j);
    for ( usize i = 0; i < Y.rows; ++i ) yj[i] = Y.at(i, j);
    spmv<T, I>(alpha, A, xj, beta, yj);
    for ( usize i = 0; i < Y.rows; ++i ) Y.at(i, j) = yj[i];
  }
}

// C := A * B  (both CSC)
// Gustavson with dense accumulator
template<arith_scalar T, micron::integral I>
[[nodiscard]] inline csc<T, I>
spgemm(const csc<T, I> &A, const csc<T, I> &B) noexcept
{
  csc<T, I> C(A.rows, B.cols);
  micron::vector<T, micron::allocator_serial<>, false> acc(A.rows, T(0));
  micron::vector<u8, micron::allocator_serial<>, false> marker(A.rows, u8(0));
  micron::vector<I, micron::allocator_serial<>, false> active(A.rows, I(0));

  // compute outer
  C.outer = typename csc<T, I>::vec_i(B.cols + 1, I(0));

  for ( usize j = 0; j < B.cols; ++j ) {
    usize n_active = 0;
    const usize bs = static_cast<usize>(B.outer.data()[j]);
    const usize be = static_cast<usize>(B.outer.data()[j + 1]);
    for ( usize bk = bs; bk < be; ++bk ) {
      const usize k = static_cast<usize>(B.inner.data()[bk]);
      const T b_kj = B.values.data()[bk];
      if ( b_kj == T(0) ) continue;
      const usize ak_start = static_cast<usize>(A.outer.data()[k]);
      const usize ak_end = static_cast<usize>(A.outer.data()[k + 1]);
      for ( usize ai = ak_start; ai < ak_end; ++ai ) {
        const usize i = static_cast<usize>(A.inner.data()[ai]);
        const T a_ik = A.values.data()[ai];
        if ( marker.data()[i] == 0 ) {
          marker.data()[i] = 1;
          active.data()[n_active++] = static_cast<I>(i);
          acc.data()[i] = a_ik * b_kj;
        } else {
          acc.data()[i] = math::fma<T>(a_ik, b_kj, acc.data()[i]);
        }
      }
    }

    // sort active indices ascending (small lists typical)
    for ( usize ii = 1; ii < n_active; ++ii ) {
      I cur = active.data()[ii];
      usize jj = ii;
      while ( jj > 0 && active.data()[jj - 1] > cur ) {
        active.data()[jj] = active.data()[jj - 1];
        --jj;
      }
      active.data()[jj] = cur;
    }

    // emit column j
    const I old_size = static_cast<I>(C.inner.size());
    for ( usize ii = 0; ii < n_active; ++ii ) {
      const I idx = active.data()[ii];
      C.inner.push_back(idx);
      C.values.push_back(acc.data()[static_cast<usize>(idx)]);
      marker.data()[static_cast<usize>(idx)] = 0;
      acc.data()[static_cast<usize>(idx)] = T(0);
    }
    C.outer.data()[j + 1] = old_size + static_cast<I>(n_active);
  }
  return C;
}

};      // namespace sparse
};      // namespace math
};      // namespace micron
