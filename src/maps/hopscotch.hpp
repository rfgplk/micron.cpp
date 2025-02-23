#pragma once

#include "../hash/hash.hpp"
#include "../memory/actions.hpp"

#include "../allocation/chunks.hpp"
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
  hopscotch_node() {}
  hopscotch_node(const hash64_t &k, V &&v) : key(k), value(micron::move(v)) {}
  template <typename... Args> hopscotch_node(const hash64_t &k, Args &&...args) : key(k), value(args...) {}

  hopscotch_node(const hopscotch_node &o) : key(o.key), value(o.value) {};
  hopscotch_node(hopscotch_node &&o) : key(o.key), value(micron::move(o.value)) {};
  hopscotch_node &operator=(const hopscotch_node &) = default;
  hopscotch_node &operator=(hopscotch_node &&) = default;
  bool
  operator!(void)
  {
    return !value;     // not set = true, set false
  }
  template <typename... Args>
  hopscotch_node &
  set(const K &k, Args &&...args)
  {
    key = hash<hash64_t>(k);
    value = micron::move(V(micron::forward(args)...));
    return *this;
  }
  hopscotch_node &
  set(const K &k, V &&val)
  {
    key = hash<hash64_t>(k);
    value = micron::move(val);
    return *this;
  }
};

template <typename K, typename V, size_t max = 4, typename Nd = hopscotch_node<K, V>> class hopscotch_map
{
  micron::fvector<hopscotch_node<K, V>> entries;
  size_t length;
  void
  resize()
  {
    micron::fvector<hopscotch_node<K, V>> t = micron::move(entries);
    entries.recreate(t.max_size() * 3);
    std::cout << entries.max_size() << std::endl;
    length = 0;
    for ( auto &e : t ) {
      if ( e.key ) {
        insert_asis(e.key, e.value);
      }
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
  ~hopscotch_map() {}
  hopscotch_map(const size_t n = 4096) : length(0) { entries.resize(n / sizeof(Nd)); }
  hopscotch_map(const hopscotch_map &) = delete;
  hopscotch_map(hopscotch_map &&o) : entries(micron::move(o.entries)) {}
  hopscotch_map &operator=(const hopscotch_map &) = delete;
  hopscotch_map &
  operator=(hopscotch_map &&o)
  {
    entries = micron::move(o.entries);
    return *this;
  };
  V &
  insert_asis(const hash64_t &k, V &value)
  {
    if ( length >= entries.max_size() / 2 )
      resize();
    hash64_t hsh = (k);
    size_t id = hsh % entries.max_size();
    for ( size_t i = 0; i <= max; i++ ) {
      size_t probe = (id + i) % entries.max_size();
      if ( !entries[probe].key ) {
        // hit
        entries[probe] = { hsh, value };
        length++;
        return entries[probe].value;
      }
    }

    for ( size_t j = 0; j <= max; j++ ) {
      size_t probe = (id + j) % entries.max_size();
      if ( entries[probe].key ) {
        for ( size_t kl = 1; kl <= max; kl++ ) {
          size_t hop = (probe + kl) % entries.max_size();
          if ( !entries[hop].key ) {
            entries[hop] = { entries[probe].key, entries[probe].value };
            entries[probe] = { hsh, value };
            return entries[probe].value;
          }
        }
      }
    }
    throw except::library_error("failed to insert");
  }

  V &
  insert(const K &k, V &&value)
  {
    if ( length >= entries.max_size() / 2 )
      resize();
    hash64_t hsh = hash<hash64_t>(k);
    size_t id = hsh % entries.max_size();
    for ( size_t i = 0; i <= max; i++ ) {
      size_t probe = (id + i) % entries.max_size();
      if ( !entries[probe].key ) {
        // hit
        entries[probe] = { hsh, value };
        length++;
        return entries[probe].value;
      }
    }

    for ( size_t j = 0; j <= max; j++ ) {
      size_t probe = (id + j) % entries.max_size();
      if ( entries[probe].key ) {
        for ( size_t kl = 1; kl <= max; kl++ ) {
          size_t hop = (probe + kl) % entries.max_size();
          if ( !entries[hop].key ) {
            entries[hop] = { entries[probe].key, entries[probe].value };
            entries[probe] = { hsh, value };
            return entries[probe].value;
          }
        }
      }
    }
    throw except::library_error("failed to insert");
  }
  V &
  find(const K &k)
  {
    size_t hsh = hash<hash64_t>(k);
    size_t id = hsh % entries.max_size();
    for ( size_t i = 0; i <= max; i++ ) {
      size_t probe = (id + i) % entries.max_size();
      if ( entries[probe].key == hsh )
        return entries[probe].value;
    }
    insert(k, V{});
  }
  V &
  operator[](const K &k)
  {
    return find(k);
  }
};

};
