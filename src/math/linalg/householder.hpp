//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Householder & Givens primitives
// & Hessenberg + tridiagonal + bidiagonal reducts

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "ops.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace decomp
{

// H = I - beta * v * v**T
template <ieee754_floating F, usize N> struct householder_t {
  vec<F, N> v;
  F beta;
  usize k0;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline householder_t<F, N>
householder_reflector(const vec<F, N> &x, usize k0) noexcept
{
  householder_t<F, N> h{};
  h.k0 = k0;
  for ( usize i = 0; i < N; ++i ) h.v.data[i] = F(0);
  if ( k0 >= N ) {
    h.beta = F(0);
    return h;
  }
  F xmax = F(0);
  for ( usize i = k0; i < N; ++i ) {
    F a = math::fabs(x.data[i]);
    if ( a > xmax ) xmax = a;
  }
  if ( xmax == F(0) ) {
    h.beta = F(0);
    return h;
  }
  F sum = F(0);
  F inv_xmax = F(1) / xmax;
  for ( usize i = k0; i < N; ++i ) {
    F s = x.data[i] * inv_xmax;
    sum = math::fma<F>(s, s, sum);
  }
  F norm_x = xmax * math::fsqrt(sum);
  // pick alpha such that v[k0] = x[k0]
  F xk = x.data[k0];
  F sign_xk = (xk >= F(0)) ? F(1) : F(-1);
  F alpha = -sign_xk * norm_x;
  // v[k0] = x[k0] - alpha
  h.v.data[k0] = xk - alpha;
  for ( usize i = k0 + 1; i < N; ++i ) h.v.data[i] = x.data[i];
  F vv = h.v.data[k0] * h.v.data[k0];
  for ( usize i = k0 + 1; i < N; ++i ) vv = math::fma<F>(h.v.data[i], h.v.data[i], vv);
  h.beta = (vv > F(0)) ? F(2) / vv : F(0);
  return h;
}

template <ieee754_floating F, usize R, usize C>
inline void
apply_householder_left(mat<F, R, C> &A, const vec<F, R> &v, F beta, usize k0) noexcept
{
  if ( beta == F(0) ) return;
  F w[C];
  for ( usize j = 0; j < C; ++j ) w[j] = F(0);
  for ( usize i = k0; i < R; ++i ) {
    F vi = v.data[i];
    if ( vi == F(0) ) continue;
    F *__restrict__ row = A.data + i * C;
    for ( usize j = 0; j < C; ++j ) w[j] = math::fma<F>(vi, row[j], w[j]);
  }
  for ( usize i = k0; i < R; ++i ) {
    F vi = v.data[i];
    if ( vi == F(0) ) continue;
    F bvi = beta * vi;
    F *__restrict__ row = A.data + i * C;
    for ( usize j = 0; j < C; ++j ) row[j] -= bvi * w[j];
  }
}

template <ieee754_floating F, usize R, usize C>
inline void
apply_householder_right(mat<F, R, C> &A, const vec<F, C> &v, F beta, usize k0) noexcept
{
  if ( beta == F(0) ) return;
  F w[R];
  for ( usize i = 0; i < R; ++i ) {
    F acc = F(0);
    F *__restrict__ row = A.data + i * C;
    for ( usize j = k0; j < C; ++j ) acc = math::fma<F>(row[j], v.data[j], acc);
    w[i] = acc;
  }
  for ( usize i = 0; i < R; ++i ) {
    F bwi = beta * w[i];
    F *__restrict__ row = A.data + i * C;
    for ( usize j = k0; j < C; ++j ) row[j] -= bwi * v.data[j];
  }
}

template <ieee754_floating F> struct givens_t {
  F c;
  F s;
};

template <ieee754_floating F>
[[nodiscard]] inline givens_t<F>
givens_rotation(F a, F b) noexcept
{
  givens_t<F> g{};
  if ( b == F(0) ) {
    g.c = (a >= F(0)) ? F(1) : F(-1);
    g.s = F(0);
    return g;
  }
  if ( a == F(0) ) {
    g.c = F(0);
    g.s = (b >= F(0)) ? F(1) : F(-1);
    return g;
  }
  if ( math::fabs(b) > math::fabs(a) ) {
    F t = a / b;
    F u = math::fsqrt(F(1) + t * t);
    if ( b < F(0) ) u = -u;
    g.s = F(1) / u;
    g.c = g.s * t;
  } else {
    F t = b / a;
    F u = math::fsqrt(F(1) + t * t);
    if ( a < F(0) ) u = -u;
    g.c = F(1) / u;
    g.s = g.c * t;
  }
  return g;
}

// A = Q * H * Q**T
template <ieee754_floating F, usize N> struct hess_result {
  mat<F, N, N> H;
  mat<F, N, N> Q;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline hess_result<F, N>
hessenberg(const mat<F, N, N> &A) noexcept
{
  hess_result<F, N> r{ A, mat<F, N, N>::identity() };
  if constexpr ( N <= 2 ) return r;

  for ( usize k = 0; k + 2 < N; ++k ) {
    vec<F, N> x{};
    for ( usize i = 0; i < N; ++i ) x.data[i] = F(0);
    for ( usize i = k + 1; i < N; ++i ) x.data[i] = r.H.data[i * N + k];
    auto h = householder_reflector<F, N>(x, k + 1);
    if ( h.beta == F(0) ) continue;
    apply_householder_left<F, N, N>(r.H, h.v, h.beta, k + 1);
    apply_householder_right<F, N, N>(r.H, h.v, h.beta, k + 1);
    apply_householder_right<F, N, N>(r.Q, h.v, h.beta, k + 1);
    for ( usize i = k + 2; i < N; ++i ) r.H.data[i * N + k] = F(0);
  }
  return r;
}

template <ieee754_floating F, usize N> struct tridiag_result {
  vec<F, N> diag;
  vec<F, N> subdiag;     // subdiag[0..N-2] valid; subdiag[N-1] unused
  mat<F, N, N> Q;
};

template <ieee754_floating F, usize N>
[[nodiscard]] inline tridiag_result<F, N>
tridiagonalize_sym(const mat<F, N, N> &A) noexcept
{
  auto h = hessenberg<F, N>(A);
  tridiag_result<F, N> r{};
  for ( usize i = 0; i < N; ++i ) r.diag.data[i] = h.H.data[i * N + i];
  for ( usize i = 0; i + 1 < N; ++i ) r.subdiag.data[i] = h.H.data[(i + 1) * N + i];
  if constexpr ( N >= 1 ) r.subdiag.data[N - 1] = F(0);
  r.Q = h.Q;
  return r;
}

// A = U * B * V**T
template <ieee754_floating F, usize R_, usize C_> struct bidiag_result {
  mat<F, R_, C_> B;
  mat<F, R_, R_> U;
  mat<F, C_, C_> V;
};

template <ieee754_floating F, usize R_, usize C_>
[[nodiscard]] inline bidiag_result<F, R_, C_>
bidiagonalize(const mat<F, R_, C_> &A) noexcept
{
  bidiag_result<F, R_, C_> r{ A, mat<F, R_, R_>::identity(), mat<F, C_, C_>::identity() };
  constexpr usize K = (R_ < C_) ? R_ : C_;
  for ( usize k = 0; k < K; ++k ) {
    {
      vec<F, R_> x{};
      for ( usize i = 0; i < R_; ++i ) x.data[i] = F(0);
      for ( usize i = k; i < R_; ++i ) x.data[i] = r.B.data[i * C_ + k];
      auto h = householder_reflector<F, R_>(x, k);
      if ( h.beta != F(0) ) {
        apply_householder_left<F, R_, C_>(r.B, h.v, h.beta, k);
        apply_householder_right<F, R_, R_>(r.U, h.v, h.beta, k);
        for ( usize i = k + 1; i < R_; ++i ) r.B.data[i * C_ + k] = F(0);
      }
    }
    if ( k + 2 <= C_ ) {
      vec<F, C_> y{};
      for ( usize j = 0; j < C_; ++j ) y.data[j] = F(0);
      for ( usize j = k + 1; j < C_; ++j ) y.data[j] = r.B.data[k * C_ + j];
      auto h = householder_reflector<F, C_>(y, k + 1);
      if ( h.beta != F(0) ) {
        apply_householder_right<F, R_, C_>(r.B, h.v, h.beta, k + 1);
        apply_householder_right<F, C_, C_>(r.V, h.v, h.beta, k + 1);
        for ( usize j = k + 2; j < C_; ++j ) r.B.data[k * C_ + j] = F(0);
      }
    }
  }
  return r;
}

namespace __impl_decomp
{

template <ieee754_floating F>
inline void
apply_householder_left_dyn(F *__restrict__ A, usize R, usize C, usize ld_A, const F *v, F beta, usize k0) noexcept
{
  if ( beta == F(0) ) return;
  micron::vector<F, micron::allocator_serial<>, false> w(C, F(0));
  for ( usize i = k0; i < R; ++i ) {
    F vi = v[i];
    if ( vi == F(0) ) continue;
    const F *row = A + i * ld_A;
    for ( usize j = 0; j < C; ++j ) w.data()[j] = math::fma<F>(vi, row[j], w.data()[j]);
  }
  for ( usize i = k0; i < R; ++i ) {
    F vi = v[i];
    if ( vi == F(0) ) continue;
    F bvi = beta * vi;
    F *row = A + i * ld_A;
    for ( usize j = 0; j < C; ++j ) row[j] -= bvi * w.data()[j];
  }
}

template <ieee754_floating F>
inline void
apply_householder_right_dyn(F *__restrict__ A, usize R, usize C, usize ld_A, const F *v, F beta, usize k0) noexcept
{
  if ( beta == F(0) ) return;
  micron::vector<F, micron::allocator_serial<>, false> w(R, F(0));
  for ( usize i = 0; i < R; ++i ) {
    F acc = F(0);
    const F *row = A + i * ld_A;
    for ( usize j = k0; j < C; ++j ) acc = math::fma<F>(row[j], v[j], acc);
    w.data()[i] = acc;
  }
  for ( usize i = 0; i < R; ++i ) {
    F bwi = beta * w.data()[i];
    F *row = A + i * ld_A;
    for ( usize j = k0; j < C; ++j ) row[j] -= bwi * v[j];
  }
}

template <ieee754_floating F>
inline F
householder_reflector_dyn(const F *x, usize R, usize k0, F *v_out) noexcept
{
  for ( usize i = 0; i < R; ++i ) v_out[i] = F(0);
  if ( k0 >= R ) return F(0);
  F xmax = F(0);
  for ( usize i = k0; i < R; ++i ) {
    F a = math::fabs(x[i]);
    if ( a > xmax ) xmax = a;
  }
  if ( xmax == F(0) ) return F(0);
  F sum = F(0);
  F inv_xmax = F(1) / xmax;
  for ( usize i = k0; i < R; ++i ) {
    F s = x[i] * inv_xmax;
    sum = math::fma<F>(s, s, sum);
  }
  F norm_x = xmax * math::fsqrt(sum);
  F xk = x[k0];
  F sign_xk = (xk >= F(0)) ? F(1) : F(-1);
  F alpha = -sign_xk * norm_x;
  v_out[k0] = xk - alpha;
  for ( usize i = k0 + 1; i < R; ++i ) v_out[i] = x[i];
  F vv = v_out[k0] * v_out[k0];
  for ( usize i = k0 + 1; i < R; ++i ) vv = math::fma<F>(v_out[i], v_out[i], vv);
  return (vv > F(0)) ? F(2) / vv : F(0);
}

};     // namespace __impl_decomp

};     // namespace decomp
};     // namespace linalg
};     // namespace math
};     // namespace micron
