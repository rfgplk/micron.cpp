//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../ieee.hpp"

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

#include "views.hpp"

namespace micron
{
namespace math
{
namespace matrix
{
namespace pack
{

#if defined(__AVX512F__)
inline constexpr usize mr_f32 = 16;
inline constexpr usize nr_f32 = 6;
inline constexpr usize mr_f64 = 8;
inline constexpr usize nr_f64 = 6;
#elif defined(__AVX2__)
inline constexpr usize mr_f32 = 8;
inline constexpr usize nr_f32 = 8;
inline constexpr usize mr_f64 = 4;
inline constexpr usize nr_f64 = 8;
#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
inline constexpr usize mr_f32 = 4;
inline constexpr usize nr_f32 = 8;
inline constexpr usize mr_f64 = 4;
inline constexpr usize nr_f64 = 4;
#else
inline constexpr usize mr_f32 = 4;
inline constexpr usize nr_f32 = 4;
inline constexpr usize mr_f64 = 4;
inline constexpr usize nr_f64 = 4;
#endif

template <typename T> inline constexpr usize mr_v = 4;
template <> inline constexpr usize mr_v<f32> = mr_f32;
template <> inline constexpr usize mr_v<f64> = mr_f64;

template <typename T> inline constexpr usize nr_v = 4;
template <> inline constexpr usize nr_v<f32> = nr_f32;
template <> inline constexpr usize nr_v<f64> = nr_f64;

inline constexpr usize default_mc = 64;
inline constexpr usize default_kc = 256;
inline constexpr usize default_nc = 256;

template <usize MR, typename T>
[[gnu::flatten, gnu::always_inline]] inline void
pack_a_panel(const T *A, ssize_t rs_A, ssize_t cs_A, usize m, usize k, T *dst) noexcept
{
  for ( usize t = 0; t < m; t += MR ) {
    const usize rows = (m - t < MR) ? (m - t) : MR;
    for ( usize p = 0; p < k; ++p ) {
      for ( usize i = 0; i < rows; ++i ) dst[i] = A[ssize_t(t + i) * rs_A + ssize_t(p) * cs_A];
      for ( usize i = rows; i < MR; ++i ) dst[i] = T(0);
      dst += MR;
    }
  }
}

template <usize NR, typename T>
[[gnu::flatten, gnu::always_inline]] inline void
pack_b_panel(const T *B, ssize_t rs_B, ssize_t cs_B, usize k, usize n, T *dst) noexcept
{
  for ( usize t = 0; t < n; t += NR ) {
    const usize cols = (n - t < NR) ? (n - t) : NR;
    for ( usize p = 0; p < k; ++p ) {
      for ( usize j = 0; j < cols; ++j ) dst[j] = B[ssize_t(p) * rs_B + ssize_t(t + j) * cs_B];
      for ( usize j = cols; j < NR; ++j ) dst[j] = T(0);
      dst += NR;
    }
  }
}

template <usize MR, usize NR, typename T>
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel_scalar(const T *Ap, const T *Bp, usize k, T alpha, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  T acc[MR][NR] = {};
  for ( usize p = 0; p < k; ++p ) {
    for ( usize i = 0; i < MR; ++i ) {
      const T a = Ap[p * MR + i];
      for ( usize j = 0; j < NR; ++j ) {
        if constexpr ( ieee754_floating<T> ) {
          acc[i][j] = math::fma<T>(a, Bp[p * NR + j], acc[i][j]);
        } else {
          acc[i][j] = acc[i][j] + a * Bp[p * NR + j];
        }
      }
    }
  }
  for ( usize i = 0; i < MR; ++i ) {
    for ( usize j = 0; j < NR; ++j ) {
      T &cij = C[ssize_t(i) * rs_C + ssize_t(j) * cs_C];
      if constexpr ( ieee754_floating<T> ) {
        cij = math::fma<T>(alpha, acc[i][j], beta * cij);
      } else {
        cij = alpha * acc[i][j] + beta * cij;
      }
    }
  }
}

template <usize MR, usize NR, typename T>
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel(const T *Ap, const T *Bp, usize k, T alpha, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  micro_kernel_scalar<MR, NR, T>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
}

#if defined(__AVX2__) && defined(__FMA__)

// 4 rows × 8 cols f64 microkernel
template <>
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel<4, 8, double>(const double *Ap, const double *Bp, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                           ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<4, 8, double>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  __m256d c00 = _mm256_setzero_pd(), c01 = _mm256_setzero_pd();
  __m256d c10 = _mm256_setzero_pd(), c11 = _mm256_setzero_pd();
  __m256d c20 = _mm256_setzero_pd(), c21 = _mm256_setzero_pd();
  __m256d c30 = _mm256_setzero_pd(), c31 = _mm256_setzero_pd();
  for ( usize p = 0; p < k; ++p ) {
    const __m256d b0 = _mm256_loadu_pd(Bp + p * 8 + 0);
    const __m256d b1 = _mm256_loadu_pd(Bp + p * 8 + 4);
    const __m256d a0 = _mm256_set1_pd(Ap[p * 4 + 0]);
    const __m256d a1 = _mm256_set1_pd(Ap[p * 4 + 1]);
    c00 = _mm256_fmadd_pd(a0, b0, c00);
    c01 = _mm256_fmadd_pd(a0, b1, c01);
    c10 = _mm256_fmadd_pd(a1, b0, c10);
    c11 = _mm256_fmadd_pd(a1, b1, c11);
    const __m256d a2 = _mm256_set1_pd(Ap[p * 4 + 2]);
    const __m256d a3 = _mm256_set1_pd(Ap[p * 4 + 3]);
    c20 = _mm256_fmadd_pd(a2, b0, c20);
    c21 = _mm256_fmadd_pd(a2, b1, c21);
    c30 = _mm256_fmadd_pd(a3, b0, c30);
    c31 = _mm256_fmadd_pd(a3, b1, c31);
  }
  const __m256d valpha = _mm256_set1_pd(alpha);
  double *r0 = C + ssize_t(0) * rs_C;
  double *r1 = C + ssize_t(1) * rs_C;
  double *r2 = C + ssize_t(2) * rs_C;
  double *r3 = C + ssize_t(3) * rs_C;
  if ( beta == 0.0 ) {
    _mm256_storeu_pd(r0 + 0, _mm256_mul_pd(valpha, c00));
    _mm256_storeu_pd(r0 + 4, _mm256_mul_pd(valpha, c01));
    _mm256_storeu_pd(r1 + 0, _mm256_mul_pd(valpha, c10));
    _mm256_storeu_pd(r1 + 4, _mm256_mul_pd(valpha, c11));
    _mm256_storeu_pd(r2 + 0, _mm256_mul_pd(valpha, c20));
    _mm256_storeu_pd(r2 + 4, _mm256_mul_pd(valpha, c21));
    _mm256_storeu_pd(r3 + 0, _mm256_mul_pd(valpha, c30));
    _mm256_storeu_pd(r3 + 4, _mm256_mul_pd(valpha, c31));
  } else if ( beta == 1.0 ) {
    _mm256_storeu_pd(r0 + 0, _mm256_fmadd_pd(valpha, c00, _mm256_loadu_pd(r0 + 0)));
    _mm256_storeu_pd(r0 + 4, _mm256_fmadd_pd(valpha, c01, _mm256_loadu_pd(r0 + 4)));
    _mm256_storeu_pd(r1 + 0, _mm256_fmadd_pd(valpha, c10, _mm256_loadu_pd(r1 + 0)));
    _mm256_storeu_pd(r1 + 4, _mm256_fmadd_pd(valpha, c11, _mm256_loadu_pd(r1 + 4)));
    _mm256_storeu_pd(r2 + 0, _mm256_fmadd_pd(valpha, c20, _mm256_loadu_pd(r2 + 0)));
    _mm256_storeu_pd(r2 + 4, _mm256_fmadd_pd(valpha, c21, _mm256_loadu_pd(r2 + 4)));
    _mm256_storeu_pd(r3 + 0, _mm256_fmadd_pd(valpha, c30, _mm256_loadu_pd(r3 + 0)));
    _mm256_storeu_pd(r3 + 4, _mm256_fmadd_pd(valpha, c31, _mm256_loadu_pd(r3 + 4)));
  } else {
    const __m256d vbeta = _mm256_set1_pd(beta);
    _mm256_storeu_pd(r0 + 0, _mm256_fmadd_pd(valpha, c00, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r0 + 0))));
    _mm256_storeu_pd(r0 + 4, _mm256_fmadd_pd(valpha, c01, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r0 + 4))));
    _mm256_storeu_pd(r1 + 0, _mm256_fmadd_pd(valpha, c10, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r1 + 0))));
    _mm256_storeu_pd(r1 + 4, _mm256_fmadd_pd(valpha, c11, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r1 + 4))));
    _mm256_storeu_pd(r2 + 0, _mm256_fmadd_pd(valpha, c20, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r2 + 0))));
    _mm256_storeu_pd(r2 + 4, _mm256_fmadd_pd(valpha, c21, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r2 + 4))));
    _mm256_storeu_pd(r3 + 0, _mm256_fmadd_pd(valpha, c30, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r3 + 0))));
    _mm256_storeu_pd(r3 + 4, _mm256_fmadd_pd(valpha, c31, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r3 + 4))));
  }
}

