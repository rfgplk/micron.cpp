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

#include "../../mutex/locks.hpp"
#include "../../mutex/mutex.hpp"

// main thread-specific api functions go here
// all external code (non-test) calling abcmalloc code should go through here

namespace abc
{
micron::mutex __global_mutex;
static bool __abcmalloc_init = false;
static __arena *__main_arena = nullptr;

// global construction
// NOTE: attr constructor will fire after zero-initialization of all static variables, but BEFORE dynamic init. of all
// objects
// second note, the 101 is of utmost importance, since there are a few other constructor attr. declared functions within
// micron (notably io & threading). with default prior the functions which were declared first will fire first, leaving
// the global __main_arena uninit and segfaulting
inline __attribute__((constructor(101))) void
__global_abcmalloc_start(void)
{
  if constexpr ( __default_global_instance and __default_construct_on_start ) {
    static __arena local_arena;
    __main_arena = &local_arena;
    __abcmalloc_init = true;
  }
}

inline __attribute__((destructor(65535))) void
__global_abcmalloc_finish(void)
{
  if constexpr ( __default_global_instance ) {
    __abcmalloc_init = false;
    __main_arena = nullptr;
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

constexpr inline __attribute__((always_inline)) auto
__guard_abcmalloc(void)
{
  if constexpr ( __default_multithread_safe == true ) {
    micron::unique_lock<micron::lock_starts::locked> __lock(__global_mutex);
    return __lock;
  } else if constexpr ( __default_multithread_safe == false ) {
    return 0;
  }
}
};
