//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../namespace.hpp"

namespace micron
{
namespace simd
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define _ri8(x) vreinterpretq_s8_s32(x)
#define _ri16(x) vreinterpretq_s16_s32(x)
#define _ri32(x) (x)
#define _ri64(x) vreinterpretq_s64_s32(x)
#define _ru8(x) vreinterpretq_u8_s32(x)
#define _ru16(x) vreinterpretq_u16_s32(x)
#define _ru32(x) vreinterpretq_u32_s32(x)
#define _ru64(x) vreinterpretq_u64_s32(x)
#define _ri8r(x) vreinterpretq_s32_s8(x)
#define _ri16r(x) vreinterpretq_s32_s16(x)
#define _ri64r(x) vreinterpretq_s32_s64(x)
#define _ru8r(x) vreinterpretq_s32_u8(x)
#define _ru16r(x) vreinterpretq_s32_u16(x)
#define _ru32r(x) vreinterpretq_s32_u32(x)
#define _ru64r(x) vreinterpretq_s32_u64(x)

#define _rf32(x) vreinterpretq_f32_s32(x)
#define _rf64(x) vreinterpretq_f64_s32(x)
#define _rf32r(x) vreinterpretq_s32_f32(x)
#define _rf64r(x) vreinterpretq_s32_f64(x)
#define _ru32f(x) vreinterpretq_u32_f32(x)
#define _ru64d(x) vreinterpretq_u64_f64(x)
#define _rf32u(x) vreinterpretq_f32_u32(x)
#define _rf64u(x) vreinterpretq_f64_u64(x)

__attribute__((always_inline)) static inline uint16x8_t
__expand_mask_u16(uint8_t k) noexcept
{
  static const uint16_t bp[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  uint16x8_t m = vdupq_n_u16(k), p = vld1q_u16(bp);
  return vceqq_u16(vandq_u16(m, p), p);
}

__attribute__((always_inline)) static inline uint32x4_t
__expand_mask_u32(uint8_t k) noexcept
{
  static const uint32_t bp[4] = { 1, 2, 4, 8 };
  uint32x4_t m = vdupq_n_u32(k), p = vld1q_u32(bp);
  return vceqq_u32(vandq_u32(m, p), p);
}

__attribute__((always_inline)) static inline uint64x2_t
__expand_mask_u64(uint8_t k) noexcept
{
  static const uint64_t bp[2] = { 1, 2 };
  uint64x2_t m = vdupq_n_u64(k), p = vld1q_u64(bp);
  return vceqq_u64(vandq_u64(m, p), p);
}

__attribute__((always_inline)) static inline float32x4_t
__scalar_f32x4(float32x4_t a, float (*fn)(float)) noexcept
{
  float r[4];
  vst1q_f32(r, a);
  r[0] = fn(r[0]);
  r[1] = fn(r[1]);
  r[2] = fn(r[2]);
  r[3] = fn(r[3]);
  return vld1q_f32(r);
}

__attribute__((always_inline)) static inline float64x2_t
__scalar_f64x2(float64x2_t a, double (*fn)(double)) noexcept
{
  double r[2];
  vst1q_f64(r, a);
  r[0] = fn(r[0]);
  r[1] = fn(r[1]);
  return vld1q_f64(r);
}

__attribute__((always_inline)) static inline float32x4_t
__scalar2_f32x4(float32x4_t a, float32x4_t b, float (*fn)(float, float)) noexcept
{
  float ra[4], rb[4];
  vst1q_f32(ra, a);
  vst1q_f32(rb, b);
  float rc[4] = { fn(ra[0], rb[0]), fn(ra[1], rb[1]), fn(ra[2], rb[2]), fn(ra[3], rb[3]) };
  return vld1q_f32(rc);
}

__attribute__((always_inline)) static inline float64x2_t
__scalar2_f64x2(float64x2_t a, float64x2_t b, double (*fn)(double, double)) noexcept
{
  double ra[2], rb[2];
  vst1q_f64(ra, a);
  vst1q_f64(rb, b);
  double rc[2] = { fn(ra[0], rb[0]), fn(ra[1], rb[1]) };
  return vld1q_f64(rc);
}

inline i128
abs_8(i128 &o)
{
  return _ri8r(vabsq_s8(_ri8(o)));
}

inline i128
abs_16(i128 &o)
{
  return _ri16r(vabsq_s16(_ri16(o)));
}

inline i128
abs_32(i128 &o)
{
  return vabsq_s32(o);
}

inline i128
abs_64(i128 &o)
{
  return _ri64r(vabsq_s64(_ri64(o)));
}

inline i128
abs(i128 &o)
{
  return vabsq_s32(o);
}

template <typename M>
inline i128
mask_abs_32(i128 src, M k, i128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vbslq_u32(msk, _ru32(vabsq_s32(o)), _ru32(src)));
}

template <typename M>
inline i128
maskz_abs_32(M k, i128 &o)
{
  return _ru32r(vandq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32(vabsq_s32(o))));
}

