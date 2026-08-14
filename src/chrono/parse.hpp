//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "calendar.hpp"
#include "civil.hpp"
#include "units.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// strict tier
//
// NOTE: the results are plain PODs rather than micron::option; micron::option isn't constexpr viable yet (port this to var when it's impl)

// TODO: this is highly inefficient, optimize

namespace micron
{
namespace chrono
{

namespace parse_err
{
inline constexpr const char *empty = "empty input";
inline constexpr const char *malformed = "malformed";
inline constexpr const char *bad_year = "bad year";
inline constexpr const char *bad_month = "bad month";
inline constexpr const char *bad_day = "bad day";
inline constexpr const char *bad_hour = "bad hour";
inline constexpr const char *bad_minute = "bad minute";
inline constexpr const char *bad_second = "bad second";
inline constexpr const char *bad_offset = "bad utc offset";
inline constexpr const char *bad_weekday = "bad weekday";
inline constexpr const char *trailing = "trailing characters";
inline constexpr const char *overflow = "overflow";
inline constexpr const char *no_offset = "missing utc offset";
};      // namespace parse_err

struct civil_result {
  civil c{};
  const char *err = nullptr;
  bool has_offset = false;

  constexpr bool
  ok() const noexcept
  {
    return err == nullptr;
  }
};

struct dur_result {
  u64 ns = 0;
  const char *err = nullptr;
  bool forever = false;

