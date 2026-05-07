//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// inv<N>(A)
// solve<N>(A, b)
// det<N>(A)
// trace(A)
// kron(A, B): Kronecker product
// lstsq<R,C>(A, b): least-squares via QR
// pinv<R,C>(A): Moore–Penrose pseudoinverse
// condition_number(A): Frobenius
// frobenius_norm(A)
// norm_inf_mat(A): max row-sum
// norm_l1_mat(A): max col-sum
// norm_p_mat(A, p): entrywise p-norm

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../mk.hpp"
#include "decomp.hpp"
#include "ops.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr T
trace(const mat<T, N, N> &A) noexcept
{
  T s{ 0 };
  for ( usize i = 0; i < N; ++i ) s = s + A.data[i * N + i];
  return s;
}

template <ieee754_floating F, usize N>
[[nodiscard]] inline constexpr F
det(const mat<F, N, N> &A) noexcept
{
  auto r = decomp::lu<F, N>(A);
  if ( r.singular ) return F(0);
  F d = F(1);
  for ( usize i = 0; i < N; ++i ) d = d * r.U.data[i * N + i];
  return d;
}

template <ieee754_floating F, usize N>
[[nodiscard]] inline vec<F, N>
solve(const mat<F, N, N> &A, const vec<F, N> &b) noexcept
{
  auto r = decomp::lu<F, N>(A);
  vec<F, N> x{};
  if ( r.singular ) {
    for ( usize i = 0; i < N; ++i ) x.data[i] = ieee::qnan_v<F>();
    return x;
  }
  vec<F, N> y{};
  for ( usize i = 0; i < N; ++i ) {
    F s = b.data[i];
    for ( usize k = 0; k < i; ++k ) s = s - r.L.data[i * N + k] * y.data[k];
    y.data[i] = s;     // L has unit diagonal
  }
  // Ux = y
  for ( usize i = N; i-- > 0; ) {
    F s = y.data[i];
    for ( usize k = i + 1; k < N; ++k ) s = s - r.U.data[i * N + k] * x.data[k];
    x.data[i] = s / r.U.data[i * N + i];
  }
  return x;
}

template <ieee754_floating F, usize N>
[[nodiscard]] inline mat<F, N, N>
inv(const mat<F, N, N> &A) noexcept
{
  auto r = decomp::lu<F, N>(A);
  mat<F, N, N> X = mat<F, N, N>::zero();
  if ( r.singular ) {
    for ( usize i = 0; i < N * N; ++i ) X.data[i] = ieee::qnan_v<F>();
    return X;
  }
  for ( usize j = 0; j < N; ++j ) {
    vec<F, N> y{};
    for ( usize i = 0; i < N; ++i ) {
      F s = (i == j) ? F(1) : F(0);
      for ( usize k = 0; k < i; ++k ) s = s - r.L.data[i * N + k] * y.data[k];
      y.data[i] = s;
    }
    for ( usize i = N; i-- > 0; ) {
      F s = y.data[i];
      for ( usize k = i + 1; k < N; ++k ) s = s - r.U.data[i * N + k] * X.data[k * N + j];
      X.data[i * N + j] = s / r.U.data[i * N + i];
    }
  }
  return X;
}

template <arith_scalar T, usize R1, usize C1, usize R2, usize C2>
[[nodiscard]] inline constexpr mat<T, R1 * R2, C1 * C2>
kron(const mat<T, R1, C1> &A, const mat<T, R2, C2> &B) noexcept
{
  mat<T, R1 * R2, C1 * C2> K = mat<T, R1 * R2, C1 * C2>::zero();
  for ( usize i = 0; i < R1; ++i ) {
    for ( usize j = 0; j < C1; ++j ) {
      const T a = A.data[i * C1 + j];
      for ( usize p = 0; p < R2; ++p ) {
        for ( usize q = 0; q < C2; ++q ) {
          K.data[(i * R2 + p) * (C1 * C2) + (j * C2 + q)] = a * B.data[p * C2 + q];
        }
      }
    }
  }
  return K;
}

// norms

