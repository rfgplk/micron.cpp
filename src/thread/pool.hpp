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

// thread pools
namespace micron
{

using standard_arena = arena::__default_arena<micron::thread_stack_size, micron::group_thread<>>;
using concurrent_arena = arena::__concurrent_arena<>;

static standard_arena *__global_threadpool = nullptr;
static concurrent_arena *__global_parallelpool = nullptr;

__attribute__((constructor)) void
__make_threadarena(void)
{
  static standard_arena local_pool;
  __global_threadpool = &local_pool;
};

#if defined(__micron_enable_concurrency_at_startup_var)
__attribute__((constructor)) void
#else
inline __attribute__((always_inline)) void
#endif
__make_parallelarena(void)
{
  static concurrent_arena local_pool;
  __global_parallelpool = &local_pool;
  // init here
  umax_t c = cpu_count();
  for ( umax_t i = 0; i < c; ++i )
    local_pool.create();
};

void
start_concurrent_pools(void)
{
#if !defined(__micron_enable_concurrency_at_startup_var)
  __make_parallelarena();
#endif
}

};     // namespace micron
