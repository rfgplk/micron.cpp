//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../except.hpp"
#include "../memory/cmemory.hpp"

#include "calendar.hpp"
#include "units.hpp"

#if defined(MICRON_CHRONO_VDSO)
#include "vdso.hpp"
#endif

namespace micron
{

enum class system_clocks : clockid_t {
  realtime = clock_realtime,                      //  0 – wall-clock time (POSIX epoch)
  realtime_coarse = clock_realtime_coarse,        //  5 – faster, lower-resolution wall clock
  realtime_alarm = clock_realtime_alarm,          //  8 – realtime, wakes suspended system
  taitime = clock_tai,                            // 11 – International Atomic Time
  monotonic = clock_monotonic,                    //  1 – monotonic, unaffected by NTP steps
  monotonic_coarse = clock_monotonic_coarse,      //  6 – faster, lower-resolution monotonic
  monotonic_raw = clock_monotonic_raw,            //  4 – monotonic, unaffected by frequency scaling
  since_boot = clock_boottime,                    //  7 – monotonic + time spent suspended
  since_boot_alarm = clock_boottime_alarm,        //  9 – since_boot, wakes suspended system
  cputime = clock_process_cputime_id,             //  2 – per-process CPU time
  cputime_this = clock_thread_cputime_id,         //  3 – per-thread  CPU time
  __end
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// never-throwing readers

namespace chrono
{

[[gnu::always_inline]] inline ssize_t
read_clock(clockid_t clk, timespec_t &ts) noexcept
{
#if defined(MICRON_CHRONO_VDSO)
  return vdso::clock_gettime(clk, ts);
#else
  return micron::clock_gettime(clk, ts);
#endif
}

[[gnu::always_inline]] inline i64
clock_ns(clockid_t clk) noexcept
{
  timespec_t ts{};
  const ssize_t r = read_clock(clk, ts);
  if ( r < 0 ) [[unlikely]]
    return static_cast<i64>(r);
  return ns_of_ts(ts);
}

[[gnu::always_inline]] inline i64
mono_ns() noexcept
{
  return clock_ns(clock_monotonic);
}

[[gnu::always_inline]] inline i64
real_ns() noexcept
{
  return clock_ns(clock_realtime);
}

[[gnu::always_inline]] inline i64
raw_ns() noexcept
{
  return clock_ns(clock_monotonic_raw);
}

[[gnu::always_inline]] inline i64
boot_ns() noexcept
{
  return clock_ns(clock_boottime);
}

[[gnu::always_inline]] inline i64
cpu_ns() noexcept
{
  return clock_ns(clock_process_cputime_id);
}

[[gnu::always_inline]] inline i64
thread_cpu_ns() noexcept
{
  return clock_ns(clock_thread_cputime_id);
}

[[gnu::always_inline]] inline i64
now_ns() noexcept
{
  return clock_ns(clock_realtime);
}

[[gnu::always_inline]] inline i64
now_us() noexcept
{
  const i64 v = clock_ns(clock_realtime);
  return v < 0 ? v : v / static_cast<i64>(ns_per_us);
}

[[gnu::always_inline]] inline i64
now_ms() noexcept
{
  const i64 v = clock_ns(clock_realtime);
  return v < 0 ? v : v / static_cast<i64>(ns_per_ms);
}

[[gnu::always_inline]] inline i64
now_s() noexcept
{
  timespec_t ts{};
  const ssize_t r = micron::clock_gettime(clock_realtime, ts);
  if ( r < 0 ) [[unlikely]]
    return static_cast<i64>(r);
  return static_cast<i64>(ts.tv_sec);
}

inline i64
clock_resolution_ns(clockid_t clk) noexcept
{
  timespec_t res{};
  const ssize_t r = micron::clock_getres(clk, res);
  if ( r < 0 ) [[unlikely]]
    return static_cast<i64>(r);
  return ns_of_ts(res);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// deadlines

// 0 on success, -errno otherwise. loops through signals
inline i32
sleep_until(clockid_t clk, i64 deadline_ns) noexcept
{
  const timespec_t ts = ts_of_ns(deadline_ns);
  for ( ;; ) {
    const i32 r = static_cast<i32>(micron::clock_nanosleep(clk, timer_abstime, ts));
    if ( r == 0 ) return 0;
    if ( -r != static_cast<i32>(error::interrupted) ) return r;
  }
}

// as above but reports the interruption instead of swallowing it
inline i32
sleep_until_once(clockid_t clk, i64 deadline_ns) noexcept
{
  const timespec_t ts = ts_of_ns(deadline_ns);
  return static_cast<i32>(micron::clock_nanosleep(clk, timer_abstime, ts));
}

inline constexpr i64 __ns_max = 0x7FFF'FFFF'FFFF'FFFFll;

inline i32
sleep_ns(i64 ns) noexcept
{
  if ( ns <= 0 ) return 0;
  const i64 start = mono_ns();
  if ( start < 0 ) return static_cast<i32>(start);
  // NOTE: a literal max, not `~0ll >> 1` -- that is an ARITHMETIC shift of -1 and yields -1, so the
  // guard fired on every call and this returned -EINVAL without ever sleeping
  if ( start > __ns_max - ns ) return -static_cast<i32>(error::invalid_arg);
  return sleep_until(clock_monotonic, start + ns);
}

struct deadline {
  i64 at_ns{ 0 };      // an instant on clock_monotonic

  constexpr deadline() noexcept = default;

  constexpr explicit deadline(i64 abs_ns) noexcept : at_ns(abs_ns) { }

  static deadline
  in_ns(i64 ns) noexcept
  {
    const i64 n = mono_ns();
    return deadline(n < 0 ? 0 : n + ns);
  }

  static deadline
  in_ms(i64 ms) noexcept
  {
    return in_ns(ms * static_cast<i64>(ns_per_ms));
  }

  constexpr bool
  armed() const noexcept
  {
    return at_ns != 0;
  }
};

inline constexpr i64
remaining_ns(i64 deadline_ns, i64 now_ns_) noexcept
{
  return now_ns_ >= deadline_ns ? 0 : deadline_ns - now_ns_;
}

inline constexpr i32
remaining_ms(i64 deadline_ns, i64 now_ns_) noexcept
{
  const i64 left = remaining_ns(deadline_ns, now_ns_) / static_cast<i64>(ns_per_ms);
  return static_cast<i32>(left > 3'600'000ll ? 3'600'000ll : left);
}

};      // namespace chrono

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// system_clock

template<system_clocks C = system_clocks::realtime> struct system_clock {
  timespec_t time_begin;
  timespec_t time_end;

  ~system_clock() = default;

  system_clock()
  {
    micron::memset(&time_begin, 0x0, sizeof(timespec_t));
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
    // NOTE: must check < 0, since this is a raw syscall uth
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) < 0 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  system_clock(const system_clock &o) noexcept : time_begin(o.time_begin), time_end(o.time_end) { }

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

  // NOTE: clears time_end
  system_clock &
  operator=(const timespec_t &ts) noexcept
  {
    time_begin = ts;
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
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
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) < 0 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  start_get(void) -> timespec_t
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) < 0 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
    return time_begin;
  }

