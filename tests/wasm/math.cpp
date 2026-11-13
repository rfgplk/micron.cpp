// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/wasm/math/special.hpp"
#include "../../src/math/mk.hpp"
#include "../../src/simd/simd.hpp"
namespace w = micron::wasm::math;
using U = unsigned __int128;
static U operands[4]{};
static_assert(micron::math::fma(0x1.0000000000000000000000000001p0L, 0x1.fffffffffffffffffffffffffffep-1L, -1.L) == -0x1p-224L);
static_assert(micron::math::fma(0x1.000002p0f, 0x1.fffffcp-1f, -1.f) == -0x1p-46f);
static_assert(micron::math::fma(0x1.0000000000001p0, 0x1.ffffffffffffep-1, -1.) == -0x1p-104);

extern "C" __attribute__((export_name("math_buffer"))) U *
math_buffer()
{
  return operands;
}

extern "C" __attribute__((export_name("math_probe"))) void
math_probe(unsigned op)
{
  const long double a = w::from<long double>(operands[0]), b = w::from<long double>(operands[1]);
  long double r;
  switch ( op ) {
  case 0:
    r = w::exp(a);
    break;
  case 1:
    r = w::exp2(a);
    break;
  case 2:
    r = w::expm1(a);
    break;
  case 3:
    r = w::log(a);
    break;
  case 4:
    r = w::log2(a);
    break;
  case 5:
    r = w::log1p(a);
    break;
  case 6:
    r = w::sin(a);
    break;
  case 7:
    r = w::cos(a);
    break;
  case 8:
    r = w::tan(a);
    break;
  case 9:
    r = w::atan(a);
    break;
  case 10:
    r = w::asin(a);
    break;
  case 11:
    r = w::acos(a);
    break;
  case 12:
    r = w::sinh(a);
    break;
  case 13:
    r = w::cosh(a);
    break;
  case 14:
    r = w::tanh(a);
    break;
  case 15:
    r = w::asinh(a);
    break;
  case 16:
    r = w::acosh(a);
    break;
  case 17:
    r = w::atanh(a);
    break;
  case 18:
    r = w::cbrt(a);
    break;
  case 19:
    r = w::pow(a, b);
    break;
  case 20:
    r = w::hypot(a, b);
    break;
  case 21:
    r = w::atan2(a, b);
    break;
  case 22:
    r = w::erf(a);
    break;
  case 23:
    r = w::erfc(a);
    break;
  case 24:
    r = w::tgamma(a);
    break;
  case 25:
    r = w::lgamma(a);
    break;
  case 26:
    r = w::j0(a);
    break;
  case 27:
    r = w::j1(a);
    break;
  case 28:
    r = w::y0(a);
    break;
  case 29:
    r = w::y1(a);
    break;
  default:
    r = a;
    break;
  }
  operands[2] = w::bits(r);
}

int
main()
{
  if ( micron::math::fsqrt(0x1p10000L) != 0x1p5000L ) return 6;
  if ( micron::math::mkbits::round_ns::trunc(0x1.0000000000000000000000000001p60L) != 0x1p60L ) return 6;
  const auto large = micron::math::mkbits::exp_ns::exp(10000.L);
  if ( !(large > 0x1p10000L) || w::isinf(large) ) return 6;
  if ( micron::math::logf128(0x1p10000L) < 6931.L ) return 6;
  if ( micron::math::mk::trig::sin(0x1p-1000L) != 0x1p-1000L ) return 6;
  if ( micron::math::mk::exp_ns::exp(10000.L) != large ) return 6;
  micron::simd::zdouble v(4.0);
  auto roots = micron::math::mk::sqrt(micron::simd::d512(v));
  if ( micron::simd::zdouble(roots)[7] != 2.0 ) return 6;
  if ( w::exp2(-16494.L) != w::from<long double>(1) ) return 6;
  return 1;
}
