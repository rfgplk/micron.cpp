//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// per iteration:
//   ys = y * 2^-i          (FP multiply by 2^-i = exponent decrement = shift)
//   xs = x * 2^-i
//   sb = sign_mask(z) & -0.0
//   x -= ys ^ sb           (subtract if z>=0, add if z<0)
//   y += xs ^ sb           (add if z>=0, subtract if z<0)
//   z -= atan_i ^ sb       (subtract if z>=0, add if z<0)

#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/cordic.hpp"
#include "_dispatch.hpp"
#include "manip.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wignored-attributes"

namespace micron
{
namespace math
{
namespace mk
{

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

namespace __cordic_simd
{
inline constexpr f64 pio2_hi = 0x1.921fb54400000p+0;
inline constexpr f64 pio2_mid = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_lo = 0x1.9880000000000p-77;
inline constexpr f64 inv_pio2 = 0x1.45f306dc9c883p-1;

using mkbits::cordic_ns::ATAN_F32;
using mkbits::cordic_ns::ATAN_F64;
using mkbits::cordic_ns::K_F32;
using mkbits::cordic_ns::K_F64;
using mkbits::cordic_ns::N_FP_F32;
using mkbits::cordic_ns::N_FP_F64;
using mkbits::cordic_ns::SHIFT_POW_F32;
using mkbits::cordic_ns::SHIFT_POW_F64;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// d256 path (4 lanes f64)

[[gnu::always_inline]] inline simd::d256
reduce_pio2_d256(simd::d256 x, simd::i256 *q_out) noexcept
{
  const simd::d256 fN = _mm256_round_pd(_mm256_mul_pd(x, _mm256_set1_pd(inv_pio2)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  simd::d256 t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_hi), x);
  t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_mid), t);
  t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_lo), t);
  const simd::i256 N32 = _mm256_castsi128_si256(_mm256_cvtpd_epi32(fN));
  *q_out = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(N32));
  return t;
}

[[gnu::flatten]] inline void
sincos_kernel_d256(simd::d256 z, simd::d256 *c_out, simd::d256 *s_out) noexcept
{
  simd::d256 x = _mm256_set1_pd(K_F64);
  simd::d256 y = _mm256_setzero_pd();
  const simd::d256 neg_zero = _mm256_set1_pd(-0.0);
  for ( int i = 0; i < N_FP_F64; ++i ) {
    const simd::d256 sm = _mm256_cmp_pd(z, _mm256_setzero_pd(), _CMP_LT_OQ);
    const simd::d256 sb = _mm256_and_pd(sm, neg_zero);
    const simd::d256 vshift = _mm256_set1_pd(SHIFT_POW_F64.v[i]);
    const simd::d256 ys = _mm256_mul_pd(y, vshift);
    const simd::d256 xs = _mm256_mul_pd(x, vshift);
    const simd::d256 ai = _mm256_set1_pd(ATAN_F64.v[i]);
    const simd::d256 ys_s = _mm256_xor_pd(ys, sb);
    const simd::d256 xs_s = _mm256_xor_pd(xs, sb);
    const simd::d256 ai_s = _mm256_xor_pd(ai, sb);
    x = _mm256_sub_pd(x, ys_s);
    y = _mm256_add_pd(y, xs_s);
    z = _mm256_sub_pd(z, ai_s);
  }
  *c_out = x;
  *s_out = y;
}

};     // namespace __cordic_simd

[[gnu::flatten]] inline simd::d256
sin_cordic(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __cordic_simd::reduce_pio2_d256(x, &q);
  simd::d256 c, s;
  __cordic_simd::sincos_kernel_d256(r, &c, &s);
  //   q=0: s, q=1: c, q=2: -s, q=3: -c
  const simd::i256 q_and1 = _mm256_and_si256(q, _mm256_set1_epi64x(1));
  const simd::d256 m_odd = _mm256_castsi256_pd(_mm256_cmpeq_epi64(q_and1, _mm256_set1_epi64x(1)));
  const simd::d256 m_neg = _mm256_castsi256_pd(_mm256_cmpeq_epi64(_mm256_and_si256(q, _mm256_set1_epi64x(2)), _mm256_set1_epi64x(2)));
  simd::d256 result = _mm256_blendv_pd(s, c, m_odd);
  result = _mm256_blendv_pd(result, fneg(result), m_neg);
  return result;
}

