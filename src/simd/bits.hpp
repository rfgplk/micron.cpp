
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "types.hpp"

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
void
prefetch(B *ptr, int s = _MM_HINT_T0)
{
  _mm_prefetch(ptr, s);
}

template <typename T>
inline void
clflush(T *addr)
{
  _mm_clflush(addr);
}

template <typename T>
inline void
clflush(T &addr)
{
  _mm_clflush(&addr);
}

inline void
mfence(void)
{
  _mm_mfence();
}

inline void
memory_fence(void)
{
  mfence();
}

inline void
lfence(void)
{
  _mm_lfence();
}

inline void
load_fence(void)
{
  lfence();
}

inline void
sfence(void)
{
  _mm_sfence();
}

inline void
store_fence(void)
{
  sfence();
}

};     // namespace micron
