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

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)
#include "../../../simd/aliases.hpp"
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
template<typename T>
[[nodiscard, gnu::always_inline]] inline constexpr T &
mat_at(T *data, usize i, usize j, ssize_t rs, ssize_t cs) noexcept
{
  return data[ssize_t(i) * rs + ssize_t(j) * cs];
}

template<typename T>
[[nodiscard, gnu::always_inline]] inline constexpr const T &
mat_at(const T *data, usize i, usize j, ssize_t rs, ssize_t cs) noexcept
{
  return data[ssize_t(i) * rs + ssize_t(j) * cs];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fma_accs (c = c + a * b)
template<typename T>
[[gnu::always_inline]] inline constexpr T
fma_acc(T a, T b, T c) noexcept
{
  if constexpr ( ieee754_floating<T> )
    return math::fma<T>(a, b, c);
  else
    return c + a * b;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gemv no-transpose panel kernels
// y[i] = beta*y[i] + alpha * sum_j A[i,j] * x[j]   for row-major A
#if defined(__AVX2__) && defined(__FMA__)

[[gnu::flatten, gnu::hot]] inline void
gemv_n_panel_avx2_f64(usize m, usize n, double alpha, const double *A, ssize_t rs_A, const double *x, double beta, double *y) noexcept
{
  if ( beta == 0.0 ) {
    const __m256d z = simd::avx::zero_f64();
    usize i = 0;
    for ( ; i + 4 <= m; i += 4 ) simd::avx::storeu_f64(y + i, z);
    for ( ; i < m; ++i ) y[i] = 0.0;
  } else if ( beta != 1.0 ) {
    const __m256d vbeta = simd::avx::splat_f64(beta);
    usize i = 0;
    for ( ; i + 4 <= m; i += 4 ) simd::avx::storeu_f64(y + i, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(y + i)));
    for ( ; i < m; ++i ) y[i] *= beta;
  }

  const __m256d valpha = simd::avx::splat_f64(alpha);
  usize i = 0;
  // 4-row dot-product fan-out: one x-load shared across 4 row-loads + 4 FMAs
  for ( ; i + 4 <= m; i += 4 ) {
    __m256d acc0 = simd::avx::zero_f64();
    __m256d acc1 = simd::avx::zero_f64();
    __m256d acc2 = simd::avx::zero_f64();
    __m256d acc3 = simd::avx::zero_f64();
    const double *r0 = A + ssize_t(i + 0) * rs_A;
    const double *r1 = A + ssize_t(i + 1) * rs_A;
    const double *r2 = A + ssize_t(i + 2) * rs_A;
    const double *r3 = A + ssize_t(i + 3) * rs_A;
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) {
      const __m256d xv = simd::avx::loadu_f64(x + j);
      acc0 = simd::fma::fma_f64(simd::avx::loadu_f64(r0 + j), xv, acc0);
      acc1 = simd::fma::fma_f64(simd::avx::loadu_f64(r1 + j), xv, acc1);
      acc2 = simd::fma::fma_f64(simd::avx::loadu_f64(r2 + j), xv, acc2);
      acc3 = simd::fma::fma_f64(simd::avx::loadu_f64(r3 + j), xv, acc3);
    }
    const __m256d h01 = simd::avx::hadd_f64(acc0, acc1);
    const __m256d h23 = simd::avx::hadd_f64(acc2, acc3);
    const __m256d lo = simd::avx::permute2f128_f64<0x20>(h01, h23);
    const __m256d hi = simd::avx::permute2f128_f64<0x31>(h01, h23);
    __m256d sums = simd::avx::add_f64(lo, hi);

    // j-tail (n%4)
    if ( j < n ) {
      alignas(32) double s[4];
      simd::avx::store_f64(s, sums);
      for ( usize jj = j; jj < n; ++jj ) {
        const double xv = x[jj];
        s[0] = math::fma<double>(r0[jj], xv, s[0]);
        s[1] = math::fma<double>(r1[jj], xv, s[1]);
        s[2] = math::fma<double>(r2[jj], xv, s[2]);
        s[3] = math::fma<double>(r3[jj], xv, s[3]);
      }
      sums = simd::avx::load_f64(s);
    }

    const __m256d yv = simd::avx::loadu_f64(y + i);
    simd::avx::storeu_f64(y + i, simd::fma::fma_f64(valpha, sums, yv));
  }
  // i-tail
  for ( ; i < m; ++i ) {
    __m256d acc = simd::avx::zero_f64();
    const double *row = A + ssize_t(i) * rs_A;
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) {
      acc = simd::fma::fma_f64(simd::avx::loadu_f64(row + j), simd::avx::loadu_f64(x + j), acc);
    }
    const __m128d lo = simd::avx::cast_f64_to_lo128(acc);
    const __m128d hi = simd::avx::extract_f128_f64<1>(acc);
    const __m128d s2 = simd::sse::add_f64(lo, hi);
    double tail = simd::sse::extract_low_f64(simd::sse::hadd_f64(s2, s2));
    for ( ; j < n; ++j ) tail = math::fma<double>(row[j], x[j], tail);
    y[i] = math::fma<double>(alpha, tail, y[i]);
  }
}

