//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

#include "../simd/intrin.hpp"

namespace micron
{
template <integral T>
  requires(!micron::is_null_pointer_v<T>)
inline size_t
strlen(const T *str)
{
  for ( size_t i = 0;; i++ ) {
    if ( str[i] == static_cast<T>('\0') )
      return i;
  }
  return 0;
}

inline size_t
wstrlen(const wchar_t *str)
{
  for ( size_t i = 0;; i++ ) {
    if ( str[i] == '\0' )
      return i;
  }
  return 0;
}

inline size_t
u16strlen(const char16_t *str)
{
  for ( size_t i = 0;; i++ ) {
    if ( str[i] == '\0' )
      return i;
  }
  return 0;
}

inline size_t
ustrlen(const char32_t *str)
{
  for ( size_t i = 0;; i++ ) {
    if ( str[i] == '\0' )
      return i;
  }
  return 0;
}

template <typename T>
inline size_t
strlen256(const T *str)
{
  const __m256i zero = _mm256_set1_epi8(0);
  const char *is = str;

  while ( true ) {
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(is));
    int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, zero));
    if ( mask != 0 ) {
      return is - str + __builtin_ctz(mask);
    }
    is += 32;
  }
  return 0;
}

template <typename T, typename F>
  requires micron::is_convertible_v<T, F>
const T *
strstr(const T *src, const F *fnd)
{
  const T *srch = reinterpret_cast<const T *>(fnd);
  size_t srch_sz = strlen(fnd);
  for ( size_t i = 0;; i++ ) {
    if ( src[i] == srch[0] ) {
      for ( size_t j = 1; j < srch_sz; j++ )
        if ( src[i] != srch[j] )
          break;
      return src + i;
    }
  }
  return src;
}

template <typename T>
T *
strrchr(const T *s, int c)
{
  size_t n = 0;
  for ( size_t i = 0;; i++ ) {
    if ( s[i] == c )
      n = c;
    else if ( s[i] == '\0' )
      return s + n;
  }
  return s + n;
}

template <typename T>
T *
strchrnul(const T *s, int c)
{
  for ( size_t i = 0;; i++ ) {
    if ( s[i] == c )
      return s + i;
    else if ( s[i] == '\0' )
      return s + i;
  }
}
template <typename F = char, typename D = char>
int
strcmp(F *__restrict src, const D *__restrict dest)
{
  const size_t len_src = strlen(src);
  const size_t len_dest = strlen(dest);
  if ( len_dest <= len_src ) {
    for ( size_t i = 0; i < len_dest; i++ )
      if ( src[i] != dest[i] )
        return src[i] - dest[i];
  } else {
    for ( size_t i = 0; i < len_src; i++ )
      if ( src[i] != dest[i] )
        return src[i] - dest[i];
  }
  return 0;     // equal
};

template <typename F = char, typename D = char>
F *
strcpy(F *__restrict dest, const D *__restrict src)
{
  const size_t len = strlen(src);
  for ( size_t n = 0; n < len; n++ )
    dest[n] = static_cast<F>(src[n]);
  return reinterpret_cast<F *>(dest);
};
};     // namespace micron
