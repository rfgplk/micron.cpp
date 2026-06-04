//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// radix sort
//   time:  O(n * ceil(keybits/8))
//   space: O(n)
//   stable: yes (non-comparison)
//
//   use: large integer/float arrays; key-projection overloads sort structs by key

#include "../algorithm/algorithm.hpp"
#include "../math/generic.hpp"
#include "../memory/memory.hpp"
#include "../memory/new.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

namespace micron
{
namespace sort
{

template<typename U> struct __sort_scratch {
  U *p;

  explicit __sort_scratch(usize n) : p(n ? new U[n] : nullptr) { }

  ~__sort_scratch() { delete[] p; }

  __sort_scratch(const __sort_scratch &) = delete;
  __sort_scratch &operator=(const __sort_scratch &) = delete;

  U *
  get() noexcept
  {
    return p;
  }
};

template<is_iterable_container T, typename KeyFn>
void
__radix_pass(T &arr, T &carr, usize n, i64 shift, umax_t bias_min, KeyFn key)
{
  usize bckt[256] = {};

  for ( usize i = 0; i < n; ++i ) {
    umax_t k = static_cast<umax_t>(static_cast<umax_t>(key(arr[i])) - bias_min);
    bckt[(k >> shift) & 0xFFu]++;
  }
  for ( i32 i = 1; i < 256; ++i ) bckt[i] += bckt[i - 1];

  for ( usize k = n; k-- > 0; ) {
    umax_t kk = static_cast<umax_t>(static_cast<umax_t>(key(arr[k])) - bias_min);
    carr[--bckt[(kk >> shift) & 0xFFu]] = arr[k];
  }
  micron::memcpy(&arr[0], &carr[0], n);      // element-count copy-back
}

template<is_iterable_container T, typename KeyFn>
void
__radix_int(T &arr, KeyFn key)
{
  using key_t = micron::remove_cvref_t<decltype(key(arr[0]))>;
  const usize n = arr.size();
  if ( n < 2 ) return;      // empty / single: sorted

  key_t kmin = key(arr[0]), kmax = key(arr[0]);
  for ( usize i = 1; i < n; ++i ) {
    key_t k = key(arr[i]);
    if ( k < kmin ) kmin = k;
    if ( kmax < k ) kmax = k;
  }
  const umax_t bias_min = static_cast<umax_t>(kmin);
  const umax_t span = static_cast<umax_t>(static_cast<umax_t>(kmax) - static_cast<umax_t>(kmin));      // biased max

  T carr = arr;      // scratch (overwritten each pass)
  for ( i64 shift = 0; shift < static_cast<i64>(sizeof(umax_t)) * 8; shift += 8 ) {
    if ( (span >> shift) == 0 ) break;      // no more significant bytes
    __radix_pass(arr, carr, n, shift, bias_min, key);
  }
}

template<is_iterable_container T>
  requires micron::is_integral_v<typename T::value_type>
void
radix(T &arr)
{
  __radix_int(arr, [](const auto &e) { return e; });
}

template<is_iterable_container T, typename KeyFn>
  requires(is_integral_key_fn<KeyFn, T> && micron::is_trivially_copyable_v<typename T::value_type>)
void
radix(T &arr, KeyFn key)
{
  __radix_int(arr, key);
}

template<is_iterable_container T>
  requires micron::is_integral_v<typename T::value_type>
void
radix(typename T::iterator start, typename T::iterator end)
{
  const max_t cnt = static_cast<max_t>(end - start);
  if ( cnt < 2 ) return;

  using value_t = typename T::value_type;
  value_t vmin = start[0], vmax = start[0];
  for ( max_t i = 1; i < cnt; ++i ) {
    if ( start[i] < vmin ) vmin = start[i];
    if ( vmax < start[i] ) vmax = start[i];
  }
  const umax_t bias_min = static_cast<umax_t>(vmin);
  const umax_t span = static_cast<umax_t>(static_cast<umax_t>(vmax) - static_cast<umax_t>(vmin));

  T carr(static_cast<typename T::size_type>(cnt));
  for ( i64 shift = 0; shift < static_cast<i64>(sizeof(umax_t)) * 8; shift += 8 ) {
    if ( (span >> shift) == 0 ) break;
    usize bckt[256] = {};
    for ( max_t i = 0; i < cnt; ++i ) bckt[(static_cast<umax_t>(static_cast<umax_t>(start[i]) - bias_min) >> shift) & 0xFFu]++;
    for ( i32 i = 1; i < 256; ++i ) bckt[i] += bckt[i - 1];
    for ( max_t k = cnt; k-- > 0; )
      carr[--bckt[(static_cast<umax_t>(static_cast<umax_t>(start[k]) - bias_min) >> shift) & 0xFFu]] = start[k];
    micron::memcpy(start, &carr[0], static_cast<u64>(cnt));
  }
}

template<is_iterable_container T>
  requires(micron::is_floating_point_v<typename T::value_type>
           && (sizeof(typename T::value_type) == 4 || sizeof(typename T::value_type) == 8))
void
fradix(T &arr)
{
  using V = typename T::value_type;
  using U = micron::__conditional_t<sizeof(V) == 4, u32, u64>;
  constexpr i64 W = static_cast<i64>(sizeof(V));
  constexpr i64 TOP = W * 8 - 1;
  const U signbit = static_cast<U>(1) << TOP;

  const usize n = arr.size();
  if ( n < 2 ) return;

  __sort_scratch<U> kbuf(n), tbuf(n);
  U *keys = kbuf.get();
  U *tmp = tbuf.get();

  for ( usize i = 0; i < n; ++i ) {
    U u = 0;
    micron::bytecpy(&u, &arr[i], W);                                 // bit-pun float -> unsigned
    keys[i] = u ^ ((static_cast<U>(0) - (u >> TOP)) | signbit);      // order-preserving forward transform
  }

  for ( i64 b = 0; b < W; ++b ) {
    i32 cnt[256] = {};
    for ( usize i = 0; i < n; ++i ) cnt[static_cast<byte>(keys[i] >> (b * 8))]++;
    for ( i32 i = 1; i < 256; ++i ) cnt[i] += cnt[i - 1];
    for ( usize i = n; i-- > 0; ) tmp[--cnt[static_cast<byte>(keys[i] >> (b * 8))]] = keys[i];
    U *swap = keys;
    keys = tmp;
    tmp = swap;
  }

  for ( usize i = 0; i < n; ++i ) {
    U u = keys[i] ^ (((keys[i] >> TOP) - static_cast<U>(1)) | signbit);      // inverse transform
    micron::bytecpy(&arr[i], &u, W);                                         // bit-pun back to float
  }
}

template<is_iterable_container T, typename KeyFn>
  requires(is_floating_key_fn<KeyFn, T> && micron::is_trivially_copyable_v<typename T::value_type>)
void
fradix(T &arr, KeyFn key)
{
  using V = micron::remove_cvref_t<decltype(key(arr[0]))>;
  using U = micron::__conditional_t<sizeof(V) == 4, u32, u64>;
  constexpr i64 W = static_cast<i64>(sizeof(V));
  constexpr i64 TOP = W * 8 - 1;
  const U signbit = static_cast<U>(1) << TOP;

  const usize n = arr.size();
  if ( n < 2 ) return;

  auto tkey = [&](const typename T::value_type &e) -> U {
    V v = key(e);
    U u = 0;
    micron::bytecpy(&u, &v, W);
    return u ^ ((static_cast<U>(0) - (u >> TOP)) | signbit);
  };

  T carr = arr;
  for ( i64 b = 0; b < W; ++b ) {
    usize cnt[256] = {};
    for ( usize i = 0; i < n; ++i ) cnt[static_cast<byte>(tkey(arr[i]) >> (b * 8))]++;
    for ( i32 i = 1; i < 256; ++i ) cnt[i] += cnt[i - 1];
    for ( usize k = n; k-- > 0; ) carr[--cnt[static_cast<byte>(tkey(arr[k]) >> (b * 8))]] = arr[k];
    micron::memcpy(&arr[0], &carr[0], n);
  }
}

};      // namespace sort
};      // namespace micron
