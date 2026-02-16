//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

#include "bits.hpp"

namespace micron
{

template <typename T, typename F, typename S = u64>
T *
_memcpy_32(T *__restrict d, const F *__restrict s, const S n) noexcept
{
  if ( n == 0 )
    return d;

  byte *dest = reinterpret_cast<byte *>(d);
  const byte *src = reinterpret_cast<const byte *>(s);

  switch ( n ) {
  case 1 :
    dest[0] = src[0];
    break;
  case 2 :
    dest[0] = src[0];
    dest[1] = src[1];
    break;
  case 3 :
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    break;
  case 4 :
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
    break;
  case 5 :
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
    dest[4] = src[4];
    break;
  case 6 :
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
    dest[4] = src[4];
    dest[5] = src[5];
    break;
  case 7 :
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
    dest[4] = src[4];
    dest[5] = src[5];
    dest[6] = src[6];
    break;
  case 8 :
    for ( u64 i = 0; i < 8; i++ )
      dest[i] = src[i];
    break;
  case 9 :
    for ( u64 i = 0; i < 9; i++ )
      dest[i] = src[i];
    break;
  case 10 :
    for ( u64 i = 0; i < 10; i++ )
      dest[i] = src[i];
    break;
  case 11 :
    for ( u64 i = 0; i < 11; i++ )
      dest[i] = src[i];
    break;
  case 12 :
    for ( u64 i = 0; i < 12; i++ )
      dest[i] = src[i];
    break;
  case 13 :
    for ( u64 i = 0; i < 13; i++ )
      dest[i] = src[i];
    break;
  case 14 :
    for ( u64 i = 0; i < 14; i++ )
      dest[i] = src[i];
    break;
  case 15 :
    for ( u64 i = 0; i < 15; i++ )
      dest[i] = src[i];
    break;
  case 16 :
    for ( u64 i = 0; i < 16; i++ )
      dest[i] = src[i];
    break;
  case 17 :
    for ( u64 i = 0; i < 17; i++ )
      dest[i] = src[i];
    break;
  case 18 :
    for ( u64 i = 0; i < 18; i++ )
      dest[i] = src[i];
    break;
  case 19 :
    for ( u64 i = 0; i < 19; i++ )
      dest[i] = src[i];
    break;
  case 20 :
    for ( u64 i = 0; i < 20; i++ )
      dest[i] = src[i];
    break;
  case 21 :
    for ( u64 i = 0; i < 21; i++ )
      dest[i] = src[i];
    break;
  case 22 :
    for ( u64 i = 0; i < 22; i++ )
      dest[i] = src[i];
    break;
  case 23 :
    for ( u64 i = 0; i < 23; i++ )
      dest[i] = src[i];
    break;
  case 24 :
    for ( u64 i = 0; i < 24; i++ )
      dest[i] = src[i];
    break;
  case 25 :
    for ( u64 i = 0; i < 25; i++ )
      dest[i] = src[i];
    break;
  case 26 :
    for ( u64 i = 0; i < 26; i++ )
      dest[i] = src[i];
    break;
  case 27 :
    for ( u64 i = 0; i < 27; i++ )
      dest[i] = src[i];
    break;
  case 28 :
    for ( u64 i = 0; i < 28; i++ )
      dest[i] = src[i];
    break;
  case 29 :
    for ( u64 i = 0; i < 29; i++ )
      dest[i] = src[i];
    break;
  case 30 :
    for ( u64 i = 0; i < 30; i++ )
      dest[i] = src[i];
    break;
  case 31 :
    for ( u64 i = 0; i < 31; i++ )
      dest[i] = src[i];
    break;
  case 32 :
    for ( u64 i = 0; i < 32; i++ )
      dest[i] = src[i];
    break;
  }
  return d;
}

template <typename T, typename F, typename S = u64>
T *
_rmemcpy_32(T &restrict _d, const F &restrict _s, const S n) noexcept
{
  T *d = &_d;
  const F *s = &_s;
  return _memcpy_32(d, s, n);
}

template <typename F, typename D>
F *
memcpy(F *restrict dest, const D *restrict src, const u64 cnt) noexcept
{
  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = static_cast<F>(src[n]);
  return dest;
};

template <typename F, typename D>
F &
rmemcpy(F &restrict dest, const D &restrict src, const u64 cnt) noexcept
{
  F *d = &dest;
  const D *s = &src;
  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      d[n] = static_cast<F>(s[n]);
      d[n + 1] = static_cast<F>(s[n + 1]);
      d[n + 2] = static_cast<F>(s[n + 2]);
      d[n + 3] = static_cast<F>(s[n + 3]);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      d[n] = static_cast<F>(s[n]);
  return dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F *
constexpr_memcpy(F *restrict dest, const D *restrict src, const u64 cnt) noexcept
{
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = static_cast<F>(src[n]);
  return dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F &
constexpr_rmemcpy(F &restrict dest, const D &restrict src, const u64 cnt) noexcept
{
  F *d = &dest;
  const D *s = &src;
  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      d[n] = static_cast<F>(s[n]);
      d[n + 1] = static_cast<F>(s[n + 1]);
      d[n + 2] = static_cast<F>(s[n + 2]);
      d[n + 3] = static_cast<F>(s[n + 3]);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      d[n] = static_cast<F>(s[n]);
  return dest;
};

template <u64 M, typename F, typename D>
F *
cmemcpy(F *restrict dest, const D *restrict src) noexcept
{
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      dest[n] = static_cast<F>(src[n]);
  return dest;
};

template <u64 M, typename F, typename D>
F &
crmemcpy(F &restrict dest, const D &restrict src) noexcept
{
  F *d = &dest;
  const D *s = &src;
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      d[n] = static_cast<F>(s[n]);
      d[n + 1] = static_cast<F>(s[n + 1]);
      d[n + 2] = static_cast<F>(s[n + 2]);
      d[n + 3] = static_cast<F>(s[n + 3]);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      d[n] = static_cast<F>(s[n]);
  return dest;
};

template <typename F, typename D, u64 alignment = alignof(F)>
F *
smemcpy(F *restrict dest, const D *restrict src, const u64 cnt) noexcept
{
  if ( dest == nullptr || src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return nullptr;

  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = static_cast<F>(src[n]);

  __mem_barrier();
  return dest;
};

template <typename F, typename D, u64 alignment = alignof(F)>
bool
rsmemcpy(F &restrict dest, const D &restrict src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return false;

  F *d = &dest;
  const D *s = &src;
  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      d[n] = static_cast<F>(s[n]);
      d[n + 1] = static_cast<F>(s[n + 1]);
      d[n + 2] = static_cast<F>(s[n + 2]);
      d[n + 3] = static_cast<F>(s[n + 3]);
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      d[n] = static_cast<F>(s[n]);

  __mem_barrier();
  return true;
};

template <u64 M, typename F, typename D, u64 alignment = alignof(F)>
F *
scmemcpy(F *restrict dest, const D *restrict src) noexcept
{
  if ( dest == nullptr || src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return nullptr;

  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      dest[n] = static_cast<F>(src[n]);

  __mem_barrier();
  return dest;
};

template <u64 M, typename F, typename D, u64 alignment = alignof(F)>
bool
rscmemcpy(F &restrict dest, const D &restrict src) noexcept
{
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return false;

  F *d = &dest;
  const D *s = &src;
  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      d[n] = static_cast<F>(s[n]);
      d[n + 1] = static_cast<F>(s[n + 1]);
      d[n + 2] = static_cast<F>(s[n + 2]);
      d[n + 3] = static_cast<F>(s[n + 3]);
    }
  else
    for ( u64 n = 0; n < M; n++ )
      d[n] = static_cast<F>(s[n]);

  __mem_barrier();
  return true;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
bytecpy(F *restrict _dest, const D *restrict _src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);

  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = src[n];

  return _dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
rbytecpy(F &restrict _dest, const D &restrict _src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  const byte *src = reinterpret_cast<const byte *>(&_src);

  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = src[n];

  return _dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F *
constexpr_bytecpy(F *restrict _dest, const D *restrict _src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);

  if ( cnt % 4 == 0 )
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = src[n];

  return _dest;
};

template <u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
cbytecpy(F *restrict _dest, const D *restrict _src) noexcept
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);

  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < M; n++ )
      dest[n] = src[n];

  return _dest;
};

template <u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
crbytecpy(F &restrict _dest, const D &restrict _src) noexcept
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  const byte *src = reinterpret_cast<const byte *>(&_src);

  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < M; n++ )
      dest[n] = src[n];

