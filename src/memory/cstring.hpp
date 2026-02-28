//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../numerics.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

#include "../simd/intrin.hpp"

namespace micron
{

template <integral T>
inline constexpr usize
strlen(const T *str) noexcept
{
  if ( !str )
    return 0;

  usize i = 0;
  while ( str[i] != static_cast<T>('\0') ) {
    ++i;
  }
  return i;
}

inline constexpr usize
wstrlen(const wchar_t *str) noexcept
{
  if ( !str )
    return 0;

  usize i = 0;
  while ( str[i] != L'\0' ) {
    ++i;
  }
  return i;
}

inline constexpr usize
u16strlen(const char16_t *str) noexcept
{
  if ( !str )
    return 0;

  usize i = 0;
  while ( str[i] != u'\0' ) {
    ++i;
  }
  return i;
}

inline constexpr usize
u32strlen(const char32_t *str) noexcept
{
  if ( !str )
    return 0;

  usize i = 0;
  while ( str[i] != U'\0' ) {
    ++i;
  }
  return i;
}

inline constexpr usize
ustrlen(const char32_t *str) noexcept
{
  if ( !str )
    return 0;

  usize i = 0;
  while ( str[i] != U'\0' ) {
    ++i;
  }
  return i;
}

inline usize
strlen256(const char *str) noexcept
{
  if ( !str )
    return 0;

  const __m256i zero = _mm256_setzero_si256();
  const char *ptr = str;

  while ( true ) {
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr));
    int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, zero));

    if ( mask != 0 ) {
      return (ptr - str) + __builtin_ctz(static_cast<unsigned int>(mask));
    }
    ptr += 32;
  }
}

template <integral T>
inline constexpr usize
strnlen(const T *str, usize maxlen) noexcept
{
  if ( !str )
    return 0;

  usize i = 0;
  while ( i < maxlen && str[i] != static_cast<T>('\0') ) {
    ++i;
  }
  return i;
}

template <integral F, integral D>
inline constexpr int
strcmp(const F *src, const D *dest) noexcept
{
  if ( !src && !dest )
    return 0;
  if ( !src )
    return -1;
  if ( !dest )
    return 1;

  usize i = 0;
  while ( src[i] != static_cast<F>('\0') && dest[i] != static_cast<D>('\0') ) {
    if ( src[i] != static_cast<F>(dest[i]) ) {
      return static_cast<int>(src[i]) - static_cast<int>(dest[i]);
    }
    ++i;
  }

  return static_cast<int>(src[i]) - static_cast<int>(dest[i]);
}

template <integral F, integral D>
inline constexpr int
strncmp(const F *src, const D *dest, usize n) noexcept
{
  if ( !src && !dest )
    return 0;
  if ( !src )
    return -1;
  if ( !dest )
    return 1;
  if ( n == 0 )
    return 0;

  usize i = 0;
  while ( i < n && src[i] != static_cast<F>('\0') && dest[i] != static_cast<D>('\0') ) {
    if ( src[i] != static_cast<F>(dest[i]) ) {
      return static_cast<int>(src[i]) - static_cast<int>(dest[i]);
    }
    ++i;
  }

  if ( i == n )
    return 0;
  return static_cast<int>(src[i]) - static_cast<int>(dest[i]);
}

template <integral F, integral D>
inline constexpr F *
strcpy(F *__restrict dest, const D *__restrict src) noexcept
{
  if ( !dest || !src )
    return dest;

  usize i = 0;
  while ( src[i] != static_cast<D>('\0') ) {
    dest[i] = static_cast<F>(src[i]);
    ++i;
  }
  dest[i] = static_cast<F>('\0');

  return dest;
}

template <integral F, integral D>
inline constexpr F *
strncpy(F *__restrict dest, const D *__restrict src, usize n) noexcept
{
  if ( !dest || !src )
    return dest;

  usize i = 0;
  while ( i < n && src[i] != static_cast<D>('\0') ) {
    dest[i] = static_cast<F>(src[i]);
    ++i;
  }

  while ( i < n ) {
    dest[i] = static_cast<F>('\0');
    ++i;
  }

  return dest;
}

