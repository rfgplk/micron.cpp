//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bubble sort
//   time:  O(n) best (sorted)  /  O(n^2) average + worst
//   space: O(1) in-place
//   stable: yes
//   use: tiny / nearly-sorted inputs, teaching; prefer insertion/sort otherwise

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{
template<is_iterable_container T, typename Cmp>
T &
__bubble(T &arr, Cmp comp)
{
  max_t n = static_cast<max_t>(arr.size());
  while ( n >= 1 ) {
    max_t k = 0;
    for ( max_t i = 1; i < n; ++i ) {
      if ( comp(arr[i], arr[i - 1]) ) {
        micron::swap(arr[i - 1], arr[i]);
        k = i;
      }
    }
    n = k;
  }
  return arr;
}

template<is_iterable_container T>
T &
bubble(T &arr)
{
  return __bubble(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
bubble(T &arr, Cmp comp)
{
  return __bubble(arr, comp);
}

template<is_iterable_container T, typename Cmp>
typename T::iterator
__bubble_it(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  max_t n = static_cast<max_t>(end - start);
  while ( n >= 1 ) {
    max_t k = 0;
    for ( max_t i = 1; i < n; ++i ) {
      if ( comp(start[i], start[i - 1]) ) {
        micron::swap(start[i - 1], start[i]);
        k = i;
      }
    }
    n = k;
  }
  return start;
}

template<is_iterable_container T>
typename T::iterator
bubble(typename T::iterator start, typename T::iterator end)
{
  return __bubble_it<T>(start, end, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
typename T::iterator
bubble(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  return __bubble_it<T>(start, end, comp);
}
};      // namespace sort
};      // namespace micron
