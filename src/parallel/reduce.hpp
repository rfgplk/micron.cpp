//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

// WARNING: element must be fully associative, fp type reordering will cause result to not be identical to the serial version

namespace micron
{
namespace parallel
{

template<class It, class T, class Op>
[[nodiscard]] micron::task<T>
reduce(It __first, It __last, T __init, Op __op)
{
  if ( __first == __last ) co_return __init;
  auto __leaf = [__op](It __a, It __b) -> T {
    T __acc = static_cast<T>(*__a);
    for ( ++__a; __a != __b; ++__a ) __acc = __op(__acc, static_cast<T>(*__a));
    return __acc;
  };
  auto __comb = [__op](T __l, T __r) -> T { return __op(__l, __r); };
  T __part = co_await __pmapreduce<It, decltype(__leaf), decltype(__comb), T>(__first, __last, __leaf, __comb,
                                                                              __grain_for(static_cast<usize>(__last - __first)));
  co_return __op(__init, __part);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// all_of / any_of / none_of

template<class It, class Pred>
[[nodiscard]] micron::task<bool>
all_of(It __first, It __last, Pred __p)
{
  auto __leaf = [__p](It __a, It __b) -> bool {
    for ( ; __a != __b; ++__a )
      if ( !__p(*__a) ) return false;
    return true;
  };
  auto __comb = [](bool __l, bool __r) -> bool { return __l && __r; };
  return __pmapreduce<It, decltype(__leaf), decltype(__comb), bool>(__first, __last, __leaf, __comb,
                                                                    __grain_for(static_cast<usize>(__last - __first)));
}

template<class It, class Pred>
[[nodiscard]] micron::task<bool>
any_of(It __first, It __last, Pred __p)
{
  auto __leaf = [__p](It __a, It __b) -> bool {
    for ( ; __a != __b; ++__a )
      if ( __p(*__a) ) return true;
    return false;
  };
  auto __comb = [](bool __l, bool __r) -> bool { return __l || __r; };
  return __pmapreduce<It, decltype(__leaf), decltype(__comb), bool>(__first, __last, __leaf, __comb,
                                                                    __grain_for(static_cast<usize>(__last - __first)));
}

template<class It, class Pred>
[[nodiscard]] micron::task<bool>
none_of(It __first, It __last, Pred __p)
{
  bool __any = co_await any_of<It, Pred>(__first, __last, __p);
  co_return !__any;
}

template<class It, class Pred>
[[nodiscard]] micron::task<usize>
count_if(It __first, It __last, Pred __p)
{
  auto __leaf = [__p](It __a, It __b) -> usize {
    usize __c = 0;
    for ( ; __a != __b; ++__a )
      if ( __p(*__a) ) ++__c;
    return __c;
  };
  auto __comb = [](usize __l, usize __r) -> usize { return __l + __r; };
  return __pmapreduce<It, decltype(__leaf), decltype(__comb), usize>(__first, __last, __leaf, __comb,
                                                                     __grain_for(static_cast<usize>(__last - __first)));
}

template<class C, class T, class Op>
[[nodiscard]] auto
reduce_range(C &__c, T __init, Op __op)
{
  return reduce(__c.begin(), __c.end(), __init, __op);
}

};      // namespace parallel
};      // namespace micron
