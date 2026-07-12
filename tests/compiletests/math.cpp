//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: micron::math porcelain compiles on every arch/opt.
// Uses only arch-portable scalar entry points (no x86-only SIMD). Not run.

#include "../../src/math.hpp"

int
main()
{
  double d = 2.0;
  d = micron::math::sqrt(d);
  d += micron::math::fsqrt(9.0);
  d += static_cast<double>(micron::math::logf64(8.0));
  d += static_cast<double>(micron::math::expf64(1.0));
  d += static_cast<double>(micron::math::powerf(2.0, 10.0));
  d += static_cast<double>(micron::math::cr::sin_f64(0.5));
  d += static_cast<double>(micron::math::cr::cos_f64(0.5));
  d += micron::math::ceil(d);
  d += micron::math::floor(d);

  float f = micron::math::hw::sqrt_ss(4.0f);
  f += micron::math::fsqrt(2.0f);

  return static_cast<int>(d + f) & 0x7f;
}
