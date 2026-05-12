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
#include "../../simd/aliases.hpp"
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

// MC chosen so MC*KC*sizeof(double) fits L2 (256 KiB Haswell)
inline constexpr usize default_mc = 72;
inline constexpr usize default_kc = 256;
inline constexpr usize default_nc = 256;

template <usize MR, typename T>
[[gnu::flatten, gnu::always_inline]] inline void
pack_a_panel(const T *A, ssize_t rs_A, ssize_t cs_A, usize m, usize k, T *dst) noexcept
{
#if defined(__AVX2__) && defined(__FMA__)
  if constexpr ( MR == 6 && sizeof(T) == 8 && ieee754_floating<T> ) {
    if !consteval {
      // rs_A == 1 means the MR-direction is contiguous (e.g. trA=true on row-major A)
      if ( rs_A == 1 ) {
        const double *A_d = reinterpret_cast<const double *>(A);
        double *dst_d = reinterpret_cast<double *>(dst);
        for ( usize t = 0; t < m; t += 6 ) {
          const usize rows = (m - t < 6) ? (m - t) : 6;
          if ( rows == 6 ) {
            for ( usize p = 0; p < k; ++p ) {
              const double *src = A_d + ssize_t(t) + ssize_t(p) * cs_A;
              simd::avx::storeu_f64(dst_d + 0, simd::avx::loadu_f64(src + 0));
              simd::sse::storeu_f64(dst_d + 4, simd::sse::loadu_f64(src + 4));
              dst_d += 6;
            }
          } else {
            for ( usize p = 0; p < k; ++p ) {
              const double *src = A_d + ssize_t(t) + ssize_t(p) * cs_A;
              for ( usize i = 0; i < rows; ++i ) dst_d[i] = src[i];
              for ( usize i = rows; i < 6; ++i ) dst_d[i] = 0.0;
              dst_d += 6;
            }
          }
        }
        return;
      }
      if ( cs_A == 1 ) {
        const double *A_d = reinterpret_cast<const double *>(A);
        double *dst_d = reinterpret_cast<double *>(dst);
        for ( usize t = 0; t < m; t += 6 ) {
          const usize rows = (m - t < 6) ? (m - t) : 6;
          if ( rows == 6 ) {
            const double *a0 = A_d + ssize_t(t + 0) * rs_A;
            const double *a1 = A_d + ssize_t(t + 1) * rs_A;
            const double *a2 = A_d + ssize_t(t + 2) * rs_A;
            const double *a3 = A_d + ssize_t(t + 3) * rs_A;
            const double *a4 = A_d + ssize_t(t + 4) * rs_A;
            const double *a5 = A_d + ssize_t(t + 5) * rs_A;
            usize p = 0;
            for ( ; p + 4 <= k; p += 4 ) {
              const __m256d r0 = simd::avx::loadu_f64(a0 + p);
              const __m256d r1 = simd::avx::loadu_f64(a1 + p);
              const __m256d r2 = simd::avx::loadu_f64(a2 + p);
              const __m256d r3 = simd::avx::loadu_f64(a3 + p);
              const __m256d r4 = simd::avx::loadu_f64(a4 + p);
              const __m256d r5 = simd::avx::loadu_f64(a5 + p);
              const __m256d t01_lo = simd::avx::unpack_lo_f64(r0, r1);     // {r0[0], r1[0], r0[2], r1[2]}
              const __m256d t01_hi = simd::avx::unpack_hi_f64(r0, r1);     // {r0[1], r1[1], r0[3], r1[3]}
              const __m256d t23_lo = simd::avx::unpack_lo_f64(r2, r3);
              const __m256d t23_hi = simd::avx::unpack_hi_f64(r2, r3);
              const __m256d t45_lo = simd::avx::unpack_lo_f64(r4, r5);
              const __m256d t45_hi = simd::avx::unpack_hi_f64(r4, r5);
              // K = p+0
              simd::avx::storeu_f64(dst_d + 0, simd::avx::permute2f128_f64<0x20>(t01_lo, t23_lo));
              simd::sse::storeu_f64(dst_d + 4, simd::avx::cast_f64_to_lo128(t45_lo));
              // K = p+1
              simd::avx::storeu_f64(dst_d + 6, simd::avx::permute2f128_f64<0x20>(t01_hi, t23_hi));
              simd::sse::storeu_f64(dst_d + 10, simd::avx::cast_f64_to_lo128(t45_hi));
              // K = p+2
              simd::avx::storeu_f64(dst_d + 12, simd::avx::permute2f128_f64<0x31>(t01_lo, t23_lo));
              simd::sse::storeu_f64(dst_d + 16, simd::avx::extract_f128_f64<1>(t45_lo));
              // K = p+3
              simd::avx::storeu_f64(dst_d + 18, simd::avx::permute2f128_f64<0x31>(t01_hi, t23_hi));
              simd::sse::storeu_f64(dst_d + 22, simd::avx::extract_f128_f64<1>(t45_hi));
              dst_d += 24;
            }
            for ( ; p < k; ++p ) {
              dst_d[0] = a0[p];
              dst_d[1] = a1[p];
              dst_d[2] = a2[p];
              dst_d[3] = a3[p];
              dst_d[4] = a4[p];
              dst_d[5] = a5[p];
              dst_d += 6;
            }
          } else {
            for ( usize p = 0; p < k; ++p ) {
              for ( usize i = 0; i < rows; ++i ) dst_d[i] = A_d[ssize_t(t + i) * rs_A + ssize_t(p)];
              for ( usize i = rows; i < 6; ++i ) dst_d[i] = 0.0;
              dst_d += 6;
            }
          }
        }
        return;
      }
    }
  }
  // MR=4 f64 row-major: 4x4 transpose pack
  if constexpr ( MR == 4 && sizeof(T) == 8 && ieee754_floating<T> ) {
    if !consteval {
      if ( rs_A == 1 ) {
        // MR-direction contiguous: just stream 4 doubles per K-step.
        const double *A_d = reinterpret_cast<const double *>(A);
        double *dst_d = reinterpret_cast<double *>(dst);
        for ( usize t = 0; t < m; t += 4 ) {
          const usize rows = (m - t < 4) ? (m - t) : 4;
          if ( rows == 4 ) {
            for ( usize p = 0; p < k; ++p ) {
              const double *src = A_d + ssize_t(t) + ssize_t(p) * cs_A;
              simd::avx::storeu_f64(dst_d, simd::avx::loadu_f64(src));
              dst_d += 4;
            }
          } else {
            for ( usize p = 0; p < k; ++p ) {
              const double *src = A_d + ssize_t(t) + ssize_t(p) * cs_A;
              for ( usize i = 0; i < rows; ++i ) dst_d[i] = src[i];
              for ( usize i = rows; i < 4; ++i ) dst_d[i] = 0.0;
              dst_d += 4;
            }
          }
        }
        return;
      }
      if ( cs_A == 1 ) {
        const double *A_d = reinterpret_cast<const double *>(A);
        double *dst_d = reinterpret_cast<double *>(dst);
        for ( usize t = 0; t < m; t += 4 ) {
          const usize rows = (m - t < 4) ? (m - t) : 4;
          if ( rows == 4 ) {
            const double *a0 = A_d + ssize_t(t + 0) * rs_A;
            const double *a1 = A_d + ssize_t(t + 1) * rs_A;
            const double *a2 = A_d + ssize_t(t + 2) * rs_A;
            const double *a3 = A_d + ssize_t(t + 3) * rs_A;
            usize p = 0;
            for ( ; p + 4 <= k; p += 4 ) {
              const __m256d r0 = simd::avx::loadu_f64(a0 + p);
              const __m256d r1 = simd::avx::loadu_f64(a1 + p);
              const __m256d r2 = simd::avx::loadu_f64(a2 + p);
              const __m256d r3 = simd::avx::loadu_f64(a3 + p);
              const __m256d t01_lo = simd::avx::unpack_lo_f64(r0, r1);
              const __m256d t01_hi = simd::avx::unpack_hi_f64(r0, r1);
              const __m256d t23_lo = simd::avx::unpack_lo_f64(r2, r3);
              const __m256d t23_hi = simd::avx::unpack_hi_f64(r2, r3);
              simd::avx::storeu_f64(dst_d + 0, simd::avx::permute2f128_f64<0x20>(t01_lo, t23_lo));
              simd::avx::storeu_f64(dst_d + 4, simd::avx::permute2f128_f64<0x20>(t01_hi, t23_hi));
              simd::avx::storeu_f64(dst_d + 8, simd::avx::permute2f128_f64<0x31>(t01_lo, t23_lo));
              simd::avx::storeu_f64(dst_d + 12, simd::avx::permute2f128_f64<0x31>(t01_hi, t23_hi));
              dst_d += 16;
            }
            for ( ; p < k; ++p ) {
              dst_d[0] = a0[p];
              dst_d[1] = a1[p];
              dst_d[2] = a2[p];
              dst_d[3] = a3[p];
              dst_d += 4;
            }
          } else {
            for ( usize p = 0; p < k; ++p ) {
              for ( usize i = 0; i < rows; ++i ) dst_d[i] = A_d[ssize_t(t + i) * rs_A + ssize_t(p)];
              for ( usize i = rows; i < 4; ++i ) dst_d[i] = 0.0;
              dst_d += 4;
            }
          }
        }
        return;
      }
    }
  }
#endif
  // Scalar generic fallback
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
#if defined(__AVX2__) && defined(__FMA__)
  if constexpr ( NR == 8 && sizeof(T) == 8 && ieee754_floating<T> ) {
    if !consteval {
      // rs_B == 1 means K-direction is contiguous (trB=true on row-major B)
      if ( rs_B == 1 ) {
        const double *B_d = reinterpret_cast<const double *>(B);
        double *dst_d = reinterpret_cast<double *>(dst);
        for ( usize t = 0; t < n; t += 8 ) {
          const usize cols = (n - t < 8) ? (n - t) : 8;
          if ( cols == 8 ) {
            usize p = 0;
            for ( ; p + 4 <= k; p += 4 ) {
              const double *c0 = B_d + ssize_t(t + 0) * cs_B + ssize_t(p);
              const double *c1 = B_d + ssize_t(t + 1) * cs_B + ssize_t(p);
              const double *c2 = B_d + ssize_t(t + 2) * cs_B + ssize_t(p);
              const double *c3 = B_d + ssize_t(t + 3) * cs_B + ssize_t(p);
              const double *c4 = B_d + ssize_t(t + 4) * cs_B + ssize_t(p);
              const double *c5 = B_d + ssize_t(t + 5) * cs_B + ssize_t(p);
              const double *c6 = B_d + ssize_t(t + 6) * cs_B + ssize_t(p);
              const double *c7 = B_d + ssize_t(t + 7) * cs_B + ssize_t(p);
              // first 4 cols
              const __m256d r0 = simd::avx::loadu_f64(c0);
              const __m256d r1 = simd::avx::loadu_f64(c1);
              const __m256d r2 = simd::avx::loadu_f64(c2);
              const __m256d r3 = simd::avx::loadu_f64(c3);
              const __m256d t01_lo = simd::avx::unpack_lo_f64(r0, r1);
              const __m256d t01_hi = simd::avx::unpack_hi_f64(r0, r1);
              const __m256d t23_lo = simd::avx::unpack_lo_f64(r2, r3);
              const __m256d t23_hi = simd::avx::unpack_hi_f64(r2, r3);
              // K=p+0..p+3, cols 0..3
              simd::avx::storeu_f64(dst_d + 0, simd::avx::permute2f128_f64<0x20>(t01_lo, t23_lo));
              simd::avx::storeu_f64(dst_d + 8, simd::avx::permute2f128_f64<0x20>(t01_hi, t23_hi));
              simd::avx::storeu_f64(dst_d + 16, simd::avx::permute2f128_f64<0x31>(t01_lo, t23_lo));
              simd::avx::storeu_f64(dst_d + 24, simd::avx::permute2f128_f64<0x31>(t01_hi, t23_hi));
              // second 4 cols
              const __m256d r4 = simd::avx::loadu_f64(c4);
              const __m256d r5 = simd::avx::loadu_f64(c5);
              const __m256d r6 = simd::avx::loadu_f64(c6);
              const __m256d r7 = simd::avx::loadu_f64(c7);
              const __m256d t45_lo = simd::avx::unpack_lo_f64(r4, r5);
              const __m256d t45_hi = simd::avx::unpack_hi_f64(r4, r5);
              const __m256d t67_lo = simd::avx::unpack_lo_f64(r6, r7);
              const __m256d t67_hi = simd::avx::unpack_hi_f64(r6, r7);
              simd::avx::storeu_f64(dst_d + 4, simd::avx::permute2f128_f64<0x20>(t45_lo, t67_lo));
              simd::avx::storeu_f64(dst_d + 12, simd::avx::permute2f128_f64<0x20>(t45_hi, t67_hi));
              simd::avx::storeu_f64(dst_d + 20, simd::avx::permute2f128_f64<0x31>(t45_lo, t67_lo));
              simd::avx::storeu_f64(dst_d + 28, simd::avx::permute2f128_f64<0x31>(t45_hi, t67_hi));
              dst_d += 32;
            }
            for ( ; p < k; ++p ) {
              for ( usize j = 0; j < 8; ++j ) dst_d[j] = B_d[ssize_t(p) + ssize_t(t + j) * cs_B];
              dst_d += 8;
            }
          } else {
            for ( usize p = 0; p < k; ++p ) {
              for ( usize j = 0; j < cols; ++j ) dst_d[j] = B_d[ssize_t(p) + ssize_t(t + j) * cs_B];
              for ( usize j = cols; j < 8; ++j ) dst_d[j] = 0.0;
              dst_d += 8;
            }
          }
        }
        return;
      }
      if ( cs_B == 1 ) {
        const double *B_d = reinterpret_cast<const double *>(B);
        double *dst_d = reinterpret_cast<double *>(dst);
        for ( usize t = 0; t < n; t += 8 ) {
          const usize cols = (n - t < 8) ? (n - t) : 8;
          if ( cols == 8 ) {
            for ( usize p = 0; p < k; ++p ) {
              const double *src = B_d + ssize_t(p) * rs_B + ssize_t(t);
              simd::avx::storeu_f64(dst_d + 0, simd::avx::loadu_f64(src + 0));
              simd::avx::storeu_f64(dst_d + 4, simd::avx::loadu_f64(src + 4));
              dst_d += 8;
            }
          } else {
            for ( usize p = 0; p < k; ++p ) {
              for ( usize j = 0; j < cols; ++j ) dst_d[j] = B_d[ssize_t(p) * rs_B + ssize_t(t + j)];
              for ( usize j = cols; j < 8; ++j ) dst_d[j] = 0.0;
              dst_d += 8;
            }
          }
        }
        return;
      }
    }
  }
  // NR=8 f32 row-major: same memcpy pattern with __m256.
  if constexpr ( NR == 8 && sizeof(T) == 4 && ieee754_floating<T> ) {
    if !consteval {
      if ( cs_B == 1 ) {
        const float *B_f = reinterpret_cast<const float *>(B);
        float *dst_f = reinterpret_cast<float *>(dst);
        for ( usize t = 0; t < n; t += 8 ) {
          const usize cols = (n - t < 8) ? (n - t) : 8;
          if ( cols == 8 ) {
            for ( usize p = 0; p < k; ++p ) {
              const float *src = B_f + ssize_t(p) * rs_B + ssize_t(t);
              simd::avx::storeu_f32(dst_f + 0, simd::avx::loadu_f32(src));
              dst_f += 8;
            }
          } else {
            for ( usize p = 0; p < k; ++p ) {
              for ( usize j = 0; j < cols; ++j ) dst_f[j] = B_f[ssize_t(p) * rs_B + ssize_t(t + j)];
              for ( usize j = cols; j < 8; ++j ) dst_f[j] = 0.0f;
              dst_f += 8;
            }
          }
        }
        return;
      }
    }
  }
#endif
  // Scalar generic fallback
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

[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel_4x8_avx2_f64(const double *Ap, const double *Bp, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                          ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<4, 8, double>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  __m256d c00 = simd::avx::zero_f64(), c01 = simd::avx::zero_f64();
  __m256d c10 = simd::avx::zero_f64(), c11 = simd::avx::zero_f64();
  __m256d c20 = simd::avx::zero_f64(), c21 = simd::avx::zero_f64();
  __m256d c30 = simd::avx::zero_f64(), c31 = simd::avx::zero_f64();

  for ( usize p = 0; p < k; ++p ) {
    const __m256d b0 = simd::avx::loadu_f64(Bp + p * 8 + 0);
    const __m256d b1 = simd::avx::loadu_f64(Bp + p * 8 + 4);
    const __m256d a0 = simd::avx::splat_f64(Ap[p * 4 + 0]);
    const __m256d a1 = simd::avx::splat_f64(Ap[p * 4 + 1]);
    c00 = simd::fma::fma_f64(a0, b0, c00);
    c01 = simd::fma::fma_f64(a0, b1, c01);
    c10 = simd::fma::fma_f64(a1, b0, c10);
    c11 = simd::fma::fma_f64(a1, b1, c11);
    const __m256d a2 = simd::avx::splat_f64(Ap[p * 4 + 2]);
    const __m256d a3 = simd::avx::splat_f64(Ap[p * 4 + 3]);
    c20 = simd::fma::fma_f64(a2, b0, c20);
    c21 = simd::fma::fma_f64(a2, b1, c21);
    c30 = simd::fma::fma_f64(a3, b0, c30);
    c31 = simd::fma::fma_f64(a3, b1, c31);
  }
  double *r0 = C + ssize_t(0) * rs_C;
  double *r1 = C + ssize_t(1) * rs_C;
  double *r2 = C + ssize_t(2) * rs_C;
  double *r3 = C + ssize_t(3) * rs_C;
  if ( beta == 0.0 ) {
    const __m256d valpha = simd::avx::splat_f64(alpha);
    simd::avx::storeu_f64(r0 + 0, simd::avx::mul_f64(valpha, c00));
    simd::avx::storeu_f64(r0 + 4, simd::avx::mul_f64(valpha, c01));
    simd::avx::storeu_f64(r1 + 0, simd::avx::mul_f64(valpha, c10));
    simd::avx::storeu_f64(r1 + 4, simd::avx::mul_f64(valpha, c11));
    simd::avx::storeu_f64(r2 + 0, simd::avx::mul_f64(valpha, c20));
    simd::avx::storeu_f64(r2 + 4, simd::avx::mul_f64(valpha, c21));
    simd::avx::storeu_f64(r3 + 0, simd::avx::mul_f64(valpha, c30));
    simd::avx::storeu_f64(r3 + 4, simd::avx::mul_f64(valpha, c31));
  } else if ( beta == 1.0 ) {
    const __m256d valpha = simd::avx::splat_f64(alpha);
    simd::avx::storeu_f64(r0 + 0, simd::fma::fma_f64(valpha, c00, simd::avx::loadu_f64(r0 + 0)));
    simd::avx::storeu_f64(r0 + 4, simd::fma::fma_f64(valpha, c01, simd::avx::loadu_f64(r0 + 4)));
    simd::avx::storeu_f64(r1 + 0, simd::fma::fma_f64(valpha, c10, simd::avx::loadu_f64(r1 + 0)));
    simd::avx::storeu_f64(r1 + 4, simd::fma::fma_f64(valpha, c11, simd::avx::loadu_f64(r1 + 4)));
    simd::avx::storeu_f64(r2 + 0, simd::fma::fma_f64(valpha, c20, simd::avx::loadu_f64(r2 + 0)));
    simd::avx::storeu_f64(r2 + 4, simd::fma::fma_f64(valpha, c21, simd::avx::loadu_f64(r2 + 4)));
    simd::avx::storeu_f64(r3 + 0, simd::fma::fma_f64(valpha, c30, simd::avx::loadu_f64(r3 + 0)));
    simd::avx::storeu_f64(r3 + 4, simd::fma::fma_f64(valpha, c31, simd::avx::loadu_f64(r3 + 4)));
  } else {
    const __m256d valpha = simd::avx::splat_f64(alpha);
    const __m256d vbeta = simd::avx::splat_f64(beta);
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

// 6 rows × 8 cols f64 microkernel
// 12 C accumulators (c00..c52)
// 2 B vectors (b0, b1)
// 2 transient A broadcasts
// 12 FMA / 8 loads = 1.5 FMA/load
[[gnu::flatten, gnu::hot, gnu::always_inline]] inline void
micro_kernel_6x8_avx2_f64(const double *Ap, const double *Bp, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                          ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    // 6x8 partial path is the scalar one; will be hit only for col_view layouts
    micro_kernel_scalar<6, 8, double>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  __m256d c00 = simd::avx::zero_f64(), c01 = simd::avx::zero_f64();
  __m256d c10 = simd::avx::zero_f64(), c11 = simd::avx::zero_f64();
  __m256d c20 = simd::avx::zero_f64(), c21 = simd::avx::zero_f64();
  __m256d c30 = simd::avx::zero_f64(), c31 = simd::avx::zero_f64();
  __m256d c40 = simd::avx::zero_f64(), c41 = simd::avx::zero_f64();
  __m256d c50 = simd::avx::zero_f64(), c51 = simd::avx::zero_f64();

  for ( usize p = 0; p < k; ++p ) {
    const __m256d b0 = simd::avx::loadu_f64(Bp + p * 8 + 0);
    const __m256d b1 = simd::avx::loadu_f64(Bp + p * 8 + 4);
    const __m256d a0 = simd::avx::splat_f64(Ap[p * 6 + 0]);
    c00 = simd::fma::fma_f64(a0, b0, c00);
    c01 = simd::fma::fma_f64(a0, b1, c01);
    const __m256d a1 = simd::avx::splat_f64(Ap[p * 6 + 1]);
    c10 = simd::fma::fma_f64(a1, b0, c10);
    c11 = simd::fma::fma_f64(a1, b1, c11);
    const __m256d a2 = simd::avx::splat_f64(Ap[p * 6 + 2]);
    c20 = simd::fma::fma_f64(a2, b0, c20);
    c21 = simd::fma::fma_f64(a2, b1, c21);
    const __m256d a3 = simd::avx::splat_f64(Ap[p * 6 + 3]);
    c30 = simd::fma::fma_f64(a3, b0, c30);
    c31 = simd::fma::fma_f64(a3, b1, c31);
    const __m256d a4 = simd::avx::splat_f64(Ap[p * 6 + 4]);
    c40 = simd::fma::fma_f64(a4, b0, c40);
    c41 = simd::fma::fma_f64(a4, b1, c41);
    const __m256d a5 = simd::avx::splat_f64(Ap[p * 6 + 5]);
    c50 = simd::fma::fma_f64(a5, b0, c50);
    c51 = simd::fma::fma_f64(a5, b1, c51);
  }

  const __m256d valpha = simd::avx::splat_f64(alpha);
  double *r0 = C + 0 * rs_C, *r1 = C + 1 * rs_C, *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C, *r4 = C + 4 * rs_C, *r5 = C + 5 * rs_C;
  if ( beta == 0.0 ) {
    simd::avx::storeu_f64(r0 + 0, simd::avx::mul_f64(valpha, c00));
    simd::avx::storeu_f64(r0 + 4, simd::avx::mul_f64(valpha, c01));
    simd::avx::storeu_f64(r1 + 0, simd::avx::mul_f64(valpha, c10));
    simd::avx::storeu_f64(r1 + 4, simd::avx::mul_f64(valpha, c11));
    simd::avx::storeu_f64(r2 + 0, simd::avx::mul_f64(valpha, c20));
    simd::avx::storeu_f64(r2 + 4, simd::avx::mul_f64(valpha, c21));
    simd::avx::storeu_f64(r3 + 0, simd::avx::mul_f64(valpha, c30));
    simd::avx::storeu_f64(r3 + 4, simd::avx::mul_f64(valpha, c31));
    simd::avx::storeu_f64(r4 + 0, simd::avx::mul_f64(valpha, c40));
    simd::avx::storeu_f64(r4 + 4, simd::avx::mul_f64(valpha, c41));
    simd::avx::storeu_f64(r5 + 0, simd::avx::mul_f64(valpha, c50));
    simd::avx::storeu_f64(r5 + 4, simd::avx::mul_f64(valpha, c51));
  } else if ( beta == 1.0 ) {
    simd::avx::storeu_f64(r0 + 0, simd::fma::fma_f64(valpha, c00, simd::avx::loadu_f64(r0 + 0)));
    simd::avx::storeu_f64(r0 + 4, simd::fma::fma_f64(valpha, c01, simd::avx::loadu_f64(r0 + 4)));
    simd::avx::storeu_f64(r1 + 0, simd::fma::fma_f64(valpha, c10, simd::avx::loadu_f64(r1 + 0)));
    simd::avx::storeu_f64(r1 + 4, simd::fma::fma_f64(valpha, c11, simd::avx::loadu_f64(r1 + 4)));
    simd::avx::storeu_f64(r2 + 0, simd::fma::fma_f64(valpha, c20, simd::avx::loadu_f64(r2 + 0)));
    simd::avx::storeu_f64(r2 + 4, simd::fma::fma_f64(valpha, c21, simd::avx::loadu_f64(r2 + 4)));
    simd::avx::storeu_f64(r3 + 0, simd::fma::fma_f64(valpha, c30, simd::avx::loadu_f64(r3 + 0)));
    simd::avx::storeu_f64(r3 + 4, simd::fma::fma_f64(valpha, c31, simd::avx::loadu_f64(r3 + 4)));
    simd::avx::storeu_f64(r4 + 0, simd::fma::fma_f64(valpha, c40, simd::avx::loadu_f64(r4 + 0)));
    simd::avx::storeu_f64(r4 + 4, simd::fma::fma_f64(valpha, c41, simd::avx::loadu_f64(r4 + 4)));
    simd::avx::storeu_f64(r5 + 0, simd::fma::fma_f64(valpha, c50, simd::avx::loadu_f64(r5 + 0)));
    simd::avx::storeu_f64(r5 + 4, simd::fma::fma_f64(valpha, c51, simd::avx::loadu_f64(r5 + 4)));
  } else {
    const __m256d vbeta = simd::avx::splat_f64(beta);
    simd::avx::storeu_f64(r0 + 0, simd::fma::fma_f64(valpha, c00, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r0 + 0))));
    simd::avx::storeu_f64(r0 + 4, simd::fma::fma_f64(valpha, c01, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r0 + 4))));
    simd::avx::storeu_f64(r1 + 0, simd::fma::fma_f64(valpha, c10, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r1 + 0))));
    simd::avx::storeu_f64(r1 + 4, simd::fma::fma_f64(valpha, c11, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r1 + 4))));
    simd::avx::storeu_f64(r2 + 0, simd::fma::fma_f64(valpha, c20, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r2 + 0))));
    simd::avx::storeu_f64(r2 + 4, simd::fma::fma_f64(valpha, c21, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r2 + 4))));
    simd::avx::storeu_f64(r3 + 0, simd::fma::fma_f64(valpha, c30, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r3 + 0))));
    simd::avx::storeu_f64(r3 + 4, simd::fma::fma_f64(valpha, c31, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r3 + 4))));
    simd::avx::storeu_f64(r4 + 0, simd::fma::fma_f64(valpha, c40, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r4 + 0))));
    simd::avx::storeu_f64(r4 + 4, simd::fma::fma_f64(valpha, c41, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r4 + 4))));
    simd::avx::storeu_f64(r5 + 0, simd::fma::fma_f64(valpha, c50, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r5 + 0))));
    simd::avx::storeu_f64(r5 + 4, simd::fma::fma_f64(valpha, c51, simd::avx::mul_f64(vbeta, simd::avx::loadu_f64(r5 + 4))));
  }
}

