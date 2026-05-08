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

__neon_ldst(int8x16_t, s8, signed char) __neon_ldst(int16x8_t, s16, signed short) __neon_ldst(int32x4_t, s32, signed int)
    __neon_ldst(int64x2_t, s64, signed long long) __neon_ldst(uint8x16_t, u8, unsigned char) __neon_ldst(uint16x8_t, u16, unsigned short)
        __neon_ldst(uint32x4_t, u32, unsigned int) __neon_ldst(uint64x2_t, u64, unsigned long long) __neon_ldst(float32x4_t, f32, float)
            __neon_ldst(float64x2_t, f64, double) __neon_ldst(poly8x16_t, p8, poly8_t) __neon_ldst(poly16x8_t, p16, poly16_t)
                __neon_ldst(poly64x2_t, p64, poly64_t)

                    __neon_ldst_64(int8x8_t, s8, signed char) __neon_ldst_64(int16x4_t, s16, signed short)
                        __neon_ldst_64(int32x2_t, s32, signed int) __neon_ldst_64(int64x1_t, s64, signed long long)
                            __neon_ldst_64(uint8x8_t, u8, unsigned char) __neon_ldst_64(uint16x4_t, u16, unsigned short)
                                __neon_ldst_64(uint32x2_t, u32, unsigned int) __neon_ldst_64(uint64x1_t, u64, unsigned long long)
                                    __neon_ldst_64(float32x2_t, f32, float) __neon_ldst_64(float64x1_t, f64, double)

#undef __neon_ldst
#undef __neon_ldst_64

                                        __inline_g int8x16_t vdupq_n_s8(signed char v) noexcept
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
  return (uint8x16_t)__builtin_aarch64_uminv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vminq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_uminv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vminq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_uminv4si_uuu(a, b);
}

__inline_g float32x4_t
vminq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_aarch64_smin_nanv4sf(a, b);
}

__inline_g float64x2_t
vminq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_aarch64_smin_nanv2df(a, b);
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
  return (uint8x16_t)__builtin_aarch64_umaxv16qi_uuu(a, b);
}

__inline_g uint16x8_t
vmaxq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint16x8_t)__builtin_aarch64_umaxv8hi_uuu(a, b);
}

__inline_g uint32x4_t
vmaxq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)__builtin_aarch64_umaxv4si_uuu(a, b);
}

__inline_g float32x4_t
vmaxq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return __builtin_aarch64_smax_nanv4sf(a, b);
}

__inline_g float64x2_t
vmaxq_f64(float64x2_t a, float64x2_t b) noexcept
{
  return __builtin_aarch64_smax_nanv2df(a, b);
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

__neon_reinterpret_ALL_FROM(s8, int8x16_t) __neon_reinterpret_ALL_FROM(s16, int16x8_t) __neon_reinterpret_ALL_FROM(s32, int32x4_t)
    __neon_reinterpret_ALL_FROM(s64, int64x2_t) __neon_reinterpret_ALL_FROM(u8, uint8x16_t) __neon_reinterpret_ALL_FROM(u16, uint16x8_t)
        __neon_reinterpret_ALL_FROM(u32, uint32x4_t) __neon_reinterpret_ALL_FROM(u64, uint64x2_t)
            __neon_reinterpret_ALL_FROM(f32, float32x4_t) __neon_reinterpret_ALL_FROM(f64, float64x2_t)

#undef __neon_reinterpret_ALL_FROM
#undef __neon_reinterpret

                __inline_g signed char vgetq_lane_s8(int8x16_t v, const int n) noexcept
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

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron
