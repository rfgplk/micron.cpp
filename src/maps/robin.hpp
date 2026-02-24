//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "hash/hash.hpp"
#include "memory/actions.hpp"

#include "allocator.hpp"
#include "concepts.hpp"
#include "except.hpp"
#include "tags.hpp"
#include "type_traits.hpp"
#include "types.hpp"

namespace micron
{
template <typename K, typename V> struct alignas(32) robin_map_node {
  hash64_t key;
  V value;           // if value is set, then it's occupied
  size_t length;     // length from starting node
  ~robin_map_node() = default;

  robin_map_node() : length(0) {}

  robin_map_node(const hash64_t &k, V &&v, size_t l) : key(k), value(micron::move(v)), length(l) {}

  template <typename... Args> robin_map_node(const hash64_t &k, size_t l, Args &&...args) : key(k), value(args...), length(l) {}

  robin_map_node(const robin_map_node &) = default;
  robin_map_node(robin_map_node &&) = default;
  robin_map_node &operator=(const robin_map_node &) = default;
  robin_map_node &operator=(robin_map_node &&) = default;

  bool
  operator!(void)
  {
    return !value;     // not set = true, set false
  }

  template <typename... Args>
  robin_map_node &
  set(const K &k, Args &&...args)
  {
    key = hash<hash64_t>(k);
    value = micron::move(V(micron::forward(args)...));
    return *this;
  }

  robin_map_node &
  set(const K &k, V &&val)
  {
    key = hash<hash64_t>(k);
    value = micron::move(val);
    return *this;
  }
};

// non STL compliant, compare predicate won't go in here
// this is a hash robin_map container which implements robin hood h. allc. under the hood
template <typename K, typename V, class Alloc = micron::allocator_serial<>, typename Nd = robin_map_node<K, V>>
  requires micron::is_move_constructible_v<V>
class robin_map : public __immutable_memory_resource<Nd, Alloc>
{
  using __mem = __immutable_memory_resource<Nd, Alloc>;

  inline hash64_t
  hsh(const K &val) const
  {
    return hash<hash64_t>(val);
  }

  inline size_t
  hsh_index(const hash64_t &hsh) const
  {
    return hsh % __mem::capacity;
  }

  inline size_t
  hsh_index(const V &val) const
  {
    return hash<hash64_t>(val) % __mem::capacity;
  }

  template <typename _N = Nd>     // this is here to help the compile inline properly
  inline __attribute__((always_inline)) Nd &
  __access(const size_t index)
  {
    return __mem::memory[index];     // to prevent pointless typing
  }

  template <typename _N = Nd>     // this is here to help the compile inline properly
  inline __attribute__((always_inline)) const Nd &
  __access(const size_t index) const
  {
    return __mem::memory[index];     // to prevent pointless typing
  }

