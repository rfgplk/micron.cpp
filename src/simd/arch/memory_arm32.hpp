//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../namespace.hpp"

#include "../__bits/__asm_blocks_arm32.hpp"

namespace micron
{
namespace simd
{

__attribute__((always_inline)) static inline void
__neon_sfence(void) noexcept
{
  __asm__ volatile("dmb st" ::: "memory");
}

__attribute__((always_inline)) static inline bool
__neon_v7_all_eq(uint8x16_t ceq) noexcept
{
  const uint64x2_t v64 = vreinterpretq_u64_u8(ceq);
  return (vgetq_lane_u64(v64, 0) & vgetq_lane_u64(v64, 1)) == ~static_cast<u64>(0);
}

template <typename T>
__attribute__((nonnull)) T *
memcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy128 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16(d + i * 16, s + i * 16);

  const u64 rem = bytes % 16;
  if ( rem ) {
    u8 tmp[16] = {};
    for ( u64 i = 0; i < rem; i++ ) tmp[i] = s[n * 16 + i];
    for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = tmp[i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
amemcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemcpy128 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 16));
  const auto *__restrict s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16_a(d + i * 16, s + i * 16);

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
ntmemcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "ntmemcpy128 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 16));
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16_nt(d + i * 16, s + i * 16);

  __neon_sfence();

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
  }

  return dest;
}

template <typename F, typename D>
F &
rmemcpy128(F &__restrict dest, const D &__restrict src, const u64 cnt) noexcept
{
  const u64 n = (cnt * sizeof(D)) / 16;
  auto *__restrict d = reinterpret_cast<u8 *>(&dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(&src);
  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16(d + i * 16, s + i * 16);
  const u64 rem = (cnt * sizeof(D)) % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
  }
  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
memmove128(T *dest, const T *src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove128 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  const auto *s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 ) return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 16;
    for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16(d + i * 16, s + i * 16);
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
    }
  } else {
    const u64 n = bytes / 16;
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 16 + i - 1] = s[n * 16 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) __bits::__block_copy_16(d + (i - 1) * 16, s + (i - 1) * 16);
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
amemmove128(T *dest, const T *src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemmove128 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 16));
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 ) return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 16;
    for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16_a(d + i * 16, s + i * 16);
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
    }
  } else {
    const u64 n = bytes / 16;
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 16 + i - 1] = s[n * 16 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) __bits::__block_copy_16_a(d + (i - 1) * 16, s + (i - 1) * 16);
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
memset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;
  const uint8x16_t v = __bits::__broadcast_byte_16(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16(s + i * 16, v);

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 16 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
amemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;
  const uint8x16_t v = __bits::__broadcast_byte_16(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16_a(s + i * 16, v);

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 16 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
ntmemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;
  const uint8x16_t v = __bits::__broadcast_byte_16(in);

  // ARMv7 NEON has no NT store; reuse aligned store + dmb st fence.
  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16_a(s + i * 16, v);

  __neon_sfence();

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 16 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) i64
memcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(src);
  const auto *d = reinterpret_cast<const u8 *>(dest);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    uint8x16_t ceq = __bits::__block_cmpeq_16(s + i * 16, d + i * 16);
    if ( !__neon_v7_all_eq(ceq) ) {
      const u64 base = i * 16;
      const u64 limit = (base + 16 < bytes) ? base + 16 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] ) return static_cast<i64>(static_cast<unsigned>(s[j])) - static_cast<i64>(static_cast<unsigned>(d[j]));
    }
  }

  const u64 rem = bytes % 16;
  if ( rem ) {
    const u64 base = n * 16;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(static_cast<unsigned>(s[base + i])) - static_cast<i64>(static_cast<unsigned>(d[base + i]));
  }

  return 0;
}

template <typename T>
__attribute__((nonnull)) i64
amemcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16));
  const auto *d = reinterpret_cast<const u8 *>(__builtin_assume_aligned(dest, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    uint8x16_t ceq = __bits::__block_cmpeq_16(s + i * 16, d + i * 16);
    if ( !__neon_v7_all_eq(ceq) ) {
      const u64 base = i * 16;
      const u64 limit = (base + 16 < bytes) ? base + 16 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] ) return static_cast<i64>(static_cast<unsigned>(s[j])) - static_cast<i64>(static_cast<unsigned>(d[j]));
    }
  }

  const u64 rem = bytes % 16;
  if ( rem ) {
    const u64 base = n * 16;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(static_cast<unsigned>(s[base + i])) - static_cast<i64>(static_cast<unsigned>(d[base + i]));
  }

  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wordset - splat a u64 across the buffer (bytes count)

