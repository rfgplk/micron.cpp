//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

#include "../algorithm/algorithm.hpp"
namespace micron
{
namespace sort
{
template <typename T>
void
__quick(T &arr, ssize_t low, ssize_t high)
{
  if ( low < high ) {
    auto pivot = arr[high];
    auto i = low - 1;
    for ( auto j = low; j < high; j++ ) {
      if ( arr[j] < pivot ) {
        i++;
        auto temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
    i++;
    auto temp = arr[i];
    arr[i] = arr[high];
    arr[high] = temp;

    __quick(arr, low, i - 1);
    __quick(arr, i + 1, high);
  }
}
template <typename T>
T &
quick(T &arr)
{
  __quick(arr, 0, arr.size() - 1);
  return arr;
}
};     // namespace sort
};     // namespace micron
