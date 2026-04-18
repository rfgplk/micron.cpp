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
#define _rf32r(x) vreinterpretq_s32_f32(x)
#define _ru32f(x) vreinterpretq_u32_f32(x)
#define _rf32u(x) vreinterpretq_f32_u32(x)

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
  uint64_t lanes[2] = { (k & 1) ? UINT64_MAX : 0ULL, (k & 2) ? UINT64_MAX : 0ULL };
  return vld1q_u64(lanes);
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

__attribute__((always_inline)) static inline float32x4_t
__scalar2_f32x4(float32x4_t a, float32x4_t b, float (*fn)(float, float)) noexcept
{
  float ra[4], rb[4];
  vst1q_f32(ra, a);
  vst1q_f32(rb, b);
  float rc[4] = { fn(ra[0], rb[0]), fn(ra[1], rb[1]), fn(ra[2], rb[2]), fn(ra[3], rb[3]) };
  return vld1q_f32(rc);
}

__attribute__((always_inline)) static inline float32x4_t
__neon_v7_sqrtq_f32(float32x4_t a) noexcept
{
  float32x4_t r = vrsqrteq_f32(a);
  r = vmulq_f32(r, vrsqrtsq_f32(vmulq_f32(a, r), r));
  r = vmulq_f32(r, vrsqrtsq_f32(vmulq_f32(a, r), r));
  float32x4_t result = vmulq_f32(a, r);
  uint32x4_t zero = vceqq_f32(a, vdupq_n_f32(0.f));
  return vbslq_f32(zero, vdupq_n_f32(0.f), result);
}

__attribute__((always_inline)) static inline int64x2_t
__neon_v7_absq_s64(int64x2_t a) noexcept
{
  int64x2_t mask = vshrq_n_s64(a, 63);
  return vsubq_s64(veorq_s64(a, mask), mask);
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
  return _ri64r(__neon_v7_absq_s64(_ri64(o)));
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
  return _ru64r(vbslq_u64(msk, vreinterpretq_u64_s64(__neon_v7_absq_s64(_ri64(o))), _ru64(src)));
}

template <typename M>
inline i128
maskz_abs_64(M k, i128 &o)
{
  return _ru64r(vandq_u64(__expand_mask_u64(static_cast<uint8_t>(k)), vreinterpretq_u64_s64(__neon_v7_absq_s64(_ri64(o)))));
}

inline f128
sqrt(f128 &o)
{
  return __neon_v7_sqrtq_f32(o);
}

inline f128
sqrt_ss(f128 a)
{
  return vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(a, 0)), a, 0);
}

inline f128
sqrt_round(f128 &o, int /*r*/)
{
  return __neon_v7_sqrtq_f32(o);
}

inline f128
sqrt_round_ss(f128 /*src*/, f128 a, f128 b, int /*r*/)
{
  return vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

template <typename M>
inline f128
mask_sqrt(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(__neon_v7_sqrtq_f32(o)), _ru32f(src)));
}

template <typename M>
inline f128
maskz_sqrt(M k, f128 &o)
{
  return _rf32u(vandq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32f(__neon_v7_sqrtq_f32(o))));
}

template <typename M>
inline f128
mask_sqrt_ss(f128 src, M k, f128 a, f128 b)
{
  return (k & 1) ? vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(b, 0)), a, 0) : src;
}

template <typename M>
inline f128
maskz_sqrt_ss(M k, f128 a, f128 b)
{
  return (k & 1) ? vsetq_lane_f32(math::sqrtf(vgetq_lane_f32(b, 0)), a, 0) : vdupq_n_f32(0.f);
}

__attribute__((always_inline)) static inline float32x4_t
__neon_rsqrt_ps(float32x4_t a) noexcept
{
  float32x4_t x = vrsqrteq_f32(a);
  return vmulq_f32(x, vrsqrtsq_f32(vmulq_f32(a, x), x));
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
rsqrt14(f128 &o)
{
  return __neon_rsqrt14_ps(o);
}

inline f128
rsqrt_ss(f128 a)
{
  return vsetq_lane_f32(1.f / math::sqrtf(vgetq_lane_f32(a, 0)), a, 0);
}

inline f128
rsqrt14_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(1.f / math::sqrtf(vgetq_lane_f32(b, 0)), a, 0);
}

template <typename M>
inline f128
mask_rsqrt14_ss(f128 src, M k, f128 a, f128 b)
{
  return (k & 1) ? rsqrt14_ss(a, b) : src;
}