template <integral F, integral D>
inline constexpr F *
strcat(F *__restrict dest, const D *__restrict src) noexcept
{
  if ( !dest || !src )
    return dest;

  usize dest_len = strlen(dest);
  usize i = 0;

  while ( src[i] != static_cast<D>('\0') ) {
    dest[dest_len + i] = static_cast<F>(src[i]);
    ++i;
  }
  dest[dest_len + i] = static_cast<F>('\0');

  return dest;
}

template <integral F, integral D>
inline constexpr F *
strncat(F *__restrict dest, const D *__restrict src, usize n) noexcept
{
  if ( !dest || !src )
    return dest;

  usize dest_len = strlen(dest);
  usize i = 0;

  while ( i < n && src[i] != static_cast<D>('\0') ) {
    dest[dest_len + i] = static_cast<F>(src[i]);
    ++i;
  }
  dest[dest_len + i] = static_cast<F>('\0');

  return dest;
}

template <integral T>
inline constexpr T *
strchr(const T *s, int c) noexcept
{
  if ( !s )
    return nullptr;

  T ch = static_cast<T>(c);
  usize i = 0;

  while ( s[i] != static_cast<T>('\0') ) {
    if ( s[i] == ch ) {
      return const_cast<T *>(s + i);
    }
    ++i;
  }

  if ( ch == static_cast<T>('\0') ) {
    return const_cast<T *>(s + i);
  }

  return nullptr;
}

template <integral T>
inline constexpr T *
strrchr(const T *s, int c) noexcept
{
  if ( !s )
    return nullptr;

  T ch = static_cast<T>(c);
  const T *last = nullptr;
  usize i = 0;

  while ( s[i] != static_cast<T>('\0') ) {
    if ( s[i] == ch ) {
      last = s + i;
    }
    ++i;
  }

  if ( ch == static_cast<T>('\0') ) {
    return const_cast<T *>(s + i);
  }

  return const_cast<T *>(last);
}

template <integral T>
inline constexpr T *
strchrnul(const T *s, int c) noexcept
{
  if ( !s )
    return nullptr;

  T ch = static_cast<T>(c);
  usize i = 0;

  while ( s[i] != static_cast<T>('\0') ) {
    if ( s[i] == ch ) {
      return const_cast<T *>(s + i);
    }
    ++i;
  }

  return const_cast<T *>(s + i);
}

template <integral T, integral F>
  requires micron::is_convertible_v<F, T>
inline constexpr const T *
strstr(const T *haystack, const F *needle) noexcept
{
  if ( !haystack || !needle )
    return nullptr;

  usize needle_len = strlen(needle);
  if ( needle_len == 0 )
    return haystack;

  usize i = 0;
  while ( haystack[i] != static_cast<T>('\0') ) {
    usize j = 0;
    while ( j < needle_len && haystack[i + j] != static_cast<T>('\0') && haystack[i + j] == static_cast<T>(needle[j]) ) {
      ++j;
    }

    if ( j == needle_len ) {
      return haystack + i;
    }
    ++i;
  }

  return nullptr;
}

template <integral T, integral F>
  requires micron::is_convertible_v<F, T>
inline constexpr const T *
strnstr(const T *haystack, const F *needle, usize len) noexcept
{
  if ( !haystack || !needle )
    return nullptr;

  usize needle_len = strlen(needle);
  if ( needle_len == 0 )
    return haystack;
  if ( needle_len > len )
    return nullptr;

  for ( usize i = 0; i <= len - needle_len && haystack[i] != static_cast<T>('\0'); ++i ) {
    usize j = 0;
    while ( j < needle_len && haystack[i + j] == static_cast<T>(needle[j]) ) {
      ++j;
    }

    if ( j == needle_len ) {
      return haystack + i;
    }
  }

  return nullptr;
}

template <integral T>
inline constexpr T *
strpbrk(const T *s, const T *accept) noexcept
{
  if ( !s || !accept )
    return nullptr;

  usize i = 0;
  while ( s[i] != static_cast<T>('\0') ) {
    usize j = 0;
    while ( accept[j] != static_cast<T>('\0') ) {
      if ( s[i] == accept[j] ) {
        return const_cast<T *>(s + i);
      }
      ++j;
    }
    ++i;
  }

  return nullptr;
}

