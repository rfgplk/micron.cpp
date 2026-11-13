//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "host.hpp"

namespace micron::wasm
{
inline thread_local unsigned __thread_id = 0;
inline thread_local unsigned __stack_begin = 0;
inline thread_local unsigned __stack_bytes = 0;
inline thread_local bool __can_wait = false;
inline unsigned __runtime_state = 0;      // 0 uninitialized, 1 initializing, 2 ready, 3 shutting down, 4 stopped

inline void *
stack_begin() noexcept
{
  return reinterpret_cast<void *>(__stack_begin);
}

inline __SIZE_TYPE__
stack_size() noexcept
{
  return __stack_bytes;
}

inline unsigned
thread_id() noexcept
{
  return __thread_id;
}

inline bool
can_wait() noexcept
{
  return __can_wait;
}

inline int
wait32(unsigned *word, unsigned expected, long long timeout_ns = -1) noexcept
{
  if ( !word || (reinterpret_cast<__UINTPTR_TYPE__>(word) & 3u) ) return -22;
  if ( timeout_ns < -1 ) return -22;
  if ( __atomic_load_n(word, __ATOMIC_ACQUIRE) != expected ) return -11;
#if defined(__micron_wasm_threads)
  if ( !__can_wait ) return -95;
  const int r = __builtin_wasm_memory_atomic_wait32(reinterpret_cast<int *>(word), static_cast<int>(expected), timeout_ns);
  return r == 0 ? 0 : (r == 1 ? -11 : -110);
#else
  (void)timeout_ns;
  return -95;
#endif
}

inline int
wake32(unsigned *word, unsigned count = 0x7fffffffu) noexcept
{
  if ( !word || (reinterpret_cast<__UINTPTR_TYPE__>(word) & 3u) ) return -22;
#if defined(__micron_wasm_threads)
  return __builtin_wasm_memory_atomic_notify(reinterpret_cast<int *>(word), count);
#else
  (void)count;
  return 0;
#endif
}

struct mutex {
  unsigned word = 0;

  void
  lock() noexcept
  {
#if defined(__micron_wasm_threads)
    unsigned expected = 0;
    if ( __atomic_compare_exchange_n(&word, &expected, 1u, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ) return;
    if ( !__can_wait ) {
      write_diag("micron: blocking mutex requires a worker\n");
      panic();
    }
    while ( __atomic_exchange_n(&word, 2u, __ATOMIC_ACQUIRE) ) (void)wait32(&word, 2u);
#else
    if ( word ) {
      write_diag("micron: recursive runtime mutex\n");
      panic();
    }
    word = 1;
#endif
  }

  void
  unlock() noexcept
  {
#if defined(__micron_wasm_threads)
    if ( __atomic_exchange_n(&word, 0u, __ATOMIC_RELEASE) == 2u ) (void)wake32(&word, 1);
#else
    word = 0;
#endif
  }
};
}      // namespace micron::wasm
