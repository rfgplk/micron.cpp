//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../numerics.hpp"
#include "../../types.hpp"
#include "../bits.hpp"
#include "../ieee.hpp"

namespace micron
{
namespace math
{
namespace mkbits
{
namespace manip
{

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fabs(F x) noexcept
{
  using T = ieee::traits<F>;
  return ieee::from_bits<F>(ieee::to_bits(x) & ~T::sign_mask);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fneg(F x) noexcept
{
  using T = ieee::traits<F>;
  return ieee::from_bits<F>(ieee::to_bits(x) ^ T::sign_mask);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
copysign(F mag, F sgn) noexcept
{
  using T = ieee::traits<F>;
  auto bm = ieee::to_bits(mag) & ~T::sign_mask;
  auto bs = ieee::to_bits(sgn) & T::sign_mask;
  return ieee::from_bits<F>(bm | bs);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr bool
signbit(F x) noexcept
{
  using T = ieee::traits<F>;
  return (ieee::to_bits(x) & T::sign_mask) != 0;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fmax(F a, F b) noexcept
{
  if ( ieee::is_nan(a) ) return ieee::is_nan(b) ? a : b;
  if ( ieee::is_nan(b) ) return a;
  if ( a == 0 && b == 0 ) return signbit(a) ? b : a;
  return (a > b) ? a : b;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fmin(F a, F b) noexcept
{
  if ( ieee::is_nan(a) ) return ieee::is_nan(b) ? a : b;
  if ( ieee::is_nan(b) ) return a;
  if ( a == 0 && b == 0 ) return signbit(a) ? a : b;
  return (a < b) ? a : b;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fdim(F a, F b) noexcept
{
  if ( ieee::is_nan(a) || ieee::is_nan(b) ) return ieee::qnan_v<F>();
  return (a > b) ? F(a - b) : F(0);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
frexp(F x, int *e) noexcept
{
  using T = ieee::traits<F>;
  using U = typename T::uint_type;
  U bits = ieee::to_bits(x);
  U absb = bits & ~T::sign_mask;
  if ( absb == 0 || absb >= T::exp_mask ) {
    *e = 0;
    return x;
  }
  int unbiased = int((absb & T::exp_mask) >> T::mant_bits) - T::exp_bias;
  if ( unbiased == -T::exp_bias ) {

    constexpr int shift = T::mant_bits + 1;
    F scaled;
    if constexpr ( sizeof(F) == 4 )
      scaled = x * F(0x1.0p24f);
    else
      scaled = x * F(0x1.0p53);
    U sb = ieee::to_bits(scaled);
    int unb2 = int(((sb & ~T::sign_mask) & T::exp_mask) >> T::mant_bits) - T::exp_bias;
    *e = unb2 - shift + 1;

    U new_bits = (sb & (T::sign_mask | T::mant_mask)) | (U(T::exp_bias - 1) << T::mant_bits);
    return ieee::from_bits<F>(new_bits);
  }
  *e = unbiased + 1;

  U new_bits = (bits & (T::sign_mask | T::mant_mask)) | (U(T::exp_bias - 1) << T::mant_bits);
  return ieee::from_bits<F>(new_bits);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
ldexp(F x, int n) noexcept
{
  using T = ieee::traits<F>;
  using U = typename T::uint_type;
  if ( x == F(0) || ieee::is_nan(x) || ieee::is_inf(x) ) return x;

  if ( n > 4 * T::exp_bias ) n = 4 * T::exp_bias;
  if ( n < -4 * T::exp_bias ) n = -4 * T::exp_bias;

  int chunk = T::exp_bias - 1;
  F r = x;
  while ( n > chunk ) {
    F m = ieee::from_bits<F>(U(T::exp_bias + chunk) << T::mant_bits);
    r = r * m;
    n -= chunk;
  }
  while ( n < -chunk ) {
    F m = ieee::from_bits<F>(U(T::exp_bias - chunk) << T::mant_bits);
    r = r * m;
    n += chunk;
  }
  if ( n != 0 ) {
    F m = ieee::from_bits<F>(U(T::exp_bias + n) << T::mant_bits);
    r = r * m;
  }
  return r;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
scalbn(F x, int n) noexcept
{
  return ldexp<F>(x, n);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr int
ilogb(F x) noexcept
{
  using T = ieee::traits<F>;
  using U = typename T::uint_type;
  U bits = ieee::to_bits(x) & ~T::sign_mask;
  if ( bits == 0 ) return numeric_limits<int>::min();
  if ( bits >= T::exp_mask ) return numeric_limits<int>::max();
  int e = int((bits & T::exp_mask) >> T::mant_bits);
  if ( e == 0 ) {

    U m = bits & T::mant_mask;
    int lz = 0;
    while ( (m & T::implicit_one) == 0 ) {
      m <<= 1;
      ++lz;
    }
    return -T::exp_bias - lz;
  }
  return e - T::exp_bias;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
logb(F x) noexcept
{
  if ( x == F(0) ) return ieee::inf_v<F>(1);
  if ( ieee::is_inf(x) ) return ieee::inf_v<F>(0);
  if ( ieee::is_nan(x) ) return ieee::qnan_v<F>();
  return F(ilogb<F>(x));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
nextafter(F a, F b) noexcept
{
  if ( ieee::is_nan(a) || ieee::is_nan(b) ) return ieee::qnan_v<F>();
  if ( a == b ) return b;
  if ( a == F(0) ) {
    using T = ieee::traits<F>;
    using U = typename T::uint_type;
    return ieee::from_bits<F>((b > 0 ? U(0) : T::sign_mask) | U(1));
  }
  return (b > a) == (a > 0) ? ieee::next_up<F>(a) : ieee::next_down<F>(a);
}

};     // namespace manip
};     // namespace mkbits
};     // namespace math
};     // namespace micron
