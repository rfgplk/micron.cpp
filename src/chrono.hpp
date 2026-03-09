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

inline constexpr i32 __dur_ns_per_sec = 1'000'000'000;
inline constexpr i32 __dur_us_per_sec = 1'000'000;
inline constexpr i32 __dur_ms_per_sec = 1'000;
inline constexpr i32 __dur_sec_per_min = 60;
inline constexpr i32 __dur_sec_per_hr = 3'600;
inline constexpr i32 __dur_sec_per_day = 86'400;

inline constexpr fduration_t duration_ratio = 1000;

inline constexpr fduration_t
days(const fduration_t s)
{
  return s / static_cast<fduration_t>(__dur_sec_per_day);
}

inline constexpr fduration_t
hours(const fduration_t s)
{
  return s / static_cast<fduration_t>(__dur_sec_per_hr);
}

inline constexpr fduration_t
minutes(const fduration_t s)
{
  return s / static_cast<fduration_t>(__dur_sec_per_min);
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

enum class unit : i32 {
  days = __dur_sec_per_day,
  hours = __dur_sec_per_hr,
  minutes = __dur_sec_per_min,
  seconds = 1,
  milliseconds = __dur_ms_per_sec,
  microseconds = __dur_us_per_sec,
  nanoseconds = __dur_ns_per_sec,
};

namespace __impl
{
template <unit U>
inline constexpr fduration_t
delta_to_unit(time_t sec, long nsec) noexcept
{
  if constexpr ( U == unit::nanoseconds ) {
    return static_cast<fduration_t>(sec) * 1e9 + static_cast<fduration_t>(nsec);
  } else if constexpr ( U == unit::microseconds ) {
    return static_cast<fduration_t>(sec) * 1e6 + static_cast<fduration_t>(nsec) * 1e-3;
  } else if constexpr ( U == unit::milliseconds ) {
    return static_cast<fduration_t>(sec) * 1e3 + static_cast<fduration_t>(nsec) * 1e-6;
  } else if constexpr ( U == unit::seconds ) {
    return static_cast<fduration_t>(sec) + static_cast<fduration_t>(nsec) * 1e-9;
  } else if constexpr ( U == unit::minutes ) {
    return (static_cast<fduration_t>(sec) + static_cast<fduration_t>(nsec) * 1e-9) / static_cast<fduration_t>(__dur_sec_per_min);
  } else if constexpr ( U == unit::hours ) {
    return (static_cast<fduration_t>(sec) + static_cast<fduration_t>(nsec) * 1e-9) / static_cast<fduration_t>(__dur_sec_per_hr);
  } else {     // unit::days
    return (static_cast<fduration_t>(sec) + static_cast<fduration_t>(nsec) * 1e-9) / static_cast<fduration_t>(__dur_sec_per_day);
  }
}

inline constexpr void
normalise(time_t &sec, long &nsec) noexcept
{
  if ( nsec < 0 ) {
    --sec;
    nsec += 1'000'000'000L;
  }
}
};     // namespace __impl

enum class system_clocks : clockid_t {
  realtime = clock_realtime,                     //  0 – wall-clock time (POSIX epoch)
  realtime_coarse = clock_realtime_coarse,       //  5 – faster, lower-resolution wall clock
  realtime_alarm = clock_realtime_alarm,         //  8 – realtime, wakes suspended system
  taitime = clock_tai,                           // 11 – International Atomic Time
  monotonic = clock_monotonic,                   //  1 – monotonic, unaffected by NTP steps
  monotonic_coarse = clock_monotonic_coarse,     //  6 – faster, lower-resolution monotonic
  monotonic_raw = clock_monotonic_raw,           //  4 – monotonic, unaffected by frequency scaling
  since_boot = clock_boottime,                   //  7 – monotonic + time spent suspended
  since_boot_alarm = clock_boottime_alarm,       //  9 – since_boot, wakes suspended system
  cputime = clock_process_cputime_id,            //  2 – per-process CPU time
  cputime_this = clock_thread_cputime_id,        //  3 – per-thread  CPU time
  __end
};

template <system_clocks C = system_clocks::realtime> struct system_clock {
  timespec_t time_begin;
  timespec_t time_end;

  ~system_clock() = default;

  system_clock()
  {
    micron::memset(&time_begin, 0x0, sizeof(timespec_t));
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  system_clock(const system_clock &o) noexcept : time_begin(o.time_begin), time_end(o.time_end) {}

  system_clock(system_clock &&o) noexcept : time_begin(o.time_begin), time_end(o.time_end)
  {
    micron::memset(&o.time_begin, 0x0, sizeof(timespec_t));
    micron::memset(&o.time_end, 0x0, sizeof(timespec_t));
  }

  system_clock &
  operator=(const system_clock &o) noexcept
  {
    time_begin = o.time_begin;
    time_end = o.time_end;
    return *this;
  }

  system_clock &
  operator=(system_clock &&o) noexcept
  {
    time_begin = o.time_begin;
    time_end = o.time_end;
    micron::memset(&o.time_begin, 0x0, sizeof(timespec_t));
    micron::memset(&o.time_end, 0x0, sizeof(timespec_t));
    return *this;
  }

  system_clock &
  operator=(const timespec_t &ts) noexcept
  {
    time_begin = ts;
    return *this;
  }

  explicit
  operator duration_t() const noexcept
  {
    time_t sec = time_end.tv_sec - time_begin.tv_sec;
    long nsec = time_end.tv_nsec - time_begin.tv_nsec;
    __impl::normalise(sec, nsec);
    return static_cast<duration_t>(sec);
  }

  inline __attribute__((always_inline)) void
  start(void)
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  start_get(void) -> timespec_t
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
    return time_begin;
  }

  inline __attribute__((always_inline)) void
  stop(void)
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_end) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  stop_get(void) -> timespec_t
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_end) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
    return time_end;
  }

  inline __attribute__((always_inline)) void
  reset(void)
  {
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) == -1 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) fduration_t
  lap(void)
  {
    stop();
    fduration_t t = read();
    time_begin = time_end;
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
    return t;
  }

  inline __attribute__((always_inline)) static auto
  now(void) -> fduration_t
  {
    timespec_t t;
    if ( micron::clock_gettime(static_cast<clockid_t>(C), t) == -1 )
      exc<except::runtime_error>("micron::system_clock::now failed to get time");
    return static_cast<fduration_t>(t.tv_sec) * 1'000.0 + static_cast<fduration_t>(t.tv_nsec) / 1'000'000.0;
  }

  inline __attribute__((always_inline)) static auto
  now_ts(void) -> timespec_t
  {
    timespec_t t;
    if ( micron::clock_gettime(static_cast<clockid_t>(C), t) == -1 )
      exc<except::runtime_error>("micron::system_clock::now_ts failed to get time");
    return t;
  }

  inline static auto
  resolution(void) -> timespec_t
  {
    timespec_t res;
    if ( micron::clock_getres(static_cast<clockid_t>(C), &res) == -1 )
      exc<except::runtime_error>("micron::system_clock::resolution failed");
    return res;
  }

  template <unit U = unit::seconds>
  inline __attribute__((always_inline)) fduration_t
  elapsed(void)
  {
    stop();
    return read<U>();
  }

  template <unit U = unit::seconds>
  auto
  read(const timespec_t &t) -> fduration_t
  {
    time_t sec = t.tv_sec - time_begin.tv_sec;
    long nsec = t.tv_nsec - time_begin.tv_nsec;
    __impl::normalise(sec, nsec);
    return __impl::delta_to_unit<U>(sec, nsec);
  }

  template <unit U = unit::seconds>
  auto
  read(void) -> fduration_t
  {
    time_t sec = time_end.tv_sec - time_begin.tv_sec;
    long nsec = time_end.tv_nsec - time_begin.tv_nsec;
    __impl::normalise(sec, nsec);
    return __impl::delta_to_unit<U>(sec, nsec);
  }

  auto
  read_ms(const timespec_t &t) -> fduration_t
  {
    return read<unit::milliseconds>(t);
  }

  auto
  read_ms(void) -> fduration_t
  {
    return read<unit::milliseconds>();
  }

  inline auto
  begin_point(void) const -> timespec_t
  {
    return time_begin;
  }

  inline auto
  end_point(void) const -> timespec_t
  {
    return time_end;
  }

  inline bool
  stopped(void) const noexcept
  {
    return time_end.tv_sec != 0 || time_end.tv_nsec != 0;
  }
};

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

  constexpr bool
  operator==(const time_point &o) const noexcept
  {
    return d == o.d;
  }

  constexpr bool
  operator!=(const time_point &o) const noexcept
  {
    return d != o.d;
  }

  constexpr bool
  operator<(const time_point &o) const noexcept
  {
    return d < o.d;
  }

  constexpr bool
  operator<=(const time_point &o) const noexcept
  {
    return d <= o.d;
  }

  constexpr bool
  operator>(const time_point &o) const noexcept
  {
    return d > o.d;
  }

  constexpr bool
  operator>=(const time_point &o) const noexcept
  {
    return d >= o.d;
  }

  template <unit U = unit::milliseconds>
  constexpr D
  as() const noexcept
  {
    fduration_t sec = static_cast<fduration_t>(d) / 1'000.0;
    if constexpr ( U == unit::milliseconds )
      return static_cast<D>(d);
    else if constexpr ( U == unit::seconds )
      return static_cast<D>(sec);
    else if constexpr ( U == unit::microseconds )
      return static_cast<D>(sec * 1e6);
    else if constexpr ( U == unit::nanoseconds )
      return static_cast<D>(sec * 1e9);
    else if constexpr ( U == unit::minutes )
      return static_cast<D>(sec / __dur_sec_per_min);
    else if constexpr ( U == unit::hours )
      return static_cast<D>(sec / __dur_sec_per_hr);
    else
      return static_cast<D>(sec / __dur_sec_per_day);
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
  is_leap() const noexcept
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
  ok() const noexcept
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
  ok() const noexcept
  {
    return d >= 1 && d <= 31;
  }
};

