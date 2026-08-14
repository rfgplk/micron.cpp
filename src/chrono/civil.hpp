//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "calendar.hpp"
#include "units.hpp"

namespace micron
{
namespace chrono
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// local time

struct civil {
  i32 y = 1970;
  u32 mo = 1;
  u32 d = 1;
  u32 h = 0;
  u32 mi = 0;
  u32 s = 0;
  u32 ns = 0;
  i32 off = 0;

  constexpr bool
  valid() const noexcept
  {
    return civil_valid(y, mo, d) && h < 24 && mi < 60 && s < 61 && ns < 1'000'000'000u;
  }

  constexpr bool
  operator==(const civil &o) const noexcept
  {
    return y == o.y && mo == o.mo && d == o.d && h == o.h && mi == o.mi && s == o.s && ns == o.ns && off == o.off;
  }

  constexpr bool
  operator!=(const civil &o) const noexcept
  {
    return !(*this == o);
  }
};

struct tz_table {
  const i64 *trans = nullptr;
  const i32 *off = nullptr;
  usize n = 0;
  i32 first = 0;
  bool loaded = false;

  constexpr tz_table() noexcept = default;

  constexpr tz_table(const i64 *t, const i32 *o, usize count, i32 before) noexcept : trans(t), off(o), n(count), first(before), loaded(true)
  {
  }
};

inline constexpr tz_table
tz_fixed(i32 offset_sec) noexcept
{
  tz_table t{};
  t.first = offset_sec;
  t.loaded = true;
  return t;
}

inline constexpr tz_table tz_utc = tz_fixed(0);

inline constexpr i32
offset_at(const tz_table &t, i64 utc) noexcept
{
  if ( !t.loaded || t.n == 0 || t.trans == nullptr ) return t.loaded ? t.first : 0;
  if ( utc < t.trans[0] ) return t.first;
  usize lo = 0, hi = t.n - 1;
  while ( lo < hi ) {
    const usize mid = lo + (hi - lo + 1) / 2;
    if ( t.trans[mid] <= utc )
      lo = mid;
    else
      hi = mid - 1;
  }
  return t.off[lo];
}

inline constexpr civil
to_civil(i64 utc_sec, i32 off) noexcept
{
  const i64 local = utc_sec + static_cast<i64>(off);
  const i64 days = floor_div(local, 86400);
  const i64 rem = local - days * 86400;
  const civil_date cd = civil_from_days(days);
  civil c{};
  c.y = cd.y;
  c.mo = cd.m;
  c.d = cd.d;
  c.h = static_cast<u32>(rem / 3600);
  c.mi = static_cast<u32>((rem % 3600) / 60);
  c.s = static_cast<u32>(rem % 60);
  c.ns = 0;
  c.off = off;
  return c;
}

inline constexpr civil
to_civil(i64 utc_sec, i32 off, u32 nsec) noexcept
{
  civil c = to_civil(utc_sec, off);
  c.ns = nsec;
  return c;
}

inline constexpr civil
to_civil(const tz_table &t, i64 utc_sec) noexcept
{
  return to_civil(utc_sec, offset_at(t, utc_sec));
}

inline constexpr civil
civil_utc(i64 utc_sec) noexcept
{
  return to_civil(utc_sec, 0);
}

inline constexpr civil
to_civil(const timespec_t &ts, i32 off) noexcept
{
  return to_civil(static_cast<i64>(ts.tv_sec), off, static_cast<u32>(ts.tv_nsec));
}

inline constexpr i64
civil_secs(const civil &c) noexcept
{
  return days_from_civil(c.y, c.mo, c.d) * 86400ll + static_cast<i64>(c.h) * 3600 + static_cast<i64>(c.mi) * 60 + static_cast<i64>(c.s);
}

inline constexpr i64
from_civil(const civil &c, const tz_table &t) noexcept
{
  const i64 local = civil_secs(c);
  if ( !t.loaded || t.n == 0 || t.trans == nullptr ) return local - static_cast<i64>(t.loaded ? t.first : 0);

  const i32 a = offset_at(t, local - 86400);
  const i32 b = offset_at(t, local + 86400);

  const i64 ca = local - static_cast<i64>(a);
  const i64 cb = local - static_cast<i64>(b);

  const bool va = offset_at(t, ca) == a;
  const bool vb = offset_at(t, cb) == b;

  if ( va && vb ) return ca < cb ? ca : cb;
  if ( va ) return ca;
  if ( vb ) return cb;

  return ca > cb ? ca : cb;
}

inline constexpr i32
offset_hours(i32 off_sec) noexcept
{
  return off_sec / 3600;
}

inline constexpr i32
offset_minutes(i32 off_sec) noexcept
{

  const i64 mag = off_sec < 0 ? -static_cast<i64>(off_sec) : static_cast<i64>(off_sec);
  return static_cast<i32>((mag % 3600) / 60);
}

inline constexpr bool
offset_negative(i32 off_sec) noexcept
{
  return off_sec < 0;
}

inline constexpr u32
weekday_of(const civil &c) noexcept
{
  return weekday_of(c.y, c.mo, c.d);
}

inline constexpr u32
day_of_year(const civil &c) noexcept
{
  return day_of_year(c.y, c.mo, c.d);
}

inline constexpr iso_week
iso_week_date(const civil &c) noexcept
{
  return iso_week_date(c.y, c.mo, c.d);
}

};      // namespace chrono
};      // namespace micron
