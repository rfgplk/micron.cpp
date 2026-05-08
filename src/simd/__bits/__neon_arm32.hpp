//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_arm32.hpp"

#if !defined(__micron_arch_arm32)
#error "__neon_arm32.hpp included on a non-armv7 build"
#endif

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

#define __neon32_ldst(T, suffix, ETYPE)                                                                                                    \
  __inline_g T vld1q_##suffix(const ETYPE *p) noexcept                                                                                     \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __builtin_memcpy(&r, p, sizeof(T));                                                                                                    \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  __inline_g void vst1q_##suffix(ETYPE *p, T v) noexcept { __builtin_memcpy(p, &v, sizeof(T)); }                                           \
  __inline_g typename ::micron::simd::__bits::__halfof32<T>::type vld1_##suffix(const ETYPE *p) noexcept                                   \
  {                                                                                                                                        \
    typename ::micron::simd::__bits::__halfof32<T>::type r;                                                                                \
    __builtin_memcpy(&r, p, sizeof(r));                                                                                                    \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  __inline_g void vst1_##suffix(ETYPE *p, typename ::micron::simd::__bits::__halfof32<T>::type v) noexcept                                 \
  {                                                                                                                                        \
    __builtin_memcpy(p, &v, sizeof(v));                                                                                                    \
  }

template <typename> struct __halfof32;

template <> struct __halfof32<int8x16_t> {
  using type = int8x8_t;
};

template <> struct __halfof32<int16x8_t> {
  using type = int16x4_t;
};

template <> struct __halfof32<int32x4_t> {
  using type = int32x2_t;
};

template <> struct __halfof32<int64x2_t> {
  using type = int64x1_t;
};

template <> struct __halfof32<uint8x16_t> {
  using type = uint8x8_t;
};

template <> struct __halfof32<uint16x8_t> {
  using type = uint16x4_t;
};

template <> struct __halfof32<uint32x4_t> {
  using type = uint32x2_t;
};

template <> struct __halfof32<uint64x2_t> {
  using type = uint64x1_t;
};

template <> struct __halfof32<float32x4_t> {
  using type = float32x2_t;
};

__neon32_ldst(int8x16_t, s8, signed char) __neon32_ldst(int16x8_t, s16, signed short) __neon32_ldst(int32x4_t, s32, signed int)
    __neon32_ldst(int64x2_t, s64, signed long long) __neon32_ldst(uint8x16_t, u8, unsigned char)
        __neon32_ldst(uint16x8_t, u16, unsigned short) __neon32_ldst(uint32x4_t, u32, unsigned int)
            __neon32_ldst(uint64x2_t, u64, unsigned long long) __neon32_ldst(float32x4_t, f32, float)

#undef __neon32_ldst

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

__inline_g int8x16_t
vaddq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return a + b;
}

__inline_g int16x8_t
vaddq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return a + b;
}

__inline_g int32x4_t
vaddq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return a + b;
}

__inline_g int64x2_t
vaddq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return a + b;
}

__inline_g uint8x16_t
vaddq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return a + b;
}

__inline_g uint16x8_t
vaddq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return a + b;
}

__inline_g uint32x4_t
vaddq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return a + b;
}

__inline_g uint64x2_t
vaddq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return a + b;
}

__inline_g float32x4_t
vaddq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return a + b;
}

__inline_g int8x16_t
vsubq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return a - b;
}

__inline_g int16x8_t
vsubq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return a - b;
}

__inline_g int32x4_t
vsubq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return a - b;
}

__inline_g int64x2_t
vsubq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return a - b;
}

__inline_g uint8x16_t
vsubq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return a - b;
}

__inline_g uint16x8_t
vsubq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return a - b;
}

__inline_g uint32x4_t
vsubq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return a - b;
}

__inline_g uint64x2_t
vsubq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return a - b;
}

__inline_g float32x4_t
vsubq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return a - b;
}

__inline_g int8x16_t
vmulq_s8(int8x16_t a, int8x16_t b) noexcept
{
  return a * b;
}

__inline_g int16x8_t
vmulq_s16(int16x8_t a, int16x8_t b) noexcept
{
  return a * b;
}

__inline_g int32x4_t
vmulq_s32(int32x4_t a, int32x4_t b) noexcept
{
  return a * b;
}

__inline_g uint8x16_t
vmulq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return a * b;
}

__inline_g uint16x8_t
vmulq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return a * b;
}

__inline_g uint32x4_t
vmulq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return a * b;
}

__inline_g float32x4_t
vmulq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return a * b;
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

__inline_g uint32x4_t
vceqq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (uint32x4_t)(a == b);
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

__inline_g uint32x4_t
vcgtq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return (uint32x4_t)(a > b);
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron
