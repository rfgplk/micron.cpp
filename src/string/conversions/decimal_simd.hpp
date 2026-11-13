//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#if !defined(MICRON_NO_SIMD) && !defined(__micron_simd_generic) && defined(__micron_x86_sse2)
#include "arch/decimal_x86.hpp"
#elif !defined(MICRON_NO_SIMD) && !defined(__micron_simd_generic) && defined(__micron_arch_arm32) && defined(__micron_arm_neon)
#include "arch/decimal_arm32.hpp"
#elif !defined(MICRON_NO_SIMD) && !defined(__micron_simd_generic) && defined(__micron_arch_arm64) && defined(__micron_arm_neon)
#include "arch/decimal_arm64.hpp"
#elif defined(__micron_wasm_simd128)
#include "arch/decimal_wasm.hpp"
#else
#include "arch/decimal_generic.hpp"
#endif

namespace micron::__impl::__digits
{

inline constexpr bool
read16(const char *source, __prefix16 &value) noexcept
{
  if consteval {
    return __scalar16(source, value);
  } else {
    return __arch::read16(source, value);
  }
}

// Callers supply exactly eight accessible bytes. Failed parses leave value alone.
inline constexpr bool
read8(const char *source, u32 &value) noexcept
{
  if consteval {
    u32 result = 0;
    for ( u32 i = 0; i < 8; ++i ) {
      const u32 digit = static_cast<u8>(source[i]) - static_cast<u32>('0');
      if ( digit > 9 ) return false;
      result = result * 10 + digit;
    }
    value = result;
    return true;
  } else {
    return __arch::read8(source, value);
  }
}

// Eight raw digits (0..9), including leading zeroes; value is below 10^8.
inline constexpr void
write8(u8 *destination, u32 value) noexcept
{
  if consteval {
    for ( u32 i = 8; i-- > 0; ) {
      destination[i] = static_cast<u8>(value % 10);
      value /= 10;
    }
  } else {
    __arch::write8(destination, value);
  }
}

inline constexpr void
ascii(char *destination, const u8 *source, usize count) noexcept
{
  if consteval {
    for ( usize i = 0; i < count; ++i ) destination[i] = static_cast<char>('0' + source[i]);
  } else {
    __arch::ascii(destination, source, count);
  }
}

};      // namespace micron::__impl::__digits
