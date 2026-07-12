//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../namespace.hpp"

#include "../__bits/__asm_blocks_amd64.hpp"
#include "../__bits/__vec_ld.hpp"

namespace micron
{
namespace simd
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy - unaligned

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
memcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memcpy256 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_32(d + i * 32, s + i * 32);

  const u64 rem = bytes % 32;
  if ( rem ) {
    u8 tmp[32] = {};
    for ( u64 i = 0; i < rem; i++ ) tmp[i] = s[n * 32 + i];
    for ( u64 i = 0; i < rem; i++ ) d[n * 32 + i] = tmp[i];
  }

  return dest;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
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
    for ( u64 i = 0; i < rem; i++ ) tmp[i] = s[n * 64 + i];
    for ( u64 i = 0; i < rem; i++ ) d[n * 64 + i] = tmp[i];
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy - aligned

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
amemcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemcpy256 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 32));
  const auto *__restrict s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 32));

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_32_a(d + i * 32, s + i * 32);

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 32 + i] = s[n * 32 + i];
  }

  return dest;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
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
    for ( u64 i = 0; i < rem; i++ ) d[n * 64 + i] = s[n * 64 + i];
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy - non-temporal

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

  _mm_sfence();

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
  }

  return dest;
}

template<typename T>
__attribute__((nonnull)) T *
ntmemcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "ntmemcpy256 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 32));
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_32_nt(d + i * 32, s + i * 32);

  _mm_sfence();

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 32 + i] = s[n * 32 + i];
  }

  return dest;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
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
    for ( u64 i = 0; i < rem; i++ ) d[n * 64 + i] = s[n * 64 + i];
  }

  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcpy - reference variants

template<typename F, typename D>
F &
rmemcpy128(F &__restrict dest, const D &__restrict src, const u64 cnt) noexcept
{
  const u64 n = (cnt * sizeof(D)) / sizeof(i128);
  auto *__restrict d = reinterpret_cast<u8 *>(&dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(&src);
  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16(d + i * 16, s + i * 16);
  const u64 rem = (cnt * sizeof(D)) % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
  }
  return dest;
}

template<typename F, typename D>
F &
rmemcpy256(F &__restrict dest, const D &__restrict src, const u64 cnt) noexcept
{
  const u64 n = (cnt * sizeof(D)) / sizeof(i256);
  auto *__restrict d = reinterpret_cast<u8 *>(&dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(&src);
  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_32(d + i * 32, s + i * 32);
  const u64 rem = (cnt * sizeof(D)) % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) d[n * 32 + i] = s[n * 32 + i];
  }
  return dest;
}

template<typename F, typename D>
[[gnu::target("avx512f")]] F &
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
    for ( u64 i = 0; i < rem; i++ ) d[n * 64 + i] = s[n * 64 + i];
  }
  return dest;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmove - unaligned

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
    // non-overlapping or dest is before src: forward copy
    const u64 n = bytes / 16;
    for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_16(d + i * 16, s + i * 16);
    const u64 rem = bytes % 16;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ ) d[n * 16 + i] = s[n * 16 + i];
    }
  } else {
    // overlapping: dest is after src, must copy backward to avoid clobbering
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
memmove256(T *dest, const T *src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove256 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  const auto *s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 ) return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 32;
    for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_32(d + i * 32, s + i * 32);
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ ) d[n * 32 + i] = s[n * 32 + i];
    }
  } else {
    // overlapping backward path: use __block_move_32 (no __restrict)
    const u64 n = bytes / 32;
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 32 + i - 1] = s[n * 32 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) __bits::__block_move_32(d + (i - 1) * 32, s + (i - 1) * 32);
  }

  return dest;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
memmove512(T *dest, const T *src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "memmove512 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(dest);
  const auto *s = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 ) return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 64;
    for ( u64 i = 0; i < n; i++ ) {
      i512 pkt = _mm512_loadu_si512(reinterpret_cast<const i512 *>(s + i * 64));
      _mm512_storeu_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
    }
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ ) d[n * 64 + i] = s[n * 64 + i];
    }
  } else {
    const u64 n = bytes / 64;
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 64 + i - 1] = s[n * 64 + i - 1];
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
    // overlapping backward path: __block_move_16 (no __restrict)
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
amemmove256(T *dest, const T *src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemmove256 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 32));
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 32));
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 ) return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 32;
    for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_32_a(d + i * 32, s + i * 32);
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ ) d[n * 32 + i] = s[n * 32 + i];
    }
  } else {
    const u64 n = bytes / 32;
    const u64 rem = bytes % 32;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 32 + i - 1] = s[n * 32 + i - 1];
    }
    for ( u64 i = n; i > 0; i-- ) __bits::__block_move_32(d + (i - 1) * 32, s + (i - 1) * 32);
  }

  return dest;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
amemmove512(T *dest, const T *src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "amemmove512 requires trivially copyable type");

  auto *d = reinterpret_cast<u8 *>(__builtin_assume_aligned(dest, 64));
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 64));
  const u64 bytes = count * sizeof(T);

  if ( d == s || bytes == 0 ) return dest;

  if ( d < s || d >= s + bytes ) {
    const u64 n = bytes / 64;
    for ( u64 i = 0; i < n; i++ ) {
      i512 pkt = _mm512_load_si512(reinterpret_cast<const i512 *>(s + i * 64));
      _mm512_store_si512(reinterpret_cast<i512 *>(d + i * 64), pkt);
    }
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = 0; i < rem; i++ ) d[n * 64 + i] = s[n * 64 + i];
    }
  } else {
    const u64 n = bytes / 64;
    const u64 rem = bytes % 64;
    if ( rem ) {
      for ( u64 i = rem; i > 0; i-- ) d[n * 64 + i - 1] = s[n * 64 + i - 1];
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

template<typename T>
__attribute__((nonnull)) T *
memset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  const i128 v = __bits::__broadcast_byte_16(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16(s + i * 16, v);

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 16 + i] = in;
  }

  return src;
}

template<typename T>
__attribute__((nonnull)) T *
memset256(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  const i256 v = __bits::__broadcast_byte_32(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_32(s + i * 32, v);

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 32 + i] = in;
  }

  return src;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
memset512(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  const i512 v = _mm512_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ ) _mm512_storeu_si512(reinterpret_cast<i512 *>(s + i * 64), v);

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 64 + i] = in;
  }

  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memset - aligned

template<typename T>
__attribute__((nonnull)) T *
amemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  const i128 v = __bits::__broadcast_byte_16(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16_a(s + i * 16, v);

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 16 + i] = in;
  }

  return src;
}

template<typename T>
__attribute__((nonnull)) T *
amemset256(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 32));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  const i256 v = __bits::__broadcast_byte_32(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_32_a(s + i * 32, v);

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 32 + i] = in;
  }

  return src;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
amemset512(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 64));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  const i512 v = _mm512_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ ) _mm512_storeu_si512(reinterpret_cast<i512 *>(s + i * 64), v);

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 64 + i] = in;
  }

  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memset - non-temporal

template<typename T>
__attribute__((nonnull)) T *
ntmemset128(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  const i128 v = __bits::__broadcast_byte_16(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16_nt(s + i * 16, v);

  _mm_sfence();

  const u64 rem = bytes % 16;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 16 + i] = in;
  }

  return src;
}

template<typename T>
__attribute__((nonnull)) T *
ntmemset256(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 32));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  const i256 v = __bits::__broadcast_byte_32(in);

  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_32_nt(s + i * 32, v);

  _mm_sfence();

  const u64 rem = bytes % 32;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 32 + i] = in;
  }

  return src;
}

template<typename T>
__attribute__((nonnull, target("avx512f"))) T *
ntmemset512(T *__restrict src, const u8 in, const u64 count) noexcept
{
  auto *s = reinterpret_cast<u8 *>(__builtin_assume_aligned(src, 64));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 64;

  const i512 v = _mm512_set1_epi8(static_cast<char>(in));

  for ( u64 i = 0; i < n; i++ ) _mm512_stream_si512(reinterpret_cast<i512 *>(s + i * 64), v);

  _mm_sfence();

  const u64 rem = bytes % 64;
  if ( rem ) {
    for ( u64 i = 0; i < rem; i++ ) s[n * 64 + i] = in;
  }

  return src;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcmp - unaligned

template<typename T>
__attribute__((nonnull)) i64
memcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(src);
  const auto *d = reinterpret_cast<const u8 *>(dest);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    const u32 mask = __bits::__block_neq_mask_16(s + i * 16, d + i * 16);
    if ( mask ) {
      const u64 idx = i * 16 + __builtin_ctz(mask);
      return static_cast<i64>(static_cast<unsigned>(s[idx])) - static_cast<i64>(static_cast<unsigned>(d[idx]));
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
memcmp256(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(src);
  const auto *d = reinterpret_cast<const u8 *>(dest);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    const u32 mask = __bits::__block_neq_mask_32(s + i * 32, d + i * 32);
    if ( mask ) {
      const u64 idx = i * 32 + __builtin_ctz(mask);
      return static_cast<i64>(static_cast<unsigned>(s[idx])) - static_cast<i64>(static_cast<unsigned>(d[idx]));
    }
  }

  const u64 rem = bytes % 32;
  if ( rem ) {
    const u64 base = n * 32;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(static_cast<unsigned>(s[base + i])) - static_cast<i64>(static_cast<unsigned>(d[base + i]));
  }

  return 0;
}

template<typename T>
__attribute__((nonnull, target("avx512bw,avx512f"))) i64
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
        if ( s[j] != d[j] ) return static_cast<i64>(static_cast<unsigned>(s[j])) - static_cast<i64>(static_cast<unsigned>(d[j]));
    }
  }

  const u64 rem = bytes % 64;
  if ( rem ) {
    const u64 base = n * 64;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(static_cast<unsigned>(s[base + i])) - static_cast<i64>(static_cast<unsigned>(d[base + i]));
  }

  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcmp - aligned

template<typename T>
__attribute__((nonnull)) i64
amemcmp128(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 16));
  const auto *d = reinterpret_cast<const u8 *>(__builtin_assume_aligned(dest, 16));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 16;

  for ( u64 i = 0; i < n; i++ ) {
    const u32 mask = __bits::__block_neq_mask_16(s + i * 16, d + i * 16);
    if ( mask ) {
      const u64 idx = i * 16 + __builtin_ctz(mask);
      return static_cast<i64>(static_cast<unsigned>(s[idx])) - static_cast<i64>(static_cast<unsigned>(d[idx]));
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
amemcmp256(const T *__restrict src, const T *__restrict dest, const u64 count) noexcept
{
  const auto *s = reinterpret_cast<const u8 *>(__builtin_assume_aligned(src, 32));
  const auto *d = reinterpret_cast<const u8 *>(__builtin_assume_aligned(dest, 32));
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    const u32 mask = __bits::__block_neq_mask_32(s + i * 32, d + i * 32);
    if ( mask ) {
      const u64 idx = i * 32 + __builtin_ctz(mask);
      return static_cast<i64>(static_cast<unsigned>(s[idx])) - static_cast<i64>(static_cast<unsigned>(d[idx]));
    }
  }

  const u64 rem = bytes % 32;
  if ( rem ) {
    const u64 base = n * 32;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(static_cast<unsigned>(s[base + i])) - static_cast<i64>(static_cast<unsigned>(d[base + i]));
  }

  return 0;
}

template<typename T>
__attribute__((nonnull, target("avx512bw,avx512f"))) i64
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
        if ( s[j] != d[j] ) return static_cast<i64>(static_cast<unsigned>(s[j])) - static_cast<i64>(static_cast<unsigned>(d[j]));
    }
  }

  const u64 rem = bytes % 64;
  if ( rem ) {
    const u64 base = n * 64;
    for ( u64 i = 0; i < rem; i++ )
      if ( s[base + i] != d[base + i] )
        return static_cast<i64>(static_cast<unsigned>(s[base + i])) - static_cast<i64>(static_cast<unsigned>(d[base + i]));
  }

  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wordset - splat a u64 across the buffer (bytes count, not word count)

__attribute__((nonnull)) static inline u8 *
wordset128(u8 *__restrict src, const u64 in, const u64 bytes) noexcept
{
  const u64 n = bytes / 16;
  const __m128i v = __bits::__broadcast_word_16(in);
  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_16(src + i * 16, v);
  const u64 rem = bytes % 16;
  if ( rem ) {
    u8 buf[16];
    __bits::__block_set_16(buf, v);
    for ( u64 i = 0; i < rem; i++ ) src[n * 16 + i] = buf[i];
  }
  return src;
}

#if defined(__micron_x86_avx2)
__attribute__((nonnull, target("avx2"))) static inline u8 *
wordset256(u8 *__restrict src, const u64 in, const u64 bytes) noexcept
{
  const u64 n = bytes / 32;
  const __m256i v = __bits::__broadcast_word_32(in);
  for ( u64 i = 0; i < n; i++ ) __bits::__block_set_32(src + i * 32, v);
  const u64 rem = bytes % 32;
  if ( rem ) {
    u8 buf[32];
    __bits::__block_set_32(buf, v);
    for ( u64 i = 0; i < rem; i++ ) src[n * 32 + i] = buf[i];
  }
  return src;
}
#endif

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
    const u32 m = __bits::__block_eq_mask_16(p + i * 16, c);
    if ( m ) return const_cast<T *>(reinterpret_cast<const T *>(p + i * 16 + __builtin_ctz(m)));
  }

  const u64 base = n * 16;
  const u64 rem = bytes % 16;
  for ( u64 i = 0; i < rem; i++ )
    if ( p[base + i] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + base + i));

  return nullptr;
}

