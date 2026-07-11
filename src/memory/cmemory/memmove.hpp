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
// TODO: also pull these out into plumbing

// cold tier selection
[[gnu::noinline]] static __attribute__((optimize("-fno-tree-loop-distribute-patterns"))) byte *
__memmove_large(byte *d, const byte *s, const u64 n) noexcept
{
  if ( d < s || d >= s + n ) {
#if defined(__micron_arch_x86_any)
    if ( n < __mem_rep_min ) return simd::__memmove_bulk_fwd(d, s, n);
    const __mem_tunables &t = __mem_tun_get();
    const bool disjoint = (d + n <= s) || (s + n <= d);
    if ( disjoint && n >= t.nt_copy_threshold ) return simd::__memcpy_bulk_nt(d, s, n);
    if ( n >= t.rep_movsb_threshold && n < t.nt_copy_threshold ) {
      simd::__bits::__rep_movsb(d, s, static_cast<usize>(n));
      return d;
    }
#elif defined(__micron_arch_arm64)
    if ( ((d + n <= s) || (s + n <= d)) && n >= __mem_nt_threshold_arm64 ) return simd::__memcpy_bulk_nt(d, s, n);
#endif
    return simd::__memmove_bulk_fwd(d, s, n);
  }
  return simd::__memmove_bulk_bwd(d, s, n);
}

[[gnu::always_inline]] static inline byte *
__memmove_bytes(byte *d, const byte *s, const u64 bytes) noexcept
{
  if ( d == s || bytes == 0 ) return d;
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
  return __memmove_large(d, s, bytes);
}

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
bytemove(F *_dest, D *_src, const u64 cnt) noexcept
{
  __memmove_bytes(reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest)),
                  reinterpret_cast<const byte *>(const_cast<micron::remove_cv_t<D> *>(_src)), cnt);
  return _dest;
};

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
rbytemove(F &_dest, D &_src, const u64 cnt) noexcept
{
  __memmove_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), cnt);
  return _dest;
};

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F *
constexpr_bytemove(F *_dest, D *_src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest));
  byte *src = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<D> *>(_src));

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ ) dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i ) dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F &
constexpr_rbytemove(F &_dest, D &_src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  byte *src = reinterpret_cast<byte *>(&_src);

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ ) dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i ) dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template<u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
cbytemove(F *_dest, D *_src) noexcept
{
  __memmove_bytes(reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest)),
                  reinterpret_cast<const byte *>(const_cast<micron::remove_cv_t<D> *>(_src)), M);
  return _dest;
};

template<u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
crbytemove(F &_dest, D &_src) noexcept
{
  __memmove_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), M);
  return _dest;
};

template<typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
sbytemove(F *_dest, D *_src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
  __memmove_bytes(reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest)),
                  reinterpret_cast<const byte *>(const_cast<micron::remove_cv_t<D> *>(_src)), cnt);
  __mem_barrier();
  return _dest;
};

template<typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rsbytemove(F &_dest, D &_src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(_dest, alignment) || !__is_aligned_to_r(_src, alignment) ) return false;
  __memmove_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), cnt);
  __mem_barrier();
  return true;
};

template<u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
scbytemove(F *_dest, D *_src) noexcept
{
  if ( _dest == nullptr || _src == nullptr ) return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) ) return nullptr;
  __memmove_bytes(reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest)),
                  reinterpret_cast<const byte *>(const_cast<micron::remove_cv_t<D> *>(_src)), M);
  __mem_barrier();
  return _dest;
};

template<u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rscrbytemove(F &_dest, D &_src) noexcept
{
  if ( !__is_aligned_to_r(_dest, alignment) || !__is_aligned_to_r(_src, alignment) ) return false;
  __memmove_bytes(reinterpret_cast<byte *>(&_dest), reinterpret_cast<const byte *>(&_src), M);
  __mem_barrier();
  return true;
};

template<typename F, typename D>
F *
memmove(F *dest, D *src, const u64 cnt) noexcept
{
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), cnt * sizeof(D));
    return dest;
  } else {
    if ( dest < src ) {
      for ( u64 i = 0; i < cnt; i++ ) dest[i] = static_cast<F>(src[i]);
    } else if ( dest > src ) {
      for ( u64 i = cnt; i > 0; --i ) dest[i - 1] = static_cast<F>(src[i - 1]);
    }
    return dest;
  }
};

template<typename F, typename D>
F &
rmemmove(F &dest, D &src, const u64 cnt) noexcept
{
  F *d = &dest;
  D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), cnt * sizeof(D));
    return dest;
  } else {
    if ( d < s ) {
      for ( u64 i = 0; i < cnt; i++ ) d[i] = static_cast<F>(s[i]);
    } else if ( d > s ) {
      for ( u64 i = cnt; i > 0; --i ) d[i - 1] = static_cast<F>(s[i - 1]);
    }
    return dest;
  }
};

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F *
constexpr_memmove(F *dest, D *src, const u64 cnt) noexcept
{
  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ ) dest[i] = static_cast<F>(src[i]);
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i ) dest[i - 1] = static_cast<F>(src[i - 1]);
  }

  return dest;
};