// 8 rows × 8 cols f32 microkernel (AVX2 ymm = 8 floats wide)
template <>
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel<8, 8, float>(const float *Ap, const float *Bp, usize k, float alpha, float beta, float *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<8, 8, float>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  __m256 c0 = _mm256_setzero_ps(), c1 = _mm256_setzero_ps();
  __m256 c2 = _mm256_setzero_ps(), c3 = _mm256_setzero_ps();
  __m256 c4 = _mm256_setzero_ps(), c5 = _mm256_setzero_ps();
  __m256 c6 = _mm256_setzero_ps(), c7 = _mm256_setzero_ps();
  for ( usize p = 0; p < k; ++p ) {
    const __m256 br = _mm256_loadu_ps(Bp + p * 8);
    c0 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 0]), br, c0);
    c1 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 1]), br, c1);
    c2 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 2]), br, c2);
    c3 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 3]), br, c3);
    c4 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 4]), br, c4);
    c5 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 5]), br, c5);
    c6 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 6]), br, c6);
    c7 = _mm256_fmadd_ps(_mm256_set1_ps(Ap[p * 8 + 7]), br, c7);
  }
  const __m256 valpha = _mm256_set1_ps(alpha);
  float *rs[8] = { C + 0 * rs_C, C + 1 * rs_C, C + 2 * rs_C, C + 3 * rs_C,
                   C + 4 * rs_C, C + 5 * rs_C, C + 6 * rs_C, C + 7 * rs_C };
  __m256 cs[8] = { c0, c1, c2, c3, c4, c5, c6, c7 };
  if ( beta == 0.0f ) {
    for ( usize i = 0; i < 8; ++i ) _mm256_storeu_ps(rs[i], _mm256_mul_ps(valpha, cs[i]));
  } else if ( beta == 1.0f ) {
    for ( usize i = 0; i < 8; ++i ) _mm256_storeu_ps(rs[i], _mm256_fmadd_ps(valpha, cs[i], _mm256_loadu_ps(rs[i])));
  } else {
    const __m256 vbeta = _mm256_set1_ps(beta);
    for ( usize i = 0; i < 8; ++i ) _mm256_storeu_ps(rs[i], _mm256_fmadd_ps(valpha, cs[i], _mm256_mul_ps(vbeta, _mm256_loadu_ps(rs[i]))));
  }
}

