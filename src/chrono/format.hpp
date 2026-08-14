//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "calendar.hpp"
#include "civil.hpp"
#include "units.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// timestamp writers

// TODO: sort of inefficient, optimize this later

namespace micron
{
namespace chrono
{

enum class offset_style : u8 {
  none,              // omit the offset entirely
  z_or_colon,        // "Z" at UTC, else "+02:00"
  colon,             // always "+00:00"
  compact,           // always "+0200"
  z_or_compact,      // "Z" at UTC, else "+0200"
};

enum class date_sep : u8 {
  iso_t,      // "2026-08-14T14:22:11"
  space,      // "2026-08-14 14:22:11"
  none,       // "20260814T142211"
};

namespace __impl
{

struct wbuf {
  char *p;
  usize cap;
  usize n;

  constexpr wbuf(char *b, usize c) noexcept : p(b), cap(c), n(0) { }

  constexpr void
  put(char ch) noexcept
  {
    if ( n < cap ) p[n] = ch;
    ++n;
  }

  constexpr void
  put_n(const char *s, usize k) noexcept
  {
    for ( usize i = 0; i < k; ++i ) put(s[i]);
  }

  constexpr void
  put_cstr(const char *s) noexcept
  {
    if ( !s ) return;
    while ( *s ) put(*s++);
  }

  constexpr void
  put_u(u64 v, u32 width, char fill = '0') noexcept
  {
    char tmp[24];
    u32 k = 0;
    do {
      tmp[k++] = static_cast<char>('0' + static_cast<char>(v % 10u));
      v /= 10u;
    } while ( v != 0 );
    for ( u32 i = k; i < width; ++i ) put(fill);
    while ( k != 0 ) put(tmp[--k]);
  }

