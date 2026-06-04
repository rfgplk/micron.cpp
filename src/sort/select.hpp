//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// selection sort  +  selection family
//   selection:    O(n^2) comparisons, O(1) in-place, unstable.
//   nth_element / quickselect:  O(n) average, in-place (k-th order statistic)
//   partial_sort: O(n + k log k) (k smallest, in order)
//   is_sorted / is_sorted_until: O(n) order predicates

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../types.hpp"

#include "quick.hpp"

namespace micron
{
namespace sort
{

template<is_iterable_container T, typename Cmp>
T &
__selection(T &arr, Cmp comp)
{
  const max_t n = static_cast<max_t>(arr.size());
  for ( max_t i = 0; i < n - 1; ++i ) {
    max_t m = i;
    for ( max_t j = i + 1; j < n; ++j )
      if ( comp(arr[j], arr[m]) ) m = j;
    if ( m != i ) micron::swap(arr[i], arr[m]);
  }
  return arr;
}

template<is_iterable_container T>
T &
selection(T &arr)
{
  return __selection(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
selection(T &arr, Cmp comp)
{
  return __selection(arr, comp);
}

template<is_iterable_container T, typename Cmp>
typename T::iterator
__selection_it(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  const max_t n = static_cast<max_t>(end - start);
  for ( max_t i = 0; i < n - 1; ++i ) {
    max_t m = i;
    for ( max_t j = i + 1; j < n; ++j )
      if ( comp(start[j], start[m]) ) m = j;
    if ( m != i ) micron::swap(start[i], start[m]);
  }
  return start;
}

template<is_iterable_container T>
typename T::iterator
selection(typename T::iterator start, typename T::iterator end)
{
  return __selection_it<T>(start, end, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
typename T::iterator
selection(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  return __selection_it<T>(start, end, comp);
}

template<typename It, typename Cmp>
void
__quickselect(It a, max_t lo, max_t hi, max_t k, Cmp comp)
{
  while ( lo < hi ) {
    if ( hi - lo >= 2 ) __q_med3_to_high(a, lo, hi, comp);
    max_t p = __q_partition(a, lo, hi, comp);
    if ( k == p ) return;
    if ( k < p )
      hi = p - 1;
    else
      lo = p + 1;
  }
}

template<is_iterable_container T, typename Cmp>
T &
__nth_element(T &arr, typename T::size_type k, Cmp comp)
{
  const max_t n = static_cast<max_t>(arr.size());
  if ( n < 2 || static_cast<max_t>(k) >= n ) return arr;
  __quickselect(arr.begin(), 0, n - 1, static_cast<max_t>(k), comp);
  return arr;
}

template<is_iterable_container T>
T &
nth_element(T &arr, typename T::size_type k)
{
  return __nth_element(arr, k, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
nth_element(T &arr, typename T::size_type k, Cmp comp)
{
  return __nth_element(arr, k, comp);
}

template<is_iterable_container T>
typename T::value_type
quickselect(T &arr, typename T::size_type k)
{
  const typename T::size_type n = arr.size();
  const typename T::size_type kk = k < n ? k : (n ? n - 1 : 0);
  __nth_element(arr, kk, [](const auto &a, const auto &b) { return a < b; });
  return arr[kk];
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
typename T::value_type
quickselect(T &arr, typename T::size_type k, Cmp comp)
{
  const typename T::size_type n = arr.size();
  const typename T::size_type kk = k < n ? k : (n ? n - 1 : 0);
  __nth_element(arr, kk, comp);
  return arr[kk];
}

template<is_iterable_container T, typename Cmp>
T &
__partial_sort(T &arr, typename T::size_type k, Cmp comp)
{
  const max_t n = static_cast<max_t>(arr.size());
  const max_t m = static_cast<max_t>(k) < n ? static_cast<max_t>(k) : n;
  if ( m < 1 || n < 2 ) return arr;
  if ( m < n ) __quickselect(arr.begin(), 0, n - 1, m - 1, comp);      // [0, m) are the m smallest
  __quick<T>(arr.begin(), 0, m - 1, comp);                             // order that prefix
  return arr;
}

template<is_iterable_container T>
T &
partial_sort(T &arr, typename T::size_type k)
{
  return __partial_sort(arr, k, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
partial_sort(T &arr, typename T::size_type k, Cmp comp)
{
  return __partial_sort(arr, k, comp);
}

template<is_iterable_container T, typename Cmp>
bool
__is_sorted(const T &arr, Cmp comp)
{
  const typename T::size_type n = arr.size();
  for ( typename T::size_type i = 1; i < n; ++i )
    if ( comp(arr[i], arr[i - 1]) ) return false;      // strict break -> not sorted
  return true;
}

template<is_iterable_container T>
bool
is_sorted(const T &arr)
{
  return __is_sorted(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
bool
is_sorted(const T &arr, Cmp comp)
{
  return __is_sorted(arr, comp);
}

template<is_iterable_container T, typename Cmp>
typename T::const_iterator
__is_sorted_until(const T &arr, Cmp comp)
{
  const typename T::size_type n = arr.size();
  for ( typename T::size_type i = 1; i < n; ++i )
    if ( comp(arr[i], arr[i - 1]) ) return arr.cbegin() + i;
  return arr.cend();
}

template<is_iterable_container T>
typename T::const_iterator
is_sorted_until(const T &arr)
{
  return __is_sorted_until(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
typename T::const_iterator
is_sorted_until(const T &arr, Cmp comp)
{
  return __is_sorted_until(arr, comp);
}
};      // namespace sort

};      // namespace micron
