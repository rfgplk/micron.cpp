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

// TODO: pull this out into plumbing, ugly like this

// cold tier selection for n > __mem_ladder_max
[[gnu::noinline]] static __attribute__((optimize("-fno-tree-loop-distribute-patterns"))) byte *
__memcpy_large(byte *restrict d, const byte *restrict s, const u64 n) noexcept
{
#if defined(__micron_arch_x86_any)
  if ( n < __mem_rep_min ) return simd::__memcpy_bulk(d, s, n);
  const __mem_tunables &t = __mem_tun_get();
  if ( n >= t.nt_copy_threshold ) return simd::__memcpy_bulk_nt(d, s, n);
  if ( n >= t.rep_movsb_threshold ) {
    simd::__bits::__rep_movsb(d, s, static_cast<usize>(n));
    return d;
  }
  return simd::__memcpy_bulk(d, s, n);
#elif defined(__micron_arch_arm64)
  if ( n >= __mem_nt_threshold_arm64 ) return simd::__memcpy_bulk_nt(d, s, n);
  return simd::__memcpy_bulk(d, s, n);
#else
  return simd::__memcpy_bulk(d, s, n);
#endif
}

[[gnu::always_inline]] static inline byte *
__memcpy_bytes(byte *restrict d, const byte *restrict s, const u64 bytes) noexcept
{
  if ( bytes == 0 ) return d;
  if ( bytes <= 32 ) {
    __ml::__copy_le32(d, s, bytes);
    return d;
  }
  if ( bytes <= 64 ) {
    __ml::__copy_33_64(d, s, bytes);
    return d;
  }
  if ( bytes <= 128 ) {
    __ml::__copy_65_128(d, s, bytes);
    return d;
  }
#if defined(__micron_x86_avx2)
  if ( bytes <= 256 ) {
    __ml::__copy_129_256(d, s, bytes);
    return d;
  }
#endif
  return __memcpy_large(d, s, bytes);
}

// NOTE: memcpy_32 (unlike other memcpies) works off of bytes and not elements
template<typename T, typename F, typename S = u64>
[[gnu::always_inline]] inline T *
__memcpy_32(T *__restrict d, const F *__restrict s, const S n) noexcept
{
  if ( n == 0 ) return d;
  byte *dest = reinterpret_cast<byte *>(d);
  const byte *src = reinterpret_cast<const byte *>(s);
  if ( n <= 32 ) {
    __ml::__copy_le32(dest, src, static_cast<u64>(n));
    return d;
  }
  __memcpy_bytes(dest, src, static_cast<u64>(n));
  return d;
}

template<typename T, typename F, typename S = u64>
T *
_rmemcpy_32(T &restrict _d, const F &restrict _s, const S n) noexcept
{
  T *d = &_d;
  const F *s = &_s;
  return __memcpy_32(d, s, n);
}

template<typename F, typename D>
constexpr F *
memcpy(F *restrict dest, const D *restrict src, const u64 cnt) noexcept
{
  if !consteval {
    if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
      __memcpy_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), cnt * sizeof(D));
      return dest;
    }
  }
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
};

template<typename F, typename D>
F &
rmemcpy(F &restrict dest, const D &restrict src, const u64 cnt) noexcept
{
  F *d = &dest;
  const D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memcpy_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), cnt * sizeof(D));
    return dest;
  } else {
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
};

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
};

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
};

template<u64 M, typename F, typename D>
F *
cmemcpy(F *restrict dest, const D *restrict src) noexcept
{
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memcpy_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), M * sizeof(D));
    return dest;
  } else {
    if constexpr ( M % 4 == 0 )
      for ( u64 n = 0; n < M; n += 4 ) {
        dest[n] = static_cast<F>(src[n]);
        dest[n + 1] = static_cast<F>(src[n + 1]);
        dest[n + 2] = static_cast<F>(src[n + 2]);
        dest[n + 3] = static_cast<F>(src[n + 3]);
      }
    else
      for ( u64 n = 0; n < M; n++ ) dest[n] = static_cast<F>(src[n]);
    return dest;
  }
};

template<u64 M, typename F, typename D>
F &
crmemcpy(F &restrict dest, const D &restrict src) noexcept
{
  F *d = &dest;
  const D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memcpy_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), M * sizeof(D));
    return dest;
  } else {
    if constexpr ( M % 4 == 0 )
      for ( u64 n = 0; n < M; n += 4 ) {
        d[n] = static_cast<F>(s[n]);
        d[n + 1] = static_cast<F>(s[n + 1]);
        d[n + 2] = static_cast<F>(s[n + 2]);
        d[n + 3] = static_cast<F>(s[n + 3]);
      }
    else
      for ( u64 n = 0; n < M; n++ ) d[n] = static_cast<F>(s[n]);
    return dest;
  }
};

template<typename F, typename D, u64 alignment = alignof(F)>
F *
smemcpy(F *restrict dest, const D *restrict src, const u64 cnt) noexcept
{
  if ( dest == nullptr || src == nullptr ) return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return nullptr;

  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memcpy_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), cnt * sizeof(D));
  } else {
    if ( cnt % 4 == 0 ) [[likely]]
      for ( u64 n = 0; n < cnt; n += 4 ) {
        dest[n] = static_cast<F>(src[n]);
        dest[n + 1] = static_cast<F>(src[n + 1]);
        dest[n + 2] = static_cast<F>(src[n + 2]);
        dest[n + 3] = static_cast<F>(src[n + 3]);
      }
    else
      for ( u64 n = 0; n < cnt; n++ ) dest[n] = static_cast<F>(src[n]);
  }

  __mem_barrier();
  return dest;
};

