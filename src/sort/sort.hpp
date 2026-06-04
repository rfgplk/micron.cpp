//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// adaptive introsort:
//   insertion (small N) -> median-of-three quicksort -> heapsort at depth limit

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../types.hpp"

#include "insertion.hpp"
#include "quick.hpp"      // __q_med3_to_high, __q_partition

namespace micron
{
namespace sort
{

template<typename It, typename Cmp>
void
__heap_range(It a, max_t lo, max_t hi, Cmp comp)
{
  const max_t n = hi - lo + 1;
  auto sift = [&](max_t root, max_t cnt) {
    for ( ;; ) {
      max_t child = (root << 1) + 1;
      if ( child >= cnt ) break;
      if ( child + 1 < cnt && comp(a[lo + child], a[lo + child + 1]) ) ++child;
      if ( !comp(a[lo + root], a[lo + child]) ) break;
      micron::swap(a[lo + root], a[lo + child]);
      root = child;
    }
  };
  for ( max_t start = n / 2; start-- > 0; ) sift(start, n);
  for ( max_t cnt = n; cnt > 1; ) {
    --cnt;
    micron::swap(a[lo], a[lo + cnt]);
    sift(0, cnt);
  }
}

template<typename It, typename Cmp>
void
__introsort(It a, max_t lo, max_t hi, max_t depth, Cmp comp)
{
  constexpr max_t THRESH = 24;
  while ( hi - lo > THRESH ) {
    if ( depth == 0 ) {
      __heap_range(a, lo, hi, comp);
      return;
    }
    --depth;
    if ( hi - lo >= 2 ) __q_med3_to_high(a, lo, hi, comp);
    max_t p = __q_partition(a, lo, hi, comp);
    if ( p - lo < hi - p ) {
      if ( lo < p - 1 ) __introsort(a, lo, p - 1, depth, comp);
      lo = p + 1;
    } else {
      if ( p + 1 < hi ) __introsort(a, p + 1, hi, depth, comp);
      hi = p - 1;
    }
  }
}

template<is_iterable_container T, typename Cmp>
T &
__sort_dispatch(T &arr, Cmp comp)
{
  const max_t n = static_cast<max_t>(arr.size());
  if ( n < 2 ) return arr;
  max_t depth = 0;
  for ( max_t t = n; t > 1; t >>= 1 ) depth += 2;      // 2 * floor(log2 n)
  __introsort(arr.begin(), 0, n - 1, depth, comp);
  __insertion(arr, 0, arr.size(), comp);      // finish the <= THRESH chunks
  return arr;
}

template<is_iterable_container T>
T &
sort(T &arr)
{
  return __sort_dispatch(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
sort(T &arr, Cmp comp)
{
  return __sort_dispatch(arr, comp);
}

};      // namespace sort
};      // namespace micron
