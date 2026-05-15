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
#include "csc.hpp"

namespace micron
{
namespace math
{
namespace sparse
{

namespace triangular_tag
{
struct lower_unit {
};

struct lower_non_unit {
};

struct upper_unit {
};

struct upper_non_unit {
};
};      // namespace triangular_tag

template<arith_scalar T, micron::integral I, typename Diag>
inline bool
solve_lower(const csc<T, I> &L, dynvec<T> &b, Diag) noexcept
{
  const usize n = L.cols;
  for ( usize j = 0; j < n; ++j ) {
    const usize cs = static_cast<usize>(L.outer.data()[j]);
    const usize ce = static_cast<usize>(L.outer.data()[j + 1]);
    if constexpr ( micron::is_same_v<Diag, triangular_tag::lower_non_unit> ) {
      // find diagonal entry (row == j)
      T diag = T(0);
      usize diag_pos = ce;
      for ( usize k = cs; k < ce; ++k ) {
        if ( static_cast<usize>(L.inner.data()[k]) == j ) {
          diag = L.values.data()[k];
          diag_pos = k;
          break;
        }
      }
      if ( diag == T(0) ) return false;
      b[j] /= diag;
      const T xj = b[j];
      for ( usize k = cs; k < ce; ++k ) {
        if ( k == diag_pos ) continue;
        const usize i = static_cast<usize>(L.inner.data()[k]);
        if ( i > j ) b[i] = math::fma<T>(-L.values.data()[k], xj, b[i]);
      }
    } else {
      // unit diagonal: assume diag = 1; just propagate
      const T xj = b[j];
      for ( usize k = cs; k < ce; ++k ) {
        const usize i = static_cast<usize>(L.inner.data()[k]);
        if ( i > j ) b[i] = math::fma<T>(-L.values.data()[k], xj, b[i]);
      }
    }
  }
  return true;
}

template<arith_scalar T, micron::integral I, typename Diag>
inline bool
solve_upper(const csc<T, I> &U, dynvec<T> &b, Diag) noexcept
{
  const usize n = U.cols;
  for ( usize jj = n; jj-- > 0; ) {
    const usize j = jj;
    const usize cs = static_cast<usize>(U.outer.data()[j]);
    const usize ce = static_cast<usize>(U.outer.data()[j + 1]);
    if constexpr ( micron::is_same_v<Diag, triangular_tag::upper_non_unit> ) {
      T diag = T(0);
      usize diag_pos = ce;
      for ( usize k = cs; k < ce; ++k ) {
        if ( static_cast<usize>(U.inner.data()[k]) == j ) {
          diag = U.values.data()[k];
          diag_pos = k;
          break;
        }
      }
      if ( diag == T(0) ) return false;
      b[j] /= diag;
      const T xj = b[j];
      for ( usize k = cs; k < ce; ++k ) {
        if ( k == diag_pos ) continue;
        const usize i = static_cast<usize>(U.inner.data()[k]);
        if ( i < j ) b[i] = math::fma<T>(-U.values.data()[k], xj, b[i]);
      }
    } else {
      const T xj = b[j];
      for ( usize k = cs; k < ce; ++k ) {
        const usize i = static_cast<usize>(U.inner.data()[k]);
        if ( i < j ) b[i] = math::fma<T>(-U.values.data()[k], xj, b[i]);
      }
    }
  }
  return true;
}

};      // namespace sparse
};      // namespace math
};      // namespace micron