#endif     // AVX2 + FMA

#if defined(__micron_arch_arm64) && defined(__micron_arm_neon)

// NEON arm64 4x4 f64 packed microkernel
template <>
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel<4, 4, double>(const double *Ap, const double *Bp, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                           ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<4, 4, double>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  float64x2_t c00 = vdupq_n_f64(0.0), c01 = vdupq_n_f64(0.0);
  float64x2_t c10 = vdupq_n_f64(0.0), c11 = vdupq_n_f64(0.0);
  float64x2_t c20 = vdupq_n_f64(0.0), c21 = vdupq_n_f64(0.0);
  float64x2_t c30 = vdupq_n_f64(0.0), c31 = vdupq_n_f64(0.0);
  for ( usize p = 0; p < k; ++p ) {
    const float64x2_t b0 = vld1q_f64(Bp + p * 4 + 0);
    const float64x2_t b1 = vld1q_f64(Bp + p * 4 + 2);
    const float64x2_t a0 = vdupq_n_f64(Ap[p * 4 + 0]);
    const float64x2_t a1 = vdupq_n_f64(Ap[p * 4 + 1]);
    const float64x2_t a2 = vdupq_n_f64(Ap[p * 4 + 2]);
    const float64x2_t a3 = vdupq_n_f64(Ap[p * 4 + 3]);
    c00 = vfmaq_f64(c00, a0, b0); c01 = vfmaq_f64(c01, a0, b1);
    c10 = vfmaq_f64(c10, a1, b0); c11 = vfmaq_f64(c11, a1, b1);
    c20 = vfmaq_f64(c20, a2, b0); c21 = vfmaq_f64(c21, a2, b1);
    c30 = vfmaq_f64(c30, a3, b0); c31 = vfmaq_f64(c31, a3, b1);
  }
  const float64x2_t valpha = vdupq_n_f64(alpha);
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  if ( beta == 0.0 ) {
    vst1q_f64(r0 + 0, vmulq_f64(valpha, c00));
    vst1q_f64(r0 + 2, vmulq_f64(valpha, c01));
    vst1q_f64(r1 + 0, vmulq_f64(valpha, c10));
    vst1q_f64(r1 + 2, vmulq_f64(valpha, c11));
    vst1q_f64(r2 + 0, vmulq_f64(valpha, c20));
    vst1q_f64(r2 + 2, vmulq_f64(valpha, c21));
    vst1q_f64(r3 + 0, vmulq_f64(valpha, c30));
    vst1q_f64(r3 + 2, vmulq_f64(valpha, c31));
  } else if ( beta == 1.0 ) {
    vst1q_f64(r0 + 0, vfmaq_f64(vld1q_f64(r0 + 0), valpha, c00));
    vst1q_f64(r0 + 2, vfmaq_f64(vld1q_f64(r0 + 2), valpha, c01));
    vst1q_f64(r1 + 0, vfmaq_f64(vld1q_f64(r1 + 0), valpha, c10));
    vst1q_f64(r1 + 2, vfmaq_f64(vld1q_f64(r1 + 2), valpha, c11));
    vst1q_f64(r2 + 0, vfmaq_f64(vld1q_f64(r2 + 0), valpha, c20));
    vst1q_f64(r2 + 2, vfmaq_f64(vld1q_f64(r2 + 2), valpha, c21));
    vst1q_f64(r3 + 0, vfmaq_f64(vld1q_f64(r3 + 0), valpha, c30));
    vst1q_f64(r3 + 2, vfmaq_f64(vld1q_f64(r3 + 2), valpha, c31));
  } else {
    const float64x2_t vbeta = vdupq_n_f64(beta);
    vst1q_f64(r0 + 0, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r0 + 0)), valpha, c00));
    vst1q_f64(r0 + 2, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r0 + 2)), valpha, c01));
    vst1q_f64(r1 + 0, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r1 + 0)), valpha, c10));
    vst1q_f64(r1 + 2, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r1 + 2)), valpha, c11));
    vst1q_f64(r2 + 0, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r2 + 0)), valpha, c20));
    vst1q_f64(r2 + 2, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r2 + 2)), valpha, c21));
    vst1q_f64(r3 + 0, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r3 + 0)), valpha, c30));
    vst1q_f64(r3 + 2, vfmaq_f64(vmulq_f64(vbeta, vld1q_f64(r3 + 2)), valpha, c31));
  }
}

