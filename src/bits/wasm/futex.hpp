//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "time.hpp"

namespace micron::wasm
{
inline long
futex(unsigned *addr, int op, unsigned expected, timespec_t *timeout, unsigned *other, unsigned mask) noexcept
{
  if ( other ) return -95;
  const int command = op & ~(128 | 256);
  if ( command == 1 || command == 10 ) {
    if ( command == 10 && mask != 0xffffffffu ) return -95;
    return wake32(addr, expected);
  }
  if ( command != 0 && command != 9 ) return -95;
  if ( command == 9 && mask != 0xffffffffu ) return -95;
  long long ns = -1;
  if ( timeout ) {
    if ( timeout->tv_sec < 0 || timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000LL
         || timeout->tv_sec > (0x7fffffffffffffffLL - timeout->tv_nsec) / 1000000000LL )
      return -22;
    ns = timeout->tv_sec * 1000000000LL + timeout->tv_nsec;
    if ( command == 9 ) {
      const long long now = __micron_wasm_host_clock(op & 256 ? 0 : 1);
      if ( now < 0 ) return now;
      ns = ns > now ? ns - now : 0;
    }
  }
  return wait32(addr, expected, ns);
}
}      // namespace micron::wasm