[[gnu::flatten, gnu::hot]] inline void
gemv_n_panel_avx2_f32(usize m, usize n, float alpha, const float *A, ssize_t rs_A, const float *x, float beta, float *y) noexcept
{
  if ( beta == 0.0f ) {
    const __m256 z = simd::avx::zero_f32();
    usize i = 0;
    for ( ; i + 8 <= m; i += 8 ) simd::avx::storeu_f32(y + i, z);
    for ( ; i < m; ++i ) y[i] = 0.0f;
  } else if ( beta != 1.0f ) {
    const __m256 vbeta = simd::avx::splat_f32(beta);
    usize i = 0;
    for ( ; i + 8 <= m; i += 8 ) simd::avx::storeu_f32(y + i, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(y + i)));
    for ( ; i < m; ++i ) y[i] *= beta;
  }

  usize i = 0;
  for ( ; i + 4 <= m; i += 4 ) {
    __m256 acc0 = simd::avx::zero_f32();
    __m256 acc1 = simd::avx::zero_f32();
    __m256 acc2 = simd::avx::zero_f32();
    __m256 acc3 = simd::avx::zero_f32();
    const float *r0 = A + ssize_t(i + 0) * rs_A;
    const float *r1 = A + ssize_t(i + 1) * rs_A;
    const float *r2 = A + ssize_t(i + 2) * rs_A;
    const float *r3 = A + ssize_t(i + 3) * rs_A;
    usize j = 0;
    for ( ; j + 8 <= n; j += 8 ) {
      const __m256 xv = simd::avx::loadu_f32(x + j);
      acc0 = simd::fma::fma_f32(simd::avx::loadu_f32(r0 + j), xv, acc0);
      acc1 = simd::fma::fma_f32(simd::avx::loadu_f32(r1 + j), xv, acc1);
      acc2 = simd::fma::fma_f32(simd::avx::loadu_f32(r2 + j), xv, acc2);
      acc3 = simd::fma::fma_f32(simd::avx::loadu_f32(r3 + j), xv, acc3);
    }
    auto reduce8 = [](__m256 v) -> float {
      __m128 lo = simd::avx::cast_f32_to_lo128(v);
      __m128 hi = simd::avx::extract_f128_f32<1>(v);
      __m128 s4 = simd::sse::add_f32(lo, hi);
      __m128 s2 = simd::sse::hadd_f32(s4, s4);
      __m128 s1 = simd::sse::hadd_f32(s2, s2);
      return simd::sse::extract_low_f32(s1);
    };
    float s0 = reduce8(acc0), s1 = reduce8(acc1);
    float s2 = reduce8(acc2), s3 = reduce8(acc3);
    for ( ; j < n; ++j ) {
      const float xv = x[j];
      s0 = math::fma<float>(r0[j], xv, s0);
      s1 = math::fma<float>(r1[j], xv, s1);
      s2 = math::fma<float>(r2[j], xv, s2);
      s3 = math::fma<float>(r3[j], xv, s3);
    }
    y[i + 0] = math::fma<float>(alpha, s0, y[i + 0]);
    y[i + 1] = math::fma<float>(alpha, s1, y[i + 1]);
    y[i + 2] = math::fma<float>(alpha, s2, y[i + 2]);
    y[i + 3] = math::fma<float>(alpha, s3, y[i + 3]);
  }
  for ( ; i < m; ++i ) {
    float acc = 0.0f;
    for ( usize j = 0; j < n; ++j ) acc = math::fma<float>(A[ssize_t(i) * rs_A + ssize_t(j)], x[j], acc);
    y[i] = math::fma<float>(alpha, acc, y[i]);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gemv transpose panel-packed kernels

[[gnu::flatten]] inline void
gemv_t_panel_avx2_f64(usize m, usize n, double alpha, const double *A, ssize_t rs_A, const double *x, double beta, double *y) noexcept
{
  constexpr usize NB = 16;       // j-strip width (4 ymm of 4 doubles each)
  constexpr usize MC = 128;      // m-block height; NB*MC doubles = 16 KiB pack, fits L1
  alignas(32) double Ap[NB * MC];

  if ( beta == 0.0 ) {
    const __m256d z = simd::avx::zero_f64();
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) simd::avx::storeu_f64(y + j, z);
    for ( ; j < n; ++j ) y[j] = 0.0;
  } else if ( beta != 1.0 ) {
    const __m256d vbeta = simd::avx::splat_f64(beta);
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) simd::avx::storeu_f64(y + j, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    __m256d y0 = simd::avx::loadu_f64(y + j + 0);
    __m256d y1 = simd::avx::loadu_f64(y + j + 4);
    __m256d y2 = simd::avx::loadu_f64(y + j + 8);
    __m256d y3 = simd::avx::loadu_f64(y + j + 12);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;

      for ( usize i = 0; i < mc; ++i ) {
        const double *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        // Prefetch the next row 8 ahead
        if ( i + 8 < mc ) {
          __builtin_prefetch(A + ssize_t(ic + i + 8) * rs_A + ssize_t(j), 0, 1);
        }
        simd::avx::store_f64(Ap + i * NB + 0, simd::avx::loadu_f64(src + 0));
        simd::avx::store_f64(Ap + i * NB + 4, simd::avx::loadu_f64(src + 4));
        simd::avx::store_f64(Ap + i * NB + 8, simd::avx::loadu_f64(src + 8));
        simd::avx::store_f64(Ap + i * NB + 12, simd::avx::loadu_f64(src + 12));
      }

      for ( usize i = 0; i < mc; ++i ) {
        const __m256d ax = simd::avx::splat_f64(alpha * x[ic + i]);
        const double *ap = Ap + i * NB;
        y0 = simd::fma::fma_f64(ax, simd::avx::load_f64(ap + 0), y0);
        y1 = simd::fma::fma_f64(ax, simd::avx::load_f64(ap + 4), y1);
        y2 = simd::fma::fma_f64(ax, simd::avx::load_f64(ap + 8), y2);
        y3 = simd::fma::fma_f64(ax, simd::avx::load_f64(ap + 12), y3);
      }
    }

    simd::avx::storeu_f64(y + j + 0, y0);
    simd::avx::storeu_f64(y + j + 4, y1);
    simd::avx::storeu_f64(y + j + 8, y2);
    simd::avx::storeu_f64(y + j + 12, y3);
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
  constexpr usize NB = 32;       // 4 ymm of 8 floats each
  constexpr usize MC = 128;      // pack 32 * 128 * 4 = 16 KiB
  alignas(32) float Ap[NB * MC];

  if ( beta == 0.0f ) {
    const __m256 z = simd::avx::zero_f32();
    usize j = 0;
    for ( ; j + 8 <= n; j += 8 ) simd::avx::storeu_f32(y + j, z);
    for ( ; j < n; ++j ) y[j] = 0.0f;
  } else if ( beta != 1.0f ) {
    const __m256 vbeta = simd::avx::splat_f32(beta);
    usize j = 0;
    for ( ; j + 8 <= n; j += 8 ) simd::avx::storeu_f32(y + j, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    __m256 y0 = simd::avx::loadu_f32(y + j + 0);
    __m256 y1 = simd::avx::loadu_f32(y + j + 8);
    __m256 y2 = simd::avx::loadu_f32(y + j + 16);
    __m256 y3 = simd::avx::loadu_f32(y + j + 24);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;
      for ( usize i = 0; i < mc; ++i ) {
        const float *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        simd::avx::store_f32(Ap + i * NB + 0, simd::avx::loadu_f32(src + 0));
        simd::avx::store_f32(Ap + i * NB + 8, simd::avx::loadu_f32(src + 8));
        simd::avx::store_f32(Ap + i * NB + 16, simd::avx::loadu_f32(src + 16));
        simd::avx::store_f32(Ap + i * NB + 24, simd::avx::loadu_f32(src + 24));
      }
      for ( usize i = 0; i < mc; ++i ) {
        const __m256 ax = simd::avx::splat_f32(alpha * x[ic + i]);
        const float *ap = Ap + i * NB;
        y0 = simd::fma::fma_f32(ax, simd::avx::load_f32(ap + 0), y0);
        y1 = simd::fma::fma_f32(ax, simd::avx::load_f32(ap + 8), y1);
        y2 = simd::fma::fma_f32(ax, simd::avx::load_f32(ap + 16), y2);
        y3 = simd::fma::fma_f32(ax, simd::avx::load_f32(ap + 24), y3);
      }
    }

    simd::avx::storeu_f32(y + j + 0, y0);
    simd::avx::storeu_f32(y + j + 8, y1);
    simd::avx::storeu_f32(y + j + 16, y2);
    simd::avx::storeu_f32(y + j + 24, y3);
  }

  for ( ; j < n; ++j ) {
    float acc = 0.0f;
    for ( usize i = 0; i < m; ++i ) acc = math::fma<float>(A[ssize_t(i) * rs_A + ssize_t(j)], x[i], acc);
    y[j] = math::fma<float>(alpha, acc, y[j]);
  }
}