#endif     // arm64 NEON

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

// NEON 4 rows × 8 cols f32 packed microkernel (arm32 + arm64)
template <>
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel<4, 8, float>(const float *Ap, const float *Bp, usize k, float alpha, float beta, float *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<4, 8, float>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  float32x4_t c00 = vdupq_n_f32(0.0f), c01 = vdupq_n_f32(0.0f);
  float32x4_t c10 = vdupq_n_f32(0.0f), c11 = vdupq_n_f32(0.0f);
  float32x4_t c20 = vdupq_n_f32(0.0f), c21 = vdupq_n_f32(0.0f);
  float32x4_t c30 = vdupq_n_f32(0.0f), c31 = vdupq_n_f32(0.0f);
  for ( usize p = 0; p < k; ++p ) {
    const float32x4_t b0 = vld1q_f32(Bp + p * 8 + 0);
    const float32x4_t b1 = vld1q_f32(Bp + p * 8 + 4);
    const float32x4_t a0 = vdupq_n_f32(Ap[p * 4 + 0]);
    const float32x4_t a1 = vdupq_n_f32(Ap[p * 4 + 1]);
    const float32x4_t a2 = vdupq_n_f32(Ap[p * 4 + 2]);
    const float32x4_t a3 = vdupq_n_f32(Ap[p * 4 + 3]);
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
    c00 = vfmaq_f32(c00, a0, b0); c01 = vfmaq_f32(c01, a0, b1);
    c10 = vfmaq_f32(c10, a1, b0); c11 = vfmaq_f32(c11, a1, b1);
    c20 = vfmaq_f32(c20, a2, b0); c21 = vfmaq_f32(c21, a2, b1);
    c30 = vfmaq_f32(c30, a3, b0); c31 = vfmaq_f32(c31, a3, b1);
#else
    c00 = vmlaq_f32(c00, a0, b0); c01 = vmlaq_f32(c01, a0, b1);
    c10 = vmlaq_f32(c10, a1, b0); c11 = vmlaq_f32(c11, a1, b1);
    c20 = vmlaq_f32(c20, a2, b0); c21 = vmlaq_f32(c21, a2, b1);
    c30 = vmlaq_f32(c30, a3, b0); c31 = vmlaq_f32(c31, a3, b1);
#endif
  }
  const float32x4_t valpha = vdupq_n_f32(alpha);
  float *r0 = C + 0 * rs_C;
  float *r1 = C + 1 * rs_C;
  float *r2 = C + 2 * rs_C;
  float *r3 = C + 3 * rs_C;
  if ( beta == 0.0f ) {
    vst1q_f32(r0 + 0, vmulq_f32(valpha, c00));
    vst1q_f32(r0 + 4, vmulq_f32(valpha, c01));
    vst1q_f32(r1 + 0, vmulq_f32(valpha, c10));
    vst1q_f32(r1 + 4, vmulq_f32(valpha, c11));
    vst1q_f32(r2 + 0, vmulq_f32(valpha, c20));
    vst1q_f32(r2 + 4, vmulq_f32(valpha, c21));
    vst1q_f32(r3 + 0, vmulq_f32(valpha, c30));
    vst1q_f32(r3 + 4, vmulq_f32(valpha, c31));
  } else {
    const float32x4_t vbeta = vdupq_n_f32(beta);
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
    vst1q_f32(r0 + 0, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r0 + 0)), valpha, c00));
    vst1q_f32(r0 + 4, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r0 + 4)), valpha, c01));
    vst1q_f32(r1 + 0, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r1 + 0)), valpha, c10));
    vst1q_f32(r1 + 4, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r1 + 4)), valpha, c11));
    vst1q_f32(r2 + 0, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r2 + 0)), valpha, c20));
    vst1q_f32(r2 + 4, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r2 + 4)), valpha, c21));
    vst1q_f32(r3 + 0, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r3 + 0)), valpha, c30));
    vst1q_f32(r3 + 4, vfmaq_f32(vmulq_f32(vbeta, vld1q_f32(r3 + 4)), valpha, c31));
