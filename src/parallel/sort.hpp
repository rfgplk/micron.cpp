//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

#include "../vector.hpp"

#include "../sort/merge.hpp"
#include "../sort/quick.hpp"

namespace micron
{
namespace parallel
{
namespace sort
{

struct __pless {
  template<class A, class B>
  bool
  operator()(const A &__a, const B &__b) const
  {
    return __a < __b;
  }
};

template<class T, class Cmp>
micron::task<void>
__pmergesort(T *__arr, max_t __lo, max_t __hi, Cmp __comp, usize __grain)
{
  const usize __n = static_cast<usize>(__hi - __lo + 1);
  if ( __n <= __grain ) {
    micron::sort::__merge_sort(__arr, __lo, __hi, __comp);      // serial stable leaf
    co_return;
  }
  const max_t __mid = __lo + (__hi - __lo) / 2;
  co_await micron::coro::fork(micron::coro::discard, __pmergesort<T, Cmp>)(__arr, __lo, __mid, __comp, __grain);
  co_await micron::coro::call(__pmergesort<T, Cmp>, __arr, __mid + 1, __hi, __comp, __grain);
  co_await micron::coro::join;
  micron::sort::__merge(__arr, __lo, __mid, __hi, __comp);      // serial merge of the two sorted halves
}

// merge (stable merge sort)
template<class It, class Cmp = __pless>
[[nodiscard]] micron::task<void>
merge(It __first, It __last, Cmp __comp = Cmp{})
{
  using T = micron::remove_cvref_t<decltype(*__first)>;
  const usize __n = static_cast<usize>(__last - __first);
  if ( __n < 2 ) co_return;
  co_await __pmergesort<T, Cmp>(__first, 0, static_cast<max_t>(__n) - 1, __comp, __grain_for(__n));
}

template<class It, class Cmp = __pless>
[[nodiscard]] micron::task<void>
stable(It __first, It __last, Cmp __comp = Cmp{})
{
  return merge<It, Cmp>(__first, __last, __comp);
}

template<class T, class Cmp>
micron::task<void>
__pquicksort(T *__arr, max_t __lo, max_t __hi, max_t __depth, Cmp __comp, usize __grain)
{
  if ( __lo >= __hi ) co_return;      // 0 or 1 element
  const usize __n = static_cast<usize>(__hi - __lo + 1);
  if ( __n <= __grain || __depth <= 0 ) {
    micron::sort::__merge_sort(__arr, __lo, __hi, __comp);      // serial leaf (complete + stable)
    co_return;
  }
  micron::sort::__q_med3_to_high(__arr, __lo, __hi, __comp);      // median-of-three pivot -> __arr[hi]
  const max_t __p = micron::sort::__q_partition(__arr, __lo, __hi, __comp);
  co_await micron::coro::fork(micron::coro::discard, __pquicksort<T, Cmp>)(__arr, __lo, __p - 1, __depth - 1, __comp, __grain);
  co_await micron::coro::call(__pquicksort<T, Cmp>, __arr, __p + 1, __hi, __depth - 1, __comp, __grain);
  co_await micron::coro::join;
}

template<class It, class Cmp = __pless>
[[nodiscard]] micron::task<void>
quick(It __first, It __last, Cmp __comp = Cmp{})
{
  using T = micron::remove_cvref_t<decltype(*__first)>;
  const usize __n = static_cast<usize>(__last - __first);
  if ( __n < 2 ) co_return;
  max_t __depth = 0;
  for ( max_t __t = static_cast<max_t>(__n); __t > 1; __t >>= 1 ) __depth += 2;      // 2 * floor(log2 n)
  co_await __pquicksort<T, Cmp>(__first, 0, static_cast<max_t>(__n) - 1, __depth, __comp, __grain_for(__n));
}

template<class It, class Cmp = __pless>
[[nodiscard]] micron::task<void>
sort(It __first, It __last, Cmp __comp = Cmp{})
{
  return quick<It, Cmp>(__first, __last, __comp);
}

template<class It, class KeyFn>
micron::task<void>
__pradix(It __arr, usize __n, KeyFn __key)
{
  using T = micron::remove_cvref_t<decltype(*__arr)>;
  using K = micron::remove_cvref_t<decltype(__key(*__arr))>;
  if ( __n < 2 ) co_return;

  K __kmin = __key(__arr[0]), __kmax = __key(__arr[0]);      // serial min/max (one pass)
  for ( usize __i = 1; __i < __n; ++__i ) {
    K __k = __key(__arr[__i]);
    if ( __k < __kmin ) __kmin = __k;
    if ( __kmax < __k ) __kmax = __k;
  }
  const umax_t __bias = static_cast<umax_t>(__kmin);
  const umax_t __span = static_cast<umax_t>(__kmax) - static_cast<umax_t>(__kmin);

  const usize __B = __grain_for(__n);
  const usize __nb = (__n + __B - 1u) / __B;
  micron::vector<usize> __histv(__nb * 256u);
  micron::vector<usize> __offv(__nb * 256u);
  micron::vector<T> __scratchv(__n);
  usize *__hist = &__histv[0];
  usize *__off = &__offv[0];
  T *__src = &__arr[0];
  T *__dst = &__scratchv[0];

  for ( i64 __shift = 0; __shift < static_cast<i64>(sizeof(umax_t)) * 8; __shift += 8 ) {
    if ( (__span >> __shift) == 0 ) break;      // no more significant bytes
    // pass 1: per-block histograms (parallel)
    {
      auto __body = [__src, __n, __B, __hist, __shift, __bias, __key](usize __b) {
        usize *__h = __hist + __b * 256u;
        for ( int __i = 0; __i < 256; ++__i ) __h[__i] = 0;
        const usize __s = __b * __B;
        const usize __e = (__s + __B < __n) ? __s + __B : __n;
        for ( usize __i = __s; __i < __e; ++__i ) {
          const umax_t __k = static_cast<umax_t>(__key(__src[__i])) - __bias;
          ++__h[(__k >> __shift) & 0xFFu];
        }
      };
      co_await __pblocks<decltype(__body)>(0, __nb, __body, 1);
    }
    {
      usize __running = 0;
      for ( int __d = 0; __d < 256; ++__d )
        for ( usize __b = 0; __b < __nb; ++__b ) {
          __off[__b * 256u + (usize)__d] = __running;
          __running += __hist[__b * 256u + (usize)__d];
        }
    }
    {
      auto __body = [__src, __dst, __n, __B, __off, __shift, __bias, __key](usize __b) {
        usize *__o = __off + __b * 256u;
        const usize __s = __b * __B;
        const usize __e = (__s + __B < __n) ? __s + __B : __n;
        for ( usize __i = __s; __i < __e; ++__i ) {
          const umax_t __k = static_cast<umax_t>(__key(__src[__i])) - __bias;
          __dst[__o[(__k >> __shift) & 0xFFu]++] = __src[__i];
        }
      };
      co_await __pblocks<decltype(__body)>(0, __nb, __body, 1);
    }
    T *__tmp = __src;
    __src = __dst;
    __dst = __tmp;
  }

  if ( __src != &__arr[0] ) {      // odd number of passes: sorted data is in scratch -> copy back (parallel)
    T *__from = __src;
    T *__to = &__arr[0];
    auto __copy = [__from, __to](T *__a, T *__b) {
      for ( T *__p = __a; __p != __b; ++__p ) __to[__p - __from] = *__p;
    };
    co_await __pmap<T *, decltype(__copy)>(__from, __from + __n, __copy, __grain_for(__n));
  }
}

template<class It>
[[nodiscard]] micron::task<void>
radix(It __first, It __last)
{
  using T = micron::remove_cvref_t<decltype(*__first)>;
  if constexpr ( micron::is_floating_point_v<T> ) {
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "parallel::sort::radix: only 32/64-bit IEEE-754 floats");
    using U = micron::__conditional_t<sizeof(T) == 4, u32, u64>;
    return __pradix<It>(__first, static_cast<usize>(__last - __first), [](T __e) -> U {
      constexpr int __top = static_cast<int>(sizeof(T)) * 8 - 1;
      const U __signbit = static_cast<U>(1) << __top;
      const U __u = __builtin_bit_cast(U, __e);
      return __u ^ ((static_cast<U>(0) - (__u >> __top)) | __signbit);      // order-preserving forward transform
    });
  } else {
    return __pradix<It>(__first, static_cast<usize>(__last - __first), [](auto __e) { return __e; });
  }
}

template<class It, class KeyFn>
[[nodiscard]] micron::task<void>
radix(It __first, It __last, KeyFn __key)
{
  return __pradix<It, KeyFn>(__first, static_cast<usize>(__last - __first), __key);
}

};      // namespace sort
};      // namespace parallel
};      // namespace micron