[[gnu::flatten]] inline simd::d256
cos_cordic(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __cordic_simd::reduce_pio2_d256(x, &q);
  simd::d256 c, s;
  __cordic_simd::sincos_kernel_d256(r, &c, &s);
  // cos(x) = sin(x + pi/2) effectively shifts the quadrant by +1
  const simd::i256 qs = _mm256_add_epi64(q, _mm256_set1_epi64x(1));
  const simd::i256 q_and1 = _mm256_and_si256(qs, _mm256_set1_epi64x(1));
  const simd::d256 m_odd = _mm256_castsi256_pd(_mm256_cmpeq_epi64(q_and1, _mm256_set1_epi64x(1)));
  const simd::d256 m_neg = _mm256_castsi256_pd(_mm256_cmpeq_epi64(_mm256_and_si256(qs, _mm256_set1_epi64x(2)), _mm256_set1_epi64x(2)));
  simd::d256 result = _mm256_blendv_pd(s, c, m_odd);
  result = _mm256_blendv_pd(result, fneg(result), m_neg);
  return result;
}

[[gnu::flatten]] inline void
sincos_cordic(simd::d256 x, simd::d256 *sn, simd::d256 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::d256
tan_cordic(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __cordic_simd::reduce_pio2_d256(x, &q);
  simd::d256 c, s;
  __cordic_simd::sincos_kernel_d256(r, &c, &s);
  const simd::d256 ratio = _mm256_div_pd(s, c);
  const simd::d256 cot = _mm256_div_pd(fneg(c), s);
  const simd::i256 q_and1 = _mm256_and_si256(q, _mm256_set1_epi64x(1));
  const simd::d256 m_odd = _mm256_castsi256_pd(_mm256_cmpeq_epi64(q_and1, _mm256_set1_epi64x(1)));
  return _mm256_blendv_pd(ratio, cot, m_odd);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// f256 path (8 lanes f32)
[[gnu::flatten]] inline simd::f256
sin_cordic(simd::f256 x) noexcept
{
  const simd::d256 lo_d = _mm256_cvtps_pd(_mm256_castps256_ps128(x));
  const simd::d256 hi_d = _mm256_cvtps_pd(_mm256_extractf128_ps(x, 1));
  const simd::f128 lo = _mm256_cvtpd_ps(sin_cordic(lo_d));
  const simd::f128 hi = _mm256_cvtpd_ps(sin_cordic(hi_d));
  return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
}

[[gnu::flatten]] inline simd::f256
cos_cordic(simd::f256 x) noexcept
{
  const simd::d256 lo_d = _mm256_cvtps_pd(_mm256_castps256_ps128(x));
  const simd::d256 hi_d = _mm256_cvtps_pd(_mm256_extractf128_ps(x, 1));
  const simd::f128 lo = _mm256_cvtpd_ps(cos_cordic(lo_d));
  const simd::f128 hi = _mm256_cvtpd_ps(cos_cordic(hi_d));
  return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
}

[[gnu::flatten]] inline simd::f256
tan_cordic(simd::f256 x) noexcept
{
  const simd::d256 lo_d = _mm256_cvtps_pd(_mm256_castps256_ps128(x));
  const simd::d256 hi_d = _mm256_cvtps_pd(_mm256_extractf128_ps(x, 1));
  const simd::f128 lo = _mm256_cvtpd_ps(tan_cordic(lo_d));
  const simd::f128 hi = _mm256_cvtpd_ps(tan_cordic(hi_d));
  return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
}

[[gnu::flatten]] inline void
sincos_cordic(simd::f256 x, simd::f256 *sn, simd::f256 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// d128 / f128 paths

[[gnu::flatten]] inline simd::d128
sin_cordic(simd::d128 x) noexcept
{
  return _mm256_castpd256_pd128(sin_cordic(_mm256_castpd128_pd256(x)));
}

[[gnu::flatten]] inline simd::d128
cos_cordic(simd::d128 x) noexcept
{
  return _mm256_castpd256_pd128(cos_cordic(_mm256_castpd128_pd256(x)));
}

[[gnu::flatten]] inline simd::d128
tan_cordic(simd::d128 x) noexcept
{
  return _mm256_castpd256_pd128(tan_cordic(_mm256_castpd128_pd256(x)));
}

[[gnu::flatten]] inline void
sincos_cordic(simd::d128 x, simd::d128 *sn, simd::d128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::f128
sin_cordic(simd::f128 x) noexcept
{
  return _mm256_castps256_ps128(sin_cordic(_mm256_castps128_ps256(x)));
}

[[gnu::flatten]] inline simd::f128
cos_cordic(simd::f128 x) noexcept
{
  return _mm256_castps256_ps128(cos_cordic(_mm256_castps128_ps256(x)));
}

[[gnu::flatten]] inline simd::f128
tan_cordic(simd::f128 x) noexcept
{
  return _mm256_castps256_ps128(tan_cordic(_mm256_castps128_ps256(x)));
}

[[gnu::flatten]] inline void
sincos_cordic(simd::f128 x, simd::f128 *sn, simd::f128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

#endif     // AVX2 + FMA

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

namespace __cordic_simd_neon
{
inline constexpr f32 pio2_hi_f = 0x1.921fb4p+0f;
inline constexpr f32 pio2_lo_f = 0x1.4442d18p-23f;
inline constexpr f32 inv_pio2_f = 0x1.45f306p-1f;

using mkbits::cordic_ns::ATAN_F32;
using mkbits::cordic_ns::K_F32;
using mkbits::cordic_ns::N_FP_F32;
using mkbits::cordic_ns::SHIFT_POW_F32;

[[gnu::always_inline]] inline simd::f128
reduce_pio2_f128(simd::f128 x, int32x4_t *q_out) noexcept
{
#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
  const float32x4_t fN = vrndnq_f32(vmulq_f32(x, vdupq_n_f32(inv_pio2_f)));
#else
  const float32x4_t scaled = vmulq_f32(x, vdupq_n_f32(inv_pio2_f));
  const int32x4_t Ni0 = vcvtq_s32_f32(scaled);
  const float32x4_t fN = vcvtq_f32_s32(Ni0);
#endif
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t t = vfmsq_f32(x, fN, vdupq_n_f32(pio2_hi_f));
  t = vfmsq_f32(t, fN, vdupq_n_f32(pio2_lo_f));
#else
  float32x4_t t = vsubq_f32(x, vmulq_f32(fN, vdupq_n_f32(pio2_hi_f)));
  t = vsubq_f32(t, vmulq_f32(fN, vdupq_n_f32(pio2_lo_f)));
#endif
  *q_out = vcvtq_s32_f32(fN);
  return t;
}

[[gnu::flatten]] inline void
sincos_kernel_f128(simd::f128 z, simd::f128 *c_out, simd::f128 *s_out) noexcept
{
  float32x4_t x = vdupq_n_f32(K_F32);
  float32x4_t y = vdupq_n_f32(0.0f);
  const uint32x4_t sign_bit_mask = vdupq_n_u32(0x80000000u);
  for ( int i = 0; i < N_FP_F32; ++i ) {
    const uint32x4_t sm = vcltq_f32(z, vdupq_n_f32(0.0f));     // all-1 where z<0
    const uint32x4_t sb = vandq_u32(sm, sign_bit_mask);
    const float32x4_t vshift = vdupq_n_f32(SHIFT_POW_F32.v[i]);
    const float32x4_t ys = vmulq_f32(y, vshift);
    const float32x4_t xs = vmulq_f32(x, vshift);
    const float32x4_t ai = vdupq_n_f32(ATAN_F32.v[i]);
    const float32x4_t ys_s = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(ys), sb));
    const float32x4_t xs_s = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(xs), sb));
    const float32x4_t ai_s = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(ai), sb));
    x = vsubq_f32(x, ys_s);
    y = vaddq_f32(y, xs_s);
    z = vsubq_f32(z, ai_s);
  }
  *c_out = x;
  *s_out = y;
}

};     // namespace __cordic_simd_neon

