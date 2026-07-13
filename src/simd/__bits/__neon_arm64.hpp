//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_arm64.hpp"

#if !defined(__micron_arch_arm64) && !defined(__arm64ec__)
#error "__neon_arm64.hpp included on a non-aarch64 build"
#endif

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"
#pragma GCC diagnostic ignored "-Wpedantic"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

#define __neon_ldst(T, suffix, ETYPE)                                                                                                      \
  __inline_g T vld1q_##suffix(const ETYPE *p) noexcept                                                                                     \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __builtin_memcpy(&r, p, sizeof(T));                                                                                                    \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  __inline_g void vst1q_##suffix(ETYPE *p, T v) noexcept { __builtin_memcpy(p, &v, sizeof(T)); }

#define __neon_ldst_64(T, suffix, ETYPE)                                                                                                   \
  __inline_g T vld1_##suffix(const ETYPE *p) noexcept                                                                                      \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __builtin_memcpy(&r, p, sizeof(T));                                                                                                    \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  __inline_g void vst1_##suffix(ETYPE *p, T v) noexcept { __builtin_memcpy(p, &v, sizeof(T)); }

__neon_ldst(int8x16_t, s8, signed char);
__neon_ldst(int16x8_t, s16, signed short);
__neon_ldst(int32x4_t, s32, signed int);
__neon_ldst(int64x2_t, s64, signed long);
__neon_ldst(uint8x16_t, u8, unsigned char);
__neon_ldst(uint16x8_t, u16, unsigned short);
__neon_ldst(uint32x4_t, u32, unsigned int);
__neon_ldst(uint64x2_t, u64, unsigned long);
__neon_ldst(float32x4_t, f32, float);
__neon_ldst(float64x2_t, f64, double);
__neon_ldst(poly8x16_t, p8, poly8_t);
__neon_ldst(poly16x8_t, p16, poly16_t);
__neon_ldst(poly64x2_t, p64, poly64_t);

__neon_ldst_64(int8x8_t, s8, signed char);
__neon_ldst_64(int16x4_t, s16, signed short);
__neon_ldst_64(int32x2_t, s32, signed int);
__neon_ldst_64(int64x1_t, s64, signed long);
__neon_ldst_64(uint8x8_t, u8, unsigned char);
__neon_ldst_64(uint16x4_t, u16, unsigned short);
__neon_ldst_64(uint32x2_t, u32, unsigned int);
__neon_ldst_64(uint64x1_t, u64, unsigned long);
__neon_ldst_64(float32x2_t, f32, float);
__neon_ldst_64(float64x1_t, f64, double);

#undef __neon_ldst
#undef __neon_ldst_64

__inline_g int8x16_t
vdupq_n_s8(signed char v) noexcept
{
  return (int8x16_t){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g int16x8_t
vdupq_n_s16(signed short v) noexcept
{
  return (int16x8_t){ v, v, v, v, v, v, v, v };
}

__inline_g int32x4_t
vdupq_n_s32(signed int v) noexcept
{
  return (int32x4_t){ v, v, v, v };
}

__inline_g int64x2_t
vdupq_n_s64(signed long long v) noexcept
{
  return (int64x2_t){ v, v };
}

__inline_g uint8x16_t
vdupq_n_u8(unsigned char v) noexcept
{
  return (uint8x16_t){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g uint16x8_t
vdupq_n_u16(unsigned short v) noexcept
{
  return (uint16x8_t){ v, v, v, v, v, v, v, v };
}

__inline_g uint32x4_t
vdupq_n_u32(unsigned int v) noexcept
{
  return (uint32x4_t){ v, v, v, v };
}

__inline_g uint64x2_t
vdupq_n_u64(unsigned long long v) noexcept
{
  return (uint64x2_t){ v, v };
}

__inline_g float32x4_t
vdupq_n_f32(float v) noexcept
{
  return (float32x4_t){ v, v, v, v };
}

__inline_g float64x2_t
vdupq_n_f64(double v) noexcept
{
  return (float64x2_t){ v, v };
}

__inline_g int8x8_t
vdup_n_s8(signed char v) noexcept
{
  return (int8x8_t){ v, v, v, v, v, v, v, v };
}

__inline_g int16x4_t
vdup_n_s16(signed short v) noexcept
{
  return (int16x4_t){ v, v, v, v };
}

__inline_g int32x2_t
vdup_n_s32(signed int v) noexcept
{
  return (int32x2_t){ v, v };
}

__inline_g int64x1_t
vdup_n_s64(signed long long v) noexcept
{
  return (int64x1_t){ v };
}

__inline_g uint8x8_t
vdup_n_u8(unsigned char v) noexcept
{
  return (uint8x8_t){ v, v, v, v, v, v, v, v };
}

__inline_g uint16x4_t
vdup_n_u16(unsigned short v) noexcept
{
  return (uint16x4_t){ v, v, v, v };
}

__inline_g uint32x2_t
vdup_n_u32(unsigned int v) noexcept
{
  return (uint32x2_t){ v, v };
}

__inline_g uint64x1_t
vdup_n_u64(unsigned long long v) noexcept
{
  return (uint64x1_t){ v };
}

__inline_g float32x2_t
vdup_n_f32(float v) noexcept
{
  return (float32x2_t){ v, v };
}

__inline_g float64x1_t
vdup_n_f64(double v) noexcept
{
  return (float64x1_t){ v };
}

__inline_g int8x16_t
vaddq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)(a + b);
}

__inline_g int16x8_t
vaddq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)(a + b);
}

__inline_g int32x4_t
vaddq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)(a + b);
}

__inline_g int64x2_t
vaddq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)(a + b);
}

__inline_g uint8x16_t
vaddq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)(a + b);
}

__inline_g uint16x8_t
vaddq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)(a + b);
}

__inline_g uint32x4_t
vaddq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)(a + b);
}

__inline_g uint64x2_t
vaddq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)(a + b);
}

__inline_g float32x4_t
vaddq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (float32x4_t)(a + b);
}

__inline_g float64x2_t
vaddq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (float64x2_t)(a + b);
}

__inline_g int8x16_t
vsubq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)(a - b);
}

__inline_g int16x8_t
vsubq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)(a - b);
}

__inline_g int32x4_t
vsubq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)(a - b);
}

__inline_g int64x2_t
vsubq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)(a - b);
}

__inline_g uint8x16_t
vsubq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)(a - b);
}

__inline_g uint16x8_t
vsubq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)(a - b);
}

__inline_g uint32x4_t
vsubq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)(a - b);
}

__inline_g uint64x2_t
vsubq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)(a - b);
}

__inline_g float32x4_t
vsubq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (float32x4_t)(a - b);
}

__inline_g float64x2_t
vsubq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (float64x2_t)(a - b);
}

__inline_g int8x16_t
vmulq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)(a * b);
}

__inline_g int16x8_t
vmulq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)(a * b);
}

__inline_g int32x4_t
vmulq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)(a * b);
}

__inline_g uint8x16_t
vmulq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)(a * b);
}

__inline_g uint16x8_t
vmulq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)(a * b);
}

__inline_g uint32x4_t
vmulq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)(a * b);
}

__inline_g float32x4_t
vmulq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (float32x4_t)(a * b);
}

__inline_g float64x2_t
vmulq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (float64x2_t)(a * b);
}

__inline_g int8x8_t
vadd_s8(int8x8_t a, int8x8_t b) noexcept
{
  return (int8x8_t)(a + b);
}

__inline_g int16x4_t
vadd_s16(int16x4_t a, int16x4_t b) noexcept
{
  return (int16x4_t)(a + b);
}

__inline_g int32x2_t
vadd_s32(int32x2_t a, int32x2_t b) noexcept
{
  return (int32x2_t)(a + b);
}

__inline_g uint8x8_t
vadd_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)(a + b);
}

__inline_g uint16x4_t
vadd_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)(a + b);
}

__inline_g uint32x2_t
vadd_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)(a + b);
}

__inline_g float32x2_t
vadd_f32(float32x2_t a, float32x2_t b) noexcept
{
  return (float32x2_t)(a + b);
}

__inline_g int8x8_t
vsub_s8(int8x8_t a, int8x8_t b) noexcept
{
  return (int8x8_t)(a - b);
}

__inline_g int16x4_t
vsub_s16(int16x4_t a, int16x4_t b) noexcept
{
  return (int16x4_t)(a - b);
}

__inline_g int32x2_t
vsub_s32(int32x2_t a, int32x2_t b) noexcept
{
  return (int32x2_t)(a - b);
}

__inline_g uint8x8_t
vsub_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)(a - b);
}

__inline_g uint16x4_t
vsub_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)(a - b);
}

__inline_g uint32x2_t
vsub_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)(a - b);
}

__inline_g float32x2_t
vsub_f32(float32x2_t a, float32x2_t b) noexcept
{
  return (float32x2_t)(a - b);
}

__inline_g int8x8_t
vmul_s8(int8x8_t a, int8x8_t b) noexcept
{
  return (int8x8_t)(a * b);
}

__inline_g int16x4_t
vmul_s16(int16x4_t a, int16x4_t b) noexcept
{
  return (int16x4_t)(a * b);
}

__inline_g int32x2_t
vmul_s32(int32x2_t a, int32x2_t b) noexcept
{
  return (int32x2_t)(a * b);
}

__inline_g uint8x8_t
vmul_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)(a * b);
}

__inline_g uint16x4_t
vmul_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)(a * b);
}

__inline_g uint32x2_t
vmul_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)(a * b);
}

__inline_g float32x2_t
vmul_f32(float32x2_t a, float32x2_t b) noexcept
{
  return (float32x2_t)(a * b);
}

__inline_g float32x4_t
vdivq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (float32x4_t)(a / b);
}

__inline_g float64x2_t
vdivq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (float64x2_t)(a / b);
}

__inline_g float32x2_t
vdiv_f32(float32x2_t a, float32x2_t b) noexcept
{
  return (float32x2_t)(a / b);
}

__inline_g int8x16_t
vandq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)((uint8x16_t)a & (uint8x16_t)b);
}

__inline_g int16x8_t
vandq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)((uint16x8_t)a & (uint16x8_t)b);
}

__inline_g int32x4_t
vandq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)((uint32x4_t)a & (uint32x4_t)b);
}

__inline_g int64x2_t
vandq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)((uint64x2_t)a & (uint64x2_t)b);
}

__inline_g uint8x16_t
vandq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return a & b;
}

__inline_g uint16x8_t
vandq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return a & b;
}

__inline_g uint32x4_t
vandq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return a & b;
}

__inline_g uint64x2_t
vandq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return a & b;
}

__inline_g int8x16_t
vorrq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)((uint8x16_t)a | (uint8x16_t)b);
}

__inline_g int16x8_t
vorrq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)((uint16x8_t)a | (uint16x8_t)b);
}

__inline_g int32x4_t
vorrq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)((uint32x4_t)a | (uint32x4_t)b);
}

