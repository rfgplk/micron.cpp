//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "errno.hpp"

#include "bits/__pause.hpp"

#include "linux/__includes.hpp"
#include "linux/process/wait.hpp"
#include "linux/sys/poll.hpp"
#include "linux/sys/signal.hpp"
#include "linux/sys/system.hpp"
#include "linux/sys/time.hpp"
#include "thread/signal.hpp"

#include "io/bits.hpp"

#include "types.hpp"

namespace micron
{

template <typename F, typename... Args>
inline void
pause(F f, Args... args)
{
  micron::sigset_t signal;
  micron::sigemptyset(signal);
  micron::sigaddset(signal, sig_cont);
  int sig = 0;
  auto s = micron::sigwait(signal, sig);
  f(sig, args...);
  return;
}
inline void
pause()
{
  micron::sigset_t signal;
  micron::sigemptyset(signal);
  micron::sigaddset(signal, sig_cont);
  micron::sigprocmask(sig_block, signal, nullptr);
  int sig = 0;
  micron::sigwait(signal, sig);
  return;
}

// WAITING SECTION

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
  int r = static_cast<int>(micron::waitid(P_PID, tid, i, exited));
  if ( r < 0 ) {
    if ( r == -10 ) {
    }
  }
  return r;
}

inline int
can_wait(int tid)
{
  // int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
  siginfo_t i;
  micron::waitid(P_PID, tid, i, exited | nowait);
  if ( i.si_code == cld_exited or i.si_code == cld_dumped or i.si_code == cld_killed )
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
  micron::sigset_t old;
  int save;
  micron::sigset_t signal;
  micron::sigemptyset(signal);
  (micron::sigaddset(signal, args), ...);
  if ( micron::sigprocmask(sig_setmask, signal, &old) < 0 )
    return -1;
  micron::sigwait(signal, save);
  if ( micron::sigprocmask(sig_setmask, old, nullptr) < 0 )
    return -1;
  return 0;
}

// SLEEPING SECTION
// nanosleep doesn't set errno anymore

// in seconds
inline void
ssleep(const umax_t s)
{
  timespec_t r, rmn;
  r.tv_sec = s;
  r.tv_nsec = 0;
  while ( micron::nanosleep(r, rmn) == -error::interrupted ) {
    r = rmn;
    __cpu_pause();
  }
}
// in milliseconds
inline void
sleep_for(umax_t ms)
{
  timespec_t r;
  r.tv_sec = ms / 1000;
  r.tv_nsec = (ms % 1000) * 1000000UL;

  while ( micron::nanosleep(r, r) == -error::interrupted ) {
    __cpu_pause();
  }
}

// in milliseconds
inline void
sleep(ulong_t ms)
{
  timespec_t r;
  r.tv_sec = ms / 1000;
  r.tv_nsec = (ms % 1000) * 1000000UL;

  while ( micron::nanosleep(r) == -error::interrupted ) {
    __cpu_pause();
  }
}

// in nanoseconds
inline void
sleep_nano(umax_t ns)
{
  timespec_t r;
  r.tv_sec = ns / 1000000000UL;
  r.tv_nsec = ns % 1000000000UL;

  while ( micron::nanosleep(r, r) == -error::interrupted ) {
    __cpu_pause();
  }
}

template <umax_t L>
void
cpu_pause()
{
  timespec_t r = { L / 1000000000UL, L % 1000000000UL };

  while ( micron::nanosleep(r, r) == -error::interrupted ) {
    __cpu_pause();
  }
}

};
