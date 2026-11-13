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
inline bool
read16(const char *source, __prefix16 &value) noexcept
{
  return __scalar16(source, value);
}

inline bool
read8(const char *source, u32 &value) noexcept
{
  u32 result = 0;
  for ( u32 i = 0; i < 8; ++i ) {
    const u32 digit = static_cast<u8>(source[i]) - static_cast<u32>('0');
    if ( digit > 9 ) return false;
    result = result * 10 + digit;
  }
  value = result;
  return true;
}

inline void
write8(u8 *destination, u32 value) noexcept
{
  for ( u32 i = 8; i-- > 0; ) {
    destination[i] = static_cast<u8>(value % 10);
    value /= 10;
  }
}

inline void
ascii(char *destination, const u8 *source, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) destination[i] = static_cast<char>('0' + source[i]);
}
};      // namespace micron::__impl::__digits::__arch