__inline_g int64x2_t
vorrq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)((uint64x2_t)a | (uint64x2_t)b);
}

__inline_g uint8x16_t
vorrq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return a | b;
}

__inline_g uint16x8_t
vorrq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return a | b;
}

__inline_g uint32x4_t
vorrq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return a | b;
}

__inline_g uint64x2_t
vorrq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return a | b;
}

__inline_g int8x16_t
veorq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)((uint8x16_t)a ^ (uint8x16_t)b);
}

__inline_g int16x8_t
veorq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)((uint16x8_t)a ^ (uint16x8_t)b);
}

__inline_g int32x4_t
veorq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)((uint32x4_t)a ^ (uint32x4_t)b);
}

__inline_g int64x2_t
veorq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)((uint64x2_t)a ^ (uint64x2_t)b);
}

__inline_g uint8x16_t
veorq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return a ^ b;
}

__inline_g uint16x8_t
veorq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return a ^ b;
}

__inline_g uint32x4_t
veorq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return a ^ b;
}

__inline_g uint64x2_t
veorq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return a ^ b;
}

__inline_g uint8x16_t
vbicq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return a & ~b;
}

__inline_g uint16x8_t
vbicq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return a & ~b;
}

__inline_g uint32x4_t
vbicq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return a & ~b;
}

__inline_g uint64x2_t
vbicq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return a & ~b;
}

__inline_g uint8x16_t
vmvnq_u8(uint8x16_t a) noexcept
{
  return ~a;
}

__inline_g uint16x8_t
vmvnq_u16(uint16x8_t a) noexcept
{
  return ~a;
}

__inline_g uint32x4_t
vmvnq_u32(uint32x4_t a) noexcept
{
  return ~a;
}

__inline_g uint8x16_t
vceqq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (uint8x16_t)(a == b);
}

__inline_g uint16x8_t
vceqq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (uint16x8_t)(a == b);
}

__inline_g uint32x4_t
vceqq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (uint32x4_t)(a == b);
}

__inline_g uint64x2_t
vceqq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (uint64x2_t)(a == b);
}

__inline_g uint8x16_t
vceqq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)(a == b);
}

__inline_g uint16x8_t
vceqq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)(a == b);
}

__inline_g uint32x4_t
vceqq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)(a == b);
}

__inline_g uint64x2_t
vceqq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)(a == b);
}

__inline_g uint32x4_t
vceqq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (uint32x4_t)(a == b);
}

__inline_g uint64x2_t
vceqq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (uint64x2_t)(a == b);
}

__inline_g uint8x16_t
vcgtq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (uint8x16_t)(a > b);
}

__inline_g uint16x8_t
vcgtq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (uint16x8_t)(a > b);
}

__inline_g uint32x4_t
vcgtq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (uint32x4_t)(a > b);
}

__inline_g uint64x2_t
vcgtq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (uint64x2_t)(a > b);
}

__inline_g uint8x16_t
vcgtq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)(a > b);
}

__inline_g uint16x8_t
vcgtq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)(a > b);
}

__inline_g uint32x4_t
vcgtq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)(a > b);
}

__inline_g uint64x2_t
vcgtq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)(a > b);
}

__inline_g uint32x4_t
vcgtq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (uint32x4_t)(a > b);
}

__inline_g uint64x2_t
vcgtq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (uint64x2_t)(a > b);
}

__inline_g uint8x16_t
vcltq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return vcgtq_s8(b, a);
}

__inline_g uint16x8_t
vcltq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return vcgtq_s16(b, a);
}

__inline_g uint32x4_t
vcltq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return vcgtq_s32(b, a);
}

__inline_g uint8x16_t
vcltq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return vcgtq_u8(b, a);
}

__inline_g uint16x8_t
vcltq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return vcgtq_u16(b, a);
}

__inline_g uint32x4_t
vcltq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return vcgtq_u32(b, a);
}

// signed 64-bit less-than: a < b  <=>  b > a (the only vcltq the s64 set lacked).
__inline_g uint64x2_t
vcltq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vcgtq_s64(b, a);
}

// >= family. GCC's vector `>=` lowers to CMGE/FCMGE and yields an all-ones lane
// mask of the matching unsigned width — exactly the NEON vcgeq_* contract.
__inline_g uint8x16_t
vcgeq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (uint8x16_t)(a >= b);
}

__inline_g uint16x8_t
vcgeq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (uint16x8_t)(a >= b);
}

__inline_g uint32x4_t
vcgeq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (uint32x4_t)(a >= b);
}

__inline_g uint64x2_t
vcgeq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (uint64x2_t)(a >= b);
}

__inline_g uint8x16_t
vcgeq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)(a >= b);
}

__inline_g uint16x8_t
vcgeq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)(a >= b);
}

__inline_g uint32x4_t
vcgeq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)(a >= b);
}

__inline_g uint64x2_t
vcgeq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)(a >= b);
}

__inline_g uint32x4_t
vcgeq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (uint32x4_t)(a >= b);
}

__inline_g uint64x2_t
vcgeq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (uint64x2_t)(a >= b);
}

// <= family. a <= b lowers to CMGE/FCMGE with swapped operands (CMLE compares
// against zero only), giving the same all-ones lane mask the vcleq_* set wants.
__inline_g uint8x16_t
vcleq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (uint8x16_t)(a <= b);
}

__inline_g uint16x8_t
vcleq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (uint16x8_t)(a <= b);
}

__inline_g uint32x4_t
vcleq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (uint32x4_t)(a <= b);
}

__inline_g uint64x2_t
vcleq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (uint64x2_t)(a <= b);
}

__inline_g uint8x16_t
vcleq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)(a <= b);
}

__inline_g uint16x8_t
vcleq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)(a <= b);
}

__inline_g uint32x4_t
vcleq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)(a <= b);
}

__inline_g uint64x2_t
vcleq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)(a <= b);
}

__inline_g uint32x4_t
vcleq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (uint32x4_t)(a <= b);
}

__inline_g uint64x2_t
vcleq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (uint64x2_t)(a <= b);
}

__inline_g int8x16_t
vminq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_sminv16qi(a, b);
}

__inline_g int16x8_t
vminq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_sminv8hi(a, b);
}

__inline_g int32x4_t
vminq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_sminv4si(a, b);
}

__inline_g uint8x16_t
vminq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_uminv16qi((__Int8x16_t)a, (__Int8x16_t)b);
}

__inline_g uint16x8_t
vminq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uminv8hi((__Int16x8_t)a, (__Int16x8_t)b);
}

__inline_g uint32x4_t
vminq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uminv4si((__Int32x4_t)a, (__Int32x4_t)b);
}

__inline_g float32x4_t
vminq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_aarch64_fminv4sf(a, b);
}

__inline_g float64x2_t
vminq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_aarch64_fminv2df(a, b);
}

__inline_g int8x16_t
vmaxq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_smaxv16qi(a, b);
}

__inline_g int16x8_t
vmaxq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_smaxv8hi(a, b);
}

__inline_g int32x4_t
vmaxq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_smaxv4si(a, b);
}

__inline_g uint8x16_t
vmaxq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_umaxv16qi((__Int8x16_t)a, (__Int8x16_t)b);
}

__inline_g uint16x8_t
vmaxq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_umaxv8hi((__Int16x8_t)a, (__Int16x8_t)b);
}

__inline_g uint32x4_t
vmaxq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_umaxv4si((__Int32x4_t)a, (__Int32x4_t)b);
}

__inline_g float32x4_t
vmaxq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_aarch64_fmaxv4sf(a, b);
}

__inline_g float64x2_t
vmaxq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_aarch64_fmaxv2df(a, b);
}

__inline_g int8x16_t
vshlq_n_s8(int8x16_t a, const int n) noexcept
{
  return (int8x16_t)((uint8x16_t)a << (uint8x16_t)vdupq_n_u8((unsigned char)n));
}

__inline_g int16x8_t
vshlq_n_s16(int16x8_t a, const int n) noexcept
{
  return (int16x8_t)((uint16x8_t)a << (uint16x8_t)vdupq_n_u16((unsigned short)n));
}

__inline_g int32x4_t
vshlq_n_s32(int32x4_t a, const int n) noexcept
{
  return (int32x4_t)((uint32x4_t)a << (uint32x4_t)vdupq_n_u32((unsigned)n));
}

__inline_g int64x2_t
vshlq_n_s64(int64x2_t a, const int n) noexcept
{
  return (int64x2_t)((uint64x2_t)a << (uint64x2_t)vdupq_n_u64((unsigned long long)n));
}

__inline_g uint8x16_t
vshlq_n_u8(uint8x16_t a, const int n) noexcept
{
  return a << (uint8x16_t)vdupq_n_u8((unsigned char)n);
}

__inline_g uint16x8_t
vshlq_n_u16(uint16x8_t a, const int n) noexcept
{
  return a << (uint16x8_t)vdupq_n_u16((unsigned short)n);
}

__inline_g uint32x4_t
vshlq_n_u32(uint32x4_t a, const int n) noexcept
{
  return a << (uint32x4_t)vdupq_n_u32((unsigned)n);
}

__inline_g uint64x2_t
vshlq_n_u64(uint64x2_t a, const int n) noexcept
{
  return a << (uint64x2_t)vdupq_n_u64((unsigned long long)n);
}

__inline_g uint8x16_t
vshrq_n_u8(uint8x16_t a, const int n) noexcept
{
  return a >> (uint8x16_t)vdupq_n_u8((unsigned char)n);
}

__inline_g uint16x8_t
vshrq_n_u16(uint16x8_t a, const int n) noexcept
{
  return a >> (uint16x8_t)vdupq_n_u16((unsigned short)n);
}

__inline_g uint32x4_t
vshrq_n_u32(uint32x4_t a, const int n) noexcept
{
  return a >> (uint32x4_t)vdupq_n_u32((unsigned)n);
}

__inline_g uint64x2_t
vshrq_n_u64(uint64x2_t a, const int n) noexcept
{
  return a >> (uint64x2_t)vdupq_n_u64((unsigned long long)n);
}

__inline_g int8x16_t
vshrq_n_s8(int8x16_t a, const int n) noexcept
{
  return a >> (int8x16_t)vdupq_n_s8((signed char)n);
}

__inline_g int16x8_t
vshrq_n_s16(int16x8_t a, const int n) noexcept
{
  return a >> (int16x8_t)vdupq_n_s16((signed short)n);
}

__inline_g int32x4_t
vshrq_n_s32(int32x4_t a, const int n) noexcept
{
  return a >> (int32x4_t)vdupq_n_s32((signed)n);
}

__inline_g int64x2_t
vshrq_n_s64(int64x2_t a, const int n) noexcept
{
  return a >> (int64x2_t)vdupq_n_s64((signed long long)n);
}

