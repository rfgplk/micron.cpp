//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#if !defined(__micron_arch_wasm32)
#error "micron: linear memory requires the wasm32 backend"
#endif

extern "C" unsigned char __heap_base;

namespace micron::wasm
{

// A reservation is permanent. Heap regions, worker stacks and TLS all use this
// cursor; releasing an allocation never returns a linear-memory reservation.
inline constinit u64 __linear_cursor = 0;
inline constinit u32 __linear_growth_epoch = 0;

struct __mutex {
  u32 word{ 0 };

  void
  lock() noexcept
  {
#if defined(__micron_wasm_threads)
    for ( ;; ) {
      u32 expected = 0;
      if ( __atomic_compare_exchange_n(&word, &expected, 1u, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ) return;
      while ( __atomic_load_n(&word, __ATOMIC_RELAXED) != 0 ) {
      }
    }
#endif
  }

  void
  unlock() noexcept
  {
#if defined(__micron_wasm_threads)
    __atomic_store_n(&word, 0u, __ATOMIC_RELEASE);
#endif
  }
};

struct __lock_guard {
  __mutex &mutex;

  explicit __lock_guard(__mutex &m) noexcept : mutex(m) { mutex.lock(); }

  ~__lock_guard() { mutex.unlock(); }

  __lock_guard(const __lock_guard &) = delete;
  __lock_guard &operator=(const __lock_guard &) = delete;
};

inline constinit __mutex __linear_mutex{};

[[nodiscard]] inline u64
linear_memory_bytes() noexcept
{
  return static_cast<u64>(__builtin_wasm_memory_size(0)) * __micron_wasm_page_size;
}

inline void
linear_memory_init() noexcept
{
  __lock_guard lock{ __linear_mutex };
  if ( __linear_cursor == 0 ) __linear_cursor = reinterpret_cast<uintptr_t>(&__heap_base);
}

[[nodiscard]] inline void *
linear_reserve(usize bytes, usize alignment = 16) noexcept
{
  if ( bytes == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0 ) return nullptr;
  __lock_guard lock{ __linear_mutex };
  const u64 cursor = __linear_cursor ? __linear_cursor : reinterpret_cast<uintptr_t>(&__heap_base);
  const u64 begin = (cursor + alignment - 1) & ~(static_cast<u64>(alignment) - 1);
  const u64 end = begin + bytes;
  // Keep the one-past pointer representable in the wasm32 address space.
  if ( begin == 0 || end > 0xffffffffull ) return nullptr;
  const u64 have = linear_memory_bytes();
  if ( end > have ) {
    const usize pages = static_cast<usize>((end - have + __micron_wasm_page_size - 1) / __micron_wasm_page_size);
    if ( __builtin_wasm_memory_grow(0, pages) == static_cast<usize>(-1) ) return nullptr;
#if defined(__micron_wasm_threads)
    __atomic_add_fetch(&__linear_growth_epoch, 1u, __ATOMIC_RELEASE);
#else
    ++__linear_growth_epoch;
#endif
  }
  __linear_cursor = end;
  return reinterpret_cast<void *>(static_cast<uintptr_t>(begin));
}

[[nodiscard]] inline u32
memory_growth_epoch() noexcept
{
#if defined(__micron_wasm_threads)
  return __atomic_load_n(&__linear_growth_epoch, __ATOMIC_ACQUIRE);
#else
  return __linear_growth_epoch;
#endif
}

};      // namespace micron::wasm
