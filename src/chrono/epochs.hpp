//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "calendar.hpp"
#include "civil.hpp"
#include "units.hpp"

namespace micron
{
namespace chrono
{
namespace epoch
{

// seconds from each epoch's zero to 1970-01-01T00:00:00Z
inline constexpr i64 ntp_to_unix = 2'208'988'800ll;            // 1900-01-01
inline constexpr i64 filetime_to_unix = 11'644'473'600ll;      // 1601-01-01
inline constexpr i64 gps_to_unix = 315'964'800ll;              // 1980-01-06
inline constexpr i64 dos_to_unix = 315'532'800ll;              // 1980-01-01
inline constexpr i64 jdn_at_unix = 2'440'588ll;                // julian day number of 1970-01-01
inline constexpr i64 mjd_at_unix = 40'587ll;                   // modified julian day of 1970-01-01
inline constexpr i64 sec_per_gps_week = 604'800ll;

inline constexpr i32 tai_minus_utc_default = 37;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// NTP (RFC 5905)

inline constexpr i64
ntp_from_unix(i64 unix_sec) noexcept
{
  return unix_sec + ntp_to_unix;
}

inline constexpr i64
unix_from_ntp(i64 ntp_sec) noexcept
{
  return ntp_sec - ntp_to_unix;
}

inline constexpr u64
ntp_timestamp(i64 unix_sec, u32 nsec) noexcept
{
  const u64 s = static_cast<u64>(ntp_from_unix(unix_sec)) & 0xFFFF'FFFFull;
  // fraction = nsec * 2^32 / 1e9, computed exactly in 64 bits
  const u64 frac = (static_cast<u64>(nsec) << 32) / 1'000'000'000ull;
  return (s << 32) | frac;
}

inline constexpr i64
unix_sec_from_ntp_timestamp(u64 ts, i64 era = 0) noexcept
{
  return unix_from_ntp(static_cast<i64>(ts >> 32) + era * (1ll << 32));
}

inline constexpr u32
nsec_from_ntp_timestamp(u64 ts) noexcept
{
  return static_cast<u32>(((ts & 0xFFFF'FFFFull) * 1'000'000'000ull) >> 32);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// windows FILETIME
// 100-nanosecond ticks since 1601-01-01

inline constexpr i64
filetime_from_unix(i64 unix_sec, u32 nsec = 0) noexcept
{
  return (unix_sec + filetime_to_unix) * 10'000'000ll + static_cast<i64>(nsec / 100u);
}

inline constexpr i64
unix_from_filetime(i64 ft) noexcept
{
  return floor_div(ft, 10'000'000ll) - filetime_to_unix;
}

inline constexpr u32
nsec_from_filetime(i64 ft) noexcept
{
  return static_cast<u32>(floor_mod(ft, 10'000'000ll) * 100ll);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// GPS
//
// GPS time has no leap seconds at all
// GPS-UTC = (TAI-UTC) - 19

struct gps_time {
  i64 week;
  i64 tow;      // time of week, seconds
};

inline constexpr i64
gps_from_unix(i64 unix_sec, i32 tai_utc = tai_minus_utc_default) noexcept
{
  return unix_sec - gps_to_unix + static_cast<i64>(tai_utc - 19);
}

inline constexpr i64
unix_from_gps(i64 gps_sec, i32 tai_utc = tai_minus_utc_default) noexcept
{
  return gps_sec + gps_to_unix - static_cast<i64>(tai_utc - 19);
}

inline constexpr gps_time
gps_week_tow(i64 unix_sec, i32 tai_utc = tai_minus_utc_default) noexcept
{
  const i64 g = gps_from_unix(unix_sec, tai_utc);
  return gps_time{ floor_div(g, sec_per_gps_week), floor_mod(g, sec_per_gps_week) };
}

inline constexpr i64
unix_from_gps_week_tow(i64 week, i64 tow, i32 tai_utc = tai_minus_utc_default) noexcept
{
  return unix_from_gps(week * sec_per_gps_week + tow, tai_utc);
}

// %%%%%%%%%%%%%
// TAI

inline constexpr i64
tai_from_unix(i64 unix_sec, i32 tai_utc = tai_minus_utc_default) noexcept
{
  return unix_sec + static_cast<i64>(tai_utc);
}

inline constexpr i64
unix_from_tai(i64 tai_sec, i32 tai_utc = tai_minus_utc_default) noexcept
{
  return tai_sec - static_cast<i64>(tai_utc);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// julian day

inline constexpr i64
jdn(i64 unix_sec) noexcept
{
  const i64 days = floor_div(unix_sec, 86400);
  const i64 secs = unix_sec - days * 86400;
  return days + jdn_at_unix + (secs >= 43200 ? 1 : 0);
}

inline constexpr i64
mjd(i64 unix_sec) noexcept
{
  return floor_div(unix_sec, 86400) + mjd_at_unix;
}

inline constexpr i64
unix_from_mjd(i64 m) noexcept
{
  return (m - mjd_at_unix) * 86400ll;
}

// julian date scaled by 1e6
inline constexpr i64
julian_date_x1e6(i64 unix_sec) noexcept
{
  // JD = unix/86400 + 2440587.5
  return (jdn_at_unix * 1'000'000ll) - 500'000ll + (unix_sec * 1'000'000ll) / 86400ll;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// MS-DOS / FAT packed date-time

struct dos_datetime {
  u16 date;
  u16 time;
};

inline constexpr dos_datetime
dos_from_civil(const civil &c) noexcept
{
  if ( c.y < 1980 || c.y > 2107 ) return dos_datetime{ 0, 0 };
  const u16 dt = static_cast<u16>(((static_cast<u32>(c.y - 1980) & 0x7Fu) << 9) | ((c.mo & 0x0Fu) << 5) | (c.d & 0x1Fu));
  const u16 tm = static_cast<u16>(((c.h & 0x1Fu) << 11) | ((c.mi & 0x3Fu) << 5) | ((c.s / 2u) & 0x1Fu));
  return dos_datetime{ dt, tm };
}

inline constexpr civil
civil_from_dos(dos_datetime v) noexcept
{
  civil c{};
  c.y = static_cast<i32>((v.date >> 9) & 0x7Fu) + 1980;
  c.mo = static_cast<u32>((v.date >> 5) & 0x0Fu);
  c.d = static_cast<u32>(v.date & 0x1Fu);
  c.h = static_cast<u32>((v.time >> 11) & 0x1Fu);
  c.mi = static_cast<u32>((v.time >> 5) & 0x3Fu);
  c.s = static_cast<u32>(v.time & 0x1Fu) * 2u;
  return c;
}

};      // namespace epoch
};      // namespace chrono
};      // namespace micron
