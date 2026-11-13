//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "runtime.hpp"

namespace micron
{
inline constexpr int clock_realtime = 0, clock_monotonic = 1, clock_process_cputime_id = 2, clock_thread_cputime_id = 3,
                     clock_monotonic_raw = 4, clock_realtime_coarse = 5, clock_monotonic_coarse = 6, clock_boottime = 7,
                     clock_realtime_alarm = 8, clock_boottime_alarm = 9, clock_tai = 11, timer_abstime = 1;
inline constexpr clock_t clocks_per_sec = 1000000;

struct timespec_t {
  time64_t tv_sec;
  i64 tv_nsec;
};

struct timeval_t {
  time64_t tv_sec;
  i64 tv_usec;
};

struct itimerspec_t {
  timespec_t it_interval, it_value;
};

struct itimerval_t {
  timeval_t it_interval, it_value;
};

inline ssize_t
clock_gettime(clockid_t id, timespec_t &out) noexcept
{
  unsigned which;
  if ( id == clock_realtime || id == clock_realtime_coarse )
    which = 0;
  else if ( id == clock_monotonic || id == clock_monotonic_coarse )
    which = 1;
  else
    return -95;
  const i64 n = __micron_wasm_host_clock(which);
  if ( n < 0 ) return static_cast<ssize_t>(n);
  out = { n / 1000000000LL, n % 1000000000LL };
  return 0;
}

inline ssize_t
clock_gettime(clockid_t id, timespec_t *out) noexcept
{
  return out ? clock_gettime(id, *out) : -22;
}

inline ssize_t
clock_getres(clockid_t id, timespec_t &out) noexcept
{
  timespec_t probe;
  const auto r = clock_gettime(id, probe);
  if ( r < 0 ) return r;
  // The host reports its guaranteed resolution, not a fabricated nanosecond clock.
  const i64 n = __micron_wasm_host_clock(id == clock_realtime || id == clock_realtime_coarse ? 2 : 3);
  if ( n <= 0 ) return -95;
  out = { n / 1000000000LL, n % 1000000000LL };
  return 0;
}

inline ssize_t
clock_getres(clockid_t id, timespec_t *out) noexcept
{
  return out ? clock_getres(id, *out) : -22;
}

inline thread_local unsigned __wasm_sleep_word = 0;

inline ssize_t
clock_nanosleep(clockid_t id, int flags, const timespec_t &ts, timespec_t *rem = nullptr) noexcept
{
  if ( ts.tv_sec < 0 || ts.tv_nsec < 0 || ts.tv_nsec >= 1000000000LL || (flags & ~timer_abstime) ) return -22;
  if ( ts.tv_sec > (0x7fffffffffffffffLL - ts.tv_nsec) / 1000000000LL ) return -22;
  i64 ns = ts.tv_sec * 1000000000LL + ts.tv_nsec;
  timespec_t now;
  const auto r = clock_gettime(id, now);
  if ( r < 0 ) return r;
  if ( flags == timer_abstime ) ns -= now.tv_sec * 1000000000LL + now.tv_nsec;
  if ( rem ) *rem = {};
  if ( ns <= 0 ) return 0;
  const auto wait = wasm::wait32(&__wasm_sleep_word, 0, ns);
  return wait == -110 ? 0 : wait;
}

inline ssize_t
nanosleep(const timespec_t &req, timespec_t &rem) noexcept
{
  return clock_nanosleep(clock_monotonic, 0, req, &rem);
}

inline ssize_t
nanosleep(const timespec_t &req) noexcept
{
  return clock_nanosleep(clock_monotonic, 0, req);
}

inline time_t
time(time_t *out = nullptr) noexcept
{
  timespec_t ts;
  if ( clock_gettime(clock_realtime, ts) < 0 ) return -1;
  if ( out ) *out = static_cast<time_t>(ts.tv_sec);
  return static_cast<time_t>(ts.tv_sec);
}

inline ssize_t
gettimeofday(timeval_t *out, void * = nullptr) noexcept
{
  if ( !out ) return -22;
  timespec_t ts;
  const auto r = clock_gettime(clock_realtime, ts);
  if ( r < 0 ) return r;
  *out = { ts.tv_sec, ts.tv_nsec / 1000 };
  return 0;
}
}      // namespace micron
