//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt


#pragma once

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
constexpr static const int __ncpubits = sizeof(unsigned long) * 8;

constexpr size_t
cpu_bits_size(size_t count)
{
  return ((count + __ncpubits - 1) / __ncpubits) * sizeof(unsigned long);
}

template <size_t N> struct __cpu_set_t {
  micron::constexpr_array<unsigned long, (N + __ncpubits - 1) / __ncpubits> __bits{};

  constexpr __cpu_set_t(void) { cpu_zero(); }
  constexpr __cpu_set_t(size_t cpu) { cpu_set(cpu); }
  constexpr void
  cpu_set(size_t cpu)
  {
    __bits[cpu / __ncpubits] |= 1UL << (cpu % __ncpubits);
  }
  constexpr void
  cpu_clr(size_t cpu)
  {
    __bits[cpu / __ncpubits] &= ~(1UL << (cpu % __ncpubits));
  }
  constexpr bool
  cpu_isset(size_t cpu) const
  {
    return (__bits[cpu / __ncpubits] & (1UL << (cpu % __ncpubits))) != 0;
  }
  constexpr void
  cpu_zero()
  {
    for ( auto &b : __bits )
      b = 0;
  }
  constexpr size_t
  cpu_count() const
  {
    size_t cnt = 0;
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
    for ( size_t i = 0; i < __bits.size(); ++i )
      __bits[i] &= other.__bits[i];
  }
  constexpr void
  cpu_or(const __cpu_set_t &other)
  {
    for ( size_t i = 0; i < __bits.size(); ++i )
      __bits[i] |= other.__bits[i];
  }
  constexpr void
  cpu_xor(const __cpu_set_t &other)
  {
    for ( size_t i = 0; i < __bits.size(); ++i )
      __bits[i] ^= other.__bits[i];
  }
  constexpr bool
  cpu_equal(const __cpu_set_t &other) const
  {
    for ( size_t i = 0; i < __bits.size(); ++i )
      if ( __bits[i] != other.__bits[i] )
        return false;
    return true;
  }
};

template <size_t N> struct __cpu_set_s_t {
  micron::constexpr_array<unsigned long, N> __bits{};
  constexpr void
  cpu_set_s(size_t cpu)
  {
    __bits[cpu / __ncpubits] |= 1UL << (cpu % __ncpubits);
  }
  constexpr void
  cpu_clr_s(size_t cpu)
  {
    __bits[cpu / __ncpubits] &= ~(1UL << (cpu % __ncpubits));
  }
  constexpr bool
  cpu_isset_s(size_t cpu) const
  {
    return (__bits[cpu / __ncpubits] & (1UL << (cpu % __ncpubits))) != 0;
  }
  constexpr void
  cpu_zero_s()
  {
    for ( auto &b : __bits )
      b = 0;
  }
  constexpr size_t
  cpu_count_s() const
  {
    size_t cnt = 0;
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
    for ( size_t i = 0; i < __bits.size(); ++i )
      __bits[i] &= other.__bits[i];
  }
  constexpr void
  cpu_or_s(const __cpu_set_s_t &other)
  {
    for ( size_t i = 0; i < __bits.size(); ++i )
      __bits[i] |= other.__bits[i];
  }
  constexpr void
  cpu_xor_s(const __cpu_set_s_t &other)
  {
    for ( size_t i = 0; i < __bits.size(); ++i )
      __bits[i] ^= other.__bits[i];
  }
  constexpr bool
  cpu_equal_s(const __cpu_set_s_t &other) const
  {
    for ( size_t i = 0; i < __bits.size(); ++i )
      if ( __bits[i] != other.__bits[i] )
        return false;
    return true;
  }
};

template <size_t N = cpu_setsize>
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

}
};
