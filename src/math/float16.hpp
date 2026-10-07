// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Two-byte storage, independent of compiler scalar aliases and the floating-point environment

namespace micron::math
{
namespace __compact
{
constexpr u32
__rne(u32 value, u32 shift) noexcept
{
  const u32 retained = value >> shift, mask = (u32(1) << shift) - 1;
  const u32 rest = value & mask, half = u32(1) << (shift - 1);
  return retained + u32(rest > half || (rest == half && (retained & 1)));
}

constexpr u16
__encode16(u32 raw) noexcept
{
  const u32 sign = (raw >> 16) & 0x8000;
  const u32 fraction = raw & 0x7fffff, exponent = (raw >> 23) & 0xff;
  if ( exponent == 255 ) return u16(sign | (fraction ? 0x7e00 : 0x7c00));
  const i32 e = i32(exponent) - 112;
  if ( e >= 31 ) return u16(sign | 0x7c00);
  if ( e < -10 ) return u16(sign);
  if ( e <= 0 ) return u16(sign | __rne(fraction | 0x800000, u32(14 - e)));
  return u16(sign | ((u32(e) << 10) + __rne(fraction, 13)));
}

constexpr f32
__decode16(u16 value) noexcept
{
  const u32 sign = u32(value & 0x8000) << 16;
  u32 e = (value >> 10) & 31, f = value & 1023;
  if ( e == 31 ) return __builtin_bit_cast(f32, sign | 0x7f800000u | (f ? (f << 13) | 0x400000u : 0u));
  if ( e == 0 ) {
    if ( f == 0 ) return __builtin_bit_cast(f32, sign);
    e = 113;
    while ( !(f & 1024) ) {
      f <<= 1;
      --e;
    }
    f &= 1023;
  } else
    e += 112;
  return __builtin_bit_cast(f32, sign | (e << 23) | (f << 13));
}

template<usize Mantissa, i32 Bias>
constexpr f64
__decode64(u16 value) noexcept
{
  const u64 sign = u64(value & 0x8000) << 48;
  constexpr u32 mask = (u32(1) << Mantissa) - 1, max_exp = (u32(1) << (15 - Mantissa)) - 1;
  const u32 exponent = (value >> Mantissa) & max_exp;
  u32 fraction = value & mask;
  if ( exponent == max_exp )
    return __builtin_bit_cast(f64,
                              sign | 0x7ff0000000000000ull | (fraction ? (u64(fraction) << (52 - Mantissa)) | 0x8000000000000ull : 0ull));
  i32 e = i32(exponent ? exponent : 1) - Bias;
  if ( !exponent ) {
    if ( !fraction ) return __builtin_bit_cast(f64, sign);
    while ( !(fraction & (mask + 1)) ) {
      fraction <<= 1;
      --e;
    }
    fraction &= mask;
  }
  return __builtin_bit_cast(f64, sign | (u64(e + 1023) << 52) | (u64(fraction) << (52 - Mantissa)));
}
};      // namespace __compact

// Conversion from f32 rounds to nearest, ties to even; overflow becomes signed
// infinity, subnormals survive, and NaNs become a signed canonical quiet NaN.
// Wider source values must be explicitly narrowed to f32 by the caller.
// References preserve nonfinite object bits under Clang finite-math call attributes.
struct float16 {
  u16 bits{};

  constexpr float16() noexcept = default;

  constexpr bool
  is_finite() const noexcept
  {
    return (bits & 0x7c00) != 0x7c00;
  }

  template<class F>
    requires(micron::is_floating_point_v<F> && sizeof(F) == 4)
  explicit constexpr float16(const F &value) noexcept : bits(__compact::__encode16(__builtin_bit_cast(u32, value)))
  {
  }

  static constexpr float16
  from_bits(u16 value) noexcept
  {
    float16 out;
    out.bits = value;
    return out;
  }

  constexpr f32
  to_float() const noexcept
  {
    return __compact::__decode16(bits);
  }

  constexpr f64
  to_double() const noexcept
  {
    return __compact::__decode64<10, 15>(bits);
  }
};

struct bfloat16 {
  u16 bits{};

  constexpr bfloat16() noexcept = default;

  constexpr bool
  is_finite() const noexcept
  {
    return (bits & 0x7f80) != 0x7f80;
  }

  template<class F>
    requires(micron::is_floating_point_v<F> && sizeof(F) == 4)
  explicit constexpr bfloat16(const F &value) noexcept
  {
    const u32 raw = __builtin_bit_cast(u32, value);
    bits = (raw & 0x7fffffff) > 0x7f800000 ? u16((raw >> 16 & 0x8000) | 0x7fc0) : u16((raw + 0x7fff + ((raw >> 16) & 1)) >> 16);
  }

  static constexpr bfloat16
  from_bits(u16 value) noexcept
  {
    bfloat16 out;
    out.bits = value;
    return out;
  }

  constexpr f32
  to_float() const noexcept
  {
    u32 raw = u32(bits) << 16;
    if ( (raw & 0x7fffffff) > 0x7f800000 ) raw |= 0x400000;
    return __builtin_bit_cast(f32, raw);
  }

  constexpr f64
  to_double() const noexcept
  {
    return __compact::__decode64<7, 127>(bits);
  }
};

static_assert(sizeof(float16) == 2 && alignof(float16) == alignof(u16));
static_assert(sizeof(bfloat16) == 2 && alignof(bfloat16) == alignof(u16));
};      // namespace micron::math
