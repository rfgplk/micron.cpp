//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#define full_barrier() __asm("" ::: "memory");

#define read_barrier() __asm("" ::: "memory");

#define write_barrier() __asm("" ::: "memory");

#define forced_read_barrier(x)                                                                                          \
  ({                                                                                                                    \
    \ __typeof(x) __x;                                                                                                  \
    \ __asm("" : "=r"(__x) : "0"(x));                                                                                   \
    \ __x;                                                                                                              \
    \                                                                                                                   \
  });
