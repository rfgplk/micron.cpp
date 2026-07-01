// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

#include "arena.hpp"

#include "../../../atomic/atomic.hpp"
#include "../../../atomic/flag.hpp"
#include "../../../bits/__pause.hpp"
#include "../../../bits/__thread_exit_hook.hpp"
#include "../../../mutex/locks.hpp"

// new threading API
// if __default_multithread_safe is off, all multithreading guards are elided at comptime, zero overhead
namespace abc
{

constexpr static const u32 __max_arenas = 64;      //  ~3 MiB BSS upper bound

alignas(__arena) inline byte __arena_pool_storage[__max_arenas * sizeof(__arena)]{};

inline __arena *__arena_pool[__max_arenas]{};

inline micron::atomic_token<u32> __arena_pool_next{ 0 };

// per-slot owner kernel-tid
inline micron::atomic_token<i32> __arena_owner[__max_arenas]{};

inline micron::atomic_flag __arena_pool_init_lock{};

inline thread_local __arena *__tls_arena = nullptr;

constexpr static const i32 __arena_slot_free = -1;

// grow-only, lock-free, single-owner stack used when all __max_arenas primary slots are concurrently allocated
struct __arena_node {
  __arena arena;
  micron::atomic_token<i32> owner{ 0 };
  __arena_node *next = nullptr;
};

inline micron::atomic_token<__arena_node *> __overflow_head{ nullptr };

// kernel tid of the calling thread.
[[gnu::always_inline]] static inline i32
__this_tid(void) noexcept
{
  return static_cast<i32>(micron::syscall(SYS_gettid));
}

// safety net for a thread that died WITHOUT running the exit hook
[[gnu::cold]] static inline bool
__owner_alive(i32 tid) noexcept
{
  if ( tid == 0 ) return false;
  const i32 pid = static_cast<i32>(micron::syscall(SYS_getpid));
  return micron::syscall(SYS_tgkill, pid, tid, 0) == 0;
}

// runs on the exiting thread (via thread_kernel) and returns its arena slot to the pool
static void
__release_tls_arena(void) noexcept
{
  __arena *a = __tls_arena;
  if ( !a ) return;
  const i32 tid = __this_tid();
  for ( u32 i = 0; i < __max_arenas; ++i ) {
    if ( __arena_pool[i] == a ) {
      if ( __arena_owner[i].get(micron::memory_order_acquire) == tid ) {
        a->__maybe_drain();      // flush pending cross-thread frees while we still own it
        __arena_owner[i].store(__arena_slot_free, micron::memory_order_release);      // recyclable (ABC-10)
      }
      __tls_arena = nullptr;
      return;
    }
  }
  // overflow arena: same release protocol on the node's own owner field
  for ( __arena_node *nd = __overflow_head.get(micron::memory_order_acquire); nd != nullptr; nd = nd->next ) {
    if ( &nd->arena == a ) {
      if ( nd->owner.get(micron::memory_order_acquire) == tid ) {
        a->__maybe_drain();
        nd->owner.store(__arena_slot_free, micron::memory_order_release);
      }
      break;
    }
  }
  __tls_arena = nullptr;
}

// must release the slot on EVERY normal thread exit (pthread / std::thread / main / foreign), not just our micron::thread
// thread_locals with a non-trivial dtor registers via __cxa_thread_atexit and runs at thread destruction
struct __arena_slot_releaser {
  inline ~__arena_slot_releaser() noexcept { __release_tls_arena(); }
};

inline thread_local __arena_slot_releaser __arena_releaser_tls{};

// cold init; called only when __tls_arena is nullptr (first hit on this thread)
[[gnu::cold, gnu::noinline]] inline __arena *
__claim_arena_slow(void) noexcept
{
  micron::__thread_exit_hook = &__release_tls_arena;      // micron::thread exit path
  (void)&__arena_releaser_tls;                            // force-instantiate the TLS-dtor releaser

  const i32 tid = __this_tid();

  {
    const u32 n = __arena_pool_next.get(micron::memory_order_acquire);
    const u32 lim = n > __max_arenas ? __max_arenas : n;
    for ( u32 i = 0; i < lim; ++i ) {
      i32 expect = __arena_slot_free;
      if ( __arena_owner[i].compare_exchange_strong(expect, tid, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
        __arena *a = __arena_pool[i];
        a->__maybe_drain();
        __tls_arena = a;
        return a;
      }
    }
  }

  // WARNING: reclaim a slot whose owner thread has died without releasing it;
  // a joined threads slot can lag the next threads reclaim, under fast churn stale slots can accumulate rapidly
  {
    const u32 n = __arena_pool_next.get(micron::memory_order_acquire);
    const u32 lim = n > __max_arenas ? __max_arenas : n;
    for ( u32 i = 0; i < lim; ++i ) {
      __arena *a = __arena_pool[i];
      if ( !a ) continue;
      i32 owner = __arena_owner[i].get(micron::memory_order_acquire);
      if ( owner <= 0 || owner == tid ) continue;
      if ( __owner_alive(owner) ) continue;
      if ( __arena_owner[i].compare_exchange_strong(owner, tid, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
        a->__maybe_drain();
        __tls_arena = a;
        return a;
      }
    }
  }

  u32 cur = __arena_pool_next.get(micron::memory_order_acquire);
  while ( cur < __max_arenas ) {
    if ( __arena_pool_next.compare_exchange_strong(cur, cur + 1, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
      __arena *a = new (&__arena_pool_storage[cur * sizeof(__arena)]) __arena();
      __arena_pool[cur] = a;
      __arena_owner[cur].store(tid, micron::memory_order_release);
      __tls_arena = a;
      return a;
    }
  }

  for ( __arena_node *nd = __overflow_head.get(micron::memory_order_acquire); nd != nullptr; nd = nd->next ) {
    i32 owner = nd->owner.get(micron::memory_order_acquire);
    const bool reclaimable = (owner == __arena_slot_free) || (owner > 0 && owner != tid && !__owner_alive(owner));
    if ( !reclaimable ) continue;
    i32 expect = owner;      // free or a dead owner; claim it
    if ( nd->owner.compare_exchange_strong(expect, tid, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
      nd->arena.__maybe_drain();
      __tls_arena = &nd->arena;
      return &nd->arena;
    }
  }

  byte *mem = micron::sys_allocator<byte>::alloc(sizeof(__arena_node));
  __arena_node *node = new (mem) __arena_node();
  node->owner.store(tid, micron::memory_order_release);
  node->next = __overflow_head.get(micron::memory_order_acquire);
  while ( !__overflow_head.compare_exchange_weak(node->next, node, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
  }
  __tls_arena = &node->arena;
  return &node->arena;
}

// hot path init; taken when arena already live
[[gnu::always_inline]] static inline __arena *
__current_arena(void) noexcept
{
  __arena *a = __tls_arena;
  if ( a ) [[likely]] {
    a->__maybe_drain();
    return a;
  }
  return __claim_arena_slow();
}

[[gnu::always_inline]] static inline bool
__route_dealloc(byte *p, usize sz) noexcept
{
  __arena *me = __current_arena();
  if constexpr ( !__default_multithread_safe ) {
    // single-thread
    return sz ? me->pop(micron::__chunk<byte>{ p, sz }) : me->pop(p);
  }
  __arena *owner = __owner_of(p);
  if ( !owner || owner == me ) [[likely]] {
    return sz ? me->pop(micron::__chunk<byte>{ p, sz }) : me->pop(p);
  }
  // wait-free: the owner's ring, falling back to the embedded-node overflow LIFO when the ring is full; never spin here
  ABC_DOCTOR(doctor::record_remote_free(p, sz);)
  (void)owner->__remote_push(p, sz);
  return true;
}

[[gnu::always_inline]] static inline __arena *
__query_arena(const void *p) noexcept
{
  if constexpr ( !__default_multithread_safe ) {
    (void)p;
    return __current_arena();
  } else {
    __arena *owner = __owner_of(p);
    return owner ? owner : __current_arena();
  }
}

template<typename Fn>
[[gnu::always_inline]] static inline void
__for_each_live_arena(Fn &&fn) noexcept
{
  if constexpr ( !__default_multithread_safe ) {
    if ( auto *a = __tls_arena; a ) fn(*a);
    return;
  }
  const u32 n = __arena_pool_next.get(micron::memory_order_acquire);
  const u32 lim = n > __max_arenas ? __max_arenas : n;
  for ( u32 i = 0; i < lim; ++i ) {
    if ( auto *a = __arena_pool[i]; a ) fn(*a);
  }
  for ( __arena_node *nd = __overflow_head.get(micron::memory_order_acquire); nd != nullptr; nd = nd->next ) fn(nd->arena);
}

// NOTE: __boot_abcmalloc was the old entry point, keeping it around in case old start files are still used
// new threading api is fully lazy (created on first alloc)
extern "C" void
__boot_abcmalloc(void) noexcept
{
  // don't inline otherwise the linkers doesn't see
}

};      // namespace abc
