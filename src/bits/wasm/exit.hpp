//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../__thread_exit_hook.hpp"
#include "runtime.hpp"

namespace micron
{
namespace __exit_internal
{
using __atexit_fn_t = void (*)(void *);
#ifndef MICRON_ATEXIT_CAP
#define MICRON_ATEXIT_CAP 4096
#endif
struct __entry {
  __atexit_fn_t fn;
  void *arg;
  void *dso;
};

inline __entry __entries[MICRON_ATEXIT_CAP]{};
inline unsigned __count = 0;
inline wasm::mutex __lock{};
inline bool __exit_in_progress = false;
inline unsigned __atexit_dropped = 0;

inline int
__push(__atexit_fn_t fn, void *arg, void *dso = nullptr) noexcept
{
  if ( !fn ) return -1;
  __lock.lock();
  if ( __count == MICRON_ATEXIT_CAP ) {
    ++__atexit_dropped;
    __lock.unlock();
    return -1;
  }
  __entries[__count++] = { fn, arg, dso };
  __lock.unlock();
  return 0;
}

inline void
__finalize_dso(void *dso) noexcept
{
  for ( ;; ) {
    __entry e{};
    __lock.lock();
    for ( unsigned i = __count; i; --i ) {
      if ( __entries[i - 1].fn && (!dso || dso == __entries[i - 1].dso) ) {
        e = __entries[i - 1];
        __entries[i - 1].fn = nullptr;
        break;
      }
    }
    while ( __count && !__entries[__count - 1].fn ) --__count;
    __lock.unlock();
    if ( !e.fn ) break;
    e.fn(e.arg);
  }
}

inline void
__atexit_thunk(void *p) noexcept
{
  reinterpret_cast<void (*)()>(p)();
}
}      // namespace __exit_internal

inline int
atexit(void (*fn)()) noexcept
{
  return __exit_internal::__push(__exit_internal::__atexit_thunk, reinterpret_cast<void *>(fn));
}

inline void
__run_exit_sequence() noexcept
{
  __run_thread_dtors();
  __exit_internal::__finalize_dso(nullptr);
  if ( __thread_exit_hook ) __thread_exit_hook();
  if ( __dynamic_tls_exit_hook ) __dynamic_tls_exit_hook();
}

[[noreturn]] inline void
sys_exit(int s)
{
  wasm::panic(s);
}

[[noreturn]] inline void
sys_group_exit(int s)
{
  wasm::panic(s);
}

[[noreturn]] inline void
_Exit(int s = 0)
{
  sys_exit(s);
}

[[noreturn]] inline void
proc_exit(int s)
{
  sys_group_exit(s);
}

[[noreturn]] inline void
quick_exit(int s)
{
  sys_exit(s);
}

[[noreturn]] inline void
abort()
{
  wasm::panic(134);
}

[[noreturn]] inline void
abort(int s)
{
  wasm::panic(s);
}

[[noreturn]] inline void
exit(int s = 0)
{
  if ( !__atomic_exchange_n(&__exit_internal::__exit_in_progress, true, __ATOMIC_ACQ_REL) ) __run_exit_sequence();
  wasm::panic(s);
}

[[noreturn]] inline void
group_exit(int s = 0)
{
  exit(s);
}
}      // namespace micron
