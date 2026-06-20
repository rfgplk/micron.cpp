//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "../syscall.hpp"

// per-thread exit hooks
namespace micron
{
inline void (*__thread_exit_hook)() noexcept = nullptr;

// set by the thread kernel to point at its __thread_payload::alive atomic_token<bool>
// WARNING: these are the ONLY per-thread (thread_local) vars the threading core may add
inline thread_local void *__micron_thread_alive_word = nullptr;

// native control word states
inline constexpr u32 __park_run = 0;         // running
inline constexpr u32 __park_parked = 1;      // sleep(): block at the next checkpoint until awaken()
inline constexpr u32 __park_dying = 2;       // terminate()/cancel(): exit at the next checkpoint

// the running thread's control word (== &__thread_payload::park)
inline thread_local u32 *__micron_thread_park = nullptr;
inline thread_local void (*__micron_thread_die)() = nullptr;

inline __attribute__((always_inline)) void
__micron_park_checkpoint() noexcept
{
  u32 *p = __micron_thread_park;
  if ( p == nullptr ) return;
  u32 v = __atomic_load_n(p, __ATOMIC_ACQUIRE);
  while ( v == __park_parked ) {
    // NOTE: deliberately the RAW syscall, not micron::__futex, abcmalloc pulls this in
    micron::syscall(SYS_futex, p, 128 /*FUTEX_WAIT|FUTEX_PRIVATE_FLAG*/, __park_parked, nullptr, nullptr, 0);
    v = __atomic_load_n(p, __ATOMIC_ACQUIRE);
  }
  if ( v == __park_dying && __micron_thread_die ) __micron_thread_die();
}
}      // namespace micron
