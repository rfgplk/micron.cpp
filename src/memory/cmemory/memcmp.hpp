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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// memcmps

template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
memcmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
i64
rmemcmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <typename F>
constexpr i64
constexpr_memcmp(const F *src, const F *dest, const u64 cnt) noexcept
{
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
cmemcmp(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F>
i64
rcmemcmp(const F &_src, const F &_dest) noexcept
{
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

// SIMD MEMCMP VARIANTS, REQUIRES ALIGNMENT
// MOVED TO /simd

template <typename F>
inline i64
memcmp_8b(const F *src, const F *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] )
    return 0;
  const byte *sa = reinterpret_cast<const byte *>(src);
  const byte *da = reinterpret_cast<const byte *>(dest);
  for ( int i = 0; i < 8; i++ )
    if ( sa[i] != da[i] )
      return &sa[i] - &da[i];
  return 0;
};

template <typename F>
inline i64
memcmp_16b(const F *src, const F *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] )
    return 0;
  const byte *sa = reinterpret_cast<const byte *>(src);
  const byte *da = reinterpret_cast<const byte *>(dest);
  for ( int i = 0; i < 16; i++ )
    if ( sa[i] != da[i] )
      return &sa[i] - &da[i];
  return 0;
};

template <typename F>
inline i64
memcmp_32b(const F *src, const F *dest) noexcept
{
  const u64 *a = reinterpret_cast<const u64 *>(src);
  const u64 *b = reinterpret_cast<const u64 *>(dest);
  if ( a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] )
    return 0;
  const byte *sa = reinterpret_cast<const byte *>(src);
  const byte *da = reinterpret_cast<const byte *>(dest);
  for ( int i = 0; i < 32; i++ )
    if ( sa[i] != da[i] )
      return &sa[i] - &da[i];
  return 0;
};

template <typename F>
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
    if ( sa[i] != da[i] )
      return &sa[i] - &da[i];
  return 0;
};

template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
smemcmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  if ( _src == nullptr || _dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) || !__is_valid_address(_dest, cnt) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
i64
rsmemcmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) || !__is_valid_address(_dest, cnt) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
scmemcmp_safe(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  if ( _src == nullptr || _dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) || !__is_valid_address(_dest, M) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
i64
rscmemcmp_safe(const F &_src, const F &_dest) noexcept
{
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) || !__is_valid_address(_dest, M) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bytecmps

__attribute__((nonnull)) i64
bytecmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
  return 0;
};

__attribute__((nonnull)) i64
bcmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  return bytecmp(src, dest, cnt);
};

i64
rbytecmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  const byte *s = &src;
  const byte *d = &dest;
  for ( u64 i = 0; i < cnt; i++ )
    if ( s[i] != d[i] )
      return &s[i] - &d[i];
  return 0;
};

i64
rbcmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  return rbytecmp(src, dest, cnt);
};

template <u64 N>
__attribute__((nonnull)) i64
cbytecmp(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  if constexpr ( N % 4 == 0 )
    for ( u64 i = 0; i < N; i += 4 ) {
      if ( src[i] != dest[i] )
        return &src[i] - &dest[i];
      if ( src[i + 1] != dest[i + 1] )
        return &src[i + 1] - &dest[i + 1];
      if ( src[i + 2] != dest[i + 2] )
        return &src[i + 2] - &dest[i + 2];
      if ( src[i + 3] != dest[i + 3] )
        return &src[i + 3] - &dest[i + 3];
    }
  else
    for ( u64 i = 0; i < N; i++ )
      if ( src[i] != dest[i] )
        return &src[i] - &dest[i];
  return 0;
};

template <u64 N>
__attribute__((nonnull)) i64
cbcmp(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  return cbytecmp<N>(src, dest);
};

