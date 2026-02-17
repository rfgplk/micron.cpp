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
inline constexpr size_t
strlen(const T *str) noexcept
{
  if ( !str )
    return 0;

  size_t i = 0;
  while ( str[i] != static_cast<T>('\0') ) {
    ++i;
  }
  return i;
}

inline constexpr size_t
wstrlen(const wchar_t *str) noexcept
{
  if ( !str )
    return 0;

  size_t i = 0;
  while ( str[i] != L'\0' ) {
    ++i;
  }
  return i;
}

inline constexpr size_t
u16strlen(const char16_t *str) noexcept
{
  if ( !str )
    return 0;

  size_t i = 0;
  while ( str[i] != u'\0' ) {
    ++i;
  }
  return i;
}

inline constexpr size_t
u32strlen(const char32_t *str) noexcept
{
  if ( !str )
    return 0;

  size_t i = 0;
  while ( str[i] != U'\0' ) {
    ++i;
  }
  return i;
}

inline constexpr size_t
ustrlen(const char32_t *str) noexcept
{
  if ( !str )
    return 0;

  size_t i = 0;
  while ( str[i] != U'\0' ) {
    ++i;
  }
  return i;
}

inline size_t
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
inline constexpr size_t
strnlen(const T *str, size_t maxlen) noexcept
{
  if ( !str )
    return 0;

  size_t i = 0;
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

  size_t i = 0;
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
strncmp(const F *src, const D *dest, size_t n) noexcept
{
  if ( !src && !dest )
    return 0;
  if ( !src )
    return -1;
  if ( !dest )
    return 1;
  if ( n == 0 )
    return 0;

  size_t i = 0;
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

  size_t i = 0;
  while ( src[i] != static_cast<D>('\0') ) {
    dest[i] = static_cast<F>(src[i]);
    ++i;
  }
  dest[i] = static_cast<F>('\0');

  return dest;
}

template <integral F, integral D>
inline constexpr F *
strncpy(F *__restrict dest, const D *__restrict src, size_t n) noexcept
{
  if ( !dest || !src )
    return dest;

  size_t i = 0;
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

  size_t dest_len = strlen(dest);
  size_t i = 0;

  while ( src[i] != static_cast<D>('\0') ) {
    dest[dest_len + i] = static_cast<F>(src[i]);
    ++i;
  }
  dest[dest_len + i] = static_cast<F>('\0');

  return dest;
}

template <integral F, integral D>
inline constexpr F *
strncat(F *__restrict dest, const D *__restrict src, size_t n) noexcept
{
  if ( !dest || !src )
    return dest;

  size_t dest_len = strlen(dest);
  size_t i = 0;

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
  size_t i = 0;

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
  size_t i = 0;

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
  size_t i = 0;

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

  size_t needle_len = strlen(needle);
  if ( needle_len == 0 )
    return haystack;

  size_t i = 0;
  while ( haystack[i] != static_cast<T>('\0') ) {
    size_t j = 0;
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
strnstr(const T *haystack, const F *needle, size_t len) noexcept
{
  if ( !haystack || !needle )
    return nullptr;

  size_t needle_len = strlen(needle);
  if ( needle_len == 0 )
    return haystack;
  if ( needle_len > len )
    return nullptr;

  for ( size_t i = 0; i <= len - needle_len && haystack[i] != static_cast<T>('\0'); ++i ) {
    size_t j = 0;
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

  size_t i = 0;
  while ( s[i] != static_cast<T>('\0') ) {
    size_t j = 0;
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
inline constexpr size_t
strspn(const T *s, const T *accept) noexcept
{
  if ( !s || !accept )
    return 0;

  size_t i = 0;
  while ( s[i] != static_cast<T>('\0') ) {
    size_t j = 0;
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
inline constexpr size_t
strcspn(const T *s, const T *reject) noexcept
{
  if ( !s || !reject )
    return strlen(s);

  size_t i = 0;
  while ( s[i] != static_cast<T>('\0') ) {
    size_t j = 0;

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

  size_t len = strlen(s);
  T *copy = new T[len + 1];

  for ( size_t i = 0; i <= len; ++i ) {
    copy[i] = s[i];
  }

  return copy;
}

template <integral T>
inline T *
strndup(const T *s, size_t n)
{
  if ( !s )
    return nullptr;

  size_t len = strnlen(s, n);
  T *copy = new T[len + 1];

  for ( size_t i = 0; i < len; ++i ) {
    copy[i] = s[i];
  }
  copy[len] = static_cast<T>('\0');

  return copy;
}

enum class str_error : i32 { nullptr_arg, invalid_length, buffer_overflow, ok, overlap_detected };

template <integral T>
inline constexpr str_error
sstrlen(const T *str, size_t maxlen, size_t *out_len) noexcept
{
  if ( !str )
    return str_error::nullptr_arg;
  if ( !out_len )
    return str_error::nullptr_arg;
  if ( maxlen == 0 )
    return str_error::invalid_length;

  size_t i = 0;
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
sstrcmp(const F *src, size_t src_max, const D *dest, size_t dest_max, int *result) noexcept
{
  if ( !src || !dest )
    return str_error::nullptr_arg;
  if ( !result )
    return str_error::nullptr_arg;
  if ( src_max == 0 || dest_max == 0 )
    return str_error::invalid_length;

  size_t i = 0;
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
sstrncmp(const F *src, size_t src_max, const D *dest, size_t dest_max, size_t n, int *result) noexcept
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

  size_t max_cmp = (n < src_max) ? n : src_max;
  max_cmp = (max_cmp < dest_max) ? max_cmp : dest_max;

  size_t i = 0;
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
sstrcpy(F *__restrict dest, size_t dest_size, const D *__restrict src, size_t src_max) noexcept
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

  size_t i = 0;
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
sstrncpy(F *__restrict dest, size_t dest_size, const D *__restrict src, size_t src_max, size_t n) noexcept
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

  size_t i = 0;
  size_t max_copy = (n < dest_size) ? n : dest_size;
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
sstrcat(F *__restrict dest, size_t dest_size, const D *__restrict src, size_t src_max) noexcept
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

  size_t dest_len = 0;
  while ( dest_len < dest_size && dest[dest_len] != static_cast<F>('\0') ) {
    ++dest_len;
  }

  if ( dest_len >= dest_size ) {
    return str_error::buffer_overflow;
  }

  size_t i = 0;
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
sstrncat(F *__restrict dest, size_t dest_size, const D *__restrict src, size_t src_max, size_t n) noexcept
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

  size_t dest_len = 0;
  while ( dest_len < dest_size && dest[dest_len] != static_cast<F>('\0') ) {
    ++dest_len;
  }

  if ( dest_len >= dest_size ) {
    return str_error::buffer_overflow;
  }

  size_t max_copy = (n < dest_size - dest_len - 1) ? n : (dest_size - dest_len - 1);
  max_copy = (max_copy < src_max) ? max_copy : src_max;

  size_t i = 0;
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
sstrchr(const T *s, size_t maxlen, int c, T **result) noexcept
{
  if ( !s || !result )
    return str_error::nullptr_arg;
  if ( maxlen == 0 )
    return str_error::invalid_length;

  T ch = static_cast<T>(c);
  size_t i = 0;

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
sstrrchr(const T *s, size_t maxlen, int c, T **result) noexcept
{
  if ( !s || !result )
    return str_error::nullptr_arg;
  if ( maxlen == 0 )
    return str_error::invalid_length;

  T ch = static_cast<T>(c);
  const T *last = nullptr;
  size_t i = 0;

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
sstrstr(const T *haystack, size_t hay_max, const F *needle, size_t needle_max, const T **result) noexcept
{
  if ( !haystack || !needle || !result )
    return str_error::nullptr_arg;
  if ( hay_max == 0 || needle_max == 0 )
    return str_error::invalid_length;

  size_t needle_len = 0;
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

  size_t i = 0;
  while ( i < hay_max && haystack[i] != static_cast<T>('\0') ) {
    if ( i + needle_len > hay_max )
      break;

    size_t j = 0;
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
