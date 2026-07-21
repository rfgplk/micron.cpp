//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// provides Itanium C++ ABI references for a TU that didn't include start
//   .. __cxa_thread_atexit
//   .. __cxa_atexit
//   .. __cxa_guard_*
//   .. __dso_handle

#if defined(MICRON_ATTACH_MODULE)

#include "../bits/__arch.hpp"
#include "../bits/__pause.hpp"
#include "../bits/__thread_exit_hook.hpp"
#include "../exit.hpp"

extern "C" {

__attribute__((used, visibility("hidden"))) int
__cxa_thread_atexit(void (*dtor)(void *), void *obj, void * /*dso*/) noexcept
{
  if ( dtor == nullptr ) return -1;
  // WARNING: on failure this drops the registration; it must never fall back to the process wide atexit table
  return micron::__push_thread_dtor(dtor, obj) ? 0 : -1;
}

// function-local static dtors: drained at _detach (the module has no process exit)
__attribute__((used, visibility("hidden"))) int
__cxa_atexit(void (*dtor)(void *), void *obj, void * /*dso*/) noexcept
{
  if ( dtor == nullptr ) return -1;
  return micron::__exit_internal::__push(dtor, obj);
}

__attribute__((used, visibility("hidden"))) int
__cxa_guard_acquire(__micron_guard_t *g)
{
  unsigned char *b = reinterpret_cast<unsigned char *>(g);
  if ( __atomic_load_n(&b[0], __ATOMIC_ACQUIRE) ) return 0;
  for ( ;; ) {
    unsigned char expect = 0;
    if ( __atomic_compare_exchange_n(&b[1], &expect, 1, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE) ) {
      if ( __atomic_load_n(&b[0], __ATOMIC_ACQUIRE) ) {
        __atomic_store_n(&b[1], 0, __ATOMIC_RELEASE);
        return 0;
      }
      return 1;
    }
    while ( __atomic_load_n(&b[1], __ATOMIC_ACQUIRE) ) __cpu_pause();
    if ( __atomic_load_n(&b[0], __ATOMIC_ACQUIRE) ) return 0;
  }
}

__attribute__((used, visibility("hidden"))) void
__cxa_guard_release(__micron_guard_t *g)
{
  unsigned char *b = reinterpret_cast<unsigned char *>(g);
  __atomic_store_n(&b[0], 1, __ATOMIC_RELEASE);
  __atomic_store_n(&b[1], 0, __ATOMIC_RELEASE);
}

__attribute__((used, visibility("hidden"))) void
__cxa_guard_abort(__micron_guard_t *g)
{
  unsigned char *b = reinterpret_cast<unsigned char *>(g);
  __atomic_store_n(&b[1], 0, __ATOMIC_RELEASE);
}

#if defined(__micron_arch_arm32)
__attribute__((used, visibility("hidden"))) int
__aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle) noexcept
{
  return __cxa_atexit(destructor, object, dso_handle);
}
#endif

}      // extern "C"

__attribute__((used, visibility("hidden"))) void *__dso_handle = &__dso_handle;

#endif