[[gnu::flatten]] inline simd::f128
sin_cordic(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __cordic_simd_neon::reduce_pio2_f128(x, &q);
  float32x4_t c, s;
  __cordic_simd_neon::sincos_kernel_f128(r, &c, &s);
  const uint32x4_t odd = vceqq_s32(vandq_s32(q, vdupq_n_s32(1)), vdupq_n_s32(1));
  const uint32x4_t neg = vceqq_s32(vandq_s32(q, vdupq_n_s32(2)), vdupq_n_s32(2));
  float32x4_t result = vbslq_f32(odd, c, s);
  result = vbslq_f32(neg, vnegq_f32(result), result);
  return result;
}

[[gnu::flatten]] inline simd::f128
cos_cordic(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __cordic_simd_neon::reduce_pio2_f128(x, &q);
  float32x4_t c, s;
  __cordic_simd_neon::sincos_kernel_f128(r, &c, &s);
  const int32x4_t qs = vaddq_s32(q, vdupq_n_s32(1));
  const uint32x4_t odd = vceqq_s32(vandq_s32(qs, vdupq_n_s32(1)), vdupq_n_s32(1));
  const uint32x4_t neg = vceqq_s32(vandq_s32(qs, vdupq_n_s32(2)), vdupq_n_s32(2));
  float32x4_t result = vbslq_f32(odd, c, s);
  result = vbslq_f32(neg, vnegq_f32(result), result);
  return result;
}

