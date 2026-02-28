//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "intrin.hpp"
#include "types.hpp"

namespace micron
{
namespace simd
{

usize
find_first_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i128 char_reg = _mm_set1_epi8(b);
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    i128 chunk = _mm_loadu_si128(reinterpret_cast<const i128 *>(ptr + i));
    i128 cmp = _mm_cmpeq_epi8(chunk, char_reg);
    int mask = _mm_movemask_epi8(cmp);
    if ( mask )
      return i + __builtin_ctz(mask);
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      return i;
  return len;
}

usize
find_first_set_256(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i256 char_reg = _mm256_set1_epi8(b);
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    i256 chunk = _mm256_loadu_si256(reinterpret_cast<const i256 *>(ptr + i));
    i256 cmp = _mm256_cmpeq_epi8(chunk, char_reg);
    int mask = _mm256_movemask_epi8(cmp);
    if ( mask )
      return i + __builtin_ctz(mask);
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      return i;
  return len;
}

usize
count_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i128 char_reg = _mm_set1_epi8(b);
  usize i = 0;
  usize cnt = 0;
  for ( ; i + 15 < len; i += 16 ) {
    i128 chunk = _mm_loadu_si128(reinterpret_cast<const i128 *>(ptr + i));
    i128 cmp = _mm_cmpeq_epi8(chunk, char_reg);
    cnt += _mm_popcnt_u32(_mm_movemask_epi8(cmp));
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      ++cnt;
  return cnt;
}

usize
count_set_256(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i256 char_reg = _mm256_set1_epi8(b);
  usize i = 0;
  usize cnt = 0;
  for ( ; i + 15 < len; i += 16 ) {
    i256 chunk = _mm256_loadu_si256(reinterpret_cast<const i256 *>(ptr + i));
    i256 cmp = _mm256_cmpeq_epi8(chunk, char_reg);
    cnt += _mm_popcnt_u32(_mm256_movemask_epi8(cmp));
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      ++cnt;
  return cnt;
}

bool
any_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i128 char_reg = _mm_set1_epi8(b);
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    i128 chunk = _mm_loadu_si128(reinterpret_cast<const i128 *>(ptr + i));
    i128 cmp = _mm_cmpeq_epi8(chunk, char_reg);

    if ( _mm_movemask_epi8(cmp) )
      return true;
  }
  // scalar tail
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      return true;
  return false;
}

bool
any_set_256(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i256 char_reg = _mm256_set1_epi8(b);
  usize i = 0;
  for ( ; i + 31 < len; i += 32 ) {
    i256 chunk = _mm256_loadu_si256(reinterpret_cast<const i256 *>(ptr + i));
    i256 cmp = _mm256_cmpeq_epi8(chunk, char_reg);

    if ( _mm256_movemask_epi8(cmp) )
      return true;
  }
  // scalar tail
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      return true;
  return false;
}

bool
all_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i128 char_reg = _mm_set1_epi8(b);
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    i128 chunk = _mm_loadu_si128(reinterpret_cast<const i128 *>(ptr + i));
    i128 cmp = _mm_cmpeq_epi8(chunk, char_reg);

    if ( _mm_movemask_epi8(cmp) != -1 )
      return false;
  }
  // scalar tail
  for ( ; i < len; ++i )
    if ( ptr[i] != b )
      return false;
  return true;
}

bool
all_set_256(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i256 char_reg = _mm256_set1_epi8(b);
  usize i = 0;
  for ( ; i + 31 < len; i += 32 ) {
    i256 chunk = _mm256_loadu_si256(reinterpret_cast<const i256 *>(ptr + i));
    i256 cmp = _mm256_cmpeq_epi8(chunk, char_reg);

    if ( _mm256_movemask_epi8(cmp) != (int)0xFFFFFFFF )
      return false;
  }
  // scalar tail
  for ( ; i < len; ++i )
    if ( ptr[i] != b )
      return false;
  return true;
}

bool
none_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i128 char_reg = _mm_set1_epi8(b);
  usize i = 0;
  for ( ; i + 15 < len; i += 16 ) {
    i128 chunk = _mm_loadu_si128(reinterpret_cast<const i128 *>(ptr + i));
    i128 cmp = _mm_cmpeq_epi8(chunk, char_reg);

    if ( _mm_movemask_epi8(cmp) )
      return false;
  }
  // scalar tail
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      return false;
  return true;
}

bool
none_set_256(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  i256 char_reg = _mm256_set1_epi8(b);
  usize i = 0;
  for ( ; i + 31 < len; i += 32 ) {
    i256 chunk = _mm256_loadu_si256(reinterpret_cast<const i256 *>(ptr + i));
    i256 cmp = _mm256_cmpeq_epi8(chunk, char_reg);

    if ( _mm256_movemask_epi8(cmp) )
      return false;
  }
  // scalar tail
  for ( ; i < len; ++i )
    if ( ptr[i] == b )
      return false;
  return true;
}

};     // namespace simd
};     // namespace micron
