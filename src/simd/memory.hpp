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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy - unaligned

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
    i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(s + i * 16));
    _mm_storeu_si128(reinterpret_cast<i128 *>(d + i * 16), pkt);
  }

  const u64 rem = bytes % 16;
  if ( rem ) {
    u8 tmp[16] = {};
    for ( u64 i = 0; i < rem; i++ )
      tmp[i] = s[n * 16 + i];
    for ( u64 i = 0; i < rem; i++ )
      d[n * 16 + i] = tmp[i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
memcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy256 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + i * 32));
    _mm256_storeu_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
  }

  const u64 rem = bytes % 32;
  if ( rem ) {
    u8 tmp[32] = {};
    for ( u64 i = 0; i < rem; i++ )
      tmp[i] = s[n * 32 + i];
    for ( u64 i = 0; i < rem; i++ )
      d[n * 32 + i] = tmp[i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
memcpy512(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy512 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  for ( u64 i = 0; i < n; i++ ) {
    i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
    _mm512_storeu_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
  }

  const u64 rem = bytes % 64;
  if ( rem ) {
    u8 tmp[64] = {};
    for ( u64 i = 0; i < rem; i++ )
      tmp[i] = s[n * 64 + i];
    for ( u64 i = 0; i < rem; i++ )
      d[n * 64 + i] = tmp[i];
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy

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
    i128 pkt = _mm_load_si128(reinterpret_cast<const i128 *>(s + i * 16));
    _mm_store_si128(reinterpret_cast<i128 *>(d + i * 16), pkt);
  }

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 16 + i] = s[n * 16 + i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
amemcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemcpy256 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 32));
  const auto *__restrict s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 32));

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    i256 pkt = _mm256_load_si256(reinterpret_cast<const i256 *>(s + i * 32));
    _mm256_store_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
  }

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 32 + i] = s[n * 32 + i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
amemcpy512(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemcpy512 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 64));
  const auto *__restrict s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 64));

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  for ( u64 i = 0; i < n; i++ ) {
    i512 pkt = _mm512_load_si512(reinterpret_cast<const i512 *>(s + i * 64));
    _mm512_store_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
  }

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 64 + i] = s[n * 64 + i];
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy

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
    i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(s + i * 16));
    _mm_stream_si128(reinterpret_cast<i128 *>(d + i * 16), pkt);
  }

  _mm_sfence();

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 16 + i] = s[n * 16 + i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
ntmemcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "ntmemcpy256 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 32));
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + i * 32));
    _mm256_stream_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
  }

  _mm_sfence();

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 32 + i] = s[n * 32 + i];
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
ntmemcpy512(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "ntmemcpy512 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 64));
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  for ( u64 i = 0; i < n; i++ ) {
    i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
    _mm512_stream_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
  }

  _mm_sfence();

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 64 + i] = s[n * 64 + i];
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy

template <typename F, typename D>
F &
rmemcpy128(F &__restrict dest, const D &__restrict src, const u64 cnt) noexcept
{
  const u64 n = (cnt * sizeof(D)) / sizeof(i128);
  auto *__restrict d = reinterpret_cast<u8 *>(&dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(&src);
  for ( u64 i = 0; i < n; i++ ) {
    i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(s + i * 16));
    _mm_storeu_si128(reinterpret_cast<i128 *>(d + i * 16), pkt);
  }
  const u64 rem = (cnt * sizeof(D)) % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 16 + i] = s[n * 16 + i];
  }
  return dest;
}

template <typename F, typename D>
F &
rmemcpy256(F &__restrict dest, const D &__restrict src, const u64 cnt) noexcept
{
  const u64 n = (cnt * sizeof(D)) / sizeof(i256);
  auto *__restrict d = reinterpret_cast<u8 *>(&dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(&src);
  for ( u64 i = 0; i < n; i++ ) {
    i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + i * 32));
    _mm256_storeu_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
  }
  const u64 rem = (cnt * sizeof(D)) % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 32 + i] = s[n * 32 + i];
  }
  return dest;
}