template <typename M>
inline i128
mask_abs_64(i128 src, M k, i128 &o)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(_ri64r(vabsq_s64(_ri64(o)))), _ru64(src)));
}

template <typename M>
inline i128
maskz_abs_64(M k, i128 &o)
{
  return _ru64r(vandq_u64(__expand_mask_u64(static_cast<uint8_t>(k)), _ru64(_ri64r(vabsq_s64(_ri64(o))))));
}

inline f128
sqrt(f128 &o)
{
  return vsqrtq_f32(o);
}

inline d128
sqrt(d128 &o)
{
  return vsqrtq_f64(o);
}

inline f128
sqrt_ss(f128 a)
{
  return vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(a, 0)), a, 0);
}

inline d128
sqrt_sd(d128 a, d128 b)
{
  return vsetq_lane_f64(math::sqrt(vgetq_lane_f64(b, 0)), a, 0);
}

inline f128
sqrt_round(f128 &o, int /*r*/)
{
  return vsqrtq_f32(o);
}

inline d128
sqrt_round(d128 &o, int /*r*/)
{
  return vsqrtq_f64(o);
}

inline f128
sqrt_round_ss(f128 /*src*/, f128 a, f128 b, int /*r*/)
{
  return vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

inline d128
sqrt_round_sd(d128 /*src*/, d128 a, d128 b, int /*r*/)
{
  return vsetq_lane_f64(math::sqrt(vgetq_lane_f64(b, 0)), a, 0);
}

template <typename M>
inline f128
mask_sqrt(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(vsqrtq_f32(o)), _ru32f(src)));
}

template <typename M>
inline f128
maskz_sqrt(M k, f128 &o)
{
  return _rf32u(vandq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32f(vsqrtq_f32(o))));
}

template <typename M>
inline d128
mask_sqrt(d128 src, M k, d128 &o)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _rf64u(vbslq_u64(msk, _ru64d(vsqrtq_f64(o)), _ru64d(src)));
}

template <typename M>
inline d128
maskz_sqrt(M k, d128 &o)
{
  return _rf64u(vandq_u64(__expand_mask_u64(static_cast<uint8_t>(k)), _ru64d(vsqrtq_f64(o))));
}

