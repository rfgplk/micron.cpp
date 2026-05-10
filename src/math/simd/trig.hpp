//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Cody-Waite π/2 reduction followed by a Remez polynomial
//
//   pio2_red::three_word_tag : 3-word Cody-Waite (default; ~1 ulp over a wide |x| range)
//   pio2_red::two_word_tag   : 2-word Cody-Waite (fewer FMAs, less precise, narrower good range)
//   pio2_red::one_fma_tag    : single FMA against the closest f64 pi/2 (rough estimate; fastest; error grows linearly with |fN|)

#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/coeff/sin_f32.hpp"
#include "../bits/coeff/sin_f64.hpp"
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

// argument-reduction precision tags for trig functions
namespace pio2_red
{

struct three_word_tag {
};

struct two_word_tag {
};

struct one_fma_tag {
};

template <typename T>
concept tag = micron::is_same_v<T, three_word_tag> or micron::is_same_v<T, two_word_tag> or micron::is_same_v<T, one_fma_tag>;

inline constexpr three_word_tag three_word{};
inline constexpr two_word_tag two_word{};
inline constexpr one_fma_tag one_fma{};

};     // namespace pio2_red

namespace __trig_simd
{
// f64 pi/2 split constants shared between x86 AVX d256 and ARM64 NEON d128
inline constexpr f64 pio2_hi = 0x1.921fb54400000p+0;         // 33-bit hi (zero tail → exact fN * pio2_hi for moderate fN)
inline constexpr f64 pio2_mid = 0x1.0b4611a626000p-34;       // ~43-bit mid
inline constexpr f64 pio2_lo = 0x1.9880000000000p-77;        // ~13-bit tail
inline constexpr f64 pio2_lo_2w = 0x1.0b4611a626331p-34;     // mid + lo folded into one f64 (2-word path)
inline constexpr f64 pio2_full = 0x1.921fb54442d18p+0;       // closest f64 to pi/2 (1-FMA path)
inline constexpr f64 inv_pio2 = 0x1.45f306dc9c883p-1;
};     // namespace __trig_simd

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

namespace __trig_simd
{

[[gnu::always_inline]] inline simd::d256
ksin(simd::d256 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const simd::d256 z = _mm256_mul_pd(r, r);
  const simd::d256 r3 = _mm256_mul_pd(z, r);
  simd::d256 p = _mm256_fmadd_pd(_mm256_set1_pd(S6), z, _mm256_set1_pd(S5));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(S4));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(S3));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(S2));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(S1));
  return _mm256_fmadd_pd(r3, p, r);
}

