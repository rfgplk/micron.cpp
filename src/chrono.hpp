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

using duration_t = time_t;
using fduration_t = f64;
constexpr fduration_t duration_ratio = 1000;     // the standard SI metric prefix (power of 1000)

inline constexpr fduration_t
days(const fduration_t s)
{
  return (s / (60 * 60 * 24));
}

inline constexpr fduration_t
hours(const fduration_t s)
{
  return (s / (60 * 60));
}

inline constexpr fduration_t
minutes(const fduration_t s)
{
  return (s / 60);
}

template <typename S = base_ratio>
inline constexpr fduration_t
seconds(const fduration_t s)
{
  return (s * S::denom) / S::num;
}

template <typename S = milli>
inline constexpr fduration_t
milliseconds(const fduration_t s)
{
  return (s * S::denom) / S::num;
}

template <typename S = micro>
inline constexpr fduration_t
microseconds(const fduration_t s)
{
  return (s * S::denom) / S::num;
}

template <typename S = nano>
inline constexpr fduration_t
nanoseconds(const fduration_t s)
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

template <system_clocks C = system_clocks::realtime> struct system_clock {
  timespec_t time_begin;
  timespec_t time_end;

  system_clock()
  {
    micron::memset(&time_begin, 0x0, sizeof(timespec_t));
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
    if ( micron::clock_gettime((clockid_t)C, time_begin) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) void
  start(void)
  {
    if ( micron::clock_gettime((clockid_t)C, time_begin) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  start_get(void) -> timespec_t
  {
    if ( micron::clock_gettime((clockid_t)C, time_begin) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
    return time_begin;
  }

  inline __attribute__((always_inline)) void
  stop(void)
  {
    if ( micron::clock_gettime((clockid_t)C, time_end) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  stop_get(void) -> timespec_t
  {
    if ( micron::clock_gettime((clockid_t)C, time_end) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
    return time_end;
  }

  inline __attribute__((always_inline)) static auto
  now(void) -> fduration_t
  {
    timespec_t t;
    if ( micron::clock_gettime((clockid_t)C, t) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
    auto msec = t.tv_nsec / 1000000;
    return static_cast<fduration_t>(t.tv_sec) * 1000 + static_cast<fduration_t>(msec);
  }

  auto
  read(const timespec_t &t) -> fduration_t
  {
    time_t sec = t.tv_sec - time_begin.tv_sec;
    long nsec = t.tv_nsec - time_begin.tv_nsec;

    if ( nsec < 0 ) {
      --sec;
      nsec += 1000000000L;
    }

    return static_cast<fduration_t>(sec) + static_cast<fduration_t>(nsec) * 1e-9;
  }

  auto
  read(void) -> fduration_t
  {
    time_t sec = time_end.tv_sec - time_begin.tv_sec;
    long nsec = time_end.tv_nsec - time_begin.tv_nsec;

    if ( nsec < 0 ) {
      --sec;
      nsec += 1000000000L;
    }

    return static_cast<fduration_t>(sec) + static_cast<fduration_t>(nsec) * 1e-9;
  }

  auto
  read_ms(const timespec_t &t) -> fduration_t
  {
    time_t sec = t.tv_sec - time_begin.tv_sec;
    long nsec = t.tv_nsec - time_begin.tv_nsec;

    if ( nsec < 0 ) {
      --sec;
      nsec += 1000000000L;
    }

    return static_cast<fduration_t>(sec) * 1000.0 + static_cast<fduration_t>(nsec) * 1e-6;
  }

  auto
  read_ms(void) -> fduration_t
  {
    time_t sec = time_end.tv_sec - time_begin.tv_sec;
    long nsec = time_end.tv_nsec - time_begin.tv_nsec;

    if ( nsec < 0 ) {
      --sec;
      nsec += 1000000000L;
    }

    return static_cast<fduration_t>(sec) * 1000.0 + static_cast<fduration_t>(nsec) * 1e-6;
  }
};

fduration_t
now(void)
{
  timespec_t t;
  if ( micron::clock_gettime((clockid_t)clock_realtime_alarm, t) == -1 )
    exc<except::runtime_error>("micron::now failed to get time");
  auto msec = t.tv_nsec / 1000000;
  return static_cast<fduration_t>(t.tv_sec) * 1000 + static_cast<fduration_t>(msec);
}

template <typename C = system_clock<>, typename D = fduration_t> struct time_point {
  D d;

  constexpr time_point() : d(0) {}

  constexpr explicit time_point(const D &dur) : d(dur) {}

  constexpr D
  time_since_epoch() const
  {
    return d;
  }

  constexpr time_point &
  operator+=(const D &dur)
  {
    d += dur;
    return *this;
  }

  constexpr time_point &
  operator-=(const D &dur)
  {
    d -= dur;
    return *this;
  }

  static time_point
  now()
  {
    return time_point(C::now());
  }
};

template <typename C, typename D>
constexpr time_point<C, D>
operator+(const time_point<C, D> &tp, const D &d)
{
  return time_point<C, D>(tp.time_since_epoch() + d);
}

template <typename C, typename D>
constexpr time_point<C, D>
operator+(const D &d, const time_point<C, D> &tp)
{
  return tp + d;
}

template <typename C, typename D>
constexpr time_point<C, D>
operator-(const time_point<C, D> &tp, const D &d)
{
  return time_point<C, D>(tp.time_since_epoch() - d);
}

template <typename C, typename D>
constexpr D
operator-(const time_point<C, D> &lhs, const time_point<C, D> &rhs)
{
  return lhs.time_since_epoch() - rhs.time_since_epoch();
}

struct time_of_day {
  fduration_t hours_val;
  fduration_t minutes_val;
  fduration_t seconds_val;
  fduration_t subseconds_val;

  constexpr time_of_day() : hours_val(0), minutes_val(0), seconds_val(0), subseconds_val(0) {}

  constexpr explicit time_of_day(fduration_t dur_since_midnight)
  {
    auto total = dur_since_midnight;
    hours_val = static_cast<fduration_t>(static_cast<duration_t>(total / 3600) % 24);
    total -= hours_val * 3600;
    minutes_val = static_cast<fduration_t>(static_cast<duration_t>(total / 60));
    total -= minutes_val * 60;
    seconds_val = static_cast<fduration_t>(static_cast<duration_t>(total));
    subseconds_val = total - seconds_val;
  }

  constexpr time_of_day(fduration_t h, fduration_t m, fduration_t s = 0, fduration_t ss = 0)
      : hours_val(h), minutes_val(m), seconds_val(s), subseconds_val(ss)
  {
  }

  constexpr fduration_t
  hours() const
  {
    return hours_val;
  }

  constexpr fduration_t
  minutes() const
  {
    return minutes_val;
  }

  constexpr fduration_t
  seconds() const
  {
    return seconds_val;
  }

  constexpr fduration_t
  subseconds() const
  {
    return subseconds_val;
  }

  constexpr fduration_t
  to_duration() const
  {
    return hours_val * 3600 + minutes_val * 60 + seconds_val + subseconds_val;
  }
};

struct year {
  int y;

  constexpr explicit year(int val) : y(val) {}

  constexpr
  operator int() const
  {
    return y;
  }

  constexpr bool
  is_leap() const
  {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
  }
};

struct month {
  unsigned m;

  constexpr explicit month(unsigned val) : m(val) {}

  constexpr
  operator unsigned() const
  {
    return m;
  }

  constexpr bool
  ok() const
  {
    return m >= 1 && m <= 12;
  }
};

struct day {
  unsigned d;

  constexpr explicit day(unsigned val) : d(val) {}

  constexpr
  operator unsigned() const
  {
    return d;
  }

  constexpr bool
  ok() const
  {
    return d >= 1 && d <= 31;
  }
};

};     // namespace micron