template <typename M>
inline f128
mask_sqrt_ss(f128 src, M k, f128 a, f128 b)
{
  if ( !(k & 1) ) return src;
  return vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

template <typename M>
inline f128
maskz_sqrt_ss(M k, f128 a, f128 b)
{
  if ( !(k & 1) ) return vdupq_n_f32(0.f);
  return vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

template <typename M>
inline d128
mask_sqrt_sd(d128 src, M k, d128 a, d128 b)
{
  if ( !(k & 1) ) return src;
  return vsetq_lane_f64(math::sqrt(vgetq_lane_f64(b, 0)), a, 0);
}

template <typename M>
inline d128
maskz_sqrt_sd(M k, d128 a, d128 b)
{
  if ( !(k & 1) ) return vdupq_n_f64(0.0);
  return vsetq_lane_f64(math::sqrt(vgetq_lane_f64(b, 0)), a, 0);
}

__attribute__((always_inline)) static inline float32x4_t
__neon_rsqrt_ps(float32x4_t a) noexcept
{
  float32x4_t x = vrsqrteq_f32(a);
  x = vmulq_f32(x, vrsqrtsq_f32(vmulq_f32(a, x), x));
  return x;
}

__attribute__((always_inline)) static inline float32x4_t
__neon_rsqrt14_ps(float32x4_t a) noexcept
{
  float32x4_t x = vrsqrteq_f32(a);
  x = vmulq_f32(x, vrsqrtsq_f32(vmulq_f32(a, x), x));
  x = vmulq_f32(x, vrsqrtsq_f32(vmulq_f32(a, x), x));
  return x;
}

inline f128
rsqrt_ps(f128 &o)
{
  return __neon_rsqrt_ps(o);
}

inline f128
rsqrt_ss(f128 a)
{
  float v = vgetq_lane_f32(a, 0);
  return vsetq_lane_f32(1.f / math::sqrtf(v), a, 0);
}

inline f128
rsqrt14(f128 &o)
{
  return __neon_rsqrt14_ps(o);
}

inline d128
rsqrt14(d128 &o)
{

  return __scalar_f64x2(o, [](double x) { return 1.0 / math::sqrt(x); });
}

inline f128
rsqrt14_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(1.f / math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

inline d128
rsqrt14_sd(d128 a, d128 b)
{
  return vsetq_lane_f64(1.0 / math::sqrt(vgetq_lane_f64(b, 0)), a, 0);
}

template <typename M>
inline f128
mask_rsqrt14_ss(f128 src, M k, f128 a, f128 b)
{
  if ( !(k & 1) ) return src;
  return vsetq_lane_f32(1.f / math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

template <typename M>
inline f128
maskz_rsqrt14_ss(M k, f128 a, f128 b)
{
  if ( !(k & 1) ) return vdupq_n_f32(0.f);
  return vsetq_lane_f32(1.f / math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

template <typename M>
inline d128
mask_rsqrt14_sd(d128 src, M k, d128 a, d128 b)
{
  if ( !(k & 1) ) return src;
  return vsetq_lane_f64(1.0 / math::sqrt(vgetq_lane_f64(b, 0)), a, 0);
}

template <typename M>
inline d128
maskz_rsqrt14_sd(M k, d128 a, d128 b)
{
  if ( !(k & 1) ) return vdupq_n_f64(0.0);
  return vsetq_lane_f64(1.0 / math::sqrt(vgetq_lane_f64(b, 0)), a, 0);
}

__attribute__((always_inline)) static inline float32x4_t
__neon_rcp_ps(float32x4_t a) noexcept
{
  float32x4_t x = vrecpeq_f32(a);
  x = vmulq_f32(x, vrecpsq_f32(a, x));
  return x;
}

__attribute__((always_inline)) static inline float32x4_t
__neon_rcp14_ps(float32x4_t a) noexcept
{
  float32x4_t x = vrecpeq_f32(a);
  x = vmulq_f32(x, vrecpsq_f32(a, x));
  x = vmulq_f32(x, vrecpsq_f32(a, x));
  return x;
}

inline f128
rcp_ps(f128 &o)
{
  return __neon_rcp_ps(o);
}

inline f128
rcp_ss(f128 a)
{
  return vsetq_lane_f32(1.f / vgetq_lane_f32(a, 0), a, 0);
}

inline f128
rcp14(f128 &o)
{
  return __neon_rcp14_ps(o);
}

inline d128
rcp14(d128 &o)
{
  return __scalar_f64x2(o, [](double x) { return 1.0 / x; });
}

inline f128
rcp14_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(1.f / vgetq_lane_f32(b, 0), a, 0);
}

inline d128
rcp14_sd(d128 a, d128 b)
{
  return vsetq_lane_f64(1.0 / vgetq_lane_f64(b, 0), a, 0);
}

template <typename M>
inline f128
mask_rcp14_ss(f128 src, M k, f128 a, f128 b)
{
  if ( !(k & 1) ) return src;
  return vsetq_lane_f32(1.f / vgetq_lane_f32(b, 0), a, 0);
}

template <typename M>
inline f128
maskz_rcp14_ss(M k, f128 a, f128 b)
{
  if ( !(k & 1) ) return vdupq_n_f32(0.f);
  return vsetq_lane_f32(1.f / vgetq_lane_f32(b, 0), a, 0);
}

template <typename M>
inline d128
mask_rcp14_sd(d128 src, M k, d128 a, d128 b)
{
  if ( !(k & 1) ) return src;
  return vsetq_lane_f64(1.0 / vgetq_lane_f64(b, 0), a, 0);
}

template <typename M>
inline d128
maskz_rcp14_sd(M k, d128 a, d128 b)
{
  if ( !(k & 1) ) return vdupq_n_f64(0.0);
  return vsetq_lane_f64(1.0 / vgetq_lane_f64(b, 0), a, 0);
}

inline f128
min(f128 &o, f128 &b)
{
  return vminq_f32(o, b);
}

inline d128
min(d128 &o, d128 &b)
{
  return vminq_f64(o, b);
}

inline f128
max(f128 &o, f128 &b)
{
  return vmaxq_f32(o, b);
}

inline d128
max(d128 &o, d128 &b)
{
  return vmaxq_f64(o, b);
}

inline f128
min_ss(f128 a, f128 b)
{
  float va = vgetq_lane_f32(a, 0), vb = vgetq_lane_f32(b, 0);
  return vsetq_lane_f32(va < vb ? va : vb, a, 0);
}

inline d128
min_sd(d128 a, d128 b)
{
  double va = vgetq_lane_f64(a, 0), vb = vgetq_lane_f64(b, 0);
  return vsetq_lane_f64(va < vb ? va : vb, a, 0);
}

inline f128
max_ss(f128 a, f128 b)
{
  float va = vgetq_lane_f32(a, 0), vb = vgetq_lane_f32(b, 0);
  return vsetq_lane_f32(va > vb ? va : vb, a, 0);
}

inline d128
max_sd(d128 a, d128 b)
{
  double va = vgetq_lane_f64(a, 0), vb = vgetq_lane_f64(b, 0);
  return vsetq_lane_f64(va > vb ? va : vb, a, 0);
}

inline f128
min_round(f128 &o, f128 &b, int /*sae*/)
{
  return vminq_f32(o, b);
}

inline d128
min_round(d128 &o, d128 &b, int /*sae*/)
{
  return vminq_f64(o, b);
}

inline f128
max_round(f128 &o, f128 &b, int /*sae*/)
{
  return vmaxq_f32(o, b);
}

inline d128
max_round(d128 &o, d128 &b, int /*sae*/)
{
  return vmaxq_f64(o, b);
}

inline f128
min_round_ss(f128 a, f128 b, int)
{
  return min_ss(a, b);
}

inline d128
min_round_sd(d128 a, d128 b, int)
{
  return min_sd(a, b);
}

inline f128
max_round_ss(f128 a, f128 b, int)
{
  return max_ss(a, b);
}

inline d128
max_round_sd(d128 a, d128 b, int)
{
  return max_sd(a, b);
}

template <typename M>
inline f128
mask_min_ss(f128 src, M k, f128 a, f128 b)
{
  return (k & 1) ? min_ss(a, b) : src;
}

template <typename M>
inline f128
maskz_min_ss(M k, f128 a, f128 b)
{
  return (k & 1) ? min_ss(a, b) : vdupq_n_f32(0.f);
}

template <typename M>
inline d128
mask_min_sd(d128 src, M k, d128 a, d128 b)
{
  return (k & 1) ? min_sd(a, b) : src;
}

template <typename M>
inline d128
maskz_min_sd(M k, d128 a, d128 b)
{
  return (k & 1) ? min_sd(a, b) : vdupq_n_f64(0.0);
}

template <typename M>
inline f128
mask_max_ss(f128 src, M k, f128 a, f128 b)
{
  return (k & 1) ? max_ss(a, b) : src;
}

template <typename M>
inline f128
maskz_max_ss(M k, f128 a, f128 b)
{
  return (k & 1) ? max_ss(a, b) : vdupq_n_f32(0.f);
}

template <typename M>
inline d128
mask_max_sd(d128 src, M k, d128 a, d128 b)
{
  return (k & 1) ? max_sd(a, b) : src;
}

template <typename M>
inline d128
maskz_max_sd(M k, d128 a, d128 b)
{
  return (k & 1) ? max_sd(a, b) : vdupq_n_f64(0.0);
}

template <typename M>
inline f128
mask_min_round_ss(f128 src, M k, f128 a, f128 b, int)
{
  return mask_min_ss(src, k, a, b);
}

template <typename M>
inline f128
maskz_min_round_ss(M k, f128 a, f128 b, int)
{
  return maskz_min_ss(k, a, b);
}

template <typename M>
inline d128
mask_min_round_sd(d128 src, M k, d128 a, d128 b, int)
{
  return mask_min_sd(src, k, a, b);
}

template <typename M>
inline d128
maskz_min_round_sd(M k, d128 a, d128 b, int)
{
  return maskz_min_sd(k, a, b);
}

template <typename M>
inline f128
mask_max_round_ss(f128 src, M k, f128 a, f128 b, int)
{
  return mask_max_ss(src, k, a, b);
}

template <typename M>
inline f128
maskz_max_round_ss(M k, f128 a, f128 b, int)
{
  return maskz_max_ss(k, a, b);
}

template <typename M>
inline d128
mask_max_round_sd(d128 src, M k, d128 a, d128 b, int)
{
  return mask_max_sd(src, k, a, b);
}

template <typename M>
inline d128
maskz_max_round_sd(M k, d128 a, d128 b, int)
{
  return maskz_max_sd(k, a, b);
}

inline i128
min_i8(i128 &o, i128 &b)
{
  return _ri8r(vminq_s8(_ri8(o), _ri8(b)));
}

inline i128
min_i16(i128 &o, i128 &b)
{
  return _ri16r(vminq_s16(_ri16(o), _ri16(b)));
}

inline i128
min_i32(i128 &o, i128 &b)
{
  return vminq_s32(o, b);
}

inline i128
max_i8(i128 &o, i128 &b)
{
  return _ri8r(vmaxq_s8(_ri8(o), _ri8(b)));
}

inline i128
max_i16(i128 &o, i128 &b)
{
  return _ri16r(vmaxq_s16(_ri16(o), _ri16(b)));
}

inline i128
max_i32(i128 &o, i128 &b)
{
  return vmaxq_s32(o, b);
}

inline i128
min_i64(i128 &o, i128 &b)
{
  uint64x2_t gt = vcgtq_s64(_ri64(o), _ri64(b));
  return _ru64r(vbslq_u64(gt, _ru64(b), _ru64(o)));
}

inline i128
max_i64(i128 &o, i128 &b)
{
  uint64x2_t gt = vcgtq_s64(_ri64(o), _ri64(b));
  return _ru64r(vbslq_u64(gt, _ru64(o), _ru64(b)));
}

inline i128
min_u8(i128 &o, i128 &b)
{
  return _ru8r(vminq_u8(_ru8(o), _ru8(b)));
}

inline i128
min_u16(i128 &o, i128 &b)
{
  return _ru16r(vminq_u16(_ru16(o), _ru16(b)));
}

inline i128
min_u32(i128 &o, i128 &b)
{
  return _ru32r(vminq_u32(_ru32(o), _ru32(b)));
}

inline i128
max_u8(i128 &o, i128 &b)
{
  return _ru8r(vmaxq_u8(_ru8(o), _ru8(b)));
}

inline i128
max_u16(i128 &o, i128 &b)
{
  return _ru16r(vmaxq_u16(_ru16(o), _ru16(b)));
}

inline i128
max_u32(i128 &o, i128 &b)
{
  return _ru32r(vmaxq_u32(_ru32(o), _ru32(b)));
}

inline i128
min_u64(i128 &o, i128 &b)
{
  uint64x2_t gt = vcgtq_u64(_ru64(o), _ru64(b));
  return _ru64r(vbslq_u64(gt, _ru64(b), _ru64(o)));
}

inline i128
max_u64(i128 &o, i128 &b)
{
  uint64x2_t gt = vcgtq_u64(_ru64(o), _ru64(b));
  return _ru64r(vbslq_u64(gt, _ru64(o), _ru64(b)));
}

inline float
reduce_max_ps(f128 &o)
{
  return vmaxvq_f32(o);
}

inline float
reduce_min_ps(f128 &o)
{
  return vminvq_f32(o);
}

inline double
reduce_max_pd(d128 &o)
{
  double v[2];
  vst1q_f64(v, o);
  return v[0] > v[1] ? v[0] : v[1];
}

inline double
reduce_min_pd(d128 &o)
{
  double v[2];
  vst1q_f64(v, o);
  return v[0] < v[1] ? v[0] : v[1];
}

inline int
reduce_max_i32(i128 &o)
{
  return (int)vmaxvq_s32(o);
}

inline long long
reduce_max_i64(i128 &o)
{
  int64_t v[2];
  vst1q_s64(v, _ri64(o));
  return (long long)(v[0] > v[1] ? v[0] : v[1]);
}

inline unsigned int
reduce_max_u32(i128 &o)
{
  return (unsigned int)vmaxvq_u32(_ru32(o));
}

inline unsigned long long
reduce_max_u64(i128 &o)
{
  uint64_t v[2];
  vst1q_u64(v, _ru64(o));
  return (unsigned long long)(v[0] > v[1] ? v[0] : v[1]);
}

inline short
reduce_max_i16(i128 &o)
{
  return (short)vmaxvq_s16(_ri16(o));
}

inline char
reduce_max_i8(i128 &o)
{
  return (char)vmaxvq_s8(_ri8(o));
}

inline unsigned short
reduce_max_u16(i128 &o)
{
  return (unsigned short)vmaxvq_u16(_ru16(o));
}

inline unsigned char
reduce_max_u8(i128 &o)
{
  return (unsigned char)vmaxvq_u8(_ru8(o));
}

inline int
reduce_min_i32(i128 &o)
{
  return (int)vminvq_s32(o);
}

inline long long
reduce_min_i64(i128 &o)
{
  int64_t v[2];
  vst1q_s64(v, _ri64(o));
  return (long long)(v[0] < v[1] ? v[0] : v[1]);
}

inline unsigned int
reduce_min_u32(i128 &o)
{
  return (unsigned int)vminvq_u32(_ru32(o));
}

inline unsigned long long
reduce_min_u64(i128 &o)
{
  uint64_t v[2];
  vst1q_u64(v, _ru64(o));
  return (unsigned long long)(v[0] < v[1] ? v[0] : v[1]);
}

inline short
reduce_min_i16(i128 &o)
{
  return (short)vminvq_s16(_ri16(o));
}

inline char
reduce_min_i8(i128 &o)
{
  return (char)vminvq_s8(_ri8(o));
}

inline unsigned short
reduce_min_u16(i128 &o)
{
  return (unsigned short)vminvq_u16(_ru16(o));
}

inline unsigned char
reduce_min_u8(i128 &o)
{
  return (unsigned char)vminvq_u8(_ru8(o));
}

template <typename M>
inline float
mask_reduce_max_ps(M k, f128 &o)
{
  f128 masked = _rf32u(vandq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32f(o)));

  uint32x4_t imsk = vmvnq_u32(__expand_mask_u32(static_cast<uint8_t>(k)));
  static const uint32_t neg_inf_bits = 0xFF800000u;
  uint32x4_t fill = vandq_u32(imsk, vdupq_n_u32(neg_inf_bits));
  return vmaxvq_f32(vorrq_f32(masked, _rf32u(fill)));
}

template <typename M>
inline float
mask_reduce_min_ps(M k, f128 &o)
{
  uint32x4_t imsk = vmvnq_u32(__expand_mask_u32(static_cast<uint8_t>(k)));
  static const uint32_t pos_inf_bits = 0x7F800000u;
  uint32x4_t fill = vandq_u32(imsk, vdupq_n_u32(pos_inf_bits));
  f128 masked = vorrq_f32(_rf32u(vandq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32f(o))), _rf32u(fill));
  return vminvq_f32(masked);
}

template <typename M>
inline int
mask_reduce_max_i32(M k, i128 &o)
{
  int32x4_t masked = vbslq_s32(__expand_mask_u32(static_cast<uint8_t>(k)), o, vdupq_n_s32(INT32_MIN));
  return (int)vmaxvq_s32(masked);
}

template <typename M>
inline int
mask_reduce_min_i32(M k, i128 &o)
{
  int32x4_t masked = vbslq_s32(__expand_mask_u32(static_cast<uint8_t>(k)), o, vdupq_n_s32(INT32_MAX));
  return (int)vminvq_s32(masked);
}

template <typename M>
inline unsigned int
mask_reduce_max_u32(M k, i128 &o)
{
  uint32x4_t masked = vbslq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32(o), vdupq_n_u32(0));
  return (unsigned int)vmaxvq_u32(masked);
}

template <typename M>
inline unsigned int
mask_reduce_min_u32(M k, i128 &o)
{
  uint32x4_t masked = vbslq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32(o), vdupq_n_u32(UINT32_MAX));
  return (unsigned int)vminvq_u32(masked);
}

template <typename M>
inline short
mask_reduce_max_i16(M k, i128 &o)
{
  int16x8_t masked = vbslq_s16(__expand_mask_u16(static_cast<uint8_t>(k)), _ri16(o), vdupq_n_s16(INT16_MIN));
  return (short)vmaxvq_s16(masked);
}

template <typename M>
inline short
mask_reduce_min_i16(M k, i128 &o)
{
  int16x8_t masked = vbslq_s16(__expand_mask_u16(static_cast<uint8_t>(k)), _ri16(o), vdupq_n_s16(INT16_MAX));
  return (short)vminvq_s16(masked);
}

template <typename M>
inline char
mask_reduce_max_i8(M k, i128 &o)
{
  int8x16_t masked = vbslq_s8(__expand_mask_u16(static_cast<uint8_t>(k)), _ri8(o), vdupq_n_s8(INT8_MIN));
  return (char)vmaxvq_s8(masked);
}

template <typename M>
inline char
mask_reduce_min_i8(M k, i128 &o)
{
  int8x16_t masked = vbslq_s8(__expand_mask_u16(static_cast<uint8_t>(k)), _ri8(o), vdupq_n_s8(INT8_MAX));
  return (char)vminvq_s8(masked);
}

template <typename M>
inline unsigned short
mask_reduce_max_u16(M k, i128 &o)
{
  uint16x8_t masked = vbslq_u16(__expand_mask_u16(static_cast<uint8_t>(k)), _ru16(o), vdupq_n_u16(0));
  return (unsigned short)vmaxvq_u16(masked);
}

template <typename M>
inline unsigned short
mask_reduce_min_u16(M k, i128 &o)
{
  uint16x8_t masked = vbslq_u16(__expand_mask_u16(static_cast<uint8_t>(k)), _ru16(o), vdupq_n_u16(UINT16_MAX));
  return (unsigned short)vminvq_u16(masked);
}

template <typename M>
inline unsigned char
mask_reduce_max_u8(M k, i128 &o)
{
  uint8x16_t masked = vbslq_u8(__expand_mask_u16(static_cast<uint8_t>(k)), _ru8(o), vdupq_n_u8(0));
  return (unsigned char)vmaxvq_u8(masked);
}

template <typename M>
inline unsigned char
mask_reduce_min_u8(M k, i128 &o)
{
  uint8x16_t masked = vbslq_u8(__expand_mask_u16(static_cast<uint8_t>(k)), _ru8(o), vdupq_n_u8(UINT8_MAX));
  return (unsigned char)vminvq_u8(masked);
}

inline f128
ceil(f128 &o)
{
  return vrndpq_f32(o);
}

inline d128
ceil(d128 &o)
{
  return vrndpq_f64(o);
}

inline f128
floor(f128 &o)
{
  return vrndmq_f32(o);
}

inline d128
floor(d128 &o)
{
  return vrndmq_f64(o);
}

inline f128
ceil_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(math::ceilf(vgetq_lane_f32(b, 0)), a, 0);
}

inline d128
ceil_sd(d128 a, d128 b)
{
  return vsetq_lane_f64(math::ceil(vgetq_lane_f64(b, 0)), a, 0);
}

inline f128
floor_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(math::floorf(vgetq_lane_f32(b, 0)), a, 0);
}

