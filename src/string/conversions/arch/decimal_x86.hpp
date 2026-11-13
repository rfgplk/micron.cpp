//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../simd/aliases.hpp"
#include "../../../types.hpp"
#include "../decimal_prefix.hpp"

namespace micron::__impl::__digits::__arch
{
namespace sse = micron::simd::sse;

inline bool
read16(const char *source, __prefix16 &value) noexcept
{
  __m128i input;
  __builtin_memcpy(&input, source, 16);
  auto digits = sse::sub_i8(input, sse::splat_i8('0'));
  const u32 valid = static_cast<u32>(sse::movemask_i8(sse::eq_i8(sse::min_u8(digits, sse::splat_i8(9)), digits)));
  const u32 dots = static_cast<u32>(sse::movemask_i8(sse::eq_i8(input, sse::splat_i8('.'))));
  if ( (valid | dots) != 65535 || (dots & (dots - 1)) ) return false;
  const u32 point = dots ? static_cast<u32>(__builtin_ctz(dots)) : 16;
  if ( dots ) {
    __block16 block;
    __builtin_memcpy(&block, &digits, 16);
    block = __compact16(block, point);
    __builtin_memcpy(&digits, &block, 16);
  }
#if defined(__micron_x86_ssse3)
  const auto pairs = sse::madd_u8s8(digits, sse::splat_i16(0x010a));
#else
  const auto weights = sse::set_i16(1, 10, 1, 10, 1, 10, 1, 10);
  const auto wide_low = sse::madd_i16(sse::unpack_lo_i8(digits, sse::zero_i128()), weights);
  const auto wide_high = sse::madd_i16(sse::unpack_hi_i8(digits, sse::zero_i128()), weights);
  const auto pairs = sse::pack_sat_i32(wide_low, wide_high);
#endif
  const auto groups = sse::madd_i16(pairs, sse::splat_i32(0x00010064));
  const u32 high
      = static_cast<u32>(sse::extract_low_i32(groups)) * 10000 + static_cast<u32>(sse::extract_low_i32(sse::bsrl_i128<4>(groups)));
  const u32 low = static_cast<u32>(sse::extract_low_i32(sse::bsrl_i128<8>(groups))) * 10000
                  + static_cast<u32>(sse::extract_low_i32(sse::bsrl_i128<12>(groups)));
  const u64 mantissa = static_cast<u64>(high) * 100000000 + low;
  value = { dots ? mantissa / 10 : mantissa, point };
  return true;
}

inline bool
read8(const char *source, u32 &value) noexcept
{
  __m128i input = sse::zero_i128();
  __builtin_memcpy(&input, source, 8);
  const auto digits = sse::sub_i8(input, sse::splat_i8('0'));
  const auto valid = sse::eq_i8(sse::min_u8(digits, sse::splat_i8(9)), digits);
  if ( (sse::movemask_i8(valid) & 255) != 255 ) return false;
#if defined(__micron_x86_ssse3)
  const auto pairs = sse::madd_u8s8(digits, sse::splat_i16(0x010a));
#else
  const auto wide = sse::unpack_lo_i8(digits, sse::zero_i128());
  const auto products = sse::madd_i16(wide, sse::set_i16(1, 10, 1, 10, 1, 10, 1, 10));
  const auto pairs = sse::pack_sat_i32(products, sse::zero_i128());
#endif
  const auto groups = sse::madd_i16(pairs, sse::splat_i32(0x00010064));
  value = static_cast<u32>(sse::extract_low_i32(groups)) * 10000 + static_cast<u32>(sse::extract_low_i32(sse::bsrl_i128<4>(groups)));
  return true;
}

inline void
write8(u8 *destination, u32 value) noexcept
{
  const u32 high = value / 10000, low = value % 10000;
  const auto pairs = sse::set_i16(0, 0, 0, 0, low % 100, low / 100, high % 100, high / 100);
  const auto tens = sse::shr_i16(sse::mul_hi_u16(pairs, sse::splat_i16(static_cast<i16>(52429))), 3);
  const auto ones = sse::sub_i16(pairs, sse::mul_lo_i16(tens, sse::splat_i16(10)));
  const auto digits = sse::pack_satu_i16(sse::unpack_lo_i16(tens, ones), sse::zero_i128());
  __builtin_memcpy(destination, &digits, 8);
}

inline void
ascii(char *destination, const u8 *source, usize count) noexcept
{
  usize i = 0;
  for ( ; count - i >= 16; i += 16 ) {
    __m128i digits;
    __builtin_memcpy(&digits, source + i, 16);
    const auto text = sse::add_i8(digits, sse::splat_i8('0'));
    __builtin_memcpy(destination + i, &text, 16);
  }
  for ( ; i < count; ++i ) destination[i] = static_cast<char>('0' + source[i]);
}
};      // namespace micron::__impl::__digits::__arch
