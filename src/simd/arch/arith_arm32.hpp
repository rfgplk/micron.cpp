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

template <typename T>
concept is_neon_simd_class = requires {
  typename T::bit_width;
  typename T::lane_width;
};

template <is_neon_simd_class T>
[[gnu::always_inline]] static inline typename T::bit_width
__r(T &v) noexcept
{
  return static_cast<typename T::bit_width>(v);
}

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
#define _ri32r(x) (x)
#define _ri64r(x) vreinterpretq_s32_s64(x)
#define _ru8r(x) vreinterpretq_s32_u8(x)
#define _ru16r(x) vreinterpretq_s32_u16(x)
#define _ru32r(x) vreinterpretq_s32_u32(x)
#define _ru64r(x) vreinterpretq_s32_u64(x)

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise

template <is_neon_simd_class T>
inline T
avx_and(T &o, T &b)
{
  return vandq_s32(__r(o), __r(b));
}

template <is_neon_simd_class T>
inline T
avx_andnot(T &o, T &b)
{
  return vbicq_s32(__r(b), __r(o));
}

template <is_neon_simd_class T>
inline T
avx_or(T &o, T &b)
{
  return vorrq_s32(__r(o), __r(b));
}

template <is_neon_simd_class T>
inline T
avx_xor(T &o, T &b)
{
  return veorq_s32(__r(o), __r(b));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shifts

template <is_neon_simd_class T>
inline T
shiftleft_16(T &o, int b)
{
  return _ri16r(vshlq_s16(_ri16(__r(o)), vdupq_n_s16(static_cast<int16_t>(b))));
}

template <is_neon_simd_class T>
inline T
shiftleft_32(T &o, int b)
{
  return vshlq_s32(__r(o), vdupq_n_s32(b));
}

template <is_neon_simd_class T>
inline T
shiftleft_64(T &o, int b)
{
  return _ri64r(vshlq_s64(_ri64(__r(o)), vdupq_n_s64(static_cast<int64_t>(b))));
}

template <is_neon_simd_class T>
inline T
shift_left(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return shiftleft_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return shiftleft_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return shiftleft_64(o, b);
}

template <is_neon_simd_class T>
inline T
shiftright_logical_16(T &o, int b)
{
  return _ru16r(vshlq_u16(_ru16(__r(o)), vdupq_n_s16(static_cast<int16_t>(-b))));
}

template <is_neon_simd_class T>
inline T
shiftright_logical_32(T &o, int b)
{
  return _ru32r(vshlq_u32(_ru32(__r(o)), vdupq_n_s32(-b)));
}

template <is_neon_simd_class T>
inline T
shiftright_logical_64(T &o, int b)
{
  return _ru64r(vshlq_u64(_ru64(__r(o)), vdupq_n_s64(static_cast<int64_t>(-b))));
}

template <is_neon_simd_class T>
inline T
shift_right_logical(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return shiftright_logical_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return shiftright_logical_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return shiftright_logical_64(o, b);
}

template <is_neon_simd_class T>
inline T
shiftright_arithmetic_16(T &o, int b)
{
  return _ri16r(vshlq_s16(_ri16(__r(o)), vdupq_n_s16(static_cast<int16_t>(-b))));
}

template <is_neon_simd_class T>
inline T
shiftright_arithmetic_32(T &o, int b)
{
  return vshlq_s32(__r(o), vdupq_n_s32(-b));
}

template <is_neon_simd_class T>
inline T
shift_right_arithmetic(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return shiftright_arithmetic_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return shiftright_arithmetic_32(o, b);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// addition

template <is_neon_simd_class T>
inline T
add_8(T &o, T &b)
{
  return _ri8r(vaddq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
add_16(T &o, T &b)
{
  return _ri16r(vaddq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
add_32(T &o, T &b)
{
  return vaddq_s32(__r(o), __r(b));
}

template <is_neon_simd_class T>
inline T
add_64(T &o, T &b)
{
  return _ri64r(vaddq_s64(_ri64(__r(o)), _ri64(__r(b))));
}

template <is_neon_simd_class T>
inline T
add(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return add_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return add_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return add_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return add_64(o, b);
}

template <is_neon_simd_class T>
inline T
adds_8(T &o, T &b)
{
  return _ri8r(vqaddq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
adds_16(T &o, T &b)
{
  return _ri16r(vqaddq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
adds(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return adds_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return adds_16(o, b);
}

template <is_neon_simd_class T>
inline T
adds_u8(T &o, T &b)
{
  return _ru8r(vqaddq_u8(_ru8(__r(o)), _ru8(__r(b))));
}

template <is_neon_simd_class T>
inline T
adds_u16(T &o, T &b)
{
  return _ru16r(vqaddq_u16(_ru16(__r(o)), _ru16(__r(b))));
}

template <is_neon_simd_class T>
inline T
adds_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return adds_u8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return adds_u16(o, b);
}

template <is_neon_simd_class T>
inline T
sub_8(T &o, T &b)
{
  return _ri8r(vsubq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
sub_16(T &o, T &b)
{
  return _ri16r(vsubq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
sub_32(T &o, T &b)
{
  return vsubq_s32(__r(o), __r(b));
}

template <is_neon_simd_class T>
inline T
sub_64(T &o, T &b)
{
  return _ri64r(vsubq_s64(_ri64(__r(o)), _ri64(__r(b))));
}

template <is_neon_simd_class T>
inline T
sub(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return sub_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return sub_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return sub_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return sub_64(o, b);
}

template <is_neon_simd_class T>
inline T
subs_8(T &o, T &b)
{
  return _ri8r(vqsubq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
subs_16(T &o, T &b)
{
  return _ri16r(vqsubq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
subs(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return subs_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return subs_16(o, b);
}

template <is_neon_simd_class T>
inline T
subs_u8(T &o, T &b)
{
  return _ru8r(vqsubq_u8(_ru8(__r(o)), _ru8(__r(b))));
}

template <is_neon_simd_class T>
inline T
subs_u16(T &o, T &b)
{
  return _ru16r(vqsubq_u16(_ru16(__r(o)), _ru16(__r(b))));
}

template <is_neon_simd_class T>
inline T
subs_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return subs_u8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return subs_u16(o, b);
}

template <is_neon_simd_class T>
inline T
hadd_16(T &o, T &b)
{
  auto uzp = vuzpq_s16(_ri16(__r(o)), _ri16(__r(b)));
  return _ri16r(vaddq_s16(uzp.val[0], uzp.val[1]));
}

template <is_neon_simd_class T>
inline T
hadd_32(T &o, T &b)
{
  auto uzp = vuzpq_s32(__r(o), __r(b));
  return vaddq_s32(uzp.val[0], uzp.val[1]);
}

template <is_neon_simd_class T>
inline T
hadd(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return hadd_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return hadd_32(o, b);
}

template <is_neon_simd_class T>
inline T
hadds_16(T &o, T &b)
{
  auto uzp = vuzpq_s16(_ri16(__r(o)), _ri16(__r(b)));
  return _ri16r(vqaddq_s16(uzp.val[0], uzp.val[1]));
}

template <is_neon_simd_class T>
inline T
hsub_16(T &o, T &b)
{
  auto uzp = vuzpq_s16(_ri16(__r(o)), _ri16(__r(b)));
  return _ri16r(vsubq_s16(uzp.val[0], uzp.val[1]));
}

template <is_neon_simd_class T>
inline T
hsub_32(T &o, T &b)
{
  auto uzp = vuzpq_s32(__r(o), __r(b));
  return vsubq_s32(uzp.val[0], uzp.val[1]);
}

template <is_neon_simd_class T>
inline T
hsub(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return hsub_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return hsub_32(o, b);
}

template <is_neon_simd_class T>
inline T
hsubs_16(T &o, T &b)
{
  auto uzp = vuzpq_s16(_ri16(__r(o)), _ri16(__r(b)));
  return _ri16r(vqsubq_s16(uzp.val[0], uzp.val[1]));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// muls

template <is_neon_simd_class T>
inline T
mullo_16(T &o, T &b)
{
  return _ri16r(vmulq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
mullo_32(T &o, T &b)
{
  return vmulq_s32(__r(o), __r(b));
}

template <is_neon_simd_class T>
inline T
mullo_64(T &, T &)
{
  static_assert(!micron::is_same_v<T, T>, "mullo_64: no hardware vmulq_s64 on NEON");
}

template <is_neon_simd_class T>
inline T
mullo(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return mullo_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return mullo_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return mullo_64(o, b);
}

template <is_neon_simd_class T>
inline T
mulhi_16(T &o, T &b)
{
  auto a16 = _ri16(__r(o)), b16 = _ri16(__r(b));
  int16x4_t lo = vshrn_n_s32(vmull_s16(vget_low_s16(a16), vget_low_s16(b16)), 16);
  int16x4_t hi = vshrn_n_s32(vmull_s16(vget_high_s16(a16), vget_high_s16(b16)), 16);
  return _ri16r(vcombine_s16(lo, hi));
}

template <is_neon_simd_class T>
inline T
mulhi(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return mulhi_16(o, b);
}

template <is_neon_simd_class T>
inline T
mulhi_u16(T &o, T &b)
{
  auto a16 = _ru16(__r(o)), b16 = _ru16(__r(b));
  uint16x4_t lo = vshrn_n_u32(vmull_u16(vget_low_u16(a16), vget_low_u16(b16)), 16);
  uint16x4_t hi = vshrn_n_u32(vmull_u16(vget_high_u16(a16), vget_high_u16(b16)), 16);
  return _ru16r(vcombine_u16(lo, hi));
}

template <is_neon_simd_class T>
inline T
mulhi_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return mulhi_u16(o, b);
}

template <is_neon_simd_class T>
inline T
mul_32_64(T &o, T &b)
{
  auto a_uzp = vuzp_s32(vget_low_s32(__r(o)), vget_high_s32(__r(o)));
  auto b_uzp = vuzp_s32(vget_low_s32(__r(b)), vget_high_s32(__r(b)));
  return _ri64r(vmull_s32(a_uzp.val[0], b_uzp.val[0]));
}

template <is_neon_simd_class T>
inline T
mul_u32_64(T &o, T &b)
{
  auto a_uzp = vuzp_u32(vget_low_u32(_ru32(__r(o))), vget_high_u32(_ru32(__r(o))));
  auto b_uzp = vuzp_u32(vget_low_u32(_ru32(__r(b))), vget_high_u32(_ru32(__r(b))));
  return _ru64r(vmull_u32(a_uzp.val[0], b_uzp.val[0]));
}

template <is_neon_simd_class T>
inline T
madd_16(T &o, T &b)
{
  auto a16 = _ri16(__r(o)), b16 = _ri16(__r(b));
  int32x4_t lo = vmull_s16(vget_low_s16(a16), vget_low_s16(b16));
  int32x4_t hi = vmull_s16(vget_high_s16(a16), vget_high_s16(b16));
  int32x2_t lo_r = vpadd_s32(vget_low_s32(lo), vget_high_s32(lo));
  int32x2_t hi_r = vpadd_s32(vget_low_s32(hi), vget_high_s32(hi));
  return vcombine_s32(lo_r, hi_r);
}

template <is_neon_simd_class T>
inline T
madd(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return madd_16(o, b);
}

template <is_neon_simd_class T>
inline T
maddubs_8(T &o, T &b)
{
  uint8x16_t a8u = _ru8(__r(o));
  int8x16_t b8s = _ri8(__r(b));
  int16x8_t a_lo = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(a8u)));
  int16x8_t a_hi = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(a8u)));
  int16x8_t b_lo = vmovl_s8(vget_low_s8(b8s));
  int16x8_t b_hi = vmovl_s8(vget_high_s8(b8s));
  int16x8_t prod_lo = vmulq_s16(a_lo, b_lo);
  int16x8_t prod_hi = vmulq_s16(a_hi, b_hi);
  int16x4x2_t lo_uzp = vuzp_s16(vget_low_s16(prod_lo), vget_high_s16(prod_lo));
  int16x4x2_t hi_uzp = vuzp_s16(vget_low_s16(prod_hi), vget_high_s16(prod_hi));
  return _ri16r(vcombine_s16(vqadd_s16(lo_uzp.val[0], lo_uzp.val[1]), vqadd_s16(hi_uzp.val[0], hi_uzp.val[1])));
}

template <is_neon_simd_class T>
inline T
maddubs(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return maddubs_8(o, b);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// avgs

template <is_neon_simd_class T>
inline T
avg_u8(T &o, T &b)
{
  return _ru8r(vrhaddq_u8(_ru8(__r(o)), _ru8(__r(b))));
}

template <is_neon_simd_class T>
inline T
avg_u16(T &o, T &b)
{
  return _ru16r(vrhaddq_u16(_ru16(__r(o)), _ru16(__r(b))));
}

template <is_neon_simd_class T>
inline T
avg_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return avg_u8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return avg_u16(o, b);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// signs

template <is_neon_simd_class T>
inline T
sign_8(T &o, T &b)
{
  int8x16_t a = _ri8(__r(o)), bv = _ri8(__r(b));
  uint8x16_t b_pos = vreinterpretq_u8_s8(vcgtq_s8(bv, vdupq_n_s8(0)));
  uint8x16_t b_neg = vreinterpretq_u8_s8(vcltq_s8(bv, vdupq_n_s8(0)));
  return _ri8r(vorrq_s8(vandq_s8(a, vreinterpretq_s8_u8(b_pos)), vandq_s8(vnegq_s8(a), vreinterpretq_s8_u8(b_neg))));
}

template <is_neon_simd_class T>
inline T
sign_16(T &o, T &b)
{
  int16x8_t a = _ri16(__r(o)), bv = _ri16(__r(b));
  uint16x8_t b_pos = vreinterpretq_u16_s16(vcgtq_s16(bv, vdupq_n_s16(0)));
  uint16x8_t b_neg = vreinterpretq_u16_s16(vcltq_s16(bv, vdupq_n_s16(0)));
  return _ri16r(vorrq_s16(vandq_s16(a, vreinterpretq_s16_u16(b_pos)), vandq_s16(vnegq_s16(a), vreinterpretq_s16_u16(b_neg))));
}

template <is_neon_simd_class T>
inline T
sign_32(T &o, T &b)
{
  int32x4_t a = __r(o), bv = __r(b);
  uint32x4_t b_pos = vreinterpretq_u32_s32(vcgtq_s32(bv, vdupq_n_s32(0)));
  uint32x4_t b_neg = vreinterpretq_u32_s32(vcltq_s32(bv, vdupq_n_s32(0)));
  return vorrq_s32(vandq_s32(a, vreinterpretq_s32_u32(b_pos)), vandq_s32(vnegq_s32(a), vreinterpretq_s32_u32(b_neg)));
}

template <is_neon_simd_class T>
inline T
sign(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return sign_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return sign_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return sign_32(o, b);
}

template <is_neon_simd_class T>
inline T
min_8(T &o, T &b)
{
  return _ri8r(vminq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
min_16(T &o, T &b)
{
  return _ri16r(vminq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
min_32(T &o, T &b)
{
  return vminq_s32(__r(o), __r(b));
}

template <is_neon_simd_class T>
inline T
min(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return min_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return min_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return min_32(o, b);
}

template <is_neon_simd_class T>
inline T
min_u8(T &o, T &b)
{
  return _ru8r(vminq_u8(_ru8(__r(o)), _ru8(__r(b))));
}

template <is_neon_simd_class T>
inline T
min_u16(T &o, T &b)
{
  return _ru16r(vminq_u16(_ru16(__r(o)), _ru16(__r(b))));
}

template <is_neon_simd_class T>
inline T
min_u32(T &o, T &b)
{
  return _ru32r(vminq_u32(_ru32(__r(o)), _ru32(__r(b))));
}

template <is_neon_simd_class T>
inline T
min_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return min_u8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return min_u16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return min_u32(o, b);
}

template <is_neon_simd_class T>
inline T
max_8(T &o, T &b)
{
  return _ri8r(vmaxq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
max_16(T &o, T &b)
{
  return _ri16r(vmaxq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
max_32(T &o, T &b)
{
  return vmaxq_s32(__r(o), __r(b));
}

template <is_neon_simd_class T>
inline T
max(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return max_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return max_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return max_32(o, b);
}

template <is_neon_simd_class T>
inline T
max_u8(T &o, T &b)
{
  return _ru8r(vmaxq_u8(_ru8(__r(o)), _ru8(__r(b))));
}

template <is_neon_simd_class T>
inline T
max_u16(T &o, T &b)
{
  return _ru16r(vmaxq_u16(_ru16(__r(o)), _ru16(__r(b))));
}

template <is_neon_simd_class T>
inline T
max_u32(T &o, T &b)
{
  return _ru32r(vmaxq_u32(_ru32(__r(o)), _ru32(__r(b))));
}

template <is_neon_simd_class T>
inline T
max_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return max_u8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return max_u16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return max_u32(o, b);
}

template <is_neon_simd_class T>
inline T
abs_8(T &o)
{
  return _ri8r(vabsq_s8(_ri8(__r(o))));
}

template <is_neon_simd_class T>
inline T
abs_16(T &o)
{
  return _ri16r(vabsq_s16(_ri16(__r(o))));
}

template <is_neon_simd_class T>
inline T
abs_32(T &o)
{
  return vabsq_s32(__r(o));
}

template <is_neon_simd_class T>
inline T
abs(T &o)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return abs_8(o);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return abs_16(o);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return abs_32(o);
}

template <is_neon_simd_class T>
inline T
sad_u8(T &o, T &b)
{
  uint8x16_t diff = vabdq_u8(_ru8(__r(o)), _ru8(__r(b)));
  return _ru64r(vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(diff))));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cmps

template <is_neon_simd_class T>
inline T
cmpeq_8(T &o, T &b)
{
  return _ru8r(vceqq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
cmpeq_16(T &o, T &b)
{
  return _ru16r(vceqq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
cmpeq_32(T &o, T &b)
{
  return _ru32r(vceqq_s32(__r(o), __r(b)));
}

template <is_neon_simd_class T>
inline T
cmpeq_64(T &o, T &b)
{

  return _ru32r(neon_v7_ceqq_s64(_ri64(__r(o)), _ri64(__r(b))));
}

template <is_neon_simd_class T>
inline T
cmpeq(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return cmpeq_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return cmpeq_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return cmpeq_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return cmpeq_64(o, b);
}

template <is_neon_simd_class T>
inline T
cmpgt_8(T &o, T &b)
{
  return _ru8r(vcgtq_s8(_ri8(__r(o)), _ri8(__r(b))));
}

template <is_neon_simd_class T>
inline T
cmpgt_16(T &o, T &b)
{
  return _ru16r(vcgtq_s16(_ri16(__r(o)), _ri16(__r(b))));
}

template <is_neon_simd_class T>
inline T
cmpgt_32(T &o, T &b)
{
  return _ru32r(vcgtq_s32(__r(o), __r(b)));
}

template <is_neon_simd_class T>
inline T
cmpgt_64(T &o, T &b)
{

  return _ru32r(neon_v7_cgtq_s64(_ri64(__r(o)), _ri64(__r(b))));
}

template <is_neon_simd_class T>
inline T
cmpgt(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return cmpgt_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return cmpgt_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return cmpgt_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return cmpgt_64(o, b);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// packs

template <is_neon_simd_class T>
inline T
packs_16(T &o, T &b)
{
  return _ri8r(vcombine_s8(vqmovn_s16(_ri16(__r(o))), vqmovn_s16(_ri16(__r(b)))));
}

template <is_neon_simd_class T>
inline T
packs_32(T &o, T &b)
{
  return _ri16r(vcombine_s16(vqmovn_s32(__r(o)), vqmovn_s32(__r(b))));
}

template <is_neon_simd_class T>
inline T
packs(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return packs_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return packs_32(o, b);
}

template <is_neon_simd_class T>
inline T
packus_16(T &o, T &b)
{
  return _ru8r(vcombine_u8(vqmovun_s16(_ri16(__r(o))), vqmovun_s16(_ri16(__r(b)))));
}

template <is_neon_simd_class T>
inline T
packus_32(T &o, T &b)
{
  return _ru16r(vcombine_u16(vqmovun_s32(__r(o)), vqmovun_s32(__r(b))));
}

template <is_neon_simd_class T>
inline T
packus(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return packus_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return packus_32(o, b);
}

template <is_neon_simd_class T>
inline T
unpacklo_8(T &o, T &b)
{
  return _ri8r(vzipq_s8(_ri8(__r(o)), _ri8(__r(b))).val[0]);
}

template <is_neon_simd_class T>
inline T
unpacklo_16(T &o, T &b)
{
  return _ri16r(vzipq_s16(_ri16(__r(o)), _ri16(__r(b))).val[0]);
}

template <is_neon_simd_class T>
inline T
unpacklo_32(T &o, T &b)
{
  return vzipq_s32(__r(o), __r(b)).val[0];
}

template <is_neon_simd_class T>
inline T
unpacklo_64(T &o, T &b)
{
  return _ri64r(vcombine_s64(vget_low_s64(_ri64(__r(o))), vget_low_s64(_ri64(__r(b)))));
}

template <is_neon_simd_class T>
inline T
unpacklo(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return unpacklo_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return unpacklo_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return unpacklo_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return unpacklo_64(o, b);
}

template <is_neon_simd_class T>
inline T
unpackhi_8(T &o, T &b)
{
  return _ri8r(vzipq_s8(_ri8(__r(o)), _ri8(__r(b))).val[1]);
}

template <is_neon_simd_class T>
inline T
unpackhi_16(T &o, T &b)
{
  return _ri16r(vzipq_s16(_ri16(__r(o)), _ri16(__r(b))).val[1]);
}

template <is_neon_simd_class T>
inline T
unpackhi_32(T &o, T &b)
{
  return vzipq_s32(__r(o), __r(b)).val[1];
}

template <is_neon_simd_class T>
inline T
unpackhi_64(T &o, T &b)
{
  return _ri64r(vcombine_s64(vget_high_s64(_ri64(__r(o))), vget_high_s64(_ri64(__r(b)))));
}

template <is_neon_simd_class T>
inline T
unpackhi(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> )
    return unpackhi_8(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return unpackhi_16(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return unpackhi_32(o, b);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return unpackhi_64(o, b);
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
#undef _ri32r
#undef _ri64r
#undef _ru8r
#undef _ru16r
#undef _ru32r
#undef _ru64r

};     // namespace simd
};     // namespace micron

#pragma GCC diagnostic pop
