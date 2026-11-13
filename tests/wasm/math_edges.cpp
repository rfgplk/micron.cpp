// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/math/special.hpp"
#include "../../src/math/generic.hpp"

namespace w = micron::wasm::math;

int
main()
{
  constexpr int exponents[]{ -16493, -16382, -1, 0, 100, 8192, 16382, 16383 }, signs[]{ -1, 1 };
  for ( int exponent : exponents ) {
    volatile long double unit = w::scalbn(1.L, exponent);
    for ( int sx : signs ) {
      for ( int sy : signs ) {
        const long double x = sx * (unit + unit / 2), y = sy * unit;
        const auto r = micron::math::remainderf128(x, y);
        if ( w::bits(r) != w::bits(-sx * (unit / 2)) ) return 6;
      }
    }
  }
  volatile long double maximum = w::from<long double>(w::format<long double>::infinity - 1);
  if ( w::lgamma(maximum) != w::inf || w::tgamma(maximum) != w::inf ) return 6;
  if ( w::lgamma(w::inf) != w::inf || w::tgamma(w::inf) != w::inf ) return 6;
  if ( w::lgamma(1.L) != 0 || w::lgamma(2.L) != 0 ) return 6;
  // Here x*log(x) exceeds maxfinite, but lgamma(x) remains finite.
  volatile long double below = maximum / 11347.L;
  const auto value = w::lgamma(below);
  if ( !(value > maximum * 0.99L && value < maximum) ) return 6;
  if ( !w::isnan(w::lgamma(w::nan<long double>())) ) return 6;
  volatile long double tiny = w::from<long double>(1);
  if ( w::y1(tiny) != -w::inf ) return 6;
  if ( !(w::y0(tiny) < -7000.L && w::y0(tiny) > -8000.L) ) return 6;
  if ( w::lgamma(-tiny) != w::lgamma(tiny) || w::tgamma(-tiny) != -w::inf ) return 6;
  return 1;
}
