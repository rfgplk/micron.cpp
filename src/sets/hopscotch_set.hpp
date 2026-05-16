//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../maps/hopscotch.hpp"
#include "__set_common.hpp"

namespace micron
{

// hopscotch_set
//
// thin set adapter over hopscotch_map<K, __set_empty_v>
template<typename K> class hopscotch_set
{
  using __map_t = hopscotch_map<K, __set_empty_v>;
  __map_t __m;

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using key_type = K;
  using value_type = K;
  using size_type = usize;

  ~hopscotch_set() = default;
  hopscotch_set() = default;
  hopscotch_set(const hopscotch_set &) = default;
  hopscotch_set(hopscotch_set &&) noexcept = default;
  hopscotch_set &operator=(const hopscotch_set &) = default;
  hopscotch_set &operator=(hopscotch_set &&) noexcept = default;

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

  void
  clear() noexcept
  {
    __m.clear();
  }

  bool
  insert(const K &k)
  {
    if ( __m.contains(k) ) return false;
    __m.insert(k, __set_empty_v{});
    return true;
  }

  bool
  insert(K &&k)
  {
    if ( __m.contains(k) ) return false;
    __m.insert(micron::move(k), __set_empty_v{});
    return true;
  }

  bool
  contains(const K &k) const
  {
    return __m.contains(k);
  }

  bool
  erase(const K &k)
  {
    return __m.erase(k);
  }
};

};      // namespace micron
