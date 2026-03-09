//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

//  _mm_bslli_si128(a, n)      : vextq_u8(zero, a, 16-n)
//  _mm_bsrli_si128(a, n)      : vextq_u8(a, zero, n)

//  _mm_sll_epi*(a, cnt)       : vshlq_s*(a, vdupq_n_s*(scalar(cnt)))
//  _mm_srl_epi*(a, cnt)       : vshlq_u*(a, vdupq_n_s*(-scalar(cnt)))
//  _mm_sra_epi*(a, cnt)       : vshlq_s*(a, vdupq_n_s*(-scalar(cnt)))

//  _mm_slli_epi*(a, n)        : vshlq_s*(a, vdupq_n_s*(+n))
//  _mm_srli_epi*(a, n)        : vshlq_u*(a, vdupq_n_s*(-n))
//  _mm_srai_epi*(a, n)        : vshlq_s*(a, vdupq_n_s*(-n))

//  _mm_sllv_epi*(a, cnt)      : vshlq_s*(a, cnt_s)
//  _mm_srlv_epi*(a, cnt)      : vshlq_u*(a, vnegq_s*(cnt_s))
//  _mm_srav_epi*(a, cnt)      : vshlq_s*(a, vnegq_s*(cnt_s))

//  _mm_rol_epi32(a, n)        : vshlq_u32 left | vshlq_u32 right (n-32)
//  _mm_rolv_epi32(a, cnt)     : vshlq_u32 left | vshlq_u32 (cnt - 32)

//  shldi_N(a, b, n)           = (a << n) | (b >> (N-n))  per lane
//  shrdi_N(a, b, n)           = (a >> n) | (b << (N-n))  per lane

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

[[gnu::always_inline]] static inline int
__scalar_cnt(int32x4_t cnt) noexcept
{
  return static_cast<int>(vgetq_lane_s64(vreinterpretq_s64_s32(cnt), 0));
}