template <u64 N>
i64
rcbytecmp(const byte &src, const byte &dest) noexcept
{
  const byte *s = &src;
  const byte *d = &dest;
  if constexpr ( N % 4 == 0 )
    for ( u64 i = 0; i < N; i += 4 ) {
      if ( s[i] != d[i] )
        return &s[i] - &d[i];
      if ( s[i + 1] != d[i + 1] )
        return &s[i + 1] - &d[i + 1];
      if ( s[i + 2] != d[i + 2] )
        return &s[i + 2] - &d[i + 2];
      if ( s[i + 3] != d[i + 3] )
        return &s[i + 3] - &d[i + 3];
    }
  else
    for ( u64 i = 0; i < N; i++ )
      if ( s[i] != d[i] )
        return &s[i] - &d[i];
  return 0;
};

template <u64 N>
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
  if ( a == b )
    return 0;
  for ( int i = 0; i < 8; i++ )
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
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
  if ( a[0] == b[0] && a[1] == b[1] )
    return 0;
  for ( int i = 0; i < 16; i++ )
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
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
  if ( a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] )
    return 0;
  for ( int i = 0; i < 32; i++ )
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
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
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
  return 0;
};

inline i64
bcmp_64b(const byte *src, const byte *dest) noexcept
{
  return bytecmp_64b(src, dest);
};

template <u64 alignment = 1>
__attribute__((nonnull)) i64
sbytecmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  if ( src == nullptr || dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) || !__is_aligned_to(dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) || !__is_valid_address(dest, cnt) )
    return numeric_limits<i64>::min();
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return &src[i] - &dest[i];
  return 0;
};

template <u64 alignment = 1>
__attribute__((nonnull)) i64
sbcmp(const byte *__restrict src, const byte *__restrict dest, const u64 cnt) noexcept
{
  return sbytecmp<alignment>(src, dest, cnt);
};

template <u64 alignment = 1>
i64
rsbytecmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(src, alignment) || !__is_aligned_to_r(dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) || !__is_valid_address(dest, cnt) )
    return numeric_limits<i64>::min();
  const byte *s = &src;
  const byte *d = &dest;
  for ( u64 i = 0; i < cnt; i++ )
    if ( s[i] != d[i] )
      return &s[i] - &d[i];
  return 0;
};

template <u64 alignment = 1>
i64
rsbcmp(const byte &src, const byte &dest, const u64 cnt) noexcept
{
  return rsbytecmp<alignment>(src, dest, cnt);
};

template <u64 N, u64 alignment = 1>
__attribute__((nonnull)) i64
scbytecmp_safe(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  if ( src == nullptr || dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) || !__is_aligned_to(dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, N) || !__is_valid_address(dest, N) )
    return numeric_limits<i64>::min();
  if constexpr ( N % 4 == 0 )
    for ( u64 i = 0; i < N; i += 4 ) {
      if ( src[i] != dest[i] )
        return &src[i] - &dest[i];
      if ( src[i + 1] != dest[i + 1] )
        return &src[i + 1] - &dest[i + 1];
      if ( src[i + 2] != dest[i + 2] )
        return &src[i + 2] - &dest[i + 2];
      if ( src[i + 3] != dest[i + 3] )
        return &src[i + 3] - &dest[i + 3];
    }
  else
    for ( u64 i = 0; i < N; i++ )
      if ( src[i] != dest[i] )
        return &src[i] - &dest[i];
  return 0;
};

template <u64 N, u64 alignment = 1>
__attribute__((nonnull)) i64
scbcmp_safe(const byte *__restrict src, const byte *__restrict dest) noexcept
{
  return scbytecmp_safe<N, alignment>(src, dest);
};

