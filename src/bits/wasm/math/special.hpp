// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "extended.hpp"
#include "gamma_coefficients.hpp"

namespace micron::wasm::math
{
inline constexpr real euler = 0.577215664901532860606512090082402431042159335939924L;
inline constexpr real sqrt_pi = 1.772453850905516027298167483341145182797549456122387L;

inline constexpr real
sinpi(real x) noexcept
{
  real r = fmod(abs(x), 2.L);
  bool negative = false;
  if ( r > 1 ) {
    r = 2 - r;
    negative = true;
  }
  if ( r > 0.5L ) r = 1 - r;
  real result = sin(pi * r);
  if ( negative ) result = -result;
  return bits(x) & format<real>::sign ? -result : result;
}

inline constexpr real
__lgamma_near_one(real z) noexcept
{
  real polynomial = __lgamma_coefficients[62];
  for ( int k = 61; k >= 0; --k ) polynomial = fma(polynomial, z, __lgamma_coefficients[k]);
  return z * fma(z, polynomial, -euler);
}

inline constexpr __ddreal
__ddlgamma_positive(real x) noexcept
{
  if ( x == 1 || x == 2 ) return {};
  if ( x >= 0.75L && x <= 1.25L ) return { __lgamma_near_one(x - 1), 0 };
  if ( x >= 1.75L && x <= 2.25L ) return __ddsum(log1p(x - 2), __lgamma_near_one(x - 2));
  __ddreal adjustment{};
  __ddreal shifted{ x, 0 };
  while ( x < 64 ) {
    const auto l = __ddadd(__ddlog(x), __dddiv({ shifted.lo, 0 }, x));
    adjustment = __ddadd(adjustment, { -l.hi, -l.lo });
    shifted = __ddadd(shifted, { 1, 0 });
    x = shifted.hi;
  }
  constexpr real coefficients[]{
    1.L / 12,  -1.L / 360,       1.L / 1260,       -1.L / 1680,        1.L / 1188,     -691.L / 360360,
    1.L / 156, -3617.L / 122400, 43867.L / 244188, -174611.L / 125400, 77683.L / 5796, -236364091.L / 1506960
  };
  const real inv = 1 / x, inv2 = inv * inv;
  real sum = coefficients[11];
  for ( int i = 10; i >= 0; --i ) sum = coefficients[i] + inv2 * sum;
  const auto logarithm = __ddadd(__ddlog(x), __dddiv({ shifted.lo, 0 }, x));
  auto result = __ddadd(__ddmul(__ddadd(shifted, { -0.5L, 0 }), logarithm), { -x, -shifted.lo });
  result = __ddadd(result, { 0.918938533204672741780329736405617639861397473637783L, 0 });
  result = __ddadd(result, { inv * sum, 0 });
  result = __ddadd(result, adjustment);
  return result;
}

inline constexpr real
lgamma(real x) noexcept
{
  if ( isnan(x) || isinf(x) ) return abs(x);
  if ( x <= 0 ) {
    if ( trunc(x) == x ) return inf;
    if ( x > -0.25L ) return __lgamma_near_one(x) - log(-x);
    return log(pi) - log(abs(sinpi(x))) - lgamma(1 - x);
  }
  if ( x >= 0x1p8192L ) {
    // x*log(x) can overflow while x*(log(x)-1) still fits. All remaining
    // Stirling terms are below a binary128 ULP here; scale before the product.
    const auto leading = __ddmul({ x * 0x1p-16L, 0 }, __ddadd(__ddlog(x), { -1, 0 }));
    return scalbn(leading.hi + leading.lo, 16);
  }
  const auto result = __ddlgamma_positive(x);
  return result.hi + result.lo;
}

inline constexpr real
tgamma(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( x == 0 ) return copysign(inf, x);
  if ( x < 0 && trunc(x) == x ) return nan<real>();
  if ( x > 2000 ) return inf;
  if ( x > 0 ) return __ddexp(__ddlgamma_positive(x));
  if ( x > -0.25L ) {
    const auto logarithm = __ddlog(-x);
    return -__ddexp(__ddadd({ __lgamma_near_one(x), 0 }, { -logarithm.hi, -logarithm.lo }));
  }
  const real s = sinpi(x);
  if ( x < -2000 ) return copysign(0.L, s);
  const auto a = __ddlog(abs(s)), b = __ddlgamma_positive(1 - x);
  const auto reflected = __ddadd(__ddadd(__ddlog(pi), { -a.hi, -a.lo }), { -b.hi, -b.lo });
  return copysign(__ddexp(reflected), s);
}

inline constexpr real
erf_small(real x) noexcept
{
  const real minus_square = -x * x;
  real term = x, sum = x;
  for ( int i = 1; i <= 120; ++i ) {
    term *= minus_square / real(i);
    sum += term / real(2 * i + 1);
  }
  return 2 / sqrt_pi * sum;
}

inline constexpr real
erfc(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( x < 0 ) return 2 - erfc(-x);
  if ( x < 1.5L ) return 1 - erf_small(x);
  if ( x > 128 ) return 0;
  const real z = x * x;
  real b = z + 0.5L, c = 0x1p1000L, d = 1 / b, h = d;
  for ( int i = 1; i <= 2000; ++i ) {
    const real a = -real(i) * (real(i) - 0.5L);
    b += 2;
    d = a * d + b;
    c = b + a / c;
    if ( abs(d) < 0x1p-1000L ) d = 0x1p-1000L;
    if ( abs(c) < 0x1p-1000L ) c = 0x1p-1000L;
    d = 1 / d;
    const real delta = d * c;
    h *= delta;
    if ( delta == 1 ) break;
  }
  const real exponential = __ddexp({ -z, -fma(x, x, -z) });
  return (exponential * x) * h / sqrt_pi;
}

inline constexpr real
erf(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( abs(x) < 1.5L ) return erf_small(x);
  return copysign(1 - erfc(abs(x)), x);
}

inline constexpr void
__bessel_series(real x, real &jzero, real &jone, real &yzero, real &yone) noexcept
{
  __ddreal square = __ddmul({ x / 2, 0 }, { -x / 2, 0 });
  __ddreal t0{ 1, 0 }, t1{ x / 2, 0 }, s0{}, s1{}, h{}, sy0{}, sy1{};
  for ( int k = 0; k < 256; ++k ) {
    s0 = __ddadd(s0, t0);
    s1 = __ddadd(s1, t1);
    auto ht = __ddmul(h, t0);
    sy0 = __ddadd(sy0, { -ht.hi, -ht.lo });
    const auto next_h = __ddadd(h, __dddiv({ 1, 0 }, real(k + 1)));
    sy1 = __ddadd(sy1, __ddmul(__ddadd(h, next_h), t1));
    h = next_h;
    t0 = __dddiv(__ddmul(t0, square), real((k + 1) * (k + 1)));
    t1 = __dddiv(__ddmul(t1, square), real((k + 1) * (k + 2)));
  }
  jzero = s0.hi + s0.lo;
  jone = s1.hi + s1.lo;
  const auto logarithm = __ddadd(__ddlog(x / 2), { euler, __euler_lo });
  const auto y0sum = __ddadd(__ddmul(logarithm, s0), sy0);
  const auto y1first = __ddadd(__ddmul(logarithm, s1), __dddiv({ -1, 0 }, x));
  const auto y1sum = __ddadd(__ddmul({ 2, 0 }, y1first), { -sy1.hi, -sy1.lo });
  yzero = (2 / pi) * (y0sum.hi + y0sum.lo);
  yone = (y1sum.hi + y1sum.lo) / pi;
}

inline constexpr void
__bessel_asymptotic(real x, int order, real &j, real &y) noexcept
{
  real sx, cx;
  sincos(x, sx, cx);
  const real cphase = order ? sx - cx : cx + sx;
  const real sphase = order ? -cx - sx : sx - cx;
  real p = 1, q = 0, term = 1;
  for ( int k = 1; k <= 40; ++k ) {
    term *= real(4 * order * order - (2 * k - 1) * (2 * k - 1)) / (8 * real(k) * x);
    if ( k & 1 )
      q += (k & 2) ? -term : term;
    else
      p += (k & 2) ? -term : term;
  }
  const real scale = sqrt(1 / pi / x);
  j = scale * (cphase * p - sphase * q);
  y = scale * (sphase * p + cphase * q);
}

inline constexpr real
j0(real x) noexcept
{
  x = abs(x);
  if ( isnan(x) ) return x;
  if ( isinf(x) ) return 0;
  if ( x == 0 ) return 1;
  real j, j1v, y, y1v;
  if ( x < 64 )
    __bessel_series(x, j, j1v, y, y1v);
  else
    __bessel_asymptotic(x, 0, j, y);
  return j;
}

inline constexpr real
j1(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( isinf(x) ) return copysign(0.L, x);
  if ( x == 0 ) return x;
  real j0v, j, y0v, y;
  if ( abs(x) < 64 )
    __bessel_series(abs(x), j0v, j, y0v, y);
  else
    __bessel_asymptotic(abs(x), 1, j, y);
  return bits(x) & format<real>::sign ? -j : j;
}

inline constexpr real
y0(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( x < 0 ) return nan<real>();
  if ( x == 0 ) return -inf;
  if ( isinf(x) ) return 0;
  // DLMF 10.8.2: the omitted x^2 terms are below a binary128 ULP.
  // Form log(x)-log(2) without underflowing x/2.
  if ( x < 0x1p-64L ) return (2 / pi) * (log(x) - ln2 + euler);
  real j, j1v, y, y1v;
  if ( x < 64 )
    __bessel_series(x, j, j1v, y, y1v);
  else
    __bessel_asymptotic(x, 0, j, y);
  return y;
}

inline constexpr real
y1(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( x < 0 ) return nan<real>();
  if ( x == 0 ) return -inf;
  if ( isinf(x) ) return 0;
  // DLMF 10.8.1: divide the finite coefficient directly, preserving -Inf
  // when 1/x exceeds the format range before compensated arithmetic.
  if ( x < 0x1p-64L ) return -(2 / pi) / x;
  real j0v, j, y0v, y;
  if ( x < 64 )
    __bessel_series(x, j0v, j, y0v, y);
  else
    __bessel_asymptotic(x, 1, j, y);
  return y;
}
}      // namespace micron::wasm::math
