//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "string.hpp"
#include "unitypes.hpp"

namespace micron
{

template <typename T = char>
inline micron::hstring<T>
with_capacity(const usize n)
{
  return micron::hstring<T>(n);
};

template <usize N, typename T = char>
inline micron::hstring<T>
with_capacity(void)
{
  return micron::hstring<T>(N);
};

// this is here to augment reverse()
template <is_string T>
inline void
invert(T &str)
{
  typename T::iterator start = str.begin();
  typename T::iterator end = str.end() - 1;
  while ( start < end ) {
    typename T::value_type t = *start;
    *start++ = *end;
    *end-- = t;
  }
}

template <typename T = char>
i64
hex_string_to_int64(const micron::hstring<T> &buf)
{
  const T *ptr = buf.begin();
  i64 result = 0;
  bool neg = false;

  if ( *ptr == '-' ) {
    neg = true;
    ++ptr;
  }

  const T *end = buf.end();
  while ( ptr != end ) {
    char c = *ptr++;
    int digit = 0;
    if ( c >= '0' && c <= '9' )
      digit = c - '0';
    else if ( c >= 'a' && c <= 'f' )
      digit = 10 + (c - 'a');
    else if ( c >= 'A' && c <= 'F' )
      digit = 10 + (c - 'A');
    else
      break;

    result = (result << 4) | digit;
  }

  return neg ? -result : result;
}

template <typename T = char>
i64
string_to_int64(const micron::hstring<T> &buf)
{
  const T *ptr = &buf[0];
  i64 result = 0;
  bool neg = false;

  if constexpr ( micron::is_signed_v<i64> ) {
    if ( *ptr == '-' ) {
      neg = true;
      ++ptr;
    }
  }

  while ( *ptr != buf.end() ) {
    result = result * 10 + (*ptr - '0');
    ++ptr;
  }

  return neg ? -result : result;
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
int_to_string(I n)
{
  micron::hstring<T> buf(12);
  I ipart = static_cast<I>(n);
  T *iptr = &buf[0];
  if constexpr ( micron::is_signed_v<I> ) {
    if ( n < 0 ) {
      *iptr++ = '-';
      ipart = -ipart;
    }
  }
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  *iptr = '\0';
  buf._buf_set_length(iptr - &buf[0]);
  invert(buf);
  return buf;
}

// WARNING: could go OOB

template <typename I, typename T = char, usize N>
  requires micron::is_integral_v<I>
inline micron::sstring<N, T>
int_to_string_stack(I n)
{
  micron::sstring<N, T> buf;
  I ipart = static_cast<I>(n);
  T *iptr = &buf[0];
  if constexpr ( micron::is_signed_v<I> ) {
    if ( n < 0 ) {
      *iptr++ = '-';
      ipart = -ipart;
    }
  }
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  *iptr = '\0';
  buf._buf_set_length(iptr - &buf[0]);
  invert(buf);
  return buf;
}

template <typename I, typename T = char>
inline micron::hstring<T>
bytes_to_string(I n)
{
  micron::hstring<T> buf(12);
  I ipart = static_cast<I>(n);
  T *iptr = &buf[0];
  if ( n < 0 ) {
    *iptr++ = '-';
    ipart = -ipart;
  }
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  *iptr = '\0';
  buf._buf_set_length(iptr - &buf[0]);
  invert(buf);
  return buf;
}

constexpr const char *
u8_check(const char *str, usize n)
{
  for ( usize i = 0; i < n; i++ ) {
    char c = *str++;
    // if ( c < 0x80 )
    //   continue;     // ASCII range

    // Check the leading byte for UTF-8 validity
    if ( (c & 0xE0) == 0xC0 ) {     // 110xxxxx
      if ( !(*str && (*str & 0xC0) == 0x80) )
        return nullptr;     // check continuation
      str++;
    } else if ( (c & 0xF0) == 0xE0 ) {     // 1110xxxx
      if ( !(*str && (*str & 0xC0) == 0x80) )
        return nullptr;     // check continuation
      str++;
      if ( !(*str && (*str & 0xC0) == 0x80) )
        return nullptr;     // check continuation
      str++;
    } else if ( (c & 0xF8) == 0xF0 ) {     // 11110xxx
      if ( !(*str && (*str & 0xC0) == 0x80) )
        return nullptr;     // check continuation
      str++;
      if ( !(*str && (*str & 0xC0) == 0x80) )
        return nullptr;     // check continuation
      str++;
      if ( !(*str && (*str & 0xC0) == 0x80) )
        return nullptr;     // check continuation
      str++;
    } else {
      return nullptr;     // invalid leading byte
    }
  }
  return str;     // all checks passed
}

constexpr const char16_t *
u16_check(const char16_t *str, usize n)
{
  for ( usize i = 0; i < n; i++ ) {
    u16 c = (*str++);

    if ( c < 0xD800 )
      continue;     // Valid range for U+0000 to U+D7FF

    if ( c >= 0xD800 && c <= 0xDBFF ) {     // High surrogate range
      if ( !(*str && *str >= 0xDC00 && *str <= 0xDFFF) )
        return nullptr;     // Valid low surrogate
      str++;
    } else if ( c >= 0xDC00 && c <= 0xDFFF ) {     // Invalid low surrogate alone
      return nullptr;
    } else if ( c >= 0xE000 ) {     // Valid range for U+E000 to U+FFFF
      continue;
    } else {
      return nullptr;     // Invalid character
    }
  }
  return str;     // All checks passed
}

constexpr const char32_t *
u32_check(const char32_t *str, usize n)
{
  for ( usize i = 0; i < n; i++ ) {
    char32_t c = (*str++);

    if ( c <= 0x10FFFF ) {     // Valid range for Unicode code points
      if ( c >= 0xD800 && c <= 0xDFFF )
        return nullptr;     // Check for surrogate range
      continue;
    }

    return nullptr;     // Invalid character
  }
  return str;     // All checks passed
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
to_string(I x)
{
  return int_to_string(x);
};

template <typename T>
inline micron::hstring<T>
to_string(const T *str)
{
  return micron::hstring<T>(str);
};

template <usize N, typename T>
inline auto     // micron::hstring<T>
to_string(const T (&str)[N])
{
  if constexpr ( N < 512 ) {
    return micron::sstring<N, T>(str);
  } else
    return micron::hstring<T>(str);
};

};     // namespace micron
