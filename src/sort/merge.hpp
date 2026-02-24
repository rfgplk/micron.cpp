//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../memory/new.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{

template <typename T, typename Cmp>
static void
__merge(T *arr, ssize_t left, ssize_t mid, ssize_t right, Cmp comp)
{
  ssize_t n1 = mid - left + 1;
  ssize_t n2 = right - mid;

  T *L = new T[n1];
  T *R = new T[n2];

  for ( ssize_t i = 0; i < n1; ++i )
    L[i] = arr[left + i];
  for ( ssize_t j = 0; j < n2; ++j )
    R[j] = arr[mid + 1 + j];

  ssize_t i = 0, j = 0, k = left;

  while ( i < n1 && j < n2 ) {
    if ( comp(R[j], L[i]) ) {
      arr[k++] = R[j++];
    } else {
      arr[k++] = L[i++];
    }
  }

  while ( i < n1 )
    arr[k++] = L[i++];

  while ( j < n2 )
    arr[k++] = R[j++];

  delete[] L;
  delete[] R;
}

template <typename T>
static void
__merge(T *arr, ssize_t left, ssize_t mid, ssize_t right)
{
  __merge(arr, left, mid, right, [](const T &a, const T &b) { return a < b; });
}

template <typename T, typename Cmp>
static void
__merge_sort(T *arr, ssize_t left, ssize_t right, Cmp comp)
{
  if ( left >= right )
    return;

  ssize_t mid = left + ((right - left) >> 1);

  __merge_sort(arr, left, mid, comp);
  __merge_sort(arr, mid + 1, right, comp);
  __merge(arr, left, mid, right, comp);
}

template <typename T>
static void
__merge_sort(T *arr, ssize_t left, ssize_t right)
{
  __merge_sort(arr, left, right, [](const T &a, const T &b) { return a < b; });
}

template <is_iterable_container C>
C &
merge(C &arr)
{
  if ( arr.size() > 1 )
    __merge_sort(&arr[0], 0, arr.size() - 1);
  return arr;
}

template <is_iterable_container C, is_valid_comp<C> Cmp>
C &
merge(C &arr, Cmp comp)
{
  if ( arr.size() > 1 )
    __merge_sort(&arr[0], 0, arr.size() - 1, comp);
  return arr;
}

template <typename T, typename Cmp>
static void
__merge_sort_bottom_up(T *arr, ssize_t n, Cmp comp)
{
  for ( ssize_t width = 1; width < n; width <<= 1 ) {
    for ( ssize_t i = 0; i < n; i += (width << 1) ) {

      ssize_t left = i;
      ssize_t mid = i + width - 1;
      ssize_t right = i + (width << 1) - 1;

      if ( mid >= n )
        continue;
      if ( right >= n )
        right = n - 1;

      __merge(arr, left, mid, right, comp);
    }
  }
}

template <typename T>
static void
__merge_sort_bottom_up(T *arr, ssize_t n)
{
  __merge_sort_bottom_up(arr, n, [](const T &a, const T &b) { return a < b; });
}

template <is_iterable_container C>
C &
merge_bottom_up(C &arr)
{
  if ( arr.size() > 1 )
    __merge_sort_bottom_up(arr.data(), arr.size());
  return arr;
}

template <is_iterable_container C, is_valid_comp<C> Cmp>
C &
merge_bottom_up(C &arr, Cmp comp)
{
  if ( arr.size() > 1 )
    __merge_sort_bottom_up(arr.data(), arr.size(), comp);
  return arr;
}

template <is_iterable_container T>
T &
merge_bottom_up(T &arr, typename T::size_type lim)
{
  __merge_sort_bottom_up(arr.data(), lim - 1);
  return arr;
}

template <is_iterable_container T, is_valid_comp<T> Cmp>
T &
merge_bottom_up(T &arr, typename T::size_type lim, Cmp comp)
{
  __merge_sort_bottom_up(arr.data(), lim - 1, comp);
  return arr;
}

};     // namespace sort
};     // namespace micron
