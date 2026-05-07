//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// primary ieee math file

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "bits.hpp"

namespace micron
{
namespace math
{

template <typename T>
concept ieee754_floating = micron::is_floating_point_v<T>;

namespace ieee
{

template <usize Sz> struct traits_n {
};

template <> struct traits_n<4> {
  using uint_type = u32;
  static constexpr int bits = 32;
  static constexpr int mant_bits = 23;
  static constexpr int exp_bits = 8;
  static constexpr int exp_bias = 127;
  static constexpr u32 sign_mask = 0x80000000u;
  static constexpr u32 exp_mask = 0x7f800000u;
  static constexpr u32 mant_mask = 0x007fffffu;
  static constexpr u32 implicit_one = 0x00800000u;
  static constexpr u32 quiet_nan_bit = 0x00400000u;
};

template <> struct traits_n<8> {
  using uint_type = u64;
  static constexpr int bits = 64;
  static constexpr int mant_bits = 52;
  static constexpr int exp_bits = 11;
  static constexpr int exp_bias = 1023;
  static constexpr u64 sign_mask = 0x8000000000000000ULL;
  static constexpr u64 exp_mask = 0x7ff0000000000000ULL;
  static constexpr u64 mant_mask = 0x000fffffffffffffULL;
  static constexpr u64 implicit_one = 0x0010000000000000ULL;
  static constexpr u64 quiet_nan_bit = 0x0008000000000000ULL;
};

template <typename F> struct traits : traits_n<sizeof(F)> {
};

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr auto
to_bits(F x) noexcept
{
  using U = typename traits<F>::uint_type;
  return bits::bit_cast<U>(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
from_bits(typename traits<F>::uint_type u) noexcept
{
  return bits::bit_cast<F>(u);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr int
exponent_raw(F x) noexcept
{
  using T = traits<F>;
  return int((to_bits(x) & T::exp_mask) >> T::mant_bits);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr int
exponent_of(F x) noexcept
{
  using T = traits<F>;
  return exponent_raw(x) - T::exp_bias;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr auto
mantissa_of(F x) noexcept
{
  using T = traits<F>;
  return to_bits(x) & T::mant_mask;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr int
sign_of(F x) noexcept
{
  using T = traits<F>;
  return int((to_bits(x) & T::sign_mask) >> (T::bits - 1));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
pack(int sign, int unbiased_exp, typename traits<F>::uint_type mantissa) noexcept
{
  using T = traits<F>;
  using U = typename T::uint_type;
  U s = U(sign & 1) << (T::bits - 1);
  U e = U((unbiased_exp + T::exp_bias) & ((1 << T::exp_bits) - 1)) << T::mant_bits;
  U m = mantissa & T::mant_mask;
  return from_bits<F>(s | e | m);
}

// NOTE: these __builtins are spliced in place by the compiler, they don't invoke libm (or at least *sholdn't*)

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_nan(F x) noexcept
{
  return __builtin_isnan(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_inf(F x) noexcept
{
  return __builtin_isinf_sign(x) != 0;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_finite(F x) noexcept
{
  return __builtin_isfinite(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_normal(F x) noexcept
{
  return __builtin_isnormal(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_subnormal(F x) noexcept
{
  using T = traits<F>;
  auto u = to_bits(x) & ~T::sign_mask;
  return u != 0 && (u & T::exp_mask) == 0;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_zero(F x) noexcept
{
  using T = traits<F>;
  return (to_bits(x) & ~T::sign_mask) == 0;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_special(F x) noexcept
{
  using T = traits<F>;
  auto u = to_bits(x) & ~T::sign_mask;
  return u == 0 || (u & T::exp_mask) == T::exp_mask || (u & T::exp_mask) == 0;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
inf_v(int sign = 0) noexcept
{
  using T = traits<F>;
  using U = typename T::uint_type;
  U s = U(sign & 1) << (T::bits - 1);
  return from_bits<F>(s | T::exp_mask);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
qnan_v(typename traits<F>::uint_type payload = 1) noexcept
{
  using T = traits<F>;
  return from_bits<F>(T::exp_mask | T::quiet_nan_bit | (payload & (T::mant_mask >> 1)));
}

// mainly helpers
template <ieee754_floating F>
[[nodiscard]] inline constexpr i64
ulp_distance(F a, F b) noexcept
{
  using T = traits<F>;
  using U = typename T::uint_type;
  if ( a == b ) return 0;
  if ( __builtin_isnan(a) || __builtin_isnan(b) ) return -1;
  auto ord = [](F v) -> i64 {
    U u = to_bits(v);
    if ( u & T::sign_mask ) return -i64(u & ~T::sign_mask);
    return i64(u);
  };
  i64 oa = ord(a);
  i64 ob = ord(b);
  return oa > ob ? (oa - ob) : (ob - oa);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
next_up(F x) noexcept
{
  using T = traits<F>;
  using U = typename T::uint_type;
  if ( __builtin_isnan(x) || (__builtin_isinf_sign(x) > 0) ) return x;
  U u = to_bits(x);
  if ( u == T::sign_mask ) return F(0);     // -0 → +0
  if ( u & T::sign_mask ) return from_bits<F>(u - 1);
  return from_bits<F>(u + 1);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
next_down(F x) noexcept
{
  using T = traits<F>;
  using U = typename T::uint_type;
  if ( __builtin_isnan(x) || (__builtin_isinf_sign(x) < 0) ) return x;
  U u = to_bits(x);
  if ( u == 0 ) return from_bits<F>(T::sign_mask | U(1));     // +0 → -smallest
  if ( u & T::sign_mask ) return from_bits<F>(u + 1);
  return from_bits<F>(u - 1);
}

};     // namespace ieee
};     // namespace math
};     // namespace micron