#define __neon_reinterpret(SUFFIX_TO, T_TO, SUFFIX_FROM, T_FROM)                                                                           \
  __inline_g T_TO vreinterpretq_##SUFFIX_TO##_##SUFFIX_FROM(T_FROM a) noexcept { return (T_TO)a; }

#define __neon_reinterpret_ALL_FROM(SUFFIX_FROM, T_FROM)                                                                                   \
  __neon_reinterpret(s8, int8x16_t, SUFFIX_FROM, T_FROM) __neon_reinterpret(s16, int16x8_t, SUFFIX_FROM, T_FROM)                           \
      __neon_reinterpret(s32, int32x4_t, SUFFIX_FROM, T_FROM) __neon_reinterpret(s64, int64x2_t, SUFFIX_FROM, T_FROM)                      \
          __neon_reinterpret(u8, uint8x16_t, SUFFIX_FROM, T_FROM) __neon_reinterpret(u16, uint16x8_t, SUFFIX_FROM, T_FROM)                 \
              __neon_reinterpret(u32, uint32x4_t, SUFFIX_FROM, T_FROM) __neon_reinterpret(u64, uint64x2_t, SUFFIX_FROM, T_FROM)            \
                  __neon_reinterpret(f32, float32x4_t, SUFFIX_FROM, T_FROM) __neon_reinterpret(f64, float64x2_t, SUFFIX_FROM, T_FROM)

__neon_reinterpret_ALL_FROM(s8, int8x16_t);
__neon_reinterpret_ALL_FROM(s16, int16x8_t);
__neon_reinterpret_ALL_FROM(s32, int32x4_t);
__neon_reinterpret_ALL_FROM(s64, int64x2_t);
__neon_reinterpret_ALL_FROM(u8, uint8x16_t);
__neon_reinterpret_ALL_FROM(u16, uint16x8_t);
__neon_reinterpret_ALL_FROM(u32, uint32x4_t);
__neon_reinterpret_ALL_FROM(u64, uint64x2_t);
__neon_reinterpret_ALL_FROM(f32, float32x4_t);
__neon_reinterpret_ALL_FROM(f64, float64x2_t);

#undef __neon_reinterpret_ALL_FROM
#undef __neon_reinterpret

__inline_g signed char
vgetq_lane_s8(int8x16_t v, const int n) noexcept
{
  return v[n];
}

__inline_g signed short
vgetq_lane_s16(int16x8_t v, const int n) noexcept
{
  return v[n];
}

__inline_g signed int
vgetq_lane_s32(int32x4_t v, const int n) noexcept
{
  return v[n];
}

__inline_g signed long long
vgetq_lane_s64(int64x2_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned char
vgetq_lane_u8(uint8x16_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned short
vgetq_lane_u16(uint16x8_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned int
vgetq_lane_u32(uint32x4_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned long long
vgetq_lane_u64(uint64x2_t v, const int n) noexcept
{
  return v[n];
}

__inline_g float
vgetq_lane_f32(float32x4_t v, const int n) noexcept
{
  return v[n];
}

__inline_g double
vgetq_lane_f64(float64x2_t v, const int n) noexcept
{
  return v[n];
}

__inline_g int8x16_t
vsetq_lane_s8(signed char x, int8x16_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int16x8_t
vsetq_lane_s16(signed short x, int16x8_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int32x4_t
vsetq_lane_s32(signed int x, int32x4_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int64x2_t
vsetq_lane_s64(signed long long x, int64x2_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint8x16_t
vsetq_lane_u8(unsigned char x, uint8x16_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint16x8_t
vsetq_lane_u16(unsigned short x, uint16x8_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint32x4_t
vsetq_lane_u32(unsigned int x, uint32x4_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint64x2_t
vsetq_lane_u64(unsigned long long x, uint64x2_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g float32x4_t
vsetq_lane_f32(float x, float32x4_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g float64x2_t
vsetq_lane_f64(double x, float64x2_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int8x16_t
vbslq_s8(uint8x16_t mask, int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)((mask & (uint8x16_t)a) | (~mask & (uint8x16_t)b));
}

__inline_g int16x8_t
vbslq_s16(uint16x8_t mask, int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)((mask & (uint16x8_t)a) | (~mask & (uint16x8_t)b));
}

__inline_g int32x4_t
vbslq_s32(uint32x4_t mask, int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)((mask & (uint32x4_t)a) | (~mask & (uint32x4_t)b));
}

__inline_g int64x2_t
vbslq_s64(uint64x2_t mask, int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)((mask & (uint64x2_t)a) | (~mask & (uint64x2_t)b));
}

__inline_g uint8x16_t
vbslq_u8(uint8x16_t mask, uint8x16_t a, uint8x16_t b) noexcept
{
  return (mask & a) | (~mask & b);
}

__inline_g uint16x8_t
vbslq_u16(uint16x8_t mask, uint16x8_t a, uint16x8_t b) noexcept
{
  return (mask & a) | (~mask & b);
}

__inline_g uint32x4_t
vbslq_u32(uint32x4_t mask, uint32x4_t a, uint32x4_t b) noexcept
{
  return (mask & a) | (~mask & b);
}

__inline_g uint64x2_t
vbslq_u64(uint64x2_t mask, uint64x2_t a, uint64x2_t b) noexcept
{
  return (mask & a) | (~mask & b);
}

__inline_g float32x4_t
vbslq_f32(uint32x4_t mask, float32x4_t a, float32x4_t b) noexcept
{
  return (float32x4_t)((mask & (uint32x4_t)a) | (~mask & (uint32x4_t)b));
}

__inline_g float64x2_t
vbslq_f64(uint64x2_t mask, float64x2_t a, float64x2_t b) noexcept
{
  return (float64x2_t)((mask & (uint64x2_t)a) | (~mask & (uint64x2_t)b));
}

__inline_g int8x16_t
vabsq_s8(int8x16_t a) noexcept
{
  return (int8x16_t)__builtin_aarch64_absv16qi(a);
}

__inline_g int16x8_t
vabsq_s16(int16x8_t a) noexcept
{
  return (int16x8_t)__builtin_aarch64_absv8hi(a);
}

__inline_g int32x4_t
vabsq_s32(int32x4_t a) noexcept
{
  return (int32x4_t)__builtin_aarch64_absv4si(a);
}

__inline_g int64x2_t
vabsq_s64(int64x2_t a) noexcept
{
  return (int64x2_t)__builtin_aarch64_absv2di(a);
}

__inline_g float32x4_t
vabsq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_absv4sf(a);
}

__inline_g float64x2_t
vabsq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_absv2df(a);
}

__inline_g int8x16_t
vabdq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_sabdv16qi(a, b);
}

__inline_g int16x8_t
vabdq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_sabdv8hi(a, b);
}

__inline_g int32x4_t
vabdq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_sabdv4si(a, b);
}

__inline_g uint8x16_t
vabdq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_uabdv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vabdq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uabdv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vabdq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uabdv4si_uuu(a, b);
}

__inline_g float32x4_t
vabdq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_aarch64_fabdv4sf(a, b);
}

__inline_g float64x2_t
vabdq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_aarch64_fabdv2df(a, b);
}

__inline_g int8x8_t
vabd_s8(int8x8_t a, int8x8_t b) noexcept
{
  return (int8x8_t)__builtin_aarch64_sabdv8qi(a, b);
}

__inline_g int16x4_t
vabd_s16(int16x4_t a, int16x4_t b) noexcept
{
  return (int16x4_t)__builtin_aarch64_sabdv4hi(a, b);
}

__inline_g int32x2_t
vabd_s32(int32x2_t a, int32x2_t b) noexcept
{
  return (int32x2_t)__builtin_aarch64_sabdv2si(a, b);
}

__inline_g uint8x8_t
vabd_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)__builtin_aarch64_uabdv8qi_uuu(a, b);
}

__inline_g uint16x4_t
vabd_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)__builtin_aarch64_uabdv4hi_uuu(a, b);
}

__inline_g uint32x2_t
vabd_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)__builtin_aarch64_uabdv2si_uuu(a, b);
}

__inline_g float32x2_t
vabd_f32(float32x2_t a, float32x2_t b) noexcept
{
  return __builtin_aarch64_fabdv2sf(a, b);
}

__inline_g float
vabds_f32(float a, float b) noexcept
{
  return __builtin_fabsf(a - b);
}

__inline_g double
vabdd_f64(double a, double b) noexcept
{
  return __builtin_fabs(a - b);
}

__inline_g int16x8_t
vabdl_s8(int8x8_t a, int8x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_sabdlv8qi(a, b);
}

__inline_g int32x4_t
vabdl_s16(int16x4_t a, int16x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_sabdlv4hi(a, b);
}

__inline_g int64x2_t
vabdl_s32(int32x2_t a, int32x2_t b) noexcept
{
  return (int64x2_t)__builtin_aarch64_sabdlv2si(a, b);
}

__inline_g uint16x8_t
vabdl_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uabdlv8qi_uuu(a, b);
}

__inline_g uint32x4_t
vabdl_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uabdlv4hi_uuu(a, b);
}

__inline_g uint64x2_t
vabdl_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint64x2_t)__builtin_aarch64_uabdlv2si_uuu(a, b);
}

__inline_g int16x8_t
vabdl_high_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_sabdl2v16qi(a, b);
}

__inline_g int32x4_t
vabdl_high_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_sabdl2v8hi(a, b);
}

__inline_g int64x2_t
vabdl_high_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int64x2_t)__builtin_aarch64_sabdl2v4si(a, b);
}

__inline_g uint16x8_t
vabdl_high_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uabdl2v16qi_uuu(a, b);
}

__inline_g uint32x4_t
vabdl_high_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uabdl2v8hi_uuu(a, b);
}

__inline_g uint64x2_t
vabdl_high_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint64x2_t)__builtin_aarch64_uabdl2v4si_uuu(a, b);
}

__inline_g int8x16_t
vabaq_s8(int8x16_t acc, int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_sabav16qi(acc, a, b);
}

__inline_g int16x8_t
vabaq_s16(int16x8_t acc, int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_sabav8hi(acc, a, b);
}

__inline_g int32x4_t
vabaq_s32(int32x4_t acc, int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_sabav4si(acc, a, b);
}

__inline_g uint8x16_t
vabaq_u8(uint8x16_t acc, uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_uabav16qi_uuuu(acc, a, b);
}

__inline_g uint16x8_t
vabaq_u16(uint16x8_t acc, uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uabav8hi_uuuu(acc, a, b);
}

__inline_g uint32x4_t
vabaq_u32(uint32x4_t acc, uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uabav4si_uuuu(acc, a, b);
}

__inline_g int8x8_t
vaba_s8(int8x8_t acc, int8x8_t a, int8x8_t b) noexcept
{
  return (int8x8_t)__builtin_aarch64_sabav8qi(acc, a, b);
}

__inline_g int16x4_t
vaba_s16(int16x4_t acc, int16x4_t a, int16x4_t b) noexcept
{
  return (int16x4_t)__builtin_aarch64_sabav4hi(acc, a, b);
}

