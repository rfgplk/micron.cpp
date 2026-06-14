//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh)

#include "../../syscall.hpp"
#include "../../types.hpp"

namespace micron::eh
{

inline void
__dbg_s(const char *s) noexcept
{
#if defined(__MICRON_EH_DEBUG)
  usize n = 0;
  while ( s[n] ) ++n;
  micron::syscall(SYS_write, 1, reinterpret_cast<const void *>(s), n);
#else
  (void)s;
#endif
}

inline void
__dbg_h(usize v) noexcept
{
#if defined(__MICRON_EH_DEBUG)
  char buf[19];
  buf[0] = '0';
  buf[1] = 'x';
  for ( int i = 0; i < 16; ++i ) {
    const unsigned nyb = (v >> ((15 - i) * 4)) & 0xf;
    buf[2 + i] = static_cast<char>(nyb < 10 ? '0' + nyb : 'a' + (nyb - 10));
  }
  buf[18] = '\n';
  micron::syscall(SYS_write, 1, reinterpret_cast<const void *>(buf), 19);
#else
  (void)v;
#endif
}

inline void
__dbg_kv(const char *k, usize v) noexcept
{
#if defined(__MICRON_EH_DEBUG)
  __dbg_s(k);
  __dbg_h(v);
#else
  (void)k;
  (void)v;
#endif
}

}      // namespace micron::eh

#endif      // __micron_eh
