//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_x86_any)
#error "bmi.hpp included on a non-x86 build"
#endif

namespace micron
{
namespace simd
{
namespace bmi
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

__inline_g unsigned int
trailing_zeros_u32(unsigned int v) noexcept
{
  return _tzcnt_u32(v);
}

__inline_g unsigned long long
trailing_zeros_u64(unsigned long long v) noexcept
{
  return _tzcnt_u64(v);
}

__inline_g unsigned int
leading_zeros_u32(unsigned int v) noexcept
{
  return _lzcnt_u32(v);
}

__inline_g unsigned long long
leading_zeros_u64(unsigned long long v) noexcept
{
  return _lzcnt_u64(v);
}

__inline_g unsigned int
andn_u32(unsigned int a, unsigned int b) noexcept
{
  return _andn_u32(a, b);
}

__inline_g unsigned long long
andn_u64(unsigned long long a, unsigned long long b) noexcept
{
  return _andn_u64(a, b);
}

__inline_g unsigned int
reset_lowest_set_u32(unsigned int v) noexcept
{
  return _blsr_u32(v);
}

__inline_g unsigned long long
reset_lowest_set_u64(unsigned long long v) noexcept
{
  return _blsr_u64(v);
}

__inline_g unsigned int
isolate_lowest_set_u32(unsigned int v) noexcept
{
  return _blsi_u32(v);
}

__inline_g unsigned long long
isolate_lowest_set_u64(unsigned long long v) noexcept
{
  return _blsi_u64(v);
}

__inline_g unsigned int
mask_below_lowest_set_u32(unsigned int v) noexcept
{
  return _blsmsk_u32(v);
}

__inline_g unsigned long long
mask_below_lowest_set_u64(unsigned long long v) noexcept
{
  return _blsmsk_u64(v);
}

__inline_g unsigned int
extract_bits_u32(unsigned int v, unsigned int s, unsigned int l) noexcept
{
  return _bextr_u32(v, s, l);
}

__inline_g unsigned long long
extract_bits_u64(unsigned long long v, unsigned int s, unsigned int l) noexcept
{
  return _bextr_u64(v, s, l);
}

__inline_g unsigned int
zero_high_above_u32(unsigned int v, unsigned int n) noexcept
{
  return _bzhi_u32(v, n);
}

__inline_g unsigned long long
zero_high_above_u64(unsigned long long v, unsigned int n) noexcept
{
  return _bzhi_u64(v, n);
}

__inline_g unsigned int
parallel_extract_u32(unsigned int v, unsigned int m) noexcept
{
  return _pext_u32(v, m);
}

__inline_g unsigned long long
parallel_extract_u64(unsigned long long v, unsigned long long m) noexcept
{
  return _pext_u64(v, m);
}

__inline_g unsigned int
parallel_deposit_u32(unsigned int v, unsigned int m) noexcept
{
  return _pdep_u32(v, m);
}

__inline_g unsigned long long
parallel_deposit_u64(unsigned long long v, unsigned long long m) noexcept
{
  return _pdep_u64(v, m);
}

__inline_g unsigned int
widen_mul_u32(unsigned int a, unsigned int b, unsigned int *hi) noexcept
{
  return _mulx_u32(a, b, hi);
}

__inline_g unsigned long long
widen_mul_u64(unsigned long long a, unsigned long long b, unsigned long long *hi) noexcept
{
  return _mulx_u64(a, b, hi);
}

__inline_g int
popcount_u32(unsigned int v) noexcept
{
  return _mm_popcnt_u32(v);
}

__inline_g int
popcount_u64(unsigned long long v) noexcept
{
  return _mm_popcnt_u64(v);
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace bmi
};     // namespace simd
};     // namespace micron