#endif      // AVX2 + FMA

#if defined(__micron_arch_arm64) && defined(__micron_arm_neon)

[[gnu::flatten]] inline void
gemv_t_panel_neon_f64(usize m, usize n, double alpha, const double *A, ssize_t rs_A, const double *x, double beta, double *y) noexcept
{
  constexpr usize NB = 8;      // 4 q-regs of 2 doubles each
  constexpr usize MC = 256;
  alignas(16) double Ap[NB * MC];

  if ( beta == 0.0 ) {
    for ( usize j = 0; j < n; ++j ) y[j] = 0.0;
  } else if ( beta != 1.0 ) {
    const float64x2_t vbeta = simd::neon::splat_f64(beta);
    usize j = 0;
    for ( ; j + 2 <= n; j += 2 ) simd::neon::store_f64(y + j, simd::neon::mul(vbeta, simd::neon::load_f64(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    float64x2_t y0 = simd::neon::load_f64(y + j + 0);
    float64x2_t y1 = simd::neon::load_f64(y + j + 2);
    float64x2_t y2 = simd::neon::load_f64(y + j + 4);
    float64x2_t y3 = simd::neon::load_f64(y + j + 6);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;
      for ( usize i = 0; i < mc; ++i ) {
        const double *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        simd::neon::store_f64(Ap + i * NB + 0, simd::neon::load_f64(src + 0));
        simd::neon::store_f64(Ap + i * NB + 2, simd::neon::load_f64(src + 2));
        simd::neon::store_f64(Ap + i * NB + 4, simd::neon::load_f64(src + 4));
        simd::neon::store_f64(Ap + i * NB + 6, simd::neon::load_f64(src + 6));
      }
      for ( usize i = 0; i < mc; ++i ) {
        const float64x2_t ax = simd::neon::splat_f64(alpha * x[ic + i]);
        const double *ap = Ap + i * NB;
        y0 = simd::neon::fma_f64(y0, ax, simd::neon::load_f64(ap + 0));
        y1 = simd::neon::fma_f64(y1, ax, simd::neon::load_f64(ap + 2));
        y2 = simd::neon::fma_f64(y2, ax, simd::neon::load_f64(ap + 4));
        y3 = simd::neon::fma_f64(y3, ax, simd::neon::load_f64(ap + 6));
      }
    }

    simd::neon::store_f64(y + j + 0, y0);
    simd::neon::store_f64(y + j + 2, y1);
    simd::neon::store_f64(y + j + 4, y2);
    simd::neon::store_f64(y + j + 6, y3);
  }

  for ( ; j < n; ++j ) {
    double acc = 0.0;
    for ( usize i = 0; i < m; ++i ) acc = math::fma<double>(A[ssize_t(i) * rs_A + ssize_t(j)], x[i], acc);
    y[j] = math::fma<double>(alpha, acc, y[j]);
  }
}

#endif      // arm64 NEON

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::flatten]] inline void
gemv_t_panel_neon_f32(usize m, usize n, float alpha, const float *A, ssize_t rs_A, const float *x, float beta, float *y) noexcept
{
  constexpr usize NB = 16;      // 4 q-regs of 4 floats each
  constexpr usize MC = 256;
  alignas(16) float Ap[NB * MC];

  if ( beta == 0.0f ) {
    for ( usize j = 0; j < n; ++j ) y[j] = 0.0f;
  } else if ( beta != 1.0f ) {
    const float32x4_t vbeta = simd::neon::splat_f32(beta);
    usize j = 0;
    for ( ; j + 4 <= n; j += 4 ) simd::neon::store_f32(y + j, simd::neon::mul(vbeta, simd::neon::load_f32(y + j)));
    for ( ; j < n; ++j ) y[j] *= beta;
  }

  usize j = 0;
  for ( ; j + NB <= n; j += NB ) {
    float32x4_t y0 = simd::neon::load_f32(y + j + 0);
    float32x4_t y1 = simd::neon::load_f32(y + j + 4);
    float32x4_t y2 = simd::neon::load_f32(y + j + 8);
    float32x4_t y3 = simd::neon::load_f32(y + j + 12);

    for ( usize ic = 0; ic < m; ic += MC ) {
      const usize mc = (m - ic < MC) ? (m - ic) : MC;
      for ( usize i = 0; i < mc; ++i ) {
        const float *src = A + ssize_t(ic + i) * rs_A + ssize_t(j);
        simd::neon::store_f32(Ap + i * NB + 0, simd::neon::load_f32(src + 0));
        simd::neon::store_f32(Ap + i * NB + 4, simd::neon::load_f32(src + 4));
        simd::neon::store_f32(Ap + i * NB + 8, simd::neon::load_f32(src + 8));
        simd::neon::store_f32(Ap + i * NB + 12, simd::neon::load_f32(src + 12));
      }
      for ( usize i = 0; i < mc; ++i ) {
        const float32x4_t ax = simd::neon::splat_f32(alpha * x[ic + i]);
        const float *ap = Ap + i * NB;
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
        y0 = simd::neon::fma_f32(y0, ax, simd::neon::load_f32(ap + 0));
        y1 = simd::neon::fma_f32(y1, ax, simd::neon::load_f32(ap + 4));
        y2 = simd::neon::fma_f32(y2, ax, simd::neon::load_f32(ap + 8));
        y3 = simd::neon::fma_f32(y3, ax, simd::neon::load_f32(ap + 12));
#else
        y0 = simd::neon::mla(y0, ax, simd::neon::load_f32(ap + 0));
        y1 = simd::neon::mla(y1, ax, simd::neon::load_f32(ap + 4));
        y2 = simd::neon::mla(y2, ax, simd::neon::load_f32(ap + 8));
        y3 = simd::neon::mla(y3, ax, simd::neon::load_f32(ap + 12));
#endif
      }
    }

    simd::neon::store_f32(y + j + 0, y0);
    simd::neon::store_f32(y + j + 4, y1);
    simd::neon::store_f32(y + j + 8, y2);
    simd::neon::store_f32(y + j + 12, y3);
  }

  for ( ; j < n; ++j ) {
    float acc = 0.0f;
    for ( usize i = 0; i < m; ++i ) acc = math::fma<float>(A[ssize_t(i) * rs_A + ssize_t(j)], x[i], acc);
    y[j] = math::fma<float>(alpha, acc, y[j]);
  }
}

