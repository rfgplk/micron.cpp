//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__mport_abi.hpp"

#include "../../bits/__pause.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline i64
wait(u32 *__addr, u32 __expected, i64 __timeout_ns) noexcept
{
  if ( __addr == nullptr ) return -22;
  if ( __atomic_load_n(__addr, __ATOMIC_ACQUIRE) != __expected ) return -11;
  if ( __timeout_ns == 0 ) return -110;

  const i64 __start = static_cast<i64>(::mc_mport_mono_ns());
  for ( ;; ) {
    if ( __atomic_load_n(__addr, __ATOMIC_ACQUIRE) != __expected ) return 0;
    if ( __timeout_ns >= 0 && (static_cast<i64>(::mc_mport_mono_ns()) - __start) >= __timeout_ns ) return -110;
    __cpu_pause();
  }
}

inline i64
wake(u32 *__addr, i32 __n) noexcept
{
  (void)__addr;
  return static_cast<i64>(__n);
}

};      // namespace port
};      // namespace micron