template <integral T>
inline constexpr usize
strspn(const T *s, const T *accept) noexcept
{
  if ( !s || !accept )
    return 0;

  usize i = 0;
  while ( s[i] != static_cast<T>('\0') ) {
    usize j = 0;
    bool found = false;

    while ( accept[j] != static_cast<T>('\0') ) {
      if ( s[i] == accept[j] ) {
        found = true;
        break;
      }
      ++j;
    }

    if ( !found )
      break;
    ++i;
  }

  return i;
}

template <integral T>
inline constexpr usize
strcspn(const T *s, const T *reject) noexcept
{
  if ( !s || !reject )
    return strlen(s);

  usize i = 0;
  while ( s[i] != static_cast<T>('\0') ) {
    usize j = 0;

    while ( reject[j] != static_cast<T>('\0') ) {
      if ( s[i] == reject[j] ) {
        return i;
      }
      ++j;
    }
    ++i;
  }

  return i;
}

template <integral T>
inline T *
strdup(const T *s)
{
  if ( !s )
    return nullptr;

  usize len = strlen(s);
  T *copy = new T[len + 1];

  for ( usize i = 0; i <= len; ++i ) {
    copy[i] = s[i];
  }

  return copy;
}

template <integral T>
inline T *
strndup(const T *s, usize n)
{
  if ( !s )
    return nullptr;

  usize len = strnlen(s, n);
  T *copy = new T[len + 1];

  for ( usize i = 0; i < len; ++i ) {
    copy[i] = s[i];
  }
  copy[len] = static_cast<T>('\0');

  return copy;
}

enum class str_error : i32 { nullptr_arg, invalid_length, buffer_overflow, ok, overlap_detected };

template <integral T>
inline constexpr str_error
sstrlen(const T *str, usize maxlen, usize *out_len) noexcept
{
  if ( !str )
    return str_error::nullptr_arg;
  if ( !out_len )
    return str_error::nullptr_arg;
  if ( maxlen == 0 )
    return str_error::invalid_length;

  usize i = 0;
  while ( i < maxlen && str[i] != static_cast<T>('\0') ) {
    ++i;
  }

  if ( i >= maxlen ) {
    return str_error::buffer_overflow;
  }

  *out_len = i;
  return str_error::ok;
}

template <integral F, integral D>
inline constexpr str_error
sstrcmp(const F *src, usize src_max, const D *dest, usize dest_max, int *result) noexcept
{
  if ( !src || !dest )
    return str_error::nullptr_arg;
  if ( !result )
    return str_error::nullptr_arg;
  if ( src_max == 0 || dest_max == 0 )
    return str_error::invalid_length;

  usize i = 0;
  while ( i < src_max && i < dest_max && src[i] != static_cast<F>('\0') && dest[i] != static_cast<D>('\0') ) {
    if ( src[i] != static_cast<F>(dest[i]) ) {
      *result = static_cast<int>(src[i]) - static_cast<int>(dest[i]);
      return str_error::ok;
    }
    ++i;
  }

  if ( i >= src_max || i >= dest_max ) {
    return str_error::buffer_overflow;
  }

  *result = static_cast<int>(src[i]) - static_cast<int>(dest[i]);
  return str_error::ok;
}

template <integral F, integral D>
inline constexpr str_error
sstrncmp(const F *src, usize src_max, const D *dest, usize dest_max, usize n, int *result) noexcept
{
  if ( !src || !dest )
    return str_error::nullptr_arg;
  if ( !result )
    return str_error::nullptr_arg;
  if ( src_max == 0 || dest_max == 0 )
    return str_error::invalid_length;
  if ( n == 0 ) {
    *result = 0;
    return str_error::ok;
  }

  usize max_cmp = (n < src_max) ? n : src_max;
  max_cmp = (max_cmp < dest_max) ? max_cmp : dest_max;

  usize i = 0;
  while ( i < max_cmp && src[i] != static_cast<F>('\0') && dest[i] != static_cast<D>('\0') ) {
    if ( src[i] != static_cast<F>(dest[i]) ) {
      *result = static_cast<int>(src[i]) - static_cast<int>(dest[i]);
      return str_error::ok;
    }
    ++i;
  }

  if ( i == n ) {
    *result = 0;
    return str_error::ok;
  }

  if ( i >= src_max || i >= dest_max ) {
    return str_error::buffer_overflow;
  }

  *result = static_cast<int>(src[i]) - static_cast<int>(dest[i]);
  return str_error::ok;
}

