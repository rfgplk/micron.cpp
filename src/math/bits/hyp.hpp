//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trig hyperbolics live here

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits.hpp"
#include "../ieee.hpp"
#include "exp.hpp"
#include "log.hpp"
#include "manip.hpp"
#include "sqrt.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace hyp_ns
{

// NOTE: small x via expm1, large via (exp(x) - exp(-x))/2
[[nodiscard, gnu::flatten]] inline constexpr f64
sinh_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  f64 ax = manip::fabs(x);
  if ( ax < 0x1.0p-28 ) return x;
  if ( ax > 710.5 ) return manip::copysign<f64>(ieee::inf_v<f64>(0), x);
  if ( ax <= 1.0 ) {
    // (expm1(x)*(2 + expm1(x))) / (2 * (1 + expm1(x)))
    f64 t = exp_ns::expm1_f64(ax);
    f64 r = 0.5 * (t + t / (1.0 + t));
    return manip::copysign<f64>(r, x);
  }
  if ( ax < 22.0 ) {
    f64 t = exp_ns::exp_f64(ax);
    f64 r = 0.5 * (t - 1.0 / t);
    return manip::copysign<f64>(r, x);
  }
  // 1/exp(ax) is below ulp(exp(ax)/2)
  f64 r = 0.5 * exp_ns::exp_f64(ax);
  return manip::copysign<f64>(r, x);
}

// uses sqrt-stable form for small x
[[nodiscard, gnu::flatten]] inline constexpr f64
cosh_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  f64 ax = manip::fabs(x);
  if ( ax > 710.5 ) return ieee::inf_v<f64>(0);
  if ( ax < 0x1.0p-28 ) return 1.0;
  if ( ax < 0.5 ) {
    // cosh(y) - 1 = expm1(y)**2 / (2 * (1 + expm1(y)))
    f64 t = exp_ns::expm1_f64(ax);
    return 1.0 + (t * t) / (2.0 * (1.0 + t));
  }
  if ( ax < 22.0 ) {
    f64 t = exp_ns::exp_f64(ax);
    return 0.5 * (t + 1.0 / t);
  }
  return 0.5 * exp_ns::exp_f64(ax);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
tanh_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  f64 ax = manip::fabs(x);
  if ( ax < 0x1.0p-28 ) return x;
  if ( ax > 22.0 ) return manip::copysign<f64>(1.0, x);
  if ( ax < 1.0 ) {
    // tanh(x) = expm1(2x) / (expm1(2x) + 2)
    f64 t = exp_ns::expm1_f64(2.0 * ax);
    f64 r = t / (t + 2.0);
    return manip::copysign<f64>(r, x);
  }
  f64 t = exp_ns::expm1_f64(-2.0 * ax);
  f64 r = -t / (t + 2.0);
  return manip::copysign<f64>(r, x);
}

// log(x + sqrt(x**2 + 1))
[[nodiscard, gnu::flatten]] inline constexpr f64
asinh_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) ) return x;
  f64 ax = manip::fabs(x);
  if ( ax < 0x1.0p-28 ) return x;
  f64 r;
  if ( ax > 0x1.0p28 ) {
    r = log_ns::log_f64(ax) + 0.6931471805599453;      // log(2*ax) ~ log(ax) + ln2
  } else if ( ax > 2.0 ) {
    r = log_ns::log_f64(2.0 * ax + 1.0 / (sqrt_ns::sqrt<f64>(x * x + 1.0) + ax));
  } else {
    f64 t = ax * ax;
    r = log_ns::log1p_f64(ax + t / (1.0 + sqrt_ns::sqrt<f64>(1.0 + t)));
  }
  return manip::copysign<f64>(r, x);
}

// log(x + sqrt(x**2 - 1)) domain x >= 1
[[nodiscard, gnu::flatten]] inline constexpr f64
acosh_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  if ( x < 1.0 ) return ieee::qnan_v<f64>();
  if ( x == 1.0 ) return 0.0;
  if ( x > 0x1.0p28 ) return log_ns::log_f64(x) + 0.6931471805599453;
  if ( x > 2.0 ) return log_ns::log_f64(2.0 * x - 1.0 / (x + sqrt_ns::sqrt<f64>(x * x - 1.0)));
  f64 t = x - 1.0;
  return log_ns::log1p_f64(t + sqrt_ns::sqrt<f64>(2.0 * t + t * t));
}

// 0.5 * log((1+x)/(1-x))
[[nodiscard, gnu::flatten]] inline constexpr f64
atanh_f64(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  f64 ax = manip::fabs(x);
  if ( ax > 1.0 ) return ieee::qnan_v<f64>();
  if ( ax == 1.0 ) return manip::copysign<f64>(ieee::inf_v<f64>(0), x);
  if ( ax < 0x1.0p-28 ) return x;
  f64 r;
  if ( ax < 0.5 ) {
    f64 t = ax + ax;
    r = 0.5 * log_ns::log1p_f64(t + t * ax / (1.0 - ax));
  } else {
    r = 0.5 * log_ns::log1p_f64(2.0 * (ax / (1.0 - ax)));
  }
  return manip::copysign<f64>(r, x);
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
sinh_f32(f32 x) noexcept
{
  return f32(sinh_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
cosh_f32(f32 x) noexcept
{
  return f32(cosh_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
tanh_f32(f32 x) noexcept
{
  return f32(tanh_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
asinh_f32(f32 x) noexcept
{
  return f32(asinh_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
acosh_f32(f32 x) noexcept
{
  return f32(acosh_f64(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
atanh_f32(f32 x) noexcept
{
  return f32(atanh_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sinh(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(sinh_f32(f32(x)));
  else
    return F(sinh_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
cosh(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(cosh_f32(f32(x)));
  else
    return F(cosh_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
tanh(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(tanh_f32(f32(x)));
  else
    return F(tanh_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
asinh(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(asinh_f32(f32(x)));
  else
    return F(asinh_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
acosh(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(acosh_f32(f32(x)));
  else
    return F(acosh_f64(f64(x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
atanh(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(atanh_f32(f32(x)));
  else
    return F(atanh_f64(f64(x)));
}

};      // namespace hyp_ns
};      // namespace mkbits
};      // namespace math
};      // namespace micron

#pragma GCC pop_options
