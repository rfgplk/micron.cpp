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
#pragma GCC diagnostic ignored "-Wpedantic"

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

template<typename> struct __halfof32;

template<> struct __halfof32<int8x16_t> {
  using type = int8x8_t;
};

template<> struct __halfof32<int16x8_t> {
  using type = int16x4_t;
};

template<> struct __halfof32<int32x4_t> {
  using type = int32x2_t;
};

template<> struct __halfof32<int64x2_t> {
  using type = int64x1_t;
};

template<> struct __halfof32<uint8x16_t> {
  using type = uint8x8_t;
};

template<> struct __halfof32<uint16x8_t> {
  using type = uint16x4_t;
};

template<> struct __halfof32<uint32x4_t> {
  using type = uint32x2_t;
};

template<> struct __halfof32<uint64x2_t> {
  using type = uint64x1_t;
};

template<> struct __halfof32<float32x4_t> {
  using type = float32x2_t;
};

__neon32_ldst(int8x16_t, s8, signed char);
__neon32_ldst(int16x8_t, s16, signed short);
__neon32_ldst(int32x4_t, s32, signed int);
__neon32_ldst(int64x2_t, s64, signed long long);
__neon32_ldst(uint8x16_t, u8, unsigned char);
__neon32_ldst(uint16x8_t, u16, unsigned short);
__neon32_ldst(uint32x4_t, u32, unsigned int);
__neon32_ldst(uint64x2_t, u64, unsigned long long);
__neon32_ldst(float32x4_t, f32, float);

#undef __neon32_ldst

