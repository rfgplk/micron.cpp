//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../hash/hash.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"

#include "../allocator.hpp"
#include "../except.hpp"
#include "../tags.hpp"
#include "../types.hpp"
#include "../vector/fvector.hpp"

#include "bits.hpp"

namespace micron
{

namespace __hop_impl
{
__attribute__((always_inline)) static inline usize
round_pow2(usize n) noexcept
{
  if ( n < 2 ) return 2;
  usize p = 1;
  while ( p < n ) {
    usize np = p << 1;
    if ( np <= p ) return p;      // overflow: saturate at the top power of two
    p = np;
  }
  return p;
}

__attribute__((always_inline)) static inline usize
round_pow2_down(usize n) noexcept
{
  if ( n < 2 ) return n;
  usize p = 1;
  for ( ;; ) {
    usize np = p << 1;
    if ( np > n || np <= p ) break;      // stop at n, or on overflow (np==0 would loop forever)
    p = np;
  }
  return p;
}

// WARNING: splitmix64 finalizer; needs to be here if we do
// hash % size with a non-pow2 size well fold the high bits into the result
__attribute__((always_inline)) static inline hash64_t
mix(hash64_t h) noexcept
{
  h ^= h >> 30;
  h *= 0xbf58476d1ce4e5b9ULL;
  h ^= h >> 27;
  h *= 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return h;
}
};      // namespace __hop_impl

template<typename K, typename V> struct hopscotch_node {
  hash64_t key;
  V value;

  ~hopscotch_node() = default;

  hopscotch_node() : key(0), value() { }

  hopscotch_node(const hash64_t &k, const V &v) : key(k), value(v) { }

  hopscotch_node(const hash64_t &k, V &&v) : key(k), value(micron::move(v)) { }

  template<typename... Args> hopscotch_node(const hash64_t &k, Args &&...args) : key(k), value(micron::forward<Args>(args)...) { }

  hopscotch_node(const hopscotch_node &o) : key(o.key), value(o.value) { }

  hopscotch_node(hopscotch_node &&o) noexcept : key(o.key), value(micron::move(o.value)) { o.key = 0; }

  hopscotch_node &
  operator=(const hopscotch_node &o)
  {
    if ( this != &o ) {
      key = o.key;
      value = o.value;
    }
    return *this;
  }

  hopscotch_node &
  operator=(hopscotch_node &&o) noexcept
  {
    if ( this != &o ) {
      key = o.key;
      value = micron::move(o.value);
      o.key = 0;
    }
    return *this;
  }

  bool
  operator!() const
  {
    return key == 0;
  }

  explicit
  operator bool() const
  {
    return key != 0;
  }

  template<typename... Args>
  hopscotch_node &
  set(const K &k, Args &&...args)
  {
    key = hash<hash64_t>(k);
    value = V(micron::forward<Args>(args)...);
    return *this;
  }

  hopscotch_node &
  set(const K &k, V &&val)
  {
    key = hash<hash64_t>(k);
    value = micron::move(val);
    return *this;
  }

  void
  clear()
  {
    key = 0;
    value = V{};
  }
};

// ON KEYS: hopscotch_map stores only hash<hash64_t>(key), NOT the key itself,
// and compares entries by that 64-bit hash; two DISTINCT keys that hash to the same value
// are therefore treated as the SAME entry (the second insert overwrites / is dropped; a lookup returns the first key's value)
// this is by design
template<typename K, typename V, usize MH = 32, typename Nd = hopscotch_node<K, V>> class hopscotch_map
{
  micron::fvector<Nd> entries;
  usize length;
  usize __bmask;

  static constexpr usize __min_size = 64;
  static constexpr usize __max_displacement = 256;      // Maximum distance to search for swap

  bool
  find_closer_slot(usize target, usize empty_slot, usize &result_slot)
  {
    if ( __bmask == 0 ) {
      return false;
    }
    const usize size = __bmask + 1u;

    usize distance = (empty_slot >= target) ? (empty_slot - target) : (size - target + empty_slot);
    if ( distance <= MH ) {
      result_slot = empty_slot;
      return true;
    }

    usize current_empty = empty_slot;

    for ( usize attempt = 0; attempt < __max_displacement; ++attempt ) {
      bool found_swap = false;
      for ( usize i = 0; i < MH && !found_swap; ++i ) {
        usize check_pos = (current_empty + size - i - 1) & __bmask;

        if ( !entries[check_pos].key ) {
          continue;      // Skip empty slots
        }

        usize entry_home = __hop_impl::mix(entries[check_pos].key) & __bmask;
        usize dist_to_empty = (current_empty >= entry_home) ? (current_empty - entry_home) : (size - entry_home + current_empty);

        if ( dist_to_empty <= MH ) {
          entries[current_empty] = micron::move(entries[check_pos]);
          entries[check_pos].clear();
          current_empty = check_pos;
          found_swap = true;

          distance = (current_empty >= target) ? (current_empty - target) : (size - target + current_empty);
          if ( distance <= MH ) {
            result_slot = current_empty;
            return true;
          }
        }
      }

      if ( !found_swap ) {
        break;
      }
    }

    return false;
  }

  void
  resize()
  {
    const micron::fvector<Nd> old_entries = micron::move(entries);
    usize old_size = old_entries.size();
    usize new_size = __hop_impl::round_pow2(old_size * 2);

    if ( new_size < __min_size ) {
      new_size = __min_size;
    }

    constexpr usize max_resize_attmpt = 4;
    for ( usize attempt = 0; attempt < max_resize_attmpt; ++attempt ) {
      entries.clear();
      entries.resize(new_size);
      usize target = __hop_impl::round_pow2(entries.max_size());
      if ( target > new_size ) {
        entries.resize(target);
        new_size = target;
      }
      __bmask = new_size - 1u;
      length = 0;

      for ( usize i = 0; i < new_size; ++i ) {
        entries[i].key = 0;
      }

      bool success = true;
      for ( const auto &n : old_entries ) {
        if ( n.key ) {
          if ( !insert_unhash(n.key, n.value) ) {
            success = false;
            break;
          }
        }
      }

      if ( success ) {
        return;
      }

      new_size *= 2;
    }

    exc<except::library_error>("Failed to resize hopscotch map after multiple attempts");
  }

  bool
  insert_unhash(const hash64_t &hsh, const V &value)
  {
    if ( hsh == 0 ) {
      return false;
    }

    if ( __bmask == 0 ) {
      return false;
    }
    usize home = __hop_impl::mix(hsh) & __bmask;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( entries[probe].key == hsh ) {
        return true;
      }
    }

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( !entries[probe].key ) {
        entries[probe] = Nd{ hsh, value };
        ++length;
        return true;
      }
    }