  constexpr void
  put_i(i64 v, u32 width, char fill = '0') noexcept
  {
    if ( v < 0 ) {
      put('-');
      put_u(static_cast<u64>(-(v + 1)) + 1u, width, fill);
    } else {
      put_u(static_cast<u64>(v), width, fill);
    }
  }
};

constexpr void
__put_year(wbuf &w, i32 y) noexcept
{
  if ( y >= 0 && y <= 9999 ) {
    w.put_u(static_cast<u64>(y), 4);
  } else {
    w.put(y < 0 ? '-' : '+');
    const i64 mag = y < 0 ? -static_cast<i64>(y) : static_cast<i64>(y);
    w.put_u(static_cast<u64>(mag), 4);
  }
}

// truncating, not rounding
constexpr void
__put_subsec(wbuf &w, u32 ns, u32 digits) noexcept
{
  if ( digits == 0 ) return;
  if ( digits > 9 ) digits = 9;
  u32 scale = 1;
  for ( u32 i = digits; i < 9; ++i ) scale *= 10u;
  w.put('.');
  w.put_u(static_cast<u64>(ns / scale), digits);
}

// WARNING: offsets are emitted to minute resolution
constexpr void
__put_offset(wbuf &w, i32 off, offset_style os) noexcept
{
  if ( os == offset_style::none ) return;
  if ( off == 0 && (os == offset_style::z_or_colon || os == offset_style::z_or_compact) ) {
    w.put('Z');
    return;
  }
  w.put(off < 0 ? '-' : '+');
  const u32 mag = static_cast<u32>(off < 0 ? -static_cast<i64>(off) : static_cast<i64>(off));
  w.put_u(mag / 3600u, 2);
  if ( os == offset_style::z_or_colon || os == offset_style::colon ) w.put(':');
  w.put_u((mag % 3600u) / 60u, 2);
}

constexpr usize
__iso8601(char *buf, usize cap, const civil &c, u32 subsec_digits, date_sep sep, offset_style os) noexcept
{
  wbuf w(buf, cap);
  __put_year(w, c.y);
  w.put('-');
  w.put_u(c.mo, 2);
  w.put('-');
  w.put_u(c.d, 2);
  w.put(sep == date_sep::space ? ' ' : 'T');
  w.put_u(c.h, 2);
  w.put(':');
  w.put_u(c.mi, 2);
  w.put(':');
  w.put_u(c.s, 2);
  __put_subsec(w, c.ns, subsec_digits);
  __put_offset(w, c.off, os);
  return w.n;
}

constexpr usize
__rfc2822(char *buf, usize cap, const civil &c, bool gmt_literal) noexcept
{
  wbuf w(buf, cap);
  const u32 wd = weekday_of(c.y, c.mo, c.d);
  w.put_cstr(weekday_abbr[wd < 7 ? wd : 0]);
  w.put(',');
  w.put(' ');
  w.put_u(c.d, 2);
  w.put(' ');
  w.put_cstr(month_abbr[c.mo <= 12 ? c.mo : 0]);
  w.put(' ');
  w.put_i(static_cast<i64>(c.y), 4);
  w.put(' ');
  w.put_u(c.h, 2);
  w.put(':');
  w.put_u(c.mi, 2);
  w.put(':');
  w.put_u(c.s, 2);
  w.put(' ');
  if ( gmt_literal )
    w.put_cstr("GMT");
  else
    __put_offset(w, c.off, offset_style::compact);
  return w.n;
}

constexpr usize
__asctime(char *buf, usize cap, const civil &c) noexcept
{
  wbuf w(buf, cap);
  const u32 wd = weekday_of(c.y, c.mo, c.d);
  w.put_cstr(weekday_abbr[wd < 7 ? wd : 0]);
  w.put(' ');
  w.put_cstr(month_abbr[c.mo <= 12 ? c.mo : 0]);
  w.put(' ');
  w.put_u(c.d, 2, ' ');      // POSIX asctime uses %3d, i.e. space padded
  w.put(' ');
  w.put_u(c.h, 2);
  w.put(':');
  w.put_u(c.mi, 2);
  w.put(':');
  w.put_u(c.s, 2);
  w.put(' ');
  w.put_i(static_cast<i64>(c.y), 4);
  return w.n;
}

constexpr usize
__compact(char *buf, usize cap, const civil &c, bool with_z) noexcept
{
  wbuf w(buf, cap);
  w.put_u(static_cast<u64>(c.y < 0 ? 0 : c.y), 4);
  w.put_u(c.mo, 2);
  w.put_u(c.d, 2);
  w.put('T');
  w.put_u(c.h, 2);
  w.put_u(c.mi, 2);
  w.put_u(c.s, 2);
  if ( with_z ) w.put('Z');
  return w.n;
}

};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// public fns

inline constexpr usize
iso8601_size(const civil &c, u32 subsec_digits = 0, date_sep sep = date_sep::iso_t, offset_style os = offset_style::z_or_colon) noexcept
{
  return __impl::__iso8601(nullptr, 0, c, subsec_digits, sep, os);
}

inline constexpr usize
write_iso8601(char *buf, usize cap, const civil &c, u32 subsec_digits = 0, date_sep sep = date_sep::iso_t,
              offset_style os = offset_style::z_or_colon) noexcept
{
  const usize need = __impl::__iso8601(buf, cap, c, subsec_digits, sep, os);
  return need <= cap ? need : 0;
}

// strict RFC 3339: 'T', and 'Z' or +HH:MM
inline constexpr usize
write_rfc3339(char *buf, usize cap, const civil &c, u32 subsec_digits = 0) noexcept
{
  return write_iso8601(buf, cap, c, subsec_digits, date_sep::iso_t, offset_style::z_or_colon);
}

inline constexpr usize
rfc3339_size(const civil &c, u32 subsec_digits = 0) noexcept
{
  return iso8601_size(c, subsec_digits, date_sep::iso_t, offset_style::z_or_colon);
}

inline constexpr usize
write_iso_full(char *buf, usize cap, const civil &c, u32 subsec_digits = 9) noexcept
{
  return write_iso8601(buf, cap, c, subsec_digits, date_sep::space, offset_style::compact);
}

inline constexpr usize
iso_full_size(const civil &c, u32 subsec_digits = 9) noexcept
{
  return iso8601_size(c, subsec_digits, date_sep::space, offset_style::compact);
}

inline constexpr usize
write_rfc2822(char *buf, usize cap, const civil &c) noexcept
{
  const usize need = __impl::__rfc2822(buf, cap, c, false);
  return need <= cap ? need : 0;
}

inline constexpr usize
rfc2822_size(const civil &c) noexcept
{
  return __impl::__rfc2822(nullptr, 0, c, false);
}

// RFC 1123 / HTTP Date
inline constexpr usize
write_http_date(char *buf, usize cap, const civil &c) noexcept
{
  const usize need = __impl::__rfc2822(buf, cap, c, true);
  return need <= cap ? need : 0;
}

inline constexpr usize
http_date_size(const civil &c) noexcept
{
  return __impl::__rfc2822(nullptr, 0, c, true);
}

inline constexpr usize
write_asctime(char *buf, usize cap, const civil &c) noexcept
{
  const usize need = __impl::__asctime(buf, cap, c);
  return need <= cap ? need : 0;
}

inline constexpr usize
asctime_size(const civil &c) noexcept
{
  return __impl::__asctime(nullptr, 0, c);
}

inline constexpr usize
write_compact(char *buf, usize cap, const civil &c, bool with_z = true) noexcept
{
  const usize need = __impl::__compact(buf, cap, c, with_z);
  return need <= cap ? need : 0;
}

inline constexpr usize
compact_size(const civil &c, bool with_z = true) noexcept
{
  return __impl::__compact(nullptr, 0, c, with_z);
}

inline constexpr usize
write_log_stamp(char *buf, usize cap, const civil &c, u32 subsec_digits = 3) noexcept
{
  return write_iso8601(buf, cap, c, subsec_digits, date_sep::space, offset_style::none);
}

inline constexpr usize
log_stamp_size(const civil &c, u32 subsec_digits = 3) noexcept
{
  return iso8601_size(c, subsec_digits, date_sep::space, offset_style::none);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// strftime port

namespace __impl
{
constexpr usize
__strftime(char *buf, usize cap, const char *fmt, const civil &c) noexcept
{
  wbuf w(buf, cap);
  if ( !fmt ) return 0;

  for ( const char *f = fmt; *f; ++f ) {
    if ( *f != '%' ) {
      w.put(*f);
      continue;
    }
    ++f;
    if ( *f == '\0' ) {
      w.put('%');
      break;
    }
    const u32 wd = weekday_of(c.y, c.mo, c.d);
    switch ( *f ) {
    case 'a':
      w.put_cstr(weekday_abbr[wd < 7 ? wd : 0]);
      break;
    case 'A':
      w.put_cstr(weekday_name[wd < 7 ? wd : 0]);
      break;
    case 'b':
    case 'h':
      w.put_cstr(month_abbr[c.mo <= 12 ? c.mo : 0]);
      break;
    case 'B':
      w.put_cstr(month_name[c.mo <= 12 ? c.mo : 0]);
      break;
    case 'c':      // the POSIX locale's date-and-time, which is asctime's shape
      w.put_cstr(weekday_abbr[wd < 7 ? wd : 0]);
      w.put(' ');
      w.put_cstr(month_abbr[c.mo <= 12 ? c.mo : 0]);
      w.put(' ');
      w.put_u(c.d, 2, ' ');
      w.put(' ');
      w.put_u(c.h, 2);
      w.put(':');
      w.put_u(c.mi, 2);
      w.put(':');
      w.put_u(c.s, 2);
      w.put(' ');
      w.put_i(static_cast<i64>(c.y), 4);
      break;
    case 'C':
      w.put_u(static_cast<u64>(c.y >= 0 ? c.y / 100 : 0), 2);
      break;
    case 'd':
      w.put_u(c.d, 2);
      break;
    case 'D':      // %m/%d/%y
      w.put_u(c.mo, 2);
      w.put('/');
      w.put_u(c.d, 2);
      w.put('/');
      w.put_u(static_cast<u64>(((c.y % 100) + 100) % 100), 2);
      break;
    case 'e':
      w.put_u(c.d, 2, ' ');
      break;
    case 'F':      // %Y-%m-%d
      __put_year(w, c.y);
      w.put('-');
      w.put_u(c.mo, 2);
      w.put('-');
      w.put_u(c.d, 2);
      break;
    case 'g':
      w.put_u(static_cast<u64>(((iso_week_date(c.y, c.mo, c.d).year % 100) + 100) % 100), 2);
      break;
    case 'G':
      w.put_i(static_cast<i64>(iso_week_date(c.y, c.mo, c.d).year), 4);
      break;
    case 'H':
      w.put_u(c.h, 2);
      break;
    case 'I':
      w.put_u(c.h % 12u == 0u ? 12u : c.h % 12u, 2);
      break;
    case 'j':
      w.put_u(day_of_year(c.y, c.mo, c.d), 3);
      break;
    case 'k':
      w.put_u(c.h, 2, ' ');
      break;
    case 'l':
      w.put_u(c.h % 12u == 0u ? 12u : c.h % 12u, 2, ' ');
      break;
    case 'm':
      w.put_u(c.mo, 2);
      break;
    case 'M':
      w.put_u(c.mi, 2);
      break;
    case 'n':
      w.put('\n');
      break;
    case 'p':
      w.put_cstr(c.h < 12u ? "AM" : "PM");
      break;
    case 'P':
      w.put_cstr(c.h < 12u ? "am" : "pm");
      break;
    case 'r':      // %I:%M:%S %p
      w.put_u(c.h % 12u == 0u ? 12u : c.h % 12u, 2);
      w.put(':');
      w.put_u(c.mi, 2);
      w.put(':');
      w.put_u(c.s, 2);
      w.put(' ');
      w.put_cstr(c.h < 12u ? "AM" : "PM");
      break;
    case 'R':      // %H:%M
      w.put_u(c.h, 2);
      w.put(':');
      w.put_u(c.mi, 2);
      break;
    case 's':      // seconds since the epoch, from the fields and the offset they carry
      w.put_i(civil_secs(c) - static_cast<i64>(c.off), 1);
      break;
    case 'S':
      w.put_u(c.s, 2);
      break;
    case 't':
      w.put('\t');
      break;
    case 'T':      // %H:%M:%S
      w.put_u(c.h, 2);
      w.put(':');
      w.put_u(c.mi, 2);
      w.put(':');
      w.put_u(c.s, 2);
      break;
    case 'u':      // 1 = Monday .. 7 = Sunday
      w.put_u(wd == 0u ? 7u : wd, 1);
      break;
    case 'U':      // week of year, Sunday as the first day
      w.put_u((day_of_year(c.y, c.mo, c.d) + 6u - wd) / 7u, 2);
      break;
    case 'V':
      w.put_u(iso_week_date(c.y, c.mo, c.d).week, 2);
      break;
    case 'w':      // 0 = Sunday
      w.put_u(wd, 1);
      break;
    case 'W':      // week of year, Monday as the first day
      w.put_u((day_of_year(c.y, c.mo, c.d) + 6u - (wd == 0u ? 6u : wd - 1u)) / 7u, 2);
      break;
    case 'x':      // %m/%d/%y in the POSIX locale
      w.put_u(c.mo, 2);
      w.put('/');
      w.put_u(c.d, 2);
      w.put('/');
      w.put_u(static_cast<u64>(((c.y % 100) + 100) % 100), 2);
      break;
    case 'X':      // %H:%M:%S in the POSIX locale
      w.put_u(c.h, 2);
      w.put(':');
      w.put_u(c.mi, 2);
      w.put(':');
      w.put_u(c.s, 2);
      break;
    case 'y':
      w.put_u(static_cast<u64>(((c.y % 100) + 100) % 100), 2);
      break;
    case 'Y':
      __put_year(w, c.y);
      break;
    case 'z':
      __put_offset(w, c.off, offset_style::compact);
      break;
    case 'Z':
      if ( c.off == 0 )
        w.put_cstr("UTC");
      else
        __put_offset(w, c.off, offset_style::compact);
      break;
    case '%':
      w.put('%');
      break;
    default:      // an unknown specifier renders literally, the way glibc does
      w.put('%');
      w.put(*f);
      break;
    }
  }
  return w.n;
}
};      // namespace __impl

inline constexpr usize
strftime_size(const char *fmt, const civil &c) noexcept
{
  return __impl::__strftime(nullptr, 0, fmt, c);
}

inline constexpr usize
write_strftime(char *buf, usize cap, const char *fmt, const civil &c) noexcept
{
  const usize need = __impl::__strftime(buf, cap, fmt, c);
  return need <= cap ? need : 0;
}

namespace __impl
{
constexpr usize
__dur_hms(char *buf, usize cap, u64 secs) noexcept
{
  wbuf w(buf, cap);
  const u64 d = secs / 86400ull;
  const u64 h = (secs / 3600ull) % 24ull;
  const u64 m = (secs / 60ull) % 60ull;
  if ( d != 0 ) {
    w.put_u(d, 1);
    w.put('d');
    w.put(' ');
    w.put_u(h, 2);
    w.put(':');
    w.put_u(m, 2);
    return w.n;
  }
  w.put_u(h, 2);
  w.put(':');
  w.put_u(m, 2);
  w.put(':');
  w.put_u(secs % 60ull, 2);
  return w.n;
}

constexpr usize
__dur_clock(char *buf, usize cap, u64 ticks, u64 hz) noexcept
{
  wbuf w(buf, cap);
  if ( hz == 0 ) hz = 1;
  const u64 secs = ticks / hz;
  if ( secs < 3600ull ) {
    w.put_u(secs / 60ull, 1);
    w.put(':');
    w.put_u(secs % 60ull, 2);
    w.put('.');
    w.put_u(((ticks % hz) * 100ull) / hz, 2);
    return w.n;
  }
  if ( secs < 86400ull ) {
    w.put_u(secs / 3600ull, 1);
    w.put(':');
    w.put_u((secs / 60ull) % 60ull, 2);
    w.put(':');
    w.put_u(secs % 60ull, 2);
    return w.n;
  }
  w.put_u(secs / 86400ull, 1);
  w.put('d');
  w.put(' ');
  w.put_u((secs / 3600ull) % 24ull, 2);
  w.put(':');
  w.put_u((secs / 60ull) % 60ull, 2);
  return w.n;
}

constexpr usize
__dur_units(char *buf, usize cap, u64 ns) noexcept
{
  wbuf w(buf, cap);
  if ( ns == 0 ) {
    w.put('0');
    w.put('s');
    return w.n;
  }

  struct comp {
    u64 scale;
    const char *suffix;
  };

  constexpr comp __c[8] = { { ns_per_week, "w" }, { ns_per_day, "d" }, { ns_per_hour, "h" }, { ns_per_min, "m" },
                            { ns_per_s, "s" },    { ns_per_ms, "ms" }, { ns_per_us, "us" },  { 1ull, "ns" } };
  u64 rest = ns;
  for ( usize i = 0; i < 8; ++i ) {
    const u64 v = rest / __c[i].scale;
    if ( v == 0 ) continue;
    w.put_u(v, 1);
    w.put_cstr(__c[i].suffix);
    rest -= v * __c[i].scale;
    if ( rest == 0 ) break;
  }
  return w.n;
}
};      // namespace __impl

inline constexpr usize
write_duration_hms(char *buf, usize cap, u64 secs) noexcept
{
  const usize need = __impl::__dur_hms(buf, cap, secs);
  return need <= cap ? need : 0;
}

inline constexpr usize
duration_hms_size(u64 secs) noexcept
{
  return __impl::__dur_hms(nullptr, 0, secs);
}

inline constexpr usize
write_duration_clock(char *buf, usize cap, u64 ticks, u64 hz) noexcept
{
  const usize need = __impl::__dur_clock(buf, cap, ticks, hz);
  return need <= cap ? need : 0;
}

inline constexpr usize
duration_clock_size(u64 ticks, u64 hz) noexcept
{
  return __impl::__dur_clock(nullptr, 0, ticks, hz);
}

inline constexpr usize
write_duration_units(char *buf, usize cap, u64 ns) noexcept
{
  const usize need = __impl::__dur_units(buf, cap, ns);
  return need <= cap ? need : 0;
}

inline constexpr usize
duration_units_size(u64 ns) noexcept
{
  return __impl::__dur_units(nullptr, 0, ns);
}

inline constexpr usize iso8601_max = 48;      // expanded year + 9 sub-second digits + "+HH:MM"
inline constexpr usize rfc2822_max = 48;
inline constexpr usize asctime_max = 32;
inline constexpr usize compact_max = 16;
inline constexpr usize duration_max = 64;

};      // namespace chrono
};      // namespace micron