__inline_g int8x16_t
vdupq_n_s8(signed char v) noexcept
{
  int8x16_t r = { v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
  return r;
}

__inline_g int16x8_t
vdupq_n_s16(signed short v) noexcept
{
  int16x8_t r = { v, v, v, v, v, v, v, v };
  return r;
}

__inline_g int32x4_t
vdupq_n_s32(signed int v) noexcept
{
  int32x4_t r = { v, v, v, v };
  return r;
}

__inline_g int64x2_t
vdupq_n_s64(signed long long v) noexcept
{
  int64x2_t r = { v, v };
  return r;
}

__inline_g uint8x16_t
vdupq_n_u8(unsigned char v) noexcept
{
  uint8x16_t r = { v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
  return r;
}

__inline_g uint16x8_t
vdupq_n_u16(unsigned short v) noexcept
{
  uint16x8_t r = { v, v, v, v, v, v, v, v };
  return r;
}

__inline_g uint32x4_t
vdupq_n_u32(unsigned int v) noexcept
{
  uint32x4_t r = { v, v, v, v };
  return r;
}

__inline_g uint64x2_t
vdupq_n_u64(unsigned long long v) noexcept
{
  uint64x2_t r = { v, v };
  return r;
}

__inline_g float32x4_t
vdupq_n_f32(float v) noexcept
{
  float32x4_t r = { v, v, v, v };
  return r;
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

__inline_g float32x4_t
vnegq_f32(float32x4_t a) noexcept
{
  return -a;
}

__inline_g int32x4_t
vnegq_s32(int32x4_t a) noexcept
{
  return -a;
}

__inline_g int16x8_t
vnegq_s16(int16x8_t a) noexcept
{
  return -a;
}

__inline_g int8x16_t
vnegq_s8(int8x16_t a) noexcept
{
  return -a;
}

__inline_g float32x2_t
vneg_f32(float32x2_t a) noexcept
{
  return -a;
}

__inline_g int32x2_t
vneg_s32(int32x2_t a) noexcept
{
  return -a;
}

__inline_g int16x4_t
vneg_s16(int16x4_t a) noexcept
{
  return -a;
}

__inline_g int8x8_t
vneg_s8(int8x8_t a) noexcept
{
  return -a;
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

__inline_g int64x2_t
veorq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)((uint64x2_t)a ^ (uint64x2_t)b);
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

#define __mc_vreint_q(DST, dst_suf, SRC, src_suf)                                                                                          \
  __inline_g DST vreinterpretq_##dst_suf##_##src_suf(SRC v) noexcept { return (DST)v; }

// uint64x2_t <-> {s64, u32, s32, u16, s16, u8, s8, f32}
__mc_vreint_q(uint64x2_t, u64, int64x2_t, s64);
__mc_vreint_q(uint64x2_t, u64, uint32x4_t, u32);
__mc_vreint_q(uint64x2_t, u64, int32x4_t, s32);
__mc_vreint_q(uint64x2_t, u64, uint16x8_t, u16) __mc_vreint_q(uint64x2_t, u64, int16x8_t, s16);
__mc_vreint_q(uint64x2_t, u64, uint8x16_t, u8);
__mc_vreint_q(uint64x2_t, u64, int8x16_t, s8);
__mc_vreint_q(uint64x2_t, u64, float32x4_t, f32);

__mc_vreint_q(int64x2_t, s64, uint64x2_t, u64);
__mc_vreint_q(int64x2_t, s64, uint32x4_t, u32);
__mc_vreint_q(int64x2_t, s64, int32x4_t, s32);

// uint32x4_t <-> {u64, s64, s32, u16, s16, u8, s8, f32}
__mc_vreint_q(uint32x4_t, u32, uint64x2_t, u64);
__mc_vreint_q(uint32x4_t, u32, int64x2_t, s64);
__mc_vreint_q(uint32x4_t, u32, int32x4_t, s32);
__mc_vreint_q(uint32x4_t, u32, float32x4_t, f32);

// int32x4_t <-> {u32, s64, u64, s16, s8, u8, u16, f32}
__mc_vreint_q(int32x4_t, s32, uint32x4_t, u32);
__mc_vreint_q(int32x4_t, s32, int64x2_t, s64);
__mc_vreint_q(int32x4_t, s32, uint64x2_t, u64);
__mc_vreint_q(int32x4_t, s32, int16x8_t, s16);
__mc_vreint_q(int32x4_t, s32, uint16x8_t, u16);
__mc_vreint_q(int32x4_t, s32, int8x16_t, s8);
__mc_vreint_q(int32x4_t, s32, uint8x16_t, u8);
__mc_vreint_q(int32x4_t, s32, float32x4_t, f32);

__mc_vreint_q(int16x8_t, s16, int32x4_t, s32);
__mc_vreint_q(int16x8_t, s16, uint64x2_t, u64);
__mc_vreint_q(int16x8_t, s16, uint16x8_t, u16);

__mc_vreint_q(int8x16_t, s8, int32x4_t, s32);
__mc_vreint_q(int8x16_t, s8, uint64x2_t, u64);
__mc_vreint_q(int8x16_t, s8, uint8x16_t, u8);

__mc_vreint_q(float32x4_t, f32, int32x4_t, s32);
__mc_vreint_q(float32x4_t, f32, uint32x4_t, u32);
__mc_vreint_q(float32x4_t, f32, uint64x2_t, u64);
__mc_vreint_q(float32x4_t, f32, int64x2_t, s64);
__mc_vreint_q(float32x4_t, f32, uint16x8_t, u16);
__mc_vreint_q(float32x4_t, f32, int16x8_t, s16);
__mc_vreint_q(float32x4_t, f32, uint8x16_t, u8);
__mc_vreint_q(float32x4_t, f32, int8x16_t, s8);

__mc_vreint_q(uint32x4_t, u32, int16x8_t, s16);
__mc_vreint_q(uint32x4_t, u32, uint16x8_t, u16);
__mc_vreint_q(uint32x4_t, u32, int8x16_t, s8);
__mc_vreint_q(uint32x4_t, u32, uint8x16_t, u8);

__mc_vreint_q(uint16x8_t, u16, uint32x4_t, u32);
__mc_vreint_q(uint16x8_t, u16, int32x4_t, s32);
__mc_vreint_q(uint16x8_t, u16, uint64x2_t, u64);
__mc_vreint_q(uint16x8_t, u16, int64x2_t, s64);
__mc_vreint_q(uint16x8_t, u16, uint8x16_t, u8);
__mc_vreint_q(uint16x8_t, u16, int8x16_t, s8);
__mc_vreint_q(uint16x8_t, u16, float32x4_t, f32);
__mc_vreint_q(uint16x8_t, u16, int16x8_t, s16);

__mc_vreint_q(uint8x16_t, u8, uint16x8_t, u16);
__mc_vreint_q(uint8x16_t, u8, int16x8_t, s16);
__mc_vreint_q(uint8x16_t, u8, uint32x4_t, u32);
__mc_vreint_q(uint8x16_t, u8, int32x4_t, s32);
__mc_vreint_q(uint8x16_t, u8, uint64x2_t, u64);
__mc_vreint_q(uint8x16_t, u8, int64x2_t, s64);
__mc_vreint_q(uint8x16_t, u8, float32x4_t, f32);
__mc_vreint_q(uint8x16_t, u8, int8x16_t, s8);

#undef __mc_vreint_q

__inline_g uint16x4_t
vreinterpret_u16_u8(uint8x8_t v) noexcept
{
  return (uint16x4_t)v;
}

// was missing
__inline_g uint32x2_t
vreinterpret_u32_u8(uint8x8_t v) noexcept
{
  return (uint32x2_t)v;
}

__inline_g uint8x8_t
vreinterpret_u8_u32(uint32x2_t v) noexcept
{
  return (uint8x8_t)v;
}

__inline_g f32
vgetq_lane_f32(float32x4_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u32
vgetq_lane_u32(uint32x4_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g i32
vgetq_lane_s32(int32x4_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u64
vgetq_lane_u64(uint64x2_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g i64
vgetq_lane_s64(int64x2_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g i16
vgetq_lane_s16(int16x8_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g i8
vgetq_lane_s8(int8x16_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u8
vgetq_lane_u8(uint8x16_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u16
vgetq_lane_u16(uint16x8_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u16
vget_lane_u16(uint16x4_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g float32x4_t
vsetq_lane_f32(f32 x, float32x4_t v, const int lane) noexcept
{
  v[lane] = x;
  return v;
}

__inline_g uint32x4_t
vsetq_lane_u32(u32 x, uint32x4_t v, const int lane) noexcept
{
  v[lane] = x;
  return v;
}

__inline_g uint16x8_t
vsetq_lane_u16(u16 x, uint16x8_t v, const int lane) noexcept
{
  v[lane] = x;
  return v;
}

__inline_g int8x16_t
vsetq_lane_s8(i8 x, int8x16_t v, const int lane) noexcept
{
  v[lane] = x;
  return v;
}

__inline_g int16x8_t
vsetq_lane_s16(i16 x, int16x8_t v, const int lane) noexcept
{
  v[lane] = x;
  return v;
}

__inline_g uint8x16_t
vsetq_lane_u8(u8 x, uint8x16_t v, const int lane) noexcept
{
  v[lane] = x;
  return v;
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

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)
__mc_vget_low_2(p64, poly64x2_t, poly64x1_t);
#endif

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

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)
__mc_vcombine_2(p64, poly64x1_t, poly64x2_t);
#endif

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

__inline_g int64x2_t
vshlq_n_s64(int64x2_t v, const int n) noexcept
{
  return v << n;
}

__inline_g int16x8_t
vshlq_n_s16(int16x8_t v, const int n) noexcept
{
  return v << n;
}

__inline_g uint32x4_t
vshrq_n_u32(uint32x4_t v, const int n) noexcept
{
  return v >> n;
}

__inline_g uint64x2_t
vshrq_n_u64(uint64x2_t v, const int n) noexcept
{
  return v >> n;
}

__inline_g int64x2_t
vshrq_n_s64(int64x2_t v, const int n) noexcept
{
  int64x2_t r;
  __asm__("vshr.s64 %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));
  return r;
}

// see the vshlq_* note below: real USHL semantics (negative count => logical
// right shift), not the UB-on-negative GCC vector `<<`.
__inline_g uint64x2_t
vshlq_u64(uint64x2_t v, int64x2_t cnt) noexcept
{
  uint64x2_t r;
  __asm__("vshl.u64 %q0, %q1, %q2" : "=w"(r) : "w"(v), "w"(cnt));
  return r;
}

__inline_g float32x4_t
vrecpeq_f32(float32x4_t v) noexcept
{
  float32x4_t r;
  __asm__("vrecpe.f32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g float32x4_t
vrecpsq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x4_t r;
  __asm__("vrecps.f32 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g float32x4_t
vrsqrteq_f32(float32x4_t v) noexcept
{
  float32x4_t r;
  __asm__("vrsqrte.f32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g uint8x8_t
vpadd_u8(uint8x8_t a, uint8x8_t b) noexcept
{
  uint8x8_t r;
  __asm__("vpadd.i8 %P0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g uint32x4_t
vrev64q_u32(uint32x4_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2);
}

__inline_g uint32x4x2_t
vzipq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  uint32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 4, 1, 5);
  r.val[1] = __builtin_shufflevector(a, b, 2, 6, 3, 7);
  return r;
}

__inline_g int64x1_t
vdup_n_s64(i64 v) noexcept
{
  int64x1_t r = { v };
  return r;
}

__inline_g uint64x1_t
vcreate_u64(u64 v) noexcept
{
  uint64x1_t r = { v };
  return r;
}

__inline_g float32x4_t
vld1q_dup_f32(const f32 *p) noexcept
{
  const f32 v = *p;
  float32x4_t r = { v, v, v, v };
  return r;
}

#define __mc_vbslq(T, suffix, MASK)                                                                                                        \
  __inline_g T vbslq_##suffix(MASK m, T a, T b) noexcept                                                                                   \
  {                                                                                                                                        \
    T r = (T)m;                                                                                                                            \
    __asm__("vbsl %q0, %q1, %q2" : "+w"(r) : "w"(a), "w"(b));                                                                              \
    return r;                                                                                                                              \
  }

__mc_vbslq(uint8x16_t, u8, uint8x16_t);
__mc_vbslq(uint16x8_t, u16, uint16x8_t);
__mc_vbslq(uint32x4_t, u32, uint32x4_t);
__mc_vbslq(uint64x2_t, u64, uint64x2_t);
__mc_vbslq(int8x16_t, s8, uint8x16_t);
__mc_vbslq(int16x8_t, s16, uint16x8_t);
__mc_vbslq(int32x4_t, s32, uint32x4_t);
__mc_vbslq(int64x2_t, s64, uint64x2_t);
__mc_vbslq(float32x4_t, f32, uint32x4_t);

#undef __mc_vbslq

__inline_g uint8x16_t
vextq_u8(uint8x16_t a, uint8x16_t b, const int n) noexcept
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

__inline_g int8x16_t
vextq_s8(int8x16_t a, int8x16_t b, const int n) noexcept
{
  return (int8x16_t)vextq_u8((uint8x16_t)a, (uint8x16_t)b, n);
}

__inline_g uint16x8_t
vextq_u16(uint16x8_t a, uint16x8_t b, const int n) noexcept
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

__inline_g int16x8_t
vextq_s16(int16x8_t a, int16x8_t b, const int n) noexcept
{
  return (int16x8_t)vextq_u16((uint16x8_t)a, (uint16x8_t)b, n);
}

__inline_g uint32x4_t
vextq_u32(uint32x4_t a, uint32x4_t b, const int n) noexcept
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

__inline_g int32x4_t
vextq_s32(int32x4_t a, int32x4_t b, const int n) noexcept
{
  return (int32x4_t)vextq_u32((uint32x4_t)a, (uint32x4_t)b, n);
}

__inline_g float32x4_t
vextq_f32(float32x4_t a, float32x4_t b, const int n) noexcept
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

__inline_g uint64x2_t
vextq_u64(uint64x2_t a, uint64x2_t b, const int n) noexcept
{
  return (n & 1) ? __builtin_shufflevector(a, b, 1, 2) : a;
}

__inline_g int64x2_t
vextq_s64(int64x2_t a, int64x2_t b, const int n) noexcept
{
  return (int64x2_t)vextq_u64((uint64x2_t)a, (uint64x2_t)b, n);
}

__inline_g int8x16_t
vabsq_s8(int8x16_t v) noexcept
{
  int8x16_t r;
  __asm__("vabs.s8 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g int16x8_t
vabsq_s16(int16x8_t v) noexcept
{
  int16x8_t r;
  __asm__("vabs.s16 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g int32x4_t
vabsq_s32(int32x4_t v) noexcept
{
  int32x4_t r;
  __asm__("vabs.s32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g float32x4_t
vabsq_f32(float32x4_t v) noexcept
{
  float32x4_t r;
  __asm__("vabs.f32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

#define __mc_vabdq(SUF, ASM, T)                                                                                                            \
  __inline_g T vabdq_##SUF(T a, T b) noexcept                                                                                              \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vabd." ASM " %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));                                                                      \
    return r;                                                                                                                              \
  }

#define __mc_vabd_d(SUF, ASM, T)                                                                                                           \
  __inline_g T vabd_##SUF(T a, T b) noexcept                                                                                               \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vabd." ASM " %P0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                                      \
    return r;                                                                                                                              \
  }

__mc_vabdq(s8, "s8", int8x16_t);
__mc_vabdq(s16, "s16", int16x8_t);
__mc_vabdq(s32, "s32", int32x4_t);
__mc_vabdq(u8, "u8", uint8x16_t);
__mc_vabdq(u16, "u16", uint16x8_t);
__mc_vabdq(u32, "u32", uint32x4_t);
__mc_vabdq(f32, "f32", float32x4_t);

__mc_vabd_d(s8, "s8", int8x8_t);
__mc_vabd_d(s16, "s16", int16x4_t);
__mc_vabd_d(s32, "s32", int32x2_t);
__mc_vabd_d(u8, "u8", uint8x8_t);
__mc_vabd_d(u16, "u16", uint16x4_t);
__mc_vabd_d(u32, "u32", uint32x2_t);
__mc_vabd_d(f32, "f32", float32x2_t);

#undef __mc_vabdq
#undef __mc_vabd_d

#define __mc_vabdl(SUF, ASM, T_IN, T_OUT)                                                                                                  \
  __inline_g T_OUT vabdl_##SUF(T_IN a, T_IN b) noexcept                                                                                    \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__("vabdl." ASM " %q0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                                     \
    return r;                                                                                                                              \
  }

__mc_vabdl(s8, "s8", int8x8_t, int16x8_t);
__mc_vabdl(s16, "s16", int16x4_t, int32x4_t);
__mc_vabdl(s32, "s32", int32x2_t, int64x2_t);
__mc_vabdl(u8, "u8", uint8x8_t, uint16x8_t);
__mc_vabdl(u16, "u16", uint16x4_t, uint32x4_t);
__mc_vabdl(u32, "u32", uint32x2_t, uint64x2_t);

#undef __mc_vabdl

#define __mc_vabaq(SUF, ASM, T)                                                                                                            \
  __inline_g T vabaq_##SUF(T acc, T a, T b) noexcept                                                                                       \
  {                                                                                                                                        \
    __asm__("vaba." ASM " %q0, %q1, %q2" : "+w"(acc) : "w"(a), "w"(b));                                                                    \
    return acc;                                                                                                                            \
  }

#define __mc_vaba_d(SUF, ASM, T)                                                                                                           \
  __inline_g T vaba_##SUF(T acc, T a, T b) noexcept                                                                                        \
  {                                                                                                                                        \
    __asm__("vaba." ASM " %P0, %P1, %P2" : "+w"(acc) : "w"(a), "w"(b));                                                                    \
    return acc;                                                                                                                            \
  }

__mc_vabaq(s8, "s8", int8x16_t);
__mc_vabaq(s16, "s16", int16x8_t);
__mc_vabaq(s32, "s32", int32x4_t);
__mc_vabaq(u8, "u8", uint8x16_t);
__mc_vabaq(u16, "u16", uint16x8_t);
__mc_vabaq(u32, "u32", uint32x4_t);

__mc_vaba_d(s8, "s8", int8x8_t);
__mc_vaba_d(s16, "s16", int16x4_t);
__mc_vaba_d(s32, "s32", int32x2_t);
__mc_vaba_d(u8, "u8", uint8x8_t);
__mc_vaba_d(u16, "u16", uint16x4_t);
__mc_vaba_d(u32, "u32", uint32x2_t);

#undef __mc_vabaq
#undef __mc_vaba_d

#define __mc_vabal(SUF, ASM, T_IN, T_OUT)                                                                                                  \
  __inline_g T_OUT vabal_##SUF(T_OUT acc, T_IN a, T_IN b) noexcept                                                                         \
  {                                                                                                                                        \
    __asm__("vabal." ASM " %q0, %P1, %P2" : "+w"(acc) : "w"(a), "w"(b));                                                                   \
    return acc;                                                                                                                            \
  }

__mc_vabal(s8, "s8", int8x8_t, int16x8_t);
__mc_vabal(s16, "s16", int16x4_t, int32x4_t);
__mc_vabal(s32, "s32", int32x2_t, int64x2_t);
__mc_vabal(u8, "u8", uint8x8_t, uint16x8_t);
__mc_vabal(u16, "u16", uint16x4_t, uint32x4_t);
__mc_vabal(u32, "u32", uint32x2_t, uint64x2_t);

#undef __mc_vabal

// widening multiply-accumulate; a8 forwards back-to-back vmlal accumulators
#define __mc_vmlal(SUF, ASM, T_IN, T_OUT)                                                                                                  \
  __inline_g T_OUT vmlal_##SUF(T_OUT acc, T_IN a, T_IN b) noexcept                                                                         \
  {                                                                                                                                        \
    __asm__("vmlal." ASM " %q0, %P1, %P2" : "+w"(acc) : "w"(a), "w"(b));                                                                   \
    return acc;                                                                                                                            \
  }

__mc_vmlal(s8, "s8", int8x8_t, int16x8_t);
__mc_vmlal(s16, "s16", int16x4_t, int32x4_t);
__mc_vmlal(s32, "s32", int32x2_t, int64x2_t);
__mc_vmlal(u8, "u8", uint8x8_t, uint16x8_t);
__mc_vmlal(u16, "u16", uint16x4_t, uint32x4_t);
__mc_vmlal(u32, "u32", uint32x2_t, uint64x2_t);

#undef __mc_vmlal

__inline_g float32x4_t
vrsqrtsq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x4_t r;
  __asm__("vrsqrts.f32 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

// emulate
__inline_g float32x4_t
vsqrtq_f32(float32x4_t v) noexcept
{
  float a0 = v[0], a1 = v[1], a2 = v[2], a3 = v[3];
  __asm__("vsqrt.f32 %0, %0" : "+t"(a0));
  __asm__("vsqrt.f32 %0, %0" : "+t"(a1));
  __asm__("vsqrt.f32 %0, %0" : "+t"(a2));
  __asm__("vsqrt.f32 %0, %0" : "+t"(a3));
  return float32x4_t{ a0, a1, a2, a3 };
}

#define __mc_vcmpq_batch(NAME, OP)                                                                                                         \
  __inline_g uint8x16_t NAME##q_s8(int8x16_t a, int8x16_t b) noexcept { return (uint8x16_t)(a OP b); }                                     \
  __inline_g uint16x8_t NAME##q_s16(int16x8_t a, int16x8_t b) noexcept { return (uint16x8_t)(a OP b); }                                    \
  __inline_g uint32x4_t NAME##q_s32(int32x4_t a, int32x4_t b) noexcept { return (uint32x4_t)(a OP b); }                                    \
  __inline_g uint8x16_t NAME##q_u8(uint8x16_t a, uint8x16_t b) noexcept { return (uint8x16_t)(a OP b); }                                   \
  __inline_g uint16x8_t NAME##q_u16(uint16x8_t a, uint16x8_t b) noexcept { return (uint16x8_t)(a OP b); }                                  \
  __inline_g uint32x4_t NAME##q_u32(uint32x4_t a, uint32x4_t b) noexcept { return (uint32x4_t)(a OP b); }                                  \
  __inline_g uint32x4_t NAME##q_f32(float32x4_t a, float32x4_t b) noexcept { return (uint32x4_t)(a OP b); }

__mc_vcmpq_batch(vclt, <) __mc_vcmpq_batch(vcle, <=) __mc_vcmpq_batch(vcge, >=)

#undef __mc_vcmpq_batch

#define __mc_vtstq(SUF, T, RT)                                                                                                             \
  __inline_g RT vtstq_##SUF(T a, T b) noexcept { return (RT)((a & b) != T{}); }

    __mc_vtstq(s8, int8x16_t, uint8x16_t);
__mc_vtstq(s16, int16x8_t, uint16x8_t);
__mc_vtstq(s32, int32x4_t, uint32x4_t);
__mc_vtstq(u8, uint8x16_t, uint8x16_t);
__mc_vtstq(u16, uint16x8_t, uint16x8_t);
__mc_vtstq(u32, uint32x4_t, uint32x4_t);

#undef __mc_vtstq

__inline_g uint32x4_t
vcageq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return vcgeq_f32(vabsq_f32(a), vabsq_f32(b));
}

__inline_g uint32x4_t
vcagtq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return vcgtq_f32(vabsq_f32(a), vabsq_f32(b));
}

__inline_g uint32x4_t
vcaleq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return vcleq_f32(vabsq_f32(a), vabsq_f32(b));
}

__inline_g uint32x4_t
vcaltq_f32(float32x4_t a, float32x4_t b) noexcept
{
  return vcltq_f32(vabsq_f32(a), vabsq_f32(b));
}

__inline_g uint64x2_t
vceqq_s64(int64x2_t a, int64x2_t b) noexcept
{
  uint32x4_t cmp = vceqq_u32((uint32x4_t)a, (uint32x4_t)b);
  return (uint64x2_t)vandq_u32(cmp, vrev64q_u32(cmp));
}

__inline_g uint64x2_t
vceqq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  uint32x4_t cmp = vceqq_u32((uint32x4_t)a, (uint32x4_t)b);
  return (uint64x2_t)vandq_u32(cmp, vrev64q_u32(cmp));
}

__inline_g uint64x2_t
vcgtq_s64(int64x2_t a, int64x2_t b) noexcept
{
  uint32x4_t hi_gt = vcgtq_s32((int32x4_t)a, (int32x4_t)b);
  uint32x4_t hi_eq = vceqq_s32((int32x4_t)a, (int32x4_t)b);
  uint32x4_t lo_gt = vcgtq_u32((uint32x4_t)a, (uint32x4_t)b);
  uint32x4_t hi_gt_r = vrev64q_u32(hi_gt);
  uint32x4_t hi_eq_r = vrev64q_u32(hi_eq);
  uint32x4_t r = vorrq_u32(hi_gt_r, vandq_u32(hi_eq_r, lo_gt));
  uint32x4x2_t z = vzipq_u32(r, r);
  return (uint64x2_t)vcombine_u32(vget_low_u32(z.val[0]), vget_low_u32(z.val[1]));
}

__inline_g uint64x2_t
vcgtq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  uint32x4_t hi_gt = vcgtq_u32((uint32x4_t)a, (uint32x4_t)b);
  uint32x4_t hi_eq = vceqq_u32((uint32x4_t)a, (uint32x4_t)b);
  uint32x4_t lo_gt = vcgtq_u32((uint32x4_t)a, (uint32x4_t)b);
  uint32x4_t hi_gt_r = vrev64q_u32(hi_gt);
  uint32x4_t hi_eq_r = vrev64q_u32(hi_eq);
  uint32x4_t r = vorrq_u32(hi_gt_r, vandq_u32(hi_eq_r, lo_gt));
  uint32x4x2_t z = vzipq_u32(r, r);
  return (uint64x2_t)vcombine_u32(vget_low_u32(z.val[0]), vget_low_u32(z.val[1]));
}

__inline_g uint64x2_t
vcltq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vcgtq_s64(b, a);
}

__inline_g uint64x2_t
vcltq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return vcgtq_u64(b, a);
}

__inline_g uint64x2_t
vcgeq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (uint64x2_t)veorq_u32((uint32x4_t)vcgtq_s64(b, a), vdupq_n_u32(0xFFFFFFFFu));
}

__inline_g uint64x2_t
vcgeq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)veorq_u32((uint32x4_t)vcgtq_u64(b, a), vdupq_n_u32(0xFFFFFFFFu));
}

__inline_g uint64x2_t
vcleq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (uint64x2_t)veorq_u32((uint32x4_t)vcgtq_s64(a, b), vdupq_n_u32(0xFFFFFFFFu));
}

__inline_g uint64x2_t
vcleq_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint64x2_t)veorq_u32((uint32x4_t)vcgtq_u64(a, b), vdupq_n_u32(0xFFFFFFFFu));
}

#define __mc_vbicq_batch(SUF, T)                                                                                                           \
  __inline_g T vbicq_##SUF(T a, T b) noexcept { return a & ~b; }

__mc_vbicq_batch(u8, uint8x16_t);
__mc_vbicq_batch(u16, uint16x8_t);
__mc_vbicq_batch(u32, uint32x4_t);
__mc_vbicq_batch(u64, uint64x2_t);

#undef __mc_vbicq_batch

#define __mc_vbicq_signed(SUF, T, U)                                                                                                       \
  __inline_g T vbicq_##SUF(T a, T b) noexcept { return (T)((U)a & ~(U)b); }

__mc_vbicq_signed(s8, int8x16_t, uint8x16_t);
__mc_vbicq_signed(s16, int16x8_t, uint16x8_t);
__mc_vbicq_signed(s32, int32x4_t, uint32x4_t);
__mc_vbicq_signed(s64, int64x2_t, uint64x2_t);

#undef __mc_vbicq_signed

#define __mc_vornq_batch(SUF, T)                                                                                                           \
  __inline_g T vornq_##SUF(T a, T b) noexcept { return a | ~b; }

__mc_vornq_batch(u8, uint8x16_t);
__mc_vornq_batch(u16, uint16x8_t);
__mc_vornq_batch(u32, uint32x4_t);
__mc_vornq_batch(u64, uint64x2_t);

#undef __mc_vornq_batch

#define __mc_vornq_signed(SUF, T, U)                                                                                                       \
  __inline_g T vornq_##SUF(T a, T b) noexcept { return (T)((U)a | ~(U)b); }

__mc_vornq_signed(s8, int8x16_t, uint8x16_t);
__mc_vornq_signed(s16, int16x8_t, uint16x8_t);
__mc_vornq_signed(s32, int32x4_t, uint32x4_t);
__mc_vornq_signed(s64, int64x2_t, uint64x2_t);

#undef __mc_vornq_signed

__inline_g int8x16_t
vmvnq_s8(int8x16_t a) noexcept
{
  return (int8x16_t) ~(uint8x16_t)a;
}

__inline_g int16x8_t
vmvnq_s16(int16x8_t a) noexcept
{
  return (int16x8_t) ~(uint16x8_t)a;
}

__inline_g int32x4_t
vmvnq_s32(int32x4_t a) noexcept
{
  return (int32x4_t) ~(uint32x4_t)a;
}

__inline_g int64x2_t
vorrq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return (int64x2_t)((uint64x2_t)a | (uint64x2_t)b);
}

#define __mc_qbinop(NAME, ASM_OP, SUF, T)                                                                                                  \
  __inline_g T NAME##q_##SUF(T a, T b) noexcept                                                                                            \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__(ASM_OP " %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));                                                                           \
    return r;                                                                                                                              \
  }

__mc_qbinop(vqadd, "vqadd.s8", s8, int8x16_t);
__mc_qbinop(vqadd, "vqadd.s16", s16, int16x8_t);
__mc_qbinop(vqadd, "vqadd.s32", s32, int32x4_t);
__mc_qbinop(vqadd, "vqadd.s64", s64, int64x2_t);
__mc_qbinop(vqadd, "vqadd.u8", u8, uint8x16_t);
__mc_qbinop(vqadd, "vqadd.u16", u16, uint16x8_t);
__mc_qbinop(vqadd, "vqadd.u32", u32, uint32x4_t);
__mc_qbinop(vqadd, "vqadd.u64", u64, uint64x2_t);

__mc_qbinop(vqsub, "vqsub.s8", s8, int8x16_t);
__mc_qbinop(vqsub, "vqsub.s16", s16, int16x8_t);
__mc_qbinop(vqsub, "vqsub.s32", s32, int32x4_t);
__mc_qbinop(vqsub, "vqsub.s64", s64, int64x2_t);
__mc_qbinop(vqsub, "vqsub.u8", u8, uint8x16_t);
__mc_qbinop(vqsub, "vqsub.u16", u16, uint16x8_t);
__mc_qbinop(vqsub, "vqsub.u32", u32, uint32x4_t);
__mc_qbinop(vqsub, "vqsub.u64", u64, uint64x2_t);

__mc_qbinop(vhadd, "vhadd.s8", s8, int8x16_t);
__mc_qbinop(vhadd, "vhadd.s16", s16, int16x8_t);
__mc_qbinop(vhadd, "vhadd.s32", s32, int32x4_t);
__mc_qbinop(vhadd, "vhadd.u8", u8, uint8x16_t);
__mc_qbinop(vhadd, "vhadd.u16", u16, uint16x8_t);
__mc_qbinop(vhadd, "vhadd.u32", u32, uint32x4_t);

__mc_qbinop(vrhadd, "vrhadd.s8", s8, int8x16_t);
__mc_qbinop(vrhadd, "vrhadd.s16", s16, int16x8_t);
__mc_qbinop(vrhadd, "vrhadd.s32", s32, int32x4_t);
__mc_qbinop(vrhadd, "vrhadd.u8", u8, uint8x16_t);
__mc_qbinop(vrhadd, "vrhadd.u16", u16, uint16x8_t);
__mc_qbinop(vrhadd, "vrhadd.u32", u32, uint32x4_t);

__mc_qbinop(vhsub, "vhsub.s8", s8, int8x16_t);
__mc_qbinop(vhsub, "vhsub.s16", s16, int16x8_t);
__mc_qbinop(vhsub, "vhsub.s32", s32, int32x4_t);
__mc_qbinop(vhsub, "vhsub.u8", u8, uint8x16_t);
__mc_qbinop(vhsub, "vhsub.u16", u16, uint16x8_t);
__mc_qbinop(vhsub, "vhsub.u32", u32, uint32x4_t);

#undef __mc_qbinop

#define __mc_vmlaq_batch(SUF, T)                                                                                                           \
  __inline_g T vmlaq_##SUF(T a, T b, T c) noexcept { return a + (b * c); }                                                                 \
  __inline_g T vmlsq_##SUF(T a, T b, T c) noexcept { return a - (b * c); }

__mc_vmlaq_batch(s8, int8x16_t);
__mc_vmlaq_batch(s16, int16x8_t);
__mc_vmlaq_batch(s32, int32x4_t);
__mc_vmlaq_batch(u8, uint8x16_t);
__mc_vmlaq_batch(u16, uint16x8_t);
__mc_vmlaq_batch(u32, uint32x4_t);
__mc_vmlaq_batch(f32, float32x4_t);

#undef __mc_vmlaq_batch

#define __mc_vmulq_n(SUF, T, ETYPE)                                                                                                        \
  __inline_g T vmulq_n_##SUF(T a, ETYPE b) noexcept { return a * vdupq_n_##SUF(b); }

__mc_vmulq_n(s16, int16x8_t, signed short);
__mc_vmulq_n(s32, int32x4_t, signed int);
__mc_vmulq_n(u16, uint16x8_t, unsigned short);
__mc_vmulq_n(u32, uint32x4_t, unsigned int);
__mc_vmulq_n(f32, float32x4_t, float);

#undef __mc_vmulq_n

__inline_g int16x8_t
vmulq_lane_s16(int16x8_t a, int16x4_t b, const int lane) noexcept
{
  return a * vdupq_n_s16(b[lane]);
}

__inline_g int32x4_t
vmulq_lane_s32(int32x4_t a, int32x2_t b, const int lane) noexcept
{
  return a * vdupq_n_s32(b[lane]);
}

__inline_g uint16x8_t
vmulq_lane_u16(uint16x8_t a, uint16x4_t b, const int lane) noexcept
{
  return a * vdupq_n_u16(b[lane]);
}

__inline_g uint32x4_t
vmulq_lane_u32(uint32x4_t a, uint32x2_t b, const int lane) noexcept
{
  return a * vdupq_n_u32(b[lane]);
}

__inline_g float32x4_t
vmulq_lane_f32(float32x4_t a, float32x2_t b, const int lane) noexcept
{
  return a * vdupq_n_f32(b[lane]);
}

#define __mc_vmull(SUF, ASM_T, IN, OUT)                                                                                                    \
  __inline_g OUT vmull_##SUF(IN a, IN b) noexcept                                                                                          \
  {                                                                                                                                        \
    OUT r;                                                                                                                                 \
    __asm__("vmull." ASM_T " %q0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                                   \
    return r;                                                                                                                              \
  }

__mc_vmull(s8, "s8", int8x8_t, int16x8_t);
__mc_vmull(s16, "s16", int16x4_t, int32x4_t);
__mc_vmull(s32, "s32", int32x2_t, int64x2_t);
__mc_vmull(u8, "u8", uint8x8_t, uint16x8_t);
__mc_vmull(u16, "u16", uint16x4_t, uint32x4_t);
__mc_vmull(u32, "u32", uint32x2_t, uint64x2_t);

#undef __mc_vmull

__inline_g int32x4_t
vmull_n_s16(int16x4_t a, signed short b) noexcept
{
  return vmull_s16(a, (int16x4_t){ b, b, b, b });
}

__inline_g int16x8_t
vqdmulhq_s16(int16x8_t a, int16x8_t b) noexcept
{
  int16x8_t r;
  __asm__("vqdmulh.s16 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g int32x4_t
vqdmulhq_s32(int32x4_t a, int32x4_t b) noexcept
{
  int32x4_t r;
  __asm__("vqdmulh.s32 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g int16x8_t
vqrdmulhq_s16(int16x8_t a, int16x8_t b) noexcept
{
  int16x8_t r;
  __asm__("vqrdmulh.s16 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g int32x4_t
vqrdmulhq_s32(int32x4_t a, int32x4_t b) noexcept
{
  int32x4_t r;
  __asm__("vqrdmulh.s32 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g int32x4_t
vqdmull_s16(int16x4_t a, int16x4_t b) noexcept
{
  int32x4_t r;
  __asm__("vqdmull.s16 %q0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g int64x2_t
vqdmull_s32(int32x2_t a, int32x2_t b) noexcept
{
  int64x2_t r;
  __asm__("vqdmull.s32 %q0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

#define __mc_vlong(NAME, ASM_T, IN, OUT)                                                                                                   \
  __inline_g OUT v##NAME##_##ASM_T(IN a, IN b) noexcept                                                                                    \
  {                                                                                                                                        \
    OUT r;                                                                                                                                 \
    __asm__("v" #NAME "." #ASM_T " %q0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                             \
    return r;                                                                                                                              \
  }

__mc_vlong(addl, s8, int8x8_t, int16x8_t);
__mc_vlong(addl, s16, int16x4_t, int32x4_t);
__mc_vlong(addl, s32, int32x2_t, int64x2_t);
__mc_vlong(addl, u8, uint8x8_t, uint16x8_t);
__mc_vlong(addl, u16, uint16x4_t, uint32x4_t);
__mc_vlong(addl, u32, uint32x2_t, uint64x2_t);

__mc_vlong(subl, s8, int8x8_t, int16x8_t);
__mc_vlong(subl, s16, int16x4_t, int32x4_t);
__mc_vlong(subl, s32, int32x2_t, int64x2_t);
__mc_vlong(subl, u8, uint8x8_t, uint16x8_t);
__mc_vlong(subl, u16, uint16x4_t, uint32x4_t);
__mc_vlong(subl, u32, uint32x2_t, uint64x2_t);

#undef __mc_vlong

#define __mc_vwide(NAME, ASM_T, T_Q, T_D)                                                                                                  \
  __inline_g T_Q v##NAME##_##ASM_T(T_Q a, T_D b) noexcept                                                                                  \
  {                                                                                                                                        \
    T_Q r;                                                                                                                                 \
    __asm__("v" #NAME "." #ASM_T " %q0, %q1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                             \
    return r;                                                                                                                              \
  }

__mc_vwide(addw, s8, int16x8_t, int8x8_t);
__mc_vwide(addw, s16, int32x4_t, int16x4_t);
__mc_vwide(addw, s32, int64x2_t, int32x2_t);
__mc_vwide(addw, u8, uint16x8_t, uint8x8_t);
__mc_vwide(addw, u16, uint32x4_t, uint16x4_t);
__mc_vwide(addw, u32, uint64x2_t, uint32x2_t);

__mc_vwide(subw, s8, int16x8_t, int8x8_t);
__mc_vwide(subw, s16, int32x4_t, int16x4_t);
__mc_vwide(subw, s32, int64x2_t, int32x2_t);
__mc_vwide(subw, u8, uint16x8_t, uint8x8_t);
__mc_vwide(subw, u16, uint32x4_t, uint16x4_t);
__mc_vwide(subw, u32, uint64x2_t, uint32x2_t);

#undef __mc_vwide

#define __mc_vnarrow_hn(NAME, ASM_T, T_Q, T_D)                                                                                             \
  __inline_g T_D v##NAME##_##ASM_T(T_Q a, T_Q b) noexcept                                                                                  \
  {                                                                                                                                        \
    T_D r;                                                                                                                                 \
    __asm__("v" #NAME "." #ASM_T " %P0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));                                                             \
    return r;                                                                                                                              \
  }

__mc_vnarrow_hn(addhn, i16, int16x8_t, int8x8_t);
__mc_vnarrow_hn(addhn, i32, int32x4_t, int16x4_t);
__mc_vnarrow_hn(addhn, i64, int64x2_t, int32x2_t);

__inline_g uint8x8_t
vaddhn_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint8x8_t)vaddhn_i16((int16x8_t)a, (int16x8_t)b);
}

__inline_g uint16x4_t
vaddhn_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint16x4_t)vaddhn_i32((int32x4_t)a, (int32x4_t)b);
}

__inline_g uint32x2_t
vaddhn_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint32x2_t)vaddhn_i64((int64x2_t)a, (int64x2_t)b);
}

__inline_g int8x8_t
vaddhn_s16(int16x8_t a, int16x8_t b) noexcept
{
  return vaddhn_i16(a, b);
}

__inline_g int16x4_t
vaddhn_s32(int32x4_t a, int32x4_t b) noexcept
{
  return vaddhn_i32(a, b);
}

__inline_g int32x2_t
vaddhn_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vaddhn_i64(a, b);
}

__mc_vnarrow_hn(subhn, i16, int16x8_t, int8x8_t) __mc_vnarrow_hn(subhn, i32, int32x4_t, int16x4_t)
    __mc_vnarrow_hn(subhn, i64, int64x2_t, int32x2_t)

        __inline_g uint8x8_t vsubhn_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint8x8_t)vsubhn_i16((int16x8_t)a, (int16x8_t)b);
}

__inline_g uint16x4_t
vsubhn_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint16x4_t)vsubhn_i32((int32x4_t)a, (int32x4_t)b);
}

__inline_g uint32x2_t
vsubhn_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint32x2_t)vsubhn_i64((int64x2_t)a, (int64x2_t)b);
}

__inline_g int8x8_t
vsubhn_s16(int16x8_t a, int16x8_t b) noexcept
{
  return vsubhn_i16(a, b);
}

__inline_g int16x4_t
vsubhn_s32(int32x4_t a, int32x4_t b) noexcept
{
  return vsubhn_i32(a, b);
}

__inline_g int32x2_t
vsubhn_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vsubhn_i64(a, b);
}

__mc_vnarrow_hn(raddhn, i16, int16x8_t, int8x8_t);
__mc_vnarrow_hn(raddhn, i32, int32x4_t, int16x4_t);
__mc_vnarrow_hn(raddhn, i64, int64x2_t, int32x2_t);
__mc_vnarrow_hn(rsubhn, i16, int16x8_t, int8x8_t);
__mc_vnarrow_hn(rsubhn, i32, int32x4_t, int16x4_t);
__mc_vnarrow_hn(rsubhn, i64, int64x2_t, int32x2_t);

#undef __mc_vnarrow_hn

#define __mc_vrhn_alias(BASE, SUF_FROM, SUF_TO, T_Q, T_D)                                                                                  \
  __inline_g T_D BASE##_##SUF_TO(T_Q a, T_Q b) noexcept { return (T_D)BASE##_##SUF_FROM((decltype(BASE##_##SUF_FROM(a, b)) *)0, a, b); }

#undef __mc_vrhn_alias

__inline_g int8x8_t
vraddhn_s16(int16x8_t a, int16x8_t b) noexcept
{
  return vraddhn_i16(a, b);
}

__inline_g int16x4_t
vraddhn_s32(int32x4_t a, int32x4_t b) noexcept
{
  return vraddhn_i32(a, b);
}

__inline_g int32x2_t
vraddhn_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vraddhn_i64(a, b);
}

__inline_g uint8x8_t
vraddhn_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint8x8_t)vraddhn_i16((int16x8_t)a, (int16x8_t)b);
}

__inline_g uint16x4_t
vraddhn_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint16x4_t)vraddhn_i32((int32x4_t)a, (int32x4_t)b);
}

__inline_g uint32x2_t
vraddhn_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint32x2_t)vraddhn_i64((int64x2_t)a, (int64x2_t)b);
}

__inline_g int8x8_t
vrsubhn_s16(int16x8_t a, int16x8_t b) noexcept
{
  return vrsubhn_i16(a, b);
}

__inline_g int16x4_t
vrsubhn_s32(int32x4_t a, int32x4_t b) noexcept
{
  return vrsubhn_i32(a, b);
}

__inline_g int32x2_t
vrsubhn_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vrsubhn_i64(a, b);
}

__inline_g uint8x8_t
vrsubhn_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  return (uint8x8_t)vrsubhn_i16((int16x8_t)a, (int16x8_t)b);
}

__inline_g uint16x4_t
vrsubhn_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint16x4_t)vrsubhn_i32((int32x4_t)a, (int32x4_t)b);
}

