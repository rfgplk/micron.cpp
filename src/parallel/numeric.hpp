//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

#include "../algorithm/math.hpp"

// WARNING: sum/mean/geomean/harmonicmean/inner_product reorder associative ops across the bisection, so for
// fp types the result is not bit identical to a left to right serial fold

namespace micron
{
namespace parallel
{

template<class It> using __pe_t = micron::remove_cvref_t<decltype(*micron::declval<It>())>;
template<class It>
using __pwide_t = micron::conditional_t<micron::is_floating_point_v<__pe_t<It>>, f128,
                                        micron::conditional_t<micron::is_signed_v<__pe_t<It>>, intmax_t, umax_t>>;

template<class It, class Acc = __pwide_t<It>>
[[nodiscard]] micron::task<Acc>
sum(It __first, It __last)
{
  if ( __first == __last ) co_return Acc{};
  auto __leaf = [](It __a, It __b) -> Acc {
    Acc __s = Acc{};
    for ( ; __a != __b; ++__a ) __s += static_cast<Acc>(*__a);
    return __s;
  };
  auto __comb = [](Acc __l, Acc __r) -> Acc { return __l + __r; };
  co_return co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), Acc>(__first, __last, __leaf, __comb,
                                                                               __grain_for(static_cast<usize>(__last - __first)));
}

template<class R = f64, class It>
[[nodiscard]] micron::task<R>
mean(It __first, It __last)
{
  if ( __first == __last ) co_return R{};
  const usize __n = static_cast<usize>(__last - __first);
  auto __s = co_await sum<It>(__first, __last);
  co_return static_cast<R>(__s) / static_cast<R>(__n);
}

template<class R = flong, class It>
[[nodiscard]] micron::task<R>
geomean(It __first, It __last)
{
  if ( __first == __last ) co_return R{};
  const usize __n = static_cast<usize>(__last - __first);
  auto __leaf = [](It __a, It __b) -> R {
    R __p = R(1);
    for ( ; __a != __b; ++__a ) __p *= static_cast<R>(*__a);
    return __p;
  };
  auto __comb = [](R __l, R __r) -> R { return __l * __r; };
  R __prod = co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), R>(__first, __last, __leaf, __comb, __grain_for(__n));
  co_return static_cast<R>(micron::math::pow(static_cast<double>(__prod), 1.0 / static_cast<double>(__n)));
}

template<class R = flong, class It>
[[nodiscard]] micron::task<R>
harmonicmean(It __first, It __last)
{
  if ( __first == __last ) co_return R{};
  const usize __n = static_cast<usize>(__last - __first);
  auto __leaf = [](It __a, It __b) -> R {
    R __s = R(0);
    for ( ; __a != __b; ++__a ) __s += (R(1) / static_cast<R>(*__a));
    return __s;
  };
  auto __comb = [](R __l, R __r) -> R { return __l + __r; };
  R __rs = co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), R>(__first, __last, __leaf, __comb, __grain_for(__n));
  co_return static_cast<R>(__n) / __rs;
}

template<class It>
[[nodiscard]] micron::task<__pe_t<It>>
max(It __first, It __last)
{
  using E = __pe_t<It>;
  if ( __first == __last ) co_return E{};
  auto __leaf = [](It __a, It __b) -> E {
    E __m = *__a;
    for ( ++__a; __a != __b; ++__a )
      if ( __m < *__a ) __m = *__a;
    return __m;
  };
  auto __comb = [](E __l, E __r) -> E { return (__l < __r) ? __r : __l; };
  co_return co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), E>(__first, __last, __leaf, __comb,
                                                                             __grain_for(static_cast<usize>(__last - __first)));
}

template<class It>
[[nodiscard]] micron::task<__pe_t<It>>
min(It __first, It __last)
{
  using E = __pe_t<It>;
  if ( __first == __last ) co_return E{};
  auto __leaf = [](It __a, It __b) -> E {
    E __m = *__a;
    for ( ++__a; __a != __b; ++__a )
      if ( *__a < __m ) __m = *__a;
    return __m;
  };
  auto __comb = [](E __l, E __r) -> E { return (__r < __l) ? __r : __l; };
  co_return co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), E>(__first, __last, __leaf, __comb,
                                                                             __grain_for(static_cast<usize>(__last - __first)));
}

template<class It> struct __at_acc {
  __pe_t<It> __val{};
  usize __idx = 0;
};

template<class It>
[[nodiscard]] micron::task<It>
max_at(It __first, It __last)
{
  using A = __at_acc<It>;
  if ( __first == __last ) co_return __last;
  auto __leaf = [__first](It __a, It __b) -> A {
    It __best = __a;
    for ( It __p = __a; __p != __b; ++__p )
      if ( *__best < *__p ) __best = __p;
    return A{ *__best, static_cast<usize>(__best - __first) };
  };
  auto __comb = [](A __l, A __r) -> A { return (__l.__val < __r.__val) ? __r : __l; };      // ties keep left
  A __res = co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), A>(__first, __last, __leaf, __comb,
                                                                             __grain_for(static_cast<usize>(__last - __first)));
  co_return __first + __res.__idx;
}