inline d128
floor_sd(d128 a, d128 b)
{
  return vsetq_lane_f64(math::floor(vgetq_lane_f64(b, 0)), a, 0);
}

inline f128
round(f128 &o, int rounding)
{
  switch ( rounding & 0x7 ) {
  case 0 :
    return vrndnq_f32(o);
  case 1 :
    return vrndmq_f32(o);
  case 2 :
    return vrndpq_f32(o);
  case 3 :
    return vrndq_f32(o);
  case 4 :
    return vrndiq_f32(o);
  default :
    return vrndnq_f32(o);
  }
}

inline d128
round(d128 &o, int rounding)
{
  switch ( rounding & 0x7 ) {
  case 0 :
    return vrndnq_f64(o);
  case 1 :
    return vrndmq_f64(o);
  case 2 :
    return vrndpq_f64(o);
  case 3 :
    return vrndq_f64(o);
  case 4 :
    return vrndiq_f64(o);
  default :
    return vrndnq_f64(o);
  }
}

inline f128
round_ss(f128 a, f128 b, int r)
{
  float val;
  switch ( r & 0x7 ) {
  case 0 :
    val = math::roundf(vgetq_lane_f32(b, 0));
    break;
  case 1 :
    val = math::floorf(vgetq_lane_f32(b, 0));
    break;
  case 2 :
    val = math::ceilf(vgetq_lane_f32(b, 0));
    break;
  case 3 :
    val = math::truncf(vgetq_lane_f32(b, 0));
    break;
  default :
    val = math::roundf(vgetq_lane_f32(b, 0));
    break;
  }
  return vsetq_lane_f32(val, a, 0);
}

