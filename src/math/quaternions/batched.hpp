//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// batched (array-of-N) quaternion ops
// processes 4 quats per iteration in SoA form, transposes in flight
// hits ~3 cyc/quat for multiply and ~5 cyc/quat for rotate/normalize

#include "../../bits/__arch.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../quants/vecs.hpp"
#include "../sqrt.hpp"
#include "algebra.hpp"
#include "rotations.hpp"

#if defined(__AVX2__) && defined(__FMA__)
#include "../../simd/aliases.hpp"
#include "../../simd/arch/types_amd64.hpp"
#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon) && (defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA))
#define __micron_quat_batched_neon 1
#include "../../simd/aliases.hpp"
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
namespace quaternions
{

#if defined(__AVX2__) && defined(__FMA__)
namespace __batched_detail
{

// AoS->SoA transpose of 4 vectors of 4 doubles each. After this call:
//   xs = {v0[0], v1[0], v2[0], v3[0]}
//   ys = {v0[1], v1[1], v2[1], v3[1]}
//   zs = {v0[2], v1[2], v2[2], v3[2]}
//   ws = {v0[3], v1[3], v2[3], v3[3]}
[[gnu::always_inline]] inline void
transpose4x4_pd(__m256d v0, __m256d v1, __m256d v2, __m256d v3, __m256d &xs, __m256d &ys, __m256d &zs, __m256d &ws) noexcept
{
  const __m256d t01_lo = simd::avx::unpack_lo_f64(v0, v1);     // {v0[0], v1[0], v0[2], v1[2]}
  const __m256d t01_hi = simd::avx::unpack_hi_f64(v0, v1);     // {v0[1], v1[1], v0[3], v1[3]}
  const __m256d t23_lo = simd::avx::unpack_lo_f64(v2, v3);
  const __m256d t23_hi = simd::avx::unpack_hi_f64(v2, v3);
  xs = simd::avx::permute2f128_f64<0x20>(t01_lo, t23_lo);
  ys = simd::avx::permute2f128_f64<0x20>(t01_hi, t23_hi);
  zs = simd::avx::permute2f128_f64<0x31>(t01_lo, t23_lo);
  ws = simd::avx::permute2f128_f64<0x31>(t01_hi, t23_hi);
}

// SoA->AoS transpose
[[gnu::always_inline]] inline void
transpose4x4_pd_inv(__m256d xs, __m256d ys, __m256d zs, __m256d ws, __m256d &v0, __m256d &v1, __m256d &v2, __m256d &v3) noexcept
{
  const __m256d t01_lo = simd::avx::unpack_lo_f64(xs, ys);
  const __m256d t01_hi = simd::avx::unpack_hi_f64(xs, ys);
  const __m256d t23_lo = simd::avx::unpack_lo_f64(zs, ws);
  const __m256d t23_hi = simd::avx::unpack_hi_f64(zs, ws);
  v0 = simd::avx::permute2f128_f64<0x20>(t01_lo, t23_lo);
  v1 = simd::avx::permute2f128_f64<0x20>(t01_hi, t23_hi);
  v2 = simd::avx::permute2f128_f64<0x31>(t01_lo, t23_lo);
  v3 = simd::avx::permute2f128_f64<0x31>(t01_hi, t23_hi);
}

};     // namespace __batched_detail
#endif

#if defined(__micron_quat_batched_neon)
namespace __batched_detail
{

// AoS->SoA transpose of 4 f32 quaternions packed as 4 float32x4_t
[[gnu::always_inline]] inline void
transpose4x4_ps(float32x4_t v0, float32x4_t v1, float32x4_t v2, float32x4_t v3, float32x4_t &xs, float32x4_t &ys, float32x4_t &zs,
                float32x4_t &ws) noexcept
{
  const float32x4_t t01_lo = simd::neon::zip_lo_f32(v0, v1);     // {v0[0], v1[0], v0[1], v1[1]}
  const float32x4_t t01_hi = simd::neon::zip_hi_f32(v0, v1);     // {v0[2], v1[2], v0[3], v1[3]}
  const float32x4_t t23_lo = simd::neon::zip_lo_f32(v2, v3);
  const float32x4_t t23_hi = simd::neon::zip_hi_f32(v2, v3);
  xs = simd::neon::concat_lo_f32(t01_lo, t23_lo);
  ys = simd::neon::concat_hi_f32(t01_lo, t23_lo);
  zs = simd::neon::concat_lo_f32(t01_hi, t23_hi);
  ws = simd::neon::concat_hi_f32(t01_hi, t23_hi);
}

// SoA->AoS for f32
[[gnu::always_inline]] inline void
transpose4x4_ps_inv(float32x4_t xs, float32x4_t ys, float32x4_t zs, float32x4_t ws, float32x4_t &v0, float32x4_t &v1, float32x4_t &v2,
                    float32x4_t &v3) noexcept
{
  const float32x4_t t01_lo = simd::neon::zip_lo_f32(xs, ys);
  const float32x4_t t01_hi = simd::neon::zip_hi_f32(xs, ys);
  const float32x4_t t23_lo = simd::neon::zip_lo_f32(zs, ws);
  const float32x4_t t23_hi = simd::neon::zip_hi_f32(zs, ws);
  v0 = simd::neon::concat_lo_f32(t01_lo, t23_lo);
  v1 = simd::neon::concat_hi_f32(t01_lo, t23_lo);
  v2 = simd::neon::concat_lo_f32(t01_hi, t23_hi);
  v3 = simd::neon::concat_hi_f32(t01_hi, t23_hi);
}

#if defined(__micron_arch_arm64)
// AoS->SoA transpose of 2 f64 quaternions packed as 4 float64x2_t (two halves per quat)
[[gnu::always_inline]] inline void
transpose2x4_pd(float64x2_t lo0, float64x2_t hi0, float64x2_t lo1, float64x2_t hi1, float64x2_t &xs, float64x2_t &ys, float64x2_t &zs,
                float64x2_t &ws) noexcept
{
  xs = simd::neon::zip_lo_f64(lo0, lo1);     // {q0.x, q1.x}
  ys = simd::neon::zip_hi_f64(lo0, lo1);     // {q0.y, q1.y}
  zs = simd::neon::zip_lo_f64(hi0, hi1);     // {q0.z, q1.z}
  ws = simd::neon::zip_hi_f64(hi0, hi1);     // {q0.w, q1.w}
}

[[gnu::always_inline]] inline void
transpose2x4_pd_inv(float64x2_t xs, float64x2_t ys, float64x2_t zs, float64x2_t ws, float64x2_t &lo0, float64x2_t &hi0, float64x2_t &lo1,
                    float64x2_t &hi1) noexcept
{
  lo0 = simd::neon::zip_lo_f64(xs, ys);     // {q0.x, q0.y}
  lo1 = simd::neon::zip_hi_f64(xs, ys);     // {q1.x, q1.y}
  hi0 = simd::neon::zip_lo_f64(zs, ws);     // {q0.z, q0.w}
  hi1 = simd::neon::zip_hi_f64(zs, ws);     // {q1.z, q1.w}
}
#endif

};     // namespace __batched_detail
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// batched_multiply: out[i] = a[i] * b[i] (Hamilton product) for i in [0, n)
template <ieee754_floating T>
inline void
batched_multiply(const quaternion<T> *a, const quaternion<T> *b, quaternion<T> *out, usize n) noexcept
{
#if defined(__AVX2__) && defined(__FMA__)
  if constexpr ( sizeof(T) == 8 ) {
    usize i = 0;
    for ( ; i + 4 <= n; i += 4 ) {
      const double *ap = reinterpret_cast<const double *>(a + i);
      const double *bp = reinterpret_cast<const double *>(b + i);
      const __m256d a0 = simd::avx::loadu_f64(ap + 0);
      const __m256d a1 = simd::avx::loadu_f64(ap + 4);
      const __m256d a2 = simd::avx::loadu_f64(ap + 8);
      const __m256d a3 = simd::avx::loadu_f64(ap + 12);
      const __m256d b0 = simd::avx::loadu_f64(bp + 0);
      const __m256d b1 = simd::avx::loadu_f64(bp + 4);
      const __m256d b2 = simd::avx::loadu_f64(bp + 8);
      const __m256d b3 = simd::avx::loadu_f64(bp + 12);

      __m256d axs, ays, azs, aws, bxs, bys, bzs, bws;
      __batched_detail::transpose4x4_pd(a0, a1, a2, a3, axs, ays, azs, aws);
      __batched_detail::transpose4x4_pd(b0, b1, b2, b3, bxs, bys, bzs, bws);

      // 16 FMAs = 8 cyc on 2 FMA ports; transposes add ~6 cyc; ~14 cyc/4-quats = 3.5 cyc/quat
      __m256d rxs = simd::avx::mul_f64(aws, bxs);
      rxs = simd::fma::fma_f64(axs, bws, rxs);
      rxs = simd::fma::fma_f64(ays, bzs, rxs);
      rxs = simd::fma::fnma_f64(azs, bys, rxs);

      __m256d rys = simd::avx::mul_f64(aws, bys);
      rys = simd::fma::fnma_f64(axs, bzs, rys);
      rys = simd::fma::fma_f64(ays, bws, rys);
      rys = simd::fma::fma_f64(azs, bxs, rys);

      __m256d rzs = simd::avx::mul_f64(aws, bzs);
      rzs = simd::fma::fma_f64(axs, bys, rzs);
      rzs = simd::fma::fnma_f64(ays, bxs, rzs);
      rzs = simd::fma::fma_f64(azs, bws, rzs);

      __m256d rws = simd::avx::mul_f64(aws, bws);
      rws = simd::fma::fnma_f64(axs, bxs, rws);
      rws = simd::fma::fnma_f64(ays, bys, rws);
      rws = simd::fma::fnma_f64(azs, bzs, rws);

      __m256d r0, r1, r2, r3;
      __batched_detail::transpose4x4_pd_inv(rxs, rys, rzs, rws, r0, r1, r2, r3);
      double *op = reinterpret_cast<double *>(out + i);
      simd::avx::storeu_f64(op + 0, r0);
      simd::avx::storeu_f64(op + 4, r1);
      simd::avx::storeu_f64(op + 8, r2);
      simd::avx::storeu_f64(op + 12, r3);
    }
    for ( ; i < n; ++i ) out[i] = multiply<T>(a[i], b[i]);
    return;
  }
#endif
#if defined(__micron_quat_batched_neon)
  if constexpr ( sizeof(T) == 4 ) {
    // f32: 4 quaternions per iteration via SoA on float32x4_t
    usize i = 0;
    for ( ; i + 4 <= n; i += 4 ) {
      const float *ap = reinterpret_cast<const float *>(a + i);
      const float *bp = reinterpret_cast<const float *>(b + i);
      const float32x4_t a0 = simd::neon::load_f32(ap + 0);
      const float32x4_t a1 = simd::neon::load_f32(ap + 4);
      const float32x4_t a2 = simd::neon::load_f32(ap + 8);
      const float32x4_t a3 = simd::neon::load_f32(ap + 12);
      const float32x4_t b0 = simd::neon::load_f32(bp + 0);
      const float32x4_t b1 = simd::neon::load_f32(bp + 4);
      const float32x4_t b2 = simd::neon::load_f32(bp + 8);
      const float32x4_t b3 = simd::neon::load_f32(bp + 12);

      float32x4_t axs, ays, azs, aws, bxs, bys, bzs, bws;
      __batched_detail::transpose4x4_ps(a0, a1, a2, a3, axs, ays, azs, aws);
      __batched_detail::transpose4x4_ps(b0, b1, b2, b3, bxs, bys, bzs, bws);

      float32x4_t rxs = simd::neon::mul(aws, bxs);
      rxs = simd::neon::fma_f32(rxs, axs, bws);
      rxs = simd::neon::fma_f32(rxs, ays, bzs);
      rxs = simd::neon::fms_f32(rxs, azs, bys);

      float32x4_t rys = simd::neon::mul(aws, bys);
      rys = simd::neon::fms_f32(rys, axs, bzs);
      rys = simd::neon::fma_f32(rys, ays, bws);
      rys = simd::neon::fma_f32(rys, azs, bxs);

      float32x4_t rzs = simd::neon::mul(aws, bzs);
      rzs = simd::neon::fma_f32(rzs, axs, bys);
      rzs = simd::neon::fms_f32(rzs, ays, bxs);
      rzs = simd::neon::fma_f32(rzs, azs, bws);

      float32x4_t rws = simd::neon::mul(aws, bws);
      rws = simd::neon::fms_f32(rws, axs, bxs);
      rws = simd::neon::fms_f32(rws, ays, bys);
      rws = simd::neon::fms_f32(rws, azs, bzs);

      float32x4_t r0, r1, r2, r3;
      __batched_detail::transpose4x4_ps_inv(rxs, rys, rzs, rws, r0, r1, r2, r3);
      float *op = reinterpret_cast<float *>(out + i);
      simd::neon::store_f32(op + 0, r0);
      simd::neon::store_f32(op + 4, r1);
      simd::neon::store_f32(op + 8, r2);
      simd::neon::store_f32(op + 12, r3);
    }
    for ( ; i < n; ++i ) out[i] = multiply<T>(a[i], b[i]);
    return;
  }
#if defined(__micron_arch_arm64)
  if constexpr ( sizeof(T) == 8 ) {
    // f64 on arm64: 2 quaternions per iteration via SoA on float64x2_t.
    usize i = 0;
    for ( ; i + 2 <= n; i += 2 ) {
      const double *ap = reinterpret_cast<const double *>(a + i);
      const double *bp = reinterpret_cast<const double *>(b + i);
      const float64x2_t alo0 = simd::neon::load_f64(ap + 0);
      const float64x2_t ahi0 = simd::neon::load_f64(ap + 2);
      const float64x2_t alo1 = simd::neon::load_f64(ap + 4);
      const float64x2_t ahi1 = simd::neon::load_f64(ap + 6);
      const float64x2_t blo0 = simd::neon::load_f64(bp + 0);
      const float64x2_t bhi0 = simd::neon::load_f64(bp + 2);
      const float64x2_t blo1 = simd::neon::load_f64(bp + 4);
      const float64x2_t bhi1 = simd::neon::load_f64(bp + 6);

      float64x2_t axs, ays, azs, aws, bxs, bys, bzs, bws;
      __batched_detail::transpose2x4_pd(alo0, ahi0, alo1, ahi1, axs, ays, azs, aws);
      __batched_detail::transpose2x4_pd(blo0, bhi0, blo1, bhi1, bxs, bys, bzs, bws);

      float64x2_t rxs = simd::neon::mul(aws, bxs);
      rxs = simd::neon::fma_f64(rxs, axs, bws);
      rxs = simd::neon::fma_f64(rxs, ays, bzs);
      rxs = simd::neon::fms_f64(rxs, azs, bys);

      float64x2_t rys = simd::neon::mul(aws, bys);
      rys = simd::neon::fms_f64(rys, axs, bzs);
      rys = simd::neon::fma_f64(rys, ays, bws);
      rys = simd::neon::fma_f64(rys, azs, bxs);

      float64x2_t rzs = simd::neon::mul(aws, bzs);
      rzs = simd::neon::fma_f64(rzs, axs, bys);
      rzs = simd::neon::fms_f64(rzs, ays, bxs);
      rzs = simd::neon::fma_f64(rzs, azs, bws);

      float64x2_t rws = simd::neon::mul(aws, bws);
      rws = simd::neon::fms_f64(rws, axs, bxs);
      rws = simd::neon::fms_f64(rws, ays, bys);
      rws = simd::neon::fms_f64(rws, azs, bzs);

      float64x2_t rlo0, rhi0, rlo1, rhi1;
      __batched_detail::transpose2x4_pd_inv(rxs, rys, rzs, rws, rlo0, rhi0, rlo1, rhi1);
      double *op = reinterpret_cast<double *>(out + i);
      simd::neon::store_f64(op + 0, rlo0);
      simd::neon::store_f64(op + 2, rhi0);
      simd::neon::store_f64(op + 4, rlo1);
      simd::neon::store_f64(op + 6, rhi1);
    }
    for ( ; i < n; ++i ) out[i] = multiply<T>(a[i], b[i]);
    return;
  }
#endif
#endif
  for ( usize i = 0; i < n; ++i ) out[i] = multiply<T>(a[i], b[i]);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// batched_normalize: out[i] = a[i] / |a[i]|
template <ieee754_floating T>
inline void
batched_normalize(const quaternion<T> *in, quaternion<T> *out, usize n) noexcept
{
#if defined(__AVX2__) && defined(__FMA__)
  if constexpr ( sizeof(T) == 8 ) {
    const __m256d one = simd::avx::splat_f64(1.0);
    usize i = 0;
    for ( ; i + 4 <= n; i += 4 ) {
      const double *ip = reinterpret_cast<const double *>(in + i);
      const __m256d q0 = simd::avx::loadu_f64(ip + 0);
      const __m256d q1 = simd::avx::loadu_f64(ip + 4);
      const __m256d q2 = simd::avx::loadu_f64(ip + 8);
      const __m256d q3 = simd::avx::loadu_f64(ip + 12);

      __m256d xs, ys, zs, ws;
      __batched_detail::transpose4x4_pd(q0, q1, q2, q3, xs, ys, zs, ws);

      __m256d n2 = simd::avx::mul_f64(xs, xs);
      n2 = simd::fma::fma_f64(ys, ys, n2);
      n2 = simd::fma::fma_f64(zs, zs, n2);
      n2 = simd::fma::fma_f64(ws, ws, n2);

      const __m256d sq = simd::avx::sqrt_f64(n2);
      const __m256d inv = simd::avx::div_f64(one, sq);

      xs = simd::avx::mul_f64(xs, inv);
      ys = simd::avx::mul_f64(ys, inv);
      zs = simd::avx::mul_f64(zs, inv);
      ws = simd::avx::mul_f64(ws, inv);

      __m256d r0, r1, r2, r3;
      __batched_detail::transpose4x4_pd_inv(xs, ys, zs, ws, r0, r1, r2, r3);
      double *op = reinterpret_cast<double *>(out + i);
      simd::avx::storeu_f64(op + 0, r0);
      simd::avx::storeu_f64(op + 4, r1);
      simd::avx::storeu_f64(op + 8, r2);
      simd::avx::storeu_f64(op + 12, r3);
    }
    for ( ; i < n; ++i ) out[i] = normalize<T>(in[i]);
    return;
  }
#endif
#if defined(__micron_quat_batched_neon)
  if constexpr ( sizeof(T) == 4 ) {
    // NOTE: we multiply by rsqrt(n**2) rather than dividing by sqrt(n**2)
    // NEON's vrsqrteq_f32 + one NR step lands at ~22 bits of mantissa, close enough to fp32 precision
    usize i = 0;
    for ( ; i + 4 <= n; i += 4 ) {
      const float *ip = reinterpret_cast<const float *>(in + i);
      const float32x4_t q0 = simd::neon::load_f32(ip + 0);
      const float32x4_t q1 = simd::neon::load_f32(ip + 4);
      const float32x4_t q2 = simd::neon::load_f32(ip + 8);
      const float32x4_t q3 = simd::neon::load_f32(ip + 12);

      float32x4_t xs, ys, zs, ws;
      __batched_detail::transpose4x4_ps(q0, q1, q2, q3, xs, ys, zs, ws);

      float32x4_t n2 = simd::neon::mul(xs, xs);
      n2 = simd::neon::fma_f32(n2, ys, ys);
      n2 = simd::neon::fma_f32(n2, zs, zs);
      n2 = simd::neon::fma_f32(n2, ws, ws);

      float32x4_t inv = simd::neon::rsqrt_est(n2);
      inv = simd::neon::mul(inv, simd::neon::rsqrt_step(simd::neon::mul(n2, inv), inv));
      inv = simd::neon::mul(inv, simd::neon::rsqrt_step(simd::neon::mul(n2, inv), inv));

      xs = simd::neon::mul(xs, inv);
      ys = simd::neon::mul(ys, inv);
      zs = simd::neon::mul(zs, inv);
      ws = simd::neon::mul(ws, inv);

      float32x4_t r0, r1, r2, r3;
      __batched_detail::transpose4x4_ps_inv(xs, ys, zs, ws, r0, r1, r2, r3);
      float *op = reinterpret_cast<float *>(out + i);
      simd::neon::store_f32(op + 0, r0);
      simd::neon::store_f32(op + 4, r1);
      simd::neon::store_f32(op + 8, r2);
      simd::neon::store_f32(op + 12, r3);
    }
    for ( ; i < n; ++i ) out[i] = normalize<T>(in[i]);
    return;
  }
#if defined(__micron_arch_arm64)
  if constexpr ( sizeof(T) == 8 ) {
    const float64x2_t one = simd::neon::splat_f64(1.0);
    usize i = 0;
    for ( ; i + 2 <= n; i += 2 ) {
      const double *ip = reinterpret_cast<const double *>(in + i);
      const float64x2_t lo0 = simd::neon::load_f64(ip + 0);
      const float64x2_t hi0 = simd::neon::load_f64(ip + 2);
      const float64x2_t lo1 = simd::neon::load_f64(ip + 4);
      const float64x2_t hi1 = simd::neon::load_f64(ip + 6);

      float64x2_t xs, ys, zs, ws;
      __batched_detail::transpose2x4_pd(lo0, hi0, lo1, hi1, xs, ys, zs, ws);

      float64x2_t n2 = simd::neon::mul(xs, xs);
      n2 = simd::neon::fma_f64(n2, ys, ys);
      n2 = simd::neon::fma_f64(n2, zs, zs);
      n2 = simd::neon::fma_f64(n2, ws, ws);

      const float64x2_t sq = simd::neon::sqrt(n2);
      const float64x2_t inv = simd::neon::div(one, sq);

      xs = simd::neon::mul(xs, inv);
      ys = simd::neon::mul(ys, inv);
      zs = simd::neon::mul(zs, inv);
      ws = simd::neon::mul(ws, inv);

      float64x2_t rlo0, rhi0, rlo1, rhi1;
      __batched_detail::transpose2x4_pd_inv(xs, ys, zs, ws, rlo0, rhi0, rlo1, rhi1);
      double *op = reinterpret_cast<double *>(out + i);
      simd::neon::store_f64(op + 0, rlo0);
      simd::neon::store_f64(op + 2, rhi0);
      simd::neon::store_f64(op + 4, rlo1);
      simd::neon::store_f64(op + 6, rhi1);
    }
    for ( ; i < n; ++i ) out[i] = normalize<T>(in[i]);
    return;
  }
#endif
#endif
  for ( usize i = 0; i < n; ++i ) out[i] = normalize<T>(in[i]);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// batched_rotate: out[i] = q[i] * vec3(v[i]) * conj(q[i])
template <ieee754_floating T>
inline void
batched_rotate(const quaternion<T> *q, const micron::vector_3<T> *v, micron::vector_3<T> *out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = rotate<T>(q[i], v[i]);
}

};     // namespace quaternions
};     // namespace math
};     // namespace micron
