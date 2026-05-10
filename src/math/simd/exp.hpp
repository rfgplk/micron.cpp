//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/coeff/exp_f32.hpp"
#include "../bits/coeff/exp_f64.hpp"
#include "_dispatch.hpp"

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

[[gnu::flatten]] inline simd::d256
exp(simd::d256 x) noexcept
{
  using namespace mkbits::coeff::exp_f64_data;

  const simd::d256 hi = _mm256_set1_pd(709.78);
  const simd::d256 lo = _mm256_set1_pd(-745.13);
  x = _mm256_min_pd(_mm256_max_pd(x, lo), hi);

  const simd::d256 fN = _mm256_round_pd(_mm256_mul_pd(x, _mm256_set1_pd(inv_ln2_32)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);

  simd::d256 r = _mm256_fnmadd_pd(fN, _mm256_set1_pd(ln2_32_hi), x);
  r = _mm256_fnmadd_pd(fN, _mm256_set1_pd(ln2_32_lo), r);

  simd::d256 p = _mm256_fmadd_pd(_mm256_set1_pd(rem[4]), r, _mm256_set1_pd(rem[3]));
  p = _mm256_fmadd_pd(p, r, _mm256_set1_pd(rem[2]));
  p = _mm256_fmadd_pd(p, r, _mm256_set1_pd(rem[1]));
  p = _mm256_fmadd_pd(p, r, _mm256_set1_pd(rem[0]));
  const simd::d256 r2 = _mm256_mul_pd(r, r);
  const simd::d256 exp_r = _mm256_fmadd_pd(p, r2, _mm256_add_pd(r, _mm256_set1_pd(1.0)));

  const __m128i N32 = _mm256_cvtpd_epi32(fN);
  const __m128i j32 = _mm_and_si128(N32, _mm_set1_epi32(31));

  const simd::d256 tw = _mm256_i32gather_pd(reinterpret_cast<const double *>(twoN), j32, 8);

  const __m128i k32 = _mm_srai_epi32(N32, 5);
  const simd::i256 k64 = _mm256_cvtepi32_epi64(k32);
  const simd::i256 biased = _mm256_add_epi64(k64, _mm256_set1_epi64x(1023));
  const simd::i256 expbits = _mm256_slli_epi64(biased, 52);
  const simd::d256 scale = _mm256_castsi256_pd(expbits);
  return _mm256_mul_pd(_mm256_mul_pd(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::f256
exp(simd::f256 x) noexcept
{
  using namespace mkbits::coeff::exp_f32_data;
  const simd::f256 hi = _mm256_set1_ps(88.722f);
  const simd::f256 lo = _mm256_set1_ps(-103.972f);
  x = _mm256_min_ps(_mm256_max_ps(x, lo), hi);
  const simd::f256 fN = _mm256_round_ps(_mm256_mul_ps(x, _mm256_set1_ps(inv_ln2_16)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  simd::f256 r = _mm256_fnmadd_ps(fN, _mm256_set1_ps(ln2_16_hi), x);
  r = _mm256_fnmadd_ps(fN, _mm256_set1_ps(ln2_16_lo), r);
  simd::f256 p = _mm256_fmadd_ps(_mm256_set1_ps(rem[1]), r, _mm256_set1_ps(rem[0]));
  const simd::f256 r2 = _mm256_mul_ps(r, r);
  const simd::f256 exp_r = _mm256_fmadd_ps(p, r2, _mm256_add_ps(r, _mm256_set1_ps(1.0f)));
  const simd::i256 N = _mm256_cvtps_epi32(fN);
  const simd::i256 j = _mm256_and_si256(N, _mm256_set1_epi32(15));
  const simd::f256 tw = _mm256_i32gather_ps(reinterpret_cast<const float *>(twoN), j, 4);
  const simd::i256 k = _mm256_srai_epi32(N, 4);
  const simd::i256 biased = _mm256_add_epi32(k, _mm256_set1_epi32(127));
  const simd::i256 expbits = _mm256_slli_epi32(biased, 23);
  const simd::f256 scale = _mm256_castsi256_ps(expbits);
  return _mm256_mul_ps(_mm256_mul_ps(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::d128
exp(simd::d128 x) noexcept
{

  return _mm256_castpd256_pd128(exp(_mm256_castpd128_pd256(x)));
}

[[gnu::flatten]] inline simd::f128
exp(simd::f128 x) noexcept
{
  return _mm256_castps256_ps128(exp(_mm256_castps128_ps256(x)));
}

[[gnu::flatten]] inline simd::d256
exp2(simd::d256 x) noexcept
{

  constexpr f64 ln2_hi = 0x1.62e42fefa39efp-1;
  constexpr f64 ln2_lo = 0x1.abc9e3b39803fp-56;
  const simd::d256 y = _mm256_fmadd_pd(x, _mm256_set1_pd(ln2_hi), _mm256_mul_pd(x, _mm256_set1_pd(ln2_lo)));
  return exp(y);
}

[[gnu::flatten]] inline simd::f256
exp2(simd::f256 x) noexcept
{
  constexpr f32 ln2 = 0x1.62e430p-1f;
  return exp(_mm256_mul_ps(x, _mm256_set1_ps(ln2)));
}

[[gnu::flatten]] inline simd::d256
expm1(simd::d256 x) noexcept
{
  return _mm256_sub_pd(exp(x), _mm256_set1_pd(1.0));
}

[[gnu::flatten]] inline simd::f256
expm1(simd::f256 x) noexcept
{
  return _mm256_sub_ps(exp(x), _mm256_set1_ps(1.0f));
}

#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::flatten]] inline simd::f128
exp(simd::f128 x) noexcept
{
  using namespace mkbits::coeff::exp_f32_data;
  const float32x4_t cap_hi = vdupq_n_f32(88.722f);
  const float32x4_t cap_lo = vdupq_n_f32(-103.972f);
  x = vminq_f32(vmaxq_f32(x, cap_lo), cap_hi);

#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
  const float32x4_t fN = vrndnq_f32(vmulq_f32(x, vdupq_n_f32(inv_ln2_16)));
#else

  const int32x4_t Ni_pre = vcvtq_s32_f32(vmulq_f32(x, vdupq_n_f32(inv_ln2_16 + 0.5f)));
  const float32x4_t fN = vcvtq_f32_s32(Ni_pre);
#endif

#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t r = vfmsq_f32(x, fN, vdupq_n_f32(ln2_16_hi));
  r = vfmsq_f32(r, fN, vdupq_n_f32(ln2_16_lo));

  float32x4_t p = vfmaq_f32(vdupq_n_f32(rem[0]), vdupq_n_f32(rem[1]), r);
#else
  float32x4_t r = vsubq_f32(x, vmulq_f32(fN, vdupq_n_f32(ln2_16_hi)));
  r = vsubq_f32(r, vmulq_f32(fN, vdupq_n_f32(ln2_16_lo)));
  float32x4_t p = vaddq_f32(vdupq_n_f32(rem[0]), vmulq_f32(vdupq_n_f32(rem[1]), r));
#endif
  const float32x4_t r2 = vmulq_f32(r, r);
  const float32x4_t one = vdupq_n_f32(1.0f);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  const float32x4_t exp_r = vfmaq_f32(vaddq_f32(r, one), p, r2);
#else
  const float32x4_t exp_r = vaddq_f32(vaddq_f32(r, one), vmulq_f32(p, r2));
#endif

  const int32x4_t Ni = vcvtq_s32_f32(fN);
  const int32x4_t j = vandq_s32(Ni, vdupq_n_s32(15));
  const f32 tw_arr[4] = {
    twoN[vgetq_lane_s32(j, 0)],
    twoN[vgetq_lane_s32(j, 1)],
    twoN[vgetq_lane_s32(j, 2)],
    twoN[vgetq_lane_s32(j, 3)],
  };
  const float32x4_t tw = vld1q_f32(tw_arr);

  const int32x4_t k = vshrq_n_s32(Ni, 4);
  const int32x4_t biased = vaddq_s32(k, vdupq_n_s32(127));
  const int32x4_t expbits = vshlq_n_s32(biased, 23);
  const float32x4_t scale = vreinterpretq_f32_s32(expbits);
  return vmulq_f32(vmulq_f32(tw, exp_r), scale);
}

#if defined(__micron_arch_arm64)

[[gnu::flatten]] inline simd::d128
exp(simd::d128 x) noexcept
{
  using namespace mkbits::coeff::exp_f64_data;
  const float64x2_t cap_hi = vdupq_n_f64(709.78);
  const float64x2_t cap_lo = vdupq_n_f64(-745.13);
  x = vminq_f64(vmaxq_f64(x, cap_lo), cap_hi);
  const float64x2_t fN = vrndnq_f64(vmulq_f64(x, vdupq_n_f64(inv_ln2_32)));
  float64x2_t r = vfmsq_f64(x, fN, vdupq_n_f64(ln2_32_hi));
  r = vfmsq_f64(r, fN, vdupq_n_f64(ln2_32_lo));
  float64x2_t p = vfmaq_f64(vdupq_n_f64(rem[3]), vdupq_n_f64(rem[4]), r);
  p = vfmaq_f64(vdupq_n_f64(rem[2]), p, r);
  p = vfmaq_f64(vdupq_n_f64(rem[1]), p, r);
  p = vfmaq_f64(vdupq_n_f64(rem[0]), p, r);
  const float64x2_t r2 = vmulq_f64(r, r);
  const float64x2_t one = vdupq_n_f64(1.0);
  const float64x2_t exp_r = vfmaq_f64(vaddq_f64(r, one), p, r2);
  const int64x2_t Ni = vcvtq_s64_f64(fN);
  const int64x2_t j = vandq_s64(Ni, vdupq_n_s64(31));
  const f64 tw_arr[2] = {
    twoN[vgetq_lane_s64(j, 0)],
    twoN[vgetq_lane_s64(j, 1)],
  };
  const float64x2_t tw = vld1q_f64(tw_arr);
  const int64x2_t k = vshrq_n_s64(Ni, 5);
  const int64x2_t biased = vaddq_s64(k, vdupq_n_s64(1023));
  const int64x2_t expbits = vshlq_n_s64(biased, 52);
  const float64x2_t scale = vreinterpretq_f64_s64(expbits);
  return vmulq_f64(vmulq_f64(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::d128
exp2(simd::d128 x) noexcept
{
  constexpr f64 ln2_hi = 0x1.62e42fefa39efp-1;
  constexpr f64 ln2_lo = 0x1.abc9e3b39803fp-56;
  const float64x2_t y = vfmaq_f64(vmulq_f64(x, vdupq_n_f64(ln2_lo)), x, vdupq_n_f64(ln2_hi));
  return exp(y);
}

[[gnu::flatten]] inline simd::d128
expm1(simd::d128 x) noexcept
{
  return vsubq_f64(exp(x), vdupq_n_f64(1.0));
}
#endif

[[gnu::flatten]] inline simd::f128
exp2(simd::f128 x) noexcept
{
  constexpr f32 ln2 = 0x1.62e430p-1f;
  return exp(vmulq_f32(x, vdupq_n_f32(ln2)));
}

[[gnu::flatten]] inline simd::f128
expm1(simd::f128 x) noexcept
{
  return vsubq_f32(exp(x), vdupq_n_f32(1.0f));
}

#endif

#if defined(__micron_x86_avx512f)

[[gnu::flatten]] inline simd::d512
exp(simd::d512 x) noexcept
{
  using namespace mkbits::coeff::exp_f64_data;
  const simd::d512 hi = _mm512_set1_pd(709.78);
  const simd::d512 lo = _mm512_set1_pd(-745.13);
  x = _mm512_min_pd(_mm512_max_pd(x, lo), hi);
  const simd::d512 fN = _mm512_roundscale_pd(_mm512_mul_pd(x, _mm512_set1_pd(inv_ln2_32)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  simd::d512 r = _mm512_fnmadd_pd(fN, _mm512_set1_pd(ln2_32_hi), x);
  r = _mm512_fnmadd_pd(fN, _mm512_set1_pd(ln2_32_lo), r);
  simd::d512 p = _mm512_fmadd_pd(_mm512_set1_pd(rem[4]), r, _mm512_set1_pd(rem[3]));
  p = _mm512_fmadd_pd(p, r, _mm512_set1_pd(rem[2]));
  p = _mm512_fmadd_pd(p, r, _mm512_set1_pd(rem[1]));
  p = _mm512_fmadd_pd(p, r, _mm512_set1_pd(rem[0]));
  const simd::d512 r2 = _mm512_mul_pd(r, r);
  const simd::d512 exp_r = _mm512_fmadd_pd(p, r2, _mm512_add_pd(r, _mm512_set1_pd(1.0)));
  const simd::i512 N = _mm512_cvtpd_epi64(fN);
  const simd::i512 j = _mm512_and_si512(N, _mm512_set1_epi64(31));
  const simd::d512 tw = _mm512_i64gather_pd(j, twoN, 8);
  const simd::i512 k = _mm512_srai_epi64(N, 5);
  const simd::i512 biased = _mm512_add_epi64(k, _mm512_set1_epi64(1023));
  const simd::i512 expbits = _mm512_slli_epi64(biased, 52);
  const simd::d512 scale = _mm512_castsi512_pd(expbits);
  return _mm512_mul_pd(_mm512_mul_pd(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::f512
exp(simd::f512 x) noexcept
{
  using namespace mkbits::coeff::exp_f32_data;
  const simd::f512 hi = _mm512_set1_ps(88.722f);
  const simd::f512 lo = _mm512_set1_ps(-103.972f);
  x = _mm512_min_ps(_mm512_max_ps(x, lo), hi);
  const simd::f512 fN = _mm512_roundscale_ps(_mm512_mul_ps(x, _mm512_set1_ps(inv_ln2_16)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  simd::f512 r = _mm512_fnmadd_ps(fN, _mm512_set1_ps(ln2_16_hi), x);
  r = _mm512_fnmadd_ps(fN, _mm512_set1_ps(ln2_16_lo), r);
  simd::f512 p = _mm512_fmadd_ps(_mm512_set1_ps(rem[1]), r, _mm512_set1_ps(rem[0]));
  const simd::f512 r2 = _mm512_mul_ps(r, r);
  const simd::f512 exp_r = _mm512_fmadd_ps(p, r2, _mm512_add_ps(r, _mm512_set1_ps(1.0f)));
  const simd::i512 N = _mm512_cvtps_epi32(fN);
  const simd::i512 j = _mm512_and_si512(N, _mm512_set1_epi32(15));
  const simd::f512 tw = _mm512_i32gather_ps(j, twoN, 4);
  const simd::i512 k = _mm512_srai_epi32(N, 4);
  const simd::i512 biased = _mm512_add_epi32(k, _mm512_set1_epi32(127));
  const simd::i512 expbits = _mm512_slli_epi32(biased, 23);
  const simd::f512 scale = _mm512_castsi512_ps(expbits);
  return _mm512_mul_ps(_mm512_mul_ps(tw, exp_r), scale);
}
#endif

};     // namespace mk
};     // namespace math
};     // namespace micron

#pragma GCC diagnostic pop
