//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../port/ident.hpp"
#include "../../except.hpp"
#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-thread slot table, keyed on the lock's address

namespace micron
{

#ifndef MICRON_MCS_DEPTH
#define MICRON_MCS_DEPTH 8      // distinct queue locks one thread may hold at once
#endif

// how many CPUs the no-TLS slot table shards across. Only read under __micron_no_tls; a shard index
// past the end wraps rather than faulting, so an under-sized value costs sharing, not memory safety.
#ifndef MICRON_MCS_CPUS
#define MICRON_MCS_CPUS 64
#endif

// THE ID IS POINTER-WIDTH, NOT u64, AND THAT IS LOAD-BEARING RATHER THAN TIDY.
//
// atomic_token asserts __atomic_always_lock_free (atomic.hpp:62). On i386 with no x87 and no SSE
// there is no 8-byte atomic LOAD at all, so atomic_token<u64> cannot be instantiated -- and this
// variable is a namespace-scope definition, so it is instantiated the moment mutex/locks.hpp is
// included, which array/conarray.hpp does unconditionally. That put array.hpp, and with it most of
// micron, out of reach of every 32-bit no-FPU build: exactly the (K) and (E) i386 configuration,
// and i386 is the one bare-metal target that boots end to end (start/metal/reset_i386.s carries
// the multiboot header). A lock-slot counter on a 32-bit machine does not need 64 bits.
// recursive_lock.hpp:16 already had this right.

inline atomic_token<usize> __lock_slot_ids{ 1 };

[[nodiscard]] inline usize
__next_lock_slot_id() noexcept
{
  return __lock_slot_ids.fetch_add(1, memory_order::acq_rel);
}

template<typename Slot, usize Depth = MICRON_MCS_DEPTH> class __lock_slot_table
{
  static_assert(Depth > 0, "__lock_slot_table needs at least one slot");
  static constexpr usize __cpu_shards = MICRON_MCS_CPUS;

  struct __entry {
    const void *owner;
    usize id;
    Slot slot;
  };

  struct __table {
    __entry e[Depth]{};
  };

  // THE PER-THREAD SLOT TABLE, AND WHAT IT BECOMES WITH NO TLS RUNTIME.
  //
  // A queue lock needs somewhere to put the node it spins on, and it must be storage no other
  // waiter can touch. `thread_local` is the natural answer and it is what a hosted build gets.
  //
  // Under __micron_no_tls there IS no thread-local storage -- not "it costs something", it does not
  // link. Worse than that on (K): a kernel module has no TLS runtime, so %fs is the PER-CPU base
  // and a @tpoff access reads and writes arbitrary per-CPU kernel memory. It compiles, it loads,
  // and it corrupts. (Measured before this gate existed: a --kernel -DMICRON_NO_TLS object using
  // mcs_lock emitted this very symbol as TLS.)
  //
  // So the no-TLS arm shards by CPU instead, which is the substitution BAREBONES.md already names
  // for abcmalloc's __tls_arena. On (E) port::cpu_id() is 0 and this is exactly a single table,
  // which is correct for a single-core image.
  //
  // THE CONTRACT THAT COMES WITH IT, stated here because it is not free: a per-CPU table is only
  // private to its user while that user cannot migrate. A caller that takes an mcs_lock in ring 0
  // must therefore hold it with preemption disabled -- the ordinary kernel discipline for a queue
  // lock, and the same shape of "this backend cannot give you the stronger thing" that
  // pages_kernel.hpp's page_protect states by returning false.
#if defined(__micron_no_tls)
  static __table &
  __tls() noexcept
  {
    static __table t[__cpu_shards]{};
    const i32 c = micron::port::cpu_id();
    return t[(c < 0 ? 0 : static_cast<usize>(c)) % __cpu_shards];
  }
#else
  static __table &
  __tls() noexcept
  {
    static thread_local __table t{};
    return t;
  }
#endif

  template<typename Pred>
  [[nodiscard]] static Slot *
  __claim_keyed(const void *lock, usize id, bool &fresh, Pred evictable) noexcept
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
  find(const void *lock, usize id) noexcept
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
  claim(const void *lock, usize id, bool &fresh)
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
  claim_evicting(const void *lock, usize id, bool &fresh, Pred evictable)
  {
    Slot *s = __claim_keyed(lock, id, fresh, evictable);
    if ( s == nullptr )
      micron::exc<except::thread_error>("queue-lock slot table exhausted and every entry is held; raise MICRON_MCS_DEPTH");
    return s;
  }

  template<typename Pred>
  [[nodiscard]] static Slot *
  try_claim(const void *lock, usize id, bool &fresh, Pred evictable) noexcept
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