// INLINE ASM
// the above version hits a GCC register-allocation pathology where one accumulator
// spills to stack via memory-source FMA, capping at ~7 fc/cy
[[gnu::hot, gnu::always_inline]] inline void
micro_kernel_6x8_avx2_f64_asm(const double *Ap_in, const double *Bp_in, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                              ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<6, 8, double>(Ap_in, Bp_in, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  double *r4 = C + 4 * rs_C;
  double *r5 = C + 5 * rs_C;
  const double *ap = Ap_in;
  const double *bp = Bp_in;
  usize kk = k;

  __asm__ volatile(
      // zero accs
      "vxorpd %%ymm4,  %%ymm4,  %%ymm4 \n\t"
      "vxorpd %%ymm5,  %%ymm5,  %%ymm5 \n\t"
      "vxorpd %%ymm6,  %%ymm6,  %%ymm6 \n\t"
      "vxorpd %%ymm7,  %%ymm7,  %%ymm7 \n\t"
      "vxorpd %%ymm8,  %%ymm8,  %%ymm8 \n\t"
      "vxorpd %%ymm9,  %%ymm9,  %%ymm9 \n\t"
      "vxorpd %%ymm10, %%ymm10, %%ymm10 \n\t"
      "vxorpd %%ymm11, %%ymm11, %%ymm11 \n\t"
      "vxorpd %%ymm12, %%ymm12, %%ymm12 \n\t"
      "vxorpd %%ymm13, %%ymm13, %%ymm13 \n\t"
      "vxorpd %%ymm14, %%ymm14, %%ymm14 \n\t"
      "vxorpd %%ymm15, %%ymm15, %%ymm15 \n\t"

      // Skip K-loop if k==0
      "testq %[k], %[k] \n\t"
      "jz 2f \n\t"

      // K-loop (12 FMAs, 8 mem ops per iter = FMA-bound at 6 cyc)
      ".p2align 4 \n\t"
      "1: \n\t"
      // Load two halves of B
      "vmovupd  0(%[bp]), %%ymm0 \n\t"
      "vmovupd 32(%[bp]), %%ymm1 \n\t"

      // rows 0,1; broadcast a0, a1; fma into c00,c01,c10,c11
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"     // c00 += b0 * a0
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"     // c01 += b1 * a0
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"     // c10 += b0 * a1
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"     // c11 += b1 * a1

      "prefetcht0 512(%[bp]) \n\t"

      // rows 2,3; broadcast a2, a3; fma into c20,c21,c30,c31
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"

      "prefetcht0 480(%[ap]) \n\t"

      // rows 4,5; broadcast a4, a5; fma into c40,c41,c50,c51
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"

      "addq $48, %[ap] \n\t"
      "addq $64, %[bp] \n\t"
      "decq %[k] \n\t"
      "jne 1b \n\t"

      "2: \n\t"
      "vbroadcastsd %[alpha], %%ymm0 \n\t"     // ymm0 = alpha (broadcast)
      "vxorpd %%xmm1, %%xmm1, %%xmm1 \n\t"     // xmm1 = 0.0
      "vucomisd %[beta], %%xmm1 \n\t"          // compare beta with 0.0
      "jne 3f \n\t"                            // if beta != 0, take beta path

      "vmulpd %%ymm4,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd %%ymm5,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd %%ymm6,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd %%ymm7,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd %%ymm8,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd %%ymm9,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "jmp 4f \n\t"

      "3: \n\t"
      "vbroadcastsd %[beta], %%ymm1 \n\t"     // ymm1 = beta (broadcast)
      // row 0
      "vmulpd  (%[r0]), %%ymm1, %%ymm2 \n\t"        // ymm2 = beta*C[0,0..3]
      "vfmadd231pd %%ymm4, %%ymm0, %%ymm2 \n\t"     // ymm2 += alpha*c00
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd 32(%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm5, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      // row 1
      "vmulpd  (%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm6, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd 32(%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm7, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      // row 2
      "vmulpd  (%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm8, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd 32(%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm9, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      // row 3
      "vmulpd  (%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd 32(%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      // row 4
      "vmulpd  (%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd 32(%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      // row 5
      "vmulpd  (%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd 32(%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"

      "4: \n\t"
      "vzeroupper \n\t"
      : [ap] "+r"(ap), [bp] "+r"(bp), [k] "+r"(kk)
      : [alpha] "m"(alpha), [beta] "m"(beta), [r0] "r"(r0), [r1] "r"(r1), [r2] "r"(r2), [r3] "r"(r3), [r4] "r"(r4), [r5] "r"(r5)
      : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14",
        "ymm15", "xmm0", "xmm1", "xmm2", "memory", "cc");
  (void)ap;
  (void)bp;
  (void)kk;
}

// INLINE ASM with 4x K-unroll
[[gnu::hot, gnu::always_inline]] inline void
micro_kernel_6x8_avx2_f64_asm_unr4(const double *Ap_in, const double *Bp_in, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                                   ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<6, 8, double>(Ap_in, Bp_in, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  double *r4 = C + 4 * rs_C;
  double *r5 = C + 5 * rs_C;
  const double *ap = Ap_in;
  const double *bp = Bp_in;
  usize k4 = k >> 2;              // number of 4-K-iter blocks
  usize ktail = k & usize(3);     // 0..3 leftover K-iters

  __asm__ volatile(
      // zero 12 accs
      "vxorpd %%ymm4,  %%ymm4,  %%ymm4 \n\t"
      "vxorpd %%ymm5,  %%ymm5,  %%ymm5 \n\t"
      "vxorpd %%ymm6,  %%ymm6,  %%ymm6 \n\t"
      "vxorpd %%ymm7,  %%ymm7,  %%ymm7 \n\t"
      "vxorpd %%ymm8,  %%ymm8,  %%ymm8 \n\t"
      "vxorpd %%ymm9,  %%ymm9,  %%ymm9 \n\t"
      "vxorpd %%ymm10, %%ymm10, %%ymm10 \n\t"
      "vxorpd %%ymm11, %%ymm11, %%ymm11 \n\t"
      "vxorpd %%ymm12, %%ymm12, %%ymm12 \n\t"
      "vxorpd %%ymm13, %%ymm13, %%ymm13 \n\t"
      "vxorpd %%ymm14, %%ymm14, %%ymm14 \n\t"
      "vxorpd %%ymm15, %%ymm15, %%ymm15 \n\t"

      "testq %[k4], %[k4] \n\t"
      "jz 5f \n\t"

      // K-unroll-4 loop body
      ".p2align 4 \n\t"
      "1: \n\t"
      // 0 roll
      "vmovupd  0(%[bp]), %%ymm0 \n\t"
      "vmovupd 32(%[bp]), %%ymm1 \n\t"
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"

      // 1 roll
      "vmovupd 64(%[bp]), %%ymm0 \n\t"
      "vmovupd 96(%[bp]), %%ymm1 \n\t"
      "vbroadcastsd 48(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 56(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "prefetcht0 1024(%[bp]) \n\t"     // 4 outer-iters ahead for B (16 K-iters)
      "vbroadcastsd 64(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 72(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 80(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 88(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"

      // 2 roll
      "vmovupd 128(%[bp]), %%ymm0 \n\t"
      "vmovupd 160(%[bp]), %%ymm1 \n\t"
      "vbroadcastsd  96(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 104(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "prefetcht0 960(%[ap]) \n\t"     // ~5 outer-iters ahead for A (20 K-iters * 48 = 960)
      "vbroadcastsd 112(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 120(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 128(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 136(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"

      // 3 roll
      "vmovupd 192(%[bp]), %%ymm0 \n\t"
      "vmovupd 224(%[bp]), %%ymm1 \n\t"
      "vbroadcastsd 144(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 152(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 160(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 168(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 176(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 184(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"

      "addq $192, %[ap] \n\t"     // 4 K-iters × 6 doubles × 8 bytes
      "addq $256, %[bp] \n\t"     // 4 K-iters × 8 doubles × 8 bytes
      "decq %[k4] \n\t"
      "jne 1b \n\t"

      "5: \n\t"
      // tail
      "testq %[ktail], %[ktail] \n\t"
      "jz 2f \n\t"
      ".p2align 4 \n\t"
      "6: \n\t"
      "vmovupd  0(%[bp]), %%ymm0 \n\t"
      "vmovupd 32(%[bp]), %%ymm1 \n\t"
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      "addq $48, %[ap] \n\t"
      "addq $64, %[bp] \n\t"
      "decq %[ktail] \n\t"
      "jne 6b \n\t"

      "2: \n\t"
      // alpha broadcast, beta check, store
      "vbroadcastsd %[alpha], %%ymm0 \n\t"
      "vxorpd %%xmm1, %%xmm1, %%xmm1 \n\t"
      "vucomisd %[beta], %%xmm1 \n\t"
      "jne 3f \n\t"
      "vmulpd %%ymm4,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd %%ymm5,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd %%ymm6,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd %%ymm7,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd %%ymm8,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd %%ymm9,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "jmp 4f \n\t"
      "3: \n\t"
      "vbroadcastsd %[beta], %%ymm1 \n\t"
      "vmulpd  (%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm4, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd 32(%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm5, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd  (%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm6, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd 32(%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm7, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd  (%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm8, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd 32(%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm9, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd  (%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd 32(%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd  (%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd 32(%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd  (%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd 32(%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "4: \n\t"
      "vzeroupper \n\t"
      : [ap] "+r"(ap), [bp] "+r"(bp), [k4] "+r"(k4), [ktail] "+r"(ktail)
      : [alpha] "m"(alpha), [beta] "m"(beta), [r0] "r"(r0), [r1] "r"(r1), [r2] "r"(r2), [r3] "r"(r3), [r4] "r"(r4), [r5] "r"(r5)
      : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14",
        "ymm15", "xmm0", "xmm1", "xmm2", "memory", "cc");
  (void)ap;
  (void)bp;
  (void)k4;
  (void)ktail;
}

// 6x8 f64 microkernel: K-unroll-4 + software-pipelined B loads (best performing so far!)
[[gnu::hot, gnu::always_inline]] inline void
micro_kernel_6x8_avx2_f64_asm_unr4_sp(const double *Ap_in, const double *Bp_in, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                                      ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<6, 8, double>(Ap_in, Bp_in, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  double *r4 = C + 4 * rs_C;
  double *r5 = C + 5 * rs_C;
  const double *ap = Ap_in;
  const double *bp = Bp_in;
  usize k4 = k >> 2;
  usize ktail = k & usize(3);

  __asm__ volatile(
      "vxorpd %%ymm4,  %%ymm4,  %%ymm4 \n\t"
      "vxorpd %%ymm5,  %%ymm5,  %%ymm5 \n\t"
      "vxorpd %%ymm6,  %%ymm6,  %%ymm6 \n\t"
      "vxorpd %%ymm7,  %%ymm7,  %%ymm7 \n\t"
      "vxorpd %%ymm8,  %%ymm8,  %%ymm8 \n\t"
      "vxorpd %%ymm9,  %%ymm9,  %%ymm9 \n\t"
      "vxorpd %%ymm10, %%ymm10, %%ymm10 \n\t"
      "vxorpd %%ymm11, %%ymm11, %%ymm11 \n\t"
      "vxorpd %%ymm12, %%ymm12, %%ymm12 \n\t"
      "vxorpd %%ymm13, %%ymm13, %%ymm13 \n\t"
      "vxorpd %%ymm14, %%ymm14, %%ymm14 \n\t"
      "vxorpd %%ymm15, %%ymm15, %%ymm15 \n\t"

      // preload B for K-iter 0 of either the unrolled loop or the
      // tail loop (whichever fires)
      "vmovupd  0(%[bp]), %%ymm0 \n\t"
      "vmovupd 32(%[bp]), %%ymm1 \n\t"

      "testq %[k4], %[k4] \n\t"
      "jz 5f \n\t"

      // K-unroll-4 loop with end-of-iter B-preload
      ".p2align 4 \n\t"
      "1: \n\t"
      // 0 roll
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=0: load B for K=1
      "vmovupd 64(%[bp]), %%ymm0 \n\t"
      "vmovupd 96(%[bp]), %%ymm1 \n\t"

      // 1 roll
      "vbroadcastsd 48(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 56(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "prefetcht0 1024(%[bp]) \n\t"     // 4 outer-iters ahead for B
      "vbroadcastsd 64(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 72(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 80(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 88(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=1: load B for K=2.
      "vmovupd 128(%[bp]), %%ymm0 \n\t"
      "vmovupd 160(%[bp]), %%ymm1 \n\t"

      // 2 roll
      "vbroadcastsd  96(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 104(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "prefetcht0 960(%[ap]) \n\t"     // ~5 outer-iters ahead for A
      "vbroadcastsd 112(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 120(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 128(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 136(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=2: load B for K=3
      "vmovupd 192(%[bp]), %%ymm0 \n\t"
      "vmovupd 224(%[bp]), %%ymm1 \n\t"

      // 3 roll
      "vbroadcastsd 144(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 152(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 160(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 168(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 176(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 184(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=3: load B for NEXT OUTER-ITRs K=0 (offset 256 from current bp)
      "vmovupd 256(%[bp]), %%ymm0 \n\t"
      "vmovupd 288(%[bp]), %%ymm1 \n\t"

      "addq $192, %[ap] \n\t"
      "addq $256, %[bp] \n\t"
      "decq %[k4] \n\t"
      "jne 1b \n\t"

      "5: \n\t"
      // tail
      "testq %[ktail], %[ktail] \n\t"
      "jz 2f \n\t"
      ".p2align 4 \n\t"
      "6: \n\t"
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      "vmovupd 64(%[bp]), %%ymm0 \n\t"     // preload B for next tail iter
      "vmovupd 96(%[bp]), %%ymm1 \n\t"
      "addq $48, %[ap] \n\t"
      "addq $64, %[bp] \n\t"
      "decq %[ktail] \n\t"
      "jne 6b \n\t"

      "2: \n\t"
      "vbroadcastsd %[alpha], %%ymm0 \n\t"
      "vxorpd %%xmm1, %%xmm1, %%xmm1 \n\t"
      "vucomisd %[beta], %%xmm1 \n\t"
      "jne 3f \n\t"
      // beta == 0 path
      "vmulpd %%ymm4,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd %%ymm5,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd %%ymm6,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd %%ymm7,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd %%ymm8,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd %%ymm9,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "jmp 4f \n\t"
      "3: \n\t"
      "vbroadcastsd %[beta], %%ymm1 \n\t"
      "vmulpd  (%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm4, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd 32(%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm5, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd  (%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm6, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd 32(%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm7, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd  (%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm8, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd 32(%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm9, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd  (%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd 32(%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd  (%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd 32(%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd  (%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd 32(%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "4: \n\t"
      "vzeroupper \n\t"
      : [ap] "+r"(ap), [bp] "+r"(bp), [k4] "+r"(k4), [ktail] "+r"(ktail)
      : [alpha] "m"(alpha), [beta] "m"(beta), [r0] "r"(r0), [r1] "r"(r1), [r2] "r"(r2), [r3] "r"(r3), [r4] "r"(r4), [r5] "r"(r5)
      : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14",
        "ymm15", "xmm0", "xmm1", "xmm2", "memory", "cc");
  (void)ap;
  (void)bp;
  (void)k4;
  (void)ktail;
}

// 6x8 f64 microkernel, aligned var
[[gnu::hot, gnu::always_inline]] inline void
micro_kernel_6x8_avx2_f64_asm_aligned(const double *Ap_in, const double *Bp_in, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                                      ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<6, 8, double>(Ap_in, Bp_in, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  double *r4 = C + 4 * rs_C;
  double *r5 = C + 5 * rs_C;
  const double *ap = Ap_in;
  const double *bp = Bp_in;
  usize kk = k;

  __asm__ volatile(
      "vxorpd %%ymm4,  %%ymm4,  %%ymm4 \n\t"
      "vxorpd %%ymm5,  %%ymm5,  %%ymm5 \n\t"
      "vxorpd %%ymm6,  %%ymm6,  %%ymm6 \n\t"
      "vxorpd %%ymm7,  %%ymm7,  %%ymm7 \n\t"
      "vxorpd %%ymm8,  %%ymm8,  %%ymm8 \n\t"
      "vxorpd %%ymm9,  %%ymm9,  %%ymm9 \n\t"
      "vxorpd %%ymm10, %%ymm10, %%ymm10 \n\t"
      "vxorpd %%ymm11, %%ymm11, %%ymm11 \n\t"
      "vxorpd %%ymm12, %%ymm12, %%ymm12 \n\t"
      "vxorpd %%ymm13, %%ymm13, %%ymm13 \n\t"
      "vxorpd %%ymm14, %%ymm14, %%ymm14 \n\t"
      "vxorpd %%ymm15, %%ymm15, %%ymm15 \n\t"
      "testq %[k], %[k] \n\t"
      "jz 2f \n\t"
      ".p2align 4 \n\t"
      "1: \n\t"
      "vmovapd  0(%[bp]), %%ymm0 \n\t"
      "vmovapd 32(%[bp]), %%ymm1 \n\t"
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "prefetcht0 512(%[bp]) \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "prefetcht0 480(%[ap]) \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      "addq $48, %[ap] \n\t"
      "addq $64, %[bp] \n\t"
      "decq %[k] \n\t"
      "jne 1b \n\t"
      "2: \n\t"
      "vbroadcastsd %[alpha], %%ymm0 \n\t"
      "vxorpd %%xmm1, %%xmm1, %%xmm1 \n\t"
      "vucomisd %[beta], %%xmm1 \n\t"
      "jne 3f \n\t"
      // beta == 0 path: store alpha*acc (aligned)
      "vmulpd %%ymm4,  %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r0]) \n\t"
      "vmulpd %%ymm5,  %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd %%ymm6,  %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r1]) \n\t"
      "vmulpd %%ymm7,  %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd %%ymm8,  %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r2]) \n\t"
      "vmulpd %%ymm9,  %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r3]) \n\t"
      "vmulpd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r4]) \n\t"
      "vmulpd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r5]) \n\t"
      "vmulpd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r5]) \n\t"
      "jmp 4f \n\t"
      "3: \n\t"
      "vbroadcastsd %[beta], %%ymm1 \n\t"
      "vmulpd  (%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm4, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r0]) \n\t"
      "vmulpd 32(%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm5, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd  (%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm6, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r1]) \n\t"
      "vmulpd 32(%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm7, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd  (%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm8, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r2]) \n\t"
      "vmulpd 32(%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm9, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd  (%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r3]) \n\t"
      "vmulpd 32(%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd  (%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r4]) \n\t"
      "vmulpd 32(%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd  (%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, (%[r5]) \n\t"
      "vmulpd 32(%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovapd %%ymm2, 32(%[r5]) \n\t"
      "4: \n\t"
      "vzeroupper \n\t"
      : [ap] "+r"(ap), [bp] "+r"(bp), [k] "+r"(kk)
      : [alpha] "m"(alpha), [beta] "m"(beta), [r0] "r"(r0), [r1] "r"(r1), [r2] "r"(r2), [r3] "r"(r3), [r4] "r"(r4), [r5] "r"(r5)
      : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14",
        "ymm15", "xmm0", "xmm1", "xmm2", "memory", "cc");
  (void)ap;
  (void)bp;
  (void)kk;
}

// experimental: relaxed-frontend 6x8
[[gnu::hot, gnu::always_inline]] inline void
micro_kernel_6x8_avx2_f64_asm_relax(const double *Ap_in, const double *Bp_in, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                                    ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<6, 8, double>(Ap_in, Bp_in, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  double *r4 = C + 4 * rs_C;
  double *r5 = C + 5 * rs_C;
  const double *ap = Ap_in;
  const double *bp = Bp_in;
  usize k2 = k >> 1;
  usize ktail = k & usize(1);

  __asm__ volatile(
      // zero 12 accumulators
      "vxorpd %%ymm4,  %%ymm4,  %%ymm4 \n\t"
      "vxorpd %%ymm5,  %%ymm5,  %%ymm5 \n\t"
      "vxorpd %%ymm6,  %%ymm6,  %%ymm6 \n\t"
      "vxorpd %%ymm7,  %%ymm7,  %%ymm7 \n\t"
      "vxorpd %%ymm8,  %%ymm8,  %%ymm8 \n\t"
      "vxorpd %%ymm9,  %%ymm9,  %%ymm9 \n\t"
      "vxorpd %%ymm10, %%ymm10, %%ymm10 \n\t"
      "vxorpd %%ymm11, %%ymm11, %%ymm11 \n\t"
      "vxorpd %%ymm12, %%ymm12, %%ymm12 \n\t"
      "vxorpd %%ymm13, %%ymm13, %%ymm13 \n\t"
      "vxorpd %%ymm14, %%ymm14, %%ymm14 \n\t"
      "vxorpd %%ymm15, %%ymm15, %%ymm15 \n\t"

      // initial B preload for K=0 of first iter (or first tail iter if k2==0)
      "vmovupd  0(%[bp]), %%ymm0 \n\t"
      "vmovupd 32(%[bp]), %%ymm1 \n\t"

      "testq %[k2], %[k2] \n\t"
      "jz 5f \n\t"

      // K-unroll-2 loop body. No .p2align. No prefetcht0.
      "1: \n\t"
      // K=0 (B in ymm0/1 from preceding preload)
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=0 end: SP B-preload for K=1
      "vmovupd 64(%[bp]), %%ymm0 \n\t"
      "vmovupd 96(%[bp]), %%ymm1 \n\t"

      // K=1
      "vbroadcastsd 48(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 56(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 64(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 72(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 80(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 88(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=1 end: SP B-preload for NEXT outer iter K=0 (offset 128 from current bp)
      "vmovupd 128(%[bp]), %%ymm0 \n\t"
      "vmovupd 160(%[bp]), %%ymm1 \n\t"

      "addq $96, %[ap] \n\t"      // 2 K-iters × 6 doubles × 8 bytes
      "addq $128, %[bp] \n\t"     // 2 K-iters × 8 doubles × 8 bytes
      "decq %[k2] \n\t"
      "jne 1b \n\t"

      "5: \n\t"
      // tail (one K-iter at most). ymm0/1 already contain the right B from
      // either the initial preload (k2==0) or the loop's cross-iter preload.
      "testq %[ktail], %[ktail] \n\t"
      "jz 2f \n\t"
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"

      "2: \n\t"
      // alpha bcast, beta test, separate store paths (this is the "separate
      // beta==0 kernel" path — pure mul on the beta==0 fork, fma+mul on betanz)
      "vbroadcastsd %[alpha], %%ymm0 \n\t"
      "vxorpd %%xmm1, %%xmm1, %%xmm1 \n\t"
      "vucomisd %[beta], %%xmm1 \n\t"
      "jne 3f \n\t"
      // beta == 0 store
      "vmulpd %%ymm4,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd %%ymm5,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd %%ymm6,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd %%ymm7,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd %%ymm8,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd %%ymm9,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "jmp 4f \n\t"
      "3: \n\t"
      "vbroadcastsd %[beta], %%ymm1 \n\t"
      "vmulpd  (%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm4, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd 32(%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm5, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd  (%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm6, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd 32(%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm7, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd  (%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm8, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd 32(%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm9, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd  (%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd 32(%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd  (%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd 32(%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd  (%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd 32(%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "4: \n\t"
      "vzeroupper \n\t"
      : [ap] "+r"(ap), [bp] "+r"(bp), [k2] "+r"(k2), [ktail] "+r"(ktail)
      : [alpha] "m"(alpha), [beta] "m"(beta), [r0] "r"(r0), [r1] "r"(r1), [r2] "r"(r2), [r3] "r"(r3), [r4] "r"(r4), [r5] "r"(r5)
      : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14",
        "ymm15", "xmm0", "xmm1", "xmm2", "memory", "cc");
  (void)ap;
  (void)bp;
  (void)k2;
  (void)ktail;
}

// experimental: 4x12 microkernel
[[gnu::hot, gnu::always_inline]] inline void
micro_kernel_4x12_avx2_f64_asm_unr4_sp(const double *Ap_in, const double *Bp_in, usize k, double alpha, double beta, double *C,
                                       ssize_t rs_C, ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<4, 12, double>(Ap_in, Bp_in, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  const double *ap = Ap_in;
  const double *bp = Bp_in;
  usize k4 = k >> 2;
  usize ktail = k & usize(3);

  __asm__ volatile("vxorpd %%ymm4,  %%ymm4,  %%ymm4 \n\t"
                   "vxorpd %%ymm5,  %%ymm5,  %%ymm5 \n\t"
                   "vxorpd %%ymm6,  %%ymm6,  %%ymm6 \n\t"
                   "vxorpd %%ymm7,  %%ymm7,  %%ymm7 \n\t"
                   "vxorpd %%ymm8,  %%ymm8,  %%ymm8 \n\t"
                   "vxorpd %%ymm9,  %%ymm9,  %%ymm9 \n\t"
                   "vxorpd %%ymm10, %%ymm10, %%ymm10 \n\t"
                   "vxorpd %%ymm11, %%ymm11, %%ymm11 \n\t"
                   "vxorpd %%ymm12, %%ymm12, %%ymm12 \n\t"
                   "vxorpd %%ymm13, %%ymm13, %%ymm13 \n\t"
                   "vxorpd %%ymm14, %%ymm14, %%ymm14 \n\t"
                   "vxorpd %%ymm15, %%ymm15, %%ymm15 \n\t"

                   // initial B preload (3 vecs of 4 doubles = 12 cols)
                   "vmovupd  0(%[bp]), %%ymm0 \n\t"
                   "vmovupd 32(%[bp]), %%ymm1 \n\t"
                   "vmovupd 64(%[bp]), %%ymm2 \n\t"

                   "testq %[k4], %[k4] \n\t"
                   "jz 5f \n\t"

                   ".p2align 4 \n\t"
                   "1: \n\t"
                   // K=0: ap +0..+24, bp +0..+64 (already loaded). 4 bcasts × 3 FMAs = 12 FMAs.
                   "vbroadcastsd  0(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm4 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm5 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm6 \n\t"
                   "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm7 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm8 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm9 \n\t"
                   "vbroadcastsd 16(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm12 \n\t"
                   "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm13 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm14 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm15 \n\t"
                   // K=0 end: SP preload K=1's B
                   "vmovupd  96(%[bp]), %%ymm0 \n\t"
                   "vmovupd 128(%[bp]), %%ymm1 \n\t"
                   "vmovupd 160(%[bp]), %%ymm2 \n\t"

                   // K=1: ap +32..+56
                   "vbroadcastsd 32(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm4 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm5 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm6 \n\t"
                   "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm7 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm8 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm9 \n\t"
                   "prefetcht0 1536(%[bp]) \n\t"     // ~4 outer-iters ahead for B
                   "vbroadcastsd 48(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm12 \n\t"
                   "vbroadcastsd 56(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm13 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm14 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm15 \n\t"
                   "vmovupd 192(%[bp]), %%ymm0 \n\t"
                   "vmovupd 224(%[bp]), %%ymm1 \n\t"
                   "vmovupd 256(%[bp]), %%ymm2 \n\t"

                   // K=2: ap +64..+88
                   "vbroadcastsd 64(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm4 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm5 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm6 \n\t"
                   "prefetcht0 640(%[ap]) \n\t"     // ~5 outer-iters ahead for A (20 K-iters × 32 = 640)
                   "vbroadcastsd 72(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm7 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm8 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm9 \n\t"
                   "vbroadcastsd 80(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm12 \n\t"
                   "vbroadcastsd 88(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm13 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm14 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm15 \n\t"
                   "vmovupd 288(%[bp]), %%ymm0 \n\t"
                   "vmovupd 320(%[bp]), %%ymm1 \n\t"
                   "vmovupd 352(%[bp]), %%ymm2 \n\t"

                   // K=3: ap +96..+120
                   "vbroadcastsd  96(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm4 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm5 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm6 \n\t"
                   "vbroadcastsd 104(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm7 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm8 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm9 \n\t"
                   "vbroadcastsd 112(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm12 \n\t"
                   "vbroadcastsd 120(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm13 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm14 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm15 \n\t"
                   // K=3 end: SP B-preload for NEXT outer iter K=0 (offset +384..+448)
                   "vmovupd 384(%[bp]), %%ymm0 \n\t"
                   "vmovupd 416(%[bp]), %%ymm1 \n\t"
                   "vmovupd 448(%[bp]), %%ymm2 \n\t"

                   "addq $128, %[ap] \n\t"     // 4 K × 4 doubles × 8 bytes
                   "addq $384, %[bp] \n\t"     // 4 K × 12 doubles × 8 bytes
                   "decq %[k4] \n\t"
                   "jne 1b \n\t"

                   "5: \n\t"
                   "testq %[ktail], %[ktail] \n\t"
                   "jz 2f \n\t"
                   ".p2align 4 \n\t"
                   "6: \n\t"
                   // tail K-step with mid-step B-preload for the next tail iter
                   "vbroadcastsd  0(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm4 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm5 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm6 \n\t"
                   "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm7 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm8 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm9 \n\t"
                   "vbroadcastsd 16(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm12 \n\t"
                   "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
                   "vfmadd231pd %%ymm0, %%ymm3, %%ymm13 \n\t"
                   "vfmadd231pd %%ymm1, %%ymm3, %%ymm14 \n\t"
                   "vfmadd231pd %%ymm2, %%ymm3, %%ymm15 \n\t"
                   "vmovupd 96(%[bp]), %%ymm0 \n\t"
                   "vmovupd 128(%[bp]), %%ymm1 \n\t"
                   "vmovupd 160(%[bp]), %%ymm2 \n\t"
                   "addq $32, %[ap] \n\t"
                   "addq $96, %[bp] \n\t"
                   "decq %[ktail] \n\t"
                   "jne 6b \n\t"

                   "2: \n\t"
                   "vbroadcastsd %[alpha], %%ymm0 \n\t"
                   "vxorpd %%xmm1, %%xmm1, %%xmm1 \n\t"
                   "vucomisd %[beta], %%xmm1 \n\t"
                   "jne 3f \n\t"
                   // beta == 0
                   "vmulpd %%ymm4,  %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r0]) \n\t"
                   "vmulpd %%ymm5,  %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r0]) \n\t"
                   "vmulpd %%ymm6,  %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r0]) \n\t"
                   "vmulpd %%ymm7,  %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r1]) \n\t"
                   "vmulpd %%ymm8,  %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r1]) \n\t"
                   "vmulpd %%ymm9,  %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r1]) \n\t"
                   "vmulpd %%ymm10, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r2]) \n\t"
                   "vmulpd %%ymm11, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r2]) \n\t"
                   "vmulpd %%ymm12, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r2]) \n\t"
                   "vmulpd %%ymm13, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r3]) \n\t"
                   "vmulpd %%ymm14, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r3]) \n\t"
                   "vmulpd %%ymm15, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r3]) \n\t"
                   "jmp 4f \n\t"
                   "3: \n\t"
                   "vbroadcastsd %[beta], %%ymm1 \n\t"
                   "vmulpd  (%[r0]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm4, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r0]) \n\t"
                   "vmulpd 32(%[r0]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm5, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r0]) \n\t"
                   "vmulpd 64(%[r0]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm6, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r0]) \n\t"
                   "vmulpd  (%[r1]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm7, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r1]) \n\t"
                   "vmulpd 32(%[r1]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm8, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r1]) \n\t"
                   "vmulpd 64(%[r1]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm9, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r1]) \n\t"
                   "vmulpd  (%[r2]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm10, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r2]) \n\t"
                   "vmulpd 32(%[r2]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm11, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r2]) \n\t"
                   "vmulpd 64(%[r2]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm12, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r2]) \n\t"
                   "vmulpd  (%[r3]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm13, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, (%[r3]) \n\t"
                   "vmulpd 32(%[r3]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm14, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 32(%[r3]) \n\t"
                   "vmulpd 64(%[r3]), %%ymm1, %%ymm2 \n\t"
                   "vfmadd231pd %%ymm15, %%ymm0, %%ymm2 \n\t"
                   "vmovupd %%ymm2, 64(%[r3]) \n\t"
                   "4: \n\t"
                   "vzeroupper \n\t"
                   : [ap] "+r"(ap), [bp] "+r"(bp), [k4] "+r"(k4), [ktail] "+r"(ktail)
                   : [alpha] "m"(alpha), [beta] "m"(beta), [r0] "r"(r0), [r1] "r"(r1), [r2] "r"(r2), [r3] "r"(r3)
                   : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13",
                     "ymm14", "ymm15", "xmm0", "xmm1", "xmm2", "memory", "cc");
  (void)ap;
  (void)bp;
  (void)k4;
  (void)ktail;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// experimental: 6x8 K-unroll-4 + SP B-preload + centered rolling pointer
[[gnu::hot, gnu::always_inline]] inline void
micro_kernel_6x8_avx2_f64_asm_roll(const double *Ap_in, const double *Bp_in, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                                   ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<6, 8, double>(Ap_in, Bp_in, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  double *r4 = C + 4 * rs_C;
  double *r5 = C + 5 * rs_C;
  const double *ap = Ap_in;
  const double *bp = Bp_in;
  usize k4 = k >> 2;
  usize ktail = k & usize(3);

  __asm__ volatile(
      "vxorpd %%ymm4,  %%ymm4,  %%ymm4 \n\t"
      "vxorpd %%ymm5,  %%ymm5,  %%ymm5 \n\t"
      "vxorpd %%ymm6,  %%ymm6,  %%ymm6 \n\t"
      "vxorpd %%ymm7,  %%ymm7,  %%ymm7 \n\t"
      "vxorpd %%ymm8,  %%ymm8,  %%ymm8 \n\t"
      "vxorpd %%ymm9,  %%ymm9,  %%ymm9 \n\t"
      "vxorpd %%ymm10, %%ymm10, %%ymm10 \n\t"
      "vxorpd %%ymm11, %%ymm11, %%ymm11 \n\t"
      "vxorpd %%ymm12, %%ymm12, %%ymm12 \n\t"
      "vxorpd %%ymm13, %%ymm13, %%ymm13 \n\t"
      "vxorpd %%ymm14, %%ymm14, %%ymm14 \n\t"
      "vxorpd %%ymm15, %%ymm15, %%ymm15 \n\t"

      // Only enter the centered path if we have at least one unr-4 iter.
      // For the no-loop case, ap/bp must stay un-centered so the tail uses
      // normal positive disps.
      "testq %[k4], %[k4] \n\t"
      "jz 7f \n\t"

      // Pre-bump pointers to center: now disp8 fits every K-step's operands.
      "addq $96, %[ap] \n\t"
      "addq $128, %[bp] \n\t"

      // Initial B preload for K=0 (centered: bp-128, bp-96 — both disp8).
      "vmovupd -128(%[bp]), %%ymm0 \n\t"
      "vmovupd  -96(%[bp]), %%ymm1 \n\t"

      ".p2align 4 \n\t"
      "1: \n\t"
      // K=0 (ap base -96..-56, bp base -128..-96)
      "vbroadcastsd -96(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd -88(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd -80(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd -72(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd -64(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd -56(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=0 end: SP preload K=1's B (disp -64, -32 — disp8)
      "vmovupd -64(%[bp]), %%ymm0 \n\t"
      "vmovupd -32(%[bp]), %%ymm1 \n\t"

      // K=1 (ap -48..-8, bp -64..-32)
      "vbroadcastsd -48(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd -40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "prefetcht0 896(%[bp]) \n\t"     // 1024 ahead of centered bp - 128 center offset
      "vbroadcastsd -32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd -24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd -16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  -8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=1 end: SP preload K=2's B (disp 0, +32 — disp8)
      "vmovupd   0(%[bp]), %%ymm0 \n\t"
      "vmovupd  32(%[bp]), %%ymm1 \n\t"

      // K=2 (ap 0..40, bp 0..32)
      "vbroadcastsd   0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd   8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "prefetcht0 864(%[ap]) \n\t"     // 960 ahead of centered ap - 96 center offset
      "vbroadcastsd  16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd  32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=2 end: SP preload K=3's B (disp +64, +96 — disp8)
      "vmovupd  64(%[bp]), %%ymm0 \n\t"
      "vmovupd  96(%[bp]), %%ymm1 \n\t"

      // K=3 (ap 48..88, bp 64..96)
      "vbroadcastsd  48(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  56(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd  64(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  72(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd  80(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  88(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      // K=3 end: cross-iter SP B-preload (disp +128, +160 — falls back to disp32)
      "vmovupd 128(%[bp]), %%ymm0 \n\t"
      "vmovupd 160(%[bp]), %%ymm1 \n\t"

      "addq $192, %[ap] \n\t"
      "addq $256, %[bp] \n\t"
      "decq %[k4] \n\t"
      "jne 1b \n\t"

      // Un-center for tail / fallthrough
      "subq $96, %[ap] \n\t"
      "subq $128, %[bp] \n\t"

      "7: \n\t"
      // tail (same as the un-aligned _unr4_sp tail; un-centered pointers)
      "testq %[ktail], %[ktail] \n\t"
      "jz 2f \n\t"
      ".p2align 4 \n\t"
      "6: \n\t"
      "vmovupd  0(%[bp]), %%ymm0 \n\t"
      "vmovupd 32(%[bp]), %%ymm1 \n\t"
      "vbroadcastsd  0(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd  8(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm4 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm5 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm6 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm7 \n\t"
      "vbroadcastsd 16(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 24(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm8 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm9 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm10 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm11 \n\t"
      "vbroadcastsd 32(%[ap]), %%ymm2 \n\t"
      "vbroadcastsd 40(%[ap]), %%ymm3 \n\t"
      "vfmadd231pd %%ymm0, %%ymm2, %%ymm12 \n\t"
      "vfmadd231pd %%ymm1, %%ymm2, %%ymm13 \n\t"
      "vfmadd231pd %%ymm0, %%ymm3, %%ymm14 \n\t"
      "vfmadd231pd %%ymm1, %%ymm3, %%ymm15 \n\t"
      "addq $48, %[ap] \n\t"
      "addq $64, %[bp] \n\t"
      "decq %[ktail] \n\t"
      "jne 6b \n\t"

      "2: \n\t"
      "vbroadcastsd %[alpha], %%ymm0 \n\t"
      "vxorpd %%xmm1, %%xmm1, %%xmm1 \n\t"
      "vucomisd %[beta], %%xmm1 \n\t"
      "jne 3f \n\t"
      "vmulpd %%ymm4,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd %%ymm5,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd %%ymm6,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd %%ymm7,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd %%ymm8,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd %%ymm9,  %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "jmp 4f \n\t"
      "3: \n\t"
      "vbroadcastsd %[beta], %%ymm1 \n\t"
      "vmulpd  (%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm4, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r0]) \n\t"
      "vmulpd 32(%[r0]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm5, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r0]) \n\t"
      "vmulpd  (%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm6, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r1]) \n\t"
      "vmulpd 32(%[r1]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm7, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r1]) \n\t"
      "vmulpd  (%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm8, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r2]) \n\t"
      "vmulpd 32(%[r2]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm9, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r2]) \n\t"
      "vmulpd  (%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm10, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r3]) \n\t"
      "vmulpd 32(%[r3]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm11, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r3]) \n\t"
      "vmulpd  (%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm12, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r4]) \n\t"
      "vmulpd 32(%[r4]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm13, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r4]) \n\t"
      "vmulpd  (%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm14, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, (%[r5]) \n\t"
      "vmulpd 32(%[r5]), %%ymm1, %%ymm2 \n\t"
      "vfmadd231pd %%ymm15, %%ymm0, %%ymm2 \n\t"
      "vmovupd %%ymm2, 32(%[r5]) \n\t"
      "4: \n\t"
      "vzeroupper \n\t"
      : [ap] "+r"(ap), [bp] "+r"(bp), [k4] "+r"(k4), [ktail] "+r"(ktail)
      : [alpha] "m"(alpha), [beta] "m"(beta), [r0] "r"(r0), [r1] "r"(r1), [r2] "r"(r2), [r3] "r"(r3), [r4] "r"(r4), [r5] "r"(r5)
      : "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14",
        "ymm15", "xmm0", "xmm1", "xmm2", "memory", "cc");
  (void)ap;
  (void)bp;
  (void)k4;
  (void)ktail;
}

[[gnu::flatten, gnu::always_inline]] inline void
pack_b_panel_12_f64(const double *B, ssize_t rs_B, ssize_t cs_B, usize k, usize n, double *dst) noexcept
{
  for ( usize t = 0; t < n; t += 12 ) {
    const usize cols = (n - t < 12) ? (n - t) : 12;
    if ( cols == 12 && cs_B == 1 ) {
      for ( usize p = 0; p < k; ++p ) {
        const double *src = B + ssize_t(p) * rs_B + ssize_t(t);
        simd::avx::storeu_f64(dst + 0, simd::avx::loadu_f64(src + 0));
        simd::avx::storeu_f64(dst + 4, simd::avx::loadu_f64(src + 4));
        simd::avx::storeu_f64(dst + 8, simd::avx::loadu_f64(src + 8));
        dst += 12;
      }
    } else {
      for ( usize p = 0; p < k; ++p ) {
        for ( usize j = 0; j < cols; ++j ) dst[j] = B[ssize_t(p) * rs_B + ssize_t(t + j) * cs_B];
        for ( usize j = cols; j < 12; ++j ) dst[j] = 0.0;
        dst += 12;
      }
    }
  }
}

// 8 rows × 8 cols f32 microkernel (AVX2 ymm = 8 floats wide)
[[gnu::flatten, gnu::always_inline]] inline void
micro_kernel_8x8_avx2_f32(const float *Ap, const float *Bp, usize k, float alpha, float beta, float *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<8, 8, float>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  // prefetch the 8 C rows we will read/write at the epilogue
  __builtin_prefetch(C + ssize_t(0) * rs_C, 1, 1);
  __builtin_prefetch(C + ssize_t(1) * rs_C, 1, 1);
  __builtin_prefetch(C + ssize_t(2) * rs_C, 1, 1);
  __builtin_prefetch(C + ssize_t(3) * rs_C, 1, 1);
  __builtin_prefetch(C + ssize_t(4) * rs_C, 1, 1);
  __builtin_prefetch(C + ssize_t(5) * rs_C, 1, 1);
  __builtin_prefetch(C + ssize_t(6) * rs_C, 1, 1);
  __builtin_prefetch(C + ssize_t(7) * rs_C, 1, 1);

  __m256 c0 = simd::avx::zero_f32(), c1 = simd::avx::zero_f32();
  __m256 c2 = simd::avx::zero_f32(), c3 = simd::avx::zero_f32();
  __m256 c4 = simd::avx::zero_f32(), c5 = simd::avx::zero_f32();
  __m256 c6 = simd::avx::zero_f32(), c7 = simd::avx::zero_f32();
  for ( usize p = 0; p < k; ++p ) {
    // light prefetch on A; B is dense-streamed and the HW prefetcher catches it
    __builtin_prefetch(Ap + (p + 16) * 8, 0, 3);
    const __m256 br = simd::avx::loadu_f32(Bp + p * 8);
    c0 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 0]), br, c0);
    c1 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 1]), br, c1);
    c2 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 2]), br, c2);
    c3 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 3]), br, c3);
    c4 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 4]), br, c4);
    c5 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 5]), br, c5);
    c6 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 6]), br, c6);
    c7 = simd::fma::fma_f32(simd::avx::splat_f32(Ap[p * 8 + 7]), br, c7);
  }
  // epilogue: unrolled to keep accumulators in registers
  const __m256 valpha = simd::avx::splat_f32(alpha);
  float *r0 = C + 0 * rs_C, *r1 = C + 1 * rs_C, *r2 = C + 2 * rs_C, *r3 = C + 3 * rs_C;
  float *r4 = C + 4 * rs_C, *r5 = C + 5 * rs_C, *r6 = C + 6 * rs_C, *r7 = C + 7 * rs_C;
  if ( beta == 0.0f ) {
    simd::avx::storeu_f32(r0, simd::avx::mul_f32(valpha, c0));
    simd::avx::storeu_f32(r1, simd::avx::mul_f32(valpha, c1));
    simd::avx::storeu_f32(r2, simd::avx::mul_f32(valpha, c2));
    simd::avx::storeu_f32(r3, simd::avx::mul_f32(valpha, c3));
    simd::avx::storeu_f32(r4, simd::avx::mul_f32(valpha, c4));
    simd::avx::storeu_f32(r5, simd::avx::mul_f32(valpha, c5));
    simd::avx::storeu_f32(r6, simd::avx::mul_f32(valpha, c6));
    simd::avx::storeu_f32(r7, simd::avx::mul_f32(valpha, c7));
  } else if ( beta == 1.0f ) {
    simd::avx::storeu_f32(r0, simd::fma::fma_f32(valpha, c0, simd::avx::loadu_f32(r0)));
    simd::avx::storeu_f32(r1, simd::fma::fma_f32(valpha, c1, simd::avx::loadu_f32(r1)));
    simd::avx::storeu_f32(r2, simd::fma::fma_f32(valpha, c2, simd::avx::loadu_f32(r2)));
    simd::avx::storeu_f32(r3, simd::fma::fma_f32(valpha, c3, simd::avx::loadu_f32(r3)));
    simd::avx::storeu_f32(r4, simd::fma::fma_f32(valpha, c4, simd::avx::loadu_f32(r4)));
    simd::avx::storeu_f32(r5, simd::fma::fma_f32(valpha, c5, simd::avx::loadu_f32(r5)));
    simd::avx::storeu_f32(r6, simd::fma::fma_f32(valpha, c6, simd::avx::loadu_f32(r6)));
    simd::avx::storeu_f32(r7, simd::fma::fma_f32(valpha, c7, simd::avx::loadu_f32(r7)));
  } else {
    const __m256 vbeta = simd::avx::splat_f32(beta);
    simd::avx::storeu_f32(r0, simd::fma::fma_f32(valpha, c0, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r0))));
    simd::avx::storeu_f32(r1, simd::fma::fma_f32(valpha, c1, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r1))));
    simd::avx::storeu_f32(r2, simd::fma::fma_f32(valpha, c2, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r2))));
    simd::avx::storeu_f32(r3, simd::fma::fma_f32(valpha, c3, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r3))));
    simd::avx::storeu_f32(r4, simd::fma::fma_f32(valpha, c4, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r4))));
    simd::avx::storeu_f32(r5, simd::fma::fma_f32(valpha, c5, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r5))));
    simd::avx::storeu_f32(r6, simd::fma::fma_f32(valpha, c6, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r6))));
    simd::avx::storeu_f32(r7, simd::fma::fma_f32(valpha, c7, simd::avx::mul_f32(vbeta, simd::avx::loadu_f32(r7))));
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
  float64x2_t c00 = simd::neon::splat_f64(0.0), c01 = simd::neon::splat_f64(0.0);
  float64x2_t c10 = simd::neon::splat_f64(0.0), c11 = simd::neon::splat_f64(0.0);
  float64x2_t c20 = simd::neon::splat_f64(0.0), c21 = simd::neon::splat_f64(0.0);
  float64x2_t c30 = simd::neon::splat_f64(0.0), c31 = simd::neon::splat_f64(0.0);
  for ( usize p = 0; p < k; ++p ) {
    const float64x2_t b0 = simd::neon::load_f64(Bp + p * 4 + 0);
    const float64x2_t b1 = simd::neon::load_f64(Bp + p * 4 + 2);
    const float64x2_t a0 = simd::neon::splat_f64(Ap[p * 4 + 0]);
    const float64x2_t a1 = simd::neon::splat_f64(Ap[p * 4 + 1]);
    const float64x2_t a2 = simd::neon::splat_f64(Ap[p * 4 + 2]);
    const float64x2_t a3 = simd::neon::splat_f64(Ap[p * 4 + 3]);
    c00 = simd::neon::fma_f64(c00, a0, b0);
    c01 = simd::neon::fma_f64(c01, a0, b1);
    c10 = simd::neon::fma_f64(c10, a1, b0);
    c11 = simd::neon::fma_f64(c11, a1, b1);
    c20 = simd::neon::fma_f64(c20, a2, b0);
    c21 = simd::neon::fma_f64(c21, a2, b1);
    c30 = simd::neon::fma_f64(c30, a3, b0);
    c31 = simd::neon::fma_f64(c31, a3, b1);
  }
  const float64x2_t valpha = simd::neon::splat_f64(alpha);
  double *r0 = C + 0 * rs_C;
  double *r1 = C + 1 * rs_C;
  double *r2 = C + 2 * rs_C;
  double *r3 = C + 3 * rs_C;
  if ( beta == 0.0 ) {
    simd::neon::store_f64(r0 + 0, simd::neon::mul(valpha, c00));
    simd::neon::store_f64(r0 + 2, simd::neon::mul(valpha, c01));
    simd::neon::store_f64(r1 + 0, simd::neon::mul(valpha, c10));
    simd::neon::store_f64(r1 + 2, simd::neon::mul(valpha, c11));
    simd::neon::store_f64(r2 + 0, simd::neon::mul(valpha, c20));
    simd::neon::store_f64(r2 + 2, simd::neon::mul(valpha, c21));
    simd::neon::store_f64(r3 + 0, simd::neon::mul(valpha, c30));
    simd::neon::store_f64(r3 + 2, simd::neon::mul(valpha, c31));
  } else if ( beta == 1.0 ) {
    simd::neon::store_f64(r0 + 0, simd::neon::fma_f64(simd::neon::load_f64(r0 + 0), valpha, c00));
    simd::neon::store_f64(r0 + 2, simd::neon::fma_f64(simd::neon::load_f64(r0 + 2), valpha, c01));
    simd::neon::store_f64(r1 + 0, simd::neon::fma_f64(simd::neon::load_f64(r1 + 0), valpha, c10));
    simd::neon::store_f64(r1 + 2, simd::neon::fma_f64(simd::neon::load_f64(r1 + 2), valpha, c11));
    simd::neon::store_f64(r2 + 0, simd::neon::fma_f64(simd::neon::load_f64(r2 + 0), valpha, c20));
    simd::neon::store_f64(r2 + 2, simd::neon::fma_f64(simd::neon::load_f64(r2 + 2), valpha, c21));
    simd::neon::store_f64(r3 + 0, simd::neon::fma_f64(simd::neon::load_f64(r3 + 0), valpha, c30));
    simd::neon::store_f64(r3 + 2, simd::neon::fma_f64(simd::neon::load_f64(r3 + 2), valpha, c31));
  } else {
    const float64x2_t vbeta = simd::neon::splat_f64(beta);
    simd::neon::store_f64(r0 + 0, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r0 + 0)), valpha, c00));
    simd::neon::store_f64(r0 + 2, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r0 + 2)), valpha, c01));
    simd::neon::store_f64(r1 + 0, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r1 + 0)), valpha, c10));
    simd::neon::store_f64(r1 + 2, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r1 + 2)), valpha, c11));
    simd::neon::store_f64(r2 + 0, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r2 + 0)), valpha, c20));
    simd::neon::store_f64(r2 + 2, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r2 + 2)), valpha, c21));
    simd::neon::store_f64(r3 + 0, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r3 + 0)), valpha, c30));
    simd::neon::store_f64(r3 + 2, simd::neon::fma_f64(simd::neon::mul(vbeta, simd::neon::load_f64(r3 + 2)), valpha, c31));
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
  float32x4_t c00 = simd::neon::splat_f32(0.0f), c01 = simd::neon::splat_f32(0.0f);
  float32x4_t c10 = simd::neon::splat_f32(0.0f), c11 = simd::neon::splat_f32(0.0f);
  float32x4_t c20 = simd::neon::splat_f32(0.0f), c21 = simd::neon::splat_f32(0.0f);
  float32x4_t c30 = simd::neon::splat_f32(0.0f), c31 = simd::neon::splat_f32(0.0f);
  for ( usize p = 0; p < k; ++p ) {
    const float32x4_t b0 = simd::neon::load_f32(Bp + p * 8 + 0);
    const float32x4_t b1 = simd::neon::load_f32(Bp + p * 8 + 4);
    const float32x4_t a0 = simd::neon::splat_f32(Ap[p * 4 + 0]);
    const float32x4_t a1 = simd::neon::splat_f32(Ap[p * 4 + 1]);
    const float32x4_t a2 = simd::neon::splat_f32(Ap[p * 4 + 2]);
    const float32x4_t a3 = simd::neon::splat_f32(Ap[p * 4 + 3]);
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
    c00 = simd::neon::fma_f32(c00, a0, b0);
    c01 = simd::neon::fma_f32(c01, a0, b1);
    c10 = simd::neon::fma_f32(c10, a1, b0);
    c11 = simd::neon::fma_f32(c11, a1, b1);
    c20 = simd::neon::fma_f32(c20, a2, b0);
    c21 = simd::neon::fma_f32(c21, a2, b1);
    c30 = simd::neon::fma_f32(c30, a3, b0);
    c31 = simd::neon::fma_f32(c31, a3, b1);
#else
    c00 = simd::neon::mla(c00, a0, b0);
    c01 = simd::neon::mla(c01, a0, b1);
    c10 = simd::neon::mla(c10, a1, b0);
    c11 = simd::neon::mla(c11, a1, b1);
    c20 = simd::neon::mla(c20, a2, b0);
    c21 = simd::neon::mla(c21, a2, b1);
    c30 = simd::neon::mla(c30, a3, b0);
    c31 = simd::neon::mla(c31, a3, b1);
#endif
  }
  const float32x4_t valpha = simd::neon::splat_f32(alpha);
  float *r0 = C + 0 * rs_C;
  float *r1 = C + 1 * rs_C;
  float *r2 = C + 2 * rs_C;
  float *r3 = C + 3 * rs_C;
  if ( beta == 0.0f ) {
    simd::neon::store_f32(r0 + 0, simd::neon::mul(valpha, c00));
    simd::neon::store_f32(r0 + 4, simd::neon::mul(valpha, c01));
    simd::neon::store_f32(r1 + 0, simd::neon::mul(valpha, c10));
    simd::neon::store_f32(r1 + 4, simd::neon::mul(valpha, c11));
    simd::neon::store_f32(r2 + 0, simd::neon::mul(valpha, c20));
    simd::neon::store_f32(r2 + 4, simd::neon::mul(valpha, c21));
    simd::neon::store_f32(r3 + 0, simd::neon::mul(valpha, c30));
    simd::neon::store_f32(r3 + 4, simd::neon::mul(valpha, c31));
  } else {
    const float32x4_t vbeta = simd::neon::splat_f32(beta);
#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
    simd::neon::store_f32(r0 + 0, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r0 + 0)), valpha, c00));
    simd::neon::store_f32(r0 + 4, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r0 + 4)), valpha, c01));
    simd::neon::store_f32(r1 + 0, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r1 + 0)), valpha, c10));
    simd::neon::store_f32(r1 + 4, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r1 + 4)), valpha, c11));
    simd::neon::store_f32(r2 + 0, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r2 + 0)), valpha, c20));
    simd::neon::store_f32(r2 + 4, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r2 + 4)), valpha, c21));
    simd::neon::store_f32(r3 + 0, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r3 + 0)), valpha, c30));
    simd::neon::store_f32(r3 + 4, simd::neon::fma_f32(simd::neon::mul(vbeta, simd::neon::load_f32(r3 + 4)), valpha, c31));
