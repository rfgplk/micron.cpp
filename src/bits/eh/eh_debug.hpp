//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh)

// Phase 4: was a raw SYS_write to fd 1. port::write_diag goes to fd 2, which is the correct
// destination for a diagnostic anyway -- the fd-1 choice looks accidental. Included here directly
// rather than relied on transitively: abcmalloc/printing.hpp is this tree's cautionary tale about a
// low-level header that only compiles because its includers happen to pull things first.
#include "../../port/panic.hpp"
#include "../../types.hpp"

namespace micron::eh
{

inline void
__dbg_s(const char *s) noexcept
{
#if defined(__MICRON_EH_DEBUG)
  usize n = 0;
  while ( s[n] ) ++n;
  micron::port::write_diag(s, n);
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
  micron::port::write_diag(buf, 19);
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
