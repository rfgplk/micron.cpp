//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/algorithm.hpp"
#include "../algorithm/mem.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{
template <typename T>
void
counting(T &arr)
{
  typename T::value_type min = micron::min(arr);
  typename T::value_type max = micron::max(arr);
  typename T::value_type range = max - min + 1;
  typename T::value_type bucket[range];
  micron::memset(&bucket[0], 0x0, range);
  for ( auto n : arr )
    bucket[n - min]++;
  for ( umax_t i = 1; i < range; i++ )
    bucket[i] += bucket[i - 1];
  T carr = arr;     // cnt arr copy
  for ( typename T::value_type j = arr.size() - 1; j >= 0; --j )
    carr[bucket[arr[j] - min] - 1] = arr[j];
  arr = carr;
}
};     // namespace sort
};     // namespace micron
