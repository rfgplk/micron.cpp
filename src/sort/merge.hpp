//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// merge sort
//   time:  O(n log n) all cases
//   space: O(n) out-of-place
//   stable: yes
//   in-place: no (allocates scratch)
//
//   use: when STABILITY is required (sort::stable routes here) or worst-case
//   O(n log n) with no quicksort cliff is wanted; predictable, allocation cost

#include "../concepts.hpp"
#include "../memory/new.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{

template<typename U> struct __merge_buf {
  U *p;

  explicit __merge_buf(max_t n) : p(new U[n > 0 ? n : 1]) { }      // never new[0] (micron new[] rejects 0)

  ~__merge_buf() { delete[] p; }

  __merge_buf(const __merge_buf &) = delete;
  __merge_buf &operator=(const __merge_buf &) = delete;
};

template<typename T, typename Cmp>
static void
__merge(T *arr, max_t left, max_t mid, max_t right, Cmp comp)
{
  max_t n1 = mid - left + 1;
  max_t n2 = right - mid;

  __merge_buf<T> Lbuf(n1);
  __merge_buf<T> Rbuf(n2);
  T *L = Lbuf.p;
  T *R = Rbuf.p;

  for ( max_t i = 0; i < n1; ++i ) L[i] = arr[left + i];
  for ( max_t j = 0; j < n2; ++j ) R[j] = arr[mid + 1 + j];

  max_t i = 0, j = 0, k = left;

  while ( i < n1 && j < n2 ) {
    if ( comp(R[j], L[i]) ) {
      arr[k++] = R[j++];
    } else {
      arr[k++] = L[i++];
    }
  }

  while ( i < n1 ) arr[k++] = L[i++];

  while ( j < n2 ) arr[k++] = R[j++];
}

template<typename T>
static void
__merge(T *arr, max_t left, max_t mid, max_t right)
{
  __merge(arr, left, mid, right, [](const T &a, const T &b) { return a < b; });
}

template<typename T, typename Cmp>
static void
__merge_sort(T *arr, max_t left, max_t right, Cmp comp)
{
  if ( left >= right ) return;

  max_t mid = left + ((right - left) >> 1);

  __merge_sort(arr, left, mid, comp);
  __merge_sort(arr, mid + 1, right, comp);
  __merge(arr, left, mid, right, comp);
}

template<typename T>
static void
__merge_sort(T *arr, max_t left, max_t right)
{
  __merge_sort(arr, left, right, [](const T &a, const T &b) { return a < b; });
}

template<is_iterable_container C>
C &
merge(C &arr)
{
  if ( arr.size() > 1 ) __merge_sort(arr.data(), 0, static_cast<max_t>(arr.size()) - 1);
  return arr;
}

template<is_iterable_container C, is_valid_comp<C> Cmp>
C &
merge(C &arr, Cmp comp)
{
  if ( arr.size() > 1 ) __merge_sort(arr.data(), 0, static_cast<max_t>(arr.size()) - 1, comp);
  return arr;
}

template<is_iterable_container T>
T &
merge(T &arr, typename T::size_type lim)
{
  typename T::size_type n = lim < arr.size() ? lim : arr.size();
  if ( n > 1 ) __merge_sort(arr.data(), 0, static_cast<max_t>(n) - 1);
  return arr;
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
merge(T &arr, typename T::size_type lim, Cmp comp)
{
  typename T::size_type n = lim < arr.size() ? lim : arr.size();
  if ( n > 1 ) __merge_sort(arr.data(), 0, static_cast<max_t>(n) - 1, comp);
  return arr;
}

template<is_iterable_container T>
typename T::iterator
merge(typename T::iterator start, typename T::iterator end)
{
  const max_t cnt = static_cast<max_t>(end - start);
  if ( cnt > 1 ) __merge_sort(start, 0, cnt - 1);
  return start;
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
typename T::iterator
merge(typename T::iterator start, typename T::iterator end, Cmp comp)
{
  const max_t cnt = static_cast<max_t>(end - start);
  if ( cnt > 1 ) __merge_sort(start, 0, cnt - 1, comp);
  return start;
}

template<typename T, typename Cmp>
static void
__merge_sort_bottom_up(T *arr, max_t n, Cmp comp)
{
  for ( max_t width = 1; width < n; width <<= 1 ) {
    for ( max_t i = 0; i < n; i += (width << 1) ) {

      max_t left = i;
      max_t mid = i + width - 1;
      max_t right = i + (width << 1) - 1;

      if ( mid >= n ) continue;
      if ( right >= n ) right = n - 1;
      if ( mid >= right ) continue;      // right run empty after clamp -> nothing to merge

      __merge(arr, left, mid, right, comp);
    }
  }
}

template<typename T>
static void
__merge_sort_bottom_up(T *arr, max_t n)
{
  __merge_sort_bottom_up(arr, n, [](const T &a, const T &b) { return a < b; });
}

template<is_iterable_container C>
C &
merge_bottom_up(C &arr)
{
  if ( arr.size() > 1 ) __merge_sort_bottom_up(arr.data(), static_cast<max_t>(arr.size()));
  return arr;
}

template<is_iterable_container C, is_valid_comp<C> Cmp>
C &
merge_bottom_up(C &arr, Cmp comp)
{
  if ( arr.size() > 1 ) __merge_sort_bottom_up(arr.data(), static_cast<max_t>(arr.size()), comp);
  return arr;
}

template<is_iterable_container T>
T &
merge_bottom_up(T &arr, typename T::size_type lim)
{
  typename T::size_type n = lim < arr.size() ? lim : arr.size();      // clamp; pass element COUNT (not lim-1)
  if ( n > 1 ) __merge_sort_bottom_up(arr.data(), static_cast<max_t>(n));
  return arr;
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
merge_bottom_up(T &arr, typename T::size_type lim, Cmp comp)
{
  typename T::size_type n = lim < arr.size() ? lim : arr.size();
  if ( n > 1 ) __merge_sort_bottom_up(arr.data(), static_cast<max_t>(n), comp);
  return arr;
}

};      // namespace sort
};      // namespace micron
