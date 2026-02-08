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

template <typename T, typename R, typename I>
void
__impl_as_heap(T &arr, R n, I i)
{
  I largest = i;
  I left = 2 * i + 1;
  I right = 2 * i + 2;

  if ( left < n && arr[left] > arr[largest] )
    largest = left;

  if ( right < n && arr[right] > arr[largest] )
    largest = right;

  if ( largest != i ) {
    // TODO: fix
    typename T::value_type tmp = arr[i];
    arr[i] = arr[largest];
    arr[largest] = tmp;
    __impl_as_heap(arr, n, largest);
  }
}
template <typename T>
void
heap(T &arr)
{
  i64 n = arr.size();

  for ( i64 i = n / 2 - 1; i >= 0; i-- )
    __impl_as_heap(arr, n, i);

  for ( i64 i = n - 1; i > 0; i-- ) {
    typename T::value_type tmp = arr[0];
    arr[0] = arr[i];
    arr[i] = tmp;
    __impl_as_heap(arr, i, 0);
  }
}

};     // namespace sort

};     // namespace micron