__inline_g int32x2_t
vaba_s32(int32x2_t acc, int32x2_t a, int32x2_t b) noexcept
{
  return (int32x2_t)__builtin_aarch64_sabav2si(acc, a, b);
}

__inline_g uint8x8_t
vaba_u8(uint8x8_t acc, uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)__builtin_aarch64_uabav8qi_uuuu(acc, a, b);
}

__inline_g uint16x4_t
vaba_u16(uint16x4_t acc, uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)__builtin_aarch64_uabav4hi_uuuu(acc, a, b);
}

__inline_g uint32x2_t
vaba_u32(uint32x2_t acc, uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)__builtin_aarch64_uabav2si_uuuu(acc, a, b);
}

__inline_g int16x8_t
vabal_s8(int16x8_t acc, int8x8_t a, int8x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_sabalv8qi(acc, a, b);
}

__inline_g int32x4_t
vabal_s16(int32x4_t acc, int16x4_t a, int16x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_sabalv4hi(acc, a, b);
}

__inline_g int64x2_t
vabal_s32(int64x2_t acc, int32x2_t a, int32x2_t b) noexcept
{
  return (int64x2_t)__builtin_aarch64_sabalv2si(acc, a, b);
}

__inline_g uint16x8_t
vabal_u8(uint16x8_t acc, uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uabalv8qi_uuuu(acc, a, b);
}

__inline_g uint32x4_t
vabal_u16(uint32x4_t acc, uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uabalv4hi_uuuu(acc, a, b);
}

__inline_g uint64x2_t
vabal_u32(uint64x2_t acc, uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint64x2_t)__builtin_aarch64_uabalv2si_uuuu(acc, a, b);
}

__inline_g int8x16_t
vnegq_s8(int8x16_t a) noexcept
{
  return -a;
}

__inline_g int16x8_t
vnegq_s16(int16x8_t a) noexcept
{
  return -a;
}

__inline_g int32x4_t
vnegq_s32(int32x4_t a) noexcept
{
  return -a;
}

__inline_g int64x2_t
vnegq_s64(int64x2_t a) noexcept
{
  return -a;
}

__inline_g float32x4_t
vnegq_f32(float32x4_t a) noexcept
{
  return -a;
}

__inline_g float64x2_t
vnegq_f64(float64x2_t a) noexcept
{
  return -a;
}

__inline_g int8x8_t
vneg_s8(int8x8_t a) noexcept
{
  return -a;
}

__inline_g int16x4_t
vneg_s16(int16x4_t a) noexcept
{
  return -a;
}

__inline_g int32x2_t
vneg_s32(int32x2_t a) noexcept
{
  return -a;
}

__inline_g int64x1_t
vneg_s64(int64x1_t a) noexcept
{
  return -a;
}

__inline_g float32x2_t
vneg_f32(float32x2_t a) noexcept
{
  return -a;
}

__inline_g float64x1_t
vneg_f64(float64x1_t a) noexcept
{
  return -a;
}

__inline_g int64x2_t
vmulq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return a * b;
}

__inline_g uint64x2_t
vmulq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return a * b;
}

__inline_g int8x16_t
vqaddq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_ssaddv16qi(a, b);
}

__inline_g int16x8_t
vqaddq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_ssaddv8hi(a, b);
}

__inline_g int32x4_t
vqaddq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_ssaddv4si(a, b);
}

__inline_g int64x2_t
vqaddq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)__builtin_aarch64_ssaddv2di(a, b);
}

__inline_g uint8x16_t
vqaddq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_usaddv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vqaddq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_usaddv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vqaddq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_usaddv4si_uuu(a, b);
}

__inline_g uint64x2_t
vqaddq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)__builtin_aarch64_usaddv2di_uuu(a, b);
}

__inline_g int8x16_t
vqsubq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_sssubv16qi(a, b);
}

__inline_g int16x8_t
vqsubq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_sssubv8hi(a, b);
}

__inline_g int32x4_t
vqsubq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_sssubv4si(a, b);
}

__inline_g int64x2_t
vqsubq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)__builtin_aarch64_sssubv2di(a, b);
}

__inline_g uint8x16_t
vqsubq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_ussubv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vqsubq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_ussubv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vqsubq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_ussubv4si_uuu(a, b);
}

__inline_g uint64x2_t
vqsubq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)__builtin_aarch64_ussubv2di_uuu(a, b);
}

__inline_g int8x8_t
vqadd_s8(int8x8_t a, int8x8_t b) noexcept
{
  return (int8x8_t)__builtin_aarch64_ssaddv8qi(a, b);
}

__inline_g int16x4_t
vqadd_s16(int16x4_t a, int16x4_t b) noexcept
{
  return (int16x4_t)__builtin_aarch64_ssaddv4hi(a, b);
}

__inline_g int32x2_t
vqadd_s32(int32x2_t a, int32x2_t b) noexcept
{
  return (int32x2_t)__builtin_aarch64_ssaddv2si(a, b);
}

__inline_g uint8x8_t
vqadd_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)__builtin_aarch64_usaddv8qi_uuu(a, b);
}

__inline_g uint16x4_t
vqadd_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)__builtin_aarch64_usaddv4hi_uuu(a, b);
}

__inline_g uint32x2_t
vqadd_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)__builtin_aarch64_usaddv2si_uuu(a, b);
}

__inline_g int8x8_t
vqsub_s8(int8x8_t a, int8x8_t b) noexcept
{
  return (int8x8_t)__builtin_aarch64_sssubv8qi(a, b);
}

__inline_g int16x4_t
vqsub_s16(int16x4_t a, int16x4_t b) noexcept
{
  return (int16x4_t)__builtin_aarch64_sssubv4hi(a, b);
}

__inline_g int32x2_t
vqsub_s32(int32x2_t a, int32x2_t b) noexcept
{
  return (int32x2_t)__builtin_aarch64_sssubv2si(a, b);
}

__inline_g uint8x8_t
vqsub_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)__builtin_aarch64_ussubv8qi_uuu(a, b);
}

__inline_g uint16x4_t
vqsub_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)__builtin_aarch64_ussubv4hi_uuu(a, b);
}

__inline_g uint32x2_t
vqsub_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)__builtin_aarch64_ussubv2si_uuu(a, b);
}

__inline_g int8x16_t
vhaddq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_shaddv16qi(a, b);
}

__inline_g int16x8_t
vhaddq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_shaddv8hi(a, b);
}

__inline_g int32x4_t
vhaddq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_shaddv4si(a, b);
}

__inline_g uint8x16_t
vhaddq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_uhaddv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vhaddq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uhaddv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vhaddq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uhaddv4si_uuu(a, b);
}

__inline_g int8x16_t
vhsubq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_shsubv16qi(a, b);
}

__inline_g int16x8_t
vhsubq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_shsubv8hi(a, b);
}

__inline_g int32x4_t
vhsubq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_shsubv4si(a, b);
}

__inline_g uint8x16_t
vhsubq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_uhsubv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vhsubq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uhsubv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vhsubq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uhsubv4si_uuu(a, b);
}

__inline_g int8x16_t
vrhaddq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return (int8x16_t)__builtin_aarch64_srhaddv16qi(a, b);
}

__inline_g int16x8_t
vrhaddq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return (int16x8_t)__builtin_aarch64_srhaddv8hi(a, b);
}

__inline_g int32x4_t
vrhaddq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return (int32x4_t)__builtin_aarch64_srhaddv4si(a, b);
}

__inline_g uint8x16_t
vrhaddq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)__builtin_aarch64_urhaddv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vrhaddq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_urhaddv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vrhaddq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_urhaddv4si_uuu(a, b);
}

__inline_g signed char
vaddvq_s8(int8x16_t v) noexcept
{
  return (signed char)__builtin_aarch64_reduc_plus_scal_v16qi(v);
}

__inline_g signed short
vaddvq_s16(int16x8_t v) noexcept
{
  return (signed short)__builtin_aarch64_reduc_plus_scal_v8hi(v);
}

__inline_g signed int
vaddvq_s32(int32x4_t v) noexcept
{
  return (signed int)__builtin_aarch64_reduc_plus_scal_v4si(v);
}

__inline_g signed long long
vaddvq_s64(int64x2_t v) noexcept
{
  return (signed long long)__builtin_aarch64_reduc_plus_scal_v2di(v);
}

__inline_g unsigned char
vaddvq_u8(uint8x16_t v) noexcept
{
  return (unsigned char)__builtin_aarch64_reduc_plus_scal_v16qi_uu(v);
}

__inline_g unsigned short
vaddvq_u16(uint16x8_t v) noexcept
{
  return (unsigned short)__builtin_aarch64_reduc_plus_scal_v8hi_uu(v);
}

__inline_g unsigned int
vaddvq_u32(uint32x4_t v) noexcept
{
  return (unsigned int)__builtin_aarch64_reduc_plus_scal_v4si_uu(v);
}

__inline_g unsigned long long
vaddvq_u64(uint64x2_t v) noexcept
{
  return (unsigned long long)__builtin_aarch64_reduc_plus_scal_v2di_uu(v);
}

__inline_g float
vaddvq_f32(float32x4_t v) noexcept
{
  return __builtin_aarch64_reduc_plus_scal_v4sf(v);
}

__inline_g double
vaddvq_f64(float64x2_t v) noexcept
{
  return __builtin_aarch64_reduc_plus_scal_v2df(v);
}

__inline_g signed char
vmaxvq_s8(int8x16_t v) noexcept
{
  return (signed char)__builtin_aarch64_reduc_smax_scal_v16qi(v);
}

__inline_g signed short
vmaxvq_s16(int16x8_t v) noexcept
{
  return (signed short)__builtin_aarch64_reduc_smax_scal_v8hi(v);
}

__inline_g signed int
vmaxvq_s32(int32x4_t v) noexcept
{
  return (signed int)__builtin_aarch64_reduc_smax_scal_v4si(v);
}

__inline_g unsigned char
vmaxvq_u8(uint8x16_t v) noexcept
{
  return (unsigned char)__builtin_aarch64_reduc_umax_scal_v16qi_uu(v);
}

__inline_g unsigned short
vmaxvq_u16(uint16x8_t v) noexcept
{
  return (unsigned short)__builtin_aarch64_reduc_umax_scal_v8hi_uu(v);
}

__inline_g unsigned int
vmaxvq_u32(uint32x4_t v) noexcept
{
  return (unsigned int)__builtin_aarch64_reduc_umax_scal_v4si_uu(v);
}

__inline_g float
vmaxvq_f32(float32x4_t v) noexcept
{
  return __builtin_aarch64_reduc_smax_nan_scal_v4sf(v);
}

__inline_g double
vmaxvq_f64(float64x2_t v) noexcept
{
  return __builtin_aarch64_reduc_smax_nan_scal_v2df(v);
}

