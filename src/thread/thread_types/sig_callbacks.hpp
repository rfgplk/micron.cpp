//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/process/signals.hpp"
#include "../../linux/sys/micthread/threads.hpp"

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
__thread_yield(int)
{
  micron::yield();
}

// true per-thread suspend/resume
inline void
__thread_suspend_handler(int)
{
  posix::sigset_t wait_mask = {};
  micron::posix::sigfillset(wait_mask);
  micron::posix::sigdelset(wait_mask, (int)signal::thread_resume);
  micron::posix::sigdelset(wait_mask, (int)signal::terminate);
  micron::posix::sigdelset(wait_mask, (int)signal::kill9);
  micron::posix::sigdelset(wait_mask, (int)signal::segfault);
  micron::posix::sigdelset(wait_mask, (int)signal::abort);
  // sigsuspend is also a cancellation point
  micron::posix::sigsuspend(wait_mask);
}

inline void
__thread_resume_handler(int)
{
  // its delivery alone wakes the suspend handler's sigsuspend; nothing else to do
}

inline void
__thread_handler()
{
  static atomic_token<bool> __installed{ false };
  bool expected = false;
  if ( !__installed.compare_exchange_strong(expected, true, memory_order_acq_rel, memory_order_acquire) ) return;
  // SIGALRM -> cooperative yield: the only app-facing signal the core still installs
  micron::create_handler(__thread_yield, signal::alarm);
  // WARNING: NEVER install a handler for signal::terminate (SIGTERM)
  // per thread termination is now controlled by the dying futex word
  // NOTE: install with thread_resume blocked in its own mask so a resume racing the tiny window before sigsuspend stays pending (blocked)
  // and is delivered the instant sigsuspend unblocks
  {
    micron::posix::sigaction_t ssa = {};
    ssa.sigaction_handler.sa_handler = __thread_suspend_handler;
    micron::posix::sigemptyset(ssa.sa_mask);
    micron::posix::sigaddset(ssa.sa_mask, (int)signal::thread_resume);
    ssa.sa_flags = micron::posix::sa_restart;
    micron::posix::sigaction((int)signal::thread_suspend, ssa, nullptr);
  }
  micron::create_handler(__thread_resume_handler, signal::thread_resume);
}

};      // namespace micron
