//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_arm64) && !defined(__micron_arch_arm32)
#error "neon.hpp included on a non-ARM build"
#endif

namespace micron
{
namespace simd
{
namespace neon
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_neon [[gnu::always_inline, gnu::artificial]] static inline

__inline_neon int8x16_t
load_i8(const signed char *p) noexcept
{
  return vld1q_s8(p);
}

__inline_neon int16x8_t
load_i16(const signed short *p) noexcept
{
  return vld1q_s16(p);
}

__inline_neon int32x4_t
load_i32(const signed int *p) noexcept
{
  return vld1q_s32(p);
}

__inline_neon int64x2_t
load_i64(const i64 *p) noexcept
{
  return vld1q_s64(p);
}

__inline_neon uint8x16_t
load_u8(const unsigned char *p) noexcept
{
  return vld1q_u8(p);
}

__inline_neon uint16x8_t
load_u16(const unsigned short *p) noexcept
{
  return vld1q_u16(p);
}

__inline_neon uint32x4_t
load_u32(const unsigned int *p) noexcept
{
  return vld1q_u32(p);
}

__inline_neon uint64x2_t
load_u64(const u64 *p) noexcept
{
  return vld1q_u64(p);
}

__inline_neon float32x4_t
load_f32(const float *p) noexcept
{
  return vld1q_f32(p);
}
#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
load_f64(const double *p) noexcept
{
  return vld1q_f64(p);
}
#endif

__inline_neon void
store_i8(signed char *p, int8x16_t v) noexcept
{
  vst1q_s8(p, v);
}

__inline_neon void
store_i16(signed short *p, int16x8_t v) noexcept
{
  vst1q_s16(p, v);
}

__inline_neon void
store_i32(signed int *p, int32x4_t v) noexcept
{
  vst1q_s32(p, v);
}

__inline_neon void
store_i64(i64 *p, int64x2_t v) noexcept
{
  vst1q_s64(p, v);
}

__inline_neon void
store_u8(unsigned char *p, uint8x16_t v) noexcept
{
  vst1q_u8(p, v);
}

__inline_neon void
store_u16(unsigned short *p, uint16x8_t v) noexcept
{
  vst1q_u16(p, v);
}

__inline_neon void
store_u32(unsigned int *p, uint32x4_t v) noexcept
{
  vst1q_u32(p, v);
}

__inline_neon void
store_u64(u64 *p, uint64x2_t v) noexcept
{
  vst1q_u64(p, v);
}

__inline_neon void
store_f32(float *p, float32x4_t v) noexcept
{
  vst1q_f32(p, v);
}
#if defined(__micron_arch_arm64)
__inline_neon void
store_f64(double *p, float64x2_t v) noexcept
{
  vst1q_f64(p, v);
}
#endif

__inline_neon int8x8_t
load_i8_h(const signed char *p) noexcept
{
  return vld1_s8(p);
}

__inline_neon int16x4_t
load_i16_h(const signed short *p) noexcept
{
  return vld1_s16(p);
}

__inline_neon int32x2_t
load_i32_h(const signed int *p) noexcept
{
  return vld1_s32(p);
}

__inline_neon int64x1_t
load_i64_h(const i64 *p) noexcept
{
  return vld1_s64(p);
}

__inline_neon uint8x8_t
load_u8_h(const unsigned char *p) noexcept
{
  return vld1_u8(p);
}

__inline_neon uint16x4_t
load_u16_h(const unsigned short *p) noexcept
{
  return vld1_u16(p);
}

__inline_neon uint32x2_t
load_u32_h(const unsigned int *p) noexcept
{
  return vld1_u32(p);
}

__inline_neon uint64x1_t
load_u64_h(const u64 *p) noexcept
{
  return vld1_u64(p);
}

__inline_neon float32x2_t
load_f32_h(const float *p) noexcept
{
  return vld1_f32(p);
}

__inline_neon void
store_i8_h(signed char *p, int8x8_t v) noexcept
{
  vst1_s8(p, v);
}

__inline_neon void
store_u8_h(unsigned char *p, uint8x8_t v) noexcept
{
  vst1_u8(p, v);
}

__inline_neon void
store_f32_h(float *p, float32x2_t v) noexcept
{
  vst1_f32(p, v);
}

__inline_neon int8x16_t
splat_i8(signed char v) noexcept
{
  return vdupq_n_s8(v);
}

__inline_neon int16x8_t
splat_i16(signed short v) noexcept
{
  return vdupq_n_s16(v);
}

__inline_neon int32x4_t
splat_i32(signed int v) noexcept
{
  return vdupq_n_s32(v);
}

__inline_neon int64x2_t
splat_i64(signed long long v) noexcept
{
  return vdupq_n_s64(v);
}

__inline_neon uint8x16_t
splat_u8(unsigned char v) noexcept
{
  return vdupq_n_u8(v);
}

__inline_neon uint16x8_t
splat_u16(unsigned short v) noexcept
{
  return vdupq_n_u16(v);
}

__inline_neon uint32x4_t
splat_u32(unsigned int v) noexcept
{
  return vdupq_n_u32(v);
}

__inline_neon uint64x2_t
splat_u64(unsigned long long v) noexcept
{
  return vdupq_n_u64(v);
}

__inline_neon float32x4_t
splat_f32(float v) noexcept
{
  return vdupq_n_f32(v);
}
#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
splat_f64(double v) noexcept
{
  return vdupq_n_f64(v);
}
#endif

__inline_neon int8x16_t
add(int8x16_t a, int8x16_t b) noexcept
{
  return vaddq_s8(a, b);
}

__inline_neon int16x8_t
add(int16x8_t a, int16x8_t b) noexcept
{
  return vaddq_s16(a, b);
}

__inline_neon int32x4_t
add(int32x4_t a, int32x4_t b) noexcept
{
  return vaddq_s32(a, b);
}

__inline_neon int64x2_t
add(int64x2_t a, int64x2_t b) noexcept
{
  return vaddq_s64(a, b);
}

__inline_neon uint8x16_t
add(uint8x16_t a, uint8x16_t b) noexcept
{
  return vaddq_u8(a, b);
}

__inline_neon uint16x8_t
add(uint16x8_t a, uint16x8_t b) noexcept
{
  return vaddq_u16(a, b);
}

__inline_neon uint32x4_t
add(uint32x4_t a, uint32x4_t b) noexcept
{
  return vaddq_u32(a, b);
}

__inline_neon uint64x2_t
add(uint64x2_t a, uint64x2_t b) noexcept
{
  return vaddq_u64(a, b);
}

__inline_neon float32x4_t
add(float32x4_t a, float32x4_t b) noexcept
{
  return vaddq_f32(a, b);
}
#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
add(float64x2_t a, float64x2_t b) noexcept
{
  return vaddq_f64(a, b);
}
#endif

__inline_neon int8x16_t
sub(int8x16_t a, int8x16_t b) noexcept
{
  return vsubq_s8(a, b);
}

__inline_neon int16x8_t
sub(int16x8_t a, int16x8_t b) noexcept
{
  return vsubq_s16(a, b);
}

__inline_neon int32x4_t
sub(int32x4_t a, int32x4_t b) noexcept
{
  return vsubq_s32(a, b);
}

__inline_neon int64x2_t
sub(int64x2_t a, int64x2_t b) noexcept
{
  return vsubq_s64(a, b);
}

__inline_neon uint8x16_t
sub(uint8x16_t a, uint8x16_t b) noexcept
{
  return vsubq_u8(a, b);
}

__inline_neon uint16x8_t
sub(uint16x8_t a, uint16x8_t b) noexcept
{
  return vsubq_u16(a, b);
}

__inline_neon uint32x4_t
sub(uint32x4_t a, uint32x4_t b) noexcept
{
  return vsubq_u32(a, b);
}

__inline_neon uint64x2_t
sub(uint64x2_t a, uint64x2_t b) noexcept
{
  return vsubq_u64(a, b);
}

__inline_neon float32x4_t
sub(float32x4_t a, float32x4_t b) noexcept
{
  return vsubq_f32(a, b);
}
#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
sub(float64x2_t a, float64x2_t b) noexcept
{
  return vsubq_f64(a, b);
}
#endif

__inline_neon int8x16_t
mul(int8x16_t a, int8x16_t b) noexcept
{
  return vmulq_s8(a, b);
}

__inline_neon int16x8_t
mul(int16x8_t a, int16x8_t b) noexcept
{
  return vmulq_s16(a, b);
}

__inline_neon int32x4_t
mul(int32x4_t a, int32x4_t b) noexcept
{
  return vmulq_s32(a, b);
}

__inline_neon uint8x16_t
mul(uint8x16_t a, uint8x16_t b) noexcept
{
  return vmulq_u8(a, b);
}

__inline_neon uint16x8_t
mul(uint16x8_t a, uint16x8_t b) noexcept
{
  return vmulq_u16(a, b);
}

__inline_neon uint32x4_t
mul(uint32x4_t a, uint32x4_t b) noexcept
{
  return vmulq_u32(a, b);
}

__inline_neon float32x4_t
mul(float32x4_t a, float32x4_t b) noexcept
{
  return vmulq_f32(a, b);
}
#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
mul(float64x2_t a, float64x2_t b) noexcept
{
  return vmulq_f64(a, b);
}

__inline_neon float32x4_t
div(float32x4_t a, float32x4_t b) noexcept
{
  return vdivq_f32(a, b);
}

__inline_neon float64x2_t
div(float64x2_t a, float64x2_t b) noexcept
{
  return vdivq_f64(a, b);
}
#endif

__inline_neon uint8x16_t
and_(uint8x16_t a, uint8x16_t b) noexcept
{
  return vandq_u8(a, b);
}

__inline_neon uint16x8_t
and_(uint16x8_t a, uint16x8_t b) noexcept
{
  return vandq_u16(a, b);
}

__inline_neon uint32x4_t
and_(uint32x4_t a, uint32x4_t b) noexcept
{
  return vandq_u32(a, b);
}

__inline_neon uint64x2_t
and_(uint64x2_t a, uint64x2_t b) noexcept
{
  return vandq_u64(a, b);
}

__inline_neon int8x16_t
and_(int8x16_t a, int8x16_t b) noexcept
{
  return vandq_s8(a, b);
}

__inline_neon int16x8_t
and_(int16x8_t a, int16x8_t b) noexcept
{
  return vandq_s16(a, b);
}

__inline_neon int32x4_t
and_(int32x4_t a, int32x4_t b) noexcept
{
  return vandq_s32(a, b);
}

__inline_neon int64x2_t
and_(int64x2_t a, int64x2_t b) noexcept
{
  return vandq_s64(a, b);
}

__inline_neon uint8x16_t
or_(uint8x16_t a, uint8x16_t b) noexcept
{
  return vorrq_u8(a, b);
}

__inline_neon uint16x8_t
or_(uint16x8_t a, uint16x8_t b) noexcept
{
  return vorrq_u16(a, b);
}

__inline_neon uint32x4_t
or_(uint32x4_t a, uint32x4_t b) noexcept
{
  return vorrq_u32(a, b);
}

__inline_neon uint64x2_t
or_(uint64x2_t a, uint64x2_t b) noexcept
{
  return vorrq_u64(a, b);
}

__inline_neon int8x16_t
or_(int8x16_t a, int8x16_t b) noexcept
{
  return vorrq_s8(a, b);
}

__inline_neon int16x8_t
or_(int16x8_t a, int16x8_t b) noexcept
{
  return vorrq_s16(a, b);
}

__inline_neon int32x4_t
or_(int32x4_t a, int32x4_t b) noexcept
{
  return vorrq_s32(a, b);
}

__inline_neon uint8x16_t
xor_(uint8x16_t a, uint8x16_t b) noexcept
{
  return veorq_u8(a, b);
}

__inline_neon uint16x8_t
xor_(uint16x8_t a, uint16x8_t b) noexcept
{
  return veorq_u16(a, b);
}

__inline_neon uint32x4_t
xor_(uint32x4_t a, uint32x4_t b) noexcept
{
  return veorq_u32(a, b);
}

__inline_neon uint64x2_t
xor_(uint64x2_t a, uint64x2_t b) noexcept
{
  return veorq_u64(a, b);
}

__inline_neon int8x16_t
xor_(int8x16_t a, int8x16_t b) noexcept
{
  return veorq_s8(a, b);
}

__inline_neon int16x8_t
xor_(int16x8_t a, int16x8_t b) noexcept
{
  return veorq_s16(a, b);
}

__inline_neon int32x4_t
xor_(int32x4_t a, int32x4_t b) noexcept
{
  return veorq_s32(a, b);
}

__inline_neon uint8x16_t
not_(uint8x16_t a) noexcept
{
  return vmvnq_u8(a);
}

__inline_neon uint16x8_t
not_(uint16x8_t a) noexcept
{
  return vmvnq_u16(a);
}

__inline_neon uint32x4_t
not_(uint32x4_t a) noexcept
{
  return vmvnq_u32(a);
}

__inline_neon uint8x16_t
eq(int8x16_t a, int8x16_t b) noexcept
{
  return vceqq_s8(a, b);
}

__inline_neon uint16x8_t
eq(int16x8_t a, int16x8_t b) noexcept
{
  return vceqq_s16(a, b);
}

__inline_neon uint32x4_t
eq(int32x4_t a, int32x4_t b) noexcept
{
  return vceqq_s32(a, b);
}

__inline_neon uint8x16_t
eq(uint8x16_t a, uint8x16_t b) noexcept
{
  return vceqq_u8(a, b);
}

__inline_neon uint16x8_t
eq(uint16x8_t a, uint16x8_t b) noexcept
{
  return vceqq_u16(a, b);
}

__inline_neon uint32x4_t
eq(uint32x4_t a, uint32x4_t b) noexcept
{
  return vceqq_u32(a, b);
}

__inline_neon uint32x4_t
eq(float32x4_t a, float32x4_t b) noexcept
{
  return vceqq_f32(a, b);
}

__inline_neon uint8x16_t
gt(int8x16_t a, int8x16_t b) noexcept
{
  return vcgtq_s8(a, b);
}

__inline_neon uint16x8_t
gt(int16x8_t a, int16x8_t b) noexcept
{
  return vcgtq_s16(a, b);
}

__inline_neon uint32x4_t
gt(int32x4_t a, int32x4_t b) noexcept
{
  return vcgtq_s32(a, b);
}

__inline_neon uint8x16_t
gt(uint8x16_t a, uint8x16_t b) noexcept
{
  return vcgtq_u8(a, b);
}

__inline_neon uint16x8_t
gt(uint16x8_t a, uint16x8_t b) noexcept
{
  return vcgtq_u16(a, b);
}

__inline_neon uint32x4_t
gt(uint32x4_t a, uint32x4_t b) noexcept
{
  return vcgtq_u32(a, b);
}

__inline_neon uint32x4_t
gt(float32x4_t a, float32x4_t b) noexcept
{
  return vcgtq_f32(a, b);
}

__inline_neon uint8x16_t
lt(int8x16_t a, int8x16_t b) noexcept
{
  return vcltq_s8(a, b);
}

__inline_neon uint16x8_t
lt(int16x8_t a, int16x8_t b) noexcept
{
  return vcltq_s16(a, b);
}

__inline_neon uint32x4_t
lt(int32x4_t a, int32x4_t b) noexcept
{
  return vcltq_s32(a, b);
}

__inline_neon uint8x16_t
lt(uint8x16_t a, uint8x16_t b) noexcept
{
  return vcltq_u8(a, b);
}

__inline_neon uint16x8_t
lt(uint16x8_t a, uint16x8_t b) noexcept
{
  return vcltq_u16(a, b);
}

__inline_neon uint32x4_t
lt(uint32x4_t a, uint32x4_t b) noexcept
{
  return vcltq_u32(a, b);
}

__inline_neon float32x4_t
min(float32x4_t a, float32x4_t b) noexcept
{
  return vminq_f32(a, b);
}

__inline_neon float32x4_t
max(float32x4_t a, float32x4_t b) noexcept
{
  return vmaxq_f32(a, b);
}

__inline_neon uint32x4_t
select(uint32x4_t mask, uint32x4_t a, uint32x4_t b) noexcept
{
  return vbslq_u32(mask, a, b);
}

__inline_neon float32x4_t
select(uint32x4_t mask, float32x4_t a, float32x4_t b) noexcept
{
  return vbslq_f32(mask, a, b);
}

// integer min/max for byte/half/word lanes (arm32 + arm64).
__inline_neon int8x16_t
min(int8x16_t a, int8x16_t b) noexcept
{
  return vminq_s8(a, b);
}

__inline_neon int16x8_t
min(int16x8_t a, int16x8_t b) noexcept
{
  return vminq_s16(a, b);
}

__inline_neon int32x4_t
min(int32x4_t a, int32x4_t b) noexcept
{
  return vminq_s32(a, b);
}

__inline_neon uint8x16_t
min(uint8x16_t a, uint8x16_t b) noexcept
{
  return vminq_u8(a, b);
}

__inline_neon uint16x8_t
min(uint16x8_t a, uint16x8_t b) noexcept
{
  return vminq_u16(a, b);
}

__inline_neon uint32x4_t
min(uint32x4_t a, uint32x4_t b) noexcept
{
  return vminq_u32(a, b);
}

__inline_neon int8x16_t
max(int8x16_t a, int8x16_t b) noexcept
{
  return vmaxq_s8(a, b);
}

__inline_neon int16x8_t
max(int16x8_t a, int16x8_t b) noexcept
{
  return vmaxq_s16(a, b);
}

__inline_neon int32x4_t
max(int32x4_t a, int32x4_t b) noexcept
{
  return vmaxq_s32(a, b);
}

__inline_neon uint8x16_t
max(uint8x16_t a, uint8x16_t b) noexcept
{
  return vmaxq_u8(a, b);
}

__inline_neon uint16x8_t
max(uint16x8_t a, uint16x8_t b) noexcept
{
  return vmaxq_u16(a, b);
}

__inline_neon uint32x4_t
max(uint32x4_t a, uint32x4_t b) noexcept
{
  return vmaxq_u32(a, b);
}

__inline_neon int8x16_t
abs(int8x16_t a) noexcept
{
  return vabsq_s8(a);
}

__inline_neon int16x8_t
abs(int16x8_t a) noexcept
{
  return vabsq_s16(a);
}

__inline_neon int32x4_t
abs(int32x4_t a) noexcept
{
  return vabsq_s32(a);
}

__inline_neon uint8x16_t
select(uint8x16_t mask, uint8x16_t a, uint8x16_t b) noexcept
{
  return vbslq_u8(mask, a, b);
}

__inline_neon uint16x8_t
select(uint16x8_t mask, uint16x8_t a, uint16x8_t b) noexcept
{
  return vbslq_u16(mask, a, b);
}

__inline_neon uint64x2_t
select(uint64x2_t mask, uint64x2_t a, uint64x2_t b) noexcept
{
  return vbslq_u64(mask, a, b);
}

__inline_neon int8x16_t
select(uint8x16_t mask, int8x16_t a, int8x16_t b) noexcept
{
  return vbslq_s8(mask, a, b);
}

#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
__inline_neon float32x4_t
fma(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return vfmaq_f32(a, b, c);
}

__inline_neon float32x4_t
fms(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return vfmsq_f32(a, b, c);
}
#endif

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
min(float64x2_t a, float64x2_t b) noexcept
{
  return vminq_f64(a, b);
}

__inline_neon float64x2_t
max(float64x2_t a, float64x2_t b) noexcept
{
  return vmaxq_f64(a, b);
}

__inline_neon int64x2_t
abs(int64x2_t a) noexcept
{
  return vabsq_s64(a);
}

__inline_neon float64x2_t
abs(float64x2_t a) noexcept
{
  return vabsq_f64(a);
}

__inline_neon float64x2_t
fma(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return vfmaq_f64(a, b, c);
}

__inline_neon float64x2_t
fms(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return vfmsq_f64(a, b, c);
}

__inline_neon float64x2_t
select(uint64x2_t mask, float64x2_t a, float64x2_t b) noexcept
{
  return vbslq_f64(mask, a, b);
}
#endif

__inline_neon float32x4_t
abs(float32x4_t a) noexcept
{
  return vabsq_f32(a);
}

__inline_neon float32x4_t
neg(float32x4_t a) noexcept
{
  return vnegq_f32(a);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
neg(float64x2_t a) noexcept
{
  return vnegq_f64(a);
}
#endif

__inline_neon float32x4_t
sqrt(float32x4_t a) noexcept
{
  return vsqrtq_f32(a);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
sqrt(float64x2_t a) noexcept
{
  return vsqrtq_f64(a);
}
#endif

__inline_neon float32x4_t
rsqrt_est(float32x4_t a) noexcept
{
  return vrsqrteq_f32(a);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
rsqrt_est(float64x2_t a) noexcept
{
  return vrsqrteq_f64(a);
}
#endif

__inline_neon float32x4_t
rsqrt_step(float32x4_t a, float32x4_t b) noexcept
{
  return vrsqrtsq_f32(a, b);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
rsqrt_step(float64x2_t a, float64x2_t b) noexcept
{
  return vrsqrtsq_f64(a, b);
}
#endif

__inline_neon float32x4_t
rcp_est(float32x4_t a) noexcept
{
  return vrecpeq_f32(a);
}

__inline_neon float32x4_t
rcp_step(float32x4_t a, float32x4_t b) noexcept
{
  return vrecpsq_f32(a, b);
}

#if defined(__micron_arm_fma) || defined(__ARM_FEATURE_FMA)
__inline_neon float32x4_t
fma_f32(float32x4_t acc, float32x4_t a, float32x4_t b) noexcept
{
  return vfmaq_f32(acc, a, b);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
fma_f64(float64x2_t acc, float64x2_t a, float64x2_t b) noexcept
{
  return vfmaq_f64(acc, a, b);
}
#endif

__inline_neon float32x4_t
fms_f32(float32x4_t acc, float32x4_t a, float32x4_t b) noexcept
{
  return vfmsq_f32(acc, a, b);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
fms_f64(float64x2_t acc, float64x2_t a, float64x2_t b) noexcept
{
  return vfmsq_f64(acc, a, b);
}
#endif
#endif

__inline_neon float32x4_t
mla(float32x4_t acc, float32x4_t a, float32x4_t b) noexcept
{
  return vmlaq_f32(acc, a, b);
}

#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
__inline_neon float32x4_t
floor(float32x4_t a) noexcept
{
  return vrndmq_f32(a);
}

__inline_neon float32x4_t
ceil(float32x4_t a) noexcept
{
  return vrndpq_f32(a);
}

__inline_neon float32x4_t
trunc(float32x4_t a) noexcept
{
  return vrndq_f32(a);
}

__inline_neon float32x4_t
rint(float32x4_t a) noexcept
{
  return vrndnq_f32(a);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
floor(float64x2_t a) noexcept
{
  return vrndmq_f64(a);
}

__inline_neon float64x2_t
ceil(float64x2_t a) noexcept
{
  return vrndpq_f64(a);
}

__inline_neon float64x2_t
trunc(float64x2_t a) noexcept
{
  return vrndq_f64(a);
}

__inline_neon float64x2_t
rint(float64x2_t a) noexcept
{
  return vrndnq_f64(a);
}
#endif
#endif

template<int LANE>
__inline_neon float
get_lane_f32(float32x4_t a) noexcept
{
  return vgetq_lane_f32(a, LANE);
}

#if defined(__micron_arch_arm64)
template<int LANE>
__inline_neon double
get_lane_f64(float64x2_t a) noexcept
{
  return vgetq_lane_f64(a, LANE);
}
#endif

template<int LANE>
__inline_neon int
get_lane_i32(int32x4_t a) noexcept
{
  return vgetq_lane_s32(a, LANE);
}

template<int LANE>
__inline_neon long long
get_lane_i64(int64x2_t a) noexcept
{
  return vgetq_lane_s64(a, LANE);
}

template<int LANE>
__inline_neon float32x4_t
set_lane_f32(float v, float32x4_t a) noexcept
{
  return vsetq_lane_f32(v, a, LANE);
}

#if defined(__micron_arch_arm64)
template<int LANE>
__inline_neon float64x2_t
set_lane_f64(double v, float64x2_t a) noexcept
{
  return vsetq_lane_f64(v, a, LANE);
}
#endif

__inline_neon uint32x4_t
bic(uint32x4_t a, uint32x4_t b) noexcept
{
  return vbicq_u32(a, b);
}

__inline_neon uint64x2_t
bic(uint64x2_t a, uint64x2_t b) noexcept
{
  return vbicq_u64(a, b);
}

__inline_neon int64x2_t
or_(int64x2_t a, int64x2_t b) noexcept
{
  return vorrq_s64(a, b);
}

__inline_neon float32x4_t
reinterpret_f32_from_u32(uint32x4_t a) noexcept
{
  return vreinterpretq_f32_u32(a);
}

__inline_neon uint32x4_t
reinterpret_u32_from_f32(float32x4_t a) noexcept
{
  return vreinterpretq_u32_f32(a);
}

__inline_neon float32x4_t
reinterpret_f32_from_s32(int32x4_t a) noexcept
{
  return vreinterpretq_f32_s32(a);
}

__inline_neon int32x4_t
reinterpret_s32_from_f32(float32x4_t a) noexcept
{
  return vreinterpretq_s32_f32(a);
}

__inline_neon int32x4_t
reinterpret_s32_from_u32(uint32x4_t a) noexcept
{
  return vreinterpretq_s32_u32(a);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
reinterpret_f64_from_u64(uint64x2_t a) noexcept
{
  return vreinterpretq_f64_u64(a);
}

__inline_neon uint64x2_t
reinterpret_u64_from_f64(float64x2_t a) noexcept
{
  return vreinterpretq_u64_f64(a);
}

__inline_neon float64x2_t
reinterpret_f64_from_s64(int64x2_t a) noexcept
{
  return vreinterpretq_f64_s64(a);
}

__inline_neon int64x2_t
reinterpret_s64_from_f64(float64x2_t a) noexcept
{
  return vreinterpretq_s64_f64(a);
}

__inline_neon int64x2_t
reinterpret_s64_from_u64(uint64x2_t a) noexcept
{
  return vreinterpretq_s64_u64(a);
}
#endif

__inline_neon float32x4_t
convert_i32_to_f32(int32x4_t a) noexcept
{
  return vcvtq_f32_s32(a);
}

__inline_neon int32x4_t
convert_f32_to_i32(float32x4_t a) noexcept
{
  return vcvtq_s32_f32(a);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
convert_i64_to_f64(int64x2_t a) noexcept
{
  return vcvtq_f64_s64(a);
}

__inline_neon int64x2_t
convert_f64_to_i64(float64x2_t a) noexcept
{
  return vcvtq_s64_f64(a);
}
#endif

template<int N>
__inline_neon int32x4_t
shl_i32(int32x4_t a) noexcept
{
  return vshlq_n_s32(a, N);
}

template<int N>
__inline_neon int64x2_t
shl_i64(int64x2_t a) noexcept
{
  return vshlq_n_s64(a, N);
}

template<int N>
__inline_neon int32x4_t
shr_arith_i32(int32x4_t a) noexcept
{
  return vshrq_n_s32(a, N);
}

template<int N>
__inline_neon int64x2_t
shr_arith_i64(int64x2_t a) noexcept
{
  return vshrq_n_s64(a, N);
}

__inline_neon float32x2_t
splat_h_f32(float v) noexcept
{
  return vdup_n_f32(v);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x1_t
splat_h_f64(double v) noexcept
{
  return vdup_n_f64(v);
}
#endif

__inline_neon uint32x4_t
lt(float32x4_t a, float32x4_t b) noexcept
{
  return vcltq_f32(a, b);
}

#if defined(__micron_arch_arm64)
__inline_neon uint64x2_t
lt(float64x2_t a, float64x2_t b) noexcept
{
  return vcltq_f64(a, b);
}

__inline_neon uint64x2_t
gt(float64x2_t a, float64x2_t b) noexcept
{
  return vcgtq_f64(a, b);
}

__inline_neon uint64x2_t
eq(int64x2_t a, int64x2_t b) noexcept
{
  return vceqq_s64(a, b);
}
#endif

__inline_neon float32x4_t
zip_lo_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 4, 1, 5);
}

__inline_neon float32x4_t
zip_hi_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 2, 6, 3, 7);
}

__inline_neon float32x4_t
concat_lo_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 1, 4, 5);
}

__inline_neon float32x4_t
concat_hi_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 2, 3, 6, 7);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
zip_lo_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2);
}

__inline_neon float64x2_t
zip_hi_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3);
}
#endif

#define __mc_neon_alias_low_high(SUF, T_Q, T_D)                                                                                            \
  __inline_neon T_D low(T_Q v) noexcept { return vget_low_##SUF(v); }                                                                      \
  __inline_neon T_D high(T_Q v) noexcept { return vget_high_##SUF(v); }

__mc_neon_alias_low_high(s8, int8x16_t, int8x8_t);
__mc_neon_alias_low_high(u8, uint8x16_t, uint8x8_t);
__mc_neon_alias_low_high(s16, int16x8_t, int16x4_t);
__mc_neon_alias_low_high(u16, uint16x8_t, uint16x4_t);
__mc_neon_alias_low_high(s32, int32x4_t, int32x2_t);
__mc_neon_alias_low_high(u32, uint32x4_t, uint32x2_t);
__mc_neon_alias_low_high(s64, int64x2_t, int64x1_t);
__mc_neon_alias_low_high(u64, uint64x2_t, uint64x1_t);
__mc_neon_alias_low_high(f32, float32x4_t, float32x2_t);
#if defined(__micron_arch_arm64)
__mc_neon_alias_low_high(f64, float64x2_t, float64x1_t);
#endif

#undef __mc_neon_alias_low_high

#define __mc_neon_alias_combine(SUF, T_D, T_Q)                                                                                             \
  __inline_neon T_Q combine(T_D lo, T_D hi) noexcept { return vcombine_##SUF(lo, hi); }

__mc_neon_alias_combine(s8, int8x8_t, int8x16_t);
__mc_neon_alias_combine(u8, uint8x8_t, uint8x16_t);
__mc_neon_alias_combine(s16, int16x4_t, int16x8_t);
__mc_neon_alias_combine(u16, uint16x4_t, uint16x8_t);
__mc_neon_alias_combine(s32, int32x2_t, int32x4_t);
__mc_neon_alias_combine(u32, uint32x2_t, uint32x4_t);
__mc_neon_alias_combine(s64, int64x1_t, int64x2_t);
__mc_neon_alias_combine(u64, uint64x1_t, uint64x2_t);
__mc_neon_alias_combine(f32, float32x2_t, float32x4_t);
#if defined(__micron_arch_arm64)
__mc_neon_alias_combine(f64, float64x1_t, float64x2_t);
#endif

#undef __mc_neon_alias_combine

__inline_neon int8x16_t
absdiff(int8x16_t a, int8x16_t b) noexcept
{
  return vabdq_s8(a, b);
}

__inline_neon int16x8_t
absdiff(int16x8_t a, int16x8_t b) noexcept
{
  return vabdq_s16(a, b);
}

__inline_neon int32x4_t
absdiff(int32x4_t a, int32x4_t b) noexcept
{
  return vabdq_s32(a, b);
}

__inline_neon uint8x16_t
absdiff(uint8x16_t a, uint8x16_t b) noexcept
{
  return vabdq_u8(a, b);
}

__inline_neon uint16x8_t
absdiff(uint16x8_t a, uint16x8_t b) noexcept
{
  return vabdq_u16(a, b);
}

__inline_neon uint32x4_t
absdiff(uint32x4_t a, uint32x4_t b) noexcept
{
  return vabdq_u32(a, b);
}

__inline_neon float32x4_t
absdiff(float32x4_t a, float32x4_t b) noexcept
{
  return vabdq_f32(a, b);
}

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
absdiff(float64x2_t a, float64x2_t b) noexcept
{
  return vabdq_f64(a, b);
}
#endif

__inline_neon int8x8_t
absdiff(int8x8_t a, int8x8_t b) noexcept
{
  return vabd_s8(a, b);
}

__inline_neon int16x4_t
absdiff(int16x4_t a, int16x4_t b) noexcept
{
  return vabd_s16(a, b);
}

__inline_neon int32x2_t
absdiff(int32x2_t a, int32x2_t b) noexcept
{
  return vabd_s32(a, b);
}

__inline_neon uint8x8_t
absdiff(uint8x8_t a, uint8x8_t b) noexcept
{
  return vabd_u8(a, b);
}

__inline_neon uint16x4_t
absdiff(uint16x4_t a, uint16x4_t b) noexcept
{
  return vabd_u16(a, b);
}

__inline_neon uint32x2_t
absdiff(uint32x2_t a, uint32x2_t b) noexcept
{
  return vabd_u32(a, b);
}

__inline_neon float32x2_t
absdiff(float32x2_t a, float32x2_t b) noexcept
{
  return vabd_f32(a, b);
}

__inline_neon int16x8_t
absdiff_long(int8x8_t a, int8x8_t b) noexcept
{
  return vabdl_s8(a, b);
}

__inline_neon int32x4_t
absdiff_long(int16x4_t a, int16x4_t b) noexcept
{
  return vabdl_s16(a, b);
}

__inline_neon int64x2_t
absdiff_long(int32x2_t a, int32x2_t b) noexcept
{
  return vabdl_s32(a, b);
}

__inline_neon uint16x8_t
absdiff_long(uint8x8_t a, uint8x8_t b) noexcept
{
  return vabdl_u8(a, b);
}

__inline_neon uint32x4_t
absdiff_long(uint16x4_t a, uint16x4_t b) noexcept
{
  return vabdl_u16(a, b);
}

__inline_neon uint64x2_t
absdiff_long(uint32x2_t a, uint32x2_t b) noexcept
{
  return vabdl_u32(a, b);
}

__inline_neon int8x16_t
absdiff_acc(int8x16_t acc, int8x16_t a, int8x16_t b) noexcept
{
  return vabaq_s8(acc, a, b);
}

__inline_neon int16x8_t
absdiff_acc(int16x8_t acc, int16x8_t a, int16x8_t b) noexcept
{
  return vabaq_s16(acc, a, b);
}

__inline_neon int32x4_t
absdiff_acc(int32x4_t acc, int32x4_t a, int32x4_t b) noexcept
{
  return vabaq_s32(acc, a, b);
}

__inline_neon uint8x16_t
absdiff_acc(uint8x16_t acc, uint8x16_t a, uint8x16_t b) noexcept
{
  return vabaq_u8(acc, a, b);
}

__inline_neon uint16x8_t
absdiff_acc(uint16x8_t acc, uint16x8_t a, uint16x8_t b) noexcept
{
  return vabaq_u16(acc, a, b);
}

__inline_neon uint32x4_t
absdiff_acc(uint32x4_t acc, uint32x4_t a, uint32x4_t b) noexcept
{
  return vabaq_u32(acc, a, b);
}

template<int N>
__inline_neon uint8x16_t
shr_imm_u8(uint8x16_t a) noexcept
{
  return vshrq_n_u8(a, N);
}

__inline_neon uint8x16_t
shl_var_u8(uint8x16_t a, int8x16_t cnt) noexcept
{
  return vshlq_u8(a, cnt);
}

__inline_neon uint8x8_t
pairwise_add_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return vpadd_u8(a, b);
}

template<int LANE>
__inline_neon uint8_t
get_lane_u8(uint8x8_t a) noexcept
{
  return vget_lane_u8(a, LANE);
}

__inline_neon uint16_t
movemask_u8(uint8x16_t v) noexcept
{
  static const int8_t __shl_arr[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };
  uint8x16_t bits = vshlq_u8(vshrq_n_u8(v, 7), vld1q_s8(__shl_arr));
  uint8x8_t lo = vget_low_u8(bits);
  uint8x8_t hi = vget_high_u8(bits);
  uint8x8_t r = vpadd_u8(lo, hi);
  r = vpadd_u8(r, r);
  r = vpadd_u8(r, r);
  return static_cast<uint16_t>(vget_lane_u8(r, 0)) | (static_cast<uint16_t>(vget_lane_u8(r, 1)) << 8);
}

__inline_neon int8x16_t
qadd(int8x16_t a, int8x16_t b) noexcept
{
  return vqaddq_s8(a, b);
}

__inline_neon int16x8_t
qadd(int16x8_t a, int16x8_t b) noexcept
{
  return vqaddq_s16(a, b);
}

__inline_neon int32x4_t
qadd(int32x4_t a, int32x4_t b) noexcept
{
  return vqaddq_s32(a, b);
}

__inline_neon int64x2_t
qadd(int64x2_t a, int64x2_t b) noexcept
{
  return vqaddq_s64(a, b);
}

__inline_neon uint8x16_t
qadd(uint8x16_t a, uint8x16_t b) noexcept
{
  return vqaddq_u8(a, b);
}

__inline_neon uint16x8_t
qadd(uint16x8_t a, uint16x8_t b) noexcept
{
  return vqaddq_u16(a, b);
}

__inline_neon uint32x4_t
qadd(uint32x4_t a, uint32x4_t b) noexcept
{
  return vqaddq_u32(a, b);
}

__inline_neon uint64x2_t
qadd(uint64x2_t a, uint64x2_t b) noexcept
{
  return vqaddq_u64(a, b);
}

__inline_neon int8x16_t
qsub(int8x16_t a, int8x16_t b) noexcept
{
  return vqsubq_s8(a, b);
}

__inline_neon int16x8_t
qsub(int16x8_t a, int16x8_t b) noexcept
{
  return vqsubq_s16(a, b);
}

__inline_neon int32x4_t
qsub(int32x4_t a, int32x4_t b) noexcept
{
  return vqsubq_s32(a, b);
}

__inline_neon int64x2_t
qsub(int64x2_t a, int64x2_t b) noexcept
{
  return vqsubq_s64(a, b);
}

__inline_neon uint8x16_t
qsub(uint8x16_t a, uint8x16_t b) noexcept
{
  return vqsubq_u8(a, b);
}

__inline_neon uint16x8_t
qsub(uint16x8_t a, uint16x8_t b) noexcept
{
  return vqsubq_u16(a, b);
}

__inline_neon uint32x4_t
qsub(uint32x4_t a, uint32x4_t b) noexcept
{
  return vqsubq_u32(a, b);
}

__inline_neon uint64x2_t
qsub(uint64x2_t a, uint64x2_t b) noexcept
{
  return vqsubq_u64(a, b);
}

__inline_neon int8x16_t
hadd(int8x16_t a, int8x16_t b) noexcept
{
  return vhaddq_s8(a, b);
}

__inline_neon int16x8_t
hadd(int16x8_t a, int16x8_t b) noexcept
{
  return vhaddq_s16(a, b);
}

__inline_neon int32x4_t
hadd(int32x4_t a, int32x4_t b) noexcept
{
  return vhaddq_s32(a, b);
}

__inline_neon uint8x16_t
hadd(uint8x16_t a, uint8x16_t b) noexcept
{
  return vhaddq_u8(a, b);
}

__inline_neon uint16x8_t
hadd(uint16x8_t a, uint16x8_t b) noexcept
{
  return vhaddq_u16(a, b);
}

__inline_neon uint32x4_t
hadd(uint32x4_t a, uint32x4_t b) noexcept
{
  return vhaddq_u32(a, b);
}

__inline_neon int8x16_t
hsub(int8x16_t a, int8x16_t b) noexcept
{
  return vhsubq_s8(a, b);
}

__inline_neon int16x8_t
hsub(int16x8_t a, int16x8_t b) noexcept
{
  return vhsubq_s16(a, b);
}

__inline_neon int32x4_t
hsub(int32x4_t a, int32x4_t b) noexcept
{
  return vhsubq_s32(a, b);
}

__inline_neon uint8x16_t
hsub(uint8x16_t a, uint8x16_t b) noexcept
{
  return vhsubq_u8(a, b);
}

__inline_neon uint16x8_t
hsub(uint16x8_t a, uint16x8_t b) noexcept
{
  return vhsubq_u16(a, b);
}

__inline_neon uint32x4_t
hsub(uint32x4_t a, uint32x4_t b) noexcept
{
  return vhsubq_u32(a, b);
}

__inline_neon int8x16_t
rhadd(int8x16_t a, int8x16_t b) noexcept
{
  return vrhaddq_s8(a, b);
}

__inline_neon int16x8_t
rhadd(int16x8_t a, int16x8_t b) noexcept
{
  return vrhaddq_s16(a, b);
}

__inline_neon int32x4_t
rhadd(int32x4_t a, int32x4_t b) noexcept
{
  return vrhaddq_s32(a, b);
}

__inline_neon uint8x16_t
rhadd(uint8x16_t a, uint8x16_t b) noexcept
{
  return vrhaddq_u8(a, b);
}

__inline_neon uint16x8_t
rhadd(uint16x8_t a, uint16x8_t b) noexcept
{
  return vrhaddq_u16(a, b);
}

__inline_neon uint32x4_t
rhadd(uint32x4_t a, uint32x4_t b) noexcept
{
  return vrhaddq_u32(a, b);
}

#if defined(__micron_arch_arm64)
__inline_neon signed char
reduce_add(int8x16_t v) noexcept
{
  return vaddvq_s8(v);
}

__inline_neon signed short
reduce_add(int16x8_t v) noexcept
{
  return vaddvq_s16(v);
}

__inline_neon signed int
reduce_add(int32x4_t v) noexcept
{
  return vaddvq_s32(v);
}

__inline_neon signed long long
reduce_add(int64x2_t v) noexcept
{
  return vaddvq_s64(v);
}

__inline_neon unsigned char
reduce_add(uint8x16_t v) noexcept
{
  return vaddvq_u8(v);
}

__inline_neon unsigned short
reduce_add(uint16x8_t v) noexcept
{
  return vaddvq_u16(v);
}

__inline_neon unsigned int
reduce_add(uint32x4_t v) noexcept
{
  return vaddvq_u32(v);
}

__inline_neon unsigned long long
reduce_add(uint64x2_t v) noexcept
{
  return vaddvq_u64(v);
}

__inline_neon float
reduce_add(float32x4_t v) noexcept
{
  return vaddvq_f32(v);
}

__inline_neon double
reduce_add(float64x2_t v) noexcept
{
  return vaddvq_f64(v);
}

__inline_neon signed char
reduce_max(int8x16_t v) noexcept
{
  return vmaxvq_s8(v);
}

__inline_neon signed short
reduce_max(int16x8_t v) noexcept
{
  return vmaxvq_s16(v);
}

__inline_neon signed int
reduce_max(int32x4_t v) noexcept
{
  return vmaxvq_s32(v);
}

__inline_neon unsigned char
reduce_max(uint8x16_t v) noexcept
{
  return vmaxvq_u8(v);
}

__inline_neon unsigned short
reduce_max(uint16x8_t v) noexcept
{
  return vmaxvq_u16(v);
}

__inline_neon unsigned int
reduce_max(uint32x4_t v) noexcept
{
  return vmaxvq_u32(v);
}

__inline_neon float
reduce_max(float32x4_t v) noexcept
{
  return vmaxvq_f32(v);
}

__inline_neon double
reduce_max(float64x2_t v) noexcept
{
  return vmaxvq_f64(v);
}

__inline_neon signed char
reduce_min(int8x16_t v) noexcept
{
  return vminvq_s8(v);
}

__inline_neon signed short
reduce_min(int16x8_t v) noexcept
{
  return vminvq_s16(v);
}

__inline_neon signed int
reduce_min(int32x4_t v) noexcept
{
  return vminvq_s32(v);
}

__inline_neon unsigned char
reduce_min(uint8x16_t v) noexcept
{
  return vminvq_u8(v);
}

__inline_neon unsigned short
reduce_min(uint16x8_t v) noexcept
{
  return vminvq_u16(v);
}

__inline_neon unsigned int
reduce_min(uint32x4_t v) noexcept
{
  return vminvq_u32(v);
}

__inline_neon float
reduce_min(float32x4_t v) noexcept
{
  return vminvq_f32(v);
}

__inline_neon double
reduce_min(float64x2_t v) noexcept
{
  return vminvq_f64(v);
}
#endif

__inline_neon int16x8_t
mul_long(int8x8_t a, int8x8_t b) noexcept
{
  return vmull_s8(a, b);
}

__inline_neon int32x4_t
mul_long(int16x4_t a, int16x4_t b) noexcept
{
  return vmull_s16(a, b);
}

__inline_neon int64x2_t
mul_long(int32x2_t a, int32x2_t b) noexcept
{
  return vmull_s32(a, b);
}

__inline_neon uint16x8_t
mul_long(uint8x8_t a, uint8x8_t b) noexcept
{
  return vmull_u8(a, b);
}

__inline_neon uint32x4_t
mul_long(uint16x4_t a, uint16x4_t b) noexcept
{
  return vmull_u16(a, b);
}

__inline_neon uint64x2_t
mul_long(uint32x2_t a, uint32x2_t b) noexcept
{
  return vmull_u32(a, b);
}

__inline_neon int16x8_t
widen(int8x8_t v) noexcept
{
  return vmovl_s8(v);
}

__inline_neon int32x4_t
widen(int16x4_t v) noexcept
{
  return vmovl_s16(v);
}

__inline_neon int64x2_t
widen(int32x2_t v) noexcept
{
  return vmovl_s32(v);
}

__inline_neon uint16x8_t
widen(uint8x8_t v) noexcept
{
  return vmovl_u8(v);
}

__inline_neon uint32x4_t
widen(uint16x4_t v) noexcept
{
  return vmovl_u16(v);
}

__inline_neon uint64x2_t
widen(uint32x2_t v) noexcept
{
  return vmovl_u32(v);
}

__inline_neon int8x8_t
narrow(int16x8_t v) noexcept
{
  return vmovn_s16(v);
}

__inline_neon int16x4_t
narrow(int32x4_t v) noexcept
{
  return vmovn_s32(v);
}

__inline_neon int32x2_t
narrow(int64x2_t v) noexcept
{
  return vmovn_s64(v);
}

__inline_neon uint8x8_t
narrow(uint16x8_t v) noexcept
{
  return vmovn_u16(v);
}

__inline_neon uint16x4_t
narrow(uint32x4_t v) noexcept
{
  return vmovn_u32(v);
}

__inline_neon uint32x2_t
narrow(uint64x2_t v) noexcept
{
  return vmovn_u64(v);
}

__inline_neon int8x8_t
narrow_sat(int16x8_t v) noexcept
{
  return vqmovn_s16(v);
}

__inline_neon int16x4_t
narrow_sat(int32x4_t v) noexcept
{
  return vqmovn_s32(v);
}

__inline_neon int32x2_t
narrow_sat(int64x2_t v) noexcept
{
  return vqmovn_s64(v);
}

__inline_neon uint8x8_t
narrow_sat(uint16x8_t v) noexcept
{
  return vqmovn_u16(v);
}

__inline_neon uint16x4_t
narrow_sat(uint32x4_t v) noexcept
{
  return vqmovn_u32(v);
}

__inline_neon uint32x2_t
narrow_sat(uint64x2_t v) noexcept
{
  return vqmovn_u64(v);
}

#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
__inline_neon float32x4_t
nearest_even(float32x4_t a) noexcept
{
  return vrndnq_f32(a);
}

__inline_neon float32x4_t
nearest_away(float32x4_t a) noexcept
{
  return vrndaq_f32(a);
}
#endif

#if defined(__micron_arch_arm64)
__inline_neon float64x2_t
nearest_even(float64x2_t a) noexcept
{
  return vrndnq_f64(a);
}

__inline_neon float64x2_t
nearest_away(float64x2_t a) noexcept
{
  return vrndaq_f64(a);
}
#endif

#undef __inline_neon

#pragma GCC diagnostic pop

};      // namespace neon
};      // namespace simd
};      // namespace micron
