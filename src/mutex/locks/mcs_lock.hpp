//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../mutex.hpp"

#include "../backoff.hpp"
#include "__node_pool.hpp"

namespace micron
{

#ifndef MICRON_MCS_RECURSION_ROUNDS
#define MICRON_MCS_RECURSION_ROUNDS 1024u
#endif

// %%%%%%%%%%%%%%%%%%
// queuing_mutex
//
// WARNING: RELEASE IS BY THE LOCK, NOT BY THE THREAD;
// unlock() with nothing held is noop; a second seqd unlock() of one acquisition is a noop; not recursive; THE ACQUIRING THREAD MUST NEVER
// EXIT AND THE LOCK MUST NOT BE DTORD WHILE AN ACQ IS PENDING
enum class __mcs_slot_state : u8 { idle = 0, busy = 1, releasing = 2 };

struct __mcs_slot {
  mcs_node node;
  atomic_token<__mcs_slot_state> state;

  constexpr __mcs_slot() noexcept : node(), state(__mcs_slot_state::idle) { }
};

class mcs_lock
{
  using __pool = __lock_slot_table<__mcs_slot>;

  const u64 __id;
  queuing_mutex __q;
  atomic_token<__mcs_slot *> __holder;
  [[no_unique_address]] __lock_stats st;

  static bool
  __idle(const __mcs_slot &s) noexcept
  {
    return s.state.get(memory_order::acquire) == __mcs_slot_state::idle;
  }

  [[gnu::noinline]] void
  __await_node(__mcs_slot *s)
  {
    default_backoff bo;
    for ( ;; ) {
      // avoid shadowing
      const __mcs_slot_state __st = s->state.get(memory_order::acquire);
      if ( __st == __mcs_slot_state::idle ) return;
      if ( __st == __mcs_slot_state::busy ) {
        if ( __holder.get(memory_order::acquire) != s )
          micron::exc<except::thread_error>("mcs_lock re-entered while this thread was still acquiring it");
        if ( bo.rounds() >= MICRON_MCS_RECURSION_ROUNDS )
          micron::exc<except::thread_error>("mcs_lock is not recursive: this thread already holds it");
      }
      bo.relax();
    }
  }

  void
  reset() noexcept
  {
    unlock();
  }

public:
  using node_type = mcs_node;

  ~mcs_lock() { __pool::release(this); }

  mcs_lock() noexcept : __id(__next_lock_slot_id()), __q(), __holder(nullptr) { }

  mcs_lock(const mcs_lock &) = delete;
  mcs_lock(mcs_lock &&) = delete;
  mcs_lock &operator=(const mcs_lock &) = delete;

  auto
  operator()()
  {
    bool fresh = false;
    __mcs_slot *s = __pool::claim_evicting(this, __id, fresh, __idle);
    if ( fresh ) s->state.store(__mcs_slot_state::idle, memory_order::relaxed);

    if ( s->state.get(memory_order::acquire) != __mcs_slot_state::idle ) __await_node(s);

    s->state.store(__mcs_slot_state::busy, memory_order::relaxed);
    __q.lock(s->node);
    __holder.store(s, memory_order::release);
    st.note_acquire();
    return &mcs_lock::reset;
  }

  auto
  lock()
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    bool fresh = false;

    __mcs_slot *s = __pool::try_claim(this, __id, fresh, __idle);
    if ( s == nullptr ) return false;
    if ( fresh ) s->state.store(__mcs_slot_state::idle, memory_order::relaxed);

    if ( s->state.get(memory_order::acquire) != __mcs_slot_state::idle ) return false;

    s->state.store(__mcs_slot_state::busy, memory_order::relaxed);
    if ( __q.try_lock(s->node) ) {
      __holder.store(s, memory_order::release);
      st.note_acquire();
      return true;
    }
    s->state.store(__mcs_slot_state::idle, memory_order::relaxed);
    return false;
  }

  void
  unlock() noexcept
  {
    __mcs_slot *s = __holder.get(memory_order::acquire);
    if ( s == nullptr ) return;
    s->state.store(__mcs_slot_state::releasing, memory_order::release);
    __holder.store(nullptr, memory_order::release);
    __q.unlock(s->node);
    s->state.store(__mcs_slot_state::idle, memory_order::release);
  }

  auto
  retrieve() noexcept
  {
    return &mcs_lock::reset;
  }

  bool
  operator!() const noexcept
  {
    return !__q.is_locked();
  }

  bool
  is_locked() const noexcept
  {
    return __q.is_locked();
  }

  [[nodiscard]] bool
  holds() const noexcept
  {
    const __mcs_slot *s = __pool::find(this, __id);
    return s != nullptr and __holder.get(memory_order::acquire) == s;
  }

  // arrivals in queue order; 0 unless MICRON_LOCK_STATS
  [[nodiscard]] u32
  enqueued() const noexcept
  {
    return __q.enqueued();
  }

  [[nodiscard]] const __lock_stats &
  stats() const noexcept
  {
    return st;
  }

  template<typename... T> friend void unlock(T &...);
};

};      // namespace micron
