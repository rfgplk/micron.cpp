//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "errno.hpp"
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

template <int P = poll_in>
inline auto
make_poll(const io::fd_t &hnd) -> pollfd
{
  if ( hnd.has_error() )
    return {};
  pollfd pfd = {};
  pfd.fd = hnd.fd;
  pfd.events = P;
  return pfd;
}

inline int
poll_for(pollfd &pfd, const int timeout)
{
  return micron::poll(pfd, 1, timeout);
}

// routines for controlling program state
inline void
halt()
{
}

inline void
stop()
{
  posix::raise(sig_stop);
}

__attribute__((noinline)) __attribute__((used)) __attribute__((optimize("O0"))) void
mark()
{
}

__attribute__((noreturn)) void
exit(int s = 0)
{
  __builtin__exit(s);
}
__attribute__((noreturn)) void
abort(void)
{
  __builtin__exit(sig_abrt);
}
__attribute__((noreturn)) void
quick_exit(const int s = sig_abrt)
{
  _Exit(s);
}

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
    posix::raise(sig_segv);
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
  r.tv_nsec = (ms % (umax_t)1000) * (umax_t)1000000;
  while ( micron::nanosleep(r, rmn) == -1 && errno == EINTR ) {
    r = rmn;
  }
}
// in ms
inline void
sleep(const ulong_t ms)
{
  timespec_t r;
  r.tv_sec = ms / (umax_t)1000;
  r.tv_nsec = ms * (umax_t)1000000;
  while ( micron::nanosleep(r) == -1 && errno == EINTR ) {
  }
}
};