#else
    simd::neon::store_f32(r0 + 0, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r0 + 0)), valpha, c00));
    simd::neon::store_f32(r0 + 4, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r0 + 4)), valpha, c01));
    simd::neon::store_f32(r1 + 0, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r1 + 0)), valpha, c10));
    simd::neon::store_f32(r1 + 4, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r1 + 4)), valpha, c11));
    simd::neon::store_f32(r2 + 0, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r2 + 0)), valpha, c20));
    simd::neon::store_f32(r2 + 4, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r2 + 4)), valpha, c21));
    simd::neon::store_f32(r3 + 0, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r3 + 0)), valpha, c30));
    simd::neon::store_f32(r3 + 4, simd::neon::mla(simd::neon::mul(vbeta, simd::neon::load_f32(r3 + 4)), valpha, c31));
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
template <> inline constexpr usize gemm_mr_v<f64> = 6;
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

inline constexpr usize __pack_a_max_doubles = default_mc * default_kc;     // 72*256 = 18 432 doubles = 144 KiB
inline constexpr usize __pack_b_max_doubles = default_kc * default_nc;     // 256*1024 = 262 144 doubles = 2 MiB

template <typename T>
[[gnu::always_inline]] inline T *
__tls_pack_a() noexcept
{
  alignas(64) static thread_local double buf[__pack_a_max_doubles];
  return reinterpret_cast<T *>(buf);
}

