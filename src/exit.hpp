//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "atomic/intrin.hpp"
#include "syscall.hpp"
#include "types.hpp"

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

inline constexpr usize __atexit_cap = 4096;

inline __atexit_entry __atexit_table[__atexit_cap] = {};
inline u32 __atexit_count = 0;
inline bool __exit_in_progress = false;

inline void
__atexit_thunk(void *p) noexcept
{
  using __noarg_fn = void (*)();
  (reinterpret_cast<__noarg_fn>(p))();
}

inline int
__push(__atexit_fn_t func, void *arg) noexcept
{
  u32 idx = micron::atom::fetch_add(&__atexit_count, 1u, micron::atomic_acq_rel);
  if ( idx >= __atexit_cap ) {
    micron::atom::fetch_sub(&__atexit_count, 1u, __ATOMIC_RELAXED);
    return -1;
  }
  __atexit_table[idx].func = func;
  micron::atom::thread_fence(micron::atomic_release);
  __atexit_table[idx].arg = arg;
  return 0;
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

// soft exit routine, fires destructors
__attribute__((noreturn)) inline void
exit(int s = exit_ok)
{
  using namespace __exit_internal;

  bool expected = false;
  if ( !micron::atom::compare_exchange(&__exit_in_progress, &expected, true, false, micron::atomic_seq_cst, __ATOMIC_RELAXED) ) {
    sys_exit(s);
  }

  for ( ;; ) {
    u32 cur = micron::atom::load(&__atexit_count, micron::atomic_acquire);
    if ( cur == 0 ) break;
    u32 idx = cur - 1;
    micron::atom::store(&__atexit_count, idx, micron::atomic_release);
    __atexit_entry e = __atexit_table[idx];
    if ( e.func ) e.func(e.arg);
  }

  // fire all destructors
  if ( __fini_array_start && __fini_array_end ) {
    for ( void (**p)(void) = __fini_array_end; p > __fini_array_start; ) {
      --p;
      if ( *p ) (*p)();
    }
  }

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

  for ( ;; ) {
    u32 cur = micron::atom::load(&__atexit_count, micron::atomic_acquire);
    if ( cur == 0 ) break;
    u32 idx = cur - 1;
    micron::atom::store(&__atexit_count, idx, micron::atomic_release);
    __atexit_entry e = __atexit_table[idx];
    if ( e.func ) e.func(e.arg);
  }

  // fire all destructors
  if ( __fini_array_start && __fini_array_end ) {
    for ( void (**p)(void) = __fini_array_end; p > __fini_array_start; ) {
      --p;
      if ( *p ) (*p)();
    }
  }

  proc_exit(s);
  __builtin_unreachable();
}

};      // namespace micron