  constexpr bool
  ok() const noexcept
  {
    return err == nullptr;
  }
};

namespace __impl
{

constexpr bool
__is_digit(char c) noexcept
{
  return c >= '0' && c <= '9';
}

constexpr bool
__fixed_digits(const char *s, usize n, usize &i, u32 count, u32 &out) noexcept
{
  out = 0;
  if ( i + count > n ) return false;
  for ( u32 k = 0; k < count; ++k ) {
    if ( !__is_digit(s[i]) ) return false;
    out = out * 10u + static_cast<u32>(s[i] - '0');
    ++i;
  }
  return true;
}

// 1..max digits, greedy
constexpr bool
__some_digits(const char *s, usize n, usize &i, u32 max, u64 &out, u32 &taken) noexcept
{
  out = 0;
  taken = 0;
  while ( i < n && taken < max && __is_digit(s[i]) ) {
    out = out * 10ull + static_cast<u64>(s[i] - '0');
    ++i;
    ++taken;
  }
  return taken != 0;
}

constexpr bool
__ieq(const char *s, usize n, usize i, const char *lit, usize k) noexcept
{
  if ( i + k > n ) return false;
  for ( usize j = 0; j < k; ++j ) {
    char a = s[i + j];
    char b = lit[j];
    if ( a >= 'A' && a <= 'Z' ) a = static_cast<char>(a + 32);
    if ( b >= 'A' && b <= 'Z' ) b = static_cast<char>(b + 32);
    if ( a != b ) return false;
  }
  return true;
}

// [+-]YYYY[Y...] or exactly YYYY
constexpr bool
__year(const char *s, usize n, usize &i, i32 &out) noexcept
{
  bool neg = false;
  bool expanded = false;
  if ( i < n && (s[i] == '+' || s[i] == '-') ) {
    neg = s[i] == '-';
    expanded = true;
    ++i;
  }
  u64 v = 0;
  u32 taken = 0;
  if ( !__some_digits(s, n, i, expanded ? 9u : 4u, v, taken) ) return false;
  if ( !expanded && taken != 4 ) return false;
  if ( expanded && taken < 4 ) return false;
  if ( v > 2'000'000'000ull ) return false;
  out = static_cast<i32>(neg ? -static_cast<i64>(v) : static_cast<i64>(v));
  return true;
}

// Z | z | +HH:MM | +HHMM | +HH
constexpr bool
__offset(const char *s, usize n, usize &i, i32 &out) noexcept
{
  if ( i >= n ) return false;
  if ( s[i] == 'Z' || s[i] == 'z' ) {
    out = 0;
    ++i;
    return true;
  }
  if ( s[i] != '+' && s[i] != '-' ) return false;
  const bool neg = s[i] == '-';
  ++i;
  u32 hh = 0, mm = 0;
  if ( !__fixed_digits(s, n, i, 2, hh) ) return false;
  if ( i < n && s[i] == ':' ) {
    ++i;
    if ( !__fixed_digits(s, n, i, 2, mm) ) return false;
  } else if ( i < n && __is_digit(s[i]) ) {
    if ( !__fixed_digits(s, n, i, 2, mm) ) return false;
  }
  if ( hh > 23 || mm > 59 ) return false;
  const i32 mag = static_cast<i32>(hh) * 3600 + static_cast<i32>(mm) * 60;
  out = neg ? -mag : mag;
  return true;
}

};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ISO 8601 / RFC 3339
// YYYY-MM-DD ( 'T' | 't' | ' ' ) HH:MM:SS [ '.' frac ] [ offset ]

inline constexpr civil_result
parse_iso8601(const char *s, usize n) noexcept
{
  civil_result r{};
  if ( !s || n == 0 ) {
    r.err = parse_err::empty;
    return r;
  }
  usize i = 0;
  i32 y = 0;
  if ( !__impl::__year(s, n, i, y) ) {
    r.err = parse_err::bad_year;
    return r;
  }
  if ( i >= n || s[i] != '-' ) {
    r.err = parse_err::malformed;
    return r;
  }
  ++i;
  u32 mo = 0, d = 0;
  if ( !__impl::__fixed_digits(s, n, i, 2, mo) ) {
    r.err = parse_err::bad_month;
    return r;
  }
  if ( i >= n || s[i] != '-' ) {
    r.err = parse_err::malformed;
    return r;
  }
  ++i;
  if ( !__impl::__fixed_digits(s, n, i, 2, d) ) {
    r.err = parse_err::bad_day;
    return r;
  }
  if ( mo < 1 || mo > 12 ) {
    r.err = parse_err::bad_month;
    return r;
  }
  if ( d < 1 || d > days_in_month(y, mo) ) {
    r.err = parse_err::bad_day;
    return r;
  }

  r.c.y = y;
  r.c.mo = mo;
  r.c.d = d;

  if ( i == n ) return r;      // date only -> midnight UTC

  if ( s[i] != 'T' && s[i] != 't' && s[i] != ' ' ) {
    r.err = parse_err::malformed;
    return r;
  }
  ++i;

  u32 h = 0, mi = 0, sec = 0;
  if ( !__impl::__fixed_digits(s, n, i, 2, h) ) {
    r.err = parse_err::bad_hour;
    return r;
  }
  if ( i >= n || s[i] != ':' ) {
    r.err = parse_err::malformed;
    return r;
  }
  ++i;
  if ( !__impl::__fixed_digits(s, n, i, 2, mi) ) {
    r.err = parse_err::bad_minute;
    return r;
  }
  if ( i < n && s[i] == ':' ) {
    ++i;
    if ( !__impl::__fixed_digits(s, n, i, 2, sec) ) {
      r.err = parse_err::bad_second;
      return r;
    }
  }
  if ( h > 23 ) {
    r.err = parse_err::bad_hour;
    return r;
  }
  if ( mi > 59 ) {
    r.err = parse_err::bad_minute;
    return r;
  }
  // 60 is a leap second, which the calendar cannot represent but the grammar must accept
  if ( sec > 60 ) {
    r.err = parse_err::bad_second;
    return r;
  }
  r.c.h = h;
  r.c.mi = mi;
  r.c.s = sec;

  if ( i < n && (s[i] == '.' || s[i] == ',') ) {
    ++i;
    u64 frac = 0;
    u32 taken = 0;
    if ( !__impl::__some_digits(s, n, i, 9u, frac, taken) ) {
      r.err = parse_err::malformed;
      return r;
    }
    for ( u32 k = taken; k < 9u; ++k ) frac *= 10ull;
    r.c.ns = static_cast<u32>(frac);
    // anything past nine digits is below the resolution we carry; consume and drop it
    while ( i < n && __impl::__is_digit(s[i]) ) ++i;
  }

  if ( i < n ) {
    i32 off = 0;
    if ( !__impl::__offset(s, n, i, off) ) {
      r.err = parse_err::bad_offset;
      return r;
    }
    r.c.off = off;
    r.has_offset = true;
  }

  if ( i != n ) {
    r.err = parse_err::trailing;
    return r;
  }
  return r;
}

inline constexpr civil_result
parse_rfc3339(const char *s, usize n) noexcept
{
  civil_result r = parse_iso8601(s, n);
  if ( !r.ok() ) return r;
  if ( !r.has_offset ) {
    r.err = parse_err::no_offset;
    return r;
  }
  return r;
}

inline constexpr i64
to_unix(const civil_result &r) noexcept
{
  civil c = r.c;
  if ( c.s == 60 ) c.s = 59;
  return civil_secs(c) - static_cast<i64>(c.off);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// RFC 2822 / RFC 1123

inline constexpr civil_result
parse_rfc2822(const char *s, usize n) noexcept
{
  civil_result r{};
  if ( !s || n == 0 ) {
    r.err = parse_err::empty;
    return r;
  }
  usize i = 0;

  if ( i + 4 <= n && !__impl::__is_digit(s[i]) ) {
    bool found = false;
    for ( u32 k = 0; k < 7; ++k ) {
      if ( __impl::__ieq(s, n, i, weekday_abbr[k], 3) ) {
        found = true;
        break;
      }
    }
    if ( !found ) {
      r.err = parse_err::bad_weekday;
      return r;
    }
    i += 3;
    if ( i < n && s[i] == ',' ) ++i;
    while ( i < n && s[i] == ' ' ) ++i;
  }

  u64 dv = 0;
  u32 taken = 0;
  if ( !__impl::__some_digits(s, n, i, 2u, dv, taken) ) {
    r.err = parse_err::bad_day;
    return r;
  }
  while ( i < n && s[i] == ' ' ) ++i;

  u32 mo = 0;
  for ( u32 k = 1; k <= 12; ++k ) {
    if ( __impl::__ieq(s, n, i, month_abbr[k], 3) ) {
      mo = k;
      break;
    }
  }
  if ( mo == 0 ) {
    r.err = parse_err::bad_month;
    return r;
  }
  i += 3;
  while ( i < n && s[i] == ' ' ) ++i;

  u64 yv = 0;
  if ( !__impl::__some_digits(s, n, i, 4u, yv, taken) ) {
    r.err = parse_err::bad_year;
    return r;
  }
  if ( taken == 2 ) yv += (yv < 50 ? 2000 : 1900);
  while ( i < n && s[i] == ' ' ) ++i;

  u32 h = 0, mi = 0, sec = 0;
  if ( !__impl::__fixed_digits(s, n, i, 2, h) ) {
    r.err = parse_err::bad_hour;
    return r;
  }
  if ( i >= n || s[i] != ':' ) {
    r.err = parse_err::malformed;
    return r;
  }
  ++i;
  if ( !__impl::__fixed_digits(s, n, i, 2, mi) ) {
    r.err = parse_err::bad_minute;
    return r;
  }
  if ( i < n && s[i] == ':' ) {
    ++i;
    if ( !__impl::__fixed_digits(s, n, i, 2, sec) ) {
      r.err = parse_err::bad_second;
      return r;
    }
  }
  while ( i < n && s[i] == ' ' ) ++i;

  i32 off = 0;
  if ( i < n ) {
    if ( __impl::__ieq(s, n, i, "GMT", 3) || __impl::__ieq(s, n, i, "UTC", 3) ) {
      i += 3;
    } else if ( __impl::__ieq(s, n, i, "UT", 2) ) {
      i += 2;
    } else if ( !__impl::__offset(s, n, i, off) ) {
      r.err = parse_err::bad_offset;
      return r;
    }
    r.has_offset = true;
  }
  while ( i < n && (s[i] == ' ' || s[i] == '\r' || s[i] == '\n') ) ++i;
  if ( i != n ) {
    r.err = parse_err::trailing;
    return r;
  }

  const i32 y = static_cast<i32>(yv);
  const u32 d = static_cast<u32>(dv);
  if ( d < 1 || d > days_in_month(y, mo) ) {
    r.err = parse_err::bad_day;
    return r;
  }
  if ( h > 23 ) {
    r.err = parse_err::bad_hour;
    return r;
  }
  if ( mi > 59 ) {
    r.err = parse_err::bad_minute;
    return r;
  }
  if ( sec > 60 ) {
    r.err = parse_err::bad_second;
    return r;
  }

  r.c.y = y;
  r.c.mo = mo;
  r.c.d = d;
  r.c.h = h;
  r.c.mi = mi;
  r.c.s = sec;
  r.c.off = off;
  return r;
}

struct until_spec {
  civil c{};
  i64 secs = 0;      // @ form's second count
  const char *err = nullptr;
  bool utc = false;            // a trailing Z
  bool epoch = false;          // @SECONDS
  bool time_only = false;      // HH:MM[:SS] with no date, so it rolls to the next occurrence

  constexpr bool
  ok() const noexcept
  {
    return err == nullptr;
  }
};

inline constexpr until_spec
parse_until(const char *s, usize n) noexcept
{
  until_spec u{};
  if ( !s || n == 0 ) {
    u.err = parse_err::empty;
    return u;
  }
  usize i = 0;

  if ( s[0] == '@' ) {
    i = 1;
    bool neg = false;
    if ( i < n && (s[i] == '-' || s[i] == '+') ) {
      neg = s[i] == '-';
      ++i;
    }
    u64 v = 0;
    u32 taken = 0;
    if ( !__impl::__some_digits(s, n, i, 19u, v, taken) ) {
      u.err = parse_err::malformed;
      return u;
    }
    if ( i != n ) {
      u.err = parse_err::trailing;
      return u;
    }
    if ( v > 0x7FFF'FFFF'FFFF'FFFFull ) {
      u.err = parse_err::overflow;
      return u;
    }
    u.epoch = true;
    u.secs = neg ? -static_cast<i64>(v) : static_cast<i64>(v);
    return u;
  }

  usize len = n;
  if ( len > 0 && (s[len - 1] == 'Z' || s[len - 1] == 'z') ) {
    u.utc = true;
    --len;
  }

  // HH:MM[:SS]
  bool has_dash = false;
  for ( usize k = 0; k < len; ++k ) {
    if ( s[k] == '-' ) {
      has_dash = true;
      break;
    }
  }

  if ( !has_dash ) {
    u32 h = 0, mi = 0, sec = 0;
    if ( !__impl::__fixed_digits(s, len, i, 2, h) ) {
      u.err = parse_err::bad_hour;
      return u;
    }
    if ( i >= len || s[i] != ':' ) {
      u.err = parse_err::malformed;
      return u;
    }
    ++i;
    if ( !__impl::__fixed_digits(s, len, i, 2, mi) ) {
      u.err = parse_err::bad_minute;
      return u;
    }
    if ( i < len && s[i] == ':' ) {
      ++i;
      if ( !__impl::__fixed_digits(s, len, i, 2, sec) ) {
        u.err = parse_err::bad_second;
        return u;
      }
    }
    if ( i != len ) {
      u.err = parse_err::trailing;
      return u;
    }
    if ( h > 23 ) {
      u.err = parse_err::bad_hour;
      return u;
    }
    if ( mi > 59 ) {
      u.err = parse_err::bad_minute;
      return u;
    }
    if ( sec > 59 ) {
      u.err = parse_err::bad_second;
      return u;
    }
    u.time_only = true;
    u.c.h = h;
    u.c.mi = mi;
    u.c.s = sec;
    return u;
  }

  const civil_result r = parse_iso8601(s, len);
  if ( !r.ok() ) {
    u.err = r.err;
    return u;
  }
  if ( r.has_offset ) {
    u.err = parse_err::trailing;
    return u;
  }
  u.c = r.c;
  return u;
}

inline constexpr i64
until_epoch(const until_spec &u, i64 now, const tz_table &t) noexcept
{
  if ( u.epoch ) return u.secs;
  civil c = u.c;
  if ( u.time_only ) {
    const civil today = to_civil(now, u.utc ? 0 : offset_at(t, now));
    c.y = today.y;
    c.mo = today.mo;
    c.d = today.d;
  }
  i64 e = u.utc ? civil_secs(c) : from_civil(c, t);
  if ( u.time_only && e <= now ) {
    civil mid = c;
    mid.h = 0;
    mid.mi = 0;
    mid.s = 0;
    const civil next = to_civil(civil_secs(mid) + 86400, 0);
    c.y = next.y;
    c.mo = next.mo;
    c.d = next.d;
    e = u.utc ? civil_secs(c) : from_civil(c, t);
  }
  return e;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// duration grammar
//
// NUMBER[.FRACTION][SUFFIX] concatenated: { 90, 2.5s, 500ms, 1h30m }

namespace __impl
{

constexpr u64
__dur_pow10(u32 e) noexcept
{
  u64 v = 1;
  for ( u32 i = 0; i < e; ++i ) v *= 10ull;
  return v;
}

// WARNING: longest match first; ms is milliseconds and m is minutes
constexpr u64
__dur_unit(const char *s, usize n, usize i, u32 &len) noexcept
{
  if ( i + 1 < n ) {
    if ( s[i] == 'n' && s[i + 1] == 's' ) {
      len = 2;
      return 1ull;
    }
    if ( s[i] == 'u' && s[i + 1] == 's' ) {
      len = 2;
      return ns_per_us;
    }
    if ( s[i] == 'm' && s[i + 1] == 's' ) {
      len = 2;
      return ns_per_ms;
    }
  }
  if ( i < n ) {
    len = 1;
    switch ( s[i] ) {
    case 's':
      return ns_per_s;
    case 'm':
      return ns_per_min;
    case 'h':
      return ns_per_hour;
    case 'd':
      return ns_per_day;
    case 'w':
      return ns_per_week;
    default:
      break;
    }
  }
  len = 0;
  return ns_per_s;
}

constexpr bool
__dur_add(u64 &acc, u64 v) noexcept
{
  if ( acc > ~0ull - v ) return false;
  acc += v;
  return true;
}

constexpr bool
__dur_mul(u64 a, u64 b, u64 &out) noexcept
{
  if ( a != 0 && b > ~0ull / a ) return false;
  out = a * b;
  return true;
}

};      // namespace __impl

inline constexpr dur_result
parse_duration_ns(const char *s, usize n) noexcept
{
  dur_result r{};
  if ( !s || n == 0 ) {
    r.err = parse_err::empty;
    return r;
  }

  if ( __impl::__ieq(s, n, 0, "infinity", 8) && n == 8 ) {
    r.forever = true;
    r.ns = ~0ull;
    return r;
  }
  if ( __impl::__ieq(s, n, 0, "inf", 3) && n == 3 ) {
    r.forever = true;
    r.ns = ~0ull;
    return r;
  }

  usize i = 0;
  u64 total = 0;
  bool any = false;

  while ( i < n ) {
    u64 whole = 0;
    u32 wtaken = 0;
    if ( !__impl::__some_digits(s, n, i, 19u, whole, wtaken) ) {
      r.err = parse_err::malformed;
      return r;
    }

    u64 frac = 0;
    u32 ftaken = 0;
    if ( i < n && s[i] == '.' ) {
      ++i;
      // NOTE: nine digits is the whole resolution; more of them cannot change a nanosecond count
      if ( !__impl::__some_digits(s, n, i, 9u, frac, ftaken) ) {
        r.err = parse_err::malformed;
        return r;
      }
      while ( i < n && __impl::__is_digit(s[i]) ) ++i;
    }

    u32 slen = 0;
    const u64 scale = __impl::__dur_unit(s, n, i, slen);
    i += slen;

    u64 part = 0;
    if ( !__impl::__dur_mul(whole, scale, part) ) {
      r.err = parse_err::overflow;
      return r;
    }
    if ( ftaken != 0 ) {
      u64 num = 0;
      if ( !__impl::__dur_mul(frac, scale, num) ) {
        r.err = parse_err::overflow;
        return r;
      }
      if ( !__impl::__dur_add(part, num / __impl::__dur_pow10(ftaken)) ) {
        r.err = parse_err::overflow;
        return r;
      }
    }
    if ( !__impl::__dur_add(total, part) ) {
      r.err = parse_err::overflow;
      return r;
    }
    any = true;

    if ( slen == 0 && i != n ) {
      r.err = parse_err::trailing;
      return r;
    }
  }

  if ( !any ) {
    r.err = parse_err::malformed;
    return r;
  }
  r.ns = total;
  return r;
}

};      // namespace chrono
};      // namespace micron
