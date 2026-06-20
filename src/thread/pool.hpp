//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

#include "arena.hpp"
#include "thread.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// thread pools
//
// available as freestanding (via micthread)

namespace micron
{

using standard_arena = arena::__default_arena<micron::thread_stack_size, micron::group_thread<>>;
using concurrent_arena = arena::__concurrent_arena<>;

inline standard_arena *__global_threadpool = nullptr;
inline concurrent_arena *__global_parallelpool = nullptr;

__attribute__((constructor)) void
__make_threadarena(void)
{
  // WARNING: intentionally never destroyed, prevents hangs in glibcs/freestanding via registering atexits
  alignas(standard_arena) static unsigned char __pool_storage[sizeof(standard_arena)];
  static standard_arena *local_pool = new (static_cast<void *>(__pool_storage)) standard_arena();
  __global_threadpool = local_pool;
};

__attribute__((destructor)) void __destroy_threadarena(void) {
  // WARNING: do NOT null the pointer here; a still-running worker may dereference __global_threadpool during process
  // teardown; nulling it concurrently is a use-after-free / null-deref race
};

#if defined(__micron_enable_concurrency_at_startup_var)
__attribute__((constructor)) void
#else
inline __attribute__((always_inline)) void
#endif
__make_parallelarena(void)
{
  // WARNING: intentionally never destroyed, prevents hangs in glibcs/freestanding
  alignas(concurrent_arena) static unsigned char __ppool_storage[sizeof(concurrent_arena)];
  static concurrent_arena *local_pool = new (static_cast<void *>(__ppool_storage)) concurrent_arena();
  static bool __inited = false;
  __global_parallelpool = local_pool;
  if ( __inited ) return;      // create() the workers exactly once, even if start_concurrent_pools() is called again
  __inited = true;
  // init here
  umax_t c = cpu_count();
  if ( c > concurrent_threads ) c = concurrent_threads;      // never create more workers than the fixed arena has slots
  for ( umax_t i = 0; i < c; ++i ) local_pool->create();
};

#if defined(__micron_enable_concurrency_at_startup_var)
__attribute__((destructor)) void
#else
inline __attribute__((always_inline)) void
#endif
__destroy_parallelarena(void) {
  // see __destroy_threadarena: do not null while workers may still read it (race)
};

void
start_concurrent_pools(void)
{
#if !defined(__micron_enable_concurrency_at_startup_var)
  __make_parallelarena();
#endif
}

void
end_concurrent_pools(void)
{
  __destroy_parallelarena();
}

};      // namespace micron
