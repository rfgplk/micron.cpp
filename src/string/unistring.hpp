//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../concepts.hpp"
#include "string.h"
#include "unitypes.hpp"

namespace micron
{

template <typename T = char8_t>
inline micron::hstring<T>
with_capacity(const size_t n)
{
  return micron::hstring<T>(n);
};

template <size_t N, typename T = char8_t>
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

template <typename I, typename T = char8_t>
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

template <typename I, typename T = char8_t, size_t N>
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
template <typename I, typename T = char8_t>
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

constexpr const char8_t *
u8_check(const char8_t *str, size_t n)
{
  for ( size_t i = 0; i < n; i++ ) {
    char8_t c = *str++;
    if ( c < 0x80 )
      continue;     // ASCII range

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
u16_check(const char16_t *str, size_t n)
{
  for ( size_t i = 0; i < n; i++ ) {
    uint16_t c = (*str++);

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
u32_check(const char32_t *str, size_t n)
{
  for ( size_t i = 0; i < n; i++ ) {
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

template <typename I, typename T = char8_t>
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

template <size_t N, typename T>
inline auto     // micron::hstring<T>
to_string(const T (&str)[N])
{
  if constexpr ( N < 512 ) {
    return micron::sstring<N, T>(str);
  } else
    return micron::hstring<T>(str);
};

};
