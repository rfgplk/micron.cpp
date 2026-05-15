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

namespace experimental
{
namespace __unroll
{

template<typename F, typename D, usize... I>
inline __attribute__((always_inline)) void
copy(F *__restrict dest, const D *__restrict src, micron::index_sequence<I...>) noexcept
{
  ((dest[I] = static_cast<F>(src[I])), ...);
}

template<usize... I>
inline __attribute__((always_inline)) void
bcopy(byte *__restrict dest, const byte *__restrict src, micron::index_sequence<I...>) noexcept
{
  ((dest[I] = src[I]), ...);
}

inline constexpr u64 unroll_limit = 1024;
};      // namespace __unroll

template<u64 N, typename T, typename F>
inline __attribute__((always_inline)) T *
__memcpy_ur(T *__restrict dest, const F *__restrict src) noexcept
{
  byte *d = reinterpret_cast<byte *>(dest);
  const byte *s = reinterpret_cast<const byte *>(src);
  __unroll::bcopy(d, s, micron::make_index_sequence<N>{});
  return dest;
}

template<typename T, typename F, typename S = u64>
T *
__memcpy_32(T *__restrict d, const F *__restrict s, const S n) noexcept
{
  switch ( n ) {
  case 0:
    return d;
  case 1:
    return __memcpy_ur<1>(d, s);
  case 2:
    return __memcpy_ur<2>(d, s);
  case 3:
    return __memcpy_ur<3>(d, s);
  case 4:
    return __memcpy_ur<4>(d, s);
  case 5:
    return __memcpy_ur<5>(d, s);
  case 6:
    return __memcpy_ur<6>(d, s);
  case 7:
    return __memcpy_ur<7>(d, s);
  case 8:
    return __memcpy_ur<8>(d, s);
  case 9:
    return __memcpy_ur<9>(d, s);
  case 10:
    return __memcpy_ur<10>(d, s);
  case 11:
    return __memcpy_ur<11>(d, s);
  case 12:
    return __memcpy_ur<12>(d, s);
  case 13:
    return __memcpy_ur<13>(d, s);
  case 14:
    return __memcpy_ur<14>(d, s);
  case 15:
    return __memcpy_ur<15>(d, s);
  case 16:
    return __memcpy_ur<16>(d, s);
  case 17:
    return __memcpy_ur<17>(d, s);
  case 18:
    return __memcpy_ur<18>(d, s);
  case 19:
    return __memcpy_ur<19>(d, s);
  case 20:
    return __memcpy_ur<20>(d, s);
  case 21:
    return __memcpy_ur<21>(d, s);
  case 22:
    return __memcpy_ur<22>(d, s);
  case 23:
    return __memcpy_ur<23>(d, s);
  case 24:
    return __memcpy_ur<24>(d, s);
  case 25:
    return __memcpy_ur<25>(d, s);
  case 26:
    return __memcpy_ur<26>(d, s);
  case 27:
    return __memcpy_ur<27>(d, s);
  case 28:
    return __memcpy_ur<28>(d, s);
  case 29:
    return __memcpy_ur<29>(d, s);
  case 30:
    return __memcpy_ur<30>(d, s);
  case 31:
    return __memcpy_ur<31>(d, s);
  case 32:
    return __memcpy_ur<32>(d, s);
  default:
    return d;
  }
}

template<typename T, typename F, typename S = u64>
T *
_rmemcpy_32(T &restrict _d, const F &restrict _s, const S n) noexcept
{
  return __memcpy_32(&_d, &_s, n);
}

template<typename F, typename D>
inline F *
memcpy_8b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<8>(dest, src);
}

template<typename F, typename D>
inline F *
memcpy_16b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<16>(dest, src);
}

template<typename F, typename D>
inline F *
memcpy_32b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<32>(dest, src);
}

template<typename F, typename D>
inline F *
memcpy_64b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<64>(dest, src);
}

template<typename F, typename D>
inline F *
memcpy_128b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<128>(dest, src);
}

template<typename F, typename D>
inline F *
memcpy_256b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<256>(dest, src);
}

template<typename F, typename D>
inline F *
memcpy_512b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<512>(dest, src);
}

template<typename F, typename D>
inline F *
memcpy_1024b(F *__restrict dest, const D *__restrict src) noexcept
{
  return __memcpy_ur<1024>(dest, src);
}

template<typename F, typename D>
constexpr F *
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = static_cast<F>(src[n]);
  return dest;
}

template<typename F, typename D>
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
    for ( u64 n = 0; n < cnt; n++ ) d[n] = static_cast<F>(s[n]);
  return dest;
}

template<typename F, typename D>
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = static_cast<F>(src[n]);
  return dest;
}

template<typename F, typename D>
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
    for ( u64 n = 0; n < cnt; n++ ) d[n] = static_cast<F>(s[n]);
  return dest;
}