template <typename M>
inline f128
maskz_rsqrt14_ss(M k, f128 a, f128 b)
{
  return (k & 1) ? rsqrt14_ss(a, b) : vdupq_n_f32(0.f);
}

__attribute__((always_inline)) static inline float32x4_t
__neon_rcp_ps(float32x4_t a) noexcept
{
  float32x4_t x = vrecpeq_f32(a);
  return vmulq_f32(x, vrecpsq_f32(a, x));
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
rcp14(f128 &o)
{
  return __neon_rcp14_ps(o);
}

inline f128
rcp_ss(f128 a)
{
  return vsetq_lane_f32(1.f / vgetq_lane_f32(a, 0), a, 0);
}

inline f128
rcp14_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(1.f / vgetq_lane_f32(b, 0), a, 0);
}

template <typename M>
inline f128
mask_rcp14_ss(f128 src, M k, f128 a, f128 b)
{
  return (k & 1) ? rcp14_ss(a, b) : src;
}

template <typename M>
inline f128
maskz_rcp14_ss(M k, f128 a, f128 b)
{
  return (k & 1) ? rcp14_ss(a, b) : vdupq_n_f32(0.f);
}

inline f128
min(f128 &o, f128 &b)
{
  return vminq_f32(o, b);
}

inline f128
max(f128 &o, f128 &b)
{
  return vmaxq_f32(o, b);
}

inline f128
min_ss(f128 a, f128 b)
{
  float va = vgetq_lane_f32(a, 0), vb = vgetq_lane_f32(b, 0);
  return vsetq_lane_f32(va < vb ? va : vb, a, 0);
}

inline f128
max_ss(f128 a, f128 b)
{
  float va = vgetq_lane_f32(a, 0), vb = vgetq_lane_f32(b, 0);
  return vsetq_lane_f32(va > vb ? va : vb, a, 0);
}

inline f128
min_round(f128 &o, f128 &b, int)
{
  return vminq_f32(o, b);
}

inline f128
max_round(f128 &o, f128 &b, int)
{
  return vmaxq_f32(o, b);
}

inline f128
min_round_ss(f128 a, f128 b, int)
{
  return min_ss(a, b);
}

inline f128
max_round_ss(f128 a, f128 b, int)
{
  return max_ss(a, b);
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
  uint64x2_t gt = vreinterpretq_u64_s64(neon_v7_cgtq_s64(_ri64(o), _ri64(b)));
  return _ru64r(vbslq_u64(gt, _ru64(b), _ru64(o)));
}

inline i128
max_i64(i128 &o, i128 &b)
{
  uint64x2_t gt = vreinterpretq_u64_s64(neon_v7_cgtq_s64(_ri64(o), _ri64(b)));
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
  uint64_t a0 = vgetq_lane_u64(_ru64(o), 0), a1 = vgetq_lane_u64(_ru64(o), 1);
  uint64_t b0 = vgetq_lane_u64(_ru64(b), 0), b1 = vgetq_lane_u64(_ru64(b), 1);
  uint64_t r[2] = { a0 < b0 ? a0 : b0, a1 < b1 ? a1 : b1 };
  return _ru64r(vld1q_u64(r));
}

inline i128
max_u64(i128 &o, i128 &b)
{
  uint64_t a0 = vgetq_lane_u64(_ru64(o), 0), a1 = vgetq_lane_u64(_ru64(o), 1);
  uint64_t b0 = vgetq_lane_u64(_ru64(b), 0), b1 = vgetq_lane_u64(_ru64(b), 1);
  uint64_t r[2] = { a0 > b0 ? a0 : b0, a1 > b1 ? a1 : b1 };
  return _ru64r(vld1q_u64(r));
}

inline float
reduce_max_ps(f128 &o)
{
  float32x2_t lo = vget_low_f32(o), hi = vget_high_f32(o);
  float32x2_t m = vpmax_f32(lo, hi);
  m = vpmax_f32(m, m);
  return vget_lane_f32(m, 0);
}

inline float
reduce_min_ps(f128 &o)
{
  float32x2_t lo = vget_low_f32(o), hi = vget_high_f32(o);
  float32x2_t m = vpmin_f32(lo, hi);
  m = vpmin_f32(m, m);
  return vget_lane_f32(m, 0);
}

inline int
reduce_max_i32(i128 &o)
{
  int32x2_t lo = vget_low_s32(o), hi = vget_high_s32(o);
  int32x2_t m = vpmax_s32(lo, hi);
  m = vpmax_s32(m, m);
  return (int)vget_lane_s32(m, 0);
}

