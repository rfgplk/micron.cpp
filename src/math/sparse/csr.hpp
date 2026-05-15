//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compressed sparse row matrix
//
//   outer  = length rows+1; outer[i] = start in inner/values for row i
//   inner  = length nnz; inner[k] = column index of values[k]
//   values = length nnz

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../quants/vec.hpp"
#include "csc.hpp"

namespace micron
{
namespace math
{
namespace sparse
{

template<arith_scalar T, micron::integral I = u32> struct csr {
  using value_type = T;
  using index_type = I;
  using vec_i = micron::vector<I, micron::allocator_serial<>, false>;
  using vec_v = micron::vector<T, micron::allocator_serial<>, false>;

  usize rows{ 0 };
  usize cols{ 0 };
  vec_i outer;       // size rows+1
  vec_i inner;       // size nnz
  vec_v values;      // size nnz

  csr() noexcept = default;

  csr(usize r, usize c) : rows(r), cols(c), outer(r + 1, I(0)), inner(0), values(0) { }

  csr(const csr &) = default;
  csr(csr &&) noexcept = default;
  csr &operator=(const csr &) = default;
  csr &operator=(csr &&) noexcept = default;

  [[nodiscard, gnu::always_inline]] usize
  nnz() const noexcept
  {
    return values.size();
  }

  [[nodiscard, gnu::always_inline]] I
  row_begin(usize i) const noexcept
  {
    return outer.data()[i];
  }

  [[nodiscard, gnu::always_inline]] I
  row_end(usize i) const noexcept
  {
    return outer.data()[i + 1];
  }

  [[nodiscard, gnu::always_inline]] usize
  row_nnz(usize i) const noexcept
  {
    return static_cast<usize>(outer.data()[i + 1] - outer.data()[i]);
  }
};

template<arith_scalar T, micron::integral I>
[[nodiscard]] inline csc<T, I>
to_csc(const csr<T, I> &A) noexcept
{
  csc<T, I> out(A.rows, A.cols);
  micron::vector<usize, micron::allocator_serial<>, false> counts(A.cols, usize(0));
  for ( usize k = 0; k < A.nnz(); ++k ) counts.data()[A.inner.data()[k]] += 1;
  out.outer.data()[0] = I(0);
  for ( usize j = 0; j < A.cols; ++j ) out.outer.data()[j + 1] = out.outer.data()[j] + static_cast<I>(counts.data()[j]);
  const usize total = A.nnz();
  out.inner = typename csc<T, I>::vec_i(total, I(0));
  out.values = typename csc<T, I>::vec_v(total, T(0));
  micron::vector<usize, micron::allocator_serial<>, false> pos(A.cols, usize(0));
  for ( usize j = 0; j < A.cols; ++j ) pos.data()[j] = static_cast<usize>(out.outer.data()[j]);
  for ( usize i = 0; i < A.rows; ++i ) {
    const usize a = static_cast<usize>(A.outer.data()[i]);
    const usize b = static_cast<usize>(A.outer.data()[i + 1]);
    for ( usize k = a; k < b; ++k ) {
      const usize j = static_cast<usize>(A.inner.data()[k]);
      const usize p = pos.data()[j];
      out.inner.data()[p] = static_cast<I>(i);
      out.values.data()[p] = A.values.data()[k];
      pos.data()[j] = p + 1;
    }
  }
  return out;
}

template<arith_scalar T, micron::integral I>
[[nodiscard]] inline csr<T, I>
to_csr(const csc<T, I> &A) noexcept
{
  csr<T, I> out(A.rows, A.cols);
  micron::vector<usize, micron::allocator_serial<>, false> counts(A.rows, usize(0));
  for ( usize k = 0; k < A.nnz(); ++k ) counts.data()[A.inner.data()[k]] += 1;
  out.outer.data()[0] = I(0);
  for ( usize i = 0; i < A.rows; ++i ) out.outer.data()[i + 1] = out.outer.data()[i] + static_cast<I>(counts.data()[i]);
  const usize total = A.nnz();
  out.inner = typename csr<T, I>::vec_i(total, I(0));
  out.values = typename csr<T, I>::vec_v(total, T(0));
  micron::vector<usize, micron::allocator_serial<>, false> pos(A.rows, usize(0));
  for ( usize i = 0; i < A.rows; ++i ) pos.data()[i] = static_cast<usize>(out.outer.data()[i]);
  for ( usize j = 0; j < A.cols; ++j ) {
    const usize a = static_cast<usize>(A.outer.data()[j]);
    const usize b = static_cast<usize>(A.outer.data()[j + 1]);
    for ( usize k = a; k < b; ++k ) {
      const usize i = static_cast<usize>(A.inner.data()[k]);
      const usize p = pos.data()[i];
      out.inner.data()[p] = static_cast<I>(j);
      out.values.data()[p] = A.values.data()[k];
      pos.data()[i] = p + 1;
    }
  }
  return out;
}

};      // namespace sparse
};      // namespace math
};      // namespace micron
