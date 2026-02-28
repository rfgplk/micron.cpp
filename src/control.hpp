//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "sync/pause.hpp"

#include "linux/__includes.hpp"
#include "linux/process/signals.hpp"
#include "types.hpp"

#include "exit.hpp"

namespace micron
{

inline void
alarm()
{
}

// a method to quickly crash a ring3 program
// pick your poison
template <int x = 0>
__attribute__((noreturn)) void
crash(void)
{
  if constexpr ( !x ) {
    __asm__ __volatile__("hlt");     // illegal instruction, the worst kind of userspace violation. immediately killthe
                                     // running program (including all children & threads), uncatchable and unstoppable,
                                     // guaranteed to work on any OS/kernel running x64 cpus. cannot be optimized away
  }
  if constexpr ( x == 1 ) {
    posix::raise(posix::sig_segv);
  } else if constexpr ( x == 2 ) {
    // assert(false);
  } else if constexpr ( x ) {
    char *a = nullptr;
    for ( int i = 0;; i++ )
      *(a + i) = 0xFF;
  }
}

inline void
halt()
{
  auto pid = posix::getpid();
  int r = posix::kill(pid, static_cast<int>(signal::terminate));
  if ( r < 0 )
    r = posix::kill(pid, static_cast<int>(signal::kill9));
}

inline void
stop()
{
  posix::raise(posix::sig_stop);
}

inline void
ignore_pipe()
{
  micron::ignore(signal::pipe);
}

inline auto
cont(posix::pid_t pid)
{
  return posix::kill(pid, static_cast<int>(signal::cont));
}

inline void
suspend_until(const posix::sigset_t &mask)
{
  micron::syscall(SYS_rt_sigsuspend, &mask, posix::__sig_syscall_size);
}

// NOTE: this is helpful for debugging, the func won't be optimized away so you can easily break on it via gdb
__attribute__((noinline)) __attribute__((used)) __attribute__((externally_visible)) __attribute__((optimize("O0"))) void
mark()
{
  __asm__ __volatile__("" ::: "memory");
}

template <typename Fn>
  requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn, int>)
inline void
on_terminate(Fn &&fn)
{
  create_handler(fn, signal::interrupt);
  create_handler(fn, signal::terminate);
  create_handler(fn, signal::hangup);
  create_handler(fn, signal::quit);
}

template <typename Fn1, typename Fn2>
  requires(micron::is_invocable_v<Fn1, int> && micron::is_invocable_v<Fn2, int>)
inline void
on_user_signals(Fn1 &&fn1, Fn2 &&fn2)
{
  create_handler(fn1, signal::usr1);
  create_handler(fn2, signal::usr2);
}

};     // namespace micron