template<u64 M, typename F, typename D>
F *
cmemcpy(F *restrict dest, const D *restrict src) noexcept
{
  if constexpr ( M * sizeof(F) <= __unroll::unroll_limit ) {
    __unroll::copy(dest, src, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) dest[n] = static_cast<F>(src[n]);
  }
  return dest;
}

template<u64 M, typename F, typename D>
F &
crmemcpy(F &restrict dest, const D &restrict src) noexcept
{
  F *d = &dest;
  const D *s = &src;
  if constexpr ( M * sizeof(F) <= __unroll::unroll_limit ) {
    __unroll::copy(d, s, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) d[n] = static_cast<F>(s[n]);
  }
  return dest;
}

template<typename F, typename D, u64 alignment = alignof(F)>
F *
smemcpy(F *restrict dest, const D *restrict src, const u64 cnt) noexcept
{
  if ( dest == nullptr || src == nullptr ) return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return nullptr;
  if ( cnt % 4 == 0 ) [[likely]]
    for ( u64 n = 0; n < cnt; n += 4 ) {
      dest[n] = static_cast<F>(src[n]);
      dest[n + 1] = static_cast<F>(src[n + 1]);
      dest[n + 2] = static_cast<F>(src[n + 2]);
      dest[n + 3] = static_cast<F>(src[n + 3]);
    }
  else
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = static_cast<F>(src[n]);
  __mem_barrier();
  return dest;
}

template<typename F, typename D, u64 alignment = alignof(F)>
bool
rsmemcpy(F &restrict dest, const D &restrict src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(dest, alignment) || !__is_aligned_to_r(src, alignment) ) return false;
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
    for ( u64 n = 0; n < cnt; n++ ) d[n] = static_cast<F>(s[n]);
  __mem_barrier();
  return true;
}

template<u64 M, typename F, typename D, u64 alignment = alignof(F)>
F *
scmemcpy(F *restrict dest, const D *restrict src) noexcept
{
  if ( dest == nullptr || src == nullptr ) return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return nullptr;
  if constexpr ( M * sizeof(F) <= __unroll::unroll_limit ) {
    __unroll::copy(dest, src, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) dest[n] = static_cast<F>(src[n]);
  }
  __mem_barrier();
  return dest;
}

template<u64 M, typename F, typename D, u64 alignment = alignof(F)>
bool
rscmemcpy(F &restrict dest, const D &restrict src) noexcept
{
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return false;
  F *d = &dest;
  const D *s = &src;
  if constexpr ( M * sizeof(F) <= __unroll::unroll_limit ) {
    __unroll::copy(d, s, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) d[n] = static_cast<F>(s[n]);
  }
  __mem_barrier();
  return true;
}

template<typename F, typename D>
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = src[n];
  return _dest;
}

template<typename F, typename D>
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = src[n];
  return _dest;
}

template<typename F, typename D>
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = src[n];
  return _dest;
}

template<u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
cbytecpy(F *restrict _dest, const D *restrict _src) noexcept
{
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);
  if constexpr ( M <= __unroll::unroll_limit ) {
    __unroll::bcopy(dest, src, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) dest[n] = src[n];
  }
  return _dest;
}

template<u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
crbytecpy(F &restrict _dest, const D &restrict _src) noexcept
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  const byte *src = reinterpret_cast<const byte *>(&_src);
  if constexpr ( M <= __unroll::unroll_limit ) {
    __unroll::bcopy(dest, src, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) dest[n] = src[n];
  }
  return _dest;
}

template<typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
sbytecpy(F *restrict _dest, const D *restrict _src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = src[n];
  __mem_barrier();
  return _dest;
}

template<typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rsbytecpy(F &restrict _dest, const D &restrict _src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return false;
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = src[n];
  __mem_barrier();
  return true;
}

template<u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
scbytecpy(F *restrict _dest, const D *restrict _src) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
  byte *dest = reinterpret_cast<byte *>(_dest);
  const byte *src = reinterpret_cast<const byte *>(_src);
  if constexpr ( M <= __unroll::unroll_limit ) {
    __unroll::bcopy(dest, src, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) dest[n] = src[n];
  }
  __mem_barrier();
  return _dest;
}

template<u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rscbytecpy(F &restrict _dest, const D &restrict _src) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return false;
  byte *dest = reinterpret_cast<byte *>(&_dest);
  const byte *src = reinterpret_cast<const byte *>(&_src);
  if constexpr ( M <= __unroll::unroll_limit ) {
    __unroll::bcopy(dest, src, micron::make_index_sequence<M>{});
  } else {
    for ( u64 n = 0; n < M; n++ ) dest[n] = src[n];
  }
  __mem_barrier();
  return true;
}

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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = src[n];
  return _dest;
}

template<u64 alignment = 1>
void *
svoidcpy(void *restrict _dest, const void *restrict _src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
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
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = src[n];
  __mem_barrier();
  return _dest;
}
};      // namespace experimental
};      // namespace micron