template <typename T>
[[gnu::always_inline]] inline T *
__tls_pack_b() noexcept
{
  alignas(64) static thread_local double buf[__pack_b_max_doubles];
  return reinterpret_cast<T *>(buf);
}

template <typename T>
[[gnu::flatten]] inline void
gemm_blocked_aligned(usize m, usize n, usize k, T alpha, const T *A, ssize_t a_rs, ssize_t a_cs, const T *B, ssize_t b_rs, ssize_t b_cs,
                     T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  constexpr usize MR = gemm_mr_v<T>;
  constexpr usize NR = gemm_nr_v<T>;
  const usize Mc = (m < default_mc) ? m : default_mc;
  const usize Kc = (k < default_kc) ? k : default_kc;
  const usize Nc = (n < default_nc) ? n : default_nc;

  T *Ap = __tls_pack_a<T>();
  T *Bp = __tls_pack_b<T>();

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
#if defined(__AVX2__) && defined(__FMA__)
              if constexpr ( MR == 6 && NR == 8 && sizeof(T) == 8 && ieee754_floating<T> ) {
                micro_kernel_6x8_avx2_f64_asm_aligned(reinterpret_cast<const double *>(Ap_tile), reinterpret_cast<const double *>(Bp_tile),
                                                      kc, double(alpha), double(beta_eff), reinterpret_cast<double *>(C_tile), rs_C, cs_C);
              } else if constexpr ( MR == 4 && NR == 8 && sizeof(T) == 8 && ieee754_floating<T> ) {
                micro_kernel_4x8_avx2_f64(reinterpret_cast<const double *>(Ap_tile), reinterpret_cast<const double *>(Bp_tile), kc,
                                          double(alpha), double(beta_eff), reinterpret_cast<double *>(C_tile), rs_C, cs_C);
              } else if constexpr ( MR == 8 && NR == 8 && sizeof(T) == 4 && ieee754_floating<T> ) {
                micro_kernel_8x8_avx2_f32(reinterpret_cast<const float *>(Ap_tile), reinterpret_cast<const float *>(Bp_tile), kc,
                                          float(alpha), float(beta_eff), reinterpret_cast<float *>(C_tile), rs_C, cs_C);
              } else
#endif
              {
                micro_kernel<MR, NR, T>(Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
              }
            } else {
              micro_kernel_partial<MR, NR, T>(mr_eff, nr_eff, Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
            }
          }
        }
      }
    }
  }
}

