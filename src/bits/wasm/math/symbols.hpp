// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../math/bits/hyp.hpp"
#include "../../../math/bits/special.hpp"
#include "../../../math/bits/sqrt.hpp"
#include "../../../math/bits/trig.hpp"
#include "../../../math/generic.hpp"
#include "conversions.hpp"
#include "special.hpp"

#define __MC_WASM_LIBM1(NAME, NS)                                                                                                          \
  extern "C" __attribute__((weak, used)) float NAME##f(float x) noexcept { return micron::math::mkbits::NS::NAME<float>(x); }              \
  extern "C" __attribute__((weak, used)) double NAME(double x) noexcept { return micron::math::mkbits::NS::NAME<double>(x); }              \
  extern "C" __attribute__((weak, used)) long double NAME##l(long double x) noexcept { return micron::wasm::math::NAME(x); }
__MC_WASM_LIBM1(exp, exp_ns)
__MC_WASM_LIBM1(exp2, exp_ns)
__MC_WASM_LIBM1(exp10, exp_ns)
__MC_WASM_LIBM1(expm1, exp_ns)
__MC_WASM_LIBM1(log, log_ns)
__MC_WASM_LIBM1(log2, log_ns)
__MC_WASM_LIBM1(log10, log_ns)
__MC_WASM_LIBM1(log1p, log_ns)
__MC_WASM_LIBM1(sin, trig_ns)
__MC_WASM_LIBM1(cos, trig_ns)
__MC_WASM_LIBM1(tan, trig_ns)
__MC_WASM_LIBM1(asin, trig_ns)
__MC_WASM_LIBM1(acos, trig_ns)
__MC_WASM_LIBM1(atan, trig_ns)
__MC_WASM_LIBM1(sinh, hyp_ns)
__MC_WASM_LIBM1(cosh, hyp_ns)
__MC_WASM_LIBM1(tanh, hyp_ns)
__MC_WASM_LIBM1(asinh, hyp_ns)
__MC_WASM_LIBM1(acosh, hyp_ns)
__MC_WASM_LIBM1(atanh, hyp_ns)
__MC_WASM_LIBM1(cbrt, sqrt_ns)
__MC_WASM_LIBM1(erf, special_ns)
__MC_WASM_LIBM1(erfc, special_ns)
__MC_WASM_LIBM1(tgamma, special_ns)
__MC_WASM_LIBM1(lgamma, special_ns)
__MC_WASM_LIBM1(j0, special_ns)
__MC_WASM_LIBM1(j1, special_ns)
__MC_WASM_LIBM1(y0, special_ns)
__MC_WASM_LIBM1(y1, special_ns)
#undef __MC_WASM_LIBM1
#define __MC_WASM_LIBM2(NAME, NS)                                                                                                          \
  extern "C" __attribute__((weak, used)) float NAME##f(float x, float y) noexcept { return micron::math::mkbits::NS::NAME<float>(x, y); }  \
  extern "C" __attribute__((weak, used)) double NAME(double x, double y) noexcept { return micron::math::mkbits::NS::NAME<double>(x, y); } \
  extern "C" __attribute__((weak, used)) long double NAME##l(long double x, long double y) noexcept                                        \
  {                                                                                                                                        \
    return micron::wasm::math::NAME(x, y);                                                                                                 \
  }
__MC_WASM_LIBM2(pow, pow_ns)
__MC_WASM_LIBM2(hypot, sqrt_ns)
__MC_WASM_LIBM2(atan2, trig_ns)
__MC_WASM_LIBM2(fmod, rem)
__MC_WASM_LIBM2(remainder, rem)
#undef __MC_WASM_LIBM2
#define __MC_WASM_SIMPLE(NAME, CALL)                                                                                                       \
  extern "C" __attribute__((weak, used)) float NAME##f(float x) noexcept { return micron::wasm::math::CALL(x); }                           \
  extern "C" __attribute__((weak, used)) double NAME(double x) noexcept { return micron::wasm::math::CALL(x); }                            \
  extern "C" __attribute__((weak, used)) long double NAME##l(long double x) noexcept { return micron::wasm::math::CALL(x); }
__MC_WASM_SIMPLE(trunc, trunc)
__MC_WASM_SIMPLE(floor, floor)
__MC_WASM_SIMPLE(ceil, ceil)
__MC_WASM_SIMPLE(round, round)
__MC_WASM_SIMPLE(rint, nearest)
__MC_WASM_SIMPLE(nearbyint, nearest)
__MC_WASM_SIMPLE(fabs, abs)
#undef __MC_WASM_SIMPLE

extern "C" __attribute__((weak, used)) float
sqrtf(float x) noexcept
{
  return __builtin_sqrtf(x);
}

extern "C" __attribute__((weak, used)) double
sqrt(double x) noexcept
{
  return __builtin_sqrt(x);
}

extern "C" __attribute__((weak, used)) long double
sqrtl(long double x) noexcept
{
  return micron::wasm::math::sqrt(x);
}

#define __MC_WASM_TYPED(SUFFIX, F)                                                                                                         \
  extern "C" __attribute__((weak, used)) F fma##SUFFIX(F a, F b, F c) noexcept { return micron::wasm::math::fma(a, b, c); }                \
  extern "C" __attribute__((weak, used)) F copysign##SUFFIX(F a, F b) noexcept { return micron::wasm::math::copysign(a, b); }              \
  extern "C" __attribute__((weak, used)) F ldexp##SUFFIX(F a, int e) noexcept { return micron::wasm::math::scalbn(a, e); }                 \
  extern "C" __attribute__((weak, used)) F scalbn##SUFFIX(F a, int e) noexcept { return micron::wasm::math::scalbn(a, e); }                \
  extern "C" __attribute__((weak, used)) F frexp##SUFFIX(F a, int *e) noexcept { return micron::wasm::math::frexp(a, e); }                 \
  extern "C" __attribute__((weak, used)) F modf##SUFFIX(F a, F *i) noexcept                                                                \
  {                                                                                                                                        \
    *i = micron::wasm::math::trunc(a);                                                                                                     \
    return micron::wasm::math::isinf(a) ? micron::wasm::math::copysign(static_cast<F>(0), a) : micron::wasm::math::copysign(a - *i, a);    \
  }                                                                                                                                        \
  extern "C" __attribute__((weak, used)) F logb##SUFFIX(F a) noexcept { return micron::math::mkbits::manip::logb<F>(a); }                  \
  extern "C" __attribute__((weak, used)) int ilogb##SUFFIX(F a) noexcept { return micron::math::mkbits::manip::ilogb<F>(a); }              \
  extern "C" __attribute__((weak, used)) F nextafter##SUFFIX(F a, F b) noexcept                                                            \
  {                                                                                                                                        \
    return micron::math::mkbits::manip::nextafter<F>(a, b);                                                                                \
  }                                                                                                                                        \
  extern "C" __attribute__((weak, used)) F fmin##SUFFIX(F a, F b) noexcept { return micron::math::mkbits::manip::fmin<F>(a, b); }          \
  extern "C" __attribute__((weak, used)) F fmax##SUFFIX(F a, F b) noexcept { return micron::math::mkbits::manip::fmax<F>(a, b); }          \
  extern "C" __attribute__((weak, used)) F fdim##SUFFIX(F a, F b) noexcept { return micron::math::mkbits::manip::fdim<F>(a, b); }
__MC_WASM_TYPED(f, float)
__MC_WASM_TYPED(, double)
__MC_WASM_TYPED(l, long double)
#undef __MC_WASM_TYPED
