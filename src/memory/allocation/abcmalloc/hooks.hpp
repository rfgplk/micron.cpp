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

#include "../../../math/__asm/hw.hpp"
#include "../../../math/generic.hpp"
#include "../../../types.hpp"
#include "__sys.hpp"
#include "config.hpp"
#include "va_reserve.hpp"

namespace abc
{
// NOTE: u64 and usize are the same on amd64 but NOT on 32-bit targets (armv7 has u64 defined but usize is __UINTPTR__ (32b))
#if defined(__micron_arch_width_32)
// ceiling on speculative growth
constexpr static const u64 __width32_sheet_cap = 64ULL << 20;
#endif

static inline usize
__saturate_pages_to_bytes(u64 pages) noexcept
{
  const u64 ps = static_cast<u64>(__system_pagesize);
  const u64 max_usize = static_cast<u64>(micron::numeric_limits<usize>::max());
#if defined(__micron_arch_width_32)
  if ( pages > __width32_sheet_cap / ps ) return static_cast<usize>(__width32_sheet_cap);
#endif
  if ( pages > max_usize / ps ) return static_cast<usize>(max_usize & ~(ps - 1));
  return static_cast<usize>(pages * ps);
}

// same contract as the pages variant above, for a count that is already in bytes: a byte count wider
// than usize can never be mapped on this target, so clamp rather than let the narrowing wrap
inline usize
__saturate_bytes(u64 bytes) noexcept
{
  const u64 max_usize = static_cast<u64>(micron::numeric_limits<usize>::max());
  return bytes > max_usize ? static_cast<usize>(max_usize) : static_cast<usize>(bytes);
}

#if defined(__micron_no_fp)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// integer sheet-sizing curves (no hardware FP)
//
// The curves below are the same shapes as the floating-point set that follows -- x^2*ln(x^1.5),
// x*sqrt(x)*400, x*ln(x)*150, 2x*ln(x)^2, sqrt(x)*ln(x)^2*125, x*(1+0.1*ln(x/1GiB)), x*ln(x)*ln(ln(x))
// -- evaluated in fixed point. They are heuristics whose result is ceil-divided by the page size and
// then rounded UP TO A POWER OF TWO, so a couple of percent of curve error is absorbed by the
// rounding and cannot change the sheet that gets chosen except very near a binade boundary.
//
// This exists because a Linux kernel module may not touch the FPU outside kernel_fpu_begin(), and
// most MCUs have no FPU at all. See __micron_no_fp in bits/__arch.hpp.

namespace __icurve
{

// floor(log2(x)), 0 for x == 0
[[gnu::always_inline]] inline constexpr u32
__ilog2(u64 x) noexcept
{
  return x ? (63u - static_cast<u32>(__builtin_clzll(x))) : 0u;
}

// ln(x) in Q10 (i.e. scaled by 1024). log2 is taken with a linear interpolation across the binade,
// which costs at most ~4% against the true curve near the middle of a binade and nothing at its
// ends; ln2 is 709/1024.
[[gnu::always_inline]] inline constexpr u64
__ln_q10(u64 x) noexcept
{
  if ( x < 2 ) return 0;
  const u32 e = __ilog2(x);
  // mantissa scaled into [1024, 2048), minus the implicit 1.0 -> the fractional bits of log2
  const u64 mant = (e >= 10) ? (x >> (e - 10)) : (x << (10 - e));
  const u64 log2_q10 = (static_cast<u64>(e) << 10) + (mant - 1024u);
  return (log2_q10 * 709u) >> 10;
}

// floor(sqrt(x)) by Newton, seeded from the bit length so it converges in a few steps
[[gnu::always_inline]] inline constexpr u64
__isqrt(u64 x) noexcept
{
  if ( x < 2 ) return x;
  u64 r = u64{ 1 } << ((__ilog2(x) >> 1) + 1);
  for ( ;; ) {
    const u64 n = (r + x / r) >> 1;
    if ( n >= r ) break;
    r = n;
  }
  return r;
}

// sqrt(x) in Q8. Truncation in a plain integer sqrt is worth up to 29% at x = 8 (isqrt(8) is 2, not
// 2.83); scaling by 2^16 before the root and taking 2^8 back out keeps the error under half a
// percent across the whole domain. Safe for x below 2^48, which is far past any sheet request.
[[gnu::always_inline]] inline constexpr u64
__isqrt_q8(u64 x) noexcept
{
  return __isqrt(x << 16);
}

// the shared tail: ceil to pages, floor at the minimum, round up to a power of two, saturate.
// integer ceil-division, where the FP path goes through `(float)t / pagesize` and loses precision
// above 2^24 -- so this tail is the more accurate of the two, not a degraded one.
[[gnu::always_inline]] inline usize
__tail(u64 t) noexcept
{
  const u64 ps = static_cast<u64>(__system_pagesize);
  u64 pages = ps ? ((t + ps - 1) / ps) : t;
  if ( pages < __default_minimum_page_mul ) pages = __default_minimum_page_mul;
  pages = micron::math::nearest_pow2ll(pages);
  return __saturate_pages_to_bytes(pages);
}

};      // namespace __icurve

inline usize
__calculate_space_cache(usize sz)
{
  // x^2 * ln(x * sqrt(x))  ==  x^2 * 1.5 * ln(x)
  const u64 s = static_cast<u64>(sz);
  const u64 t = ((s * s) * __icurve::__ln_q10(s) * 3u) >> 11;      // /1024 for Q10, /2 for the 1.5
  return __icurve::__tail(t);
}

inline usize
__calculate_space_small(usize sz)
{
  // x * sqrt(x) * 400
  const u64 s = static_cast<u64>(sz);
  return __icurve::__tail((s * __icurve::__isqrt_q8(s) * 400u) >> 8);
}

inline usize
__calculate_space_medium(usize sz)
{
  // x * ln(x) * 150
  const u64 s = static_cast<u64>(sz);
  return __icurve::__tail((s * __icurve::__ln_q10(s) * 150u) >> 10);
}

inline usize
__calculate_space_large(usize sz)
{
  // 2 * x * ln(x)^2
  const u64 s = static_cast<u64>(sz);
  const u64 lg = __icurve::__ln_q10(s);
  return __icurve::__tail((2u * s * lg * lg) >> 20);
}

inline usize
__calculate_space_huge(usize sz)
{
  // sqrt(x) * ln(x)^2 * 125
  const u64 s = static_cast<u64>(sz);
  const u64 lg = __icurve::__ln_q10(s);
  return __icurve::__tail((__icurve::__isqrt_q8(s) * lg * lg * 125u) >> 28);
}

inline usize
__calculate_space_bulk(usize sz)
{
  // x * (1 + 0.1 * ln(x / 1 GiB)), never shrinking
  constexpr u64 gib = u64{ 1024 } * 1024 * 1024;
  const u64 s = static_cast<u64>(sz);
  // ln of a ratio below 1 is negative, and the FP form floors the factor at 1.0; mirror that by
  // taking the growth term only once the size is past a gibibyte
  u64 t = s;
  if ( s > gib ) {
    const u64 lg = __icurve::__ln_q10(s / gib);      // Q10
    t = s + ((s * lg) / (10u << 10));                // + x * 0.1 * ln(ratio)
  }

  usize pow2_sz = 1;
  while ( pow2_sz < t ) {
    if ( pow2_sz > (micron::numeric_limits<usize>::max() >> 1) ) break;
    pow2_sz <<= 1;
  }
#if defined(__micron_arch_width_32)
  if ( pow2_sz > static_cast<usize>(__width32_sheet_cap) ) pow2_sz = static_cast<usize>(__width32_sheet_cap);
#endif
  return pow2_sz;
}

inline usize
__calculate_space_saturated(usize sz)
{
  // x * ln(x) * ln(ln(x))
  const u64 s = static_cast<u64>(sz);
  const u64 lg = __icurve::__ln_q10(s);                       // Q10
  const u64 lglg = __icurve::__ln_q10((lg + 512u) >> 10);      // ln(ln(x)), Q10, on the rounded inner value
  const u64 t = (s * lg * lglg) >> 20;
  // WARNING: the FP form applies nearest_pow2ll to the PAGE COUNT it computed via float, then feeds
  // that to __saturate_pages_to_bytes -- it never re-divides. __tail does the same thing.
  return __icurve::__tail(t);
}

#else

inline usize
__calculate_space_cache(usize sz)
{
  // x^2 * ln(x * sqrt(x))
  // WARNING: square in double; usize sz*sz wraps at sz >= 2^16 on width-32
  u64 t = static_cast<u64>(static_cast<flong>((double)sz * (double)sz)
                           * micron::math::logf128(static_cast<flong>((double)sz * micron::math::hw::sqrt_sd((double)sz))));
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
  u64 t = static_cast<u64>(static_cast<double>(sz) * micron::math::hw::sqrt_sd(static_cast<double>(sz)) * 400.0);
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
  u64 t = static_cast<u64>(f_sz * micron::math::logf128(f_sz) * 150.0L);
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
  u64 t = static_cast<u64>(2.0L * f_sz * lg * lg);
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
  u64 t = static_cast<u64>(micron::math::hw::sqrt_sd(f_sz) * lg * lg * 125.0);
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
  long double factor = 1.0L + 0.1L * micron::math::logf128(static_cast<flong>(static_cast<double>(sz) / (1024 * 1024 * 1024)));
  if ( factor < 1.0L ) factor = 1.0L;      // never shrink

  usize t = static_cast<usize>(sz * factor);

  usize pow2_sz = 1;
  while ( pow2_sz < t ) {
    if ( pow2_sz > (micron::numeric_limits<usize>::max() >> 1) ) break;      // next shift would wrap to 0 (32-bit)
    pow2_sz <<= 1;
  }
#if defined(__micron_arch_width_32)
  if ( pow2_sz > static_cast<usize>(__width32_sheet_cap) ) pow2_sz = static_cast<usize>(__width32_sheet_cap);
#endif
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
  // route through the saturating pages->bytes conversion: the raw multiply wraps on width-32
  sz = __saturate_pages_to_bytes(
      micron::math::nearest_pow2ll(((usize)t_2) < __default_minimum_page_mul ? __default_minimum_page_mul : (usize)t_2));
  return sz;
}

#endif      // __micron_no_fp

void *
__get_kernel_memory(u64 sz)
{
  return micron::sys_allocator<byte>::alloc(static_cast<usize>(sz));
}

template<typename T>
inline T
__get_kernel_chunk(u64 sz)
{
  if ( auto *p = __va_carve(static_cast<usize>(sz)); p ) [[likely]] {
    const usize rounded = (static_cast<usize>(sz) + __sheet_align_mask) & ~__sheet_align_mask;
    return { reinterpret_cast<byte *>(p), rounded };
  }
  return { micron::sys_allocator<byte>::alloc(static_cast<usize>(sz)), static_cast<usize>(sz) };
}

template<typename T>
inline void
__release_kernel_chunk(const T &mem)
{
  if ( __va_contains(mem.ptr) ) {
    __va_release(micron::ptr_cast<addr_t *>(mem.ptr), mem.len);
    return;
  }
  micron::sys_allocator<byte>::dealloc(mem.ptr, mem.len);
}
};      // namespace abc
