//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

#if !defined(__micron_arch_x86_any)
#error "__bmi.hpp included on a non-x86 build"
#endif

// freestanding BMI1 & BMI2 & LZCNT & TZCNT [bmiintrin.h, bmi2intrin.h...]

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

__inline_g unsigned int
_tzcnt_u32(unsigned int v) noexcept
{
  return v ? (unsigned)__builtin_ctz(v) : 32u;
}

__inline_g unsigned long long
_tzcnt_u64(unsigned long long v) noexcept
{
  return v ? (unsigned long long)__builtin_ctzll(v) : 64ull;
}

__inline_g unsigned int
_lzcnt_u32(unsigned int v) noexcept
{
  return v ? (unsigned)__builtin_clz(v) : 32u;
}

__inline_g unsigned long long
_lzcnt_u64(unsigned long long v) noexcept
{
  return v ? (unsigned long long)__builtin_clzll(v) : 64ull;
}

__inline_g unsigned int
__tzcnt_u32(unsigned int v) noexcept
{
  return _tzcnt_u32(v);
}

__inline_g unsigned long long
__tzcnt_u64(unsigned long long v) noexcept
{
  return _tzcnt_u64(v);
}

__inline_g unsigned int
__lzcnt_u32(unsigned int v) noexcept
{
  return _lzcnt_u32(v);
}

__inline_g unsigned long long
__lzcnt_u64(unsigned long long v) noexcept
{
  return _lzcnt_u64(v);
}

__inline_g unsigned int
_andn_u32(unsigned int a, unsigned int b) noexcept
{
  return ~a & b;
}

__inline_g unsigned long long
_andn_u64(unsigned long long a, unsigned long long b) noexcept
{
  return ~a & b;
}

__inline_g unsigned int
_blsr_u32(unsigned int v) noexcept
{
  return v & (v - 1);
}

__inline_g unsigned long long
_blsr_u64(unsigned long long v) noexcept
{
  return v & (v - 1);
}

__inline_g unsigned int
_blsi_u32(unsigned int v) noexcept
{
  return v & (~v + 1);
}

__inline_g unsigned long long
_blsi_u64(unsigned long long v) noexcept
{
  return v & (~v + 1);
}

__inline_g unsigned int
_blsmsk_u32(unsigned int v) noexcept
{
  return v ^ (v - 1);
}

__inline_g unsigned long long
_blsmsk_u64(unsigned long long v) noexcept
{
  return v ^ (v - 1);
}

__inline_g unsigned int
_bextr_u32(unsigned int v, unsigned int start, unsigned int len) noexcept
{
  return __builtin_ia32_bextr_u32(v, ((start & 0xff) | ((len & 0xff) << 8)));
}

__inline_g unsigned long long
_bextr_u64(unsigned long long v, unsigned int start, unsigned int len) noexcept
{
  return __builtin_ia32_bextr_u64(v, ((start & 0xff) | ((len & 0xff) << 8)));
}

__inline_g unsigned int
__bextr_u32(unsigned int v, unsigned int control) noexcept
{
  return __builtin_ia32_bextr_u32(v, control);
}

__inline_g unsigned long long
__bextr_u64(unsigned long long v, unsigned long long c) noexcept
{
  return __builtin_ia32_bextr_u64(v, (unsigned)c);
}

__inline_g unsigned int
_bzhi_u32(unsigned int v, unsigned int n) noexcept
{
  return __builtin_ia32_bzhi_si(v, n);
}

__inline_g unsigned long long
_bzhi_u64(unsigned long long v, unsigned int n) noexcept
{
  return __builtin_ia32_bzhi_di(v, n);
}

__inline_g unsigned int
_pext_u32(unsigned int v, unsigned int mask) noexcept
{
  return __builtin_ia32_pext_si(v, mask);
}

__inline_g unsigned long long
_pext_u64(unsigned long long v, unsigned long long mask) noexcept
{
  return __builtin_ia32_pext_di(v, mask);
}

__inline_g unsigned int
_pdep_u32(unsigned int v, unsigned int mask) noexcept
{
  return __builtin_ia32_pdep_si(v, mask);
}

__inline_g unsigned long long
_pdep_u64(unsigned long long v, unsigned long long mask) noexcept
{
  return __builtin_ia32_pdep_di(v, mask);
}

__inline_g unsigned int
_mulx_u32(unsigned int a, unsigned int b, unsigned int *hi) noexcept
{
  unsigned long long p = (unsigned long long)a * b;
  *hi = (unsigned int)(p >> 32);
  return (unsigned int)p;
}

__inline_g unsigned long long
_mulx_u64(unsigned long long a, unsigned long long b, unsigned long long *hi) noexcept
{
  unsigned __int128 p = (unsigned __int128)a * b;
  *hi = (unsigned long long)(p >> 64);
  return (unsigned long long)p;
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_tzcnt_u32);
__inject_i(_tzcnt_u64);
__inject_i(_lzcnt_u32);
__inject_i(_lzcnt_u64);
__inject_i(__tzcnt_u32);
__inject_i(__tzcnt_u64);
__inject_i(__lzcnt_u32);
__inject_i(__lzcnt_u64);
__inject_i(_andn_u32);
__inject_i(_andn_u64);
__inject_i(_blsr_u32);
__inject_i(_blsr_u64);
__inject_i(_blsi_u32);
__inject_i(_blsi_u64);
__inject_i(_blsmsk_u32);
__inject_i(_blsmsk_u64);
__inject_i(_bextr_u32);
__inject_i(_bextr_u64);
__inject_i(__bextr_u32);
__inject_i(__bextr_u64);
__inject_i(_bzhi_u32);
__inject_i(_bzhi_u64);
__inject_i(_pext_u32);
__inject_i(_pext_u64);
__inject_i(_pdep_u32);
__inject_i(_pdep_u64);
__inject_i(_mulx_u32);
__inject_i(_mulx_u64);
#undef __inject_i
#endif
