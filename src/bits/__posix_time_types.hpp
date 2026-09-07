//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// POSIX time records and clock ids; types only
//
// relocated from linux/sys/time.hpp

#include "../types.hpp"
#include "__posix_types.hpp"

namespace micron
{

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

};      // namespace micron
