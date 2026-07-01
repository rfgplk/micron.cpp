//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "engine.hpp"

namespace micron
{
namespace parallel
{

inline constexpr usize __pfind_npos = static_cast<usize>(-1);

// leftmost / rightmost index combine
struct __pfind_left {
  usize
  operator()(usize __l, usize __r) const noexcept
  {
    return (__l != __pfind_npos) ? __l : __r;
  }
};

struct __pfind_right {
  usize
  operator()(usize __l, usize __r) const noexcept
  {
    return (__r != __pfind_npos) ? __r : __l;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// find_if / find_if_not / find
template<class It, class Pred>
[[nodiscard]] micron::task<It>
find_if(It __first, It __last, Pred __pred)
{
  auto __leaf = [__first, __pred](It __a, It __b) -> usize {
    for ( It __p = __a; __p != __b; ++__p )
      if ( __pred(*__p) ) return static_cast<usize>(__p - __first);
    return __pfind_npos;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_left, usize>(__first, __last, __leaf, __pfind_left{},
                                                                               __grain_for(static_cast<usize>(__last - __first)));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

template<class It, class Pred>
[[nodiscard]] micron::task<It>
find_if_not(It __first, It __last, Pred __pred)
{
  auto __leaf = [__first, __pred](It __a, It __b) -> usize {
    for ( It __p = __a; __p != __b; ++__p )
      if ( !__pred(*__p) ) return static_cast<usize>(__p - __first);
    return __pfind_npos;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_left, usize>(__first, __last, __leaf, __pfind_left{},
                                                                               __grain_for(static_cast<usize>(__last - __first)));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

template<class It, class V>
[[nodiscard]] micron::task<It>
find(It __first, It __last, V __value)
{
  auto __leaf = [__first, __value](It __a, It __b) -> usize {
    for ( It __p = __a; __p != __b; ++__p )
      if ( *__p == __value ) return static_cast<usize>(__p - __first);
    return __pfind_npos;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_left, usize>(__first, __last, __leaf, __pfind_left{},
                                                                               __grain_for(static_cast<usize>(__last - __first)));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// find_last / find_last_if / find_last_if_not

template<class It, class Pred>
[[nodiscard]] micron::task<It>
find_last_if(It __first, It __last, Pred __pred)
{
  auto __leaf = [__first, __pred](It __a, It __b) -> usize {
    usize __r = __pfind_npos;
    for ( It __p = __a; __p != __b; ++__p )
      if ( __pred(*__p) ) __r = static_cast<usize>(__p - __first);
    return __r;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_right, usize>(__first, __last, __leaf, __pfind_right{},
                                                                                __grain_for(static_cast<usize>(__last - __first)));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

template<class It, class V>
[[nodiscard]] micron::task<It>
find_last(It __first, It __last, V __value)
{
  auto __leaf = [__first, __value](It __a, It __b) -> usize {
    usize __r = __pfind_npos;
    for ( It __p = __a; __p != __b; ++__p )
      if ( *__p == __value ) __r = static_cast<usize>(__p - __first);
    return __r;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_right, usize>(__first, __last, __leaf, __pfind_right{},
                                                                                __grain_for(static_cast<usize>(__last - __first)));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

// leftmost index where seq1 != seq2
template<class It1, class It2>
[[nodiscard]] micron::task<It1>
mismatch(It1 __f1, It1 __l1, It2 __f2)
{
  auto __leaf = [__f1, __f2](It1 __a, It1 __b) -> usize {
    for ( It1 __p = __a; __p != __b; ++__p ) {
      const usize __i = static_cast<usize>(__p - __f1);
      if ( !(*__p == __f2[__i]) ) return __i;
    }
    return __pfind_npos;
  };
  usize __i = co_await __pmapreduce<It1, decltype(__leaf), __pfind_left, usize>(__f1, __l1, __leaf, __pfind_left{},
                                                                                __grain_for(static_cast<usize>(__l1 - __f1)));
  co_return (__i == __pfind_npos) ? __l1 : (__f1 + __i);
}

// leftmost i with *(i) == *(i+1)
template<class It>
[[nodiscard]] micron::task<It>
adjacent_find(It __first, It __last)
{
  auto __leaf = [__first, __last](It __a, It __b) -> usize {
    for ( It __p = __a; __p != __b; ++__p ) {
      It __q = __p + 1;
      if ( __q != __last && *__p == *__q ) return static_cast<usize>(__p - __first);
    }
    return __pfind_npos;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_left, usize>(__first, __last, __leaf, __pfind_left{},
                                                                               __grain_for(static_cast<usize>(__last - __first)));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

// leftmost occurrence of [pf,pl) in [first,last)
template<class It, class It2>
[[nodiscard]] micron::task<It>
search(It __first, It __last, It2 __pf, It2 __pl)
{
  const usize __plen = static_cast<usize>(__pl - __pf);
  if ( __plen == 0 ) co_return __first;
  const usize __n = static_cast<usize>(__last - __first);
  if ( __plen > __n ) co_return __last;
  auto __leaf = [__first, __last, __pf, __plen](It __a, It __b) -> usize {
    for ( It __p = __a; __p != __b; ++__p ) {
      if ( static_cast<usize>(__last - __p) < __plen ) break;      // not enough room for a match
      bool __m = true;
      for ( usize __k = 0; __k < __plen; ++__k )
        if ( !(__p[__k] == __pf[__k]) ) {
          __m = false;
          break;
        }
      if ( __m ) return static_cast<usize>(__p - __first);
    }
    return __pfind_npos;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_left, usize>(__first, __last, __leaf, __pfind_left{}, __grain_for(__n));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

template<class It, class V>
[[nodiscard]] micron::task<It>
search_n(It __first, It __last, usize __count, V __value)
{
  if ( __count == 0 ) co_return __first;
  const usize __n = static_cast<usize>(__last - __first);
  if ( __count > __n ) co_return __last;
  auto __leaf = [__first, __last, __count, __value](It __a, It __b) -> usize {
    for ( It __p = __a; __p != __b; ++__p ) {
      if ( static_cast<usize>(__last - __p) < __count ) break;
      bool __m = true;
      for ( usize __k = 0; __k < __count; ++__k )
        if ( !(__p[__k] == __value) ) {
          __m = false;
          break;
        }
      if ( __m ) return static_cast<usize>(__p - __first);
    }
    return __pfind_npos;
  };
  usize __i = co_await __pmapreduce<It, decltype(__leaf), __pfind_left, usize>(__first, __last, __leaf, __pfind_left{}, __grain_for(__n));
  co_return (__i == __pfind_npos) ? __last : (__first + __i);
}

};      // namespace parallel
};      // namespace micron