inline int
reduce_min_i32(i128 &o)
{
  int32x2_t lo = vget_low_s32(o), hi = vget_high_s32(o);
  int32x2_t m = vpmin_s32(lo, hi);
  m = vpmin_s32(m, m);
  return (int)vget_lane_s32(m, 0);
}

inline long long
reduce_max_i64(i128 &o)
{
  int64_t v[2];
  vst1q_s64(v, _ri64(o));
  return (long long)(v[0] > v[1] ? v[0] : v[1]);
}

inline long long
reduce_min_i64(i128 &o)
{
  int64_t v[2];
  vst1q_s64(v, _ri64(o));
  return (long long)(v[0] < v[1] ? v[0] : v[1]);
}

inline unsigned int
reduce_max_u32(i128 &o)
{
  uint32x2_t lo = vget_low_u32(_ru32(o)), hi = vget_high_u32(_ru32(o));
  uint32x2_t m = vpmax_u32(lo, hi);
  m = vpmax_u32(m, m);
  return (unsigned int)vget_lane_u32(m, 0);
}

inline unsigned int
reduce_min_u32(i128 &o)
{
  uint32x2_t lo = vget_low_u32(_ru32(o)), hi = vget_high_u32(_ru32(o));
  uint32x2_t m = vpmin_u32(lo, hi);
  m = vpmin_u32(m, m);
  return (unsigned int)vget_lane_u32(m, 0);
}

inline unsigned long long
reduce_max_u64(i128 &o)
{
  uint64_t v[2];
  vst1q_u64(v, _ru64(o));
  return (unsigned long long)(v[0] > v[1] ? v[0] : v[1]);
}

inline unsigned long long
reduce_min_u64(i128 &o)
{
  uint64_t v[2];
  vst1q_u64(v, _ru64(o));
  return (unsigned long long)(v[0] < v[1] ? v[0] : v[1]);
}

inline short
reduce_max_i16(i128 &o)
{
  int16x4_t lo = vget_low_s16(_ri16(o)), hi = vget_high_s16(_ri16(o));
  int16x4_t m = vpmax_s16(lo, hi);
  m = vpmax_s16(m, m);
  m = vpmax_s16(m, m);
  return (short)vget_lane_s16(m, 0);
}

inline short
reduce_min_i16(i128 &o)
{
  int16x4_t lo = vget_low_s16(_ri16(o)), hi = vget_high_s16(_ri16(o));
  int16x4_t m = vpmin_s16(lo, hi);
  m = vpmin_s16(m, m);
  m = vpmin_s16(m, m);
  return (short)vget_lane_s16(m, 0);
}

inline char
reduce_max_i8(i128 &o)
{
  int8x8_t lo = vget_low_s8(_ri8(o)), hi = vget_high_s8(_ri8(o));
  int8x8_t m = vpmax_s8(lo, hi);
  m = vpmax_s8(m, m);
  m = vpmax_s8(m, m);
  m = vpmax_s8(m, m);
  return (char)vget_lane_s8(m, 0);
}

inline char
reduce_min_i8(i128 &o)
{
  int8x8_t lo = vget_low_s8(_ri8(o)), hi = vget_high_s8(_ri8(o));
  int8x8_t m = vpmin_s8(lo, hi);
  m = vpmin_s8(m, m);
  m = vpmin_s8(m, m);
  m = vpmin_s8(m, m);
  return (char)vget_lane_s8(m, 0);
}

inline unsigned short
reduce_max_u16(i128 &o)
{
  uint16x4_t lo = vget_low_u16(_ru16(o)), hi = vget_high_u16(_ru16(o));
  uint16x4_t m = vpmax_u16(lo, hi);
  m = vpmax_u16(m, m);
  m = vpmax_u16(m, m);
  return (unsigned short)vget_lane_u16(m, 0);
}

inline unsigned short
reduce_min_u16(i128 &o)
{
  uint16x4_t lo = vget_low_u16(_ru16(o)), hi = vget_high_u16(_ru16(o));
  uint16x4_t m = vpmin_u16(lo, hi);
  m = vpmin_u16(m, m);
  m = vpmin_u16(m, m);
  return (unsigned short)vget_lane_u16(m, 0);
}

inline unsigned char
reduce_max_u8(i128 &o)
{
  uint8x8_t lo = vget_low_u8(_ru8(o)), hi = vget_high_u8(_ru8(o));
  uint8x8_t m = vpmax_u8(lo, hi);
  m = vpmax_u8(m, m);
  m = vpmax_u8(m, m);
  m = vpmax_u8(m, m);
  return (unsigned char)vget_lane_u8(m, 0);
}

