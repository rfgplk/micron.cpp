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
using __bytes16 = u8 __attribute__((vector_size(16)));
using __signed16 = i8 __attribute__((vector_size(16)));
using __words8 = u16 __attribute__((vector_size(16)));
using __dwords4 = u32 __attribute__((vector_size(16)));

inline bool
read16(const char *source, __prefix16 &value) noexcept
{
  __bytes16 input;
  __builtin_memcpy(&input, source, 16);
  auto digits = input - __bytes16{ 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48 };
  const auto invalid = digits > __bytes16{ 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 };
  const auto periods = input == __bytes16{ 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46 };
  const u32 dots = static_cast<u32>(__builtin_wasm_bitmask_i8x16(__builtin_bit_cast(__signed16, periods)));
  const u32 bad = static_cast<u32>(__builtin_wasm_bitmask_i8x16(__builtin_bit_cast(__signed16, invalid)));
  if ( bad != dots || (dots & (dots - 1)) ) return false;
  const u32 point = dots ? static_cast<u32>(__builtin_ctz(dots)) : 16;
  if ( dots ) {
    __block16 block;
    __builtin_memcpy(&block, &digits, 16);
    block = __compact16(block, point);
    __builtin_memcpy(&digits, &block, 16);
  }
  const auto even = __builtin_convertvector(__builtin_shufflevector(digits, digits, 0, 2, 4, 6, 8, 10, 12, 14), __words8);
  const auto odd = __builtin_convertvector(__builtin_shufflevector(digits, digits, 1, 3, 5, 7, 9, 11, 13, 15), __words8);
  const auto pairs = even * __words8{ 10, 10, 10, 10, 10, 10, 10, 10 } + odd;
  const auto high = __builtin_convertvector(__builtin_shufflevector(pairs, pairs, 0, 2, 4, 6), __dwords4);
  const auto low = __builtin_convertvector(__builtin_shufflevector(pairs, pairs, 1, 3, 5, 7), __dwords4);
  const auto groups = high * __dwords4{ 100, 100, 100, 100 } + low;
  const u64 mantissa = static_cast<u64>(groups[0] * 10000 + groups[1]) * 100000000 + groups[2] * 10000 + groups[3];
  value = { dots ? mantissa / 10 : mantissa, point };
  return true;
}

inline bool
read8(const char *source, u32 &value) noexcept
{
  __bytes16 input{};
  __builtin_memcpy(&input, source, 8);
  const auto digits = input - __bytes16{ 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48 };
  const auto invalid = digits > __bytes16{ 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 };
  if ( __builtin_wasm_bitmask_i8x16(__builtin_bit_cast(__signed16, invalid)) & 255 ) return false;
  const auto even = __builtin_convertvector(__builtin_shufflevector(digits, digits, 0, 2, 4, 6, 0, 0, 0, 0), __words8);
  const auto odd = __builtin_convertvector(__builtin_shufflevector(digits, digits, 1, 3, 5, 7, 0, 0, 0, 0), __words8);
  const auto pairs = even * __words8{ 10, 10, 10, 10, 10, 10, 10, 10 } + odd;
  const auto high = __builtin_convertvector(__builtin_shufflevector(pairs, pairs, 0, 2, 0, 0), __dwords4);
  const auto low = __builtin_convertvector(__builtin_shufflevector(pairs, pairs, 1, 3, 0, 0), __dwords4);
  const auto groups = high * __dwords4{ 100, 100, 100, 100 } + low;
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
  using __words4 = u16 __attribute__((vector_size(8)));
  const auto q = __builtin_convertvector(tens, __words4), r = __builtin_convertvector(ones, __words4);
  const auto interleaved = __builtin_shufflevector(q, r, 0, 4, 1, 5, 2, 6, 3, 7);
  using __bytes8 = u8 __attribute__((vector_size(8)));
  const auto digits = __builtin_convertvector(interleaved, __bytes8);
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
