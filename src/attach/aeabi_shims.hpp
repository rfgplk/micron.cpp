//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ARMv7 AEABI helper shims for guest modules (arm32 only)

#if defined(MICRON_ATTACH_MODULE) && defined(__micron_arch_arm32)

namespace
{
[[gnu::always_inline]] inline void
__micron_aeabi_udivmod32(unsigned int n, unsigned int d, unsigned int &q_out, unsigned int &r_out) noexcept
{
  if ( d == 0 ) __builtin_trap();
  unsigned int q = 0;
  unsigned int r = 0;
  for ( int i = 0; i < 32; ++i ) {
    r = (r << 1) | (n >> 31);
    n <<= 1;
    q <<= 1;
    if ( r >= d ) {
      r -= d;
      q |= 1;
    }
  }
  q_out = q;
  r_out = r;
}
};      // namespace

extern "C" {

__attribute__((weak, used)) unsigned int
__aeabi_uidiv(unsigned int n, unsigned int d) noexcept
{
  unsigned int q;
  unsigned int r;
  __micron_aeabi_udivmod32(n, d, q, r);
  return q;
}

__attribute__((weak, used)) int
__aeabi_idiv(int n, int d) noexcept
{
  const bool neg = (n < 0) ^ (d < 0);
  const unsigned int un = (n < 0) ? -static_cast<unsigned int>(n) : static_cast<unsigned int>(n);
  const unsigned int ud = (d < 0) ? -static_cast<unsigned int>(d) : static_cast<unsigned int>(d);
  unsigned int q;
  unsigned int r;
  __micron_aeabi_udivmod32(un, ud, q, r);
  return neg ? static_cast<int>(0u - q) : static_cast<int>(q);
}

__attribute__((naked, weak, used)) void
__aeabi_uldivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, r6, r7, r8, lr}\n"
                   "mov    r4, r0\n"
                   "mov    r5, r1\n"
                   "mov    r6, r2\n"
                   "mov    r7, r3\n"
                   "bl     __udivdi3\n"
                   "mov    r8, r0\n"
                   "umull  r2, r3, r0, r6\n"
                   "mla    r3, r1, r6, r3\n"
                   "mla    r3, r0, r7, r3\n"
                   "subs   r2, r4, r2\n"
                   "sbc    r3, r5, r3\n"
                   "mov    r0, r8\n"
                   "pop    {r4, r5, r6, r7, r8, pc}\n");
}

__attribute__((naked, weak, used)) void
__aeabi_ldivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, r6, r7, r8, lr}\n"
                   "mov    r4, r0\n"
                   "mov    r5, r1\n"
                   "mov    r6, r2\n"
                   "mov    r7, r3\n"
                   "bl     __divdi3\n"
                   "mov    r8, r0\n"
                   "umull  r2, r3, r0, r6\n"
                   "mla    r3, r1, r6, r3\n"
                   "mla    r3, r0, r7, r3\n"
                   "subs   r2, r4, r2\n"
                   "sbc    r3, r5, r3\n"
                   "mov    r0, r8\n"
                   "pop    {r4, r5, r6, r7, r8, pc}\n");
}

__attribute__((naked, weak, used)) void
__aeabi_idivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, lr}\n"
                   "mov    r4, r0\n"
                   "mov    r5, r1\n"
                   "bl     __aeabi_idiv\n"
                   "mul    r1, r0, r5\n"
                   "sub    r1, r4, r1\n"
                   "pop    {r4, r5, pc}\n");
}

__attribute__((naked, weak, used)) void
__aeabi_uidivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, lr}\n"
                   "mov    r4, r0\n"
                   "mov    r5, r1\n"
                   "bl     __aeabi_uidiv\n"
                   "mul    r1, r0, r5\n"
                   "sub    r1, r4, r1\n"
                   "pop    {r4, r5, pc}\n");
}

__attribute__((weak, used, pcs("aapcs"))) double
__aeabi_ul2d(unsigned long long x) noexcept
{
  const unsigned int hi = static_cast<unsigned int>(x >> 32);
  const unsigned int lo = static_cast<unsigned int>(x);
  return static_cast<double>(hi) * 4294967296.0 + static_cast<double>(lo);
}

__attribute__((weak, used, pcs("aapcs"))) double
__aeabi_l2d(long long x) noexcept
{
  if ( x < 0 ) return -__aeabi_ul2d(0ull - static_cast<unsigned long long>(x));
  return __aeabi_ul2d(static_cast<unsigned long long>(x));
}

__attribute__((weak, used, pcs("aapcs"))) float
__aeabi_ul2f(unsigned long long x) noexcept
{
  return static_cast<float>(__aeabi_ul2d(x));
}

__attribute__((weak, used, pcs("aapcs"))) float
__aeabi_l2f(long long x) noexcept
{
  return static_cast<float>(__aeabi_l2d(x));
}

__attribute__((weak, used, pcs("aapcs"))) unsigned long long
__aeabi_d2ulz(double x) noexcept
{
  if ( !(x >= 0.0) ) return 0;
  if ( x >= 18446744073709551616.0 ) return ~0ULL;
  if ( x < 4294967296.0 ) return static_cast<unsigned int>(x);
  unsigned int hi = static_cast<unsigned int>(x * (1.0 / 4294967296.0));
  double lo_d = x - static_cast<double>(hi) * 4294967296.0;
  if ( lo_d < 0.0 ) {
    --hi;
    lo_d += 4294967296.0;
  } else if ( lo_d >= 4294967296.0 ) {
    ++hi;
    lo_d -= 4294967296.0;
  }
  const unsigned int lo = static_cast<unsigned int>(lo_d);
  return (static_cast<unsigned long long>(hi) << 32) | lo;
}

__attribute__((weak, used, pcs("aapcs"))) long long
__aeabi_d2lz(double x) noexcept
{
  if ( x != x ) return 0;
  if ( x >= 9223372036854775808.0 ) return 0x7FFFFFFFFFFFFFFFLL;
  if ( x < -9223372036854775808.0 ) return static_cast<long long>(0x8000000000000000ULL);
  if ( x >= 0.0 ) return static_cast<long long>(__aeabi_d2ulz(x));
  return static_cast<long long>(0ull - __aeabi_d2ulz(-x));
}

__attribute__((weak, used, pcs("aapcs"))) unsigned long long
__aeabi_f2ulz(float x) noexcept
{
  return __aeabi_d2ulz(static_cast<double>(x));
}

__attribute__((weak, used, pcs("aapcs"))) long long
__aeabi_f2lz(float x) noexcept
{
  return __aeabi_d2lz(static_cast<double>(x));
}
}

#endif
