//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// FREESTANDING LIBM SYMBOLS
//
// WARNING: THESE AREN'T FOR EXTERNAL USE, THE COMPILER EMITS CALLS TO THESE IF USING __BUILTIN_* QUALIFIED FNS FOR ERRNO CALLBACK OR IF NO
// HARDWARE FORM EXISTS ALL DEFINITIONS ARE WEAK SO WE DON'T COLLIDE WITH REAL LIBM OR TU INCLUDE SPAGHETTI; THIS SHOULD EXCLUSIVELY BE
// INCLUDED IN START.CPP
//
// NEVER USE THIS IN REGULAR CODE, PREFER USING REGULAR MICRON MATH FNS

#include "../types.hpp"

#include "__asm/hw.hpp"
#include "cr.hpp"
#include "generic.hpp"

#include "bits/exp.hpp"
#include "bits/hyp.hpp"
#include "bits/log.hpp"
#include "bits/manip.hpp"
#include "bits/special.hpp"

#pragma GCC diagnostic push
// we are deliberately (re)defining functions the compiler knows as builtins
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"

#define __MC_M1(NAME, DBODY, FBODY, LBODY)                                                                                                 \
  extern "C" __attribute__((weak)) double NAME(double x) noexcept { return (DBODY); }                                                      \
  extern "C" __attribute__((weak)) float NAME##f(float x) noexcept { return (FBODY); }                                                     \
  extern "C" __attribute__((weak)) long double NAME##l(long double x) noexcept { return (LBODY); }

#define __MC_M2(NAME, DBODY, FBODY, LBODY)                                                                                                 \
  extern "C" __attribute__((weak)) double NAME(double x, double y) noexcept { return (DBODY); }                                            \
  extern "C" __attribute__((weak)) float NAME##f(float x, float y) noexcept { return (FBODY); }                                            \
  extern "C" __attribute__((weak)) long double NAME##l(long double x, long double y) noexcept { return (LBODY); }

__MC_M1(sqrt, micron::math::hw::sqrt_sd(x), micron::math::hw::sqrt_ss(x),
        static_cast<long double>(micron::math::hw::sqrt_sd(static_cast<double>(x))))

// __builtin_fabs is a pure bit-op on every arch
__MC_M1(cbrt, __builtin_copysign(micron::math::powerf(__builtin_fabs(x), 1.0 / 3.0), x),
        __builtin_copysignf(micron::math::powerf32(__builtin_fabsf(x), static_cast<float>(1.0 / 3.0)), x),
        static_cast<long double>(__builtin_copysign(micron::math::powerf(__builtin_fabs(static_cast<double>(x)), 1.0 / 3.0),
                                                    static_cast<double>(x))))

__MC_M1(exp, micron::math::expf64(x), micron::math::expf32(x), micron::math::expf128(x))
__MC_M1(exp2, micron::math::powerf(2.0, x), micron::math::powerf32(2.0f, x), micron::math::powerflong(2.0L, x))

__MC_M1(log, micron::math::logf64(x), micron::math::logf32(x), micron::math::logf128(x))
__MC_M1(log2, micron::math::flog2(x), micron::math::flog2(x), micron::math::flog2(x))
__MC_M1(log10, micron::math::log10f64(x), micron::math::log10f32(x), micron::math::log10f128(x))

__MC_M1(sin, micron::math::cr::sin_f64(x), micron::math::cr::sin_f32(x),
        static_cast<long double>(micron::math::cr::sin_f64(static_cast<double>(x))))
__MC_M1(cos, micron::math::cr::cos_f64(x), micron::math::cr::cos_f32(x),
        static_cast<long double>(micron::math::cr::cos_f64(static_cast<double>(x))))
__MC_M1(tan, micron::math::cr::sin_f64(x) / micron::math::cr::cos_f64(x), micron::math::cr::sin_f32(x) / micron::math::cr::cos_f32(x),
        static_cast<long double>(micron::math::cr::sin_f64(static_cast<double>(x)) / micron::math::cr::cos_f64(static_cast<double>(x))))

// these are libcalls on armv7-a (no vrintX)
__MC_M1(ceil, micron::math::ceil(x), micron::math::ceil(x), micron::math::ceil(x))
__MC_M1(floor, micron::math::floor(x), micron::math::floor(x), micron::math::floor(x))
__MC_M1(round, micron::math::round(x), micron::math::round(x), micron::math::round(x))
__MC_M1(trunc, (x < 0 ? micron::math::ceil(x) : micron::math::floor(x)), (x < 0 ? micron::math::ceil(x) : micron::math::floor(x)),
        (x < 0 ? micron::math::ceil(x) : micron::math::floor(x)))