  inline __attribute__((always_inline)) void
  __shift(size_t index)
  {
    // shift all entries to accomodate for deletion
    index = (index + 1) % __mem::capacity;
    while ( !!__access(index) && __access(index) > 0 ) {
      __access((index - 1 + __mem::capacity) % __mem::capacity) = micron::move(__access(index));
      __access(index).key = 0x0;
      __access((index - 1 + __mem::capacity) % __mem::capacity).length--;
      index = (index + 1) % __mem::capacity;
    }
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef size_t size_type;
  typedef Nd value_type;
  typedef Nd &reference;
  typedef Nd &ref;
  typedef const Nd &const_reference;
  typedef const Nd &const_ref;
  typedef Nd *pointer;
  typedef const Nd *const_pointer;
  typedef Nd *iterator;
  typedef const Nd *const_iterator;

  ~robin_map()
  {
    if ( __mem::memory == nullptr )
      return;
    clear();
  }

  robin_map() : __mem((Alloc::auto_size() >= sizeof(Nd) ? Alloc::auto_size() : sizeof(Nd))) {}

  robin_map(const size_t n) : __mem(n * sizeof(Nd)) {}

  robin_map(const robin_map &) = delete;

  robin_map(robin_map &&o) : __mem(micron::move(o)) {}

  robin_map &operator=(const robin_map &) = delete;

  robin_map &
  operator=(robin_map &&o)
  {
    if ( __mem::memory ) {
      clear();
    }
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    o.memory = nullptr;
    o.length = 0;
    o.capacity = 0;
    return *this;
  }

  void reserve() = delete;     // this robin_maps index is capacity bound, cannot grow the container

  void
  clear()
  {
    if constexpr ( micron::is_object_v<V> ) {
      for ( size_t i = 0; i < __mem::capacity; i++ )
        if ( __access(i).key )
          __access(i).~Nd();
    }
    micron::memset(&__mem::memory[0], 0x0, __mem::capacity);
    __mem::length = 0;
  }

  inline V &
  at(const K &k)     // access at K
  {
    return this->operator[](k);
  }

  inline V &
  operator[](const K &k)     // access at K
  {
    hash64_t kh = hsh(k);
    size_t index = hsh_index(kh);
    size_t plen = 0;

    while ( __access(index).key && plen <= __access(index).length ) {
      if ( __access(index).key == kh ) {
        return __access(index).value;
      }
      ++plen;
      index = (index + 1) % __mem::capacity;
    }
    // conform to standard behavior, if no hit insert empty object
    return insert(k, V{});
  }

  iterator
  begin()
  {
    return &__mem::memory[0];     // the memory is contiguous so this is fine
  }

  const_iterator
  begin() const
  {
    return &__mem::memory[0];     // the memory is contiguous so this is fine
  }

  const_iterator
  cbegin() const
  {
    return &__mem::memory[0];     // the memory is contiguous so this is fine
  }

  iterator
  end()
  {
    return &__mem::memory[__mem::length];     // the memory is contiguous so this is fine
  }

  const_iterator
  end() const
  {
    return &__mem::memory[__mem::length];     // the memory is contiguous so this is fine
  }

  const_iterator
  cend() const
  {
    return &__mem::memory[__mem::length];     // the memory is contiguous so this is fine
  }

  size_t
  size() const
  {
    return __mem::length;
  }

  size_t
  max_size() const
  {
    return __mem::capacity;
  }

  void
  swap(robin_map &o) noexcept
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
  }

  V &
  insert(const K &k, V &&value)
  {
    hash64_t key = hsh(k);
    auto index = hsh_index(key);
    size_t plen = 0;
    while ( !!__access(index) ) {
      if ( __access(index).key == key ) {
        __access(index).set(k, micron::move(value));
      }
      if ( __access(index).length < plen ) {
        micron::swap(key, __access(index).key);
        micron::swap(value, __access(index).value);
        micron::swap(plen, __access(index).length);
      }
      ++plen;
      index = (index + 1) % __mem::capacity;
    }
    new (&__access(index)) Nd(key, micron::move(value), plen);
    ++__mem::length;
    return __access(index).value;
  }

  template <typename... Args>
  V &
  emplace(const K &k, Args &&...args)
  {
    hash64_t key = hsh(k);
    auto index = hsh_index(key);
    size_t plen = 0;
    new (&__access(index)) Nd(key, plen, micron::forward<Args>(args)...);
    ++__mem::length;
    return __access(index).value;
  }

  bool
  erase(const K &k)
  {
    hash64_t key = hsh(k);
    auto index = hsh_index(key);
    size_t plen = 0;

    while ( !!__access(index) && plen <= __access(index).length ) {
      if ( __access(index).key == k ) {
        __access(index).key = 0x0;
        __mem::length--;
        __shift(index);
        return true;
      }
      plen++;
      index = (index + 1) % __mem::capacity;
    }
    return false;
  }

  size_t
  exists(const K &k) const
  {
    return get(k, V{}) ? 1 : 0;
  }

  size_t
  count(const K &k) const     // NOTE: robin hood robin_maps support multiple elements unlike STL robin_maps
  {
    hash64_t kh = hsh(k);
    size_t index = hsh_index(kh);
    size_t plen = 0;
    size_t count = 0;
    while ( __access(index).key && plen <= __access(index).length ) {
      if ( __access(index).key == kh )
        count++;
      ++plen;
      index = (index + 1) % __mem::capacity;
    }
    return count;
  }
};

};     // namespace micron