__inline_g signed char
vminvq_s8(int8x16_t v) noexcept
{
  return (signed char)__builtin_aarch64_reduc_smin_scal_v16qi(v);
}

__inline_g signed short
vminvq_s16(int16x8_t v) noexcept
{
  return (signed short)__builtin_aarch64_reduc_smin_scal_v8hi(v);
}

__inline_g signed int
vminvq_s32(int32x4_t v) noexcept
{
  return (signed int)__builtin_aarch64_reduc_smin_scal_v4si(v);
}

__inline_g unsigned char
vminvq_u8(uint8x16_t v) noexcept
{
  return (unsigned char)__builtin_aarch64_reduc_umin_scal_v16qi_uu(v);
}

__inline_g unsigned short
vminvq_u16(uint16x8_t v) noexcept
{
  return (unsigned short)__builtin_aarch64_reduc_umin_scal_v8hi_uu(v);
}

__inline_g unsigned int
vminvq_u32(uint32x4_t v) noexcept
{
  return (unsigned int)__builtin_aarch64_reduc_umin_scal_v4si_uu(v);
}

__inline_g float
vminvq_f32(float32x4_t v) noexcept
{
  return __builtin_aarch64_reduc_smin_nan_scal_v4sf(v);
}

__inline_g double
vminvq_f64(float64x2_t v) noexcept
{
  return __builtin_aarch64_reduc_smin_nan_scal_v2df(v);
}

__inline_g float32x4_t
vsqrtq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_sqrtv4sf(a);
}

__inline_g float64x2_t
vsqrtq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_sqrtv2df(a);
}

__inline_g float32x2_t
vsqrt_f32(float32x2_t a) noexcept
{
  return __builtin_aarch64_sqrtv2sf(a);
}

__inline_g float32x4_t
vrndq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_btruncv4sf(a);
}

__inline_g float64x2_t
vrndq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_btruncv2df(a);
}

__inline_g float32x4_t
vrndmq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_floorv4sf(a);
}

__inline_g float64x2_t
vrndmq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_floorv2df(a);
}

__inline_g float32x4_t
vrndpq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_ceilv4sf(a);
}

__inline_g float64x2_t
vrndpq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_ceilv2df(a);
}

__inline_g float32x4_t
vrndaq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_roundv4sf(a);
}

__inline_g float64x2_t
vrndaq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_roundv2df(a);
}

__inline_g float32x4_t
vrndnq_f32(float32x4_t a) noexcept
{
  float32x4_t r;
  __asm__("frintn %0.4s, %1.4s" : "=w"(r) : "w"(a));
  return r;
}

__inline_g float64x2_t
vrndnq_f64(float64x2_t a) noexcept
{
  float64x2_t r;
  __asm__("frintn %0.2d, %1.2d" : "=w"(r) : "w"(a));
  return r;
}

__inline_g float32x4_t
vrndiq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_nearbyintv4sf(a);
}

__inline_g float64x2_t
vrndiq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_nearbyintv2df(a);
}

__inline_g float32x4_t
vrndxq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_rintv4sf(a);
}

__inline_g float64x2_t
vrndxq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_rintv2df(a);
}

__inline_g int16x8_t
vmovl_s8(int8x8_t v) noexcept
{
  return __builtin_convertvector(v, int16x8_t);
}

__inline_g int32x4_t
vmovl_s16(int16x4_t v) noexcept
{
  return __builtin_convertvector(v, int32x4_t);
}

__inline_g int64x2_t
vmovl_s32(int32x2_t v) noexcept
{
  return __builtin_convertvector(v, int64x2_t);
}

__inline_g uint16x8_t
vmovl_u8(uint8x8_t v) noexcept
{
  return __builtin_convertvector(v, uint16x8_t);
}

__inline_g uint32x4_t
vmovl_u16(uint16x4_t v) noexcept
{
  return __builtin_convertvector(v, uint32x4_t);
}

__inline_g uint64x2_t
vmovl_u32(uint32x2_t v) noexcept
{
  return __builtin_convertvector(v, uint64x2_t);
}

__inline_g int16x8_t
vmovl_high_s8(int8x16_t v) noexcept
{
  return __builtin_convertvector(__builtin_shufflevector(v, v, 8, 9, 10, 11, 12, 13, 14, 15), int16x8_t);
}

__inline_g int32x4_t
vmovl_high_s16(int16x8_t v) noexcept
{
  return __builtin_convertvector(__builtin_shufflevector(v, v, 4, 5, 6, 7), int32x4_t);
}

__inline_g int64x2_t
vmovl_high_s32(int32x4_t v) noexcept
{
  return __builtin_convertvector(__builtin_shufflevector(v, v, 2, 3), int64x2_t);
}

__inline_g uint16x8_t
vmovl_high_u8(uint8x16_t v) noexcept
{
  return __builtin_convertvector(__builtin_shufflevector(v, v, 8, 9, 10, 11, 12, 13, 14, 15), uint16x8_t);
}

__inline_g uint32x4_t
vmovl_high_u16(uint16x8_t v) noexcept
{
  return __builtin_convertvector(__builtin_shufflevector(v, v, 4, 5, 6, 7), uint32x4_t);
}

__inline_g uint64x2_t
vmovl_high_u32(uint32x4_t v) noexcept
{
  return __builtin_convertvector(__builtin_shufflevector(v, v, 2, 3), uint64x2_t);
}

__inline_g int8x8_t
vmovn_s16(int16x8_t v) noexcept
{
  return __builtin_convertvector(v, int8x8_t);
}

__inline_g int16x4_t
vmovn_s32(int32x4_t v) noexcept
{
  return __builtin_convertvector(v, int16x4_t);
}

__inline_g int32x2_t
vmovn_s64(int64x2_t v) noexcept
{
  return __builtin_convertvector(v, int32x2_t);
}

__inline_g uint8x8_t
vmovn_u16(uint16x8_t v) noexcept
{
  return __builtin_convertvector(v, uint8x8_t);
}

__inline_g uint16x4_t
vmovn_u32(uint32x4_t v) noexcept
{
  return __builtin_convertvector(v, uint16x4_t);
}

__inline_g uint32x2_t
vmovn_u64(uint64x2_t v) noexcept
{
  return __builtin_convertvector(v, uint32x2_t);
}

__inline_g int8x16_t
vmovn_high_s16(int8x8_t lo, int16x8_t v) noexcept
{
  return __builtin_shufflevector(lo, __builtin_convertvector(v, int8x8_t), 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
}

__inline_g int16x8_t
vmovn_high_s32(int16x4_t lo, int32x4_t v) noexcept
{
  return __builtin_shufflevector(lo, __builtin_convertvector(v, int16x4_t), 0, 1, 2, 3, 4, 5, 6, 7);
}

__inline_g int32x4_t
vmovn_high_s64(int32x2_t lo, int64x2_t v) noexcept
{
  return __builtin_shufflevector(lo, __builtin_convertvector(v, int32x2_t), 0, 1, 2, 3);
}

__inline_g uint8x16_t
vmovn_high_u16(uint8x8_t lo, uint16x8_t v) noexcept
{
  return __builtin_shufflevector(lo, __builtin_convertvector(v, uint8x8_t), 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
}

__inline_g uint16x8_t
vmovn_high_u32(uint16x4_t lo, uint32x4_t v) noexcept
{
  return __builtin_shufflevector(lo, __builtin_convertvector(v, uint16x4_t), 0, 1, 2, 3, 4, 5, 6, 7);
}

__inline_g uint32x4_t
vmovn_high_u64(uint32x2_t lo, uint64x2_t v) noexcept
{
  return __builtin_shufflevector(lo, __builtin_convertvector(v, uint32x2_t), 0, 1, 2, 3);
}

__inline_g int16x8_t
vmull_s8(int8x8_t a, int8x8_t b) noexcept
{
  return __builtin_convertvector(a, int16x8_t) * __builtin_convertvector(b, int16x8_t);
}

__inline_g int32x4_t
vmull_s16(int16x4_t a, int16x4_t b) noexcept
{
  return __builtin_convertvector(a, int32x4_t) * __builtin_convertvector(b, int32x4_t);
}

__inline_g int64x2_t
vmull_s32(int32x2_t a, int32x2_t b) noexcept
{
  return __builtin_convertvector(a, int64x2_t) * __builtin_convertvector(b, int64x2_t);
}

__inline_g uint16x8_t
vmull_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return __builtin_convertvector(a, uint16x8_t) * __builtin_convertvector(b, uint16x8_t);
}

__inline_g uint32x4_t
vmull_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return __builtin_convertvector(a, uint32x4_t) * __builtin_convertvector(b, uint32x4_t);
}

__inline_g uint64x2_t
vmull_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return __builtin_convertvector(a, uint64x2_t) * __builtin_convertvector(b, uint64x2_t);
}

__inline_g int8x8_t
vqmovn_s16(int16x8_t v) noexcept
{
  return (int8x8_t)__builtin_aarch64_sqmovnv8hi(v);
}

__inline_g int16x4_t
vqmovn_s32(int32x4_t v) noexcept
{
  return (int16x4_t)__builtin_aarch64_sqmovnv4si(v);
}

__inline_g int32x2_t
vqmovn_s64(int64x2_t v) noexcept
{
  return (int32x2_t)__builtin_aarch64_sqmovnv2di(v);
}

__inline_g uint8x8_t
vqmovn_u16(uint16x8_t v) noexcept
{
  uint8x8_t r;
  __asm__("uqxtn %0.8b, %1.8h" : "=w"(r) : "w"(v));
  return r;
}

__inline_g uint16x4_t
vqmovn_u32(uint32x4_t v) noexcept
{
  uint16x4_t r;
  __asm__("uqxtn %0.4h, %1.4s" : "=w"(r) : "w"(v));
  return r;
}

__inline_g uint32x2_t
vqmovn_u64(uint64x2_t v) noexcept
{
  uint32x2_t r;
  __asm__("uqxtn %0.2s, %1.2d" : "=w"(r) : "w"(v));
  return r;
}

__inline_g uint8x8_t
vqmovun_s16(int16x8_t v) noexcept
{
  return (uint8x8_t)__builtin_aarch64_sqmovunv8hi_us(v);
}

__inline_g uint16x4_t
vqmovun_s32(int32x4_t v) noexcept
{
  return (uint16x4_t)__builtin_aarch64_sqmovunv4si_us(v);
}

__inline_g uint32x2_t
vqmovun_s64(int64x2_t v) noexcept
{
  return (uint32x2_t)__builtin_aarch64_sqmovunv2di_us(v);
}

__inline_g float32x4_t
vrecpeq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_frecpev4sf(a);
}

__inline_g float64x2_t
vrecpeq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_frecpev2df(a);
}

__inline_g float32x4_t
vrecpsq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_aarch64_frecpsv4sf(a, b);
}

__inline_g float64x2_t
vrecpsq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_aarch64_frecpsv2df(a, b);
}

