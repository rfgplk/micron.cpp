//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitonic sort
//   time:  O(n log^2 n) all cases
//   space: O(1) in-place
//   stable: no
//   power-of-two lengths only (non-pow2 -> insertion fallback)
//
//   default ascending path is SIMD-vectorised ~2.5-3.6x (AVX2/NEON) for fundamental keys

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../memory/actions.hpp"
#include "../simd/network.hpp"
#include "../types.hpp"

#include "insertion.hpp"

namespace micron
{
namespace sort
{
template<is_iterable_container T, typename Cmp>
T &
__bitonic(T &arr, Cmp comp)
{
  const max_t n = static_cast<max_t>(arr.size());
  if ( n < 2 ) return arr;
  if ( (n & (n - 1)) != 0 ) {      // not a power of two
    __insertion(arr, 0, arr.size(), comp);
    return arr;
  }
  for ( max_t k = 2; k <= n; k <<= 1 ) {
    for ( max_t j = k >> 1; j > 0; j >>= 1 ) {
      for ( max_t i = 0; i < n; ++i ) {
        const max_t l = i ^ j;
        if ( l > i ) {
          const bool ascending = ((i & k) == 0);
          const bool out_of_order = ascending ? comp(arr[l], arr[i]) : comp(arr[i], arr[l]);
          if ( out_of_order ) micron::swap(arr[i], arr[l]);
        }
      }
    }
  }
  return arr;
}

template<is_iterable_container T>
T &
bitonic(T &arr)
{
  using V = typename T::value_type;
  if constexpr ( micron::simd::__net_has_simd<V>() ) {
    const usize n = arr.size();
    if ( n >= 2 && (n & (n - 1)) == 0 ) {      // power of two + SIMD key -> vectorised network
      micron::simd::bitonic_sort_pow2(arr.data(), n);
      return arr;
    }
  }
  return __bitonic(arr, [](const auto &a, const auto &b) { return a < b; });
}

template<is_iterable_container T, is_valid_comp<T> Cmp>
T &
bitonic(T &arr, Cmp comp)
{
  return __bitonic(arr, comp);
}
};      // namespace sort
};      // namespace micron
