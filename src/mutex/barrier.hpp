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
