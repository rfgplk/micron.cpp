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
constexpr static const i32 wait_clone = static_cast<i32>(0x80000000u); /* Wait for cloned process.  */
constexpr static const clock_t clocks_per_sec = static_cast<i32>(1000000u);
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
  i32 tv_sec;  /* Seconds (legacy 32-bit timeval; getrusage/setitimer use durations, not absolute time) */
  i32 tv_usec; /* Microseconds */
};

struct timespec_t {
  time64_t tv_sec;
  i64 tv_nsec;
};
#endif

// guard against Y2038
#if defined(__micron_arch_width_32)
constexpr long __sys_clock_gettime = SYS_clock_gettime64;
constexpr long __sys_clock_settime = SYS_clock_settime64;
constexpr long __sys_clock_getres = SYS_clock_getres_time64;
constexpr long __sys_clock_nanosleep = SYS_clock_nanosleep_time64;
constexpr long __sys_timerfd_settime = SYS_timerfd_settime64;
constexpr long __sys_timerfd_gettime = SYS_timerfd_gettime64;
constexpr long __sys_timer_settime = SYS_timer_settime64;
constexpr long __sys_timer_gettime = SYS_timer_gettime64;
#else
constexpr long __sys_clock_gettime = SYS_clock_gettime;
constexpr long __sys_clock_settime = SYS_clock_settime;
constexpr long __sys_clock_getres = SYS_clock_getres;
constexpr long __sys_clock_nanosleep = SYS_clock_nanosleep;
constexpr long __sys_timerfd_settime = SYS_timerfd_settime;
constexpr long __sys_timerfd_gettime = SYS_timerfd_gettime;
constexpr long __sys_timer_settime = SYS_timer_settime;
constexpr long __sys_timer_gettime = SYS_timer_gettime;
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
#if defined(__micron_arch_width_32)
  return micron::syscall(SYS_clock_nanosleep_time64, clock_monotonic, 0, &req, &rem);
#else
  return micron::syscall(SYS_nanosleep, &req, &rem);
#endif
}

auto
nanosleep(const timespec_t &req)
{
#if defined(__micron_arch_width_32)
  return micron::syscall(SYS_clock_nanosleep_time64, clock_monotonic, 0, &req, nullptr);
#else
  return micron::syscall(SYS_nanosleep, &req, nullptr);
#endif
}

ssize_t
clock_gettime(clockid_t clc, timespec_t &tm)
{
  return micron::syscall(__sys_clock_gettime, clc, &tm);
}

ssize_t
clock_getres(clockid_t clc, timespec_t *res)
{
  return micron::syscall(__sys_clock_getres, clc, res);
}

ssize_t
clock_getres(clockid_t clc, timespec_t &res)
{
  return micron::syscall(__sys_clock_getres, clc, &res);
}

ssize_t
clock_settime(clockid_t clc, const timespec_t &tm)
{
  return micron::syscall(__sys_clock_settime, clc, &tm);
}

ssize_t
clock_nanosleep(clockid_t clock, i32 flags, timespec_t &tm, timespec_t *rmn)
{
  return micron::syscall(__sys_clock_nanosleep, clock, flags, &tm, rmn);
}

ssize_t
clock_nanosleep(clockid_t clock, i32 flags, const timespec_t &tm)
{
  return micron::syscall(__sys_clock_nanosleep, clock, flags, &tm, nullptr);
}

clock_t
clock(void)
{
  timespec_t tm;
  if ( clock_gettime(clock_process_cputime_id, tm) != 0 ) return -1;
  return (tm.tv_sec * clocks_per_sec + tm.tv_nsec / (1000000000 / clocks_per_sec));
}

time64_t
time(void)
{
#if defined(__micron_arch_amd64)
  return micron::syscall(SYS_time, nullptr);      // amd64 SYS_time returns a 64-bit time_t
#else
  timespec_t __ts{};
  if ( clock_gettime(clock_realtime, __ts) != 0 ) return static_cast<time64_t>(-1);
  return static_cast<time64_t>(__ts.tv_sec);
#endif
}

inline double
difftime(time_t t1, time_t t0)
{
  return static_cast<double>(t1) - static_cast<double>(t0);
}

i32
timerfd_create(clockid_t clockid, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_timerfd_create, clockid, flags));
}

