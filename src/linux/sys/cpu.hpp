//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

// WARNING: nasty workaround, but we need this here since the attr setting functions in pthread need cpu_set_t in global
// ns and we'll run into conflicting declarations with bits/cpu-set otherwise

#ifndef _BITS_CPU_SET_H
#define _BITS_CPU_SET_H 1

typedef struct {
  __syscall_ulong_type __bits[1024 / (8 * sizeof(__syscall_ulong_type))];
} cpu_set_t;
#endif

#include "../../array/constexprarray.hpp"

namespace micron
{

namespace posix
{

int
getcpu(u32 *cpu, u32 *node)
{
  return static_cast<int>(micron::syscall(SYS_getcpu, cpu, node, nullptr));
}

int
getcpu(void)
{
  int cpu = 0;
  micron::syscall(SYS_getcpu, &cpu, nullptr, nullptr);
  return cpu;
}

constexpr static const int cpu_setsize = 1024;
constexpr static const int __ncpubits = sizeof(__syscall_ulong_type) * 8;

constexpr usize
cpu_bits_size(usize count)
{
  return ((count + __ncpubits - 1) / __ncpubits) * sizeof(__syscall_ulong_type);
}

template <usize N> struct __cpu_set_t {
  micron::constexpr_array<unsigned long, (N + __ncpubits - 1) / __ncpubits> __bits{};

  constexpr __cpu_set_t(void) { cpu_zero(); }

  constexpr __cpu_set_t(usize cpu) { cpu_set(cpu); }

  constexpr void
  cpu_set(usize cpu)
  {
    __bits[cpu / __ncpubits] |= 1UL << (cpu % __ncpubits);
  }

  constexpr void
  cpu_clr(usize cpu)
  {
    __bits[cpu / __ncpubits] &= ~(1UL << (cpu % __ncpubits));
  }

  constexpr bool
  cpu_isset(usize cpu) const
  {
    return (__bits[cpu / __ncpubits] & (1UL << (cpu % __ncpubits))) != 0;
  }

  constexpr void
  cpu_zero()
  {
    for ( auto &b : __bits )
      b = 0;
  }

  constexpr usize
  cpu_count() const
  {
    usize cnt = 0;
    for ( auto b : __bits ) {
      for ( int i = 0; i < __ncpubits; ++i )
        if ( b & (1UL << i) )
          ++cnt;
    }
    return cnt;
  }

  constexpr void
  cpu_and(const __cpu_set_t &other)
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      __bits[i] &= other.__bits[i];
  }

  constexpr void
  cpu_or(const __cpu_set_t &other)
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      __bits[i] |= other.__bits[i];
  }

  constexpr void
  cpu_xor(const __cpu_set_t &other)
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      __bits[i] ^= other.__bits[i];
  }

  constexpr bool
  cpu_equal(const __cpu_set_t &other) const
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      if ( __bits[i] != other.__bits[i] )
        return false;
    return true;
  }

  constexpr usize
  size() const
  {
    return sizeof(__bits);
  }
};

template <usize N> struct __cpu_set_s_t {
  micron::constexpr_array<unsigned long, N> __bits{};

  constexpr void
  cpu_set_s(usize cpu)
  {
    __bits[cpu / __ncpubits] |= 1UL << (cpu % __ncpubits);
  }

  constexpr void
  cpu_clr_s(usize cpu)
  {
    __bits[cpu / __ncpubits] &= ~(1UL << (cpu % __ncpubits));
  }

  constexpr bool
  cpu_isset_s(usize cpu) const
  {
    return (__bits[cpu / __ncpubits] & (1UL << (cpu % __ncpubits))) != 0;
  }

  constexpr void
  cpu_zero_s()
  {
    for ( auto &b : __bits )
      b = 0;
  }

  constexpr usize
  cpu_count_s() const
  {
    usize cnt = 0;
    for ( auto b : __bits ) {
      for ( int i = 0; i < __ncpubits; ++i )
        if ( b & (1UL << i) )
          ++cnt;
    }
    return cnt;
  }

  constexpr void
  cpu_and_s(const __cpu_set_s_t &other)
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      __bits[i] &= other.__bits[i];
  }

  constexpr void
  cpu_or_s(const __cpu_set_s_t &other)
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      __bits[i] |= other.__bits[i];
  }

  constexpr void
  cpu_xor_s(const __cpu_set_s_t &other)
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      __bits[i] ^= other.__bits[i];
  }

  constexpr bool
  cpu_equal_s(const __cpu_set_s_t &other) const
  {
    for ( usize i = 0; i < __bits.size(); ++i )
      if ( __bits[i] != other.__bits[i] )
        return false;
    return true;
  }
};

template <usize N = cpu_setsize>
constexpr auto
cpu_alloc()
{
  return __cpu_set_s_t<(N + __ncpubits - 1) / __ncpubits>{};
}

constexpr void
cpu_free(auto &)
{
}     // no-op in constexpr context

using cpu_set_t = __cpu_set_t<cpu_setsize>;

}     // namespace posix
};     // namespace micron
