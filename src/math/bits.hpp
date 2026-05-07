//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{
namespace math
{
namespace bits
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rotates

template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr T
rol(T x, int r) noexcept
{
  constexpr int w = sizeof(T) * 8;
  const int s = r & (w - 1);
  if ( s == 0 ) return x;
  return static_cast<T>((x << s) | (x >> (w - s)));
}

template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr T
ror(T x, int r) noexcept
{
  constexpr int w = sizeof(T) * 8;
  const int s = r & (w - 1);
  if ( s == 0 ) return x;
  return static_cast<T>((x >> s) | (x << (w - s)));
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
rol32(u32 x, int r) noexcept
{
  const int s = r & 31;
  if ( s == 0 ) return x;
  return (x << s) | (x >> (32 - s));
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
ror32(u32 x, int r) noexcept
{
  const int s = r & 31;
  if ( s == 0 ) return x;
  return (x >> s) | (x << (32 - s));
}

[[nodiscard, gnu::always_inline]] inline constexpr u64
rol64(u64 x, int r) noexcept
{
  const int s = r & 63;
  if ( s == 0 ) return x;
  return (x << s) | (x >> (64 - s));
}

[[nodiscard, gnu::always_inline]] inline constexpr u64
ror64(u64 x, int r) noexcept
{
  const int s = r & 63;
  if ( s == 0 ) return x;
  return (x >> s) | (x << (64 - s));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// counts

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr int
clz(T x) noexcept
{
  return micron::countl_zero(x);
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr int
ctz(T x) noexcept
{
  return micron::countr_zero(x);
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr int
popcount(T x) noexcept
{
  return micron::popcount(x);
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr int
parity(T x) noexcept
{
  return micron::parity(x);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// abs
[[nodiscard, gnu::always_inline]] inline constexpr i64
abs64(i64 x) noexcept
{
  const i64 mask = x >> 63;
  return (x ^ mask) - mask;
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
abs32(i32 x) noexcept
{
  const i32 mask = x >> 31;
  return (x ^ mask) - mask;
}

template <typename F>
  requires(micron::is_floating_point_v<F>)
[[nodiscard, gnu::always_inline]] inline constexpr int
sign_bit(F x) noexcept
{
  return __builtin_signbit(x);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bit casts

template <typename To, typename From>
[[nodiscard, gnu::always_inline]] inline constexpr To
bit_cast(const From &from) noexcept
{
  static_assert(sizeof(To) == sizeof(From), "math::bits::bit_cast size mismatch");
  static_assert(micron::is_trivially_copyable_v<To>, "bit_cast destination must be trivially copyable");
  static_assert(micron::is_trivially_copyable_v<From>, "bit_cast source must be trivially copyable");
  return __builtin_bit_cast(To, from);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// byteswaps

[[nodiscard, gnu::always_inline]] inline constexpr u16
byteswap16(u16 x) noexcept
{
  return __builtin_bswap16(x);
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
byteswap32(u32 x) noexcept
{
  return __builtin_bswap32(x);
}

[[nodiscard, gnu::always_inline]] inline constexpr u64
byteswap64(u64 x) noexcept
{
  return __builtin_bswap64(x);
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr T
byteswap(T x) noexcept
{
  if constexpr ( sizeof(T) == 1 )
    return x;
  else if constexpr ( sizeof(T) == 2 )
    return static_cast<T>(__builtin_bswap16(static_cast<u16>(x)));
  else if constexpr ( sizeof(T) == 4 )
    return static_cast<T>(__builtin_bswap32(static_cast<u32>(x)));
  else if constexpr ( sizeof(T) == 8 )
    return static_cast<T>(__builtin_bswap64(static_cast<u64>(x)));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// floors ceils
template <typename T>
  requires(micron::is_integral_v<T> && micron::is_unsigned_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr T
floor_pow2(T x) noexcept
{
  if ( x == 0 ) return 0;
  return T(1) << (sizeof(T) * 8 - 1 - clz(x));
}

template <typename T>
  requires(micron::is_integral_v<T> && micron::is_unsigned_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr T
ceil_pow2(T x) noexcept
{
  if ( x <= 1 ) return 1;
  return T(1) << (sizeof(T) * 8 - clz(static_cast<T>(x - 1)));
}

};     // namespace bits
};     // namespace math
};     // namespace micron
