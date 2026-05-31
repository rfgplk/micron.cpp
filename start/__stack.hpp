//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <micron/bits/__arch.hpp>
#include <micron/memory/stack.hpp>
#include <micron/syscall.hpp>
#include <micron/types.hpp>

#include "__auxv.hpp"

namespace micron
{

// get the main thread stack region
inline void
__stack_init([[maybe_unused]] const auxv_t *auxv) noexcept
{
#if defined(__micron_freestanding)
  // NOTE: RLIMIT_STACK is resource #3 on every Linux ABI
  // prlimit64 always uses 64-bit rlimit fields regardless of the arch word size
  constexpr int __rlimit_stack = 3;
  constexpr u64 __rlim64_infinity = ~static_cast<u64>(0);

  unsigned long page_sz = __auxv_lookup(auxv, at_pagesz);
  if ( page_sz == 0 ) page_sz = 4096;
  const uintptr_t pg = static_cast<uintptr_t>(page_sz);

  uintptr_t top = static_cast<uintptr_t>(__auxv_lookup(auxv, at_execfn));
  if ( top == 0 ) top = static_cast<uintptr_t>(__auxv_lookup(auxv, at_random));
  if ( top == 0 ) top = reinterpret_cast<uintptr_t>(auxv);
  const uintptr_t hi = (top + (pg - 1)) & ~(pg - 1);

  struct __rl64 {
    u64 cur;
    u64 max;
  } rl{ 0, 0 };

  usize size = static_cast<usize>(default_stack_size);
  const long pr = micron::syscall(SYS_prlimit64, static_cast<pid_t>(0), __rlimit_stack, nullptr, &rl);
  if ( !micron::syscall_failed(pr) ) {
    const u64 cur = rl.cur;
    if ( cur != __rlim64_infinity && cur >= (64ull * 1024ull) && cur <= (1024ull * 1024ull * 1024ull) ) size = static_cast<usize>(cur);
  }
  size = (size + (static_cast<usize>(pg) - 1)) & ~(static_cast<usize>(pg) - 1);
  const uintptr_t lo = hi - static_cast<uintptr_t>(size);

  __micron_main_stack.start = reinterpret_cast<addr_t *>(lo);
  __micron_main_stack.size = size;
#endif
}

};      // namespace micron