[[gnu::flatten]] inline void
sincos_cordic(simd::f128 x, simd::f128 *sn, simd::f128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::f128
tan_cordic(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __cordic_simd_neon::reduce_pio2_f128(x, &q);
  float32x4_t c, s;
  __cordic_simd_neon::sincos_kernel_f128(r, &c, &s);
#if defined(__micron_arch_arm64)
  const float32x4_t ratio = vdivq_f32(s, c);
  const float32x4_t cot = vdivq_f32(vnegq_f32(c), s);
#else
  float32x4_t rc = vrecpeq_f32(c);
  rc = vmulq_f32(rc, vrecpsq_f32(c, rc));
  rc = vmulq_f32(rc, vrecpsq_f32(c, rc));
  const float32x4_t ratio = vmulq_f32(s, rc);
  float32x4_t rs = vrecpeq_f32(s);
  rs = vmulq_f32(rs, vrecpsq_f32(s, rs));
  rs = vmulq_f32(rs, vrecpsq_f32(s, rs));
  const float32x4_t cot = vmulq_f32(vnegq_f32(c), rs);
#endif
  const uint32x4_t odd = vceqq_s32(vandq_s32(q, vdupq_n_s32(1)), vdupq_n_s32(1));
  return vbslq_f32(odd, cot, ratio);
}

#if defined(__micron_arch_arm64)

namespace __cordic_simd_neon_d
{
inline constexpr f64 pio2_hi = 0x1.921fb54400000p+0;
inline constexpr f64 pio2_mid = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_lo = 0x1.9880000000000p-77;
inline constexpr f64 inv_pio2 = 0x1.45f306dc9c883p-1;

using mkbits::cordic_ns::ATAN_F64;
using mkbits::cordic_ns::K_F64;
using mkbits::cordic_ns::N_FP_F64;
using mkbits::cordic_ns::SHIFT_POW_F64;

[[gnu::always_inline]] inline simd::d128
reduce(simd::d128 x, int64x2_t *q_out) noexcept
{
  const float64x2_t fN = vrndnq_f64(vmulq_f64(x, vdupq_n_f64(inv_pio2)));
  float64x2_t t = vfmsq_f64(x, fN, vdupq_n_f64(pio2_hi));
  t = vfmsq_f64(t, fN, vdupq_n_f64(pio2_mid));
  t = vfmsq_f64(t, fN, vdupq_n_f64(pio2_lo));
  *q_out = vcvtq_s64_f64(fN);
  return t;
}

[[gnu::flatten]] inline void
sincos_kernel_d128(simd::d128 z, simd::d128 *c_out, simd::d128 *s_out) noexcept
{
  float64x2_t x = vdupq_n_f64(K_F64);
  float64x2_t y = vdupq_n_f64(0.0);
  const uint64x2_t sign_bit_mask = vdupq_n_u64(0x8000000000000000ULL);
  for ( int i = 0; i < N_FP_F64; ++i ) {
    const uint64x2_t sm = vcltq_f64(z, vdupq_n_f64(0.0));
    const uint64x2_t sb = vandq_u64(sm, sign_bit_mask);
    const float64x2_t vshift = vdupq_n_f64(SHIFT_POW_F64.v[i]);
    const float64x2_t ys = vmulq_f64(y, vshift);
    const float64x2_t xs = vmulq_f64(x, vshift);
    const float64x2_t ai = vdupq_n_f64(ATAN_F64.v[i]);
    const float64x2_t ys_s = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(ys), sb));
    const float64x2_t xs_s = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(xs), sb));
    const float64x2_t ai_s = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(ai), sb));
    x = vsubq_f64(x, ys_s);
    y = vaddq_f64(y, xs_s);
    z = vsubq_f64(z, ai_s);
  }
  *c_out = x;
  *s_out = y;
}

};     // namespace __cordic_simd_neon_d