inline d128
round_sd(d128 a, d128 b, int r)
{
  double val;
  switch ( r & 0x7 ) {
  case 0 :
    val = math::round(vgetq_lane_f64(b, 0));
    break;
  case 1 :
    val = math::floor(vgetq_lane_f64(b, 0));
    break;
  case 2 :
    val = math::ceil(vgetq_lane_f64(b, 0));
    break;
  case 3 :
    val = math::trunc(vgetq_lane_f64(b, 0));
    break;
  default :
    val = math::round(vgetq_lane_f64(b, 0));
    break;
  }
  return vsetq_lane_f64(val, a, 0);
}

inline f128
trunc(f128 &o)
{
  return vrndq_f32(o);
}

inline d128
trunc(d128 &o)
{
  return vrndq_f64(o);
}

inline f128
nearbyint(f128 &o)
{
  return vrndnq_f32(o);
}

inline d128
nearbyint(d128 &o)
{
  return vrndnq_f64(o);
}

inline f128
rint(f128 &o)
{
  return vrndiq_f32(o);
}

inline d128
rint(d128 &o)
{
  return vrndiq_f64(o);
}

inline f128
exp(f128 &o)
{
  return __scalar_f32x4(o, math::expf);
}

inline d128
exp(d128 &o)
{
  return __scalar_f64x2(o, math::exp);
}

