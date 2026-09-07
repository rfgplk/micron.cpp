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

// WARNING: USED IS NOT OPTIONAL HERE WEAK ALONE ISN'T ENOUGH
#define __mc_libgcc_sym __attribute__((weak, used, retain))

#if !defined(__x86_64__) && !defined(__aarch64__)

extern "C" __mc_libgcc_sym unsigned long long
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

extern "C" __mc_libgcc_sym unsigned long long
__udivdi3(unsigned long long n, unsigned long long d) noexcept
{
  return __udivmoddi4(n, d, nullptr);
}

extern "C" __mc_libgcc_sym unsigned long long
__umoddi3(unsigned long long n, unsigned long long d) noexcept
{
  unsigned long long r = 0;
  __udivmoddi4(n, d, &r);
  return r;
}

extern "C" __mc_libgcc_sym long long
__divdi3(long long n, long long d) noexcept
{
  bool neg = (n < 0) != (d < 0);
  unsigned long long un = n < 0 ? 0ull - static_cast<unsigned long long>(n) : static_cast<unsigned long long>(n);
  unsigned long long ud = d < 0 ? 0ull - static_cast<unsigned long long>(d) : static_cast<unsigned long long>(d);
  unsigned long long q = __udivmoddi4(un, ud, nullptr);
  // WARNING: unsigned negate -- LLONG_MIN / 1 yields q == 2^63 and negating that as a long long is UB
  return neg ? static_cast<long long>(0ull - q) : static_cast<long long>(q);
}

extern "C" __mc_libgcc_sym long long
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
extern "C" __mc_libgcc_sym int
__ctzdi2(unsigned long long x) noexcept
{
  if ( x == 0 ) return 64;
  unsigned lo = static_cast<unsigned>(x);
  if ( lo ) return __builtin_ctz(lo);
  return 32 + __builtin_ctz(static_cast<unsigned>(x >> 32));
}

extern "C" __mc_libgcc_sym int
__clzdi2(unsigned long long x) noexcept
{
  if ( x == 0 ) return 64;
  unsigned hi = static_cast<unsigned>(x >> 32);
  if ( hi ) return __builtin_clz(hi);
  return 32 + __builtin_clz(static_cast<unsigned>(x));
}