template <typename T>
[[gnu::flatten]] inline void
gemm_blocked_aligned_exp_a(usize m, usize n, usize k, T alpha, const T *A, ssize_t a_rs, ssize_t a_cs, const T *B, ssize_t b_rs,
                           ssize_t b_cs, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  constexpr usize MR = gemm_mr_v<T>;
  constexpr usize NR = gemm_nr_v<T>;
  const usize Mc = (m < default_mc) ? m : default_mc;
  const usize Kc = (k < default_kc) ? k : default_kc;
  const usize Nc = (n < default_nc) ? n : default_nc;

  T *Ap = __tls_pack_a<T>();
  T *Bp = __tls_pack_b<T>();

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
#if defined(__AVX2__) && defined(__FMA__)
              if constexpr ( MR == 6 && NR == 8 && sizeof(T) == 8 && ieee754_floating<T> ) {
                micro_kernel_6x8_avx2_f64_asm_relax(reinterpret_cast<const double *>(Ap_tile), reinterpret_cast<const double *>(Bp_tile),
                                                    kc, double(alpha), double(beta_eff), reinterpret_cast<double *>(C_tile), rs_C, cs_C);
              } else
#endif
              {
                micro_kernel<MR, NR, T>(Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
              }
            } else {
              micro_kernel_partial<MR, NR, T>(mr_eff, nr_eff, Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
            }
          }
        }
      }
    }
  }
}