inline f128
exp2(f128 &o)
{
  return __scalar_f32x4(o, math::exp2f);
}

inline d128
exp2(d128 &o)
{
  return __scalar_f64x2(o, math::exp2);
}

inline f128
exp10(f128 &o)
{
  return __scalar_f32x4(o, [](float x) { return math::powf(10.f, x); });
}

inline d128
exp10(d128 &o)
{
  return __scalar_f64x2(o, [](double x) { return math::pow(10.0, x); });
}

inline f128
expm1(f128 &o)
{
  return __scalar_f32x4(o, math::expm1f);
}

inline d128
expm1(d128 &o)
{
  return __scalar_f64x2(o, math::expm1);
}

inline f128
log(f128 &o)
{
  return __scalar_f32x4(o, math::logf);
}

inline d128
log(d128 &o)
{
  return __scalar_f64x2(o, math::log);
}

inline f128
log2(f128 &o)
{
  return __scalar_f32x4(o, math::log2f);
}

inline d128
log2(d128 &o)
{
  return __scalar_f64x2(o, math::log2);
}

inline f128
log10(f128 &o)
{
  return __scalar_f32x4(o, math::log10f);
}

inline d128
log10(d128 &o)
{
  return __scalar_f64x2(o, math::log10);
}

inline f128
log1p(f128 &o)
{
  return __scalar_f32x4(o, math::log1pf);
}