__inline_g uint32x2_t
vrsubhn_u64(uint64x2_t a, uint64x2_t b) noexcept
{
  return (uint32x2_t)vrsubhn_i64((int64x2_t)a, (int64x2_t)b);
}

__inline_g int8x8_t
vmovn_s16(int16x8_t v) noexcept
{
  int8x8_t r;
  __asm__("vmovn.i16 %P0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g int16x4_t
vmovn_s32(int32x4_t v) noexcept
{
  int16x4_t r;
  __asm__("vmovn.i32 %P0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g int32x2_t
vmovn_s64(int64x2_t v) noexcept
{
  int32x2_t r;
  __asm__("vmovn.i64 %P0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g uint8x8_t
vmovn_u16(uint16x8_t v) noexcept
{
  return (uint8x8_t)vmovn_s16((int16x8_t)v);
}

__inline_g uint16x4_t
vmovn_u32(uint32x4_t v) noexcept
{
  return (uint16x4_t)vmovn_s32((int32x4_t)v);
}

__inline_g uint32x2_t
vmovn_u64(uint64x2_t v) noexcept
{
  return (uint32x2_t)vmovn_s64((int64x2_t)v);
}

#define __mc_vqmovn(SUF, ASM, T_Q, T_D)                                                                                                    \
  __inline_g T_D vqmovn_##SUF(T_Q v) noexcept                                                                                              \
  {                                                                                                                                        \
    T_D r;                                                                                                                                 \
    __asm__("vqmovn." ASM " %P0, %q1" : "=w"(r) : "w"(v));                                                                                 \
    return r;                                                                                                                              \
  }

__mc_vqmovn(s16, "s16", int16x8_t, int8x8_t);
__mc_vqmovn(s32, "s32", int32x4_t, int16x4_t);
__mc_vqmovn(s64, "s64", int64x2_t, int32x2_t);
__mc_vqmovn(u16, "u16", uint16x8_t, uint8x8_t);
__mc_vqmovn(u32, "u32", uint32x4_t, uint16x4_t);
__mc_vqmovn(u64, "u64", uint64x2_t, uint32x2_t);

#undef __mc_vqmovn

#define __mc_vqmovun(SUF, ASM, T_Q, T_D)                                                                                                   \
  __inline_g T_D vqmovun_##SUF(T_Q v) noexcept                                                                                             \
  {                                                                                                                                        \
    T_D r;                                                                                                                                 \
    __asm__("vqmovun." ASM " %P0, %q1" : "=w"(r) : "w"(v));                                                                                \
    return r;                                                                                                                              \
  }

__mc_vqmovun(s16, "s16", int16x8_t, uint8x8_t);
__mc_vqmovun(s32, "s32", int32x4_t, uint16x4_t);
__mc_vqmovun(s64, "s64", int64x2_t, uint32x2_t);

#undef __mc_vqmovun

#define __mc_vshlq_n(SUF, T)                                                                                                               \
  __inline_g T vshlq_n_##SUF(T v, const int n) noexcept { return v << n; }

__mc_vshlq_n(s8, int8x16_t);
__mc_vshlq_n(s32, int32x4_t);
__mc_vshlq_n(u8, uint8x16_t);
__mc_vshlq_n(u16, uint16x8_t);
__mc_vshlq_n(u32, uint32x4_t);
__mc_vshlq_n(u64, uint64x2_t);

#undef __mc_vshlq_n

#define __mc_vshrq_n(SUF, T)                                                                                                               \
  __inline_g T vshrq_n_##SUF(T v, const int n) noexcept { return v >> n; }

__mc_vshrq_n(s8, int8x16_t);
__mc_vshrq_n(s16, int16x8_t);
__mc_vshrq_n(s32, int32x4_t);
__mc_vshrq_n(u8, uint8x16_t);
__mc_vshrq_n(u16, uint16x8_t);

#undef __mc_vshrq_n

// vshlq_* must lower to the hardware VSHL (NOT the GCC vector `<<`): a negative
// per-lane count is a RIGHT shift (arithmetic for .s* / logical for .u*) and
// |count| >= lane width yields 0. The `<<` operator makes negative counts UB
// (garbage), but every right-shift caller passes vdupq_n(-count) and relies on
// the real VSHL semantics. Inline asm guarantees the true op. The count operand
// keeps its caller-facing type (signed for the unsigned-data variants).
#define __mc_vshlq_signed(SUF, ASM, T)                                                                                                     \
  __inline_g T vshlq_##SUF(T v, T cnt) noexcept                                                                                            \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vshl." ASM " %q0, %q1, %q2" : "=w"(r) : "w"(v), "w"(cnt));                                                                    \
    return r;                                                                                                                              \
  }

__mc_vshlq_signed(s8, "s8", int8x16_t);
__mc_vshlq_signed(s16, "s16", int16x8_t);
__mc_vshlq_signed(s32, "s32", int32x4_t);
__mc_vshlq_signed(s64, "s64", int64x2_t);

#undef __mc_vshlq_signed

#define __mc_vshlq_unsigned(SUF, ASM, T, CT)                                                                                               \
  __inline_g T vshlq_##SUF(T v, CT cnt) noexcept                                                                                           \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vshl." ASM " %q0, %q1, %q2" : "=w"(r) : "w"(v), "w"(cnt));                                                                    \
    return r;                                                                                                                              \
  }

__mc_vshlq_unsigned(u8, "u8", uint8x16_t, int8x16_t);
__mc_vshlq_unsigned(u16, "u16", uint16x8_t, int16x8_t);
__mc_vshlq_unsigned(u32, "u32", uint32x4_t, int32x4_t);

#undef __mc_vshlq_unsigned

#define __mc_vshq_asm(NAME, ASM, SUF, T_OUT, T_IN)                                                                                         \
  __inline_g T_OUT NAME##_##SUF(T_IN a, T_IN b) noexcept                                                                                   \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__(#NAME "." ASM " %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));                                                                    \
    return r;                                                                                                                              \
  }

__mc_vshq_asm(vqshlq, "s8", s8, int8x16_t, int8x16_t);
__mc_vshq_asm(vqshlq, "s16", s16, int16x8_t, int16x8_t);
__mc_vshq_asm(vqshlq, "s32", s32, int32x4_t, int32x4_t);
__mc_vshq_asm(vqshlq, "s64", s64, int64x2_t, int64x2_t);
__mc_vshq_asm(vqshlq, "u8", u8, uint8x16_t, uint8x16_t);
__mc_vshq_asm(vqshlq, "u16", u16, uint16x8_t, uint16x8_t);
__mc_vshq_asm(vqshlq, "u32", u32, uint32x4_t, uint32x4_t);
__mc_vshq_asm(vqshlq, "u64", u64, uint64x2_t, uint64x2_t);

__mc_vshq_asm(vrshlq, "s8", s8, int8x16_t, int8x16_t);
__mc_vshq_asm(vrshlq, "s16", s16, int16x8_t, int16x8_t);
__mc_vshq_asm(vrshlq, "s32", s32, int32x4_t, int32x4_t);
__mc_vshq_asm(vrshlq, "s64", s64, int64x2_t, int64x2_t);
__mc_vshq_asm(vrshlq, "u8", u8, uint8x16_t, uint8x16_t);
__mc_vshq_asm(vrshlq, "u16", u16, uint16x8_t, uint16x8_t);
__mc_vshq_asm(vrshlq, "u32", u32, uint32x4_t, uint32x4_t);
__mc_vshq_asm(vrshlq, "u64", u64, uint64x2_t, uint64x2_t);

__mc_vshq_asm(vqrshlq, "s8", s8, int8x16_t, int8x16_t);
__mc_vshq_asm(vqrshlq, "s16", s16, int16x8_t, int16x8_t);
__mc_vshq_asm(vqrshlq, "s32", s32, int32x4_t, int32x4_t);
__mc_vshq_asm(vqrshlq, "s64", s64, int64x2_t, int64x2_t);
__mc_vshq_asm(vqrshlq, "u8", u8, uint8x16_t, uint8x16_t);
__mc_vshq_asm(vqrshlq, "u16", u16, uint16x8_t, uint16x8_t);
__mc_vshq_asm(vqrshlq, "u32", u32, uint32x4_t, uint32x4_t);
__mc_vshq_asm(vqrshlq, "u64", u64, uint64x2_t, uint64x2_t);

#undef __mc_vshq_asm

#define __mc_vqshlq_n(SUF, ASM, T)                                                                                                         \
  __inline_g T vqshlq_n_##SUF(T v, const int n) noexcept                                                                                   \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vqshl." ASM " %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));                                                                      \
    return r;                                                                                                                              \
  }

__mc_vqshlq_n(s8, "s8", int8x16_t);
__mc_vqshlq_n(s16, "s16", int16x8_t);
__mc_vqshlq_n(s32, "s32", int32x4_t) __mc_vqshlq_n(s64, "s64", int64x2_t);
__mc_vqshlq_n(u8, "u8", uint8x16_t);
__mc_vqshlq_n(u16, "u16", uint16x8_t);
__mc_vqshlq_n(u32, "u32", uint32x4_t);
__mc_vqshlq_n(u64, "u64", uint64x2_t);

#undef __mc_vqshlq_n

#define __mc_vqshluq_n(SUF, ASM, T_IN, T_OUT)                                                                                              \
  __inline_g T_OUT vqshluq_n_##SUF(T_IN v, const int n) noexcept                                                                           \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__("vqshlu." ASM " %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));                                                                     \
    return r;                                                                                                                              \
  }

__mc_vqshluq_n(s8, "s8", int8x16_t, uint8x16_t);
__mc_vqshluq_n(s16, "s16", int16x8_t, uint16x8_t);
__mc_vqshluq_n(s32, "s32", int32x4_t, uint32x4_t);
__mc_vqshluq_n(s64, "s64", int64x2_t, uint64x2_t);

#undef __mc_vqshluq_n

#define __mc_vsraq_n(SUF, T)                                                                                                               \
  __inline_g T vsraq_n_##SUF(T a, T b, const int n) noexcept { return a + (b >> n); }

__mc_vsraq_n(s8, int8x16_t);
__mc_vsraq_n(s16, int16x8_t);
__mc_vsraq_n(s32, int32x4_t);
__mc_vsraq_n(s64, int64x2_t);
__mc_vsraq_n(u8, uint8x16_t);
__mc_vsraq_n(u16, uint16x8_t);
__mc_vsraq_n(u32, uint32x4_t);
__mc_vsraq_n(u64, uint64x2_t);

#undef __mc_vsraq_n

#define __mc_vrsraq_n(SUF, ASM, T)                                                                                                         \
  __inline_g T vrsraq_n_##SUF(T a, T b, const int n) noexcept                                                                              \
  {                                                                                                                                        \
    T r = a;                                                                                                                               \
    __asm__("vrsra." ASM " %q0, %q1, %2" : "+w"(r) : "w"(b), "i"(n));                                                                      \
    return r;                                                                                                                              \
  }

__mc_vrsraq_n(s8, "s8", int8x16_t);
__mc_vrsraq_n(s16, "s16", int16x8_t);
__mc_vrsraq_n(s32, "s32", int32x4_t);
__mc_vrsraq_n(s64, "s64", int64x2_t);
__mc_vrsraq_n(u8, "u8", uint8x16_t);
__mc_vrsraq_n(u16, "u16", uint16x8_t);
__mc_vrsraq_n(u32, "u32", uint32x4_t);
__mc_vrsraq_n(u64, "u64", uint64x2_t);

#undef __mc_vrsraq_n

#define __mc_vrshrq_n(SUF, ASM, T)                                                                                                         \
  __inline_g T vrshrq_n_##SUF(T v, const int n) noexcept                                                                                   \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vrshr." ASM " %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));                                                                      \
    return r;                                                                                                                              \
  }