template <typename T>
[[gnu::flatten]] inline void
gemm_blocked_aligned_exp_b(usize m, usize n, usize k, T alpha, const T *A, ssize_t a_rs, ssize_t a_cs, const T *B, ssize_t b_rs,
                           ssize_t b_cs, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
#if defined(__AVX2__) && defined(__FMA__)
  if constexpr ( sizeof(T) == 8 && ieee754_floating<T> ) {
    constexpr usize MR = 4;
    constexpr usize NR = 12;
    // Tighter NC because each B-pack row is now 12 doubles, not 8/256
    constexpr usize local_nc = 252;     // multiple of 12, <= default_nc
    const usize Mc = (m < default_mc) ? m : default_mc;
    const usize Kc = (k < default_kc) ? k : default_kc;
    const usize Nc = (n < local_nc) ? n : local_nc;

    T *Ap = __tls_pack_a<T>();
    T *Bp = __tls_pack_b<T>();

    for ( usize jc = 0; jc < n; jc += Nc ) {
      const usize nc = ((n - jc) < Nc) ? (n - jc) : Nc;
      for ( usize pc = 0; pc < k; pc += Kc ) {
        const usize kc = ((k - pc) < Kc) ? (k - pc) : Kc;
        const T beta_eff = (pc == 0) ? beta : T(1);

        const T *B_block = B + ssize_t(pc) * b_rs + ssize_t(jc) * b_cs;
        pack_b_panel_12_f64(reinterpret_cast<const double *>(B_block), b_rs, b_cs, kc, nc, reinterpret_cast<double *>(Bp));

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
                micro_kernel_4x12_avx2_f64_asm_unr4_sp(reinterpret_cast<const double *>(Ap_tile), reinterpret_cast<const double *>(Bp_tile),
                                                       kc, double(alpha), double(beta_eff), reinterpret_cast<double *>(C_tile), rs_C, cs_C);
              } else {
                micro_kernel_partial<MR, NR, T>(mr_eff, nr_eff, Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
              }
            }
          }
        }
      }
    }
    return;
  }