inline unsigned char
reduce_min_u8(i128 &o)
{
  uint8x8_t lo = vget_low_u8(_ru8(o)), hi = vget_high_u8(_ru8(o));
  uint8x8_t m = vpmin_u8(lo, hi);
  m = vpmin_u8(m, m);
  m = vpmin_u8(m, m);
  m = vpmin_u8(m, m);
  return (unsigned char)vget_lane_u8(m, 0);
}

template <typename M>
inline float
mask_reduce_max_ps(M k, f128 &o)
{
  uint32x4_t imsk = vmvnq_u32(__expand_mask_u32(static_cast<uint8_t>(k)));
  static const uint32_t neg_inf_bits = 0xFF800000u;
  f128 masked = vorrq_f32(_rf32u(vandq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32f(o))),
                          _rf32u(vandq_u32(imsk, vdupq_n_u32(neg_inf_bits))));
  return reduce_max_ps(masked);
}

template <typename M>
inline float
mask_reduce_min_ps(M k, f128 &o)
{
  uint32x4_t imsk = vmvnq_u32(__expand_mask_u32(static_cast<uint8_t>(k)));
  static const uint32_t pos_inf_bits = 0x7F800000u;
  f128 masked = vorrq_f32(_rf32u(vandq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32f(o))),
                          _rf32u(vandq_u32(imsk, vdupq_n_u32(pos_inf_bits))));
  return reduce_min_ps(masked);
}

template <typename M>
inline int
mask_reduce_max_i32(M k, i128 &o)
{
  i128 m = vbslq_s32(__expand_mask_u32(static_cast<uint8_t>(k)), o, vdupq_n_s32(INT32_MIN));
  return reduce_max_i32(m);
}

template <typename M>
inline int
mask_reduce_min_i32(M k, i128 &o)
{
  i128 m = vbslq_s32(__expand_mask_u32(static_cast<uint8_t>(k)), o, vdupq_n_s32(INT32_MAX));
  return reduce_min_i32(m);
}

template <typename M>
inline unsigned int
mask_reduce_max_u32(M k, i128 &o)
{
  i128 m = _ru32r(vbslq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32(o), vdupq_n_u32(0)));
  return reduce_max_u32(m);
}

template <typename M>
inline unsigned int
mask_reduce_min_u32(M k, i128 &o)
{
  i128 m = _ru32r(vbslq_u32(__expand_mask_u32(static_cast<uint8_t>(k)), _ru32(o), vdupq_n_u32(UINT32_MAX)));
  return reduce_min_u32(m);
}

template <typename M>
inline short
mask_reduce_max_i16(M k, i128 &o)
{
  i128 m = _ri16r(vbslq_s16(__expand_mask_u16(static_cast<uint8_t>(k)), _ri16(o), vdupq_n_s16(INT16_MIN)));
  return reduce_max_i16(m);
}

template <typename M>
inline short
mask_reduce_min_i16(M k, i128 &o)
{
  i128 m = _ri16r(vbslq_s16(__expand_mask_u16(static_cast<uint8_t>(k)), _ri16(o), vdupq_n_s16(INT16_MAX)));
  return reduce_min_i16(m);
}

template <typename M>
inline char
mask_reduce_max_i8(M k, i128 &o)
{
  i128 m = _ri8r(vbslq_s8(__expand_mask_u16(static_cast<uint8_t>(k)), _ri8(o), vdupq_n_s8(INT8_MIN)));
  return reduce_max_i8(m);
}

template <typename M>
inline char
mask_reduce_min_i8(M k, i128 &o)
{
  i128 m = _ri8r(vbslq_s8(__expand_mask_u16(static_cast<uint8_t>(k)), _ri8(o), vdupq_n_s8(INT8_MAX)));
  return reduce_min_i8(m);
}

template <typename M>
inline unsigned short
mask_reduce_max_u16(M k, i128 &o)
{
  i128 m = _ru16r(vbslq_u16(__expand_mask_u16(static_cast<uint8_t>(k)), _ru16(o), vdupq_n_u16(0)));
  return reduce_max_u16(m);
}

template <typename M>
inline unsigned short
mask_reduce_min_u16(M k, i128 &o)
{
  i128 m = _ru16r(vbslq_u16(__expand_mask_u16(static_cast<uint8_t>(k)), _ru16(o), vdupq_n_u16(UINT16_MAX)));
  return reduce_min_u16(m);
}