struct year_month_day {
  micron::year yr;
  micron::month mo;
  micron::day dy;

  constexpr year_month_day(micron::year y, micron::month m, micron::day d) : yr(y), mo(m), dy(d) {}

  static constexpr year_month_day
  from_unix(time_t unix_sec) noexcept
  {
    long z = static_cast<long>(unix_sec / 86400) + 719468L;
    long era = (z >= 0 ? z : z - 146096L) / 146097L;
    long doe = z - era * 146097L;
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long y = yoe + era * 400;
    long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long mp = (5 * doy + 2) / 153;
    unsigned d_ = static_cast<unsigned>(doy - (153 * mp + 2) / 5 + 1);
    unsigned m_ = static_cast<unsigned>(mp < 10 ? mp + 3 : mp - 9);
    y += (m_ <= 2 ? 1 : 0);
    return year_month_day{ micron::year(static_cast<int>(y)), micron::month(m_), micron::day(d_) };
  }

  constexpr time_t
  to_unix() const noexcept
  {
    int y = static_cast<int>(yr) - (static_cast<unsigned>(mo) <= 2 ? 1 : 0);
    unsigned m = static_cast<unsigned>(mo) + (static_cast<unsigned>(mo) <= 2 ? 9 : -3);
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = static_cast<unsigned>(y - era * 400);
    unsigned doy = (153 * m + 2) / 5 + static_cast<unsigned>(dy) - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<time_t>((era * 146097L + static_cast<long>(doe) - 719468L) * 86400L);
  }