inline d128
log1p(d128 &o)
{
  return __scalar_f64x2(o, math::log1p);
}

inline f128
logb(f128 &o)
{
  return __scalar_f32x4(o, math::logbf);
}

inline d128
logb(d128 &o)
{
  return __scalar_f64x2(o, math::logb);
}

inline f128
pow(f128 &o, f128 &b)
{
  return __scalar2_f32x4(o, b, math::powf);
}

inline d128
pow(d128 &o, d128 &b)
{
  return __scalar2_f64x2(o, b, math::pow);
}

inline f128
cbrt(f128 &o)
{
  return __scalar_f32x4(o, math::cbrtf);
}

inline d128
cbrt(d128 &o)
{
  return __scalar_f64x2(o, math::cbrt);
}

inline f128
invcbrt(f128 &o)
{
  return __scalar_f32x4(o, [](float x) { return 1.f / math::cbrtf(x); });
}

inline d128
invcbrt(d128 &o)
{
  return __scalar_f64x2(o, [](double x) { return 1.0 / math::cbrt(x); });
}

inline f128
invsqrt(f128 &o)
{
  return __scalar_f32x4(o, [](float x) { return 1.f / math::sqrtf(x); });
}

inline d128
invsqrt(d128 &o)
{
  return __scalar_f64x2(o, [](double x) { return 1.0 / math::sqrt(x); });
}

