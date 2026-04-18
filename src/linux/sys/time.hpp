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
constexpr static const i32 nohang = 1;   /* Don't block waiting.  */
constexpr static const i32 untraced = 2; /* Report status of stopped children.  */
/* Bits in the fourth argument to `waitid'.  */
constexpr static const i32 stopped = 2;             /* Report stopped child (same as WUNTRACED). */
constexpr static const i32 exited = 4;              /* Report dead child.  */
constexpr static const i32 continued = 8;           /* Report continued child.  */
constexpr static const i32 nowait = 0x01000000;     /* Don't reap, just poll status.  */
constexpr static const i32 nothread = 0x20000000;   /* Don't wait on children of other threads
                                    in this group */
constexpr static const i32 wait_all = 0x40000000;   /* Wait for any child.  */
constexpr static const i32 wait_clone = 0x80000000; /* Wait for cloned process.  */
constexpr static const clock_t clocks_per_sec = 1000000;
constexpr static const i32 clock_realtime = 0;
/* Monotonic system-wide clock.  */
constexpr static const i32 clock_monotonic = 1;
/* High-resolution timer from the CPU.  */
constexpr static const i32 clock_process_cputime_id = 2;
/* Thread-specific CPU-time clock.  */
constexpr static const i32 clock_thread_cputime_id = 3;
/* Monotonic system-wide clock, not adjusted for frequency scaling.  */
constexpr static const i32 clock_monotonic_raw = 4;
/* Identifier for system-wide realtime clock, updated only on ticks.  */
constexpr static const i32 clock_realtime_coarse = 5;
/* Monotonic system-wide clock, updated only on ticks.  */
constexpr static const i32 clock_monotonic_coarse = 6;
/* Monotonic system-wide clock that includes time spent in suspension.  */
constexpr static const i32 clock_boottime = 7;
/* Like clock_realtime but also wakes suspended system.  */
constexpr static const i32 clock_realtime_alarm = 8;
/* Like clock_boottime but also wakes suspended system.  */
constexpr static const i32 clock_boottime_alarm = 9;
/* Like clock_realtime but in International Atomic Time.  */
constexpr static const i32 clock_tai = 11;
constexpr static const i32 timer_abstime = 1;

/* Flags for timerfd_create.  */
constexpr static const i32 tfd_cloexec = 0x80000; /* Set close-on-exec flag.  */
constexpr static const i32 tfd_nonblock = 0x800;  /* Set non-blocking flag.   */

/* Flags for timerfd_settime.  */
constexpr static const i32 tfd_timer_abstime = (1 << 0);       /* Absolute expiry time.          */
constexpr static const i32 tfd_timer_cancel_on_set = (1 << 1); /* Cancel on clock discontinuity. */

/* Flags for timer_create signal notification.  */
constexpr static const i32 sigev_none = 1;      /* No async notification.          */
constexpr static const i32 sigev_signal = 0;    /* Notify via signal.              */
constexpr static const i32 sigev_thread = 2;    /* Deliver via thread function.    */
constexpr static const i32 sigev_thread_id = 4; /* Deliver to specific thread.     */

#if __wordsize == 64
struct timeval_t {
  time64_t tv_sec;              /* Seconds.  */
  posix::suseconds64_t tv_usec; /* Microseconds.  */
};

struct timespec_t {
  time64_t tv_sec; /* Seconds.  */
  slong_t tv_nsec; /* Nanoseconds.  */
};
#elif __wordsize == 32
struct timeval_t {
  i32 tv_sec;  /* Seconds */
  i32 tv_usec; /* Microseconds */
};

struct timespec_t {
  i32 tv_sec;  /* Seconds */
  i32 tv_nsec; /* Nanoseconds */
};
#endif

struct itimerspec_t {
  timespec_t it_interval; /* Timer period (0 = one-shot).  */
  timespec_t it_value;    /* Initial expiry (0 = disarmed). */
};

struct itimerval_t {
  timeval_t it_interval; /* Timer period.  */
  timeval_t it_value;    /* Time until next expiry.  */
};

constexpr static const i32 itimer_real = 0;    /* Decrements in real time; sends SIGALRM.    */
constexpr static const i32 itimer_virtual = 1; /* Decrements in process virtual time.        */
constexpr static const i32 itimer_prof = 2;    /* Decrements in process + kernel time.       */

union sigval_t {
  i32 sival_int;
  void *sival_ptr;
};

