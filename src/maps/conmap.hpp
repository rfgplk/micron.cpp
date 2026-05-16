//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../bits.hpp"
#include "../hash/hash.hpp"
#include "../types.hpp"

#include "../allocator.hpp"
#include "../except.hpp"
#include "../memory/cache.hpp"
#include "../memory/new.hpp"
#include "../mutex/locks/guard_lock.hpp"
#include "../mutex/locks/spin_lock.hpp"
#include "../tuple.hpp"

#include "robin.hpp"

#include <new>

namespace micron
{

// conmap
//
// striped robin_map; capacity is fixed, no resizing
// each stripe owns its own robin_map and a spin_lock
// routing is the high 8 bits of the 64-bit hash, the low bits go
// into the per-stripe robin probe so there is no correlation

// non-resizable: rehash semantics across stripes would defeat the lock-free dynamics
template<typename K, typename V, usize Stripes = 64, class Alloc = micron::allocator_serial<>>
  requires(Stripes >= 1 and (Stripes & (Stripes - 1)) == 0 and micron::is_move_constructible_v<V>)
class conmap
{
  static constexpr usize __stripe_mask = Stripes - 1u;
  static constexpr u64 __cache_line = cache_line_size();

  using __map_t = robin_map<K, V, Alloc>;

  struct __stripe {
    spin_lock lock;
    __map_t map;
    // pad to a multiple of cache line to keep stripes on separate lines
    char __pad[((sizeof(spin_lock) + sizeof(__map_t)) % __cache_line == 0)
                   ? 1
                   : __cache_line - ((sizeof(spin_lock) + sizeof(__map_t)) % __cache_line)];

    __stripe(usize cap_per_stripe) : lock(), map(cap_per_stripe) { }
  };

  __stripe *__stripes_buf = nullptr;
  usize __per_stripe_cap = 0;

  static usize
  __sid(hash64_t h) noexcept
  {
    u64 x = static_cast<u64>(h);
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    x = x ^ (x >> 31);
    return static_cast<usize>(x) & __stripe_mask;
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = K;
  using mapped_type = V;

  ~conmap()
  {
    if ( !__stripes_buf ) return;
    for ( usize i = 0; i < Stripes; ++i ) __stripes_buf[i].~__stripe();
    ::operator delete(__stripes_buf);
    __stripes_buf = nullptr;
  }

  conmap(const conmap &) = delete;
  conmap &operator=(const conmap &) = delete;

  explicit conmap(usize total_capacity = Stripes * 64u)
  {
    __per_stripe_cap = total_capacity / Stripes;
    if ( __per_stripe_cap < 16 ) __per_stripe_cap = 16;
    __stripes_buf = static_cast<__stripe *>(::operator new(sizeof(__stripe) * Stripes));
    for ( usize i = 0; i < Stripes; ++i ) {
      new (&__stripes_buf[i]) __stripe(__per_stripe_cap);
    }
  }

  conmap(conmap &&o) noexcept : __stripes_buf(o.__stripes_buf), __per_stripe_cap(o.__per_stripe_cap)
  {
    o.__stripes_buf = nullptr;
    o.__per_stripe_cap = 0;
  }

  conmap &
  operator=(conmap &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( __stripes_buf ) {
      for ( usize i = 0; i < Stripes; ++i ) __stripes_buf[i].~__stripe();
      ::operator delete(__stripes_buf);
    }
    __stripes_buf = o.__stripes_buf;
    __per_stripe_cap = o.__per_stripe_cap;
    o.__stripes_buf = nullptr;
    o.__per_stripe_cap = 0;
    return *this;
  }

  static constexpr usize
  stripe_count() noexcept
  {
    return Stripes;
  }

  usize
  size() const noexcept
  {
    usize total = 0;
    for ( usize i = 0; i < Stripes; ++i ) total += __stripes_buf[i].map.size();
    return total;
  }

  usize
  capacity() const noexcept
  {
    usize total = 0;
    for ( usize i = 0; i < Stripes; ++i ) total += __stripes_buf[i].map.max_size();
    return total;
  }

  bool
  empty() const noexcept
  {
    return size() == 0;
  }

  void
  clear() noexcept
  {
    for ( usize i = 0; i < Stripes; ++i ) {
      auto reset = __stripes_buf[i].lock.lock();
      __stripes_buf[i].map.clear();
      (__stripes_buf[i].lock.*reset)();
    }
  }

  bool
  insert(const K &k, const V &v)
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    bool existed = (s.map.find_hash(kh, k) != nullptr);
    if ( !existed ) {
      V cv = v;
      s.map.insert(k, micron::move(cv));
    }
    (s.lock.*reset)();
    return !existed;
  }

