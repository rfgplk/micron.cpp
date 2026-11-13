//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__thread_exit_hook.hpp"
#include "memory.hpp"
#include "runtime.hpp"

#if defined(__micron_eh)
extern "C" void __micron_wasm_finish_exceptions() noexcept;
#endif

#ifndef MICRON_WASM_MAX_THREADS
#define MICRON_WASM_MAX_THREADS 16
#endif
#ifndef MICRON_WASM_STACK_SIZE
#define MICRON_WASM_STACK_SIZE 1048576
#endif

namespace micron::wasm
{
using thread_entry = int (*)(void *);

struct thread_handle {
  unsigned slot = 0, generation = 0;
};

struct thread_slot {
  unsigned state = 0;      // 0 free, 1 starting, 2 running, 3 complete, 4 failed
  unsigned generation = 0;
  unsigned cancelled = 0, park = 0;
  unsigned stack = 0, stack_bytes = 0, tls = 0;
  thread_entry entry = nullptr;
  void *arg = nullptr;
  int result = 0;
};

inline thread_slot __threads[MICRON_WASM_MAX_THREADS]{};
inline mutex __threads_lock{};
inline unsigned __live_threads = 0;
inline thread_local bool __thread_finished = false;
inline constexpr long long __join_poll_ns = 1000000;
static_assert(MICRON_WASM_MAX_THREADS >= 2 && MICRON_WASM_MAX_THREADS <= 1024);
static_assert(MICRON_WASM_STACK_SIZE >= 65536 && MICRON_WASM_STACK_SIZE % 16 == 0);

inline bool
cancellation_requested() noexcept
{
  return __thread_id > 1 && __atomic_load_n(&__threads[__thread_id - 1].cancelled, __ATOMIC_ACQUIRE) != 0;
}

[[noreturn]] inline void
__exit_worker() noexcept
{
  __micron_wasm_host_thread_exit();
}

inline int
spawn(thread_entry entry, void *arg, thread_handle &handle) noexcept
{
#if defined(__micron_wasm_threads)
  if ( !entry || !can_wait() ) return -95;
  const unsigned runtime = __atomic_load_n(&__runtime_state, __ATOMIC_ACQUIRE);
  if ( runtime != 1 && runtime != 2 ) return -16;
  __threads_lock.lock();
  unsigned slot = 1;
  const unsigned locked_runtime = __atomic_load_n(&__runtime_state, __ATOMIC_ACQUIRE);
  if ( locked_runtime != 1 && locked_runtime != 2 ) {
    __threads_lock.unlock();
    return -16;
  }
  while ( slot < MICRON_WASM_MAX_THREADS && __atomic_load_n(&__threads[slot].state, __ATOMIC_ACQUIRE) ) ++slot;
  if ( slot == MICRON_WASM_MAX_THREADS ) {
    __threads_lock.unlock();
    return -11;
  }
  auto &t = __threads[slot];
  if ( !t.stack ) {
    t.stack = reinterpret_cast<unsigned>(linear_reserve(MICRON_WASM_STACK_SIZE, 16));
    if ( !t.stack ) {
      __threads_lock.unlock();
      return -12;
    }
    t.stack_bytes = MICRON_WASM_STACK_SIZE;
  }
  if ( !t.tls ) {
    t.tls = reinterpret_cast<unsigned>(linear_reserve(__builtin_wasm_tls_size(), __builtin_wasm_tls_align()));
    if ( !t.tls ) {
      __threads_lock.unlock();
      return -12;
    }
  }
  const unsigned generation = __atomic_add_fetch(&t.generation, 1u, __ATOMIC_RELAXED);
  t.entry = entry;
  t.arg = arg;
  t.result = 0;
  t.cancelled = 0;
  t.park = 0;
  __atomic_store_n(&t.state, 1u, __ATOMIC_RELEASE);
  __atomic_fetch_add(&__live_threads, 1u, __ATOMIC_ACQ_REL);
  handle = { slot, generation };
  __threads_lock.unlock();
  const int r = __micron_wasm_host_worker_spawn(slot, t.stack + t.stack_bytes, t.tls, reinterpret_cast<unsigned>(&t.state));
  if ( r < 0 ) {
    __threads_lock.lock();
    __atomic_store_n(&t.state, 0u, __ATOMIC_RELEASE);
    __atomic_fetch_sub(&__live_threads, 1u, __ATOMIC_ACQ_REL);
    __threads_lock.unlock();
    handle = {};
    return r;
  }
  return 0;
#else
  (void)entry;
  (void)arg;
  (void)handle;
  return -95;
#endif
}

inline int
request_cancel(thread_handle handle) noexcept
{
  if ( !handle.slot || handle.slot >= MICRON_WASM_MAX_THREADS ) return -22;
  __threads_lock.lock();
  auto &t = __threads[handle.slot];
  if ( __atomic_load_n(&t.generation, __ATOMIC_RELAXED) != handle.generation || !__atomic_load_n(&t.state, __ATOMIC_ACQUIRE) ) {
    __threads_lock.unlock();
    return -22;
  }
  __atomic_store_n(&t.cancelled, 1u, __ATOMIC_RELEASE);
  __atomic_store_n(&t.park, micron::__park_dying, __ATOMIC_RELEASE);
  __threads_lock.unlock();
  (void)wake32(&t.park);
  return 0;
}

inline int
__set_park(thread_handle handle, bool parked) noexcept
{
  if ( !handle.slot || handle.slot >= MICRON_WASM_MAX_THREADS ) return -22;
  __threads_lock.lock();
  auto &t = __threads[handle.slot];
  const unsigned state = __atomic_load_n(&t.state, __ATOMIC_ACQUIRE);
  if ( __atomic_load_n(&t.generation, __ATOMIC_RELAXED) != handle.generation || (state != 1 && state != 2) ) {
    __threads_lock.unlock();
    return -22;
  }
  if ( __atomic_load_n(&t.cancelled, __ATOMIC_ACQUIRE) ) {
    __threads_lock.unlock();
    return -125;
  }
  __atomic_store_n(&t.park, parked ? micron::__park_parked : micron::__park_run, __ATOMIC_RELEASE);
  __threads_lock.unlock();
  if ( !parked ) (void)wake32(&t.park);
  return 0;
}

template<void (*BeforeState)(thread_handle) = nullptr>
inline int
__join(thread_handle &handle, int *result = nullptr) noexcept
{
  if ( !handle.slot || handle.slot >= MICRON_WASM_MAX_THREADS ) return -22;
  if ( handle.slot + 1 == __thread_id ) return -35;
  auto &t = __threads[handle.slot];
  unsigned state;
  for ( ;; ) {
    if ( __atomic_load_n(&t.generation, __ATOMIC_ACQUIRE) != handle.generation ) return -22;
    if constexpr ( BeforeState != nullptr ) BeforeState(handle);
    state = __atomic_load_n(&t.state, __ATOMIC_ACQUIRE);
    if ( state != 1 && state != 2 ) break;
    // A copied handle can be reaped and its slot reused between the two loads.
    // Bound the state-only wait so it cannot wait indefinitely on that successor.
    const int r = wait32(&t.state, state, __join_poll_ns);
    if ( r < 0 && r != -11 && r != -110 ) return r;
  }
  __threads_lock.lock();
  state = __atomic_load_n(&t.state, __ATOMIC_ACQUIRE);
  if ( __atomic_load_n(&t.generation, __ATOMIC_RELAXED) != handle.generation || (state != 3 && state != 4) ) {
    __threads_lock.unlock();
    return -22;
  }
  if ( result ) *result = t.result;
  const int status = state == 4 ? -5 : 0;
  __atomic_store_n(&t.state, 0u, __ATOMIC_RELEASE);
  __atomic_fetch_sub(&__live_threads, 1u, __ATOMIC_ACQ_REL);
  __threads_lock.unlock();
  handle = {};
  return status;
}

inline int
join(thread_handle &handle, int *result = nullptr) noexcept
{
  return __join(handle, result);
}

inline void
finish_thread(bool cancelled = false) noexcept
{
  if ( __thread_finished ) return;
  __thread_finished = true;
  micron::__micron_thread_park = nullptr;
  micron::__micron_thread_die = nullptr;
#if defined(__micron_eh)
  if ( cancelled ) __micron_wasm_finish_exceptions();
#else
  (void)cancelled;
#endif
  __run_thread_dtors();
  if ( __thread_exit_hook ) __thread_exit_hook();
  if ( __dynamic_tls_exit_hook ) __dynamic_tls_exit_hook();
}
}      // namespace micron::wasm
