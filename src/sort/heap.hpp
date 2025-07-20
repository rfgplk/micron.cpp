//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
namespace sort
{

template <typename T>
void
__impl_as_heap(T &arr, int n, int i)
{
  int largest = i;
  int left = 2 * i + 1;
  int right = 2 * i + 2;

  if ( left < n && arr[left] > arr[largest] )
    largest = left;

  if ( right < n && arr[right] > arr[largest] )
    largest = right;

  if ( largest != i ) {
    size_t tmp = arr[i];
    arr[i] = arr[largest];
    arr[largest] = tmp;
    __impl_as_heap(arr, n, largest);
  }
}
template <typename T>
void
heap(T &arr)
{
  int n = arr.size();

  for ( int i = n / 2 - 1; i >= 0; i-- )
    __impl_as_heap(arr, n, i);

  for ( int i = n - 1; i > 0; i-- ) {
    size_t tmp = arr[0];
    arr[0] = arr[i];
    arr[i] = tmp;
    __impl_as_heap(arr, i, 0);
  }
}

};     // namespace sort

};     // namespace micron
