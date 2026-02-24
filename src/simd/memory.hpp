//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "types.hpp"

namespace micron
{
namespace simd
{
template <typename T>
T *
memcpy128(T *restrict dest, const T *restrict src, u64 count)
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy128 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  auto *s = reinterpret_cast<const u8 *>(src);

  u64 bytes = count * sizeof(T);
  u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(s + i * 16));
    _mm_storeu_si128(reinterpret_cast<i128 *>(d + i * 16), pkt);
  }

  u64 rem = bytes % 16;
  if ( rem ) {
    u8 tmp[16];
    for ( u64 i = 0; i < rem; i++ )
      tmp[i] = s[n * 16 + i];
    i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(tmp));
    _mm_storeu_si128(reinterpret_cast<i128 *>(d + n * 16), pkt);
  }

  return dest;
}

template <typename F, typename D>
F &
rmemcpy128(F &restrict dest, D &restrict src, const u64 cnt)
{
  for ( u64 n = 0; n < ((cnt * sizeof(D)) / (u64)sizeof(i128)); n++ ) {
    i128 pkt = _mm_loadu_si128(reinterpret_cast<i128 *>(&src[n]));
    _mm_storeu_si128(reinterpret_cast<i128 *>(&dest[n]), pkt);
  }
  return reinterpret_cast<F &>(dest);
};

template <typename T>
T *
memcpy256(T *restrict dest, const T *restrict src, u64 count)
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy256 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  auto *s = reinterpret_cast<const u8 *>(src);

  u64 bytes = count * sizeof(T);
  u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + i * 32));
    _mm256_storeu_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
  }

  u64 rem = bytes % 32;
  if ( rem ) {
    u8 tmp[32];
    for ( u64 i = 0; i < rem; i++ )
      tmp[i] = s[n * 32 + i];
    i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(tmp));
    _mm256_storeu_si256(reinterpret_cast<i256 *>(d + n * 32), pkt);
  }

  return dest;
}

template <typename F, typename D>
F &
rmemcpy256(F &restrict dest, D &restrict src, const u64 cnt)
{
  for ( u64 n = 0; n < ((cnt * sizeof(D)) / (u64)sizeof(i256)); n++ ) {
    i256 pkt = _mm256_loadu_si256(reinterpret_cast<i256 *>(&src[n]));
    _mm256_storeu_si256(reinterpret_cast<i256 *>(&dest[n]), pkt);
  }
  return reinterpret_cast<F &>(dest);
};

template <typename T>
T *
memcpy512(T *restrict dest, const T *restrict src, u64 count)
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy512 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  auto *s = reinterpret_cast<const u8 *>(src);

  u64 bytes = count * sizeof(T);
  u64 n = bytes / 64;

  for ( u64 i = 0; i < n; i++ ) {
    i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
    _mm512_storeu_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
  }

  u64 rem = bytes % 64;
  if ( rem ) {
    u8 tmp[64];
    for ( u64 i = 0; i < rem; i++ )
      tmp[i] = s[n * 64 + i];
    i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(tmp));
    _mm512_storeu_si512(reinterpret_cast<i512 *>(d + n * 64), pkt);
  }

  return dest;
}

template <typename F, typename D>
F *
memmove128(F *dest, D *src, const u64 cnt)
{
  if ( dest < src || dest >= src + cnt ) {
    for ( u64 i = 0; i <= cnt; i += 16 ) {
      i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(src + i));
      _mm_storeu_si128(reinterpret_cast<i128 *>(dest + i), pkt);
    }
  } else {
    u64 i = 0;
    while ( i >= 16 ) {
      i -= 16;
      i128 pkt = _mm_loadu_si128(reinterpret_cast<const i128 *>(src + i));
      _mm_storeu_si128(reinterpret_cast<i128 *>(src + i), pkt);
    }
  }
  return reinterpret_cast<F *>(dest);
};

template <typename T>
T *
memmove256(T *dest, const T *src, u64 count)
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove256 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  auto *s = reinterpret_cast<const u8 *>(src);

  u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s ) {
    u64 n = bytes / 32;
    for ( u64 i = 0; i < n; i++ ) {
      i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + i * 32));
      _mm256_storeu_si256(reinterpret_cast<i256 *>(d + i * 32), pkt);
    }
    u64 rem = bytes % 32;
    if ( rem ) {
      u8 tmp[32];
      for ( u64 i = 0; i < rem; i++ )
        tmp[i] = s[n * 32 + i];
      i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(tmp));
      _mm256_storeu_si256(reinterpret_cast<i256 *>(d + n * 32), pkt);
    }
  } else {
    u64 n = bytes / 32;
    u64 rem = bytes % 32;
    if ( rem ) {
      u8 tmp[32];
      for ( u64 i = 0; i < rem; i++ )
        tmp[i] = s[bytes - rem + i];
      i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(tmp));
      _mm256_storeu_si256(reinterpret_cast<i256 *>(d + bytes - rem), pkt);
    }
    for ( u64 i = n; i > 0; i-- ) {
      i256 pkt = _mm256_loadu_si256(reinterpret_cast<const i256 *>(s + (i - 1) * 32));
      _mm256_storeu_si256(reinterpret_cast<i256 *>(d + (i - 1) * 32), pkt);
    }
  }

  return dest;
}

template <typename T>
T *
memmove512(T *dest, const T *src, u64 count)
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove512 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  auto *s = reinterpret_cast<const u8 *>(src);

  u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 )
    return dest;

  if ( d < s ) {
    u64 n = bytes / 64;
    for ( u64 i = 0; i < n; i++ ) {
      i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
      _mm512_storeu_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
    }
    u64 rem = bytes % 64;
    if ( rem ) {
      u8 tmp[64];
      for ( u64 i = 0; i < rem; i++ )
        tmp[i] = s[n * 64 + i];
      i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(tmp));
      _mm512_storeu_si512(reinterpret_cast<i512 *>(d + n * 64), pkt);
    }
  } else {
    u64 n = bytes / 64;
    u64 rem = bytes % 64;
    if ( rem ) {
      u8 tmp[64];
      for ( u64 i = 0; i < rem; i++ )
        tmp[i] = s[bytes - rem + i];
      i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(tmp));
      _mm512_storeu_si512(reinterpret_cast<i512 *>(d + bytes - rem), pkt);
    }
    for ( u64 i = n; i > 0; i-- ) {
      i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + (i - 1) * 64));
      _mm512_storeu_si512(reinterpret_cast<i512 *>(d + (i - 1) * 64), pkt);
    }
  }

  return dest;
}

};     // namespace simd
};     // namespace micron
