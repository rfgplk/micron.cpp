//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "types.hpp"

namespace micron
{

namespace bits
{
constexpr int byte = 8;
constexpr int _char = 8;
};     // namespace bits

namespace maximum
{

constexpr u8 u8bit = 0xFF;
constexpr u16 u16bit = 0xFFFF;
constexpr u32 u32bit = 0xFFFFFFFFL;
constexpr u64 u64bit = 0xFFFFFFFFFFFFFFFFuLL;
constexpr umax_t maxbit = 0xFFFFFFFFFFFFFFFFuLL;

constexpr i8 i8bit = 127;
constexpr i16 i16bit = 32767;
constexpr i32 i32bit = 2147483647;
constexpr i64 i64bit = 9223372036854775807;
constexpr f32 f32bit = 3.402823466e+38f;
constexpr f64 f64bit = 1.79769313486231e+308;
};     // namespace maximum

namespace minimum
{
constexpr i8 i8bit = -127 - 1;
constexpr i16 i16bit = -32767 - 1;
constexpr i32 i32bit = -2147483647 - 1;
constexpr i64 i64bit = -9223372036854775807 - 1;
constexpr f32 f32bit = -maximum::f32bit;
constexpr f64 f64bit = -maximum::f64bit;
};     // namespace minimum

constexpr f32 pi = 3.14159265359f;
constexpr f32 half_pi = 1.5707963267f;
constexpr f64 pi64 = 3.14159265359f;
constexpr f64 half_pi64 = 1.5707963267f;

template <class T> class numeric_limits
{
public:
  static constexpr bool is_specialized = false;

  static constexpr T
  min() noexcept
  {
    return T();
  }

  static constexpr T
  max() noexcept
  {
    return T();
  }

  static constexpr T
  lowest() noexcept
  {
    return T();
  }

  static constexpr int digits = 0;
  static constexpr int digits10 = 0;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr int radix = 0;

  static constexpr T
  epsilon() noexcept
  {
    return T();
  }

  static constexpr T
  round_error() noexcept
  {
    return T();
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr T
  infinity() noexcept
  {
    return T();
  }

  static constexpr T
  quiet_NaN() noexcept
  {
    return T();
  }

  static constexpr T
  signaling_NaN() noexcept
  {
    return T();
  }

  static constexpr T
  denorm_min() noexcept
  {
    return T();
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = false;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<bool>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr bool
  min() noexcept
  {
    return false;
  }

  static constexpr bool
  max() noexcept
  {
    return true;
  }

  static constexpr bool
  lowest() noexcept
  {
    return false;
  }

  static constexpr int digits = 1;
  static constexpr int digits10 = 0;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr bool
  epsilon() noexcept
  {
    return false;
  }

  static constexpr bool
  round_error() noexcept
  {
    return false;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr bool
  infinity() noexcept
  {
    return false;
  }

  static constexpr bool
  quiet_NaN() noexcept
  {
    return false;
  }

  static constexpr bool
  signaling_NaN() noexcept
  {
    return false;
  }

  static constexpr bool
  denorm_min() noexcept
  {
    return false;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<char>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr char
  min() noexcept
  {
    return '\0';
  }

  static constexpr char
  max() noexcept
  {
    return '\x7f';
  }

  static constexpr char
  lowest() noexcept
  {
    return '\0';
  }

  static constexpr int digits = 7;
  static constexpr int digits10 = 2;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr char
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr char
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr char
  infinity() noexcept
  {
    return 0;
  }

  static constexpr char
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr char
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr char
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<signed char>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr signed char
  min() noexcept
  {
    return -128;
  }

  static constexpr signed char
  max() noexcept
  {
    return 127;
  }

  static constexpr signed char
  lowest() noexcept
  {
    return -128;
  }

  static constexpr int digits = 7;
  static constexpr int digits10 = 2;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr signed char
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr signed char
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr signed char
  infinity() noexcept
  {
    return 0;
  }

  static constexpr signed char
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr signed char
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr signed char
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<unsigned char>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr unsigned char
  min() noexcept
  {
    return 0;
  }

  static constexpr unsigned char
  max() noexcept
  {
    return 255;
  }

  static constexpr unsigned char
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 8;
  static constexpr int digits10 = 2;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr unsigned char
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr unsigned char
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr unsigned char
  infinity() noexcept
  {
    return 0;
  }

  static constexpr unsigned char
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned char
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned char
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<char8_t>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr char8_t
  min() noexcept
  {
    return 0;
  }

  static constexpr char8_t
  max() noexcept
  {
    return 255;
  }

  static constexpr char8_t
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 8;
  static constexpr int digits10 = 2;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr char8_t
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr char8_t
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr char8_t
  infinity() noexcept
  {
    return 0;
  }

  static constexpr char8_t
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr char8_t
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr char8_t
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<char16_t>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr char16_t
  min() noexcept
  {
    return 0;
  }

  static constexpr char16_t
  max() noexcept
  {
    return 0xFFFF;
  }

  static constexpr char16_t
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 16;
  static constexpr int digits10 = 4;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr char16_t
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr char16_t
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr char16_t
  infinity() noexcept
  {
    return 0;
  }

  static constexpr char16_t
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr char16_t
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr char16_t
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<char32_t>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr char32_t
  min() noexcept
  {
    return 0;
  }

  static constexpr char32_t
  max() noexcept
  {
    return 0xFFFFFFFF;
  }

  static constexpr char32_t
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 32;
  static constexpr int digits10 = 9;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr char32_t
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr char32_t
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr char32_t
  infinity() noexcept
  {
    return 0;
  }

  static constexpr char32_t
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr char32_t
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr char32_t
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<wchar_t>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr wchar_t
  min() noexcept
  {
    return 0;
  }

  static constexpr wchar_t
  max() noexcept
  {
    return 0x7FFFFFFF;
  }

  static constexpr wchar_t
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 31;
  static constexpr int digits10 = 9;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr wchar_t
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr wchar_t
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr wchar_t
  infinity() noexcept
  {
    return 0;
  }

  static constexpr wchar_t
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr wchar_t
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr wchar_t
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<short>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr short
  min() noexcept
  {
    return -32768;
  }

  static constexpr short
  max() noexcept
  {
    return 32767;
  }

  static constexpr short
  lowest() noexcept
  {
    return -32768;
  }

  static constexpr int digits = 15;
  static constexpr int digits10 = 4;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr short
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr short
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr short
  infinity() noexcept
  {
    return 0;
  }

  static constexpr short
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr short
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr short
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<unsigned short>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr unsigned short
  min() noexcept
  {
    return 0;
  }

  static constexpr unsigned short
  max() noexcept
  {
    return 65535;
  }

  static constexpr unsigned short
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 16;
  static constexpr int digits10 = 4;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr unsigned short
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr unsigned short
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr unsigned short
  infinity() noexcept
  {
    return 0;
  }

  static constexpr unsigned short
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned short
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned short
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<int>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr int
  min() noexcept
  {
    return -2147483648;
  }

  static constexpr int
  max() noexcept
  {
    return 2147483647;
  }

  static constexpr int
  lowest() noexcept
  {
    return -2147483648;
  }

  static constexpr int digits = 31;
  static constexpr int digits10 = 9;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr int
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr int
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr int
  infinity() noexcept
  {
    return 0;
  }

  static constexpr int
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr int
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr int
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<unsigned int>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr unsigned int
  min() noexcept
  {
    return 0;
  }

  static constexpr unsigned int
  max() noexcept
  {
    return 4294967295U;
  }

  static constexpr unsigned int
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 32;
  static constexpr int digits10 = 9;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr unsigned int
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr unsigned int
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr unsigned int
  infinity() noexcept
  {
    return 0;
  }

  static constexpr unsigned int
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned int
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned int
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<long>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr long
  min() noexcept
  {
    return -2147483648L;
  }

  static constexpr long
  max() noexcept
  {
    return 2147483647L;
  }

  static constexpr long
  lowest() noexcept
  {
    return -2147483648L;
  }

  static constexpr int digits = 31;
  static constexpr int digits10 = 9;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr long
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr long
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr long
  infinity() noexcept
  {
    return 0;
  }

  static constexpr long
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr long
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr long
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<unsigned long>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr unsigned long
  min() noexcept
  {
    return 0;
  }

  static constexpr unsigned long
  max() noexcept
  {
    return 4294967295UL;
  }

  static constexpr unsigned long
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 32;
  static constexpr int digits10 = 9;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr unsigned long
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr unsigned long
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr unsigned long
  infinity() noexcept
  {
    return 0;
  }

  static constexpr unsigned long
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned long
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned long
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<long long>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr long long
  min() noexcept
  {
    return -9223372036854775807LL - 1;
  }

  static constexpr long long
  max() noexcept
  {
    return 9223372036854775807LL;
  }

  static constexpr long long
  lowest() noexcept
  {
    return -9223372036854775807LL - 1;
  }

  static constexpr int digits = 63;
  static constexpr int digits10 = 18;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr long long
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr long long
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr long long
  infinity() noexcept
  {
    return 0;
  }

  static constexpr long long
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr long long
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr long long
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<unsigned long long>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr unsigned long long
  min() noexcept
  {
    return 0;
  }

  static constexpr unsigned long long
  max() noexcept
  {
    return 18446744073709551615ULL;
  }

  static constexpr unsigned long long
  lowest() noexcept
  {
    return 0;
  }

  static constexpr int digits = 64;
  static constexpr int digits10 = 19;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;

  static constexpr unsigned long long
  epsilon() noexcept
  {
    return 0;
  }

  static constexpr unsigned long long
  round_error() noexcept
  {
    return 0;
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool has_denorm_loss = false;

  static constexpr unsigned long long
  infinity() noexcept
  {
    return 0;
  }

  static constexpr unsigned long long
  quiet_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned long long
  signaling_NaN() noexcept
  {
    return 0;
  }

  static constexpr unsigned long long
  denorm_min() noexcept
  {
    return 0;
  }

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = true;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<float>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr float
  min() noexcept
  {
    return 1.17549435e-38f;
  }

  static constexpr float
  max() noexcept
  {
    return 3.40282347e+38f;
  }

  static constexpr float
  lowest() noexcept
  {
    return -3.40282347e+38f;
  }

  static constexpr int digits = 24;
  static constexpr int digits10 = 6;
  static constexpr int max_digits10 = 9;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr int radix = 2;

  static constexpr float
  epsilon() noexcept
  {
    return 1.19209290e-7f;
  }

  static constexpr float
  round_error() noexcept
  {
    return 0.5f;
  }

  static constexpr int min_exponent = -125;
  static constexpr int min_exponent10 = -37;
  static constexpr int max_exponent = 128;
  static constexpr int max_exponent10 = 38;

  static constexpr bool has_infinity = true;
  static constexpr bool has_quiet_NaN = true;
  static constexpr bool has_signaling_NaN = true;
  static constexpr bool has_denorm_loss = false;

  static constexpr float
  infinity() noexcept
  {
    return __builtin_huge_valf();
  }

  static constexpr float
  quiet_NaN() noexcept
  {
    return __builtin_nanf("");
  }

  static constexpr float
  signaling_NaN() noexcept
  {
    return __builtin_nansf("");
  }

  static constexpr float
  denorm_min() noexcept
  {
    return 1.40129846e-45f;
  }

  static constexpr bool is_iec559 = true;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<double>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr double
  min() noexcept
  {
    return 2.2250738585072014e-308;
  }

  static constexpr double
  max() noexcept
  {
    return 1.7976931348623157e+308;
  }

  static constexpr double
  lowest() noexcept
  {
    return -1.7976931348623157e+308;
  }

  static constexpr int digits = 53;
  static constexpr int digits10 = 15;
  static constexpr int max_digits10 = 17;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr int radix = 2;

  static constexpr double
  epsilon() noexcept
  {
    return 2.2204460492503131e-16;
  }

  static constexpr double
  round_error() noexcept
  {
    return 0.5;
  }

  static constexpr int min_exponent = -1021;
  static constexpr int min_exponent10 = -307;
  static constexpr int max_exponent = 1024;
  static constexpr int max_exponent10 = 308;

  static constexpr bool has_infinity = true;
  static constexpr bool has_quiet_NaN = true;
  static constexpr bool has_signaling_NaN = true;
  static constexpr bool has_denorm_loss = false;

  static constexpr double
  infinity() noexcept
  {
    return __builtin_huge_val();
  }

  static constexpr double
  quiet_NaN() noexcept
  {
    return __builtin_nan("");
  }

  static constexpr double
  signaling_NaN() noexcept
  {
    return __builtin_nans("");
  }

  static constexpr double
  denorm_min() noexcept
  {
    return 4.9406564584124654e-324;
  }

  static constexpr bool is_iec559 = true;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <> class numeric_limits<long double>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr long double
  min() noexcept
  {
    return 3.36210314311209350626e-4932L;
  }

  static constexpr long double
  max() noexcept
  {
    return 1.18973149535723176502e+4932L;
  }

  static constexpr long double
  lowest() noexcept
  {
    return -1.18973149535723176502e+4932L;
  }

  static constexpr int digits = 64;
  static constexpr int digits10 = 18;
  static constexpr int max_digits10 = 21;

  static constexpr bool is_signed = true;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr int radix = 2;

  static constexpr long double
  epsilon() noexcept
  {
    return 1.08420217248550443401e-19L;
  }

  static constexpr long double
  round_error() noexcept
  {
    return 0.5L;
  }

  static constexpr int min_exponent = -16381;
  static constexpr int min_exponent10 = -4931;
  static constexpr int max_exponent = 16384;
  static constexpr int max_exponent10 = 4932;

  static constexpr bool has_infinity = true;
  static constexpr bool has_quiet_NaN = true;
  static constexpr bool has_signaling_NaN = true;
  static constexpr bool has_denorm_loss = false;

  static constexpr long double
  infinity() noexcept
  {
    return __builtin_huge_vall();
  }

  static constexpr long double
  quiet_NaN() noexcept
  {
    return __builtin_nanl("");
  }

  static constexpr long double
  signaling_NaN() noexcept
  {
    return __builtin_nansl("");
  }

  static constexpr long double
  denorm_min() noexcept
  {
    return 3.64519953188247460253e-4951L;
  }

  static constexpr bool is_iec559 = true;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
};

template <class T> class numeric_limits<const T> : public numeric_limits<T>
{
};

template <class T> class numeric_limits<volatile T> : public numeric_limits<T>
{
};

template <class T> class numeric_limits<const volatile T> : public numeric_limits<T>
{
};

};     // namespace micron
