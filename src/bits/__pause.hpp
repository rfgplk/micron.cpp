//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

inline __attribute__((always_inline)) void
__cpu_pause(void)
{
#if defined(__micron_arch_x86_any)
  asm volatile("pause" ::: "memory");
  __builtin_ia32_pause();
#elif defined(__micron_arch_arm_any)
  __asm__ __volatile__("yield" ::: "memory");
  //__builtin_arm_yield();
#endif
}