__attribute__((nonnull)) static inline u8 *
wordset128(u8 *__restrict src, const u64 in, const u64 bytes) noexcept
{
  const u64 n = bytes / 16;
  const uint8x16_t v = __bits::__broadcast_word_16(in);
  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16(src + i * 16, v);
  const u64 rem = bytes % 16;
  if ( rem ) {
    u8 buf[16];
    __bits::__block_set_16(buf, v);
    for ( u64 i = 0; i < rem; i++ ) src[n * 16 + i] = buf[i];
  }
  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memchr - byte search

template <typename T>
__attribute__((nonnull)) T *
memchr128(const T *src, u8 c, const u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    const u64 m = __bits::__block_eq_mask_16(p + i * 16, c);
    if ( m ) return const_cast<T *>(reinterpret_cast<const T *>(p + i * 16 + (__builtin_ctzll(m) >> 2)));
  }

  const u64 base = n * 16;
  const u64 rem = bytes % 16;
  for ( u64 i = 0; i < rem; i++ )
    if ( p[base + i] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + base + i));

  return nullptr;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memrchr - reverse byte search

template <typename T>
__attribute__((nonnull)) T *
memrchr128(const T *src, u8 c, const u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;
  const u64 rem = bytes % 16;
  const u64 base = n * 16;

  for ( u64 i = rem; i > 0; i-- )
    if ( p[base + i - 1] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + base + i - 1));

  for ( u64 i = n; i > 0; i-- ) {
    const u64 m = __bits::__block_eq_mask_16(p + (i - 1) * 16, c);
    if ( m ) {
      const u32 hi = (63u - __builtin_clzll(m)) >> 2;
      return const_cast<T *>(reinterpret_cast<const T *>(p + (i - 1) * 16 + hi));
    }
  }

  return nullptr;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mempcpy - memcpy returning d + n

template <typename T>
__attribute__((nonnull)) T *
mempcpy128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "mempcpy128 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16(d + i * 16, s + i * 16);

  const u64 rem = bytes % 16;
  if ( rem ) {
    u8 tmp[16] = {};
    for ( u64 i = 0; i < rem; i++ ) tmp[i] = s[n * 16 + i];
    for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = tmp[i];
  }

  return reinterpret_cast<T *>(d + bytes);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmem - substring search

template <typename T>
T *
memmem128(const T *hay, const u64 hlen, const T *nee, const u64 nlen) noexcept
{
  const auto *h = reinterpret_cast<const u8 *>(hay);
  const auto *ne = reinterpret_cast<const u8 *>(nee);
  const u64 hbytes = hlen * sizeof(T);
  const u64 nbytes = nlen * sizeof(T);

  if ( nbytes == 0 ) return const_cast<T *>(hay);
  if ( nbytes > hbytes ) return nullptr;

  const u64 limit = hbytes - nbytes + 1;
  const u8 first = ne[0];

  u64 i = 0;
  while ( i + 16 <= limit ) {
    const u64 m = __bits::__block_eq_mask_16(h + i, first);
    if ( m ) {
      u64 mm = m;
      while ( mm ) {
        const u32 off = __builtin_ctzll(mm) >> 2;
        const u64 cand = i + off;
        if ( cand >= limit ) return nullptr;
        if ( nbytes == 1 ) return const_cast<T *>(reinterpret_cast<const T *>(h + cand));
        bool match = true;
        for ( u64 j = 1; j < nbytes; j++ )
          if ( h[cand + j] != ne[j] ) {
            match = false;
            break;
          }
        if ( match ) return const_cast<T *>(reinterpret_cast<const T *>(h + cand));
        mm &= ~(0xFull << (__builtin_ctzll(mm) & ~3u));
      }
    }
    i += 16;
  }

  for ( ; i < limit; i++ ) {
    if ( h[i] != first ) continue;
    bool match = true;
    for ( u64 j = 1; j < nbytes; j++ )
      if ( h[i + j] != ne[j] ) {
        match = false;
        break;
      }
    if ( match ) return const_cast<T *>(reinterpret_cast<const T *>(h + i));
  }

  return nullptr;
}

};     // namespace simd
};     // namespace micron
