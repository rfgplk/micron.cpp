//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../math/generic.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "string.hpp"

#include "../maps/hopscotch.hpp"
#include "../tuple.hpp"

#include "../array.hpp"
#include "../slice.hpp"
#include "../vector.hpp"

#include "conversions/floating_point.hpp"
#include "conversions/integral.hpp"

namespace micron
{

template <typename K, typename V, usize MH = 32> using dictionary_t = hopscotch_map<K, V, MH>;

/*
template <typename T>
inline micron::hstring<T>
to_string(const T *str)
{
  return micron::hstring<T>(str);
};
*/

template <is_string T>
inline auto
c_str(const T &str) -> const char *
{
  // guard against empty strings
  if ( str.empty() ) return "";
  return reinterpret_cast<const char *>(&str[0]);
}

namespace format
{

namespace __impl
{

constexpr usize __max_frac_digits_f32 = 9;      // f32 has ~7 sig digits
constexpr usize __max_frac_digits_f64 = 18;     // f64 has ~15-17 sig digits

inline constexpr f64 __pow10_tbl[19] = { 1.0,
                                         10.0,
                                         100.0,
                                         1000.0,
                                         10000.0,
                                         100000.0,
                                         1000000.0,
                                         10000000.0,
                                         100000000.0,
                                         1000000000.0,
                                         10000000000.0,
                                         100000000000.0,
                                         1000000000000.0,
                                         10000000000000.0,
                                         100000000000000.0,
                                         1000000000000000.0,
                                         10000000000000000.0,
                                         100000000000000000.0,
                                         1000000000000000000.0 };

constexpr usize __fmt_buf_size = 72;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// format-only buffer helpers (bool, pointer)

inline usize
bool_to_buf(char *buf, usize buf_sz, bool val)
{
  if ( val ) {
    if ( buf_sz < 4 ) return 0;
    buf[0] = 't';
    buf[1] = 'r';
    buf[2] = 'u';
    buf[3] = 'e';
    return 4;
  } else {
    if ( buf_sz < 5 ) return 0;
    buf[0] = 'f';
    buf[1] = 'a';
    buf[2] = 'l';
    buf[3] = 's';
    buf[4] = 'e';
    return 5;
  }
}

inline usize
ptr_to_buf(char *buf, usize buf_sz, const void *ptr)
{

  if ( buf_sz < 4 ) return 0;
  if ( ptr == nullptr ) {
    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = '0';
    return 3;
  }
  buf[0] = '0';
  buf[1] = 'x';

  char tmp[20];
  char *tend = tmp + 20;
  char *tstart = micron::__impl::uint_to_buf_base_backward(tend, reinterpret_cast<u64>(ptr), 16, false);
  usize dlen = static_cast<usize>(tend - tstart);
  if ( dlen + 2 > buf_sz ) dlen = buf_sz - 2;
  micron::bytecpy(buf + 2, tstart, dlen);
  return 2 + dlen;
}

inline usize
fmt_uint_to_buf(char *buf, usize buf_sz, u64 val, u32 base, bool upper)
{

  char tmp[72];
  char *tend = tmp + 72;
  char *start;
  if ( base == 10 )
    start = micron::__impl::uint_to_buf_backward(tend, val);
  else
    start = micron::__impl::uint_to_buf_base_backward(tend, val, base, upper);
  usize len = static_cast<usize>(tend - start);
  if ( len > buf_sz ) len = buf_sz;
  micron::bytecpy(buf, start, len);
  return len;
}

inline usize
fmt_int_to_buf(char *buf, usize buf_sz, i64 val, u32 base, bool upper)
{
  usize off = 0;
  u64 uval;
  if ( val < 0 && base == 10 ) {
    if ( buf_sz == 0 ) return 0;
    buf[0] = '-';
    off = 1;
    uval = static_cast<u64>(-(val + 1)) + 1ull;
  } else {
    uval = static_cast<u64>(val);
  }
  return off + fmt_uint_to_buf(buf + off, buf_sz - off, uval, base, upper);
}

inline usize
fmt_float_to_buf(char *buf, usize buf_sz, f64 val, u32 precision)
{
  return micron::__impl::__ryu::d2f_buffered(val, buf, buf_sz, precision);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// format spec parser: {[index][:[[fill]align][width][.precision][type]]}

struct fmt_spec {
  char fill = ' ';
  char align = '\0';     // '<' left, '>' right, '^' center
  u32 width = 0;
  u32 prec = 6;     // default float precision
  bool has_prec = false;
  char type = '\0';     // d, x, X, o, b, f, e, s, p
  bool alt = false;     // '#' flag
};

inline fmt_spec
parse_spec(const char *start, const char *end)
{
  fmt_spec s{};
  const char *p = start;
  if ( p >= end ) return s;

  // fill + align
  if ( (end - p) >= 2 && (p[1] == '<' || p[1] == '>' || p[1] == '^') ) {
    s.fill = p[0];
    s.align = p[1];
    p += 2;
  } else if ( *p == '<' || *p == '>' || *p == '^' ) {
    s.align = *p;
    ++p;
  }

  if ( p < end && *p == '#' ) {
    s.alt = true;
    ++p;
  }

  // width
  while ( p < end && *p >= '0' && *p <= '9' ) {
    s.width = s.width * 10 + (*p - '0');
    ++p;
  }

  // precision
  if ( p < end && *p == '.' ) {
    ++p;
    s.prec = 0;
    s.has_prec = true;
    while ( p < end && *p >= '0' && *p <= '9' ) {
      s.prec = s.prec * 10 + (*p - '0');
      ++p;
    }
  }

  // type
  if ( p < end ) {
    s.type = *p;
  }

  return s;
}

inline void
apply_padding(hstring<schar> &out, const char *content, usize content_len, const fmt_spec &spec)
{
  if ( spec.width == 0 || content_len >= spec.width ) {
    out.append(content, content_len);
    return;
  }
  usize pad_total = spec.width - content_len;
  char fill = spec.fill ? spec.fill : ' ';
  char align = spec.align;

  // default alignment
  if ( align == '\0' ) {
    if ( spec.type == 's' || spec.type == '\0' )
      align = '<';
    else
      align = '>';
  }

  if ( align == '>' ) {
    for ( usize i = 0; i < pad_total; ++i ) out += fill;
    out.append(content, content_len);
  } else if ( align == '<' ) {
    out.append(content, content_len);
    for ( usize i = 0; i < pad_total; ++i ) out += fill;
  } else {
    usize left = pad_total / 2;
    usize right = pad_total - left;
    for ( usize i = 0; i < left; ++i ) out += fill;
    out.append(content, content_len);
    for ( usize i = 0; i < right; ++i ) out += fill;
  }
}

};     // namespace __impl

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// char classification: constexpr value overloads

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isupper(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return (t <= 0x5A && t >= 0x41);
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
islower(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return (t <= 0x7A && t >= 0x61);
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline T
to_upper(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    if ( t >= 0x61 && t <= 0x7A ) return static_cast<T>(t - 32);
  }
  return t;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline T
to_lower(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    if ( t >= 0x41 && t <= 0x5A ) return static_cast<T>(t + 32);
  }
  return t;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isalnum(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return (c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A);
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isalpha(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A);
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
iscntrl(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isdigit(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return c >= 0x30 && c <= 0x39;
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isgraph(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return c >= 0x21 && c <= 0x7E;
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isprint(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return c >= 0x20 && c <= 0x7E;
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
ispunct(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return (c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) || (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7E);
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isspace(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return c == 0x20 || (c >= 0x09 && c <= 0x0D);
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isxdigit(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return (c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x46) || (c >= 0x61 && c <= 0x66);
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isascii(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return c >= 0x00 && c <= 0x7F;
  }
  return false;
}

template <typename T>
  requires micron::is_fundamental_v<T>
constexpr inline bool
isblank(const T c)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    return c == 0x20 || c == 0x09;
  }
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// char classification: iterator overloads

template <typename T>
inline bool
isupper(typename T::iterator t)
{
  return isupper<typename T::value_type>(*t);
}

template <is_string T>
inline bool
islower(typename T::iterator t)
{
  return islower<typename T::value_type>(*t);
}

template <typename T>
inline void
to_upper(typename T::iterator t)
{
  *t = to_upper<typename T::value_type>(*t);
}

template <typename T>
inline void
to_lower(typename T::iterator t)
{
  *t = to_lower<typename T::value_type>(*t);
}

template <typename T>
inline bool
isalnum(typename T::iterator t)
{
  return isalnum<typename T::value_type>(*t);
}

template <typename T>
inline bool
isalpha(typename T::iterator t)
{
  return isalpha<typename T::value_type>(*t);
}

template <typename T>
inline bool
iscntrl(typename T::iterator t)
{
  return iscntrl<typename T::value_type>(*t);
}

template <typename T>
inline bool
isdigit(typename T::iterator t)
{
  return isdigit<typename T::value_type>(*t);
}

template <typename T>
inline bool
isgraph(typename T::iterator t)
{
  return isgraph<typename T::value_type>(*t);
}

template <typename T>
inline bool
isprint(typename T::iterator t)
{
  return isprint<typename T::value_type>(*t);
}

template <typename T>
inline bool
ispunct(typename T::iterator t)
{
  return ispunct<typename T::value_type>(*t);
}

template <typename T>
inline bool
isspace(typename T::iterator t)
{
  return isspace<typename T::value_type>(*t);
}

template <typename T>
inline bool
isxdigit(typename T::iterator t)
{
  return isxdigit<typename T::value_type>(*t);
}

template <typename T>
inline bool
isascii(typename T::iterator t)
{
  return isascii<typename T::value_type>(*t);
}

template <typename T>
inline bool
isblank(typename T::iterator t)
{
  return isblank<typename T::value_type>(*t);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// char* to_upper / to_lower

inline void
to_upper(char *str)
{
  if ( str == nullptr ) return;
  while ( *str ) {
    if ( *str >= 0x61 && *str <= 0x7A ) *str -= 32;
    ++str;
  }
}

inline void
to_lower(char *str)
{
  if ( str == nullptr ) return;
  while ( *str ) {
    if ( *str >= 0x41 && *str <= 0x5A ) *str += 32;
    ++str;
  }
}

inline hstring<schar>
to_upper(const char *str, usize len)
{
  hstring<schar> result(str);
  if ( len < result.size() ) result.truncate(len);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x61 && *itr <= 0x7A ) *itr -= 32;
  return result;
}

inline hstring<schar>
to_lower(const char *str, usize len)
{
  hstring<schar> result(str);
  if ( len < result.size() ) result.truncate(len);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x41 && *itr <= 0x5A ) *itr += 32;
  return result;
}

template <usize N>
inline hstring<schar>
to_upper(const char (&str)[N])
{
  hstring<schar> result(str);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x61 && *itr <= 0x7A ) *itr -= 32;
  return result;
}

template <usize N>
inline hstring<schar>
to_lower(const char (&str)[N])
{
  hstring<schar> result(str);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x41 && *itr <= 0x5A ) *itr += 32;
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// _all / _any string predicates

template <is_string T>
bool
isalnum_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isalnum<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isalpha_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isalpha<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
iscntrl_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !iscntrl<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isdigit_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isdigit<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isgraph_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isgraph<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
islower_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !islower<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isupper_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isupper<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isprint_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isprint<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
ispunct_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !ispunct<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isspace_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isspace<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isxdigit_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isxdigit<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isascii_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isascii<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isblank_all(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( !isblank<T>(itr) ) return false;
  }
  return true;
}

template <is_string T>
bool
isalnum_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isalnum<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isalpha_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isalpha<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
iscntrl_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( iscntrl<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isdigit_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isdigit<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isgraph_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isgraph<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
islower_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( islower<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isupper_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isupper<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isprint_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isprint<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
ispunct_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( ispunct<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isspace_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isspace<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isxdigit_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isxdigit<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isascii_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isascii<T>(itr) ) return true;
  }
  return false;
}

template <is_string T>
bool
isblank_any(const T &str)
{
  if ( str.empty() ) return false;
  for ( auto itr = str.begin(); itr != str.end(); ++itr ) {
    if ( isblank<T>(itr) ) return true;
  }
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// casefold / upper

template <is_string T>
T &
casefold(T &str)
{
  auto itr = str.begin();
  while ( itr != str.end() ) {
    if ( isupper<T>(itr) ) to_lower<T>(itr);
    ++itr;
  }
  return str;
}

template <is_string T>
T
casefold(const T &str)
{
  T result(str);
  return casefold(result);
}

inline void
casefold(char *str)
{
  if ( str == nullptr ) return;
  while ( *str ) {
    if ( *str >= 0x41 && *str <= 0x5A ) *str += 32;
    ++str;
  }
}

inline hstring<schar>
casefold(const char *str, usize len)
{
  hstring<schar> result(str);
  if ( len < result.size() ) result.truncate(len);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x41 && *itr <= 0x5A ) *itr += 32;
  return result;
}

template <usize N>
inline hstring<schar>
casefold(const char (&str)[N])
{
  hstring<schar> result(str);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x41 && *itr <= 0x5A ) *itr += 32;
  return result;
}

template <is_string T>
T &
upper(T &str)
{
  auto itr = str.begin();
  while ( itr != str.end() ) {
    if ( islower<T>(itr) ) to_upper<T>(itr);
    ++itr;
  }
  return str;
}

template <is_string T>
T
upper(const T &str)
{
  T result(str);
  return upper(result);
}

inline void
upper(char *str)
{
  if ( str == nullptr ) return;
  while ( *str ) {
    if ( *str >= 0x61 && *str <= 0x7A ) *str -= 32;
    ++str;
  }
}

inline hstring<schar>
upper(const char *str, usize len)
{
  hstring<schar> result(str);
  if ( len < result.size() ) result.truncate(len);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x61 && *itr <= 0x7A ) *itr -= 32;
  return result;
}

template <usize N>
inline hstring<schar>
upper(const char (&str)[N])
{
  hstring<schar> result(str);
  for ( auto itr = result.begin(); itr != result.end(); ++itr )
    if ( *itr >= 0x61 && *itr <= 0x7A ) *itr -= 32;
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// strip

template <char Tk = ' ', is_string T>
T &
strip(T &data)
{
  if ( data.empty() ) return data;
  auto start = data.begin();
  auto stop = data.end();
  while ( start != stop && *start == Tk ) ++start;
  while ( stop != start && *(stop - 1) == Tk ) --stop;
  if ( start == data.begin() && stop == data.end() ) return data;
  usize new_len = static_cast<usize>(stop - start);
  if ( start != data.begin() ) micron::bytemove(data.begin(), start, new_len);
  data.set_size(new_len);
  return data;
}

template <char Tk = ' ', is_string T>
T
strip(const T &data)
{
  T result(data);
  return strip<Tk>(result);
}

template <char Tk = ' '>
char *
strip(char *data, usize &len)
{
  if ( data == nullptr || len == 0 ) return data;
  usize start = 0;
  while ( start < len && data[start] == Tk ) ++start;
  usize end = len;
  while ( end > start && data[end - 1] == Tk ) --end;
  usize new_len = end - start;
  if ( start > 0 ) micron::bytemove(data, data + start, new_len);
  data[new_len] = 0;
  len = new_len;
  return data;
}

template <char Tk = ' '>
hstring<schar>
strip(const char *data, usize len)
{
  hstring<schar> result(data);
  if ( len < result.size() ) result.truncate(len);
  return strip<Tk>(result);
}

template <char Tk = ' ', usize N>
hstring<schar>
strip(const char (&data)[N])
{
  hstring<schar> result(data);
  return strip<Tk>(result);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ends_with / starts_with

template <is_string T>
bool
ends_with(const T &data, const char *fnd)
{
  auto sz = micron::strlen(fnd);
  if ( sz > data.size() ) return false;
  return strcmp(data.end() - sz, &fnd[0]) == 0;
}

template <is_string T>
bool
ends_with(const T &data, const T &fnd)
{
  if ( fnd.size() > data.size() ) return false;
  return strcmp(data.end() - fnd.size(), fnd.begin()) == 0;
}

inline bool
ends_with(const char *data, usize data_len, const char *fnd)
{
  if ( data == nullptr || fnd == nullptr ) return false;
  usize fnd_len = micron::strlen(fnd);
  if ( fnd_len > data_len ) return false;
  return strcmp(data + data_len - fnd_len, fnd) == 0;
}

template <is_string T>
bool
ends_with(const char *data, usize data_len, const T &fnd)
{
  if ( data == nullptr || fnd.empty() ) return false;
  if ( fnd.size() > data_len ) return false;
  return strcmp(data + data_len - fnd.size(), fnd.begin()) == 0;
}

template <usize N, usize M>
inline bool
ends_with(const char (&data)[N], const char (&fnd)[M])
{
  constexpr usize data_len = N - 1;
  constexpr usize fnd_len = M - 1;
  if constexpr ( fnd_len > data_len ) return false;
  return strcmp(data + data_len - fnd_len, fnd) == 0;
}

template <is_string T>
bool
starts_with(const T &data, const char *fnd)
{
  auto sz = micron::strlen(fnd);
  if ( sz > data.size() ) return false;
  auto buf = data.substr(0, sz);
  return strcmp(buf.begin(), &fnd[0]) == 0;
}

template <is_string T>
bool
starts_with(const T &data, const T &fnd)
{
  if ( fnd.size() > data.size() ) return false;
  auto buf = data.substr(0, fnd.size());
  return strcmp(buf.begin(), fnd.begin()) == 0;
}

inline bool
starts_with(const char *data, usize data_len, const char *fnd)
{
  if ( data == nullptr || fnd == nullptr ) return false;
  usize fnd_len = micron::strlen(fnd);
  if ( fnd_len > data_len ) return false;
  return strncmp(data, fnd, fnd_len) == 0;
}

template <is_string T>
bool
starts_with(const char *data, usize data_len, const T &fnd)
{
  if ( data == nullptr || fnd.empty() ) return false;
  if ( fnd.size() > data_len ) return false;
  return strncmp(data, fnd.begin(), fnd.size()) == 0;
}

template <usize N, usize M>
inline bool
starts_with(const char (&data)[N], const char (&fnd)[M])
{
  constexpr usize data_len = N - 1;
  constexpr usize fnd_len = M - 1;
  if constexpr ( fnd_len > data_len ) return false;
  return strncmp(data, fnd, fnd_len) == 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// concat

template <is_string T>
T
concat(const char *lhs, const char *rhs)
{
  T str;
  str += lhs;
  str += rhs;
  return str;
}

inline hstring<schar>
concat(const char *lhs, const char *rhs)
{
  hstring str;
  str += lhs;
  str += rhs;
  return str;
}

template <usize N, typename T>
sstring<N, T> &
concat(sstring<N, T> &lhs, const char *rhs)
{
  auto sz = micron::strlen(rhs);
  if ( sz + lhs.size() > lhs.max_size() ) exc<except::library_error>("concat range error.");
  auto *p = lhs.begin();
  micron::bytemove(p + sz, p, lhs.size());
  micron::bytecpy(p, rhs, sz);
  lhs.set_size(lhs.size() + sz);
  return lhs;
}

template <usize N, typename T>
sstring<N, T> &
concat(const char *lhs, sstring<N, T> &rhs)
{
  auto sz = micron::strlen(lhs);
  if ( sz + rhs.size() > rhs.max_size() ) exc<except::library_error>("concat range error.");
  auto *p = rhs.begin();
  micron::bytemove(p + sz, p, rhs.size());
  micron::bytecpy(p, lhs, sz);
  rhs.set_size(rhs.size() + sz);
  return rhs;
}

template <is_string... T>
hstring<schar>
concat(const T &...strs)
{
  hstring<schar> str;
  ((str += strs), ...);
  return str;
}

inline hstring<schar>
concat(const char *lhs, usize lhs_len, const char *rhs, usize rhs_len)
{
  hstring<schar> str(lhs_len + rhs_len + 1);
  str.append(lhs, lhs_len + 1);
  str.append(rhs, rhs_len + 1);
  return str;
}

template <usize N, usize M>
inline hstring<schar>
concat(const char (&lhs)[N], const char (&rhs)[M])
{
  hstring<schar> str;
  str += lhs;
  str += rhs;
  return str;
}

template <is_string T>
T
concat(const T &lhs, const T &rhs)
{
  T str(lhs);
  str += rhs;
  return str;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// split

template <is_string T>
T
split(const T &data, usize at = 0)
{
  if ( at > data.size() ) exc<except::library_error>("micron::split() out of bounds.");
  if ( !at ) return data.substr(data.size() / 2, data.size() - (data.size() / 2));
  return data.substr(at, data.size() - at);
}

template <is_string T>
T
split(const T &data, typename T::const_iterator itr)
{
  if ( itr >= data.end() or itr < data.begin() ) exc<except::library_error>("micron::split() out of bounds.");
  usize pos = static_cast<usize>(itr - data.begin());
  return data.substr(pos, data.size() - pos);
}

template <is_string T, is_string O>
O
split(const T &data, typename T::const_iterator itr)
{
  if ( itr >= data.end() or itr < data.begin() ) exc<except::library_error>("micron::split() out of bounds.");
  usize pos = static_cast<usize>(itr - data.begin());
  return data.substr(pos, data.size() - pos);
}

inline hstring<schar>
split(const char *data, usize len, usize at = 0)
{
  if ( data == nullptr ) exc<except::library_error>("micron::split() null pointer.");
  if ( at > len ) exc<except::library_error>("micron::split() out of bounds.");
  usize pos = at ? at : len / 2;
  return hstring<schar>(data + pos, data + len);
}

inline hstring<schar>
split_delim(const char *data, usize len, char delim)
{
  if ( data == nullptr ) exc<except::library_error>("micron::split() null pointer.");
  for ( usize i = 0; i < len; ++i )
    if ( data[i] == delim ) return hstring<schar>(data, data + i);
  return hstring<schar>(data, data + len);
}

template <usize N>
inline hstring<schar>
split(const char (&data)[N], usize at = 0)
{
  constexpr usize len = N - 1;
  if ( at > len ) exc<except::library_error>("micron::split() out of bounds.");
  usize pos = at ? at : len / 2;
  return hstring<schar>(data + pos, data + len);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// boyer-moore search

template <is_string T>
micron::fvector<int>
build_bad_char(const T &pattern)
{
  micron::fvector<int> bad(256, -1);
  for ( usize i = 0; i < pattern.size(); ++i ) bad[static_cast<unsigned char>(pattern[i])] = i;
  return bad;
}

template <is_string T>
micron::fvector<int>
build_good_suffix(const T &pattern)
{
  usize m = pattern.size();
  micron::fvector<int> shift(m + 1, 0);
  micron::fvector<int> border(m + 1, 0);
  for ( usize k = 0; k <= m; ++k ) shift[k] = m;
  int i = m, j = m + 1;
  border[i] = j;
  while ( i > 0 ) {
    while ( j <= static_cast<int>(m) && pattern[i - 1] != pattern[j - 1] ) {
      if ( shift[j] == static_cast<int>(m) ) shift[j] = j - i;
      j = border[j];
    }
    --i;
    --j;
    border[i] = j;
  }
  j = border[0];
  for ( i = 0; i <= static_cast<int>(m); ++i ) {
    if ( shift[i] == static_cast<int>(m) ) shift[i] = j;
    if ( i == j ) j = border[j];
  }
  return shift;
}

template <is_string T>
micron::fvector<int>
boyer_moore_search(const T &text, const T &pattern)
{
  micron::fvector<int> result;
  usize n = text.size(), m = pattern.size();
  if ( m == 0 ) return result;
  result.reserve(n / (m ? m : 1));
  auto bad = build_bad_char(pattern);
  auto good = build_good_suffix(pattern);
  usize s = 0;
  while ( s <= n - m ) {
    int j = m - 1;
    while ( j >= 0 && pattern[j] == text[s + j] ) --j;
    if ( j < 0 ) {
      result.inline_push_back(s);
      s += good[0];
    } else {
      int bc_shift = j - bad[static_cast<unsigned char>(text[s + j])];
      s += micron::max(1, micron::max(bc_shift, good[j + 1]));
    }
  }
  return result;
}

template <usize Pt, usize N>
constexpr micron::carray<usize, N>
bm_find(const char (&text)[N], const char (&pattern)[Pt], usize &found_count)
{
  micron::carray<int, 256> bad{};
  micron::carray<usize, Pt + 1> good{};
  micron::carray<usize, N> result{};
  found_count = 0;
  bad.fill(-1);
  for ( usize i = 0; i < Pt - 1; ++i ) bad[static_cast<unsigned char>(pattern[i])] = i;
  micron::carray<int, Pt + 1> border{};
  usize m = Pt - 1;
  int i = m, j = m + 1;
  border[i] = j;
  while ( i > 0 ) {
    while ( j <= static_cast<int>(m) && pattern[i - 1] != pattern[j - 1] ) j = border[j];
    --i;
    --j;
    border[i] = j;
  }
  j = border[0];
  for ( i = 0; i <= static_cast<int>(m); ++i ) {
    if ( good[i] == 0 ) good[i] = j;
    if ( i == j ) j = border[j];
  }
  usize s = 0;
  while ( s <= N - Pt ) {
    int k = Pt - 1;
    while ( k >= 0 && pattern[k] == text[s + k] ) --k;
    if ( k < 0 ) {
      result[found_count++] = s;
      s += good[0];
    } else {
      int bc_shift = k - bad[static_cast<unsigned char>(text[s + k])];
      s += micron::max(1, micron::max(bc_shift, good[k + 1]));
    }
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fast_find (horspool)

template <is_string T>
auto
fast_find(const T &data, const char *fnd) -> typename T::const_iterator
{
  usize n = data.size(), m = micron::strlen(fnd);
  if ( m == 0 || m > n ) return data.end();
  u16 shift[256];
  for ( usize i = 0; i < 256; ++i ) shift[i] = static_cast<u16>(m);
  for ( usize i = 0; i < m - 1; ++i ) shift[static_cast<unsigned char>(fnd[i])] = static_cast<u16>(m - 1 - i);
  auto it = data.begin();
  auto end_it = data.begin() + (n - m + 1);
  while ( it != end_it ) {
    usize j = m;
    while ( j > 0 && (*(it + j - 1) == fnd[j - 1]) ) --j;
    if ( j == 0 ) return it;
    it += shift[static_cast<unsigned char>(*(it + m - 1))];
  }
  return data.end();
}

inline const char *
fast_find(const char *data, usize n, const char *fnd)
{
  if ( data == nullptr || fnd == nullptr ) return nullptr;
  usize m = micron::strlen(fnd);
  if ( m == 0 || m > n ) return nullptr;
  u16 shift[256];
  for ( usize i = 0; i < 256; ++i ) shift[i] = static_cast<u16>(m);
  for ( usize i = 0; i < m - 1; ++i ) shift[static_cast<unsigned char>(fnd[i])] = static_cast<u16>(m - 1 - i);
  const char *it = data;
  const char *end_it = data + (n - m + 1);
  while ( it != end_it ) {
    usize j = m;
    while ( j > 0 && *(it + j - 1) == fnd[j - 1] ) --j;
    if ( j == 0 ) return it;
    it += shift[static_cast<unsigned char>(*(it + m - 1))];
  }
  return nullptr;
}

template <usize N, usize M>
inline const char *
fast_find(const char (&data)[N], const char (&fnd)[M])
{
  return fast_find(static_cast<const char *>(data), N - 1, static_cast<const char *>(fnd));
}

template <is_string T>
auto
fast_find(const T &data, const T &fnd) -> typename T::const_iterator
{
  usize n = data.size(), m = fnd.size();
  if ( m == 0 || m > n ) return data.end();
  u16 shift[256];
  for ( usize i = 0; i < 256; ++i ) shift[i] = static_cast<u16>(m);
  for ( usize i = 0; i < m - 1; ++i ) shift[static_cast<unsigned char>(fnd[i])] = static_cast<u16>(m - 1 - i);
  auto it = data.begin();
  auto end_it = data.begin() + (n - m + 1);
  while ( it != end_it ) {
    usize j = m;
    while ( j > 0 && *(it + j - 1) == fnd[j - 1] ) --j;
    if ( j == 0 ) return it;
    it += shift[static_cast<unsigned char>(*(it + m - 1))];
  }
  return data.end();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// contains

template <is_string T>
auto
contains(const T &data, typename T::const_iterator from, const char fnd) -> bool
{
  if ( data.empty() || from < data.begin() || from >= data.end() ) return false;
  for ( auto itr = from; itr != data.end(); ++itr )
    if ( *itr == fnd ) return true;
  return false;
}

template <is_string T>
auto
contains(const T &data, typename T::const_iterator from, const char *fnd) -> bool
{
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() || sz == 0 || from < data.begin() || from >= data.end() ) return false;
  for ( auto itr = from; itr != data.end(); ++itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return true;
  }
  return false;
}

template <is_string T>
auto
contains(const T &data, const char fnd) -> bool
{
  if ( data.empty() ) return false;
  for ( auto itr = data.begin(); itr != data.end(); ++itr )
    if ( *itr == fnd ) return true;
  return false;
}

template <is_string T>
auto
contains(const T &data, const char *fnd) -> bool
{
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() || sz == 0 ) return false;
  for ( auto itr = data.begin(); itr + sz <= data.end(); ++itr ) {
    u64 j = 0;
    while ( j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return true;
  }
  return false;
}

template <is_string T>
bool
contains(const T &data, const T &fnd)
{
  if ( fnd.empty() || fnd.size() > data.size() ) return false;
  for ( auto itr = data.begin(); itr + fnd.size() <= data.end(); ++itr ) {
    usize j = 0;
    while ( j < fnd.size() && *(itr + j) == fnd[j] ) ++j;
    if ( j == fnd.size() ) return true;
  }
  return false;
}

inline bool
contains(const char *data, usize len, const char fnd)
{
  if ( data == nullptr || len == 0 ) return false;
  for ( usize i = 0; i < len; ++i )
    if ( data[i] == fnd ) return true;
  return false;
}

inline bool
contains(const char *data, usize len, const char *fnd)
{
  if ( data == nullptr || fnd == nullptr ) return false;
  usize sz = micron::strlen(fnd);
  if ( sz == 0 || sz > len ) return false;
  for ( usize i = 0; i <= len - sz; ++i ) {
    usize j = 0;
    while ( j < sz && data[i + j] == fnd[j] ) ++j;
    if ( j == sz ) return true;
  }
  return false;
}

template <usize N, usize M>
inline bool
contains(const char (&data)[N], const char (&fnd)[M])
{
  return contains(static_cast<const char *>(data), N - 1, static_cast<const char *>(fnd));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// find

template <is_string T>
auto
find(const T &data, const char fnd) -> typename T::const_iterator
{
  if ( data.empty() ) return nullptr;
  for ( auto itr = data.begin(); itr != data.end(); ++itr )
    if ( *itr == fnd ) return itr;
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(T &data, const char *fnd) -> typename T::iterator
{
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = data.begin(); itr + sz <= data.end(); ++itr ) {
    u64 j = 0;
    while ( j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, const char *fnd) -> typename T::const_iterator
{
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = data.begin(); itr + sz <= data.end(); ++itr ) {
    u64 j = 0;
    while ( j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
  }
  return (typename T::iterator) nullptr;
}

const char *
find(const char *data, const char *end, const char fnd)
{
  if ( data < end || data >= end || !end || !data ) return nullptr;
  for ( auto itr = data; itr != end; ++itr )
    if ( *itr == fnd ) return itr;
  return nullptr;
}

template <is_string T>
auto
find(const T &data, typename T::const_iterator from, const char fnd) -> typename T::const_iterator
{
  if ( from < data.begin() || from >= data.end() || !from || data.empty() ) return nullptr;
  for ( auto itr = from; itr != data.end(); ++itr )
    if ( *itr == fnd ) return itr;
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, typename T::iterator from, const T &fnd) -> typename T::iterator
{
  if ( from < data.begin() || from >= data.end() || !from ) return nullptr;
  usize sz = fnd.size();
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = from; itr + sz <= data.end(); ++itr ) {
    u64 j = 0;
    while ( j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, typename T::iterator from, const char *fnd) -> typename T::iterator
{
  if ( from < data.begin() || from >= data.end() || !from ) return nullptr;
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = from; itr + sz <= data.end(); ++itr ) {
    u64 j = 0;
    while ( j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, const T &fnd) -> typename T::iterator
{
  if ( fnd.size() > data.size() || data.empty() || fnd.empty() ) return nullptr;
  for ( auto itr = data.begin(); itr + fnd.size() <= data.end(); ++itr ) {
    u64 j = 0;
    while ( j < fnd.size() && *(itr + j) == fnd[j] ) ++j;
    if ( j == fnd.size() ) return itr;
  }
  return nullptr;
}

inline const char *
find(const char *data, usize len, const char fnd)
{
  if ( data == nullptr || len == 0 ) return nullptr;
  for ( usize i = 0; i < len; ++i )
    if ( data[i] == fnd ) return data + i;
  return nullptr;
}

inline const char *
find(const char *data, usize len, const char *fnd)
{
  if ( data == nullptr || fnd == nullptr ) return nullptr;
  usize sz = micron::strlen(fnd);
  if ( sz == 0 || sz > len ) return nullptr;
  for ( usize i = 0; i <= len - sz; ++i ) {
    u64 j = 0;
    while ( j < sz && data[i + j] == fnd[j] ) ++j;
    if ( j == sz ) return data + i;
  }
  return nullptr;
}

template <usize N>
inline const char *
find(const char (&data)[N], const char fnd)
{
  return find(static_cast<const char *>(data), N - 1, fnd);
}

template <usize N, usize M>
inline const char *
find(const char (&data)[N], const char (&fnd)[M])
{
  return find(static_cast<const char *>(data), N - 1, static_cast<const char *>(fnd));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// find_reverse

template <is_string T>
auto
find_reverse(const T &data, typename T::const_iterator from, const char fnd) -> typename T::const_iterator
{
  if ( from < data.begin() || from >= data.end() || !from ) return nullptr;
  for ( auto itr = from;; --itr ) {
    if ( *(itr) == fnd ) return itr;
    if ( itr == data.begin() ) break;
  }
  return (typename T::const_iterator) nullptr;
}

template <is_string T>
auto
find_reverse(const T &data, typename T::const_iterator from, const T &fnd) -> typename T::const_iterator
{
  if ( from < data.begin() || from >= data.end() || !from ) return nullptr;
  usize sz = fnd.size();
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = from;; --itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
    if ( itr == data.begin() ) break;
  }
  return (typename T::const_iterator) nullptr;
}

template <is_string T>
auto
find_reverse(const T &data, typename T::const_iterator from, const char *fnd) -> typename T::const_iterator
{
  if ( from < data.begin() || from >= data.end() || !from ) return nullptr;
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = from;; --itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
    if ( itr == data.begin() ) break;
  }
  return (typename T::const_iterator) nullptr;
}

template <is_string T>
auto
find_reverse(const T &data, typename T::iterator from, const T &fnd) -> typename T::iterator
{
  if ( from < data.begin() || from >= data.end() || !from ) return nullptr;
  usize sz = fnd.size();
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = from;; --itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
    if ( itr == data.begin() ) break;
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find_reverse(const T &data, typename T::iterator from, const char *fnd) -> typename T::iterator
{
  if ( from < data.begin() || from >= data.end() || !from ) return nullptr;
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() || sz == 0 ) return nullptr;
  for ( auto itr = from;; --itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j < sz && *(itr + j) == fnd[j] ) ++j;
    if ( j == sz ) return itr;
    if ( itr == data.begin() ) break;
  }
  return (typename T::iterator) nullptr;
}

inline const char *
find_reverse(const char *data, usize len, usize from, const char fnd)
{
  if ( data == nullptr || from >= len ) return nullptr;
  for ( usize i = from + 1; i-- > 0; )
    if ( data[i] == fnd ) return data + i;
  return nullptr;
}

inline const char *
find_reverse(const char *data, usize len, usize from, const char *fnd)
{
  if ( data == nullptr || fnd == nullptr ) return nullptr;
  usize sz = micron::strlen(fnd);
  if ( sz == 0 || sz > len || from >= len ) return nullptr;
  usize effective = (from <= len - sz) ? from : len - sz;
  for ( usize i = effective + 1; i-- > 0; ) {
    u64 j = 0;
    while ( j < sz && data[i + j] == fnd[j] ) ++j;
    if ( j == sz ) return data + i;
  }
  return nullptr;
}

template <usize N>
inline const char *
find_reverse(const char (&data)[N], usize from, const char fnd)
{
  return find_reverse(static_cast<const char *>(data), N - 1, from, fnd);
}

template <usize N, usize M>
inline const char *
find_reverse(const char (&data)[N], usize from, const char (&fnd)[M])
{
  return find_reverse(static_cast<const char *>(data), N - 1, from, static_cast<const char *>(fnd));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// is_in / is_not_in

template <is_string T>
bool
is_not_in(const char *fnd, const T &data)
{
  return find(data, fnd) == nullptr;
}

template <is_string T>
bool
is_not_in(const T &data, const char *fnd)
{
  return find(data, fnd) == nullptr;
}

template <is_string T>
bool
is_not_in(const T &data, const T &fnd)
{
  return find(data, fnd) == nullptr;
}

template <is_string T>
bool
is_in(const char *fnd, const T &data)
{
  return find(data, fnd) != nullptr;
}

template <is_string T>
bool
is_in(const T &data, const char *fnd)
{
  return find(data, fnd) != nullptr;
}

template <is_string T>
bool
is_in(const T &data, const T &fnd)
{
  return find(data, fnd) != nullptr;
}

inline bool
is_not_in(const char *data, usize len, const char *fnd)
{
  return find(data, len, fnd) == nullptr;
}

inline bool
is_not_in(const char *data, usize len, const char fnd)
{
  return find(data, len, fnd) == nullptr;
}

inline bool
is_in(const char *data, usize len, const char *fnd)
{
  return find(data, len, fnd) != nullptr;
}

inline bool
is_in(const char *data, usize len, const char fnd)
{
  return find(data, len, fnd) != nullptr;
}

template <usize N, usize M>
inline bool
is_not_in(const char (&data)[N], const char (&fnd)[M])
{
  return find(data, fnd) == nullptr;
}

template <usize N, usize M>
inline bool
is_in(const char (&data)[N], const char (&fnd)[M])
{
  return find(data, fnd) != nullptr;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// replace / replace_all

template <is_string T>
T &
replace(T &str, const char *lhs, const char *rhs)
{
  auto sz = str.size();
  auto szl = micron::strlen(lhs);
  auto szr = micron::strlen(rhs);
  if ( szr > szl && (sz - szl + szr) > str.max_size() ) exc<except::library_error>("replace range error.");
  typename T::iterator itr = find(str, lhs);
  if ( itr == nullptr ) return str;
  usize pos = static_cast<usize>(itr - str.begin());
  usize tail_len = sz - pos - szl;
  micron::bytemove(itr + szr, itr + szl, tail_len);
  micron::bytecpy(itr, rhs, szr);
  str.set_size(sz - szl + szr);
  return str;
}

template <is_string T>
T
replace(const T &str, const char *lhs, const char *rhs)
{
  T result(str);
  return replace(result, lhs, rhs);
}

inline char *
replace(char *str, usize &len, const char *lhs, const char *rhs)
{
  if ( str == nullptr || lhs == nullptr || rhs == nullptr ) return str;
  usize szl = micron::strlen(lhs);
  usize szr = micron::strlen(rhs);
  if ( szl == 0 || szl > len ) return str;
  char *itr = nullptr;
  for ( usize i = 0; i <= len - szl; ++i ) {
    u64 j = 0;
    while ( j < szl && str[i + j] == lhs[j] ) ++j;
    if ( j == szl ) {
      itr = str + i;
      break;
    }
  }
  if ( itr == nullptr ) return str;
  usize pos = static_cast<usize>(itr - str);
  usize tail_len = len - pos - szl;
  micron::bytemove(itr + szr, itr + szl, tail_len);
  micron::bytecpy(itr, rhs, szr);
  len = len - szl + szr;
  str[len] = 0;
  return str;
}

template <usize N, usize M, usize R>
inline hstring<schar>
replace(const char (&str)[N], const char (&lhs)[M], const char (&rhs)[R])
{
  hstring<schar> result(str);
  return replace(result, static_cast<const char *>(lhs), static_cast<const char *>(rhs));
}

template <is_string T>
T &
replace_all(T &str, const char *lhs, const char *rhs)
{
  if ( micron::strcmp(lhs, rhs) == 0 ) return str;
  auto szl = micron::strlen(lhs);
  auto szr = micron::strlen(rhs);
  typename T::iterator itr = find(str, lhs);
  while ( itr != nullptr ) {
    auto sz = str.size();
    if ( szr > szl && (sz - szl + szr) > str.max_size() ) exc<except::library_error>("replace_all range error.");
    usize pos = static_cast<usize>(itr - str.begin());
    usize tail_len = sz - pos - szl;
    micron::bytemove(itr + szr, itr + szl, tail_len);
    micron::bytecpy(itr, rhs, szr);
    str.set_size(sz - szl + szr);
    itr = find(str, str.begin() + pos + szr, lhs);
  }
  return str;
}

template <char N, is_string T>
T &
replace_all(T &str, const char *lhs)
{
  auto sz = str.size();
  auto szl = micron::strlen(lhs);
  if ( (szl) + sz > str.max_size() ) exc<except::library_error>("concat range error.");
  typename T::iterator itr = find(str, lhs);
  while ( itr != nullptr ) {
    sz = str.size();
    micron::memset(itr, N, 1);
    micron::bytemove(itr + 1, itr + szl, sz - ((1 - szl) > 0 ? (1 - szl) : 0));
    str.set_size(str.size() - math::abs(szl - 1));
    usize pos = static_cast<usize>(itr - str.begin());
    itr = find(str, str.begin() + pos + 1, lhs);
  }
  return str;
}

template <is_string T>
T
replace_all(const T &str, const char *lhs, const char *rhs)
{
  T result(str);
  return replace_all(result, lhs, rhs);
}

inline char *
replace_all(char *str, usize &len, const char *lhs, const char *rhs)
{
  if ( micron::strcmp(lhs, rhs) == 0 || str == nullptr || lhs == nullptr || rhs == nullptr ) return str;
  usize szl = micron::strlen(lhs);
  usize szr = micron::strlen(rhs);
  if ( szl == 0 ) return str;
  usize pos = 0;
  while ( pos + szl <= len ) {
    u64 j = 0;
    while ( j < szl && str[pos + j] == lhs[j] ) ++j;
    if ( j == szl ) {
      usize tail_len = len - pos - szl;
      micron::bytemove(str + pos + szr, str + pos + szl, tail_len);
      micron::bytecpy(str + pos, rhs, szr);
      len = len - szl + szr;
      str[len] = 0;
      pos += szr;
    } else
      ++pos;
  }
  return str;
}

template <usize N, usize M, usize R>
inline hstring<schar>
replace_all(const char (&str)[N], const char (&lhs)[M], const char (&rhs)[R])
{
  hstring<schar> result(str);
  return replace_all(result, static_cast<const char *>(lhs), static_cast<const char *>(rhs));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// string-to-numeric (to_float, to_double, to_integer, to_long)

template <typename T>
f32
to_float(const T &o)
{
  const char *p = reinterpret_cast<const char *>(o.cdata());
  // skip leading whitespace
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  // integer part
  u64 ipart = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) ipart = ipart * 10 + (*p++ - '0');
  // fractional part
  u64 frac = 0;
  usize fdigits = 0;
  if ( *p == '.' ) {
    ++p;
    while ( static_cast<u32>(*p - '0') <= 9 ) {
      if ( fdigits < __impl::__max_frac_digits_f32 ) {
        frac = frac * 10 + (*p - '0');
        ++fdigits;
      }
      ++p;
    }
  }
  f32 result = static_cast<f32>(ipart);
  if ( fdigits > 0 ) result += static_cast<f32>(static_cast<f64>(frac) / __impl::__pow10_tbl[fdigits]);
  return neg ? -result : result;
}

inline f32
to_float(const char *buf)
{
  if ( buf == nullptr ) return 0.0f;
  const char *p = buf;
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  u64 ipart = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) ipart = ipart * 10 + (*p++ - '0');
  u64 frac = 0;
  usize fdigits = 0;
  if ( *p == '.' ) {
    ++p;
    while ( static_cast<u32>(*p - '0') <= 9 ) {
      if ( fdigits < __impl::__max_frac_digits_f32 ) {
        frac = frac * 10 + (*p - '0');
        ++fdigits;
      }
      ++p;
    }
  }
  f32 result = static_cast<f32>(ipart);
  if ( fdigits > 0 ) result += static_cast<f32>(static_cast<f64>(frac) / __impl::__pow10_tbl[fdigits]);
  return neg ? -result : result;
}

inline f32
to_float(const char *buf, usize len)
{
  if ( buf == nullptr || len == 0 ) return 0.0f;
  const char *p = buf;
  const char *end = buf + len;
  while ( p < end && (*p == ' ' || *p == '\t') ) ++p;
  bool neg = false;
  if ( p < end && *p == '-' ) {
    neg = true;
    ++p;
  } else if ( p < end && *p == '+' )
    ++p;
  u64 ipart = 0;
  while ( p < end && static_cast<u32>(*p - '0') <= 9 ) ipart = ipart * 10 + (*p++ - '0');
  u64 frac = 0;
  usize fdigits = 0;
  if ( p < end && *p == '.' ) {
    ++p;
    while ( p < end && static_cast<u32>(*p - '0') <= 9 ) {
      if ( fdigits < __impl::__max_frac_digits_f32 ) {
        frac = frac * 10 + (*p - '0');
        ++fdigits;
      }
      ++p;
    }
  }
  f32 result = static_cast<f32>(ipart);
  if ( fdigits > 0 ) result += static_cast<f32>(static_cast<f64>(frac) / __impl::__pow10_tbl[fdigits]);
  return neg ? -result : result;
}

template <usize N>
inline f32
to_float(const char (&buf)[N])
{
  return to_float(static_cast<const char *>(buf), N - 1);
}

template <typename T>
f64
to_double(const T &o)
{
  const char *p = reinterpret_cast<const char *>(o.cdata());
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  u64 ipart = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) ipart = ipart * 10 + (*p++ - '0');
  u64 frac = 0;
  usize fdigits = 0;
  if ( *p == '.' ) {
    ++p;
    while ( static_cast<u32>(*p - '0') <= 9 ) {
      if ( fdigits < __impl::__max_frac_digits_f64 ) {
        frac = frac * 10 + (*p - '0');
        ++fdigits;
      }
      ++p;
    }
  }
  f64 result = static_cast<f64>(ipart);
  if ( fdigits > 0 ) result += static_cast<f64>(frac) / __impl::__pow10_tbl[fdigits];
  return neg ? -result : result;
}

inline f64
to_double(const char *buf)
{
  if ( buf == nullptr ) return 0.0;
  const char *p = buf;
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  u64 ipart = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) ipart = ipart * 10 + (*p++ - '0');
  u64 frac = 0;
  usize fdigits = 0;
  if ( *p == '.' ) {
    ++p;
    while ( static_cast<u32>(*p - '0') <= 9 ) {
      if ( fdigits < __impl::__max_frac_digits_f64 ) {
        frac = frac * 10 + (*p - '0');
        ++fdigits;
      }
      ++p;
    }
  }
  f64 result = static_cast<f64>(ipart);
  if ( fdigits > 0 ) result += static_cast<f64>(frac) / __impl::__pow10_tbl[fdigits];
  return neg ? -result : result;
}

inline f64
to_double(const char *buf, usize len)
{
  if ( buf == nullptr || len == 0 ) return 0.0;
  const char *p = buf;
  const char *end = buf + len;
  while ( p < end && (*p == ' ' || *p == '\t') ) ++p;
  bool neg = false;
  if ( p < end && *p == '-' ) {
    neg = true;
    ++p;
  } else if ( p < end && *p == '+' )
    ++p;
  u64 ipart = 0;
  while ( p < end && static_cast<u32>(*p - '0') <= 9 ) ipart = ipart * 10 + (*p++ - '0');
  u64 frac = 0;
  usize fdigits = 0;
  if ( p < end && *p == '.' ) {
    ++p;
    while ( p < end && static_cast<u32>(*p - '0') <= 9 ) {
      if ( fdigits < __impl::__max_frac_digits_f64 ) {
        frac = frac * 10 + (*p - '0');
        ++fdigits;
      }
      ++p;
    }
  }
  f64 result = static_cast<f64>(ipart);
  if ( fdigits > 0 ) result += static_cast<f64>(frac) / __impl::__pow10_tbl[fdigits];
  return neg ? -result : result;
}

template <usize N>
inline f64
to_double(const char (&buf)[N])
{
  return to_double(static_cast<const char *>(buf), N - 1);
}

template <typename T>
i32
to_integer(const T &o)
{
  const char *p = reinterpret_cast<const char *>(o.cdata());
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  u32 acc = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) acc = acc * 10 + (*p++ - '0');
  return neg ? -static_cast<i32>(acc) : static_cast<i32>(acc);
}

inline i32
to_integer(const char *buf)
{
  if ( buf == nullptr ) return 0;
  const char *p = buf;
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  u32 acc = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) acc = acc * 10 + (*p++ - '0');
  return neg ? -static_cast<i32>(acc) : static_cast<i32>(acc);
}

inline i32
to_integer(const char *buf, usize len)
{
  if ( buf == nullptr || len == 0 ) return 0;
  const char *p = buf;
  const char *end = buf + len;
  while ( p < end && (*p == ' ' || *p == '\t') ) ++p;
  bool neg = false;
  if ( p < end && *p == '-' ) {
    neg = true;
    ++p;
  } else if ( p < end && *p == '+' )
    ++p;
  u32 acc = 0;
  while ( p < end && static_cast<u32>(*p - '0') <= 9 ) acc = acc * 10 + (*p++ - '0');
  return neg ? -static_cast<i32>(acc) : static_cast<i32>(acc);
}

template <usize N>
inline i32
to_integer(const char (&buf)[N])
{
  return to_integer(static_cast<const char *>(buf), N - 1);
}

constexpr int
xdigit_to_val(char c) noexcept
{
  if ( c >= '0' && c <= '9' ) return c - '0';
  if ( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
  if ( c >= 'A' && c <= 'F' ) return c - 'A' + 10;
  return -1;
}

constexpr u32
parse_decimal(const char *&p) noexcept
{
  u32 val = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) {
    val = val * 10 + (*p - '0');
    ++p;
  }
  return val;
}

constexpr u32
parse_hex(const char *&p) noexcept
{
  u32 val = 0;
  int d;
  while ( (d = xdigit_to_val(*p)) >= 0 ) {
    val = val * 16 + d;
    ++p;
  }
  return val;
}

constexpr int
parse_hex_byte(const char *&p) noexcept
{
  int d = xdigit_to_val(*p);
  if ( d < 0 ) return -1;
  ++p;
  int d2 = xdigit_to_val(*p);
  if ( d2 >= 0 ) {
    d = (d << 4) | d2;
    ++p;
  }
  return d;
}

constexpr u32
parse_octal(const char *&p) noexcept
{
  u32 val = 0;
  while ( static_cast<u32>(*p - '0') <= 7 ) {
    val = val * 8 + (*p - '0');
    ++p;
  }
  return val;
}

constexpr u8
to_hex_char(u8 nibble) noexcept
{
  return nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
}

template <typename R, is_string T>
R *
to_pointer_addr(typename T::iterator start, typename T::iterator end, u32 base = 16)
{
  const char *p = reinterpret_cast<const char *>(start);
  const char *pe = reinterpret_cast<const char *>(end);
  while ( p < pe && (*p == ' ' || *p == '\t') ) ++p;
  if ( base == 16 && p + 1 < pe && p[0] == '0' && (p[1] == 'x' || p[1] == 'X') ) p += 2;
  u64 result = 0;
  while ( p < pe ) {
    u32 digit;
    char ch = *p;
    if ( static_cast<u32>(ch - '0') <= 9 )
      digit = ch - '0';
    else if ( base > 10 && static_cast<u32>(ch - 'A') <= 5 )
      digit = ch - 'A' + 10;
    else if ( base > 10 && static_cast<u32>(ch - 'a') <= 5 )
      digit = ch - 'a' + 10;
    else
      break;
    if ( digit >= base ) break;
    result = result * base + digit;
    ++p;
  }
  return reinterpret_cast<R *>(reinterpret_cast<u64 *>(result));
}

template <typename T>
i64
to_long(const T &o)
{
  const char *p = reinterpret_cast<const char *>(o.cdata());
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  u64 acc = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) acc = acc * 10 + (*p++ - '0');
  return neg ? -static_cast<i64>(acc) : static_cast<i64>(acc);
}

// to_long: null-terminated c-string
inline i64
to_long(const char *buf)
{
  if ( buf == nullptr ) return 0;
  const char *p = buf;
  while ( *p == ' ' || *p == '\t' ) ++p;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  } else if ( *p == '+' )
    ++p;
  u64 acc = 0;
  while ( static_cast<u32>(*p - '0') <= 9 ) acc = acc * 10 + (*p++ - '0');
  return neg ? -static_cast<i64>(acc) : static_cast<i64>(acc);
}

// to_long: bounded c-string
inline i64
to_long(const char *buf, usize len)
{
  if ( buf == nullptr || len == 0 ) return 0;
  const char *p = buf;
  const char *end = buf + len;
  while ( p < end && (*p == ' ' || *p == '\t') ) ++p;
  bool neg = false;
  if ( p < end && *p == '-' ) {
    neg = true;
    ++p;
  } else if ( p < end && *p == '+' )
    ++p;
  u64 acc = 0;
  while ( p < end && static_cast<u32>(*p - '0') <= 9 ) acc = acc * 10 + (*p++ - '0');
  return neg ? -static_cast<i64>(acc) : static_cast<i64>(acc);
}

template <usize N>
inline i64
to_long(const char (&buf)[N])
{
  return to_long(static_cast<const char *>(buf), N - 1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pad / truncate / precision

template <is_string T>
T
pad_left(const T &str, usize width, char fill = ' ')
{
  if ( str.size() >= width ) return str;
  T result;
  for ( usize i = 0; i < width - str.size(); ++i ) result += fill;
  result += str;
  return result;
}

template <is_string T>
T
pad_right(const T &str, usize width, char fill = ' ')
{
  if ( str.size() >= width ) return str;
  T result(str);
  for ( usize i = 0; i < width - str.size(); ++i ) result += fill;
  return result;
}

template <is_string T>
T
pad_center(const T &str, usize width, char fill = ' ')
{
  if ( str.size() >= width ) return str;
  T result;
  usize pad = width - str.size();
  usize left = pad / 2;
  for ( usize i = 0; i < left; ++i ) result += fill;
  result += str;
  for ( usize i = 0; i < pad - left; ++i ) result += fill;
  return result;
}

inline hstring<schar>
pad_left(const char *str, usize width, char fill = ' ')
{
  hstring<schar> s(str);
  return pad_left(s, width, fill);
}

inline hstring<schar>
pad_right(const char *str, usize width, char fill = ' ')
{
  hstring<schar> s(str);
  return pad_right(s, width, fill);
}

inline hstring<schar>
pad_center(const char *str, usize width, char fill = ' ')
{
  hstring<schar> s(str);
  return pad_center(s, width, fill);
}

template <is_string T>
T
truncate(const T &str, usize width)
{
  if ( str.size() <= width ) return str;
  return str.substr(0, width);
}

template <is_string T>
T
limit(const T &str, usize width)
{
  return truncate(str, width);
}

inline hstring<schar>
truncate(const char *str, usize width)
{
  hstring<schar> s(str);
  return truncate(s, width);
}

inline hstring<schar>
limit(const char *str, usize width)
{
  return truncate(str, width);
}

inline hstring<schar>
precision(f64 value, u32 digits)
{
  char buf[__impl::__fmt_buf_size];
  usize n = micron::__impl::__ryu::d2f_buffered(value, buf, __impl::__fmt_buf_size, digits);
  return hstring<schar>(buf, buf + n);
}

inline hstring<schar>
precision(f32 value, u32 digits)
{
  return precision(static_cast<f64>(value), digits);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// join

template <typename Container> hstring<schar> join(const Container &items, const char *delimiter);
template <typename Container> hstring<schar> join(const Container &items, const char *delimiter, const char *prefix, const char *suffix);
template <typename Container, is_string Delim> hstring<schar> join(const Container &items, const Delim &delimiter);
template <is_string T> hstring<schar> join_strings(const micron::fvector<T> &items, const char *delimiter);

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// capitalize / title_case / swap_case

template <is_string T>
T
capitalize(const T &str)
{
  if ( str.empty() ) return str;
  T result(str);
  auto itr = result.begin();
  while ( itr != result.end() ) {
    if ( isupper<typename T::value_type>(*itr) ) *itr = to_lower<typename T::value_type>(*itr);
    ++itr;
  }
  if ( islower<typename T::value_type>(*result.begin()) ) *result.begin() = to_upper<typename T::value_type>(*result.begin());
  return result;
}

inline hstring<schar>
capitalize(const char *str)
{
  hstring<schar> s(str);
  return capitalize(s);
}

inline hstring<schar>
capitalize(const char *str, usize len)
{
  hstring<schar> s(str);
  if ( len < s.size() ) s.truncate(len);
  return capitalize(s);
}

template <usize N>
inline hstring<schar>
capitalize(const char (&str)[N])
{
  hstring<schar> s(str);
  return capitalize(s);
}

template <is_string T>
T
title_case(const T &str)
{
  if ( str.empty() ) return str;
  T result(str);
  bool word_start = true;
  for ( auto itr = result.begin(); itr != result.end(); ++itr ) {
    if ( isspace<typename T::value_type>(*itr) || ispunct<typename T::value_type>(*itr) )
      word_start = true;
    else if ( word_start ) {
      if ( islower<typename T::value_type>(*itr) ) *itr = to_upper<typename T::value_type>(*itr);
      word_start = false;
    } else {
      if ( isupper<typename T::value_type>(*itr) ) *itr = to_lower<typename T::value_type>(*itr);
    }
  }
  return result;
}

inline hstring<schar>
title_case(const char *str)
{
  hstring<schar> s(str);
  return title_case(s);
}

inline hstring<schar>
title_case(const char *str, usize len)
{
  hstring<schar> s(str);
  if ( len < s.size() ) s.truncate(len);
  return title_case(s);
}

template <usize N>
inline hstring<schar>
title_case(const char (&str)[N])
{
  hstring<schar> s(str);
  return title_case(s);
}

template <is_string T>
T
swap_case(const T &str)
{
  if ( str.empty() ) return str;
  T result(str);
  for ( auto itr = result.begin(); itr != result.end(); ++itr ) {
    if ( isupper<typename T::value_type>(*itr) )
      *itr = to_lower<typename T::value_type>(*itr);
    else if ( islower<typename T::value_type>(*itr) )
      *itr = to_upper<typename T::value_type>(*itr);
  }
  return result;
}

inline hstring<schar>
swap_case(const char *str)
{
  hstring<schar> s(str);
  return swap_case(s);
}

inline hstring<schar>
swap_case(const char *str, usize len)
{
  hstring<schar> s(str);
  if ( len < s.size() ) s.truncate(len);
  return swap_case(s);
}

template <usize N>
inline hstring<schar>
swap_case(const char (&str)[N])
{
  hstring<schar> s(str);
  return swap_case(s);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// repeat

template <is_string T>
T
repeat(const T &str, usize count)
{
  T result;
  for ( usize i = 0; i < count; ++i ) result += str;
  return result;
}

inline hstring<schar>
repeat(const char *str, usize count)
{
  hstring<schar> result;
  for ( usize i = 0; i < count; ++i ) result += str;
  return result;
}

inline hstring<schar>
repeat(char c, usize count)
{
  hstring<schar> result;
  for ( usize i = 0; i < count; ++i ) result += c;
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// formatter traits

template <typename T, typename Enable = void> struct formatter;

template <> struct formatter<i32> {
  static inline usize
  write(char *buf, usize buf_sz, i32 val, const __impl::fmt_spec &spec)
  {
    u32 base = 10;
    bool upper = false;
    if ( spec.type == 'x' )
      base = 16;
    else if ( spec.type == 'X' ) {
      base = 16;
      upper = true;
    } else if ( spec.type == 'o' )
      base = 8;
    else if ( spec.type == 'b' )
      base = 2;
    usize off = 0;
    if ( spec.alt ) {
      if ( base == 16 ) {
        buf[0] = '0';
        buf[1] = upper ? 'X' : 'x';
        off = 2;
      } else if ( base == 8 ) {
        buf[0] = '0';
        off = 1;
      } else if ( base == 2 ) {
        buf[0] = '0';
        buf[1] = 'b';
        off = 2;
      }
    }
    return off + __impl::fmt_int_to_buf(buf + off, buf_sz - off, val, base, upper);
  }
};

template <> struct formatter<i64> {
  static inline usize
  write(char *buf, usize buf_sz, i64 val, const __impl::fmt_spec &spec)
  {
    u32 base = 10;
    bool upper = false;
    if ( spec.type == 'x' )
      base = 16;
    else if ( spec.type == 'X' ) {
      base = 16;
      upper = true;
    } else if ( spec.type == 'o' )
      base = 8;
    else if ( spec.type == 'b' )
      base = 2;
    usize off = 0;
    if ( spec.alt ) {
      if ( base == 16 ) {
        buf[0] = '0';
        buf[1] = upper ? 'X' : 'x';
        off = 2;
      } else if ( base == 8 ) {
        buf[0] = '0';
        off = 1;
      } else if ( base == 2 ) {
        buf[0] = '0';
        buf[1] = 'b';
        off = 2;
      }
    }
    return off + __impl::fmt_int_to_buf(buf + off, buf_sz - off, val, base, upper);
  }
};

template <> struct formatter<u32> {
  static inline usize
  write(char *buf, usize buf_sz, u32 val, const __impl::fmt_spec &spec)
  {
    u32 base = 10;
    bool upper = false;
    if ( spec.type == 'x' )
      base = 16;
    else if ( spec.type == 'X' ) {
      base = 16;
      upper = true;
    } else if ( spec.type == 'o' )
      base = 8;
    else if ( spec.type == 'b' )
      base = 2;
    usize off = 0;
    if ( spec.alt ) {
      if ( base == 16 ) {
        buf[0] = '0';
        buf[1] = upper ? 'X' : 'x';
        off = 2;
      } else if ( base == 8 ) {
        buf[0] = '0';
        off = 1;
      } else if ( base == 2 ) {
        buf[0] = '0';
        buf[1] = 'b';
        off = 2;
      }
    }
    return off + __impl::fmt_uint_to_buf(buf + off, buf_sz - off, val, base, upper);
  }
};

template <> struct formatter<u64> {
  static inline usize
  write(char *buf, usize buf_sz, u64 val, const __impl::fmt_spec &spec)
  {
    u32 base = 10;
    bool upper = false;
    if ( spec.type == 'x' )
      base = 16;
    else if ( spec.type == 'X' ) {
      base = 16;
      upper = true;
    } else if ( spec.type == 'o' )
      base = 8;
    else if ( spec.type == 'b' )
      base = 2;
    usize off = 0;
    if ( spec.alt ) {
      if ( base == 16 ) {
        buf[0] = '0';
        buf[1] = upper ? 'X' : 'x';
        off = 2;
      } else if ( base == 8 ) {
        buf[0] = '0';
        off = 1;
      } else if ( base == 2 ) {
        buf[0] = '0';
        buf[1] = 'b';
        off = 2;
      }
    }
    return off + __impl::fmt_uint_to_buf(buf + off, buf_sz - off, val, base, upper);
  }
};

template <> struct formatter<i16> {
  static inline usize
  write(char *buf, usize buf_sz, i16 val, const __impl::fmt_spec &spec)
  {
    return formatter<i32>::write(buf, buf_sz, static_cast<i32>(val), spec);
  }
};

template <> struct formatter<u16> {
  static inline usize
  write(char *buf, usize buf_sz, u16 val, const __impl::fmt_spec &spec)
  {
    return formatter<u32>::write(buf, buf_sz, static_cast<u32>(val), spec);
  }
};

template <> struct formatter<i8> {
  static inline usize
  write(char *buf, usize buf_sz, i8 val, const __impl::fmt_spec &spec)
  {
    return formatter<i32>::write(buf, buf_sz, static_cast<i32>(val), spec);
  }
};

template <> struct formatter<u8> {
  static inline usize
  write(char *buf, usize buf_sz, u8 val, const __impl::fmt_spec &spec)
  {
    return formatter<u32>::write(buf, buf_sz, static_cast<u32>(val), spec);
  }
};

template <> struct formatter<f32> {
  static inline usize
  write(char *buf, usize buf_sz, f32 val, const __impl::fmt_spec &spec)
  {
    u32 prec = spec.has_prec ? spec.prec : 6;
    return __impl::fmt_float_to_buf(buf, buf_sz, static_cast<f64>(val), prec);
  }
};

template <> struct formatter<f64> {
  static inline usize
  write(char *buf, usize buf_sz, f64 val, const __impl::fmt_spec &spec)
  {
    u32 prec = spec.has_prec ? spec.prec : 6;
    return __impl::fmt_float_to_buf(buf, buf_sz, val, prec);
  }
};

template <> struct formatter<bool> {
  static inline usize
  write(char *buf, usize buf_sz, bool val, const __impl::fmt_spec &)
  {
    return __impl::bool_to_buf(buf, buf_sz, val);
  }
};

template <> struct formatter<char> {
  static inline usize
  write(char *buf, usize buf_sz, char val, const __impl::fmt_spec &)
  {
    if ( buf_sz == 0 ) return 0;
    buf[0] = val;
    return 1;
  }
};

template <> struct formatter<const char *> {
  static inline usize
  write(char *buf, usize buf_sz, const char *val, const __impl::fmt_spec &spec)
  {
    if ( val == nullptr ) {
      if ( buf_sz < 6 ) return 0;
      buf[0] = '(';
      buf[1] = 'n';
      buf[2] = 'u';
      buf[3] = 'l';
      buf[4] = 'l';
      buf[5] = ')';
      return 6;
    }
    usize len = micron::strlen(val);
    if ( spec.has_prec && spec.prec < len ) len = spec.prec;
    if ( len > buf_sz ) len = buf_sz;
    micron::bytecpy(buf, val, len);
    return len;
  }
};

template <> struct formatter<char *> {
  static inline usize
  write(char *buf, usize buf_sz, char *val, const __impl::fmt_spec &spec)
  {
    return formatter<const char *>::write(buf, buf_sz, val, spec);
  }
};

template <typename T> struct formatter<T, micron::enable_if_t<micron::is_string_v<T>>> {
  static inline usize
  write(char *buf, usize buf_sz, const T &val, const __impl::fmt_spec &spec)
  {
    usize len = val.size();
    if ( spec.has_prec && spec.prec < len ) len = spec.prec;
    if ( len > buf_sz ) len = buf_sz;
    micron::bytecpy(buf, val.begin(), len);
    return len;
  }
};

template <typename T> struct formatter<T *, micron::enable_if_t<!micron::is_same_v<T, char> && !micron::is_same_v<T, const char>>> {
  static inline usize
  write(char *buf, usize buf_sz, T *val, const __impl::fmt_spec &)
  {
    return __impl::ptr_to_buf(buf, buf_sz, static_cast<const void *>(val));
  }
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// format_value

template <typename T>
inline hstring<schar>
format_value(const T &val)
{
  char buf[__impl::__fmt_buf_size];
  __impl::fmt_spec spec{};
  usize n = formatter<T>::write(buf, __impl::__fmt_buf_size, val, spec);
  return hstring<schar>(buf, buf + n);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// join (definitions, uses format_value)

template <typename Container>
hstring<schar>
join(const Container &items, const char *delimiter)
{
  hstring<schar> result;
  usize delim_len = micron::strlen(delimiter);
  bool first = true;
  for ( auto itr = items.begin(); itr != items.end(); ++itr ) {
    if ( !first ) result.append(delimiter, delim_len);
    result += format_value(*itr);
    first = false;
  }
  return result;
}

template <typename Container>
hstring<schar>
join(const Container &items, const char *delimiter, const char *prefix, const char *suffix)
{
  hstring<schar> result;
  result += prefix;
  result += join(items, delimiter);
  result += suffix;
  return result;
}

template <typename Container, is_string Delim>
hstring<schar>
join(const Container &items, const Delim &delimiter)
{
  hstring<schar> result;
  bool first = true;
  for ( auto itr = items.begin(); itr != items.end(); ++itr ) {
    if ( !first ) result += delimiter;
    result += format_value(*itr);
    first = false;
  }
  return result;
}

template <is_string T>
hstring<schar>
join_strings(const micron::fvector<T> &items, const char *delimiter)
{
  hstring<schar> result;
  usize delim_len = micron::strlen(delimiter);
  bool first = true;
  for ( usize i = 0; i < items.size(); ++i ) {
    if ( !first ) result.append(delimiter, delim_len);
    result += items[i];
    first = false;
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// format engine

namespace __impl
{

inline void
format_one(hstring<schar> &, const char *, const char *, usize)
{
  // base case: no args left
}

template <typename T, typename... Rest>
inline void
format_one(hstring<schar> &out, const char *spec_start, const char *spec_end, usize arg_index, const T &val, const Rest &...rest)
{
  if ( arg_index > 0 ) {
    format_one(out, spec_start, spec_end, arg_index - 1, rest...);
    return;
  }
  fmt_spec spec = parse_spec(spec_start, spec_end);
  char buf[__fmt_buf_size];
  usize n = formatter<T>::write(buf, __fmt_buf_size, val, spec);
  apply_padding(out, buf, n, spec);
}

}     // namespace __impl

template <typename... Args>
inline hstring<schar>
format(const char *fmt, const Args &...args)
{
  hstring<schar> out;
  if ( fmt == nullptr ) return out;
  usize auto_index = 0;
  const char *p = fmt;
  while ( *p ) {
    if ( *p == '{' ) {
      if ( *(p + 1) == '{' ) {
        out += '{';
        p += 2;
        continue;
      }
      ++p;
      const char *close = p;
      while ( *close && *close != '}' ) ++close;
      if ( *close != '}' ) break;
      const char *colon = p;
      while ( colon < close && *colon != ':' ) ++colon;
      usize index = auto_index;
      bool has_explicit_index = false;
      if ( colon > p ) {
        const char *num_p = p;
        bool all_digits = true;
        while ( num_p < colon ) {
          if ( *num_p < '0' || *num_p > '9' ) {
            all_digits = false;
            break;
          }
          ++num_p;
        }
        if ( all_digits ) {
          index = 0;
          num_p = p;
          while ( num_p < colon ) {
            index = index * 10 + (*num_p - '0');
            ++num_p;
          }
          has_explicit_index = true;
        }
      }
      const char *spec_start = (colon < close) ? colon + 1 : close;
      const char *spec_end = close;
      __impl::format_one(out, spec_start, spec_end, index, args...);
      if ( !has_explicit_index )
        ++auto_index;
      else
        auto_index = index + 1;
      p = close + 1;
    } else if ( *p == '}' && *(p + 1) == '}' ) {
      out += '}';
      p += 2;
    } else {
      out += *p;
      ++p;
    }
  }
  return out;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// count(data, sub [, start [, end]])

template <is_string T>
usize
count(const T &data, const char *sub, usize start = 0, usize end_pos = ~static_cast<usize>(0))
{
  usize sz = micron::strlen(sub);
  if ( sz == 0 || data.empty() || start >= data.size() ) return 0;
  if ( end_pos > data.size() ) end_pos = data.size();
  usize n = 0;
  for ( usize i = start; i + sz <= end_pos; ++i ) {
    usize j = 0;
    while ( j < sz && data[i + j] == sub[j] ) ++j;
    if ( j == sz ) {
      ++n;
      i += sz - 1;
    }
  }
  return n;
}

template <is_string T>
usize
count(const T &data, const T &sub, usize start = 0, usize end_pos = ~static_cast<usize>(0))
{
  if ( sub.empty() || data.empty() || start >= data.size() ) return 0;
  if ( end_pos > data.size() ) end_pos = data.size();
  usize n = 0;
  for ( usize i = start; i + sub.size() <= end_pos; ++i ) {
    usize j = 0;
    while ( j < sub.size() && data[i + j] == sub[j] ) ++j;
    if ( j == sub.size() ) {
      ++n;
      i += sub.size() - 1;
    }
  }
  return n;
}

template <is_string T>
usize
count(const T &data, char ch, usize start = 0, usize end_pos = ~static_cast<usize>(0))
{
  if ( data.empty() || start >= data.size() ) return 0;
  if ( end_pos > data.size() ) end_pos = data.size();
  usize n = 0;
  for ( usize i = start; i < end_pos; ++i )
    if ( data[i] == ch ) ++n;
  return n;
}

inline usize
count(const char *data, usize len, const char *sub)
{
  usize sz = micron::strlen(sub);
  if ( sz == 0 || len == 0 ) return 0;
  usize n = 0;
  for ( usize i = 0; i + sz <= len; ++i ) {
    usize j = 0;
    while ( j < sz && data[i + j] == sub[j] ) ++j;
    if ( j == sz ) {
      ++n;
      i += sz - 1;
    }
  }
  return n;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// index / rindex (like find but throw on miss)

template <is_string T>
usize
index(const T &data, const char *fnd, usize start = 0)
{
  if ( start >= data.size() ) exc<except::library_error>("substring not found");
  for ( usize i = start; i + micron::strlen(fnd) <= data.size(); ++i ) {
    usize sz = micron::strlen(fnd);
    usize j = 0;
    while ( j < sz && data[i + j] == fnd[j] ) ++j;
    if ( j == sz ) return i;
  }
  exc<except::library_error>("substring not found");
  return ~static_cast<usize>(0);
}

template <is_string T>
usize
index(const T &data, const T &fnd, usize start = 0)
{
  if ( start >= data.size() || fnd.empty() ) exc<except::library_error>("substring not found");
  for ( usize i = start; i + fnd.size() <= data.size(); ++i ) {
    usize j = 0;
    while ( j < fnd.size() && data[i + j] == fnd[j] ) ++j;
    if ( j == fnd.size() ) return i;
  }
  exc<except::library_error>("substring not found");
  return ~static_cast<usize>(0);
}

template <is_string T>
usize
rindex(const T &data, const char *fnd, usize start = ~static_cast<usize>(0))
{
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() ) exc<except::library_error>("substring not found");
  usize from = (start > data.size() - sz) ? data.size() - sz : start;
  for ( usize i = from + 1; i-- > 0; ) {
    usize j = 0;
    while ( j < sz && data[i + j] == fnd[j] ) ++j;
    if ( j == sz ) return i;
  }
  exc<except::library_error>("substring not found");
  return ~static_cast<usize>(0);
}

template <is_string T>
usize
rindex(const T &data, const T &fnd, usize start = ~static_cast<usize>(0))
{
  if ( fnd.size() > data.size() || data.empty() ) exc<except::library_error>("substring not found");
  usize from = (start > data.size() - fnd.size()) ? data.size() - fnd.size() : start;
  for ( usize i = from + 1; i-- > 0; ) {
    usize j = 0;
    while ( j < fnd.size() && data[i + j] == fnd[j] ) ++j;
    if ( j == fnd.size() ) return i;
  }
  exc<except::library_error>("substring not found");
  return ~static_cast<usize>(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// positional find returning usize (npos on miss)

constexpr usize npos = ~static_cast<usize>(0);

template <is_string T>
usize
find_pos(const T &data, const char *fnd, usize start = 0)
{
  usize sz = micron::strlen(fnd);
  if ( sz == 0 || sz > data.size() || start >= data.size() ) return npos;
  for ( usize i = start; i + sz <= data.size(); ++i ) {
    usize j = 0;
    while ( j < sz && data[i + j] == fnd[j] ) ++j;
    if ( j == sz ) return i;
  }
  return npos;
}

template <is_string T>
usize
find_pos(const T &data, const T &fnd, usize start = 0)
{
  if ( fnd.empty() || fnd.size() > data.size() || start >= data.size() ) return npos;
  for ( usize i = start; i + fnd.size() <= data.size(); ++i ) {
    usize j = 0;
    while ( j < fnd.size() && data[i + j] == fnd[j] ) ++j;
    if ( j == fnd.size() ) return i;
  }
  return npos;
}

template <is_string T>
usize
find_pos(const T &data, char ch, usize start = 0)
{
  for ( usize i = start; i < data.size(); ++i )
    if ( data[i] == ch ) return i;
  return npos;
}

template <is_string T>
usize
rfind_pos(const T &data, const char *fnd, usize start = ~static_cast<usize>(0))
{
  usize sz = micron::strlen(fnd);
  if ( sz > data.size() || data.empty() ) return npos;
  usize from = (start > data.size() - sz) ? data.size() - sz : start;
  for ( usize i = from + 1; i-- > 0; ) {
    usize j = 0;
    while ( j < sz && data[i + j] == fnd[j] ) ++j;
    if ( j == sz ) return i;
  }
  return npos;
}

template <is_string T>
usize
rfind_pos(const T &data, const T &fnd, usize start = ~static_cast<usize>(0))
{
  if ( fnd.size() > data.size() || data.empty() ) return npos;
  usize from = (start > data.size() - fnd.size()) ? data.size() - fnd.size() : start;
  for ( usize i = from + 1; i-- > 0; ) {
    usize j = 0;
    while ( j < fnd.size() && data[i + j] == fnd[j] ) ++j;
    if ( j == fnd.size() ) return i;
  }
  return npos;
}

template <is_string T>
usize
rfind_pos(const T &data, char ch, usize start = ~static_cast<usize>(0))
{
  if ( data.empty() ) return npos;
  usize from = (start >= data.size()) ? data.size() - 1 : start;
  for ( usize i = from + 1; i-- > 0; )
    if ( data[i] == ch ) return i;
  return npos;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// additional is* predicates

template <is_string T>
bool
isdecimal_all(const T &str)
{
  // same as isdigit_all for ASCII
  return isdigit_all(str);
}

template <is_string T>
bool
isnumeric_all(const T &str)
{
  // in ASCII, numeric == digit
  return isdigit_all(str);
}

template <is_string T>
bool
isidentifier(const T &str)
{
  // C/C++ identifier: starts with alpha or _, then alnum or _
  if ( str.empty() ) return false;
  char first = str[0];
  if ( !((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || first == '_') ) return false;
  for ( usize i = 1; i < str.size(); ++i ) {
    char c = str[i];
    if ( !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') ) return false;
  }
  return true;
}

template <is_string T>
bool
istitle(const T &str)
{
  // every word starts with uppercase, rest lowercase
  if ( str.empty() ) return false;
  bool word_start = true;
  bool any_cased = false;
  for ( usize i = 0; i < str.size(); ++i ) {
    char c = str[i];
    bool up = (c >= 'A' && c <= 'Z');
    bool lo = (c >= 'a' && c <= 'z');
    if ( word_start ) {
      if ( lo ) return false;
      if ( up ) {
        any_cased = true;
        word_start = false;
      }
    } else {
      if ( up ) return false;
      if ( lo ) any_cased = true;
    }
    if ( !up && !lo ) word_start = true;
  }
  return any_cased;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lstrip / rstrip

template <char Tk = ' ', is_string T>
T &
lstrip(T &data)
{
  if ( data.empty() ) return data;
  auto start = data.begin();
  while ( start != data.end() && *start == Tk ) ++start;
  if ( start == data.begin() ) return data;
  usize new_len = static_cast<usize>(data.end() - start);
  micron::bytemove(data.begin(), start, new_len);
  data.set_size(new_len);
  return data;
}

template <char Tk = ' ', is_string T>
T
lstrip(const T &data)
{
  T result(data);
  return lstrip<Tk>(result);
}

template <char Tk = ' ', is_string T>
T &
rstrip(T &data)
{
  if ( data.empty() ) return data;
  auto stop = data.end();
  while ( stop != data.begin() && *(stop - 1) == Tk ) --stop;
  usize new_len = static_cast<usize>(stop - data.begin());
  data.set_size(new_len);
  return data;
}

template <char Tk = ' ', is_string T>
T
rstrip(const T &data)
{
  T result(data);
  return rstrip<Tk>(result);
}

// multi-char strip: strip any char in chars set
template <is_string T>
T
lstrip_chars(const T &data, const char *chars)
{
  T result(data);
  if ( result.empty() || chars == nullptr ) return result;
  usize clen = micron::strlen(chars);
  usize start = 0;
  while ( start < result.size() ) {
    bool found = false;
    for ( usize j = 0; j < clen; ++j ) {
      if ( result[start] == chars[j] ) {
        found = true;
        break;
      }
    }
    if ( !found ) break;
    ++start;
  }
  if ( start > 0 ) {
    usize new_len = result.size() - start;
    micron::bytemove(result.begin(), result.begin() + start, new_len);
    result.set_size(new_len);
  }
  return result;
}

template <is_string T>
T
rstrip_chars(const T &data, const char *chars)
{
  T result(data);
  if ( result.empty() || chars == nullptr ) return result;
  usize clen = micron::strlen(chars);
  usize end_pos = result.size();
  while ( end_pos > 0 ) {
    bool found = false;
    for ( usize j = 0; j < clen; ++j ) {
      if ( result[end_pos - 1] == chars[j] ) {
        found = true;
        break;
      }
    }
    if ( !found ) break;
    --end_pos;
  }
  result.set_size(end_pos);
  return result;
}

template <is_string T>
T
strip_chars(const T &data, const char *chars)
{
  return rstrip_chars(lstrip_chars(data, chars), chars);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lower (explicit alias for casefold)

template <is_string T>
T
lower(const T &str)
{
  return casefold(str);
}

template <is_string T>
T &
lower(T &str)
{
  return casefold(str);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// zfill

template <is_string T>
T
zfill(const T &str, usize width)
{
  if ( str.size() >= width ) return str;
  T result;
  usize pad = width - str.size();
  usize content_start = 0;
  // preserve sign
  if ( str.size() > 0 && (str[0] == '-' || str[0] == '+') ) {
    result += str[0];
    content_start = 1;
  }
  for ( usize i = 0; i < pad; ++i ) result += '0';
  for ( usize i = content_start; i < str.size(); ++i ) result += str[i];
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// split → fvector

template <is_string T>
micron::fvector<T>
split_to(const T &data, const char *sep, i64 maxsplit = -1)
{
  micron::fvector<T> result;
  if ( data.empty() ) return result;

  usize sep_len = micron::strlen(sep);
  if ( sep_len == 0 ) {
    // split on whitespace runs
    usize i = 0;
    i64 splits = 0;
    while ( i < data.size() ) {
      while ( i < data.size() && (data[i] == ' ' || data[i] == '\t' || data[i] == '\n' || data[i] == '\r') ) ++i;
      if ( i >= data.size() ) break;
      if ( maxsplit >= 0 && splits >= maxsplit ) {
        result.inline_push_back(move(data.substr(i, data.size() - i)));
        return result;
      }
      usize start = i;
      while ( i < data.size() && data[i] != ' ' && data[i] != '\t' && data[i] != '\n' && data[i] != '\r' ) ++i;
      result.inline_push_back(move(data.substr(start, i - start)));
      ++splits;
    }
    return result;
  }

  usize start = 0;
  i64 splits = 0;
  while ( start <= data.size() ) {
    if ( maxsplit >= 0 && splits >= maxsplit ) {
      result.inline_push_back(move(data.substr(start, data.size() - start)));
      return result;
    }
    usize pos = find_pos(data, sep, start);
    if ( pos == npos ) {
      result.inline_push_back(move(data.substr(start, data.size() - start)));
      return result;
    }
    result.inline_push_back(move(data.substr(start, pos - start)));
    start = pos + sep_len;
    ++splits;
  }
  return result;
}

template <is_string T>
micron::fvector<T>
split_to(const T &data, char sep, i64 maxsplit = -1)
{
  micron::fvector<T> result;
  if ( data.empty() ) return result;
  usize start = 0;
  i64 splits = 0;
  for ( usize i = 0; i < data.size(); ++i ) {
    if ( data[i] == sep ) {
      if ( maxsplit >= 0 && splits >= maxsplit ) {
        result.inline_push_back(move(data.substr(start, data.size() - start)));
        return result;
      }
      result.inline_push_back(move(data.substr(start, i - start)));
      start = i + 1;
      ++splits;
    }
  }
  result.inline_push_back(move(data.substr(start, data.size() - start)));
  return result;
}

// rsplit: split from right
template <is_string T>
micron::fvector<T>
rsplit_to(const T &data, char sep, i64 maxsplit = -1)
{
  micron::fvector<T> parts;
  if ( data.empty() ) return parts;
  usize end_pos = data.size();
  i64 splits = 0;
  for ( usize i = data.size(); i-- > 0; ) {
    if ( data[i] == sep ) {
      if ( maxsplit >= 0 && splits >= maxsplit ) break;
      parts.inline_push_back(data.substr(i + 1, end_pos - i - 1));
      end_pos = i;
      ++splits;
    }
  }
  parts.inline_push_back(data.substr(0, end_pos));
  // reverse
  for ( usize i = 0, j = parts.size() - 1; i < j; ++i, --j ) {
    T tmp = micron::move(parts[i]);
    parts[i] = micron::move(parts[j]);
    parts[j] = micron::move(tmp);
  }
  return parts;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// splitlines
template <is_string T>
micron::vector<T>
splitlines(const T &data, bool keepends = false)
{
  micron::vector<T> result;
  if ( data.empty() ) return result;

  usize sz = data.size();
  usize start = 0;
  usize i = 0;

  while ( i < sz ) {
    if ( data[i] == '\n' ) {
      usize end_len = 1;
      if ( keepends )
        result.move_back(data.substr(start, i - start + end_len));
      else
        result.move_back(data.substr(start, i - start));
      start = i + end_len;
      i = start;
    } else if ( data[i] == '\r' ) {
      usize end_len = (i + 1 < sz && data[i + 1] == '\n') ? 2 : 1;
      if ( keepends )
        result.move_back(data.substr(start, i - start + end_len));
      else
        result.move_back(data.substr(start, i - start));
      start = i + end_len;
      i = start;
    } else {
      ++i;
    }
  }

  if ( start < sz ) result.move_back(data.substr(start, sz - start));

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// partition → tuple<T, T, T>

template <is_string T>
micron::tuple<T, T, T>
partition(const T &data, const char *sep)
{
  usize pos = find_pos(data, sep);
  if ( pos == npos ) return micron::tuple<T, T, T>(data, T(), T());
  usize sep_len = micron::strlen(sep);
  return micron::tuple<T, T, T>(data.substr(0, pos), data.substr(pos, sep_len), data.substr(pos + sep_len, data.size() - pos - sep_len));
}

template <is_string T>
micron::tuple<T, T, T>
partition(const T &data, const T &sep)
{
  usize pos = find_pos(data, sep);
  if ( pos == npos ) return micron::tuple<T, T, T>(data, T(), T());
  return micron::tuple<T, T, T>(data.substr(0, pos), data.substr(pos, sep.size()),
                                data.substr(pos + sep.size(), data.size() - pos - sep.size()));
}

template <is_string T>
micron::tuple<T, T, T>
rpartition(const T &data, const char *sep)
{
  usize pos = rfind_pos(data, sep);
  if ( pos == npos ) return micron::tuple<T, T, T>(T(), T(), data);
  usize sep_len = micron::strlen(sep);
  return micron::tuple<T, T, T>(data.substr(0, pos), data.substr(pos, sep_len), data.substr(pos + sep_len, data.size() - pos - sep_len));
}

template <is_string T>
micron::tuple<T, T, T>
rpartition(const T &data, const T &sep)
{
  usize pos = rfind_pos(data, sep);
  if ( pos == npos ) return micron::tuple<T, T, T>(T(), T(), data);
  return micron::tuple<T, T, T>(data.substr(0, pos), data.substr(pos, sep.size()),
                                data.substr(pos + sep.size(), data.size() - pos - sep.size()));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// translate / maketrans

// translation table: maps char → char (or -1 for delete)
struct trans_table {
  i16 mapping[256];

  trans_table()
  {
    for ( int i = 0; i < 256; ++i ) mapping[i] = static_cast<i16>(i);
  }
};

// maketrans(from, to): each char in from maps to corresponding char in to
inline trans_table
maketrans(const char *from, const char *to)
{
  trans_table t;
  usize flen = micron::strlen(from);
  usize tlen = micron::strlen(to);
  usize len = flen < tlen ? flen : tlen;
  for ( usize i = 0; i < len; ++i ) t.mapping[static_cast<unsigned char>(from[i])] = static_cast<i16>(static_cast<unsigned char>(to[i]));
  return t;
}

// maketrans(from, to, delete_chars): additionally mark chars for deletion
inline trans_table
maketrans(const char *from, const char *to, const char *del)
{
  trans_table t = maketrans(from, to);
  if ( del ) {
    usize dlen = micron::strlen(del);
    for ( usize i = 0; i < dlen; ++i ) t.mapping[static_cast<unsigned char>(del[i])] = -1;
  }
  return t;
}

template <is_string T>
T
translate(const T &str, const trans_table &table)
{
  T result;
  for ( usize i = 0; i < str.size(); ++i ) {
    i16 m = table.mapping[static_cast<unsigned char>(str[i])];
    if ( m >= 0 ) result += static_cast<char>(m);
    // m == -1: delete
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// extensive replace API

// replace first N occurrences (count == -1 means all)
template <is_string T>
T
replace_n(const T &str, const char *old_s, const char *new_s, i64 count = -1)
{
  if ( count == 0 ) return str;
  T result(str);
  if ( count < 0 ) return replace_all(result, old_s, new_s);
  auto szl = micron::strlen(old_s);
  auto szr = micron::strlen(new_s);
  i64 replaced = 0;
  typename T::iterator itr = find(result, old_s);
  while ( itr != nullptr && (count < 0 || replaced < count) ) {
    auto sz = result.size();
    usize pos = static_cast<usize>(itr - result.begin());
    usize tail_len = sz - pos - szl;
    micron::bytemove(itr + szr, itr + szl, tail_len);
    micron::bytecpy(itr, new_s, szr);
    result.set_size(sz - szl + szr);
    ++replaced;
    itr = find(result, result.begin() + pos + szr, old_s);
  }
  return result;
}

template <is_string T>
T
replace_n(const T &str, const T &old_s, const T &new_s, i64 count = -1)
{
  return replace_n(str, old_s.begin(), new_s.begin(), count);
}

// replace single char → char
template <is_string T>
T
replace_char(const T &str, char old_c, char new_c)
{
  T result(str);
  for ( usize i = 0; i < result.size(); ++i )
    if ( result[i] == old_c ) result[i] = new_c;
  return result;
}

// replace single char → string
template <is_string T>
T
replace_char(const T &str, char old_c, const char *new_s)
{
  T result;
  usize new_len = micron::strlen(new_s);
  for ( usize i = 0; i < str.size(); ++i ) {
    if ( str[i] == old_c )
      result.append(new_s, new_len);
    else
      result += str[i];
  }
  return result;
}

// replace byte range
template <is_string T>
T
replace_range(const T &str, usize start, usize end_pos, const char *replacement)
{
  if ( start >= str.size() ) return str;
  if ( end_pos > str.size() ) end_pos = str.size();
  T result;
  for ( usize i = 0; i < start; ++i ) result += str[i];
  result += replacement;
  for ( usize i = end_pos; i < str.size(); ++i ) result += str[i];
  return result;
}

template <is_string T>
T
replace_range(const T &str, usize start, usize end_pos, const T &replacement)
{
  return replace_range(str, start, end_pos, replacement.begin());
}

// erase all occurrences of a char
template <is_string T>
T
erase_char(const T &str, char ch)
{
  T result;
  for ( usize i = 0; i < str.size(); ++i )
    if ( str[i] != ch ) result += str[i];
  return result;
}

// erase all occurrences of a substring
template <is_string T>
T
erase_all(const T &str, const char *sub)
{
  T result(str);
  return replace_all(result, sub, "");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// join (method-style: separator.join(items))
// already defined in format.hpp as free function;
// add char-separator variant

template <is_string T>
hstring<schar>
join_char(char sep, const micron::fvector<T> &items)
{
  hstring<schar> result;
  bool first = true;
  for ( usize i = 0; i < items.size(); ++i ) {
    if ( !first ) result += sep;
    result += items[i];
    first = false;
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// center / ljust / rjust (aliases for pad_center / pad_right / pad_left)
// these are already defined via pad_* in format.hpp but add explicit aliases

template <is_string T>
T
center(const T &str, usize width, char fillchar = ' ')
{
  return pad_center(str, width, fillchar);
}

template <is_string T>
T
ljust(const T &str, usize width, char fillchar = ' ')
{
  return pad_right(str, width, fillchar);
}

template <is_string T>
T
rjust(const T &str, usize width, char fillchar = ' ')
{
  return pad_left(str, width, fillchar);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// _stack alternatives for functions that return new strings

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
zfill_stack(const T &str, usize width)
{
  // compute into tmp, copy to sstring
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize pos = 0;
  usize pad = (str.size() >= width) ? 0 : width - str.size();
  usize content_start = 0;
  if ( str.size() > 0 && (str[0] == '-' || str[0] == '+') ) {
    if ( pos < Sz - 1 ) out[pos++] = str[0];
    content_start = 1;
  }
  for ( usize i = 0; i < pad && pos < Sz - 1; ++i ) out[pos++] = '0';
  for ( usize i = content_start; i < str.size() && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(str[i]);
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
lower_stack(const T &str)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  if ( len > Sz - 1 ) len = Sz - 1;
  for ( usize i = 0; i < len; ++i ) {
    char c = str[i];
    out[i] = static_cast<C>((c >= 'A' && c <= 'Z') ? c + 32 : c);
  }
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
upper_stack(const T &str)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  if ( len > Sz - 1 ) len = Sz - 1;
  for ( usize i = 0; i < len; ++i ) {
    char c = str[i];
    out[i] = static_cast<C>((c >= 'a' && c <= 'z') ? c - 32 : c);
  }
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
capitalize_stack(const T &str)
{
  micron::sstring<Sz, C> result = lower_stack<Sz, C>(str);
  if ( result.size() > 0 ) {
    char c = result[0];
    if ( c >= 'a' && c <= 'z' ) result[0] = static_cast<C>(c - 32);
  }
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
title_case_stack(const T &str)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  if ( len > Sz - 1 ) len = Sz - 1;
  bool word_start = true;
  for ( usize i = 0; i < len; ++i ) {
    char c = str[i];
    bool is_sp = (c == ' ' || (c >= 0x09 && c <= 0x0D));
    bool is_pu = ((c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) || (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7E));
    if ( is_sp || is_pu ) {
      out[i] = static_cast<C>(c);
      word_start = true;
    } else if ( word_start ) {
      out[i] = static_cast<C>((c >= 'a' && c <= 'z') ? c - 32 : c);
      word_start = false;
    } else {
      out[i] = static_cast<C>((c >= 'A' && c <= 'Z') ? c + 32 : c);
    }
  }
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
swap_case_stack(const T &str)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  if ( len > Sz - 1 ) len = Sz - 1;
  for ( usize i = 0; i < len; ++i ) {
    char c = str[i];
    if ( c >= 'A' && c <= 'Z' )
      out[i] = static_cast<C>(c + 32);
    else if ( c >= 'a' && c <= 'z' )
      out[i] = static_cast<C>(c - 32);
    else
      out[i] = static_cast<C>(c);
  }
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
center_stack(const T &str, usize width, char fillchar = ' ')
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  if ( len >= width || len >= Sz - 1 ) {
    usize clen = len > Sz - 1 ? Sz - 1 : len;
    for ( usize i = 0; i < clen; ++i ) out[i] = static_cast<C>(str[i]);
    out[clen] = '\0';
    result._buf_set_length(clen);
    return result;
  }
  usize pad = width - len;
  usize left = pad / 2;
  usize right = pad - left;
  usize pos = 0;
  for ( usize i = 0; i < left && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(fillchar);
  for ( usize i = 0; i < len && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(str[i]);
  for ( usize i = 0; i < right && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(fillchar);
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
ljust_stack(const T &str, usize width, char fillchar = ' ')
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  usize pos = 0;
  for ( usize i = 0; i < len && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(str[i]);
  while ( pos < width && pos < Sz - 1 ) out[pos++] = static_cast<C>(fillchar);
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
rjust_stack(const T &str, usize width, char fillchar = ' ')
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  usize pad = (len >= width) ? 0 : width - len;
  usize pos = 0;
  for ( usize i = 0; i < pad && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(fillchar);
  for ( usize i = 0; i < len && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(str[i]);
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
translate_stack(const T &str, const trans_table &table)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize pos = 0;
  for ( usize i = 0; i < str.size() && pos < Sz - 1; ++i ) {
    i16 m = table.mapping[static_cast<unsigned char>(str[i])];
    if ( m >= 0 ) out[pos++] = static_cast<C>(m);
  }
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
replace_char_stack(const T &str, char old_c, char new_c)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize len = str.size();
  if ( len > Sz - 1 ) len = Sz - 1;
  for ( usize i = 0; i < len; ++i ) out[i] = static_cast<C>(str[i] == old_c ? new_c : str[i]);
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
erase_char_stack(const T &str, char ch)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize pos = 0;
  for ( usize i = 0; i < str.size() && pos < Sz - 1; ++i )
    if ( str[i] != ch ) out[pos++] = static_cast<C>(str[i]);
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

template <usize Sz, typename C = schar, is_string T>
micron::sstring<Sz, C>
repeat_stack(const T &str, usize n)
{
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize pos = 0;
  for ( usize r = 0; r < n; ++r )
    for ( usize i = 0; i < str.size() && pos < Sz - 1; ++i ) out[pos++] = static_cast<C>(str[i]);
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

};     // namespace format
// shorthand alias
namespace fmt = format;
};     // namespace micron
