//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "arena.hpp"

// main thread-specific api functions go here
// all external code (non-test) calling abcmalloc code should go through here

namespace abc
{
thread_local static bool __abcmalloc_init = false;
thread_local static __arena *__main_arena = nullptr;

// global construction
// NOTE: attr constructor will fire after zero-initialization of all static variables, but BEFORE dynamic init. of all
// objects
// second note, the 101 is of utmost importance, since there are a few other constructor attr. declared functions within
// micron (notably io & threading). with default prior the functions which were declared first will fire first, leaving
// the global __main_arena uninit and segfaulting
inline __attribute__((constructor(101))) void
__global_abcmalloc_start(void)
{
  if constexpr ( __default_global_instance ) {
    thread_local static __arena local_arena;
    __main_arena = &local_arena;
    __abcmalloc_init = true;
  }
}

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

};
