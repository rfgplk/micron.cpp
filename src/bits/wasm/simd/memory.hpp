// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"

namespace micron::simd
{
template<typename T>
inline usize
__wasm_bytes(u64 count) noexcept
{
  if ( count > u64(~usize(0)) / sizeof(T) ) __builtin_trap();
  return usize(count) * sizeof(T);
}

template<typename T>
inline T *
memcpy128(T *dest, const T *src, u64 count) noexcept
{
  const usize n = __wasm_bytes<T>(count);
  if ( n ) __builtin_memcpy(dest, src, n);
  return dest;
}

template<typename T>
inline T *
memmove128(T *dest, const T *src, u64 count) noexcept
{
  const usize n = __wasm_bytes<T>(count);
  if ( n ) __builtin_memmove(dest, src, n);
  return dest;
}

template<typename T>
inline T *
memset128(T *dest, u8 value, u64 count) noexcept
{
  const usize n = __wasm_bytes<T>(count);
  if ( n ) __builtin_memset(dest, value, n);
  return dest;
}

template<typename T>
inline i64
memcmp128(const T *left, const T *right, u64 count) noexcept
{
  const auto *a = reinterpret_cast<const u8 *>(left), *b = reinterpret_cast<const u8 *>(right);
  const usize n = __wasm_bytes<T>(count);
  for ( usize i = 0; i < n; ++i )
    if ( a[i] != b[i] ) return i64(a[i]) - i64(b[i]);
  return 0;
}

template<typename T>
inline T *
memchr128(const T *ptr, u8 value, u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(ptr);
  const usize n = __wasm_bytes<T>(count);
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == value ) return reinterpret_cast<T *>(const_cast<u8 *>(p + i));
  return nullptr;
}

template<typename T>
inline T *
memrchr128(const T *ptr, u8 value, u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(ptr);
  for ( usize i = __wasm_bytes<T>(count); i; --i )
    if ( p[i - 1] == value ) return reinterpret_cast<T *>(const_cast<u8 *>(p + i - 1));
  return nullptr;
}

template<typename T>
inline T *
mempcpy128(T *dest, const T *src, u64 count) noexcept
{
  memcpy128(dest, src, count);
  return count ? dest + count : dest;
}

template<typename T>
inline T *
memmem128(const T *hay, u64 hlen, const T *needle, u64 nlen) noexcept
{
  const usize h = __wasm_bytes<T>(hlen), n = __wasm_bytes<T>(nlen);
  const auto *a = reinterpret_cast<const u8 *>(hay), *b = reinterpret_cast<const u8 *>(needle);
  if ( !n ) return const_cast<T *>(hay);
  if ( n > h ) return nullptr;
  for ( usize i = 0; i <= h - n; ++i )
    if ( !memcmp128(a + i, b, n) ) return reinterpret_cast<T *>(const_cast<u8 *>(a + i));
  return nullptr;
}

inline u8 *
wordset128(u8 *dest, u64 value, u64 count) noexcept
{
  const usize n = __wasm_bytes<u8>(count);
  for ( usize i = 0; i < n; ++i ) dest[i] = u8(value >> ((i & 7) * 8));
  return dest;
}

template<typename F, typename D>
inline F &
rmemcpy128(F &dest, const D &src, u64 count) noexcept
{
  const usize n = __wasm_bytes<D>(count);
  if ( n ) __builtin_memcpy(&dest, &src, n);
  return dest;
}

#define __micron_wasm_copy_alias(NAME, BASE)                                                                                               \
  template<typename T> inline T *NAME(T *d, const T *s, u64 n) noexcept { return BASE(d, s, n); }
#define __micron_wasm_set_alias(NAME)                                                                                                      \
  template<typename T> inline T *NAME(T *d, u8 c, u64 n) noexcept { return memset128(d, c, n); }
#define __micron_wasm_cmp_alias(NAME)                                                                                                      \
  template<typename T> inline i64 NAME(const T *a, const T *b, u64 n) noexcept { return memcmp128(a, b, n); }
#define __micron_wasm_memory_width(W)                                                                                                      \
  __micron_wasm_copy_alias(amemcpy##W, memcpy128) __micron_wasm_copy_alias(ntmemcpy##W, memcpy128)                                         \
      __micron_wasm_copy_alias(amemmove##W, memmove128) __micron_wasm_set_alias(amemset##W) __micron_wasm_set_alias(ntmemset##W)           \
          __micron_wasm_cmp_alias(amemcmp##W)
__micron_wasm_memory_width(128) __micron_wasm_memory_width(256) __micron_wasm_memory_width(512)
    __micron_wasm_copy_alias(memcpy256, memcpy128) __micron_wasm_copy_alias(memcpy512, memcpy128)
        __micron_wasm_copy_alias(memmove256, memmove128) __micron_wasm_copy_alias(memmove512, memmove128)
            __micron_wasm_copy_alias(mempcpy256, mempcpy128) __micron_wasm_copy_alias(mempcpy512, mempcpy128)
                __micron_wasm_set_alias(memset256) __micron_wasm_set_alias(memset512) __micron_wasm_cmp_alias(memcmp256)
                    __micron_wasm_cmp_alias(memcmp512)
#undef __micron_wasm_memory_width
#undef __micron_wasm_cmp_alias
#undef __micron_wasm_set_alias
#undef __micron_wasm_copy_alias

                        template<typename T>
                        inline T *memchr256(const T *p, u8 c, u64 n) noexcept
{
  return memchr128(p, c, n);
}

template<typename T>
inline T *
memrchr256(const T *p, u8 c, u64 n) noexcept
{
  return memrchr128(p, c, n);
}

template<typename T>
inline T *
memmem256(const T *p, u64 n, const T *q, u64 m) noexcept
{
  return memmem128(p, n, q, m);
}

template<typename F, typename D>
inline F &
rmemcpy256(F &d, const D &s, u64 n) noexcept
{
  return rmemcpy128(d, s, n);
}

template<typename F, typename D>
inline F &
rmemcpy512(F &d, const D &s, u64 n) noexcept
{
  return rmemcpy128(d, s, n);
}

inline u8 *
wordset256(u8 *p, u64 v, u64 n) noexcept
{
  return wordset128(p, v, n);
}

inline u8 *
__memset_bulk(u8 *p, u8 v, u64 n) noexcept
{
  return memset128(p, v, n);
}

inline u8 *
__memset_bulk_zero(u8 *p, u64 n) noexcept
{
  return memset128(p, 0, n);
}

inline u8 *
__memset_bulk_nt(u8 *p, u8 v, u64 n) noexcept
{
  return memset128(p, v, n);
}

inline u8 *
__wordset_bulk(u8 *p, u64 v, u64 n) noexcept
{
  return wordset128(p, v, n);
}

inline u8 *
__memcpy_bulk(u8 *p, const u8 *q, u64 n) noexcept
{
  return memcpy128(p, q, n);
}

inline u8 *
__memcpy_bulk_nt(u8 *p, const u8 *q, u64 n) noexcept
{
  return memcpy128(p, q, n);
}

inline u8 *
__memmove_bulk_fwd(u8 *p, const u8 *q, u64 n) noexcept
{
  return memmove128(p, q, n);
}

inline u8 *
__memmove_bulk_bwd(u8 *p, const u8 *q, u64 n) noexcept
{
  return memmove128(p, q, n);
}
};      // namespace micron::simd
