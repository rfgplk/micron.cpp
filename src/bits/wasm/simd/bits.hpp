// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"

namespace micron
{
template<int A, typename B>
  requires(A == 32 || A == 64 || A == 128 || A == 256 || A == 512)
constexpr bool
is_aligned(B *p)
{
  return (reinterpret_cast<uintptr_t>(p) & (A / 8 - 1)) == 0;
}

template<int L = 3, typename B>
inline void
prefetch(B *)
{
  static_assert(L >= 0 && L <= 3, "prefetch locality must be 0..3");
}

inline void
memory_fence() noexcept
{
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

inline void
load_fence() noexcept
{
  __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

inline void
store_fence() noexcept
{
  __atomic_thread_fence(__ATOMIC_RELEASE);
}

inline void
mfence() noexcept
{
  memory_fence();
}

inline void
lfence() noexcept
{
  load_fence();
}

inline void
sfence() noexcept
{
  store_fence();
}
};      // namespace micron
