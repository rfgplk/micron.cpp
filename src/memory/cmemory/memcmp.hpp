//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "bits.hpp"

#include "../addr.hpp"

#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

#include "../../numerics.hpp"

namespace micron
{

[[gnu::always_inline]] static inline u64
__cmp_word(const byte *p) noexcept
{
  u64 v;
  __builtin_memcpy(&v, p, 8);
  return v;
}

[[gnu::always_inline]] static inline i64
__cmp_word_sign(u64 x, u64 y) noexcept
{
  const u64 d = x ^ y;
  const unsigned sh = static_cast<unsigned>(__builtin_ctzll(d)) & ~7u;
  return static_cast<i64>((x >> sh) & 0xffu) - static_cast<i64>((y >> sh) & 0xffu);
}

[[gnu::always_inline]] static inline i64
__memcmp_bytes(const byte *__restrict a, const byte *__restrict b, const u64 bytes) noexcept
{
  if ( bytes == 0 ) return 0;
  if ( bytes < __simd_dispatch_threshold ) {
#if defined(__micron_endian_little)
    // short spans in overlapping words instead of a byte loop
    if ( bytes >= 8 ) {
      u64 i = 0;
      for ( ; i + 8 <= bytes; i += 8 ) {
        const u64 x = __cmp_word(a + i);
        const u64 y = __cmp_word(b + i);
        if ( x != y ) return __cmp_word_sign(x, y);
      }
      if ( i == bytes ) return 0;
      const u64 x = __cmp_word(a + bytes - 8);
      const u64 y = __cmp_word(b + bytes - 8);
      return x != y ? __cmp_word_sign(x, y) : 0;
    }
    if ( bytes >= 4 ) {
      u32 x0, y0, x1, y1;
      __builtin_memcpy(&x0, a, 4);
      __builtin_memcpy(&y0, b, 4);
      if ( x0 != y0 ) return __cmp_word_sign(x0, y0);
      __builtin_memcpy(&x1, a + bytes - 4, 4);
      __builtin_memcpy(&y1, b + bytes - 4, 4);
      return x1 != y1 ? __cmp_word_sign(x1, y1) : 0;
    }
#endif
    for ( u64 i = 0; i < bytes; i++ )
      if ( a[i] != b[i] ) return static_cast<i64>(static_cast<unsigned>(a[i])) - static_cast<i64>(static_cast<unsigned>(b[i]));
    return 0;
  }
#if defined(__micron_x86_avx512f)
  if ( bytes >= 64 ) return simd::memcmp512<byte>(a, b, bytes);
#endif
#if defined(__micron_x86_avx2)
  return simd::memcmp256<byte>(a, b, bytes);
#else
  return simd::memcmp128<byte>(a, b, bytes);
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%
// memcmps

template<typename T>
constexpr i64
__memcmp_scalar(const T *src, const T *dest, const u64 cnt) noexcept
{
  if constexpr ( sizeof(T) == 1 ) {
    for ( u64 i = 0; i < cnt; i++ ) {
      const unsigned x = static_cast<unsigned char>(src[i]);
      const unsigned y = static_cast<unsigned char>(dest[i]);
      if ( x != y ) return static_cast<i64>(x) - static_cast<i64>(y);
    }
  } else {
    for ( u64 i = 0; i < cnt; i++ )
      if ( src[i] != dest[i] ) return src[i] < dest[i] ? -1 : 1;
  }
  return 0;
};

template<typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) constexpr i64
memcmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  if constexpr ( micron::is_same_v<T, F> ) {
    if !consteval {
      if constexpr ( sizeof(T) == 1 )
        return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), cnt);
    }
    return __memcmp_scalar<T>(_src, _dest, cnt);
  } else {
    const T *src = reinterpret_cast<const T *>(_src);
    const T *dest = reinterpret_cast<const T *>(_dest);
    if constexpr ( sizeof(T) == 1 )
      return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), cnt);
    else
      return __memcmp_scalar<T>(src, dest, cnt);
  }
};

template<typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
i64
rmemcmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  if constexpr ( sizeof(T) == 1 ) {
    return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), cnt);
  } else {
    return __memcmp_scalar<T>(src, dest, cnt);
  }
};

template<typename F>
constexpr i64
constexpr_memcmp(const F *src, const F *dest, const u64 cnt) noexcept
{
  return micron::memcmp<F, F>(src, dest, cnt);
};