template <typename M>
inline unsigned char
mask_reduce_max_u8(M k, i128 &o)
{
  i128 m = _ru8r(vbslq_u8(__expand_mask_u16(static_cast<uint8_t>(k)), _ru8(o), vdupq_n_u8(0)));
  return reduce_max_u8(m);
}

template <typename M>
inline unsigned char
mask_reduce_min_u8(M k, i128 &o)
{
  i128 m = _ru8r(vbslq_u8(__expand_mask_u16(static_cast<uint8_t>(k)), _ru8(o), vdupq_n_u8(UINT8_MAX)));
  return reduce_min_u8(m);
}

__attribute__((always_inline)) static inline float32x4_t
__v7_roundq(float32x4_t a, float (*fn)(float)) noexcept
{
  float r[4];
  vst1q_f32(r, a);
  r[0] = fn(r[0]);
  r[1] = fn(r[1]);
  r[2] = fn(r[2]);
  r[3] = fn(r[3]);
  return vld1q_f32(r);
}

inline f128
ceil(f128 &o)
{
  return __v7_roundq(o, math::ceilf);
}

inline f128
floor(f128 &o)
{
  return __v7_roundq(o, math::floorf);
}

inline f128
trunc(f128 &o)
{
  return __v7_roundq(o, math::truncf);
}

inline f128
ceil_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(math::ceilf(vgetq_lane_f32(b, 0)), a, 0);
}

inline f128
floor_ss(f128 a, f128 b)
{
  return vsetq_lane_f32(math::floorf(vgetq_lane_f32(b, 0)), a, 0);
}

inline f128
nearbyint(f128 &o)
{
  return __v7_roundq(o, math::nearbyintf);
}

inline f128
rint(f128 &o)
{
  return __v7_roundq(o, math::rintf);
}

inline f128
round(f128 &o, int rounding)
{
  switch ( rounding & 0x7 ) {
  case 0 :
    return __v7_roundq(o, math::roundf);
  case 1 :
    return __v7_roundq(o, math::floorf);
  case 2 :
    return __v7_roundq(o, math::ceilf);
  case 3 :
    return __v7_roundq(o, math::truncf);
  case 4 :
    return __v7_roundq(o, math::nearbyintf);
  default :
    return __v7_roundq(o, math::roundf);
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

inline f128
exp(f128 &o)
{
  return __scalar_f32x4(o, math::expf);
}

inline f128
exp2(f128 &o)
{
  return __scalar_f32x4(o, math::exp2f);
}

inline f128
exp10(f128 &o)
{
  return __scalar_f32x4(o, [](float x) { return math::powf(10.f, x); });
}

inline f128
expm1(f128 &o)
{
  return __scalar_f32x4(o, math::expm1f);
}

inline f128
log(f128 &o)
{
  return __scalar_f32x4(o, math::logf);
}

inline f128
log2(f128 &o)
{
  return __scalar_f32x4(o, math::log2f);
}

inline f128
log10(f128 &o)
{
  return __scalar_f32x4(o, math::log10f);
}

inline f128
log1p(f128 &o)
{
  return __scalar_f32x4(o, math::log1pf);
}

inline f128
logb(f128 &o)
{
  return __scalar_f32x4(o, math::logbf);
}

inline f128
pow(f128 &o, f128 &b)
{
  return __scalar2_f32x4(o, b, math::powf);
}

inline f128
cbrt(f128 &o)
{
  return __scalar_f32x4(o, math::cbrtf);
}

inline f128
invcbrt(f128 &o)
{
  return __scalar_f32x4(o, [](float x) { return 1.f / math::cbrtf(x); });
}

inline f128
invsqrt(f128 &o)
{
  return __scalar_f32x4(o, [](float x) { return 1.f / math::sqrtf(x); });
}

inline f128
hypot(f128 &o, f128 &b)
{
  return __scalar2_f32x4(o, b, math::hypotf);
}

template <typename M>
inline f128
mask_exp(f128 src, M k, f128 &o)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _rf32u(vbslq_u32(msk, _ru32f(exp(o)), _ru32f(src)));
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
  return __neon_v7_sqrtq_f32(o);
}

inline f128
svml_ceil(f128 &o)
{
  return ceil(o);
}

inline f128
svml_floor(f128 &o)
{
  return floor(o);
}

inline f128
svml_round(f128 &o)
{
  return __v7_roundq(o, math::roundf);
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
#undef _rf32r
#undef _ru32f
#undef _rf32u

#pragma GCC diagnostic pop

};     // namespace simd
};     // namespace micron
