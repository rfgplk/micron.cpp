//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"
#include "../decimal_prefix.hpp"

namespace micron::__impl::__digits::__arch
{
using __bytes8 = u8 __attribute__((vector_size(8)));
using __bytes16 = u8 __attribute__((vector_size(16)));
using __words4 = u16 __attribute__((vector_size(8)));
using __dwords2 = u32 __attribute__((vector_size(8)));
using __dwords4 = u32 __attribute__((vector_size(16)));

inline bool
read16(const char *source, __prefix16 &value) noexcept
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return __scalar16(source, value);
#else
  __bytes16 input;
  __builtin_memcpy(&input, source, 16);
  auto digits = input - __bytes16{ 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48 };
  const auto dots = input == __bytes16{ 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46 };
  const auto invalid = (digits > __bytes16{ 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 }) & ~dots;
  __block16 bad, mask;
  __builtin_memcpy(&bad, &invalid, 16);
  __builtin_memcpy(&mask, &dots, 16);
  if ( bad.low || bad.high || (mask.low && mask.high) ) return false;
  const u64 bits = mask.low ? mask.low : mask.high;
  u32 point = 16;
  if ( bits ) {
    const u32 lane = static_cast<u32>(__builtin_ctzll(bits)) / 8;
    if ( bits != (u64{ 255 } << (lane * 8)) ) return false;
    point = lane + (mask.low ? 0 : 8);
    __block16 block;
    __builtin_memcpy(&block, &digits, 16);
    block = __compact16(block, point);
    __builtin_memcpy(&digits, &block, 16);
  }
  using __words8 = u16 __attribute__((vector_size(16)));
  const auto words = __builtin_bit_cast(__words8, digits);
  const auto pairs
      = (words & __words8{ 255, 255, 255, 255, 255, 255, 255, 255 }) * __words8{ 10, 10, 10, 10, 10, 10, 10, 10 } + (words >> 8);
  const auto packed = __builtin_bit_cast(__dwords4, pairs);
  const auto groups = (packed & __dwords4{ 65535, 65535, 65535, 65535 }) * __dwords4{ 100, 100, 100, 100 } + (packed >> 16);
  const u64 mantissa = static_cast<u64>(groups[0] * 10000 + groups[1]) * 100000000 + groups[2] * 10000 + groups[3];
  value = { bits ? mantissa / 10 : mantissa, point };
  return true;
#endif
}

inline bool
read8(const char *source, u32 &value) noexcept
{
  __bytes8 bytes;
  __builtin_memcpy(&bytes, source, 8);
  const auto digits = bytes - __bytes8{ 48, 48, 48, 48, 48, 48, 48, 48 };
  const auto invalid = digits > __bytes8{ 9, 9, 9, 9, 9, 9, 9, 9 };
  if ( __builtin_bit_cast(u64, invalid) ) return false;
  const auto words = __builtin_bit_cast(__words4, digits);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  const auto pairs = (words >> 8) * __words4{ 10, 10, 10, 10 } + (words & __words4{ 255, 255, 255, 255 });
  const auto packed = __builtin_bit_cast(__dwords2, pairs);
  const auto groups = (packed >> 16) * __dwords2{ 100, 100 } + (packed & __dwords2{ 65535, 65535 });
#else
  const auto pairs = (words & __words4{ 255, 255, 255, 255 }) * __words4{ 10, 10, 10, 10 } + (words >> 8);
  const auto packed = __builtin_bit_cast(__dwords2, pairs);
  const auto groups = (packed & __dwords2{ 65535, 65535 }) * __dwords2{ 100, 100 } + (packed >> 16);
#endif
  value = groups[0] * 10000 + groups[1];
  return true;
}

inline void
write8(u8 *destination, u32 value) noexcept
{
  const u32 high = value / 10000, low = value % 10000;
  const __dwords4 pairs{ high / 100, high % 100, low / 100, low % 100 };
  const auto tens = (pairs * __dwords4{ 52429, 52429, 52429, 52429 }) >> 19;
  const auto ones = pairs - tens * __dwords4{ 10, 10, 10, 10 };
  const auto q = __builtin_convertvector(tens, __words4), r = __builtin_convertvector(ones, __words4);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  const auto digits = (q << 8) | r;
#else
  const auto digits = q | (r << 8);
#endif
  __builtin_memcpy(destination, &digits, 8);
}

inline void
ascii(char *destination, const u8 *source, usize count) noexcept
{
  usize i = 0;
  const __bytes16 zero{ 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48 };
  for ( ; count - i >= 16; i += 16 ) {
    __bytes16 digits;
    __builtin_memcpy(&digits, source + i, 16);
    const auto text = digits + zero;
    __builtin_memcpy(destination + i, &text, 16);
  }
  for ( ; i < count; ++i ) destination[i] = static_cast<char>('0' + source[i]);
}
};      // namespace micron::__impl::__digits::__arch
