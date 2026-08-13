//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/intrin.hpp"

// NOTE: these emit a real fence

#define full_barrier() ::micron::atom::thread_fence(::micron::atomic_seq_cst)

#define read_barrier() ::micron::atom::thread_fence(::micron::atomic_acquire)

#define write_barrier() ::micron::atom::thread_fence(::micron::atomic_release)

// compiler-only reorder barrier: emits no instruction
#define compiler_barrier() __asm("" ::: "memory")

// launders a value through a register so the compiler cannot re-fold it
#define forced_read_barrier(x)                                                                                                             \
  ({                                                                                                                                       \
    __typeof__(x) __x = (x);                                                                                                               \
    __asm("" : "+r"(__x));                                                                                                                 \
    __x;                                                                                                                                   \
  })