template <u64 N, u64 alignment = 1>
i64
rscbytecmp_safe(const byte &src, const byte &dest) noexcept
{
  if ( !__is_aligned_to_r(src, alignment) || !__is_aligned_to_r(dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, N) || !__is_valid_address(dest, N) )
    return numeric_limits<i64>::min();
  const byte *s = &src;
  const byte *d = &dest;
  if constexpr ( N % 4 == 0 )
    for ( u64 i = 0; i < N; i += 4 ) {
      if ( s[i] != d[i] )
        return &s[i] - &d[i];
      if ( s[i + 1] != d[i + 1] )
        return &s[i + 1] - &d[i + 1];
      if ( s[i + 2] != d[i + 2] )
        return &s[i + 2] - &d[i + 2];
      if ( s[i + 3] != d[i + 3] )
        return &s[i + 3] - &d[i + 3];
    }
  else
    for ( u64 i = 0; i < N; i++ )
      if ( s[i] != d[i] )
        return &s[i] - &d[i];
  return 0;
};

template <u64 N, u64 alignment = 1>
i64
rscbcmp_safe(const byte &src, const byte &dest) noexcept
{
  return rscbytecmp_safe<N, alignment>(src, dest);
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// typecmps

template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
typecmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <typename T, typename F>
  requires(!micron::is_null_pointer_v<F>)
i64
rtypecmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F>
__attribute__((nonnull)) i64
ctypecmp(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F>
i64
rctypecmp(const F &_src, const F &_dest) noexcept
{
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
__attribute__((nonnull)) i64
stypecmp(const F *__restrict _src, const F *__restrict _dest, const u64 cnt) noexcept
{
  if ( _src == nullptr || _dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) || !__is_valid_address(_dest, cnt) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <typename T, typename F, u64 alignment = alignof(T)>
  requires(!micron::is_null_pointer_v<F>)
i64
rstypecmp(const F &_src, const F &_dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, cnt) || !__is_valid_address(_dest, cnt) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
__attribute__((nonnull)) i64
sctypecmp_safe(const F *__restrict _src, const F *__restrict _dest) noexcept
{
  if ( _src == nullptr || _dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) || !__is_valid_address(_dest, M) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(_src);
  const T *dest = reinterpret_cast<const T *>(_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M, typename T, typename F, u64 alignment = alignof(T)>
i64
rsctypecmp_safe(const F &_src, const F &_dest) noexcept
{
  if ( !__is_aligned_to(_src, alignment) || !__is_aligned_to(_dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(_src, M) || !__is_valid_address(_dest, M) )
    return numeric_limits<i64>::min();
  const T *src = reinterpret_cast<const T *>(&_src);
  const T *dest = reinterpret_cast<const T *>(&_dest);
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wordcmps

__attribute__((nonnull)) i64
wordcmp(const word *__restrict src, const word *__restrict dest, const u64 cnt) noexcept
{
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

i64
rwordcmp(const word &src, const word &dest, const u64 cnt) noexcept
{
  const word *s = &src;
  const word *d = &dest;
  for ( u64 i = 0; i < cnt; i++ )
    if ( s[i] != d[i] )
      return reinterpret_cast<const byte *>(&s[i]) - reinterpret_cast<const byte *>(&d[i]);
  return 0;
};

template <u64 M>
__attribute__((nonnull)) i64
cwordcmp(const word *__restrict src, const word *__restrict dest) noexcept
{
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 M>
i64
rcwordcmp(const word &src, const word &dest) noexcept
{
  const word *s = &src;
  const word *d = &dest;
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( s[i] != d[i] )
        return reinterpret_cast<const byte *>(&s[i]) - reinterpret_cast<const byte *>(&d[i]);
      if ( s[i + 1] != d[i + 1] )
        return reinterpret_cast<const byte *>(&s[i + 1]) - reinterpret_cast<const byte *>(&d[i + 1]);
      if ( s[i + 2] != d[i + 2] )
        return reinterpret_cast<const byte *>(&s[i + 2]) - reinterpret_cast<const byte *>(&d[i + 2]);
      if ( s[i + 3] != d[i + 3] )
        return reinterpret_cast<const byte *>(&s[i + 3]) - reinterpret_cast<const byte *>(&d[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( s[i] != d[i] )
        return reinterpret_cast<const byte *>(&s[i]) - reinterpret_cast<const byte *>(&d[i]);
  return 0;
};

inline i64
wordcmp_4w(const word *src, const word *dest) noexcept
{
  if ( src[0] != dest[0] )
    return reinterpret_cast<const byte *>(&src[0]) - reinterpret_cast<const byte *>(&dest[0]);
  if ( src[1] != dest[1] )
    return reinterpret_cast<const byte *>(&src[1]) - reinterpret_cast<const byte *>(&dest[1]);
  if ( src[2] != dest[2] )
    return reinterpret_cast<const byte *>(&src[2]) - reinterpret_cast<const byte *>(&dest[2]);
  if ( src[3] != dest[3] )
    return reinterpret_cast<const byte *>(&src[3]) - reinterpret_cast<const byte *>(&dest[3]);
  return 0;
};

inline i64
wordcmp_8w(const word *src, const word *dest) noexcept
{
  for ( int i = 0; i < 8; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

inline i64
wordcmp_16w(const word *src, const word *dest) noexcept
{
  for ( int i = 0; i < 16; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

inline i64
wordcmp_32w(const word *src, const word *dest) noexcept
{
  for ( int i = 0; i < 32; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 alignment = alignof(word)>
__attribute__((nonnull)) i64
swordcmp(const word *__restrict src, const word *__restrict dest, const u64 cnt) noexcept
{
  if ( src == nullptr || dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) || !__is_aligned_to(dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) || !__is_valid_address(dest, cnt) )
    return numeric_limits<i64>::min();
  for ( u64 i = 0; i < cnt; i++ )
    if ( src[i] != dest[i] )
      return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

template <u64 alignment = alignof(word)>
i64
rswordcmp(const word &src, const word &dest, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(src, alignment) || !__is_aligned_to_r(dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, cnt) || !__is_valid_address(dest, cnt) )
    return numeric_limits<i64>::min();
  const word *s = &src;
  const word *d = &dest;
  for ( u64 i = 0; i < cnt; i++ )
    if ( s[i] != d[i] )
      return reinterpret_cast<const byte *>(&s[i]) - reinterpret_cast<const byte *>(&d[i]);
  return 0;
};

// SAFE COMPILE-TIME CONSTANT WORDCMP - TEMPLATE COUNT ONLY
template <u64 M, u64 alignment = alignof(word)>
__attribute__((nonnull)) i64
scwordcmp_safe(const word *__restrict src, const word *__restrict dest) noexcept
{
  if ( src == nullptr || dest == nullptr )
    return numeric_limits<i64>::min();
  if ( !__is_aligned_to(src, alignment) || !__is_aligned_to(dest, alignment) )
    return numeric_limits<i64>::min();
  if ( !__is_valid_address(src, M) || !__is_valid_address(dest, M) )
    return numeric_limits<i64>::min();
  if constexpr ( M % 4 == 0 )
    for ( u64 i = 0; i < M; i += 4 ) {
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
      if ( src[i + 1] != dest[i + 1] )
        return reinterpret_cast<const byte *>(&src[i + 1]) - reinterpret_cast<const byte *>(&dest[i + 1]);
      if ( src[i + 2] != dest[i + 2] )
        return reinterpret_cast<const byte *>(&src[i + 2]) - reinterpret_cast<const byte *>(&dest[i + 2]);
      if ( src[i + 3] != dest[i + 3] )
        return reinterpret_cast<const byte *>(&src[i + 3]) - reinterpret_cast<const byte *>(&dest[i + 3]);
    }
  else
    for ( u64 i = 0; i < M; i++ )
      if ( src[i] != dest[i] )
        return reinterpret_cast<const byte *>(&src[i]) - reinterpret_cast<const byte *>(&dest[i]);
  return 0;
};

};     // namespace micron