template<typename F, typename D, u64 alignment = alignof(F)>
bool
rsmemcpy(F &restrict dest, const D &restrict src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(dest, alignment) || !__is_aligned_to_r(src, alignment) ) return false;

  F *d = &dest;
  const D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memcpy_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), cnt * sizeof(D));
  } else {
    if ( cnt % 4 == 0 ) [[likely]]
      for ( u64 n = 0; n < cnt; n += 4 ) {
        d[n] = static_cast<F>(s[n]);
        d[n + 1] = static_cast<F>(s[n + 1]);
        d[n + 2] = static_cast<F>(s[n + 2]);
        d[n + 3] = static_cast<F>(s[n + 3]);
      }
    else
      for ( u64 n = 0; n < cnt; n++ ) d[n] = static_cast<F>(s[n]);
  }

  __mem_barrier();
  return true;
};

template<u64 M, typename F, typename D, u64 alignment = alignof(F)>
F *
scmemcpy(F *restrict dest, const D *restrict src) noexcept
{
  if ( dest == nullptr || src == nullptr ) return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return nullptr;

  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memcpy_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), M * sizeof(D));
  } else {
    if constexpr ( M % 4 == 0 )
      for ( u64 n = 0; n < M; n += 4 ) {
        dest[n] = static_cast<F>(src[n]);
        dest[n + 1] = static_cast<F>(src[n + 1]);
        dest[n + 2] = static_cast<F>(src[n + 2]);
        dest[n + 3] = static_cast<F>(src[n + 3]);
      }
    else
      for ( u64 n = 0; n < M; n++ ) dest[n] = static_cast<F>(src[n]);
  }

  __mem_barrier();
  return dest;
};

template<u64 M, typename F, typename D, u64 alignment = alignof(F)>
bool
rscmemcpy(F &restrict dest, const D &restrict src) noexcept
{
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return false;

  F *d = &dest;
  const D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memcpy_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), M * sizeof(D));
  } else {
    if constexpr ( M % 4 == 0 )
      for ( u64 n = 0; n < M; n += 4 ) {
        d[n] = static_cast<F>(s[n]);
        d[n + 1] = static_cast<F>(s[n + 1]);
        d[n + 2] = static_cast<F>(s[n + 2]);
        d[n + 3] = static_cast<F>(s[n + 3]);
      }
    else
      for ( u64 n = 0; n < M; n++ ) d[n] = static_cast<F>(s[n]);
  }

  __mem_barrier();
  return true;
};

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
bytecpy(F *restrict _dest, const D *restrict _src, const u64 cnt) noexcept
{
  __memcpy_bytes(reinterpret_cast<byte *>(_dest), reinterpret_cast<const byte *>(_src), cnt);
  return _dest;
};

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
rbytecpy(F &restrict _dest, const D &restrict _src, const u64 cnt) noexcept
{
  __memcpy_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), cnt);
  return _dest;
};

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
};

template<u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
cbytecpy(F *restrict _dest, const D *restrict _src) noexcept
{
  __memcpy_bytes(reinterpret_cast<byte *>(_dest), reinterpret_cast<const byte *>(_src), M);
  return _dest;
};

template<u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
crbytecpy(F &restrict _dest, const D &restrict _src) noexcept
{
  __memcpy_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), M);
  return _dest;
};

template<typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
sbytecpy(F *restrict _dest, const D *restrict _src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
  __memcpy_bytes(reinterpret_cast<byte *>(_dest), reinterpret_cast<const byte *>(_src), cnt);
  __mem_barrier();
  return _dest;
};

template<typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rsbytecpy(F &restrict _dest, const D &restrict _src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return false;
  __memcpy_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), cnt);
  __mem_barrier();
  return true;
};

template<u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
scbytecpy(F *restrict _dest, const D *restrict _src) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
  __memcpy_bytes(reinterpret_cast<byte *>(_dest), reinterpret_cast<const byte *>(_src), M);
  __mem_barrier();
  return _dest;
};

template<u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rscbytecpy(F &restrict _dest, const D &restrict _src) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return false;
  __memcpy_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), M);
  __mem_barrier();
  return true;
};

void *
voidcpy(void *restrict _dest, const void *restrict _src, const u64 cnt) noexcept
{
  __memcpy_bytes(reinterpret_cast<byte *>(_dest), reinterpret_cast<const byte *>(_src), cnt);
  return _dest;
};

template<u64 alignment = 1>
void *
svoidcpy(void *restrict _dest, const void *restrict _src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
  __memcpy_bytes(reinterpret_cast<byte *>(_dest), reinterpret_cast<const byte *>(_src), cnt);
  __mem_barrier();
  return _dest;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mempcpy

template<typename F, typename D>
F *
mempcpy(F *restrict dest, const D *restrict src, const u64 cnt) noexcept
{
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    const u64 bytes = cnt * sizeof(D);
    __memcpy_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), bytes);
    return reinterpret_cast<F *>(reinterpret_cast<byte *>(dest) + bytes);
  } else {
    for ( u64 n = 0; n < cnt; n++ ) dest[n] = static_cast<F>(src[n]);
    return dest + cnt;
  }
}

};      // namespace micron

#if defined(__micron_freestanding)
// c-abi
extern "C" __attribute__((used, optimize("-fno-tree-loop-distribute-patterns"))) inline void *
memcpy(void *__restrict d, const void *__restrict s, __SIZE_TYPE__ n) noexcept
{
  micron::__memcpy_bytes(static_cast<byte *>(d), static_cast<const byte *>(s), static_cast<u64>(n));
  return d;
}
#endif