inline f128
hypot(f128 &o, f128 &b)
{
  return __scalar2_f32x4(o, b, math::hypotf);
}

inline d128
hypot(d128 &o, d128 &b)
{
  return __scalar2_f64x2(o, b, math::hypot);
}

template <typename M>
inline f128
mask_exp(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(exp(o)), _ru32f(src)));
}

template <typename M>
inline d128
mask_exp(d128 src, M k, d128 &o)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _rf64u(vbslq_u64(msk, _ru64d(exp(o)), _ru64d(src)));
}

template <typename M>
inline f128
mask_exp2(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(exp2(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_exp10(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(exp10(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_expm1(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(expm1(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_log(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(log(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_log2(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(log2(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_log10(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(log10(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_log1p(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(log1p(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_logb(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(logb(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_pow(f128 src, M k, f128 &o, f128 &b)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(pow(o, b)), _ru32f(src)));
}

template <typename M>
inline f128
mask_cbrt(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(cbrt(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_invsqrt(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(invsqrt(o)), _ru32f(src)));
}

template <typename M>
inline f128
mask_hypot(f128 src, M k, f128 &o, f128 &b)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(hypot(o, b)), _ru32f(src)));
}

inline f128
svml_sqrt(f128 &o)
{
  return vsqrtq_f32(o);
}

inline d128
svml_sqrt(d128 &o)
{
  return vsqrtq_f64(o);
}

inline f128
svml_ceil(f128 &o)
{
  return vrndpq_f32(o);
}

inline d128
svml_ceil(d128 &o)
{
  return vrndpq_f64(o);
}

inline f128
svml_floor(f128 &o)
{
  return vrndmq_f32(o);
}

inline d128
svml_floor(d128 &o)
{
  return vrndmq_f64(o);
}

inline f128
svml_round(f128 &o)
{
  return vrndnq_f32(o);
}

inline d128
svml_round(d128 &o)
{
  return vrndnq_f64(o);
}

#undef _ri8
#undef _ri16
#undef _ri32
#undef _ri64
#undef _ru8
#undef _ru16
#undef _ru32
#undef _ru64
#undef _ri8r
#undef _ri16r
#undef _ri64r
#undef _ru8r
#undef _ru16r
#undef _ru32r
#undef _ru64r
#undef _rf32
#undef _rf64
#undef _rf32r
#undef _rf64r
#undef _ru32f
#undef _ru64d
#undef _rf32u
#undef _rf64u

#pragma GCC diagnostic pop

};     // namespace simd
};     // namespace micron