#endif      // arm any NEON

// %%%%%%%%%%%%%%%%%%%%%%%%%
// gemv_kernels
template<typename T>
[[gnu::flatten]] inline constexpr void
gemv_kernel(bool tr, usize m, usize n, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *x, ssize_t incx, T beta, T *y,
            ssize_t incy) noexcept
{
  // row-major A panel kernels: tr-case uses axpy fan-out; no-trans uses
  // dot-product fan-out across 4 consecutive rows.
  if !consteval {
    if ( cs_A == 1 && incx == 1 && incy == 1 ) {
      if constexpr ( ieee754_floating<T> ) {
#if defined(__AVX2__) && defined(__FMA__)
        if constexpr ( sizeof(T) == 8 ) {
          if ( tr ) {
            gemv_t_panel_avx2_f64(m, n, double(alpha), reinterpret_cast<const double *>(A), rs_A, reinterpret_cast<const double *>(x),
                                  double(beta), reinterpret_cast<double *>(y));
          } else {
            gemv_n_panel_avx2_f64(m, n, double(alpha), reinterpret_cast<const double *>(A), rs_A, reinterpret_cast<const double *>(x),
                                  double(beta), reinterpret_cast<double *>(y));
          }
          return;
        } else if constexpr ( sizeof(T) == 4 ) {
          if ( tr ) {
            gemv_t_panel_avx2_f32(m, n, float(alpha), reinterpret_cast<const float *>(A), rs_A, reinterpret_cast<const float *>(x),
                                  float(beta), reinterpret_cast<float *>(y));
          } else {
            gemv_n_panel_avx2_f32(m, n, float(alpha), reinterpret_cast<const float *>(A), rs_A, reinterpret_cast<const float *>(x),
                                  float(beta), reinterpret_cast<float *>(y));
          }
          return;
        }
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
        if ( tr ) {
          if constexpr ( sizeof(T) == 8 ) {
            gemv_t_panel_neon_f64(m, n, double(alpha), reinterpret_cast<const double *>(A), rs_A, reinterpret_cast<const double *>(x),
                                  double(beta), reinterpret_cast<double *>(y));
            return;
          } else if constexpr ( sizeof(T) == 4 ) {
            gemv_t_panel_neon_f32(m, n, float(alpha), reinterpret_cast<const float *>(A), rs_A, reinterpret_cast<const float *>(x),
                                  float(beta), reinterpret_cast<float *>(y));
            return;
          }
        }
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
        if ( tr ) {
          if constexpr ( sizeof(T) == 4 ) {
            gemv_t_panel_neon_f32(m, n, float(alpha), reinterpret_cast<const float *>(A), rs_A, reinterpret_cast<const float *>(x),
                                  float(beta), reinterpret_cast<float *>(y));
            return;
          }
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
template<typename T>
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
template<typename T>
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
    if ( beta == T(0) )
      yi = alpha * acc;
    else if ( beta == T(1) )
      yi = fma_acc<T>(alpha, acc, yi);
    else
      yi = fma_acc<T>(alpha, acc, beta * yi);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// syr_kernel
template<typename T>
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

template<typename T>
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

template<typename T>
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
template<typename T>
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
// small 4×8 AVX2 dgemm microkernel
#if defined(__AVX2__) && defined(__FMA__)

[[gnu::flatten]] inline void
gemm_4x8_avx2_f64(usize m, usize n, usize k, double alpha, const double *A, ssize_t a_rs, ssize_t a_cs, const double *B, ssize_t b_rs,
                  ssize_t b_cs, double beta, double *C, ssize_t rs_C) noexcept
{
  alignas(32) double Bp[8 * 1024];      // stack panel for one 8-col B strip
  const __m256d valpha = simd::avx::splat_f64(alpha);
  const bool beta_zero = (beta == 0.0);
  const bool beta_one = (beta == 1.0);
  const __m256d vbeta = simd::avx::splat_f64(beta);

  for ( usize j = 0; j + 8 <= n; j += 8 ) {
    // pack B[0:k, j:j+8] into Bp
    if ( b_cs == 1 ) {
      for ( usize p = 0; p < k; ++p ) {
        const double *src = B + ssize_t(p) * b_rs + ssize_t(j);
        simd::avx::store_f64(Bp + p * 8 + 0, simd::avx::loadu_f64(src + 0));
        simd::avx::store_f64(Bp + p * 8 + 4, simd::avx::loadu_f64(src + 4));
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

      __m256d c00 = simd::avx::zero_f64(), c01 = simd::avx::zero_f64();
      __m256d c10 = simd::avx::zero_f64(), c11 = simd::avx::zero_f64();
      __m256d c20 = simd::avx::zero_f64(), c21 = simd::avx::zero_f64();
      __m256d c30 = simd::avx::zero_f64(), c31 = simd::avx::zero_f64();
      for ( usize p = 0; p < k; ++p ) {
        const __m256d b0 = simd::avx::load_f64(Bp + p * 8 + 0);
        const __m256d b1 = simd::avx::load_f64(Bp + p * 8 + 4);
        const __m256d a0 = simd::avx::splat_f64(a0p[ssize_t(p) * a_cs]);
        const __m256d a1 = simd::avx::splat_f64(a1p[ssize_t(p) * a_cs]);
        c00 = simd::fma::fma_f64(a0, b0, c00);
        c01 = simd::fma::fma_f64(a0, b1, c01);
        c10 = simd::fma::fma_f64(a1, b0, c10);
        c11 = simd::fma::fma_f64(a1, b1, c11);
        const __m256d a2 = simd::avx::splat_f64(a2p[ssize_t(p) * a_cs]);
        const __m256d a3 = simd::avx::splat_f64(a3p[ssize_t(p) * a_cs]);
        c20 = simd::fma::fma_f64(a2, b0, c20);
        c21 = simd::fma::fma_f64(a2, b1, c21);
        c30 = simd::fma::fma_f64(a3, b0, c30);
        c31 = simd::fma::fma_f64(a3, b1, c31);
      }
      double *r0 = C + ssize_t(i + 0) * rs_C + ssize_t(j);
      double *r1 = C + ssize_t(i + 1) * rs_C + ssize_t(j);
      double *r2 = C + ssize_t(i + 2) * rs_C + ssize_t(j);
      double *r3 = C + ssize_t(i + 3) * rs_C + ssize_t(j);
      if ( beta_zero ) {
        simd::avx::storeu_f64(r0 + 0, simd::avx::mul_f64(valpha, c00));
        simd::avx::storeu_f64(r0 + 4, simd::avx::mul_f64(valpha, c01));
        simd::avx::storeu_f64(r1 + 0, simd::avx::mul_f64(valpha, c10));
        simd::avx::storeu_f64(r1 + 4, simd::avx::mul_f64(valpha, c11));
        simd::avx::storeu_f64(r2 + 0, simd::avx::mul_f64(valpha, c20));
        simd::avx::storeu_f64(r2 + 4, simd::avx::mul_f64(valpha, c21));
        simd::avx::storeu_f64(r3 + 0, simd::avx::mul_f64(valpha, c30));
        simd::avx::storeu_f64(r3 + 4, simd::avx::mul_f64(valpha, c31));
      } else if ( beta_one ) {
        simd::avx::storeu_f64(r0 + 0, simd::fma::fma_f64(valpha, c00, simd::avx::loadu_f64(r0 + 0)));
        simd::avx::storeu_f64(r0 + 4, simd::fma::fma_f64(valpha, c01, simd::avx::loadu_f64(r0 + 4)));
        simd::avx::storeu_f64(r1 + 0, simd::fma::fma_f64(valpha, c10, simd::avx::loadu_f64(r1 + 0)));
        simd::avx::storeu_f64(r1 + 4, simd::fma::fma_f64(valpha, c11, simd::avx::loadu_f64(r1 + 4)));
        simd::avx::storeu_f64(r2 + 0, simd::fma::fma_f64(valpha, c20, simd::avx::loadu_f64(r2 + 0)));
        simd::avx::storeu_f64(r2 + 4, simd::fma::fma_f64(valpha, c21, simd::avx::loadu_f64(r2 + 4)));
        simd::avx::storeu_f64(r3 + 0, simd::fma::fma_f64(valpha, c30, simd::avx::loadu_f64(r3 + 0)));
        simd::avx::storeu_f64(r3 + 4, simd::fma::fma_f64(valpha, c31, simd::avx::loadu_f64(r3 + 4)));
      } else {
        simd::avx::storeu_f64(r0 + 0, simd::fma::fma_f64(valpha, c00, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r0 + 0))));
        simd::avx::storeu_f64(r0 + 4, simd::fma::fma_f64(valpha, c01, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r0 + 4))));
        simd::avx::storeu_f64(r1 + 0, simd::fma::fma_f64(valpha, c10, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r1 + 0))));
        simd::avx::storeu_f64(r1 + 4, simd::fma::fma_f64(valpha, c11, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r1 + 4))));
        simd::avx::storeu_f64(r2 + 0, simd::fma::fma_f64(valpha, c20, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r2 + 0))));
        simd::avx::storeu_f64(r2 + 4, simd::fma::fma_f64(valpha, c21, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r2 + 4))));
        simd::avx::storeu_f64(r3 + 0, simd::fma::fma_f64(valpha, c30, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r3 + 0))));
        simd::avx::storeu_f64(r3 + 4, simd::fma::fma_f64(valpha, c31, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r3 + 4))));
      }
    }
  }
}

