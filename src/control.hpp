//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "linux/calls.hpp"
#include "linux/sys/time.hpp"
#include "linux/sys/signal.hpp"
#include "linux/process/wait.hpp"
#include "thread/signal.hpp"
#include "errno.hpp"

//#include <signal.h>

#include "types.hpp"

namespace micron
{
// routines for controlling program state
inline void
halt()
{
}

inline void
stop()
{
  ::raise(SIG_STOP);
}

template <typename F, typename... Args>
inline void
pause(F f, Args... args)
{
  sigset_t signal;
  sigemptyset(&signal);
  sigaddset(&signal, SIG_CONT);
  int sig = 0;
  auto s = sigwait(&signal, &sig);
  f(sig, args...);
  return;
}
inline void
pause()
{
  sigset_t signal;
  sigemptyset(&signal);
  sigaddset(&signal, SIG_CONT);
  sigprocmask(SIG_BLOCK, &signal, nullptr);
  int sig = 0;
  sigwait(&signal, &sig);
  return;
}
// wait for pid to finish
inline int
wait(int pid)
{
  int status = 0;
  return micron::waitpid(pid, &status, 0);
}

// wait for pid to finish
inline int
wait_thread(int tid)
{
  // int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
  siginfo_t i;
  return static_cast<int>(micron::waitid(P_PID, tid, i, exited | stopped));
}

inline int
can_wait(int tid)
{
  // int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
  siginfo_t i;
  micron::waitid(P_PID, tid, i, exited | nowait);
  if ( i.si_code == CLD_EXITED or i.si_code == CLD_DUMPED or i.si_code == CLD_KILLED )
    return true;
  return false;
}

inline int
try_wait_thread(int tid)
{
  // int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
  int status = 0;
  return micron::waitpid(tid, &status, nohang);
}
// wait for signal, equivalent to sigsuspend
template <typename... Args>
inline int
await(Args... args)
{
  sigset_t old;
  int save;
  sigset_t signal;
  ::sigemptyset(&signal);
  (::sigaddset(&signal, args), ...);
  if ( ::sigprocmask(SIG_SETMASK, &signal, &old) < 0 )
    return -1;
  ::sigwait(&signal, &save);
  if ( ::sigprocmask(SIG_SETMASK, &old, nullptr) < 0 )
    return -1;
  return 0;
}
inline void
alarm()
{
}

// a method to quickly crash a ring3 program
// pick your poison
template <int x = 0>
__attribute__((no_return)) void
crash(void)
{
  if constexpr ( !x ) {
    __asm__ __volatile__("hlt");     // illegal instruction, the worst kind of userspace violation. immediately killthe
                                     // running program (including all children & threads), uncatchable and unstoppable,
                                     // guaranteed to work on any OS/kernel running x64 cpus. cannot be optimized away
  }
  if constexpr ( x == 1 ) {
    ::raise(SIGSEGV);
  } else if constexpr ( x == 2 ) {
    // assert(false);
  } else if constexpr ( x ) {
    char *a = nullptr;
    for ( int i = 0;; i++ )
      *(a + i) = 0xFF;
  }
}

// in seconds
inline void
ssleep(const umax_t s)
{
  timespec_t r, rmn;
  r.tv_sec = s;
  r.tv_nsec = 0;
  while ( micron::nanosleep(r, rmn) == -1 && errno == EINTR ) {
    r = rmn;
  }
}

// in ms
inline void
sleep_for(const umax_t ms)
{
  timespec_t r, rmn;
  r.tv_sec = ms / (umax_t)1000;
  r.tv_nsec = (ms % (umax_t)1000) * (umax_t)1000;
  while ( micron::nanosleep(r, rmn) == -1 && errno == EINTR ) {
    r = rmn;
  }
}
// in ms
inline void
sleep(const umax_t ms)
{
  timespec_t r;
  r.tv_sec = ms / (umax_t)1000;
  r.tv_nsec = (ms % (umax_t)1000) * (umax_t)1000;
  while ( micron::nanosleep(r) == -1 && errno == EINTR ) {
  }
}
};
