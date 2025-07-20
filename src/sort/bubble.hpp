//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/mem.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{
template <typename T>
void
bubble(T &arr)
{
  i64 n = arr.size();
  while ( n >= 1 ) {
    i64 k = 0;
    for ( i64 i = 1; i < n; i++ ) {
      if ( arr[i - 1] > arr[i] ) {
        swap(arr[i - 1], arr[i]);
        k = i;
      }
    }
    n = k;
  }
}
};     // namespace sort
};     // namespace micron