[[gnu::always_inline]] inline simd::d256
kcos(simd::d256 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const simd::d256 z = _mm256_mul_pd(r, r);
  const simd::d256 hz = _mm256_mul_pd(_mm256_set1_pd(0.5), z);
  simd::d256 p = _mm256_fmadd_pd(_mm256_set1_pd(C6), z, _mm256_set1_pd(C5));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(C4));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(C3));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(C2));
  p = _mm256_fmadd_pd(p, z, _mm256_set1_pd(C1));
  return _mm256_fmadd_pd(_mm256_mul_pd(z, z), p, _mm256_sub_pd(_mm256_set1_pd(1.0), hz));
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::always_inline]] inline simd::d256
reduce_pio2(simd::d256 x, simd::i256 *q_out) noexcept
{
  const simd::d256 fN = _mm256_round_pd(_mm256_mul_pd(x, _mm256_set1_pd(inv_pio2)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  simd::d256 t;
  if constexpr ( micron::is_same_v<R, pio2_red::one_fma_tag> ) {
    t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_full), x);
  } else if constexpr ( micron::is_same_v<R, pio2_red::two_word_tag> ) {
    t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_hi), x);
    t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_lo_2w), t);
  } else {
    t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_hi), x);
    t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_mid), t);
    t = _mm256_fnmadd_pd(fN, _mm256_set1_pd(pio2_lo), t);
  }
  const simd::i256 N32 = _mm256_castsi128_si256(_mm256_cvtpd_epi32(fN));
  *q_out = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(N32));
  return t;
}

};     // namespace __trig_simd

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d256
sin(simd::d256 x, R = {}) noexcept
{
  simd::i256 q;
  const simd::d256 r = __trig_simd::reduce_pio2<R>(x, &q);
  const simd::d256 s = __trig_simd::ksin(r);
  const simd::d256 c = __trig_simd::kcos(r);
  //   q=0: s
  //   q=1: c
  //   q=2: -s
  //   q=3: -c
  const simd::i256 q_and1 = _mm256_and_si256(q, _mm256_set1_epi64x(1));
  const simd::d256 m_odd = _mm256_castsi256_pd(_mm256_cmpeq_epi64(q_and1, _mm256_set1_epi64x(1)));
  const simd::d256 m_neg = _mm256_castsi256_pd(_mm256_cmpeq_epi64(_mm256_and_si256(q, _mm256_set1_epi64x(2)), _mm256_set1_epi64x(2)));
  simd::d256 result = _mm256_blendv_pd(s, c, m_odd);
  result = _mm256_blendv_pd(result, fneg(result), m_neg);
  return result;
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d256
cos(simd::d256 x, R = {}) noexcept
{
  simd::i256 q;
  const simd::d256 r = __trig_simd::reduce_pio2<R>(x, &q);
  const simd::d256 s = __trig_simd::ksin(r);
  const simd::d256 c = __trig_simd::kcos(r);
  // q=0: c, q=1: -s, q=2: -c, q=3: s
  // equivalent to sin(x + pi/2)
  const simd::i256 q_shift = _mm256_add_epi64(q, _mm256_set1_epi64x(1));
  const simd::i256 q_and1 = _mm256_and_si256(q_shift, _mm256_set1_epi64x(1));
  const simd::d256 m_odd = _mm256_castsi256_pd(_mm256_cmpeq_epi64(q_and1, _mm256_set1_epi64x(1)));
  const simd::d256 m_neg = _mm256_castsi256_pd(_mm256_cmpeq_epi64(_mm256_and_si256(q_shift, _mm256_set1_epi64x(2)), _mm256_set1_epi64x(2)));
  simd::d256 result = _mm256_blendv_pd(s, c, m_odd);
  result = _mm256_blendv_pd(result, fneg(result), m_neg);
  return result;
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline void
sincos(simd::d256 x, simd::d256 *sn, simd::d256 *cs, R r = {}) noexcept
{
  *sn = sin(x, r);
  *cs = cos(x, r);
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d256
tan(simd::d256 x, R = {}) noexcept
{
  simd::i256 q;
  const simd::d256 r = __trig_simd::reduce_pio2<R>(x, &q);
  const simd::d256 s = __trig_simd::ksin(r);
  const simd::d256 c = __trig_simd::kcos(r);
  const simd::d256 ratio = _mm256_div_pd(s, c);
  const simd::d256 cot = _mm256_div_pd(fneg(c), s);
  const simd::i256 q_and1 = _mm256_and_si256(q, _mm256_set1_epi64x(1));
  const simd::d256 m_odd = _mm256_castsi256_pd(_mm256_cmpeq_epi64(q_and1, _mm256_set1_epi64x(1)));
  return _mm256_blendv_pd(ratio, cot, m_odd);
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::f256
sin(simd::f256 x, R r = {}) noexcept
{
  const simd::d256 lo_d = _mm256_cvtps_pd(_mm256_castps256_ps128(x));
  const simd::d256 hi_d = _mm256_cvtps_pd(_mm256_extractf128_ps(x, 1));
  const simd::f128 lo = _mm256_cvtpd_ps(sin(lo_d, r));
  const simd::f128 hi = _mm256_cvtpd_ps(sin(hi_d, r));
  return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::f256
cos(simd::f256 x, R r = {}) noexcept
{
  const simd::d256 lo_d = _mm256_cvtps_pd(_mm256_castps256_ps128(x));
  const simd::d256 hi_d = _mm256_cvtps_pd(_mm256_extractf128_ps(x, 1));
  const simd::f128 lo = _mm256_cvtpd_ps(cos(lo_d, r));
  const simd::f128 hi = _mm256_cvtpd_ps(cos(hi_d, r));
  return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d128
sin(simd::d128 x, R r = {}) noexcept
{
  return _mm256_castpd256_pd128(sin(_mm256_castpd128_pd256(x), r));
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::f128
sin(simd::f128 x, R r = {}) noexcept
{
  return _mm256_castps256_ps128(sin(_mm256_castps128_ps256(x), r));
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d128
cos(simd::d128 x, R r = {}) noexcept
{
  return _mm256_castpd256_pd128(cos(_mm256_castpd128_pd256(x), r));
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::f128
cos(simd::f128 x, R r = {}) noexcept
{
  return _mm256_castps256_ps128(cos(_mm256_castps128_ps256(x), r));
}

#endif     // AVX2 + FMA

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

namespace __trig_simd_neon
{
inline constexpr f32 pio2_hi_f = 0x1.921fb4p+0f;       // 24-bit hi (zero tail bit → exact for small fN)
inline constexpr f32 pio2_lo_f = 0x1.4442d18p-23f;     // residual to f32 precision
inline constexpr f32 pio2_full_f = 0x1.921fb6p+0f;     // closest f32 to π/2 (1-FMA path)
inline constexpr f32 inv_pio2_f = 0x1.45f306p-1f;

[[gnu::always_inline]] inline simd::f128
ksin_f128(simd::f128 r) noexcept
{
  using namespace mkbits::coeff::sin_f32_data;
  const float32x4_t z = vmulq_f32(r, r);
  const float32x4_t r3 = vmulq_f32(z, r);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t p = vfmaq_f32(vdupq_n_f32(S3), vdupq_n_f32(S4), z);
  p = vfmaq_f32(vdupq_n_f32(S2), p, z);
  p = vfmaq_f32(vdupq_n_f32(S1), p, z);
  return vfmaq_f32(r, r3, p);
#else
  float32x4_t p = vaddq_f32(vdupq_n_f32(S3), vmulq_f32(vdupq_n_f32(S4), z));
  p = vaddq_f32(vdupq_n_f32(S2), vmulq_f32(p, z));
  p = vaddq_f32(vdupq_n_f32(S1), vmulq_f32(p, z));
  return vaddq_f32(r, vmulq_f32(r3, p));
#endif
}

[[gnu::always_inline]] inline simd::f128
kcos_f128(simd::f128 r) noexcept
{
  using namespace mkbits::coeff::sin_f32_data;
  const float32x4_t z = vmulq_f32(r, r);
  const float32x4_t hz = vmulq_f32(vdupq_n_f32(0.5f), z);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t p = vfmaq_f32(vdupq_n_f32(C3), vdupq_n_f32(C4), z);
  p = vfmaq_f32(vdupq_n_f32(C2), p, z);
  p = vfmaq_f32(vdupq_n_f32(C1), p, z);
  return vfmaq_f32(vsubq_f32(vdupq_n_f32(1.0f), hz), vmulq_f32(z, z), p);
#else
  float32x4_t p = vaddq_f32(vdupq_n_f32(C3), vmulq_f32(vdupq_n_f32(C4), z));
  p = vaddq_f32(vdupq_n_f32(C2), vmulq_f32(p, z));
  p = vaddq_f32(vdupq_n_f32(C1), vmulq_f32(p, z));
  return vaddq_f32(vsubq_f32(vdupq_n_f32(1.0f), hz), vmulq_f32(vmulq_f32(z, z), p));
#endif
}

template <pio2_red::tag R = pio2_red::three_word_tag>
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
  float32x4_t t;
  if constexpr ( micron::is_same_v<R, pio2_red::one_fma_tag> ) {
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
    t = vfmsq_f32(x, fN, vdupq_n_f32(pio2_full_f));
#else
    t = vsubq_f32(x, vmulq_f32(fN, vdupq_n_f32(pio2_full_f)));
#endif
  } else {
    // f32 mantissa fits in the 2-word split; three_word_tag and two_word_tag share this path.
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
    t = vfmsq_f32(x, fN, vdupq_n_f32(pio2_hi_f));
    t = vfmsq_f32(t, fN, vdupq_n_f32(pio2_lo_f));
#else
    t = vsubq_f32(x, vmulq_f32(fN, vdupq_n_f32(pio2_hi_f)));
    t = vsubq_f32(t, vmulq_f32(fN, vdupq_n_f32(pio2_lo_f)));
#endif
  }
  *q_out = vcvtq_s32_f32(fN);
  return t;
}

};     // namespace __trig_simd_neon

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::f128
sin(simd::f128 x, R = {}) noexcept
{
  int32x4_t q;
  const float32x4_t r = __trig_simd_neon::reduce_pio2_f128<R>(x, &q);
  const float32x4_t s = __trig_simd_neon::ksin_f128(r);
  const float32x4_t c = __trig_simd_neon::kcos_f128(r);
  const uint32x4_t odd = vceqq_s32(vandq_s32(q, vdupq_n_s32(1)), vdupq_n_s32(1));
  const uint32x4_t neg = vceqq_s32(vandq_s32(q, vdupq_n_s32(2)), vdupq_n_s32(2));
  float32x4_t result = vbslq_f32(odd, c, s);
  result = vbslq_f32(neg, vnegq_f32(result), result);
  return result;
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::f128
cos(simd::f128 x, R = {}) noexcept
{
  int32x4_t q;
  const float32x4_t r = __trig_simd_neon::reduce_pio2_f128<R>(x, &q);
  const float32x4_t s = __trig_simd_neon::ksin_f128(r);
  const float32x4_t c = __trig_simd_neon::kcos_f128(r);
  // cos = sin(x + π/2)
  const int32x4_t qs = vaddq_s32(q, vdupq_n_s32(1));
  const uint32x4_t odd = vceqq_s32(vandq_s32(qs, vdupq_n_s32(1)), vdupq_n_s32(1));
  const uint32x4_t neg = vceqq_s32(vandq_s32(qs, vdupq_n_s32(2)), vdupq_n_s32(2));
  float32x4_t result = vbslq_f32(odd, c, s);
  result = vbslq_f32(neg, vnegq_f32(result), result);
  return result;
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline void
sincos(simd::f128 x, simd::f128 *sn, simd::f128 *cs, R r = {}) noexcept
{
  *sn = sin(x, r);
  *cs = cos(x, r);
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::f128
tan(simd::f128 x, R = {}) noexcept
{
  int32x4_t q;
  const float32x4_t r = __trig_simd_neon::reduce_pio2_f128<R>(x, &q);
  const float32x4_t s = __trig_simd_neon::ksin_f128(r);
  const float32x4_t c = __trig_simd_neon::kcos_f128(r);
#if defined(__micron_arch_arm64)
  const float32x4_t ratio = vdivq_f32(s, c);
  const float32x4_t cot = vdivq_f32(vnegq_f32(c), s);
#else
  // arm32 lacks hw vdivq_f32; refine vrecpeq via two Newton-Raphson steps.
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
namespace __trig_simd_neon_d
{
[[gnu::always_inline]] inline simd::d128
ksin(simd::d128 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const float64x2_t z = vmulq_f64(r, r);
  const float64x2_t r3 = vmulq_f64(z, r);
  float64x2_t p = vfmaq_f64(vdupq_n_f64(S5), vdupq_n_f64(S6), z);
  p = vfmaq_f64(vdupq_n_f64(S4), p, z);
  p = vfmaq_f64(vdupq_n_f64(S3), p, z);
  p = vfmaq_f64(vdupq_n_f64(S2), p, z);
  p = vfmaq_f64(vdupq_n_f64(S1), p, z);
  return vfmaq_f64(r, r3, p);
}

[[gnu::always_inline]] inline simd::d128
kcos(simd::d128 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const float64x2_t z = vmulq_f64(r, r);
  const float64x2_t hz = vmulq_f64(vdupq_n_f64(0.5), z);
  float64x2_t p = vfmaq_f64(vdupq_n_f64(C5), vdupq_n_f64(C6), z);
  p = vfmaq_f64(vdupq_n_f64(C4), p, z);
  p = vfmaq_f64(vdupq_n_f64(C3), p, z);
  p = vfmaq_f64(vdupq_n_f64(C2), p, z);
  p = vfmaq_f64(vdupq_n_f64(C1), p, z);
  return vfmaq_f64(vsubq_f64(vdupq_n_f64(1.0), hz), vmulq_f64(z, z), p);
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::always_inline]] inline simd::d128
reduce(simd::d128 x, int64x2_t *q_out) noexcept
{
  using namespace __trig_simd;
  const float64x2_t fN = vrndnq_f64(vmulq_f64(x, vdupq_n_f64(inv_pio2)));
  float64x2_t t;
  if constexpr ( micron::is_same_v<R, pio2_red::one_fma_tag> ) {
    t = vfmsq_f64(x, fN, vdupq_n_f64(pio2_full));
  } else if constexpr ( micron::is_same_v<R, pio2_red::two_word_tag> ) {
    t = vfmsq_f64(x, fN, vdupq_n_f64(pio2_hi));
    t = vfmsq_f64(t, fN, vdupq_n_f64(pio2_lo_2w));
  } else {
    t = vfmsq_f64(x, fN, vdupq_n_f64(pio2_hi));
    t = vfmsq_f64(t, fN, vdupq_n_f64(pio2_mid));
    t = vfmsq_f64(t, fN, vdupq_n_f64(pio2_lo));
  }
  *q_out = vcvtq_s64_f64(fN);
  return t;
}
};     // namespace __trig_simd_neon_d

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d128
sin(simd::d128 x, R = {}) noexcept
{
  int64x2_t q;
  const float64x2_t r = __trig_simd_neon_d::reduce<R>(x, &q);
  const float64x2_t s = __trig_simd_neon_d::ksin(r);
  const float64x2_t c = __trig_simd_neon_d::kcos(r);
  const uint64x2_t odd = vceqq_s64(vandq_s64(q, vdupq_n_s64(1)), vdupq_n_s64(1));
  const uint64x2_t neg = vceqq_s64(vandq_s64(q, vdupq_n_s64(2)), vdupq_n_s64(2));
  float64x2_t result = vbslq_f64(odd, c, s);
  result = vbslq_f64(neg, vnegq_f64(result), result);
  return result;
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d128
cos(simd::d128 x, R = {}) noexcept
{
  int64x2_t q;
  const float64x2_t r = __trig_simd_neon_d::reduce<R>(x, &q);
  const float64x2_t s = __trig_simd_neon_d::ksin(r);
  const float64x2_t c = __trig_simd_neon_d::kcos(r);
  const int64x2_t qs = vaddq_s64(q, vdupq_n_s64(1));
  const uint64x2_t odd = vceqq_s64(vandq_s64(qs, vdupq_n_s64(1)), vdupq_n_s64(1));
  const uint64x2_t neg = vceqq_s64(vandq_s64(qs, vdupq_n_s64(2)), vdupq_n_s64(2));
  float64x2_t result = vbslq_f64(odd, c, s);
  result = vbslq_f64(neg, vnegq_f64(result), result);
  return result;
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline void
sincos(simd::d128 x, simd::d128 *sn, simd::d128 *cs, R r = {}) noexcept
{
  *sn = sin(x, r);
  *cs = cos(x, r);
}

template <pio2_red::tag R = pio2_red::three_word_tag>
[[gnu::flatten]] inline simd::d128
tan(simd::d128 x, R = {}) noexcept
{
  int64x2_t q;
  const float64x2_t r = __trig_simd_neon_d::reduce<R>(x, &q);
  const float64x2_t s = __trig_simd_neon_d::ksin(r);
  const float64x2_t c = __trig_simd_neon_d::kcos(r);
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
