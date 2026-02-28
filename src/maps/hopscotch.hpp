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

namespace micron
{

template <typename K, typename V> struct hopscotch_node {
  hash64_t key;
  V value;

  ~hopscotch_node() = default;

  hopscotch_node() : key(0), value() {}

  hopscotch_node(const hash64_t &k, const V &v) : key(k), value(v) {}

  hopscotch_node(const hash64_t &k, V &&v) : key(k), value(micron::move(v)) {}

  template <typename... Args> hopscotch_node(const hash64_t &k, Args &&...args) : key(k), value(micron::forward<Args>(args)...) {}

  hopscotch_node(const hopscotch_node &o) : key(o.key), value(o.value) {}

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

  template <typename... Args>
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

template <typename K, typename V, usize MH = 32, typename Nd = hopscotch_node<K, V>> class hopscotch_map
{
  micron::fvector<Nd> entries;
  usize length;

  static constexpr usize __min_size = 64;
  static constexpr usize __max_displacement = 256;     // Maximum distance to search for swap

  bool
  find_closer_slot(usize target, usize empty_slot, usize &result_slot)
  {
    usize size = entries.max_size();
    if ( size == 0 ) {
      return false;
    }

    usize distance = (empty_slot >= target) ? (empty_slot - target) : (size - target + empty_slot);
    if ( distance <= MH ) {
      result_slot = empty_slot;
      return true;
    }

    usize current_empty = empty_slot;

    for ( usize attempt = 0; attempt < __max_displacement; ++attempt ) {
      usize search_start = (current_empty >= MH) ? (current_empty - MH) : 0;

      bool found_swap = false;
      for ( usize i = 0; i < MH && !found_swap; ++i ) {
        usize check_pos = (current_empty + size - i - 1) % size;

        if ( !entries[check_pos].key ) {
          continue;     // Skip empty slots
        }

        usize entry_home = entries[check_pos].key % size;
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
    usize new_size = old_size * 2;

    if ( new_size < __min_size ) {
      new_size = __min_size;
    }

    constexpr usize max_resize_attmpt = 4;
    for ( usize attempt = 0; attempt < max_resize_attmpt; ++attempt ) {
      entries.clear();
      entries.resize(new_size);
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

    usize size = entries.max_size();
    if ( size == 0 ) {
      return false;
    }
    usize home = hsh % size;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( entries[probe].key == hsh ) {
        return true;
      }
    }

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( !entries[probe].key ) {
        entries[probe] = Nd{ hsh, value };
        ++length;
        return true;
      }
    }

    usize empty_slot = (home + MH + 1) % size;
    constexpr usize SEARCH_LIMIT = 512;

    for ( usize i = 0; i < SEARCH_LIMIT; ++i ) {
      usize check = (empty_slot + i) % size;
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

  ~hopscotch_map() {}

  hopscotch_map(const usize n = 4096) : length(0)
  {
    usize initial_size = (n / sizeof(Nd));
    if ( initial_size < __min_size ) {
      initial_size = __min_size;
    }
    entries.resize(initial_size);

    for ( usize i = 0; i < initial_size; ++i ) {
      entries[i].key = 0;
    }
  }

  hopscotch_map(const hopscotch_map &) = delete;

  hopscotch_map(hopscotch_map &&o) noexcept : entries(micron::move(o.entries)), length(o.length) { o.length = 0; }

  hopscotch_map &operator=(const hopscotch_map &) = delete;

  hopscotch_map &
  operator=(hopscotch_map &&o) noexcept
  {
    if ( this != &o ) {
      entries = micron::move(o.entries);
      length = o.length;
      o.length = 0;
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
    for ( auto &entry : entries ) {
      entry.clear();
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

    if ( length >= entries.max_size() * 3 / 4 ) {
      resize();
      size = entries.max_size();
    }

    usize home = hsh % size;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( entries[probe].key == hsh ) {
        return &entries[probe].value;
      }
    }

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( !entries[probe].key ) {
        entries[probe] = Nd{ hsh, value };
        ++length;
        return &entries[probe].value;
      }
    }

    usize empty_slot = (home + MH + 1) % size;
    constexpr usize SEARCH_LIMIT = 512;

    for ( usize i = 0; i < SEARCH_LIMIT; ++i ) {
      usize check = (empty_slot + i) % size;
      if ( !entries[check].key ) {
        usize result_slot;
        if ( find_closer_slot(home, check, result_slot) ) {
          entries[result_slot] = Nd{ hsh, value };
          ++length;
          return &entries[result_slot].value;
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

    if ( length >= entries.max_size() * 3 / 4 ) {
      resize();
      size = entries.max_size();
    }

    usize home = hsh % size;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( entries[probe].key == hsh ) {
        return micron::addressof(entries[probe].value);
      }
    }

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( !entries[probe].key ) {
        entries[probe] = Nd{ hsh, micron::move(value) };
        ++length;
        return micron::addressof(entries[probe].value);
      }
    }

    usize empty_slot = (home + MH + 1) % size;
    constexpr usize SEARCH_LIMIT = 512;

    for ( usize i = 0; i < SEARCH_LIMIT; ++i ) {
      usize check = (empty_slot + i) % size;
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

  template <typename... Args>
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

    usize home = hsh % size;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( entries[probe].key == hsh ) {
        return &entries[probe].value;
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

    usize size = entries.max_size();
    if ( size == 0 ) {
      return nullptr;
    }

    usize home = hsh % size;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
      if ( entries[probe].key == hsh ) {
        return micron::addressof(entries[probe].value);
      }
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

    usize size = entries.max_size();
    if ( size == 0 ) {
      return false;
    }

    usize home = hsh % size;

    for ( usize i = 0; i <= MH; ++i ) {
      usize probe = (home + i) % size;
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
};

}     // namespace micron
