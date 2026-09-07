//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../namespace.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// generic (scalar) byte-predicate backend

namespace micron
{
namespace simd
{

namespace __gen_bw
{

__attribute__((always_inline)) inline constexpr u64
__splat(const unsigned char b) noexcept
{
  return static_cast<u64>(b) * 0x0101010101010101ull;
}

__attribute__((always_inline)) inline u64
__load_w(const unsigned char *p) noexcept
{
  u64 w;
  __builtin_memcpy(&w, p, 8);
  return w;
}

__attribute__((always_inline)) inline constexpr u64
__zmask(const u64 w) noexcept
{
  return (w - 0x0101010101010101ull) & ~w & 0x8080808080808080ull;
}

__attribute__((always_inline)) inline usize
__popcnt_flags(const u64 m) noexcept
{
#if defined(__POPCNT__) || defined(__micron_arm_any)
  return static_cast<usize>(__builtin_popcountll(m));
#else
  return static_cast<usize>((((m >> 7) * 0x0101010101010101ull) >> 56) & 0xFFu);
#endif
}

};      // namespace __gen_bw

inline usize
find_first_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const unsigned char c = static_cast<unsigned char>(b);
  const u64 key = __gen_bw::__splat(c);
  usize i = 0;
  for ( ; i + 8 <= len; i += 8 ) {
    if ( __gen_bw::__zmask(__gen_bw::__load_w(ptr + i) ^ key) ) {
      for ( usize j = i; j < i + 8; ++j )
        if ( ptr[j] == c ) return j;
    }
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == c ) return i;
  return len;
}

inline usize
find_first_set_256(const void *_ptr, usize len, const char b)
{
  return find_first_set_128(_ptr, len, b);
}

inline usize
count_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const unsigned char c = static_cast<unsigned char>(b);
  const u64 key = __gen_bw::__splat(c);
  usize i = 0;
  usize cnt = 0;
  for ( ; i + 8 <= len; i += 8 ) {
    const u64 m = __gen_bw::__zmask(__gen_bw::__load_w(ptr + i) ^ key);

    if ( m ) cnt += __gen_bw::__popcnt_flags(m);
  }
  for ( ; i < len; ++i )
    if ( ptr[i] == c ) ++cnt;
  return cnt;
}

inline usize
count_set_256(const void *_ptr, usize len, const char b)
{
  return count_set_128(_ptr, len, b);
}

inline bool
any_set_128(const void *_ptr, usize len, const char b)
{
  return find_first_set_128(_ptr, len, b) != len;
}

inline bool
any_set_256(const void *_ptr, usize len, const char b)
{
  return any_set_128(_ptr, len, b);
}

inline bool
all_set_128(const void *_ptr, usize len, const char b)
{
  const unsigned char *ptr = static_cast<const unsigned char *>(_ptr);
  const unsigned char c = static_cast<unsigned char>(b);
  const u64 key = __gen_bw::__splat(c);
  usize i = 0;
  for ( ; i + 8 <= len; i += 8 )
    if ( __gen_bw::__load_w(ptr + i) != key ) return false;
  for ( ; i < len; ++i )
    if ( ptr[i] != c ) return false;
  return true;
}

inline bool
all_set_256(const void *_ptr, usize len, const char b)
{
  return all_set_128(_ptr, len, b);
}

inline bool
none_set_128(const void *_ptr, usize len, const char b)
{
  return find_first_set_128(_ptr, len, b) == len;
}

inline bool
none_set_256(const void *_ptr, usize len, const char b)
{
  return none_set_128(_ptr, len, b);
}

};      // namespace simd
};      // namespace micron