    usize empty_slot = (home + MH + 1) & __bmask;
    constexpr usize SEARCH_LIMIT = 512;

    for ( usize i = 0; i < SEARCH_LIMIT; ++i ) {
      usize check = (empty_slot + i) & __bmask;
      if ( !entries[check].key ) {
        usize result_slot;
        if ( find_closer_slot(home, check, result_slot) ) {
          entries[result_slot] = Nd{ hsh, value };
          ++length;
          return true;
        }
        break;
      }
    }

    return false;
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using key_type = K;
  using mapped_type = V;
  typedef usize size_type;
  typedef Nd value_type;
  typedef Nd &reference;
  typedef Nd &ref;
  typedef const Nd &const_reference;
  typedef const Nd &const_ref;
  typedef Nd *pointer;
  typedef const Nd *const_pointer;
  typedef Nd *iterator;
  typedef const Nd *const_iterator;

  ~hopscotch_map() { }

  hopscotch_map(const usize n = 4096) : length(0), __bmask(0)
  {
    usize initial_size = (n / sizeof(Nd));
    if ( initial_size < __min_size ) {
      initial_size = __min_size;
    }
    initial_size = __hop_impl::round_pow2(initial_size);
    entries.resize(initial_size);
    usize target = __hop_impl::round_pow2(entries.max_size());
    if ( target > initial_size ) {
      entries.resize(target);
      initial_size = target;
    }
    __bmask = initial_size - 1u;

    for ( usize i = 0; i < initial_size; ++i ) {
      entries[i].key = 0;
    }
  }

  hopscotch_map(const hopscotch_map &) = delete;

  hopscotch_map(hopscotch_map &&o) noexcept : entries(micron::move(o.entries)), length(o.length), __bmask(o.__bmask)
  {
    o.length = 0;
    o.__bmask = 0;
  }

  hopscotch_map &operator=(const hopscotch_map &) = delete;

  hopscotch_map &
  operator=(hopscotch_map &&o) noexcept
  {
    if ( this != &o ) {
      entries = micron::move(o.entries);
      length = o.length;
      __bmask = o.__bmask;
      o.length = 0;
      o.__bmask = 0;
    }
    return *this;
  }

  usize
  size() const noexcept
  {
    return length;
  }

  bool
  empty() const noexcept
  {
    return length == 0;
  }

  usize
  capacity() const noexcept
  {
    return entries.max_size();
  }

  float
  load_factor() const noexcept
  {
    return entries.max_size() > 0 ? static_cast<float>(length) / entries.max_size() : 0.0f;
  }