[[gnu::always_inline]] static inline uint16x8_t
__expand_mask_u16(uint8_t k) noexcept
{
  static const uint16_t bp[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  uint16x8_t m = vdupq_n_u16(k);
  uint16x8_t p = vld1q_u16(bp);
  return vceqq_u16(vandq_u16(m, p), p);
}

[[gnu::always_inline]] static inline uint32x4_t
__expand_mask_u32(uint8_t k) noexcept
{
  static const uint32_t bp[4] = { 1, 2, 4, 8 };
  uint32x4_t m = vdupq_n_u32(k);
  uint32x4_t p = vld1q_u32(bp);
  return vceqq_u32(vandq_u32(m, p), p);
}

[[gnu::always_inline]] static inline uint64x2_t
__expand_mask_u64(uint8_t k) noexcept
{
  static const uint64_t bp[2] = { 1, 2 };
  uint64x2_t m = vdupq_n_u64(k);
  uint64x2_t p = vld1q_u64(bp);
  return vceqq_u64(vandq_u64(m, p), p);
}

[[gnu::always_inline]] static inline int32x4_t
__neon_bslli(int32x4_t v, int n) noexcept
{
  if ( n <= 0 )
    return v;
  if ( n >= 16 )
    return vdupq_n_s32(0);
  uint8x16_t a = _ru8(v), z = vdupq_n_u8(0);
  switch ( n ) {
  case 1 :
    return _ru8r(vextq_u8(z, a, 15));
  case 2 :
    return _ru8r(vextq_u8(z, a, 14));
  case 3 :
    return _ru8r(vextq_u8(z, a, 13));
  case 4 :
    return _ru8r(vextq_u8(z, a, 12));
  case 5 :
    return _ru8r(vextq_u8(z, a, 11));
  case 6 :
    return _ru8r(vextq_u8(z, a, 10));
  case 7 :
    return _ru8r(vextq_u8(z, a, 9));
  case 8 :
    return _ru8r(vextq_u8(z, a, 8));
  case 9 :
    return _ru8r(vextq_u8(z, a, 7));
  case 10 :
    return _ru8r(vextq_u8(z, a, 6));
  case 11 :
    return _ru8r(vextq_u8(z, a, 5));
  case 12 :
    return _ru8r(vextq_u8(z, a, 4));
  case 13 :
    return _ru8r(vextq_u8(z, a, 3));
  case 14 :
    return _ru8r(vextq_u8(z, a, 2));
  case 15 :
    return _ru8r(vextq_u8(z, a, 1));
  default :
    return vdupq_n_s32(0);
  }
}

[[gnu::always_inline]] static inline int32x4_t
__neon_bsrli(int32x4_t v, int n) noexcept
{
  if ( n <= 0 )
    return v;
  if ( n >= 16 )
    return vdupq_n_s32(0);
  uint8x16_t a = _ru8(v), z = vdupq_n_u8(0);
  switch ( n ) {
  case 1 :
    return _ru8r(vextq_u8(a, z, 1));
  case 2 :
    return _ru8r(vextq_u8(a, z, 2));
  case 3 :
    return _ru8r(vextq_u8(a, z, 3));
  case 4 :
    return _ru8r(vextq_u8(a, z, 4));
  case 5 :
    return _ru8r(vextq_u8(a, z, 5));
  case 6 :
    return _ru8r(vextq_u8(a, z, 6));
  case 7 :
    return _ru8r(vextq_u8(a, z, 7));
  case 8 :
    return _ru8r(vextq_u8(a, z, 8));
  case 9 :
    return _ru8r(vextq_u8(a, z, 9));
  case 10 :
    return _ru8r(vextq_u8(a, z, 10));
  case 11 :
    return _ru8r(vextq_u8(a, z, 11));
  case 12 :
    return _ru8r(vextq_u8(a, z, 12));
  case 13 :
    return _ru8r(vextq_u8(a, z, 13));
  case 14 :
    return _ru8r(vextq_u8(a, z, 14));
  case 15 :
    return _ru8r(vextq_u8(a, z, 15));
  default :
    return vdupq_n_s32(0);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// byte shifts

template <is_neon_simd_class T>
inline T
byte_shift_left(T &o, int imm8)
{
  return __neon_bslli(__r(o), imm8);
}

template <is_neon_simd_class T>
inline T
byte_shift_right(T &o, int imm8)
{
  return __neon_bsrli(__r(o), imm8);
}

template <is_neon_simd_class T>
inline T
slli_si(T &o, int imm8)
{
  return byte_shift_left(o, imm8);
}

template <is_neon_simd_class T>
inline T
srli_si(T &o, int imm8)
{
  return byte_shift_right(o, imm8);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sll — scalar count from i128 container

template <is_neon_simd_class T>
inline T
sll_16(T &o, i128 count)
{
  const int c = __scalar_cnt(count);
  return _ri16r(vshlq_s16(_ri16(__r(o)), vdupq_n_s16(static_cast<int16_t>(c))));
}

template <is_neon_simd_class T>
inline T
sll_32(T &o, i128 count)
{
  return vshlq_s32(__r(o), vdupq_n_s32(__scalar_cnt(count)));
}

template <is_neon_simd_class T>
inline T
sll_64(T &o, i128 count)
{
  return _ri64r(vshlq_s64(_ri64(__r(o)), vdupq_n_s64(static_cast<int64_t>(__scalar_cnt(count)))));
}

template <is_neon_simd_class T>
inline T
sll(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return sll_16(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return sll_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return sll_64(o, count);
}

template <is_neon_simd_class T>
inline T
slli_16(T &o, int imm8)
{
  return _ri16r(vshlq_s16(_ri16(__r(o)), vdupq_n_s16(static_cast<int16_t>(imm8))));
}

template <is_neon_simd_class T>
inline T
slli_32(T &o, int imm8)
{
  return vshlq_s32(__r(o), vdupq_n_s32(imm8));
}

template <is_neon_simd_class T>
inline T
slli_64(T &o, int imm8)
{
  return _ri64r(vshlq_s64(_ri64(__r(o)), vdupq_n_s64(static_cast<int64_t>(imm8))));
}

template <is_neon_simd_class T>
inline T
slli(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return slli_16(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return slli_32(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return slli_64(o, imm8);
}

template <is_neon_simd_class T>
inline T
sllv_16(T &o, T &count)
{
  return _ri16r(vshlq_s16(_ri16(__r(o)), _ri16(__r(count))));
}

template <is_neon_simd_class T>
inline T
sllv_32(T &o, T &count)
{
  return vshlq_s32(__r(o), __r(count));
}

template <is_neon_simd_class T>
inline T
sllv_64(T &o, T &count)
{
  return _ri64r(vshlq_s64(_ri64(__r(o)), _ri64(__r(count))));
}

template <is_neon_simd_class T>
inline T
sllv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return sllv_16(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return sllv_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return sllv_64(o, count);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// srl — logical right shift (scalar count)

template <is_neon_simd_class T>
inline T
srl_16(T &o, i128 count)
{
  const int c = -__scalar_cnt(count);
  return _ru16r(vshlq_u16(_ru16(__r(o)), vdupq_n_s16(static_cast<int16_t>(c))));
}

template <is_neon_simd_class T>
inline T
srl_32(T &o, i128 count)
{
  return _ru32r(vshlq_u32(_ru32(__r(o)), vdupq_n_s32(-__scalar_cnt(count))));
}

template <is_neon_simd_class T>
inline T
srl_64(T &o, i128 count)
{
  return _ru64r(vshlq_u64(_ru64(__r(o)), vdupq_n_s64(static_cast<int64_t>(-__scalar_cnt(count)))));
}

template <is_neon_simd_class T>
inline T
srl(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return srl_16(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return srl_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return srl_64(o, count);
}

template <is_neon_simd_class T>
inline T
srli_16(T &o, int imm8)
{
  return _ru16r(vshlq_u16(_ru16(__r(o)), vdupq_n_s16(static_cast<int16_t>(-imm8))));
}

template <is_neon_simd_class T>
inline T
srli_32(T &o, int imm8)
{
  return _ru32r(vshlq_u32(_ru32(__r(o)), vdupq_n_s32(-imm8)));
}

template <is_neon_simd_class T>
inline T
srli_64(T &o, int imm8)
{
  return _ru64r(vshlq_u64(_ru64(__r(o)), vdupq_n_s64(static_cast<int64_t>(-imm8))));
}

template <is_neon_simd_class T>
inline T
srli(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return srli_16(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return srli_32(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return srli_64(o, imm8);
}

template <is_neon_simd_class T>
inline T
srlv_16(T &o, T &count)
{
  return _ru16r(vshlq_u16(_ru16(__r(o)), vnegq_s16(_ri16(__r(count)))));
}

template <is_neon_simd_class T>
inline T
srlv_32(T &o, T &count)
{
  return _ru32r(vshlq_u32(_ru32(__r(o)), vnegq_s32(__r(count))));
}

template <is_neon_simd_class T>
inline T
srlv_64(T &o, T &count)
{
  return _ru64r(vshlq_u64(_ru64(__r(o)), vnegq_s64(_ri64(__r(count)))));
}

template <is_neon_simd_class T>
inline T
srlv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return srlv_16(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return srlv_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return srlv_64(o, count);
}

template <is_neon_simd_class T>
inline T
sra_16(T &o, i128 count)
{
  const int c = -__scalar_cnt(count);
  return _ri16r(vshlq_s16(_ri16(__r(o)), vdupq_n_s16(static_cast<int16_t>(c))));
}

template <is_neon_simd_class T>
inline T
sra_32(T &o, i128 count)
{
  return vshlq_s32(__r(o), vdupq_n_s32(-__scalar_cnt(count)));
}

template <is_neon_simd_class T>
inline T
sra_64(T &o, i128 count)
{
  return _ri64r(vshlq_s64(_ri64(__r(o)), vdupq_n_s64(static_cast<int64_t>(-__scalar_cnt(count)))));
}

template <is_neon_simd_class T>
inline T
sra(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return sra_16(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return sra_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return sra_64(o, count);
}

template <is_neon_simd_class T>
inline T
srai_16(T &o, int imm8)
{
  return _ri16r(vshlq_s16(_ri16(__r(o)), vdupq_n_s16(static_cast<int16_t>(-imm8))));
}

template <is_neon_simd_class T>
inline T
srai_32(T &o, int imm8)
{
  return vshlq_s32(__r(o), vdupq_n_s32(-imm8));
}

template <is_neon_simd_class T>
inline T
srai_64(T &o, int imm8)
{
  return _ri64r(vshlq_s64(_ri64(__r(o)), vdupq_n_s64(static_cast<int64_t>(-imm8))));
}

template <is_neon_simd_class T>
inline T
srai(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return srai_16(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return srai_32(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return srai_64(o, imm8);
}

template <is_neon_simd_class T>
inline T
srav_16(T &o, T &count)
{
  return _ri16r(vshlq_s16(_ri16(__r(o)), vnegq_s16(_ri16(__r(count)))));
}

template <is_neon_simd_class T>
inline T
srav_32(T &o, T &count)
{
  return vshlq_s32(__r(o), vnegq_s32(__r(count)));
}

template <is_neon_simd_class T>
inline T
srav_64(T &o, T &count)
{
  return _ri64r(vshlq_s64(_ri64(__r(o)), vnegq_s64(_ri64(__r(count)))));
}

template <is_neon_simd_class T>
inline T
srav(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return srav_16(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return srav_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return srav_64(o, count);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rotates

template <is_neon_simd_class T>
inline T
rol_32(T &o, int imm8)
{
  uint32x4_t a = _ru32(__r(o));
  return _ru32r(vorrq_u32(vshlq_u32(a, vdupq_n_s32(imm8)), vshlq_u32(a, vdupq_n_s32(imm8 - 32))));
}

template <is_neon_simd_class T>
inline T
rol_64(T &o, int imm8)
{
  uint64x2_t a = _ru64(__r(o));
  return _ru64r(
      vorrq_u64(vshlq_u64(a, vdupq_n_s64(static_cast<int64_t>(imm8))), vshlq_u64(a, vdupq_n_s64(static_cast<int64_t>(imm8 - 64)))));
}

template <is_neon_simd_class T>
inline T
rol(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return rol_32(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return rol_64(o, imm8);
}

template <is_neon_simd_class T>
inline T
rolv_32(T &o, T &count)
{
  uint32x4_t a = _ru32(__r(o));
  int32x4_t c = __r(count);
  int32x4_t nc = vsubq_s32(c, vdupq_n_s32(32));
  return _ru32r(vorrq_u32(vshlq_u32(a, c), vshlq_u32(a, nc)));
}

template <is_neon_simd_class T>
inline T
rolv_64(T &o, T &count)
{
  uint64x2_t a = _ru64(__r(o));
  int64x2_t c = _ri64(__r(count));
  int64x2_t nc = vsubq_s64(c, vdupq_n_s64(64));
  return _ru64r(vorrq_u64(vshlq_u64(a, c), vshlq_u64(a, nc)));
}

template <is_neon_simd_class T>
inline T
rolv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return rolv_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return rolv_64(o, count);
}

template <is_neon_simd_class T>
inline T
ror_32(T &o, int imm8)
{
  uint32x4_t a = _ru32(__r(o));
  return _ru32r(vorrq_u32(vshlq_u32(a, vdupq_n_s32(-imm8)), vshlq_u32(a, vdupq_n_s32(32 - imm8))));
}

template <is_neon_simd_class T>
inline T
ror_64(T &o, int imm8)
{
  uint64x2_t a = _ru64(__r(o));
  return _ru64r(
      vorrq_u64(vshlq_u64(a, vdupq_n_s64(static_cast<int64_t>(-imm8))), vshlq_u64(a, vdupq_n_s64(static_cast<int64_t>(64 - imm8)))));
}

template <is_neon_simd_class T>
inline T
ror(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return ror_32(o, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return ror_64(o, imm8);
}

template <is_neon_simd_class T>
inline T
rorv_32(T &o, T &count)
{
  uint32x4_t a = _ru32(__r(o));
  int32x4_t c = __r(count);
  int32x4_t nc = vsubq_s32(vdupq_n_s32(32), c);
  return _ru32r(vorrq_u32(vshlq_u32(a, vnegq_s32(c)), vshlq_u32(a, nc)));
}

template <is_neon_simd_class T>
inline T
rorv_64(T &o, T &count)
{
  uint64x2_t a = _ru64(__r(o));
  int64x2_t c = _ri64(__r(count));
  int64x2_t nc = vsubq_s64(vdupq_n_s64(64), c);
  return _ru64r(vorrq_u64(vshlq_u64(a, vnegq_s64(c)), vshlq_u64(a, nc)));
}

template <is_neon_simd_class T>
inline T
rorv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return rorv_32(o, count);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return rorv_64(o, count);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// double-width shift left (shldi)

template <is_neon_simd_class T>
inline T
shldi_16(T &a, T &b, int imm8)
{
  int16x8_t av = _ri16(__r(a));
  uint16x8_t bv = _ru16(__r(b));
  return _ri16r(vreinterpretq_s16_u16(vorrq_u16(vreinterpretq_u16_s16(vshlq_s16(av, vdupq_n_s16(static_cast<int16_t>(imm8)))),
                                                vshlq_u16(bv, vdupq_n_s16(static_cast<int16_t>(imm8 - 16))))));
}

template <is_neon_simd_class T>
inline T
shldi_32(T &a, T &b, int imm8)
{
  uint32x4_t av = _ru32(__r(a));
  uint32x4_t bv = _ru32(__r(b));
  return _ru32r(vorrq_u32(vshlq_u32(av, vdupq_n_s32(imm8)), vshlq_u32(bv, vdupq_n_s32(imm8 - 32))));
}

template <is_neon_simd_class T>
inline T
shldi_64(T &a, T &b, int imm8)
{
  uint64x2_t av = _ru64(__r(a));
  uint64x2_t bv = _ru64(__r(b));
  return _ru64r(
      vorrq_u64(vshlq_u64(av, vdupq_n_s64(static_cast<int64_t>(imm8))), vshlq_u64(bv, vdupq_n_s64(static_cast<int64_t>(imm8 - 64)))));
}

template <is_neon_simd_class T>
inline T
shldi(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return shldi_16(a, b, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return shldi_32(a, b, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return shldi_64(a, b, imm8);
}

template <is_neon_simd_class T>
inline T
shldv_16(T &a, T &b, T &c)
{
  int16x8_t av = _ri16(__r(a));
  uint16x8_t bv = _ru16(__r(b));
  int16x8_t cs = _ri16(__r(c));
  int16x8_t nc = vsubq_s16(cs, vdupq_n_s16(16));
  return _ri16r(vreinterpretq_s16_u16(vorrq_u16(vreinterpretq_u16_s16(vshlq_s16(av, cs)), vshlq_u16(bv, nc))));
}

template <is_neon_simd_class T>
inline T
shldv_32(T &a, T &b, T &c)
{
  uint32x4_t av = _ru32(__r(a));
  uint32x4_t bv = _ru32(__r(b));
  int32x4_t cs = __r(c);
  int32x4_t nc = vsubq_s32(cs, vdupq_n_s32(32));
  return _ru32r(vorrq_u32(vshlq_u32(av, cs), vshlq_u32(bv, nc)));
}

template <is_neon_simd_class T>
inline T
shldv_64(T &a, T &b, T &c)
{
  uint64x2_t av = _ru64(__r(a));
  uint64x2_t bv = _ru64(__r(b));
  int64x2_t cs = _ri64(__r(c));
  int64x2_t nc = vsubq_s64(cs, vdupq_n_s64(64));
  return _ru64r(vorrq_u64(vshlq_u64(av, cs), vshlq_u64(bv, nc)));
}

template <is_neon_simd_class T>
inline T
shldv(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return shldv_16(a, b, c);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return shldv_32(a, b, c);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return shldv_64(a, b, c);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// double-width shift right (shrdi)

template <is_neon_simd_class T>
inline T
shrdi_16(T &a, T &b, int imm8)
{
  uint16x8_t av = _ru16(__r(a));
  uint16x8_t bv = _ru16(__r(b));
  return _ru16r(
      vorrq_u16(vshlq_u16(av, vdupq_n_s16(static_cast<int16_t>(-imm8))), vshlq_u16(bv, vdupq_n_s16(static_cast<int16_t>(16 - imm8)))));
}

template <is_neon_simd_class T>
inline T
shrdi_32(T &a, T &b, int imm8)
{
  uint32x4_t av = _ru32(__r(a));
  uint32x4_t bv = _ru32(__r(b));
  return _ru32r(vorrq_u32(vshlq_u32(av, vdupq_n_s32(-imm8)), vshlq_u32(bv, vdupq_n_s32(32 - imm8))));
}

template <is_neon_simd_class T>
inline T
shrdi_64(T &a, T &b, int imm8)
{
  uint64x2_t av = _ru64(__r(a));
  uint64x2_t bv = _ru64(__r(b));
  return _ru64r(
      vorrq_u64(vshlq_u64(av, vdupq_n_s64(static_cast<int64_t>(-imm8))), vshlq_u64(bv, vdupq_n_s64(static_cast<int64_t>(64 - imm8)))));
}

template <is_neon_simd_class T>
inline T
shrdi(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return shrdi_16(a, b, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return shrdi_32(a, b, imm8);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return shrdi_64(a, b, imm8);
}

template <is_neon_simd_class T>
inline T
shrdv_16(T &a, T &b, T &c)
{
  uint16x8_t av = _ru16(__r(a));
  uint16x8_t bv = _ru16(__r(b));
  int16x8_t cs = _ri16(__r(c));
  int16x8_t nc = vsubq_s16(vdupq_n_s16(16), cs);
  return _ru16r(vorrq_u16(vshlq_u16(av, vnegq_s16(cs)), vshlq_u16(bv, nc)));
}

template <is_neon_simd_class T>
inline T
shrdv_32(T &a, T &b, T &c)
{
  uint32x4_t av = _ru32(__r(a));
  uint32x4_t bv = _ru32(__r(b));
  int32x4_t cs = __r(c);
  int32x4_t nc = vsubq_s32(vdupq_n_s32(32), cs);
  return _ru32r(vorrq_u32(vshlq_u32(av, vnegq_s32(cs)), vshlq_u32(bv, nc)));
}

template <is_neon_simd_class T>
inline T
shrdv_64(T &a, T &b, T &c)
{
  uint64x2_t av = _ru64(__r(a));
  uint64x2_t bv = _ru64(__r(b));
  int64x2_t cs = _ri64(__r(c));
  int64x2_t nc = vsubq_s64(vdupq_n_s64(64), cs);
  return _ru64r(vorrq_u64(vshlq_u64(av, vnegq_s64(cs)), vshlq_u64(bv, nc)));
}

template <is_neon_simd_class T>
inline T
shrdv(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> )
    return shrdv_16(a, b, c);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> )
    return shrdv_32(a, b, c);
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> )
    return shrdv_64(a, b, c);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// masked operations

template <is_neon_simd_class T, typename M>
inline T
mask_slli_16(T &src, M k, T &o, int imm8)
{
  uint16x8_t msk = __expand_mask_u16(static_cast<uint8_t>(k));
  return _ri16r(vreinterpretq_s16_u16(vbslq_u16(msk, _ru16(slli_16(o, imm8)), _ru16(__r(src)))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_slli_16(M k, T &o, int imm8)
{
  uint16x8_t msk = __expand_mask_u16(static_cast<uint8_t>(k));
  return _ri16r(vreinterpretq_s16_u16(vandq_u16(msk, _ru16(slli_16(o, imm8)))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_slli_32(T &src, M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vbslq_u32(msk, _ru32(slli_32(o, imm8)), _ru32(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_slli_32(M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vandq_u32(msk, _ru32(slli_32(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_slli_64(T &src, M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(slli_64(o, imm8)), _ru64(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_slli_64(M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vandq_u64(msk, _ru64(slli_64(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_srli_16(T &src, M k, T &o, int imm8)
{
  uint16x8_t msk = __expand_mask_u16(static_cast<uint8_t>(k));
  return _ri16r(vreinterpretq_s16_u16(vbslq_u16(msk, _ru16(srli_16(o, imm8)), _ru16(__r(src)))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_srli_16(M k, T &o, int imm8)
{
  uint16x8_t msk = __expand_mask_u16(static_cast<uint8_t>(k));
  return _ri16r(vreinterpretq_s16_u16(vandq_u16(msk, _ru16(srli_16(o, imm8)))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_srli_32(T &src, M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vbslq_u32(msk, _ru32(srli_32(o, imm8)), _ru32(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_srli_32(M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vandq_u32(msk, _ru32(srli_32(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_srli_64(T &src, M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(srli_64(o, imm8)), _ru64(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_srli_64(M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vandq_u64(msk, _ru64(srli_64(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_srai_16(T &src, M k, T &o, int imm8)
{
  uint16x8_t msk = __expand_mask_u16(static_cast<uint8_t>(k));
  return _ri16r(vreinterpretq_s16_u16(vbslq_u16(msk, _ru16(srai_16(o, imm8)), _ru16(__r(src)))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_srai_16(M k, T &o, int imm8)
{
  uint16x8_t msk = __expand_mask_u16(static_cast<uint8_t>(k));
  return _ri16r(vreinterpretq_s16_u16(vandq_u16(msk, _ru16(srai_16(o, imm8)))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_srai_32(T &src, M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vbslq_u32(msk, _ru32(srai_32(o, imm8)), _ru32(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_srai_32(M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vandq_u32(msk, _ru32(srai_32(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_srai_64(T &src, M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(srai_64(o, imm8)), _ru64(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_srai_64(M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vandq_u64(msk, _ru64(srai_64(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_rol_32(T &src, M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vbslq_u32(msk, _ru32(rol_32(o, imm8)), _ru32(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_rol_32(M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vandq_u32(msk, _ru32(rol_32(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_rol_64(T &src, M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(rol_64(o, imm8)), _ru64(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_rol_64(M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vandq_u64(msk, _ru64(rol_64(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_ror_32(T &src, M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vbslq_u32(msk, _ru32(ror_32(o, imm8)), _ru32(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_ror_32(M k, T &o, int imm8)
{
  uint32x4_t msk = __expand_mask_u32(static_cast<uint8_t>(k));
  return _ru32r(vandq_u32(msk, _ru32(ror_32(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_ror_64(T &src, M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(ror_64(o, imm8)), _ru64(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_ror_64(M k, T &o, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vandq_u64(msk, _ru64(ror_64(o, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_shldi_64(T &src, M k, T &a, T &b, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(shldi_64(a, b, imm8)), _ru64(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_shldi_64(M k, T &a, T &b, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vandq_u64(msk, _ru64(shldi_64(a, b, imm8))));
}

template <is_neon_simd_class T, typename M>
inline T
mask_shrdi_64(T &src, M k, T &a, T &b, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vbslq_u64(msk, _ru64(shrdi_64(a, b, imm8)), _ru64(__r(src))));
}

template <is_neon_simd_class T, typename M>
inline T
maskz_shrdi_64(M k, T &a, T &b, int imm8)
{
  uint64x2_t msk = __expand_mask_u64(static_cast<uint8_t>(k));
  return _ru64r(vandq_u64(msk, _ru64(shrdi_64(a, b, imm8))));
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

#pragma GCC diagnostic pop

};     // namespace simd
};     // namespace micron