[[gnu::flatten]] inline void
gemm_4x8_avx2_f64_aligned(usize m, usize n, usize k, double alpha, const double *A, ssize_t a_rs, ssize_t a_cs, const double *B,
                          ssize_t b_rs, ssize_t b_cs, double beta, double *C, ssize_t rs_C) noexcept
{
  alignas(32) double Bp[8 * 1024];
  const __m256d valpha = simd::avx::splat_f64(alpha);
  const bool beta_zero = (beta == 0.0);
  const bool beta_one = (beta == 1.0);
  const __m256d vbeta = simd::avx::splat_f64(beta);

  for ( usize j = 0; j + 8 <= n; j += 8 ) {
    if ( b_cs == 1 ) {
      for ( usize p = 0; p < k; ++p ) {
        const double *src = B + ssize_t(p) * b_rs + ssize_t(j);
        simd::avx::store_f64(Bp + p * 8 + 0, simd::avx::loadu_f64(src + 0));
        simd::avx::store_f64(Bp + p * 8 + 4, simd::avx::loadu_f64(src + 4));
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

      __m256d c00 = simd::avx::zero_f64(), c01 = simd::avx::zero_f64();
      __m256d c10 = simd::avx::zero_f64(), c11 = simd::avx::zero_f64();
      __m256d c20 = simd::avx::zero_f64(), c21 = simd::avx::zero_f64();
      __m256d c30 = simd::avx::zero_f64(), c31 = simd::avx::zero_f64();
      for ( usize p = 0; p < k; ++p ) {
        const __m256d b0 = simd::avx::load_f64(Bp + p * 8 + 0);
        const __m256d b1 = simd::avx::load_f64(Bp + p * 8 + 4);
        const __m256d a0 = simd::avx::splat_f64(a0p[ssize_t(p) * a_cs]);
        const __m256d a1 = simd::avx::splat_f64(a1p[ssize_t(p) * a_cs]);
        c00 = simd::fma::fma_f64(a0, b0, c00);
        c01 = simd::fma::fma_f64(a0, b1, c01);
        c10 = simd::fma::fma_f64(a1, b0, c10);
        c11 = simd::fma::fma_f64(a1, b1, c11);
        const __m256d a2 = simd::avx::splat_f64(a2p[ssize_t(p) * a_cs]);
        const __m256d a3 = simd::avx::splat_f64(a3p[ssize_t(p) * a_cs]);
        c20 = simd::fma::fma_f64(a2, b0, c20);
        c21 = simd::fma::fma_f64(a2, b1, c21);
        c30 = simd::fma::fma_f64(a3, b0, c30);
        c31 = simd::fma::fma_f64(a3, b1, c31);
      }
      double *r0 = C + ssize_t(i + 0) * rs_C + ssize_t(j);
      double *r1 = C + ssize_t(i + 1) * rs_C + ssize_t(j);
      double *r2 = C + ssize_t(i + 2) * rs_C + ssize_t(j);
      double *r3 = C + ssize_t(i + 3) * rs_C + ssize_t(j);
      if ( beta_zero ) {
        simd::avx::store_f64(r0 + 0, simd::avx::mul_f64(valpha, c00));
        simd::avx::store_f64(r0 + 4, simd::avx::mul_f64(valpha, c01));
        simd::avx::store_f64(r1 + 0, simd::avx::mul_f64(valpha, c10));
        simd::avx::store_f64(r1 + 4, simd::avx::mul_f64(valpha, c11));
        simd::avx::store_f64(r2 + 0, simd::avx::mul_f64(valpha, c20));
        simd::avx::store_f64(r2 + 4, simd::avx::mul_f64(valpha, c21));
        simd::avx::store_f64(r3 + 0, simd::avx::mul_f64(valpha, c30));
        simd::avx::store_f64(r3 + 4, simd::avx::mul_f64(valpha, c31));
      } else if ( beta_one ) {
        simd::avx::store_f64(r0 + 0, simd::fma::fma_f64(valpha, c00, simd::avx::load_f64(r0 + 0)));
        simd::avx::store_f64(r0 + 4, simd::fma::fma_f64(valpha, c01, simd::avx::load_f64(r0 + 4)));
        simd::avx::store_f64(r1 + 0, simd::fma::fma_f64(valpha, c10, simd::avx::load_f64(r1 + 0)));
        simd::avx::store_f64(r1 + 4, simd::fma::fma_f64(valpha, c11, simd::avx::load_f64(r1 + 4)));
        simd::avx::store_f64(r2 + 0, simd::fma::fma_f64(valpha, c20, simd::avx::load_f64(r2 + 0)));
        simd::avx::store_f64(r2 + 4, simd::fma::fma_f64(valpha, c21, simd::avx::load_f64(r2 + 4)));
        simd::avx::store_f64(r3 + 0, simd::fma::fma_f64(valpha, c30, simd::avx::load_f64(r3 + 0)));
        simd::avx::store_f64(r3 + 4, simd::fma::fma_f64(valpha, c31, simd::avx::load_f64(r3 + 4)));
      } else {
        simd::avx::store_f64(r0 + 0, simd::fma::fma_f64(valpha, c00, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r0 + 0))));
        simd::avx::store_f64(r0 + 4, simd::fma::fma_f64(valpha, c01, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r0 + 4))));
        simd::avx::store_f64(r1 + 0, simd::fma::fma_f64(valpha, c10, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r1 + 0))));
        simd::avx::store_f64(r1 + 4, simd::fma::fma_f64(valpha, c11, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r1 + 4))));
        simd::avx::store_f64(r2 + 0, simd::fma::fma_f64(valpha, c20, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r2 + 0))));
        simd::avx::store_f64(r2 + 4, simd::fma::fma_f64(valpha, c21, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r2 + 4))));
        simd::avx::store_f64(r3 + 0, simd::fma::fma_f64(valpha, c30, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r3 + 0))));
        simd::avx::store_f64(r3 + 4, simd::fma::fma_f64(valpha, c31, simd::avx::mul_f64(vbeta, simd::avx::load_f64(r3 + 4))));
      }
    }
  }
}