  void
  clear()
  {
    if ( __bmask == 0 ) return;
    // clear only the logical table (__bmask+1)
    usize sz = __bmask + 1u;
    for ( usize i = 0; i < sz; ++i ) {
      entries[i].clear();
    }
    length = 0;
  }

  V *
  insert_asis(const hash64_t &hsh, const V &value)
  {
    if ( hsh == 0 ) {
      exc<except::library_error>("Invalid hash value (0)");
    }

    usize size = entries.max_size();
    if ( size == 0 ) {
      exc<except::library_error>("Hopscotch map not initialized");
    }

    if ( length >= (__bmask + 1u) * 3 / 4 ) {
      resize();
    }

    usize home = __hop_impl::mix(hsh) & __bmask;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( entries[probe].key == hsh ) {
        return micron::addressof(entries[probe].value);
      }
    }

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( !entries[probe].key ) {
        entries[probe] = Nd{ hsh, value };
        ++length;
        return micron::addressof(entries[probe].value);
      }
    }

    usize empty_slot = (home + MH + 1) & __bmask;
    constexpr usize SEARCH_LIMIT = 512;

    for ( usize i = 0; i < SEARCH_LIMIT; ++i ) {
      usize check = (empty_slot + i) & __bmask;
      if ( !entries[check].key ) {
        usize result_slot;
        if ( find_closer_slot(home, check, result_slot) ) {
          entries[result_slot] = Nd{ hsh, value };
          ++length;
          return micron::addressof(entries[result_slot].value);
        }
        break;
      }
    }

    resize();
    return insert_asis(hsh, value);
  }

  V *
  insert(const K &k, const V &value)
  {
    hash64_t hsh = hash<hash64_t>(k);
    return insert_asis(hsh, value);
  }

  V *
  insert(const K &k, V &&value)
  {
    hash64_t hsh = hash<hash64_t>(k);

    if ( hsh == 0 ) {
      exc<except::library_error>("Invalid hash value (0)");
    }

    usize size = entries.max_size();
    if ( size == 0 ) {
      exc<except::library_error>("Hopscotch map not initialized");
    }

    if ( length >= (__bmask + 1u) * 3 / 4 ) {
      resize();
    }

    usize home = __hop_impl::mix(hsh) & __bmask;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( entries[probe].key == hsh ) {
        return micron::addressof(entries[probe].value);
      }
    }

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( !entries[probe].key ) {
        entries[probe] = Nd{ hsh, micron::move(value) };
        ++length;
        return micron::addressof(entries[probe].value);
      }
    }

    usize empty_slot = (home + MH + 1) & __bmask;
    constexpr usize SEARCH_LIMIT = 512;

    for ( usize i = 0; i < SEARCH_LIMIT; ++i ) {
      usize check = (empty_slot + i) & __bmask;
      if ( !entries[check].key ) {
        usize result_slot;
        if ( find_closer_slot(home, check, result_slot) ) {
          entries[result_slot] = Nd{ hsh, micron::move(value) };
          ++length;
          return micron::addressof(entries[result_slot].value);
        }
        break;
      }
    }

    resize();
    return insert(k, micron::move(value));
  }

  template<typename... Args>
  V *
  emplace(const K &k, Args &&...args)
  {
    hash64_t hsh = hash<hash64_t>(k);

    if ( hsh == 0 ) {
      exc<except::library_error>("Invalid hash value (0)");
    }

    usize size = entries.max_size();
    if ( size == 0 ) {
      exc<except::library_error>("Hopscotch map not initialized");
    }

    usize home = __hop_impl::mix(hsh) & __bmask;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( entries[probe].key == hsh ) {
        return micron::addressof(entries[probe].value);
      }
    }

    V value(micron::forward<Args>(args)...);
    return insert(k, micron::move(value));
  }

  V *
  find(const K &k)
  {
    hash64_t hsh = hash<hash64_t>(k);
    if ( hsh == 0 ) {
      return nullptr;
    }

    if ( __bmask == 0 ) {
      return nullptr;
    }

    usize home = __hop_impl::mix(hsh) & __bmask;

    // unrolled 4x probe
    __builtin_prefetch(&entries[home], 0, 1);
    constexpr usize __slots = MH + 1;               // 33 slots
    constexpr usize __tail = __slots & 3u;          // 1 trailing slot
    constexpr usize __head = __slots - __tail;      // 32 covered by groups
    for ( usize i = 0; i < __head; i += 4 ) {
      const usize p0 = (home + i + 0) & __bmask;
      const usize p1 = (home + i + 1) & __bmask;
      const usize p2 = (home + i + 2) & __bmask;
      const usize p3 = (home + i + 3) & __bmask;
      const hash64_t k0 = entries[p0].key;
      const hash64_t k1 = entries[p1].key;
      const hash64_t k2 = entries[p2].key;
      const hash64_t k3 = entries[p3].key;
      u32 hits = (u32(k0 == hsh) << 0) | (u32(k1 == hsh) << 1) | (u32(k2 == hsh) << 2) | (u32(k3 == hsh) << 3);
      if ( hits ) {
        u32 b = static_cast<u32>(__builtin_ctz(hits));
        const usize probe = (home + i + b) & __bmask;
        return micron::addressof(entries[probe].value);
      }
    }
    // scalar tail
    for ( usize i = __head; i < __slots; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( entries[probe].key == hsh ) return micron::addressof(entries[probe].value);
    }
    return nullptr;
  }

  const V *
  find(const K &k) const
  {
    return const_cast<hopscotch_map *>(this)->find(k);
  }

  bool
  contains(const K &k) const
  {
    return find(k) != nullptr;
  }

  bool
  erase(const K &k)
  {
    hash64_t hsh = hash<hash64_t>(k);
    if ( hsh == 0 ) {
      return false;
    }

    if ( __bmask == 0 ) {
      return false;
    }

    usize home = __hop_impl::mix(hsh) & __bmask;

    // unrolled 4x probe
    constexpr usize __slots = MH + 1;
    constexpr usize __tail = __slots & 3u;
    constexpr usize __head = __slots - __tail;
    for ( usize i = 0; i < __head; i += 4 ) {
      const usize p0 = (home + i + 0) & __bmask;
      const usize p1 = (home + i + 1) & __bmask;
      const usize p2 = (home + i + 2) & __bmask;
      const usize p3 = (home + i + 3) & __bmask;
      const hash64_t k0 = entries[p0].key;
      const hash64_t k1 = entries[p1].key;
      const hash64_t k2 = entries[p2].key;
      const hash64_t k3 = entries[p3].key;
      u32 hits = (u32(k0 == hsh) << 0) | (u32(k1 == hsh) << 1) | (u32(k2 == hsh) << 2) | (u32(k3 == hsh) << 3);
      if ( hits ) {
        u32 b = static_cast<u32>(__builtin_ctz(hits));
        const usize probe = (home + i + b) & __bmask;
        entries[probe].clear();
        --length;
        return true;
      }
    }
    for ( usize i = __head; i < __slots; ++i ) {
      usize probe = (home + i) & __bmask;
      if ( entries[probe].key == hsh ) {
        entries[probe].clear();
        --length;
        return true;
      }
    }
    return false;
  }

  V &
  operator[](const K &k)
  {
    V *v = find(k);
    if ( v ) {
      return *v;
    }

    V *result = insert(k, V{});
    if ( !result ) {
      exc<except::library_error>("Failed to insert into hopscotch map");
    }
    return *result;
  }

  V &
  at(const K &k)
  {
    V *v = find(k);
    if ( !v ) {
      exc<except::library_error>("Key not found in hopscotch map");
    }
    return *v;
  }

  const V &
  at(const K &k) const
  {
    const V *v = find(k);
    if ( !v ) {
      exc<except::library_error>("Key not found in hopscotch map");
    }
    return *v;
  }

  V &
  add(const K &k, V value)
  {
    V *result = insert(k, micron::move(value));
    if ( !result ) {
      exc<except::library_error>("Failed to add to hopscotch map");
    }
    return *result;
  }

  iterator
  begin()
  {
    return entries.data();
  }

  iterator
  end()
  {
    return entries.data() + entries.size();
  }

  const_iterator
  begin() const
  {
    return entries.data();
  }

  const_iterator
  end() const
  {
    return entries.data() + entries.size();
  }

  const_iterator
  cbegin() const
  {
    return entries.data();
  }

  const_iterator
  cend() const
  {
    return entries.data() + entries.size();
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const hash64_t &, V &>
  void
  for_each(Fn &&fn)
  {
    if ( __bmask == 0 ) return;      // moved-from / uninitialized: null entries buffer
    usize sz = __bmask + 1u;
    for ( usize i = 0; i < sz; ++i )
      if ( entries[i].key != 0 ) fn(entries[i].key, entries[i].value);
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const hash64_t &, const V &>
  void
  for_each(Fn &&fn) const
  {
    if ( __bmask == 0 ) return;      // moved-from / uninitialized: null entries buffer
    usize sz = __bmask + 1u;
    for ( usize i = 0; i < sz; ++i )
      if ( entries[i].key != 0 ) fn(entries[i].key, entries[i].value);
  }
};

template<typename K, typename V, usize MH = 32, typename Nd = hopscotch_node<K, V>> using hopscotch = hopscotch_map<K, V, MH, Nd>;

}      // namespace micron
