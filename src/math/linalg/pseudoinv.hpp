//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Moore-Penrose pseudoinverse and the sing val decomp derived utilities
//  pinv(A, tol): A+; Moore-Penrose pseudoinverse via thin SVD
//  rank(A, tol): numerical rank
//  null(A, tol): orthonormal basis of the null space
//  orth(A, tol): orthonormal basis of the column space

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../constants.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "decomp.hpp"
#include "decomp_ext.hpp"
#include "ops.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace pseudoinv
{

template <ieee754_floating F> struct svd_result_dyn {
  dynmat<F> U;     // rows × rows (full)
  dynvec<F> S;     // length min(rows, cols)
  dynmat<F> V;     // cols × cols
  bool converged;
};

template <ieee754_floating F>
[[nodiscard]] inline svd_result_dyn<F>
svd(const dynmat<F> &A) noexcept
{
  const usize R = A.rows;
  const usize C = A.cols;
  svd_result_dyn<F> r{ dynmat<F>::zero(R, R), dynvec<F>(C), dynmat<F>::identity(C), false };

  dynmat<F> W = A;
  constexpr int max_sweeps = 30;
  const F eps = default_eps<F>();
  F *__restrict__ Wp = W.data();
  F *__restrict__ Vp = r.V.data();
  const usize ld_W = W.ld;
  const usize ld_V = r.V.ld;

  for ( int sweep = 0; sweep < max_sweeps; ++sweep ) {
    F off = F(0);
    for ( usize p = 0; p + 1 < C; ++p ) {
      for ( usize q = p + 1; q < C; ++q ) {
        F *__restrict__ col_p = Wp + p;
        F *__restrict__ col_q = Wp + q;
        F a = F(0), b = F(0), c = F(0);
        for ( usize i = 0; i < R; ++i ) {
          const F wp = col_p[i * ld_W];
          const F wq = col_q[i * ld_W];
          a = math::fma<F>(wp, wp, a);
          b = math::fma<F>(wq, wq, b);
          c = math::fma<F>(wp, wq, c);
        }
        off += math::fabs(c);
        if ( math::fabs(c) < eps * math::fsqrt(a * b + F(1e-30)) ) continue;
        const F theta = (b - a) / (F(2) * c);
        const F t
            = (theta >= F(0)) ? F(1) / (theta + math::fsqrt(F(1) + theta * theta)) : F(1) / (theta - math::fsqrt(F(1) + theta * theta));
        const F cs = F(1) / math::fsqrt(F(1) + t * t);
        const F sn = t * cs;
        for ( usize i = 0; i < R; ++i ) {
          F *__restrict__ wp_p = col_p + i * ld_W;
          F *__restrict__ wp_q = col_q + i * ld_W;
          const F wp = *wp_p;
          const F wq = *wp_q;
          *wp_p = cs * wp - sn * wq;
          *wp_q = sn * wp + cs * wq;
        }
        F *__restrict__ vcol_p = Vp + p;
        F *__restrict__ vcol_q = Vp + q;
        for ( usize i = 0; i < C; ++i ) {
          F *__restrict__ vp_p = vcol_p + i * ld_V;
          F *__restrict__ vp_q = vcol_q + i * ld_V;
          const F vp = *vp_p;
          const F vq = *vp_q;
          *vp_p = cs * vp - sn * vq;
          *vp_q = sn * vp + cs * vq;
        }
      }
    }
    if ( off < eps ) {
      r.converged = true;
      break;
    }
  }

  F *__restrict__ Up = r.U.data();
  const usize ld_U = r.U.ld;
  for ( usize j = 0; j < C; ++j ) {
    const F *__restrict__ wcol = Wp + j;
    F s = F(0);
    for ( usize i = 0; i < R; ++i ) {
      const F wij = wcol[i * ld_W];
      s = math::fma<F>(wij, wij, s);
    }
    s = math::fsqrt(s);
    r.S[j] = s;
    F *__restrict__ ucol = Up + j;
    if ( s > F(0) ) {
      const F inv = F(1) / s;
      for ( usize i = 0; i < R; ++i ) ucol[i * ld_U] = wcol[i * ld_W] * inv;
    } else {
      for ( usize i = 0; i < R; ++i ) ucol[i * ld_U] = F(0);
    }
  }

  for ( usize i = 0; i + 1 < C; ++i ) {
    usize maxj = i;
    for ( usize j = i + 1; j < C; ++j )
      if ( r.S[j] > r.S[maxj] ) maxj = j;
    if ( maxj != i ) {
      F tmp = r.S[i];
      r.S[i] = r.S[maxj];
      r.S[maxj] = tmp;
      F *__restrict__ ucol_i = Up + i;
      F *__restrict__ ucol_j = Up + maxj;
      for ( usize rr = 0; rr < R; ++rr ) {
        F *p_i = ucol_i + rr * ld_U;
        F *p_j = ucol_j + rr * ld_U;
        F t2 = *p_i;
        *p_i = *p_j;
        *p_j = t2;
      }
      F *__restrict__ vcol_i = Vp + i;
      F *__restrict__ vcol_j = Vp + maxj;
      for ( usize rr = 0; rr < C; ++rr ) {
        F *p_i = vcol_i + rr * ld_V;
        F *p_j = vcol_j + rr * ld_V;
        F t2 = *p_i;
        *p_i = *p_j;
        *p_j = t2;
      }
    }
  }
  return r;
}

template <ieee754_floating F, usize R_, usize C_>
  requires(R_ >= C_)
[[nodiscard]] inline mat<F, C_, R_>
pinv(const mat<F, R_, C_> &A, F tol = F(0)) noexcept
{
  auto s = decomp::svd<F, R_, C_>(A);
  // determine effective tolerance
  F smax = F(0);
  constexpr usize K = (R_ < C_) ? R_ : C_;
  for ( usize i = 0; i < K; ++i )
    if ( s.S.data[i] > smax ) smax = s.S.data[i];
  F use_tol = (tol > F(0)) ? tol : F((R_ > C_) ? R_ : C_) * smax * default_eps<F>();

  // inverse singular values
  F sinv[K];
  for ( usize i = 0; i < K; ++i ) sinv[i] = (s.S.data[i] > use_tol) ? F(1) / s.S.data[i] : F(0);

  mat<F, C_, R_> P{};
  for ( usize i = 0; i < C_; ++i ) {
    for ( usize j = 0; j < R_; ++j ) {
      F acc = F(0);
      for ( usize k = 0; k < K; ++k ) acc = math::fma<F>(s.V.data[i * C_ + k] * sinv[k], s.U.data[j * R_ + k], acc);
      P.data[i * R_ + j] = acc;
    }
  }
  return P;
}

template <ieee754_floating F>
[[nodiscard]] inline dynmat<F>
pinv(const dynmat<F> &A, F tol = F(0)) noexcept
{
  if ( A.rows < A.cols ) {
    dynmat<F> At(A.cols, A.rows);
    for ( usize i = 0; i < A.rows; ++i )
      for ( usize j = 0; j < A.cols; ++j ) At.at(j, i) = A.at(i, j);
    auto Pt = pinv<F>(At, tol);
    dynmat<F> P(A.cols, A.rows);
    for ( usize i = 0; i < Pt.rows; ++i )
      for ( usize j = 0; j < Pt.cols; ++j ) P.at(j, i) = Pt.at(i, j);
    return P;
  }
  auto s = svd<F>(A);
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  F smax = F(0);
  for ( usize i = 0; i < K; ++i )
    if ( s.S[i] > smax ) smax = s.S[i];
  F use_tol = (tol > F(0)) ? tol : F((R > C) ? R : C) * smax * default_eps<F>();

  micron::vector<F, micron::allocator_serial<>, false> sinv(K);
  F *__restrict__ sinv_p = sinv.data();
  for ( usize i = 0; i < K; ++i ) sinv_p[i] = (s.S[i] > use_tol) ? F(1) / s.S[i] : F(0);

  dynmat<F> P(C, R, F(0));
  F *__restrict__ Pp = P.data();
  const usize ld_P = P.ld;
  const F *__restrict__ Vd = s.V.data();
  const F *__restrict__ Ud = s.U.data();
  const usize ld_V = s.V.ld;
  const usize ld_U = s.U.ld;
  for ( usize i = 0; i < C; ++i ) {
    const F *__restrict__ vrow = Vd + i * ld_V;
    F *__restrict__ prow = Pp + i * ld_P;
    for ( usize j = 0; j < R; ++j ) {
      const F *__restrict__ urow = Ud + j * ld_U;
      F acc = F(0);
      for ( usize k = 0; k < K; ++k ) acc = math::fma<F>(vrow[k] * sinv_p[k], urow[k], acc);
      prow[j] = acc;
    }
  }
  return P;
}

template <ieee754_floating F, usize R_, usize C_>
  requires(R_ >= C_)
[[nodiscard]] inline usize
rank(const mat<F, R_, C_> &A, F tol = F(0)) noexcept
{
  auto s = decomp::svd<F, R_, C_>(A);
  constexpr usize K = (R_ < C_) ? R_ : C_;
  F smax = F(0);
  for ( usize i = 0; i < K; ++i )
    if ( s.S.data[i] > smax ) smax = s.S.data[i];
  F use_tol = (tol > F(0)) ? tol : F((R_ > C_) ? R_ : C_) * smax * default_eps<F>();
  usize rk = 0;
  for ( usize i = 0; i < K; ++i )
    if ( s.S.data[i] > use_tol ) ++rk;
  return rk;
}

template <ieee754_floating F>
[[nodiscard]] inline usize
rank(const dynmat<F> &A, F tol = F(0)) noexcept
{
  if ( A.rows < A.cols ) {
    dynmat<F> At(A.cols, A.rows);
    for ( usize i = 0; i < A.rows; ++i )
      for ( usize j = 0; j < A.cols; ++j ) At.at(j, i) = A.at(i, j);
    return rank<F>(At, tol);
  }
  auto s = svd<F>(A);
  const usize K = (A.rows < A.cols) ? A.rows : A.cols;
  F smax = F(0);
  for ( usize i = 0; i < K; ++i )
    if ( s.S[i] > smax ) smax = s.S[i];
  F use_tol = (tol > F(0)) ? tol : F((A.rows > A.cols) ? A.rows : A.cols) * smax * default_eps<F>();
  usize rk = 0;
  for ( usize i = 0; i < K; ++i )
    if ( s.S[i] > use_tol ) ++rk;
  return rk;
}

template <ieee754_floating F>
[[nodiscard]] inline dynmat<F>
orth(const dynmat<F> &A, F tol = F(0)) noexcept
{
  if ( A.rows < A.cols ) {
    dynmat<F> At(A.cols, A.rows);
    for ( usize i = 0; i < A.rows; ++i )
      for ( usize j = 0; j < A.cols; ++j ) At.at(j, i) = A.at(i, j);
    auto s = svd<F>(At);
    F smax = F(0);
    for ( usize i = 0; i < s.S.size(); ++i )
      if ( s.S[i] > smax ) smax = s.S[i];
    F use_tol = (tol > F(0)) ? tol : F((A.rows > A.cols) ? A.rows : A.cols) * smax * default_eps<F>();
    usize rk = 0;
    for ( usize i = 0; i < s.S.size(); ++i )
      if ( s.S[i] > use_tol ) ++rk;
    dynmat<F> O(A.rows, rk);
    for ( usize i = 0; i < A.rows; ++i )
      for ( usize j = 0; j < rk; ++j ) O.at(i, j) = s.V.at(i, j);
    return O;
  }
  auto s = svd<F>(A);
  const usize K = (A.rows < A.cols) ? A.rows : A.cols;
  F smax = F(0);
  for ( usize i = 0; i < K; ++i )
    if ( s.S[i] > smax ) smax = s.S[i];
  F use_tol = (tol > F(0)) ? tol : F((A.rows > A.cols) ? A.rows : A.cols) * smax * default_eps<F>();
  usize rk = 0;
  for ( usize i = 0; i < K; ++i )
    if ( s.S[i] > use_tol ) ++rk;
  dynmat<F> O(A.rows, rk);
  for ( usize i = 0; i < A.rows; ++i )
    for ( usize j = 0; j < rk; ++j ) O.at(i, j) = s.U.at(i, j);
  return O;
}

template <ieee754_floating F>
[[nodiscard]] inline dynmat<F>
null(const dynmat<F> &A, F tol = F(0)) noexcept
{
  if ( A.rows < A.cols ) {
    dynmat<F> At(A.cols, A.rows);
    for ( usize i = 0; i < A.rows; ++i )
      for ( usize j = 0; j < A.cols; ++j ) At.at(j, i) = A.at(i, j);
    auto s = svd<F>(At);
    F smax = F(0);
    for ( usize i = 0; i < s.S.size(); ++i )
      if ( s.S[i] > smax ) smax = s.S[i];
    F use_tol = (tol > F(0)) ? tol : F((A.rows > A.cols) ? A.rows : A.cols) * smax * default_eps<F>();
    usize rk = 0;
    for ( usize i = 0; i < s.S.size(); ++i )
      if ( s.S[i] > use_tol ) ++rk;
    const usize ns = A.cols - rk;
    dynmat<F> N(A.cols, ns);
    for ( usize i = 0; i < A.cols; ++i )
      for ( usize j = 0; j < ns; ++j ) N.at(i, j) = s.U.at(i, rk + j);
    return N;
  }
  auto s = svd<F>(A);
  const usize K = (A.rows < A.cols) ? A.rows : A.cols;
  F smax = F(0);
  for ( usize i = 0; i < K; ++i )
    if ( s.S[i] > smax ) smax = s.S[i];
  F use_tol = (tol > F(0)) ? tol : F((A.rows > A.cols) ? A.rows : A.cols) * smax * default_eps<F>();
  usize rk = 0;
  for ( usize i = 0; i < K; ++i )
    if ( s.S[i] > use_tol ) ++rk;
  const usize ns = A.cols - rk;
  dynmat<F> N(A.cols, ns);
  for ( usize i = 0; i < A.cols; ++i )
    for ( usize j = 0; j < ns; ++j ) N.at(i, j) = s.V.at(i, rk + j);
  return N;
}

};     // namespace pseudoinv
};     // namespace linalg
};     // namespace math
};     // namespace micron