__MC_M1(rint, micron::math::rint(x), micron::math::rint(x), static_cast<long double>(micron::math::rint(static_cast<double>(x))))
__MC_M1(nearbyint, micron::math::nearbyint(x), micron::math::nearbyint(x),
        static_cast<long double>(micron::math::nearbyint(static_cast<double>(x))))

// __builtin_fabs/copysign are pure bit-ops, never libcalls
__MC_M1(fabs, __builtin_fabs(x), __builtin_fabsf(x), __builtin_fabsl(x))
__MC_M2(copysign, __builtin_copysign(x, y), __builtin_copysignf(x, y), __builtin_copysignl(x, y))

__MC_M2(pow, micron::math::powerf(x, y), micron::math::powerf32(x, y), micron::math::powerflong(x, y))
__MC_M2(remainder, micron::math::remainder(x, y), micron::math::remainder(x, y),
        static_cast<long double>(micron::math::remainder(static_cast<double>(x), static_cast<double>(y))))
__MC_M2(hypot, micron::math::hw::sqrt_sd(x *x + y * y), micron::math::hw::sqrt_ss(x *x + y * y),
        static_cast<long double>(micron::math::hw::sqrt_sd(static_cast<double>(x) * static_cast<double>(x)
                                                           + static_cast<double>(y) * static_cast<double>(y))))

extern "C" __attribute__((weak)) double
fmod(double x, double y) noexcept
{
  double q = x / y;
  double t = (q < 0 ? micron::math::ceil(q) : micron::math::floor(q));
  return x - t * y;
}

extern "C" __attribute__((weak)) float
fmodf(float x, float y) noexcept
{
  float q = x / y;
  float t = (q < 0 ? micron::math::ceil(q) : micron::math::floor(q));
  return x - t * y;
}

extern "C" __attribute__((weak)) long double
fmodl(long double x, long double y) noexcept
{
  long double q = x / y;
  long double t = (q < 0 ? micron::math::ceil(q) : micron::math::floor(q));
  return x - t * y;
}

// we're letting the optimizer deal with there
extern "C" __attribute__((weak)) double
fma(double a, double b, double c) noexcept
{
  return a * b + c;
}

extern "C" __attribute__((weak)) float
fmaf(float a, float b, float c) noexcept
{
  return a * b + c;
}

extern "C" __attribute__((weak)) long double
fmal(long double a, long double b, long double c) noexcept
{
  return a * b + c;
}

// missing __builtin_* symbols (these should be almost all of them, although these libm fns fmax fmin fdim ldexp frexp modf scalbn nextafter
// ilogb remquo lrint llrint lround llround still don't exist in micron)

// inverse trig
__MC_M1(asin, micron::math::mkbits::trig_ns::asin<f64>(x), micron::math::mkbits::trig_ns::asin<f32>(x),
        static_cast<long double>(micron::math::mkbits::trig_ns::asin<f64>(static_cast<double>(x))))
__MC_M1(acos, micron::math::mkbits::trig_ns::acos<f64>(x), micron::math::mkbits::trig_ns::acos<f32>(x),
        static_cast<long double>(micron::math::mkbits::trig_ns::acos<f64>(static_cast<double>(x))))
__MC_M1(atan, micron::math::mkbits::trig_ns::atan<f64>(x), micron::math::mkbits::trig_ns::atan<f32>(x),
        static_cast<long double>(micron::math::mkbits::trig_ns::atan<f64>(static_cast<double>(x))))

// hyperbolic
__MC_M1(sinh, micron::math::mkbits::hyp_ns::sinh<f64>(x), micron::math::mkbits::hyp_ns::sinh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::sinh<f64>(static_cast<double>(x))))
__MC_M1(cosh, micron::math::mkbits::hyp_ns::cosh<f64>(x), micron::math::mkbits::hyp_ns::cosh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::cosh<f64>(static_cast<double>(x))))
__MC_M1(tanh, micron::math::mkbits::hyp_ns::tanh<f64>(x), micron::math::mkbits::hyp_ns::tanh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::tanh<f64>(static_cast<double>(x))))
__MC_M1(asinh, micron::math::mkbits::hyp_ns::asinh<f64>(x), micron::math::mkbits::hyp_ns::asinh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::asinh<f64>(static_cast<double>(x))))
__MC_M1(acosh, micron::math::mkbits::hyp_ns::acosh<f64>(x), micron::math::mkbits::hyp_ns::acosh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::acosh<f64>(static_cast<double>(x))))
__MC_M1(atanh, micron::math::mkbits::hyp_ns::atanh<f64>(x), micron::math::mkbits::hyp_ns::atanh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::atanh<f64>(static_cast<double>(x))))

