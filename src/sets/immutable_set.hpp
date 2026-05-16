//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../maps/immutable.hpp"
#include "__set_common.hpp"

namespace micron
{

// immutable_set
//
// thin set adapter over immutable_map<K, __set_empty_v>;  persistent, LLRB backed, structural sharing
template<typename K> class immutable_set
{
  using __map_t = immutable_map<K, __set_empty_v>;
  __map_t __m;

public:
  using category_type = map_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  using key_type = K;
  using value_type = K;
  using size_type = usize;

  ~immutable_set() = default;
  immutable_set() = default;
  immutable_set(const immutable_set &) = default;
  immutable_set(immutable_set &&) noexcept = default;
  immutable_set &operator=(const immutable_set &) = default;
  immutable_set &operator=(immutable_set &&) noexcept = default;

  usize
  size() const noexcept
  {
    return __m.size();
  }

  bool
  empty() const noexcept
  {
    return __m.empty();
  }

  immutable_set
  insert(const K &k) const
  {
    immutable_set r = *this;
    r.__m = r.__m.insert(k, __set_empty_v{});
    return r;
  }

  immutable_set
  erase(const K &k) const
  {
    immutable_set r = *this;
    r.__m = r.__m.erase(k);
    return r;
  }

  bool
  contains(const K &k) const
  {
    return __m.contains(k);
  }
};

};      // namespace micron
