// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "real.hpp"
#include "two_over_pi.hpp"

namespace micron::wasm::math
{
using real = long double;
inline constexpr real pi = 3.141592653589793238462643383279502884197169399375105820974944L;
inline constexpr real half_pi = pi / 2;
inline constexpr real ln2 = 0.693147180559945309417232121458176568075500134360255254120680L;
inline constexpr real ln2_hi = 0x1.62e42fefa39ef35793c7673000000p-1L;
inline constexpr real ln2_lo = 1.947045092380749951587959573333273802785282989092885123574676214186875E-31L;
inline constexpr real ln10 = 2.302585092994045684017991454684364207601101488628772976033328L;
inline constexpr real inf = from<real>(format<real>::infinity);

inline constexpr real
exp(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( x > 11357.L ) return inf;
  if ( x < -11434.L ) return 0;
  const int n = int(nearest(x / ln2));
  const real r = fma(-real(n), ln2_hi, x) - real(n) * ln2_lo;
  real sum = 1, term = 1;
  for ( int i = 1; i <= 40; ++i ) {
    term *= r / real(i);
    sum += term;
  }
  return scalbn(sum, n);
}

inline constexpr real
exp2(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( x >= 16384.L ) return inf;
  if ( x < -16495.L ) return 0;
  const int n = int(nearest(x));
  return scalbn(exp((x - real(n)) * ln2), n);
}

inline constexpr real
exp10(real x) noexcept
{
  return exp(x * ln10);
}

inline constexpr real
expm1(real x) noexcept
{
  if ( abs(x) > 0.5L ) return exp(x) - 1;
  if ( x == 0 ) return x;
  real sum = x, term = x;
  for ( int i = 2; i <= 40; ++i ) {
    term *= x / real(i);
    sum += term;
  }
  return sum;
}

inline constexpr real
log(real x) noexcept
{
  if ( isnan(x) || x == inf ) return x;
  if ( x == 0 ) return -inf;
  if ( x < 0 ) return nan<real>();
  int e;
  real m = frexp(x, &e);
  if ( m < 0.707106781186547524400844362104849039284835937688474L ) {
    m *= 2;
    --e;
  }
  const real z = (m - 1) / (m + 1), z2 = z * z;
  real sum = z, power = z;
  for ( int i = 3; i <= 101; i += 2 ) {
    power *= z2;
    sum += power / real(i);
  }
  return fma(real(e), ln2_hi, 2 * sum + real(e) * ln2_lo);
}

struct __ddreal {
  real hi{}, lo{};
};

inline constexpr __ddreal
__ddsum(real a, real b) noexcept
{
  const real s = a + b, bp = s - a;
  return { s, (a - (s - bp)) + (b - bp) };
}

inline constexpr __ddreal
__ddadd(__ddreal a, __ddreal b) noexcept
{
  auto s = __ddsum(a.hi, b.hi);
  return __ddsum(s.hi, s.lo + a.lo + b.lo);
}

inline constexpr __ddreal
__ddmul(__ddreal a, __ddreal b) noexcept
{
  const real p = a.hi * b.hi;
  return __ddsum(p, fma(a.hi, b.hi, -p) + (a.hi * b.lo + a.lo * b.hi) + a.lo * b.lo);
}

inline constexpr __ddreal
__dddiv(__ddreal a, real b) noexcept
{
  const real q = a.hi / b;
  return __ddsum(q, (fma(-q, b, a.hi) + a.lo) / b);
}

inline constexpr __ddreal
__dddiv(__ddreal a, __ddreal b) noexcept
{
  const real q = a.hi / b.hi;
  return __ddsum(q, ((fma(-q, b.hi, a.hi) + a.lo) - q * b.lo) / b.hi);
}

inline constexpr __ddreal
__ddlog(real x) noexcept
{
  int e;
  real m = frexp(x, &e);
  if ( m < 0.707106781186547524400844362104849039284835937688474L ) {
    m *= 2;
    --e;
  }
  const auto z = __dddiv({ m - 1, 0 }, __ddsum(m, 1)), z2 = __ddmul(z, z);
  auto sum = z, power = z;
  for ( int i = 3; i <= 151; i += 2 ) {
    power = __ddmul(power, z2);
    sum = __ddadd(sum, __dddiv(power, real(i)));
  }
  return __ddadd(
      __ddmul(
          { real(e), 0 },
          { ln2,
            -7.008139474549585163412662008771625673777725287234181290190813091556509261166929314266314479764241869442967329248365E-36L }),
      __ddmul({ 2, 0 }, sum));
}

inline constexpr real
__ddexp(__ddreal x) noexcept
{
  if ( isnan(x.hi) ) return x.hi;
  if ( x.hi > 11357.L ) return inf;
  if ( x.hi < -11434.L ) return 0;
  const int n = int(nearest(x.hi / ln2));
  const real r = (fma(-real(n), ln2_hi, x.hi) - real(n) * ln2_lo) + x.lo;
  return scalbn(exp(r), n);
}

inline constexpr real
log2(real x) noexcept
{
  if ( !(x > 0) || isinf(x) ) return log(x);
  int e;
  const real m = frexp(x, &e);
  return real(e) + log(m) / ln2;
}

inline constexpr real
log10(real x) noexcept
{
  return log(x) / ln10;
}

inline constexpr real
log1p(real x) noexcept
{
  if ( x == 0 ) return x;
  if ( abs(x) < 0.5L ) {
    const real z = x / (2 + x), z2 = z * z;
    real sum = z, power = z;
    for ( int i = 3; i <= 151; i += 2 ) {
      power *= z2;
      sum += power / real(i);
    }
    return 2 * sum;
  }
  const real u = 1 + x;
  if ( u <= 0 || isnan(u) || isinf(u) ) return log(u);
  return log(u) + (x - (u - 1)) / u;
}

inline constexpr real
cbrt(real x) noexcept
{
  if ( x == 0 || isinf(x) || isnan(x) ) return x;
  int e;
  const real m = frexp(abs(x), &e);
  int n = e / 3, r = e % 3;
  if ( r < 0 ) {
    r += 3;
    --n;
  }
  const real a = scalbn(m, r);
  real root = 1;
  for ( int i = 0; i < 12; ++i ) root = (2 * root + a / (root * root)) / 3;
  return copysign(scalbn(root, n), x);
}

inline constexpr real
hypot(real x, real y) noexcept
{
  x = abs(x);
  y = abs(y);
  if ( isinf(x) || isinf(y) ) return inf;
  if ( isnan(x) || isnan(y) ) return nan<real>();
  if ( x < y ) {
    const real t = x;
    x = y;
    y = t;
  }
  if ( y == 0 ) return x;
  int e;
  const real sx = frexp(x, &e), sy = scalbn(y, -e);
  return scalbn(sqrt(fma(sx, sx, sy * sy)), e);
}

inline constexpr bool
odd_integer(real x) noexcept
{
  const auto p = decompose(abs(x));
  return p.exponent <= 0 && p.exponent > -113 && ((p.significand >> -p.exponent) & 1);
}

inline constexpr real
pow(real x, real y) noexcept
{
  if ( y == 0 || x == 1 ) return 1;
  if ( isnan(x) || isnan(y) ) return nan<real>();
  const real ax = abs(x);
  const bool integral = trunc(y) == y, odd = integral && odd_integer(y);
  if ( isinf(y) ) return ax == 1 ? 1 : ((ax > 1) == (y > 0) ? inf : 0);
  if ( x == 0 ) return copysign(y < 0 ? inf : 0.L, odd ? x : 1.L);
  if ( isinf(x) ) return copysign(y < 0 ? 0.L : inf, odd ? x : 1.L);
  if ( x < 0 && !integral ) return nan<real>();
  real result;
  // Repeated squaring amplifies rounded products in proportion to the power.
  // Keep the cheap path for small powers; retain compensation for larger ones.
  if ( integral && abs(y) <= 32 ) {
    i64 n = i64(y);
    real base = n < 0 ? 1 / ax : ax;
    if ( n < 0 ) n = -n;
    result = 1;
    while ( n ) {
      if ( n & 1 ) result *= base;
      n >>= 1;
      if ( n ) base *= base;
    }
  } else {
    const auto logarithm = __ddlog(ax);
    const real estimate = y * logarithm.hi;
    result = estimate > 11357.L ? inf : estimate < -11434.L ? 0.L : __ddexp(__ddmul({ y, 0 }, logarithm));
  }
  return x < 0 && odd ? -result : result;
}

// Full-exponent Payne-Hanek reduction. The 528-limb constant is integer data;
// no binary64 conversion limits the input exponent or significand.
inline constexpr int
reduce(real x, real &r) noexcept
{
  const auto p = decompose(abs(x));
  u32 product[532]{};
  for ( unsigned j = 0; j < 4; ++j ) {
    const u32 word = u32(p.significand >> (32 * j));
    u64 carry = 0;
    for ( unsigned i = 0; i < 528; ++i ) {
      const u64 v = u64(__two_over_pi[i]) * word + product[i + j] + carry;
      product[i + j] = u32(v);
      carry = v >> 32;
    }
    product[528 + j] = u32(carry);
  }
  const int point = int(__two_over_pi_bits) - p.exponent;
  auto bit = [&](int at) constexpr -> unsigned { return at < 0 || at >= 532 * 32 ? 0u : (product[at / 32] >> (at % 32)) & 1u; };
  unsigned quadrant = bit(point) | (bit(point + 1) << 1);
  const bool up = bit(point - 1) != 0;
  u64 fraction[4]{};
  for ( int i = 0; i < 256; ++i )
    if ( bit(point - 256 + i) ) fraction[i / 64] |= u64(1) << (i % 64);
  if ( up ) {
    ++quadrant;
    // Values below the retained 256 bits are nonzero for finite binary inputs;
    // floor(1-f) at this precision is the one's complement of floor(f).
    for ( auto &v : fraction ) v = ~v;
  }
  int leading = -1;
  for ( int i = 3; i >= 0; --i )
    if ( fraction[i] ) {
      leading = i * 64 + 63 - __builtin_clzll(fraction[i]);
      break;
    }
  if ( leading < 0 ) {
    r = 0;
    return int(quadrant & 3);
  }
  const int shift = leading - 115;
  wide extended = 0;
  bool sticky = true;
  for ( int i = 0; i <= leading; ++i )
    if ( (fraction[i / 64] >> (i % 64)) & 1 ) {
      if ( i >= shift ) extended |= wide(1) << (i - shift);
    }
  if ( sticky ) extended |= 1;
  r = pack<real>(up ? format<real>::sign : 0, leading - 256 + 16383, extended) * half_pi;
  if ( bits(x) & format<real>::sign ) {
    r = -r;
    quadrant = 0u - quadrant;
  }
  return int(quadrant & 3);
}

inline constexpr void
sincos(real x, real &s, real &c) noexcept
{
  if ( isnan(x) || isinf(x) ) {
    s = c = nan<real>();
    return;
  }
  if ( x == 0 ) {
    s = x;
    c = 1;
    return;
  }
  real r = x;
  const int quadrant = abs(x) <= pi / 4 ? 0 : reduce(x, r);
  const real r2 = -r * r;
  real st = r, ct = 1;
  s = st;
  c = ct;
  for ( int i = 1; i <= 24; ++i ) {
    st *= r2 / real((2 * i) * (2 * i + 1));
    ct *= r2 / real((2 * i - 1) * (2 * i));
    s += st;
    c += ct;
  }
  const real saved = s;
  switch ( quadrant ) {
  case 1:
    s = c;
    c = -saved;
    break;
  case 2:
    s = -s;
    c = -c;
    break;
  case 3:
    s = -c;
    c = saved;
    break;
  }
}

inline constexpr real
sin(real x) noexcept
{
  real s, c;
  sincos(x, s, c);
  return s;
}

inline constexpr real
cos(real x) noexcept
{
  real s, c;
  sincos(x, s, c);
  return c;
}

inline constexpr real
tan(real x) noexcept
{
  real s, c;
  sincos(x, s, c);
  return s / c;
}

inline constexpr real
atan(real x) noexcept
{
  if ( isnan(x) ) return x;
  if ( isinf(x) ) return copysign(half_pi, x);
  if ( x == 0 ) return x;
  real a = abs(x);
  const bool reciprocal = a > 1;
  if ( reciprocal ) a = 1 / a;
  const bool translate = a > 0.414213562373095048801688724209698078569671875376948L;
  if ( translate ) a = (a - 1) / (a + 1);
  const real a2 = -a * a;
  real term = a, result = a;
  for ( int i = 3; i <= 191; i += 2 ) {
    term *= a2;
    result += term / real(i);
  }
  if ( translate ) result += pi / 4;
  if ( reciprocal ) result = half_pi - result;
  return copysign(result, x);
}

inline constexpr real
atan2(real y, real x) noexcept
{
  if ( isnan(x) || isnan(y) ) return nan<real>();
  const bool negative_x = (bits(x) & format<real>::sign) != 0;
  if ( y == 0 ) return negative_x ? copysign(pi, y) : y;
  if ( x == 0 ) return copysign(half_pi, y);
  if ( isinf(x) && isinf(y) ) return copysign(negative_x ? 3 * pi / 4 : pi / 4, y);
  real r = atan(abs(y / x));
  if ( negative_x ) r = pi - r;
  return copysign(r, y);
}

inline constexpr real
asin(real x) noexcept
{
  if ( abs(x) > 1 ) return nan<real>();
  if ( abs(x) == 1 ) return copysign(half_pi, x);
  return atan2(x, sqrt((1 - x) * (1 + x)));
}

inline constexpr real
acos(real x) noexcept
{
  if ( abs(x) > 1 ) return nan<real>();
  if ( x == 1 ) return 0;
  if ( x == -1 ) return pi;
  return 2 * atan2(sqrt(1 - x), sqrt(1 + x));
}

inline constexpr real
sinh(real x) noexcept
{
  const real a = abs(x);
  if ( a < 1 ) {
    const real t = expm1(a);
    return copysign((t + t / (1 + t)) / 2, x);
  }
  const real t = exp(a / 2);
  return copysign((t / 2) * t - (0.5L / t) / t, x);
}

inline constexpr real
cosh(real x) noexcept
{
  const real a = abs(x);
  if ( a < 1 ) {
    const real t = expm1(a);
    return 1 + t * t / (2 * (1 + t));
  }
  const real t = exp(a / 2);
  return (t / 2) * t + (0.5L / t) / t;
}

inline constexpr real
tanh(real x) noexcept
{
  const real a = abs(x);
  if ( a > 40 ) return copysign(1.L, x);
  const real t = expm1(-2 * a);
  return copysign(-t / (t + 2), x);
}

inline constexpr real
asinh(real x) noexcept
{
  const real a = abs(x);
  if ( a > 0x1p60L ) return copysign(log(a) + ln2, x);
  return copysign(log1p(a + a * a / (1 + sqrt(1 + a * a))), x);
}

inline constexpr real
acosh(real x) noexcept
{
  if ( x < 1 ) return nan<real>();
  if ( x > 0x1p60L ) return log(x) + ln2;
  return log1p((x - 1) + sqrt((x - 1) * (x + 1)));
}

inline constexpr real
atanh(real x) noexcept
{
  if ( abs(x) > 1 ) return nan<real>();
  return copysign(log1p(2 * abs(x) / (1 - abs(x))) / 2, x);
}

inline constexpr real
fmod(real x, real y) noexcept
{
  if ( isnan(x) || isnan(y) || isinf(x) || y == 0 ) return nan<real>();
  if ( x == 0 || isinf(y) ) return x;
  const real sign = x;
  x = abs(x);
  y = abs(y);
  if ( x < y ) return sign;
  auto a = decompose(x), b = decompose(y);
  const int am = msb(a.significand), bm = msb(b.significand);
  a.exponent += am;
  b.exponent += bm;
  a.significand <<= 112 - am;
  b.significand <<= 112 - bm;
  for ( int e = a.exponent; e >= b.exponent; --e ) {
    if ( a.significand >= b.significand ) a.significand -= b.significand;
    if ( e != b.exponent ) a.significand <<= 1;
  }
  if ( !a.significand ) return copysign(0.L, sign);
  const int leading = msb(a.significand);
  return pack<real>(bits(sign) & format<real>::sign, b.exponent - 112 + leading + 16383, a.significand << (115 - leading));
}

inline constexpr real
remainder(real x, real y) noexcept
{
  if ( isnan(x) || isnan(y) || isinf(x) || y == 0 ) return nan<real>();
  if ( isinf(y) ) return x;
  real r = fmod(x, y), ay = abs(y), ar = abs(r);
  // Above half the finite range the truncating quotient can only be zero or one.
  const bool odd = ay > 0x1.ffffffffffffffffffffffffffffp16382L ? abs(x) >= ay : abs(fmod(x, 2 * ay)) >= ay;
  if ( ar > ay - ar || (ar == ay - ar && odd) ) r -= copysign(ay, x);
  return r;
}
};      // namespace micron::wasm::math