struct sigevent_t {
  sigval_t sigev_value;
  i32 sigev_signo;
  i32 sigev_notify;
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

ssize_t
clock_gettime(clockid_t clc, timespec_t &tm)
{
  return micron::syscall(SYS_clock_gettime, clc, &tm);
}

ssize_t
clock_getres(clockid_t clc, timespec_t *res)
{
  return micron::syscall(SYS_clock_getres, clc, res);
}

ssize_t
clock_getres(clockid_t clc, timespec_t &res)
{
  return micron::syscall(SYS_clock_getres, clc, &res);
}

ssize_t
clock_settime(clockid_t clc, const timespec_t &tm)
{
  return micron::syscall(SYS_clock_settime, clc, &tm);
}

ssize_t
clock_nanosleep(clockid_t clock, i32 flags, timespec_t &tm, timespec_t *rmn)
{
  return micron::syscall(SYS_clock_nanosleep, clock, flags, &tm, rmn);
}

ssize_t
clock_nanosleep(clockid_t clock, i32 flags, const timespec_t &tm)
{
  return micron::syscall(SYS_clock_nanosleep, clock, flags, &tm, nullptr);
}

clock_t
clock(void)
{
  timespec_t tm;
  if ( clock_gettime(clock_process_cputime_id, tm) != 0 ) return -1;
  return (tm.tv_sec * clocks_per_sec + tm.tv_nsec / (1000000000 / clocks_per_sec));
}

time_t
time(void)
{
#if !defined(__micron_arch_arm32)
  return micron::syscall(SYS_time, nullptr);
#else
  return micron::syscall(SYS_clock_gettime, nullptr);
#endif
}

double difftime(time_t t0, time_t t1);

i32
timerfd_create(clockid_t clockid, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_timerfd_create, clockid, flags));
}

i32
timerfd_settime(i32 fd, i32 flags, const itimerspec_t &new_val, itimerspec_t *old_val)
{
  return static_cast<i32>(micron::syscall(SYS_timerfd_settime, fd, flags, &new_val, old_val));
}

i32
timerfd_settime(i32 fd, i32 flags, const itimerspec_t &new_val)
{
  return static_cast<i32>(micron::syscall(SYS_timerfd_settime, fd, flags, &new_val, nullptr));
}

i32
timerfd_gettime(i32 fd, itimerspec_t &cur)
{
  return static_cast<i32>(micron::syscall(SYS_timerfd_gettime, fd, &cur));
}

i32
timer_create(clockid_t clockid, sigevent_t *sevp, timer_t &timerid)
{
  return static_cast<i32>(micron::syscall(SYS_timer_create, clockid, sevp, &timerid));
}

i32
timer_create(clockid_t clockid, timer_t &timerid)
{
  return static_cast<i32>(micron::syscall(SYS_timer_create, clockid, nullptr, &timerid));
}

i32
timer_delete(timer_t timerid)
{
  return static_cast<i32>(micron::syscall(SYS_timer_delete, timerid));
}

i32
timer_settime(timer_t timerid, i32 flags, const itimerspec_t &new_val, itimerspec_t *old_val)
{
  return static_cast<i32>(micron::syscall(SYS_timer_settime, timerid, flags, &new_val, old_val));
}

i32
timer_settime(timer_t timerid, i32 flags, const itimerspec_t &new_val)
{
  return static_cast<i32>(micron::syscall(SYS_timer_settime, timerid, flags, &new_val, nullptr));
}

i32
timer_gettime(timer_t timerid, itimerspec_t &cur)
{
  return static_cast<i32>(micron::syscall(SYS_timer_gettime, timerid, &cur));
}

i32
timer_getoverrun(timer_t timerid)
{
  return static_cast<i32>(micron::syscall(SYS_timer_getoverrun, timerid));
}

// legacy

i32
setitimer(i32 which, const itimerval_t &new_val, itimerval_t *old_val)
{
  return static_cast<i32>(micron::syscall(SYS_setitimer, which, &new_val, old_val));
}

i32
setitimer(i32 which, const itimerval_t &new_val)
{
  return static_cast<i32>(micron::syscall(SYS_setitimer, which, &new_val, nullptr));
}

i32
getitimer(i32 which, itimerval_t &cur)
{
  return static_cast<i32>(micron::syscall(SYS_getitimer, which, &cur));
}

u32
alarm(u32 seconds)
{
#if !defined(__micron_arch_arm32)
  return static_cast<i32>(micron::syscall(SYS_alarm, seconds));
#else
  return 0;
#endif
}

i32
clock_getcpuclockid(posix::pid_t pid, clockid_t &clc)
{
  clc = static_cast<clockid_t>(~(static_cast<u32>(pid) << 3));
  timespec_t probe;
  long r = micron::syscall(SYS_clock_gettime, clc, &probe);
  if ( r != 0 ) return static_cast<i32>(-r); /* return positive errno */
  return 0;
}

u64
timerfd_read(i32 fd)
{
  u64 count = 0;
  micron::syscall(SYS_read, fd, &count, sizeof(count));
  return count;
}

};     // namespace micron