template<class It>
[[nodiscard]] micron::task<It>
min_at(It __first, It __last)
{
  using A = __at_acc<It>;
  if ( __first == __last ) co_return __last;
  auto __leaf = [__first](It __a, It __b) -> A {
    It __best = __a;
    for ( It __p = __a; __p != __b; ++__p )
      if ( *__p < *__best ) __best = __p;
    return A{ *__best, static_cast<usize>(__best - __first) };
  };
  auto __comb = [](A __l, A __r) -> A { return (__r.__val < __l.__val) ? __r : __l; };      // ties keep left
  A __res = co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), A>(__first, __last, __leaf, __comb,
                                                                             __grain_for(static_cast<usize>(__last - __first)));
  co_return __first + __res.__idx;
}

template<class It1, class It2, class T>
[[nodiscard]] micron::task<T>
inner_product(It1 __f1, It1 __l1, It2 __f2, T __init)
{
  if ( __f1 == __l1 ) co_return __init;
  auto __leaf = [__f1, __f2](It1 __a, It1 __b) -> T {
    T __s = T{};
    for ( It1 __p = __a; __p != __b; ++__p ) {
      const usize __i = static_cast<usize>(__p - __f1);
      __s += static_cast<T>(*__p) * static_cast<T>(__f2[__i]);
    }
    return __s;
  };
  auto __comb = [](T __l, T __r) -> T { return __l + __r; };
  T __part = co_await __pmapreduce<It1, decltype(__leaf), decltype(__comb), T>(__f1, __l1, __leaf, __comb,
                                                                               __grain_for(static_cast<usize>(__l1 - __f1)));
  co_return __init + __part;
}

// ---- count (== value) ----
template<class It, class V>
[[nodiscard]] micron::task<usize>
count(It __first, It __last, V __value)
{
  auto __leaf = [__value](It __a, It __b) -> usize {
    usize __c = 0;
    for ( ; __a != __b; ++__a )
      if ( *__a == __value ) ++__c;
    return __c;
  };
  auto __comb = [](usize __l, usize __r) -> usize { return __l + __r; };
  return __pmapreduce<It, decltype(__leaf), decltype(__comb), usize>(__first, __last, __leaf, __comb,
                                                                     __grain_for(static_cast<usize>(__last - __first)));
}

template<class It1, class It2>
[[nodiscard]] micron::task<bool>
equal(It1 __f1, It1 __l1, It2 __f2)
{
  auto __leaf = [__f1, __f2](It1 __a, It1 __b) -> bool {
    for ( It1 __p = __a; __p != __b; ++__p ) {
      const usize __i = static_cast<usize>(__p - __f1);
      if ( !(*__p == __f2[__i]) ) return false;
    }
    return true;
  };
  auto __comb = [](bool __l, bool __r) -> bool { return __l && __r; };
  return __pmapreduce<It1, decltype(__leaf), decltype(__comb), bool>(__f1, __l1, __leaf, __comb,
                                                                     __grain_for(static_cast<usize>(__l1 - __f1)));
}

template<class It> struct __sorted_acc {
  bool __ok = true;
  __pe_t<It> __fv{};
  __pe_t<It> __lv{};
};

template<class It>
[[nodiscard]] micron::task<bool>
is_sorted(It __first, It __last)
{
  using A = __sorted_acc<It>;
  if ( __first == __last ) co_return true;
  auto __leaf = [](It __a, It __b) -> A {
    A __s;
    __s.__fv = *__a;
    __pe_t<It> __prev = *__a;
    __s.__ok = true;
    for ( It __p = __a + 1; __p != __b; ++__p ) {
      if ( *__p < __prev ) __s.__ok = false;
      __prev = *__p;
    }
    __s.__lv = __prev;
    return __s;
  };
  auto __comb = [](A __l, A __r) -> A {
    A __s;
    __s.__fv = __l.__fv;
    __s.__lv = __r.__lv;
    __s.__ok = __l.__ok && __r.__ok && !(__r.__fv < __l.__lv);      // boundary: l.last <= r.first
    return __s;
  };
  A __res = co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), A>(__first, __last, __leaf, __comb,
                                                                             __grain_for(static_cast<usize>(__last - __first)));
  co_return __res.__ok;
}

template<class It, class V>
[[nodiscard]] micron::task<bool>
contains(It __first, It __last, V __value)
{
  auto __leaf = [__value](It __a, It __b) -> bool {
    for ( ; __a != __b; ++__a )
      if ( *__a == __value ) return true;
    return false;
  };
  auto __comb = [](bool __l, bool __r) -> bool { return __l || __r; };
  return __pmapreduce<It, decltype(__leaf), decltype(__comb), bool>(__first, __last, __leaf, __comb,
                                                                    __grain_for(static_cast<usize>(__last - __first)));
}

};      // namespace parallel
};      // namespace micron