template <typename F, typename D>
F &
rmemcpy512(F &__restrict dest, const D &__restrict src, const u64 cnt) noexcept
{
  const u64 n = (cnt * sizeof(D)) / sizeof(i512);
  auto *__restrict d = reinterpret_cast<u8 *>(&dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(&src);
  for ( u64 i = 0; i < n; i++ ) {
    i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
    _mm512_storeu_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
  }
  const u64 rem = (cnt * sizeof(D)) % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      d[n * 64 + i] = s[n * 64 + i];
  }
  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmove

template <typename T>
__attribute__((nonnull)) T *
memmove128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove128 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  const auto *s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s || d >= s + bytes ) {
    // non-overlapping or dest is before src: forward copy
    const u64 n = bytes / 16;
    for ( u64 i = 0; i < n; i++ ) {
      i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(s + i * 16));
      _mm_storeu_si128(reinterpret_cast<i128 *>(d + i * 16), pkt);
    }
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ )
        d[n * 16 + i] = s[n * 16 + i];
    }
  } else {
    // overlapping: dest is after src, must copy backward to avoid clobbering
    const u64 n = bytes / 16;
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- )
        d[n * 16 + i - 1] = s[n * 16 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) {
      i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(s + (i - 1) * 16));
      _mm_storeu_si128(reinterpret_cast<i128 *>(d + (i - 1) * 16), pkt);
    }
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
memmove256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove256 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  const auto *s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 32;
    for ( u64 i = 0; i < n; i++ ) {
      i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + i * 32));
      _mm256_storeu_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
    }
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ )
        d[n * 32 + i] = s[n * 32 + i];
    }
  } else {
    const u64 n = bytes / 32;
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- )
        d[n * 32 + i - 1] = s[n * 32 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) {
      i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + (i - 1) * 32));
      _mm256_storeu_si256(reinterpret_cast<i256 *>(d + (i - 1) * 32), pkt);
    }
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
memmove512(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove512 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  const auto *s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 64;
    for ( u64 i = 0; i < n; i++ ) {
      i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
      _mm512_storeu_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
    }
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ )
        d[n * 64 + i] = s[n * 64 + i];
    }
  } else {
    const u64 n = bytes / 64;
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- )
        d[n * 64 + i - 1] = s[n * 64 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) {
      i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + (i - 1) * 64));
      _mm512_storeu_si512(reinterpret_cast<i512 *>(d + (i - 1) * 64), pkt);
    }
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmove - aligned variants

template <typename T>
__attribute__((nonnull)) T *
amemmove128(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemmove128 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 16));
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 16;
    for ( u64 i = 0; i < n; i++ ) {
      i128 pkt = _mm_load_si128(reinterpret_cast<const i128 *>(s + i * 16));
      _mm_store_si128(reinterpret_cast<i128 *>(d + i * 16), pkt);
    }
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ )
        d[n * 16 + i] = s[n * 16 + i];
    }
  } else {
    const u64 n = bytes / 16;
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- )
        d[n * 16 + i - 1] = s[n * 16 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) {
      i128 pkt = _mm_load_si128(reinterpret_cast<const i128 *>(s + (i - 1) * 16));
      _mm_store_si128(reinterpret_cast<i128 *>(d + (i - 1) * 16), pkt);
    }
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
amemmove256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemmove256 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 32));
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 32));
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 32;
    for ( u64 i = 0; i < n; i++ ) {
      i256 pkt = _mm256_load_si256(reinterpret_cast<const i256 *>(s + i * 32));
      _mm256_store_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
    }
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ )
        d[n * 32 + i] = s[n * 32 + i];
    }
  } else {
    const u64 n = bytes / 32;
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- )
        d[n * 32 + i - 1] = s[n * 32 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) {
      i256 pkt = _mm256_load_si256(reinterpret_cast<const i256 *>(s + (i - 1) * 32));
      _mm256_store_si256(reinterpret_cast<i256 *>(d + (i - 1) * 32), pkt);
    }
  }

  return dest;
}

