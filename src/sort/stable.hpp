//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// stable
//   time:  O(n log n) all cases
//   space: O(n)
//   stable: yes
//
//   use: choose stability by name; mirrors merge's full overload surface

#include "../concepts.hpp"
#include "../types.hpp"

#include "merge.hpp"

namespace micron
{
namespace sort
{
template<is_iterable_container T>
T &
stable(T &arr)
{
  // merge by default
  // NOTE: may swap this out
  return merge(arr);
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
stable(T &arr, Cmp comp)
{
  return merge(arr, comp);
}

template<is_iterable_container T>
T &
stable(T &arr, typename T::size_type lim)
{
  return merge(arr, lim);
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
stable(T &arr, typename T::size_type lim, Cmp comp)
{
  return merge(arr, lim, comp);
}

template<is_iterable_container T>
typename T::iterator
stable(typename T::iterator start, typename T::iterator end)
{
  return merge<T>(start, end);
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
typename T::iterator
stable(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  return merge<T>(start, end, comp);
}
};      // namespace sort
};      // namespace micron
