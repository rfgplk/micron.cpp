//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compressed sparse column matrix
//
// layout (LAPACK style):
//   outer  = length cols+1; outer[j] = start index in inner/values for col j; outer[cols] = nnz total
//   inner  = length nnz; inner[k] = row index of values[k]
//   values = length nnz; the coefficients

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../quants/vec.hpp"

namespace micron
{
namespace math
{
namespace sparse
{

template<arith_scalar T, micron::integral I = u32> struct csc {
  using value_type = T;
  using index_type = I;
  using vec_i = micron::vector<I, micron::allocator_serial<>, false>;
  using vec_v = micron::vector<T, micron::allocator_serial<>, false>;

  usize rows{ 0 };
  usize cols{ 0 };
  vec_i outer;       // size cols+1
  vec_i inner;       // size nnz
  vec_v values;      // size nnz

  csc() noexcept = default;

  csc(usize r, usize c) : rows(r), cols(c), outer(c + 1, I(0)), inner(0), values(0) { }

  csc(const csc &) = default;
  csc(csc &&) noexcept = default;
  csc &operator=(const csc &) = default;
  csc &operator=(csc &&) noexcept = default;

  [[nodiscard, gnu::always_inline]] usize
  nnz() const noexcept
  {
    return values.size();
  }

  [[nodiscard, gnu::always_inline]] I
  col_begin(usize j) const noexcept
  {
    return outer.data()[j];
  }

  [[nodiscard, gnu::always_inline]] I
  col_end(usize j) const noexcept
  {
    return outer.data()[j + 1];
  }

  [[nodiscard, gnu::always_inline]] usize
  col_nnz(usize j) const noexcept
  {
    return static_cast<usize>(outer.data()[j + 1] - outer.data()[j]);
  }

  // NOTE: triplets MUST be sorted by column (and within column by row for best perf)
  static csc
  from_triplets_sorted(usize r, usize c, const I *rows_arr, const I *cols_arr, const T *vals_arr, usize n)
  {
    csc out(r, c);
    micron::vector<usize, micron::allocator_serial<>, false> counts(c, usize(0));
    for ( usize k = 0; k < n; ++k ) counts.data()[cols_arr[k]] += 1;
    out.outer.data()[0] = I(0);
    for ( usize j = 0; j < c; ++j ) out.outer.data()[j + 1] = out.outer.data()[j] + static_cast<I>(counts.data()[j]);
    const usize total = static_cast<usize>(out.outer.data()[c]);
    out.inner = vec_i(total, I(0));
    out.values = vec_v(total, T(0));
    micron::vector<usize, micron::allocator_serial<>, false> pos(c, usize(0));
    for ( usize j = 0; j < c; ++j ) pos.data()[j] = static_cast<usize>(out.outer.data()[j]);
    for ( usize k = 0; k < n; ++k ) {
      const usize j = static_cast<usize>(cols_arr[k]);
      const usize p = pos.data()[j];
      out.inner.data()[p] = rows_arr[k];
      out.values.data()[p] = vals_arr[k];
      pos.data()[j] = p + 1;
    }
    return out;
  }
};

};      // namespace sparse
};      // namespace math
};      // namespace micron
