//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/coeff/log_f32.hpp"
#include "../bits/coeff/log_f64.hpp"
#include "_dispatch.hpp"

namespace micron
{
namespace math
{
namespace mk
{

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

[[gnu::flatten]] inline simd::d256
log(simd::d256 x) noexcept
{
  using namespace mkbits::coeff::log_f64_data;

  const simd::i256 xi = _mm256_castpd_si256(x);

  const simd::i256 exp_field = _mm256_srli_epi64(xi, 52);
  simd::i256 k = _mm256_sub_epi64(_mm256_and_si256(exp_field, _mm256_set1_epi64x(0x7FF)), _mm256_set1_epi64x(1023));
  const simd::i256 mant_mask = _mm256_set1_epi64x(0x000FFFFFFFFFFFFFLL);
  const simd::i256 bias_field = _mm256_set1_epi64x(0x3FF0000000000000LL);
  simd::d256 m = _mm256_castsi256_pd(_mm256_or_si256(_mm256_and_si256(xi, mant_mask), bias_field));

  const simd::d256 sqrt_half = _mm256_set1_pd(0x1.6a09e667f3bcdp-1);
  const simd::d256 mask = _mm256_cmp_pd(m, sqrt_half, _CMP_LT_OQ);
  m = _mm256_blendv_pd(m, _mm256_add_pd(m, m), mask);
  k = _mm256_sub_epi64(k, _mm256_and_si256(_mm256_castpd_si256(mask), _mm256_set1_epi64x(1)));

  const simd::d256 f = _mm256_sub_pd(m, _mm256_set1_pd(1.0));
  const simd::d256 s = _mm256_div_pd(f, _mm256_add_pd(_mm256_set1_pd(2.0), f));
  const simd::d256 z = _mm256_mul_pd(s, s);
  const simd::d256 w = _mm256_mul_pd(z, z);

  simd::d256 t1 = _mm256_fmadd_pd(_mm256_set1_pd(Lg6), w, _mm256_set1_pd(Lg4));
  t1 = _mm256_fmadd_pd(t1, w, _mm256_set1_pd(Lg2));
  t1 = _mm256_mul_pd(t1, w);

  simd::d256 t2 = _mm256_fmadd_pd(_mm256_set1_pd(Lg7), w, _mm256_set1_pd(Lg5));
  t2 = _mm256_fmadd_pd(t2, w, _mm256_set1_pd(Lg3));
  t2 = _mm256_fmadd_pd(t2, w, _mm256_set1_pd(Lg1));
  t2 = _mm256_mul_pd(t2, z);
  const simd::d256 R = _mm256_add_pd(t1, t2);
  const simd::d256 hfsq = _mm256_mul_pd(_mm256_set1_pd(0.5), _mm256_mul_pd(f, f));
  const simd::d256 log_m = _mm256_add_pd(_mm256_mul_pd(s, _mm256_add_pd(hfsq, R)), _mm256_sub_pd(f, hfsq));

  const simd::d256 fk = _mm256_setr_pd(f64(_mm256_extract_epi64(k, 0)), f64(_mm256_extract_epi64(k, 1)), f64(_mm256_extract_epi64(k, 2)),
                                       f64(_mm256_extract_epi64(k, 3)));
  return _mm256_fmadd_pd(fk, _mm256_set1_pd(ln2_hi), _mm256_fmadd_pd(fk, _mm256_set1_pd(ln2_lo), log_m));
}

[[gnu::flatten]] inline simd::f256
log(simd::f256 x) noexcept
{
  using namespace mkbits::coeff::log_f32_data;
  const simd::i256 xi = _mm256_castps_si256(x);
  simd::i256 k = _mm256_sub_epi32(_mm256_and_si256(_mm256_srli_epi32(xi, 23), _mm256_set1_epi32(0xFF)), _mm256_set1_epi32(127));
  simd::f256 m = _mm256_castsi256_ps(_mm256_or_si256(_mm256_and_si256(xi, _mm256_set1_epi32(0x007FFFFF)), _mm256_set1_epi32(0x3F800000)));
  const simd::f256 sqrt_half = _mm256_set1_ps(0x1.6a09e6p-1f);
  const simd::f256 mask = _mm256_cmp_ps(m, sqrt_half, _CMP_LT_OQ);
  m = _mm256_blendv_ps(m, _mm256_add_ps(m, m), mask);
  k = _mm256_sub_epi32(k, _mm256_and_si256(_mm256_castps_si256(mask), _mm256_set1_epi32(1)));
  const simd::f256 f = _mm256_sub_ps(m, _mm256_set1_ps(1.0f));
  const simd::f256 s = _mm256_div_ps(f, _mm256_add_ps(_mm256_set1_ps(2.0f), f));
  const simd::f256 z = _mm256_mul_ps(s, s);
  const simd::f256 w = _mm256_mul_ps(z, z);
  simd::f256 t1 = _mm256_fmadd_ps(_mm256_set1_ps(Lg4), w, _mm256_set1_ps(Lg2));
  t1 = _mm256_mul_ps(t1, w);
  simd::f256 t2 = _mm256_fmadd_ps(_mm256_set1_ps(Lg3), w, _mm256_set1_ps(Lg1));
  t2 = _mm256_mul_ps(t2, z);
  const simd::f256 R = _mm256_add_ps(t1, t2);
  const simd::f256 hfsq = _mm256_mul_ps(_mm256_set1_ps(0.5f), _mm256_mul_ps(f, f));
  const simd::f256 log_m = _mm256_add_ps(_mm256_mul_ps(s, _mm256_add_ps(hfsq, R)), _mm256_sub_ps(f, hfsq));
  const simd::f256 fk = _mm256_cvtepi32_ps(k);
  return _mm256_fmadd_ps(fk, _mm256_set1_ps(ln2_hi), _mm256_fmadd_ps(fk, _mm256_set1_ps(ln2_lo), log_m));
}

[[gnu::flatten]] inline simd::d256
log2(simd::d256 x) noexcept
{
  return _mm256_mul_pd(log(x), _mm256_set1_pd(0x1.71547652b82fep+0));
}

[[gnu::flatten]] inline simd::f256
log2(simd::f256 x) noexcept
{
  return _mm256_mul_ps(log(x), _mm256_set1_ps(0x1.715476p+0f));
}

[[gnu::flatten]] inline simd::d256
log10(simd::d256 x) noexcept
{
  return _mm256_mul_pd(log(x), _mm256_set1_pd(0x1.bcb7b1526e50ep-2));
}

[[gnu::flatten]] inline simd::f256
log10(simd::f256 x) noexcept
{
  return _mm256_mul_ps(log(x), _mm256_set1_ps(0x1.bcb7b2p-2f));
}

[[gnu::flatten]] inline simd::d128
log(simd::d128 x) noexcept
{
  return _mm256_castpd256_pd128(log(_mm256_castpd128_pd256(x)));
}

[[gnu::flatten]] inline simd::f128
log(simd::f128 x) noexcept
{
  return _mm256_castps256_ps128(log(_mm256_castps128_ps256(x)));
}

[[gnu::flatten]] inline simd::d128
log2(simd::d128 x) noexcept
{
  return _mm256_castpd256_pd128(log2(_mm256_castpd128_pd256(x)));
}

[[gnu::flatten]] inline simd::f128
log2(simd::f128 x) noexcept
{
  return _mm256_castps256_ps128(log2(_mm256_castps128_ps256(x)));
}

[[gnu::flatten]] inline simd::d128
log10(simd::d128 x) noexcept
{
  return _mm256_castpd256_pd128(log10(_mm256_castpd128_pd256(x)));
}

[[gnu::flatten]] inline simd::f128
log10(simd::f128 x) noexcept
{
  return _mm256_castps256_ps128(log10(_mm256_castps128_ps256(x)));
}

#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::flatten]] inline simd::f128
log(simd::f128 x) noexcept
{
  using namespace mkbits::coeff::log_f32_data;
  const int32x4_t xi = vreinterpretq_s32_f32(x);

  int32x4_t k = vsubq_s32(vandq_s32(vshrq_n_s32(xi, 23), vdupq_n_s32(0xFF)), vdupq_n_s32(127));

  const int32x4_t mant_mask = vdupq_n_s32(0x007FFFFF);
  const int32x4_t bias = vdupq_n_s32(0x3F800000);
  float32x4_t m = vreinterpretq_f32_s32(vorrq_s32(vandq_s32(xi, mant_mask), bias));

  const float32x4_t sqrt_half = vdupq_n_f32(0x1.6a09e6p-1f);
  const uint32x4_t lt = vcltq_f32(m, sqrt_half);
  m = vbslq_f32(lt, vaddq_f32(m, m), m);
  k = vsubq_s32(k, vandq_s32(vreinterpretq_s32_u32(lt), vdupq_n_s32(1)));
  const float32x4_t f = vsubq_f32(m, vdupq_n_f32(1.0f));
#if defined(__micron_arch_arm64)
  const float32x4_t s = vdivq_f32(f, vaddq_f32(vdupq_n_f32(2.0f), f));
#else

  const float32x4_t denom = vaddq_f32(vdupq_n_f32(2.0f), f);
  float32x4_t recip = vrecpeq_f32(denom);
  recip = vmulq_f32(recip, vrecpsq_f32(denom, recip));
  recip = vmulq_f32(recip, vrecpsq_f32(denom, recip));
  const float32x4_t s = vmulq_f32(f, recip);
#endif
  const float32x4_t z = vmulq_f32(s, s);
  const float32x4_t w = vmulq_f32(z, z);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t t1 = vfmaq_f32(vdupq_n_f32(Lg2), vdupq_n_f32(Lg4), w);
  t1 = vmulq_f32(t1, w);
  float32x4_t t2 = vfmaq_f32(vdupq_n_f32(Lg1), vdupq_n_f32(Lg3), w);
  t2 = vmulq_f32(t2, z);
#else
  float32x4_t t1 = vmulq_f32(vaddq_f32(vdupq_n_f32(Lg2), vmulq_f32(vdupq_n_f32(Lg4), w)), w);
  float32x4_t t2 = vmulq_f32(vaddq_f32(vdupq_n_f32(Lg1), vmulq_f32(vdupq_n_f32(Lg3), w)), z);
#endif
  const float32x4_t R = vaddq_f32(t1, t2);
  const float32x4_t hfsq = vmulq_f32(vdupq_n_f32(0.5f), vmulq_f32(f, f));
  const float32x4_t log_m = vaddq_f32(vmulq_f32(s, vaddq_f32(hfsq, R)), vsubq_f32(f, hfsq));
  const float32x4_t fk = vcvtq_f32_s32(k);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  return vfmaq_f32(vfmaq_f32(log_m, fk, vdupq_n_f32(ln2_lo)), fk, vdupq_n_f32(ln2_hi));
#else
  return vaddq_f32(vaddq_f32(vmulq_f32(fk, vdupq_n_f32(ln2_hi)), vmulq_f32(fk, vdupq_n_f32(ln2_lo))), log_m);
#endif
}

[[gnu::flatten]] inline simd::f128
log2(simd::f128 x) noexcept
{
  return vmulq_f32(log(x), vdupq_n_f32(0x1.715476p+0f));
}

[[gnu::flatten]] inline simd::f128
log10(simd::f128 x) noexcept
{
  return vmulq_f32(log(x), vdupq_n_f32(0x1.bcb7b2p-2f));
}

#if defined(__micron_arch_arm64)

[[gnu::flatten]] inline simd::d128
log(simd::d128 x) noexcept
{
  using namespace mkbits::coeff::log_f64_data;
  const int64x2_t xi = vreinterpretq_s64_f64(x);
  int64x2_t k = vsubq_s64(vandq_s64(vshrq_n_s64(xi, 52), vdupq_n_s64(0x7FF)), vdupq_n_s64(1023));
  const int64x2_t mant_mask = vdupq_n_s64(0x000FFFFFFFFFFFFFLL);
  const int64x2_t bias = vdupq_n_s64(0x3FF0000000000000LL);
  float64x2_t m = vreinterpretq_f64_s64(vorrq_s64(vandq_s64(xi, mant_mask), bias));
  const float64x2_t sqrt_half = vdupq_n_f64(0x1.6a09e667f3bcdp-1);
  const uint64x2_t lt = vcltq_f64(m, sqrt_half);
  m = vbslq_f64(lt, vaddq_f64(m, m), m);
  k = vsubq_s64(k, vandq_s64(vreinterpretq_s64_u64(lt), vdupq_n_s64(1)));
  const float64x2_t f = vsubq_f64(m, vdupq_n_f64(1.0));
  const float64x2_t s = vdivq_f64(f, vaddq_f64(vdupq_n_f64(2.0), f));
  const float64x2_t z = vmulq_f64(s, s);
  const float64x2_t w = vmulq_f64(z, z);
  float64x2_t t1 = vfmaq_f64(vdupq_n_f64(Lg4), vdupq_n_f64(Lg6), w);
  t1 = vfmaq_f64(vdupq_n_f64(Lg2), t1, w);
  t1 = vmulq_f64(t1, w);
  float64x2_t t2 = vfmaq_f64(vdupq_n_f64(Lg5), vdupq_n_f64(Lg7), w);
  t2 = vfmaq_f64(vdupq_n_f64(Lg3), t2, w);
  t2 = vfmaq_f64(vdupq_n_f64(Lg1), t2, w);
  t2 = vmulq_f64(t2, z);
  const float64x2_t R = vaddq_f64(t1, t2);
  const float64x2_t hfsq = vmulq_f64(vdupq_n_f64(0.5), vmulq_f64(f, f));
  const float64x2_t log_m = vaddq_f64(vmulq_f64(s, vaddq_f64(hfsq, R)), vsubq_f64(f, hfsq));

  const float64x2_t fk = vcvtq_f64_s64(k);
  return vfmaq_f64(vfmaq_f64(log_m, fk, vdupq_n_f64(ln2_lo)), fk, vdupq_n_f64(ln2_hi));
}

[[gnu::flatten]] inline simd::d128
log2(simd::d128 x) noexcept
{
  return vmulq_f64(log(x), vdupq_n_f64(0x1.71547652b82fep+0));
}

[[gnu::flatten]] inline simd::d128
log10(simd::d128 x) noexcept
{
  return vmulq_f64(log(x), vdupq_n_f64(0x1.bcb7b1526e50ep-2));
}
#endif

#endif

};     // namespace mk
};     // namespace math
};     // namespace micron