#else
    vst1q_f32(r0 + 0, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r0 + 0)), valpha, c00));
    vst1q_f32(r0 + 4, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r0 + 4)), valpha, c01));
    vst1q_f32(r1 + 0, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r1 + 0)), valpha, c10));
    vst1q_f32(r1 + 4, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r1 + 4)), valpha, c11));
    vst1q_f32(r2 + 0, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r2 + 0)), valpha, c20));
    vst1q_f32(r2 + 4, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r2 + 4)), valpha, c21));
    vst1q_f32(r3 + 0, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r3 + 0)), valpha, c30));
    vst1q_f32(r3 + 4, vmlaq_f32(vmulq_f32(vbeta, vld1q_f32(r3 + 4)), valpha, c31));
#endif
  }
}

#endif     // arm any NEON

template <usize MR, usize NR, typename T>
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel_partial(usize mr, usize nr, const T *Ap, const T *Bp, usize k, T alpha, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  T acc[MR][NR] = {};
  for ( usize p = 0; p < k; ++p ) {
    for ( usize i = 0; i < mr; ++i ) {
      const T a = Ap[p * MR + i];
      for ( usize j = 0; j < nr; ++j ) {
        if constexpr ( ieee754_floating<T> ) {
          acc[i][j] = math::fma<T>(a, Bp[p * NR + j], acc[i][j]);
        } else {
          acc[i][j] = acc[i][j] + a * Bp[p * NR + j];
        }
      }
    }
  }
  if ( beta == T(0) ) {
    for ( usize i = 0; i < mr; ++i )
      for ( usize j = 0; j < nr; ++j ) C[ssize_t(i) * rs_C + ssize_t(j) * cs_C] = alpha * acc[i][j];
  } else if ( beta == T(1) ) {
    for ( usize i = 0; i < mr; ++i )
      for ( usize j = 0; j < nr; ++j ) {
        T &cij = C[ssize_t(i) * rs_C + ssize_t(j) * cs_C];
        if constexpr ( ieee754_floating<T> )
          cij = math::fma<T>(alpha, acc[i][j], cij);
        else
          cij = alpha * acc[i][j] + cij;
      }
  } else {
    for ( usize i = 0; i < mr; ++i )
      for ( usize j = 0; j < nr; ++j ) {
        T &cij = C[ssize_t(i) * rs_C + ssize_t(j) * cs_C];
        if constexpr ( ieee754_floating<T> )
          cij = math::fma<T>(alpha, acc[i][j], beta * cij);
        else
          cij = alpha * acc[i][j] + beta * cij;
      }
  }
}

