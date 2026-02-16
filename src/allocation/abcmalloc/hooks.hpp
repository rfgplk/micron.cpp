// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "../../math/generic.hpp"
#include "../../types.hpp"
#include "__sys.hpp"
#include "config.hpp"

namespace abc
{
// note u64 and size_t are the same type

inline size_t
__calculate_space_cache(size_t sz)
{
  // x^3 * ln(x * sqrt(x))
  u64 t = static_cast<u64>((float)(sz * sz * sz) * micron::math::logf128((double)sz * __builtin_sqrt((double)sz)));
  float t_2 = (float)t / 4096;
  t_2 = micron::math::ceil(t_2);
  sz = micron::math::nearest_pow2ll(((size_t)t_2) < __default_minimum_page_mul ? __default_minimum_page_mul : (size_t)t_2)
       * 4096;     // still align to pg
  return sz;
}

inline size_t
__calculate_space_small(size_t sz)
{
  // (old) x^2/log(x)
  // x^2 * ln(x)
  u64 t = static_cast<u64>((float)(sz * sz) * micron::math::logf128((float)sz));
  float t_2 = (float)t / 4096;
  t_2 = micron::math::ceil(t_2);
  sz = micron::math::nearest_pow2ll(((size_t)t_2) < __default_minimum_page_mul ? __default_minimum_page_mul : (size_t)t_2) * 4096;
  return sz;
}

inline size_t
__calculate_space_medium(size_t sz)
{
  // x * ln(x) * ln(x)
  flong f_sz = static_cast<flong>(sz);
  u64 t = static_cast<u64>(f_sz * micron::math::logf128(f_sz) * (micron::math::logf128(f_sz)));
  float t_2 = (float)t / 4096;
  t_2 = micron::math::ceil(t_2);
  sz = micron::math::nearest_pow2ll(((size_t)t_2) < __default_minimum_page_mul ? __default_minimum_page_mul : (size_t)t_2) * 4096;
  return sz;
}

inline size_t
__calculate_space_huge(size_t sz)
{
  // x * ln(x) * ln(ln(ln(x))
  flong f_sz = static_cast<flong>(sz);
  u64 t = static_cast<u64>(f_sz * micron::math::logf128(f_sz) * micron::math::logf128(micron::math::logf128(micron::math::logf128(f_sz))));
  float t_2 = (float)t / 4096;
  t_2 = micron::math::ceil(t_2);
  sz = micron::math::nearest_pow2ll(((size_t)t_2) < __default_minimum_page_mul ? __default_minimum_page_mul : (size_t)t_2) * 4096;
  return sz;
}
inline size_t
__calculate_space_bulk(size_t sz)
{
  // logarithmic taper
  long double factor = 1.0 + 0.1 * micron::math::logf128(static_cast<double>(sz) / (1024 * 1024 * 1024));
  if ( factor < 1.0 )
    factor = 1.0;     // never shrink

  size_t t = static_cast<size_t>(sz * factor);

  size_t pow2_sz = 1;
  while ( pow2_sz < t )
    pow2_sz <<= 1;
  return pow2_sz;
}
inline size_t
__calculate_space_saturated(size_t sz)
{
  // x * ln(x) * (ln(ln(x)))
  flong f_sz = static_cast<flong>(sz);
  u64 t = static_cast<u64>(f_sz * micron::math::logf128(f_sz) * (micron::math::logf128(micron::math::logf128(f_sz))));
  float t_2 = (float)t / 4096;
  t_2 = micron::math::ceil(t_2);
  sz = micron::math::nearest_pow2ll(((size_t)t_2) < __default_minimum_page_mul ? __default_minimum_page_mul : (size_t)t_2) * 4096;
  return sz;
}
void *
__get_kernel_memory(u64 sz)
{
  return micron::sys_allocator<byte>::alloc(sz);
}

template <typename T>
inline T
__get_kernel_chunk(u64 sz)
{
  return { micron::sys_allocator<byte>::alloc(sz), sz };
}

template <typename T>
inline void
__release_kernel_chunk(const T &mem)
{
  micron::sys_allocator<byte>::dealloc(mem.ptr, mem.len);
}
};
