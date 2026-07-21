//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"
#include "__attach_hook.hpp"

#include "../syscall.hpp"
#include "../types.hpp"

// per-thread exit hooks
namespace micron
{
inline void (*__thread_exit_hook)() noexcept = nullptr;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-thread C++ dtor list (attach only)
//
// guest modules don't have _start, so its thread_local objects with non-trivial
// dtors route their __cxa_thread_atexit registrations here
#if defined(MICRON_ATTACH_MODULE)
inline bool
__push_thread_dtor(void (*dtor)(void *), void *obj) noexcept
{
  if ( __micron_attach_thread_atexit == nullptr ) return false;
  return __micron_attach_thread_atexit(dtor, obj) == 0;
}

inline void
__run_thread_dtors() noexcept
{
  if ( __micron_attach_run_thread_dtors ) __micron_attach_run_thread_dtors();
}
#elif defined(__micron_attach_capable)
struct __tdtor_node {
  void (*dtor)(void *);
  void *obj;
};

inline constexpr u32 __tdtor_cap = 128;
inline thread_local __tdtor_node __tdtors[__tdtor_cap];
inline thread_local u32 __tdtor_n = 0;

inline bool
__push_thread_dtor(void (*dtor)(void *), void *obj) noexcept
{
  if ( __tdtor_n >= __tdtor_cap ) return false;
  __tdtors[__tdtor_n].dtor = dtor;
  __tdtors[__tdtor_n].obj = obj;
  ++__tdtor_n;
  return true;
}

inline void
__run_thread_dtors() noexcept
{
  while ( __tdtor_n ) {
    --__tdtor_n;
    __tdtor_node e = __tdtors[__tdtor_n];
    if ( e.dtor ) e.dtor(e.obj);
  }
}

// the two entry points a host publishes in micron_attach_info for its guests
inline int
__attach_thread_atexit(void (*dtor)(void *), void *obj) noexcept
{
  return __push_thread_dtor(dtor, obj) ? 0 : -1;
}

inline void
__attach_run_thread_dtors() noexcept
{
  __run_thread_dtors();
}
#else
inline __attribute__((always_inline)) void
__run_thread_dtors() noexcept
{
}
#endif

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
};      // namespace micron