template<typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F &
constexpr_rmemmove(F &dest, D &src, const u64 cnt) noexcept
{
  F *d = &dest;
  D *s = &src;

  if ( d < s ) {
    for ( u64 i = 0; i < cnt; i++ ) d[i] = static_cast<F>(s[i]);
  } else if ( d > s ) {
    for ( u64 i = cnt; i > 0; --i ) d[i - 1] = static_cast<F>(s[i - 1]);
  }

  return dest;
};

template<u64 M, typename F, typename D>
F *
cmemmove(F *dest, D *src) noexcept
{
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), M * sizeof(D));
    return dest;
  } else {
    if ( dest < src ) {
      for ( u64 i = 0; i < M; i++ ) dest[i] = static_cast<F>(src[i]);
    } else if ( dest > src ) {
      for ( u64 i = M; i > 0; --i ) dest[i - 1] = static_cast<F>(src[i - 1]);
    }
    return dest;
  }
};

template<u64 M, typename F, typename D>
F &
crmemmove(F &dest, D &src) noexcept
{
  F *d = &dest;
  D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), M * sizeof(D));
    return dest;
  } else {
    if ( d < s ) {
      for ( u64 i = 0; i < M; i++ ) d[i] = static_cast<F>(s[i]);
    } else if ( d > s ) {
      for ( u64 i = M; i > 0; --i ) d[i - 1] = static_cast<F>(s[i - 1]);
    }
    return dest;
  }
};

template<typename F, typename D, u64 alignment = alignof(F)>
F *
smemmove(F *dest, D *src, const u64 cnt) noexcept
{
  if ( dest == nullptr || src == nullptr ) return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return nullptr;

  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), cnt * sizeof(D));
  } else {
    if ( dest < src ) {
      for ( u64 i = 0; i < cnt; i++ ) dest[i] = static_cast<F>(src[i]);
    } else if ( dest > src ) {
      for ( u64 i = cnt; i > 0; --i ) dest[i - 1] = static_cast<F>(src[i - 1]);
    }
  }

  __mem_barrier();
  return dest;
};

template<typename F, typename D, u64 alignment = alignof(F)>
bool
rsmemmove(F &dest, D &src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to_r(dest, alignment) || !__is_aligned_to_r(src, alignment) ) return false;

  F *d = &dest;
  D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), cnt * sizeof(D));
  } else {
    if ( d < s ) {
      for ( u64 i = 0; i < cnt; i++ ) d[i] = static_cast<F>(s[i]);
    } else if ( d > s ) {
      for ( u64 i = cnt; i > 0; --i ) d[i - 1] = static_cast<F>(s[i - 1]);
    }
  }

  __mem_barrier();
  return true;
};

template<u64 M, typename F, typename D, u64 alignment = alignof(F)>
F *
scmemmove(F *dest, D *src) noexcept
{
  if ( dest == nullptr || src == nullptr ) return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) ) return nullptr;

  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src), M * sizeof(D));
  } else {
    if ( dest < src ) {
      for ( u64 i = 0; i < M; i++ ) dest[i] = static_cast<F>(src[i]);
    } else if ( dest > src ) {
      for ( u64 i = M; i > 0; --i ) dest[i - 1] = static_cast<F>(src[i - 1]);
    }
  }

  __mem_barrier();
  return dest;
};

template<u64 M, typename F, typename D, u64 alignment = alignof(F)>
bool
rscmemmove(F &dest, D &src) noexcept
{
  if ( !__is_aligned_to_r(dest, alignment) || !__is_aligned_to_r(src, alignment) ) return false;

  F *d = &dest;
  D *s = &src;
  if constexpr ( sizeof(F) == sizeof(D) && micron::is_trivially_copyable_v<F> && micron::is_trivially_copyable_v<D> ) {
    __memmove_bytes(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), M * sizeof(D));
  } else {
    if ( d < s ) {
      for ( u64 i = 0; i < M; i++ ) d[i] = static_cast<F>(s[i]);
    } else if ( d > s ) {
      for ( u64 i = M; i > 0; --i ) d[i - 1] = static_cast<F>(s[i - 1]);
    }
  }

  __mem_barrier();
  return true;
};

};      // namespace micron

#if defined(__micron_freestanding)
// c-abi
extern "C" __attribute__((used, optimize("-fno-tree-loop-distribute-patterns"))) inline void *
memmove(void *d, const void *s, __SIZE_TYPE__ n) noexcept
{
  micron::__memmove_bytes(static_cast<byte *>(d), static_cast<const byte *>(s), static_cast<u64>(n));
  return d;
}
#endif
