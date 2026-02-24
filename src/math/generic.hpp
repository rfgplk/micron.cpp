//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../endian.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

namespace micron
{
namespace math
{

template <typename T>
  requires(micron::is_integral_v<T>)
constexpr T
gcd(T a, T b)
{
  using U = micron::make_unsigned_t<T>;

  if ( a == 0 )
    return b < 0 ? -b : b;
  if ( b == 0 )
    return a < 0 ? -a : a;

  U ua = a < 0 ? U(-a) : U(a);
  U ub = b < 0 ? U(-b) : U(b);

  const int i = countr_zero(ua);
  ua >>= i;

  const int j = countr_zero(ub);
  ub >>= j;

  const int k = i < j ? i : j;

  for ( ;; ) {
    if ( ua > ub ) {
      U tmp = ua;
      ua = ub;
      ub = tmp;
    }
    ub -= ua;
    if ( !ub )
      return T(ua << k);
    ub >>= countr_zero(ub);
  }
}

template <typename T>
  requires(micron::is_integral_v<T>)
constexpr T
abs(T v)
{
  using U = micron::make_unsigned_t<T>;
  if constexpr ( micron::is_signed_v<T> )
    return v < 0 ? T(U(-(U)v)) : v;
  else
    return v;
}

template <typename T>
  requires(micron::is_integral_v<T>)
constexpr int
digits(T x)
{
  using U = micron::make_unsigned_t<T>;
  U v = x < 0 ? U(-x) : U(x);

  if ( v == 0 )
    return 1;

  int count = 0;
  while ( v ) {
    v /= 10;
    ++count;
  }
  return count;
}

template <typename T>
int
__digits(T x)
{
  x = abs(x);
  if ( x < (T)1e1 )
    return 1;
  if ( x < (T)1e2 )
    return 2;
  if ( x < (T)1e3 )
    return 3;
  if ( x < (T)1e4 )
    return 4;
  if ( x < (T)1e5 )
    return 5;
  if ( x < (T)1e6 )
    return 6;
  if ( x < (T)1e7 )
    return 7;
  if ( x < (T)1e8 )
    return 8;
  if ( x < (T)1e9 )
    return 9;
  // only for larger types
  if constexpr ( sizeof(T) > 4 ) {
    if ( x < (T)1e10 )
      return 10;
    if ( x < (T)1e11 )
      return 11;
    if ( x < (T)1e12 )
      return 12;
    if ( x < (T)1e13 )
      return 13;
    if ( x < (T)1e14 )
      return 14;
    if ( x < (T)1e15 )
      return 15;
    if ( x < (T)1e16 )
      return 16;
    if ( x < (T)1e17 )
      return 17;
    if ( x < (T)1e18 )
      return 18;
    if ( x < (T)1e19 )
      return 19;
    if ( x < (T)1e20 )
      return 20;
  }
  return 0;
}

template <typename T, typename F>
  requires(micron::is_integral_v<T> && micron::is_integral_v<F>)
constexpr T
power_loop(T x, F p)
{
  if ( p < 0 )
    return 0;

  T r = 1;
  for ( ; p > 0; --p )
    r *= x;
  return r;
}

template <typename T, typename F>
  requires(micron::is_integral_v<T> && micron::is_integral_v<F>)
constexpr T
power(T x, F p)
{
  if ( p < 0 )
    return 0;

  if ( p == 0 )
    return 1;

  if ( p % 2 == 0 ) {
    T t = power(x, p / 2);
    return t * t;
  } else {
    T n = power(x, p / 2);
    return x * n * n;
  }
}

template <typename T>
  requires(micron::is_integral_v<T>)
constexpr T
pow(T base, i32 exp)
{
  if ( exp < 0 )
    return T(0);

  T result = 1;
  while ( exp > 0 ) {
    if ( exp & 1 )
      result *= base;
    base *= base;
    exp >>= 1;
  }
  return result;
}

// due to the extensive complexity of implementing ieee754 fully
// (arch-specific), builtins will handle this, until i get around to
// porting it in full
static constexpr double huge = 1.0e300, tiny = 1.0e-300;

constexpr f32
powerf32(f32 x, f32 y)
{
  return __builtin_powf(x, y);
}

constexpr f64
powerf(f64 x, f64 y)
{
  return __builtin_pow(x, y);
}

constexpr flong
powerflong(flong x, flong y)
{
  return __builtin_powl(x, y);
}

constexpr float
fpow(float x, float y) noexcept
{
  return powerf32(x, y);
}

constexpr double
fpow(double x, double y) noexcept
{
  return powerf(x, y);
}

constexpr long double
fpow(long double x, long double y) noexcept
{
  return powerflong(x, y);
}

constexpr f64
expf64(f64 x)
{
  return __builtin_exp(x);
}

constexpr f32
expf32(f32 x)
{
  return __builtin_expf(x);
}

constexpr flong
expf128(flong x)
{
  return __builtin_expl(x);
}

constexpr float
fexp(float x) noexcept
{
  return expf32(x);
}

constexpr double
fexp(double x) noexcept
{
  return expf64(x);
}

constexpr long double
fexp(long double x) noexcept
{
  return expf128(x);
}

constexpr f32
logf32(f32 x)
{
  return __builtin_logf(x);
}

constexpr f64
logf64(f64 x)
{
  return __builtin_log(x);
}

constexpr flong
logf128(flong x)
{
  return __builtin_logl(x);
}

constexpr f32
roundf32(f32 x)
{
  return __builtin_roundf(x);
}

constexpr f64
roundf64(f64 x)
{
  return __builtin_round(x);
}

constexpr flong
roundf128(flong x)
{
  return __builtin_roundl(x);
}

constexpr f32
maxf32(f32 x, f32 y)
{
  return __builtin_fmaxf(x, y);
}

constexpr f64
maxf64(f64 x, f64 y)
{
  return __builtin_fmax(x, y);
}

constexpr flong
maxf128(flong x, flong y)
{
  return __builtin_fmaxl(x, y);
}

template <typename T>
  requires(micron::is_floating_point_v<T>)
constexpr T
fmax(T x, T y)
{
  if constexpr ( micron::same_as<T, f32> ) {
    return maxf(x, y);
  }
  if constexpr ( micron::same_as<T, f64> ) {
    return maxf64(x, y);
  }
  if constexpr ( micron::same_as<T, f128> ) {
    return maxf128(x, y);
  }
}

template <typename T>
  requires is_floating_point_v<T>
constexpr T
fmin(T a, T b)
{
  return (a < b) ? a : b;
}

// NOTE: in case the compiler can't constant fold these at ct, the log10 func from GLIBC is needed, compile with -lm if
// that's the case
constexpr float
flog2(float x) noexcept
{
  return __builtin_log2f(x);
}

constexpr double
flog2(double x) noexcept
{
  return __builtin_log2(x);
}

constexpr long double
flog2(long double x) noexcept
{
  return __builtin_log2l(x);
}

constexpr i32
log2(i32 x)
{
  if ( x <= 0 )
    return -1;
  return 31 - __builtin_clz(x);
}

constexpr i64
log2ll(i64 x)
{
  if ( x <= 0 )
    return -1;
  return (i64)63 - (i64)__builtin_clzll(x);
}

constexpr f32
log10f32(f32 x)
{
  return __builtin_log10f(x);
}

constexpr f64
log10f64(f64 x)
{
  return __builtin_log10(x);
}

constexpr flong
log10f128(flong x)
{
  return __builtin_log10l(x);
}

constexpr f64
remainderf64(f64 x, f64 y)
{
  return __builtin_remainder(x, y);
}

constexpr flong
remainderf128(flong x, flong y)
{
  return __builtin_remainderl(x, y);
}

constexpr flong
remainderf128(flong x, f32 y)
{
  return __builtin_remainderl(x, y);
}

template <typename T>
  requires(micron::is_integral_v<T>)
constexpr T
nearest_pow2(T x)
{
  if ( x <= 0 )
    return 1;

  u32 v = (u32)x;

  int l = 31 - __builtin_clz(v);
  u32 lower = 1U << l;
  u32 upper = 1U << (l + 1);

  return (v - lower <= upper - v) ? T(lower) : T(upper);
}

template <typename T>
  requires(micron::is_integral_v<T>)
constexpr T
nearest_pow2ll(T x)
{
  if ( x <= 0 )
    return 1;

  u64 v = (u64)x;

  int l = 63 - __builtin_clzll(v);
  u64 lower = 1ULL << l;
  u64 upper = 1ULL << (l + 1);

  return (v - lower <= upper - v) ? T(lower) : T(upper);
}

template <typename T>
  requires(micron::is_integral_v<T>)
T
__nearest_pow2ll(T x)
{
  return x == 0 ? 1u
                : (((x) - (1u << ((8 * sizeof(x) - 1) - __builtin_clzll(x)))) <= ((1u << ((8 * sizeof(x)) - __builtin_clzll(x))) - (x))
                       ? (1u << ((8 * sizeof(x) - 1) - __builtin_clzll(x)))
                       : (1u << ((8 * sizeof(x)) - __builtin_clzll(x))));
}

template <typename T>
  requires(micron::is_integral_v<T>)
T
__nearest_pow2(T x)
{
  return x == 0 ? 1u
                : (((x) - (1u << ((8 * sizeof(x) - 1) - __builtin_clz(x)))) <= ((1u << ((8 * sizeof(x)) - __builtin_clz(x))) - (x))
                       ? (1u << ((8 * sizeof(x) - 1) - __builtin_clz(x)))
                       : (1u << ((8 * sizeof(x)) - __builtin_clz(x))));
}

// constexpr double powerf(double x, double y)
//{
//  note: TODO
// }

template <typename T>
  requires(micron::is_floating_point_v<T>)
constexpr T
round(T t)
{
  if ( t >= 0 )
    return T((i64)(t + T(0.5)));
  else
    return T((i64)(t - T(0.5)));
}

template <typename T>
  requires(micron::is_floating_point_v<T>)
constexpr T
ceil(T s)
{
  i64 i = (i64)s;
  if ( s > T(i) )
    return T(i + 1);
  return T(i);
}

template <typename T>
  requires(micron::is_floating_point_v<T>)
constexpr T
floor(T s)
{
  i64 i = (i64)s;
  if ( s < T(i) )
    return T(i - 1);
  return T(i);
}

template <typename R, typename T>
  requires(micron::is_convertible_v<R, T> and micron::is_floating_point_v<T>)
constexpr R
round(T t)
{
  if ( t >= 0 )
    return static_cast<R>(T((i64)(t + T(0.5))));
  else
    return static_cast<R>(T((i64)(t - T(0.5))));
}

template <typename R, typename T>
  requires(micron::is_convertible_v<R, T> and micron::is_floating_point_v<T>)
constexpr R
ceil(T s)
{
  i64 i = (i64)s;
  if ( s > T(i) )
    return static_cast<R>((i + 1));
  return static_cast<R>(T(i));
}

template <typename R, typename T>
  requires(micron::is_convertible_v<R, T> and micron::is_floating_point_v<T>)
constexpr R
floor(T s)
{
  i64 i = (i64)s;
  if ( s < T(i) )
    return static_cast<R>(T(i - 1));
  return static_cast<R>(T(i));
}

template <typename T>
constexpr T
fround(T x) noexcept
{
  return round(x);
}

template <typename T>
constexpr T
fceil(T x) noexcept
{
  return ceil(x);
}

template <typename T>
constexpr T
ffloor(T x) noexcept
{
  return floor(x);
}

// trunc: cast to integer, cast back (no std::trunc)
template <typename T>
constexpr T
ftrunc(T x) noexcept
{
  return static_cast<T>(static_cast<long long>(x));
}

constexpr int
isfinite(f32 x)
{
  return __builtin_isfinite(x);
}

constexpr int
isfinite(f64 x)
{
  return __builtin_isfinite(x);
}

template <typename T>
constexpr int
isign(T x) noexcept
{
  return (x > T{}) - (x < T{});
}

constexpr int
isinf(f32 x)
{
  return __builtin_isinf(x);
}

constexpr int
isinf(f64 x)
{
  return __builtin_isinf(x);
}

constexpr int
isnan(f32 x)
{
  return __builtin_isnan(x);
}

constexpr int
isnan(f64 x)
{
  return __builtin_isnan(x);
}

constexpr int
isnormal(f32 x)
{
  return __builtin_isnormal(x);
}

constexpr int
isnormal(f64 x)
{
  return __builtin_isnormal(x);
}

constexpr int
signbit(f32 x)
{
  return __builtin_signbit(x);
}

constexpr int
signbit(f64 x)
{
  return __builtin_signbit(x);
}

constexpr int
isgreater(f32 a, f32 b)
{
  return __builtin_isgreater(a, b);
}

constexpr int
isgreater(f64 a, f64 b)
{
  return __builtin_isgreater(a, b);
}

constexpr int
isgreaterequal(f32 a, f32 b)
{
  return __builtin_isgreaterequal(a, b);
}

constexpr int
isgreaterequal(f64 a, f64 b)
{
  return __builtin_isgreaterequal(a, b);
}

constexpr int
isless(f32 a, f32 b)
{
  return __builtin_isless(a, b);
}

constexpr int
isless(f64 a, f64 b)
{
  return __builtin_isless(a, b);
}

constexpr int
islessequal(f32 a, f32 b)
{
  return __builtin_islessequal(a, b);
}

constexpr int
islessequal(f64 a, f64 b)
{
  return __builtin_islessequal(a, b);
}

constexpr int
islessgreater(f32 a, f32 b)
{
  return __builtin_islessgreater(a, b);
}

constexpr int
islessgreater(f64 a, f64 b)
{
  return __builtin_islessgreater(a, b);
}

constexpr int
isunordered(f32 a, f32 b)
{
  return __builtin_isunordered(a, b);
}

constexpr int
isunordered(f64 a, f64 b)
{
  return __builtin_isunordered(a, b);
}

constexpr f32
rint(f32 x)
{
  return __builtin_rintf(x);
}

constexpr f64
rint(f64 x)
{
  return __builtin_rint(x);
}

constexpr long
lrint(f32 x)
{
  return __builtin_lrintf(x);
}

constexpr long
lrint(f64 x)
{
  return __builtin_lrint(x);
}

constexpr long long
llrint(f32 x)
{
  return __builtin_llrintf(x);
}

constexpr long long
llrint(f64 x)
{
  return __builtin_llrint(x);
}

constexpr f32
nearbyint(f32 x)
{
  return __builtin_nearbyintf(x);
}

constexpr f64
nearbyint(f64 x)
{
  return __builtin_nearbyint(x);
}

constexpr f32
scalbn(f32 x, int n)
{
  return __builtin_scalbnf(x, n);
}

constexpr f64
scalbn(f64 x, int n)
{
  return __builtin_scalbn(x, n);
}

constexpr f32
scalbln(f32 x, long n)
{
  return __builtin_scalblnf(x, n);
}

constexpr f64
scalbln(f64 x, long n)
{
  return __builtin_scalbln(x, n);
}

constexpr f32
copysign(f32 x, f32 y)
{
  return __builtin_copysignf(x, y);
}

constexpr f64
copysign(f64 x, f64 y)
{
  return __builtin_copysign(x, y);
}

constexpr f32
fabs(f32 x)
{
  return __builtin_fabsf(x);
}

constexpr f64
fabs(f64 x)
{
  return __builtin_fabs(x);
}

template <typename T>
constexpr T
fabsmax(T a, T b)
{
  return fmax(fabs(a), fabs(b));
}

constexpr f32
remainder(f32 x, f32 y)
{
  return __builtin_remainderf(x, y);
}

constexpr f64
remainder(f64 x, f64 y)
{
  return __builtin_remainder(x, y);
}

inline f32
remquo(f32 x, f32 y, int *q)
{
  return __builtin_remquof(x, y, q);
}

inline f64
remquo(f64 x, f64 y, int *q)
{
  return __builtin_remquo(x, y, q);
}

inline f32
nanf(const char *tag)
{
  return __builtin_nanf(tag);
}

inline f64
nan(const char *tag)
{
  return __builtin_nan(tag);
}

constexpr i32
abs(i32 x)
{
  return __builtin_abs(x);
}

constexpr long
labs(long x)
{
  return __builtin_labs(x);
}

constexpr long long
llabs(long long x)
{
  return __builtin_llabs(x);
}

constexpr float
ffmaf(float a, float b, float c) noexcept
{
  return __builtin_fmaf(a, b, c);
}

constexpr double
ffma(double a, double b, double c) noexcept
{
  return __builtin_fma(a, b, c);
}

constexpr long double
ffmal(long double a, long double b, long double c) noexcept
{
  return __builtin_fmal(a, b, c);
}

template <typename T>
  requires(micron::is_floating_point_v<T>)
constexpr T
fma(T x, T y)
{
  if constexpr ( micron::same_as<T, float> ) {
    return fmaf(x, y);
  }
  if constexpr ( micron::same_as<T, double> ) {
    return ffma(x, y);
  }
  if constexpr ( micron::same_as<T, long double> ) {
    return ffmal(x, y);
  }
}

template <typename T>
  requires is_floating_point_v<T>
constexpr T
ffract(T x)
{
  return x - static_cast<T>(static_cast<long long>(x));
}

template <typename T>
  requires is_floating_point_v<T>
constexpr T
fclamp(T x, T lo, T hi)
{
  return (x < lo) ? lo : ((x > hi) ? hi : x);
}

};     // namespace math
};     // namespace micron
