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

namespace micron
{

// conmap
//
// striped robin_map; capacity is fixed, no resizing
// each stripe owns its own robin_map and a spin_lock
// routing runs a splitmix64 finalizer over the whole hash and masks
// to the stripe count, decorrelating the stripe index from the per-stripe robin
// probe (which uses the low bits of the same hash)
//
// THREAD SAFETY: per-operation thread-safe (each public op locks at most ONE
// stripe via RAII, so a throw cannot leak the lock)

// non-resizable: rehash semantics across stripes would defeat the lock-free dynamics
template<typename K, typename V, usize Stripes = 64, class Alloc = micron::allocator_serial<>>
  requires(Stripes >= 1 and (Stripes & (Stripes - 1)) == 0 and micron::is_move_constructible_v<V>)
class conmap
{
  static constexpr usize __stripe_mask = Stripes - 1u;
  static constexpr u64 __cache_line = cache_line_size();

  using __map_t = robin_map<K, V, Alloc>;

  struct alignas(__cache_line) __stripe {
    mutable spin_lock lock;
    __map_t map;

    __stripe(usize cap_per_stripe) : lock(), map(cap_per_stripe) { }
  };

  class __guard
  {
    spin_lock *__lock;

  public:
    __attribute__((always_inline)) explicit __guard(spin_lock &lock) : __lock(micron::addressof(lock)) { __lock->lock(); }
    __guard(const __guard &) = delete;
    __guard &operator=(const __guard &) = delete;
    __attribute__((always_inline)) ~__guard() { __lock->unlock(); }
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
    ::operator delete(__stripes_buf, static_cast<std::align_val_t>(__cache_line));
    __stripes_buf = nullptr;
  }

  conmap(const conmap &) = delete;
  conmap &operator=(const conmap &) = delete;

  explicit conmap(usize total_capacity = Stripes * 64u)
  {
    if constexpr ( Stripes > static_cast<usize>(-1) / sizeof(__stripe) )
      exc<except::library_error>("conmap: stripe allocation overflow");
    __per_stripe_cap = total_capacity / Stripes;
    if ( __per_stripe_cap < 16 ) __per_stripe_cap = 16;
    __stripes_buf = static_cast<__stripe *>(
        ::operator new(sizeof(__stripe) * Stripes, static_cast<std::align_val_t>(__cache_line)));
#if !defined(__micron_freestanding) || defined(__micron_eh)
    usize built = 0;
    try {
      for ( ; built < Stripes; ++built ) new (&__stripes_buf[built]) __stripe(__per_stripe_cap);
    } catch ( ... ) {
      while ( built ) __stripes_buf[--built].~__stripe();
      ::operator delete(__stripes_buf, static_cast<std::align_val_t>(__cache_line));
      __stripes_buf = nullptr;
      throw;
    }
#else
    for ( usize i = 0; i < Stripes; ++i ) {
      new (&__stripes_buf[i]) __stripe(__per_stripe_cap);
    }
#endif
  }

  // NOT safe to move/move-assign/swap concurrently with any other access to
  // either map
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
      ::operator delete(__stripes_buf, static_cast<std::align_val_t>(__cache_line));
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

  // approximate under concurrent mutation
  usize
  size() const noexcept
  {
    if ( !__stripes_buf ) return 0;
    usize total = 0;
    for ( usize i = 0; i < Stripes; ++i ) {
      __guard __g(__stripes_buf[i].lock);
      total += __stripes_buf[i].map.size();
    }
    return total;
  }

  usize
  capacity() const noexcept
  {
    if ( !__stripes_buf ) return 0;
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
    if ( !__stripes_buf ) return;
    for ( usize i = 0; i < Stripes; ++i ) {
      __guard __g(__stripes_buf[i].lock);
      __stripes_buf[i].map.clear();
    }
  }

  bool
  insert_hash(hash64_t kh, const K &k, const V &v)
  {
    if ( !__stripes_buf ) [[unlikely]]
      exc<except::library_error>("conmap: insert on moved-from map");
    __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);      // RAII: a throw from robin (stripe full) still unlocks
    bool existed = (s.map.find_hash(kh, k) != nullptr);
    if ( !existed ) {
      V cv = v;
      s.map.insert_hash(kh, k, micron::move(cv));
    }
    return !existed;
  }

