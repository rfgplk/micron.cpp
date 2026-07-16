//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// doesn't use __builtins anymore; reroutes through to the math kernel qualified namespaces (mkbits::)

// NOTE: logs have been moved to log.hpp

#include "../types.hpp"
#include "bits/exp.hpp"
#include "bits/hyp.hpp"
#include "bits/log.hpp"
#include "bits/manip.hpp"
#include "bits/pow.hpp"
#include "bits/rem.hpp"
#include "bits/round.hpp"
#include "bits/special.hpp"
#include "bits/sqrt.hpp"
#include "bits/trig.hpp"

namespace micron
{
namespace math
{

#define __micron_trig_unary_fn(NAME, KERNEL_NS, KERNEL_FN)                                                                                 \
  constexpr float NAME(float x) noexcept { return float(mkbits::KERNEL_NS::KERNEL_FN<f32>(f32(x))); }                                      \
  constexpr double NAME(double x) noexcept { return double(mkbits::KERNEL_NS::KERNEL_FN<f64>(f64(x))); }                                   \
  constexpr long double NAME(long double x) noexcept { return static_cast<long double>(mkbits::KERNEL_NS::KERNEL_FN<f64>(f64(x))); }

#define __micron_trig_binary_fn(NAME, KERNEL_NS, KERNEL_FN)                                                                                \
  constexpr float NAME(float x, float y) noexcept { return float(mkbits::KERNEL_NS::KERNEL_FN<f32>(f32(x), f32(y))); }                     \
  constexpr double NAME(double x, double y) noexcept { return double(mkbits::KERNEL_NS::KERNEL_FN<f64>(f64(x), f64(y))); }                 \
  constexpr long double NAME(long double x, long double y) noexcept                                                                        \
  {                                                                                                                                        \
    return static_cast<long double>(mkbits::KERNEL_NS::KERNEL_FN<f64>(f64(x), f64(y)));                                                    \
  }

__micron_trig_unary_fn(sin, trig_ns, sin);
__micron_trig_unary_fn(cos, trig_ns, cos);
__micron_trig_unary_fn(tan, trig_ns, tan);
__micron_trig_unary_fn(asin, trig_ns, asin);
__micron_trig_unary_fn(acos, trig_ns, acos);
__micron_trig_unary_fn(atan, trig_ns, atan);
__micron_trig_binary_fn(atan2, trig_ns, atan2);
__micron_trig_unary_fn(sinh, hyp_ns, sinh);
__micron_trig_unary_fn(cosh, hyp_ns, cosh);
__micron_trig_unary_fn(tanh, hyp_ns, tanh);
__micron_trig_unary_fn(exp, exp_ns, exp);
__micron_trig_unary_fn(sqrt, sqrt_ns, sqrt);
__micron_trig_binary_fn(pow, pow_ns, pow);
__micron_trig_unary_fn(floor, round_ns, floor);
__micron_trig_unary_fn(ceil, round_ns, ceil);
__micron_trig_unary_fn(fabs, manip, fabs);
__micron_trig_binary_fn(fmod, rem, fmod);

#undef __micron_trig_unary_fn
#undef __micron_trig_binary_fn

inline float
frexp(float x, int *exponent) noexcept
{
  return float(mkbits::manip::frexp<f32>(f32(x), exponent));
}

inline double
frexp(double x, int *exponent) noexcept
{
  return double(mkbits::manip::frexp<f64>(f64(x), exponent));
}

inline long double
frexp(long double x, int *exponent) noexcept
{
  return static_cast<long double>(mkbits::manip::frexp<f64>(f64(x), exponent));
}

constexpr float
ldexp(float x, int n) noexcept
{
  return float(mkbits::manip::ldexp<f32>(f32(x), n));
}

constexpr double
ldexp(double x, int n) noexcept
{
  return double(mkbits::manip::ldexp<f64>(f64(x), n));
}

constexpr long double
ldexp(long double x, int n) noexcept
{
  return static_cast<long double>(mkbits::manip::ldexp<f64>(f64(x), n));
}

inline float
modf(float x, float *iptr) noexcept
{
  float i = float(mkbits::round_ns::trunc<f32>(f32(x)));
  *iptr = i;
  return float(x - i);
}

inline double
modf(double x, double *iptr) noexcept
{
  double i = double(mkbits::round_ns::trunc<f64>(f64(x)));
  *iptr = i;
  return double(x - i);
}

inline long double
modf(long double x, long double *iptr) noexcept
{
  long double i = static_cast<long double>(mkbits::round_ns::trunc<f64>(f64(x)));
  *iptr = i;
  return x - i;
}

constexpr float
erf(float x) noexcept
{
  return float(mkbits::special_ns::erf<f32>(f32(x)));
}

constexpr double
erf(double x) noexcept
{
  return double(mkbits::special_ns::erf<f64>(f64(x)));
}

constexpr long double
erf(long double x) noexcept
{
  return static_cast<long double>(mkbits::special_ns::erf<f64>(f64(x)));
}

constexpr float
erfc(float x) noexcept
{
  return float(mkbits::special_ns::erfc<f32>(f32(x)));
}

constexpr double
erfc(double x) noexcept
{
  return double(mkbits::special_ns::erfc<f64>(f64(x)));
}

constexpr long double
erfc(long double x) noexcept
{
  return static_cast<long double>(mkbits::special_ns::erfc<f64>(f64(x)));
}

constexpr float
tgamma(float x) noexcept
{
  return float(mkbits::special_ns::tgamma<f32>(f32(x)));
}

constexpr double
tgamma(double x) noexcept
{
  return double(mkbits::special_ns::tgamma<f64>(f64(x)));
}

constexpr long double
tgamma(long double x) noexcept
{
  return static_cast<long double>(mkbits::special_ns::tgamma<f64>(f64(x)));
}

constexpr float
exp2(float x) noexcept
{
  return float(mkbits::exp_ns::exp2<f32>(f32(x)));
}

constexpr double
exp2(double x) noexcept
{
  return double(mkbits::exp_ns::exp2<f64>(f64(x)));
}

constexpr long double
exp2(long double x) noexcept
{
  return static_cast<long double>(mkbits::exp_ns::exp2<f64>(f64(x)));
}

#if defined(__STDCPP_FLOAT16_T__) && defined(_GLIBCXX_FLOAT_IS_IEEE_BINARY32)
constexpr _Float16
sin(_Float16 x) noexcept
{
  return _Float16(mkbits::trig_ns::sin<f32>(f32(x)));
}

constexpr _Float16
cos(_Float16 x) noexcept
{
  return _Float16(mkbits::trig_ns::cos<f32>(f32(x)));
}

constexpr _Float16
tan(_Float16 x) noexcept
{
  return _Float16(mkbits::trig_ns::tan<f32>(f32(x)));
}

constexpr _Float16
asin(_Float16 x) noexcept
{
  return _Float16(mkbits::trig_ns::asin<f32>(f32(x)));
}

constexpr _Float16
acos(_Float16 x) noexcept
{
  return _Float16(mkbits::trig_ns::acos<f32>(f32(x)));
}

constexpr _Float16
atan(_Float16 x) noexcept
{
  return _Float16(mkbits::trig_ns::atan<f32>(f32(x)));
}

constexpr _Float16
atan2(_Float16 y, _Float16 x) noexcept
{
  return _Float16(mkbits::trig_ns::atan2<f32>(f32(y), f32(x)));
}

constexpr _Float16
sinh(_Float16 x) noexcept
{
  return _Float16(mkbits::hyp_ns::sinh<f32>(f32(x)));
}

constexpr _Float16
cosh(_Float16 x) noexcept
{
  return _Float16(mkbits::hyp_ns::cosh<f32>(f32(x)));
}

constexpr _Float16
tanh(_Float16 x) noexcept
{
  return _Float16(mkbits::hyp_ns::tanh<f32>(f32(x)));
}

constexpr _Float16
asinh(_Float16 x) noexcept
{
  return _Float16(mkbits::hyp_ns::asinh<f32>(f32(x)));
}

constexpr _Float16
acosh(_Float16 x) noexcept
{
  return _Float16(mkbits::hyp_ns::acosh<f32>(f32(x)));
}

constexpr _Float16
atanh(_Float16 x) noexcept
{
  return _Float16(mkbits::hyp_ns::atanh<f32>(f32(x)));
}

constexpr _Float16
exp(_Float16 x) noexcept
{
  return _Float16(mkbits::exp_ns::exp<f32>(f32(x)));
}

constexpr _Float16
sqrt(_Float16 x) noexcept
{
  return _Float16(mkbits::sqrt_ns::sqrt<f32>(f32(x)));
}

constexpr _Float16
pow(_Float16 x, _Float16 y) noexcept
{
  return _Float16(mkbits::pow_ns::pow<f32>(f32(x), f32(y)));
}

constexpr _Float16
floor(_Float16 x) noexcept
{
  return _Float16(mkbits::round_ns::floor<f32>(f32(x)));
}

constexpr _Float16
ceil(_Float16 x) noexcept
{
  return _Float16(mkbits::round_ns::ceil<f32>(f32(x)));
}

constexpr _Float16
fabs(_Float16 x) noexcept
{
  return _Float16(mkbits::manip::fabs<f32>(f32(x)));
}

constexpr _Float16
fmod(_Float16 x, _Float16 y) noexcept
{
  return _Float16(mkbits::rem::fmod<f32>(f32(x), f32(y)));
}

inline _Float16
frexp(_Float16 x, int *e) noexcept
{
  return _Float16(mkbits::manip::frexp<f32>(f32(x), e));
}

constexpr _Float16
ldexp(_Float16 x, int n) noexcept
{
  return _Float16(mkbits::manip::ldexp<f32>(f32(x), n));
}

inline _Float16
modf(_Float16 x, _Float16 *iptr) noexcept
{
  f32 i = f32(mkbits::round_ns::trunc<f32>(f32(x)));
  *iptr = _Float16(i);
  return _Float16(f32(x) - i);
}
#endif

#if defined(__STDCPP_FLOAT128_T__) && defined(_GLIBCXX_HAVE_FLOAT128_MATH)
constexpr _Float128
sin(_Float128 x) noexcept
{
  return _Float128(mkbits::trig_ns::sin<f64>(f64(x)));
}

constexpr _Float128
cos(_Float128 x) noexcept
{
  return _Float128(mkbits::trig_ns::cos<f64>(f64(x)));
}

constexpr _Float128
tan(_Float128 x) noexcept
{
  return _Float128(mkbits::trig_ns::tan<f64>(f64(x)));
}

constexpr _Float128
asin(_Float128 x) noexcept
{
  return _Float128(mkbits::trig_ns::asin<f64>(f64(x)));
}

constexpr _Float128
acos(_Float128 x) noexcept
{
  return _Float128(mkbits::trig_ns::acos<f64>(f64(x)));
}

constexpr _Float128
atan(_Float128 x) noexcept
{
  return _Float128(mkbits::trig_ns::atan<f64>(f64(x)));
}

constexpr _Float128
atan2(_Float128 y, _Float128 x) noexcept
{
  return _Float128(mkbits::trig_ns::atan2<f64>(f64(y), f64(x)));
}

constexpr _Float128
sinh(_Float128 x) noexcept
{
  return _Float128(mkbits::hyp_ns::sinh<f64>(f64(x)));
}

constexpr _Float128
cosh(_Float128 x) noexcept
{
  return _Float128(mkbits::hyp_ns::cosh<f64>(f64(x)));
}

constexpr _Float128
tanh(_Float128 x) noexcept
{
  return _Float128(mkbits::hyp_ns::tanh<f64>(f64(x)));
}

constexpr _Float128
exp(_Float128 x) noexcept
{
  return _Float128(mkbits::exp_ns::exp<f64>(f64(x)));
}

constexpr _Float128
sqrt(_Float128 x) noexcept
{
  return _Float128(mkbits::sqrt_ns::sqrt<f64>(f64(x)));
}

constexpr _Float128
pow(_Float128 x, _Float128 y) noexcept
{
  return _Float128(mkbits::pow_ns::pow<f64>(f64(x), f64(y)));
}

constexpr _Float128
floor(_Float128 x) noexcept
{
  return _Float128(mkbits::round_ns::floor<f64>(f64(x)));
}

constexpr _Float128
ceil(_Float128 x) noexcept
{
  return _Float128(mkbits::round_ns::ceil<f64>(f64(x)));
}

constexpr _Float128
fabs(_Float128 x) noexcept
{
  return _Float128(mkbits::manip::fabs<f64>(f64(x)));
}

constexpr _Float128
fmod(_Float128 x, _Float128 y) noexcept
{
  return _Float128(mkbits::rem::fmod<f64>(f64(x), f64(y)));
}

inline _Float128
frexp(_Float128 x, int *e) noexcept
{
  return _Float128(mkbits::manip::frexp<f64>(f64(x), e));
}

constexpr _Float128
ldexp(_Float128 x, int n) noexcept
{
  return _Float128(mkbits::manip::ldexp<f64>(f64(x), n));
}

inline _Float128
modf(_Float128 x, _Float128 *iptr) noexcept
{
  f64 i = f64(mkbits::round_ns::trunc<f64>(f64(x)));
  *iptr = _Float128(i);
  return _Float128(f64(x) - i);
}
#endif

constexpr float
fcos(float x) noexcept
{
  return float(mkbits::trig_ns::cos<f32>(f32(x)));
}

constexpr double
fcos(double x) noexcept
{
  return double(mkbits::trig_ns::cos<f64>(f64(x)));
}

constexpr long double
fcos(long double x) noexcept
{
  return static_cast<long double>(mkbits::trig_ns::cos<f64>(f64(x)));
}

constexpr float
ftan(float x) noexcept
{
  return float(mkbits::trig_ns::tan<f32>(f32(x)));
}

constexpr double
ftan(double x) noexcept
{
  return double(mkbits::trig_ns::tan<f64>(f64(x)));
}

constexpr long double
ftan(long double x) noexcept
{
  return static_cast<long double>(mkbits::trig_ns::tan<f64>(f64(x)));
}

constexpr float
fasin(float x) noexcept
{
  return float(mkbits::trig_ns::asin<f32>(f32(x)));
}

constexpr double
fasin(double x) noexcept
{
  return double(mkbits::trig_ns::asin<f64>(f64(x)));
}

constexpr long double
fasin(long double x) noexcept
{
  return static_cast<long double>(mkbits::trig_ns::asin<f64>(f64(x)));
}

constexpr float
facos(float x) noexcept
{
  return float(mkbits::trig_ns::acos<f32>(f32(x)));
}

constexpr double
facos(double x) noexcept
{
  return double(mkbits::trig_ns::acos<f64>(f64(x)));
}

constexpr long double
facos(long double x) noexcept
{
  return static_cast<long double>(mkbits::trig_ns::acos<f64>(f64(x)));
}

constexpr float
fatan(float x) noexcept
{
  return float(mkbits::trig_ns::atan<f32>(f32(x)));
}

constexpr double
fatan(double x) noexcept
{
  return double(mkbits::trig_ns::atan<f64>(f64(x)));
}

constexpr long double
fatan(long double x) noexcept
{
  return static_cast<long double>(mkbits::trig_ns::atan<f64>(f64(x)));
}

constexpr float
fatan2(float y, float x) noexcept
{
  return float(mkbits::trig_ns::atan2<f32>(f32(y), f32(x)));
}

constexpr double
fatan2(double y, double x) noexcept
{
  return double(mkbits::trig_ns::atan2<f64>(f64(y), f64(x)));
}

constexpr long double
fatan2(long double y, long double x) noexcept
{
  return static_cast<long double>(mkbits::trig_ns::atan2<f64>(f64(y), f64(x)));
}

};      // namespace math
};      // namespace micron
