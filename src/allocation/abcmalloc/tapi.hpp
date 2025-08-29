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

// initialize abcmalloc, once per thread
inline __attribute__((always_inline)) void
__init_abcmalloc(void)
{
  if ( !__abcmalloc_init ) [[unlikely]] {
    thread_local static __arena local_arena;
    __main_arena = &local_arena;
    __abcmalloc_init = true;
  }
}

};
