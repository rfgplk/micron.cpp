//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../bits/sqrt.hpp"
#include "../bits/trig.hpp"
#include "../generic.hpp"
#include "../sqrt.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace ops
{

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr T
dot(const vec<T, N> &a, const vec<T, N> &b) noexcept
{
  const T *__restrict__ pa = a.data;
  const T *__restrict__ pb = b.data;
  if constexpr ( N >= 4 ) {
    // dual accumulators
    T acc0{}, acc1{};
    constexpr usize H = N / 2;
    for ( usize i = 0; i < H; ++i ) {
      if constexpr ( micron::is_floating_point_v<T> )
        acc0 = math::fma<T>(pa[i], pb[i], acc0);
      else
        acc0 += pa[i] * pb[i];
    }
    for ( usize i = H; i < N; ++i ) {
      if constexpr ( micron::is_floating_point_v<T> )
        acc1 = math::fma<T>(pa[i], pb[i], acc1);
      else
        acc1 += pa[i] * pb[i];
    }
    return acc0 + acc1;
  } else {
    T acc{};
    for ( usize i = 0; i < N; ++i ) {
      if constexpr ( micron::is_floating_point_v<T> )
        acc = math::fma<T>(pa[i], pb[i], acc);
      else
        acc += pa[i] * pb[i];
    }
    return acc;
  }
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 3>
cross(const vec<F, 3> &a, const vec<F, 3> &b) noexcept
{
  const F *__restrict__ pa = a.data;
  const F *__restrict__ pb = b.data;
  return { math::fma<F>(pa[1], pb[2], -pa[2] * pb[1]), math::fma<F>(pa[2], pb[0], -pa[0] * pb[2]),
           math::fma<F>(pa[0], pb[1], -pa[1] * pb[0]) };
}

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, R, C>
outer(const vec<T, R> &a, const vec<T, C> &b) noexcept
{
  mat<T, R, C> m{};
  for ( usize r = 0; r < R; ++r )
    for ( usize c = 0; c < C; ++c ) m.data[r * C + c] = a.data[r] * b.data[c];
  return m;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// norms
template <ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline F
norm(const vec<F, N> &v) noexcept
{
  return math::fsqrt(dot(v, v));
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr T
norm_sq(const vec<T, N> &v) noexcept
{
  return dot(v, v);
}

template <ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr F
norm_l1(const vec<F, N> &v) noexcept
{
  F s = F(0);
  for ( usize i = 0; i < N; ++i ) s += math::fabs(v.data[i]);
  return s;
}

template <ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr F
norm_inf(const vec<F, N> &v) noexcept
{
  F m = math::fabs(v.data[0]);
  for ( usize i = 1; i < N; ++i ) {
    F a = math::fabs(v.data[i]);
    if ( a > m ) m = a;
  }
  return m;
}

template <ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline vec<F, N>
normalize(const vec<F, N> &v) noexcept
{
  F n = norm(v);
  return (n == F(0)) ? v : v / n;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// hadamard / abs / clamp
template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
hadamard(const vec<T, N> &a, const vec<T, N> &b) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = a.data[i] * b.data[i];
  return r;
}

template <ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, N>
abs_v(const vec<F, N> &a) noexcept
{
  vec<F, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = math::fabs(a.data[i]);
  return r;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
clamp_v(const vec<T, N> &a, T lo, T hi) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = (a.data[i] < lo) ? lo : ((a.data[i] > hi) ? hi : a.data[i]);
  return r;
}

template <arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
clamp_v(const vec<T, N> &a, const vec<T, N> &lo, const vec<T, N> &hi) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) {
    T x = a.data[i];
    r.data[i] = (x < lo.data[i]) ? lo.data[i] : ((x > hi.data[i]) ? hi.data[i] : x);
  }
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// interpolations
template <ieee754_floating F, usize N, typename U>
  requires(micron::is_convertible_v<U, F>)
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, N>
lerp(const vec<F, N> &a, const vec<F, N> &b, U t) noexcept
{
  F ft = static_cast<F>(t);
  vec<F, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = math::fma<F>(ft, b.data[i] - a.data[i], a.data[i]);
  return r;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
lerp_s(F a, F b, F t) noexcept
{
  return math::fma<F>(t, b - a, a);
}

template <ieee754_floating F, typename U>
  requires(micron::is_convertible_v<U, F>)
[[nodiscard, gnu::always_inline]] inline quat<F>
nlerp(const quat<F> &a, const quat<F> &b, U t) noexcept
{
  F ft = static_cast<F>(t);
  vec<F, 4> va{ a.data[0], a.data[1], a.data[2], a.data[3] };
  vec<F, 4> vb{ b.data[0], b.data[1], b.data[2], b.data[3] };
  vec<F, 4> r = lerp(va, vb, ft);
  F n = norm(r);
  quat<F> q{};
  for ( usize i = 0; i < 4; ++i ) q.data[i] = r.data[i] / n;
  return q;
}

template <ieee754_floating F, typename U>
  requires(micron::is_convertible_v<U, F>)
[[nodiscard]] inline quat<F>
slerp(const quat<F> &a, const quat<F> &b, U t) noexcept
{
  F ft = static_cast<F>(t);
  vec<F, 4> va{ a.data[0], a.data[1], a.data[2], a.data[3] };
  vec<F, 4> vb{ b.data[0], b.data[1], b.data[2], b.data[3] };
  F cos_theta = dot(va, vb);
  if ( cos_theta < F(0) ) {
    for ( usize i = 0; i < 4; ++i ) vb.data[i] = -vb.data[i];
    cos_theta = -cos_theta;
  }
  if ( cos_theta > F(0.9995) ) {
    vec<F, 4> r = lerp(va, vb, ft);
    F n = norm(r);
    quat<F> q{};
    for ( usize i = 0; i < 4; ++i ) q.data[i] = r.data[i] / n;
    return q;
  }
  const F sin_theta = math::fsqrt(F(1) - cos_theta * cos_theta);
  const F theta = mkbits::trig_ns::acos<F>(cos_theta);
  const F inv_sin_theta = F(1) / sin_theta;
  const F w_a = mkbits::trig_ns::sin<F>((F(1) - ft) * theta) * inv_sin_theta;
  const F w_b = mkbits::trig_ns::sin<F>(ft * theta) * inv_sin_theta;
  quat<F> q{};
  for ( usize i = 0; i < 4; ++i ) q.data[i] = w_a * va.data[i] + w_b * vb.data[i];
  return q;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reg ops
template <ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, N>
reflect(const vec<F, N> &i, const vec<F, N> &n) noexcept
{
  F d = dot(i, n);
  vec<F, N> r{};
  for ( usize k = 0; k < N; ++k ) r.data[k] = math::fma<F>(F(-2) * d, n.data[k], i.data[k]);
  return r;
}

template <ieee754_floating F, usize N>
[[nodiscard]] inline vec<F, N>
refract(const vec<F, N> &i, const vec<F, N> &n, F eta) noexcept
{
  F d = dot(i, n);
  F k = F(1) - eta * eta * (F(1) - d * d);
  vec<F, N> r{};
  if ( k < F(0) ) {
    for ( usize j = 0; j < N; ++j ) r.data[j] = F(0);
    return r;
  }
  F s = math::fsqrt(k);
  for ( usize j = 0; j < N; ++j ) r.data[j] = eta * i.data[j] - (eta * d + s) * n.data[j];
  return r;
}

template <ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline vec<F, N>
project(const vec<F, N> &u, const vec<F, N> &v) noexcept
{
  F num = dot(u, v);
  F den = dot(v, v);
  F t = num / den;
  vec<F, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = t * v.data[i];
  return r;
}

template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr vec<T, R>
gemv(const mat<T, R, C> &m, const vec<T, C> &v) noexcept
{
  const T *__restrict__ a = m.data;
  const T *__restrict__ vp = v.data;
  vec<T, R> r{};
  for ( usize i = 0; i < R; ++i ) {
    const T *__restrict__ row = a + i * C;
    T acc{};
    for ( usize j = 0; j < C; ++j ) {
      if constexpr ( micron::is_floating_point_v<T> )
        acc = math::fma<T>(row[j], vp[j], acc);
      else
        acc += row[j] * vp[j];
    }
    r.data[i] = acc;
  }
  return r;
}

template <arith_scalar T, usize M, usize K, usize N>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr mat<T, M, N>
gemm(const mat<T, M, K> &A, const mat<T, K, N> &B) noexcept
{
  const T *__restrict__ a = A.data;
  const T *__restrict__ b = B.data;
  mat<T, M, N> C = mat<T, M, N>::zero();
  T *__restrict__ c = C.data;
  for ( usize i = 0; i < M; ++i ) {
    const T *__restrict__ row = a + i * K;
    for ( usize j = 0; j < N; ++j ) {
      T acc{};
      for ( usize p = 0; p < K; ++p ) {
        if constexpr ( micron::is_floating_point_v<T> )
          acc = math::fma<T>(row[p], b[p * N + j], acc);
        else
          acc += row[p] * b[p * N + j];
      }
      c[i * N + j] = acc;
    }
  }
  return C;
}

// transpose
template <arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, C, R>
transpose(const mat<T, R, C> &m) noexcept
{
  mat<T, C, R> r{};
  for ( usize i = 0; i < R; ++i )
    for ( usize j = 0; j < C; ++j ) r.data[j * R + i] = m.data[i * C + j];
  return r;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
det2(const mat<F, 2, 2> &m) noexcept
{
  return math::fma<F>(m.data[0], m.data[3], -m.data[1] * m.data[2]);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
det3(const mat<F, 3, 3> &m) noexcept
{
  const F *a = m.data;
  // a[i*3+j]
  F m0 = math::fma<F>(a[4], a[8], -a[5] * a[7]);
  F m1 = math::fma<F>(a[3], a[8], -a[5] * a[6]);
  F m2 = math::fma<F>(a[3], a[7], -a[4] * a[6]);
  return math::fma<F>(a[0], m0, math::fma<F>(-a[1], m1, a[2] * m2));
}

template <ieee754_floating F>
[[nodiscard]] inline constexpr F
det4(const mat<F, 4, 4> &m) noexcept
{
  const F *__restrict__ a = m.data;
  // 2x2 minors of the lower 2 rows
  F c0 = math::fma<F>(a[10], a[15], -a[11] * a[14]);
  F c1 = math::fma<F>(a[9], a[15], -a[11] * a[13]);
  F c2 = math::fma<F>(a[9], a[14], -a[10] * a[13]);
  F c3 = math::fma<F>(a[8], a[15], -a[11] * a[12]);
  F c4 = math::fma<F>(a[8], a[14], -a[10] * a[12]);
  F c5 = math::fma<F>(a[8], a[13], -a[9] * a[12]);
  // expansion along the first row
  F t0 = a[0] * math::fma<F>(a[5], c0, math::fma<F>(-a[6], c1, a[7] * c2));
  F t1 = a[1] * math::fma<F>(a[4], c0, math::fma<F>(-a[6], c3, a[7] * c4));
  F t2 = a[2] * math::fma<F>(a[4], c1, math::fma<F>(-a[5], c3, a[7] * c5));
  F t3 = a[3] * math::fma<F>(a[4], c2, math::fma<F>(-a[5], c4, a[6] * c5));
  return t0 - t1 + t2 - t3;
}

template <ieee754_floating F>
[[nodiscard]] inline constexpr mat<F, 2, 2>
inv2(const mat<F, 2, 2> &m) noexcept
{
  F d = det2(m);
  F inv = F(1) / d;
  mat<F, 2, 2> r{};
  r.data[0] = m.data[3] * inv;
  r.data[1] = -m.data[1] * inv;
  r.data[2] = -m.data[2] * inv;
  r.data[3] = m.data[0] * inv;
  return r;
}

template <ieee754_floating F>
[[nodiscard]] inline constexpr mat<F, 3, 3>
inv3(const mat<F, 3, 3> &m) noexcept
{
  const F *a = m.data;
  F c00 = math::fma<F>(a[4], a[8], -a[5] * a[7]);
  F c01 = math::fma<F>(a[5], a[6], -a[3] * a[8]);
  F c02 = math::fma<F>(a[3], a[7], -a[4] * a[6]);
  F c10 = math::fma<F>(a[2], a[7], -a[1] * a[8]);
  F c11 = math::fma<F>(a[0], a[8], -a[2] * a[6]);
  F c12 = math::fma<F>(a[1], a[6], -a[0] * a[7]);
  F c20 = math::fma<F>(a[1], a[5], -a[2] * a[4]);
  F c21 = math::fma<F>(a[2], a[3], -a[0] * a[5]);
  F c22 = math::fma<F>(a[0], a[4], -a[1] * a[3]);
  F d = a[0] * c00 + a[1] * c01 + a[2] * c02;
  F inv = F(1) / d;
  mat<F, 3, 3> r{};
  r.data[0] = c00 * inv;
  r.data[1] = c10 * inv;
  r.data[2] = c20 * inv;
  r.data[3] = c01 * inv;
  r.data[4] = c11 * inv;
  r.data[5] = c21 * inv;
  r.data[6] = c02 * inv;
  r.data[7] = c12 * inv;
  r.data[8] = c22 * inv;
  return r;
}

template <ieee754_floating F>
[[nodiscard]] inline constexpr mat<F, 4, 4>
inv4_adj(const mat<F, 4, 4> &m) noexcept
{
  const F *__restrict__ a = m.data;
  F b00 = a[0] * a[5] - a[1] * a[4];
  F b01 = a[0] * a[6] - a[2] * a[4];
  F b02 = a[0] * a[7] - a[3] * a[4];
  F b03 = a[1] * a[6] - a[2] * a[5];
  F b04 = a[1] * a[7] - a[3] * a[5];
  F b05 = a[2] * a[7] - a[3] * a[6];
  F b06 = a[8] * a[13] - a[9] * a[12];
  F b07 = a[8] * a[14] - a[10] * a[12];
  F b08 = a[8] * a[15] - a[11] * a[12];
  F b09 = a[9] * a[14] - a[10] * a[13];
  F b10 = a[9] * a[15] - a[11] * a[13];
  F b11 = a[10] * a[15] - a[11] * a[14];

  F d = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
  F inv = F(1) / d;

  mat<F, 4, 4> r{};
  r.data[0] = (a[5] * b11 - a[6] * b10 + a[7] * b09) * inv;
  r.data[1] = (-a[1] * b11 + a[2] * b10 - a[3] * b09) * inv;
  r.data[2] = (a[13] * b05 - a[14] * b04 + a[15] * b03) * inv;
  r.data[3] = (-a[9] * b05 + a[10] * b04 - a[11] * b03) * inv;
  r.data[4] = (-a[4] * b11 + a[6] * b08 - a[7] * b07) * inv;
  r.data[5] = (a[0] * b11 - a[2] * b08 + a[3] * b07) * inv;
  r.data[6] = (-a[12] * b05 + a[14] * b02 - a[15] * b01) * inv;
  r.data[7] = (a[8] * b05 - a[10] * b02 + a[11] * b01) * inv;
  r.data[8] = (a[4] * b10 - a[5] * b08 + a[7] * b06) * inv;
  r.data[9] = (-a[0] * b10 + a[1] * b08 - a[3] * b06) * inv;
  r.data[10] = (a[12] * b04 - a[13] * b02 + a[15] * b00) * inv;
  r.data[11] = (-a[8] * b04 + a[9] * b02 - a[11] * b00) * inv;
  r.data[12] = (-a[4] * b09 + a[5] * b07 - a[6] * b06) * inv;
  r.data[13] = (a[0] * b09 - a[1] * b07 + a[2] * b06) * inv;
  r.data[14] = (-a[12] * b03 + a[13] * b01 - a[14] * b00) * inv;
  r.data[15] = (a[8] * b03 - a[9] * b01 + a[10] * b00) * inv;
  return r;
}

// Schur-complement inverse
namespace inv4_impl
{
template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, 2, 2>
mul2(const mat<F, 2, 2> &a, const mat<F, 2, 2> &b) noexcept
{
  return mat<F, 2, 2>{
    { math::fma<F>(a.data[0], b.data[0], a.data[1] * b.data[2]), math::fma<F>(a.data[0], b.data[1], a.data[1] * b.data[3]),
      math::fma<F>(a.data[2], b.data[0], a.data[3] * b.data[2]), math::fma<F>(a.data[2], b.data[1], a.data[3] * b.data[3]) }
  };
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, 2, 2>
add2(const mat<F, 2, 2> &a, const mat<F, 2, 2> &b) noexcept
{
  return mat<F, 2, 2>{ { a.data[0] + b.data[0], a.data[1] + b.data[1], a.data[2] + b.data[2], a.data[3] + b.data[3] } };
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, 2, 2>
sub2(const mat<F, 2, 2> &a, const mat<F, 2, 2> &b) noexcept
{
  return mat<F, 2, 2>{ { a.data[0] - b.data[0], a.data[1] - b.data[1], a.data[2] - b.data[2], a.data[3] - b.data[3] } };
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, 2, 2>
neg2(const mat<F, 2, 2> &a) noexcept
{
  return mat<F, 2, 2>{ { -a.data[0], -a.data[1], -a.data[2], -a.data[3] } };
}

};     // namespace inv4_impl

template <ieee754_floating F>
[[nodiscard]] inline constexpr mat<F, 4, 4>
inv4_blocked(const mat<F, 4, 4> &m) noexcept
{
  const F *__restrict__ a = m.data;
  mat<F, 2, 2> A{ { a[0], a[1], a[4], a[5] } };
  mat<F, 2, 2> B{ { a[2], a[3], a[6], a[7] } };
  mat<F, 2, 2> C{ { a[8], a[9], a[12], a[13] } };
  mat<F, 2, 2> D{ { a[10], a[11], a[14], a[15] } };

  const F detA = math::fma<F>(A.data[0], A.data[3], -A.data[1] * A.data[2]);
  const F a_norm2 = A.data[0] * A.data[0] + A.data[1] * A.data[1] + A.data[2] * A.data[2] + A.data[3] * A.data[3];
  const F eps2 = math::default_eps<F>() * math::default_eps<F>();
  if ( detA * detA < eps2 * a_norm2 * a_norm2 ) return inv4_adj<F>(m);

  const F invDetA = F(1) / detA;
  const mat<F, 2, 2> Y{ { A.data[3] * invDetA, -A.data[1] * invDetA, -A.data[2] * invDetA, A.data[0] * invDetA } };
  const mat<F, 2, 2> CY = inv4_impl::mul2<F>(C, Y);
  const mat<F, 2, 2> CYB = inv4_impl::mul2<F>(CY, B);
  const mat<F, 2, 2> X = inv4_impl::sub2<F>(D, CYB);

  const F detX = math::fma<F>(X.data[0], X.data[3], -X.data[1] * X.data[2]);
  if ( detX == F(0) ) return inv4_adj<F>(m);
  const F invDetX = F(1) / detX;
  const mat<F, 2, 2> X_inv{ { X.data[3] * invDetX, -X.data[1] * invDetX, -X.data[2] * invDetX, X.data[0] * invDetX } };

  const mat<F, 2, 2> YB = inv4_impl::mul2<F>(Y, B);
  const mat<F, 2, 2> YB_Xinv = inv4_impl::mul2<F>(YB, X_inv);
  const mat<F, 2, 2> UR = inv4_impl::neg2<F>(YB_Xinv);
  const mat<F, 2, 2> Xinv_CY = inv4_impl::mul2<F>(X_inv, CY);
  const mat<F, 2, 2> LL = inv4_impl::neg2<F>(Xinv_CY);
  const mat<F, 2, 2> YB_Xinv_CY = inv4_impl::mul2<F>(YB_Xinv, CY);
  const mat<F, 2, 2> UL = inv4_impl::add2<F>(Y, YB_Xinv_CY);

  mat<F, 4, 4> r{};
  r.data[0] = UL.data[0];
  r.data[1] = UL.data[1];
  r.data[4] = UL.data[2];
  r.data[5] = UL.data[3];
  r.data[2] = UR.data[0];
  r.data[3] = UR.data[1];
  r.data[6] = UR.data[2];
  r.data[7] = UR.data[3];
  r.data[8] = LL.data[0];
  r.data[9] = LL.data[1];
  r.data[12] = LL.data[2];
  r.data[13] = LL.data[3];
  r.data[10] = X_inv.data[0];
  r.data[11] = X_inv.data[1];
  r.data[14] = X_inv.data[2];
  r.data[15] = X_inv.data[3];
  return r;
}

template <ieee754_floating F>
[[nodiscard]] inline constexpr mat<F, 4, 4>
inv4(const mat<F, 4, 4> &m) noexcept
{
  return inv4_adj<F>(m);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline vec<F, 2>
solve2(const mat<F, 2, 2> &A, const vec<F, 2> &b) noexcept
{
  return gemv(inv2(A), b);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline vec<F, 3>
solve3(const mat<F, 3, 3> &A, const vec<F, 3> &b) noexcept
{
  return gemv(inv3(A), b);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline vec<F, 4>
solve4(const mat<F, 4, 4> &A, const vec<F, 4> &b) noexcept
{
  return gemv(inv4(A), b);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr quat<F>
mul(const quat<F> &a, const quat<F> &b) noexcept
{
  // (x,y,z,w) Hamilton product
  F ax = a.data[0], ay = a.data[1], az = a.data[2], aw = a.data[3];
  F bx = b.data[0], by = b.data[1], bz = b.data[2], bw = b.data[3];
  return { math::fma<F>(aw, bx, math::fma<F>(ax, bw, math::fma<F>(ay, bz, -az * by))),
           math::fma<F>(aw, by, math::fma<F>(ay, bw, math::fma<F>(az, bx, -ax * bz))),
           math::fma<F>(aw, bz, math::fma<F>(az, bw, math::fma<F>(ax, by, -ay * bx))),
           math::fma<F>(aw, bw, -math::fma<F>(ax, bx, math::fma<F>(ay, by, az * bz))) };
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr quat<F>
conjugate(const quat<F> &q) noexcept
{
  return { -q.data[0], -q.data[1], -q.data[2], q.data[3] };
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
norm(const quat<F> &q) noexcept
{
  return math::fsqrt(
      math::fma<F>(q.data[0], q.data[0], math::fma<F>(q.data[1], q.data[1], math::fma<F>(q.data[2], q.data[2], q.data[3] * q.data[3]))));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline quat<F>
normalize(const quat<F> &q) noexcept
{
  F n = norm(q);
  if ( n == F(0) ) return q;
  F inv = F(1) / n;
  return { q.data[0] * inv, q.data[1] * inv, q.data[2] * inv, q.data[3] * inv };
}

// rotate vector by quaternion
template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 3>
rotate(const quat<F> &q, const vec<F, 3> &v) noexcept
{
  // r = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
  vec<F, 3> qv{ q.data[0], q.data[1], q.data[2] };
  vec<F, 3> t = cross(qv, v);
  vec<F, 3> u = cross(qv, t);
  return { v.data[0] + F(2) * (q.data[3] * t.data[0] + u.data[0]), v.data[1] + F(2) * (q.data[3] * t.data[1] + u.data[1]),
           v.data[2] + F(2) * (q.data[3] * t.data[2] + u.data[2]) };
}

};     // namespace ops
};     // namespace linalg
};     // namespace math
};     // namespace micron