// exp/log tail
__MC_M1(exp10, micron::math::mkbits::exp_ns::exp10<f64>(x), micron::math::mkbits::exp_ns::exp10<f32>(x),
        static_cast<long double>(micron::math::mkbits::exp_ns::exp10<f64>(static_cast<double>(x))))
__MC_M1(expm1, micron::math::mkbits::exp_ns::expm1<f64>(x), micron::math::mkbits::exp_ns::expm1<f32>(x),
        static_cast<long double>(micron::math::mkbits::exp_ns::expm1<f64>(static_cast<double>(x))))
__MC_M1(log1p, micron::math::mkbits::log_ns::log1p<f64>(x), micron::math::mkbits::log_ns::log1p<f32>(x),
        static_cast<long double>(micron::math::mkbits::log_ns::log1p<f64>(static_cast<double>(x))))
__MC_M1(logb, micron::math::mkbits::manip::logb<f64>(x), micron::math::mkbits::manip::logb<f32>(x),
        static_cast<long double>(micron::math::mkbits::manip::logb<f64>(static_cast<double>(x))))

// error fn / gamma
__MC_M1(erf, micron::math::mkbits::special_ns::erf<f64>(x), micron::math::mkbits::special_ns::erf<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::erf<f64>(static_cast<double>(x))))
__MC_M1(erfc, micron::math::mkbits::special_ns::erfc<f64>(x), micron::math::mkbits::special_ns::erfc<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::erfc<f64>(static_cast<double>(x))))
__MC_M1(tgamma, micron::math::mkbits::special_ns::tgamma<f64>(x), micron::math::mkbits::special_ns::tgamma<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::tgamma<f64>(static_cast<double>(x))))
__MC_M1(lgamma, micron::math::mkbits::special_ns::lgamma<f64>(x), micron::math::mkbits::special_ns::lgamma<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::lgamma<f64>(static_cast<double>(x))))

// bessel
__MC_M1(j0, micron::math::mkbits::special_ns::j0<f64>(x), micron::math::mkbits::special_ns::j0<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::j0<f64>(static_cast<double>(x))))
__MC_M1(j1, micron::math::mkbits::special_ns::j1<f64>(x), micron::math::mkbits::special_ns::j1<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::j1<f64>(static_cast<double>(x))))
__MC_M1(y0, micron::math::mkbits::special_ns::y0<f64>(x), micron::math::mkbits::special_ns::y0<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::y0<f64>(static_cast<double>(x))))
__MC_M1(y1, micron::math::mkbits::special_ns::y1<f64>(x), micron::math::mkbits::special_ns::y1<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::y1<f64>(static_cast<double>(x))))

extern "C" __attribute__((weak)) double
atan2(double y, double x) noexcept
{
  return micron::math::mkbits::trig_ns::atan2<f64>(y, x);
}

extern "C" __attribute__((weak)) float
atan2f(float y, float x) noexcept
{
  return micron::math::mkbits::trig_ns::atan2<f32>(y, x);
}

extern "C" __attribute__((weak)) long double
atan2l(long double y, long double x) noexcept
{
  return static_cast<long double>(micron::math::mkbits::trig_ns::atan2<f64>(static_cast<double>(y), static_cast<double>(x)));
}

static_assert(micron::math::mkbits::trig_ns::atan2<f64>(1.0, 0.0) > 1.5707 && micron::math::mkbits::trig_ns::atan2<f64>(1.0, 0.0) < 1.5709,
              "atan2 kernel takes (ordinate, abscissa): atan2(y=1, x=0) must be +pi/2");
static_assert(micron::math::mkbits::trig_ns::atan2<f64>(0.0, -1.0) > 3.1415
                  && micron::math::mkbits::trig_ns::atan2<f64>(0.0, -1.0) < 3.1417,
              "atan2 kernel takes (ordinate, abscissa): atan2(y=0, x=-1) must be +pi");
static_assert(micron::math::mkbits::trig_ns::atan2<f64>(-1.0, 0.0) < -1.5707,
              "atan2 kernel takes (ordinate, abscissa): atan2(y=-1, x=0) must be -pi/2");

#undef __MC_M1
#undef __MC_M2

#pragma GCC diagnostic pop

// libgcc integer fns
#include "__gcc_int_syms.hpp"

// libgcc binary128 soft fp fns
#include "__gcc_fp128_syms.hpp"
