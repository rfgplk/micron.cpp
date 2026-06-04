//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shell sort
//   time:  ~O(n^1.25) average (gap-dependent)  /  ~O(n log^2 n)
//   space: O(1) in-place
//   stable: no
//
//   use: medium arrays with no recursion/allocation; strong on patterned data

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{
template<is_iterable_container T, typename Cmp>
T &
__shell(T &arr, Cmp comp)
{
  using value_t = typename T::value_type;
  const max_t n = static_cast<max_t>(arr.size());
  if ( n < 2 ) return arr;

  // Ciura base sequence
  max_t gaps[96];
  max_t ng = 0;
  static constexpr max_t ciura[] = { 1, 4, 10, 23, 57, 132, 301, 701, 1750 };
  for ( max_t i = 0; i < 9; ++i ) gaps[ng++] = ciura[i];
  while ( ng < 96 && gaps[ng - 1] < n ) {
    gaps[ng] = gaps[ng - 1] * 9 / 4;
    ++ng;
  }

  for ( max_t g = ng; g-- > 0; ) {
    const max_t gap = gaps[g];
    if ( gap >= n ) continue;      // a gap >= n would touch nothing
    for ( max_t i = gap; i < n; ++i ) {
      value_t key = arr[i];
      max_t j = i;
      while ( j >= gap && comp(key, arr[j - gap]) ) {
        arr[j] = arr[j - gap];
        j -= gap;
      }
      arr[j] = key;
    }
  }
  return arr;
}

template<is_iterable_container T>
T &
shell(T &arr)
{
  return __shell(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
shell(T &arr, Cmp comp)
{
  return __shell(arr, comp);
}
};      // namespace sort
};      // namespace micron