#endif      // AVX2 + FMA

// %%%%%%%%%%%%%%%%%%%%%%%%%
// gemm_kernel
template<typename T>
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

  // inline 4x8 kernel with no panel packing
#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( sizeof(T) == 8 && ieee754_floating<T> ) {
      const u64 nflops = u64(m) * u64(n) * u64(k);
      const bool size_ok = (a_cs == 1) ? (nflops < (256ull * 256ull * 256ull)) : (nflops < (64ull * 64ull * 64ull));
      if ( size_ok && cs_C == 1 && (m % 4u) == 0 && (n % 8u) == 0 && k > 0 && k <= 1024 ) {
        gemm_4x8_avx2_f64(m, n, k, double(alpha), reinterpret_cast<const double *>(A), a_rs, a_cs, reinterpret_cast<const double *>(B),
                          b_rs, b_cs, double(beta), reinterpret_cast<double *>(C), rs_C);
        return;
      }
    }
  }
#endif

  // packed/blocked path for everything the inline kernel can't take or for problems large enough that pack overhead amortizes
  if !consteval {
    if ( micron::math::matrix::pack::gemm_should_block_for_layout<T>(m, n, k, b_cs, cs_C) ) {
      micron::math::matrix::pack::gemm_blocked<T>(m, n, k, alpha, A, a_rs, a_cs, B, b_rs, b_cs, beta, C, rs_C, cs_C);
      return;
    }
  }

  // scalar fallback path (very small problems and consteval)
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gemm_kernel_aligned
template<typename T>
[[gnu::flatten]] inline constexpr void
gemm_kernel_aligned(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B,
                    ssize_t rs_B, ssize_t cs_B, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  const ssize_t a_rs = trA ? cs_A : rs_A;
  const ssize_t a_cs = trA ? rs_A : cs_A;
  const ssize_t b_rs = trB ? cs_B : rs_B;
  const ssize_t b_cs = trB ? rs_B : cs_B;

#if defined(__AVX2__) && defined(__FMA__)
  if !consteval {
    if constexpr ( sizeof(T) == 8 && ieee754_floating<T> ) {
      const u64 nflops = u64(m) * u64(n) * u64(k);
      const bool size_ok = (a_cs == 1) ? (nflops < (256ull * 256ull * 256ull)) : (nflops < (64ull * 64ull * 64ull));
      if ( size_ok && cs_C == 1 && (m % 4u) == 0 && (n % 8u) == 0 && k > 0 && k <= 1024 ) {
        gemm_4x8_avx2_f64_aligned(m, n, k, double(alpha), reinterpret_cast<const double *>(A), a_rs, a_cs,
                                  reinterpret_cast<const double *>(B), b_rs, b_cs, double(beta), reinterpret_cast<double *>(C), rs_C);
        return;
      }
    }
  }
#endif

  if !consteval {
    if ( micron::math::matrix::pack::gemm_should_block_for_layout<T>(m, n, k, b_cs, cs_C) ) {
      micron::math::matrix::pack::gemm_blocked_aligned<T>(m, n, k, alpha, A, a_rs, a_cs, B, b_rs, b_cs, beta, C, rs_C, cs_C);
      return;
    }
  }

  // Fall back to the unaligned dispatch
  gemm_kernel<T>(trA, trB, m, n, k, alpha, A, rs_A, cs_A, B, rs_B, cs_B, beta, C, rs_C, cs_C);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gemm_kernel_aligned experiments
template<typename T>
[[gnu::flatten]] inline void
gemm_kernel_aligned_exp_a(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B,
                          ssize_t rs_B, ssize_t cs_B, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  const ssize_t a_rs = trA ? cs_A : rs_A;
  const ssize_t a_cs = trA ? rs_A : cs_A;
  const ssize_t b_rs = trB ? cs_B : rs_B;
  const ssize_t b_cs = trB ? rs_B : cs_B;
  micron::math::matrix::pack::gemm_blocked_aligned_exp_a<T>(m, n, k, alpha, A, a_rs, a_cs, B, b_rs, b_cs, beta, C, rs_C, cs_C);
}

template<typename T>
[[gnu::flatten]] inline void
gemm_kernel_aligned_exp_b(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B,
                          ssize_t rs_B, ssize_t cs_B, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  const ssize_t a_rs = trA ? cs_A : rs_A;
  const ssize_t a_cs = trA ? rs_A : cs_A;
  const ssize_t b_rs = trB ? cs_B : rs_B;
  const ssize_t b_cs = trB ? rs_B : cs_B;
  micron::math::matrix::pack::gemm_blocked_aligned_exp_b<T>(m, n, k, alpha, A, a_rs, a_cs, B, b_rs, b_cs, beta, C, rs_C, cs_C);
}

template<typename T>
[[gnu::flatten]] inline void
gemm_kernel_aligned_exp_c(bool trA, bool trB, usize m, usize n, usize k, T alpha, const T *A, ssize_t rs_A, ssize_t cs_A, const T *B,
                          ssize_t rs_B, ssize_t cs_B, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  const ssize_t a_rs = trA ? cs_A : rs_A;
  const ssize_t a_cs = trA ? rs_A : cs_A;
  const ssize_t b_rs = trB ? cs_B : rs_B;
  const ssize_t b_cs = trB ? rs_B : cs_B;
  micron::math::matrix::pack::gemm_blocked_aligned_exp_c<T>(m, n, k, alpha, A, a_rs, a_cs, B, b_rs, b_cs, beta, C, rs_C, cs_C);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trsm_kernel
template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
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
template<typename T>
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
      if ( beta == T(0) )
        cij = alpha * acc;
      else if ( beta == T(1) )
        cij = fma_acc<T>(alpha, acc, cij);
      else
        cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// syr2k_kernel
template<typename T>
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
      if ( beta == T(0) )
        cij = alpha * acc;
      else if ( beta == T(1) )
        cij = fma_acc<T>(alpha, acc, cij);
      else
        cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

template<typename T>
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
      if ( beta == T(0) )
        cij = alpha * acc;
      else if ( beta == T(1) )
        cij = fma_acc<T>(alpha, acc, cij);
      else
        cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

template<typename T>
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
      if ( beta == T(0) )
        cij = alpha * acc;
      else if ( beta == T(1) )
        cij = fma_acc<T>(alpha, acc, cij);
      else
        cij = fma_acc<T>(alpha, acc, beta * cij);
    }
  }
}

};      // namespace bits
};      // namespace blas
};      // namespace math
};      // namespace micron