template<u64 M, typename T>
constexpr i64
__cmemcmp_scalar(const T *src, const T *dest) noexcept
{
  const auto at = [](const T *p, u64 i) constexpr noexcept {
    if constexpr ( sizeof(T) == 1 )
      return static_cast<unsigned>(static_cast<unsigned char>(p[i]));
    else
      return p[i];
  };
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( at(src, i) != at(dest, i) ) return at(src, i) < at(dest, i) ? -1 : 1;
      if ( at(src, i + 1) != at(dest, i + 1) ) return at(src, i + 1) < at(dest, i + 1) ? -1 : 1;
      if ( at(src, i + 2) != at(dest, i + 2) ) return at(src, i + 2) < at(dest, i + 2) ? -1 : 1;
      if ( at(src, i + 3) != at(dest, i + 3) ) return at(src, i + 3) < at(dest, i + 3) ? -1 : 1;
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( at(src, i) != at(dest, i) ) return at(src, i) < at(dest, i) ? -1 : 1;
  return 0;
}

template<u64 M, typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) constexpr i64
cmemcmp(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  if constexpr ( micron::is_same_v<T, F> ) {
    if !consteval {
      if constexpr ( sizeof(T) == 1 ) return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), M);
    }
    return __cmemcmp_scalar<M, T>(_src, _dest);
  } else {
    const T *src = reinterpret_cast<const T *>(_src);
    const T *dest = reinterpret_cast<const T *>(_dest);
    if constexpr ( sizeof(T) == 1 ) return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), M);
    return __cmemcmp_scalar<M, T>(src, dest);
  }
};

template<u64 M, typename T, typename F>
constexpr i64
rcmemcmp(const F &_src, const F &_dest) noexcept
{
  if constexpr ( micron::is_same_v<T, F> ) {
    const T *src = __builtin_addressof(_src);
    const T *dest = __builtin_addressof(_dest);
    if !consteval {
      if constexpr ( sizeof(T) == 1 ) return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), M);
    }
    return __cmemcmp_scalar<M, T>(src, dest);
  } else if constexpr ( micron::is_same_v<micron::remove_cv_t<micron::remove_extent_t<F>>, T> ) {
    const T *src = __builtin_addressof(_src[0]);
    const T *dest = __builtin_addressof(_dest[0]);
    if !consteval {
      if constexpr ( sizeof(T) == 1 ) return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), M);
    }
    return __cmemcmp_scalar<M, T>(src, dest);
  } else {
    const T *src = reinterpret_cast<const T *>(__builtin_addressof(_src));
    const T *dest = reinterpret_cast<const T *>(__builtin_addressof(_dest));
    if constexpr ( sizeof(T) == 1 ) return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), M);
    return __cmemcmp_scalar<M, T>(src, dest);
  }
};

// SIMD MEMCMP VARIANTS, REQUIRES ALIGNMENT
// MOVED TO /simd

template<typename F>
inline i64
memcmp_8b(const F *src, const F *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] ) return 0;
  const byte *sa = reinterpret_cast<const byte *>(src);
  const byte *da = reinterpret_cast<const byte *>(dest);
  for ( int i = 0; i < 8; i++ )
    if ( sa[i] != da[i] ) return &sa[i] - &da[i];
  return 0;
};

template<typename F>
inline i64
memcmp_16b(const F *src, const F *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] ) return 0;
  const byte *sa = reinterpret_cast<const byte *>(src);
  const byte *da = reinterpret_cast<const byte *>(dest);
  for ( int i = 0; i < 16; i++ )
    if ( sa[i] != da[i] ) return &sa[i] - &da[i];
  return 0;
};

template<typename F>
inline i64
memcmp_32b(const F *src, const F *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] ) return 0;
  const byte *sa = reinterpret_cast<const byte *>(src);
  const byte *da = reinterpret_cast<const byte *>(dest);
  for ( int i = 0; i < 32; i++ )
    if ( sa[i] != da[i] ) return &sa[i] - &da[i];
  return 0;
};

template<typename F>
inline i64
memcmp_64b(const F *src, const F *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] && a[4] == b[4] && a[5] == b[5] && a[6] == b[6] && a[7] == b[7] )
    return 0;
  const byte *sa = reinterpret_cast<const byte *>(src);
  const byte *da = reinterpret_cast<const byte *>(dest);
  for ( int i = 0; i < 64; i++ )
    if ( sa[i] != da[i] ) return &sa[i] - &da[i];
  return 0;
};