__inline_g float32x4_t
vrsqrteq_f32(float32x4_t a) noexcept
{
  return __builtin_aarch64_rsqrtev4sf(a);
}

__inline_g float64x2_t
vrsqrteq_f64(float64x2_t a) noexcept
{
  return __builtin_aarch64_rsqrtev2df(a);
}

__inline_g float32x4_t
vrsqrtsq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_aarch64_rsqrtsv4sf(a, b);
}

__inline_g float64x2_t
vrsqrtsq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_aarch64_rsqrtsv2df(a, b);
}

__inline_g int8x16_t
vbicq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return a & ~b;
}

__inline_g int16x8_t
vbicq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return a & ~b;
}

__inline_g int32x4_t
vbicq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return a & ~b;
}

__inline_g int64x2_t
vbicq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return a & ~b;
}

__inline_g int8x16_t
vmvnq_s8(int8x16_t a) noexcept
{
  return ~a;
}

__inline_g int16x8_t
vmvnq_s16(int16x8_t a) noexcept
{
  return ~a;
}

__inline_g int32x4_t
vmvnq_s32(int32x4_t a) noexcept
{
  return ~a;
}

__inline_g float32x4_t
vorrq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (float32x4_t)((uint32x4_t)a | (uint32x4_t)b);
}

__inline_g float64x2_t
vorrq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (float64x2_t)((uint64x2_t)a | (uint64x2_t)b);
}

__inline_g int8x16_t
vextq_s8(int8x16_t a, int8x16_t b, const int n) noexcept
{
  switch ( n & 15 ) {
  case 0:
    return a;
  case 1:
    return __builtin_shufflevector(a, b, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  case 2:
    return __builtin_shufflevector(a, b, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17);
  case 3:
    return __builtin_shufflevector(a, b, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18);
  case 4:
    return __builtin_shufflevector(a, b, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19);
  case 5:
    return __builtin_shufflevector(a, b, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
  case 6:
    return __builtin_shufflevector(a, b, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21);
  case 7:
    return __builtin_shufflevector(a, b, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22);
  case 8:
    return __builtin_shufflevector(a, b, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23);
  case 9:
    return __builtin_shufflevector(a, b, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24);
  case 10:
    return __builtin_shufflevector(a, b, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25);
  case 11:
    return __builtin_shufflevector(a, b, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26);
  case 12:
    return __builtin_shufflevector(a, b, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27);
  case 13:
    return __builtin_shufflevector(a, b, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28);
  case 14:
    return __builtin_shufflevector(a, b, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29);
  case 15:
    return __builtin_shufflevector(a, b, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30);
  default:
    return a;
  }
}

__inline_g uint8x16_t
vextq_u8(uint8x16_t a, uint8x16_t b, const int n) noexcept
{
  return (uint8x16_t)vextq_s8((int8x16_t)a, (int8x16_t)b, n);
}

__inline_g int16x8_t
vextq_s16(int16x8_t a, int16x8_t b, const int n) noexcept
{
  switch ( n & 7 ) {
  case 0:
    return a;
  case 1:
    return __builtin_shufflevector(a, b, 1, 2, 3, 4, 5, 6, 7, 8);
  case 2:
    return __builtin_shufflevector(a, b, 2, 3, 4, 5, 6, 7, 8, 9);
  case 3:
    return __builtin_shufflevector(a, b, 3, 4, 5, 6, 7, 8, 9, 10);
  case 4:
    return __builtin_shufflevector(a, b, 4, 5, 6, 7, 8, 9, 10, 11);
  case 5:
    return __builtin_shufflevector(a, b, 5, 6, 7, 8, 9, 10, 11, 12);
  case 6:
    return __builtin_shufflevector(a, b, 6, 7, 8, 9, 10, 11, 12, 13);
  case 7:
    return __builtin_shufflevector(a, b, 7, 8, 9, 10, 11, 12, 13, 14);
  default:
    return a;
  }
}

__inline_g uint16x8_t
vextq_u16(uint16x8_t a, uint16x8_t b, const int n) noexcept
{
  return (uint16x8_t)vextq_s16((int16x8_t)a, (int16x8_t)b, n);
}

__inline_g int32x4_t
vextq_s32(int32x4_t a, int32x4_t b, const int n) noexcept
{
  switch ( n & 3 ) {
  case 0:
    return a;
  case 1:
    return __builtin_shufflevector(a, b, 1, 2, 3, 4);
  case 2:
    return __builtin_shufflevector(a, b, 2, 3, 4, 5);
  case 3:
    return __builtin_shufflevector(a, b, 3, 4, 5, 6);
  default:
    return a;
  }
}

__inline_g uint32x4_t
vextq_u32(uint32x4_t a, uint32x4_t b, const int n) noexcept
{
  return (uint32x4_t)vextq_s32((int32x4_t)a, (int32x4_t)b, n);
}

__inline_g float32x4_t
vextq_f32(float32x4_t a, float32x4_t b, const int n) noexcept
{
  return (float32x4_t)vextq_s32((int32x4_t)a, (int32x4_t)b, n);
}

__inline_g int64x2_t
vextq_s64(int64x2_t a, int64x2_t b, const int n) noexcept
{
  return (n & 1) ? __builtin_shufflevector(a, b, 1, 2) : a;
}

__inline_g uint64x2_t
vextq_u64(uint64x2_t a, uint64x2_t b, const int n) noexcept
{
  return (n & 1) ? __builtin_shufflevector(a, b, 1, 2) : a;
}

__inline_g float64x2_t
vextq_f64(float64x2_t a, float64x2_t b, const int n) noexcept
{
  return (n & 1) ? __builtin_shufflevector(a, b, 1, 2) : a;
}

__inline_g signed char
vget_lane_s8(int8x8_t v, const int n) noexcept
{
  return v[n];
}

__inline_g signed short
vget_lane_s16(int16x4_t v, const int n) noexcept
{
  return v[n];
}

__inline_g signed int
vget_lane_s32(int32x2_t v, const int n) noexcept
{
  return v[n];
}

__inline_g signed long long
vget_lane_s64(int64x1_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned char
vget_lane_u8(uint8x8_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned short
vget_lane_u16(uint16x4_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned int
vget_lane_u32(uint32x2_t v, const int n) noexcept
{
  return v[n];
}

__inline_g unsigned long long
vget_lane_u64(uint64x1_t v, const int n) noexcept
{
  return v[n];
}

__inline_g float
vget_lane_f32(float32x2_t v, const int n) noexcept
{
  return v[n];
}

__inline_g double
vget_lane_f64(float64x1_t v, const int n) noexcept
{
  return v[n];
}

__inline_g int8x8_t
vset_lane_s8(signed char x, int8x8_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int16x4_t
vset_lane_s16(signed short x, int16x4_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int32x2_t
vset_lane_s32(signed int x, int32x2_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int64x1_t
vset_lane_s64(signed long long x, int64x1_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint8x8_t
vset_lane_u8(unsigned char x, uint8x8_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint16x4_t
vset_lane_u16(unsigned short x, uint16x4_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint32x2_t
vset_lane_u32(unsigned int x, uint32x2_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g uint64x1_t
vset_lane_u64(unsigned long long x, uint64x1_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g float32x2_t
vset_lane_f32(float x, float32x2_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g float64x1_t
vset_lane_f64(double x, float64x1_t v, const int n) noexcept
{
  v[n] = x;
  return v;
}

__inline_g int8x16_t
vpaddq_s8(int8x16_t a, int8x16_t b) noexcept
{
  int8x16_t lo = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
  int8x16_t hi = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
  return (int8x16_t)(lo + hi);
}

__inline_g int16x8_t
vpaddq_s16(int16x8_t a, int16x8_t b) noexcept
{
  int16x8_t lo = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
  int16x8_t hi = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
  return (int16x8_t)(lo + hi);
}

__inline_g int32x4_t
vpaddq_s32(int32x4_t a, int32x4_t b) noexcept
{
  int32x4_t lo = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  int32x4_t hi = __builtin_shufflevector(a, b, 1, 3, 5, 7);
  return (int32x4_t)(lo + hi);
}

__inline_g int64x2_t
vpaddq_s64(int64x2_t a, int64x2_t b) noexcept
{
  int64x2_t lo = __builtin_shufflevector(a, b, 0, 2);
  int64x2_t hi = __builtin_shufflevector(a, b, 1, 3);
  return (int64x2_t)(lo + hi);
}

__inline_g uint8x16_t
vpaddq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)vpaddq_s8((int8x16_t)a, (int8x16_t)b);
}

__inline_g uint16x8_t
vpaddq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)vpaddq_s16((int16x8_t)a, (int16x8_t)b);
}

__inline_g uint32x4_t
vpaddq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)vpaddq_s32((int32x4_t)a, (int32x4_t)b);
}

__inline_g uint64x2_t
vpaddq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)vpaddq_s64((int64x2_t)a, (int64x2_t)b);
}

__inline_g float32x4_t
vpaddq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x4_t lo = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  float32x4_t hi = __builtin_shufflevector(a, b, 1, 3, 5, 7);
  return lo + hi;
}

__inline_g float64x2_t
vpaddq_f64(float64x2_t a, float64x2_t b) noexcept
{
  float64x2_t lo = __builtin_shufflevector(a, b, 0, 2);
  float64x2_t hi = __builtin_shufflevector(a, b, 1, 3);
  return lo + hi;
}

__inline_g int8x8_t
vpadd_s8(int8x8_t a, int8x8_t b) noexcept
{
  int8x8_t lo = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
  int8x8_t hi = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
  return (int8x8_t)(lo + hi);
}

__inline_g int16x4_t
vpadd_s16(int16x4_t a, int16x4_t b) noexcept
{
  int16x4_t lo = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  int16x4_t hi = __builtin_shufflevector(a, b, 1, 3, 5, 7);
  return (int16x4_t)(lo + hi);
}

__inline_g int32x2_t
vpadd_s32(int32x2_t a, int32x2_t b) noexcept
{
  int32x2_t lo = __builtin_shufflevector(a, b, 0, 2);
  int32x2_t hi = __builtin_shufflevector(a, b, 1, 3);
  return (int32x2_t)(lo + hi);
}

__inline_g uint8x8_t
vpadd_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  return (uint8x8_t)vpadd_s8((int8x8_t)a, (int8x8_t)b);
}

__inline_g uint16x4_t
vpadd_u16(uint16x4_t a, uint16x4_t b) noexcept
{
  return (uint16x4_t)vpadd_s16((int16x4_t)a, (int16x4_t)b);
}

__inline_g uint32x2_t
vpadd_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  return (uint32x2_t)vpadd_s32((int32x2_t)a, (int32x2_t)b);
}

__inline_g float32x2_t
vpadd_f32(float32x2_t a, float32x2_t b) noexcept
{
  float32x2_t lo = __builtin_shufflevector(a, b, 0, 2);
  float32x2_t hi = __builtin_shufflevector(a, b, 1, 3);
  return lo + hi;
}

__inline_g int16x8_t
vpaddlq_s8(int8x16_t v) noexcept
{
  int16x8_t lo = __builtin_convertvector(__builtin_shufflevector(v, v, 0, 2, 4, 6, 8, 10, 12, 14), int16x8_t);
  int16x8_t hi = __builtin_convertvector(__builtin_shufflevector(v, v, 1, 3, 5, 7, 9, 11, 13, 15), int16x8_t);
  return lo + hi;
}

__inline_g int32x4_t
vpaddlq_s16(int16x8_t v) noexcept
{
  int32x4_t lo = __builtin_convertvector(__builtin_shufflevector(v, v, 0, 2, 4, 6), int32x4_t);
  int32x4_t hi = __builtin_convertvector(__builtin_shufflevector(v, v, 1, 3, 5, 7), int32x4_t);
  return lo + hi;
}

__inline_g int64x2_t
vpaddlq_s32(int32x4_t v) noexcept
{
  int64x2_t lo = __builtin_convertvector(__builtin_shufflevector(v, v, 0, 2), int64x2_t);
  int64x2_t hi = __builtin_convertvector(__builtin_shufflevector(v, v, 1, 3), int64x2_t);
  return lo + hi;
}

__inline_g uint16x8_t
vpaddlq_u8(uint8x16_t v) noexcept
{
  uint16x8_t lo = __builtin_convertvector(__builtin_shufflevector(v, v, 0, 2, 4, 6, 8, 10, 12, 14), uint16x8_t);
  uint16x8_t hi = __builtin_convertvector(__builtin_shufflevector(v, v, 1, 3, 5, 7, 9, 11, 13, 15), uint16x8_t);
  return lo + hi;
}

__inline_g uint32x4_t
vpaddlq_u16(uint16x8_t v) noexcept
{
  uint32x4_t lo = __builtin_convertvector(__builtin_shufflevector(v, v, 0, 2, 4, 6), uint32x4_t);
  uint32x4_t hi = __builtin_convertvector(__builtin_shufflevector(v, v, 1, 3, 5, 7), uint32x4_t);
  return lo + hi;
}

__inline_g uint64x2_t
vpaddlq_u32(uint32x4_t v) noexcept
{
  uint64x2_t lo = __builtin_convertvector(__builtin_shufflevector(v, v, 0, 2), uint64x2_t);
  uint64x2_t hi = __builtin_convertvector(__builtin_shufflevector(v, v, 1, 3), uint64x2_t);
  return lo + hi;
}

#define __mc_vpadalq(SUF, ASM, T_ACC, T_IN, LA, LB)                                                                                         \
  __inline_g T_ACC vpadalq_##SUF(T_ACC a, T_IN b) noexcept                                                                                  \
  {                                                                                                                                         \
    T_ACC r = a;                                                                                                                            \
    __asm__(ASM " %0." LA ", %1." LB : "+w"(r) : "w"(b));                                                                                   \
    return r;                                                                                                                               \
  }

__mc_vpadalq(s8, "sadalp", int16x8_t, int8x16_t, "8h", "16b");
__mc_vpadalq(s16, "sadalp", int32x4_t, int16x8_t, "4s", "8h");
__mc_vpadalq(s32, "sadalp", int64x2_t, int32x4_t, "2d", "4s");
__mc_vpadalq(u8, "uadalp", uint16x8_t, uint8x16_t, "8h", "16b");
__mc_vpadalq(u16, "uadalp", uint32x4_t, uint16x8_t, "4s", "8h");
__mc_vpadalq(u32, "uadalp", uint64x2_t, uint32x4_t, "2d", "4s");

#undef __mc_vpadalq

__inline_g int8x16_t
vrev16q_s8(int8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
}

__inline_g uint8x16_t
vrev16q_u8(uint8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
}

__inline_g int8x16_t
vrev32q_s8(int8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
}

__inline_g uint8x16_t
vrev32q_u8(uint8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
}

__inline_g int16x8_t
vrev32q_s16(int16x8_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2, 5, 4, 7, 6);
}

__inline_g uint16x8_t
vrev32q_u16(uint16x8_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2, 5, 4, 7, 6);
}

__inline_g int8x16_t
vrev64q_s8(int8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
}

__inline_g uint8x16_t
vrev64q_u8(uint8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
}

__inline_g int16x8_t
vrev64q_s16(int16x8_t v) noexcept
{
  return __builtin_shufflevector(v, v, 3, 2, 1, 0, 7, 6, 5, 4);
}

__inline_g uint16x8_t
vrev64q_u16(uint16x8_t v) noexcept
{
  return __builtin_shufflevector(v, v, 3, 2, 1, 0, 7, 6, 5, 4);
}

__inline_g int32x4_t
vrev64q_s32(int32x4_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2);
}

