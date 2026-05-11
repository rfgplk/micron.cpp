//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// BLAS level 1 routines, linear time
//
//   swap
//   copy
//   scal
//   axpy       (FMA)
//   axpby      (FMA)
//   dot         (FMA)
//   dot_kahan
//   sdsdot
//   asum     (Kahan for floats)
//   nrm2
//   iamax
//   iamin
//   rotg
//   rot
//   rotmg
//   rotm

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../dispatch.hpp"
#include "../mk.hpp"
#include "../quants/views.hpp"

#if defined(__AVX2__) && defined(__FMA__)
#include "../../simd/arch/types_amd64.hpp"
#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
#if defined(__micron_arch_arm64)
#include "../../simd/arch/types_arm64.hpp"
#else
#include "../../simd/arch/types_arm32.hpp"
#endif
#endif

namespace micron
{
namespace math
{
namespace blas
{
namespace level1
{

using micron::math::quants::vec_view;

namespace __impl_level1
{

template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr T &
ref(T *p, usize i, ssize_t inc) noexcept
{
  return p[ssize_t(i) * inc];
}

#if defined(__AVX2__) && defined(__FMA__)

[[nodiscard, gnu::flatten]] inline f64
dot_packed_f64(const double *__restrict__ x, const double *__restrict__ y, usize n) noexcept
{
  __m256d s0 = _mm256_setzero_pd();
  __m256d s1 = _mm256_setzero_pd();
  __m256d s2 = _mm256_setzero_pd();
  __m256d s3 = _mm256_setzero_pd();
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    s0 = _mm256_fmadd_pd(_mm256_loadu_pd(x + i + 0), _mm256_loadu_pd(y + i + 0), s0);
    s1 = _mm256_fmadd_pd(_mm256_loadu_pd(x + i + 4), _mm256_loadu_pd(y + i + 4), s1);
    s2 = _mm256_fmadd_pd(_mm256_loadu_pd(x + i + 8), _mm256_loadu_pd(y + i + 8), s2);
    s3 = _mm256_fmadd_pd(_mm256_loadu_pd(x + i + 12), _mm256_loadu_pd(y + i + 12), s3);
  }
  // 4-wide tail
  for ( ; i + 4 <= n; i += 4 ) s0 = _mm256_fmadd_pd(_mm256_loadu_pd(x + i), _mm256_loadu_pd(y + i), s0);
  // horizontal reduction across the 4 chains
  __m256d sum = _mm256_add_pd(_mm256_add_pd(s0, s1), _mm256_add_pd(s2, s3));
  __m128d hi = _mm256_extractf128_pd(sum, 1);
  __m128d lo = _mm256_castpd256_pd128(sum);
  __m128d half = _mm_add_pd(hi, lo);
  __m128d hh = _mm_unpackhi_pd(half, half);
  f64 r = _mm_cvtsd_f64(_mm_add_sd(half, hh));
  // 1-wide scalar epilogue
  for ( ; i < n; ++i ) r = math::fma<f64>(x[i], y[i], r);
  return r;
}

[[nodiscard, gnu::flatten]] inline f32
dot_packed_f32(const float *__restrict__ x, const float *__restrict__ y, usize n) noexcept
{
  __m256 s0 = _mm256_setzero_ps();
  __m256 s1 = _mm256_setzero_ps();
  __m256 s2 = _mm256_setzero_ps();
  __m256 s3 = _mm256_setzero_ps();
  usize i = 0;
  for ( ; i + 32 <= n; i += 32 ) {
    s0 = _mm256_fmadd_ps(_mm256_loadu_ps(x + i + 0), _mm256_loadu_ps(y + i + 0), s0);
    s1 = _mm256_fmadd_ps(_mm256_loadu_ps(x + i + 8), _mm256_loadu_ps(y + i + 8), s1);
    s2 = _mm256_fmadd_ps(_mm256_loadu_ps(x + i + 16), _mm256_loadu_ps(y + i + 16), s2);
    s3 = _mm256_fmadd_ps(_mm256_loadu_ps(x + i + 24), _mm256_loadu_ps(y + i + 24), s3);
  }
  for ( ; i + 8 <= n; i += 8 ) s0 = _mm256_fmadd_ps(_mm256_loadu_ps(x + i), _mm256_loadu_ps(y + i), s0);
  __m256 sum = _mm256_add_ps(_mm256_add_ps(s0, s1), _mm256_add_ps(s2, s3));
  __m128 hi = _mm256_extractf128_ps(sum, 1);
  __m128 lo = _mm256_castps256_ps128(sum);
  __m128 q4 = _mm_add_ps(hi, lo);
  __m128 q2 = _mm_add_ps(q4, _mm_movehl_ps(q4, q4));
  __m128 q1 = _mm_add_ss(q2, _mm_shuffle_ps(q2, q2, 0x1));
  f32 r = _mm_cvtss_f32(q1);
  for ( ; i < n; ++i ) r = math::fma<f32>(x[i], y[i], r);
  return r;
}

[[gnu::flatten]] inline void
axpy_packed_f64(double alpha, const double *__restrict__ x, double *__restrict__ y, usize n) noexcept
{
  const __m256d va = _mm256_set1_pd(alpha);
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    __m256d y0 = _mm256_loadu_pd(y + i + 0);
    __m256d y1 = _mm256_loadu_pd(y + i + 4);
    __m256d y2 = _mm256_loadu_pd(y + i + 8);
    __m256d y3 = _mm256_loadu_pd(y + i + 12);
    y0 = _mm256_fmadd_pd(va, _mm256_loadu_pd(x + i + 0), y0);
    y1 = _mm256_fmadd_pd(va, _mm256_loadu_pd(x + i + 4), y1);
    y2 = _mm256_fmadd_pd(va, _mm256_loadu_pd(x + i + 8), y2);
    y3 = _mm256_fmadd_pd(va, _mm256_loadu_pd(x + i + 12), y3);
    _mm256_storeu_pd(y + i + 0, y0);
    _mm256_storeu_pd(y + i + 4, y1);
    _mm256_storeu_pd(y + i + 8, y2);
    _mm256_storeu_pd(y + i + 12, y3);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    __m256d yk = _mm256_fmadd_pd(va, _mm256_loadu_pd(x + i), _mm256_loadu_pd(y + i));
    _mm256_storeu_pd(y + i, yk);
  }
  for ( ; i < n; ++i ) y[i] = math::fma<f64>(alpha, x[i], y[i]);
}

[[gnu::flatten]] inline void
axpy_packed_f32(float alpha, const float *__restrict__ x, float *__restrict__ y, usize n) noexcept
{
  const __m256 va = _mm256_set1_ps(alpha);
  usize i = 0;
  for ( ; i + 32 <= n; i += 32 ) {
    __m256 y0 = _mm256_fmadd_ps(va, _mm256_loadu_ps(x + i + 0), _mm256_loadu_ps(y + i + 0));
    __m256 y1 = _mm256_fmadd_ps(va, _mm256_loadu_ps(x + i + 8), _mm256_loadu_ps(y + i + 8));
    __m256 y2 = _mm256_fmadd_ps(va, _mm256_loadu_ps(x + i + 16), _mm256_loadu_ps(y + i + 16));
    __m256 y3 = _mm256_fmadd_ps(va, _mm256_loadu_ps(x + i + 24), _mm256_loadu_ps(y + i + 24));
    _mm256_storeu_ps(y + i + 0, y0);
    _mm256_storeu_ps(y + i + 8, y1);
    _mm256_storeu_ps(y + i + 16, y2);
    _mm256_storeu_ps(y + i + 24, y3);
  }
  for ( ; i + 8 <= n; i += 8 ) {
    __m256 yk = _mm256_fmadd_ps(va, _mm256_loadu_ps(x + i), _mm256_loadu_ps(y + i));
    _mm256_storeu_ps(y + i, yk);
  }
  for ( ; i < n; ++i ) y[i] = math::fma<f32>(alpha, x[i], y[i]);
}

[[nodiscard, gnu::flatten]] inline f64
asum_packed_f64(const double *__restrict__ x, usize n) noexcept
{
  // bit-mask 0x7FFFFFFFFFFFFFFF clears the sign bit (gives |x|).
  const __m256d sign_mask = _mm256_castsi256_pd(_mm256_set1_epi64x(0x7FFFFFFFFFFFFFFFLL));
  __m256d s0 = _mm256_setzero_pd();
  __m256d s1 = _mm256_setzero_pd();
  __m256d s2 = _mm256_setzero_pd();
  __m256d s3 = _mm256_setzero_pd();
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    s0 = _mm256_add_pd(s0, _mm256_and_pd(_mm256_loadu_pd(x + i + 0), sign_mask));
    s1 = _mm256_add_pd(s1, _mm256_and_pd(_mm256_loadu_pd(x + i + 4), sign_mask));
    s2 = _mm256_add_pd(s2, _mm256_and_pd(_mm256_loadu_pd(x + i + 8), sign_mask));
    s3 = _mm256_add_pd(s3, _mm256_and_pd(_mm256_loadu_pd(x + i + 12), sign_mask));
  }
  for ( ; i + 4 <= n; i += 4 ) s0 = _mm256_add_pd(s0, _mm256_and_pd(_mm256_loadu_pd(x + i), sign_mask));
  __m256d sum = _mm256_add_pd(_mm256_add_pd(s0, s1), _mm256_add_pd(s2, s3));
  __m128d hi = _mm256_extractf128_pd(sum, 1);
  __m128d lo = _mm256_castpd256_pd128(sum);
  __m128d half = _mm_add_pd(hi, lo);
  __m128d hh = _mm_unpackhi_pd(half, half);
  f64 r = _mm_cvtsd_f64(_mm_add_sd(half, hh));
  for ( ; i < n; ++i ) r += mk::manip::fabs<f64>(x[i]);
  return r;
}

[[nodiscard, gnu::flatten]] inline f32
asum_packed_f32(const float *__restrict__ x, usize n) noexcept
{
  const __m256 sign_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
  __m256 s0 = _mm256_setzero_ps();
  __m256 s1 = _mm256_setzero_ps();
  __m256 s2 = _mm256_setzero_ps();
  __m256 s3 = _mm256_setzero_ps();
  usize i = 0;
  for ( ; i + 32 <= n; i += 32 ) {
    s0 = _mm256_add_ps(s0, _mm256_and_ps(_mm256_loadu_ps(x + i + 0), sign_mask));
    s1 = _mm256_add_ps(s1, _mm256_and_ps(_mm256_loadu_ps(x + i + 8), sign_mask));
    s2 = _mm256_add_ps(s2, _mm256_and_ps(_mm256_loadu_ps(x + i + 16), sign_mask));
    s3 = _mm256_add_ps(s3, _mm256_and_ps(_mm256_loadu_ps(x + i + 24), sign_mask));
  }
  for ( ; i + 8 <= n; i += 8 ) s0 = _mm256_add_ps(s0, _mm256_and_ps(_mm256_loadu_ps(x + i), sign_mask));
  __m256 sum = _mm256_add_ps(_mm256_add_ps(s0, s1), _mm256_add_ps(s2, s3));
  __m128 hi = _mm256_extractf128_ps(sum, 1);
  __m128 lo = _mm256_castps256_ps128(sum);
  __m128 q4 = _mm_add_ps(hi, lo);
  __m128 q2 = _mm_add_ps(q4, _mm_movehl_ps(q4, q4));
  __m128 q1 = _mm_add_ss(q2, _mm_shuffle_ps(q2, q2, 0x1));
  f32 r = _mm_cvtss_f32(q1);
  for ( ; i < n; ++i ) r += mk::manip::fabs<f32>(x[i]);
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scal
// 4-chain unroll: 16 doubles / 32 floats per outer iteration

[[gnu::flatten]] inline void
scal_packed_f64(double alpha, double *__restrict__ x, usize n) noexcept
{
  const __m256d va = _mm256_set1_pd(alpha);
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    __m256d x0 = _mm256_loadu_pd(x + i + 0);
    __m256d x1 = _mm256_loadu_pd(x + i + 4);
    __m256d x2 = _mm256_loadu_pd(x + i + 8);
    __m256d x3 = _mm256_loadu_pd(x + i + 12);
    _mm256_storeu_pd(x + i + 0, _mm256_mul_pd(x0, va));
    _mm256_storeu_pd(x + i + 4, _mm256_mul_pd(x1, va));
    _mm256_storeu_pd(x + i + 8, _mm256_mul_pd(x2, va));
    _mm256_storeu_pd(x + i + 12, _mm256_mul_pd(x3, va));
  }
  for ( ; i + 4 <= n; i += 4 ) {
    _mm256_storeu_pd(x + i, _mm256_mul_pd(_mm256_loadu_pd(x + i), va));
  }
  for ( ; i < n; ++i ) x[i] = x[i] * alpha;
}

[[gnu::flatten]] inline void
scal_packed_f32(float alpha, float *__restrict__ x, usize n) noexcept
{
  const __m256 va = _mm256_set1_ps(alpha);
  usize i = 0;
  for ( ; i + 32 <= n; i += 32 ) {
    __m256 x0 = _mm256_loadu_ps(x + i + 0);
    __m256 x1 = _mm256_loadu_ps(x + i + 8);
    __m256 x2 = _mm256_loadu_ps(x + i + 16);
    __m256 x3 = _mm256_loadu_ps(x + i + 24);
    _mm256_storeu_ps(x + i + 0, _mm256_mul_ps(x0, va));
    _mm256_storeu_ps(x + i + 8, _mm256_mul_ps(x1, va));
    _mm256_storeu_ps(x + i + 16, _mm256_mul_ps(x2, va));
    _mm256_storeu_ps(x + i + 24, _mm256_mul_ps(x3, va));
  }
  for ( ; i + 8 <= n; i += 8 ) {
    _mm256_storeu_ps(x + i, _mm256_mul_ps(_mm256_loadu_ps(x + i), va));
  }
  for ( ; i < n; ++i ) x[i] = x[i] * alpha;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// nrm2_fast: sum-of-squares with a single sqrt at the end
// WARNING: NOT overflow safe
[[nodiscard, gnu::flatten]] inline f64
nrm2_fast_packed_f64(const double *__restrict__ x, usize n) noexcept
{
  __m256d s0 = _mm256_setzero_pd();
  __m256d s1 = _mm256_setzero_pd();
  __m256d s2 = _mm256_setzero_pd();
  __m256d s3 = _mm256_setzero_pd();
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    __m256d x0 = _mm256_loadu_pd(x + i + 0);
    __m256d x1 = _mm256_loadu_pd(x + i + 4);
    __m256d x2 = _mm256_loadu_pd(x + i + 8);
    __m256d x3 = _mm256_loadu_pd(x + i + 12);
    s0 = _mm256_fmadd_pd(x0, x0, s0);
    s1 = _mm256_fmadd_pd(x1, x1, s1);
    s2 = _mm256_fmadd_pd(x2, x2, s2);
    s3 = _mm256_fmadd_pd(x3, x3, s3);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    __m256d xv = _mm256_loadu_pd(x + i);
    s0 = _mm256_fmadd_pd(xv, xv, s0);
  }
  __m256d sum = _mm256_add_pd(_mm256_add_pd(s0, s1), _mm256_add_pd(s2, s3));
  __m128d hi = _mm256_extractf128_pd(sum, 1);
  __m128d lo = _mm256_castpd256_pd128(sum);
  __m128d half = _mm_add_pd(hi, lo);
  __m128d hh = _mm_unpackhi_pd(half, half);
  f64 r = _mm_cvtsd_f64(_mm_add_sd(half, hh));
  for ( ; i < n; ++i ) r = math::fma<f64>(x[i], x[i], r);
  return r;
}

[[nodiscard, gnu::flatten]] inline f32
nrm2_fast_packed_f32(const float *__restrict__ x, usize n) noexcept
{
  __m256 s0 = _mm256_setzero_ps();
  __m256 s1 = _mm256_setzero_ps();
  __m256 s2 = _mm256_setzero_ps();
  __m256 s3 = _mm256_setzero_ps();
  usize i = 0;
  for ( ; i + 32 <= n; i += 32 ) {
    __m256 x0 = _mm256_loadu_ps(x + i + 0);
    __m256 x1 = _mm256_loadu_ps(x + i + 8);
    __m256 x2 = _mm256_loadu_ps(x + i + 16);
    __m256 x3 = _mm256_loadu_ps(x + i + 24);
    s0 = _mm256_fmadd_ps(x0, x0, s0);
    s1 = _mm256_fmadd_ps(x1, x1, s1);
    s2 = _mm256_fmadd_ps(x2, x2, s2);
    s3 = _mm256_fmadd_ps(x3, x3, s3);
  }
  for ( ; i + 8 <= n; i += 8 ) {
    __m256 xv = _mm256_loadu_ps(x + i);
    s0 = _mm256_fmadd_ps(xv, xv, s0);
  }
  __m256 sum = _mm256_add_ps(_mm256_add_ps(s0, s1), _mm256_add_ps(s2, s3));
  __m128 hi = _mm256_extractf128_ps(sum, 1);
  __m128 lo = _mm256_castps256_ps128(sum);
  __m128 q4 = _mm_add_ps(hi, lo);
  __m128 q2 = _mm_add_ps(q4, _mm_movehl_ps(q4, q4));
  __m128 q1 = _mm_add_ss(q2, _mm_shuffle_ps(q2, q2, 0x1));
  f32 r = _mm_cvtss_f32(q1);
  for ( ; i < n; ++i ) r = math::fma<f32>(x[i], x[i], r);
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// iamax
[[nodiscard, gnu::flatten]] inline usize
iamax_packed_f64(const double *__restrict__ x, usize n) noexcept
{
  if ( n == 0 ) return 0;
  if ( n < 8 ) {
    double mx = mk::manip::fabs<f64>(x[0]);
    usize idx = 0;
    for ( usize k = 1; k < n; ++k ) {
      const double a = mk::manip::fabs<f64>(x[k]);
      if ( a > mx ) { mx = a; idx = k; }
    }
    return idx;
  }
  // 2-chain unroll
  const __m256d abs_mask = _mm256_castsi256_pd(_mm256_set1_epi64x(0x7FFFFFFFFFFFFFFFLL));
  __m256d vmax0 = _mm256_and_pd(_mm256_loadu_pd(x + 0), abs_mask);
  __m256d vmax1 = _mm256_and_pd(_mm256_loadu_pd(x + 4), abs_mask);
  __m256i vidx0 = _mm256_setr_epi64x(0, 1, 2, 3);
  __m256i vidx1 = _mm256_setr_epi64x(4, 5, 6, 7);
  __m256i vcand0 = _mm256_setr_epi64x(8, 9, 10, 11);
  __m256i vcand1 = _mm256_setr_epi64x(12, 13, 14, 15);
  const __m256i step8 = _mm256_set1_epi64x(8);
  const __m256i step4 = _mm256_set1_epi64x(4);
  usize i = 8;
  for ( ; i + 8 <= n; i += 8 ) {
    __m256d v0 = _mm256_and_pd(_mm256_loadu_pd(x + i + 0), abs_mask);
    __m256d v1 = _mm256_and_pd(_mm256_loadu_pd(x + i + 4), abs_mask);
    __m256d gt0 = _mm256_cmp_pd(v0, vmax0, _CMP_GT_OQ);
    __m256d gt1 = _mm256_cmp_pd(v1, vmax1, _CMP_GT_OQ);
    vmax0 = _mm256_blendv_pd(vmax0, v0, gt0);
    vmax1 = _mm256_blendv_pd(vmax1, v1, gt1);
    vidx0 = _mm256_blendv_epi8(vidx0, vcand0, _mm256_castpd_si256(gt0));
    vidx1 = _mm256_blendv_epi8(vidx1, vcand1, _mm256_castpd_si256(gt1));
    vcand0 = _mm256_add_epi64(vcand0, step8);
    vcand1 = _mm256_add_epi64(vcand1, step8);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    __m256d v = _mm256_and_pd(_mm256_loadu_pd(x + i), abs_mask);
    __m256d gt = _mm256_cmp_pd(v, vmax0, _CMP_GT_OQ);
    vmax0 = _mm256_blendv_pd(vmax0, v, gt);
    vidx0 = _mm256_blendv_epi8(vidx0, vcand0, _mm256_castpd_si256(gt));
    vcand0 = _mm256_add_epi64(vcand0, step4);
  }
  alignas(32) double lanes_v0[4], lanes_v1[4];
  alignas(32) i64 lanes_i0[4], lanes_i1[4];
  _mm256_store_pd(lanes_v0, vmax0);
  _mm256_store_pd(lanes_v1, vmax1);
  _mm256_store_si256(reinterpret_cast<__m256i *>(lanes_i0), vidx0);
  _mm256_store_si256(reinterpret_cast<__m256i *>(lanes_i1), vidx1);
  double best = lanes_v0[0];
  usize best_idx = usize(lanes_i0[0]);
  for ( int k = 1; k < 4; ++k ) {
    const double a = lanes_v0[k];
    const usize j = usize(lanes_i0[k]);
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( int k = 0; k < 4; ++k ) {
    const double a = lanes_v1[k];
    const usize j = usize(lanes_i1[k]);
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( ; i < n; ++i ) {
    const double a = mk::manip::fabs<f64>(x[i]);
    if ( a > best ) { best = a; best_idx = i; }
  }
  return best_idx;
}

[[nodiscard, gnu::flatten]] inline usize
iamax_packed_f32(const float *__restrict__ x, usize n) noexcept
{
  if ( n == 0 ) return 0;
  if ( n < 16 ) {
    float mx = mk::manip::fabs<f32>(x[0]);
    usize idx = 0;
    for ( usize k = 1; k < n; ++k ) {
      const float a = mk::manip::fabs<f32>(x[k]);
      if ( a > mx ) { mx = a; idx = k; }
    }
    return idx;
  }
  // 2-chain unroll: 16 floats per outer iter, two independent dep chains
  const __m256 abs_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
  __m256 vmax0 = _mm256_and_ps(_mm256_loadu_ps(x + 0), abs_mask);
  __m256 vmax1 = _mm256_and_ps(_mm256_loadu_ps(x + 8), abs_mask);
  __m256i vidx0 = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
  __m256i vidx1 = _mm256_setr_epi32(8, 9, 10, 11, 12, 13, 14, 15);
  __m256i vcand0 = _mm256_setr_epi32(16, 17, 18, 19, 20, 21, 22, 23);
  __m256i vcand1 = _mm256_setr_epi32(24, 25, 26, 27, 28, 29, 30, 31);
  const __m256i step16 = _mm256_set1_epi32(16);
  const __m256i step8 = _mm256_set1_epi32(8);
  usize i = 16;
  for ( ; i + 16 <= n; i += 16 ) {
    __m256 v0 = _mm256_and_ps(_mm256_loadu_ps(x + i + 0), abs_mask);
    __m256 v1 = _mm256_and_ps(_mm256_loadu_ps(x + i + 8), abs_mask);
    __m256 gt0 = _mm256_cmp_ps(v0, vmax0, _CMP_GT_OQ);
    __m256 gt1 = _mm256_cmp_ps(v1, vmax1, _CMP_GT_OQ);
    vmax0 = _mm256_blendv_ps(vmax0, v0, gt0);
    vmax1 = _mm256_blendv_ps(vmax1, v1, gt1);
    vidx0 = _mm256_blendv_epi8(vidx0, vcand0, _mm256_castps_si256(gt0));
    vidx1 = _mm256_blendv_epi8(vidx1, vcand1, _mm256_castps_si256(gt1));
    vcand0 = _mm256_add_epi32(vcand0, step16);
    vcand1 = _mm256_add_epi32(vcand1, step16);
  }
  for ( ; i + 8 <= n; i += 8 ) {
    __m256 v = _mm256_and_ps(_mm256_loadu_ps(x + i), abs_mask);
    __m256 gt = _mm256_cmp_ps(v, vmax0, _CMP_GT_OQ);
    vmax0 = _mm256_blendv_ps(vmax0, v, gt);
    vidx0 = _mm256_blendv_epi8(vidx0, vcand0, _mm256_castps_si256(gt));
    vcand0 = _mm256_add_epi32(vcand0, step8);
  }
  alignas(32) float lanes_v0[8], lanes_v1[8];
  alignas(32) i32 lanes_i0[8], lanes_i1[8];
  _mm256_store_ps(lanes_v0, vmax0);
  _mm256_store_ps(lanes_v1, vmax1);
  _mm256_store_si256(reinterpret_cast<__m256i *>(lanes_i0), vidx0);
  _mm256_store_si256(reinterpret_cast<__m256i *>(lanes_i1), vidx1);
  float best = lanes_v0[0];
  usize best_idx = usize(u32(lanes_i0[0]));
  for ( int k = 1; k < 8; ++k ) {
    const float a = lanes_v0[k];
    const usize j = usize(u32(lanes_i0[k]));
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( int k = 0; k < 8; ++k ) {
    const float a = lanes_v1[k];
    const usize j = usize(u32(lanes_i1[k]));
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( ; i < n; ++i ) {
    const float a = mk::manip::fabs<f32>(x[i]);
    if ( a > best ) { best = a; best_idx = i; }
  }
  return best_idx;
}

#endif     // AVX2 + FMA

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// NEON scal
[[gnu::flatten]] inline void
scal_packed_f32_neon(float alpha, float *__restrict__ x, usize n) noexcept
{
  const float32x4_t va = vdupq_n_f32(alpha);
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    float32x4_t x0 = vld1q_f32(x + i + 0);
    float32x4_t x1 = vld1q_f32(x + i + 4);
    float32x4_t x2 = vld1q_f32(x + i + 8);
    float32x4_t x3 = vld1q_f32(x + i + 12);
    vst1q_f32(x + i + 0, vmulq_f32(x0, va));
    vst1q_f32(x + i + 4, vmulq_f32(x1, va));
    vst1q_f32(x + i + 8, vmulq_f32(x2, va));
    vst1q_f32(x + i + 12, vmulq_f32(x3, va));
  }
  for ( ; i + 4 <= n; i += 4 ) {
    vst1q_f32(x + i, vmulq_f32(vld1q_f32(x + i), va));
  }
  for ( ; i < n; ++i ) x[i] = x[i] * alpha;
}

#if defined(__micron_arch_arm64)
[[gnu::flatten]] inline void
scal_packed_f64_neon(double alpha, double *__restrict__ x, usize n) noexcept
{
  const float64x2_t va = vdupq_n_f64(alpha);
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    float64x2_t x0 = vld1q_f64(x + i + 0);
    float64x2_t x1 = vld1q_f64(x + i + 2);
    float64x2_t x2 = vld1q_f64(x + i + 4);
    float64x2_t x3 = vld1q_f64(x + i + 6);
    vst1q_f64(x + i + 0, vmulq_f64(x0, va));
    vst1q_f64(x + i + 2, vmulq_f64(x1, va));
    vst1q_f64(x + i + 4, vmulq_f64(x2, va));
    vst1q_f64(x + i + 6, vmulq_f64(x3, va));
  }
  for ( ; i + 2 <= n; i += 2 ) {
    vst1q_f64(x + i, vmulq_f64(vld1q_f64(x + i), va));
  }
  for ( ; i < n; ++i ) x[i] = x[i] * alpha;
}
#endif     // arm64

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// NEON nrm2_fast

[[nodiscard, gnu::flatten]] inline f32
nrm2_fast_packed_f32_neon(const float *__restrict__ x, usize n) noexcept
{
  float32x4_t s0 = vdupq_n_f32(0.0f);
  float32x4_t s1 = vdupq_n_f32(0.0f);
  float32x4_t s2 = vdupq_n_f32(0.0f);
  float32x4_t s3 = vdupq_n_f32(0.0f);
  usize i = 0;
  for ( ; i + 16 <= n; i += 16 ) {
    float32x4_t a = vld1q_f32(x + i + 0);
    float32x4_t b = vld1q_f32(x + i + 4);
    float32x4_t c = vld1q_f32(x + i + 8);
    float32x4_t d = vld1q_f32(x + i + 12);
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
    s0 = vfmaq_f32(s0, a, a);
    s1 = vfmaq_f32(s1, b, b);
    s2 = vfmaq_f32(s2, c, c);
    s3 = vfmaq_f32(s3, d, d);
#else
    s0 = vmlaq_f32(s0, a, a);
    s1 = vmlaq_f32(s1, b, b);
    s2 = vmlaq_f32(s2, c, c);
    s3 = vmlaq_f32(s3, d, d);
#endif
  }
  for ( ; i + 4 <= n; i += 4 ) {
    float32x4_t a = vld1q_f32(x + i);
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
    s0 = vfmaq_f32(s0, a, a);
#else
    s0 = vmlaq_f32(s0, a, a);
#endif
  }
  float32x4_t s = vaddq_f32(vaddq_f32(s0, s1), vaddq_f32(s2, s3));
  f32 r = vgetq_lane_f32(s, 0) + vgetq_lane_f32(s, 1) + vgetq_lane_f32(s, 2) + vgetq_lane_f32(s, 3);
  for ( ; i < n; ++i ) r = math::fma<f32>(x[i], x[i], r);
  return r;
}

#if defined(__micron_arch_arm64)
[[nodiscard, gnu::flatten]] inline f64
nrm2_fast_packed_f64_neon(const double *__restrict__ x, usize n) noexcept
{
  float64x2_t s0 = vdupq_n_f64(0.0);
  float64x2_t s1 = vdupq_n_f64(0.0);
  float64x2_t s2 = vdupq_n_f64(0.0);
  float64x2_t s3 = vdupq_n_f64(0.0);
  usize i = 0;
  for ( ; i + 8 <= n; i += 8 ) {
    float64x2_t a = vld1q_f64(x + i + 0);
    float64x2_t b = vld1q_f64(x + i + 2);
    float64x2_t c = vld1q_f64(x + i + 4);
    float64x2_t d = vld1q_f64(x + i + 6);
    s0 = vfmaq_f64(s0, a, a);
    s1 = vfmaq_f64(s1, b, b);
    s2 = vfmaq_f64(s2, c, c);
    s3 = vfmaq_f64(s3, d, d);
  }
  for ( ; i + 2 <= n; i += 2 ) {
    float64x2_t a = vld1q_f64(x + i);
    s0 = vfmaq_f64(s0, a, a);
  }
  float64x2_t s = vaddq_f64(vaddq_f64(s0, s1), vaddq_f64(s2, s3));
  f64 r = vgetq_lane_f64(s, 0) + vgetq_lane_f64(s, 1);
  for ( ; i < n; ++i ) r = math::fma<f64>(x[i], x[i], r);
  return r;
}
#endif     // arm64

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// NEON iamax
[[nodiscard, gnu::flatten]] inline usize
iamax_packed_f32_neon(const float *__restrict__ x, usize n) noexcept
{
  if ( n == 0 ) return 0;
  if ( n < 8 ) {
    float mx = mk::manip::fabs<f32>(x[0]);
    usize idx = 0;
    for ( usize k = 1; k < n; ++k ) {
      const float a = mk::manip::fabs<f32>(x[k]);
      if ( a > mx ) { mx = a; idx = k; }
    }
    return idx;
  }
  // 2 independent chains, 8 floats per outer iter.
  float32x4_t vmax0 = vabsq_f32(vld1q_f32(x + 0));
  float32x4_t vmax1 = vabsq_f32(vld1q_f32(x + 4));
  uint32x4_t vidx0 = (uint32x4_t){ 0u, 1u, 2u, 3u };
  uint32x4_t vidx1 = (uint32x4_t){ 4u, 5u, 6u, 7u };
  uint32x4_t vcand0 = (uint32x4_t){ 8u, 9u, 10u, 11u };
  uint32x4_t vcand1 = (uint32x4_t){ 12u, 13u, 14u, 15u };
  const uint32x4_t step8 = vdupq_n_u32(8);
  const uint32x4_t step4 = vdupq_n_u32(4);
  usize i = 8;
  for ( ; i + 8 <= n; i += 8 ) {
    float32x4_t v0 = vabsq_f32(vld1q_f32(x + i + 0));
    float32x4_t v1 = vabsq_f32(vld1q_f32(x + i + 4));
    uint32x4_t gt0 = vcgtq_f32(v0, vmax0);
    uint32x4_t gt1 = vcgtq_f32(v1, vmax1);
    vmax0 = vbslq_f32(gt0, v0, vmax0);
    vmax1 = vbslq_f32(gt1, v1, vmax1);
    vidx0 = vbslq_u32(gt0, vcand0, vidx0);
    vidx1 = vbslq_u32(gt1, vcand1, vidx1);
    vcand0 = vaddq_u32(vcand0, step8);
    vcand1 = vaddq_u32(vcand1, step8);
  }
  for ( ; i + 4 <= n; i += 4 ) {
    float32x4_t v = vabsq_f32(vld1q_f32(x + i));
    uint32x4_t gt = vcgtq_f32(v, vmax0);
    vmax0 = vbslq_f32(gt, v, vmax0);
    vidx0 = vbslq_u32(gt, vcand0, vidx0);
    vcand0 = vaddq_u32(vcand0, step4);
  }
  alignas(16) float lanes_v0[4], lanes_v1[4];
  alignas(16) u32 lanes_i0[4], lanes_i1[4];
  vst1q_f32(lanes_v0, vmax0);
  vst1q_f32(lanes_v1, vmax1);
  vst1q_u32(lanes_i0, vidx0);
  vst1q_u32(lanes_i1, vidx1);
  float best = lanes_v0[0];
  usize best_idx = usize(lanes_i0[0]);
  for ( int k = 1; k < 4; ++k ) {
    const float a = lanes_v0[k];
    const usize j = usize(lanes_i0[k]);
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( int k = 0; k < 4; ++k ) {
    const float a = lanes_v1[k];
    const usize j = usize(lanes_i1[k]);
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( ; i < n; ++i ) {
    const float a = mk::manip::fabs<f32>(x[i]);
    if ( a > best ) { best = a; best_idx = i; }
  }
  return best_idx;
}

#if defined(__micron_arch_arm64)
[[nodiscard, gnu::flatten]] inline usize
iamax_packed_f64_neon(const double *__restrict__ x, usize n) noexcept
{
  if ( n == 0 ) return 0;
  if ( n < 4 ) {
    double mx = mk::manip::fabs<f64>(x[0]);
    usize idx = 0;
    for ( usize k = 1; k < n; ++k ) {
      const double a = mk::manip::fabs<f64>(x[k]);
      if ( a > mx ) { mx = a; idx = k; }
    }
    return idx;
  }
  // 2 independent chains, 4 doubles per outer iter.
  float64x2_t vmax0 = vabsq_f64(vld1q_f64(x + 0));
  float64x2_t vmax1 = vabsq_f64(vld1q_f64(x + 2));
  uint64x2_t vidx0 = (uint64x2_t){ 0ULL, 1ULL };
  uint64x2_t vidx1 = (uint64x2_t){ 2ULL, 3ULL };
  uint64x2_t vcand0 = (uint64x2_t){ 4ULL, 5ULL };
  uint64x2_t vcand1 = (uint64x2_t){ 6ULL, 7ULL };
  const uint64x2_t step4 = vdupq_n_u64(4);
  const uint64x2_t step2 = vdupq_n_u64(2);
  usize i = 4;
  for ( ; i + 4 <= n; i += 4 ) {
    float64x2_t v0 = vabsq_f64(vld1q_f64(x + i + 0));
    float64x2_t v1 = vabsq_f64(vld1q_f64(x + i + 2));
    uint64x2_t gt0 = vcgtq_f64(v0, vmax0);
    uint64x2_t gt1 = vcgtq_f64(v1, vmax1);
    vmax0 = vbslq_f64(gt0, v0, vmax0);
    vmax1 = vbslq_f64(gt1, v1, vmax1);
    vidx0 = vbslq_u64(gt0, vcand0, vidx0);
    vidx1 = vbslq_u64(gt1, vcand1, vidx1);
    vcand0 = vaddq_u64(vcand0, step4);
    vcand1 = vaddq_u64(vcand1, step4);
  }
  for ( ; i + 2 <= n; i += 2 ) {
    float64x2_t v = vabsq_f64(vld1q_f64(x + i));
    uint64x2_t gt = vcgtq_f64(v, vmax0);
    vmax0 = vbslq_f64(gt, v, vmax0);
    vidx0 = vbslq_u64(gt, vcand0, vidx0);
    vcand0 = vaddq_u64(vcand0, step2);
  }
  alignas(16) double lanes_v0[2], lanes_v1[2];
  alignas(16) u64 lanes_i0[2], lanes_i1[2];
  vst1q_f64(lanes_v0, vmax0);
  vst1q_f64(lanes_v1, vmax1);
  vst1q_u64(lanes_i0, vidx0);
  vst1q_u64(lanes_i1, vidx1);
  double best = lanes_v0[0];
  usize best_idx = usize(lanes_i0[0]);
  {
    const double a = lanes_v0[1];
    const usize j = usize(lanes_i0[1]);
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( int k = 0; k < 2; ++k ) {
    const double a = lanes_v1[k];
    const usize j = usize(lanes_i1[k]);
    if ( a > best || (a == best && j < best_idx) ) { best = a; best_idx = j; }
  }
  for ( ; i < n; ++i ) {
    const double a = mk::manip::fabs<f64>(x[i]);
    if ( a > best ) { best = a; best_idx = i; }
  }
  return best_idx;
}
#endif     // arm64

#endif     // NEON

};     // namespace __impl_level1

template <typename T>
[[gnu::flatten]] inline constexpr void
axpy(T alpha, const T *__restrict__ x, const T *__restrict__ x_end, T *__restrict__ y) noexcept
{
  const usize n = usize(x_end - x);
#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( ieee754_floating<T> ) {
      if constexpr ( sizeof(T) == 8 ) {
        __impl_level1::axpy_packed_f64(double(alpha), reinterpret_cast<const double *>(x), reinterpret_cast<double *>(y), n);
        return;
      } else if constexpr ( sizeof(T) == 4 ) {
        __impl_level1::axpy_packed_f32(float(alpha), reinterpret_cast<const float *>(x), reinterpret_cast<float *>(y), n);
        return;
      }
    }
  }
#endif
  usize i = 0;
  if constexpr ( ieee754_floating<T> ) {
    for ( ; i + 4 <= n; i += 4 ) {
      y[i + 0] = math::fma<T>(alpha, x[i + 0], y[i + 0]);
      y[i + 1] = math::fma<T>(alpha, x[i + 1], y[i + 1]);
      y[i + 2] = math::fma<T>(alpha, x[i + 2], y[i + 2]);
      y[i + 3] = math::fma<T>(alpha, x[i + 3], y[i + 3]);
    }
    for ( ; i < n; ++i ) y[i] = math::fma<T>(alpha, x[i], y[i]);
  } else {
    for ( ; i + 4 <= n; i += 4 ) {
      y[i + 0] = y[i + 0] + alpha * x[i + 0];
      y[i + 1] = y[i + 1] + alpha * x[i + 1];
      y[i + 2] = y[i + 2] + alpha * x[i + 2];
      y[i + 3] = y[i + 3] + alpha * x[i + 3];
    }
    for ( ; i < n; ++i ) y[i] = y[i] + alpha * x[i];
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
axpy(usize n, T alpha, const T *x, ssize_t incx, T *y, ssize_t incy) noexcept
{
  if ( incx == 1 && incy == 1 ) {
    axpy<T>(alpha, x, x + n, y);
    return;
  }
  if constexpr ( ieee754_floating<T> ) {
    for ( usize i = 0; i < n; ++i )
      __impl_level1::ref(y, i, incy) = math::fma<T>(alpha, __impl_level1::ref(x, i, incx), __impl_level1::ref(y, i, incy));
  } else {
    for ( usize i = 0; i < n; ++i )
      __impl_level1::ref(y, i, incy) = __impl_level1::ref(y, i, incy) + alpha * __impl_level1::ref(x, i, incx);
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
axpy(T alpha, const vec_view<T> &x, vec_view<T> y) noexcept
{
  axpy<T>(x.n, alpha, x.data, x.inc, y.data, y.inc);
}

template <typename T>
[[gnu::flatten]] inline constexpr void
axpby(T alpha, const T *__restrict__ x, const T *__restrict__ x_end, T beta, T *__restrict__ y) noexcept
{
  const usize n = usize(x_end - x);
  usize i = 0;
  if constexpr ( ieee754_floating<T> ) {
    for ( ; i + 4 <= n; i += 4 ) {
      y[i + 0] = math::fma<T>(alpha, x[i + 0], beta * y[i + 0]);
      y[i + 1] = math::fma<T>(alpha, x[i + 1], beta * y[i + 1]);
      y[i + 2] = math::fma<T>(alpha, x[i + 2], beta * y[i + 2]);
      y[i + 3] = math::fma<T>(alpha, x[i + 3], beta * y[i + 3]);
    }
    for ( ; i < n; ++i ) y[i] = math::fma<T>(alpha, x[i], beta * y[i]);
  } else {
    for ( ; i + 4 <= n; i += 4 ) {
      y[i + 0] = beta * y[i + 0] + alpha * x[i + 0];
      y[i + 1] = beta * y[i + 1] + alpha * x[i + 1];
      y[i + 2] = beta * y[i + 2] + alpha * x[i + 2];
      y[i + 3] = beta * y[i + 3] + alpha * x[i + 3];
    }
    for ( ; i < n; ++i ) y[i] = beta * y[i] + alpha * x[i];
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
axpby(usize n, T alpha, const T *x, ssize_t incx, T beta, T *y, ssize_t incy) noexcept
{
  if ( incx == 1 && incy == 1 ) {
    axpby<T>(alpha, x, x + n, beta, y);
    return;
  }
  if constexpr ( ieee754_floating<T> ) {
    for ( usize i = 0; i < n; ++i )
      __impl_level1::ref(y, i, incy) = math::fma<T>(alpha, __impl_level1::ref(x, i, incx), beta * __impl_level1::ref(y, i, incy));
  } else {
    for ( usize i = 0; i < n; ++i )
      __impl_level1::ref(y, i, incy) = beta * __impl_level1::ref(y, i, incy) + alpha * __impl_level1::ref(x, i, incx);
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
axpby(T alpha, const vec_view<T> &x, T beta, vec_view<T> y) noexcept
{
  axpby<T>(x.n, alpha, x.data, x.inc, beta, y.data, y.inc);
}

template <typename T>
[[gnu::flatten]] inline constexpr void
scal(T alpha, T *first, T *last) noexcept
{
  const usize n = usize(last - first);
#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( ieee754_floating<T> ) {
      if constexpr ( sizeof(T) == 8 ) {
        __impl_level1::scal_packed_f64(double(alpha), reinterpret_cast<double *>(first), n);
        return;
      } else if constexpr ( sizeof(T) == 4 ) {
        __impl_level1::scal_packed_f32(float(alpha), reinterpret_cast<float *>(first), n);
        return;
      }
    }
  }
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if !consteval {
    if constexpr ( ieee754_floating<T> ) {
      if constexpr ( sizeof(T) == 8 ) {
        __impl_level1::scal_packed_f64_neon(double(alpha), reinterpret_cast<double *>(first), n);
        return;
      } else if constexpr ( sizeof(T) == 4 ) {
        __impl_level1::scal_packed_f32_neon(float(alpha), reinterpret_cast<float *>(first), n);
        return;
      }
    }
  }
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  if !consteval {
    if constexpr ( ieee754_floating<T> ) {
      if constexpr ( sizeof(T) == 4 ) {
        __impl_level1::scal_packed_f32_neon(float(alpha), reinterpret_cast<float *>(first), n);
        return;
      }
    }
  }
#endif
  for ( usize i = 0; i < n; ++i ) first[i] = first[i] * alpha;
}

template <typename T>
[[gnu::flatten]] inline constexpr void
scal(usize n, T alpha, T *x, ssize_t incx) noexcept
{
  if ( incx == 1 ) {
    scal<T>(alpha, x, x + n);
    return;
  }
  for ( usize i = 0; i < n; ++i ) __impl_level1::ref(x, i, incx) = __impl_level1::ref(x, i, incx) * alpha;
}

template <typename T>
[[gnu::flatten]] inline constexpr void
scal(T alpha, vec_view<T> x) noexcept
{
  scal<T>(x.n, alpha, x.data, x.inc);
}

namespace __impl_level1
{

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr F
asum_block(const F *p, usize n) noexcept
{
  F s0 = F(0), s1 = F(0), s2 = F(0), s3 = F(0);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    s0 += mk::manip::fabs<F>(p[i + 0]);
    s1 += mk::manip::fabs<F>(p[i + 1]);
    s2 += mk::manip::fabs<F>(p[i + 2]);
    s3 += mk::manip::fabs<F>(p[i + 3]);
  }
  F tail = F(0);
  for ( ; i < n; ++i ) tail += mk::manip::fabs<F>(p[i]);
  return ((s0 + s1) + (s2 + s3)) + tail;
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr F
asum_pairwise(const F *p, usize n) noexcept
{
  if ( n <= 64 ) return asum_block<F>(p, n);
  const usize half = n / 2;
  return asum_pairwise<F>(p, half) + asum_pairwise<F>(p + half, n - half);
}

};     // namespace __impl_level1

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
asum(const F *first, const F *last) noexcept
{
  const usize n = usize(last - first);
#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( sizeof(F) == 8 ) {
      return F(__impl_level1::asum_packed_f64(reinterpret_cast<const double *>(first), n));
    } else if constexpr ( sizeof(F) == 4 ) {
      return F(__impl_level1::asum_packed_f32(reinterpret_cast<const float *>(first), n));
    }
  }
#endif
  return __impl_level1::asum_pairwise<F>(first, n);
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::flatten]] inline constexpr T
asum(const T *first, const T *last) noexcept
{
  T s{ 0 };
  for ( ; first != last; ++first ) {
    const T v = *first;
    s = s + (v < T(0) ? T(-v) : v);
  }
  return s;
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
asum(usize n, const T *x, ssize_t incx) noexcept
{
  if ( incx == 1 ) return asum<T>(x, x + n);
  if constexpr ( ieee754_floating<T> ) {
    T s = T(0), c = T(0);
    for ( usize i = 0; i < n; ++i ) {
      const T y = mk::manip::fabs<T>(__impl_level1::ref(x, i, incx)) - c;
      const T t = s + y;
      c = (t - s) - y;
      s = t;
    }
    return s;
  } else {
    T s{ 0 };
    for ( usize i = 0; i < n; ++i ) {
      const T v = __impl_level1::ref(x, i, incx);
      s = s + (v < T(0) ? T(-v) : v);
    }
    return s;
  }
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
asum(const vec_view<T> &x) noexcept
{
  return asum<T>(x.n, x.data, x.inc);
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
nrm2(const F *first, const F *last) noexcept
{
  F scale = F(0);
  F ssq = F(1);
  for ( const F *p = first; p != last; ++p ) {
    const F ax = mk::manip::fabs<F>(*p);
    if ( ax != F(0) ) {
      if ( scale < ax ) {
        const F r = scale / ax;
        ssq = F(1) + ssq * r * r;
        scale = ax;
      } else {
        const F r = ax / scale;
        ssq = math::fma<F>(r, r, ssq);
      }
    }
  }
  return (scale == F(0)) ? F(0) : scale * mk::pow_ns::sqrt<F>(ssq);
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
nrm2(usize n, const F *x, ssize_t incx) noexcept
{
  if ( incx == 1 ) return nrm2<F>(x, x + n);
  if ( n == 0 ) return F(0);
  F mx = F(0);
  for ( usize i = 0; i < n; ++i ) {
    const F a = mk::manip::fabs<F>(__impl_level1::ref(x, i, incx));
    if ( a > mx ) mx = a;
  }
  if ( mx == F(0) ) return F(0);
  F s = F(0);
  for ( usize i = 0; i < n; ++i ) {
    const F r = __impl_level1::ref(x, i, incx) / mx;
    s = math::fma<F>(r, r, s);
  }
  return mx * mk::pow_ns::sqrt<F>(s);
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
nrm2(const vec_view<F> &x) noexcept
{
  return nrm2<F>(x.n, x.data, x.inc);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// nrm2_fast
template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
nrm2_fast(const F *first, const F *last) noexcept
{
  const usize n = usize(last - first);
#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( sizeof(F) == 8 ) {
      return F(mk::pow_ns::sqrt<f64>(
          __impl_level1::nrm2_fast_packed_f64(reinterpret_cast<const double *>(first), n)));
    } else if constexpr ( sizeof(F) == 4 ) {
      return F(mk::pow_ns::sqrt<f32>(
          __impl_level1::nrm2_fast_packed_f32(reinterpret_cast<const float *>(first), n)));
    }
  }
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if !consteval {
    if constexpr ( sizeof(F) == 8 ) {
      return F(mk::pow_ns::sqrt<f64>(
          __impl_level1::nrm2_fast_packed_f64_neon(reinterpret_cast<const double *>(first), n)));
    } else if constexpr ( sizeof(F) == 4 ) {
      return F(mk::pow_ns::sqrt<f32>(
          __impl_level1::nrm2_fast_packed_f32_neon(reinterpret_cast<const float *>(first), n)));
    }
  }
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  if !consteval {
    if constexpr ( sizeof(F) == 4 ) {
      return F(mk::pow_ns::sqrt<f32>(
          __impl_level1::nrm2_fast_packed_f32_neon(reinterpret_cast<const float *>(first), n)));
    }
  }
#endif
  F s0 = F(0), s1 = F(0), s2 = F(0), s3 = F(0);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    s0 = math::fma<F>(first[i + 0], first[i + 0], s0);
    s1 = math::fma<F>(first[i + 1], first[i + 1], s1);
    s2 = math::fma<F>(first[i + 2], first[i + 2], s2);
    s3 = math::fma<F>(first[i + 3], first[i + 3], s3);
  }
  F tail = F(0);
  for ( ; i < n; ++i ) tail = math::fma<F>(first[i], first[i], tail);
  return mk::pow_ns::sqrt<F>(((s0 + s1) + (s2 + s3)) + tail);
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
nrm2_fast(usize n, const F *x, ssize_t incx) noexcept
{
  if ( incx == 1 ) return nrm2_fast<F>(x, x + n);
  if ( n == 0 ) return F(0);
  F s0 = F(0), s1 = F(0), s2 = F(0), s3 = F(0);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    const F a0 = __impl_level1::ref(x, i + 0, incx);
    const F a1 = __impl_level1::ref(x, i + 1, incx);
    const F a2 = __impl_level1::ref(x, i + 2, incx);
    const F a3 = __impl_level1::ref(x, i + 3, incx);
    s0 = math::fma<F>(a0, a0, s0);
    s1 = math::fma<F>(a1, a1, s1);
    s2 = math::fma<F>(a2, a2, s2);
    s3 = math::fma<F>(a3, a3, s3);
  }
  F tail = F(0);
  for ( ; i < n; ++i ) {
    const F a = __impl_level1::ref(x, i, incx);
    tail = math::fma<F>(a, a, tail);
  }
  return mk::pow_ns::sqrt<F>(((s0 + s1) + (s2 + s3)) + tail);
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
nrm2_fast(const vec_view<F> &x) noexcept
{
  return nrm2_fast<F>(x.n, x.data, x.inc);
}

namespace __impl_level1
{

template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr T
abs_v(T v) noexcept
{
  // branchless
  if constexpr ( ieee754_floating<T> )
    return mk::manip::fabs<T>(v);
  else
    return (v < T(0)) ? T(-v) : v;
}

};     // namespace __impl_level1

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamax(const T *first, const T *last) noexcept
{
  if ( first == last ) return 0;
  const usize n = usize(last - first);
#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( ieee754_floating<T> ) {
      if constexpr ( sizeof(T) == 8 ) {
        return __impl_level1::iamax_packed_f64(reinterpret_cast<const double *>(first), n);
      } else if constexpr ( sizeof(T) == 4 ) {
        return __impl_level1::iamax_packed_f32(reinterpret_cast<const float *>(first), n);
      }
    }
  }
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if !consteval {
    if constexpr ( ieee754_floating<T> ) {
      if constexpr ( sizeof(T) == 8 ) {
        return __impl_level1::iamax_packed_f64_neon(reinterpret_cast<const double *>(first), n);
      } else if constexpr ( sizeof(T) == 4 ) {
        return __impl_level1::iamax_packed_f32_neon(reinterpret_cast<const float *>(first), n);
      }
    }
  }
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  if !consteval {
    if constexpr ( ieee754_floating<T> ) {
      if constexpr ( sizeof(T) == 4 ) {
        return __impl_level1::iamax_packed_f32_neon(reinterpret_cast<const float *>(first), n);
      }
    }
  }
#endif
  T mx = __impl_level1::abs_v<T>(first[0]);
  usize idx = 0;
  for ( usize i = 1; i < n; ++i ) {
    const T a = __impl_level1::abs_v<T>(first[i]);
    const bool gt = a > mx;
    mx = gt ? a : mx;
    idx = gt ? i : idx;
  }
  return idx;
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamin(const T *first, const T *last) noexcept
{
  if ( first == last ) return 0;
  const usize n = usize(last - first);
  T mn = __impl_level1::abs_v<T>(first[0]);
  usize idx = 0;
  for ( usize i = 1; i < n; ++i ) {
    const T a = __impl_level1::abs_v<T>(first[i]);
    const bool lt = a < mn;
    mn = lt ? a : mn;
    idx = lt ? i : idx;
  }
  return idx;
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamax(usize n, const T *x, ssize_t incx) noexcept
{
  if ( incx == 1 ) return iamax<T>(x, x + n);
  if ( n == 0 ) return 0;
  T mx = __impl_level1::abs_v<T>(__impl_level1::ref(x, 0, incx));
  usize idx = 0;
  for ( usize i = 1; i < n; ++i ) {
    const T a = __impl_level1::abs_v<T>(__impl_level1::ref(x, i, incx));
    const bool gt = a > mx;
    mx = gt ? a : mx;
    idx = gt ? i : idx;
  }
  return idx;
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamin(usize n, const T *x, ssize_t incx) noexcept
{
  if ( incx == 1 ) return iamin<T>(x, x + n);
  if ( n == 0 ) return 0;
  T mn = __impl_level1::abs_v<T>(__impl_level1::ref(x, 0, incx));
  usize idx = 0;
  for ( usize i = 1; i < n; ++i ) {
    const T a = __impl_level1::abs_v<T>(__impl_level1::ref(x, i, incx));
    const bool lt = a < mn;
    mn = lt ? a : mn;
    idx = lt ? i : idx;
  }
  return idx;
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamax(const vec_view<T> &x) noexcept
{
  return iamax<T>(x.n, x.data, x.inc);
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamin(const vec_view<T> &x) noexcept
{
  return iamin<T>(x.n, x.data, x.inc);
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
dot(const F *__restrict__ x, const F *__restrict__ x_end, const F *__restrict__ y) noexcept
{
  const usize n = usize(x_end - x);
#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( sizeof(F) == 8 ) {
      return F(__impl_level1::dot_packed_f64(reinterpret_cast<const double *>(x), reinterpret_cast<const double *>(y), n));
    } else if constexpr ( sizeof(F) == 4 ) {
      return F(__impl_level1::dot_packed_f32(reinterpret_cast<const float *>(x), reinterpret_cast<const float *>(y), n));
    }
  }
#endif
  F s0 = F(0), s1 = F(0), s2 = F(0), s3 = F(0);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    // 4 accumulators for full throughput (ind data streams)
    s0 = math::fma<F>(x[i + 0], y[i + 0], s0);
    s1 = math::fma<F>(x[i + 1], y[i + 1], s1);
    s2 = math::fma<F>(x[i + 2], y[i + 2], s2);
    s3 = math::fma<F>(x[i + 3], y[i + 3], s3);
  }
  F tail = F(0);
  for ( ; i < n; ++i ) tail = math::fma<F>(x[i], y[i], tail);
  return ((s0 + s1) + (s2 + s3)) + tail;
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::flatten]] inline constexpr T
dot(const T *__restrict__ x, const T *__restrict__ x_end, const T *__restrict__ y) noexcept
{
  const usize n = usize(x_end - x);
  T s0{ 0 }, s1{ 0 }, s2{ 0 }, s3{ 0 };
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    s0 = s0 + x[i + 0] * y[i + 0];
    s1 = s1 + x[i + 1] * y[i + 1];
    s2 = s2 + x[i + 2] * y[i + 2];
    s3 = s3 + x[i + 3] * y[i + 3];
  }
  T tail{ 0 };
  for ( ; i < n; ++i ) tail = tail + x[i] * y[i];
  return ((s0 + s1) + (s2 + s3)) + tail;
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
dot(usize n, const T *x, ssize_t incx, const T *y, ssize_t incy) noexcept
{
  if ( incx == 1 && incy == 1 ) return dot<T>(x, x + n, y);
  if constexpr ( ieee754_floating<T> ) {
    T s = T(0);
    for ( usize i = 0; i < n; ++i ) s = math::fma<T>(__impl_level1::ref(x, i, incx), __impl_level1::ref(y, i, incy), s);
    return s;
  } else {
    T s{ 0 };
    for ( usize i = 0; i < n; ++i ) s = s + (__impl_level1::ref(x, i, incx) * __impl_level1::ref(y, i, incy));
    return s;
  }
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
dot(const vec_view<T> &x, const vec_view<T> &y) noexcept
{
  return dot<T>(x.n, x.data, x.inc, y.data, y.inc);
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
dot_kahan(const F *__restrict__ x, const F *__restrict__ x_end, const F *__restrict__ y) noexcept
{
  const usize n = usize(x_end - x);
  // two accumulators
  F s0 = F(0), c0 = F(0);
  F s1 = F(0), c1 = F(0);
  usize i = 0;
  for ( ; i + 2 <= n; i += 2 ) {
    const F p0 = x[i + 0] * y[i + 0];
    const F yy0 = p0 - c0;
    const F t0 = s0 + yy0;
    c0 = (t0 - s0) - yy0;
    s0 = t0;
    const F p1 = x[i + 1] * y[i + 1];
    const F yy1 = p1 - c1;
    const F t1 = s1 + yy1;
    c1 = (t1 - s1) - yy1;
    s1 = t1;
  }
  F s = s0, c = c0;
  {
    const F yy = (s1 - c1) - c;
    const F t = s + yy;
    c = (t - s) - yy;
    s = t;
  }
  for ( ; i < n; ++i ) {
    const F p = x[i] * y[i];
    const F yy = p - c;
    const F t = s + yy;
    c = (t - s) - yy;
    s = t;
  }
  return s;
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
dot_kahan(usize n, const F *x, ssize_t incx, const F *y, ssize_t incy) noexcept
{
  if ( incx == 1 && incy == 1 ) return dot_kahan<F>(x, x + n, y);
  F s = F(0), c = F(0);
  for ( usize i = 0; i < n; ++i ) {
    const F p = __impl_level1::ref(x, i, incx) * __impl_level1::ref(y, i, incy);
    const F yy = p - c;
    const F t = s + yy;
    c = (t - s) - yy;
    s = t;
  }
  return s;
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
dot_kahan(const vec_view<F> &x, const vec_view<F> &y) noexcept
{
  return dot_kahan<F>(x.n, x.data, x.inc, y.data, y.inc);
}

[[nodiscard, gnu::flatten]] inline f32
sdsdot(usize n, f32 b, const f32 *x, ssize_t incx, const f32 *y, ssize_t incy) noexcept
{
  f64 s = f64(b);
  if ( incx == 1 && incy == 1 ) {
    for ( usize i = 0; i < n; ++i ) s = math::fma<f64>(f64(x[i]), f64(y[i]), s);
  } else {
    for ( usize i = 0; i < n; ++i ) s = math::fma<f64>(f64(__impl_level1::ref(x, i, incx)), f64(__impl_level1::ref(y, i, incy)), s);
  }
  return f32(s);
}

template <typename T>
[[gnu::flatten]] inline constexpr void
swap(T *x, T *x_end, T *y) noexcept
{
  for ( ; x != x_end; ++x, ++y ) {
    T tmp = *x;
    *x = *y;
    *y = tmp;
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
swap(usize n, T *x, ssize_t incx, T *y, ssize_t incy) noexcept
{
  if ( incx == 1 && incy == 1 ) {
    swap<T>(x, x + n, y);
    return;
  }
  for ( usize i = 0; i < n; ++i ) {
    T tmp = __impl_level1::ref(x, i, incx);
    __impl_level1::ref(x, i, incx) = __impl_level1::ref(y, i, incy);
    __impl_level1::ref(y, i, incy) = tmp;
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
swap(vec_view<T> x, vec_view<T> y) noexcept
{
  swap<T>(x.n, x.data, x.inc, y.data, y.inc);
}

template <typename T>
[[gnu::flatten]] inline constexpr void
copy(const T *__restrict__ x, const T *__restrict__ x_end, T *__restrict__ y) noexcept
{
  for ( ; x != x_end; ++x, ++y ) *y = *x;
}

template <typename T>
[[gnu::flatten]] inline constexpr void
copy(usize n, const T *x, ssize_t incx, T *y, ssize_t incy) noexcept
{
  if ( incx == 1 && incy == 1 ) {
    copy<T>(x, x + n, y);
    return;
  }
  for ( usize i = 0; i < n; ++i ) __impl_level1::ref(y, i, incy) = __impl_level1::ref(x, i, incx);
}

template <typename T>
[[gnu::flatten]] inline constexpr void
copy(const vec_view<T> &x, vec_view<T> y) noexcept
{
  copy<T>(x.n, x.data, x.inc, y.data, y.inc);
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr void
rotg(F &a, F &b, F &c, F &s) noexcept
{
  const F aa = mk::manip::fabs<F>(a);
  const F ab = mk::manip::fabs<F>(b);
  F sigma;
  if ( aa > ab )
    sigma = (a < F(0)) ? F(-1) : F(1);
  else
    sigma = (b < F(0)) ? F(-1) : F(1);
  if ( aa == F(0) && ab == F(0) ) {
    c = F(1);
    s = F(0);
    a = F(0);
    b = F(0);
    return;
  }
  const F scale = aa + ab;
  const F na = a / scale;
  const F nb = b / scale;
  const F r = sigma * scale * mk::pow_ns::sqrt<F>(math::fma<F>(na, na, nb * nb));
  c = a / r;
  s = b / r;
  F z;
  if ( aa > ab )
    z = s;
  else if ( c != F(0) )
    z = F(1) / c;
  else
    z = F(1);
  a = r;
  b = z;
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr void
rot(usize n, F *x, ssize_t incx, F *y, ssize_t incy, F c, F s) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    F &xi = __impl_level1::ref(x, i, incx);
    F &yi = __impl_level1::ref(y, i, incy);
    const F t = math::fma<F>(c, xi, s * yi);
    yi = math::fma<F>(c, yi, -s * xi);
    xi = t;
  }
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr void
rot(vec_view<F> x, vec_view<F> y, F c, F s) noexcept
{
  rot<F>(x.n, x.data, x.inc, y.data, y.inc, c, s);
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr void
rotmg(F &d1, F &d2, F &x1, F y1, F (&param)[5]) noexcept
{
  constexpr F GAM = F(4096);
  constexpr F GAMSQ = GAM * GAM;
  constexpr F RGAMSQ = F(1) / GAMSQ;

  F flag, h11 = F(0), h21 = F(0), h12 = F(0), h22 = F(0);

  if ( d1 < F(0) ) {
    flag = F(-1);
    d1 = F(0);
    d2 = F(0);
    x1 = F(0);
  } else {
    const F p2 = d2 * y1;
    if ( p2 == F(0) ) {
      flag = F(-2);
      param[0] = flag;
      return;
    }
    const F p1 = d1 * x1;
    const F q2 = p2 * y1;
    const F q1 = p1 * x1;
    if ( mk::manip::fabs<F>(q1) > mk::manip::fabs<F>(q2) ) {
      h21 = -y1 / x1;
      h12 = p2 / p1;
      const F u = F(1) - h12 * h21;
      if ( u > F(0) ) {
        flag = F(0);
        d1 = d1 / u;
        d2 = d2 / u;
        x1 = x1 * u;
      } else {
        flag = F(-1);
        h11 = h21 = h12 = h22 = F(0);
        d1 = d2 = F(0);
        x1 = F(0);
      }
    } else {
      if ( q2 < F(0) ) {
        flag = F(-1);
        h11 = h21 = h12 = h22 = F(0);
        d1 = d2 = F(0);
        x1 = F(0);
      } else {
        flag = F(1);
        h11 = p1 / p2;
        h22 = x1 / y1;
        const F u = F(1) + h11 * h22;
        const F tmp = d2 / u;
        d2 = d1 / u;
        d1 = tmp;
        x1 = y1 * u;
      }
    }

    if ( d1 != F(0) ) {
      while ( (d1 <= RGAMSQ) || (d1 >= GAMSQ) ) {
        if ( flag == F(0) ) {
          h11 = F(1);
          h22 = F(1);
          flag = F(-1);
        } else {
          h21 = F(-1);
          h12 = F(1);
          flag = F(-1);
        }
        if ( d1 <= RGAMSQ ) {
          d1 = d1 * GAMSQ;
          x1 = x1 / GAM;
          h11 = h11 / GAM;
          h12 = h12 / GAM;
        } else {
          d1 = d1 / GAMSQ;
          x1 = x1 * GAM;
          h11 = h11 * GAM;
          h12 = h12 * GAM;
        }
      }
    }
    if ( d2 != F(0) ) {
      while ( (mk::manip::fabs<F>(d2) <= RGAMSQ) || (mk::manip::fabs<F>(d2) >= GAMSQ) ) {
        if ( flag == F(0) ) {
          h11 = F(1);
          h22 = F(1);
          flag = F(-1);
        } else {
          h21 = F(-1);
          h12 = F(1);
          flag = F(-1);
        }
        if ( mk::manip::fabs<F>(d2) <= RGAMSQ ) {
          d2 = d2 * GAMSQ;
          h21 = h21 / GAM;
          h22 = h22 / GAM;
        } else {
          d2 = d2 / GAMSQ;
          h21 = h21 * GAM;
          h22 = h22 * GAM;
        }
      }
    }
  }

  if ( flag < F(0) ) {
    param[1] = h11;
    param[2] = h21;
    param[3] = h12;
    param[4] = h22;
  } else if ( flag == F(0) ) {
    param[2] = h21;
    param[3] = h12;
  } else {
    param[1] = h11;
    param[4] = h22;
  }
  param[0] = flag;
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr void
rotm(usize n, F *x, ssize_t incx, F *y, ssize_t incy, const F (&param)[5]) noexcept
{
  const F flag = param[0];
  if ( flag == F(-2) ) return;
  F h11, h21, h12, h22;
  if ( flag == F(0) ) {
    h11 = F(1);
    h22 = F(1);
    h21 = param[2];
    h12 = param[3];
  } else if ( flag == F(1) ) {
    h11 = param[1];
    h22 = param[4];
    h21 = F(-1);
    h12 = F(1);
  } else {
    h11 = param[1];
    h21 = param[2];
    h12 = param[3];
    h22 = param[4];
  }
  for ( usize i = 0; i < n; ++i ) {
    F &xi = __impl_level1::ref(x, i, incx);
    F &yi = __impl_level1::ref(y, i, incy);
    const F xt = math::fma<F>(h11, xi, h12 * yi);
    yi = math::fma<F>(h21, xi, h22 * yi);
    xi = xt;
  }
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr void
rotm(vec_view<F> x, vec_view<F> y, const F (&param)[5]) noexcept
{
  rotm<F>(x.n, x.data, x.inc, y.data, y.inc, param);
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
axpy(typename C::value_type alpha, const C &x, C &y) noexcept
{
  axpy<typename C::value_type>(alpha, x.cbegin(), x.cend(), y.begin());
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
axpby(typename C::value_type alpha, const C &x, typename C::value_type beta, C &y) noexcept
{
  axpby<typename C::value_type>(alpha, x.cbegin(), x.cend(), beta, y.begin());
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
scal(typename C::value_type alpha, C &x) noexcept
{
  scal<typename C::value_type>(alpha, x.begin(), x.end());
}

template <typename C, typename... Cs>
  requires(sizeof...(Cs) >= 1) && is_iterable_container<C> && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<C>)
          && (!micron::is_const_v<Cs> && ...)
[[gnu::flatten]] inline void
scal(typename C::value_type alpha, C &x, Cs &...xs) noexcept
{
  scal<typename C::value_type>(alpha, x.begin(), x.end());
  ((scal<typename Cs::value_type>(typename Cs::value_type(alpha), xs.begin(), xs.end())), ...);
}

template <is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
asum(const C &c) noexcept
{
  return asum<typename C::value_type>(c.cbegin(), c.cend());
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
nrm2(const C &c) noexcept
{
  return nrm2<typename C::value_type>(c.cbegin(), c.cend());
}

template <is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamax(const C &c) noexcept
{
  return iamax<typename C::value_type>(c.cbegin(), c.cend());
}

template <is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr usize
iamin(const C &c) noexcept
{
  return iamin<typename C::value_type>(c.cbegin(), c.cend());
}

template <is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
dot(const C &x, const C &y) noexcept
{
  return dot<typename C::value_type>(x.cbegin(), x.cend(), y.cbegin());
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
dot_kahan(const C &x, const C &y) noexcept
{
  return dot_kahan<typename C::value_type>(x.cbegin(), x.cend(), y.cbegin());
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
swap(C &x, C &y) noexcept
{
  swap<typename C::value_type>(x.begin(), x.end(), y.begin());
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
copy(const C &x, C &y) noexcept
{
  copy<typename C::value_type>(x.cbegin(), x.cend(), y.begin());
}

};     // namespace level1
};     // namespace blas
};     // namespace math
};     // namespace micron