template <integral F, integral D>
inline constexpr str_error
sstrcpy(F *__restrict dest, usize dest_size, const D *__restrict src, usize src_max) noexcept
{
  if ( !dest || !src )
    return str_error::nullptr_arg;
  if ( dest_size == 0 || src_max == 0 )
    return str_error::invalid_length;

  const void *d_start = static_cast<const void *>(dest);
  const void *d_end = static_cast<const void *>(dest + dest_size);
  const void *s_start = static_cast<const void *>(src);
  const void *s_end = static_cast<const void *>(src + src_max);

  if ( (s_start >= d_start && s_start < d_end) || (d_start >= s_start && d_start < s_end) ) {
    return str_error::overlap_detected;
  }

  usize i = 0;
  while ( i < dest_size - 1 && i < src_max && src[i] != static_cast<D>('\0') ) {
    dest[i] = static_cast<F>(src[i]);
    ++i;
  }

  if ( i < src_max && src[i] != static_cast<D>('\0') ) {
    dest[dest_size - 1] = static_cast<F>('\0');
    return str_error::buffer_overflow;
  }

  dest[i] = static_cast<F>('\0');
  return str_error::ok;
}

template <integral F, integral D>
inline constexpr str_error
sstrncpy(F *__restrict dest, usize dest_size, const D *__restrict src, usize src_max, usize n) noexcept
{
  if ( !dest || !src )
    return str_error::nullptr_arg;
  if ( dest_size == 0 || src_max == 0 )
    return str_error::invalid_length;
  if ( n > dest_size )
    return str_error::buffer_overflow;

  const void *d_start = static_cast<const void *>(dest);
  const void *d_end = static_cast<const void *>(dest + dest_size);
  const void *s_start = static_cast<const void *>(src);
  const void *s_end = static_cast<const void *>(src + src_max);

  if ( (s_start >= d_start && s_start < d_end) || (d_start >= s_start && d_start < s_end) ) {
    return str_error::overlap_detected;
  }

  usize i = 0;
  usize max_copy = (n < dest_size) ? n : dest_size;
  max_copy = (max_copy < src_max) ? max_copy : src_max;

  while ( i < max_copy && src[i] != static_cast<D>('\0') ) {
    dest[i] = static_cast<F>(src[i]);
    ++i;
  }

  while ( i < n && i < dest_size ) {
    dest[i] = static_cast<F>('\0');
    ++i;
  }

  return str_error::ok;
}

template <integral F, integral D>
inline constexpr str_error
sstrcat(F *__restrict dest, usize dest_size, const D *__restrict src, usize src_max) noexcept
{
  if ( !dest || !src )
    return str_error::nullptr_arg;
  if ( dest_size == 0 || src_max == 0 )
    return str_error::invalid_length;

  const void *d_start = static_cast<const void *>(dest);
  const void *d_end = static_cast<const void *>(dest + dest_size);
  const void *s_start = static_cast<const void *>(src);
  const void *s_end = static_cast<const void *>(src + src_max);

  if ( (s_start >= d_start && s_start < d_end) || (d_start >= s_start && d_start < s_end) ) {
    return str_error::overlap_detected;
  }

  usize dest_len = 0;
  while ( dest_len < dest_size && dest[dest_len] != static_cast<F>('\0') ) {
    ++dest_len;
  }

  if ( dest_len >= dest_size ) {
    return str_error::buffer_overflow;
  }

  usize i = 0;
  while ( dest_len + i < dest_size - 1 && i < src_max && src[i] != static_cast<D>('\0') ) {
    dest[dest_len + i] = static_cast<F>(src[i]);
    ++i;
  }

  if ( i < src_max && src[i] != static_cast<D>('\0') ) {
    dest[dest_size - 1] = static_cast<F>('\0');
    return str_error::buffer_overflow;
  }

  dest[dest_len + i] = static_cast<F>('\0');
  return str_error::ok;
}

