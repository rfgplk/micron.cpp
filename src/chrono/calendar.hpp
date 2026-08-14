//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "units.hpp"

namespace micron
{
namespace chrono
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// floor division for calendar units

inline constexpr i64
floor_div(i64 a, i64 b) noexcept
{
  i64 q = a / b;
  if ( (a % b != 0) && ((a < 0) != (b < 0)) ) --q;
  return q;
}

inline constexpr i64
floor_mod(i64 a, i64 b) noexcept
{
  return a - floor_div(a, b) * b;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// proleptic gregorian calendar
//
// howard hinnant's civil_from_days / days_from_civil

struct civil_date {
  i32 y;
  u32 m;
  u32 d;
};

inline constexpr bool
is_leap(i32 y) noexcept
{
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

inline constexpr u32
days_in_month(i32 y, u32 m) noexcept
{
  constexpr u32 __dm[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if ( m == 2 ) return is_leap(y) ? 29u : 28u;
  return m >= 1 && m <= 12 ? __dm[m] : 0u;
}

inline constexpr u32
days_in_year(i32 y) noexcept
{
  return is_leap(y) ? 366u : 365u;
}

// days since 1970-01-01, negative before it
inline constexpr i64
days_from_civil(i32 y_, u32 m_, u32 d_) noexcept
{
  i64 y = static_cast<i64>(y_);
  const i64 m = static_cast<i64>(m_);
  const i64 d = static_cast<i64>(d_);
  y -= m <= 2;
  const i64 era = floor_div(y, 400);
  const i64 yoe = y - era * 400;                                       // [0, 399]
  const i64 doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;      // [0, 365]
  const i64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;               // [0, 146096]
  return era * 146097 + doe - 719468;
}

inline constexpr civil_date
civil_from_days(i64 z) noexcept
{
  z += 719468;
  const i64 era = floor_div(z, 146097);
  const i64 doe = z - era * 146097;                                           // [0, 146096]
  const i64 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;      // [0, 399]
  const i64 y = yoe + era * 400;
  const i64 doy = doe - (365 * yoe + yoe / 4 - yoe / 100);           // [0, 365]
  const i64 mp = (5 * doy + 2) / 153;                                // [0, 11]
  const u32 d = static_cast<u32>(doy - (153 * mp + 2) / 5 + 1);      // [1, 31]
  const u32 m = static_cast<u32>(mp < 10 ? mp + 3 : mp - 9);         // [1, 12]
  return civil_date{ static_cast<i32>(y + (m <= 2)), m, d };
}

inline constexpr bool
civil_valid(i32 y, u32 m, u32 d) noexcept
{
  return m >= 1 && m <= 12 && d >= 1 && d <= days_in_month(y, m);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// weekday / ordinal / iso week date

// 0 = Sunday .. 6 = Saturday. 1970-01-01 was a Thursday
inline constexpr u32
weekday_from_days(i64 z) noexcept
{
  return static_cast<u32>(floor_mod(z + 4, 7));
}

inline constexpr u32
weekday_of(i32 y, u32 m, u32 d) noexcept
{
  return weekday_from_days(days_from_civil(y, m, d));
}

// 1 = Monday .. 7 = Sunday, the ISO-8601 numbering
inline constexpr u32
iso_weekday_of(i32 y, u32 m, u32 d) noexcept
{
  const u32 w = weekday_of(y, m, d);
  return w == 0 ? 7u : w;
}

// 1 .. 365/366
inline constexpr u32
day_of_year(i32 y, u32 m, u32 d) noexcept
{
  return static_cast<u32>(days_from_civil(y, m, d) - days_from_civil(y, 1, 1) + 1);
}

struct iso_week {
  i32 year;         // ISO year
  u32 week;         // 1 .. 53
  u32 weekday;      // 1 = Monday .. 7 = Sunday
};

// NOTE: the ISO year of a date is the calendar year of the thursday in its week
inline constexpr iso_week
iso_week_date(i32 y, u32 m, u32 d) noexcept
{
  const i64 z = days_from_civil(y, m, d);
  const u32 wd = static_cast<u32>(floor_mod(z + 3, 7)) + 1u;      // 1=Mon..7=Sun without a second lookup
  const i64 thursday = z + (4 - static_cast<i64>(wd));
  const civil_date tc = civil_from_days(thursday);
  const i64 jan1 = days_from_civil(tc.y, 1, 1);
  return iso_week{ tc.y, static_cast<u32>((thursday - jan1) / 7 + 1), wd };
}

inline constexpr const char *month_abbr[13] = { "???", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

inline constexpr const char *month_name[13]
    = { "???", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

// indexed 0 = Sunday, matching weekday_from_days
inline constexpr const char *weekday_abbr[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

inline constexpr const char *weekday_name[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

};      // namespace chrono

struct time_of_day {
  fduration_t hours_val;
  fduration_t minutes_val;
  fduration_t seconds_val;
  fduration_t subseconds_val;

  constexpr time_of_day() : hours_val(0), minutes_val(0), seconds_val(0), subseconds_val(0) { }

  constexpr explicit time_of_day(fduration_t dur_since_midnight)
  {
    duration_t whole = static_cast<duration_t>(dur_since_midnight);
    fduration_t frac = dur_since_midnight - static_cast<fduration_t>(whole);
    if ( frac < 0 ) {
      --whole;
      frac += 1;
    }
    duration_t s = whole % static_cast<duration_t>(__dur_sec_per_day);
    if ( s < 0 ) s += static_cast<duration_t>(__dur_sec_per_day);
    hours_val = static_cast<fduration_t>(s / 3600);
    minutes_val = static_cast<fduration_t>((s / 60) % 60);
    seconds_val = static_cast<fduration_t>(s % 60);
    subseconds_val = frac;
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

  constexpr explicit year(int val) : y(val) { }

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

  constexpr explicit month(unsigned val) : m(val) { }

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

  constexpr explicit day(unsigned val) : d(val) { }

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

  constexpr year_month_day(micron::year y, micron::month m, micron::day d) : yr(y), mo(m), dy(d) { }

  // NOTE: floor division; C truncates toward zero
  static constexpr year_month_day
  from_unix(time_t unix_sec) noexcept
  {
    const chrono::civil_date c = chrono::civil_from_days(chrono::floor_div(static_cast<i64>(unix_sec), 86400));
    return year_month_day{ micron::year(c.y), micron::month(c.m), micron::day(c.d) };
  }

  static constexpr year_month_day
  from_days(i64 days) noexcept
  {
    const chrono::civil_date c = chrono::civil_from_days(days);
    return year_month_day{ micron::year(c.y), micron::month(c.m), micron::day(c.d) };
  }

  constexpr i64
  to_days() const noexcept
  {
    return chrono::days_from_civil(static_cast<i32>(yr), static_cast<unsigned>(mo), static_cast<unsigned>(dy));
  }

  constexpr time_t
  to_unix() const noexcept
  {
    return static_cast<time_t>(to_days() * 86400LL);
  }

  constexpr bool
  ok() const noexcept
  {
    return chrono::civil_valid(static_cast<i32>(yr), static_cast<unsigned>(mo), static_cast<unsigned>(dy));
  }

  constexpr unsigned
  weekday() const noexcept
  {
    return chrono::weekday_from_days(to_days());
  }

  constexpr unsigned
  day_of_year() const noexcept
  {
    return chrono::day_of_year(static_cast<i32>(yr), static_cast<unsigned>(mo), static_cast<unsigned>(dy));
  }
};

};      // namespace micron
