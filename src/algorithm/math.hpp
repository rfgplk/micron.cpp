
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../math/constants.hpp"
#include "../math/generic.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../concepts.hpp"
namespace micron
{

// container agnostic functions for arith. operations on contiguous data
// T* sig. func. are blind iterators
// T& sig. func. take in a container which requires .begin and .end funcs
template <is_iterable_container T>
constexpr T &
sin(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::sin(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
cos(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::cos(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
tan(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::tan(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
asin(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::asin(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
acos(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::acos(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
atan(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::atan(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
sinh(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::sinh(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
cosh(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::cosh(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
tanh(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::tanh(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
exp(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::exp(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
log(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::log(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
log10(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::log10(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
sqrt(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::sqrt(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
cbrt(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::cbrt(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
absolute(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::abs(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
sign(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = (*it > 0) - (*it < 0);
  return cont;
}

template <is_iterable_container T>
constexpr T &
clip(T &cont, const typename T::value_type lo, const typename T::value_type hi)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = (*it < lo) ? lo : ((*it > hi) ? hi : *it);
  return cont;
}

template <is_iterable_container T>
constexpr T &
degrees(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = *it * (180.0 / math::pi);
  return cont;
}

template <is_iterable_container T>
constexpr T &
radians(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = *it * (math::pi / 180.0);
  return cont;
}

template <is_iterable_container T>
constexpr T &
asinh(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::asinh(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
acosh(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::acosh(static_cast<double>(*it));
  return cont;
}

template <is_iterable_container T>
constexpr T &
atanh(T &cont)
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it )
    *it = math::atanh(static_cast<double>(*it));
  return cont;
}

// EXPERIMENTAL - DERIVATIVES, MIGHT REMOVE
// TODO: EXPAND OR REMOVE

template <typename F, typename T>
constexpr T
derivative(F f, T x, T h = T(1e-6))
{
  return (f(x + h) - f(x - h)) / (T(2) * h);
}

// gradient of function f: R^n -> R
template <typename F, typename V>
V
jacobian(F f, const V &x, typename V::value_type h = typename V::value_type(1e-6))
{
  using T = typename V::value_type;

  V grad = x;
  const size_t n = x.size();

  for ( size_t i = 0; i < n; ++i ) {
    V xp = x;
    V xm = x;

    xp[i] += h;
    xm[i] -= h;

    grad[i] = (f(xp) - f(xm)) / (T(2) * h);
  }

  return grad;
}

// hessian of function f: R^n -> R
template <typename F, typename V>
auto
hessian(F f, const V &x, typename V::value_type h = typename V::value_type(1e-5))
{
  using T = typename V::value_type;
  const size_t n = x.size();

  typename V::template rebind<V>::type H(n, V(n));

  for ( size_t i = 0; i < n; ++i ) {
    for ( size_t j = 0; j < n; ++j ) {

      V xpp = x;
      V xpm = x;
      V xmp = x;
      V xmm = x;

      xpp[i] += h;
      xpp[j] += h;
      xpm[i] += h;
      xpm[j] -= h;
      xmp[i] -= h;
      xmp[j] += h;
      xmm[i] -= h;
      xmm[j] -= h;

      H[i][j] = (f(xpp) - f(xpm) - f(xmp) + f(xmm)) / (T(4) * h * h);
    }
  }

  return H;
}
}     // namespace micron