extern "C" __mc_libgcc_sym int
__ffsdi2(unsigned long long x) noexcept
{
  return x ? 1 + __ctzdi2(x) : 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ARM EABI DIVISION HELPERS
#if defined(__micron_arch_arm32) && defined(__ARM_EABI__) && !defined(MICRON_CRT_PROVIDES_AEABI)

extern "C" __mc_libgcc_sym unsigned int
__udivmodsi4(unsigned int n, unsigned int d, unsigned int *rem) noexcept
{
  if ( d == 0 ) {
    if ( rem ) *rem = 0;
    return 0;      // freestanding: no SIGFPE trap emulation, define div-by-zero as 0
  }
  unsigned int q = 0, r = 0;
  for ( int i = 31; i >= 0; --i ) {
    r = (r << 1) | ((n >> i) & 1u);
    if ( r >= d ) {
      r -= d;
      q |= (1u << i);
    }
  }
  if ( rem ) *rem = r;
  return q;
}

extern "C" __mc_libgcc_sym long long
__divmoddi4(long long n, long long d, long long *rem) noexcept
{
  const bool __qneg = (n < 0) != (d < 0);
  const bool __rneg = n < 0;
  unsigned long long un = n < 0 ? 0ull - static_cast<unsigned long long>(n) : static_cast<unsigned long long>(n);
  unsigned long long ud = d < 0 ? 0ull - static_cast<unsigned long long>(d) : static_cast<unsigned long long>(d);
  unsigned long long ur = 0;
  const unsigned long long uq = __udivmoddi4(un, ud, &ur);
  if ( rem ) *rem = __rneg ? static_cast<long long>(0ull - ur) : static_cast<long long>(ur);
  return __qneg ? static_cast<long long>(0ull - uq) : static_cast<long long>(uq);
}

extern "C" __mc_libgcc_sym unsigned int
__aeabi_uidiv(unsigned int n, unsigned int d) noexcept
{
  return __udivmodsi4(n, d, nullptr);
}

extern "C" __mc_libgcc_sym int
__aeabi_idiv(int n, int d) noexcept
{
  const bool neg = (n < 0) != (d < 0);
  const unsigned int un = n < 0 ? 0u - static_cast<unsigned int>(n) : static_cast<unsigned int>(n);
  const unsigned int ud = d < 0 ? 0u - static_cast<unsigned int>(d) : static_cast<unsigned int>(d);
  const unsigned int q = __udivmodsi4(un, ud, nullptr);
  return neg ? static_cast<int>(0u - q) : static_cast<int>(q);
}

extern "C" __mc_libgcc_sym unsigned long long
__aeabi_uidivmod(unsigned int n, unsigned int d) noexcept
{
  unsigned int r = 0;
  const unsigned int q = __udivmodsi4(n, d, &r);
  return static_cast<unsigned long long>(q) | (static_cast<unsigned long long>(r) << 32);
}

extern "C" __mc_libgcc_sym long long
__aeabi_idivmod(int n, int d) noexcept
{
  const bool qneg = (n < 0) != (d < 0);
  const bool rneg = n < 0;
  const unsigned int un = n < 0 ? 0u - static_cast<unsigned int>(n) : static_cast<unsigned int>(n);
  const unsigned int ud = d < 0 ? 0u - static_cast<unsigned int>(d) : static_cast<unsigned int>(d);
  unsigned int ur = 0;
  const unsigned int uq = __udivmodsi4(un, ud, &ur);
  const unsigned int q = qneg ? 0u - uq : uq;
  const unsigned int r = rneg ? 0u - ur : ur;
  return static_cast<long long>(static_cast<unsigned long long>(q) | (static_cast<unsigned long long>(r) << 32));
}

#if defined(__thumb__)
#define __mc_aeabi_thumb ".thumb_func\n\t"
#else
#define __mc_aeabi_thumb ""
#endif

#define __mc_aeabi_stub(__nm, __callee)                                                                                                    \
  __asm__(".text\n\t"                                                                                                                      \
          ".syntax unified\n\t"                                                                                                            \
          ".align 2\n\t"                                                                                                                   \
          ".weak " __nm "\n\t"                                                                                                             \
          ".type " __nm ", %function\n\t" __mc_aeabi_thumb __nm ":\n\t"                                                                    \
          "push {r4, lr}\n\t"                                                                                                              \
          "sub  sp, sp, #16\n\t"                                                                                                           \
          "add  r4, sp, #8\n\t"                                                                                                            \
          "str  r4, [sp]\n\t"                                                                                                              \
          "bl   " __callee "\n\t"                                                                                                          \
          "ldr  r2, [sp, #8]\n\t"                                                                                                          \
          "ldr  r3, [sp, #12]\n\t"                                                                                                         \
          "add  sp, sp, #16\n\t"                                                                                                           \
          "pop  {r4, pc}\n\t"                                                                                                              \
          ".size " __nm ", . - " __nm "\n\t"                                                                                               \
          ".previous\n\t")

__mc_aeabi_stub("__aeabi_uldivmod", "__udivmoddi4");
__mc_aeabi_stub("__aeabi_ldivmod", "__divmoddi4");

#undef __mc_aeabi_stub
#undef __mc_aeabi_thumb

#endif      // __micron_arch_arm32 && __ARM_EABI__

#endif

extern "C" __mc_libgcc_sym int
__popcountsi2(unsigned int v) noexcept
{
  v = v - ((v >> 1) & 0x55555555u);
  v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u);
  v = (v + (v >> 4)) & 0x0f0f0f0fu;
  return (int)((v * 0x01010101u) >> 24);
}

extern "C" __mc_libgcc_sym int
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
__micron_diagnostic_push
__micron_diagnostic_ignored("-Wpedantic")
extern "C" __mc_libgcc_sym __micron_optimize_no_tree_loop_distribute unsigned __int128
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

extern "C" __mc_libgcc_sym __micron_optimize_no_tree_loop_distribute unsigned __int128
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

__micron_diagnostic_pop
#endif

#undef __mc_libgcc_sym
