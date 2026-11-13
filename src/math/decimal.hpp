//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../string/conversions/decimal.hpp"
#include "../types.hpp"
#include "ieee.hpp"

namespace micron::math
{

enum class decimal_error : u8 { none, nonfinite, short_output, internal_capacity, invalid_precision };

struct decimal_result {
  usize size = 0;
  decimal_error error = decimal_error::none;

  constexpr explicit
  operator bool() const noexcept
  {
    return error == decimal_error::none;
  }
};

// Binary64 needs at most 767 exact significant digits
inline constexpr usize binary64_decimal_capacity = 800;

inline constexpr usize binary64_significant_decimal_capacity = 32;

namespace __impl
{

[[nodiscard]] inline constexpr decimal_result
format(char *destination, usize capacity, f64 value, u32 significant_digits) noexcept
{
  const u64 bits = ieee::to_bits(value);
  const u64 exponent = (bits >> 52) & 0x7ff;
  if ( exponent == 0x7ff ) return { 0, decimal_error::nonfinite };
  u64 mantissa = bits & 0xfffffffffffffull;
  i32 binary_exponent = -1074;
  if ( exponent ) {
    mantissa |= u64{ 1 } << 52;
    binary_exponent = static_cast<i32>(exponent) - 1075;
  }
  micron::__impl::__dec::__decimal decimal{};
  micron::__impl::__dec::__dec_load_u64(decimal, mantissa);
  micron::__impl::__dec::__dec_shift(decimal, binary_exponent);
  if ( decimal.truncated ) return { 0, decimal_error::internal_capacity };

  if ( significant_digits && significant_digits < decimal.ndigits ) {
    const bool up = micron::__impl::__dec::__dec_round_up(decimal, significant_digits);
    decimal.ndigits = significant_digits;
    if ( up ) {
      u32 position = significant_digits;
      while ( position && decimal.d[position - 1] == 9 ) decimal.d[--position] = 0;
      if ( position )
        ++decimal.d[position - 1];
      else {
        decimal.d[0] = 1;
        decimal.ndigits = 1;
        ++decimal.point;
      }
    }
    micron::__impl::__dec::__dec_trim(decimal);
  }

  char output[binary64_decimal_capacity]{};
  usize size = 0;
  if ( bits >> 63 ) output[size++] = '-';
  if ( !mantissa ) {
    output[size++] = '0';
  } else if ( decimal.point > 0 && decimal.point <= 16 && decimal.ndigits <= 32 ) {
    for ( i32 i = 0; i < decimal.point || i < static_cast<i32>(decimal.ndigits); ++i ) {
      if ( i == decimal.point ) output[size++] = '.';
      output[size++] = i < static_cast<i32>(decimal.ndigits) ? static_cast<char>('0' + decimal.d[i]) : '0';
    }
  } else if ( decimal.point <= 0 && decimal.point >= -6 && static_cast<i32>(decimal.ndigits) - decimal.point <= 32 ) {
    output[size++] = '0';
    output[size++] = '.';
    for ( i32 i = 0; i < -decimal.point; ++i ) output[size++] = '0';
    for ( u32 i = 0; i < decimal.ndigits; ++i ) output[size++] = static_cast<char>('0' + decimal.d[i]);
  } else {
    output[size++] = static_cast<char>('0' + decimal.d[0]);
    if ( decimal.ndigits > 1 ) {
      output[size++] = '.';
      for ( u32 i = 1; i < decimal.ndigits; ++i ) output[size++] = static_cast<char>('0' + decimal.d[i]);
    }
    output[size++] = 'e';
    i32 scientific_exponent = decimal.point - 1;
    if ( scientific_exponent < 0 ) {
      output[size++] = '-';
      scientific_exponent = -scientific_exponent;
    }
    char digits[4]{};
    u32 count = 0;
    do {
      digits[count++] = static_cast<char>('0' + scientific_exponent % 10);
      scientific_exponent /= 10;
    } while ( scientific_exponent );
    while ( count ) output[size++] = digits[--count];
  }
  if ( !destination || size > capacity ) return { 0, decimal_error::short_output };
  for ( usize i = 0; i < size; ++i ) destination[i] = output[i];
  return { size };
}

};      // namespace __impl

// Exact finite value, preserving the existing spelling and atomic-output contract
[[nodiscard]] inline constexpr decimal_result
binary64_to_decimal(char *destination, usize capacity, f64 value) noexcept
{
  return __impl::format(destination, capacity, value, 0);
}

// Round to nearest significant decimal digits, with ties to even
[[nodiscard]] inline constexpr decimal_result
binary64_to_decimal(char *destination, usize capacity, f64 value, u32 significant_digits) noexcept
{
  if ( significant_digits < 1 || significant_digits > 17 ) return { 0, decimal_error::invalid_precision };
  return __impl::format(destination, capacity, value, significant_digits);
}

};      // namespace micron::math
