//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "atomic/intrin.hpp"
#include "bits/__pause.hpp"
#include "bits/__profile.hpp"
#include "syscall.hpp"
#include "types.hpp"

#include "bits/__attach_hook.hpp"
#include "bits/__thread_exit_hook.hpp"

extern "C" {
// strong definition in io/__std.hpp, weakly stubbed in start.cpp
extern void __shutdown_io_buffers(void) __attribute__((weak));
}

namespace micron
{

// hard exit path
__attribute__((noreturn)) inline void
sys_exit(int ret)
{
  micron::syscall(SYS_exit, ret);
  __builtin_unreachable();
}

__attribute__((noreturn)) inline void
sys_group_exit(int ret)
{
#if defined(__micron_attach_capable)
  // WARNING: an attached guest must NEVER SYS_exit_group the host.
  if ( micron::__micron_attach_fatal ) {
    micron::__micron_attach_fatal(ret);
    for ( ;; ) micron::syscall(SYS_exit, ret);
  }
#endif
  micron::syscall(SYS_exit_group, ret);
  __builtin_unreachable();
}

constexpr static const int exit_ok = 0;

__attribute__((noreturn)) inline void
_Exit(int s = exit_ok)
{
  sys_exit(s);
  __builtin_unreachable();
}

__attribute__((noreturn)) inline void
proc_exit(const int s)
{
  sys_group_exit(s);
  __builtin_unreachable();
}

__attribute__((noreturn)) inline void
quick_exit(const int s)
{
  sys_exit(s);
  __builtin_unreachable();
}

// on abort we must drain buffers before exiting, otherwise data will be dropped
__attribute__((noreturn)) inline void
abort(void)
{
  if ( __shutdown_io_buffers ) __shutdown_io_buffers();
  sys_group_exit(6);
  __builtin_unreachable();
}

__attribute__((noreturn)) inline void
abort(int ret)
{
  if ( __shutdown_io_buffers ) __shutdown_io_buffers();
  sys_group_exit(ret);
  __builtin_unreachable();
}

namespace __exit_internal
{

using __atexit_fn_t = void (*)(void *);

struct __atexit_entry {
  __atexit_fn_t func;
  void *arg;
};

#ifndef MICRON_ATEXIT_CAP
#define MICRON_ATEXIT_CAP 4096
#endif
inline constexpr usize __atexit_cap = MICRON_ATEXIT_CAP;
static_assert(__atexit_cap >= 8, "micron: MICRON_ATEXIT_CAP must leave room for the runtime's own handlers.");

inline __atexit_entry __atexit_table[__atexit_cap] = {};

// WARNING: this is a monotonic allocation cursor, must never be lowered
inline u32 __atexit_count = 0;

inline u64 __atexit_pub[(__atexit_cap + 63) / 64] = {};
inline u32 __atexit_dropped = 0;      // reserved but never published entries the drain gave up on; should stay 0
inline bool __exit_in_progress = false;
inline bool __fini_fired = false;      // .fini_array is one shot, even across concurrent drainers

// how many no progress passes the drain will make waiting on a publication before giving up
inline constexpr u32 __atexit_publish_rounds = 1u << 16;

[[gnu::always_inline]] inline bool
__published(u32 idx) noexcept
{
  return (__atomic_load_n(&__atexit_pub[idx / 64], __ATOMIC_ACQUIRE) & (1ull << (idx % 64))) != 0;
}

inline void
__atexit_thunk(void *p) noexcept
{
  using __noarg_fn = void (*)();
  (reinterpret_cast<__noarg_fn>(p))();
}

inline int
__push(__atexit_fn_t func, void *arg) noexcept
{
  u32 idx = micron::atom::load(&__atexit_count, micron::atomic_acquire);
  for ( ;; ) {
    if ( idx >= __atexit_cap ) return -1;
    if ( micron::atom::compare_exchange(&__atexit_count, &idx, idx + 1u, false, micron::atomic_acq_rel, __ATOMIC_ACQUIRE) ) break;
  }
  __atexit_table[idx].arg = arg;
  __atomic_store_n(&__atexit_table[idx].func, func, __ATOMIC_RELAXED);
  __atomic_fetch_or(&__atexit_pub[idx / 64], 1ull << (idx % 64), __ATOMIC_ACQ_REL);      // releases both fields
  return 0;
}

// WARNING: arg may only be read after this returns nonnull
[[gnu::always_inline]] inline __atexit_fn_t
__claim(u32 idx) noexcept
{
  if ( !__published(idx) ) return nullptr;
  return __atomic_exchange_n(&__atexit_table[idx].func, static_cast<__atexit_fn_t>(nullptr), __ATOMIC_ACQ_REL);
}

};      // namespace __exit_internal

inline int
atexit(void (*fn)()) noexcept
{
  if ( fn == nullptr ) return -1;
  return __exit_internal::__push(__exit_internal::__atexit_thunk, reinterpret_cast<void *>(fn));
}

extern "C" {
extern void (*__fini_array_start[])(void) __attribute__((weak, visibility("hidden")));
extern void (*__fini_array_end[])(void) __attribute__((weak, visibility("hidden")));
}

inline void
__drain_atexit_table() noexcept
{
  using namespace __exit_internal;
  u32 lo = 0;
  u32 hi = micron::atom::load(&__atexit_count, micron::atomic_acquire);
  u32 stall = 0;
  while ( lo != hi ) {
    u32 pending = hi;      // lowest index seen reserved but unpublished this pass
    bool progress = false;
    for ( u32 i = hi; i-- != lo; ) {
      if ( !__published(i) ) {
        pending = i;
        continue;
      }
      __atexit_fn_t f = __claim(i);
      if ( f == nullptr ) continue;      // another drainer got there first
      progress = true;
      f(__atexit_table[i].arg);          // arg is safe to read
    }
    const u32 next = micron::atom::load(&__atexit_count, micron::atomic_acquire);
    if ( pending == hi ) {      // everything below hi is accounted for
      if ( next == hi ) break;
      lo = hi;
      hi = next;
      stall = 0;
      continue;
    }
    if ( !progress && next == hi ) {      // only unpublished entries left and nobody moved
      if ( ++stall > __atexit_publish_rounds ) {
        for ( u32 i = pending; i < hi; ++i )
          if ( !__published(i) ) __atomic_fetch_add(&__atexit_dropped, 1u, __ATOMIC_RELAXED);
        break;
      }
      __cpu_pause();
    } else {
      stall = 0;
    }
    lo = pending;
    hi = next;
  }
#if !defined(MICRON_ATTACH_MODULE)
  bool __fini_expected = false;
  if ( !micron::atom::compare_exchange(&__fini_fired, &__fini_expected, true, false, micron::atomic_seq_cst, __ATOMIC_RELAXED) ) return;
  if ( __fini_array_start && __fini_array_end ) {
    for ( void (**p)(void) = __fini_array_end; p > __fini_array_start; ) {
      --p;
      if ( *p ) (*p)();
    }
  }
#endif
}

// WARNING: a guest module must not do this; its __run_thread_dtors() forwards to the hosts list, so a guest calling exit() would destroy
// the host thread's thread_locals
//
// WARNING: these run arbitrary user dtors
__attribute__((always_inline)) inline void
__run_exit_sequence(void) noexcept
{
#if !defined(MICRON_ATTACH_MODULE)
  micron::__run_thread_dtors();
#endif
  __drain_atexit_table();
#if !defined(MICRON_ATTACH_MODULE)
  micron::__run_thread_dtors();
#endif
}

// soft exit routine, fires destructors
__attribute__((noreturn)) inline void
exit(int s = exit_ok)
{
  using namespace __exit_internal;

  bool expected = false;
  if ( !micron::atom::compare_exchange(&__exit_in_progress, &expected, true, false, micron::atomic_seq_cst, __ATOMIC_RELAXED) ) {
    sys_exit(s);
  }

  __run_exit_sequence();

  sys_exit(s);
  __builtin_unreachable();
}

__attribute__((noreturn)) inline void
group_exit(int s = exit_ok)
{
  using namespace __exit_internal;

  bool expected = false;
  if ( !micron::atom::compare_exchange(&__exit_in_progress, &expected, true, false, micron::atomic_seq_cst, __ATOMIC_RELAXED) ) {
    sys_group_exit(s);
  }

  __run_exit_sequence();

  proc_exit(s);
  __builtin_unreachable();
}

};      // namespace micron