__inline_g uint32x4_t
vrev64q_u32(uint32x4_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2);
}

__inline_g float32x4_t
vrev64q_f32(float32x4_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2);
}

__inline_g int8x16_t
vzip1q_s8(int8x16_t a, int8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
}

__inline_g int8x16_t
vzip2q_s8(int8x16_t a, int8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
}

__inline_g uint8x16_t
vzip1q_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
}

__inline_g uint8x16_t
vzip2q_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
}

__inline_g int16x8_t
vzip1q_s16(int16x8_t a, int16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 8, 1, 9, 2, 10, 3, 11);
}

__inline_g int16x8_t
vzip2q_s16(int16x8_t a, int16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 4, 12, 5, 13, 6, 14, 7, 15);
}

__inline_g uint16x8_t
vzip1q_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 8, 1, 9, 2, 10, 3, 11);
}

__inline_g uint16x8_t
vzip2q_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 4, 12, 5, 13, 6, 14, 7, 15);
}

__inline_g int32x4_t
vzip1q_s32(int32x4_t a, int32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 4, 1, 5);
}

__inline_g int32x4_t
vzip2q_s32(int32x4_t a, int32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 2, 6, 3, 7);
}

__inline_g uint32x4_t
vzip1q_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 4, 1, 5);
}

__inline_g uint32x4_t
vzip2q_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 2, 6, 3, 7);
}

__inline_g float32x4_t
vzip1q_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 4, 1, 5);
}

__inline_g float32x4_t
vzip2q_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 2, 6, 3, 7);
}

__inline_g int64x2_t
vzip1q_s64(int64x2_t a, int64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2);
}

__inline_g int64x2_t
vzip2q_s64(int64x2_t a, int64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3);
}

__inline_g uint64x2_t
vzip1q_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2);
}

__inline_g uint64x2_t
vzip2q_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3);
}

__inline_g float64x2_t
vzip1q_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2);
}

__inline_g float64x2_t
vzip2q_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3);
}

__inline_g int8x16_t
vuzp1q_s8(int8x16_t a, int8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
}

__inline_g int8x16_t
vuzp2q_s8(int8x16_t a, int8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
}

__inline_g uint8x16_t
vuzp1q_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
}

__inline_g uint8x16_t
vuzp2q_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
}

__inline_g int16x8_t
vuzp1q_s16(int16x8_t a, int16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
}

__inline_g int16x8_t
vuzp2q_s16(int16x8_t a, int16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
}

__inline_g uint16x8_t
vuzp1q_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
}

__inline_g uint16x8_t
vuzp2q_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
}

__inline_g int32x4_t
vuzp1q_s32(int32x4_t a, int32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2, 4, 6);
}

__inline_g int32x4_t
vuzp2q_s32(int32x4_t a, int32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3, 5, 7);
}

__inline_g uint32x4_t
vuzp1q_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2, 4, 6);
}

__inline_g uint32x4_t
vuzp2q_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3, 5, 7);
}

__inline_g float32x4_t
vuzp1q_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 2, 4, 6);
}

__inline_g float32x4_t
vuzp2q_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 3, 5, 7);
}

// D-register (64-bit) combined unzips. AArch64 has no struct-returning VUZP, only
// UZP1/UZP2, but the ACLE vuzp_* form (val[0]=even lanes, val[1]=odd lanes) is
// what the half-width arith paths (mul_32_64 / mul_u32_64 / maddubs_8) call.
__inline_g int32x2x2_t
vuzp_s32(int32x2_t a, int32x2_t b) noexcept
{
  int32x2x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3);
  return r;
}

__inline_g uint32x2x2_t
vuzp_u32(uint32x2_t a, uint32x2_t b) noexcept
{
  uint32x2x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3);
  return r;
}

__inline_g int16x4x2_t
vuzp_s16(int16x4_t a, int16x4_t b) noexcept
{
  int16x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7);
  return r;
}

__inline_g int8x16_t
vtrn1q_s8(int8x16_t a, int8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 16, 2, 18, 4, 20, 6, 22, 8, 24, 10, 26, 12, 28, 14, 30);
}

__inline_g int8x16_t
vtrn2q_s8(int8x16_t a, int8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 17, 3, 19, 5, 21, 7, 23, 9, 25, 11, 27, 13, 29, 15, 31);
}

__inline_g uint8x16_t
vtrn1q_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 16, 2, 18, 4, 20, 6, 22, 8, 24, 10, 26, 12, 28, 14, 30);
}

__inline_g uint8x16_t
vtrn2q_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 17, 3, 19, 5, 21, 7, 23, 9, 25, 11, 27, 13, 29, 15, 31);
}

__inline_g int16x8_t
vtrn1q_s16(int16x8_t a, int16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 8, 2, 10, 4, 12, 6, 14);
}

__inline_g int16x8_t
vtrn2q_s16(int16x8_t a, int16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 9, 3, 11, 5, 13, 7, 15);
}

__inline_g uint16x8_t
vtrn1q_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 8, 2, 10, 4, 12, 6, 14);
}

__inline_g uint16x8_t
vtrn2q_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 9, 3, 11, 5, 13, 7, 15);
}

__inline_g int32x4_t
vtrn1q_s32(int32x4_t a, int32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 4, 2, 6);
}

__inline_g int32x4_t
vtrn2q_s32(int32x4_t a, int32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 5, 3, 7);
}

__inline_g uint32x4_t
vtrn1q_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 4, 2, 6);
}

__inline_g uint32x4_t
vtrn2q_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 5, 3, 7);
}

__inline_g float32x4_t
vtrn1q_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 0, 4, 2, 6);
}

__inline_g float32x4_t
vtrn2q_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_shufflevector(a, b, 1, 5, 3, 7);
}

// vshlq_* must lower to the hardware SSHL/USHL (NOT the GCC vector `<<`):
// a negative per-lane count is a RIGHT shift (arithmetic for signed, logical for
// unsigned) and |count| >= lane width yields 0. The `<<` operator makes negative
// counts UB (garbage), but every right-shift caller passes vdupq_n(-count) and
// relies on the real SSHL/USHL semantics. Inline asm guarantees the true op.
#define __mc_vshlq_signed(SUF, LAY, T)                                                                                                     \
  __inline_g T vshlq_##SUF(T v, T cnt) noexcept                                                                                            \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("sshl %0." LAY ", %1." LAY ", %2." LAY : "=w"(r) : "w"(v), "w"(cnt));                                                          \
    return r;                                                                                                                              \
  }

