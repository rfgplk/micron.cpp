//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{
template <typename T>
void
bitonic(T &arr)
{
  i64 n = arr.size();
  if ( n % 2 == 0 )
    for ( i64 k = 0; k <= n; k *= 2 ) {
      for ( i64 j = k / 2; j > 0; j /= 2 ) {
        for ( i64 i = 0; i < n; i++ ) {
          i64 l = i ^ j;
          if ( l > i )
            if ( ((i & k) == 0 and arr[i] > arr[l]) or (((i & k) != 0) and (arr[i] < arr[l])) )
              swap(arr[i], arr[l]);
        }
      }
    }
}
};     // namespace sort
};     // namespace micron
