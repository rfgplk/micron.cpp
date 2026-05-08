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
load_i64(const signed long long *p) noexcept
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
load_u64(const unsigned long long *p) noexcept
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
store_i64(signed long long *p, int64x2_t v) noexcept
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
store_u64(unsigned long long *p, uint64x2_t v) noexcept
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
load_i64_h(const signed long long *p) noexcept
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
load_u64_h(const unsigned long long *p) noexcept
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

#if defined(__micron_arch_arm64)
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

__inline_neon float32x4_t
min(float32x4_t a, float32x4_t b) noexcept
{
  return vminq_f32(a, b);
}

__inline_neon float64x2_t
min(float64x2_t a, float64x2_t b) noexcept
{
  return vminq_f64(a, b);
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

__inline_neon float32x4_t
max(float32x4_t a, float32x4_t b) noexcept
{
  return vmaxq_f32(a, b);
}

__inline_neon float64x2_t
max(float64x2_t a, float64x2_t b) noexcept
{
  return vmaxq_f64(a, b);
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

__inline_neon int64x2_t
abs(int64x2_t a) noexcept
{
  return vabsq_s64(a);
}

__inline_neon float32x4_t
abs(float32x4_t a) noexcept
{
  return vabsq_f32(a);
}

__inline_neon float64x2_t
abs(float64x2_t a) noexcept
{
  return vabsq_f64(a);
}

__inline_neon float32x4_t
fma(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return vfmaq_f32(a, b, c);
}

__inline_neon float64x2_t
fma(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return vfmaq_f64(a, b, c);
}

__inline_neon float32x4_t
fms(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return vfmsq_f32(a, b, c);
}

__inline_neon float64x2_t
fms(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return vfmsq_f64(a, b, c);
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

__inline_neon uint32x4_t
select(uint32x4_t mask, uint32x4_t a, uint32x4_t b) noexcept
{
  return vbslq_u32(mask, a, b);
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

__inline_neon float32x4_t
select(uint32x4_t mask, float32x4_t a, float32x4_t b) noexcept
{
  return vbslq_f32(mask, a, b);
}

__inline_neon float64x2_t
select(uint64x2_t mask, float64x2_t a, float64x2_t b) noexcept
{
  return vbslq_f64(mask, a, b);
}
#endif

#undef __inline_neon

#pragma GCC diagnostic pop

};     // namespace neon
};     // namespace simd
};     // namespace micron
