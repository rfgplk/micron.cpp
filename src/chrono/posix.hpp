//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../linux/elf/auxval.hpp"
#include "../linux/sys/time.hpp"

#include "calendar.hpp"
#include "civil.hpp"
#include "format.hpp"
#include "parse.hpp"
#include "units.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// libc compat time layer

namespace micron
{
namespace posix
{

struct tm_t {
  int tm_sec = 0;
  int tm_min = 0;
  int tm_hour = 0;
  int tm_mday = 1;
  int tm_mon = 0;
  int tm_year = 70;
  int tm_wday = 0;
  int tm_yday = 0;
  int tm_isdst = -1;
  long tm_gmtoff = 0;
  const char *tm_zone = nullptr;
};

// %%%%%%%%%%%%%%%%%%%%%%
// tm <-> civil

inline constexpr chrono::civil
civil_of_tm(const tm_t &t) noexcept
{
  chrono::civil c{};
  c.y = t.tm_year + 1900;
  c.mo = static_cast<u32>(t.tm_mon + 1);
  c.d = static_cast<u32>(t.tm_mday);
  c.h = static_cast<u32>(t.tm_hour);
  c.mi = static_cast<u32>(t.tm_min);
  c.s = static_cast<u32>(t.tm_sec);
  c.off = static_cast<i32>(t.tm_gmtoff);
  return c;
}

inline constexpr void
tm_of_civil(const chrono::civil &c, tm_t &out) noexcept
{
  out.tm_sec = static_cast<int>(c.s);
  out.tm_min = static_cast<int>(c.mi);
  out.tm_hour = static_cast<int>(c.h);
  out.tm_mday = static_cast<int>(c.d);
  out.tm_mon = static_cast<int>(c.mo) - 1;
  out.tm_year = c.y - 1900;
  out.tm_wday = static_cast<int>(chrono::weekday_of(c.y, c.mo, c.d));
  out.tm_yday = static_cast<int>(chrono::day_of_year(c.y, c.mo, c.d)) - 1;
  out.tm_gmtoff = c.off;
}

// %%%%%%%%%%%%%%%%%%
// broken-down time

inline tm_t *
gmtime_r(const time_t *t, tm_t *out) noexcept
{
  if ( !t || !out ) return nullptr;
  tm_of_civil(chrono::civil_utc(static_cast<i64>(*t)), *out);
  out->tm_isdst = 0;
  out->tm_gmtoff = 0;
  out->tm_zone = "UTC";
  return out;
}

inline tm_t *
localtime_r(const time_t *t, tm_t *out, const chrono::tz_table &z) noexcept
{
  if ( !t || !out ) return nullptr;
  const i32 off = chrono::offset_at(z, static_cast<i64>(*t));
  tm_of_civil(chrono::to_civil(static_cast<i64>(*t), off), *out);
  out->tm_isdst = -1;
  out->tm_gmtoff = off;
  out->tm_zone = nullptr;
  return out;
}

inline time_t
timegm(tm_t *t) noexcept
{
  if ( !t ) return static_cast<time_t>(-1);
  const chrono::civil in = civil_of_tm(*t);
  const i64 secs = chrono::civil_secs(in);
  tm_of_civil(chrono::civil_utc(secs), *t);
  t->tm_isdst = 0;
  t->tm_gmtoff = 0;
  return static_cast<time_t>(secs);
}

inline time_t
mktime(tm_t *t, const chrono::tz_table &z) noexcept
{
  if ( !t ) return static_cast<time_t>(-1);
  const chrono::civil in = civil_of_tm(*t);
  const i64 secs = chrono::from_civil(in, z);
  const i32 off = chrono::offset_at(z, secs);
  tm_of_civil(chrono::to_civil(secs, off), *t);
  t->tm_gmtoff = off;
  return static_cast<time_t>(secs);
}

// %%%%%%%%%%%%%%%%%%%%%
// rendering

inline constexpr usize asctime_buf = 26;

inline char *
asctime_r(const tm_t *t, char *buf) noexcept
{
  if ( !t || !buf ) return nullptr;
  const chrono::civil c = civil_of_tm(*t);
  if ( chrono::asctime_size(c) + 2 > asctime_buf ) {
    buf[0] = '\0';
    return nullptr;
  }
  const usize n = chrono::write_asctime(buf, asctime_buf - 2, c);
  if ( n == 0 ) {
    buf[0] = '\0';
    return nullptr;
  }
  buf[n] = '\n';
  buf[n + 1] = '\0';
  return buf;
}

inline char *
ctime_r(const time_t *t, char *buf, const chrono::tz_table &z) noexcept
{
  if ( !t || !buf ) return nullptr;
  tm_t tmp{};
  if ( !localtime_r(t, &tmp, z) ) {
    buf[0] = '\0';
    return nullptr;
  }
  return asctime_r(&tmp, buf);
}

inline usize
strftime(char *s, usize max, const char *format, const tm_t *t) noexcept
{
  if ( !s || !format || !t || max == 0 ) return 0;
  const chrono::civil c = civil_of_tm(*t);
  const usize need = chrono::strftime_size(format, c);
  if ( need + 1 > max ) return 0;
  const usize n = chrono::write_strftime(s, max - 1, format, c);
  s[n] = '\0';
  return n;
}

// %%%%%%%%%%%%%%%%%%%%%
// strptime

namespace __impl
{

constexpr bool
__sp_digits(const char *&p, const char *end, u32 max, i64 &out, bool &any) noexcept
{
  out = 0;
  any = false;
  u32 taken = 0;
  bool neg = false;
  if ( p < end && (*p == '-' || *p == '+') ) {
    neg = *p == '-';
    ++p;
  }
  while ( p < end && taken < max && *p >= '0' && *p <= '9' ) {
    out = out * 10 + (*p - '0');
    ++p;
    ++taken;
    any = true;
  }
  if ( neg ) out = -out;
  return any;
}

constexpr int
__sp_match(const char *&p, const char *end, const char *const *tbl, u32 lo, u32 hi, u32 abbr_len) noexcept
{
  for ( u32 k = lo; k <= hi; ++k ) {
    const char *w = tbl[k];
    // the full name first, so "June" is not consumed as "Jun" leaving a stray 'e'
    usize full = 0;
    while ( w[full] ) ++full;
    if ( chrono::__impl::__ieq(p, static_cast<usize>(end - p), 0, w, full) ) {
      p += full;
      return static_cast<int>(k);
    }
    if ( chrono::__impl::__ieq(p, static_cast<usize>(end - p), 0, w, abbr_len) ) {
      p += abbr_len;
      return static_cast<int>(k);
    }
  }
  return -1;
}

};      // namespace __impl

inline const char *
strptime(const char *s, const char *format, tm_t *t) noexcept
{
  if ( !s || !format || !t ) return nullptr;
  const char *p = s;
  const char *end = s;
  while ( *end ) ++end;

  bool have_ymd = false;
  bool pm = false;
  bool have_ampm = false;

  for ( const char *f = format; *f; ++f ) {
    if ( *f == ' ' || *f == '\t' || *f == '\n' ) {
      while ( p < end && (*p == ' ' || *p == '\t' || *p == '\n') ) ++p;
      continue;
    }
    if ( *f != '%' ) {
      if ( p >= end || *p != *f ) return nullptr;
      ++p;
      continue;
    }
    ++f;
    if ( *f == '\0' ) return nullptr;

    i64 v = 0;
    bool any = false;
    switch ( *f ) {
    case 'Y':
      if ( !__impl::__sp_digits(p, end, 6, v, any) ) return nullptr;
      t->tm_year = static_cast<int>(v - 1900);
      have_ymd = true;
      break;
    case 'y':
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      t->tm_year = static_cast<int>(v < 69 ? v + 100 : v);
      have_ymd = true;
      break;
    case 'C':
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      t->tm_year = static_cast<int>(v * 100 - 1900);
      break;
    case 'm':
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      if ( v < 1 || v > 12 ) return nullptr;
      t->tm_mon = static_cast<int>(v - 1);
      have_ymd = true;
      break;
    case 'd':
    case 'e':
      while ( p < end && *p == ' ' ) ++p;
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      if ( v < 1 || v > 31 ) return nullptr;
      t->tm_mday = static_cast<int>(v);
      have_ymd = true;
      break;
    case 'H':
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      if ( v > 23 ) return nullptr;
      t->tm_hour = static_cast<int>(v);
      break;
    case 'I':
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      if ( v < 1 || v > 12 ) return nullptr;
      t->tm_hour = static_cast<int>(v % 12);
      break;
    case 'M':
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      if ( v > 59 ) return nullptr;
      t->tm_min = static_cast<int>(v);
      break;
    case 'S':
      if ( !__impl::__sp_digits(p, end, 2, v, any) ) return nullptr;
      if ( v > 60 ) return nullptr;
      t->tm_sec = static_cast<int>(v);
      break;
    case 'j':
      if ( !__impl::__sp_digits(p, end, 3, v, any) ) return nullptr;
      if ( v < 1 || v > 366 ) return nullptr;
      t->tm_yday = static_cast<int>(v - 1);
      break;
    case 'b':
    case 'B':
    case 'h': {
      const int m = __impl::__sp_match(p, end, chrono::month_name, 1, 12, 3);
      if ( m < 0 ) return nullptr;
      t->tm_mon = m - 1;
      have_ymd = true;
      break;
    }
    case 'a':
    case 'A': {
      const int w = __impl::__sp_match(p, end, chrono::weekday_name, 0, 6, 3);
      if ( w < 0 ) return nullptr;
      t->tm_wday = w;
      break;
    }
    case 'p':
    case 'P': {
      if ( chrono::__impl::__ieq(p, static_cast<usize>(end - p), 0, "AM", 2) ) {
        pm = false;
        have_ampm = true;
        p += 2;
      } else if ( chrono::__impl::__ieq(p, static_cast<usize>(end - p), 0, "PM", 2) ) {
        pm = true;
        have_ampm = true;
        p += 2;
      } else {
        return nullptr;
      }
      break;
    }
    case 'z': {
      usize i = 0;
      i32 off = 0;
      if ( !chrono::__impl::__offset(p, static_cast<usize>(end - p), i, off) ) return nullptr;
      p += i;
      t->tm_gmtoff = off;
      break;
    }
    case 's':
      if ( !__impl::__sp_digits(p, end, 19, v, any) ) return nullptr;
      tm_of_civil(chrono::civil_utc(v), *t);
      have_ymd = true;
      break;
    case 'n':
    case 't':
      while ( p < end && (*p == ' ' || *p == '\t' || *p == '\n') ) ++p;
      break;
    case '%':
      if ( p >= end || *p != '%' ) return nullptr;
      ++p;
      break;
    case 'T':
    case 'X': {
      const char *r = strptime(p, "%H:%M:%S", t);
      if ( !r ) return nullptr;
      p = r;
      break;
    }
    case 'R': {
      const char *r = strptime(p, "%H:%M", t);
      if ( !r ) return nullptr;
      p = r;
      break;
    }
    case 'F': {
      const char *r = strptime(p, "%Y-%m-%d", t);
      if ( !r ) return nullptr;
      p = r;
      have_ymd = true;
      break;
    }
    case 'D':
    case 'x': {
      const char *r = strptime(p, "%m/%d/%y", t);
      if ( !r ) return nullptr;
      p = r;
      have_ymd = true;
      break;
    }
    case 'c': {
      const char *r = strptime(p, "%a %b %e %H:%M:%S %Y", t);
      if ( !r ) return nullptr;
      p = r;
      have_ymd = true;
      break;
    }
    default:
      return nullptr;
    }
  }

  if ( have_ampm ) {
    if ( pm && t->tm_hour < 12 ) t->tm_hour += 12;
    if ( !pm && t->tm_hour == 12 ) t->tm_hour = 0;
  }
  if ( have_ymd ) {
    const chrono::civil c = civil_of_tm(*t);
    t->tm_wday = static_cast<int>(chrono::weekday_of(c.y, c.mo, c.d));
    t->tm_yday = static_cast<int>(chrono::day_of_year(c.y, c.mo, c.d)) - 1;
  }
  return p;
}

// %%%%%%%%%%%%%%
// sleeping

inline i32
usleep(u64 usec) noexcept
{
  timespec_t r = chrono::ts_of_ns(static_cast<i64>(usec) * static_cast<i64>(chrono::ns_per_us));
  timespec_t rem{};
  for ( ;; ) {
    const i32 v = static_cast<i32>(micron::nanosleep(r, rem));
    if ( v == 0 ) return 0;
    if ( -v != static_cast<i32>(error::interrupted) ) return v;
    r = rem;
  }
}

inline i32
nsleep(u64 nsec) noexcept
{
  timespec_t r = chrono::ts_of_ns(static_cast<i64>(nsec));
  timespec_t rem{};
  for ( ;; ) {
    const i32 v = static_cast<i32>(micron::nanosleep(r, rem));
    if ( v == 0 ) return 0;
    if ( -v != static_cast<i32>(error::interrupted) ) return v;
    r = rem;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sysconf (preliminary layer)

inline constexpr int _sc_arg_max = 0;
inline constexpr int _sc_clk_tck = 2;
inline constexpr int _sc_open_max = 4;
inline constexpr int _sc_pagesize = 30;
inline constexpr int _sc_page_size = 30;
inline constexpr int _sc_nprocessors_conf = 83;
inline constexpr int _sc_nprocessors_onln = 84;

inline long
clk_tck(void) noexcept
{
  const unsigned long v = micron::getauxval(micron::at_clktck);
  return v != 0 ? static_cast<long>(v) : 100l;
}

inline long
sysconf(int name) noexcept
{
  switch ( name ) {
  case _sc_clk_tck:
    return clk_tck();
  case _sc_pagesize: {
    const unsigned long v = micron::getauxval(micron::at_pagesz);
    return v != 0 ? static_cast<long>(v) : static_cast<long>(__micron_page_size_default);
  }
  default:
    return -1l;
  }
}

};      // namespace posix
};      // namespace micron