template<typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
smemcmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  if ( _src == nullptr or _dest == nullptr ) return micron::numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return micron::numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) or !__is_valid_address(_dest, cnt) ) return micron::numeric_limits<i64>::min();
  if constexpr ( sizeof(T) == 1 ) {
    return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), cnt);
  } else {
    const T *src = reinterpret_cast<const T *>(_src);
    const T *dest = reinterpret_cast<const T *>(_dest);
    for ( u64 i = 0; i < cnt; i++ )
      if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
    return 0;
  }
};

template<typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
i64
rsmemcmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) or !__is_valid_address(_dest, cnt) ) return numeric_limits<i64>::min();
  if constexpr ( sizeof(T) == 1 ) {
    return __memcmp_bytes(reinterpret_cast<const byte *>(&_src), reinterpret_cast<const byte *>(&_dest), cnt);
  } else {
    const T *src = reinterpret_cast<const T *>(&_src);
    const T *dest = reinterpret_cast<const T *>(&_dest);
    for ( u64 i = 0; i < cnt; i++ )
      if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
    return 0;
  }
};

template<u64 M, typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
scmemcmp_safe(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  if ( _src == nullptr or _dest == nullptr ) return numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) or !__is_valid_address(_dest, M) ) return numeric_limits<i64>::min();
  if constexpr ( sizeof(T) == 1 ) {
    return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), M);
  } else {
    const T *src = reinterpret_cast<const T *>(_src);
    const T *dest = reinterpret_cast<const T *>(_dest);
    if constexpr ( M % 4 == 0 )
      for ( u64 i = 0; i < M; i += 4 ) {
        if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
        if ( src[i + 1] != dest[i + 1] ) return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
        if ( src[i + 2] != dest[i + 2] ) return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
        if ( src[i + 3] != dest[i + 3] ) return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
      }
    else
      for ( u64 i = 0; i < M; i++ )
        if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
    return 0;
  }
};

template<u64 M, typename T, typename F, u64 alignment = alignof(T)>
i64
rscmemcmp_safe(const F &_src, const F &_dest) noexcept
{
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) or !__is_valid_address(_dest, M) ) return numeric_limits<i64>::min();
  if constexpr ( sizeof(T) == 1 ) {
    return __memcmp_bytes(reinterpret_cast<const byte *>(&_src), reinterpret_cast<const byte *>(&_dest), M);
  } else {
    const T *src = reinterpret_cast<const T *>(&_src);
    const T *dest = reinterpret_cast<const T *>(&_dest);
    if constexpr ( M % 4 == 0 )
      for ( u64 i = 0; i < M; i += 4 ) {
        if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
        if ( src[i + 1] != dest[i + 1] ) return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
        if ( src[i + 2] != dest[i + 2] ) return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
        if ( src[i + 3] != dest[i + 3] ) return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
      }
    else
      for ( u64 i = 0; i < M; i++ )
        if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
    return 0;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bytecmps

__attribute__((nonnull)) i64
bytecmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  return __memcmp_bytes(src, dest, cnt);
};

__attribute__((nonnull)) i64
bcmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  return bytecmp(src, dest, cnt);
};

i64
rbytecmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  return __memcmp_bytes(&src, &dest, cnt);
};

i64
rbcmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  return rbytecmp(src, dest, cnt);
};

template<u64 N>
__attribute__((nonnull)) i64
cbytecmp(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  return __memcmp_bytes(src, dest, N);
};

template<u64 N>
__attribute__((nonnull)) i64
cbcmp(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  return cbytecmp<N>(src, dest);
};

template<u64 N>
i64
rcbytecmp(const byte &src, const byte &dest) noexcept
{
  return __memcmp_bytes(&src, &dest, N);
};

template<u64 N>
i64
rcbcmp(const byte &src, const byte &dest) noexcept
{
  return rcbytecmp<N>(src, dest);
};

inline i64
bytecmp_8b(const byte *src, const byte *dest) noexcept
{
  const u64 a = *reinterpret_cast<const u64 *>(src);
  const u64 b = *reinterpret_cast<const u64 *>(dest);
  if ( a == b ) return 0;
  for ( int i = 0; i < 8; i++ )
    if ( src[i] != dest[i] ) return &src[i] - &dest[i];
  return 0;
};

inline i64
bcmp_8b(const byte *src, const byte *dest) noexcept
{
  return bytecmp_8b(src, dest);
};

