
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../math/constants.hpp"
#include "../math/generic.hpp"
#include "../math/log.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../bits/__visit_kv.hpp"
#include "../concepts.hpp"

namespace micron
{

// container agnostic functions for arith. operations on contiguous data
// T* sig. func. are blind iterators
// T& sig. func. take in a container which requires .begin and .end funcs
template<is_iterable_container T>
constexpr T &
sin(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::sin(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
cos(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::cos(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
tan(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::tan(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
asin(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::asin(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
acos(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::acos(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
atan(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::atan(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
sinh(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::sinh(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
cosh(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::cosh(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
tanh(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::tanh(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
exp(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::exp(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
log(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::log(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
log10(T &cont) noexcept
{
  // micron::math has log10f32/log10f64 but no unqualified log10. Use the
  // identity log10(x) = log(x) / log(10) via the generic log overload.
  constexpr double ln10 = 2.30258509299404568402;
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::log(static_cast<double>(*it)) / ln10;
  return cont;
}

template<is_iterable_container T>
constexpr T &
sqrt(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::sqrt(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
cbrt(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = math::cbrt(static_cast<double>(*it));
  return cont;
}

template<is_iterable_container T>
constexpr T &
absolute(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) {
    auto v = *it;
    *it = v < typename T::value_type{} ? -v : v;
  }
  return cont;
}

template<is_iterable_container T>
constexpr T &
sign(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = (*it > 0) - (*it < 0);
  return cont;
}

template<is_iterable_container T>
constexpr T &
clip(T &cont, const typename T::value_type lo, const typename T::value_type hi) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = (*it < lo) ? lo : ((*it > hi) ? hi : *it);
  return cont;
}

template<is_iterable_container T>
constexpr T &
degrees(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = *it * (180.0 / math::pi);
  return cont;
}

template<is_iterable_container T>
constexpr T &
radians(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) *it = *it * (math::pi / 180.0);
  return cont;
}

template<is_iterable_container T>
constexpr T &
asinh(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) {
    double x = static_cast<double>(*it);
    *it = math::log(x + math::sqrt(x * x + 1.0));
  }
  return cont;
}

template<is_iterable_container T>
constexpr T &
acosh(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) {
    double x = static_cast<double>(*it);
    *it = math::log(x + math::sqrt(x * x - 1.0));
  }
  return cont;
}

template<is_iterable_container T>
constexpr T &
atanh(T &cont) noexcept
{
  for ( auto *it = cont.begin(); it != cont.end(); ++it ) {
    double x = static_cast<double>(*it);
    *it = 0.5 * math::log((1.0 + x) / (1.0 - x));
  }
  return cont;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
sin(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::sin(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
cos(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::cos(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
tan(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::tan(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
asin(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::asin(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
acos(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::acos(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
atan(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::atan(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
sinh(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::sinh(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
cosh(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::cosh(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
tanh(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::tanh(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
exp(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::exp(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
log(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::log(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
log10(M &m) noexcept
{
  constexpr double ln10 = 2.30258509299404568402;
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::log(static_cast<double>(v)) / ln10; });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
sqrt(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::sqrt(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
cbrt(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = math::cbrt(static_cast<double>(v)); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
absolute(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) {
    auto x = v;
    v = x < typename M::mapped_type{} ? -x : x;
  });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
sign(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = (v > 0) - (v < 0); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
clip(M &m, const typename M::mapped_type lo, const typename M::mapped_type hi) noexcept
{
  __impl::visit_kv(m, [&](const auto &, auto &v) { v = (v < lo) ? lo : ((v > hi) ? hi : v); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
degrees(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = v * (180.0 / math::pi); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
radians(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) { v = v * (math::pi / 180.0); });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
asinh(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) {
    double x = static_cast<double>(v);
    v = math::log(x + math::sqrt(x * x + 1.0));
  });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
acosh(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) {
    double x = static_cast<double>(v);
    v = math::log(x + math::sqrt(x * x - 1.0));
  });
  return m;
}

template<is_mutable_map M>
  requires micron::is_arithmetic_v<typename M::mapped_type>
constexpr M &
atanh(M &m) noexcept
{
  __impl::visit_kv(m, [](const auto &, auto &v) {
    double x = static_cast<double>(v);
    v = 0.5 * math::log((1.0 + x) / (1.0 - x));
  });
  return m;
}

// EXPERIMENTAL - DERIVATIVES, MIGHT REMOVE
// TODO: EXPAND OR REMOVE

template<typename F, typename T>
constexpr T
derivative(F f, T x, T h = T(1e-6)) noexcept
{
  return (f(x + h) - f(x - h)) / (T(2) * h);
}

// gradient of function f: R^n -> R
template<typename F, typename V>
V
jacobian(F f, const V &x, typename V::value_type h = typename V::value_type(1e-6)) noexcept
{
  using T = typename V::value_type;

  V grad = x;
  const usize n = x.size();

  for ( usize i = 0; i < n; ++i ) {
    V xp = x;
    V xm = x;

    xp[i] += h;
    xm[i] -= h;

    grad[i] = (f(xp) - f(xm)) / (T(2) * h);
  }

  return grad;
}

// hessian of function f: R^n -> R
template<typename F, typename V>
auto
hessian(F f, const V &x, typename V::value_type h = typename V::value_type(1e-5)) noexcept
{
  using T = typename V::value_type;
  const usize n = x.size();

  typename V::template rebind<V>::type H(n, V(n));

  for ( usize i = 0; i < n; ++i ) {
    for ( usize j = 0; j < n; ++j ) {

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
}      // namespace micron
