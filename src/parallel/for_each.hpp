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

// fn(*it) for every element
template<class It, class Fn>
[[nodiscard]] micron::task<void>
for_each(It __first, It __last, Fn __fn)
{
  auto __leaf = [__fn](It __a, It __b) {
    for ( ; __a != __b; ++__a ) __fn(*__a);
  };
  return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));
}

// out[i] = fn(in[i])
template<class It, class Out, class Fn>
[[nodiscard]] micron::task<void>
transform(It __first, It __last, Out __out, Fn __fn)
{
  auto __leaf = [__first, __out, __fn](It __a, It __b) {
    for ( It __p = __a; __p != __b; ++__p ) {
      const usize __i = static_cast<usize>(__p - __first);
      __out[__i] = __fn(*__p);
    }
  };
  return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));
}

// out[i] = fn(in[i], in2[i])
template<class It, class It2, class Out, class Fn>
[[nodiscard]] micron::task<void>
transform(It __first, It __last, It2 __first2, Out __out, Fn __fn)
{
  auto __leaf = [__first, __first2, __out, __fn](It __a, It __b) {
    for ( It __p = __a; __p != __b; ++__p ) {
      const usize __i = static_cast<usize>(__p - __first);
      __out[__i] = __fn(*__p, __first2[__i]);
    }
  };
  return __pmap<It, decltype(__leaf)>(__first, __last, __leaf, __grain_for(static_cast<usize>(__last - __first)));
}

template<class C, class Fn>
[[nodiscard]] auto
for_each_range(C &__c, Fn __fn)
{
  return for_each(__c.begin(), __c.end(), __fn);
}

};      // namespace parallel
};      // namespace micron