#if defined(__micron_x86_avx2)
template<typename T>
__attribute__((nonnull, target("avx2"))) T *
memchr256(const T *src, u8 c, const u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) {
    const u32 m = __bits::__block_eq_mask_32(p + i * 32, c);
    if ( m ) return const_cast<T *>(reinterpret_cast<const T *>(p + i * 32 + __builtin_ctz(m)));
  }

  const u64 base = n * 32;
  const u64 rem = bytes % 32;
  for ( u64 i = 0; i < rem; i++ )
    if ( p[base + i] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + base + i));

  return nullptr;
}
#endif

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
    const u32 m = __bits::__block_eq_mask_16(p + (i - 1) * 16, c);
    if ( m ) {
      const u32 hi = 31u - __builtin_clz(m);
      return const_cast<T *>(reinterpret_cast<const T *>(p + (i - 1) * 16 + hi));
    }
  }

  return nullptr;
}

#if defined(__micron_x86_avx2)
template<typename T>
__attribute__((nonnull, target("avx2"))) T *
memrchr256(const T *src, u8 c, const u64 count) noexcept
{
  const auto *p = reinterpret_cast<const u8 *>(src);
  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;
  const u64 rem = bytes % 32;
  const u64 base = n * 32;

  for ( u64 i = rem; i > 0; i-- )
    if ( p[base + i - 1] == c ) return const_cast<T *>(reinterpret_cast<const T *>(p + base + i - 1));

  for ( u64 i = n; i > 0; i-- ) {
    const u32 m = __bits::__block_eq_mask_32(p + (i - 1) * 32, c);
    if ( m ) {
      const u32 hi = 31u - __builtin_clz(m);
      return const_cast<T *>(reinterpret_cast<const T *>(p + (i - 1) * 32 + hi));
    }
  }

  return nullptr;
}
#endif

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

