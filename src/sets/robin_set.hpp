//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../maps/robin.hpp"
#include "__set_common.hpp"

namespace micron
{

// robin_set
//
// thin set adapter over robin_map<K, __set_empty_v>; (cannot grow at runtime)
template<typename K, class Alloc = micron::allocator_serial<>> class robin_set
{
  using __map_t = robin_map<K, __set_empty_v, Alloc>;
  __map_t __m;

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using key_type = K;
  using value_type = K;
  using size_type = usize;

  ~robin_set() = default;
  robin_set() = default;

  explicit robin_set(usize n) : __m(n) { }

  robin_set(const robin_set &) = delete;

  robin_set(robin_set &&o) noexcept : __m(micron::move(o.__m)) { }

  robin_set &operator=(const robin_set &) = delete;

  robin_set &
  operator=(robin_set &&o) noexcept
  {
    __m = micron::move(o.__m);
    return *this;
  }

  usize
  size() const noexcept
  {
    return __m.size();
  }

  usize
  max_size() const noexcept
  {
    return __m.max_size();
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
  clear() noexcept
  {
    __m.clear();
  }

  // returns true on actual insert, false if already present
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

  // iterator -- yields const K&
  class iterator
  {
    typename __map_t::iterator __it;
    typename __map_t::iterator __end;

    void
    skip()
    {
      while ( __it != __end ) {
        // robin map iterator is Nd*, where Nd has .key
        // ctrl gates visibility - cheap visibility check via underlying map design
        break;
      }
    }

  public:
    iterator(typename __map_t::iterator it, typename __map_t::iterator e) : __it(it), __end(e) { }

    const K &
    operator*() const
    {
      return __it->key;
    }

    iterator &
    operator++()
    {
      ++__it;
      return *this;
    }

    bool
    operator==(const iterator &o) const
    {
      return __it == o.__it;
    }

    bool
    operator!=(const iterator &o) const
    {
      return __it != o.__it;
    }
  };

  // robin_map iterators expose raw node pointers; users wanting a clean walk
  // should use for_each which honors the occupancy ctrl bytes.
  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    __m.for_each([&](auto &node) { fn(node.key); });
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    __m.for_each([&](const auto &node) { fn(node.key); });
  }
};

};      // namespace micron
