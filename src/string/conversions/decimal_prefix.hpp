//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

namespace micron::__impl::__digits
{
struct __prefix16 {
  u64 mantissa;
  u32 point;
};

inline constexpr bool
__scalar16(const char *source, __prefix16 &value) noexcept
{
  u64 mantissa = 0;
  u32 point = 16;
  for ( u32 i = 0; i < 16; ++i ) {
    if ( source[i] == '.' && point == 16 ) {
      point = i;
      continue;
    }
    const u32 digit = static_cast<u8>(source[i]) - static_cast<u32>('0');
    if ( digit > 9 ) return false;
    mantissa = mantissa * 10 + digit;
  }
  value = { mantissa, point };
  return true;
}

struct __block16 {
  u64 low, high;
};

// Little-endian byte deletion. The vacated last byte becomes a raw zero digit.
inline __block16
__compact16(__block16 value, u32 point) noexcept
{
  if ( point < 8 ) {
    const u64 mask = (u64{ 1 } << (point * 8)) - 1;
    value.low = (value.low & mask) | ((value.low >> 8) & ~mask) | (value.high << 56);
    value.high >>= 8;
  } else {
    const u64 mask = (u64{ 1 } << ((point - 8) * 8)) - 1;
    value.high = (value.high & mask) | ((value.high >> 8) & ~mask);
  }
  return value;
}
};      // namespace micron::__impl::__digits
