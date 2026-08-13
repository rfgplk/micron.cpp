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

// gfx register cores
#include "../__vec_simd.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace ops
{

template<arith_scalar T, usize N>
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

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 3>
cross(const vec<F, 3> &a, const vec<F, 3> &b) noexcept
{
  if !consteval {
#if defined(__micron_gfx_simd)
    if constexpr ( micron::same_as<F, f32> ) {
      vec<F, 3> out{};
      const simd::f128 va = __vsimd::__load(reinterpret_cast<const float *>(a.data));
      const simd::f128 vb = __vsimd::__load(reinterpret_cast<const float *>(b.data));
      __vsimd::__store(reinterpret_cast<float *>(out.data), __vsimd::__cross3(va, vb));
      return out;
    }
#endif
  }
  const F *__restrict__ pa = a.data;
  const F *__restrict__ pb = b.data;
  return { math::fma<F>(pa[1], pb[2], -pa[2] * pb[1]), math::fma<F>(pa[2], pb[0], -pa[0] * pb[2]),
           math::fma<F>(pa[0], pb[1], -pa[1] * pb[0]) };
}

template<arith_scalar T, usize R, usize C>
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
template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline F
norm(const vec<F, N> &v) noexcept
{
  return math::fsqrt(dot(v, v));
}

template<arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr T
norm_sq(const vec<T, N> &v) noexcept
{
  return dot(v, v);
}

template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr F
norm_l1(const vec<F, N> &v) noexcept
{
  F s = F(0);
  for ( usize i = 0; i < N; ++i ) s += math::fabs(v.data[i]);
  return s;
}

template<ieee754_floating F, usize N>
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

template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline vec<F, N>
normalize(const vec<F, N> &v) noexcept
{
#if defined(__micron_gfx_simd)
  if constexpr ( micron::same_as<F, f32> && (N == 3 || N == 4) ) {
    static_assert(sizeof(vec<f32, 3>) == 16 && sizeof(vec<f32, 4>) == 16);
    vec<F, N> out{};
    const simd::f128 vv = __vsimd::__load(reinterpret_cast<const float *>(v.data));
    simd::f128 n2;
    if constexpr ( N == 3 )
      n2 = __vsimd::__dot3_splat(vv, vv);
    else
      n2 = __vsimd::__dot4_splat(vv, vv);
    const simd::f128 r = __vsimd::__mul(vv, __vsimd::__inv_sqrt_exact(n2));
    // n == 0 <=> n2 == 0 (sqrt of any nonzero, incl. denormal, is nonzero)
    const simd::f128 zmask = __vsimd::__cmp_eq(n2, __vsimd::__splat(0.0f));
    __vsimd::__store(reinterpret_cast<float *>(out.data), __vsimd::__select(zmask, vv, r));
    return out;
  }
#endif
  const F n2 = dot(v, v);
  if ( n2 == F(0) ) return v;
  const F inv = __vsimd::__inv_sqrt_exact_s(n2);
  vec<F, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = v.data[i] * inv;
  return r;
}

// policy::fast -- rsqrt estimate + Newton refinement (~2^-22 rel, f32 only);
// every other type/policy answers the exact tier above
template<ieee754_floating F, usize N, math::policy::policy_tag P>
[[nodiscard, gnu::always_inline]] inline vec<F, N>
normalize(const vec<F, N> &v, P) noexcept
{
  if constexpr ( micron::is_same_v<P, math::policy::fast_tag> && micron::same_as<F, f32> ) {
#if defined(__micron_gfx_simd)
    if constexpr ( N == 3 || N == 4 ) {
      vec<F, N> out{};
      const simd::f128 vv = __vsimd::__load(reinterpret_cast<const float *>(v.data));
      simd::f128 n2;
      if constexpr ( N == 3 )
        n2 = __vsimd::__dot3_splat(vv, vv);
      else
        n2 = __vsimd::__dot4_splat(vv, vv);
      const simd::f128 r = __vsimd::__mul(vv, __vsimd::__inv_sqrt_fast(n2));
      const simd::f128 zmask = __vsimd::__cmp_eq(n2, __vsimd::__splat(0.0f));
      __vsimd::__store(reinterpret_cast<float *>(out.data), __vsimd::__select(zmask, vv, r));
      return out;
    }
#endif
    const F n2 = dot(v, v);
    if ( n2 == F(0) ) return v;
    const F inv = __vsimd::__inv_sqrt_fast_s(n2);
    vec<F, N> r{};
    for ( usize i = 0; i < N; ++i ) r.data[i] = v.data[i] * inv;
    return r;
  } else {
    return normalize<F, N>(v);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// hadamard / abs / clamp
template<arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
hadamard(const vec<T, N> &a, const vec<T, N> &b) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = a.data[i] * b.data[i];
  return r;
}

template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, N>
abs_v(const vec<F, N> &a) noexcept
{
  vec<F, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = math::fabs(a.data[i]);
  return r;
}

template<arith_scalar T, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<T, N>
clamp_v(const vec<T, N> &a, T lo, T hi) noexcept
{
  vec<T, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = (a.data[i] < lo) ? lo : ((a.data[i] > hi) ? hi : a.data[i]);
  return r;
}

template<arith_scalar T, usize N>
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
template<ieee754_floating F, usize N, typename U>
  requires(micron::is_convertible_v<U, F>)
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, N>
lerp(const vec<F, N> &a, const vec<F, N> &b, U t) noexcept
{
  F ft = static_cast<F>(t);
  vec<F, N> r{};
  for ( usize i = 0; i < N; ++i ) r.data[i] = math::fma<F>(ft, b.data[i] - a.data[i], a.data[i]);
  return r;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
lerp_s(F a, F b, F t) noexcept
{
  return math::fma<F>(t, b - a, a);
}

template<ieee754_floating F, typename U>
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

template<ieee754_floating F, typename U>
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
template<ieee754_floating F, usize N>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, N>
reflect(const vec<F, N> &i, const vec<F, N> &n) noexcept
{
  F d = dot(i, n);
  vec<F, N> r{};
  for ( usize k = 0; k < N; ++k ) r.data[k] = math::fma<F>(F(-2) * d, n.data[k], i.data[k]);
  return r;
}

template<ieee754_floating F, usize N>
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

template<ieee754_floating F, usize N>
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

template<arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr vec<T, R>
gemv(const mat<T, R, C> &m, const vec<T, C> &v) noexcept
{
  if !consteval {
#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)
    // row-major mat4*vec4 (f32)
    if constexpr ( micron::same_as<T, f32> && R == 4 && C == 4 ) {
      vec<T, 4> out{};
      const float *mp = reinterpret_cast<const float *>(m.data);
      const __m128 vv = simd::sse::loadu_f32(reinterpret_cast<const float *>(v.data));
      const __m128 p0 = simd::sse::mul_f32(simd::sse::loadu_f32(mp + 0), vv);
      const __m128 p1 = simd::sse::mul_f32(simd::sse::loadu_f32(mp + 4), vv);
      const __m128 p2 = simd::sse::mul_f32(simd::sse::loadu_f32(mp + 8), vv);
      const __m128 p3 = simd::sse::mul_f32(simd::sse::loadu_f32(mp + 12), vv);
      const __m128 s01 = simd::sse::hadd_f32(p0, p1);
      const __m128 s23 = simd::sse::hadd_f32(p2, p3);
      const __m128 rr = simd::sse::hadd_f32(s01, s23);
      simd::sse::storeu_f32(reinterpret_cast<float *>(out.data), rr);
      return out;
    }
#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
    if constexpr ( micron::same_as<T, f32> && R == 4 && C == 4 ) {
      vec<T, 4> out{};
      const float *mp = reinterpret_cast<const float *>(m.data);
      const float *vp = reinterpret_cast<const float *>(v.data);
      const float32x4_t m0 = simd::neon::load_f32(mp + 0);
      const float32x4_t m1 = simd::neon::load_f32(mp + 4);
      const float32x4_t m2 = simd::neon::load_f32(mp + 8);
      const float32x4_t m3 = simd::neon::load_f32(mp + 12);
      const float32x4_t e0 = simd::neon::zip_lo_f32(m0, m1);
      const float32x4_t e1 = simd::neon::zip_hi_f32(m0, m1);
      const float32x4_t e2 = simd::neon::zip_lo_f32(m2, m3);
      const float32x4_t e3 = simd::neon::zip_hi_f32(m2, m3);
      const float32x4_t c0 = simd::neon::concat_lo_f32(e0, e2);
      const float32x4_t c1 = simd::neon::concat_hi_f32(e0, e2);
      const float32x4_t c2 = simd::neon::concat_lo_f32(e1, e3);
      const float32x4_t c3 = simd::neon::concat_hi_f32(e1, e3);
      float32x4_t r = simd::neon::mul(simd::neon::splat_f32(vp[0]), c0);
      r = simd::neon::add(r, simd::neon::mul(simd::neon::splat_f32(vp[1]), c1));
      r = simd::neon::add(r, simd::neon::mul(simd::neon::splat_f32(vp[2]), c2));
      r = simd::neon::add(r, simd::neon::mul(simd::neon::splat_f32(vp[3]), c3));
      simd::neon::store_f32(reinterpret_cast<float *>(out.data), r);
      return out;
    }
#endif
  }
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

template<arith_scalar T, usize M, usize K, usize N>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr mat<T, M, N>
gemm(const mat<T, M, K> &A, const mat<T, K, N> &B) noexcept
{
  if !consteval {
#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)
    if constexpr ( micron::same_as<T, f32> && M == 4 && K == 4 && N == 4 ) {
      mat<T, 4, 4> Cm = mat<T, 4, 4>::zero();
      const float *ap = reinterpret_cast<const float *>(A.data);
      const float *bp = reinterpret_cast<const float *>(B.data);
      float *cp = reinterpret_cast<float *>(Cm.data);
      const __m128 b0 = simd::sse::loadu_f32(bp + 0);
      const __m128 b1 = simd::sse::loadu_f32(bp + 4);
      const __m128 b2 = simd::sse::loadu_f32(bp + 8);
      const __m128 b3 = simd::sse::loadu_f32(bp + 12);
      for ( int i = 0; i < 4; ++i ) {
        const __m128 ar = simd::sse::loadu_f32(ap + i * 4);
        __m128 r = simd::sse::mul_f32(simd::sse::shuffle_f32<0x00>(ar, ar), b0);
        r = simd::fma::fma_f32(simd::sse::shuffle_f32<0x55>(ar, ar), b1, r);
        r = simd::fma::fma_f32(simd::sse::shuffle_f32<0xAA>(ar, ar), b2, r);
        r = simd::fma::fma_f32(simd::sse::shuffle_f32<0xFF>(ar, ar), b3, r);
        simd::sse::storeu_f32(cp + i * 4, r);
      }
      return Cm;
    }
#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
    if constexpr ( micron::same_as<T, f32> && M == 4 && K == 4 && N == 4 ) {
      mat<T, 4, 4> Cm = mat<T, 4, 4>::zero();
      const float *ap = reinterpret_cast<const float *>(A.data);
      const float *bp = reinterpret_cast<const float *>(B.data);
      float *cp = reinterpret_cast<float *>(Cm.data);
      const float32x4_t b0 = simd::neon::load_f32(bp + 0);
      const float32x4_t b1 = simd::neon::load_f32(bp + 4);
      const float32x4_t b2 = simd::neon::load_f32(bp + 8);
      const float32x4_t b3 = simd::neon::load_f32(bp + 12);
      for ( int i = 0; i < 4; ++i ) {
        float32x4_t r = simd::neon::mul(simd::neon::splat_f32(ap[i * 4 + 0]), b0);
        r = simd::neon::add(r, simd::neon::mul(simd::neon::splat_f32(ap[i * 4 + 1]), b1));
        r = simd::neon::add(r, simd::neon::mul(simd::neon::splat_f32(ap[i * 4 + 2]), b2));
        r = simd::neon::add(r, simd::neon::mul(simd::neon::splat_f32(ap[i * 4 + 3]), b3));
        simd::neon::store_f32(cp + i * 4, r);
      }
      return Cm;
    }
#endif
  }
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
template<arith_scalar T, usize R, usize C>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, C, R>
transpose(const mat<T, R, C> &m) noexcept
{
  mat<T, C, R> r{};
  for ( usize i = 0; i < R; ++i )
    for ( usize j = 0; j < C; ++j ) r.data[j * R + i] = m.data[i * C + j];
  return r;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
det2(const mat<F, 2, 2> &m) noexcept
{
  return math::fma<F>(m.data[0], m.data[3], -m.data[1] * m.data[2]);
}

template<ieee754_floating F>
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

template<ieee754_floating F>
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

template<ieee754_floating F>
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

template<ieee754_floating F>
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

// mat<F,4,4> to be fully overwritten
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr mat<F, 4, 4>
__mat4_scratch(void) noexcept
{
  if !consteval {
    return mat<F, 4, 4>(micron::__mat_uninit);
  }
  return mat<F, 4, 4>{};
}

template<ieee754_floating F>
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

  // in-place stores keep register pressure low
  mat<F, 4, 4> r = __mat4_scratch<F>();
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

#if defined(__micron_gfx_simd)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 2x2-block Cramer inverse

// u * v
[[nodiscard, gnu::always_inline]] inline simd::f128
__m2mul(simd::f128 u, simd::f128 v) noexcept
{
  return __vsimd::__fma(u, __vsimd::__swz<0, 3, 0, 3>(v), __vsimd::__mul(__vsimd::__swz<1, 0, 3, 2>(u), __vsimd::__swz<2, 1, 2, 1>(v)));
}

// (u#) * v
[[nodiscard, gnu::always_inline]] inline simd::f128
__m2adjmul(simd::f128 u, simd::f128 v) noexcept
{
  return __vsimd::__fms(__vsimd::__swz<3, 3, 0, 0>(u), v, __vsimd::__mul(__vsimd::__swz<1, 1, 2, 2>(u), __vsimd::__swz<2, 3, 0, 1>(v)));
}

// u * (v#)
[[nodiscard, gnu::always_inline]] inline simd::f128
__m2muladj(simd::f128 u, simd::f128 v) noexcept
{
  return __vsimd::__fms(u, __vsimd::__swz<3, 0, 3, 0>(v), __vsimd::__mul(__vsimd::__swz<1, 0, 3, 2>(u), __vsimd::__swz<2, 1, 2, 1>(v)));
}

// M = | A B |    iM = 1/|M| * |  X#  Y# |
//     | C D |                 |  Z#  W# |
[[nodiscard, gnu::always_inline]] inline mat<f32, 4, 4>
__inv4_f32(const mat<f32, 4, 4> &m) noexcept
{
  const float *mp = reinterpret_cast<const float *>(m.data);
  const simd::f128 r0 = __vsimd::__load(mp + 0);
  const simd::f128 r1 = __vsimd::__load(mp + 4);
  const simd::f128 r2 = __vsimd::__load(mp + 8);
  const simd::f128 r3 = __vsimd::__load(mp + 12);

  const simd::f128 A = __vsimd::__shuf2<0, 1, 0, 1>(r0, r1);
  const simd::f128 B = __vsimd::__shuf2<2, 3, 2, 3>(r0, r1);
  const simd::f128 C = __vsimd::__shuf2<0, 1, 0, 1>(r2, r3);
  const simd::f128 D = __vsimd::__shuf2<2, 3, 2, 3>(r2, r3);

  // (|A|, |B|, |C|, |D|)
  const simd::f128 detSub = __vsimd::__sub(__vsimd::__mul(__vsimd::__shuf2<0, 2, 0, 2>(r0, r2), __vsimd::__shuf2<1, 3, 1, 3>(r1, r3)),
                                           __vsimd::__mul(__vsimd::__shuf2<1, 3, 1, 3>(r0, r2), __vsimd::__shuf2<0, 2, 0, 2>(r1, r3)));
  const simd::f128 detA = __vsimd::__swz<0, 0, 0, 0>(detSub);
  const simd::f128 detB = __vsimd::__swz<1, 1, 1, 1>(detSub);
  const simd::f128 detC = __vsimd::__swz<2, 2, 2, 2>(detSub);
  const simd::f128 detD = __vsimd::__swz<3, 3, 3, 3>(detSub);

  const simd::f128 D_C = __m2adjmul(D, C);
  const simd::f128 A_B = __m2adjmul(A, B);
  const simd::f128 X_ = __vsimd::__fms(detD, A, __m2mul(B, D_C));
  const simd::f128 W_ = __vsimd::__fms(detA, D, __m2mul(C, A_B));
  const simd::f128 Y_ = __vsimd::__fms(detB, C, __m2muladj(D, A_B));
  const simd::f128 Z_ = __vsimd::__fms(detC, B, __m2muladj(A, D_C));

  // |M| = |A||D| + |B||C| - tr((A#B)(D#C))
  const simd::f128 tr = __vsimd::__sum_splat(__vsimd::__mul(A_B, __vsimd::__swz<0, 2, 1, 3>(D_C)));
  const simd::f128 detM = __vsimd::__sub(__vsimd::__fma(detB, detC, __vsimd::__mul(detA, detD)), tr);

  // adjugate signs folded into the one exact division
  const simd::f128 rDetM = __vsimd::__recip_signed(__vsimd::__setr(1.0f, -1.0f, -1.0f, 1.0f), detM);
  const simd::f128 X = __vsimd::__mul(X_, rDetM);
  const simd::f128 Y = __vsimd::__mul(Y_, rDetM);
  const simd::f128 Z = __vsimd::__mul(Z_, rDetM);
  const simd::f128 W = __vsimd::__mul(W_, rDetM);

  mat<f32, 4, 4> r(micron::__mat_uninit);
  float *rp = reinterpret_cast<float *>(r.data);
  __vsimd::__store(rp + 0, __vsimd::__shuf2<3, 1, 3, 1>(X, Y));
  __vsimd::__store(rp + 4, __vsimd::__shuf2<2, 0, 2, 0>(X, Y));
  __vsimd::__store(rp + 8, __vsimd::__shuf2<3, 1, 3, 1>(Z, W));
  __vsimd::__store(rp + 12, __vsimd::__shuf2<2, 0, 2, 0>(Z, W));
  return r;
}
#endif

template<ieee754_floating F>
[[nodiscard]] inline constexpr mat<F, 4, 4>
inv4(const mat<F, 4, 4> &m) noexcept
{
  if !consteval {
#if defined(__micron_gfx_simd)
    if constexpr ( micron::same_as<F, f32> ) return __inv4_f32(m);
#endif
  }
  return inv4_adj<F>(m);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline vec<F, 2>
solve2(const mat<F, 2, 2> &A, const vec<F, 2> &b) noexcept
{
  return gemv(inv2(A), b);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline vec<F, 3>
solve3(const mat<F, 3, 3> &A, const vec<F, 3> &b) noexcept
{
  return gemv(inv3(A), b);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline vec<F, 4>
solve4(const mat<F, 4, 4> &A, const vec<F, 4> &b) noexcept
{
  return gemv(inv4(A), b);
}

template<ieee754_floating F>
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

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr quat<F>
conjugate(const quat<F> &q) noexcept
{
  return { -q.data[0], -q.data[1], -q.data[2], q.data[3] };
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
norm(const quat<F> &q) noexcept
{
  return math::fsqrt(
      math::fma<F>(q.data[0], q.data[0], math::fma<F>(q.data[1], q.data[1], math::fma<F>(q.data[2], q.data[2], q.data[3] * q.data[3]))));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline quat<F>
normalize(const quat<F> &q) noexcept
{
  F n = norm(q);
  if ( n == F(0) ) return q;
  F inv = F(1) / n;
  return { q.data[0] * inv, q.data[1] * inv, q.data[2] * inv, q.data[3] * inv };
}

// rotate vector by quaternion
template<ieee754_floating F>
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

};      // namespace ops
};      // namespace linalg
};      // namespace math
};      // namespace micron