__mc_vrshrq_n(s8, "s8", int8x16_t);
__mc_vrshrq_n(s16, "s16", int16x8_t);
__mc_vrshrq_n(s32, "s32", int32x4_t);
__mc_vrshrq_n(s64, "s64", int64x2_t);
__mc_vrshrq_n(u8, "u8", uint8x16_t);
__mc_vrshrq_n(u16, "u16", uint16x8_t);
__mc_vrshrq_n(u32, "u32", uint32x4_t);
__mc_vrshrq_n(u64, "u64", uint64x2_t);

#undef __mc_vrshrq_n

#define __mc_vsliq_n(SUF, ASM, T)                                                                                                          \
  __inline_g T vsliq_n_##SUF(T a, T b, const int n) noexcept                                                                               \
  {                                                                                                                                        \
    T r = a;                                                                                                                               \
    __asm__("vsli." ASM " %q0, %q1, %2" : "+w"(r) : "w"(b), "i"(n));                                                                       \
    return r;                                                                                                                              \
  }

__mc_vsliq_n(s8, "8", int8x16_t);
__mc_vsliq_n(s16, "16", int16x8_t);
__mc_vsliq_n(s32, "32", int32x4_t);
__mc_vsliq_n(s64, "64", int64x2_t);
__mc_vsliq_n(u8, "8", uint8x16_t);
__mc_vsliq_n(u16, "16", uint16x8_t);
__mc_vsliq_n(u32, "32", uint32x4_t);
__mc_vsliq_n(u64, "64", uint64x2_t);

#undef __mc_vsliq_n

#define __mc_vsriq_n(SUF, ASM, T)                                                                                                          \
  __inline_g T vsriq_n_##SUF(T a, T b, const int n) noexcept                                                                               \
  {                                                                                                                                        \
    T r = a;                                                                                                                               \
    __asm__("vsri." ASM " %q0, %q1, %2" : "+w"(r) : "w"(b), "i"(n));                                                                       \
    return r;                                                                                                                              \
  }

