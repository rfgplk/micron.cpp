//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

#include "../concepts.hpp"

#include "../algorithm/algorithm.hpp"

namespace micron
{
namespace sort
{
template <is_iterable_container T, is_valid_comp<T> Cmp>
void
__quick(typename T::iterator start, ssize_t low, ssize_t high, Cmp comp)
{
  if ( low < high ) {
    auto pivot = start[high];
    auto i = low - 1;
    for ( auto j = low; j < high; j++ ) {
      if ( comp(start[j], pivot) ) {
        i++;
        auto temp = start[i];
        start[i] = start[j];
        start[j] = temp;
      }
    }
    i++;
    auto temp = start[i];
    start[i] = start[high];
    start[high] = temp;

    __quick<T>(start, low, i - 1, comp);
    __quick<T>(start, i + 1, high, comp);
  }
}

template <is_iterable_container T>
void
__quick(typename T::iterator start, ssize_t low, ssize_t high)
{
  if ( low < high ) {
    auto pivot = start[high];
    auto i = low - 1;
    for ( auto j = low; j < high; j++ ) {
      if ( start[j] < pivot ) {
        i++;
        auto temp = start[i];
        start[i] = start[j];
        start[j] = temp;
      }
    }
    i++;
    auto temp = start[i];
    start[i] = start[high];
    start[high] = temp;

    __quick<T>(start, low, i - 1);
    __quick<T>(start, i + 1, high);
  }
}

template <is_iterable_container T>
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

template <is_iterable_container T, is_valid_comp<T> Cmp>
void
__quick(T &arr, ssize_t low, ssize_t high, Cmp comp)
{
  if ( low < high ) {
    auto pivot = arr[high];
    auto i = low - 1;
    for ( auto j = low; j < high; j++ ) {
      if ( comp(arr[j], pivot) ) {
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

template <is_iterable_container T>
typename T::iterator
quick(typename T::iterator start, typename T::iterator end)
{
  __quick<T>(start, 0, (end - start) - 1);
  return start;
}

template <is_iterable_container T, is_valid_comp<T> Cmp>
typename T::iterator
quick(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  __quick<T>(start, 0, (end - start) - 1, comp);
  return start;
}

template <is_iterable_container T>
T &
quick(T &arr)
{
  __quick(arr, 0, arr.size() - 1);
  return arr;
}

template <is_iterable_container T>
T &
quick(T &arr, typename T::size_type lim)
{
  __quick(arr, 0, lim - 1);
  return arr;
}

template <is_iterable_container T, is_valid_comp<T> Cmp>
T &
quick(T &arr, typename T::size_type lim, Cmp comp)
{
  __quick(arr, 0, lim - 1, comp);
  return arr;
}

template <is_iterable_container T, is_valid_comp<T> Cmp>
T &
quick(T &arr, Cmp comp)
{
  __quick(arr, 0, arr.size() - 1, comp);
  return arr;
}

};     // namespace sort
};     // namespace micron
