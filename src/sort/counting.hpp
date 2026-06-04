//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// counting sort
//   time:  O(n + range)
//   space: O(n + range)
//   stable: yes
//
//   best when key span = O(n); wide / sparse / domain-overflowing spans auto-fall
//   back to radix (O(n) memory, no span-sized alloc). key-projection overload
//   sorts structs by a derived integer key

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "radix.hpp"

namespace micron
{
namespace sort
{
template<is_iterable_container T, typename KeyFn>
void
__counting_impl(T &arr, KeyFn key)
{
  using key_t = micron::remove_cvref_t<decltype(key(arr[0]))>;
  const usize n = arr.size();
  if ( n < 2 ) return;      // empty / single element: already sorted

  key_t kmin = key(arr[0]), kmax = key(arr[0]);
  for ( usize i = 1; i < n; ++i ) {
    key_t k = key(arr[i]);
    if ( k < kmin ) kmin = k;
    if ( kmax < k ) kmax = k;
  }

  const umax_t span = static_cast<umax_t>(static_cast<umax_t>(kmax) - static_cast<umax_t>(kmin));      // biased max
  const umax_t n_u = static_cast<umax_t>(n);
  constexpr umax_t CAP = static_cast<umax_t>(1) << 27;      // hard bucket-count ceiling (~1 GiB of umax_t)
  if ( span == ~static_cast<umax_t>(0) || span > 4u * n_u || span >= CAP ) {
    radix(arr, key);      // wide / sparse / full-domain: radix is the right tool
    return;
  }

  const umax_t range = span + 1u;
  const umax_t bias_min = static_cast<umax_t>(kmin);

  __sort_scratch<umax_t> bbuf(static_cast<usize>(range));
  umax_t *bucket = bbuf.get();
  for ( usize i = 0; i < static_cast<usize>(range); ++i ) bucket[i] = 0;
  for ( usize i = 0; i < n; ++i ) ++bucket[static_cast<umax_t>(static_cast<umax_t>(key(arr[i])) - bias_min)];
  for ( umax_t i = 1; i < range; ++i ) bucket[i] += bucket[i - 1];

  T carr = arr;      // output buffer
  // scatter right-to-left for stability; index variable is a position, not a key
  for ( usize k = n; k-- > 0; ) {
    const umax_t idx = static_cast<umax_t>(static_cast<umax_t>(key(arr[k])) - bias_min);
    carr[--bucket[idx]] = arr[k];
  }

  arr = carr;
}

template<is_iterable_container T>
  requires micron::is_integral_v<typename T::value_type>
void
counting(T &arr)
{
  __counting_impl(arr, [](const auto &e) { return e; });
}

template<is_iterable_container T, typename KeyFn>
  requires(is_integral_key_fn<KeyFn, T> && micron::is_trivially_copyable_v<typename T::value_type>)
void
counting(T &arr, KeyFn key)
{
  __counting_impl(arr, key);
}

};      // namespace sort
};      // namespace micron