#endif
  gemm_blocked_aligned<T>(m, n, k, alpha, A, a_rs, a_cs, B, b_rs, b_cs, beta, C, rs_C, cs_C);
}

template <typename T>
[[gnu::flatten]] inline void
gemm_blocked_aligned_exp_c(usize m, usize n, usize k, T alpha, const T *A, ssize_t a_rs, ssize_t a_cs, const T *B, ssize_t b_rs,
                           ssize_t b_cs, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  constexpr usize MR = gemm_mr_v<T>;
  constexpr usize NR = gemm_nr_v<T>;
  const usize Mc = (m < default_mc) ? m : default_mc;
  const usize Kc = (k < default_kc) ? k : default_kc;
  const usize Nc = (n < default_nc) ? n : default_nc;

  T *Ap = __tls_pack_a<T>();
  T *Bp = __tls_pack_b<T>();

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
#if defined(__AVX2__) && defined(__FMA__)
              if constexpr ( MR == 6 && NR == 8 && sizeof(T) == 8 && ieee754_floating<T> ) {
                micro_kernel_6x8_avx2_f64_asm_roll(reinterpret_cast<const double *>(Ap_tile), reinterpret_cast<const double *>(Bp_tile), kc,
                                                   double(alpha), double(beta_eff), reinterpret_cast<double *>(C_tile), rs_C, cs_C);
              } else
#endif
              {
                micro_kernel<MR, NR, T>(Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
              }
            } else {
              micro_kernel_partial<MR, NR, T>(mr_eff, nr_eff, Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
            }
          }
        }
      }
    }
  }
}

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
              // NOTE: don't match against type identity; double and f64 aren't the same type