  bool
  insert(K &&k, V &&v)
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    bool existed = (s.map.find_hash(kh, k) != nullptr);
    if ( !existed ) {
      K kc = micron::move(k);
      s.map.insert(micron::move(kc), micron::move(v));
    }
    (s.lock.*reset)();
    return !existed;
  }

  float
  load_factor() const noexcept
  {
    usize cap = capacity();
    return cap > 0u ? static_cast<float>(size()) / static_cast<float>(cap) : 0.0f;
  }

  usize
  max_size() const noexcept
  {
    return capacity();
  }

  void
  swap(conmap &o) noexcept
  {
    __stripe *t = __stripes_buf;
    __stripes_buf = o.__stripes_buf;
    o.__stripes_buf = t;
    usize p = __per_stripe_cap;
    __per_stripe_cap = o.__per_stripe_cap;
    o.__per_stripe_cap = p;
  }

  V
  at(const K &k) const
  {
    V v{};
    if ( !find(k, v) ) [[unlikely]]
      exc<except::library_error>("conmap::at(): key not found");
    return v;
  }

  template<typename... Args>
  bool
  emplace(const K &k, Args &&...args)
  {
    return insert(k, V(micron::forward<Args>(args)...));
  }

  bool
  insert_or_assign(const K &k, const V &v)
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    V *ex = s.map.find_hash(kh, k);
    bool newly = (ex == nullptr);
    if ( newly ) {
      V cv = v;
      s.map.insert(k, micron::move(cv));
    } else {
      *ex = v;
    }
    (s.lock.*reset)();
    return newly;
  }

  bool
  find(const K &k, V &out) const
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    const V *p = s.map.find_hash(kh, k);
    bool ok = (p != nullptr);
    if ( ok ) out = *p;
    (s.lock.*reset)();
    return ok;
  }

  bool
  contains(const K &k) const
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    bool found = (s.map.find_hash(kh, k) != nullptr);
    (s.lock.*reset)();
    return found;
  }

  usize
  count(const K &k) const
  {
    return contains(k) ? 1u : 0u;
  }

  bool
  erase(const K &k)
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    bool removed = s.map.erase_hash(kh, k);
    (s.lock.*reset)();
    return removed;
  }

  template<typename Fn>
  bool
  update(const K &k, Fn &&fn)
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    V *p = s.map.find_hash(kh, k);
    bool ok = (p != nullptr);
    if ( ok ) fn(*p);
    (s.lock.*reset)();
    return ok;
  }

  template<typename Fn>
  bool
  upsert(const K &k, Fn &&fn, V fallback)
  {
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    auto reset = s.lock.lock();
    V *p = s.map.find_hash(kh, k);
    bool inserted = false;
    if ( p ) {
      fn(*p);
    } else {
      s.map.insert(k, micron::move(fallback));
      inserted = true;
    }
    (s.lock.*reset)();
    return inserted;
  }

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    for ( usize i = 0; i < Stripes; ++i ) {
      auto reset = __stripes_buf[i].lock.lock();
      __stripes_buf[i].map.for_each([&](auto &node) { fn(node.key, node.value); });
      (__stripes_buf[i].lock.*reset)();
    }
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    for ( usize i = 0; i < Stripes; ++i ) {
      auto reset = __stripes_buf[i].lock.lock();
      __stripes_buf[i].map.for_each([&](const auto &node) { fn(node.key, node.value); });
      (__stripes_buf[i].lock.*reset)();
    }
  }
};

};      // namespace micron