i32
timerfd_settime(i32 fd, i32 flags, const itimerspec_t &new_val, itimerspec_t *old_val)
{
  return static_cast<i32>(micron::syscall(__sys_timerfd_settime, fd, flags, &new_val, old_val));
}

i32
timerfd_settime(i32 fd, i32 flags, const itimerspec_t &new_val)
{
  return static_cast<i32>(micron::syscall(__sys_timerfd_settime, fd, flags, &new_val, nullptr));
}

i32
timerfd_gettime(i32 fd, itimerspec_t &cur)
{
  return static_cast<i32>(micron::syscall(__sys_timerfd_gettime, fd, &cur));
}

i32
timer_create(clockid_t clockid, sigevent_t *sevp, timer_t &timerid)
{
  timerid = nullptr;
  return static_cast<i32>(micron::syscall(SYS_timer_create, clockid, sevp, &timerid));
}

i32
timer_create(clockid_t clockid, timer_t &timerid)
{
  timerid = nullptr;
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
  return static_cast<i32>(micron::syscall(__sys_timer_settime, timerid, flags, &new_val, old_val));
}

i32
timer_settime(timer_t timerid, i32 flags, const itimerspec_t &new_val)
{
  return static_cast<i32>(micron::syscall(__sys_timer_settime, timerid, flags, &new_val, nullptr));
}

i32
timer_gettime(timer_t timerid, itimerspec_t &cur)
{
  return static_cast<i32>(micron::syscall(__sys_timer_gettime, timerid, &cur));
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

// NOTE: SYS_alarm is x86 only
u32
alarm(u32 seconds)
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  return static_cast<u32>(micron::syscall(SYS_alarm, seconds));
#else
  itimerval_t nv{};
  itimerval_t ov{};
  nv.it_value.tv_sec = static_cast<decltype(nv.it_value.tv_sec)>(seconds);
  nv.it_value.tv_usec = 0;
  nv.it_interval.tv_sec = 0;
  nv.it_interval.tv_usec = 0;
  if ( setitimer(itimer_real, nv, &ov) < 0 ) return 0;
  u32 rem = static_cast<u32>(ov.it_value.tv_sec);
  if ( ov.it_value.tv_usec != 0 ) ++rem;
  return rem;
#endif
}

i32
clock_getcpuclockid(posix::pid_t pid, clockid_t &clc)
{
  clc = static_cast<clockid_t>(((~static_cast<u32>(pid)) << 3) | 2u);
  timespec_t probe;
  long r = micron::syscall(__sys_clock_gettime, clc, &probe);
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

// legacy wall-clock
auto
gettimeofday(timeval_t &tv) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_gettimeofday, &tv, nullptr));
}

struct tms_t {
  clock_t tms_utime;  /* user CPU time */
  clock_t tms_stime;  /* system CPU time */
  clock_t tms_cutime; /* user CPU time of waited-for children */
  clock_t tms_cstime; /* system CPU time of waited-for children */
};

clock_t
times(tms_t &buf)
{
  return static_cast<clock_t>(micron::syscall(SYS_times, &buf));
}

i32
settimeofday(const timeval_t &tv)
{
  return static_cast<i32>(micron::syscall(SYS_settimeofday, &tv, nullptr));
}

struct __timex_timeval {
  i64 tv_sec;
  i64 tv_usec;
};

struct timex_t {
  u32 modes; /* mode selector */
  i32 __pad0;
  i64 offset;   /* time offset, usec (or nsec with ADJ_NANO) */
  i64 freq;     /* frequency offset, scaled ppm */
  i64 maxerror; /* maximum error, usec */
  i64 esterror; /* estimated error, usec */
  i32 status;   /* clock command / status */
  i32 __pad1;
  i64 constant;  /* pll time constant */
  i64 precision; /* clock precision, usec (ro) */
  i64 tolerance; /* clock frequency tolerance, ppm (ro) */
  __timex_timeval time;
  i64 tick;    /* usec between clock ticks */
  i64 ppsfreq; /* pps frequency, scaled ppm (ro) */
  i64 jitter;  /* pps jitter, usec (ro) */
  i32 shift;   /* interval duration, s (ro) */
  i32 __pad2;
  i64 stabil; /* pps stability, scaled ppm (ro) */
  i64 jitcnt; /* jitter limit exceeded (ro) */
  i64 calcnt; /* calibration intervals (ro) */
  i64 errcnt; /* calibration errors (ro) */
  i64 stbcnt; /* stability limit exceeded (ro) */
  i32 tai;    /* TAI offset, seconds (ro) */
  i32 __pad3[11];
};