__mc_vsriq_n(s8, "8", int8x16_t);
__mc_vsriq_n(s16, "16", int16x8_t);
__mc_vsriq_n(s32, "32", int32x4_t);
__mc_vsriq_n(s64, "64", int64x2_t);
__mc_vsriq_n(u8, "8", uint8x16_t);
__mc_vsriq_n(u16, "16", uint16x8_t);
__mc_vsriq_n(u32, "32", uint32x4_t);
__mc_vsriq_n(u64, "64", uint64x2_t);

#undef __mc_vsriq_n

#define __mc_vshll_n(SUF, ASM, T_IN, T_OUT)                                                                                                \
  __inline_g T_OUT vshll_n_##SUF(T_IN v, const int n) noexcept                                                                             \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__("vshll." ASM " %q0, %P1, %2" : "=w"(r) : "w"(v), "i"(n));                                                                      \
    return r;                                                                                                                              \
  }

__mc_vshll_n(s8, "s8", int8x8_t, int16x8_t);
__mc_vshll_n(s16, "s16", int16x4_t, int32x4_t);
__mc_vshll_n(s32, "s32", int32x2_t, int64x2_t);
__mc_vshll_n(u8, "u8", uint8x8_t, uint16x8_t);
__mc_vshll_n(u16, "u16", uint16x4_t, uint32x4_t);
__mc_vshll_n(u32, "u32", uint32x2_t, uint64x2_t);

#undef __mc_vshll_n

#define __mc_vshrn_n(NAME, SUF, ASM, T_IN, T_OUT)                                                                                          \
  __inline_g T_OUT v##NAME##_n_##SUF(T_IN v, const int n) noexcept                                                                         \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__("v" #NAME "." ASM " %P0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));                                                                 \
    return r;                                                                                                                              \
  }

__mc_vshrn_n(shrn, s16, "i16", int16x8_t, int8x8_t);
__mc_vshrn_n(shrn, s32, "i32", int32x4_t, int16x4_t);
__mc_vshrn_n(shrn, s64, "i64", int64x2_t, int32x2_t);
__mc_vshrn_n(shrn, u16, "i16", uint16x8_t, uint8x8_t);
__mc_vshrn_n(shrn, u32, "i32", uint32x4_t, uint16x4_t);
__mc_vshrn_n(shrn, u64, "i64", uint64x2_t, uint32x2_t);

__mc_vshrn_n(qshrn, s16, "s16", int16x8_t, int8x8_t);
__mc_vshrn_n(qshrn, s32, "s32", int32x4_t, int16x4_t);
__mc_vshrn_n(qshrn, s64, "s64", int64x2_t, int32x2_t);
__mc_vshrn_n(qshrn, u16, "u16", uint16x8_t, uint8x8_t);
__mc_vshrn_n(qshrn, u32, "u32", uint32x4_t, uint16x4_t);
__mc_vshrn_n(qshrn, u64, "u64", uint64x2_t, uint32x2_t);

__mc_vshrn_n(rshrn, s16,
             "i1"
             "6",
             int16x8_t, int8x8_t);
__mc_vshrn_n(rshrn, s32, "i32", int32x4_t, int16x4_t);
__mc_vshrn_n(rshrn, s64, "i64", int64x2_t, int32x2_t);
__mc_vshrn_n(rshrn, u16, "i16", uint16x8_t, uint8x8_t);
__mc_vshrn_n(rshrn, u32, "i32", uint32x4_t, uint16x4_t);
__mc_vshrn_n(rshrn, u64, "i64", uint64x2_t, uint32x2_t);

__mc_vshrn_n(qrshrn, s16, "s16", int16x8_t, int8x8_t);
__mc_vshrn_n(qrshrn, s32, "s32", int32x4_t, int16x4_t);
__mc_vshrn_n(qrshrn, s64, "s64", int64x2_t, int32x2_t);
__mc_vshrn_n(qrshrn, u16, "u16", uint16x8_t, uint8x8_t);
__mc_vshrn_n(qrshrn, u32,
             "u3"
             "2",
             uint32x4_t, uint16x4_t);
__mc_vshrn_n(qrshrn, u64, "u64", uint64x2_t, uint32x2_t);

__mc_vshrn_n(qshrun, s16, "s16", int16x8_t, uint8x8_t);
__mc_vshrn_n(qshrun, s32, "s32", int32x4_t, uint16x4_t);
__mc_vshrn_n(qshrun, s64, "s64", int64x2_t, uint32x2_t);
__mc_vshrn_n(qrshrun, s16, "s16", int16x8_t, uint8x8_t);
__mc_vshrn_n(qrshrun, s32, "s32", int32x4_t, uint16x4_t);
__mc_vshrn_n(qrshrun, s64, "s64", int64x2_t, uint32x2_t);

#undef __mc_vshrn_n

#define __mc_vmm_q(NAME, ASM, SUF, T)                                                                                                      \
  __inline_g T NAME##q_##SUF(T a, T b) noexcept                                                                                            \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__(#NAME "." ASM " %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));                                                                    \
    return r;                                                                                                                              \
  }

__mc_vmm_q(vmax, "s8", s8, int8x16_t);
__mc_vmm_q(vmax, "s16", s16, int16x8_t);
__mc_vmm_q(vmax, "s32", s32, int32x4_t);
__mc_vmm_q(vmax, "u8", u8, uint8x16_t);
__mc_vmm_q(vmax, "u16", u16, uint16x8_t);
__mc_vmm_q(vmax, "u32", u32, uint32x4_t);
__mc_vmm_q(vmax, "f32", f32, float32x4_t);

__mc_vmm_q(vmin, "s8", s8, int8x16_t);
__mc_vmm_q(vmin,
           "s1"
           "6",
           s16, int16x8_t);
__mc_vmm_q(vmin, "s32", s32, int32x4_t);
__mc_vmm_q(vmin, "u8", u8, uint8x16_t);
__mc_vmm_q(vmin, "u16", u16, uint16x8_t);
__mc_vmm_q(vmin, "u32", u32, uint32x4_t);
__mc_vmm_q(vmin, "f32", f32, float32x4_t);

#undef __mc_vmm_q

__inline_g uint32x4_t
__neon32_is_nan_f32(float32x4_t v) noexcept
{
  uint32x4_t bits = (uint32x4_t)v;
  uint32x4_t exp_mask = vdupq_n_u32(0x7F800000u);
  uint32x4_t mantissa_mask = vdupq_n_u32(0x007FFFFFu);
  uint32x4_t exp_all_ones = vceqq_u32(vandq_u32(bits, exp_mask), exp_mask);
  uint32x4_t mantissa_nz = vmvnq_u32(vceqq_u32(vandq_u32(bits, mantissa_mask), vdupq_n_u32(0)));
  return vandq_u32(exp_all_ones, mantissa_nz);
}

__inline_g float32x4_t
vmaxnmq_f32(float32x4_t a, float32x4_t b) noexcept
{
  uint32x4_t a_nan = __neon32_is_nan_f32(a);
  uint32x4_t b_nan = __neon32_is_nan_f32(b);
  float32x4_t mx = vmaxq_f32(a, b);
  float32x4_t r = vbslq_f32(a_nan, b, mx);
  return vbslq_f32(b_nan, a, r);
}

__inline_g float32x4_t
vminnmq_f32(float32x4_t a, float32x4_t b) noexcept
{
  uint32x4_t a_nan = __neon32_is_nan_f32(a);
  uint32x4_t b_nan = __neon32_is_nan_f32(b);
  float32x4_t mn = vminq_f32(a, b);
  float32x4_t r = vbslq_f32(a_nan, b, mn);
  return vbslq_f32(b_nan, a, r);
}

#define __mc_vpadd_d(SUF, ASM, T)                                                                                                          \
  __inline_g T vpadd_##SUF(T a, T b) noexcept                                                                                              \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vpadd." ASM " %P0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                                     \
    return r;                                                                                                                              \
  }

__mc_vpadd_d(s8, "i8", int8x8_t);
__mc_vpadd_d(s16, "i16", int16x4_t);
__mc_vpadd_d(s32, "i32", int32x2_t);
__mc_vpadd_d(u16, "i16", uint16x4_t);
__mc_vpadd_d(u32, "i32", uint32x2_t);
__mc_vpadd_d(f32, "f32", float32x2_t);

#undef __mc_vpadd_d

__inline_g int8x16_t
vpaddq_s8(int8x16_t a, int8x16_t b) noexcept
{
  int8x8_t pa = vpadd_s8((int8x8_t)vget_low_u8((uint8x16_t)a), (int8x8_t)vget_high_u8((uint8x16_t)a));
  int8x8_t pb = vpadd_s8((int8x8_t)vget_low_u8((uint8x16_t)b), (int8x8_t)vget_high_u8((uint8x16_t)b));
  return (int8x16_t)__builtin_shufflevector(pa, pb, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
}

__inline_g uint8x16_t
vpaddq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  return (uint8x16_t)vpaddq_s8((int8x16_t)a, (int8x16_t)b);
}

__inline_g int32x4_t
vpaddq_s32(int32x4_t a, int32x4_t b) noexcept
{
  int32x2_t pa = vpadd_s32((int32x2_t)vget_low_u32((uint32x4_t)a), (int32x2_t)vget_high_u32((uint32x4_t)a));
  int32x2_t pb = vpadd_s32((int32x2_t)vget_low_u32((uint32x4_t)b), (int32x2_t)vget_high_u32((uint32x4_t)b));
  return (int32x4_t)__builtin_shufflevector(pa, pb, 0, 1, 2, 3);
}

__inline_g uint32x4_t
vpaddq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  return (uint32x4_t)vpaddq_s32((int32x4_t)a, (int32x4_t)b);
}

__inline_g float32x4_t
vpaddq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x2_t pa = vpadd_f32(vget_low_f32(a), vget_high_f32(a));
  float32x2_t pb = vpadd_f32(vget_low_f32(b), vget_high_f32(b));
  return vcombine_f32(pa, pb);
}

#define __mc_vpaddl(NAME, SUF, ASM, T_IN, T_OUT)                                                                                           \
  __inline_g T_OUT NAME##_##SUF(T_IN v) noexcept                                                                                           \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__("vpaddl." ASM " %P0, %P1" : "=w"(r) : "w"(v));                                                                                 \
    return r;                                                                                                                              \
  }

__mc_vpaddl(vpaddl, s8, "s8", int8x8_t, int16x4_t);
__mc_vpaddl(vpaddl, s16, "s16", int16x4_t, int32x2_t);
__mc_vpaddl(vpaddl, s32, "s32", int32x2_t, int64x1_t);
__mc_vpaddl(vpaddl, u8, "u8", uint8x8_t, uint16x4_t);
__mc_vpaddl(vpaddl, u16, "u16", uint16x4_t, uint32x2_t);
__mc_vpaddl(vpaddl, u32, "u32", uint32x2_t, uint64x1_t);

#undef __mc_vpaddl

#define __mc_vpaddlq(SUF, ASM, T_IN, T_OUT)                                                                                                \
  __inline_g T_OUT vpaddlq_##SUF(T_IN v) noexcept                                                                                          \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__("vpaddl." ASM " %q0, %q1" : "=w"(r) : "w"(v));                                                                                 \
    return r;                                                                                                                              \
  }

__mc_vpaddlq(s8, "s8", int8x16_t, int16x8_t);
__mc_vpaddlq(s16, "s16", int16x8_t, int32x4_t);
__mc_vpaddlq(s32, "s32", int32x4_t, int64x2_t);
__mc_vpaddlq(u8, "u8", uint8x16_t, uint16x8_t);
__mc_vpaddlq(u16, "u16", uint16x8_t, uint32x4_t);
__mc_vpaddlq(u32, "u32", uint32x4_t, uint64x2_t);

#undef __mc_vpaddlq

#define __mc_vpadal(SUF, ASM, T_ACC, T_IN)                                                                                                 \
  __inline_g T_ACC vpadal_##SUF(T_ACC a, T_IN b) noexcept                                                                                  \
  {                                                                                                                                        \
    T_ACC r = a;                                                                                                                           \
    __asm__("vpadal." ASM " %P0, %P1" : "+w"(r) : "w"(b));                                                                                 \
    return r;                                                                                                                              \
  }

__mc_vpadal(s8, "s8", int16x4_t, int8x8_t);
__mc_vpadal(s16, "s16", int32x2_t, int16x4_t);
__mc_vpadal(s32, "s32", int64x1_t, int32x2_t);
__mc_vpadal(u8, "u8", uint16x4_t, uint8x8_t);
__mc_vpadal(u16, "u16", uint32x2_t, uint16x4_t);
__mc_vpadal(u32, "u32", uint64x1_t, uint32x2_t);

#undef __mc_vpadal

#define __mc_vpadalq(SUF, ASM, T_ACC, T_IN)                                                                                                \
  __inline_g T_ACC vpadalq_##SUF(T_ACC a, T_IN b) noexcept                                                                                 \
  {                                                                                                                                        \
    T_ACC r = a;                                                                                                                           \
    __asm__("vpadal." ASM " %q0, %q1" : "+w"(r) : "w"(b));                                                                                 \
    return r;                                                                                                                              \
  }

__mc_vpadalq(s8, "s8", int16x8_t, int8x16_t);
__mc_vpadalq(s16, "s16", int32x4_t, int16x8_t);
__mc_vpadalq(s32, "s32", int64x2_t, int32x4_t);
__mc_vpadalq(u8, "u8", uint16x8_t, uint8x16_t);
__mc_vpadalq(u16, "u16", uint32x4_t, uint16x8_t);
__mc_vpadalq(u32, "u32", uint64x2_t, uint32x4_t);

#undef __mc_vpadalq

#define __mc_vpmm(NAME, ASM, SUF, T)                                                                                                       \
  __inline_g T NAME##_##SUF(T a, T b) noexcept                                                                                             \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__(#NAME "." ASM " %P0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                                    \
    return r;                                                                                                                              \
  }

__mc_vpmm(vpmax, "s8", s8, int8x8_t);
__mc_vpmm(vpmax, "s16", s16, int16x4_t);
__mc_vpmm(vpmax, "s32", s32, int32x2_t);
__mc_vpmm(vpmax, "u8", u8, uint8x8_t);
__mc_vpmm(vpmax, "u16", u16, uint16x4_t);
__mc_vpmm(vpmax, "u32", u32, uint32x2_t);
__mc_vpmm(vpmax, "f32", f32, float32x2_t);

__mc_vpmm(vpmin, "s8", s8, int8x8_t);
__mc_vpmm(vpmin, "s16", s16, int16x4_t);
__mc_vpmm(vpmin, "s32", s32, int32x2_t);
__mc_vpmm(vpmin, "u8", u8, uint8x8_t);
__mc_vpmm(vpmin, "u16", u16, uint16x4_t);
__mc_vpmm(vpmin, "u32", u32, uint32x2_t);
__mc_vpmm(vpmin, "f32", f32, float32x2_t);

