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
};

namespace maximum
{

constexpr u8 u8bit = 0xFF;
constexpr u16 u16bit = 0xFFFF;
constexpr u32 u32bit = 0xFFFFFFFF;
constexpr u64 u64bit = 0xFFFFFFFFFFFFFFFF;
constexpr umax_t maxbit = 0xFFFFFFFFFFFFFFFF;

constexpr i8 i8bit = 127;
constexpr i16 i16bit = 32767;
constexpr i32 i32bit = 2147483647;
constexpr i64 i64bit = 9223372036854775807;
constexpr f32 f32bit = 3.402823466e+38f;
constexpr f64 f64bit = 1.79769313486231e+308;
};
namespace minimum
{
constexpr i8 i8bit = -127 - 1;
constexpr i16 i16bit = -32767 - 1;
constexpr i32 i32bit = -2147483647 - 1;
constexpr i64 i64bit = -9223372036854775807 - 1;
constexpr f32 f32bit = -maximum::f32bit;
constexpr f64 f64bit = -maximum::f64bit;
};

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
  // static constexpr float_denorm_style has_denorm = denorm_absent;
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
  //static constexpr float_round_style round_style = round_toward_zero;
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
  static constexpr int digits10 = 1;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr int radix = 2;
  static constexpr bool
  epsilon() noexcept
  {
    return bool();
  }
  static constexpr bool
  round_error() noexcept
  {
    return bool();
  }

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  // static constexpr float_denorm_style has_denorm = denorm_absent;
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
  static constexpr bool is_bounded = false;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
  //static constexpr float_round_style round_style = round_toward_zero;
};

template <> class numeric_limits<char>
{
public:
  static constexpr bool is_specialized = true;

  static constexpr char
  min() noexcept
  {
    return false;
  }
  static constexpr char
  max() noexcept
  {
    return true;
  }
  static constexpr char
  lowest() noexcept
  {
    return false;
  }

  static constexpr int digits = 1;
  static constexpr int digits10 = 1;
  static constexpr int max_digits10 = 0;

  static constexpr bool is_signed = false;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr int radix = 2;

  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;

  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  // static constexpr float_denorm_style has_denorm = denorm_absent;
  static constexpr bool has_denorm_loss = false;

  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = false;
  static constexpr bool is_modulo = false;

  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
  //static constexpr float_round_style round_style = round_toward_zero;
};
};