#if defined(__micron_x86_avx2)
template<typename T>
__attribute__((nonnull, target("avx2"))) T *
mempcpy256(T *__restrict dest, const T *__restrict src, const u64 count) noexcept
{
  static_assert(micron::is_trivially_copyable_v<T>, "mempcpy256 requires trivially copyable type");

  auto *__restrict d = reinterpret_cast<u8 *>(dest);
  const auto *__restrict s = reinterpret_cast<const u8 *>(src);

  const u64 bytes = count * sizeof(T);
  const u64 n = bytes / 32;

  for ( u64 i = 0; i < n; i++ ) __bits::__block_copy_32(d + i * 32, s + i * 32);

  const u64 rem = bytes % 32;
  if ( rem ) {
    u8 tmp[32] = {};
    for ( u64 i = 0; i < rem; i++ ) tmp[i] = s[n * 32 + i];
    for ( u64 i = 0; i < rem; i++ ) d[n * 32 + i] = tmp[i];
  }

  return reinterpret_cast<T *>(d + bytes);
}
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memmem

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
    const u32 m = __bits::__block_eq_mask_16(h + i, first);
    if ( m ) {
      u32 mm = m;
      while ( mm ) {
        const u32 off = __builtin_ctz(mm);
        const u64 cand = i + off;
        if ( cand >= limit ) return nullptr;
        if ( nbytes == 1 ) return const_cast<T *>(reinterpret_cast<const T *>(h + cand));
        bool match = true;
        u64 j = 1;
        while ( j + 16 <= nbytes ) {
          if ( __bits::__block_neq_mask_16(h + cand + j, ne + j) ) {
            match = false;
            break;
          }
          j += 16;
        }
        if ( match ) {
          for ( ; j < nbytes; j++ )
            if ( h[cand + j] != ne[j] ) {
              match = false;
              break;
            }
        }
        if ( match ) return const_cast<T *>(reinterpret_cast<const T *>(h + cand));
        mm &= mm - 1u;
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

#if defined(__micron_x86_avx2)
template<typename T>
__attribute__((target("avx2"))) T *
memmem256(const T *hay, const u64 hlen, const T *nee, const u64 nlen) noexcept
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
  while ( i + 32 <= limit ) {
    const u32 m = __bits::__block_eq_mask_32(h + i, first);
    if ( m ) {
      u32 mm = m;
      while ( mm ) {
        const u32 off = __builtin_ctz(mm);
        const u64 cand = i + off;
        if ( cand >= limit ) return nullptr;
        if ( nbytes == 1 ) return const_cast<T *>(reinterpret_cast<const T *>(h + cand));
        bool match = true;
        u64 j = 1;
        while ( j + 32 <= nbytes ) {
          if ( __bits::__block_neq_mask_32(h + cand + j, ne + j) ) {
            match = false;
            break;
          }
          j += 32;
        }
        if ( match ) {
          while ( j + 16 <= nbytes ) {
            if ( __bits::__block_neq_mask_16(h + cand + j, ne + j) ) {
              match = false;
              break;
            }
            j += 16;
          }
        }
        if ( match ) {
          for ( ; j < nbytes; j++ )
            if ( h[cand + j] != ne[j] ) {
              match = false;
              break;
            }
        }
        if ( match ) return const_cast<T *>(reinterpret_cast<const T *>(h + cand));
        mm &= mm - 1u;
      }
    }
    i += 32;
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
#endif

#if defined(__micron_x86_avx2)

__attribute__((nonnull)) inline u8 *
__memset_bulk(u8 *__restrict d, const u8 v, const u64 n) noexcept
{
  const __m256i vv = __bits::__broadcast_byte_32(v);
  __bits::__block_set_32(d, vv);
  u8 *p = reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(d) + 32) & ~static_cast<uintptr_t>(31));
  u8 *const e = d + n;
  for ( ; p + 128 <= e; p += 128 ) {
    __bits::__block_set_32_a(p, vv);
    __bits::__block_set_32_a(p + 32, vv);
    __bits::__block_set_32_a(p + 64, vv);
    __bits::__block_set_32_a(p + 96, vv);
  }
  __bits::__block_set_32(e - 128, vv);
  __bits::__block_set_32(e - 96, vv);
  __bits::__block_set_32(e - 64, vv);
  __bits::__block_set_32(e - 32, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memset_bulk_nt(u8 *__restrict d, const u8 v, const u64 n) noexcept
{
  const __m256i vv = __bits::__broadcast_byte_32(v);
  __bits::__block_set_32(d, vv);
  u8 *p = reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(d) + 32) & ~static_cast<uintptr_t>(31));
  u8 *const e = d + n;
  for ( ; p + 128 <= e; p += 128 ) {
    __bits::__block_set_32_nt(p, vv);
    __bits::__block_set_32_nt(p + 32, vv);
    __bits::__block_set_32_nt(p + 64, vv);
    __bits::__block_set_32_nt(p + 96, vv);
  }
  __bits::__sfence();
  __bits::__block_set_32(e - 128, vv);
  __bits::__block_set_32(e - 96, vv);
  __bits::__block_set_32(e - 64, vv);
  __bits::__block_set_32(e - 32, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__wordset_bulk(u8 *__restrict d, const u64 w, const u64 n) noexcept
{
  // WARNING: n must be > 64 and n % period[8] == 0, not guarding behind branch
  // dst is aligned for the bulk loop by rotating the pattern to the aligned
  // phase
  const __m256i vv = __bits::__broadcast_word_32(w);
  if ( n <= 256 ) {
    u64 i = 0;
    for ( ; i + 32 <= n; i += 32 ) __bits::__block_set_32(d + i, vv);
    if ( i < n ) __bits::__block_set_32(d + n - 32, vv);
    return d;
  }
  __bits::__block_set_32(d, vv);
  u8 *p = reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(d) + 32) & ~static_cast<uintptr_t>(31));
  const u32 rot = (static_cast<u32>(p - d) & 7u) * 8u;
  const u64 w2 = rot ? ((w >> rot) | (w << (64u - rot))) : w;
  const __m256i vv2 = __bits::__broadcast_word_32(w2);
  u8 *const e = d + n;
  for ( ; p + 128 <= e; p += 128 ) {
    __bits::__block_set_32_a(p, vv2);
    __bits::__block_set_32_a(p + 32, vv2);
    __bits::__block_set_32_a(p + 64, vv2);
    __bits::__block_set_32_a(p + 96, vv2);
  }
  for ( ; p + 32 <= e; p += 32 ) __bits::__block_set_32_a(p, vv2);
  if ( p < e ) __bits::__block_set_32(e - 32, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memcpy_bulk(u8 *__restrict d, const u8 *__restrict s, const u64 n) noexcept
{
  // NOTE: loop batches all 4 loads before any store; interleaving puts every load in the shadow of a store it can 4K alias against
  // leading to a 2x collapse when src/dst are not 32-co-aligned; on HSW-E/Zen(sparse testing on Zen?)
  const __ml::__v32 h = __ml::__ld32(s);
  const __ml::__v32 t0 = __ml::__ld32(s + n - 128);
  const __ml::__v32 t1 = __ml::__ld32(s + n - 96);
  const __ml::__v32 t2 = __ml::__ld32(s + n - 64);
  const __ml::__v32 t3 = __ml::__ld32(s + n - 32);
  __ml::__st32(d, h);
  const u64 off = 32 - (reinterpret_cast<uintptr_t>(d) & 31);
  u8 *pd = d + off;
  const u8 *ps = s + off;
  u64 rem = n - off;
  for ( ; rem >= 128; rem -= 128, pd += 128, ps += 128 ) {
    __ml::__v32 a = __ml::__ld32(ps);
    __ml::__v32 b = __ml::__ld32(ps + 32);
    __ml::__v32 c = __ml::__ld32(ps + 64);
    __ml::__v32 e = __ml::__ld32(ps + 96);
    __ml::__pin4(a, b, c, e);
    __ml::__st32(pd, a);
    __ml::__st32(pd + 32, b);
    __ml::__st32(pd + 64, c);
    __ml::__st32(pd + 96, e);
  }
  __ml::__st32(d + n - 128, t0);
  __ml::__st32(d + n - 96, t1);
  __ml::__st32(d + n - 64, t2);
  __ml::__st32(d + n - 32, t3);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memcpy_bulk_nt(u8 *__restrict d, const u8 *__restrict s, const u64 n) noexcept
{
  __bits::__block_copy_32(d, s);
  const u64 off = 32 - (reinterpret_cast<uintptr_t>(d) & 31);
  u8 *pd = d + off;
  const u8 *ps = s + off;
  u64 rem = n - off;
  for ( ; rem >= 128; rem -= 128, pd += 128, ps += 128 ) {
    __bits::__prefetch_t0(ps + 256);
    __bits::__prefetch_t0(ps + 320);
    __ml::__v32 a = __ml::__ld32(ps);
    __ml::__v32 b = __ml::__ld32(ps + 32);
    __ml::__v32 c = __ml::__ld32(ps + 64);
    __ml::__v32 e = __ml::__ld32(ps + 96);
    __ml::__pin4(a, b, c, e);
    __bits::__block_set_32_nt(pd, reinterpret_cast<__m256i>(a));
    __bits::__block_set_32_nt(pd + 32, reinterpret_cast<__m256i>(b));
    __bits::__block_set_32_nt(pd + 64, reinterpret_cast<__m256i>(c));
    __bits::__block_set_32_nt(pd + 96, reinterpret_cast<__m256i>(e));
  }
  __bits::__sfence();
  __bits::__block_copy_32(d + n - 128, s + n - 128);
  __bits::__block_copy_32(d + n - 96, s + n - 96);
  __bits::__block_copy_32(d + n - 64, s + n - 64);
  __bits::__block_copy_32(d + n - 32, s + n - 32);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memmove_bulk_fwd(u8 *d, const u8 *s, const u64 n) noexcept
{
  const __ml::__v32 h = __ml::__ld32(s);
  const __ml::__v32 t0 = __ml::__ld32(s + n - 128);
  const __ml::__v32 t1 = __ml::__ld32(s + n - 96);
  const __ml::__v32 t2 = __ml::__ld32(s + n - 64);
  const __ml::__v32 t3 = __ml::__ld32(s + n - 32);
  const u64 off = 32 - (reinterpret_cast<uintptr_t>(d) & 31);
  u8 *pd = d + off;
  const u8 *ps = s + off;
  u64 rem = n - off;
  for ( ; rem >= 128; rem -= 128, pd += 128, ps += 128 ) {
    __ml::__v32 a = __ml::__ld32(ps);
    __ml::__v32 b = __ml::__ld32(ps + 32);
    __ml::__v32 c = __ml::__ld32(ps + 64);
    __ml::__v32 e = __ml::__ld32(ps + 96);
    __ml::__pin4(a, b, c, e);
    __ml::__st32(pd, a);
    __ml::__st32(pd + 32, b);
    __ml::__st32(pd + 64, c);
    __ml::__st32(pd + 96, e);
  }
  __ml::__st32(d + n - 128, t0);
  __ml::__st32(d + n - 96, t1);
  __ml::__st32(d + n - 64, t2);
  __ml::__st32(d + n - 32, t3);
  __ml::__st32(d, h);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memmove_bulk_bwd(u8 *d, const u8 *s, const u64 n) noexcept
{
  const __ml::__v32 t = __ml::__ld32(s + n - 32);
  const __ml::__v32 h0 = __ml::__ld32(s);
  const __ml::__v32 h1 = __ml::__ld32(s + 32);
  const __ml::__v32 h2 = __ml::__ld32(s + 64);
  const __ml::__v32 h3 = __ml::__ld32(s + 96);
  u8 *pe = reinterpret_cast<u8 *>(reinterpret_cast<uintptr_t>(d + n) & ~static_cast<uintptr_t>(31));
  const u8 *pse = s + (pe - d);
  while ( static_cast<u64>(pe - d) > 128 ) {
    pe -= 128;
    pse -= 128;
    __ml::__v32 a = __ml::__ld32(pse + 96);
    __ml::__v32 b = __ml::__ld32(pse + 64);
    __ml::__v32 c = __ml::__ld32(pse + 32);
    __ml::__v32 e = __ml::__ld32(pse);
    __ml::__pin4(a, b, c, e);
    __ml::__st32(pe + 96, a);
    __ml::__st32(pe + 64, b);
    __ml::__st32(pe + 32, c);
    __ml::__st32(pe, e);
  }
  __ml::__st32(d, h0);
  __ml::__st32(d + 32, h1);
  __ml::__st32(d + 64, h2);
  __ml::__st32(d + 96, h3);
  __ml::__st32(d + n - 32, t);
  return d;
}

#else
// SSE2 fallback variants
__attribute__((nonnull)) inline u8 *
__memset_bulk(u8 *__restrict d, const u8 v, const u64 n) noexcept
{
  const __m128i vv = __bits::__broadcast_byte_16(v);
  __bits::__block_set_16(d, vv);
  u8 *p = reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(d) + 16) & ~static_cast<uintptr_t>(15));
  u8 *const e = d + n;
  for ( ; p + 64 <= e; p += 64 ) {
    __bits::__block_set_16_a(p, vv);
    __bits::__block_set_16_a(p + 16, vv);
    __bits::__block_set_16_a(p + 32, vv);
    __bits::__block_set_16_a(p + 48, vv);
  }
  __bits::__block_set_16(e - 64, vv);
  __bits::__block_set_16(e - 48, vv);
  __bits::__block_set_16(e - 32, vv);
  __bits::__block_set_16(e - 16, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memset_bulk_nt(u8 *__restrict d, const u8 v, const u64 n) noexcept
{
  const __m128i vv = __bits::__broadcast_byte_16(v);
  __bits::__block_set_16(d, vv);
  u8 *p = reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(d) + 16) & ~static_cast<uintptr_t>(15));
  u8 *const e = d + n;
  for ( ; p + 64 <= e; p += 64 ) {
    __bits::__block_set_16_nt(p, vv);
    __bits::__block_set_16_nt(p + 16, vv);
    __bits::__block_set_16_nt(p + 32, vv);
    __bits::__block_set_16_nt(p + 48, vv);
  }
  __bits::__sfence();
  __bits::__block_set_16(e - 64, vv);
  __bits::__block_set_16(e - 48, vv);
  __bits::__block_set_16(e - 32, vv);
  __bits::__block_set_16(e - 16, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__wordset_bulk(u8 *__restrict d, const u64 w, const u64 n) noexcept
{
  const __m128i vv = __bits::__broadcast_word_16(w);
  if ( n <= 256 ) {
    u64 i = 0;
    for ( ; i + 16 <= n; i += 16 ) __bits::__block_set_16(d + i, vv);
    if ( i < n ) __bits::__block_set_16(d + n - 16, vv);
    return d;
  }
  __bits::__block_set_16(d, vv);
  u8 *p = reinterpret_cast<u8 *>((reinterpret_cast<uintptr_t>(d) + 16) & ~static_cast<uintptr_t>(15));
  const u32 rot = (static_cast<u32>(p - d) & 7u) * 8u;
  const u64 w2 = rot ? ((w >> rot) | (w << (64u - rot))) : w;
  const __m128i vv2 = __bits::__broadcast_word_16(w2);
  u8 *const e = d + n;
  for ( ; p + 64 <= e; p += 64 ) {
    __bits::__block_set_16_a(p, vv2);
    __bits::__block_set_16_a(p + 16, vv2);
    __bits::__block_set_16_a(p + 32, vv2);
    __bits::__block_set_16_a(p + 48, vv2);
  }
  for ( ; p + 16 <= e; p += 16 ) __bits::__block_set_16_a(p, vv2);
  if ( p < e ) __bits::__block_set_16(e - 16, vv);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memcpy_bulk(u8 *__restrict d, const u8 *__restrict s, const u64 n) noexcept
{
  const __ml::__v16 h = __ml::__ld16(s);
  const __ml::__v16 t0 = __ml::__ld16(s + n - 64);
  const __ml::__v16 t1 = __ml::__ld16(s + n - 48);
  const __ml::__v16 t2 = __ml::__ld16(s + n - 32);
  const __ml::__v16 t3 = __ml::__ld16(s + n - 16);
  __ml::__st16(d, h);
  const u64 off = 16 - (reinterpret_cast<uintptr_t>(d) & 15);
  u8 *pd = d + off;
  const u8 *ps = s + off;
  u64 rem = n - off;
  for ( ; rem >= 64; rem -= 64, pd += 64, ps += 64 ) {
    __ml::__v16 a = __ml::__ld16(ps);
    __ml::__v16 b = __ml::__ld16(ps + 16);
    __ml::__v16 c = __ml::__ld16(ps + 32);
    __ml::__v16 e = __ml::__ld16(ps + 48);
    __ml::__pin4(a, b, c, e);
    __ml::__st16(pd, a);
    __ml::__st16(pd + 16, b);
    __ml::__st16(pd + 32, c);
    __ml::__st16(pd + 48, e);
  }
  __ml::__st16(d + n - 64, t0);
  __ml::__st16(d + n - 48, t1);
  __ml::__st16(d + n - 32, t2);
  __ml::__st16(d + n - 16, t3);
  return d;
}

__attribute__((nonnull)) inline u8 *
__memcpy_bulk_nt(u8 *__restrict d, const u8 *__restrict s, const u64 n) noexcept
{
  __bits::__block_copy_16(d, s);
  const u64 off = 16 - (reinterpret_cast<uintptr_t>(d) & 15);
  u8 *pd = d + off;
  const u8 *ps = s + off;
  u64 rem = n - off;
  for ( ; rem >= 64; rem -= 64, pd += 64, ps += 64 ) {
    __bits::__prefetch_t0(ps + 256);
    __ml::__v16 a = __ml::__ld16(ps);
    __ml::__v16 b = __ml::__ld16(ps + 16);
    __ml::__v16 c = __ml::__ld16(ps + 32);
    __ml::__v16 e = __ml::__ld16(ps + 48);
    __ml::__pin4(a, b, c, e);
    __bits::__block_set_16_nt(pd, reinterpret_cast<__m128i>(a));
    __bits::__block_set_16_nt(pd + 16, reinterpret_cast<__m128i>(b));
    __bits::__block_set_16_nt(pd + 32, reinterpret_cast<__m128i>(c));
    __bits::__block_set_16_nt(pd + 48, reinterpret_cast<__m128i>(e));
  }
  __bits::__sfence();
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
    __ml::__v16 a = __ml::__ld16(ps);
    __ml::__v16 b = __ml::__ld16(ps + 16);
    __ml::__v16 c = __ml::__ld16(ps + 32);
    __ml::__v16 e = __ml::__ld16(ps + 48);
    __ml::__pin4(a, b, c, e);
    __ml::__st16(pd, a);
    __ml::__st16(pd + 16, b);
    __ml::__st16(pd + 32, c);
    __ml::__st16(pd + 48, e);
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
    __ml::__v16 a = __ml::__ld16(pse + 48);
    __ml::__v16 b = __ml::__ld16(pse + 32);
    __ml::__v16 c = __ml::__ld16(pse + 16);
    __ml::__v16 e = __ml::__ld16(pse);
    __ml::__pin4(a, b, c, e);
    __ml::__st16(pe + 48, a);
    __ml::__st16(pe + 32, b);
    __ml::__st16(pe + 16, c);
    __ml::__st16(pe, e);
  }
  __ml::__st16(d, h0);
  __ml::__st16(d + 16, h1);
  __ml::__st16(d + 32, h2);
  __ml::__st16(d + 48, h3);
  __ml::__st16(d + n - 16, t);
  return d;
}

#endif      // __micron_x86_avx2

};      // namespace simd
};      // namespace micron