  constexpr bool
  ok() const noexcept
  {
    return mo.ok() && dy.ok();
  }
};

template <system_clocks C = system_clocks::realtime> struct auto_timer {
  system_clock<C> clk;
  fduration_t *out;

  explicit auto_timer(fduration_t *result = nullptr) : clk(), out(result) {}

  ~auto_timer()
  {
    clk.stop();
    if ( out )
      *out = clk.template read<unit::seconds>();
  }

  auto_timer(const auto_timer &) = delete;
  auto_timer &operator=(const auto_timer &) = delete;
  auto_timer(auto_timer &&) = default;
  auto_timer &operator=(auto_timer &&) = default;
};

inline fduration_t
now(void)
{
  timespec_t t;
  if ( micron::clock_gettime(static_cast<clockid_t>(clock_realtime), t) == -1 )
    exc<except::runtime_error>("micron::now failed to get time");
  return static_cast<fduration_t>(t.tv_sec) * 1'000.0 + static_cast<fduration_t>(t.tv_nsec) / 1'000'000.0;
}

inline timespec_t
now_ts(void)
{
  timespec_t t;
  if ( micron::clock_gettime(static_cast<clockid_t>(clock_realtime), t) == -1 )
    exc<except::runtime_error>("micron::now_ts failed to get time");
  return t;
}

inline time_t
unix_time(void)
{
  return micron::time();
}

inline year_month_day
today(void)
{
  return year_month_day::from_unix(micron::time());
}

inline time_of_day
time_of_day_now(void)
{
  timespec_t t;
  if ( micron::clock_gettime(static_cast<clockid_t>(clock_realtime), t) == -1 )
    exc<except::runtime_error>("micron::time_of_day_now failed to get time");
  fduration_t secs_today = static_cast<fduration_t>(t.tv_sec % __dur_sec_per_day) + static_cast<fduration_t>(t.tv_nsec) * 1e-9;
  return time_of_day(secs_today);
}

template <unit U = unit::seconds>
inline fduration_t
elapsed(const timespec_t &begin, const timespec_t &end) noexcept
{
  time_t sec = end.tv_sec - begin.tv_sec;
  long nsec = end.tv_nsec - begin.tv_nsec;
  __impl::normalise(sec, nsec);
  return __impl::delta_to_unit<U>(sec, nsec);
}

inline fduration_t
timediff(time_t t0, time_t t1) noexcept
{
  return static_cast<fduration_t>(t1) - static_cast<fduration_t>(t0);
}

}     // namespace micron
