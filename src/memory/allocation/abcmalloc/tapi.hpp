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
#include "../../../mutex/locks.hpp"

// new threading API
// if __default_multithread_safe is off, all multithreading guards are elided at comptime, zero overhead
namespace abc
{

constexpr static const u32 __max_arenas = 64;      //  ~3 MiB BSS upper bound

alignas(__arena) inline byte __arena_pool_storage[__max_arenas * sizeof(__arena)]{};

inline __arena *__arena_pool[__max_arenas]{};

inline micron::atomic_token<u32> __arena_pool_next{ 0 };

inline micron::atomic_flag __arena_pool_init_lock{};

inline thread_local __arena *__tls_arena = nullptr;

// cold init; called only when __tls_arena is nullptr (first hit)
[[gnu::cold, gnu::noinline]] inline __arena *
__claim_arena_slow(void) noexcept
{
  const u32 idx = __arena_pool_next.fetch_add(1, micron::memory_order_acq_rel);

  if ( idx >= __max_arenas ) [[unlikely]] {
    micron::free_guard<> g{ &__arena_pool_init_lock };
    if ( !__arena_pool[0] ) {
      __arena_pool[0] = new (&__arena_pool_storage[0]) __arena();
    }
    __tls_arena = __arena_pool[0];
    return __tls_arena;
  }

  if ( idx == 0 ) [[unlikely]] {
    micron::free_guard<> g{ &__arena_pool_init_lock };
    if ( !__arena_pool[0] ) {
      __arena_pool[0] = new (&__arena_pool_storage[0]) __arena();
    }
    __tls_arena = __arena_pool[0];
    return __tls_arena;
  }

  __arena *a = new (&__arena_pool_storage[idx * sizeof(__arena)]) __arena();
  __arena_pool[idx] = a;
  __tls_arena = a;
  return a;
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
  while ( !owner->__remote_push(p, sz) ) [[unlikely]] {
    for ( int i = 0; i < 64; ++i ) __cpu_pause();
  }
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
}

// NOTE: __boot_abcmalloc was the old entry point, keeping it around in case old start files are still used
// new threading api is fully lazy (created on first alloc)
extern "C" void
__boot_abcmalloc(void) noexcept
{
  // don't inline otherwise the linkers doesn't see
}

};      // namespace abc
