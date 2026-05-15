//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// SPD-only fast paths

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../constants.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "decomp.hpp"
#include "ops.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace spd
{

template<ieee754_floating F, usize N>
inline void
chol_solve(const mat<F, N, N> &L, vec<F, N> &x_inout) noexcept
{
  for ( usize i = 0; i < N; ++i ) {
    F s = x_inout.data[i];
    for ( usize j = 0; j < i; ++j ) s = math::fma<F>(-L.data[i * N + j], x_inout.data[j], s);
    x_inout.data[i] = s / L.data[i * N + i];
  }
  for ( usize ii = N; ii-- > 0; ) {
    F s = x_inout.data[ii];
    for ( usize j = ii + 1; j < N; ++j ) s = math::fma<F>(-L.data[j * N + ii], x_inout.data[j], s);
    x_inout.data[ii] = s / L.data[ii * N + ii];
  }
}

template<ieee754_floating F>
inline void
chol_solve(const dynmat<F> &L, dynvec<F> &x_inout) noexcept
{
  const usize N = L.rows;
  const usize ld = L.ld;
  const F *__restrict__ Ld = L.data();
  for ( usize i = 0; i < N; ++i ) {
    F s = x_inout[i];
    for ( usize j = 0; j < i; ++j ) s = math::fma<F>(-Ld[i * ld + j], x_inout[j], s);
    x_inout[i] = s / Ld[i * ld + i];
  }
  for ( usize ii = N; ii-- > 0; ) {
    F s = x_inout[ii];
    for ( usize j = ii + 1; j < N; ++j ) s = math::fma<F>(-Ld[j * ld + ii], x_inout[j], s);
    x_inout[ii] = s / Ld[ii * ld + ii];
  }
}

template<ieee754_floating F, usize N> struct inv_sympd_result {
  mat<F, N, N> X;
  bool spd;
};

template<ieee754_floating F, usize N>
[[nodiscard]] inline inv_sympd_result<F, N>
inv_sympd(const mat<F, N, N> &A) noexcept
{
  inv_sympd_result<F, N> r{ mat<F, N, N>::zero(), true };
  auto c = decomp::chol(A);
  if ( !c.spd ) {
    r.spd = false;
    return r;
  }
  for ( usize j = 0; j < N; ++j ) {
    vec<F, N> e{};
    for ( usize i = 0; i < N; ++i ) e.data[i] = (i == j) ? F(1) : F(0);
    chol_solve<F, N>(c.L, e);
    for ( usize i = 0; i < N; ++i ) r.X.data[i * N + j] = e.data[i];
  }
  return r;
}

template<ieee754_floating F> struct inv_sympd_result_dyn {
  dynmat<F> X;
  bool spd;
};

template<ieee754_floating F> struct chol_result_dyn {
  dynmat<F> L;
  bool spd;
};

template<ieee754_floating F>
[[nodiscard]] inline chol_result_dyn<F>
chol(const dynmat<F> &A) noexcept
{
  const usize N = A.rows;
  chol_result_dyn<F> r{ dynmat<F>::zero(N, N), true };
  F *__restrict__ L = r.L.data();
  const usize ld = r.L.ld;
  const F *__restrict__ Ad = A.data();
  const usize ldA = A.ld;
  for ( usize i = 0; i < N; ++i ) {
    for ( usize j = 0; j <= i; ++j ) {
      F s = Ad[i * ldA + j];
      for ( usize k = 0; k < j; ++k ) s = math::fma<F>(-L[i * ld + k], L[j * ld + k], s);
      if ( i == j ) {
        if ( s <= F(0) ) {
          r.spd = false;
          return r;
        }
        L[i * ld + j] = math::fsqrt(s);
      } else {
        L[i * ld + j] = s / L[j * ld + j];
      }
    }
  }
  return r;
}

template<ieee754_floating F>
[[nodiscard]] inline inv_sympd_result_dyn<F>
inv_sympd(const dynmat<F> &A) noexcept
{
  const usize N = A.rows;
  inv_sympd_result_dyn<F> r{ dynmat<F>::zero(N, N), true };
  auto c = chol<F>(A);
  if ( !c.spd ) {
    r.spd = false;
    return r;
  }
  for ( usize j = 0; j < N; ++j ) {
    dynvec<F> e(N, F(0));
    e[j] = F(1);
    chol_solve<F>(c.L, e);
    for ( usize i = 0; i < N; ++i ) r.X.at(i, j) = e[i];
  }
  return r;
}

template<ieee754_floating F, usize N>
[[nodiscard]] inline decomp::log_det_result<F>
log_det_sympd(const mat<F, N, N> &A) noexcept
{
  auto c = decomp::chol(A);
  if ( !c.spd ) return { F(0), 0 };
  F s = F(0);
  for ( usize i = 0; i < N; ++i ) s += math::log(c.L.data[i * N + i]);
  return { F(2) * s, 1 };
}

template<ieee754_floating F>
[[nodiscard]] inline decomp::log_det_result<F>
log_det_sympd(const dynmat<F> &A) noexcept
{
  auto c = chol<F>(A);
  if ( !c.spd ) return { F(0), 0 };
  const usize N = A.rows;
  const usize ld = c.L.ld;
  F s = F(0);
  for ( usize i = 0; i < N; ++i ) s += math::log(c.L.data()[i * ld + i]);
  return { F(2) * s, 1 };
}

template<ieee754_floating F> struct chol_pivot_result_dyn {
  dynmat<F> L;
  micron::vector<usize, micron::allocator_serial<>, false> perm;
  usize rank;
  F max_pivot;
};

template<ieee754_floating F>
[[nodiscard]] inline chol_pivot_result_dyn<F>
chol_pivot(const dynmat<F> &A, F tol = F(0)) noexcept
{
  const usize N = A.rows;
  chol_pivot_result_dyn<F> r{ dynmat<F>::zero(N, N), micron::vector<usize, micron::allocator_serial<>, false>(N), 0, F(0) };
  for ( usize i = 0; i < N; ++i ) r.perm.data()[i] = i;

  micron::vector<F, micron::allocator_serial<>, false> diag(N);
  for ( usize i = 0; i < N; ++i ) diag.data()[i] = A.at(i, i);

  dynmat<F> M(N, N);
  for ( usize i = 0; i < N; ++i )
    for ( usize j = 0; j < N; ++j ) M.at(i, j) = A.at(i, j);

  F max_diag = F(0);
  for ( usize i = 0; i < N; ++i )
    if ( diag.data()[i] > max_diag ) max_diag = diag.data()[i];
  const F effective_tol = (tol > F(0)) ? tol : max_diag * default_eps<F>() * F(N);

  for ( usize k = 0; k < N; ++k ) {
    F best = diag.data()[r.perm.data()[k]];
    usize piv = k;
    for ( usize i = k + 1; i < N; ++i ) {
      F v = diag.data()[r.perm.data()[i]];
      if ( v > best ) {
        best = v;
        piv = i;
      }
    }
    if ( k == 0 ) r.max_pivot = best;
    if ( best <= effective_tol ) break;

    if ( piv != k ) {
      usize t = r.perm.data()[k];
      r.perm.data()[k] = r.perm.data()[piv];
      r.perm.data()[piv] = t;
    }
    const usize pk = r.perm.data()[k];

    F lkk = math::fsqrt(diag.data()[pk]);
    F *__restrict__ L_data = r.L.data();
    const usize ld_L = r.L.ld;
    L_data[pk * ld_L + k] = lkk;
    const F inv_lkk = F(1) / lkk;
    const F *__restrict__ L_pk_row = L_data + pk * ld_L;
    F *__restrict__ diag_p = diag.data();
    const usize *__restrict__ perm_p = r.perm.data();
    for ( usize ii = k + 1; ii < N; ++ii ) {
      const usize pi = perm_p[ii];
      F *__restrict__ L_pi_row = L_data + pi * ld_L;
      F sub = F(0);
      for ( usize j = 0; j < k; ++j ) sub = math::fma<F>(L_pi_row[j], L_pk_row[j], sub);
      const F lik = (M.at(pi, pk) - sub) * inv_lkk;
      L_pi_row[k] = lik;
      diag_p[pi] -= lik * lik;
    }
    r.rank = k + 1;
  }
  return r;
}

};      // namespace spd
};      // namespace linalg
};      // namespace math
};      // namespace micron
