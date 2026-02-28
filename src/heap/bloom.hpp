//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../memory/allocation/chunks.hpp"
#include "../memory/memory.hpp"
#include "../slice.hpp"

#include "../bitfield.hpp"
#include "../hash/hash.hpp"

namespace micron
{

template <typename T, usize N, usize L = (usize)(N / (N / 4) * __builtin_log(2))> class bloom_filter
{
  bitfield<N> bits;
  usize length;

  hash64_t
  hash_round(const T &key, const usize rnd)
  {
    return hash64(&key, sizeof(T), fib_32(rnd)) % length;
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

  bloom_filter(void) : bits(false), length(N) {}

  void
  insert(const T &key)
  {
    for ( usize i = 0; i < L; i++ )
      bits.set(hash_round(key, i));
  }

  void
  emplace(T &&key)
  {
    for ( usize i = 0; i < L; i++ )
      bits.set(hash_round(key, i));
  }

  bool
  contains(const T &key)
  {
    bool f = true;
    for ( usize i = 0; i < L; i++ )
      f = f && bits[hash_round(key, i)];
    return f;
  }
};

};     // namespace micron
