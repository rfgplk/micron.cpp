//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

#include "../vector.hpp"

namespace micron
{
namespace parallel
{

// out[i] = op(in[0], in[1], ..., in[i]
template<class In, class Out, class Op>
micron::task<void>
__pscan_incl(In __in, Out __out, usize __n, Op __op)
{
  using T = micron::remove_cvref_t<decltype(*__in)>;
  if ( __n == 0 ) co_return;
  const usize __B = __grain_for(__n);
  const usize __nb = (__n + __B - 1u) / __B;
  if ( __nb <= 1u ) {      // single block: plain serial scan
    T __acc = __in[0];
    __out[0] = __acc;
    for ( usize __i = 1; __i < __n; ++__i ) {
      __acc = __op(__acc, __in[__i]);
      __out[__i] = __acc;
    }
    co_return;
  }

  micron::vector<T> __bs(__nb);
  T *__block = &__bs[0];
  {
    auto __body = [__in, __n, __B, __block, __op](usize __b) {
      const usize __s = __b * __B;
      const usize __e = (__s + __B < __n) ? __s + __B : __n;
      T __acc = __in[__s];
      for ( usize __i = __s + 1; __i < __e; ++__i ) __acc = __op(__acc, __in[__i]);
      __block[__b] = __acc;
    };
    co_await __pblocks<decltype(__body)>(0, __nb, __body, 1);
  }
  micron::vector<T> __cr(__nb);
  T *__carry = &__cr[0];
  {
    T __acc = __block[0];
    if ( __nb >= 2u ) __carry[1] = __acc;
    for ( usize __b = 2; __b < __nb; ++__b ) {
      __acc = __op(__acc, __block[__b - 1u]);
      __carry[__b] = __acc;
    }
  }
  {
    auto __body = [__in, __out, __n, __B, __carry, __op](usize __b) {
      const usize __s = __b * __B;
      const usize __e = (__s + __B < __n) ? __s + __B : __n;
      T __acc = (__b == 0) ? T(__in[__s]) : __op(__carry[__b], __in[__s]);
      __out[__s] = __acc;
      for ( usize __i = __s + 1; __i < __e; ++__i ) {
        __acc = __op(__acc, __in[__i]);
        __out[__i] = __acc;
      }
    };
    co_await __pblocks<decltype(__body)>(0, __nb, __body, 1);
  }
}

template<class It, class Out, class Op>
[[nodiscard]] micron::task<void>
scan(It __first, It __last, Out __out, Op __op)
{
  return __pscan_incl<It, Out, Op>(__first, __out, static_cast<usize>(__last - __first), __op);
}

template<class It, class Out, class T, class Op>
[[nodiscard]] micron::task<void>
scanl(It __first, It __last, Out __out, T __init, Op __op)
{
  const usize __n = static_cast<usize>(__last - __first);
  __out[0] = __init;
  if ( __n == 0 ) co_return;
  co_await __pscan_incl<It, Out, Op>(__first, __out + 1, __n, __op);      // out[1+i] = op-fold(in[0..i])
  auto __seed = __init;
  auto __leaf = [__seed, __op](Out __a, Out __b) {
    for ( ; __a != __b; ++__a ) *__a = __op(__seed, *__a);
  };
  co_await __pmap<Out, decltype(__leaf)>(__out + 1, __out + 1 + __n, __leaf, __grain_for(__n));
}

};      // namespace parallel
};      // namespace micron