  return _dest;
};

template <typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
sbytecpy(F *restrict _dest, const D *restrict _src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return nullptr;

  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);

  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = src[n];

  __mem_barrier();
  return _dest;
};

template <typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rsbytecpy(F &restrict _dest, const D &restrict _src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return false;

  byte *dest = reinterpret_cast<byte *>(&_dest);
  const byte *src = reinterpret_cast<const byte *>(&_src);

  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = src[n];

  __mem_barrier();
  return true;
};

template <u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
scbytecpy(F *restrict _dest, const D *restrict _src) noexcept
{
  if ( _dest == nullptr || _src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return nullptr;

  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);

  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < M; n++ )
      dest[n] = src[n];

  __mem_barrier();
  return _dest;
};

template <u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rscbytecpy(F &restrict _dest, const D &restrict _src) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return false;

  byte *dest = reinterpret_cast<byte *>(&_dest);
  const byte *src = reinterpret_cast<const byte *>(&_src);

  if constexpr ( M % 4 == 0 )
    for ( u64 n = 0; n < M; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < M; n++ )
      dest[n] = src[n];

  __mem_barrier();
  return true;
};

void *
voidcpy(void *restrict _dest, const void *restrict _src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);

  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = src[n];

  return _dest;
};

template <u64 alignment = 1>
void *
svoidcpy(void *restrict _dest, const void *restrict _src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return nullptr;

  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);

  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = src[n];
      dest[n + 1] = src[n + 1];
      dest[n + 2] = src[n + 2];
      dest[n + 3] = src[n + 3];
    }
  else
    for ( u64 n = 0; n < cnt; n++ )
      dest[n] = src[n];

  __mem_barrier();
  return _dest;
};

};
