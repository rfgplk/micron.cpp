//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"

#include "auxv.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// getauxval

namespace micron
{

inline constexpr usize __auxv_cache_max = 48;

struct __auxv_cache_t {
  auxv_t e[__auxv_cache_max];
  usize n;
  u32 state;      // 0 = unread, 1 = published
};

inline __auxv_cache_t __auxv_cache{};

[[gnu::cold, gnu::noinline]] inline void
__auxv_load(void) noexcept
{
  usize count = 0;
  // NOTE: avoid pulling in io spaghetti
  const long fd = micron::syscall(SYS_openat, -100 /* AT_FDCWD */, "/proc/self/auxv", 0 /* O_RDONLY */, 0);
  if ( !micron::syscall_failed(fd) ) {
    for ( ; count < __auxv_cache_max; ) {
      auxv_t ent{};
      const long r = micron::syscall(SYS_read, fd, &ent, sizeof(ent));
      if ( r != static_cast<long>(sizeof(ent)) ) break;
      if ( ent.a_type == at_null ) break;
      __auxv_cache.e[count++] = ent;
    }
    micron::syscall(SYS_close, fd);
  }
  __auxv_cache.n = count;
  // NOTE: avoid pulling in the whole atomic chain, spaghetti
  __atomic_store_n(&__auxv_cache.state, 1u, __ATOMIC_RELEASE);
}

inline unsigned long
getauxval(unsigned long type) noexcept
{
  if ( __atomic_load_n(&__auxv_cache.state, __ATOMIC_ACQUIRE) == 0u ) [[unlikely]]
    __auxv_load();
  for ( usize i = 0; i < __auxv_cache.n; ++i ) {
    if ( __auxv_cache.e[i].a_type == type ) return __auxv_cache.e[i].a_val;
  }
  return 0;
}

inline bool
auxval_has(unsigned long type) noexcept
{
  if ( __atomic_load_n(&__auxv_cache.state, __ATOMIC_ACQUIRE) == 0u ) [[unlikely]]
    __auxv_load();
  for ( usize i = 0; i < __auxv_cache.n; ++i ) {
    if ( __auxv_cache.e[i].a_type == type ) return true;
  }
  return false;
}

template<typename T = void>
inline T *
getauxptr(unsigned long type) noexcept
{
  return reinterpret_cast<T *>(getauxval(type));
}

};      // namespace micron
