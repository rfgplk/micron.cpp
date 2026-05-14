//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../types.hpp"
#include "bits/exp.hpp"
#include "bits/impl.hpp"
#include "bits/log.hpp"
#include "constants.hpp"
#include "generic.hpp"
#include "ieee.hpp"

namespace micron
{
namespace math
{

constexpr float
log(float x) noexcept
{
  return float(mkbits::log_ns::log<f32>(f32(x)));
}

constexpr double
log(double x) noexcept
{
  return double(mkbits::log_ns::log<f64>(f64(x)));
}

constexpr long double
log(long double x) noexcept
{
  return static_cast<long double>(mkbits::log_ns::log<f64>(f64(x)));
}

constexpr float
log10(float x) noexcept
{
  return float(mkbits::log_ns::log10<f32>(f32(x)));
}

constexpr double
log10(double x) noexcept
{
  return double(mkbits::log_ns::log10<f64>(f64(x)));
}

constexpr long double
log10(long double x) noexcept
{
  return static_cast<long double>(mkbits::log_ns::log10<f64>(f64(x)));
}

#if defined(__STDCPP_FLOAT16_T__) && defined(_GLIBCXX_FLOAT_IS_IEEE_BINARY32)
constexpr _Float16
log(_Float16 x) noexcept
{
  return _Float16(mkbits::log_ns::log<f32>(f32(x)));
}

constexpr _Float16
log10(_Float16 x) noexcept
{
  return _Float16(mkbits::log_ns::log10<f32>(f32(x)));
}
#endif

#if defined(__STDCPP_FLOAT128_T__) && defined(_GLIBCXX_HAVE_FLOAT128_MATH)
constexpr _Float128
log(_Float128 x) noexcept
{
  return _Float128(mkbits::log_ns::log<f64>(f64(x)));
}

constexpr _Float128
log10(_Float128 x) noexcept
{
  return _Float128(mkbits::log_ns::log10<f64>(f64(x)));
}
#endif

constexpr float
flog(float x) noexcept
{
  return float(mkbits::log_ns::log<f32>(f32(x)));
}

constexpr double
flog(double x) noexcept
{
  return double(mkbits::log_ns::log<f64>(f64(x)));
}

constexpr long double
flog(long double x) noexcept
{
  return static_cast<long double>(mkbits::log_ns::log<f64>(f64(x)));
}

constexpr float
flog10(float x) noexcept
{
  return float(mkbits::log_ns::log10<f32>(f32(x)));
}

constexpr double
flog10(double x) noexcept
{
  return double(mkbits::log_ns::log10<f64>(f64(x)));
}

constexpr long double
flog10(long double x) noexcept
{
  return static_cast<long double>(mkbits::log_ns::log10<f64>(f64(x)));
}

namespace log_impl
{

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log(F x) noexcept
{
  return mkbits::log_ns::log<F>(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log1p(F x) noexcept
{
  return mkbits::log_ns::log1p<F>(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp(F x) noexcept
{
  return mkbits::exp_ns::exp<F>(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
expm1(F x) noexcept
{
  return mkbits::exp_ns::expm1<F>(x);
}

};     // namespace log_impl

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log_base(F x, F b) noexcept
{
  return log_impl::log<F>(x) / log_impl::log<F>(b);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log2p1(F x) noexcept
{
  return log_impl::log1p<F>(x) * constant_log2e<F>;
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log1mexp(F x) noexcept
{
  if ( x > -constant_ln2<F> ) {
    return log_impl::log<F>(-log_impl::expm1<F>(x));
  }
  return log_impl::log1p<F>(-log_impl::exp<F>(x));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log_diff_exp(F a, F b) noexcept
{
  return a + log1mexp<F>(b - a);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log_sum_exp(F a, F b) noexcept
{
  if ( __builtin_isinf_sign(a) < 0 ) return b;
  if ( __builtin_isinf_sign(b) < 0 ) return a;
  F m = a > b ? a : b;
  return m + log_impl::log1p<F>(log_impl::exp<F>((a < b ? a : b) - m));
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
log_sum_exp_n(const F *__restrict__ v, usize N) noexcept
{
  if ( N == 0 ) return ieee::inf_v<F>(1);     // -inf for empty
  const F *const end = v + N;
  F mx = *v;
  for ( const F *p = v + 1; p != end; ++p ) {
    F vi = *p;
    if ( vi > mx ) mx = vi;
  }
  if ( __builtin_isinf_sign(mx) < 0 ) return mx;     // all -inf

  F acc0 = F(0), acc1 = F(0);
  const F *p0 = v, *p1 = v + N / 2;
  const F *e0 = v + N / 2;
  while ( p0 != e0 ) acc0 += log_impl::exp<F>(*p0++ - mx);
  while ( p1 != end ) acc1 += log_impl::exp<F>(*p1++ - mx);
  return mx + log_impl::log<F>(acc0 + acc1);
}

template <ieee754_floating F>
[[nodiscard]] inline F
log_add_exp_vec(const F *__restrict__ v, usize N) noexcept
{
  return log_sum_exp_n<F>(v, N);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
logistic(F x) noexcept
{
  if ( x >= F(0) ) {
    return F(1) / (F(1) + log_impl::exp<F>(-x));
  }
  F e = log_impl::exp<F>(x);
  return e / (F(1) + e);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log_logistic(F x) noexcept
{
  if ( x >= F(0) ) return -log_impl::log1p<F>(log_impl::exp<F>(-x));
  return x - log_impl::log1p<F>(log_impl::exp<F>(x));
}

// log(1 + exp(x)) overflow guard
template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
softplus(F x) noexcept
{
  constexpr F t = (micron::is_same_v<F, float> || micron::is_same_v<F, f32>) ? F(15) : F(35);
  if ( x > t ) return x;
  if ( x < -t ) return log_impl::exp<F>(x);
  return log_impl::log1p<F>(log_impl::exp<F>(x));
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
xlogx(F x) noexcept
{
  return x == F(0) ? F(0) : x * log_impl::log<F>(x);
}

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
xlogy(F x, F y) noexcept
{
  return x == F(0) ? F(0) : x * log_impl::log<F>(y);
}

template <ieee754_floating F>
[[gnu::flatten]] inline void
softmax(F *__restrict__ v, usize N) noexcept
{
  if ( N == 0 ) return;
  F mx = v[0];
  for ( usize i = 1; i < N; ++i )
    if ( v[i] > mx ) mx = v[i];
  F sum = F(0);
  for ( usize i = 0; i < N; ++i ) {
    v[i] = log_impl::exp<F>(v[i] - mx);
    sum += v[i];
  }
  F inv = F(1) / sum;
  for ( usize i = 0; i < N; ++i ) v[i] *= inv;
}

template <ieee754_floating F>
[[gnu::flatten]] inline void
softmax(const F *__restrict__ in, F *__restrict__ out, usize N) noexcept
{
  if ( N == 0 ) return;
  F mx = in[0];
  for ( usize i = 1; i < N; ++i )
    if ( in[i] > mx ) mx = in[i];
  F sum = F(0);
  for ( usize i = 0; i < N; ++i ) {
    out[i] = log_impl::exp<F>(in[i] - mx);
    sum += out[i];
  }
  const F inv = F(1) / sum;
  for ( usize i = 0; i < N; ++i ) out[i] *= inv;
}

template <ieee754_floating F>
[[gnu::flatten]] inline void
log_softmax(F *__restrict__ v, usize N) noexcept
{
  if ( N == 0 ) return;
  F lse = log_sum_exp_n<F>(v, N);
  for ( usize i = 0; i < N; ++i ) v[i] -= lse;
}

template <ieee754_floating F>
[[gnu::flatten]] inline void
log_softmax(const F *__restrict__ in, F *__restrict__ out, usize N) noexcept
{
  if ( N == 0 ) return;
  const F lse = log_sum_exp_n<F>(in, N);
  for ( usize i = 0; i < N; ++i ) out[i] = in[i] - lse;
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
softmax(C &c) noexcept
{
  softmax<typename C::value_type>(c.begin(), c.size());
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
softmax(const C &in, C &out) noexcept
{
  softmax<typename C::value_type>(in.cbegin(), out.begin(), in.size());
}

template <typename... Cs>
  requires(sizeof...(Cs) >= 2) && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<Cs> && ...)
          && (ieee754_floating<typename Cs::value_type> && ...)
[[gnu::flatten]] inline void
softmax(Cs &...cs) noexcept
{
  ((softmax<typename Cs::value_type>(cs.begin(), cs.size())), ...);
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
log_softmax(C &c) noexcept
{
  log_softmax<typename C::value_type>(c.begin(), c.size());
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
log_softmax(const C &in, C &out) noexcept
{
  log_softmax<typename C::value_type>(in.cbegin(), out.begin(), in.size());
}

template <typename... Cs>
  requires(sizeof...(Cs) >= 2) && (is_iterable_container<Cs> && ...) && (!micron::is_const_v<Cs> && ...)
          && (ieee754_floating<typename Cs::value_type> && ...)
[[gnu::flatten]] inline void
log_softmax(Cs &...cs) noexcept
{
  ((log_softmax<typename Cs::value_type>(cs.begin(), cs.size())), ...);
}

};     // namespace math
};     // namespace micron
