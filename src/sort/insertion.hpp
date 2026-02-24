//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{

template <is_iterable_container T, typename Cmp>
void
__insertion(T &arr, typename T::size_type start, typename T::size_type end, Cmp comp)
{
  using value_t = typename T::value_type;
  for ( typename T::size_type i = start + 1; i < end; ++i ) {
    value_t key = arr[i];
    ssize_t j = i;
    while ( j > 0 && comp(key, arr[j - 1]) ) {
      arr[j] = arr[j - 1];
      --j;
    }
    arr[j] = key;
  }
}

template <is_iterable_container T>
void
__insertion(T &arr, typename T::size_type start, typename T::size_type end)
{
  __insertion(arr, start, end, [](const auto &a, const auto &b) { return a < b; });
}

template <is_iterable_container T>
T &
insertion(T &arr)
{
  __insertion(arr, 0, arr.size());
  return arr;
}

template <is_iterable_container T, is_valid_comp<T> Cmp>
T &
insertion(T &arr, Cmp comp)
{
  __insertion(arr, 0, arr.size(), comp);
  return arr;
}

template <is_iterable_container T>
T &
insertion(T &arr, typename T::size_type lim)
{
  __insertion(arr, 0, lim);
  return arr;
}

template <is_iterable_container T, is_valid_comp<T> Cmp>
T &
insertion(T &arr, typename T::size_type lim, Cmp comp)
{
  __insertion(arr, 0, lim, comp);
  return arr;
}

template <is_iterable_container T>
typename T::iterator
insertion(typename T::iterator start, typename T::iterator end)
{
  T &arr = *reinterpret_cast<T *>(nullptr);
  __insertion(arr, 0, end - start);
  return start;
}

template <is_iterable_container T, is_valid_comp<T> Cmp>
typename T::iterator
insertion(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  T &arr = *reinterpret_cast<T *>(nullptr);
  __insertion(arr, 0, end - start, comp);
  return start;
}

};     // namespace sort
};     // namespace micron
