//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "../vector/ivector.hpp"

namespace micron
{
namespace sort
{
void
merge(int *arr, int left, int mid, int right)
{
  int n1 = mid - left + 1;
  int n2 = right - mid;

  int *L = new int[n1];
  int *R = new int[n2];

  for ( int i = 0; i < n1; ++i )
    L[i] = arr[left + i];
  for ( int j = 0; j < n2; ++j )
    R[j] = arr[mid + 1 + j];

  int i = 0, j = 0, k = left;
  while ( i < n1 && j < n2 ) {
    if ( L[i] <= R[j] ) {
      arr[k++] = L[i++];
    } else {
      arr[k++] = R[j++];
    }
  }

  while ( i < n1 ) {
    arr[k++] = L[i++];
  }
  while ( j < n2 ) {
    arr[k++] = R[j++];
  }

  delete[] L;
  delete[] R;
}
};     // namespace sort
};     // namespace micron
