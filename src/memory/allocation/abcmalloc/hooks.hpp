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

#include "../../../math/generic.hpp"
#include "../../../types.hpp"
#include "__sys.hpp"
#include "config.hpp"
#include "va_reserve.hpp"

namespace abc
{
// NOTE: u64 and usize are the same on amd64 but NOT on 32-bit targets (armv7 has u64 defined but usize is __UINTPTR__ (32b))
static inline usize
__saturate_pages_to_bytes(u64 pages) noexcept
{
  const u64 ps = static_cast<u64>(__system_pagesize);
  const u64 max_usize = static_cast<u64>(micron::numeric_limits<usize>::max());
  if ( pages > max_usize / ps ) return static_cast<usize>(max_usize & ~(ps - 1));
  return static_cast<usize>(pages * ps);
}

inline usize
__calculate_space_cache(usize sz)
{
  // x^2 * ln(x * sqrt(x))
  u64 t = static_cast<u64>((float)(sz * sz) * micron::math::logf128((double)sz * __builtin_sqrt((double)sz)));
  float t_2 = (float)t / __system_pagesize;
  t_2 = micron::math::ceil(t_2);
  u64 pages = static_cast<u64>(t_2);
  if ( pages < __default_minimum_page_mul ) pages = __default_minimum_page_mul;
  pages = micron::math::nearest_pow2ll(pages);
  return __saturate_pages_to_bytes(pages);
}

inline usize
__calculate_space_small(usize sz)
{
  // new equation: x * sqrt(x) * 400 smooth sublinear-in-class growth; 4 MiB at sz=513, ~128 MiB at sz=4095
  u64 t = static_cast<u64>(static_cast<double>(sz) * __builtin_sqrt(static_cast<double>(sz)) * 400.0);
  float t_2 = (float)t / __system_pagesize;
  t_2 = micron::math::ceil(t_2);
  u64 pages = static_cast<u64>(t_2);
  if ( pages < __default_minimum_page_mul ) pages = __default_minimum_page_mul;
  pages = micron::math::nearest_pow2ll(pages);
  return __saturate_pages_to_bytes(pages);
}

inline usize
__calculate_space_medium(usize sz)
{
  // new equation: x * ln(x) * 150;  4 MiB at sz=4096, ~64 MiB at sz=32768; hot tier so heavy floor
  flong f_sz = static_cast<flong>(sz);
  u64 t = static_cast<u64>(f_sz * micron::math::logf128(f_sz) * 150.0);
  float t_2 = (float)t / __system_pagesize;
  t_2 = micron::math::ceil(t_2);
  u64 pages = static_cast<u64>(t_2);
  if ( pages < __default_minimum_page_mul ) pages = __default_minimum_page_mul;
  pages = micron::math::nearest_pow2ll(pages);
  return __saturate_pages_to_bytes(pages);
}

inline usize
__calculate_space_large(usize sz)
{
  // 2 * x * ln(x)^2; the old medium equation scaled by a factor of 2
  flong f_sz = static_cast<flong>(sz);
  flong lg = micron::math::logf128(f_sz);
  u64 t = static_cast<u64>(2.0 * f_sz * lg * lg);
  float t_2 = (float)t / __system_pagesize;
  t_2 = micron::math::ceil(t_2);
  u64 pages = static_cast<u64>(t_2);
  if ( pages < __default_minimum_page_mul ) pages = __default_minimum_page_mul;
  pages = micron::math::nearest_pow2ll(pages);
  return __saturate_pages_to_bytes(pages);
}

inline usize
__calculate_space_huge(usize sz)
{
  // sqrt(x) * ln(x)^2 * 125; aggressive at the floor, tapers at the top
  double f_sz = static_cast<double>(sz);
  double lg = static_cast<double>(micron::math::logf128(static_cast<flong>(sz)));
  u64 t = static_cast<u64>(__builtin_sqrt(f_sz) * lg * lg * 125.0);
  float t_2 = (float)t / __system_pagesize;
  t_2 = micron::math::ceil(t_2);
  u64 pages = static_cast<u64>(t_2);
  if ( pages < __default_minimum_page_mul ) pages = __default_minimum_page_mul;
  pages = micron::math::nearest_pow2ll(pages);
  return __saturate_pages_to_bytes(pages);
}

inline usize
__calculate_space_bulk(usize sz)
{
  // logarithmic taper
  long double factor = 1.0 + 0.1 * micron::math::logf128(static_cast<double>(sz) / (1024 * 1024 * 1024));
  if ( factor < 1.0 ) factor = 1.0;      // never shrink

  usize t = static_cast<usize>(sz * factor);

  usize pow2_sz = 1;
  while ( pow2_sz < t ) pow2_sz <<= 1;
  return pow2_sz;
}

inline usize
__calculate_space_saturated(usize sz)
{
  // x * ln(x) * (ln(ln(x)))
  flong f_sz = static_cast<flong>(sz);
  u64 t = static_cast<u64>(f_sz * micron::math::logf128(f_sz) * (micron::math::logf128(micron::math::logf128(f_sz))));
  float t_2 = (float)t / __system_pagesize;
  t_2 = micron::math::ceil(t_2);
  sz = micron::math::nearest_pow2ll(((usize)t_2) < __default_minimum_page_mul ? __default_minimum_page_mul : (usize)t_2)
       * __system_pagesize;
  return sz;
}

void *
__get_kernel_memory(u64 sz)
{
  return micron::sys_allocator<byte>::alloc(sz);
}

template<typename T>
inline T
__get_kernel_chunk(u64 sz)
{
  if ( auto *p = __va_carve(static_cast<usize>(sz)); p ) [[likely]] {
    const usize rounded = (static_cast<usize>(sz) + __sheet_align_mask) & ~__sheet_align_mask;
    return { reinterpret_cast<byte *>(p), rounded };
  }
  return { micron::sys_allocator<byte>::alloc(sz), static_cast<usize>(sz) };
}

template<typename T>
inline void
__release_kernel_chunk(const T &mem)
{
  if ( __va_contains(mem.ptr) ) {
    __va_release(reinterpret_cast<addr_t *>(mem.ptr), mem.len);
    return;
  }
  micron::sys_allocator<byte>::dealloc(mem.ptr, mem.len);
}
};      // namespace abc
