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

namespace micron
{

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
bytemove(F *_dest, D *_src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest));
  byte *src = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<D> *>(_src));

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
rbytemove(F &_dest, D &_src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  byte *src = reinterpret_cast<byte *>(&_src);

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F *
constexpr_bytemove(F *_dest, D *_src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest));
  byte *src = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<D> *>(_src));

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F &
constexpr_rbytemove(F &_dest, D &_src, const u64 cnt) noexcept
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  byte *src = reinterpret_cast<byte *>(&_src);

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template <u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
cbytemove(F *_dest, D *_src) noexcept
{
  byte *dest = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest));
  byte *src = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<D> *>(_src));

  if ( dest < src ) {
    for ( u64 i = 0; i < M; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = M; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template <u64 M, typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F &
crbytemove(F &_dest, D &_src) noexcept
{
  byte *dest = reinterpret_cast<byte *>(&_dest);
  byte *src = reinterpret_cast<byte *>(&_src);

  if ( dest < src ) {
    for ( u64 i = 0; i < M; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = M; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  return _dest;
};

template <typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
sbytemove(F *_dest, D *_src, const u64 cnt) noexcept
{
  if ( _dest == nullptr || _src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return nullptr;

  byte *dest = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest));
  byte *src = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<D> *>(_src));

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  __mem_barrier();
  return _dest;
};

template <typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rsbytemove(F &_dest, D &_src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return false;

  byte *dest = reinterpret_cast<byte *>(&_dest);
  byte *src = reinterpret_cast<byte *>(&_src);

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  __mem_barrier();
  return true;
};

template <u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
F *
scbytemove(F *_dest, D *_src) noexcept
{
  if ( _dest == nullptr || _src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return nullptr;

  byte *dest = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<F> *>(_dest));
  byte *src = reinterpret_cast<byte *>(const_cast<micron::remove_cv_t<D> *>(_src));

  if ( dest < src ) {
    for ( u64 i = 0; i < M; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = M; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  __mem_barrier();
  return _dest;
};

template <u64 M, typename F, typename D, u64 alignment = 1>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
bool
rscrbytemove(F &_dest, D &_src) noexcept
{
  if ( !__is_aligned_to(_dest, alignment) || !__is_aligned_to(_src, alignment) )
    return false;

  byte *dest = reinterpret_cast<byte *>(&_dest);
  byte *src = reinterpret_cast<byte *>(&_src);

  if ( dest < src ) {
    for ( u64 i = 0; i < M; i++ )
      dest[i] = src[i];
  } else if ( dest > src ) {
    for ( u64 i = M; i > 0; --i )
      dest[i - 1] = src[i - 1];
  }

  __mem_barrier();
  return true;
};

template <typename F, typename D>
F *
memmove(F *dest, D *src, const u64 cnt) noexcept
{
  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = static_cast<F>(src[i]);
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = static_cast<F>(src[i - 1]);
  }

  return dest;
};

template <typename F, typename D>
F &
rmemmove(F &dest, D &src, const u64 cnt) noexcept
{
  F *d = &dest;
  D *s = &src;

  if ( d < s ) {
    for ( u64 i = 0; i < cnt; i++ )
      d[i] = static_cast<F>(s[i]);
  } else if ( d > s ) {
    for ( u64 i = cnt; i > 0; --i )
      d[i - 1] = static_cast<F>(s[i - 1]);
  }

  return dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F *
constexpr_memmove(F *dest, D *src, const u64 cnt) noexcept
{
  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = static_cast<F>(src[i]);
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = static_cast<F>(src[i - 1]);
  }

  return dest;
};

template <typename F, typename D>
  requires(micron::is_fundamental_v<F> && micron::is_fundamental_v<D>)
constexpr F &
constexpr_rmemmove(F &dest, D &src, const u64 cnt) noexcept
{
  F *d = &dest;
  D *s = &src;

  if ( d < s ) {
    for ( u64 i = 0; i < cnt; i++ )
      d[i] = static_cast<F>(s[i]);
  } else if ( d > s ) {
    for ( u64 i = cnt; i > 0; --i )
      d[i - 1] = static_cast<F>(s[i - 1]);
  }

  return dest;
};

template <u64 M, typename F, typename D>
F *
cmemmove(F *dest, D *src) noexcept
{
  if ( dest < src ) {
    for ( u64 i = 0; i < M; i++ )
      dest[i] = static_cast<F>(src[i]);
  } else if ( dest > src ) {
    for ( u64 i = M; i > 0; --i )
      dest[i - 1] = static_cast<F>(src[i - 1]);
  }

  return dest;
};

template <u64 M, typename F, typename D>
F &
crmemmove(F &dest, D &src) noexcept
{
  F *d = &dest;
  D *s = &src;

  if ( d < s ) {
    for ( u64 i = 0; i < M; i++ )
      d[i] = static_cast<F>(s[i]);
  } else if ( d > s ) {
    for ( u64 i = M; i > 0; --i )
      d[i - 1] = static_cast<F>(s[i - 1]);
  }

  return dest;
};

template <typename F, typename D, u64 alignment = alignof(F)>
F *
smemmove(F *dest, D *src, const u64 cnt) noexcept
{
  if ( dest == nullptr || src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return nullptr;

  if ( dest < src ) {
    for ( u64 i = 0; i < cnt; i++ )
      dest[i] = static_cast<F>(src[i]);
  } else if ( dest > src ) {
    for ( u64 i = cnt; i > 0; --i )
      dest[i - 1] = static_cast<F>(src[i - 1]);
  }

  __mem_barrier();
  return dest;
};

template <typename F, typename D, u64 alignment = alignof(F)>
bool
rsmemmove(F &dest, D &src, const u64 cnt) noexcept
{
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return false;

  F *d = &dest;
  D *s = &src;

  if ( d < s ) {
    for ( u64 i = 0; i < cnt; i++ )
      d[i] = static_cast<F>(s[i]);
  } else if ( d > s ) {
    for ( u64 i = cnt; i > 0; --i )
      d[i - 1] = static_cast<F>(s[i - 1]);
  }

  __mem_barrier();
  return true;
};

template <u64 M, typename F, typename D, u64 alignment = alignof(F)>
F *
scmemmove(F *dest, D *src) noexcept
{
  if ( dest == nullptr || src == nullptr )
    return nullptr;
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return nullptr;

  if ( dest < src ) {
    for ( u64 i = 0; i < M; i++ )
      dest[i] = static_cast<F>(src[i]);
  } else if ( dest > src ) {
    for ( u64 i = M; i > 0; --i )
      dest[i - 1] = static_cast<F>(src[i - 1]);
  }

  __mem_barrier();
  return dest;
};

template <u64 M, typename F, typename D, u64 alignment = alignof(F)>
bool
rscmemmove(F &dest, D &src) noexcept
{
  if ( !__is_aligned_to(dest, alignment) || !__is_aligned_to(src, alignment) )
    return false;

  F *d = &dest;
  D *s = &src;

  if ( d < s ) {
    for ( u64 i = 0; i < M; i++ )
      d[i] = static_cast<F>(s[i]);
  } else if ( d > s ) {
    for ( u64 i = M; i > 0; --i )
      d[i - 1] = static_cast<F>(s[i - 1]);
  }

  __mem_barrier();
  return true;
};

};
