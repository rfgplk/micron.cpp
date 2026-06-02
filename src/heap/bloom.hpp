//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/memory.hpp"
#include "../slice.hpp"

#include "../bitfield.hpp"
#include "../hash/hash.hpp"
#include "../type_traits.hpp"

namespace micron
{

consteval usize
__bloom_optimal_k(const usize m, const usize n, const usize cap = 64)
{
  const double k = (static_cast<double>(m) / static_cast<double>(n)) * 0.6931471805599453;
  // round-half-up; k is always >= 0 here so no negative branch is needed
  usize r = static_cast<usize>(k + 0.5);
  if ( r < 1 ) r = 1;
  if ( r > cap ) r = cap;
  return r;
}

template<typename T, usize N, usize ExpectedN = N / 8, usize L = __bloom_optimal_k(N, ExpectedN)>
  requires micron::is_trivially_copyable_v<T>
class bloom_filter
{
  static_assert(N > 0, "micron::bloom_filter: N (bit count) must be > 0");
  static_assert(N % 8 == 0, "micron::bloom_filter: N must be a multiple of 8 (bitfield<N> requirement)");
  static_assert(ExpectedN > 0, "micron::bloom_filter: ExpectedN (expected element count) must be > 0");
  static_assert(L >= 1, "micron::bloom_filter: L (hash rounds) must be >= 1");

  bitfield<N> bits;

  hash64_t
  hash_round(const T &key, const usize rnd) const
  {
    return hash64(&key, sizeof(T), fib_32(static_cast<u32>(rnd))) % N;
  }

public:
  using category_type = theap_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef usize size_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  bloom_filter(void) : bits(false) { }

  void
  insert(const T &key)
  {
    for ( usize i = 0; i < L; i++ ) bits.set(hash_round(key, i));
  }

  void
  emplace(T &&key)
  {
    for ( usize i = 0; i < L; i++ ) bits.set(hash_round(key, i));
  }

  bool
  contains(const T &key) const
  {
    bool f = true;
    for ( usize i = 0; i < L; i++ ) f = f && bits[hash_round(key, i)];
    return f;
  }
};

};      // namespace micron