template <typename T>
__attribute__((nonnull)) T *
amemmove512(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemmove512 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 64));
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 64));
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 64;
    for ( u64 i = 0; i < n; i++ ) {
      i512 pkt = _mm512_load_si512(reinterpret_cast<const i512 *>(s + i * 64));
      _mm512_store_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
    }
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ )
        d[n * 64 + i] = s[n * 64 + i];
    }
  } else {
    const u64 n = bytes / 64;
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- )
        d[n * 64 + i - 1] = s[n * 64 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) {
      i512 pkt = _mm512_load_si512(reinterpret_cast<const i512 *>(s + (i - 1) * 64));
      _mm512_store_si512(reinterpret_cast<i512 *>(d + (i - 1) * 64), pkt);
    }
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memset - unaligned

template <typename T>
__attribute__((nonnull)) T *
memset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  // broadcast byte to fill entire register
  const i128 v = _mm_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm_storeu_si128(reinterpret_cast<i128 *>(s + i * 16), v);

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 16 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
memset256(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  const i256 v = _mm256_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm256_storeu_si256(reinterpret_cast<i256 *>(s + i * 32), v);

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 32 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
memset512(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  const i512 v = _mm512_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm512_storeu_si512(reinterpret_cast<i512 *>(s + i * 64), v);

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 64 + i] = in;
  }

  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memset - aligned

template <typename T>
__attribute__((nonnull)) T *
amemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  const i128 v = _mm_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm_store_si128(reinterpret_cast<i128 *>(s + i * 16), v);

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 16 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
amemset256(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 32));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  const i256 v = _mm256_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm256_store_si256(reinterpret_cast<i256 *>(s + i * 32), v);

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 32 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
amemset512(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 64));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  const i512 v = _mm512_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm512_storeu_si512(reinterpret_cast<i512 *>(s + i * 64), v);

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 64 + i] = in;
  }

  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memset - non-temporal

template <typename T>
__attribute__((nonnull)) T *
ntmemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  const i128 v = _mm_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm_stream_si128(reinterpret_cast<i128 *>(s + i * 16), v);

  _mm_sfence();

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 16 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
ntmemset256(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 32));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  const i256 v = _mm256_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm256_stream_si256(reinterpret_cast<i256 *>(s + i * 32), v);

  _mm_sfence();

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 32 + i] = in;
  }

  return src;
}

template <typename T>
__attribute__((nonnull)) T *
ntmemset512(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 64));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  const i512 v = _mm512_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ )
    _mm512_stream_si512(reinterpret_cast<i512 *>(s + i * 64), v);

  _mm_sfence();

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ )
      s[n * 64 + i] = in;
  }

  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcmp

template <typename T>
__attribute__((nonnull)) i64
memcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(src);
  const auto *d = reinterpret_cast<const u8 *>(dest);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    i128 va = _mm_loadu_si128(reinterpret_cast<const i128 *>(s + i * 16));
    i128 vb = _mm_loadu_si128(reinterpret_cast<const i128 *>(d + i * 16));
    i128 cmp = _mm_cmpeq_epi8(va, vb);
    int mask = _mm_movemask_epi8(cmp);
    if ( mask != 0xFFFF ) {
      const u64 base = i * 16;
      const u64 limit = base + 16 < bytes ? base + 16 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] )
          return static_cast<i64>(&s[j] - &d[j]);
    }
  }

  const u64 rem = bytes % 16;
  if ( rem ) {
    const u64 base = n * 16;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(&s[base + i] - &d[base + i]);
  }

  return 0;
}

template <typename T>
__attribute__((nonnull)) i64
memcmp256(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(src);
  const auto *d = reinterpret_cast<const u8 *>(dest);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    i256 va = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + i * 32));
    i256 vb = _mm256_loadu_si256(reinterpret_cast<const i256 *>(d + i * 32));
    i256 cmp = _mm256_cmpeq_epi8(va, vb);
    int mask = _mm256_movemask_epi8(cmp);
    if ( mask != -1 ) {
      const u64 base = i * 32;
      const u64 limit = base + 32 < bytes ? base + 32 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] )
          return static_cast<i64>(&s[j] - &d[j]);
    }
  }

  const u64 rem = bytes % 32;
  if ( rem ) {
    const u64 base = n * 32;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(&s[base + i] - &d[base + i]);
  }

  return 0;
}

