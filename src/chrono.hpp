//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "except.hpp"
#include "linux/sys/time.hpp"
#include "math/ratios.hpp"
#include "memory/cmemory.hpp"

namespace micron
{

// standard duration
using duration_t = __time_t;
using duration_d = double;
constexpr duration_d duration_ratio = 1000;     // the standard SI metric prefix (power of 1000)

inline constexpr duration_d
days(const duration_d s)
{
  return (s / (60 * 60 * 24));
}

inline constexpr duration_d
hours(const duration_d s)
{
  return (s / (60 * 60));
}

inline constexpr duration_d
minutes(const duration_d s)
{
  return (s / 60);
}

template <typename S = base_ratio>
inline constexpr duration_d
seconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}
template <typename S = milli>
inline constexpr duration_d
milliseconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}
template <typename S = micro>
inline constexpr duration_d
microseconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}
template <typename S = nano>
inline constexpr duration_d
nanoseconds(const duration_d s)
{
  return (s * S::denom) / S::num;
}

enum class system_clocks : clockid_t {
  realtime_set = clock_realtime,
  realtime = clock_realtime_alarm,
  realtime_coarse = clock_process_cputime_id,
  taitime = clock_tai,
  monotonic = clock_monotonic,
  monotonic_coarse = clock_monotonic_coarse,
  monotonic_raw = clock_monotonic_raw,
  since_boot = clock_boottime,
  cputime = clock_process_cputime_id,
  cputime_this = clock_thread_cputime_id,
  __end
};

// clock meant to get general time
template <system_clocks C = system_clocks::realtime> struct system_clock {
  timespec_t time_begin;
  timespec_t time_end;

  system_clock()
  {
    micron::memset(&time_begin, 0x0, sizeof(timespec_t));
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
    if ( micron::clock_gettime((clockid_t)C, time_begin) == -1 )
      throw except::runtime_error("micron::system_clock failed to get time");
  }
  inline __attribute__((always_inline)) void
  start(void)
  {
    if ( micron::clock_gettime((clockid_t)C, time_begin) == -1 )
      throw except::runtime_error("micron::system_clock failed to get time");
  }
  inline __attribute__((always_inline)) auto
  start_get(void) -> timespec_t
  {
    if ( micron::clock_gettime((clockid_t)C, time_begin) == -1 )
      throw except::runtime_error("micron::system_clock failed to get time");
    return time_begin;
  }
  inline __attribute__((always_inline)) void
  stop(void)
  {
    if ( micron::clock_gettime((clockid_t)C, time_end) == -1 )
      throw except::runtime_error("micron::system_clock failed to get time");
  }
  inline __attribute__((always_inline)) auto
  stop_get(void) -> timespec_t
  {
    if ( micron::clock_gettime((clockid_t)C, time_end) == -1 )
      throw except::runtime_error("micron::system_clock failed to get time");
    return time_end;
  }
  inline __attribute__((always_inline)) static auto
  now(void) -> double
  {
    timespec_t t;
    if ( micron::clock_gettime((clockid_t)C, t) == -1 )
      throw except::runtime_error("micron::system_clock failed to get time");
    auto sec = t.tv_sec;
    auto msec = (t.tv_nsec) / 1000000000;
    return static_cast<double>(sec) + static_cast<double>(msec);
  }
  auto
  read(const timespec_t &t) -> double
  {
    auto sec = t.tv_sec - time_begin.tv_sec;
    auto msec = (t.tv_nsec - time_begin.tv_nsec) / 1000000000;
    return static_cast<double>(sec) + static_cast<double>(msec);
  }
  auto
  read_ms(const timespec_t &t) -> double
  {
    auto sec = t.tv_sec - time_begin.tv_sec;
    auto msec = (t.tv_nsec - time_begin.tv_nsec) / 1000000;
    return micron::milliseconds(static_cast<double>(sec)) + static_cast<double>(msec);
  }
  auto
  read(void) -> double
  {
    auto sec = time_end.tv_sec - time_begin.tv_sec;
    auto msec = (time_end.tv_nsec - time_begin.tv_nsec) / 1000000000;
    return static_cast<double>(sec) + static_cast<double>(msec);
  }
  auto
  read_ms(void) -> double
  {
    auto sec = time_end.tv_sec - time_begin.tv_sec;
    auto msec = (time_end.tv_nsec - time_begin.tv_nsec) / 1000000;
    return micron::milliseconds(static_cast<double>(sec)) + static_cast<double>(msec);
  }
};

duration_d
now(void)
{
  timespec_t t;
  if ( micron::clock_gettime((clockid_t)clock_realtime_alarm, t) == -1 )
    throw except::runtime_error("micron::now failed to get time");
  auto sec = t.tv_sec;
  auto msec = (t.tv_nsec) / 1000000000;
  return static_cast<double>(sec) + static_cast<double>(msec);
}

};
