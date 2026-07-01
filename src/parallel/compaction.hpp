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

// keep(i) is a predicate over the global index i in [0, n)
template<class In, class Out, class Keep>
micron::task<usize>
__pcompact_idx(In __in, Out __out, usize __n, Keep __keep)
{
  if ( __n == 0 ) co_return 0;
  const usize __B = __grain_for(__n);
  const usize __nb = (__n + __B - 1u) / __B;
  if ( __nb <= 1u ) {      // single block: serial compaction
    usize __k = 0;
    for ( usize __i = 0; __i < __n; ++__i )
      if ( __keep(__i) ) __out[__k++] = __in[__i];
    co_return __k;
  }

  micron::vector<usize> __cnt(__nb);
  usize *__counts = &__cnt[0];
  {
    auto __body = [__n, __B, __counts, __keep](usize __b) {
      const usize __s = __b * __B;
      const usize __e = (__s + __B < __n) ? __s + __B : __n;
      usize __c = 0;
      for ( usize __i = __s; __i < __e; ++__i )
        if ( __keep(__i) ) ++__c;
      __counts[__b] = __c;
    };
    co_await __pblocks<decltype(__body)>(0, __nb, __body, 1);
  }
  micron::vector<usize> __off(__nb);
  usize *__offsets = &__off[0];
  usize __total = 0;
  for ( usize __b = 0; __b < __nb; ++__b ) {
    __offsets[__b] = __total;
    __total += __counts[__b];
  }
  {
    auto __body = [__in, __out, __n, __B, __offsets, __keep](usize __b) {
      const usize __s = __b * __B;
      const usize __e = (__s + __B < __n) ? __s + __B : __n;
      usize __pos = __offsets[__b];
      for ( usize __i = __s; __i < __e; ++__i )
        if ( __keep(__i) ) __out[__pos++] = __in[__i];
    };
    co_await __pblocks<decltype(__body)>(0, __nb, __body, 1);
  }
  co_return __total;
}

template<class It, class Out, class Pred>
[[nodiscard]] micron::task<usize>
where(It __first, It __last, Out __out, Pred __pred)
{
  auto __keep = [__first, __pred](usize __i) -> bool { return __pred(__first[__i]); };
  return __pcompact_idx<It, Out, decltype(__keep)>(__first, __out, static_cast<usize>(__last - __first), __keep);
}

template<class It, class Out, class Pred>
[[nodiscard]] micron::task<usize>
filter(It __first, It __last, Out __out, Pred __pred)
{
  return where<It, Out, Pred>(__first, __last, __out, __pred);
}

template<class It, class Out>
[[nodiscard]] micron::task<usize>
unique(It __first, It __last, Out __out)
{
  auto __keep = [__first](usize __i) -> bool { return __i == 0 || !(__first[__i] == __first[__i - 1u]); };
  return __pcompact_idx<It, Out, decltype(__keep)>(__first, __out, static_cast<usize>(__last - __first), __keep);
}

};      // namespace parallel
};      // namespace micron
