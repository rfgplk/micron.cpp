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
inline constexpr usize nr_f32 = 4;
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
[[gnu::flatten]] inline void
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
[[gnu::flatten]] inline void
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
[[gnu::flatten]] inline void
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
[[gnu::flatten]] inline void
micro_kernel(const T *Ap, const T *Bp, usize k, T alpha, T beta, T *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  micro_kernel_scalar<MR, NR, T>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
}

#if defined(__AVX2__) && defined(__FMA__)

template <>
[[gnu::flatten]] inline void
micro_kernel<4, 4, double>(const double *Ap, const double *Bp, usize k, double alpha, double beta, double *C, ssize_t rs_C,
                           ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<4, 4, double>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  __m256d c0 = _mm256_setzero_pd();
  __m256d c1 = _mm256_setzero_pd();
  __m256d c2 = _mm256_setzero_pd();
  __m256d c3 = _mm256_setzero_pd();
  for ( usize p = 0; p < k; ++p ) {
    const __m256d br = _mm256_loadu_pd(Bp + p * 4);
    const __m256d a0 = _mm256_set1_pd(Ap[p * 4 + 0]);
    const __m256d a1 = _mm256_set1_pd(Ap[p * 4 + 1]);
    const __m256d a2 = _mm256_set1_pd(Ap[p * 4 + 2]);
    const __m256d a3 = _mm256_set1_pd(Ap[p * 4 + 3]);
    c0 = _mm256_fmadd_pd(a0, br, c0);
    c1 = _mm256_fmadd_pd(a1, br, c1);
    c2 = _mm256_fmadd_pd(a2, br, c2);
    c3 = _mm256_fmadd_pd(a3, br, c3);
  }
  const __m256d valpha = _mm256_set1_pd(alpha);
  double *r0 = C + ssize_t(0) * rs_C;
  double *r1 = C + ssize_t(1) * rs_C;
  double *r2 = C + ssize_t(2) * rs_C;
  double *r3 = C + ssize_t(3) * rs_C;
  if ( beta == 0.0 ) {
    _mm256_storeu_pd(r0, _mm256_mul_pd(valpha, c0));
    _mm256_storeu_pd(r1, _mm256_mul_pd(valpha, c1));
    _mm256_storeu_pd(r2, _mm256_mul_pd(valpha, c2));
    _mm256_storeu_pd(r3, _mm256_mul_pd(valpha, c3));
  } else {
    const __m256d vbeta = _mm256_set1_pd(beta);
    _mm256_storeu_pd(r0, _mm256_fmadd_pd(valpha, c0, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r0))));
    _mm256_storeu_pd(r1, _mm256_fmadd_pd(valpha, c1, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r1))));
    _mm256_storeu_pd(r2, _mm256_fmadd_pd(valpha, c2, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r2))));
    _mm256_storeu_pd(r3, _mm256_fmadd_pd(valpha, c3, _mm256_mul_pd(vbeta, _mm256_loadu_pd(r3))));
  }
}

template <>
[[gnu::flatten]] inline void
micro_kernel<8, 4, float>(const float *Ap, const float *Bp, usize k, float alpha, float beta, float *C, ssize_t rs_C, ssize_t cs_C) noexcept
{
  if ( cs_C != 1 ) {
    micro_kernel_scalar<8, 4, float>(Ap, Bp, k, alpha, beta, C, rs_C, cs_C);
    return;
  }
  __m128 c0 = _mm_setzero_ps();
  __m128 c1 = _mm_setzero_ps();
  __m128 c2 = _mm_setzero_ps();
  __m128 c3 = _mm_setzero_ps();
  __m128 c4 = _mm_setzero_ps();
  __m128 c5 = _mm_setzero_ps();
  __m128 c6 = _mm_setzero_ps();
  __m128 c7 = _mm_setzero_ps();
  for ( usize p = 0; p < k; ++p ) {
    const __m128 br = _mm_loadu_ps(Bp + p * 4);
    c0 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 0]), br, c0);
    c1 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 1]), br, c1);
    c2 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 2]), br, c2);
    c3 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 3]), br, c3);
    c4 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 4]), br, c4);
    c5 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 5]), br, c5);
    c6 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 6]), br, c6);
    c7 = _mm_fmadd_ps(_mm_set1_ps(Ap[p * 8 + 7]), br, c7);
  }
  const __m128 valpha = _mm_set1_ps(alpha);
  __m128 *cs[8] = { &c0, &c1, &c2, &c3, &c4, &c5, &c6, &c7 };
  if ( beta == 0.0f ) {
    for ( usize i = 0; i < 8; ++i ) _mm_storeu_ps(C + ssize_t(i) * rs_C, _mm_mul_ps(valpha, *cs[i]));
  } else {
    const __m128 vbeta = _mm_set1_ps(beta);
    for ( usize i = 0; i < 8; ++i ) {
      float *ri = C + ssize_t(i) * rs_C;
      _mm_storeu_ps(ri, _mm_fmadd_ps(valpha, *cs[i], _mm_mul_ps(vbeta, _mm_loadu_ps(ri))));
    }
  }
}

#endif     // AVX2 + FMA

template <usize MR, usize NR, typename T>
[[gnu::flatten]] inline void
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
template <> inline constexpr usize gemm_nr_v<f64> = 4;
template <> inline constexpr usize gemm_mr_v<f32> = 8;
template <> inline constexpr usize gemm_nr_v<f32> = 4;
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

// NOTE: gemm_x4x4 avx2 microkernel keeps up with the blocked path until we exceed L2 cache
template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr bool
gemm_should_block(usize m, usize n, usize k) noexcept
{
  if constexpr ( !ieee754_floating<T> ) return false;
  return (m >= 64) && (n >= 64) && (k >= 64) && ((m * n * k) >= (256u * 256u * 256u));
}

};     // namespace pack
};     // namespace matrix
};     // namespace math
};     // namespace micron
