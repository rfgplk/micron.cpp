//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// BLAS micro-kernels
//
// NOTE: the level 2 & 3 routines take in an (row_stride, col_stride) pair;
// for row_view  rs=ld, cs=1;
// for col_view  rs=1, cs=ld
#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../bits/impl.hpp"
#include "../../ieee.hpp"
#include "../../matrix/pack.hpp"

#if defined(__AVX2__) && defined(__FMA__)
#include "../../../simd/arch/types_amd64.hpp"
#endif

namespace micron
{
namespace math
{
namespace blas
{
namespace bits
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ats()
template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr T &
mat_at(T *data, usize i, usize j, ssize_t rs, ssize_t cs) noexcept
{
  return data[ssize_t(i) * rs + ssize_t(j) * cs];
}

template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr const T &
mat_at(const T *data, usize i, usize j, ssize_t rs, ssize_t cs) noexcept
{
  return data[ssize_t(i) * rs + ssize_t(j) * cs];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fma_accs (c = c + a * b)
template <typename T>
[[gnu::always_inline]] inline constexpr T
fma_acc(T a, T b, T c) noexcept
{
  if constexpr ( ieee754_floating<T> )
    return math::fma<T>(a, b, c);
  else
    return c + a * b;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gemv transpose panel-packed kernels
#if defined(__AVX2__) && defined(__FMA__)

[[gnu::flatten]] inline void
gemv_t_panel_avx2_f64(usize m, usize n, double alpha, const double *A, ssize_t rs_A, const double *x, double beta, double *y) noexcept
{
  constexpr usize NB = 16;      // j-strip width (4 ymm of 4 doubles each)
  constexpr usize MC = 128;     // m-block height; NB*MC doubles = 16 KiB pack, fits L1
  alignas(32) double Ap[NB * MC];

  if ( beta == 0.0 ) {
    const __m256d z = _mm256_setzero_pd();
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) _mm256_storeu_pd(y + j, z);
    for ( ; j < n; ++j ) y[j] = 0.0;
  } else if ( beta != 1.0 ) {
    const __m256d vbeta = _mm256_set1_pd(beta);
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) _mm256_storeu_pd(y + j, _mm256_mul_pd(vbeta, _mm256_loadu_pd(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    __m256d y0 = _mm256_loadu_pd(y + j + 0);
    __m256d y1 = _mm256_loadu_pd(y + j + 4);
    __m256d y2 = _mm256_loadu_pd(y + j + 8);
    __m256d y3 = _mm256_loadu_pd(y + j + 12);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;

      for ( usize i = 0; i < mc; ++i ) {
        const double *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        _mm256_store_pd(Ap + i * NB + 0, _mm256_loadu_pd(src + 0));
        _mm256_store_pd(Ap + i * NB + 4, _mm256_loadu_pd(src + 4));
        _mm256_store_pd(Ap + i * NB + 8, _mm256_loadu_pd(src + 8));
        _mm256_store_pd(Ap + i * NB + 12, _mm256_loadu_pd(src + 12));
      }

      for ( usize i = 0; i < mc; ++i ) {
        const __m256d ax = _mm256_set1_pd(alpha * x[ic + i]);
        const double *ap = Ap + i * NB;
        y0 = _mm256_fmadd_pd(ax, _mm256_load_pd(ap + 0), y0);
        y1 = _mm256_fmadd_pd(ax, _mm256_load_pd(ap + 4), y1);
        y2 = _mm256_fmadd_pd(ax, _mm256_load_pd(ap + 8), y2);
        y3 = _mm256_fmadd_pd(ax, _mm256_load_pd(ap + 12), y3);
      }
    }

    _mm256_storeu_pd(y + j + 0, y0);
    _mm256_storeu_pd(y + j + 4, y1);
    _mm256_storeu_pd(y + j + 8, y2);
    _mm256_storeu_pd(y + j + 12, y3);
  }

  // tail
  for ( ; j < n; ++j ) {
    double acc = 0.0;
    for ( usize i = 0; i < m; ++i ) acc = math::fma<double>(A[ssize_t(i) * rs_A + ssize_t(j)], x[i], acc);
    y[j] = math::fma<double>(alpha, acc, y[j]);
  }
}

[[gnu::flatten]] inline void
gemv_t_panel_avx2_f32(usize m, usize n, float alpha, const float *A, ssize_t rs_A, const float *x, float beta, float *y) noexcept
{
  constexpr usize NB = 32;      // 4 ymm of 8 floats each
  constexpr usize MC = 128;     // pack 32 * 128 * 4 = 16 KiB
  alignas(32) float Ap[NB * MC];

  if ( beta == 0.0f ) {
    const __m256 z = _mm256_setzero_ps();
    usize j = 0;
    for ( ; j + 8 <= n; j += 8 ) _mm256_storeu_ps(y + j, z);
    for ( ; j < n; ++j ) y[j] = 0.0f;
  } else if ( beta != 1.0f ) {
    const __m256 vbeta = _mm256_set1_ps(beta);
    usize j = 0;
    for ( ; j + 8 <= n; j += 8 ) _mm256_storeu_ps(y + j, _mm256_mul_ps(vbeta, _mm256_loadu_ps(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    __m256 y0 = _mm256_loadu_ps(y + j + 0);
    __m256 y1 = _mm256_loadu_ps(y + j + 8);
    __m256 y2 = _mm256_loadu_ps(y + j + 16);
    __m256 y3 = _mm256_loadu_ps(y + j + 24);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;
      for ( usize i = 0; i < mc; ++i ) {
        const float *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        _mm256_store_ps(Ap + i * NB + 0, _mm256_loadu_ps(src + 0));
        _mm256_store_ps(Ap + i * NB + 8, _mm256_loadu_ps(src + 8));
        _mm256_store_ps(Ap + i * NB + 16, _mm256_loadu_ps(src + 16));
        _mm256_store_ps(Ap + i * NB + 24, _mm256_loadu_ps(src + 24));
      }
      for ( usize i = 0; i < mc; ++i ) {
        const __m256 ax = _mm256_set1_ps(alpha * x[ic + i]);
        const float *ap = Ap + i * NB;
        y0 = _mm256_fmadd_ps(ax, _mm256_load_ps(ap + 0), y0);
        y1 = _mm256_fmadd_ps(ax, _mm256_load_ps(ap + 8), y1);
        y2 = _mm256_fmadd_ps(ax, _mm256_load_ps(ap + 16), y2);
        y3 = _mm256_fmadd_ps(ax, _mm256_load_ps(ap + 24), y3);
      }
    }

    _mm256_storeu_ps(y + j + 0, y0);
    _mm256_storeu_ps(y + j + 8, y1);
    _mm256_storeu_ps(y + j + 16, y2);
    _mm256_storeu_ps(y + j + 24, y3);
  }

  for ( ; j < n; ++j ) {
    float acc = 0.0f;
    for ( usize i = 0; i < m; ++i ) acc = math::fma<float>(A[ssize_t(i) * rs_A + ssize_t(j)], x[i], acc);
    y[j] = math::fma<float>(alpha, acc, y[j]);
  }
}

#endif     // AVX2 + FMA

#if defined(__micron_arch_arm64) && defined(__micron_arm_neon)

[[gnu::flatten]] inline void
gemv_t_panel_neon_f64(usize m, usize n, double alpha, const double *A, ssize_t rs_A, const double *x, double beta, double *y) noexcept
{
  constexpr usize NB = 8;     // 4 q-regs of 2 doubles each
  constexpr usize MC = 256;
  alignas(16) double Ap[NB * MC];

  if ( beta == 0.0 ) {
    for ( usize j = 0; j < n; ++j ) y[j] = 0.0;
  } else if ( beta != 1.0 ) {
    const float64x2_t vbeta = vdupq_n_f64(beta);
    usize j = 0;
    for ( ; j + 2 <= n; j += 2 ) vst1q_f64(y + j, vmulq_f64(vbeta, vld1q_f64(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    float64x2_t y0 = vld1q_f64(y + j + 0);
    float64x2_t y1 = vld1q_f64(y + j + 2);
    float64x2_t y2 = vld1q_f64(y + j + 4);
    float64x2_t y3 = vld1q_f64(y + j + 6);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;
      for ( usize i = 0; i < mc; ++i ) {
        const double *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        vst1q_f64(Ap + i * NB + 0, vld1q_f64(src + 0));
        vst1q_f64(Ap + i * NB + 2, vld1q_f64(src + 2));
        vst1q_f64(Ap + i * NB + 4, vld1q_f64(src + 4));
        vst1q_f64(Ap + i * NB + 6, vld1q_f64(src + 6));
      }
      for ( usize i = 0; i < mc; ++i ) {
        const float64x2_t ax = vdupq_n_f64(alpha * x[ic + i]);
        const double *ap = Ap + i * NB;
        y0 = vfmaq_f64(y0, ax, vld1q_f64(ap + 0));
        y1 = vfmaq_f64(y1, ax, vld1q_f64(ap + 2));
        y2 = vfmaq_f64(y2, ax, vld1q_f64(ap + 4));
        y3 = vfmaq_f64(y3, ax, vld1q_f64(ap + 6));
      }
    }

    vst1q_f64(y + j + 0, y0);
    vst1q_f64(y + j + 2, y1);
    vst1q_f64(y + j + 4, y2);
    vst1q_f64(y + j + 6, y3);
  }

  for ( ; j < n; ++j ) {
    double acc = 0.0;
    for ( usize i = 0; i < m; ++i ) acc = math::fma<double>(A[ssize_t(i) * rs_A + ssize_t(j)], x[i], acc);
    y[j] = math::fma<double>(alpha, acc, y[j]);
  }
}

#endif     // arm64 NEON

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::flatten]] inline void
gemv_t_panel_neon_f32(usize m, usize n, float alpha, const float *A, ssize_t rs_A, const float *x, float beta, float *y) noexcept
{
  constexpr usize NB = 16;     // 4 q-regs of 4 floats each
  constexpr usize MC = 256;
  alignas(16) float Ap[NB * MC];

  if ( beta == 0.0f ) {
    for ( usize j = 0; j < n; ++j ) y[j] = 0.0f;
  } else if ( beta != 1.0f ) {
    const float32x4_t vbeta = vdupq_n_f32(beta);
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) vst1q_f32(y + j, vmulq_f32(vbeta, vld1q_f32(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    float32x4_t y0 = vld1q_f32(y + j + 0);
    float32x4_t y1 = vld1q_f32(y + j + 4);
    float32x4_t y2 = vld1q_f32(y + j + 8);
    float32x4_t y3 = vld1q_f32(y + j + 12);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;
      for ( usize i = 0; i < mc; ++i ) {
        const float *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        vst1q_f32(Ap + i * NB + 0, vld1q_f32(src + 0));
        vst1q_f32(Ap + i * NB + 4, vld1q_f32(src + 4));
        vst1q_f32(Ap + i * NB + 8, vld1q_f32(src + 8));
        vst1q_f32(Ap + i * NB + 12, vld1q_f32(src + 12));
      }
      for ( usize i = 0; i < mc; ++i ) {
        const float32x4_t ax = vdupq_n_f32(alpha * x[ic + i]);
        const float *ap = Ap + i * NB;
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
        y0 = vfmaq_f32(y0, ax, vld1q_f32(ap + 0));
        y1 = vfmaq_f32(y1, ax, vld1q_f32(ap + 4));
        y2 = vfmaq_f32(y2, ax, vld1q_f32(ap + 8));
        y3 = vfmaq_f32(y3, ax, vld1q_f32(ap + 12));
#else
        y0 = vmlaq_f32(y0, ax, vld1q_f32(ap + 0));
        y1 = vmlaq_f32(y1, ax, vld1q_f32(ap + 4));
        y2 = vmlaq_f32(y2, ax, vld1q_f32(ap + 8));
        y3 = vmlaq_f32(y3, ax, vld1q_f32(ap + 12));
#endif
      }
    }

    vst1q_f32(y + j + 0, y0);
    vst1q_f32(y + j + 4, y1);
    vst1q_f32(y + j + 8, y2);
    vst1q_f32(y + j + 12, y3);
  }

  for ( ; j < n; ++j ) {
    float acc = 0.0f;
    for ( usize i = 0; i < m; ++i ) acc = math::fma<float>(A[ssize_t(i) * rs_A + ssize_t(j)], x[i], acc);
    y[j] = math::fma<float>(alpha, acc, y[j]);
  }
}

#endif     // arm any NEON

// %%%%%%%%%%%%%%%%%%%%%%%%%
// gemv_kernels
template <typename T>
[[gnu::flatten]] inline constexpr void
gemv_kernel(bool tr, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *x, ssize_t incx, T beta, T *y,
            ssize_t incy) noexcept
{
  // row-major A with tr=true would otherwise walk stride-ld columns of A
  if !consteval {
    if ( tr && cs_A == 1 && incx == 1 && incy == 1 ) {
      if constexpr ( ieee754_floating<T> ) {
#if defined(__AVX2__) && defined(__FMA__)
        if constexpr ( sizeof(T) == 8 ) {
          gemv_t_panel_avx2_f64(m, n, double(alpha), reinterpret_cast<const double *>(A), rs_A, reinterpret_cast<const double *>(x),
                                double(beta), reinterpret_cast<double *>(y));
          return;
        } else if constexpr ( sizeof(T) == 4 ) {
          gemv_t_panel_avx2_f32(m, n, float(alpha), reinterpret_cast<const float *>(A), rs_A, reinterpret_cast<const float *>(x),
                                float(beta), reinterpret_cast<float *>(y));
          return;
        }
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
        if constexpr ( sizeof(T) == 8 ) {
          gemv_t_panel_neon_f64(m, n, double(alpha), reinterpret_cast<const double *>(A), rs_A, reinterpret_cast<const double *>(x),
                                double(beta), reinterpret_cast<double *>(y));
          return;
        } else if constexpr ( sizeof(T) == 4 ) {
          gemv_t_panel_neon_f32(m, n, float(alpha), reinterpret_cast<const float *>(A), rs_A, reinterpret_cast<const float *>(x),
                                float(beta), reinterpret_cast<float *>(y));
          return;
        }
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
        if constexpr ( sizeof(T) == 4 ) {
          gemv_t_panel_neon_f32(m, n, float(alpha), reinterpret_cast<const float *>(A), rs_A, reinterpret_cast<const float *>(x),
                                float(beta), reinterpret_cast<float *>(y));
          return;
        }
#endif
      }
    }
  }

  const usize m_out = tr ? n : m;
  const usize n_in = tr ? m : n;
  const ssize_t step_inner = tr ? rs_A : cs_A;
  const ssize_t step_outer = tr ? cs_A : rs_A;
  const bool beta_zero = (beta == T(0));
  const bool beta_one = (beta == T(1));
  for ( usize i = 0; i < m_out; ++i ) {
    const T *row = A + ssize_t(i) * step_outer;
    // four accumulators
    T s0{}, s1{}, s2{}, s3{};
    usize j = 0;
    if ( incx == 1 && step_inner == 1 ) {
      for ( ; j + 4 <= n_in; j += 4 ) {
        s0 = fma_acc<T>(row[j + 0], x[j + 0], s0);
        s1 = fma_acc<T>(row[j + 1], x[j + 1], s1);
        s2 = fma_acc<T>(row[j + 2], x[j + 2], s2);
        s3 = fma_acc<T>(row[j + 3], x[j + 3], s3);
      }
    } else {
      for ( ; j + 4 <= n_in; j += 4 ) {
        s0 = fma_acc<T>(row[ssize_t(j + 0) * step_inner], x[ssize_t(j + 0) * incx], s0);
        s1 = fma_acc<T>(row[ssize_t(j + 1) * step_inner], x[ssize_t(j + 1) * incx], s1);
        s2 = fma_acc<T>(row[ssize_t(j + 2) * step_inner], x[ssize_t(j + 2) * incx], s2);
        s3 = fma_acc<T>(row[ssize_t(j + 3) * step_inner], x[ssize_t(j + 3) * incx], s3);
      }
    }
    T tail{};
    for ( ; j < n_in; ++j ) tail = fma_acc<T>(row[ssize_t(j) * step_inner], x[ssize_t(j) * incx], tail);
    const T acc = ((s0 + s1) + (s2 + s3)) + tail;
    T &yi = y[ssize_t(i) * incy];
    if ( beta_zero ) {
      yi = alpha * acc;
    } else if ( beta_one ) {
      yi = fma_acc<T>(alpha, acc, yi);
    } else {
      yi = fma_acc<T>(alpha, acc, beta * yi);
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ger_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
ger_kernel(usize m, usize n, T alpha, const T *x, ssize_t incx, const T *y, ssize_t incy, T *A, ssize_t rs_A, ssize_t cs_A) noexcept
{
  for ( usize i = 0; i < m; ++i ) {
    const T axi = alpha * x[ssize_t(i) * incx];
    for ( usize j = 0; j < n; ++j ) {
      T &aij = mat_at(A, i, j, rs_A, cs_A);
      aij = fma_acc<T>(axi, y[ssize_t(j) * incy], aij);
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// symv_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
symv_kernel(bool upper, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *x, ssize_t incx, T beta, T *y,
            ssize_t incy) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    T acc{};
    for ( usize j = 0; j < n; ++j ) {
      const usize r = (upper ? (i <= j ? i : j) : (i >= j ? i : j));
      const usize c = (upper ? (i <= j ? j : i) : (i >= j ? j : i));
      acc = fma_acc<T>(mat_at(A, r, c, rs_A, cs_A), x[ssize_t(j) * incx], acc);
    }
    T &yi = y[ssize_t(i) * incy];
    yi = fma_acc<T>(alpha, acc, beta * yi);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// syr_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
syr_kernel(bool upper, usize n, T alpha, const T *x, ssize_t incx, T *A, ssize_t rs_A, ssize_t cs_A) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    const T axi = alpha * x[ssize_t(i) * incx];
    if ( upper ) {
      for ( usize j = i; j < n; ++j ) {
        T &aij = mat_at(A, i, j, rs_A, cs_A);
        aij = fma_acc<T>(axi, x[ssize_t(j) * incx], aij);
      }
    } else {
      for ( usize j = 0; j <= i; ++j ) {
        T &aij = mat_at(A, i, j, rs_A, cs_A);
        aij = fma_acc<T>(axi, x[ssize_t(j) * incx], aij);
      }
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// syr2_kernel

template <typename T>
[[gnu::flatten]] inline constexpr void
syr2_kernel(bool upper, usize n, T alpha, const T *x, ssize_t incx, const T *y, ssize_t incy, T *A, ssize_t rs_A, ssize_t cs_A) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    const T axi = alpha * x[ssize_t(i) * incx];
    const T ayi = alpha * y[ssize_t(i) * incy];
    if ( upper ) {
      for ( usize j = i; j < n; ++j ) {
        T &aij = mat_at(A, i, j, rs_A, cs_A);
        aij = fma_acc<T>(axi, y[ssize_t(j) * incy], aij);
        aij = fma_acc<T>(ayi, x[ssize_t(j) * incx], aij);
      }
    } else {
      for ( usize j = 0; j <= i; ++j ) {
        T &aij = mat_at(A, i, j, rs_A, cs_A);
        aij = fma_acc<T>(axi, y[ssize_t(j) * incy], aij);
        aij = fma_acc<T>(ayi, x[ssize_t(j) * incx], aij);
      }
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trmv_kernel

template <typename T>
[[gnu::flatten]] inline constexpr void
trmv_kernel(bool upper, bool tr, bool unit_diag, usize n, const T *A, ssize_t rs_A, ssize_t cs_A, T *x, ssize_t incx) noexcept
{
  if ( !tr ) {
    if ( upper ) {
      for ( usize i = 0; i < n; ++i ) {
        T s{};
        const T diag = unit_diag ? T(1) : mat_at(A, i, i, rs_A, cs_A);
        s = diag * x[ssize_t(i) * incx];
        for ( usize j = i + 1; j < n; ++j ) s = fma_acc<T>(mat_at(A, i, j, rs_A, cs_A), x[ssize_t(j) * incx], s);
        x[ssize_t(i) * incx] = s;
      }
    } else {
      for ( usize ii = n; ii-- > 0; ) {
        T s{};
        const T diag = unit_diag ? T(1) : mat_at(A, ii, ii, rs_A, cs_A);
        s = diag * x[ssize_t(ii) * incx];
        for ( usize j = 0; j < ii; ++j ) s = fma_acc<T>(mat_at(A, ii, j, rs_A, cs_A), x[ssize_t(j) * incx], s);
        x[ssize_t(ii) * incx] = s;
      }
    }
  } else {
    if ( upper ) {
      for ( usize ii = n; ii-- > 0; ) {
        T s{};
        const T diag = unit_diag ? T(1) : mat_at(A, ii, ii, rs_A, cs_A);
        s = diag * x[ssize_t(ii) * incx];
        for ( usize j = 0; j < ii; ++j ) s = fma_acc<T>(mat_at(A, j, ii, rs_A, cs_A), x[ssize_t(j) * incx], s);
        x[ssize_t(ii) * incx] = s;
      }
    } else {
      for ( usize i = 0; i < n; ++i ) {
        T s{};
        const T diag = unit_diag ? T(1) : mat_at(A, i, i, rs_A, cs_A);
        s = diag * x[ssize_t(i) * incx];
        for ( usize j = i + 1; j < n; ++j ) s = fma_acc<T>(mat_at(A, j, i, rs_A, cs_A), x[ssize_t(j) * incx], s);
        x[ssize_t(i) * incx] = s;
      }
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trsv_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
trsv_kernel(bool upper, bool tr, bool unit_diag, usize n, const T *A, ssize_t rs_A, ssize_t cs_A, T *x, ssize_t incx) noexcept
{
  if ( !tr ) {
    if ( upper ) {
      for ( usize ii = n; ii-- > 0; ) {
        T s = x[ssize_t(ii) * incx];
        for ( usize j = ii + 1; j < n; ++j ) s -= mat_at(A, ii, j, rs_A, cs_A) * x[ssize_t(j) * incx];
        if ( !unit_diag ) s /= mat_at(A, ii, ii, rs_A, cs_A);
        x[ssize_t(ii) * incx] = s;
      }
    } else {
      for ( usize i = 0; i < n; ++i ) {
        T s = x[ssize_t(i) * incx];
        for ( usize j = 0; j < i; ++j ) s -= mat_at(A, i, j, rs_A, cs_A) * x[ssize_t(j) * incx];
        if ( !unit_diag ) s /= mat_at(A, i, i, rs_A, cs_A);
        x[ssize_t(i) * incx] = s;
      }
    }
  } else {
    if ( upper ) {
      for ( usize i = 0; i < n; ++i ) {
        T s = x[ssize_t(i) * incx];
        for ( usize j = 0; j < i; ++j ) s -= mat_at(A, j, i, rs_A, cs_A) * x[ssize_t(j) * incx];
        if ( !unit_diag ) s /= mat_at(A, i, i, rs_A, cs_A);
        x[ssize_t(i) * incx] = s;
      }
    } else {
      for ( usize ii = n; ii-- > 0; ) {
        T s = x[ssize_t(ii) * incx];
        for ( usize j = ii + 1; j < n; ++j ) s -= mat_at(A, j, ii, rs_A, cs_A) * x[ssize_t(j) * incx];
        if ( !unit_diag ) s /= mat_at(A, ii, ii, rs_A, cs_A);
        x[ssize_t(ii) * incx] = s;
      }
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// small 4×8 AVX2 microkernel
#if defined(__AVX2__) && defined(__FMA__)

[[gnu::flatten]] inline void
gemm_4x8_avx2_f64(usize m, usize n, usize k, double alpha, const double *A, ssize_t a_rs, ssize_t a_cs, const double *B, ssize_t b_rs,
                  ssize_t b_cs, double beta, double *C, ssize_t rs_C) noexcept
{
  alignas(32) double Bp[8 * 1024];     // stack panel for one 8-col B strip
  const __m256d valpha = _mm256_set1_pd(alpha);
  const bool beta_zero = (beta == 0.0);
  const bool beta_one = (beta == 1.0);
  const __m256d vbeta = _mm256_set1_pd(beta);

  for ( usize j = 0; j + 8 <= n; j += 8 ) {
    // pack B[0:k, j:j+8] into Bp
    // For NN (b_cs == 1) this is a contiguous copy
    // for NT/TT (b_cs != 1) it pays one stride traversal up front
    if ( b_cs == 1 ) {
      for ( usize p = 0; p < k; ++p ) {
        const double *src = B + ssize_t(p) * b_rs + ssize_t(j);
        _mm256_store_pd(Bp + p * 8 + 0, _mm256_loadu_pd(src + 0));
        _mm256_store_pd(Bp + p * 8 + 4, _mm256_loadu_pd(src + 4));
      }
    } else {
      for ( usize p = 0; p < k; ++p ) {
        const double *base = B + ssize_t(p) * b_rs + ssize_t(j) * b_cs;
        Bp[p * 8 + 0] = base[0 * b_cs];
        Bp[p * 8 + 1] = base[1 * b_cs];
        Bp[p * 8 + 2] = base[2 * b_cs];
        Bp[p * 8 + 3] = base[3 * b_cs];
        Bp[p * 8 + 4] = base[4 * b_cs];
        Bp[p * 8 + 5] = base[5 * b_cs];
        Bp[p * 8 + 6] = base[6 * b_cs];
        Bp[p * 8 + 7] = base[7 * b_cs];
      }
    }

    for ( usize i = 0; i + 4 <= m; i += 4 ) {
      const double *a0p = A + ssize_t(i + 0) * a_rs;
      const double *a1p = A + ssize_t(i + 1) * a_rs;
      const double *a2p = A + ssize_t(i + 2) * a_rs;
      const double *a3p = A + ssize_t(i + 3) * a_rs;

      __m256d c00 = _mm256_setzero_pd(), c01 = _mm256_setzero_pd();
      __m256d c10 = _mm256_setzero_pd(), c11 = _mm256_setzero_pd();
      __m256d c20 = _mm256_setzero_pd(), c21 = _mm256_setzero_pd();
      __m256d c30 = _mm256_setzero_pd(), c31 = _mm256_setzero_pd();
      for ( usize p = 0; p < k; ++p ) {
        const __m256d b0 = _mm256_load_pd(Bp + p * 8 + 0);
        const __m256d b1 = _mm256_load_pd(Bp + p * 8 + 4);
        const __m256d a0 = _mm256_set1_pd(a0p[ssize_t(p) * a_cs]);
        const __m256d a1 = _mm256_set1_pd(a1p[ssize_t(p) * a_cs]);
        c00 = _mm256_fmadd_pd(a0, b0, c00);
        c01 = _mm256_fmadd_pd(a0, b1, c01);
        c10 = _mm256_fmadd_pd(a1, b0, c10);
        c11 = _mm256_fmadd_pd(a1, b1, c11);
        const __m256d a2 = _mm256_set1_pd(a2p[ssize_t(p) * a_cs]);
        const __m256d a3 = _mm256_set1_pd(a3p[ssize_t(p) * a_cs]);
        c20 = _mm256_fmadd_pd(a2, b0, c20);
        c21 = _mm256_fmadd_pd(a2, b1, c21);
        c30 = _mm256_fmadd_pd(a3, b0, c30);
        c31 = _mm256_fmadd_pd(a3, b1, c31);
      }
      double *r0 = C + ssize_t(i + 0) * rs_C + ssize_t(j);
      double *r1 = C + ssize_t(i + 1) * rs_C + ssize_t(j);
      double *r2 = C + ssize_t(i + 2) * rs_C + ssize_t(j);
      double *r3 = C + ssize_t(i + 3) * rs_C + ssize_t(j);
      if ( beta_zero ) {
        _mm256_storeu_pd(r0 + 0, _mm256_mul_pd(valpha, c00));
        _mm256_storeu_pd(r0 + 4, _mm256_mul_pd(valpha, c01));
        _mm256_storeu_pd(r1 + 0, _mm256_mul_pd(valpha, c10));
        _mm256_storeu_pd(r1 + 4, _mm256_mul_pd(valpha, c11));
        _mm256_storeu_pd(r2 + 0, _mm256_mul_pd(valpha, c20));
        _mm256_storeu_pd(r2 + 4, _mm256_mul_pd(valpha, c21));
        _mm256_storeu_pd(r3 + 0, _mm256_mul_pd(valpha, c30));
        _mm256_storeu_pd(r3 + 4, _mm256_mul_pd(valpha, c31));
      } else if ( beta_one ) {
        _mm256_storeu_pd(r0 + 0, _mm256_fmadd_pd(valpha, c00, _mm256_loadu_pd(r0 + 0)));
        _mm256_storeu_pd(r0 + 4, _mm256_fmadd_pd(valpha, c01, _mm256_loadu_pd(r0 + 4)));
        _mm256_storeu_pd(r1 + 0, _mm256_fmadd_pd(valpha, c10, _mm256_loadu_pd(r1 + 0)));
        _mm256_storeu_pd(r1 + 4, _mm256_fmadd_pd(valpha, c11, _mm256_loadu_pd(r1 + 4)));
        _mm256_storeu_pd(r2 + 0, _mm256_fmadd_pd(valpha, c20, _mm256_loadu_pd(r2 + 0)));
        _mm256_storeu_pd(r2 + 4, _mm256_fmadd_pd(valpha, c21, _mm256_loadu_pd(r2 + 4)));
        _mm256_storeu_pd(r3 + 0, _mm256_fmadd_pd(valpha, c30, _mm256_loadu_pd(r3 + 0)));
        _mm256_storeu_pd(r3 + 4, _mm256_fmadd_pd(valpha, c31, _mm256_loadu_pd(r3 + 4)));
      } else {
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
  }
}

#endif     // AVX2 + FMA

// %%%%%%%%%%%%%%%%%%%%%%%%%
// gemm_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
gemm_kernel(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B, ssize_t rs_B,
            ssize_t cs_B, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  const ssize_t a_rs = trA ? cs_A : rs_A;
  const ssize_t a_cs = trA ? rs_A : cs_A;
  const ssize_t b_rs = trB ? cs_B : rs_B;
  const ssize_t b_cs = trB ? rs_B : cs_B;

  const bool beta_zero = (beta == T(0));
  const bool beta_one = (beta == T(1));

#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( sizeof(T) == 8 && ieee754_floating<T> ) {
      if ( cs_C == 1 && (m % 4u) == 0 && (n % 8u) == 0 && k > 0 && k <= 1024 ) {
        gemm_4x8_avx2_f64(m, n, k, double(alpha), reinterpret_cast<const double *>(A), a_rs, a_cs, reinterpret_cast<const double *>(B),
                          b_rs, b_cs, double(beta), reinterpret_cast<double *>(C), rs_C);
        return;
      }
    }
  }
#endif

  // BLIS-style blocked path for anything the inline kernel can't take
  if !consteval {
    if ( micron::math::matrix::pack::gemm_should_block_for_layout<T>(m, n, k, b_cs, cs_C) ) {
      micron::math::matrix::pack::gemm_blocked<T>(m, n, k, alpha, A, a_rs, a_cs, B, b_rs, b_cs, beta, C, rs_C, cs_C);
      return;
    }
  }

  // scalar fallback path
  for ( usize i = 0; i < m; ++i ) {
    T *c_row = C + ssize_t(i) * rs_C;
    const T *a_row = A + ssize_t(i) * a_rs;

    if ( beta_zero ) {
      if ( cs_C == 1 ) {
        for ( usize j = 0; j < n; ++j ) c_row[j] = T(0);
      } else {
        for ( usize j = 0; j < n; ++j ) c_row[ssize_t(j) * cs_C] = T(0);
      }
    } else if ( !beta_one ) {
      if ( cs_C == 1 ) {
        for ( usize j = 0; j < n; ++j ) c_row[j] *= beta;
      } else {
        for ( usize j = 0; j < n; ++j ) c_row[ssize_t(j) * cs_C] *= beta;
      }
    }

    for ( usize p = 0; p < k; ++p ) {
      const T a = alpha * a_row[ssize_t(p) * a_cs];
      const T *b_row = B + ssize_t(p) * b_rs;
      usize j = 0;
      if ( b_cs == 1 && cs_C == 1 ) {
        for ( ; j + 4 <= n; j += 4 ) {
          c_row[j + 0] = fma_acc<T>(a, b_row[j + 0], c_row[j + 0]);
          c_row[j + 1] = fma_acc<T>(a, b_row[j + 1], c_row[j + 1]);
          c_row[j + 2] = fma_acc<T>(a, b_row[j + 2], c_row[j + 2]);
          c_row[j + 3] = fma_acc<T>(a, b_row[j + 3], c_row[j + 3]);
        }
        for ( ; j < n; ++j ) c_row[j] = fma_acc<T>(a, b_row[j], c_row[j]);
      } else {
        for ( ; j + 4 <= n; j += 4 ) {
          T &c0 = c_row[ssize_t(j + 0) * cs_C];
          T &c1 = c_row[ssize_t(j + 1) * cs_C];
          T &c2 = c_row[ssize_t(j + 2) * cs_C];
          T &c3 = c_row[ssize_t(j + 3) * cs_C];
          c0 = fma_acc<T>(a, b_row[ssize_t(j + 0) * b_cs], c0);
          c1 = fma_acc<T>(a, b_row[ssize_t(j + 1) * b_cs], c1);
          c2 = fma_acc<T>(a, b_row[ssize_t(j + 2) * b_cs], c2);
          c3 = fma_acc<T>(a, b_row[ssize_t(j + 3) * b_cs], c3);
        }
        for ( ; j < n; ++j ) {
          T &cj = c_row[ssize_t(j) * cs_C];
          cj = fma_acc<T>(a, b_row[ssize_t(j) * b_cs], cj);
        }
      }
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trsm_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
trsm_left_kernel(bool upper, bool tr, bool unit_diag, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, T *B, ssize_t rs_B,
                 ssize_t cs_B) noexcept
{
  // pre-scale B by alpha
  if ( alpha != T(1) ) {
    for ( usize i = 0; i < m; ++i )
      for ( usize j = 0; j < n; ++j ) mat_at(B, i, j, rs_B, cs_B) *= alpha;
  }
  // solve column by column
  for ( usize j = 0; j < n; ++j ) {
    if ( !tr ) {
      if ( upper ) {
        for ( usize ii = m; ii-- > 0; ) {
          T s = mat_at(B, ii, j, rs_B, cs_B);
          for ( usize p = ii + 1; p < m; ++p ) s -= mat_at(A, ii, p, rs_A, cs_A) * mat_at(B, p, j, rs_B, cs_B);
          if ( !unit_diag ) s /= mat_at(A, ii, ii, rs_A, cs_A);
          mat_at(B, ii, j, rs_B, cs_B) = s;
        }
      } else {
        for ( usize i = 0; i < m; ++i ) {
          T s = mat_at(B, i, j, rs_B, cs_B);
          for ( usize p = 0; p < i; ++p ) s -= mat_at(A, i, p, rs_A, cs_A) * mat_at(B, p, j, rs_B, cs_B);
          if ( !unit_diag ) s /= mat_at(A, i, i, rs_A, cs_A);
          mat_at(B, i, j, rs_B, cs_B) = s;
        }
      }
    } else {
      if ( upper ) {
        for ( usize i = 0; i < m; ++i ) {
          T s = mat_at(B, i, j, rs_B, cs_B);
          for ( usize p = 0; p < i; ++p ) s -= mat_at(A, p, i, rs_A, cs_A) * mat_at(B, p, j, rs_B, cs_B);
          if ( !unit_diag ) s /= mat_at(A, i, i, rs_A, cs_A);
          mat_at(B, i, j, rs_B, cs_B) = s;
        }
      } else {
        for ( usize ii = m; ii-- > 0; ) {
          T s = mat_at(B, ii, j, rs_B, cs_B);
          for ( usize p = ii + 1; p < m; ++p ) s -= mat_at(A, p, ii, rs_A, cs_A) * mat_at(B, p, j, rs_B, cs_B);
          if ( !unit_diag ) s /= mat_at(A, ii, ii, rs_A, cs_A);
          mat_at(B, ii, j, rs_B, cs_B) = s;
        }
      }
    }
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
trsm_right_kernel(bool upper, bool tr, bool unit_diag, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, T *B,
                  ssize_t rs_B, ssize_t cs_B) noexcept
{
  if ( alpha != T(1) ) {
    for ( usize i = 0; i < m; ++i )
      for ( usize j = 0; j < n; ++j ) mat_at(B, i, j, rs_B, cs_B) *= alpha;
  }
  // solve row by row
  for ( usize i = 0; i < m; ++i ) {
    if ( !tr ) {
      if ( upper ) {
        for ( usize j = 0; j < n; ++j ) {
          T s = mat_at(B, i, j, rs_B, cs_B);
          for ( usize p = 0; p < j; ++p ) s -= mat_at(B, i, p, rs_B, cs_B) * mat_at(A, p, j, rs_A, cs_A);
          if ( !unit_diag ) s /= mat_at(A, j, j, rs_A, cs_A);
          mat_at(B, i, j, rs_B, cs_B) = s;
        }
      } else {
        for ( usize jj = n; jj-- > 0; ) {
          T s = mat_at(B, i, jj, rs_B, cs_B);
          for ( usize p = jj + 1; p < n; ++p ) s -= mat_at(B, i, p, rs_B, cs_B) * mat_at(A, p, jj, rs_A, cs_A);
          if ( !unit_diag ) s /= mat_at(A, jj, jj, rs_A, cs_A);
          mat_at(B, i, jj, rs_B, cs_B) = s;
        }
      }
    } else {
      if ( upper ) {
        for ( usize jj = n; jj-- > 0; ) {
          T s = mat_at(B, i, jj, rs_B, cs_B);
          for ( usize p = jj + 1; p < n; ++p ) s -= mat_at(B, i, p, rs_B, cs_B) * mat_at(A, jj, p, rs_A, cs_A);
          if ( !unit_diag ) s /= mat_at(A, jj, jj, rs_A, cs_A);
          mat_at(B, i, jj, rs_B, cs_B) = s;
        }
      } else {
        for ( usize j = 0; j < n; ++j ) {
          T s = mat_at(B, i, j, rs_B, cs_B);
          for ( usize p = 0; p < j; ++p ) s -= mat_at(B, i, p, rs_B, cs_B) * mat_at(A, j, p, rs_A, cs_A);
          if ( !unit_diag ) s /= mat_at(A, j, j, rs_A, cs_A);
          mat_at(B, i, j, rs_B, cs_B) = s;
        }
      }
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trmm_kernel

template <typename T>
[[gnu::flatten]] inline constexpr void
trmm_left_kernel(bool upper, bool tr, bool unit_diag, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, T *B, ssize_t rs_B,
                 ssize_t cs_B) noexcept
{
  for ( usize j = 0; j < n; ++j ) {
    if ( !tr ) {
      if ( upper ) {
        for ( usize i = 0; i < m; ++i ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, i, i, rs_A, cs_A);
          s = diag * mat_at(B, i, j, rs_B, cs_B);
          for ( usize p = i + 1; p < m; ++p ) s = fma_acc<T>(mat_at(A, i, p, rs_A, cs_A), mat_at(B, p, j, rs_B, cs_B), s);
          mat_at(B, i, j, rs_B, cs_B) = alpha * s;
        }
      } else {
        for ( usize ii = m; ii-- > 0; ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, ii, ii, rs_A, cs_A);
          s = diag * mat_at(B, ii, j, rs_B, cs_B);
          for ( usize p = 0; p < ii; ++p ) s = fma_acc<T>(mat_at(A, ii, p, rs_A, cs_A), mat_at(B, p, j, rs_B, cs_B), s);
          mat_at(B, ii, j, rs_B, cs_B) = alpha * s;
        }
      }
    } else {
      if ( upper ) {
        for ( usize ii = m; ii-- > 0; ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, ii, ii, rs_A, cs_A);
          s = diag * mat_at(B, ii, j, rs_B, cs_B);
          for ( usize p = 0; p < ii; ++p ) s = fma_acc<T>(mat_at(A, p, ii, rs_A, cs_A), mat_at(B, p, j, rs_B, cs_B), s);
          mat_at(B, ii, j, rs_B, cs_B) = alpha * s;
        }
      } else {
        for ( usize i = 0; i < m; ++i ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, i, i, rs_A, cs_A);
          s = diag * mat_at(B, i, j, rs_B, cs_B);
          for ( usize p = i + 1; p < m; ++p ) s = fma_acc<T>(mat_at(A, p, i, rs_A, cs_A), mat_at(B, p, j, rs_B, cs_B), s);
          mat_at(B, i, j, rs_B, cs_B) = alpha * s;
        }
      }
    }
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
trmm_right_kernel(bool upper, bool tr, bool unit_diag, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, T *B,
                  ssize_t rs_B, ssize_t cs_B) noexcept
{
  for ( usize i = 0; i < m; ++i ) {
    if ( !tr ) {
      if ( upper ) {
        for ( usize jj = n; jj-- > 0; ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, jj, jj, rs_A, cs_A);
          s = mat_at(B, i, jj, rs_B, cs_B) * diag;
          for ( usize p = 0; p < jj; ++p ) s = fma_acc<T>(mat_at(B, i, p, rs_B, cs_B), mat_at(A, p, jj, rs_A, cs_A), s);
          mat_at(B, i, jj, rs_B, cs_B) = alpha * s;
        }
      } else {
        for ( usize j = 0; j < n; ++j ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, j, j, rs_A, cs_A);
          s = mat_at(B, i, j, rs_B, cs_B) * diag;
          for ( usize p = j + 1; p < n; ++p ) s = fma_acc<T>(mat_at(B, i, p, rs_B, cs_B), mat_at(A, p, j, rs_A, cs_A), s);
          mat_at(B, i, j, rs_B, cs_B) = alpha * s;
        }
      }
    } else {
      if ( upper ) {
        for ( usize j = 0; j < n; ++j ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, j, j, rs_A, cs_A);
          s = mat_at(B, i, j, rs_B, cs_B) * diag;
          for ( usize p = j + 1; p < n; ++p ) s = fma_acc<T>(mat_at(B, i, p, rs_B, cs_B), mat_at(A, j, p, rs_A, cs_A), s);
          mat_at(B, i, j, rs_B, cs_B) = alpha * s;
        }
      } else {
        for ( usize jj = n; jj-- > 0; ) {
          T s{};
          const T diag = unit_diag ? T(1) : mat_at(A, jj, jj, rs_A, cs_A);
          s = mat_at(B, i, jj, rs_B, cs_B) * diag;
          for ( usize p = 0; p < jj; ++p ) s = fma_acc<T>(mat_at(B, i, p, rs_B, cs_B), mat_at(A, jj, p, rs_A, cs_A), s);
          mat_at(B, i, jj, rs_B, cs_B) = alpha * s;
        }
      }
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// syrk_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
syrk_kernel(bool upper, bool tr, usize n, usize k, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, T beta, T *C, ssize_t rs_C,
            ssize_t cs_C) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    const usize j_lo = upper ? i : 0;
    const usize j_hi = upper ? n : (i + 1);
    for ( usize j = j_lo; j < j_hi; ++j ) {
      T acc{};
      for ( usize p = 0; p < k; ++p ) {
        const T a = tr ? mat_at(A, p, i, rs_A, cs_A) : mat_at(A, i, p, rs_A, cs_A);
        const T b = tr ? mat_at(A, p, j, rs_A, cs_A) : mat_at(A, j, p, rs_A, cs_A);
        acc = fma_acc<T>(a, b, acc);
      }
      T &cij = mat_at(C, i, j, rs_C, cs_C);
      cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// syr2k_kernel
template <typename T>
[[gnu::flatten]] inline constexpr void
syr2k_kernel(bool upper, bool tr, usize n, usize k, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B, ssize_t rs_B, ssize_t cs_B,
             T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    const usize j_lo = upper ? i : 0;
    const usize j_hi = upper ? n : (i + 1);
    for ( usize j = j_lo; j < j_hi; ++j ) {
      T acc{};
      for ( usize p = 0; p < k; ++p ) {
        const T ai = tr ? mat_at(A, p, i, rs_A, cs_A) : mat_at(A, i, p, rs_A, cs_A);
        const T bj = tr ? mat_at(B, p, j, rs_B, cs_B) : mat_at(B, j, p, rs_B, cs_B);
        const T aj = tr ? mat_at(A, p, j, rs_A, cs_A) : mat_at(A, j, p, rs_A, cs_A);
        const T bi = tr ? mat_at(B, p, i, rs_B, cs_B) : mat_at(B, i, p, rs_B, cs_B);
        acc = fma_acc<T>(ai, bj, acc);
        acc = fma_acc<T>(aj, bi, acc);
      }
      T &cij = mat_at(C, i, j, rs_C, cs_C);
      cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
symm_left_kernel(bool upper, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B, ssize_t rs_B, ssize_t cs_B,
                 T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  for ( usize i = 0; i < m; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      T acc{};
      for ( usize p = 0; p < m; ++p ) {
        const usize r = (upper ? (i <= p ? i : p) : (i >= p ? i : p));
        const usize c = (upper ? (i <= p ? p : i) : (i >= p ? p : i));
        acc = fma_acc<T>(mat_at(A, r, c, rs_A, cs_A), mat_at(B, p, j, rs_B, cs_B), acc);
      }
      T &cij = mat_at(C, i, j, rs_C, cs_C);
      cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
symm_right_kernel(bool upper, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B, ssize_t rs_B, ssize_t cs_B,
                  T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  for ( usize i = 0; i < m; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      T acc{};
      for ( usize p = 0; p < n; ++p ) {
        const usize r = (upper ? (p <= j ? p : j) : (p >= j ? p : j));
        const usize c = (upper ? (p <= j ? j : p) : (p >= j ? j : p));
        acc = fma_acc<T>(mat_at(B, i, p, rs_B, cs_B), mat_at(A, r, c, rs_A, cs_A), acc);
      }
      T &cij = mat_at(C, i, j, rs_C, cs_C);
      cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

};     // namespace bits
};     // namespace blas
};     // namespace math
};     // namespace micron