template <typename T> inline constexpr usize gemm_mr_v = mr_v<T>;
template <typename T> inline constexpr usize gemm_nr_v = nr_v<T>;

#if defined(__AVX2__) && defined(__FMA__)
template <> inline constexpr usize gemm_mr_v<f64> = 4;
template <> inline constexpr usize gemm_nr_v<f64> = 8;
template <> inline constexpr usize gemm_mr_v<f32> = 8;
template <> inline constexpr usize gemm_nr_v<f32> = 8;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
template <> inline constexpr usize gemm_mr_v<f64> = 4;
template <> inline constexpr usize gemm_nr_v<f64> = 4;
template <> inline constexpr usize gemm_mr_v<f32> = 4;
template <> inline constexpr usize gemm_nr_v<f32> = 8;
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
template <> inline constexpr usize gemm_mr_v<f32> = 4;
template <> inline constexpr usize gemm_nr_v<f32> = 8;
#endif

template <typename T>
[[gnu::flatten]] inline void
gemm_blocked(usize m, usize n, usize k, T alpha, const T *A, ssize_t a_rs, ssize_t a_cs, const T *B, ssize_t b_rs, ssize_t b_cs, T beta,
             T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  constexpr usize MR = gemm_mr_v<T>;
  constexpr usize NR = gemm_nr_v<T>;
  const usize Mc = (m < default_mc) ? m : default_mc;
  const usize Kc = (k < default_kc) ? k : default_kc;
  const usize Nc = (n < default_nc) ? n : default_nc;

  using buf_t = micron::vector<T, micron::allocator_serial<>, false>;
  buf_t a_pack(Mc * Kc);
  buf_t b_pack(Kc * Nc);
  T *Ap = a_pack.data();
  T *Bp = b_pack.data();

  for ( usize jc = 0; jc < n; jc += Nc ) {
    const usize nc = ((n - jc) < Nc) ? (n - jc) : Nc;
    for ( usize pc = 0; pc < k; pc += Kc ) {
      const usize kc = ((k - pc) < Kc) ? (k - pc) : Kc;
      const T beta_eff = (pc == 0) ? beta : T(1);

      const T *B_block = B + ssize_t(pc) * b_rs + ssize_t(jc) * b_cs;
      pack_b_panel<NR, T>(B_block, b_rs, b_cs, kc, nc, Bp);

      for ( usize ic = 0; ic < m; ic += Mc ) {
        const usize mc = ((m - ic) < Mc) ? (m - ic) : Mc;
        const T *A_block = A + ssize_t(ic) * a_rs + ssize_t(pc) * a_cs;
        pack_a_panel<MR, T>(A_block, a_rs, a_cs, mc, kc, Ap);

        for ( usize jr = 0; jr < nc; jr += NR ) {
          const usize nr_eff = ((nc - jr) < NR) ? (nc - jr) : NR;
          const T *Bp_tile = Bp + (jr / NR) * NR * kc;
          for ( usize ir = 0; ir < mc; ir += MR ) {
            const usize mr_eff = ((mc - ir) < MR) ? (mc - ir) : MR;
            const T *Ap_tile = Ap + (ir / MR) * MR * kc;
            T *C_tile = C + ssize_t(ic + ir) * rs_C + ssize_t(jc + jr) * cs_C;
            if ( mr_eff == MR && nr_eff == NR ) {
              micro_kernel<MR, NR, T>(Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
            } else {
              micro_kernel_partial<MR, NR, T>(mr_eff, nr_eff, Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
            }
          }
        }
      }
    }
  }
}

// NOTE: pick the BLIS-style blocked path when the problem is large enough
template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr bool
gemm_should_block(usize m, usize n, usize k) noexcept
{
  if constexpr ( !ieee754_floating<T> ) return false;
  return (m >= 64) && (n >= 64) && (k >= 64) && ((m * n * k) >= (256u * 256u * 256u));
}

// NOTE: For NT/TT, B's effective contiguous stride is along the leading dim, which breaks the microkernels packed load assumption
template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr bool
gemm_should_block_for_layout(usize m, usize n, usize k, ssize_t b_cs_eff, ssize_t cs_C) noexcept
{
  if constexpr ( !ieee754_floating<T> ) return false;
  if ( gemm_should_block<T>(m, n, k) ) return true;
  const bool strided = (cs_C != 1) || (b_cs_eff != 1);
  if ( strided ) return (m >= 64) && (n >= 64) && (k >= 64);
  return false;
}

};     // namespace pack
};     // namespace matrix
};     // namespace math
};     // namespace micron
