//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../concepts.hpp"

#include "../type_traits.hpp"
#include "../types.hpp"
#include "intrin.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// aarch32 types
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

using f128 = float32x4_t;     // 4 × f32
using i128 = int32x4_t;       // 4 × s32 (canonical integer view)

using f256 = float32x4x2_t;     // 8 × f32 (.val[0]=lo, .val[1]=hi)
using i256 = int32x4x2_t;       // 8 × s32 view

template <typename T>
concept is_simd_128_type = micron::same_as<T, f128> || micron::same_as<T, i128>;

template <typename T>
concept is_simd_256_type = micron::same_as<T, f256> || micron::same_as<T, i256>;

template <typename T>
concept is_int_flag_type
    = micron::same_as<T, __uv8> || micron::same_as<T, __uv16> || micron::same_as<T, __uv32> || micron::same_as<T, __uv64>
      || micron::same_as<T, __v8> || micron::same_as<T, __v16> || micron::same_as<T, __v32> || micron::same_as<T, __v64>;

template <typename T>
concept is_flag_type = micron::same_as<T, __vf> || micron::same_as<T, __v8> || micron::same_as<T, __v16> || micron::same_as<T, __v32>
                       || micron::same_as<T, __v64>;

template <typename F>
constexpr bool
__is_64_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v64> || micron::is_same_v<F, __uv64> )
    return true;
  return false;
}

template <typename F>
constexpr bool
__is_32_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v32> || micron::is_same_v<F, __uv32> )
    return true;
  return false;
}

template <typename F>
constexpr bool
__is_16_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v16> || micron::is_same_v<F, __uv16> )
    return true;
  return false;
}

template <typename F>
constexpr bool
__is_8_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v8> || micron::is_same_v<F, __uv8> )
    return true;
  return false;
}

inline int
neon_movemask_f32(float32x4_t v) noexcept
{
  const uint32x4_t m = vshrq_n_u32(vreinterpretq_u32_f32(v), 31);
  return static_cast<int>(vgetq_lane_u32(m, 0) | (vgetq_lane_u32(m, 1) << 1) | (vgetq_lane_u32(m, 2) << 2) | (vgetq_lane_u32(m, 3) << 3));
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

inline float32x4_t
neon_v7_divq_f32(float32x4_t a, float32x4_t b) noexcept
{
  float32x4_t recip = vrecpeq_f32(b);
  recip = vmulq_f32(vrecpsq_f32(b, recip), recip);
  recip = vmulq_f32(vrecpsq_f32(b, recip), recip);
  return vmulq_f32(a, recip);
}

inline uint64x2_t
neon_v7_ceqq_s64(int64x2_t a, int64x2_t b) noexcept
{
  uint32x4_t cmp = vceqq_u32(vreinterpretq_u32_s64(a), vreinterpretq_u32_s64(b));

  return vreinterpretq_u64_u32(vandq_u32(cmp, vrev64q_u32(cmp)));
}

inline uint64x2_t
neon_v7_cgtq_s64(int64x2_t a, int64x2_t b) noexcept
{

  uint32x4_t hi_gt = vcgtq_s32(vreinterpretq_s32_s64(a), vreinterpretq_s32_s64(b));
  uint32x4_t hi_eq = vceqq_s32(vreinterpretq_s32_s64(a), vreinterpretq_s32_s64(b));
  uint32x4_t lo_gt = vcgtq_u32(vreinterpretq_u32_s64(a), vreinterpretq_u32_s64(b));

  uint32x4_t hi_gt_r = vrev64q_u32(hi_gt);
  uint32x4_t hi_eq_r = vrev64q_u32(hi_eq);

  uint32x4_t r = vorrq_u32(hi_gt_r, vandq_u32(hi_eq_r, lo_gt));

  uint32x4x2_t z = vzipq_u32(r, r);
  return vreinterpretq_u64_u32(vcombine_u32(vget_low_u32(z.val[0]), vget_low_u32(z.val[1])));
}

inline uint64x2_t
neon_v7_cgeq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vreinterpretq_u64_u32(veorq_u32(vreinterpretq_u32_u64(neon_v7_cgtq_s64(b, a)), vdupq_n_u32(0xFFFFFFFFu)));
}

inline uint64x2_t
neon_v7_cltq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return neon_v7_cgtq_s64(b, a);
}

inline uint64x2_t
neon_v7_cleq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vreinterpretq_u64_u32(veorq_u32(vreinterpretq_u32_u64(neon_v7_cgtq_s64(a, b)), vdupq_n_u32(0xFFFFFFFFu)));
}

inline uint64x2_t
neon_v7_cneq_s64(int64x2_t a, int64x2_t b) noexcept
{
  return vreinterpretq_u64_u32(veorq_u32(vreinterpretq_u32_u64(neon_v7_ceqq_s64(a, b)), vdupq_n_u32(0xFFFFFFFFu)));
}

};     // namespace simd
};     // namespace micron