  inline __attribute__((always_inline)) void
  stop(void)
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_end) < 0 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
  }

  inline __attribute__((always_inline)) auto
  stop_get(void) -> timespec_t
  {
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_end) < 0 )
      exc<except::runtime_error>("micron::system_clock failed to get time");
    return time_end;
  }

  inline __attribute__((always_inline)) void
  reset(void)
  {
    micron::memset(&time_end, 0x0, sizeof(timespec_t));
    if ( micron::clock_gettime(static_cast<clockid_t>(C), time_begin) < 0 )
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
    if ( micron::clock_gettime(static_cast<clockid_t>(C), t) < 0 )
      exc<except::runtime_error>("micron::system_clock::now failed to get time");
    return static_cast<fduration_t>(t.tv_sec) * 1'000.0 + static_cast<fduration_t>(t.tv_nsec) / 1'000'000.0;
  }

  inline __attribute__((always_inline)) static auto
  now_ts(void) -> timespec_t
  {
    timespec_t t;
    if ( micron::clock_gettime(static_cast<clockid_t>(C), t) < 0 )
      exc<except::runtime_error>("micron::system_clock::now_ts failed to get time");
    return t;
  }

  inline __attribute__((always_inline)) static auto
  now_ns(void) noexcept -> i64
  {
    return chrono::clock_ns(static_cast<clockid_t>(C));
  }

  inline static auto
  resolution(void) -> timespec_t
  {
    timespec_t res;
    if ( micron::clock_getres(static_cast<clockid_t>(C), &res) < 0 ) exc<except::runtime_error>("micron::system_clock::resolution failed");
    return res;
  }

  template<unit U = unit::seconds>
  inline __attribute__((always_inline)) fduration_t
  elapsed(void)
  {
    stop();
    return read<U>();
  }

  template<unit U = unit::seconds>
  auto
  read(const timespec_t &t) -> fduration_t
  {
    time_t sec = t.tv_sec - time_begin.tv_sec;
    long nsec = t.tv_nsec - time_begin.tv_nsec;
    __impl::normalise(sec, nsec);
    return __impl::delta_to_unit<U>(sec, nsec);
  }

  template<unit U = unit::seconds>
  auto
  read(void) -> fduration_t
  {
    time_t sec = time_end.tv_sec - time_begin.tv_sec;
    long nsec = static_cast<long>(time_end.tv_nsec - time_begin.tv_nsec);
    __impl::normalise(sec, nsec);
    return __impl::delta_to_unit<U>(sec, nsec);
  }

  template<unit U = unit::seconds>
  static auto
  read(const timespec_t &a, const timespec_t &b) -> fduration_t
  {
    time_t sec = a.tv_sec - b.tv_sec;
    long nsec = static_cast<long>(a.tv_nsec - b.tv_nsec);
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

  static auto
  read_ms(const timespec_t &a, const timespec_t &b) -> fduration_t
  {
    return read<unit::milliseconds>(a, b);
  }

  auto
  read_us(void) -> fduration_t
  {
    return read<unit::microseconds>();
  }

  auto
  read_us(const timespec_t &t) -> fduration_t
  {
    return read<unit::microseconds>(t);
  }

  static auto
  read_us(const timespec_t &a, const timespec_t &b) -> fduration_t
  {
    return read<unit::microseconds>(a, b);
  }

  auto
  read_ns(void) -> fduration_t
  {
    return read<unit::nanoseconds>();
  }

  auto
  read_ns(const timespec_t &t) -> fduration_t
  {
    return read<unit::nanoseconds>(t);
  }

  static auto
  read_ns(const timespec_t &a, const timespec_t &b) -> fduration_t
  {
    return read<unit::nanoseconds>(a, b);
  }

  // deciseconds
  auto
  read_ds(void) -> fduration_t
  {
    return read<unit::seconds>() * 10.0;
  }

  auto
  read_ds(const timespec_t &t) -> fduration_t
  {
    return read<unit::seconds>(t) * 10.0;
  }

  static auto
  read_ds(const timespec_t &a, const timespec_t &b) -> fduration_t
  {
    return read<unit::seconds>(a, b) * 10.0;
  }

  auto
  delta_ns(void) const noexcept -> i64
  {
    return chrono::ns_of_ts(time_end) - chrono::ns_of_ts(time_begin);
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

// NOTE: the default system_clock<> is realtime, anything measuring an interval should use steady_clock
using steady_clock = system_clock<system_clocks::monotonic>;
using raw_clock = system_clock<system_clocks::monotonic_raw>;
using boot_clock = system_clock<system_clocks::since_boot>;
using cpu_clock = system_clock<system_clocks::cputime>;
using thread_cpu_clock = system_clock<system_clocks::cputime_this>;

// %%%%%%%%%%%%%%%%%%%%
// time_point

template<typename C = system_clock<>, typename D = fduration_t> struct time_point {
  D d;

  constexpr time_point() : d(0) { }

  constexpr explicit time_point(const D &dur) : d(dur) { }

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

  template<unit U = unit::milliseconds>
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

template<typename C, typename D, typename S, enable_if_t<is_convertible_v<S, D>, int> = 0>
constexpr time_point<C, D>
operator+(const time_point<C, D> &tp, const S &s)
{
  return time_point<C, D>(tp.time_since_epoch() + static_cast<D>(s));
}

template<typename C, typename D, typename S, enable_if_t<is_convertible_v<S, D>, int> = 0>
constexpr time_point<C, D>
operator+(const S &s, const time_point<C, D> &tp)
{
  return time_point<C, D>(tp.time_since_epoch() + static_cast<D>(s));
}

template<typename C, typename D, typename S, enable_if_t<is_convertible_v<S, D>, int> = 0>
constexpr time_point<C, D>
operator-(const time_point<C, D> &tp, const S &s)
{
  return time_point<C, D>(tp.time_since_epoch() - static_cast<D>(s));
}

template<typename C, typename D>
constexpr D
operator-(const time_point<C, D> &lhs, const time_point<C, D> &rhs)
{
  return lhs.time_since_epoch() - rhs.time_since_epoch();
}

// %%%%%%%%%%%%%%%%%%%%
// scoped timers

template<system_clocks C = system_clocks::realtime> struct auto_timer {
  system_clock<C> clk;
  fduration_t *out;

  explicit auto_timer(fduration_t *result = nullptr) : clk(), out(result) { }

  ~auto_timer()
  {
    if ( !out ) return;
    clk.stop();
    *out = clk.template read<unit::seconds>();
  }

  auto_timer(const auto_timer &) = delete;
  auto_timer &operator=(const auto_timer &) = delete;

  auto_timer(auto_timer &&o) noexcept : clk(static_cast<system_clock<C> &&>(o.clk)), out(o.out) { o.out = nullptr; }

  auto_timer &
  operator=(auto_timer &&o) noexcept
  {
    if ( this == &o ) return *this;
    clk = static_cast<system_clock<C> &&>(o.clk);
    out = o.out;
    o.out = nullptr;
    return *this;
  }
};

struct scoped_timer {
  i64 begin_ns;
  i64 *out;

  explicit scoped_timer(i64 *result = nullptr) noexcept : begin_ns(chrono::mono_ns()), out(result) { }

  ~scoped_timer()
  {
    if ( !out ) return;
    const i64 e = chrono::mono_ns();
    *out = (e < 0 || begin_ns < 0) ? -1 : e - begin_ns;
  }

  scoped_timer(const scoped_timer &) = delete;
  scoped_timer &operator=(const scoped_timer &) = delete;

  scoped_timer(scoped_timer &&o) noexcept : begin_ns(o.begin_ns), out(o.out) { o.out = nullptr; }

  scoped_timer &
  operator=(scoped_timer &&o) noexcept
  {
    if ( this == &o ) return *this;
    begin_ns = o.begin_ns;
    out = o.out;
    o.out = nullptr;
    return *this;
  }

  i64
  elapsed_ns() const noexcept
  {
    const i64 e = chrono::mono_ns();
    return (e < 0 || begin_ns < 0) ? -1 : e - begin_ns;
  }
};

inline fduration_t
now(void)
{
  timespec_t t;
  if ( micron::clock_gettime(static_cast<clockid_t>(clock_realtime), t) < 0 ) exc<except::runtime_error>("micron::now failed to get time");
  return static_cast<fduration_t>(t.tv_sec) * 1'000.0 + static_cast<fduration_t>(t.tv_nsec) / 1'000'000.0;
}

inline timespec_t
now_ts(void)
{
  timespec_t t;
  if ( micron::clock_gettime(static_cast<clockid_t>(clock_realtime), t) < 0 )
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
  if ( micron::clock_gettime(static_cast<clockid_t>(clock_realtime), t) < 0 )
    exc<except::runtime_error>("micron::time_of_day_now failed to get time");
  fduration_t secs_today = static_cast<fduration_t>(t.tv_sec % __dur_sec_per_day) + static_cast<fduration_t>(t.tv_nsec) * 1e-9;
  return time_of_day(secs_today);
}

template<unit U = unit::seconds>
inline fduration_t
elapsed(const timespec_t &begin, const timespec_t &end) noexcept
{
  time_t sec = end.tv_sec - begin.tv_sec;
  long nsec = static_cast<long>(end.tv_nsec - begin.tv_nsec);
  __impl::normalise(sec, nsec);
  return __impl::delta_to_unit<U>(sec, nsec);
}

inline fduration_t
timediff(time_t t0, time_t t1) noexcept
{
  return static_cast<fduration_t>(t1) - static_cast<fduration_t>(t0);
}

using chrono::mono_ns;
using chrono::now_ns;
using chrono::now_us;
using chrono::real_ns;
using chrono::remaining_ms;
using chrono::remaining_ns;
using chrono::sleep_until;

};      // namespace micron