  bool
  insert_hash(hash64_t kh, K &&k, V &&v)
  {
    if ( !__stripes_buf ) [[unlikely]]
      exc<except::library_error>("conmap: insert on moved-from map");
    __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);
    return s.map.insert_hash_if_absent(kh, micron::move(k), micron::move(v)).a;
  }

  bool
  insert_hash(hash64_t kh, const K &k, V &&v)
  {
    if ( !__stripes_buf ) [[unlikely]]
      exc<except::library_error>("conmap: insert on moved-from map");
    __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);
    return s.map.insert_hash_if_absent(kh, k, micron::move(v)).a;
  }

  bool
  insert(const K &k, const V &v)
  {
    return insert_hash(hash<hash64_t>(k), k, v);
  }

  bool
  insert(K &&k, V &&v)
  {
    hash64_t kh = hash<hash64_t>(k);
    return insert_hash(kh, micron::move(k), micron::move(v));
  }

  bool
  insert(const K &k, V &&v)
  {
    return insert_hash(hash<hash64_t>(k), k, micron::move(v));
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
    if ( !__stripes_buf ) [[unlikely]]
      exc<except::library_error>("conmap: insert on moved-from map");
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);
    V *ex = s.map.find_hash(kh, k);
    bool newly = (ex == nullptr);
    if ( newly ) {
      V cv = v;
      s.map.insert_hash(kh, k, micron::move(cv));
    } else {
      *ex = v;
    }
    return newly;
  }

  bool
  find_hash(hash64_t kh, const K &k, V &out) const
  {
    if ( !__stripes_buf ) return false;
    const __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);        // s.lock is mutable
    const V *p = s.map.find_hash(kh, k);      // const overload: a const conmap does not mutate
    bool ok = (p != nullptr);
    if ( ok ) out = *p;
    return ok;
  }

  bool
  find(const K &k, V &out) const
  {
    return find_hash(hash<hash64_t>(k), k, out);
  }

  bool
  contains_hash(hash64_t kh, const K &k) const
  {
    if ( !__stripes_buf ) return false;
    const __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);
    return s.map.find_hash(kh, k) != nullptr;
  }

  bool
  contains(const K &k) const
  {
    return contains_hash(hash<hash64_t>(k), k);
  }

  usize
  count(const K &k) const
  {
    return contains(k) ? 1u : 0u;
  }

  bool
  erase_hash(hash64_t kh, const K &k)
  {
    if ( !__stripes_buf ) return false;
    __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);
    return s.map.erase_hash(kh, k);
  }

  bool
  erase(const K &k)
  {
    return erase_hash(hash<hash64_t>(k), k);
  }

  template<typename Fn>
  bool
  update(const K &k, Fn &&fn)
  {
    if ( !__stripes_buf ) return false;
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);
    V *p = s.map.find_hash(kh, k);
    bool ok = (p != nullptr);
    if ( ok ) fn(*p);
    return ok;
  }

  template<typename Fn>
  bool
  upsert(const K &k, Fn &&fn, V fallback)
  {
    if ( !__stripes_buf ) [[unlikely]]
      exc<except::library_error>("conmap: insert on moved-from map");
    hash64_t kh = hash<hash64_t>(k);
    __stripe &s = __stripes_buf[__sid(kh)];
    __guard __g(s.lock);
    auto result = s.map.insert_hash_if_absent(kh, k, micron::move(fallback));
    if ( !result.a ) fn(*result.b);
    return result.a;
  }

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    if ( !__stripes_buf ) return;
    for ( usize i = 0; i < Stripes; ++i ) {
      __guard __g(__stripes_buf[i].lock);
      __stripes_buf[i].map.for_each([&](auto &node) { fn(node.key, node.value); });
    }
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    if ( !__stripes_buf ) return;
    for ( usize i = 0; i < Stripes; ++i ) {
      const __stripe &st = __stripes_buf[i];
      __guard __g(st.lock);
      st.map.for_each([&](const auto &node) { fn(node.key, node.value); });
    }
  }
};

};      // namespace micron
