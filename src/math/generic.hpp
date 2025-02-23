#pragma once

#include "../bits.hpp"
#include "../endian.hpp"
#include "../fp.hpp"
#include "../types.hpp"
#include <type_traits>

namespace micron
{
namespace math
{

template <typename T>
constexpr T
gcd(T a, T b)
{
  if ( a == 0 )
    return b;
  if ( b == 0 )
    return a;
  const int i = countr_zero(a);
  a >>= i;
  const int j = countr_zero(b);
  b >>= j;
  const int k = i < j ? i : j;

  for ( ;; ) {
    if ( a > b ) {
      T tmp = a;
      a = b;
      b = tmp;     // swap
    }
    b -= a;
    if ( !b )
      return a << k;
    b >>= countr_zero(b);
  }
}

template <typename T>
T
abs(T i)
{
  return i < 0 ? -i : i;
}

template <typename T>
int
digits(T x)
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
  requires(std::is_integral_v<T>) && (std::is_integral_v<F>)
constexpr T power_loop(T x, F p)
{
  if ( p == 0 )
    return 1;
  T r = 1;
  for ( ; p > 0; p-- )
    r *= x;
  return r;
}
template <typename T, typename F>
  requires(std::is_integral_v<T>) && (std::is_integral_v<F>)
constexpr T power(T x, F p)
{
  if ( p == 0 )
    return 1;
  else if ( p % 2 == 0 ) {
    auto t = power(x, p / 2);
    return t * t;
  } else {
    auto n = power(x, p / 2);
    return x * n * n;
  }
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
constexpr f32
remainderf32(f32 x, f32 y)
{
  return __builtin_remainderf(x, y);
}
constexpr f64
remainderf64(f64 x, f32 y)
{
  return __builtin_remainder(x, y);
}
constexpr flong
remainderf128(flong x, f32 y)
{
  return __builtin_remainderl(x, y);
}
// constexpr double powerf(double x, double y)
//{
//  note: TODO
// }

template <typename T>
T
round(T t)
  requires(std::is_floating_point_v<T>)
{
  return static_cast<T>(static_cast<ssize_t>(t + 0.5));
}

template <typename T>
T
ceil(T s)
  requires(std::is_floating_point_v<T>)
{
  ssize_t i = static_cast<ssize_t>(s);
  return (s > i) ? static_cast<T>(i + 1) : static_cast<T>(i);
}
template <typename T>
T
floor(T s)
  requires(std::is_floating_point_v<T>)
{
  return static_cast<T>(static_cast<ssize_t>(s));
}
};     // namespace math
};     // namespace micron