__mc_vshlq_signed(s8, "16b", int8x16_t);
__mc_vshlq_signed(s16, "8h", int16x8_t);
__mc_vshlq_signed(s32, "4s", int32x4_t);
__mc_vshlq_signed(s64, "2d", int64x2_t);

#undef __mc_vshlq_signed

#define __mc_vshlq_unsigned(SUF, LAY, T, CT)                                                                                               \
  __inline_g T vshlq_##SUF(T v, CT cnt) noexcept                                                                                           \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("ushl %0." LAY ", %1." LAY ", %2." LAY : "=w"(r) : "w"(v), "w"(cnt));                                                          \
    return r;                                                                                                                              \
  }

__mc_vshlq_unsigned(u8, "16b", uint8x16_t, int8x16_t);
__mc_vshlq_unsigned(u16, "8h", uint16x8_t, int16x8_t);
__mc_vshlq_unsigned(u32, "4s", uint32x4_t, int32x4_t);
__mc_vshlq_unsigned(u64, "2d", uint64x2_t, int64x2_t);

#undef __mc_vshlq_unsigned

__inline_g int8x8_t
vshrn_n_s16(int16x8_t v, const int n) noexcept
{
  return __builtin_convertvector(v >> (int16x8_t)vdupq_n_s16((signed short)n), int8x8_t);
}

__inline_g int16x4_t
vshrn_n_s32(int32x4_t v, const int n) noexcept
{
  return __builtin_convertvector(v >> (int32x4_t)vdupq_n_s32(n), int16x4_t);
}

__inline_g int32x2_t
vshrn_n_s64(int64x2_t v, const int n) noexcept
{
  return __builtin_convertvector(v >> (int64x2_t)vdupq_n_s64((signed long long)n), int32x2_t);
}

__inline_g uint8x8_t
vshrn_n_u16(uint16x8_t v, const int n) noexcept
{
  return __builtin_convertvector(v >> (uint16x8_t)vdupq_n_u16((unsigned short)n), uint8x8_t);
}

__inline_g uint16x4_t
vshrn_n_u32(uint32x4_t v, const int n) noexcept
{
  return __builtin_convertvector(v >> (uint32x4_t)vdupq_n_u32((unsigned)n), uint16x4_t);
}

__inline_g uint32x2_t
vshrn_n_u64(uint64x2_t v, const int n) noexcept
{
  return __builtin_convertvector(v >> (uint64x2_t)vdupq_n_u64((unsigned long long)n), uint32x2_t);
}

__inline_g uint16x4_t
vreinterpret_u16_u8(uint8x8_t a) noexcept
{
  return (uint16x4_t)a;
}

__inline_g uint8x8_t
vreinterpret_u8_u16(uint16x4_t a) noexcept
{
  return (uint8x8_t)a;
}

__inline_g uint16x4_t
vreinterpret_u16_s16(int16x4_t a) noexcept
{
  return (uint16x4_t)a;
}

__inline_g int16x4_t
vreinterpret_s16_u16(uint16x4_t a) noexcept
{
  return (int16x4_t)a;
}

__inline_g uint32x2_t
vreinterpret_u32_u8(uint8x8_t a) noexcept
{
  return (uint32x2_t)a;
}

__inline_g uint32x2_t
vreinterpret_u32_u16(uint16x4_t a) noexcept
{
  return (uint32x2_t)a;
}

__inline_g uint8x8_t
vreinterpret_u8_u32(uint32x2_t a) noexcept
{
  return (uint8x8_t)a;
}

__inline_g uint16x4_t
vreinterpret_u16_u32(uint32x2_t a) noexcept
{
  return (uint16x4_t)a;
}

__inline_g float32x2_t
vreinterpret_f32_u32(uint32x2_t a) noexcept
{
  return (float32x2_t)a;
}

__inline_g uint32x2_t
vreinterpret_u32_f32(float32x2_t a) noexcept
{
  return (uint32x2_t)a;
}

#define __mc_vget_low_16(SUF, T_Q, T_D)                                                                                                    \
  __inline_g T_D vget_low_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 0, 1, 2, 3, 4, 5, 6, 7); }                          \
  __inline_g T_D vget_high_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 8, 9, 10, 11, 12, 13, 14, 15); }

#define __mc_vget_low_8(SUF, T_Q, T_D)                                                                                                     \
  __inline_g T_D vget_low_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 0, 1, 2, 3); }                                      \
  __inline_g T_D vget_high_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 4, 5, 6, 7); }

#define __mc_vget_low_4(SUF, T_Q, T_D)                                                                                                     \
  __inline_g T_D vget_low_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 0, 1); }                                            \
  __inline_g T_D vget_high_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 2, 3); }

#define __mc_vget_low_2(SUF, T_Q, T_D)                                                                                                     \
  __inline_g T_D vget_low_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 0); }                                               \
  __inline_g T_D vget_high_##SUF(T_Q v) noexcept { return __builtin_shufflevector(v, v, 1); }

__mc_vget_low_16(s8, int8x16_t, int8x8_t);
__mc_vget_low_16(u8, uint8x16_t, uint8x8_t);
__mc_vget_low_16(p8, poly8x16_t, poly8x8_t);
__mc_vget_low_8(s16, int16x8_t, int16x4_t);
__mc_vget_low_8(u16, uint16x8_t, uint16x4_t);
__mc_vget_low_8(p16, poly16x8_t, poly16x4_t);
__mc_vget_low_4(s32, int32x4_t, int32x2_t);
__mc_vget_low_4(u32, uint32x4_t, uint32x2_t);
__mc_vget_low_4(f32, float32x4_t, float32x2_t);
__mc_vget_low_2(s64, int64x2_t, int64x1_t);
__mc_vget_low_2(u64, uint64x2_t, uint64x1_t);
__mc_vget_low_2(p64, poly64x2_t, poly64x1_t);
__mc_vget_low_2(f64, float64x2_t, float64x1_t);

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__micron_arm_fp16)
__mc_vget_low_8(f16, float16x8_t, float16x4_t);
#endif

#if defined(__ARM_FEATURE_BF16) || defined(__micron_arm_bf16)
__mc_vget_low_8(bf16, bfloat16x8_t, bfloat16x4_t);
#endif

#undef __mc_vget_low_16
#undef __mc_vget_low_8
#undef __mc_vget_low_4
#undef __mc_vget_low_2

#define __mc_vcombine_16(SUF, T_D, T_Q)                                                                                                    \
  __inline_g T_Q vcombine_##SUF(T_D lo, T_D hi) noexcept                                                                                   \
  {                                                                                                                                        \
    return __builtin_shufflevector(lo, hi, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);                                          \
  }

#define __mc_vcombine_8(SUF, T_D, T_Q)                                                                                                     \
  __inline_g T_Q vcombine_##SUF(T_D lo, T_D hi) noexcept { return __builtin_shufflevector(lo, hi, 0, 1, 2, 3, 4, 5, 6, 7); }

#define __mc_vcombine_4(SUF, T_D, T_Q)                                                                                                     \
  __inline_g T_Q vcombine_##SUF(T_D lo, T_D hi) noexcept { return __builtin_shufflevector(lo, hi, 0, 1, 2, 3); }

#define __mc_vcombine_2(SUF, T_D, T_Q)                                                                                                     \
  __inline_g T_Q vcombine_##SUF(T_D lo, T_D hi) noexcept { return __builtin_shufflevector(lo, hi, 0, 1); }

__mc_vcombine_16(s8, int8x8_t, int8x16_t);
__mc_vcombine_16(u8, uint8x8_t, uint8x16_t);
__mc_vcombine_16(p8, poly8x8_t, poly8x16_t);
__mc_vcombine_8(s16, int16x4_t, int16x8_t);
__mc_vcombine_8(u16, uint16x4_t, uint16x8_t);
__mc_vcombine_8(p16, poly16x4_t, poly16x8_t);
__mc_vcombine_4(s32, int32x2_t, int32x4_t);
__mc_vcombine_4(u32, uint32x2_t, uint32x4_t);
__mc_vcombine_4(f32, float32x2_t, float32x4_t);
__mc_vcombine_2(s64, int64x1_t, int64x2_t);
__mc_vcombine_2(u64, uint64x1_t, uint64x2_t);
__mc_vcombine_2(p64, poly64x1_t, poly64x2_t);
__mc_vcombine_2(f64, float64x1_t, float64x2_t);

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__micron_arm_fp16)
__mc_vcombine_8(f16, float16x4_t, float16x8_t);
#endif

#if defined(__ARM_FEATURE_BF16) || defined(__micron_arm_bf16)
__mc_vcombine_8(bf16, bfloat16x4_t, bfloat16x8_t);
#endif

#undef __mc_vcombine_16
#undef __mc_vcombine_8
#undef __mc_vcombine_4
#undef __mc_vcombine_2

__inline_g uint64x1_t
vcreate_u64(u64 v) noexcept
{
  uint64x1_t r = { v };
  return r;
}

__inline_g float32x4_t
vld1q_dup_f32(const float *p) noexcept
{
  const float v = *p;
  float32x4_t r = { v, v, v, v };
  return r;
}

__inline_g float32x4_t
vcvtq_f32_s32(int32x4_t v) noexcept
{
  return __builtin_convertvector(v, float32x4_t);
}

__inline_g int32x4_t
vcvtq_s32_f32(float32x4_t v) noexcept
{
  return __builtin_convertvector(v, int32x4_t);
}

__inline_g float64x2_t
vcvtq_f64_s64(int64x2_t v) noexcept
{
  return __builtin_convertvector(v, float64x2_t);
}

__inline_g int64x2_t
vcvtq_s64_f64(float64x2_t v) noexcept
{
  return __builtin_convertvector(v, int64x2_t);
}

__inline_g uint32x4_t
vcltq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (uint32x4_t)(a < b);
}

__inline_g uint64x2_t
vcltq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return (uint64x2_t)(a < b);
}

__inline_g float32x4_t
vmlaq_f32(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return a + b * c;
}

__inline_g uint8x16_t
vqtbl1q_u8(uint8x16_t table, uint8x16_t idx) noexcept
{
  return (uint8x16_t)__builtin_aarch64_qtbl1v16qi((int8x16_t)table, (int8x16_t)idx);
}

__inline_g uint8x16_t
vqtbx1q_u8(uint8x16_t fallback, uint8x16_t table, uint8x16_t idx) noexcept
{
  uint8x16_t tbl = vqtbl1q_u8(table, idx);
  uint8x16_t inrange = vqtbl1q_u8(vdupq_n_u8(0xff), idx);
  return vbslq_u8(inrange, tbl, fallback);
}

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron
