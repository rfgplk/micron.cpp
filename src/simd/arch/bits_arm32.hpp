//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../port/__backend.hpp"
// under the same gate as its only call site (:43); see the note in chrono/tz.hpp
#if defined(__micron_port_linux)
#include "../../port/backends/__syscall.hpp"
#endif
#include "../../memory/addr.hpp"      // micron::addressof
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

// NOTE: armv7-a has no PL0 cache-maintenance instruction -- USERSPACE has to ask the kernel.
// Phase 4: that is a property of the privilege level, not of the architecture. A kernel module and
// a bare-metal image both run at PL1 and can issue the maintenance ops directly, so the syscall is
// gated on the linux backend and everything else goes through the compiler builtin, which lowers to
// the right instruction sequence for the target.
template<typename T>
inline void
clflush(T *addr)
{
  const uintptr_t __b = reinterpret_cast<uintptr_t>(addr);
#if defined(__micron_port_linux)
  micron::syscall(SYS_arm_cacheflush, __b, __b + sizeof(T), 0);
#else
  __builtin___clear_cache(reinterpret_cast<char *>(__b), reinterpret_cast<char *>(__b + sizeof(T)));
#endif
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