template <integral F, integral D>
inline constexpr str_error
sstrncat(F *__restrict dest, usize dest_size, const D *__restrict src, usize src_max, usize n) noexcept
{
  if ( !dest || !src )
    return str_error::nullptr_arg;
  if ( dest_size == 0 || src_max == 0 )
    return str_error::invalid_length;

  const void *d_start = static_cast<const void *>(dest);
  const void *d_end = static_cast<const void *>(dest + dest_size);
  const void *s_start = static_cast<const void *>(src);
  const void *s_end = static_cast<const void *>(src + src_max);

  if ( (s_start >= d_start && s_start < d_end) || (d_start >= s_start && d_start < s_end) ) {
    return str_error::overlap_detected;
  }

  usize dest_len = 0;
  while ( dest_len < dest_size && dest[dest_len] != static_cast<F>('\0') ) {
    ++dest_len;
  }

  if ( dest_len >= dest_size ) {
    return str_error::buffer_overflow;
  }

  usize max_copy = (n < dest_size - dest_len - 1) ? n : (dest_size - dest_len - 1);
  max_copy = (max_copy < src_max) ? max_copy : src_max;

  usize i = 0;
  while ( i < max_copy && src[i] != static_cast<D>('\0') ) {
    dest[dest_len + i] = static_cast<F>(src[i]);
    ++i;
  }

  dest[dest_len + i] = static_cast<F>('\0');

  if ( i < n && i < src_max && src[i] != static_cast<D>('\0') ) {
    return str_error::buffer_overflow;
  }

  return str_error::ok;
}

template <integral T>
inline constexpr str_error
sstrchr(const T *s, usize maxlen, int c, T **result) noexcept
{
  if ( !s || !result )
    return str_error::nullptr_arg;
  if ( maxlen == 0 )
    return str_error::invalid_length;

  T ch = static_cast<T>(c);
  usize i = 0;

  while ( i < maxlen && s[i] != static_cast<T>('\0') ) {
    if ( s[i] == ch ) {
      *result = const_cast<T *>(s + i);
      return str_error::ok;
    }
    ++i;
  }

  if ( i >= maxlen ) {
    return str_error::buffer_overflow;
  }

  if ( ch == static_cast<T>('\0') ) {
    *result = const_cast<T *>(s + i);
    return str_error::ok;
  }

  *result = nullptr;
  return str_error::ok;
}

template <integral T>
inline constexpr str_error
sstrrchr(const T *s, usize maxlen, int c, T **result) noexcept
{
  if ( !s || !result )
    return str_error::nullptr_arg;
  if ( maxlen == 0 )
    return str_error::invalid_length;

  T ch = static_cast<T>(c);
  const T *last = nullptr;
  usize i = 0;

  while ( i < maxlen && s[i] != static_cast<T>('\0') ) {
    if ( s[i] == ch ) {
      last = s + i;
    }
    ++i;
  }

  if ( i >= maxlen ) {
    return str_error::buffer_overflow;
  }

  if ( ch == static_cast<T>('\0') ) {
    *result = const_cast<T *>(s + i);
    return str_error::ok;
  }

  *result = const_cast<T *>(last);
  return str_error::ok;
}

template <integral T, integral F>
  requires micron::is_convertible_v<F, T>
inline constexpr str_error
sstrstr(const T *haystack, usize hay_max, const F *needle, usize needle_max, const T **result) noexcept
{
  if ( !haystack || !needle || !result )
    return str_error::nullptr_arg;
  if ( hay_max == 0 || needle_max == 0 )
    return str_error::invalid_length;

  usize needle_len = 0;
  while ( needle_len < needle_max && needle[needle_len] != static_cast<F>('\0') ) {
    ++needle_len;
  }

  if ( needle_len >= needle_max ) {
    return str_error::buffer_overflow;
  }

  if ( needle_len == 0 ) {
    *result = haystack;
    return str_error::ok;
  }

  usize i = 0;
  while ( i < hay_max && haystack[i] != static_cast<T>('\0') ) {
    if ( i + needle_len > hay_max )
      break;

    usize j = 0;
    while ( j < needle_len && haystack[i + j] == static_cast<T>(needle[j]) ) {
      ++j;
    }

    if ( j == needle_len ) {
      *result = haystack + i;
      return str_error::ok;
    }
    ++i;
  }

  if ( i >= hay_max ) {
    return str_error::buffer_overflow;
  }

  *result = nullptr;
  return str_error::ok;
}

}     // namespace micron
