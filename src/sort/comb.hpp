//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// comb sort
//   time:  ~O(n log n) average  /  O(n^2) worst
//   space: O(1) in-place
//   stable: no
//
//   use: simple cache-friendly improvement over bubble; not a primary sort

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{
template<is_iterable_container T, typename Cmp>
T &
__comb(T &arr, Cmp comp)
{
  const max_t n = static_cast<max_t>(arr.size());
  if ( n < 2 ) return arr;

  max_t gap = n;
  bool swapped = true;
  while ( gap > 1 || swapped ) {
    gap = (gap * 10) / 13;      // shrink by ~1.3
    if ( gap == 9 || gap == 10 ) gap = 11;
    if ( gap < 1 ) gap = 1;

    swapped = false;
    for ( max_t i = 0; i + gap < n; ++i ) {
      if ( comp(arr[i + gap], arr[i]) ) {
        micron::swap(arr[i], arr[i + gap]);
        swapped = true;
      }
    }
  }
  return arr;
}

template<is_iterable_container T>
T &
comb(T &arr)
{
  return __comb(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
comb(T &arr, Cmp comp)
{
  return __comb(arr, comp);
}
};      // namespace sort
};      // namespace micron
