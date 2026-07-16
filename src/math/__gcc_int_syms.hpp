//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// FREESTANDING LIBGCC INTEGER SYMBOLS (32-BIT TARGETS)
//
// WARNING: NOT FOR EXTERNAL USE. on 32-bit targets the compiler lowers 64-bit
// division/modulo to libgcc calls (__udivdi3 & friends); freestanding -nostdlib links have
// no libgcc, so any u64 divide reachable from a freestanding TU fails to link. these are
// weak shift-subtract implementations, the same shim mechanism as __gcc_math_syms.hpp.
// NEVER call these directly; write normal division and let the compiler lower it.

#include "../types.hpp"

#if !defined(__x86_64__) && !defined(__aarch64__)

extern "C" __attribute__((weak)) unsigned long long
__udivmoddi4(unsigned long long n, unsigned long long d, unsigned long long *rem) noexcept
{
  if ( d == 0 ) {
    if ( rem ) *rem = 0;
    return 0;      // freestanding: no SIGFPE trap emulation, define div-by-zero as 0
  }
  unsigned long long q = 0, r = 0;
  for ( int i = 63; i >= 0; --i ) {
    r = (r << 1) | ((n >> i) & 1ull);
    if ( r >= d ) {
      r -= d;
      q |= (1ull << i);
    }
  }
  if ( rem ) *rem = r;
  return q;
}

extern "C" __attribute__((weak)) unsigned long long
__udivdi3(unsigned long long n, unsigned long long d) noexcept
{
  return __udivmoddi4(n, d, nullptr);
}

extern "C" __attribute__((weak)) unsigned long long
__umoddi3(unsigned long long n, unsigned long long d) noexcept
{
  unsigned long long r = 0;
  __udivmoddi4(n, d, &r);
  return r;
}

extern "C" __attribute__((weak)) long long
__divdi3(long long n, long long d) noexcept
{
  bool neg = (n < 0) != (d < 0);
  unsigned long long un = n < 0 ? 0ull - static_cast<unsigned long long>(n) : static_cast<unsigned long long>(n);
  unsigned long long ud = d < 0 ? 0ull - static_cast<unsigned long long>(d) : static_cast<unsigned long long>(d);
  unsigned long long q = __udivmoddi4(un, ud, nullptr);
  return neg ? -static_cast<long long>(q) : static_cast<long long>(q);
}

extern "C" __attribute__((weak)) long long
__moddi3(long long n, long long d) noexcept
{
  unsigned long long un = n < 0 ? 0ull - static_cast<unsigned long long>(n) : static_cast<unsigned long long>(n);
  unsigned long long ud = d < 0 ? 0ull - static_cast<unsigned long long>(d) : static_cast<unsigned long long>(d);
  unsigned long long r = 0;
  __udivmoddi4(un, ud, &r);
  return n < 0 ? -static_cast<long long>(r) : static_cast<long long>(r);
}

// 64-bit bit-scans: higher optimization levels lower __builtin_ctzll/clzll on u64 to these
// on 32-bit targets instead of the two-half inline sequence
extern "C" __attribute__((weak)) int
__ctzdi2(unsigned long long x) noexcept
{
  if ( x == 0 ) return 64;
  unsigned lo = static_cast<unsigned>(x);
  if ( lo ) return __builtin_ctz(lo);
  return 32 + __builtin_ctz(static_cast<unsigned>(x >> 32));
}

extern "C" __attribute__((weak)) int
__clzdi2(unsigned long long x) noexcept
{
  if ( x == 0 ) return 64;
  unsigned hi = static_cast<unsigned>(x >> 32);
  if ( hi ) return __builtin_clz(hi);
  return 32 + __builtin_clz(static_cast<unsigned>(x));
}

extern "C" __attribute__((weak)) int
__ffsdi2(unsigned long long x) noexcept
{
  return x ? 1 + __ctzdi2(x) : 0;
}

extern "C" __attribute__((weak)) int
__popcountdi2(unsigned long long x) noexcept
{
  return __builtin_popcount(static_cast<unsigned>(x)) + __builtin_popcount(static_cast<unsigned>(x >> 32));
}

#endif      // 32-bit targets only
