//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../types.hpp"

namespace micron
{

template<int A, typename B>
  requires(A == 32 || A == 64 || A == 128 || A == 256 || A == 512)
constexpr bool
is_aligned(B *ptr)
{
  return reinterpret_cast<uintptr_t>(ptr) % (A / 8) == 0;
}

template<int L = 3, typename B>
inline void
prefetch(B *ptr)
{
  static_assert(L >= 0 && L <= 3, "prefetch locality must be 0 (NTA) .. 3 (T0)");
  __builtin_prefetch(ptr, 0, L);
}

// NOTE: armv7-a has no PL0 cache-maintenance instruction
template<typename T>
inline void
clflush(T *addr)
{
  const uintptr_t __b = reinterpret_cast<uintptr_t>(addr);
  micron::syscall(SYS_arm_cacheflush, __b, __b + sizeof(T), 0);
}

template<typename T>
inline void
clflush(T &addr)
{
  clflush(micron::addressof(addr));
}

inline void
mfence(void)
{
  asm volatile("dmb ish" ::: "memory");
}

inline void
memory_fence(void)
{
  mfence();
}

inline void
lfence(void)
{
  asm volatile("dmb ish" ::: "memory");
}

inline void
load_fence(void)
{
  lfence();
}

inline void
sfence(void)
{
  asm volatile("dmb ishst" ::: "memory");
}

inline void
store_fence(void)
{
  sfence();
}
};      // namespace micron
