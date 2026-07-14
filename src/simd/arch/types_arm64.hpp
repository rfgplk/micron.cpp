//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../concepts.hpp"

#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../intrin.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// aarch64 types
// dispatched via ns

namespace micron
{
namespace simd
{

using __vf = float;
using __vd = double;
using __v8 = i8;
using __v16 = i16;
using __v32 = i32;
using __v64 = i64;
using __uv8 = u8;
using __uv16 = u16;
using __uv32 = u32;
using __uv64 = u64;

using b64 = uint8x8_t;

using f128 = float32x4_t;
using d128 = float64x2_t;
using i128 = int32x4_t;

using f256 = float32x4x2_t;
using d256 = float64x2x2_t;
using i256 = int32x4x2_t;

template<typename T>
concept is_simd_type = micron::same_as<T, b64> || micron::same_as<T, f128> || micron::same_as<T, d128> || micron::same_as<T, i128>
                       || micron::same_as<T, f256> || micron::same_as<T, d256> || micron::same_as<T, i256>;

template<typename T>
concept is_simd_128_type = micron::same_as<T, f128> || micron::same_as<T, d128> || micron::same_as<T, i128>;

template<typename T>
concept is_simd_256_type = micron::same_as<T, f256> || micron::same_as<T, d256> || micron::same_as<T, i256>;

template<typename T>
concept is_int_flag_type
    = micron::same_as<T, __uv8> || micron::same_as<T, __uv16> || micron::same_as<T, __uv32> || micron::same_as<T, __uv64>
      || micron::same_as<T, __v8> || micron::same_as<T, __v16> || micron::same_as<T, __v32> || micron::same_as<T, __v64>;

template<typename T>
concept is_flag_type = micron::same_as<T, __vd> || micron::same_as<T, __vf> || micron::same_as<T, __v8> || micron::same_as<T, __v16>
                       || micron::same_as<T, __v32> || micron::same_as<T, __v64>;

template<typename F>
constexpr bool
__is_64_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v64> || micron::is_same_v<F, __uv64> ) return true;
  return false;
}

template<typename F>
constexpr bool
__is_32_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v32> || micron::is_same_v<F, __uv32> ) return true;
  return false;
}

template<typename F>
constexpr bool
__is_16_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v16> || micron::is_same_v<F, __uv16> ) return true;
  return false;
}

template<typename F>
constexpr bool
__is_8_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v8> || micron::is_same_v<F, __uv8> ) return true;
  return false;
}

inline int
neon_movemask_f32(float32x4_t v) noexcept
{
  const uint32x4_t m = vshrq_n_u32(vreinterpretq_u32_f32(v), 31);
  return static_cast<int>(vgetq_lane_u32(m, 0) | (vgetq_lane_u32(m, 1) << 1) | (vgetq_lane_u32(m, 2) << 2) | (vgetq_lane_u32(m, 3) << 3));
}

inline int
neon_movemask_f64(float64x2_t v) noexcept
{
  const uint64x2_t m = vshrq_n_u64(vreinterpretq_u64_f64(v), 63);
  return static_cast<int>(static_cast<unsigned>(vgetq_lane_u64(m, 0)) | (static_cast<unsigned>(vgetq_lane_u64(m, 1)) << 1));
}

inline int
neon_movemask_si128(int32x4_t v) noexcept
{
  const uint32x4_t m = vshrq_n_u32(vreinterpretq_u32_s32(v), 31);
  return static_cast<int>(vgetq_lane_u32(m, 0) | (vgetq_lane_u32(m, 1) << 1) | (vgetq_lane_u32(m, 2) << 2) | (vgetq_lane_u32(m, 3) << 3));
}

inline int
neon_movemask_f32x2(float32x4x2_t v) noexcept
{
  return neon_movemask_f32(v.val[0]) | (neon_movemask_f32(v.val[1]) << 4);
}

inline int
neon_movemask_f64x2(float64x2x2_t v) noexcept
{
  return neon_movemask_f64(v.val[0]) | (neon_movemask_f64(v.val[1]) << 2);
}

inline int
neon_movemask_si256(int32x4x2_t v) noexcept
{
  return neon_movemask_si128(v.val[0]) | (neon_movemask_si128(v.val[1]) << 4);
}

inline int32x4_t
neon_all_ones_si128(void) noexcept
{
  return vdupq_n_s32(-1);
}

inline int32x4x2_t
neon_all_ones_si256(void) noexcept
{
  int32x4x2_t r;
  r.val[0] = vdupq_n_s32(-1);
  r.val[1] = vdupq_n_s32(-1);
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lane-mask expansion
//
// moved here (from math/shifts), used to be duplicated, causing TU compile include issues

__attribute__((always_inline)) static inline uint16x8_t
__expand_mask_u16(uint8_t k) noexcept
{
  static const uint16_t bp[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  uint16x8_t m = vdupq_n_u16(k), p = vld1q_u16(bp);
  return vceqq_u16(vandq_u16(m, p), p);
}

__attribute__((always_inline)) static inline uint8x16_t
__expand_mask_u8(uint16_t k) noexcept
{
  static const uint8_t bp[16] = { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };

  uint8x16_t sel = vld1q_u8(bp);
  uint8x8_t lo = vdup_n_u8(static_cast<uint8_t>(k & 0xFFu));
  uint8x8_t hi = vdup_n_u8(static_cast<uint8_t>((k >> 8) & 0xFFu));
  uint8x16_t m = vcombine_u8(lo, hi);
  return vceqq_u8(vandq_u8(m, sel), sel);
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

};      // namespace simd
};      // namespace micron
