//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// FREESTANDING LIBGCC INTEGER SYMBOLS
//
// WARNING: NOT FOR EXTERNAL USE. FREESTANDING -NOSTDLIB LINKS HAVE NO LIBGCC, BUT THE
// COMPILER STILL LOWERS SOME INTEGER BUILTINS TO LIBGCC CALLS

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

#endif

extern "C" __attribute__((weak)) int
__popcountsi2(unsigned int v) noexcept
{
  v = v - ((v >> 1) & 0x55555555u);
  v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u);
  v = (v + (v >> 4)) & 0x0f0f0f0fu;
  return (int)((v * 0x01010101u) >> 24);
}

extern "C" __attribute__((weak)) int
__popcountdi2(unsigned long long v) noexcept
{
#if defined(__x86_64__) || defined(__aarch64__)
  v = v - ((v >> 1) & 0x5555555555555555ull);
  v = (v & 0x3333333333333333ull) + ((v >> 2) & 0x3333333333333333ull);
  v = (v + (v >> 4)) & 0x0f0f0f0f0f0f0f0full;
  return (int)((v * 0x0101010101010101ull) >> 56);
#else
  return __popcountsi2((unsigned int)v) + __popcountsi2((unsigned int)(v >> 32));
#endif
}

#if defined(__SIZEOF_INT128__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

extern "C" __attribute__((weak, optimize("-fno-tree-loop-distribute-patterns"))) unsigned __int128
__udivti3(unsigned __int128 n, unsigned __int128 d) noexcept
{
  if ( d == 0 ) __builtin_trap();
  unsigned __int128 q = 0;
  unsigned __int128 r = 0;
  for ( int i = 0; i < 128; ++i ) {
    r = (r << 1) | static_cast<unsigned __int128>(n >> 127);
    n <<= 1;
    q <<= 1;
    if ( r >= d ) {
      r -= d;
      q |= 1;
    }
  }
  return q;
}

extern "C" __attribute__((weak, optimize("-fno-tree-loop-distribute-patterns"))) unsigned __int128
__umodti3(unsigned __int128 n, unsigned __int128 d) noexcept
{
  if ( d == 0 ) __builtin_trap();
  unsigned __int128 r = 0;
  for ( int i = 0; i < 128; ++i ) {
    r = (r << 1) | static_cast<unsigned __int128>(n >> 127);
    n <<= 1;
    if ( r >= d ) r -= d;
  }
  return r;
}

#pragma GCC diagnostic pop
#endif
