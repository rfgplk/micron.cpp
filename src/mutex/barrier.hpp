//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// NOTE: these are COMPILER reorder barriers only
// (no hardware fence is emitted on a weak memory model such as ARM)

#define full_barrier() __asm("" ::: "memory")

#define read_barrier() __asm("" ::: "memory")

#define write_barrier() __asm("" ::: "memory")

#define forced_read_barrier(x)                                                                                                             \
  ({                                                                                                                                       \
    __typeof__(x) __x = (x);                                                                                                               \
    __asm("" : "+r"(__x));                                                                                                                 \
    __x;                                                                                                                                   \
  })
