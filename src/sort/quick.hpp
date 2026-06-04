//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// quicksort
//   time:  O(n log n) average
//   O(n^2) worst (low-cardinality / adversarial)
//   space: O(log n) stack (depth-bounded)
//   stable: no
//   in-place: yes
//
//   WARNING: degrades to ~O(n^2) on many equal keys
//  prefer sort::sort (introsort) as the default;
//   raw quick is here for the introsort core and explicit use on known-random data

#include "../types.hpp"

#include "../concepts.hpp"

#include "../algorithm/algorithm.hpp"
#include "../memory/actions.hpp"

namespace micron
{
namespace sort
{
// median-of-three
template<typename It, typename Cmp>
[[gnu::always_inline]] static inline void
__q_med3_to_high(It a, max_t lo, max_t hi, Cmp comp)
{
  max_t mid = lo + ((hi - lo) >> 1);
  if ( comp(a[mid], a[lo]) ) micron::swap(a[mid], a[lo]);
  if ( comp(a[hi], a[lo]) ) micron::swap(a[hi], a[lo]);
  if ( comp(a[hi], a[mid]) ) micron::swap(a[hi], a[mid]);
  micron::swap(a[mid], a[hi]);      // median (a[mid]) becomes the pivot at a[hi]
}

// Lomuto partition
template<typename It, typename Cmp>
static inline max_t
__q_partition(It a, max_t lo, max_t hi, Cmp comp)
{
  auto pivot = a[hi];
  max_t i = lo - 1;
  for ( max_t j = lo; j < hi; ++j )
    if ( comp(a[j], pivot) ) {
      ++i;
      if ( i != j ) micron::swap(a[i], a[j]);
    }
  ++i;
  if ( i != hi ) micron::swap(a[i], a[hi]);
  return i;
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
void
__quick(typename T::iterator start, max_t low, max_t high, Cmp comp)
{
  while ( low < high ) {
    if ( high - low >= 2 ) __q_med3_to_high(start, low, high, comp);
    max_t p = __q_partition(start, low, high, comp);
    if ( p - low < high - p ) {
      if ( low < p - 1 ) __quick<T>(start, low, p - 1, comp);
      low = p + 1;
    } else {
      if ( p + 1 < high ) __quick<T>(start, p + 1, high, comp);
      high = p - 1;
    }
  }
}

template<is_iterable_container T>
void
__quick(typename T::iterator start, max_t low, max_t high)
{
  __quick<T>(start, low, high, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
void
__quick(T &arr, max_t low, max_t high, Cmp comp)
{
  __quick<T>(arr.begin(), low, high, comp);
}

template<is_iterable_container T>
void
__quick(T &arr, max_t low, max_t high)
{
  __quick<T>(arr.begin(), low, high);
}

template<is_iterable_container T>
typename T::iterator
quick(typename T::iterator start, typename T::iterator end)
{
  if ( end > start ) __quick<T>(start, 0, (end - start) - 1);
  return start;
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
typename T::iterator
quick(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  if ( end > start ) __quick<T>(start, 0, (end - start) - 1, comp);
  return start;
}

template<is_iterable_container T>
T &
quick(T &arr)
{
  if ( arr.size() > 1 ) __quick<T>(arr.begin(), 0, static_cast<max_t>(arr.size()) - 1);
  return arr;
}

template<is_iterable_container T>
T &
quick(T &arr, typename T::size_type lim)
{
  typename T::size_type n = lim < arr.size() ? lim : arr.size();      // clamp: never index past the end
  if ( n > 1 ) __quick<T>(arr.begin(), 0, static_cast<max_t>(n) - 1);
  return arr;
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
quick(T &arr, typename T::size_type lim, Cmp comp)
{
  typename T::size_type n = lim < arr.size() ? lim : arr.size();
  if ( n > 1 ) __quick<T>(arr.begin(), 0, static_cast<max_t>(n) - 1, comp);
  return arr;
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
quick(T &arr, Cmp comp)
{
  if ( arr.size() > 1 ) __quick<T>(arr.begin(), 0, static_cast<max_t>(arr.size()) - 1, comp);
  return arr;
}

};      // namespace sort
};      // namespace micron
