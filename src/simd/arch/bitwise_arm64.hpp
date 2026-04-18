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

__attribute__((always_inline)) static inline uint32_t
__neon_movemask_u8(uint8x16_t v) noexcept
{

  static const uint8x16_t kBitMask = { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };
  uint8x16_t bits = vandq_u8(v, kBitMask);
  uint8x8_t lo = vget_low_u8(bits);
  uint8x8_t hi = vget_high_u8(bits);

  lo = vpadd_u8(lo, hi);
  lo = vpadd_u8(lo, lo);
  lo = vpadd_u8(lo, lo);
  return vget_lane_u16(vreinterpret_u16_u8(lo), 0);
}

usize
find_first_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const uint8x16_t char_reg = vdupq_n_u8(static_cast<uint8_t>(b));
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    uint8x16_t chunk = vld1q_u8(ptr + i);
    uint8x16_t cmp = vceqq_u8(chunk, char_reg);
    uint32_t mask = __neon_movemask_u8(cmp);
    if ( mask ) return i + static_cast<usize>(__builtin_ctz(mask));
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == static_cast<unsigned char>(b) ) return i;
  return len;
}

usize
count_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const uint8x16_t char_reg = vdupq_n_u8(static_cast<uint8_t>(b));
  usize i = 0;
  usize cnt = 0;
  for ( ; i + 15 < len; i += 16 ) {
    uint8x16_t chunk = vld1q_u8(ptr + i);
    uint8x16_t cmp = vceqq_u8(chunk, char_reg);
    cnt += static_cast<usize>(__builtin_popcount(__neon_movemask_u8(cmp)));
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == static_cast<unsigned char>(b) ) ++cnt;
  return cnt;
}

bool
any_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const uint8x16_t char_reg = vdupq_n_u8(static_cast<uint8_t>(b));
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    uint8x16_t chunk = vld1q_u8(ptr + i);
    uint8x16_t cmp = vceqq_u8(chunk, char_reg);
    if ( __neon_movemask_u8(cmp) ) return true;
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == static_cast<unsigned char>(b) ) return true;
  return false;
}

bool
all_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const uint8x16_t char_reg = vdupq_n_u8(static_cast<uint8_t>(b));
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    uint8x16_t chunk = vld1q_u8(ptr + i);
    uint8x16_t cmp = vceqq_u8(chunk, char_reg);
    if ( __neon_movemask_u8(cmp) != 0xFFFFu ) return false;
  }
  for ( ; i < len; ++i )
    if ( ptr[i] != static_cast<unsigned char>(b) ) return false;
  return true;
}

bool
none_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const uint8x16_t char_reg = vdupq_n_u8(static_cast<uint8_t>(b));
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    uint8x16_t chunk = vld1q_u8(ptr + i);
    uint8x16_t cmp = vceqq_u8(chunk, char_reg);
    if ( __neon_movemask_u8(cmp) ) return false;
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == static_cast<unsigned char>(b) ) return false;
  return true;
}

};     // namespace simd
};     // namespace micron