template <ieee754_floating F, usize R_, usize C_>
[[nodiscard, gnu::flatten]] inline constexpr F
frobenius_norm(const mat<F, R_, C_> &A) noexcept
{
  F s = F(0);
  for ( usize i = 0; i < R_ * C_; ++i ) s = math::fma<F>(A.data[i], A.data[i], s);
  return mk::pow_ns::sqrt<F>(s);
}

template <ieee754_floating F, usize R_, usize C_>
[[nodiscard, gnu::flatten]] inline constexpr F
norm_inf_mat(const mat<F, R_, C_> &A) noexcept
{
  // ||A||_∞ = max row sum of |A_ij|
  F m = F(0);
  for ( usize i = 0; i < R_; ++i ) {
    F s = F(0);
    for ( usize j = 0; j < C_; ++j ) s = s + mk::manip::fabs<F>(A.data[i * C_ + j]);
    if ( s > m ) m = s;
  }
  return m;
}

template <ieee754_floating F, usize R_, usize C_>
[[nodiscard, gnu::flatten]] inline constexpr F
norm_l1_mat(const mat<F, R_, C_> &A) noexcept
{
  F m = F(0);
  for ( usize j = 0; j < C_; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < R_; ++i ) s = s + mk::manip::fabs<F>(A.data[i * C_ + j]);
    if ( s > m ) m = s;
  }
  return m;
}

template <ieee754_floating F, usize R_, usize C_>
[[nodiscard, gnu::flatten]] inline constexpr F
norm_p_mat(const mat<F, R_, C_> &A, F p) noexcept
{
  F s = F(0);
  for ( usize i = 0; i < R_ * C_; ++i ) {
    s = s + mk::pow_ns::pow<F>(mk::manip::fabs<F>(A.data[i]), p);
  }
  return mk::pow_ns::pow<F>(s, F(1) / p);
}

template <ieee754_floating F, usize N>
[[nodiscard]] inline F
condition_number(const mat<F, N, N> &A) noexcept
{
  const F na = frobenius_norm(A);
  const auto Ai = inv<F, N>(A);
  if ( ieee::is_nan(Ai.data[0]) ) return ieee::inf_v<F>(0);
  return na * frobenius_norm(Ai);
}

template <ieee754_floating F, usize R_, usize C_>
  requires(R_ >= C_)
[[nodiscard]] inline vec<F, C_>
lstsq(const mat<F, R_, C_> &A, const vec<F, R_> &b) noexcept
{
  auto qr_d = decomp::qr<F, R_, C_>(A);
  vec<F, C_> y{};
  for ( usize j = 0; j < C_; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < R_; ++i ) s = math::fma<F>(qr_d.Q.data[i * C_ + j], b.data[i], s);
    y.data[j] = s;
  }
  vec<F, C_> x{};
  for ( usize i = C_; i-- > 0; ) {
    F s = y.data[i];
    for ( usize k = i + 1; k < C_; ++k ) s = s - qr_d.R.data[i * C_ + k] * x.data[k];
    x.data[i] = s / qr_d.R.data[i * C_ + i];
  }
  return x;
}

template <ieee754_floating F, usize R_, usize C_>
  requires(R_ >= C_)
[[nodiscard]] inline mat<F, C_, R_>
pinv(const mat<F, R_, C_> &A) noexcept
{
  auto qr_d = decomp::qr<F, R_, C_>(A);
  mat<F, C_, C_> Rinv = mat<F, C_, C_>::zero();
  for ( usize j = 0; j < C_; ++j ) {
    for ( usize i = C_; i-- > 0; ) {
      F s = (i == j) ? F(1) : F(0);
      for ( usize k = i + 1; k < C_; ++k ) s = s - qr_d.R.data[i * C_ + k] * Rinv.data[k * C_ + j];
      Rinv.data[i * C_ + j] = s / qr_d.R.data[i * C_ + i];
    }
  }
  mat<F, C_, R_> P = mat<F, C_, R_>::zero();
  for ( usize i = 0; i < C_; ++i ) {
    for ( usize j = 0; j < R_; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < C_; ++k ) s = math::fma<F>(Rinv.data[i * C_ + k], qr_d.Q.data[j * C_ + k], s);
      P.data[i * R_ + j] = s;
    }
  }
  return P;
}

};     // namespace linalg
};     // namespace math
};     // namespace micron