[[gnu::flatten]] inline simd::d128
sin_cordic(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __cordic_simd_neon_d::reduce(x, &q);
  float64x2_t c, s;
  __cordic_simd_neon_d::sincos_kernel_d128(r, &c, &s);
  const uint64x2_t odd = vceqq_s64(vandq_s64(q, vdupq_n_s64(1)), vdupq_n_s64(1));
  const uint64x2_t neg = vceqq_s64(vandq_s64(q, vdupq_n_s64(2)), vdupq_n_s64(2));
  float64x2_t result = vbslq_f64(odd, c, s);
  result = vbslq_f64(neg, vnegq_f64(result), result);
  return result;
}

[[gnu::flatten]] inline simd::d128
cos_cordic(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __cordic_simd_neon_d::reduce(x, &q);
  float64x2_t c, s;
  __cordic_simd_neon_d::sincos_kernel_d128(r, &c, &s);
  const int64x2_t qs = vaddq_s64(q, vdupq_n_s64(1));
  const uint64x2_t odd = vceqq_s64(vandq_s64(qs, vdupq_n_s64(1)), vdupq_n_s64(1));
  const uint64x2_t neg = vceqq_s64(vandq_s64(qs, vdupq_n_s64(2)), vdupq_n_s64(2));
  float64x2_t result = vbslq_f64(odd, c, s);
  result = vbslq_f64(neg, vnegq_f64(result), result);
  return result;
}

[[gnu::flatten]] inline void
sincos_cordic(simd::d128 x, simd::d128 *sn, simd::d128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::d128
tan_cordic(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __cordic_simd_neon_d::reduce(x, &q);
  float64x2_t c, s;
  __cordic_simd_neon_d::sincos_kernel_d128(r, &c, &s);
  const float64x2_t ratio = vdivq_f64(s, c);
  const float64x2_t cot = vdivq_f64(vnegq_f64(c), s);
  const uint64x2_t odd = vceqq_s64(vandq_s64(q, vdupq_n_s64(1)), vdupq_n_s64(1));
  return vbslq_f64(odd, cot, ratio);
}

#endif     // arm64 d128

#endif     // arm_any && neon

};     // namespace mk
};     // namespace math
};     // namespace micron

#pragma GCC diagnostic pop
