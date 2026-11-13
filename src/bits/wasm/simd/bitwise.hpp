// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"

namespace micron::simd
{
inline u32
__wasm_equal_bytes(const u8 *p, u8 value) noexcept
{
#if defined(__micron_wasm_simd128)
  using V = __wasm_native<signed char>::type;
  V bytes, values;
  __builtin_memcpy(&bytes, p, 16);
  for ( usize i = 0; i < 16; ++i ) values[i] = static_cast<signed char>(value);
  return __builtin_wasm_bitmask_i8x16(bytes == values);
#else
  u32 mask = 0;
  for ( usize i = 0; i < 16; ++i ) mask |= u32(p[i] == value) << i;
  return mask;
#endif
}

inline usize
find_first_set_128(const void *ptr, usize len, char value) noexcept
{
  const auto *p = static_cast<const u8 *>(ptr);
  usize i = 0;
  for ( ; len - i >= 16; i += 16 ) {
    const u32 mask = __wasm_equal_bytes(p + i, u8(value));
    if ( mask ) return i + __builtin_ctz(mask);
  }
  for ( ; i < len; ++i )
    if ( p[i] == u8(value) ) return i;
  return len;
}

inline usize
count_set_128(const void *ptr, usize len, char value) noexcept
{
  const auto *p = static_cast<const u8 *>(ptr);
  usize i = 0, count = 0;
  for ( ; len - i >= 16; i += 16 ) count += __builtin_popcount(__wasm_equal_bytes(p + i, u8(value)));
  for ( ; i < len; ++i ) count += p[i] == u8(value);
  return count;
}

inline bool
all_set_128(const void *ptr, usize len, char value) noexcept
{
  return count_set_128(ptr, len, value) == len;
}

inline bool
any_set_128(const void *ptr, usize len, char value) noexcept
{
  return find_first_set_128(ptr, len, value) != len;
}

inline bool
none_set_128(const void *ptr, usize len, char value) noexcept
{
  return !any_set_128(ptr, len, value);
}

inline usize
find_first_set_256(const void *p, usize n, char c) noexcept
{
  return find_first_set_128(p, n, c);
}

inline usize
count_set_256(const void *p, usize n, char c) noexcept
{
  return count_set_128(p, n, c);
}

inline bool
all_set_256(const void *p, usize n, char c) noexcept
{
  return all_set_128(p, n, c);
}

inline bool
any_set_256(const void *p, usize n, char c) noexcept
{
  return any_set_128(p, n, c);
}

inline bool
none_set_256(const void *p, usize n, char c) noexcept
{
  return none_set_128(p, n, c);
}
};      // namespace micron::simd
