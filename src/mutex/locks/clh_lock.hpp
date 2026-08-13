//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../except.hpp"
#include "../../memory/cache.hpp"

#include "../backoff.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CLH queue lock
//
// FIFO like MCS, and cheaper to release;
// no spin waiting

struct alignas(cache_line_size()) clh_node {
  atomic_token<bool> locked;

  constexpr clh_node() noexcept : locked(false) { }
};

template<usize MaxThreads = 32, spin_policy P = spin_yield> class basic_clh_lock
{
  static_assert(MaxThreads > 0, "basic_clh_lock needs room for at least one thread");

  static constexpr u32 __node_count = static_cast<u32>(MaxThreads) + 1u;      // + 1 for the initial dummy

  alignas(cache_line_size()) atomic_token<clh_node *> __tail;
  atomic_token<clh_node *> __held;          // the node the holder published, or null when unheld
  atomic_token<clh_node *> __consumed;      // the node it inherited; recycled by the release

  alignas(cache_line_size()) atomic_token<u8> __pooled[__node_count];
  atomic_token<u32> __drawn;      // nodes in existence; only grows when the pool comes up empty
  atomic_token<u32> __hint;       // most recently returned index: the scan's first guess

  clh_node __arena[__node_count];
  [[no_unique_address]] __lock_stats st;

  clh_node *
  __take() noexcept
  {
    for ( u32 pass = 0; pass < 2u; ++pass ) {
      const u32 n = __drawn.get(memory_order::relaxed);
      u32 i = __hint.get(memory_order::relaxed);
      if ( i >= n ) i = 0u;
      for ( u32 k = 0; k < n; ++k ) {
        if ( __pooled[i].get(memory_order::relaxed) != 0u
             and __pooled[i].compare_and_swap(1u, 0u, memory_order::acq_rel, memory_order::relaxed) )
          return &__arena[i];
        if ( ++i >= n ) i = 0u;
      }
      u32 d = n;
      while ( d < __node_count )
        if ( __drawn.compare_exchange_weak(d, d + 1u, memory_order::acq_rel, memory_order::relaxed) ) return &__arena[d];
    }
    return nullptr;
  }

  void
  __give(clh_node *n) noexcept
  {
    const u32 i = static_cast<u32>(n - __arena);
    __pooled[i].store(1u, memory_order::release);
    __hint.store(i, memory_order::relaxed);
  }

  void
  reset() noexcept
  {
    unlock();
  }

public:
  using node_type = clh_node;

  ~basic_clh_lock() = default;

  basic_clh_lock() noexcept : __tail(nullptr), __held(nullptr), __consumed(nullptr), __drawn(1), __hint(0)
  {
    for ( u32 i = 0; i < __node_count; ++i ) __pooled[i].store(0u, memory_order::relaxed);
    __arena[0].locked.store(false, memory_order::relaxed);
    __tail.store(&__arena[0], memory_order::release);
  }

  basic_clh_lock(const basic_clh_lock &) = delete;
  basic_clh_lock(basic_clh_lock &&) = delete;
  basic_clh_lock &operator=(const basic_clh_lock &) = delete;

  auto
  operator()()
  {
    clh_node *me = __take();
    if ( me == nullptr ) micron::exc<except::thread_error>("clh_lock arena exhausted; raise its MaxThreads");

    me->locked.store(true, memory_order::relaxed);
    clh_node *pred = __tail.swap(me, memory_order::acq_rel);

    if ( pred->locked.get(memory_order::acquire) ) {
      __counted_backoff<P> bo(st);
      while ( pred->locked.get(memory_order::acquire) ) bo.relax();
    }

    __consumed.store(pred, memory_order::relaxed);
    __held.store(me, memory_order::release);      // release: pairs with the acquire in unlock()
    st.note_acquire();
    return &basic_clh_lock::reset;
  }

  auto
  lock()
  {
    return operator()();
  }

  // CLH has no way to back out of the tail swap;
  // this succeeds only when the queue is provably empty
  bool
  try_lock() noexcept
  {
    clh_node *t = __tail.get(memory_order::acquire);
    if ( t == nullptr or t->locked.get(memory_order::acquire) ) return false;

    clh_node *me = __take();
    if ( me == nullptr ) return false;      // the arena is full; try_lock may not throw

    me->locked.store(true, memory_order::relaxed);
    if ( !__tail.compare_and_swap(t, me, memory_order::acq_rel, memory_order::acquire) ) {
      me->locked.store(false, memory_order::relaxed);
      __give(me);
      return false;
    }

    if ( t->locked.get(memory_order::acquire) ) {
      if ( __tail.compare_and_swap(me, t, memory_order::acq_rel, memory_order::relaxed) ) {
        me->locked.store(false, memory_order::relaxed);
        __give(me);
        return false;
      }
      __counted_backoff<P> bo(st);
      while ( t->locked.get(memory_order::acquire) ) bo.relax();
      __give(t);
      me->locked.store(false, memory_order::release);
      return false;
    }

    __consumed.store(t, memory_order::relaxed);
    __held.store(me, memory_order::release);
    st.note_acquire();
    return true;
  }

  // pooling the inherited node before the handoff is safe
  void
  unlock() noexcept
  {
    clh_node *me = __held.get(memory_order::acquire);
    if ( me == nullptr ) return;

    clh_node *pred = __consumed.get(memory_order::relaxed);
    __held.store(nullptr, memory_order::release);
    __give(pred);
    me->locked.store(false, memory_order::release);
  }

  auto
  retrieve() noexcept
  {
    return &basic_clh_lock::reset;
  }

  bool
  operator!() const noexcept
  {
    return !is_locked();
  }

  bool
  is_locked() const noexcept
  {
    const clh_node *t = __tail.get(memory_order::relaxed);
    return t != nullptr and t->locked.get(memory_order::relaxed);
  }

  [[nodiscard]] u32
  participants() const noexcept
  {
    return __drawn.get(memory_order::relaxed) - 1u;
  }

  [[nodiscard]] const __lock_stats &
  stats() const noexcept
  {
    return st;
  }

  template<typename... T> friend void unlock(T &...);
};

using clh_lock = basic_clh_lock<32, spin_yield>;

};      // namespace micron
