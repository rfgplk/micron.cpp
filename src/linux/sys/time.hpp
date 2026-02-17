
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../sys/types.hpp"

namespace micron
{

/* Bits in the third argument to `waitpid'.  */
constexpr int nohang = 1;   /* Don't block waiting.  */
constexpr int untraced = 2; /* Report status of stopped children.  */

/* Bits in the fourth argument to `waitid'.  */
constexpr int stopped = 2;         /* Report stopped child (same as WUNTRACED). */
constexpr int exited = 4;          /* Report dead child.  */
constexpr int continued = 8;       /* Report continued child.  */
constexpr int nowait = 0x01000000; /* Don't reap, just poll status.  */

constexpr int nothread = 0x20000000;   /* Don't wait on children of other threads
                                    in this group */
constexpr int wait_all = 0x40000000;   /* Wait for any child.  */
constexpr int wait_clone = 0x80000000; /* Wait for cloned process.  */

constexpr clock_t clocks_per_sec = 1000000;

constexpr int clock_realtime = 0;
/* Monotonic system-wide clock.  */
constexpr int clock_monotonic = 1;
/* High-resolution timer from the CPU.  */
constexpr int clock_process_cputime_id = 2;
/* Thread-specific CPU-time clock.  */
constexpr int clock_thread_cputime_id = 3;
/* Monotonic system-wide clock, not adjusted for frequency scaling.  */
constexpr int clock_monotonic_raw = 4;
/* Identifier for system-wide realtime clock, updated only on ticks.  */
constexpr int clock_realtime_coarse = 5;
/* Monotonic system-wide clock, updated only on ticks.  */
constexpr int clock_monotonic_coarse = 6;
/* Monotonic system-wide clock that includes time spent in suspension.  */
constexpr int clock_boottime = 7;
/* Like clock_realtime but also wakes suspended system.  */
constexpr int clock_realtime_alarm = 8;
/* Like clock_boottime but also wakes suspended system.  */
constexpr int clock_boottime_alarm = 9;
/* Like clock_realtime but in International Atomic Time.  */
constexpr int clock_tai = 11;

constexpr int timer_abstime = 1;

struct timeval_t {
  time64_t tv_sec;              /* Seconds.  */
  posix::suseconds64_t tv_usec; /* Microseconds.  */
};

struct timespec_t {
  time64_t tv_sec; /* Seconds.  */
  slong_t tv_nsec; /* Nanoseconds.  */
};

auto
nanosleep(const timespec_t &req, timespec_t &rem)
{
  return micron::syscall(SYS_nanosleep, &req, &rem);
}

auto
nanosleep(const timespec_t &req)
{
  return micron::syscall(SYS_nanosleep, &req, nullptr);
}

long int
clock_gettime(clockid_t clc, timespec_t &tm)
{
  return micron::syscall(SYS_clock_gettime, clc, &tm);
}

clock_t
clock(void)
{
  timespec_t tm;
  if ( clock_gettime(clock_process_cputime_id, tm) != 0 )
    return -1;
  return (tm.tv_sec * clocks_per_sec + tm.tv_nsec / (1000000000 / clocks_per_sec));
}

time_t
time(void)
{
  return micron::syscall(SYS_time, nullptr);
}

double difftime(time_t t0, time_t t1);

long int
clock_getres(clockid_t clc, timespec_t *res)
{
  return micron::syscall(SYS_clock_getres, clc, res);
}

int clock_settime(clockid_t clc, timespec_t &tm);
int clock_getcpuclockid(posix::pid_t pid, clockid_t &clc);

long int
clock_nanosleep(clockid_t clock, int flags, timespec_t &tm, timespec_t *rmn)
{
  return micron::syscall(SYS_clock_nanosleep, clock, flags, &tm, rmn);
}
};
