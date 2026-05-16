//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../maps/heap_swiss.hpp"
#include "__set_common.hpp"

namespace micron
{

// heap_swiss_set
//
// thin set adapter over heap_swiss_map<K, __set_empty_v, Alloc>
template<typename K, class Alloc = micron::allocator_serial<>> class heap_swiss_set
{
  using __map_t = heap_swiss_map<K, __set_empty_v, Alloc>;
  __map_t __m;

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using key_type = K;
  using value_type = K;
  using size_type = usize;

  ~heap_swiss_set() = default;
  heap_swiss_set() = default;

  explicit heap_swiss_set(usize n) : __m(n) { }

  heap_swiss_set(const heap_swiss_set &) = default;
  heap_swiss_set(heap_swiss_set &&) noexcept = default;
  heap_swiss_set &operator=(const heap_swiss_set &) = default;
  heap_swiss_set &operator=(heap_swiss_set &&) noexcept = default;

  usize
  size() const noexcept
  {
    return __m.size();
  }

  usize
  capacity() const noexcept
  {
    return __m.capacity();
  }

  bool
  empty() const noexcept
  {
    return __m.empty();
  }

  float
  load_factor() const noexcept
  {
    return __m.load_factor();
  }

  void
  reserve(usize n)
  {
    __m.reserve(n);
  }

  void
  clear() noexcept
  {
    __m.clear();
  }

  bool
  insert(const K &k)
  {
    return __m.insert(k, __set_empty_v{}).a;
  }

  bool
  insert(K &&k)
  {
    return __m.insert(micron::move(k), __set_empty_v{}).a;
  }

  template<typename... Args>
  bool
  emplace(Args &&...args)
  {
    K tmp(micron::forward<Args>(args)...);
    return insert(micron::move(tmp));
  }

  bool
  contains(const K &k) const
  {
    return __m.contains(k);
  }

  usize
  count(const K &k) const
  {
    return __m.count(k);
  }

  bool
  erase(const K &k)
  {
    return __m.erase(k);
  }
};

template<typename K, class Alloc = micron::allocator_serial<>> using hswiss_set = heap_swiss_set<K, Alloc>;

};      // namespace micron
