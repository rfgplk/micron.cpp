//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../quants/dynvec.hpp"
#include "../sqrt.hpp"
#include "csc.hpp"
#include "csr.hpp"
#include "simd_kernels.hpp"
#include "sparse_vec.hpp"

namespace micron
{
namespace math
{
namespace sparse
{

template<ieee754_floating T, micron::integral I>
inline void
scal(T alpha, csc<T, I> &A) noexcept
{
  scal_values<T>(alpha, A.values.data(), A.nnz());
}

template<ieee754_floating T, micron::integral I>
inline void
scal(T alpha, csr<T, I> &A) noexcept
{
  scal_values<T>(alpha, A.values.data(), A.nnz());
}

template<ieee754_floating T, micron::integral I>
inline void
scal(T alpha, sparse_vec<T, I> &v) noexcept
{
  scal_values<T>(alpha, v.values.data(), v.nnz());
}

template<ieee754_floating T, micron::integral I>
[[nodiscard]] inline T
norm_frob(const csc<T, I> &A) noexcept
{
  T s = norm_sq_values<T>(A.values.data(), A.nnz());
  return math::fsqrt(s);
}

template<ieee754_floating T, micron::integral I>
[[nodiscard]] inline T
norm_frob(const csr<T, I> &A) noexcept
{
  T s = norm_sq_values<T>(A.values.data(), A.nnz());
  return math::fsqrt(s);
}

template<ieee754_floating T, micron::integral I>
[[nodiscard]] inline T
norm_l2(const sparse_vec<T, I> &v) noexcept
{
  T s = norm_sq_values<T>(v.values.data(), v.nnz());
  return math::fsqrt(s);
}

template<ieee754_floating T, micron::integral I>
[[nodiscard]] inline T
dot(const sparse_vec<T, I> &a, const sparse_vec<T, I> &b) noexcept
{
  T s = T(0);
  usize ia = 0, ib = 0;
  const usize na = a.nnz();
  const usize nb = b.nnz();
  while ( ia < na && ib < nb ) {
    const I ka = a.idx.data()[ia];
    const I kb = b.idx.data()[ib];
    if ( ka == kb ) {
      s = math::fma<T>(a.values.data()[ia], b.values.data()[ib], s);
      ++ia;
      ++ib;
    } else if ( ka < kb ) {
      ++ia;
    } else {
      ++ib;
    }
  }
  return s;
}

template<ieee754_floating T, micron::integral I>
[[nodiscard]] inline T
dot(const sparse_vec<T, I> &a, const dynvec<T> &b) noexcept
{
  T s = T(0);
  const usize na = a.nnz();
  for ( usize k = 0; k < na; ++k ) s = math::fma<T>(a.values.data()[k], b[a.idx.data()[k]], s);
  return s;
}

};      // namespace sparse
};      // namespace math
};      // namespace micron
