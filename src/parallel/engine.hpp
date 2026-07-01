//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../tasks/tasks.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{
namespace parallel
{

// ~8 leaves per worker, tiny ranges stay serial
inline usize
__grain_for(usize __n) noexcept
{
  const u32 __w = (micron::coro::__global_engine != nullptr) ? micron::coro::__global_engine->n : 1u;
  usize __g = __n / (static_cast<usize>(__w) * 8u + 1u);
  if ( __g < 1024u ) __g = 1024u;
  return __g;
}

template<class It, class Leaf>
micron::task<void>
__pmap(It __first, It __last, Leaf __leaf, usize __grain)
{
  const usize __n = static_cast<usize>(__last - __first);
  if ( __n <= __grain ) {
    __leaf(__first, __last);
    co_return;
  }
  It __mid = __first + __n / 2;
  co_await micron::coro::fork(micron::coro::discard, __pmap<It, Leaf>)(__first, __mid, __leaf, __grain);
  co_await micron::coro::call(__pmap<It, Leaf>, __mid, __last, __leaf, __grain);
  co_await micron::coro::join;
}

template<class Body>
micron::task<void>
__pblocks(usize __lo, usize __hi, Body __body, usize __grain)
{
  const usize __n = __hi - __lo;
  if ( __n <= __grain ) {
    for ( usize __b = __lo; __b < __hi; ++__b ) __body(__b);
    co_return;
  }
  const usize __mid = __lo + __n / 2;
  co_await micron::coro::fork(micron::coro::discard, __pblocks<Body>)(__lo, __mid, __body, __grain);
  co_await micron::coro::call(__pblocks<Body>, __mid, __hi, __body, __grain);
  co_await micron::coro::join;
}

template<class It, class Leaf, class Comb, class T>
micron::task<T>
__pmapreduce(It __first, It __last, Leaf __leaf, Comb __comb, usize __grain)
{
  const usize __n = static_cast<usize>(__last - __first);
  if ( __n <= __grain ) co_return __leaf(__first, __last);
  It __mid = __first + __n / 2;
  T __l;
  co_await micron::coro::fork(&__l, __pmapreduce<It, Leaf, Comb, T>)(__first, __mid, __leaf, __comb, __grain);
  T __r = co_await micron::coro::call(__pmapreduce<It, Leaf, Comb, T>, __mid, __last, __leaf, __comb, __grain);
  co_await micron::coro::join;
  co_return __comb(__l, __r);
}

};      // namespace parallel
};      // namespace micron