inline i64
bytecmp_16b(const byte *src, const byte *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] ) return 0;
  for ( int i = 0; i < 16; i++ )
    if ( src[i] != dest[i] ) return &src[i] - &dest[i];
  return 0;
};

inline i64
bcmp_16b(const byte *src, const byte *dest) noexcept
{
  return bytecmp_16b(src, dest);
};

inline i64
bytecmp_32b(const byte *src, const byte *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] ) return 0;
  for ( int i = 0; i < 32; i++ )
    if ( src[i] != dest[i] ) return &src[i] - &dest[i];
  return 0;
};

inline i64
bcmp_32b(const byte *src, const byte *dest) noexcept
{
  return bytecmp_32b(src, dest);
};

inline i64
bytecmp_64b(const byte *src, const byte *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] && a[4] == b[4] && a[5] == b[5] && a[6] == b[6] && a[7] == b[7] )
    return 0;
  for ( int i = 0; i < 64; i++ )
    if ( src[i] != dest[i] ) return &src[i] - &dest[i];
  return 0;
};

inline i64
bcmp_64b(const byte *src, const byte *dest) noexcept
{
  return bytecmp_64b(src, dest);
};

template<u64 alignment = 1>
__attribute__((nonnull)) i64
sbytecmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  if ( src == nullptr or dest == nullptr ) return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) or !__is_aligned_to(dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) or !__is_valid_address(dest, cnt) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(src, dest, cnt);
};

template<u64 alignment = 1>
__attribute__((nonnull)) i64
sbcmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  return sbytecmp<alignment>(src, dest, cnt);
};

template<u64 alignment = 1>
i64
rsbytecmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(src, alignment) or !__is_aligned_to_r(dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) or !__is_valid_address(dest, cnt) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(&src, &dest, cnt);
};

template<u64 alignment = 1>
i64
rsbcmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  return rsbytecmp<alignment>(src, dest, cnt);
};

template<u64 N, u64 alignment = 1>
__attribute__((nonnull)) i64
scbytecmp_safe(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  if ( src == nullptr or dest == nullptr ) return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) or !__is_aligned_to(dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, N) or !__is_valid_address(dest, N) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(src, dest, N);
};

template<u64 N, u64 alignment = 1>
__attribute__((nonnull)) i64
scbcmp_safe(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  return scbytecmp_safe<N, alignment>(src, dest);
};

template<u64 N, u64 alignment = 1>
i64
rscbytecmp_safe(const byte &src, const byte &dest) noexcept
{
  if ( !__is_aligned_to_r(src, alignment) or !__is_aligned_to_r(dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, N) or !__is_valid_address(dest, N) ) return numeric_limits<i64>::min();
  const byte *s = &src;
  const byte *d = &dest;
  if constexpr ( N % 4 == 0 )
    for ( u64 i = 0; i < N; i += 4 ) {
      if ( s[i] != d[i] ) return &s[i] - &d[i];
      if ( s[i + 1] != d[i + 1] ) return &s[i + 1] - &d[i + 1];
      if ( s[i + 2] != d[i + 2] ) return &s[i + 2] - &d[i + 2];
      if ( s[i + 3] != d[i + 3] ) return &s[i + 3] - &d[i + 3];
    }
  else
    for ( u64 i = 0; i < N; i++ )
      if ( s[i] != d[i] ) return &s[i] - &d[i];
  return 0;
};

template<u64 N, u64 alignment = 1>
i64
rscbcmp_safe(const byte &src, const byte &dest) noexcept
{
  return rscbytecmp_safe<N, alignment>(src, dest);
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// typecmps

template<typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
typecmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), cnt * sizeof(T));
};

template<typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
i64
rtypecmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(&_src), reinterpret_cast<const byte *>(&_dest), cnt * sizeof(T));
};

template<u64 M, typename T, typename F>
__attribute__((nonnull)) i64
ctypecmp(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), M * sizeof(T));
};

template<u64 M, typename T, typename F>
i64
rctypecmp(const F &_src, const F &_dest) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(&_src), reinterpret_cast<const byte *>(&_dest), M * sizeof(T));
};

template<typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
stypecmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  if ( _src == nullptr or _dest == nullptr ) return numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) or !__is_valid_address(_dest, cnt) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), cnt * sizeof(T));
};

template<typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
i64
rstypecmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) or !__is_valid_address(_dest, cnt) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(reinterpret_cast<const byte *>(&_src), reinterpret_cast<const byte *>(&_dest), cnt * sizeof(T));
};

