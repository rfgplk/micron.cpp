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

#include "../../../bits/__pause.hpp"

#include "../../../mutex/locks.hpp"

// main thread-specific api functions go here
// all external code (non-test) calling abcmalloc code should go through here

namespace abc
{
enum abc_state : u32 { __abc_unloaded = 0, __abc_loading, __abc_ready };

static micron::atomic_token<u32> __global_abc_state{ __abc_unloaded };
static micron::atomic_flag __global_abc_mtx{};
// micron::mutex __global_mutex;
static bool __abcmalloc_init = false;
static __arena *__main_arena = nullptr;

using __abcmalloc_locktype = micron::free_guard<>;

constexpr inline __attribute__((always_inline)) auto
__guard_abcmalloc(void)
{
  if constexpr ( __default_multithread_safe == true ) {
    __abcmalloc_locktype __guard(__global_abc_mtx, micron::adopt_lock);
    return __guard;
  } else if constexpr ( __default_multithread_safe == false ) {
    return 0;
  }
}

static inline __arena *
__start_abcmalloc_init(void)
{
  // micron::free_guard<> guard(__global_abc_mtx, micron::adopt_lock);
  [[maybe_unused]] auto __lock = micron::move(__guard_abcmalloc());
  u32 state = __global_abc_state.get(micron::memory_order_acquire);
  if ( state == __abc_ready ) [[likely]]
    return __main_arena;
  if ( state == __abc_unloaded ) {
    u32 exp = 0;
    if ( __global_abc_state.compare_exchange_strong(exp, __abc_loading, micron::memory_order::acq_rel) ) {
      static __arena local_arena;
      __main_arena = &local_arena;
      __abcmalloc_init = true;
      __global_abc_state.store(__abc_ready, micron::memory_order_release);
      return __main_arena;
    }
  }
  while ( __global_abc_state.get(micron::memory_order_acquire) != __abc_ready )
    __cpu_pause();
  return __main_arena;
}

/*
// initialize abcmalloc, once per thread
inline __attribute__((always_inline)) void
__init_abcmalloc(void)
{
  if constexpr ( __default_single_instance ) {
    if ( !__abcmalloc_init ) [[unlikely]] {
      thread_local static __arena local_arena;
      __main_arena = &local_arena;
      __abcmalloc_init = true;
    }
  }
}

constexpr inline __attribute__((always_inline)) auto
__guard_abcmalloc(void)
{
  if constexpr ( __default_multithread_safe == true ) {
    micron::unique_lock<micron::lock_starts::locked> __lock(__global_mutex);
    return __lock;
  } else if constexpr ( __default_multithread_safe == false ) {
    return 0;
  }
}*/
};     // namespace abc
