//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/process/signals.hpp"
#include "../../linux/sys/__threads.hpp"

#include "../../atomic/atomic.hpp"
#include "../../bits/__thread_exit_hook.hpp"
#include "../../control.hpp"

namespace micron
{

// WARNING: without inline we hit ODR at link time
inline void
__thread_sigchld(int)
{
}

inline void
__thread_sigthrottle(int)
{
  // configure
  micron::ssleep(1);
}

inline void
__thread_cancel(int)
{
  // culled
  // calling pthread_testcancel() from a signal handler (the old behaviour) is NOT async-signal-safe
}

inline void
__thread_yield(int)
{
  micron::yield();
}

// WARNING: this no longer fires destructors, pthread exit wasn't fully async signal safe
inline __attribute__((noreturn)) void
__thread_stop(int)
{
  // SIGTERM forcibly stops this thread. The full exit epilogue (exit hook / pthread cleanup) is NOT
  // async-signal-safe and cannot run here, but clearing alive IS
  if ( micron::__micron_thread_alive_word )
    static_cast<atomic_token<bool> *>(micron::__micron_thread_alive_word)->store(false, memory_order_seq_cst);
  micron::syscall(SYS_exit, 0);
  __builtin_unreachable();
}

inline void
__thread_handler()
{
  static atomic_token<bool> __installed{ false };
  bool expected = false;
  if ( !__installed.compare_exchange_strong(expected, true, memory_order_acq_rel, memory_order_acquire) ) return;
  auto sa = micron::create_handler(__thread_sigthrottle, signal::user_signal_1);
  micron::add_action(sa, __thread_yield, signal::alarm);
  micron::create_handler(__thread_cancel, signal::user_signal_2);
  micron::create_handler(__thread_stop, signal::terminate);
}

};      // namespace micron
