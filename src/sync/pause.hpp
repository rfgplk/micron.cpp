//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"

#include "../bits/__pause.hpp"

#include "../linux/__includes.hpp"
#include "../linux/process/wait.hpp"
#include "../linux/sys/poll.hpp"
#include "../linux/sys/signal.hpp"
#include "../linux/sys/system.hpp"
#include "../linux/sys/time.hpp"

#include "../linux/process/signals.hpp"

#include "../io/bits.hpp"

#include "../chrono.hpp"

#include "../types.hpp"

namespace micron
{
template <umax_t L>
void
cpu_pause()
{
  timespec_t r = { L / 1000000000UL, L % 1000000000UL };

  while ( micron::nanosleep(r, r) == -error::interrupted ) {
    __cpu_pause();
  }
}

template <typename F, typename... Args>
inline void
pause(F f, Args... args)
{
  micron::posix::sigset_t signal;
  micron::posix::sigemptyset(signal);
  micron::posix::sigaddset(signal, posix::sig_cont);
  int sig = 0;
  auto s = micron::posix::sigwait(signal, sig);
  f(sig, args...);
  return;
}

inline void
pause()
{
  micron::posix::sigset_t signal;
  micron::posix::sigemptyset(signal);
  micron::posix::sigaddset(signal, posix::sig_cont);
  micron::posix::sigprocmask(posix::sig_block, signal, nullptr);
  int sig = 0;
  micron::posix::sigwait(signal, sig);
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
  // int waitid(idtype_t idtype, id_t id, micron::siginfo_t *infop, int options);
  micron::posix::siginfo_t i;
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
  // int waitid(idtype_t idtype, id_t id, micron::siginfo_t *infop, int options);
  micron::posix::siginfo_t i;
  micron::waitid(P_PID, tid, i, exited | nowait);
  if ( i.si_code == posix::cld_exited or i.si_code == posix::cld_dumped or i.si_code == posix::cld_killed )
    return true;
  return false;
}

inline int
try_wait_thread(int tid)
{
  // int waitid(idtype_t idtype, id_t id, micron::siginfo_t *infop, int options);
  int status = 0;
  return micron::waitpid(tid, &status, nohang);
}

// wait for signal, equivalent to sigsuspend
template <typename... Args>
inline int
await(Args... args)
{
  micron::posix::sigset_t old;
  int save;
  micron::posix::sigset_t signal;
  micron::posix::sigemptyset(signal);
  (micron::posix::sigaddset(signal, args), ...);
  if ( micron::posix::sigprocmask(posix::sig_setmask, signal, &old) < 0 )
    return -1;
  micron::posix::sigwait(signal, save);
  if ( micron::posix::sigprocmask(posix::sig_setmask, old, nullptr) < 0 )
    return -1;
  return 0;
}

// SLEEPING SECTION

void
spin_for(fduration_t timeout)
{
  auto start = micron::system_clock<>::now();
  for ( ;; ) {
    if ( (micron::system_clock<>::now() - start >= timeout) )
      break;
    __cpu_pause();
  }
}

void
wait_for(fduration_t timeout)
{
  auto start = micron::system_clock<>::now();
  for ( ;; ) {
    if ( (micron::system_clock<>::now() - start >= timeout) )
      break;
    // since its a ms dt
    cpu_pause<1000>();
  }
}

void
sleep_duration(const fduration_t s)
{
  timespec_t r, rmn;

  const auto total_ms = s;

  r.tv_sec = static_cast<time64_t>(total_ms / 1000.0);

  const auto frac_ms = total_ms - static_cast<micron::fduration_t>(r.tv_sec) * 1000.0;

  r.tv_nsec = static_cast<long>(frac_ms * 1'000'000.0);
  while ( micron::nanosleep(r, rmn) == -error::interrupted ) {
    r = rmn;
    __cpu_pause();
  }
}

// in seconds
void
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
void
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
void
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
void
sleep_nano(umax_t ns)
{
  timespec_t r;
  r.tv_sec = ns / 1000000000UL;
  r.tv_nsec = ns % 1000000000UL;

  while ( micron::nanosleep(r, r) == -error::interrupted ) {
    __cpu_pause();
  }
}

};     // namespace micron