#if defined(__AVX2__) && defined(__FMA__)
              if constexpr ( MR == 6 && NR == 8 && sizeof(T) == 8 && ieee754_floating<T> ) {
                micro_kernel_6x8_avx2_f64_asm_unr4_sp(reinterpret_cast<const double *>(Ap_tile), reinterpret_cast<const double *>(Bp_tile),
                                                      kc, double(alpha), double(beta_eff), reinterpret_cast<double *>(C_tile), rs_C, cs_C);
              } else if constexpr ( MR == 4 && NR == 8 && sizeof(T) == 8 && ieee754_floating<T> ) {
                micro_kernel_4x8_avx2_f64(reinterpret_cast<const double *>(Ap_tile), reinterpret_cast<const double *>(Bp_tile), kc,
                                          double(alpha), double(beta_eff), reinterpret_cast<double *>(C_tile), rs_C, cs_C);
              } else if constexpr ( MR == 8 && NR == 8 && sizeof(T) == 4 && ieee754_floating<T> ) {
                micro_kernel_8x8_avx2_f32(reinterpret_cast<const float *>(Ap_tile), reinterpret_cast<const float *>(Bp_tile), kc,
                                          float(alpha), float(beta_eff), reinterpret_cast<float *>(C_tile), rs_C, cs_C);
              } else
#endif
              {
                micro_kernel<MR, NR, T>(Ap_tile, Bp_tile, kc, alpha, beta_eff, C_tile, rs_C, cs_C);
              }
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
  return (m >= 8) && (n >= 8) && (k >= 8) && ((m * n * k) >= (32u * 32u * 32u));
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