#undef __mc_vpmm

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

__inline_g float32x4_t
vrev64q_f32(float32x4_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2);
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
vrev16q_s8(int8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
}

__inline_g uint8x16_t
vrev16q_u8(uint8x16_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
}

__inline_g uint8x8_t
vrev64_u8(uint8x8_t v) noexcept
{
  return __builtin_shufflevector(v, v, 7, 6, 5, 4, 3, 2, 1, 0);
}

__inline_g uint8x8_t
vrev32_u8(uint8x8_t v) noexcept
{
  return __builtin_shufflevector(v, v, 3, 2, 1, 0, 7, 6, 5, 4);
}

__inline_g uint8x8_t
vrev16_u8(uint8x8_t v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 0, 3, 2, 5, 4, 7, 6);
}

__inline_g int8x16x2_t
vzipq_s8(int8x16_t a, int8x16_t b) noexcept
{
  int8x16x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
  r.val[1] = __builtin_shufflevector(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
  return r;
}

__inline_g uint8x16x2_t
vzipq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  uint8x16x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
  r.val[1] = __builtin_shufflevector(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);
  return r;
}

__inline_g int16x8x2_t
vzipq_s16(int16x8_t a, int16x8_t b) noexcept
{
  int16x8x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 8, 1, 9, 2, 10, 3, 11);
  r.val[1] = __builtin_shufflevector(a, b, 4, 12, 5, 13, 6, 14, 7, 15);
  return r;
}

__inline_g uint16x8x2_t
vzipq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  uint16x8x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 8, 1, 9, 2, 10, 3, 11);
  r.val[1] = __builtin_shufflevector(a, b, 4, 12, 5, 13, 6, 14, 7, 15);
  return r;
}

__inline_g int32x4x2_t
vzipq_s32(int32x4_t a, int32x4_t b) noexcept
{
  int32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 4, 1, 5);
  r.val[1] = __builtin_shufflevector(a, b, 2, 6, 3, 7);
  return r;
}

__inline_g float32x4x2_t
vzipq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 4, 1, 5);
  r.val[1] = __builtin_shufflevector(a, b, 2, 6, 3, 7);
  return r;
}

__inline_g int8x16x2_t
vtrnq_s8(int8x16_t a, int8x16_t b) noexcept
{
  int8x16x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 16, 2, 18, 4, 20, 6, 22, 8, 24, 10, 26, 12, 28, 14, 30);
  r.val[1] = __builtin_shufflevector(a, b, 1, 17, 3, 19, 5, 21, 7, 23, 9, 25, 11, 27, 13, 29, 15, 31);
  return r;
}

__inline_g uint8x16x2_t
vtrnq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  uint8x16x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 16, 2, 18, 4, 20, 6, 22, 8, 24, 10, 26, 12, 28, 14, 30);
  r.val[1] = __builtin_shufflevector(a, b, 1, 17, 3, 19, 5, 21, 7, 23, 9, 25, 11, 27, 13, 29, 15, 31);
  return r;
}

__inline_g int16x8x2_t
vtrnq_s16(int16x8_t a, int16x8_t b) noexcept
{
  int16x8x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 8, 2, 10, 4, 12, 6, 14);
  r.val[1] = __builtin_shufflevector(a, b, 1, 9, 3, 11, 5, 13, 7, 15);
  return r;
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

__inline_g uint16x8x2_t
vtrnq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  uint16x8x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 8, 2, 10, 4, 12, 6, 14);
  r.val[1] = __builtin_shufflevector(a, b, 1, 9, 3, 11, 5, 13, 7, 15);
  return r;
}

__inline_g int32x4x2_t
vtrnq_s32(int32x4_t a, int32x4_t b) noexcept
{
  int32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 4, 2, 6);
  r.val[1] = __builtin_shufflevector(a, b, 1, 5, 3, 7);
  return r;
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

__inline_g uint32x4x2_t
vtrnq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  uint32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 4, 2, 6);
  r.val[1] = __builtin_shufflevector(a, b, 1, 5, 3, 7);
  return r;
}

__inline_g float32x4x2_t
vtrnq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 4, 2, 6);
  r.val[1] = __builtin_shufflevector(a, b, 1, 5, 3, 7);
  return r;
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

__inline_g int8x16x2_t
vuzpq_s8(int8x16_t a, int8x16_t b) noexcept
{
  int8x16x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
  return r;
}

__inline_g uint8x16x2_t
vuzpq_u8(uint8x16_t a, uint8x16_t b) noexcept
{
  uint8x16x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);
  return r;
}

__inline_g int16x8x2_t
vuzpq_s16(int16x8_t a, int16x8_t b) noexcept
{
  int16x8x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
  return r;
}

__inline_g uint16x8x2_t
vuzpq_u16(uint16x8_t a, uint16x8_t b) noexcept
{
  uint16x8x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6, 8, 10, 12, 14);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7, 9, 11, 13, 15);
  return r;
}

__inline_g int32x4x2_t
vuzpq_s32(int32x4_t a, int32x4_t b) noexcept
{
  int32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7);
  return r;
}

__inline_g uint32x4x2_t
vuzpq_u32(uint32x4_t a, uint32x4_t b) noexcept
{
  uint32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7);
  return r;
}

__inline_g float32x4x2_t
vuzpq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x4x2_t r;
  r.val[0] = __builtin_shufflevector(a, b, 0, 2, 4, 6);
  r.val[1] = __builtin_shufflevector(a, b, 1, 3, 5, 7);
  return r;
}

// D-register (64-bit) unzips: val[0] gathers the even lanes of a then b, val[1]
// the odd lanes — the standard VUZP.<n> deinterleave the half-width arith paths
// (mul_32_64 / mul_u32_64 / maddubs_8) rely on.
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

__inline_g uint8x8_t
vtbl1_u8(uint8x8_t table, uint8x8_t idx) noexcept
{
  uint8x8_t r;
  __asm__("vtbl.8 %P0, {%P1}, %P2" : "=w"(r) : "w"(table), "w"(idx));
  return r;
}

__inline_g int8x8_t
vtbl1_s8(int8x8_t table, int8x8_t idx) noexcept
{
  int8x8_t r;
  __asm__("vtbl.8 %P0, {%P1}, %P2" : "=w"(r) : "w"(table), "w"(idx));
  return r;
}

__inline_g uint8x8_t
vtbx1_u8(uint8x8_t dst, uint8x8_t table, uint8x8_t idx) noexcept
{
  uint8x8_t r = dst;
  __asm__("vtbx.8 %P0, {%P1}, %P2" : "+w"(r) : "w"(table), "w"(idx));
  return r;
}

__inline_g int8x8_t
vtbx1_s8(int8x8_t dst, int8x8_t table, int8x8_t idx) noexcept
{
  int8x8_t r = dst;
  __asm__("vtbx.8 %P0, {%P1}, %P2" : "+w"(r) : "w"(table), "w"(idx));
  return r;
}

