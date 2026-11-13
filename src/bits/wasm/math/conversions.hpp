// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "real.hpp"

namespace micron::wasm::math
{
template<typename F>
inline F
integer_float(wide magnitude, bool negative) noexcept
{
  using T = format<F>;
  if ( !magnitude ) return F(0);
  const int leading = msb(magnitude), shift = leading - int(T::mantissa) - 3;
  const wide extended = shift > 0 ? jam(magnitude, unsigned(shift)) : magnitude << -shift;
  return pack<F>(negative ? T::sign : 0, leading + T::bias, extended);
}

template<typename F, bool Signed>
inline wide
float_integer(F x) noexcept
{
  using T = format<F>;
  const bool negative = (bits(x) & T::sign) != 0;
  const wide maximum = Signed ? (wide(1) << 127) - !negative : ~wide(0);
  if constexpr ( !Signed )
    if ( negative ) return 0;
  if ( isnan(x) || isinf(x) ) return negative && Signed ? wide(0) - maximum : maximum;
  const auto p = decompose(x);
  const int leading = msb(p.significand) + p.exponent;
  if ( leading < 0 ) return 0;
  if ( leading >= (Signed ? 127 : 128) ) return negative && Signed ? wide(0) - maximum : maximum;
  const wide value = p.exponent < 0 ? p.significand >> -p.exponent : p.significand << p.exponent;
  return negative ? wide(0) - value : value;
}
}      // namespace micron::wasm::math

#define __MC_WASM_INT_FLOAT(SUFFIX, F)                                                                                                     \
  extern "C" __attribute__((weak, used)) F __floatti##SUFFIX(__int128 x) noexcept                                                          \
  {                                                                                                                                        \
    const auto u = static_cast<unsigned __int128>(x);                                                                                      \
    return micron::wasm::math::integer_float<F>(x < 0 ? 0 - u : u, x < 0);                                                                 \
  }                                                                                                                                        \
  extern "C" __attribute__((weak, used)) F __floatunti##SUFFIX(unsigned __int128 x) noexcept                                               \
  {                                                                                                                                        \
    return micron::wasm::math::integer_float<F>(x, false);                                                                                 \
  }                                                                                                                                        \
  extern "C" __attribute__((weak, used)) __int128 __fix##SUFFIX##ti(F x) noexcept                                                          \
  {                                                                                                                                        \
    return static_cast<__int128>(micron::wasm::math::float_integer<F, true>(x));                                                           \
  }                                                                                                                                        \
  extern "C" __attribute__((weak, used)) unsigned __int128 __fixuns##SUFFIX##ti(F x) noexcept                                              \
  {                                                                                                                                        \
    return micron::wasm::math::float_integer<F, false>(x);                                                                                 \
  }
__MC_WASM_INT_FLOAT(sf, float)
__MC_WASM_INT_FLOAT(df, double)
__MC_WASM_INT_FLOAT(tf, long double)
#undef __MC_WASM_INT_FLOAT
