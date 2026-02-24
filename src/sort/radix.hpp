//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/algorithm.hpp"
#include "../math/generic.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

#include "counting.hpp"

namespace micron
{
namespace sort
{
template <is_iterable_container T>
void
__impl_count(T &arr, T &carr, i32 size, i32 rdx)
{
  i32 bckt[10] = { 0 };

  for ( auto e : arr )
    bckt[(e / rdx) % 10]++;

  for ( i32 i = 1; i < 10; i++ )
    bckt[i] += bckt[i - 1];

  for ( i32 k = size - 1; k >= 0; --k )
    carr[bckt[(arr[k] / rdx) % 10]-- - 1] = arr[k];
  // arr = carr;
  micron::memcpy(&arr[0], &carr[0], arr.size());     // avoiding copy assign deletion (yes it's bad)
}

template <is_iterable_container T>
void
__impl_count(typename T::iterator start, typename T::iterator end, T &carr, i32 size, i32 rdx)
{
  i32 bckt[10] = { 0 };

  for ( auto __start = start; __start != end; ++start )
    bckt[(*__start / rdx) % 10]++;

  for ( i32 i = 1; i < 10; i++ )
    bckt[i] += bckt[i - 1];

  for ( i32 k = size - 1; k >= 0; --k )
    carr[bckt[(start[k] / rdx) % 10]-- - 1] = start[k];
  // arr = carr;
  micron::memcpy(start, &carr[0], end - start);     // avoiding copy assign deletion (yes it's bad)
}

template <is_iterable_container T>
void
radix(T &arr)
{
  // get maximum
  umax_t max = micron::max(arr.begin(), arr.end());

  i32 n = arr.size();
  T carr(n);     // TODO: turn into slice eventually
  for ( i32 rdx = 1; max / rdx > 0; rdx *= 10 )
    __impl_count(arr, carr, n, rdx);
}

template <is_iterable_container T>
void
radix(typename T::iterator start, typename T::iterator end)
{
  // get maximum
  umax_t max = micron::max(start, end);

  i32 n = end - start;
  T carr(n);     // TODO: turn into slice eventually
  for ( i32 rdx = 1; max / rdx > 0; rdx *= 10 )
    __impl_count(start, end, carr, n, rdx);
}

template <is_iterable_container T>
  requires micron::is_floating_point_v<typename T::value_type>
void
fradix(T &arr)
{
  T bckt(arr.size());
  for ( i64 b = 0; b < 4; b++ ) {
    i32 cnt[256] = {};
    for ( typename T::value_type f : arr ) {
      byte *ptr = reinterpret_cast<byte *>(&f);
      cnt[ptr[b]]++;
    }
    for ( i32 i = 1; i < 256; i++ )
      cnt[i] += cnt[i - 1];
    for ( i32 i = static_cast<i32>(arr.size() - 1); i >= 0; i-- ) {
      byte *ptr = reinterpret_cast<byte *>(&arr[i]);
      bckt[--cnt[ptr[b]]] = arr[i];
    }
    micron::memcpy(&arr[0], &bckt[0], arr.size());
  }
}

};     // namespace sort
};     // namespace micron
