//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// primary arithmetic file; currently supports the following modes
//  checked    = returns checked_result<T>{T value; bool overflow}
//  saturating = clamps to T_min / T_max
//  wrapping   = wraps modulo 2^N
//  widening   = promotes to 2x-width type
//  carrying   = returns (result, carry_out)

#include "../concepts.hpp"
#include "../numerics.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{
namespace math
{
namespace arith
{

template<typename T> struct checked_result {
  T value;
  bool overflow;

  [[nodiscard, gnu::always_inline]] constexpr explicit
  operator bool() const noexcept
  {
    return !overflow;
  }
};

static_assert(sizeof(checked_result<u64>) <= 16, "checked_result<u64> must fit two registers");
static_assert(sizeof(checked_result<i64>) <= 16, "checked_result<i64> must fit two registers");

template<typename T>
concept arith_integral = micron::is_integral_v<T> && !micron::is_same_v<T, bool>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// checked
namespace checked
{
template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr checked_result<T>
add(T a, T b) noexcept
{
  checked_result<T> r{};
  r.overflow = __builtin_add_overflow(a, b, &r.value);
  return r;
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr checked_result<T>
sub(T a, T b) noexcept
{
  checked_result<T> r{};
  r.overflow = __builtin_sub_overflow(a, b, &r.value);
  return r;
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr checked_result<T>
mul(T a, T b) noexcept
{
  checked_result<T> r{};
  r.overflow = __builtin_mul_overflow(a, b, &r.value);
  return r;
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr checked_result<T>
neg(T a) noexcept
{
  // overflow iff a == T_min
  if constexpr ( micron::is_signed_v<T> ) {
    checked_result<T> r{};
    r.overflow = (a == numeric_limits<T>::min());
    r.value = r.overflow ? a : T(-a);
    return r;
  } else {
    return { T(0 - a), a != 0 };
  }
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr checked_result<T>
div(T a, T b) noexcept
{
  checked_result<T> r{};
  if constexpr ( micron::is_signed_v<T> ) {
    if ( b == T(-1) && a == numeric_limits<T>::min() ) {
      r.overflow = true;
      r.value = a;
      return r;
    }
  }
  r.overflow = false;
  r.value = a / b;
  return r;
}

};      // namespace checked

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// saturating

namespace saturating
{

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
add(T a, T b) noexcept
{
  auto r = checked::add(a, b);
  if ( !r.overflow ) return r.value;
  if constexpr ( micron::is_signed_v<T> )
    return (a < T{ 0 }) ? numeric_limits<T>::min() : numeric_limits<T>::max();
  else
    return numeric_limits<T>::max();
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
sub(T a, T b) noexcept
{
  auto r = checked::sub(a, b);
  if ( !r.overflow ) return r.value;
  if constexpr ( micron::is_signed_v<T> )
    return (a < T{ 0 }) ? numeric_limits<T>::min() : numeric_limits<T>::max();
  else
    return T{ 0 };
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
mul(T a, T b) noexcept
{
  auto r = checked::mul(a, b);
  if ( !r.overflow ) return r.value;
  if constexpr ( micron::is_signed_v<T> )
    return ((a < T{ 0 }) ^ (b < T{ 0 })) ? numeric_limits<T>::min() : numeric_limits<T>::max();
  else
    return numeric_limits<T>::max();
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
neg(T a) noexcept
{
  auto r = checked::neg(a);
  if ( !r.overflow ) return r.value;
  return numeric_limits<T>::max();
}

};      // namespace saturating

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wrapping
namespace wrapping
{

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
add(T a, T b) noexcept
{
  using U = micron::make_unsigned_t<T>;
  return T(U(a) + U(b));
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
sub(T a, T b) noexcept
{
  using U = micron::make_unsigned_t<T>;
  return T(U(a) - U(b));
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
mul(T a, T b) noexcept
{
  using U = micron::make_unsigned_t<T>;
  return T(U(a) * U(b));
}

template<arith_integral T>
[[nodiscard, gnu::always_inline]] inline constexpr T
neg(T a) noexcept
{
  using U = micron::make_unsigned_t<T>;
  return T(U(0) - U(a));
}

};      // namespace wrapping

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// widening

template<typename T> struct widen {
  using type = T;
};

template<> struct widen<i8> {
  using type = i16;
};

template<> struct widen<u8> {
  using type = u16;
};

template<> struct widen<i16> {
  using type = i32;
};

template<> struct widen<u16> {
  using type = u32;
};

template<> struct widen<i32> {
  using type = i64;
};

template<> struct widen<u32> {
  using type = u64;
};

// we currently don't support extended types for non 64-bit platforms, struct __int128 is too tricky of a workaround
#if defined(__micron_arch_width_64)
template<> struct widen<i64> {
  using type = i128;
};

template<> struct widen<u64> {
  using type = u128;
};
#endif

template<typename T> using widen_t = typename widen<T>::type;

namespace widening
{

template<arith_integral T>
  requires(!micron::is_same_v<widen_t<T>, T>)
[[nodiscard, gnu::always_inline]] inline constexpr widen_t<T>
add(T a, T b) noexcept
{
  return widen_t<T>(a) + widen_t<T>(b);
}

template<arith_integral T>
  requires(!micron::is_same_v<widen_t<T>, T>)
[[nodiscard, gnu::always_inline]] inline constexpr widen_t<T>
sub(T a, T b) noexcept
{
  return widen_t<T>(a) - widen_t<T>(b);
}

template<arith_integral T>
  requires(!micron::is_same_v<widen_t<T>, T>)
[[nodiscard, gnu::always_inline]] inline constexpr widen_t<T>
mul(T a, T b) noexcept
{
  return widen_t<T>(a) * widen_t<T>(b);
}

};      // namespace widening

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// carrying
namespace carrying
{

template<typename T>
  requires(micron::is_integral_v<T> && micron::is_unsigned_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr micron::tuple<T, T>
adc(T a, T b, T carry_in) noexcept
{
  auto s0 = checked::add<T>(a, b);
  auto s1 = checked::add<T>(s0.value, carry_in);
  return micron::make_tuple(s1.value, T(T(s0.overflow) | T(s1.overflow)));
}

template<typename T>
  requires(micron::is_integral_v<T> && micron::is_unsigned_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr micron::tuple<T, T>
sbb(T a, T b, T borrow_in) noexcept
{
  auto s0 = checked::sub<T>(a, b);
  auto s1 = checked::sub<T>(s0.value, borrow_in);
  return micron::make_tuple(s1.value, T(T(s0.overflow) | T(s1.overflow)));
}

[[nodiscard, gnu::always_inline]] inline constexpr micron::tuple<u64, u64>
mul64(u64 a, u64 b) noexcept
{
#if defined(__micron_arch_width_64) && defined(__SIZEOF_INT128__)
  __uint128_t p = static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b);
  return micron::make_tuple(u64(p), u64(p >> 64));
#else
  const u64 a_lo = u32(a), a_hi = a >> 32;
  const u64 b_lo = u32(b), b_hi = b >> 32;
  const u64 p0 = a_lo * b_lo;
  const u64 p1 = a_lo * b_hi;
  const u64 p2 = a_hi * b_lo;
  const u64 p3 = a_hi * b_hi;
  const u64 mid = (p0 >> 32) + u32(p1) + u32(p2);
  u64 lo = (p0 & 0xffffffffULL) | (mid << 32);
  u64 hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
  return micron::make_tuple(lo, hi);
#endif
}

};      // namespace carrying

};      // namespace arith
};      // namespace math
};      // namespace micron