template<u64 M, typename T, typename F, u64 alignment = alignof(T)>
__attribute__((nonnull)) i64
sctypecmp_safe(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  if ( _src == nullptr or _dest == nullptr ) return numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) or !__is_valid_address(_dest, M) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(reinterpret_cast<const byte *>(_src), reinterpret_cast<const byte *>(_dest), M * sizeof(T));
};

template<u64 M, typename T, typename F, u64 alignment = alignof(T)>
i64
rsctypecmp_safe(const F &_src, const F &_dest) noexcept
{
  if ( !__is_aligned_to(_src, alignment) or !__is_aligned_to(_dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) or !__is_valid_address(_dest, M) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(reinterpret_cast<const byte *>(&_src), reinterpret_cast<const byte *>(&_dest), M * sizeof(T));
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wordcmps

__attribute__((nonnull)) i64
wordcmp(const word *__restrict src, const word *__restrict dest, const u64 cnt) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), cnt * sizeof(word));
};

i64
rwordcmp(const word &src, const word &dest, const u64 cnt) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(&src), reinterpret_cast<const byte *>(&dest), cnt * sizeof(word));
};

template<u64 M>
__attribute__((nonnull)) i64
cwordcmp(const word *__restrict src, const word *__restrict dest) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), M * sizeof(word));
};

template<u64 M>
i64
rcwordcmp(const word &src, const word &dest) noexcept
{
  return __memcmp_bytes(reinterpret_cast<const byte *>(&src), reinterpret_cast<const byte *>(&dest), M * sizeof(word));
};

inline i64
wordcmp_4w(const word *src, const word *dest) noexcept
{
  if ( src[0] != dest[0] ) return reinterpret_cast<const byte *>(&src[0]) - reinterpret_cast<const byte *>(&dest[0]);
  if ( src[1] != dest[1] ) return reinterpret_cast<const byte *>(&src[1]) - reinterpret_cast<const byte *>(&dest[1]);
  if ( src[2] != dest[2] ) return reinterpret_cast<const byte *>(&src[2]) - reinterpret_cast<const byte *>(&dest[2]);
  if ( src[3] != dest[3] ) return reinterpret_cast<const byte *>(&src[3]) - reinterpret_cast<const byte *>(&dest[3]);
  return 0;
};

inline i64
wordcmp_8w(const word *src, const word *dest) noexcept
{
  for ( int i = 0; i < 8; i++ )
    if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

inline i64
wordcmp_16w(const word *src, const word *dest) noexcept
{
  for ( int i = 0; i < 16; i++ )
    if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

inline i64
wordcmp_32w(const word *src, const word *dest) noexcept
{
  for ( int i = 0; i < 32; i++ )
    if ( src[i] != dest[i] ) return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template<u64 alignment = alignof(word)>
__attribute__((nonnull)) i64
swordcmp(const word *__restrict src, const word *__restrict dest, const u64 cnt) noexcept
{
  if ( src == nullptr or dest == nullptr ) return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) or !__is_aligned_to(dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) or !__is_valid_address(dest, cnt) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), cnt * sizeof(word));
};

template<u64 alignment = alignof(word)>
i64
rswordcmp(const word &src, const word &dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(src, alignment) or !__is_aligned_to_r(dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) or !__is_valid_address(dest, cnt) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(reinterpret_cast<const byte *>(&src), reinterpret_cast<const byte *>(&dest), cnt * sizeof(word));
};

// SAFE COMPILE-TIME CONSTANT WORDCMP - TEMPLATE COUNT ONLY
template<u64 M, u64 alignment = alignof(word)>
__attribute__((nonnull)) i64
scwordcmp_safe(const word *__restrict src, const word *__restrict dest) noexcept
{
  if ( src == nullptr or dest == nullptr ) return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) or !__is_aligned_to(dest, alignment) ) return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, M) or !__is_valid_address(dest, M) ) return numeric_limits<i64>::min();
  return __memcmp_bytes(reinterpret_cast<const byte *>(src), reinterpret_cast<const byte *>(dest), M * sizeof(word));
};

};      // namespace micron

#if defined(__micron_freestanding)
// c-abi
extern "C" __attribute__((used, optimize("-fno-tree-loop-distribute-patterns"))) inline int
memcmp(const void *a, const void *b, __SIZE_TYPE__ n) noexcept
{
  return static_cast<int>(micron::__memcmp_bytes(static_cast<const byte *>(a), static_cast<const byte *>(b), static_cast<u64>(n)));
}
#endif
