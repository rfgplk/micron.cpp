//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

#include "../algorithm/algorithm.hpp"
#include "../algorithm/arith.hpp"
#include "../algorithm/math.hpp"

namespace micron
{
namespace parallel
{

template<class It, class P>
[[nodiscard]] micron::task<void>
fill(It __first, It __last, P __value)
{
  auto __leaf = [__value](It __a, It __b) { micron::fill(__a, __b, __value); };
  return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));
}

template<class It, class Fn>
[[nodiscard]] micron::task<void>
generate(It __first, It __last, Fn __fn)
{
  auto __leaf = [__fn](It __a, It __b) {
    for ( ; __a != __b; ++__a ) *__a = __fn();
  };
  return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));
}

// in place; maps the first half, swaps symmetric elements
template<class It>
[[nodiscard]] micron::task<void>
reverse(It __first, It __last)
{
  const usize __n = static_cast<usize>(__last - __first);
  It __half = __first + __n / 2;
  auto __leaf = [__first, __n](It __a, It __b) {
    for ( It __p = __a; __p != __b; ++__p ) {
      const usize __i = static_cast<usize>(__p - __first);
      It __m = __first + (__n - 1u - __i);
      auto __t = *__p;
      *__p = *__m;
      *__m = __t;
    }
  };
  return __pmap<It, decltype(__leaf)>(__first, __half, __leaf, __grain_for(__n / 2u));
}

// out[i] = in[n-1-i]
template<class It, class Out>
[[nodiscard]] micron::task<void>
reverse_copy(It __first, It __last, Out __out)
{
  const usize __n = static_cast<usize>(__last - __first);
  auto __leaf = [__first, __out, __n](It __a, It __b) {
    for ( It __p = __a; __p != __b; ++__p ) {
      const usize __i = static_cast<usize>(__p - __first);
      __out[__i] = *(__first + (__n - 1u - __i));
    }
  };
  return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(__n));
}

// in place; leaf reuses serial SIMD range form
#define MICRON_PAR_RANGE_INPLACE(NAME)                                                                                                     \
  template<class It> [[nodiscard]] micron::task<void> NAME(It __first, It __last)                                                          \
  {                                                                                                                                        \
    auto __leaf = [](It __a, It __b) { micron::NAME(__a, __b); };                                                                          \
    return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));                       \
  }
MICRON_PAR_RANGE_INPLACE(round)
MICRON_PAR_RANGE_INPLACE(ceil)
MICRON_PAR_RANGE_INPLACE(floor)
#undef MICRON_PAR_RANGE_INPLACE

// in place; *p = math::FN((double)*p)
#define MICRON_PAR_MATH1(NAME)                                                                                                             \
  template<class It> [[nodiscard]] micron::task<void> NAME(It __first, It __last)                                                          \
  {                                                                                                                                        \
    auto __leaf = [](It __a, It __b) {                                                                                                     \
      for ( ; __a != __b; ++__a )                                                                                                          \
          *__a = static_cast<micron::remove_cvref_t<decltype(*__a)>>(micron::math::NAME(static_cast<double>(*__a)));                       \
    };                                                                                                                                     \
    return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));                       \
  }
MICRON_PAR_MATH1(sin)
MICRON_PAR_MATH1(cos)
MICRON_PAR_MATH1(tan)
MICRON_PAR_MATH1(asin)
MICRON_PAR_MATH1(acos)
MICRON_PAR_MATH1(atan)
MICRON_PAR_MATH1(sinh)
MICRON_PAR_MATH1(cosh)
MICRON_PAR_MATH1(tanh)
MICRON_PAR_MATH1(exp)
MICRON_PAR_MATH1(log)
MICRON_PAR_MATH1(log10)
MICRON_PAR_MATH1(sqrt)
MICRON_PAR_MATH1(cbrt)
#undef MICRON_PAR_MATH1

#define MICRON_PAR_MATH_EXPR(NAME, EXPR)                                                                                                   \
  template<class It> [[nodiscard]] micron::task<void> NAME(It __first, It __last)                                                          \
  {                                                                                                                                        \
    auto __leaf = [](It __a, It __b) {                                                                                                     \
      for ( ; __a != __b; ++__a ) {                                                                                                        \
        const double __x = static_cast<double>(*__a);                                                                                      \
        *__a = static_cast<micron::remove_cvref_t<decltype(*__a)>>(EXPR);                                                                  \
      }                                                                                                                                    \
    };                                                                                                                                     \
    return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));                       \
  }
MICRON_PAR_MATH_EXPR(asinh, micron::math::log(__x + micron::math::sqrt(__x * __x + 1.0)))
MICRON_PAR_MATH_EXPR(acosh, micron::math::log(__x + micron::math::sqrt(__x * __x - 1.0)))
MICRON_PAR_MATH_EXPR(atanh, 0.5 * micron::math::log((1.0 + __x) / (1.0 - __x)))
#undef MICRON_PAR_MATH_EXPR

#define MICRON_PAR_ARITH1(NAME)                                                                                                            \
  template<class It, class Y> [[nodiscard]] micron::task<void> NAME(It __first, It __last, Y __y)                                          \
  {                                                                                                                                        \
    auto __leaf = [__y](It __a, It __b) { micron::NAME(__a, __b, __y); };                                                                  \
    return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));                       \
  }
MICRON_PAR_ARITH1(add)
MICRON_PAR_ARITH1(multiply)
MICRON_PAR_ARITH1(divide)
MICRON_PAR_ARITH1(subtract)
MICRON_PAR_ARITH1(pow)
#undef MICRON_PAR_ARITH1

};      // namespace parallel
};      // namespace micron
