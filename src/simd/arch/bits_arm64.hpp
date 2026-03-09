//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../types.hpp"

namespace micron
{

template <int A, typename B>
  requires(A == 32 or A == 64 or A == 128 or A == 256 or A == 512)
constexpr bool
is_aligned(B *ptr)
{
  return reinterpret_cast<uintptr_t>(ptr) % (A / 8) == 0;
}

template <typename B>
inline void
prefetch(B *ptr, int = 0)
{
  __builtin_prefetch(ptr, 0, 3);
}

template <typename T>
inline void
clflush(T *addr)
{
  asm volatile("dc civac, %0" ::"r"(addr) : "memory");
}

template <typename T>
inline void
clflush(T &addr)
{
  asm volatile("dc civac, %0" ::"r"(&addr) : "memory");
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
  asm volatile("dmb ishld" ::: "memory");
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