__inline_g float32x4_t
vcvtq_f32_s32(int32x4_t v) noexcept
{
  float32x4_t r;
  __asm__("vcvt.f32.s32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g float32x4_t
vcvtq_f32_u32(uint32x4_t v) noexcept
{
  float32x4_t r;
  __asm__("vcvt.f32.u32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g int32x4_t
vcvtq_s32_f32(float32x4_t v) noexcept
{
  int32x4_t r;
  __asm__("vcvt.s32.f32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g uint32x4_t
vcvtq_u32_f32(float32x4_t v) noexcept
{
  uint32x4_t r;
  __asm__("vcvt.u32.f32 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g float32x4_t
vcvtq_n_f32_s32(int32x4_t v, const int n) noexcept
{
  float32x4_t r;
  __asm__("vcvt.f32.s32 %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));
  return r;
}

__inline_g float32x4_t
vcvtq_n_f32_u32(uint32x4_t v, const int n) noexcept
{
  float32x4_t r;
  __asm__("vcvt.f32.u32 %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));
  return r;
}

__inline_g int32x4_t
vcvtq_n_s32_f32(float32x4_t v, const int n) noexcept
{
  int32x4_t r;
  __asm__("vcvt.s32.f32 %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));
  return r;
}

__inline_g uint32x4_t
vcvtq_n_u32_f32(float32x4_t v, const int n) noexcept
{
  uint32x4_t r;
  __asm__("vcvt.u32.f32 %q0, %q1, %2" : "=w"(r) : "w"(v), "i"(n));
  return r;
}

#define __mc_vmovl(SUF, ASM, T_IN, T_OUT)                                                                                                  \
  __inline_g T_OUT vmovl_##SUF(T_IN v) noexcept                                                                                            \
  {                                                                                                                                        \
    T_OUT r;                                                                                                                               \
    __asm__("vmovl." ASM " %q0, %P1" : "=w"(r) : "w"(v));                                                                                  \
    return r;                                                                                                                              \
  }

__mc_vmovl(s8, "s8", int8x8_t, int16x8_t);
__mc_vmovl(s16, "s16", int16x4_t, int32x4_t);
__mc_vmovl(s32, "s32", int32x2_t, int64x2_t);
__mc_vmovl(u8, "u8", uint8x8_t, uint16x8_t);
__mc_vmovl(u16, "u16", uint16x4_t, uint32x4_t);
__mc_vmovl(u32, "u32", uint32x2_t, uint64x2_t);

#undef __mc_vmovl

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__micron_arm_fp16)
__inline_g float32x4_t
vcvt_f32_f16(float16x4_t v) noexcept
{
  float32x4_t r;
  __asm__("vcvt.f32.f16 %q0, %P1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g float16x4_t
vcvt_f16_f32(float32x4_t v) noexcept
{
  float16x4_t r;
  __asm__("vcvt.f16.f32 %P0, %q1" : "=w"(r) : "w"(v));
  return r;
}
#endif

#define __mc_vclzq(SUF, ASM, T)                                                                                                            \
  __inline_g T vclzq_##SUF(T v) noexcept                                                                                                   \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vclz." ASM " %q0, %q1" : "=w"(r) : "w"(v));                                                                                   \
    return r;                                                                                                                              \
  }

__mc_vclzq(s8, "i8", int8x16_t);
__mc_vclzq(s16, "i16", int16x8_t);
__mc_vclzq(s32, "i32", int32x4_t);
__mc_vclzq(u8, "i8", uint8x16_t);
__mc_vclzq(u16, "i16", uint16x8_t);
__mc_vclzq(u32, "i32", uint32x4_t);

#undef __mc_vclzq

#define __mc_vclsq(SUF, ASM, T)                                                                                                            \
  __inline_g T vclsq_##SUF(T v) noexcept                                                                                                   \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vcls." ASM " %q0, %q1" : "=w"(r) : "w"(v));                                                                                   \
    return r;                                                                                                                              \
  }

__mc_vclsq(s8, "s8", int8x16_t);
__mc_vclsq(s16, "s16", int16x8_t);
__mc_vclsq(s32, "s32", int32x4_t);

#undef __mc_vclsq

__inline_g uint8x16_t
vcntq_u8(uint8x16_t v) noexcept
{
  uint8x16_t r;
  __asm__("vcnt.8 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

__inline_g int8x16_t
vcntq_s8(int8x16_t v) noexcept
{
  int8x16_t r;
  __asm__("vcnt.8 %q0, %q1" : "=w"(r) : "w"(v));
  return r;
}

#define __mc_vld1q_dup(SUF, T, ETYPE)                                                                                                      \
  __inline_g T vld1q_dup_##SUF(const ETYPE *p) noexcept { return vdupq_n_##SUF(*p); }

__mc_vld1q_dup(s8, int8x16_t, signed char);
__mc_vld1q_dup(s16, int16x8_t, signed short);
__mc_vld1q_dup(s32, int32x4_t, signed int);
__mc_vld1q_dup(s64, int64x2_t, signed long long);
__mc_vld1q_dup(u8, uint8x16_t, unsigned char);
__mc_vld1q_dup(u16, uint16x8_t, unsigned short);
__mc_vld1q_dup(u32, uint32x4_t, unsigned int);
__mc_vld1q_dup(u64, uint64x2_t, unsigned long long);

#undef __mc_vld1q_dup

#define __mc_vld1q_lane(SUF, T, ETYPE)                                                                                                     \
  __inline_g T vld1q_lane_##SUF(const ETYPE *p, T v, const int lane) noexcept                                                              \
  {                                                                                                                                        \
    v[lane] = *p;                                                                                                                          \
    return v;                                                                                                                              \
  }                                                                                                                                        \
  __inline_g void vst1q_lane_##SUF(ETYPE *p, T v, const int lane) noexcept { *p = v[lane]; }

__mc_vld1q_lane(s8, int8x16_t, signed char);
__mc_vld1q_lane(s16, int16x8_t, signed short);
__mc_vld1q_lane(s32, int32x4_t, signed int);
__mc_vld1q_lane(s64, int64x2_t, signed long long);
__mc_vld1q_lane(u8, uint8x16_t, unsigned char);
__mc_vld1q_lane(u16, uint16x8_t, unsigned short);
__mc_vld1q_lane(u32, uint32x4_t, unsigned int);
__mc_vld1q_lane(u64, uint64x2_t, unsigned long long);
__mc_vld1q_lane(f32, float32x4_t, float);

#undef __mc_vld1q_lane

__inline_g void
vst1_lane_u32(unsigned int *p, uint32x2_t v, const int lane) noexcept
{
  *p = v[lane];
}

#define __mc_vld2q_8(SUF, TBASE, ETYPE)                                                                                                    \
  __inline_g TBASE##x2_t vld2q_##SUF(const ETYPE *p) noexcept                                                                              \
  {                                                                                                                                        \
    TBASE##_t lo = vld1q_##SUF(p);                                                                                                         \
    TBASE##_t hi = vld1q_##SUF(p + 16);                                                                                                    \
    TBASE##x2_t r;                                                                                                                         \
    r.val[0] = __builtin_shufflevector(lo, hi, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);                                 \
    r.val[1] = __builtin_shufflevector(lo, hi, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31);                                 \
    return r;                                                                                                                              \
  }

__mc_vld2q_8(s8, int8x16, signed char);
__mc_vld2q_8(u8, uint8x16, unsigned char);

#undef __mc_vld2q_8

#define __mc_vld2q_16(SUF, TBASE, ETYPE)                                                                                                   \
  __inline_g TBASE##x2_t vld2q_##SUF(const ETYPE *p) noexcept                                                                              \
  {                                                                                                                                        \
    TBASE##_t lo = vld1q_##SUF(p);                                                                                                         \
    TBASE##_t hi = vld1q_##SUF(p + 8);                                                                                                     \
    TBASE##x2_t r;                                                                                                                         \
    r.val[0] = __builtin_shufflevector(lo, hi, 0, 2, 4, 6, 8, 10, 12, 14);                                                                 \
    r.val[1] = __builtin_shufflevector(lo, hi, 1, 3, 5, 7, 9, 11, 13, 15);                                                                 \
    return r;                                                                                                                              \
  }

__mc_vld2q_16(s16, int16x8, signed short);
__mc_vld2q_16(u16, uint16x8, unsigned short);

#undef __mc_vld2q_16

#define __mc_vld2q_32(SUF, TBASE, ETYPE)                                                                                                   \
  __inline_g TBASE##x2_t vld2q_##SUF(const ETYPE *p) noexcept                                                                              \
  {                                                                                                                                        \
    TBASE##_t lo = vld1q_##SUF(p);                                                                                                         \
    TBASE##_t hi = vld1q_##SUF(p + 4);                                                                                                     \
    TBASE##x2_t r;                                                                                                                         \
    r.val[0] = __builtin_shufflevector(lo, hi, 0, 2, 4, 6);                                                                                \
    r.val[1] = __builtin_shufflevector(lo, hi, 1, 3, 5, 7);                                                                                \
    return r;                                                                                                                              \
  }

__mc_vld2q_32(s32, int32x4, signed int);
__mc_vld2q_32(u32, uint32x4, unsigned int);
__mc_vld2q_32(f32, float32x4, float);

#undef __mc_vld2q_32

#define __mc_vst2q_8(SUF, TBASE, ETYPE)                                                                                                    \
  __inline_g void vst2q_##SUF(ETYPE *p, TBASE##x2_t v) noexcept                                                                            \
  {                                                                                                                                        \
    TBASE##_t lo = __builtin_shufflevector(v.val[0], v.val[1], 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);                    \
    TBASE##_t hi = __builtin_shufflevector(v.val[0], v.val[1], 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);              \
    vst1q_##SUF(p, lo);                                                                                                                    \
    vst1q_##SUF(p + 16, hi);                                                                                                               \
  }

__mc_vst2q_8(s8, int8x16, signed char);
__mc_vst2q_8(u8, uint8x16, unsigned char);

#undef __mc_vst2q_8

#define __mc_vst2q_16(SUF, TBASE, ETYPE)                                                                                                   \
  __inline_g void vst2q_##SUF(ETYPE *p, TBASE##x2_t v) noexcept                                                                            \
  {                                                                                                                                        \
    TBASE##_t lo = __builtin_shufflevector(v.val[0], v.val[1], 0, 8, 1, 9, 2, 10, 3, 11);                                                  \
    TBASE##_t hi = __builtin_shufflevector(v.val[0], v.val[1], 4, 12, 5, 13, 6, 14, 7, 15);                                                \
    vst1q_##SUF(p, lo);                                                                                                                    \
    vst1q_##SUF(p + 8, hi);                                                                                                                \
  }

__mc_vst2q_16(s16, int16x8, signed short);
__mc_vst2q_16(u16, uint16x8, unsigned short);

#undef __mc_vst2q_16

#define __mc_vst2q_32(SUF, TBASE, ETYPE)                                                                                                   \
  __inline_g void vst2q_##SUF(ETYPE *p, TBASE##x2_t v) noexcept                                                                            \
  {                                                                                                                                        \
    TBASE##_t lo = __builtin_shufflevector(v.val[0], v.val[1], 0, 4, 1, 5);                                                                \
    TBASE##_t hi = __builtin_shufflevector(v.val[0], v.val[1], 2, 6, 3, 7);                                                                \
    vst1q_##SUF(p, lo);                                                                                                                    \
    vst1q_##SUF(p + 4, hi);                                                                                                                \
  }

__mc_vst2q_32(s32, int32x4, signed int);
__mc_vst2q_32(u32, uint32x4, unsigned int);
__mc_vst2q_32(f32, float32x4, float);

#undef __mc_vst2q_32

__inline_g int32x4x3_t
vld3q_s32(const signed int *p) noexcept
{

  int32x4_t v0 = vld1q_s32(p);
  int32x4_t v1 = vld1q_s32(p + 4);
  int32x4_t v2 = vld1q_s32(p + 8);
  int32x4x3_t r;
  r.val[0] = (int32x4_t){ v0[0], v0[3], v1[2], v2[1] };
  r.val[1] = (int32x4_t){ v0[1], v1[0], v1[3], v2[2] };
  r.val[2] = (int32x4_t){ v0[2], v1[1], v2[0], v2[3] };
  return r;
}

__inline_g uint32x4x3_t
vld3q_u32(const unsigned int *p) noexcept
{
  int32x4x3_t s = vld3q_s32((const signed int *)p);
  uint32x4x3_t r;
  r.val[0] = (uint32x4_t)s.val[0];
  r.val[1] = (uint32x4_t)s.val[1];
  r.val[2] = (uint32x4_t)s.val[2];
  return r;
}

__inline_g float32x4x3_t
vld3q_f32(const float *p) noexcept
{
  float32x4_t v0 = vld1q_f32(p);
  float32x4_t v1 = vld1q_f32(p + 4);
  float32x4_t v2 = vld1q_f32(p + 8);
  float32x4x3_t r;
  r.val[0] = (float32x4_t){ v0[0], v0[3], v1[2], v2[1] };
  r.val[1] = (float32x4_t){ v0[1], v1[0], v1[3], v2[2] };
  r.val[2] = (float32x4_t){ v0[2], v1[1], v2[0], v2[3] };
  return r;
}

__inline_g void
vst3q_s32(signed int *p, int32x4x3_t v) noexcept
{

  for ( int i = 0; i < 4; ++i ) {
    p[i * 3 + 0] = v.val[0][i];
    p[i * 3 + 1] = v.val[1][i];
    p[i * 3 + 2] = v.val[2][i];
  }
}

__inline_g void
vst3q_u32(unsigned int *p, uint32x4x3_t v) noexcept
{
  int32x4x3_t s;
  s.val[0] = (int32x4_t)v.val[0];
  s.val[1] = (int32x4_t)v.val[1];
  s.val[2] = (int32x4_t)v.val[2];
  vst3q_s32((signed int *)p, s);
}

__inline_g void
vst3q_f32(float *p, float32x4x3_t v) noexcept
{
  for ( int i = 0; i < 4; ++i ) {
    p[i * 3 + 0] = v.val[0][i];
    p[i * 3 + 1] = v.val[1][i];
    p[i * 3 + 2] = v.val[2][i];
  }
}

__inline_g int32x4x4_t
vld4q_s32(const signed int *p) noexcept
{
  int32x4_t v0 = vld1q_s32(p);
  int32x4_t v1 = vld1q_s32(p + 4);
  int32x4_t v2 = vld1q_s32(p + 8);
  int32x4_t v3 = vld1q_s32(p + 12);
  int32x4x4_t r;
  r.val[0] = (int32x4_t){ v0[0], v1[0], v2[0], v3[0] };
  r.val[1] = (int32x4_t){ v0[1], v1[1], v2[1], v3[1] };
  r.val[2] = (int32x4_t){ v0[2], v1[2], v2[2], v3[2] };
  r.val[3] = (int32x4_t){ v0[3], v1[3], v2[3], v3[3] };
  return r;
}

__inline_g uint32x4x4_t
vld4q_u32(const unsigned int *p) noexcept
{
  int32x4x4_t s = vld4q_s32((const signed int *)p);
  uint32x4x4_t r;
  r.val[0] = (uint32x4_t)s.val[0];
  r.val[1] = (uint32x4_t)s.val[1];
  r.val[2] = (uint32x4_t)s.val[2];
  r.val[3] = (uint32x4_t)s.val[3];
  return r;
}

__inline_g float32x4x4_t
vld4q_f32(const float *p) noexcept
{
  float32x4_t v0 = vld1q_f32(p);
  float32x4_t v1 = vld1q_f32(p + 4);
  float32x4_t v2 = vld1q_f32(p + 8);
  float32x4_t v3 = vld1q_f32(p + 12);
  float32x4x4_t r;
  r.val[0] = (float32x4_t){ v0[0], v1[0], v2[0], v3[0] };
  r.val[1] = (float32x4_t){ v0[1], v1[1], v2[1], v3[1] };
  r.val[2] = (float32x4_t){ v0[2], v1[2], v2[2], v3[2] };
  r.val[3] = (float32x4_t){ v0[3], v1[3], v2[3], v3[3] };
  return r;
}

__inline_g void
vst4q_s32(signed int *p, int32x4x4_t v) noexcept
{
  for ( int i = 0; i < 4; ++i ) {
    p[i * 4 + 0] = v.val[0][i];
    p[i * 4 + 1] = v.val[1][i];
    p[i * 4 + 2] = v.val[2][i];
    p[i * 4 + 3] = v.val[3][i];
  }
}

__inline_g void
vst4q_u32(unsigned int *p, uint32x4x4_t v) noexcept
{
  int32x4x4_t s;
  s.val[0] = (int32x4_t)v.val[0];
  s.val[1] = (int32x4_t)v.val[1];
  s.val[2] = (int32x4_t)v.val[2];
  s.val[3] = (int32x4_t)v.val[3];
  vst4q_s32((signed int *)p, s);
}

__inline_g void
vst4q_f32(float *p, float32x4x4_t v) noexcept
{
  for ( int i = 0; i < 4; ++i ) {
    p[i * 4 + 0] = v.val[0][i];
    p[i * 4 + 1] = v.val[1][i];
    p[i * 4 + 2] = v.val[2][i];
    p[i * 4 + 3] = v.val[3][i];
  }
}

__inline_g uint8x16x3_t
vld3q_u8(const unsigned char *p) noexcept
{
  uint8x16x3_t r;
  for ( int i = 0; i < 16; ++i ) {
    r.val[0][i] = p[i * 3 + 0];
    r.val[1][i] = p[i * 3 + 1];
    r.val[2][i] = p[i * 3 + 2];
  }
  return r;
}

__inline_g uint8x16x4_t
vld4q_u8(const unsigned char *p) noexcept
{
  uint8x16x4_t r;
  for ( int i = 0; i < 16; ++i ) {
    r.val[0][i] = p[i * 4 + 0];
    r.val[1][i] = p[i * 4 + 1];
    r.val[2][i] = p[i * 4 + 2];
    r.val[3][i] = p[i * 4 + 3];
  }
  return r;
}

__inline_g void
vst3q_u8(unsigned char *p, uint8x16x3_t v) noexcept
{
  for ( int i = 0; i < 16; ++i ) {
    p[i * 3 + 0] = v.val[0][i];
    p[i * 3 + 1] = v.val[1][i];
    p[i * 3 + 2] = v.val[2][i];
  }
}

__inline_g void
vst4q_u8(unsigned char *p, uint8x16x4_t v) noexcept
{
  for ( int i = 0; i < 16; ++i ) {
    p[i * 4 + 0] = v.val[0][i];
    p[i * 4 + 1] = v.val[1][i];
    p[i * 4 + 2] = v.val[2][i];
    p[i * 4 + 3] = v.val[3][i];
  }
}

__inline_g uint16x8_t
vmull_p8(poly8x8_t a, poly8x8_t b) noexcept
{
  uint16x8_t r;
  __asm__("vmull.p8 %q0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_g poly8x16_t
vextq_p8(poly8x16_t a, poly8x16_t b, const int n) noexcept
{
  return (poly8x16_t)vextq_s8((int8x16_t)a, (int8x16_t)b, n);
}

__inline_g poly16x8_t
vextq_p16(poly16x8_t a, poly16x8_t b, const int n) noexcept
{
  return (poly16x8_t)vextq_s16((int16x8_t)a, (int16x8_t)b, n);
}

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)

__inline_g uint8x16_t
vmull_p64(poly64x1_t a, poly64x1_t b) noexcept
{
  uint8x16_t r;
  __asm__("vmull.p64 %q0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}
#endif

#define __mc_vop_d(NAME, OP, SUF, T)                                                                                                       \
  __inline_g T NAME##_##SUF(T a, T b) noexcept { return a OP b; }

__mc_vop_d(vadd, +, s8, int8x8_t);
__mc_vop_d(vadd, +, s16, int16x4_t);
__mc_vop_d(vadd, +, s32, int32x2_t);
__mc_vop_d(vadd, +, s64, int64x1_t);
__mc_vop_d(vadd, +, u8, uint8x8_t);
__mc_vop_d(vadd, +, u16, uint16x4_t);
__mc_vop_d(vadd, +, u32, uint32x2_t);
__mc_vop_d(vadd, +, u64, uint64x1_t);
__mc_vop_d(vadd, +, f32, float32x2_t);

__mc_vop_d(vsub, -, s8, int8x8_t);
__mc_vop_d(vsub, -, s16, int16x4_t);
__mc_vop_d(vsub, -, s32, int32x2_t);
__mc_vop_d(vsub, -, s64, int64x1_t);
__mc_vop_d(vsub, -, u8, uint8x8_t);
__mc_vop_d(vsub, -, u16, uint16x4_t);
__mc_vop_d(vsub, -, u32, uint32x2_t);
__mc_vop_d(vsub, -, u64, uint64x1_t);
__mc_vop_d(vsub, -, f32, float32x2_t);

__mc_vop_d(vmul, *, s8, int8x8_t);
__mc_vop_d(vmul, *, s16, int16x4_t);
__mc_vop_d(vmul, *, s32, int32x2_t);
__mc_vop_d(vmul, *, u8, uint8x8_t);
__mc_vop_d(vmul, *, u16, uint16x4_t);
__mc_vop_d(vmul, *, u32, uint32x2_t);
__mc_vop_d(vmul, *, f32, float32x2_t);

#undef __mc_vop_d

#define __mc_vbit_d(NAME, OP, SUF, T)                                                                                                      \
  __inline_g T NAME##_##SUF(T a, T b) noexcept { return a OP b; }

__mc_vbit_d(vand, &, u8, uint8x8_t);
__mc_vbit_d(vand, &, u16, uint16x4_t);
__mc_vbit_d(vand, &, u32, uint32x2_t);
__mc_vbit_d(vand, &, u64, uint64x1_t);
__mc_vbit_d(vorr, |, u8, uint8x8_t);
__mc_vbit_d(vorr, |, u16, uint16x4_t);
__mc_vbit_d(vorr, |, u32, uint32x2_t);
__mc_vbit_d(vorr, |, u64, uint64x1_t);
__mc_vbit_d(veor, ^, u8, uint8x8_t);
__mc_vbit_d(veor, ^, u16, uint16x4_t);
__mc_vbit_d(veor, ^, u32, uint32x2_t);
__mc_vbit_d(veor, ^, u64, uint64x1_t);

#undef __mc_vbit_d

#define __mc_vbit_d_signed(NAME, OP, SUF, T, U)                                                                                            \
  __inline_g T NAME##_##SUF(T a, T b) noexcept { return (T)((U)a OP(U) b); }

__mc_vbit_d_signed(vand, &, s8, int8x8_t, uint8x8_t);
__mc_vbit_d_signed(vand, &, s16, int16x4_t, uint16x4_t);
__mc_vbit_d_signed(vand, &, s32, int32x2_t, uint32x2_t);
__mc_vbit_d_signed(vand, &, s64, int64x1_t, uint64x1_t);
__mc_vbit_d_signed(vorr, |, s8, int8x8_t, uint8x8_t);
__mc_vbit_d_signed(vorr, |, s16, int16x4_t, uint16x4_t);
__mc_vbit_d_signed(vorr, |, s32, int32x2_t, uint32x2_t);
__mc_vbit_d_signed(vorr, |, s64, int64x1_t, uint64x1_t);
__mc_vbit_d_signed(veor, ^, s8, int8x8_t, uint8x8_t);
__mc_vbit_d_signed(veor, ^, s16, int16x4_t, uint16x4_t);
__mc_vbit_d_signed(veor, ^, s32, int32x2_t, uint32x2_t);
__mc_vbit_d_signed(veor, ^, s64, int64x1_t, uint64x1_t);

#undef __mc_vbit_d_signed

__inline_g uint8x8_t
vmvn_u8(uint8x8_t a) noexcept
{
  return ~a;
}

__inline_g uint16x4_t
vmvn_u16(uint16x4_t a) noexcept
{
  return ~a;
}

__inline_g uint32x2_t
vmvn_u32(uint32x2_t a) noexcept
{
  return ~a;
}

__inline_g int8x8_t
vmvn_s8(int8x8_t a) noexcept
{
  return (int8x8_t) ~(uint8x8_t)a;
}

__inline_g int16x4_t
vmvn_s16(int16x4_t a) noexcept
{
  return (int16x4_t) ~(uint16x4_t)a;
}

__inline_g int32x2_t
vmvn_s32(int32x2_t a) noexcept
{
  return (int32x2_t) ~(uint32x2_t)a;
}

#define __mc_vbic_d(SUF, T)                                                                                                                \
  __inline_g T vbic_##SUF(T a, T b) noexcept { return a & ~b; }                                                                            \
  __inline_g T vorn_##SUF(T a, T b) noexcept { return a | ~b; }

__mc_vbic_d(u8, uint8x8_t) __mc_vbic_d(u16, uint16x4_t) __mc_vbic_d(u32, uint32x2_t) __mc_vbic_d(u64, uint64x1_t)

#undef __mc_vbic_d

#define __mc_vcmp_d(NAME, OP)                                                                                                              \
  __inline_g uint8x8_t NAME##_s8(int8x8_t a, int8x8_t b) noexcept { return (uint8x8_t)(a OP b); }                                          \
  __inline_g uint16x4_t NAME##_s16(int16x4_t a, int16x4_t b) noexcept { return (uint16x4_t)(a OP b); }                                     \
  __inline_g uint32x2_t NAME##_s32(int32x2_t a, int32x2_t b) noexcept { return (uint32x2_t)(a OP b); }                                     \
  __inline_g uint8x8_t NAME##_u8(uint8x8_t a, uint8x8_t b) noexcept { return (uint8x8_t)(a OP b); }                                        \
  __inline_g uint16x4_t NAME##_u16(uint16x4_t a, uint16x4_t b) noexcept { return (uint16x4_t)(a OP b); }                                   \
  __inline_g uint32x2_t NAME##_u32(uint32x2_t a, uint32x2_t b) noexcept { return (uint32x2_t)(a OP b); }                                   \
  __inline_g uint32x2_t NAME##_f32(float32x2_t a, float32x2_t b) noexcept { return (uint32x2_t)(a OP b); }

    __mc_vcmp_d(vceq, ==) __mc_vcmp_d(vcgt, >) __mc_vcmp_d(vclt, <) __mc_vcmp_d(vcle, <=) __mc_vcmp_d(vcge, >=)

#undef __mc_vcmp_d

#define __mc_vtst_d(SUF, T, RT)                                                                                                            \
  __inline_g RT vtst_##SUF(T a, T b) noexcept { return (RT)((a & b) != T{}); }

        __mc_vtst_d(s8, int8x8_t, uint8x8_t);
__mc_vtst_d(s16, int16x4_t, uint16x4_t);
__mc_vtst_d(s32, int32x2_t, uint32x2_t);
__mc_vtst_d(u8, uint8x8_t, uint8x8_t);
__mc_vtst_d(u16, uint16x4_t, uint16x4_t);
__mc_vtst_d(u32, uint32x2_t, uint32x2_t);

#undef __mc_vtst_d

#define __mc_vshl_n_d(SUF, T)                                                                                                              \
  __inline_g T vshl_n_##SUF(T v, const int n) noexcept { return v << n; }                                                                  \
  __inline_g T vshr_n_##SUF(T v, const int n) noexcept { return v >> n; }

__mc_vshl_n_d(s8, int8x8_t);
__mc_vshl_n_d(s16, int16x4_t);
__mc_vshl_n_d(s32, int32x2_t);
__mc_vshl_n_d(u8, uint8x8_t);
__mc_vshl_n_d(u16, uint16x4_t);
__mc_vshl_n_d(u32, uint32x2_t);
__mc_vshl_n_d(u64, uint64x1_t);

#undef __mc_vshl_n_d

__inline_g int64x1_t
vshr_n_s64(int64x1_t v, const int n) noexcept
{
  int64x1_t r;
  __asm__("vshr.s64 %P0, %P1, %2" : "=w"(r) : "w"(v), "i"(n));
  return r;
}

__inline_g int64x1_t
vshl_n_s64(int64x1_t v, const int n) noexcept
{
  return v << n;
}

#define __mc_vmm_d(NAME, ASM, SUF, T)                                                                                                      \
  __inline_g T NAME##_##SUF(T a, T b) noexcept                                                                                             \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__(#NAME "." ASM " %P0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                                    \
    return r;                                                                                                                              \
  }

__mc_vmm_d(vmax, "s8", s8, int8x8_t);
__mc_vmm_d(vmax, "s16", s16, int16x4_t);
__mc_vmm_d(vmax, "s32", s32, int32x2_t);
__mc_vmm_d(vmax, "u8", u8, uint8x8_t);
__mc_vmm_d(vmax, "u16", u16, uint16x4_t);
__mc_vmm_d(vmax, "u32", u32, uint32x2_t);
__mc_vmm_d(vmax, "f32", f32, float32x2_t);

__mc_vmm_d(vmin, "s8", s8, int8x8_t);
__mc_vmm_d(vmin, "s16", s16, int16x4_t);
__mc_vmm_d(vmin, "s32", s32, int32x2_t);
__mc_vmm_d(vmin, "u8", u8, uint8x8_t);
__mc_vmm_d(vmin, "u16", u16, uint16x4_t);
__mc_vmm_d(vmin, "u32", u32, uint32x2_t);
__mc_vmm_d(vmin, "f32", f32, float32x2_t);

#undef __mc_vmm_d

#define __mc_vabs_d(SUF, ASM, T)                                                                                                           \
  __inline_g T vabs_##SUF(T v) noexcept                                                                                                    \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__("vabs." ASM " %P0, %P1" : "=w"(r) : "w"(v));                                                                                   \
    return r;                                                                                                                              \
  }

__mc_vabs_d(s8, "s8", int8x8_t);
__mc_vabs_d(s16, "s16", int16x4_t);
__mc_vabs_d(s32, "s32", int32x2_t);
__mc_vabs_d(f32, "f32", float32x2_t);

#undef __mc_vabs_d

#define __mc_vbsl_d(SUF, T, MASK)                                                                                                          \
  __inline_g T vbsl_##SUF(MASK m, T a, T b) noexcept                                                                                       \
  {                                                                                                                                        \
    T r = (T)m;                                                                                                                            \
    __asm__("vbsl %P0, %P1, %P2" : "+w"(r) : "w"(a), "w"(b));                                                                              \
    return r;                                                                                                                              \
  }

__mc_vbsl_d(u8, uint8x8_t, uint8x8_t);
__mc_vbsl_d(u16, uint16x4_t, uint16x4_t);
__mc_vbsl_d(u32, uint32x2_t, uint32x2_t);
__mc_vbsl_d(u64, uint64x1_t, uint64x1_t);
__mc_vbsl_d(s8, int8x8_t, uint8x8_t);
__mc_vbsl_d(s16, int16x4_t, uint16x4_t);
__mc_vbsl_d(s32, int32x2_t, uint32x2_t);
__mc_vbsl_d(s64, int64x1_t, uint64x1_t);
__mc_vbsl_d(f32, float32x2_t, uint32x2_t);

#undef __mc_vbsl_d

__inline_g uint8x8_t
vext_u8(uint8x8_t a, uint8x8_t b, const int n) noexcept
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

__inline_g int8x8_t
vext_s8(int8x8_t a, int8x8_t b, const int n) noexcept
{
  return (int8x8_t)vext_u8((uint8x8_t)a, (uint8x8_t)b, n);
}

__inline_g uint16x4_t
vext_u16(uint16x4_t a, uint16x4_t b, const int n) noexcept
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

__inline_g int16x4_t
vext_s16(int16x4_t a, int16x4_t b, const int n) noexcept
{
  return (int16x4_t)vext_u16((uint16x4_t)a, (uint16x4_t)b, n);
}

__inline_g uint32x2_t
vext_u32(uint32x2_t a, uint32x2_t b, const int n) noexcept
{
  return (n & 1) ? __builtin_shufflevector(a, b, 1, 2) : a;
}

__inline_g int32x2_t
vext_s32(int32x2_t a, int32x2_t b, const int n) noexcept
{
  return (int32x2_t)vext_u32((uint32x2_t)a, (uint32x2_t)b, n);
}

__inline_g float32x2_t
vext_f32(float32x2_t a, float32x2_t b, const int n) noexcept
{
  return (n & 1) ? __builtin_shufflevector(a, b, 1, 2) : a;
}

#define __mc_qbinop_d(NAME, ASM_OP, SUF, T)                                                                                                \
  __inline_g T NAME##_##SUF(T a, T b) noexcept                                                                                             \
  {                                                                                                                                        \
    T r;                                                                                                                                   \
    __asm__(ASM_OP " %P0, %P1, %P2" : "=w"(r) : "w"(a), "w"(b));                                                                           \
    return r;                                                                                                                              \
  }

__mc_qbinop_d(vqadd, "vqadd.s8", s8, int8x8_t);
__mc_qbinop_d(vqadd, "vqadd.s16", s16, int16x4_t);
__mc_qbinop_d(vqadd, "vqadd.s32", s32, int32x2_t);
__mc_qbinop_d(vqadd, "vqadd.s64", s64, int64x1_t);
__mc_qbinop_d(vqadd, "vqadd.u8", u8, uint8x8_t);
__mc_qbinop_d(vqadd, "vqadd.u16", u16, uint16x4_t);
__mc_qbinop_d(vqadd, "vqadd.u32", u32, uint32x2_t);
__mc_qbinop_d(vqadd, "vqadd.u64", u64, uint64x1_t);

__mc_qbinop_d(vqsub, "vqsub.s8", s8, int8x8_t);
__mc_qbinop_d(vqsub, "vqsub.s16", s16, int16x4_t);
__mc_qbinop_d(vqsub, "vqsub.s32", s32, int32x2_t);
__mc_qbinop_d(vqsub, "vqsub.s64", s64, int64x1_t);
__mc_qbinop_d(vqsub, "vqsub.u8", u8, uint8x8_t);
__mc_qbinop_d(vqsub, "vqsub.u16", u16, uint16x4_t);
__mc_qbinop_d(vqsub, "vqsub.u32", u32, uint32x2_t);
__mc_qbinop_d(vqsub, "vqsub.u64", u64, uint64x1_t);

#undef __mc_qbinop_d

__inline_g int8x8_t
vdup_n_s8(signed char v) noexcept
{
  int8x8_t r = { v, v, v, v, v, v, v, v };
  return r;
}

__inline_g int16x4_t
vdup_n_s16(signed short v) noexcept
{
  int16x4_t r = { v, v, v, v };
  return r;
}

__inline_g int32x2_t
vdup_n_s32(signed int v) noexcept
{
  int32x2_t r = { v, v };
  return r;
}

__inline_g uint8x8_t
vdup_n_u8(unsigned char v) noexcept
{
  uint8x8_t r = { v, v, v, v, v, v, v, v };
  return r;
}

__inline_g uint16x4_t
vdup_n_u16(unsigned short v) noexcept
{
  uint16x4_t r = { v, v, v, v };
  return r;
}

__inline_g uint32x2_t
vdup_n_u32(unsigned int v) noexcept
{
  uint32x2_t r = { v, v };
  return r;
}

__inline_g uint64x1_t
vdup_n_u64(unsigned long long v) noexcept
{
  uint64x1_t r = { v };
  return r;
}

__inline_g float32x2_t
vdup_n_f32(float v) noexcept
{
  float32x2_t r = { v, v };
  return r;
}

__inline_g float32x2_t
vdup_lane_f32(float32x2_t v, const int lane) noexcept
{
  return vdup_n_f32(v[lane]);
}

__inline_g i8
vget_lane_s8(int8x8_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u8
vget_lane_u8(uint8x8_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g i16
vget_lane_s16(int16x4_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g i32
vget_lane_s32(int32x2_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u32
vget_lane_u32(uint32x2_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g f32
vget_lane_f32(float32x2_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g i64
vget_lane_s64(int64x1_t v, const int lane) noexcept
{
  return v[lane];
}

__inline_g u64
vget_lane_u64(uint64x1_t v, const int lane) noexcept
{
  return v[lane];
}

#define __mc_vset_lane_d(SUF, ETYPE, T)                                                                                                    \
  __inline_g T vset_lane_##SUF(ETYPE x, T v, const int lane) noexcept                                                                      \
  {                                                                                                                                        \
    v[lane] = x;                                                                                                                           \
    return v;                                                                                                                              \
  }

__mc_vset_lane_d(s8, i8, int8x8_t);
__mc_vset_lane_d(s16, i16, int16x4_t);
__mc_vset_lane_d(s32, i32, int32x2_t);
__mc_vset_lane_d(s64, i64, int64x1_t);
__mc_vset_lane_d(u8, u8, uint8x8_t);
__mc_vset_lane_d(u16, u16, uint16x4_t);
__mc_vset_lane_d(u32, u32, uint32x2_t);
__mc_vset_lane_d(u64, u64, uint64x1_t);
__mc_vset_lane_d(f32, f32, float32x2_t);

#undef __mc_vset_lane_d

__inline_g uint8x16_t
vqtbl1q_u8(uint8x16_t table, uint8x16_t idx) noexcept
{
  uint8x8_t tlo = vget_low_u8(table);
  uint8x8_t thi = vget_high_u8(table);
  uint8x16_t idx8 = vsubq_u8(idx, vdupq_n_u8(8));
  uint8x8_t rlo = vtbx1_u8(vtbl1_u8(tlo, vget_low_u8(idx)), thi, vget_low_u8(idx8));
  uint8x8_t rhi = vtbx1_u8(vtbl1_u8(tlo, vget_high_u8(idx)), thi, vget_high_u8(idx8));
  return vcombine_u8(rlo, rhi);
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron
