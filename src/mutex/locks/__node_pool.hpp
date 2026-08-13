//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../except.hpp"
#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-thread slot table, keyed on the lock's address

namespace micron
{

#ifndef MICRON_MCS_DEPTH
#define MICRON_MCS_DEPTH 8      // distinct queue locks one thread may hold at once
#endif

inline atomic_token<u64> __lock_slot_ids{ 1 };

[[nodiscard]] inline u64
__next_lock_slot_id() noexcept
{
  return __lock_slot_ids.fetch_add(1, memory_order::acq_rel);
}

template<typename Slot, usize Depth = MICRON_MCS_DEPTH> class __lock_slot_table
{
  static_assert(Depth > 0, "__lock_slot_table needs at least one slot");

  struct __entry {
    const void *owner;
    u64 id;
    Slot slot;
  };

  struct __table {
    __entry e[Depth]{};
  };

  static __table &
  __tls() noexcept
  {
    static thread_local __table t{};
    return t;
  }

  template<typename Pred>
  [[nodiscard]] static Slot *
  __claim_keyed(const void *lock, u64 id, bool &fresh, Pred evictable) noexcept
  {
    __table &t = __tls();
    usize free = Depth;
    for ( usize i = 0; i < Depth; ++i ) {
      if ( t.e[i].owner == lock ) {
        if ( t.e[i].id != id ) {      // same address, different lock
          fresh = true;
          t.e[i].id = id;
        }
        return &t.e[i].slot;
      }
      if ( t.e[i].owner == nullptr and free == Depth ) free = i;
    }
    if ( free == Depth ) {
      for ( usize i = 0; i < Depth; ++i )
        if ( evictable(static_cast<const Slot &>(t.e[i].slot)) ) {
          free = i;
          break;
        }
    }
    if ( free == Depth ) return nullptr;
    t.e[free].owner = lock;
    t.e[free].id = id;
    fresh = true;
    return &t.e[free].slot;
  }

public:
  [[nodiscard]] static Slot *
  find(const void *lock) noexcept
  {
    __table &t = __tls();
    for ( usize i = 0; i < Depth; ++i )
      if ( t.e[i].owner == lock ) return &t.e[i].slot;
    return nullptr;
  }

  [[nodiscard]] static Slot *
  find(const void *lock, u64 id) noexcept
  {
    __table &t = __tls();
    for ( usize i = 0; i < Depth; ++i )
      if ( t.e[i].owner == lock and t.e[i].id == id ) return &t.e[i].slot;
    return nullptr;
  }

  [[nodiscard]] static Slot *
  claim(const void *lock)
  {
    __table &t = __tls();
    usize free = Depth;
    for ( usize i = 0; i < Depth; ++i ) {
      if ( t.e[i].owner == lock ) return &t.e[i].slot;
      if ( t.e[i].owner == nullptr and free == Depth ) free = i;
    }
    if ( free == Depth ) micron::exc<except::thread_error>("queue-lock slot table exhausted; raise MICRON_MCS_DEPTH");
    t.e[free].owner = lock;
    t.e[free].id = 0;
    return &t.e[free].slot;
  }

  [[nodiscard]] static Slot *
  claim(const void *lock, u64 id, bool &fresh)
  {
    Slot *s = __claim_keyed(lock, id, fresh, [](const Slot &) { return false; });
    if ( s == nullptr ) micron::exc<except::thread_error>("queue-lock slot table exhausted; raise MICRON_MCS_DEPTH");
    return s;
  }

  template<typename Pred>
  [[nodiscard]] static Slot *
  claim_evicting(const void *lock, Pred evictable)
  {
    __table &t = __tls();
    usize free = Depth;
    for ( usize i = 0; i < Depth; ++i ) {
      if ( t.e[i].owner == lock ) return &t.e[i].slot;
      if ( t.e[i].owner == nullptr and free == Depth ) free = i;
    }
    if ( free == Depth ) {
      for ( usize i = 0; i < Depth; ++i )
        if ( evictable(t.e[i].slot) ) {
          free = i;
          break;
        }
    }
    if ( free == Depth )
      micron::exc<except::thread_error>("queue-lock slot table exhausted and every entry is held; raise MICRON_MCS_DEPTH");
    t.e[free].owner = lock;
    t.e[free].id = 0;
    return &t.e[free].slot;
  }

  template<typename Pred>
  [[nodiscard]] static Slot *
  claim_evicting(const void *lock, u64 id, bool &fresh, Pred evictable)
  {
    Slot *s = __claim_keyed(lock, id, fresh, evictable);
    if ( s == nullptr )
      micron::exc<except::thread_error>("queue-lock slot table exhausted and every entry is held; raise MICRON_MCS_DEPTH");
    return s;
  }

  template<typename Pred>
  [[nodiscard]] static Slot *
  try_claim(const void *lock, u64 id, bool &fresh, Pred evictable) noexcept
  {
    return __claim_keyed(lock, id, fresh, evictable);
  }

  static void
  release(const void *lock) noexcept
  {
    __table &t = __tls();
    for ( usize i = 0; i < Depth; ++i )
      if ( t.e[i].owner == lock ) {
        t.e[i].owner = nullptr;
        t.e[i].id = 0;
        return;
      }
  }

  [[nodiscard]] static usize
  depth() noexcept
  {
    __table &t = __tls();
    usize n = 0;
    for ( usize i = 0; i < Depth; ++i )
      if ( t.e[i].owner != nullptr ) ++n;
    return n;
  }

  [[nodiscard]] static constexpr usize
  capacity() noexcept
  {
    return Depth;
  }
};

};      // namespace micron