template <typename T>
__attribute__((nonnull)) i64
memcmp512(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(src);
  const auto *d = reinterpret_cast<const u8 *>(dest);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  for ( u64 i = 0; i < n; i++ ) {
    i512 va = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
    i512 vb = _mm512_loadu_si512(reinterpret_cast<const i512 *>(d + i * 64));
    u64 mask = _mm512_cmpeq_epi8_mask(va, vb);
    if ( mask != ~static_cast<u64>(0) ) {
      const u64 base = i * 64;
      const u64 limit = base + 64 < bytes ? base + 64 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] )
          return static_cast<i64>(&s[j] - &d[j]);
    }
  }

  const u64 rem = bytes % 64;
  if ( rem ) {
    const u64 base = n * 64;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(&s[base + i] - &d[base + i]);
  }

  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcmp - aligned

template <typename T>
__attribute__((nonnull)) i64
amemcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16));
  const auto *d = reinterpret_cast<const u8 *>(__builtin_assume_aligned(dest, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    i128 va = _mm_load_si128(reinterpret_cast<const i128 *>(s + i * 16));
    i128 vb = _mm_load_si128(reinterpret_cast<const i128 *>(d + i * 16));
    i128 cmp = _mm_cmpeq_epi8(va, vb);
    int mask = _mm_movemask_epi8(cmp);
    if ( mask != 0xFFFF ) {
      const u64 base = i * 16;
      const u64 limit = base + 16 < bytes ? base + 16 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] )
          return static_cast<i64>(&s[j] - &d[j]);
    }
  }

  const u64 rem = bytes % 16;
  if ( rem ) {
    const u64 base = n * 16;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(&s[base + i] - &d[base + i]);
  }

  return 0;
}

template <typename T>
__attribute__((nonnull)) i64
amemcmp256(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 32));
  const auto *d = reinterpret_cast<const u8 *>(__builtin_assume_aligned(dest, 32));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    i256 va = _mm256_load_si256(reinterpret_cast<const i256 *>(s + i * 32));
    i256 vb = _mm256_load_si256(reinterpret_cast<const i256 *>(d + i * 32));
    i256 cmp = _mm256_cmpeq_epi8(va, vb);
    int mask = _mm256_movemask_epi8(cmp);
    if ( mask != -1 ) {
      const u64 base = i * 32;
      const u64 limit = base + 32 < bytes ? base + 32 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] )
          return static_cast<i64>(&s[j] - &d[j]);
    }
  }

  const u64 rem = bytes % 32;
  if ( rem ) {
    const u64 base = n * 32;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(&s[base + i] - &d[base + i]);
  }

  return 0;
}

template <typename T>
__attribute__((nonnull)) i64
amemcmp512(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 64));
  const auto *d = reinterpret_cast<const u8 *>(__builtin_assume_aligned(dest, 64));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  for ( u64 i = 0; i < n; i++ ) {
    i512 va = _mm512_load_si512(reinterpret_cast<const i512 *>(s + i * 64));
    i512 vb = _mm512_load_si512(reinterpret_cast<const i512 *>(d + i * 64));
    u64 mask = _mm512_cmpeq_epi8_mask(va, vb);
    if ( mask != ~static_cast<u64>(0) ) {
      const u64 base = i * 64;
      const u64 limit = base + 64 < bytes ? base + 64 : bytes;
      for ( u64 j = base; j < limit; j++ )
        if ( s[j] != d[j] )
          return static_cast<i64>(&s[j] - &d[j]);
    }
  }

  const u64 rem = bytes % 64;
  if ( rem ) {
    const u64 base = n * 64;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(&s[base + i] - &d[base + i]);
  }

  return 0;
}

};     // namespace simd
};     // namespace micron
