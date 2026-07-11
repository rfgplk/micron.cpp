//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../namespace.hpp"

#include "../__bits/__asm_blocks_arm32.hpp"
#include "../__bits/__vec_ld.hpp"

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

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename F, typename D>
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

template<typename T>
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
    // overlapping backward path: use __block_move_16 (no __restrict)
    const u64 n = bytes / 16;
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 16 + i - 1] = s[n * 16 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) __bits::__block_move_16(d + (i - 1) * 16, s + (i - 1) * 16);
  }

  return dest;
}

template<typename T>
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
    // overlapping backward path: use __block_move_16 (no __restrict)
    const u64 n = bytes / 16;
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 16 + i - 1] = s[n * 16 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) __bits::__block_move_16(d + (i - 1) * 16, s + (i - 1) * 16);
  }

  return dest;
}

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
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

template<typename T>
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

__attribute__((nonnull)) inline u8 *
__memset_bulk(u8 *__restrict d, const u8 v, const u64 n) noexcept
{
  const uint8x16_t vv = __bits::__broadcast_byte_16(v);
  __bits::__block_set_16(d, vv);
  u8 *p = reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(d) + 16) & ~static_cast<uintptr_t>(15));
  u8 *const e = d + n;
  for ( ; p + 64 <= e; p += 64 ) {
    __bits::__block_set_16(p, vv);
    __bits::__block_set_16(p + 16, vv);
    __bits::__block_set_16(p + 32, vv);
    __bits::__block_set_16(p + 48, vv);
  }
  __bits::__block_set_16(e - 64, vv);
  __bits::__block_set_16(e - 48, vv);
  __bits::__block_set_16(e - 32, vv);
  __bits::__block_set_16(e - 16, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__wordset_bulk(u8 *__restrict d, const u64 w, const u64 n) noexcept
{
  const uint8x16_t vv = __bits::__broadcast_word_16(w);
  u64 i = 0;
  for ( ; i + 64 <= n; i += 64 ) {
    __bits::__block_set_16(d + i, vv);
    __bits::__block_set_16(d + i + 16, vv);
    __bits::__block_set_16(d + i + 32, vv);
    __bits::__block_set_16(d + i + 48, vv);
  }
  for ( ; i + 16 <= n; i += 16 ) __bits::__block_set_16(d + i, vv);
  if ( i < n ) __bits::__block_set_16(d + n - 16, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memcpy_bulk(u8 *__restrict d, const u8 *__restrict s, const u64 n) noexcept
{
  __bits::__block_copy_16(d, s);
  const u64 off = 16 - (reinterpret_cast<uintptr_t>(d) & 15);
  u8 *pd = d + off;
  const u8 *ps = s + off;
  u64 rem = n - off;
  for ( ; rem >= 64; rem -= 64, pd += 64, ps += 64 ) {
    __bits::__pld(ps + 256);
    __bits::__block_copy_16(pd, ps);
    __bits::__block_copy_16(pd + 16, ps + 16);
    __bits::__block_copy_16(pd + 32, ps + 32);
    __bits::__block_copy_16(pd + 48, ps + 48);
  }
  __bits::__block_copy_16(d + n - 64, s + n - 64);
  __bits::__block_copy_16(d + n - 48, s + n - 48);
  __bits::__block_copy_16(d + n - 32, s + n - 32);
  __bits::__block_copy_16(d + n - 16, s + n - 16);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memmove_bulk_fwd(u8 *d, const u8 *s, const u64 n) noexcept
{
  const __ml::__v16 h = __ml::__ld16(s);
  const __ml::__v16 t0 = __ml::__ld16(s + n - 64);
  const __ml::__v16 t1 = __ml::__ld16(s + n - 48);
  const __ml::__v16 t2 = __ml::__ld16(s + n - 32);
  const __ml::__v16 t3 = __ml::__ld16(s + n - 16);
  const u64 off = 16 - (reinterpret_cast<uintptr_t>(d) & 15);
  u8 *pd = d + off;
  const u8 *ps = s + off;
  u64 rem = n - off;
  for ( ; rem >= 64; rem -= 64, pd += 64, ps += 64 ) {
    __bits::__pld(ps + 256);
    __bits::__block_move_16(pd, ps);
    __bits::__block_move_16(pd + 16, ps + 16);
    __bits::__block_move_16(pd + 32, ps + 32);
    __bits::__block_move_16(pd + 48, ps + 48);
  }
  __ml::__st16(d + n - 64, t0);
  __ml::__st16(d + n - 48, t1);
  __ml::__st16(d + n - 32, t2);
  __ml::__st16(d + n - 16, t3);
  __ml::__st16(d, h);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memmove_bulk_bwd(u8 *d, const u8 *s, const u64 n) noexcept
{
  const __ml::__v16 t = __ml::__ld16(s + n - 16);
  const __ml::__v16 h0 = __ml::__ld16(s);
  const __ml::__v16 h1 = __ml::__ld16(s + 16);
  const __ml::__v16 h2 = __ml::__ld16(s + 32);
  const __ml::__v16 h3 = __ml::__ld16(s + 48);
  u8 *pe = reinterpret_cast<u8 *>(reinterpret_cast<uintptr_t>(d + n) & ~static_cast<uintptr_t>(15));
  const u8 *pse = s + (pe - d);
  while ( static_cast<u64>(pe - d) > 64 ) {
    pe -= 64;
    pse -= 64;
    __bits::__block_move_16(pe + 48, pse + 48);
    __bits::__block_move_16(pe + 32, pse + 32);
    __bits::__block_move_16(pe + 16, pse + 16);
    __bits::__block_move_16(pe, pse);
  }
  __ml::__st16(d, h0);
  __ml::__st16(d + 16, h1);
  __ml::__st16(d + 32, h2);
  __ml::__st16(d + 48, h3);
  __ml::__st16(d + n - 16, t);
  return d;
}

};      // namespace simd
};      // namespace micron
