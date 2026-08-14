//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/sys/time.hpp"
#include "../math/ratios.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// seconds -> unit converters

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

template<typename S = base_ratio>
inline constexpr fduration_t
seconds(const fduration_t s)
{
  return (s * S::denom) / S::num;
}

template<typename S = milli>
inline constexpr fduration_t
milliseconds(const fduration_t s)
{
  return (s * S::denom) / S::num;
}

template<typename S = micro>
inline constexpr fduration_t
microseconds(const fduration_t s)
{
  return (s * S::denom) / S::num;
}

template<typename S = nano>
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
template<unit U>
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
  } else {      // unit::days
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
};      // namespace __impl

namespace chrono
{

inline constexpr u64 ns_per_us = 1'000ull;
inline constexpr u64 ns_per_ms = 1'000ull * ns_per_us;
inline constexpr u64 ns_per_s = 1'000ull * ns_per_ms;
inline constexpr u64 ns_per_min = 60ull * ns_per_s;
inline constexpr u64 ns_per_hour = 60ull * ns_per_min;
inline constexpr u64 ns_per_day = 24ull * ns_per_hour;
inline constexpr u64 ns_per_week = 7ull * ns_per_day;

inline constexpr i64 sec_per_min = 60;
inline constexpr i64 sec_per_hour = 3'600;
inline constexpr i64 sec_per_day = 86'400;
inline constexpr i64 sec_per_week = 604'800;

using ts_nsec_t = decltype(timespec_t{}.tv_nsec);
inline constexpr ts_nsec_t __ts_ns_per_sec = static_cast<ts_nsec_t>(1'000'000'000);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// timespec arithmetic

inline constexpr void
ts_normalise(timespec_t &ts) noexcept
{
  if ( ts.tv_nsec >= __ts_ns_per_sec ) {
    ts.tv_sec += static_cast<time64_t>(ts.tv_nsec / __ts_ns_per_sec);
    ts.tv_nsec %= __ts_ns_per_sec;
  } else if ( ts.tv_nsec < 0 ) {
    const ts_nsec_t borrow = (-ts.tv_nsec + (__ts_ns_per_sec - 1)) / __ts_ns_per_sec;
    ts.tv_sec -= static_cast<time64_t>(borrow);
    ts.tv_nsec += borrow * __ts_ns_per_sec;
  }
}

inline constexpr timespec_t
ts_of_ns(i64 ns) noexcept
{
  timespec_t ts{};
  ts.tv_sec = static_cast<time64_t>(ns / static_cast<i64>(ns_per_s));
  ts.tv_nsec = static_cast<ts_nsec_t>(ns % static_cast<i64>(ns_per_s));
  ts_normalise(ts);
  return ts;
}

inline constexpr timespec_t
ts_of_us(i64 us) noexcept
{
  return ts_of_ns(us * static_cast<i64>(ns_per_us));
}

inline constexpr timespec_t
ts_of_ms(i64 ms) noexcept
{
  return ts_of_ns(ms * static_cast<i64>(ns_per_ms));
}

inline constexpr i64
ns_of_ts(const timespec_t &ts) noexcept
{
  return static_cast<i64>(ts.tv_sec) * static_cast<i64>(ns_per_s) + static_cast<i64>(ts.tv_nsec);
}

inline constexpr i64
us_of_ts(const timespec_t &ts) noexcept
{
  return static_cast<i64>(ts.tv_sec) * 1'000'000 + static_cast<i64>(ts.tv_nsec) / 1'000;
}

inline constexpr i64
ms_of_ts(const timespec_t &ts) noexcept
{
  return static_cast<i64>(ts.tv_sec) * 1'000 + static_cast<i64>(ts.tv_nsec) / 1'000'000;
}

inline constexpr timespec_t
ts_add(const timespec_t &a, const timespec_t &b) noexcept
{
  timespec_t r{};
  r.tv_sec = a.tv_sec + b.tv_sec;
  r.tv_nsec = a.tv_nsec + b.tv_nsec;
  ts_normalise(r);
  return r;
}

inline constexpr timespec_t
ts_sub(const timespec_t &a, const timespec_t &b) noexcept
{
  timespec_t r{};
  r.tv_sec = a.tv_sec - b.tv_sec;
  r.tv_nsec = a.tv_nsec - b.tv_nsec;
  ts_normalise(r);
  return r;
}

// -1 / 0 / +1
inline constexpr int
ts_cmp(const timespec_t &a, const timespec_t &b) noexcept
{
  if ( a.tv_sec != b.tv_sec ) return a.tv_sec < b.tv_sec ? -1 : 1;
  if ( a.tv_nsec != b.tv_nsec ) return a.tv_nsec < b.tv_nsec ? -1 : 1;
  return 0;
}

inline constexpr bool
ts_is_zero(const timespec_t &a) noexcept
{
  return a.tv_sec == 0 && a.tv_nsec == 0;
}

namespace __impl
{
constexpr umax_t
__gcd(umax_t a, umax_t b) noexcept
{
  return b == 0 ? a : __gcd(b, a % b);
}
};      // namespace __impl

template<typename Rep, typename Period = base_ratio> struct duration {
  using rep = Rep;
  using period = Period;

  Rep __r{};

  constexpr duration() noexcept = default;

  constexpr explicit duration(const Rep &v) noexcept : __r(v) { }

  constexpr Rep
  count() const noexcept
  {
    return __r;
  }

  constexpr duration
  operator+() const noexcept
  {
    return *this;
  }

  constexpr duration
  operator-() const noexcept
  {
    return duration(static_cast<Rep>(-__r));
  }

  constexpr duration &
  operator+=(const duration &o) noexcept
  {
    __r += o.__r;
    return *this;
  }

  constexpr duration &
  operator-=(const duration &o) noexcept
  {
    __r -= o.__r;
    return *this;
  }

  template<typename S>
    requires micron::is_arithmetic_v<S>
  constexpr duration &
  operator*=(const S &v) noexcept
  {
    __r = static_cast<Rep>(__r * static_cast<Rep>(v));
    return *this;
  }

  template<typename S>
    requires micron::is_arithmetic_v<S>
  constexpr duration &
  operator/=(const S &v) noexcept
  {
    __r = static_cast<Rep>(__r / static_cast<Rep>(v));
    return *this;
  }

  template<typename S>
    requires micron::is_arithmetic_v<S>
  constexpr duration &
  operator%=(const S &v) noexcept
  {
    __r = static_cast<Rep>(__r % static_cast<Rep>(v));
    return *this;
  }

  constexpr duration &
  operator++() noexcept
  {
    ++__r;
    return *this;
  }

  constexpr duration &
  operator--() noexcept
  {
    --__r;
    return *this;
  }

  constexpr bool
  operator==(const duration &o) const noexcept
  {
    return __r == o.__r;
  }

  constexpr bool
  operator!=(const duration &o) const noexcept
  {
    return __r != o.__r;
  }

  constexpr bool
  operator<(const duration &o) const noexcept
  {
    return __r < o.__r;
  }

  constexpr bool
  operator<=(const duration &o) const noexcept
  {
    return __r <= o.__r;
  }

  constexpr bool
  operator>(const duration &o) const noexcept
  {
    return __r > o.__r;
  }

  constexpr bool
  operator>=(const duration &o) const noexcept
  {
    return __r >= o.__r;
  }

  static constexpr duration
  zero() noexcept
  {
    return duration(static_cast<Rep>(0));
  }
};

template<typename Rep, typename Period>
constexpr duration<Rep, Period>
operator+(const duration<Rep, Period> &a, const duration<Rep, Period> &b) noexcept
{
  return duration<Rep, Period>(static_cast<Rep>(a.count() + b.count()));
}

template<typename Rep, typename Period>
constexpr duration<Rep, Period>
operator-(const duration<Rep, Period> &a, const duration<Rep, Period> &b) noexcept
{
  return duration<Rep, Period>(static_cast<Rep>(a.count() - b.count()));
}

template<typename Rep, typename Period, typename S>
  requires micron::is_arithmetic_v<S>
constexpr duration<Rep, Period>
operator*(const duration<Rep, Period> &a, const S &v) noexcept
{
  return duration<Rep, Period>(static_cast<Rep>(a.count() * static_cast<Rep>(v)));
}

template<typename Rep, typename Period, typename S>
  requires micron::is_arithmetic_v<S>
constexpr duration<Rep, Period>
operator*(const S &v, const duration<Rep, Period> &a) noexcept
{
  return duration<Rep, Period>(static_cast<Rep>(a.count() * static_cast<Rep>(v)));
}

template<typename Rep, typename Period, typename S>
  requires micron::is_arithmetic_v<S>
constexpr duration<Rep, Period>
operator/(const duration<Rep, Period> &a, const S &v) noexcept
{
  return duration<Rep, Period>(static_cast<Rep>(a.count() / static_cast<Rep>(v)));
}

template<typename Rep, typename Period>
constexpr Rep
operator/(const duration<Rep, Period> &a, const duration<Rep, Period> &b) noexcept
{
  return static_cast<Rep>(a.count() / b.count());
}

template<typename Rep, typename Period>
constexpr duration<Rep, Period>
operator%(const duration<Rep, Period> &a, const duration<Rep, Period> &b) noexcept
{
  return duration<Rep, Period>(static_cast<Rep>(a.count() % b.count()));
}

template<typename To, typename Rep, typename Period>
constexpr To
duration_cast(const duration<Rep, Period> &d) noexcept
{
  using to_rep = typename To::rep;
  using to_period = typename To::period;

  constexpr umax_t __n = Period::num * to_period::denom;
  constexpr umax_t __d = Period::denom * to_period::num;
  constexpr umax_t __g = __impl::__gcd(__n, __d);
  constexpr umax_t __rn = __n / __g;
  constexpr umax_t __rd = __d / __g;

  using __wide = umax_t;
  if constexpr ( __rn == 1 && __rd == 1 )
    return To(static_cast<to_rep>(d.count()));
  else if constexpr ( __rn == 1 )
    return To(static_cast<to_rep>(d.count() / static_cast<Rep>(static_cast<__wide>(__rd))));
  else if constexpr ( __rd == 1 )
    return To(static_cast<to_rep>(d.count() * static_cast<to_rep>(static_cast<__wide>(__rn))));
  else
    return To(
        static_cast<to_rep>(d.count() * static_cast<to_rep>(static_cast<__wide>(__rn)) / static_cast<to_rep>(static_cast<__wide>(__rd))));
}

using dur_ns = duration<i64, nano>;
using dur_us = duration<i64, micro>;
using dur_ms = duration<i64, milli>;
using dur_s = duration<i64, base_ratio>;
using dur_min = duration<i64, ratio<60, 1>>;
using dur_hr = duration<i64, ratio<3'600, 1>>;
using dur_day = duration<i64, ratio<86'400, 1>>;
using dur_week = duration<i64, ratio<604'800, 1>>;

inline constexpr timespec_t
ts_of(const dur_ns &d) noexcept
{
  return ts_of_ns(d.count());
}

inline constexpr dur_ns
dur_of_ts(const timespec_t &ts) noexcept
{
  return dur_ns(ns_of_ts(ts));
}

};      // namespace chrono

};      // namespace micron