/* timex modes */
constexpr static const u32 adj_offset = 0x0001;
constexpr static const u32 adj_frequency = 0x0002;
constexpr static const u32 adj_maxerror = 0x0004;
constexpr static const u32 adj_esterror = 0x0008;
constexpr static const u32 adj_status = 0x0010;
constexpr static const u32 adj_timeconst = 0x0020;
constexpr static const u32 adj_tai = 0x0080;
constexpr static const u32 adj_setoffset = 0x0100;
constexpr static const u32 adj_microsecond = 0x1000;
constexpr static const u32 adj_nanosecond = 0x2000;
constexpr static const u32 adj_tick = 0x4000;
constexpr static const u32 adj_offset_singleshot = 0x8001;
constexpr static const u32 adj_offset_ss_read = 0xa001;

/* clock status bits */
constexpr static const i32 sta_pll = 0x0001;
constexpr static const i32 sta_ppsfreq = 0x0002;
constexpr static const i32 sta_ppstime = 0x0004;
constexpr static const i32 sta_fll = 0x0008;
constexpr static const i32 sta_ins = 0x0010;    /* insert a leap second */
constexpr static const i32 sta_del = 0x0020;    /* delete a leap second */
constexpr static const i32 sta_unsync = 0x0040; /* clock is NOT synchronised */
constexpr static const i32 sta_freqhold = 0x0080;
constexpr static const i32 sta_ppssignal = 0x0100;
constexpr static const i32 sta_ppsjitter = 0x0200;
constexpr static const i32 sta_ppswander = 0x0400;
constexpr static const i32 sta_ppserror = 0x0800;
constexpr static const i32 sta_clockerr = 0x1000;
constexpr static const i32 sta_nano = 0x2000; /* offset/precision are in nanoseconds */
constexpr static const i32 sta_mode = 0x4000;
constexpr static const i32 sta_clk = 0x8000;

/* adjtimex return values -- these are the SUCCESS codes, not errors */
constexpr static const i32 time_ok = 0;
constexpr static const i32 time_ins = 1;   /* a leap second will be inserted at end of day */
constexpr static const i32 time_del = 2;   /* a leap second will be deleted at end of day */
constexpr static const i32 time_oop = 3;   /* a leap second is in progress */
constexpr static const i32 time_wait = 4;  /* a leap second has occurred */
constexpr static const i32 time_error = 5; /* the clock is not synchronised */

#if defined(__micron_arch_width_32)
constexpr long __sys_adjtimex = SYS_clock_adjtime64;
constexpr long __sys_clock_adjtime = SYS_clock_adjtime64;
constexpr long __sys_sched_rr_get_interval = SYS_sched_rr_get_interval_time64;
#else
constexpr long __sys_adjtimex = SYS_adjtimex;
constexpr long __sys_clock_adjtime = SYS_clock_adjtime;
constexpr long __sys_sched_rr_get_interval = SYS_sched_rr_get_interval;
#endif

i32
adjtimex(timex_t &tx)
{
#if defined(__micron_arch_width_32)
  return static_cast<i32>(micron::syscall(__sys_adjtimex, clock_realtime, &tx));
#else
  return static_cast<i32>(micron::syscall(__sys_adjtimex, &tx));
#endif
}

i32
clock_adjtime(clockid_t clc, timex_t &tx)
{
  return static_cast<i32>(micron::syscall(__sys_clock_adjtime, clc, &tx));
}

i32
tai_offset(void)
{
  timex_t tx{};
  tx.modes = 0;      // a pure query
  const i32 r = adjtimex(tx);
  if ( r < 0 ) return r;
  return tx.tai;
}

i32
sched_rr_get_interval(posix::pid_t pid, timespec_t &ts)
{
  return static_cast<i32>(micron::syscall(__sys_sched_rr_get_interval, pid, &ts));
}

};      // namespace micron
