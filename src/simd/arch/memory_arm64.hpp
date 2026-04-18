//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../namespace.hpp"

namespace micron
{
namespace simd
{

__attribute__((always_inline)) static inline void
__neon_sfence(void) noexcept
{
  __asm__ volatile("dmb ishst" ::: "memory");
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

  for ( u64 i = 0; i < n; i++ ) {
    uint8x16_t pkt = vld1q_u8(s + i * 16);
    vst1q_u8(d + i * 16, pkt);
  }

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

  for ( u64 i = 0; i < n; i++ ) {
    uint8x16_t pkt = vld1q_u8(s + i * 16);
    vst1q_u8(d + i * 16, pkt);
  }

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

  for ( u64 i = 0; i < n; i++ ) {
    uint8x16_t pkt = vld1q_u8(s + i * 16);
    vst1q_u8(d + i * 16, pkt);
  }

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
  for ( u64 i = 0; i < n; i++ ) {
    uint8x16_t pkt = vld1q_u8(s + i * 16);
    vst1q_u8(d + i * 16, pkt);
  }
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
    for ( u64 i = 0; i < n; i++ ) {
      vst1q_u8(d + i * 16, vld1q_u8(s + i * 16));
    }
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
    for ( u64 i = n; i > 0; i-- ) {
      vst1q_u8(d + (i - 1) * 16, vld1q_u8(s + (i - 1) * 16));
    }
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
    for ( u64 i = 0; i < n; i++ ) {
      vst1q_u8(d + i * 16, vld1q_u8(s + i * 16));
    }
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
    for ( u64 i = n; i > 0; i-- ) {
      vst1q_u8(d + (i - 1) * 16, vld1q_u8(s + (i - 1) * 16));
    }
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
  const uint8x16_t v = vdupq_n_u8(in);

  for ( u64 i = 0; i < n; i++ ) vst1q_u8(s + i * 16, v);

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
  const uint8x16_t v = vdupq_n_u8(in);

  for ( u64 i = 0; i < n; i++ ) vst1q_u8(s + i * 16, v);

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
  const uint8x16_t v = vdupq_n_u8(in);

  for ( u64 i = 0; i < n; i++ ) vst1q_u8(s + i * 16, v);

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
    uint8x16_t va = vld1q_u8(s + i * 16);
    uint8x16_t vb = vld1q_u8(d + i * 16);
    uint8x16_t ceq = vceqq_u8(va, vb);
    if ( vminvq_u8(ceq) != 0xFF ) {

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
    uint8x16_t va = vld1q_u8(s + i * 16);
    uint8x16_t vb = vld1q_u8(d + i * 16);
    uint8x16_t ceq = vceqq_u8(va, vb);
    if ( vminvq_u8(ceq) != 0xFF ) {
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

};     // namespace simd
};     // namespace micron
