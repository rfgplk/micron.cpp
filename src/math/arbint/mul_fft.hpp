//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "mul_basecase.hpp"
#include "mul_nussbaumer.hpp"

#if defined(__micron_arbint_fft)
#include "../../memory/placement_new.hpp"
#include "../fft/kernels.hpp"
#endif

namespace micron::math::mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// binary64 convolution
// 2*q + log2(N) <= 35 leaves 18 mantissa bits for transform error
// re-anchor twiddle recurrences every 32 coefficients
// Larger products use overlap-add; no floating coefficient grows with the
// number of tiles; all sizes below are in limbs, including scratch

struct __fft_shape {
  usize an, bn;
  __fft_plan plan;
};

[[nodiscard]] inline constexpr __fft_shape
__fft_pick_shape(usize an, usize bn) noexcept
{
  if ( !an || !bn ) return {};
  usize a = an, b = bn;
  if ( an / bn >= 8u ) a = __nuss_size_mul(3u, bn);
  if ( bn / an >= 8u ) b = __nuss_size_mul(3u, an);
  const auto whole = __fft_make_plan(a, b);
  if ( whole.valid ) return { a, b, whole };
  a = a < __fft_block_limbs ? a : __fft_block_limbs;
  b = b < __fft_block_limbs ? b : __fft_block_limbs;
  return { a, b, __fft_make_plan(a, b) };
}

[[nodiscard]] inline constexpr usize
fft_itch(usize an, usize bn) noexcept
{
  if ( !an || !bn ) return 0u;
  if ( __nuss_size_add(an, bn) == __nuss_size_max ) return __nuss_size_max;
  const usize exact = nussbaumer_itch(an, bn);
#if defined(__micron_arbint_fft)
  const auto s = __fft_pick_shape(an, bn);
  // three half-length arrays, complex/real twiddles, alignment slack, and a tile
  const usize work = __nuss_size_add(__nuss_size_mul(s.plan.n / 4u, 144u / limb_bytes), __nuss_size_add(s.an + s.bn, 24u / limb_bytes));
  return work > exact ? work : exact;
#else
  return exact;
#endif
}

#if defined(__micron_arbint_fft)

using __fft_complex = micron::math::complex<double>;

inline void
__fft_roots(__fft_complex *tw, usize count, usize j, usize n) noexcept
{
  const auto step = fft::__impl::__root<double>(j, n);
  __fft_complex powers[20];
  for ( usize bit = 5u; (usize{ 1 } << bit) < count; ++bit ) powers[bit - 5u] = fft::__impl::__root<double>(j * (usize{ 1 } << bit), n);
  for ( usize k = 0u; k < count; k += 32u ) {
    // each anchor uses at most log2(N) independently evaluated roots;
    // recurrence error cannot accumulate across the whole table
    __fft_complex w{ 1.0, 0.0 };
    for ( usize bits = k >> 5u, bit = 0u; bits; bits >>= 1u, ++bit )
      if ( bits & 1u ) w *= powers[bit];
    const usize end = k + 32u < count ? k + 32u : count;
    for ( usize i = k; i < end; ++i ) {
      tw[i] = w;
      w *= step;
    }
  }
}

inline void
__fft_twiddles(__fft_complex *tw, usize n) noexcept
{
  while ( n > 8u ) {
    const usize m = n / 4u;
    for ( usize j = 1u; j <= 3u; ++j ) __fft_roots(tw + (j - 1u) * m, m, j, n);
    tw += 3u * m;
    n = m;
  }
}

template<bool Inverse>
inline void
__fft_transform(const __fft_complex *in, __fft_complex *out, const __fft_complex *tw, usize n, usize stride = 1u) noexcept
{
  if ( n == 2u ) return fft::__impl::__leaf<2, Inverse>(in, out, stride);
  if ( n == 4u ) return fft::__impl::__leaf<4, Inverse>(in, out, stride);
  if ( n == 8u ) return fft::__impl::__leaf<8, Inverse>(in, out, stride);
  const usize m = n / 4u;
  for ( usize j = 0u; j < 4u; ++j ) __fft_transform<Inverse>(in + j * stride, out + j * m, tw + 3u * m, m, stride * 4u);
  fft::__impl::__stage<4, Inverse>(out, tw, m, 0u, m);
}

template<u64 P>
[[nodiscard]] inline u64
__fft_residue(const limb_t *p, usize n) noexcept
{
  u64 r = 0u;
  for ( usize i = n; i-- > 0u; ) {
    if constexpr ( limb_bits == 64u ) r = ((r << 32u) | u32(u64(p[i]) >> 32u)) % P;
    r = ((r << 32u) | u32(p[i])) % P;
  }
  return r;
}

template<u64 P>
[[nodiscard]] inline bool
__fft_check(const limb_t *r, const limb_t *a, usize an, const limb_t *b, usize bn) noexcept
{
  return __fft_residue<P>(r, an + bn) == (__fft_residue<P>(a, an) * __fft_residue<P>(b, bn)) % P;
}

// pack even/odd real samples into an N/2 complex transform
// slot zero stores the independent real DC and Nyquist bins in its real and imaginary fields
template<bool Inverse>
inline void
__fft_real_pass(__fft_complex *b, const __fft_complex *roots, usize h) noexcept
{
  const auto z = b[0];
  b[0] = { z.real + z.imag, z.real - z.imag };
  if constexpr ( Inverse ) b[0] *= 0.5;
  for ( usize k = 1u; k < h / 2u; ++k ) {
    const auto u = b[k], v = conj(b[h - k]);
    const auto sum = (u + v) * 0.5;
    auto w = roots[k];
    if constexpr ( Inverse ) w = conj(w);
    const auto diff = (u - v) * w * 0.5;
    const __fft_complex rotated = Inverse ? __fft_complex{ -diff.imag, diff.real } : __fft_complex{ diff.imag, -diff.real };
    b[k] = sum + rotated;
    b[h - k] = conj(sum - rotated);
  }
  b[h / 2u] = conj(b[h / 2u]);
}

inline void
__fft_load(__fft_complex *a, usize h, const limb_t *ap, usize an, usize q) noexcept
{
  const usize digits = __nuss_ceil_div(an * limb_bits, q);
  for ( usize k = 0u; k < h; ++k )
    a[k] = { 2u * k < digits ? double(__nuss_digit(ap, an, 2u * k, q)) : 0.0,
             2u * k + 1u < digits ? double(__nuss_digit(ap, an, 2u * k + 1u, q)) : 0.0 };
}

template<bool Square>
[[nodiscard]] inline bool
__fft_product(limb_t *rp, const limb_t *ap, usize an, const limb_t *bp, usize bn, const __fft_plan &p, __fft_complex *a, __fft_complex *b,
              __fft_complex *c, const __fft_complex *tw, const __fft_complex *roots) noexcept
{
  const usize ad = __nuss_ceil_div(an * limb_bits, p.digit_bits), bd = __nuss_ceil_div(bn * limb_bits, p.digit_bits);
  const usize h = p.n / 2u;
  __fft_load(a, h, ap, an, p.digit_bits);
  __fft_transform<false>(a, b, tw, h);
  __fft_real_pass<false>(b, roots, h);
  if constexpr ( Square ) {
    b[0] = { b[0].real * b[0].real, b[0].imag * b[0].imag };
    fft::__impl::__multiply(b + 1u, b + 1u, h - 1u);
  } else {
    __fft_load(a, h, bp, bn, p.digit_bits);
    __fft_transform<false>(a, c, tw, h);
    __fft_real_pass<false>(c, roots, h);
    b[0] = { b[0].real * c[0].real, b[0].imag * c[0].imag };
    fft::__impl::__multiply(b + 1u, c + 1u, h - 1u);
  }
  __fft_real_pass<true>(b, roots, h);
  __fft_transform<true>(b, a, tw, h);
  const double scale = 1.0 / double(h);
  const u64 mask = (u64{ 1 } << p.digit_bits) - 1u;
  const double bound = double(ad < bd ? ad : bd) * double(mask * mask);
  __nuss_packer pack{ rp, an + bn, 0u, 0u, 0u, true };
  u64 carry = 0u;
  for ( usize k = 0u; k < ad + bd - 1u; ++k ) {
    const double v = ((k & 1u) ? a[k / 2u].imag : a[k / 2u].real) * scale;
    if ( !(v >= -0.125 && v <= bound + 0.125) ) return false;
    const u64 nearest = static_cast<u64>(v + 0.5);
    const double error = v - double(nearest);
    if ( error < -0.125 || error > 0.125 ) return false;
    const u64 word = nearest + carry;
    pack.push(static_cast<limb_t>(word & mask), p.digit_bits);
    carry = word >> p.digit_bits;
  }
  while ( carry ) {
    pack.push(static_cast<limb_t>(carry & mask), p.digit_bits);
    carry >>= p.digit_bits;
  }
  pack.finish();
  // consistency guards supplement the bounded rounding error
  return pack.exact && __fft_check<4294967291ull>(rp, ap, an, bp, bn) && __fft_check<4294967279ull>(rp, ap, an, bp, bn);
}

template<bool Square>
[[nodiscard]] inline bool
__fft_run(limb_t *rp, const limb_t *ap, usize an, const limb_t *bp, usize bn, limb_t *scratch) noexcept
{
  const auto s = __fft_pick_shape(an, bn);
  limb_t *aligned = scratch;
  while ( reinterpret_cast<usize>(aligned) % 8u ) ++aligned;
  const usize h = s.plan.n / 2u, count = 9u * s.plan.n / 4u + 1u;
  auto *a = ::new (static_cast<void *>(aligned)) __fft_complex[count];
  auto *b = a + h, *c = b + h, *tw = c + h, *roots = tw + h;
  limb_t *part = aligned + count * sizeof(__fft_complex) / limb_bytes;
  __fft_twiddles(tw, h);
  __fft_roots(roots, h / 2u + 1u, 1u, s.plan.n);
  usize prepared = s.plan.n;
  bool ok = true;
  if ( s.an == an && s.bn == bn ) {
    ok = __fft_product<Square>(rp, ap, an, bp, bn, s.plan, a, b, c, tw, roots);
  } else {
    zero(rp, an + bn);
    for ( usize ai = 0u; ai < an && ok; ai += s.an ) {
      const usize atake = an - ai < s.an ? an - ai : s.an;
      for ( usize bi = 0u; bi < bn && ok; bi += s.bn ) {
        if constexpr ( Square ) {
          if ( bi > ai ) break;
        }
        const usize btake = bn - bi < s.bn ? bn - bi : s.bn;
        if ( atake <= 2u || btake <= 2u ) {
          if ( atake >= btake )
            mul_basecase(part, ap + ai, atake, bp + bi, btake);
          else
            mul_basecase(part, bp + bi, btake, ap + ai, atake);
        } else {
          const auto p = __fft_make_plan(atake, btake);
          if ( p.n != prepared ) {
            __fft_twiddles(tw, p.n / 2u);
            __fft_roots(roots, p.n / 4u + 1u, 1u, p.n);
            prepared = p.n;
          }
          if ( Square && ai == bi )
            ok = __fft_product<true>(part, ap + ai, atake, bp + bi, btake, p, a, b, c, tw, roots);
          else
            ok = __fft_product<false>(part, ap + ai, atake, bp + bi, btake, p, a, b, c, tw, roots);
        }
        if ( !ok ) break;
        __nuss_add_partial(rp, an + bn, ai + bi, part, atake + btake);
        if ( Square && ai != bi ) __nuss_add_partial(rp, an + bn, ai + bi, part, atake + btake);
      }
    }
  }
  // restore limb lifetimes before another mpn operation reuses this scratch
  (void)::new (static_cast<void *>(aligned)) limb_t[count * sizeof(__fft_complex) / limb_bytes];
  return ok;
}
#endif

inline constexpr void
mul_fft(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn, limb_t *scratch) noexcept
{
  if ( !an || !bn ) return zero(rp, an + bn);
#if defined(__micron_arbint_fft)
  if !consteval {
    if ( __fft_run<false>(rp, ap, an, bp, bn, scratch) ) return;
  }
#endif
  mul_nussbaumer(rp, ap, an, bp, bn, scratch);
}

inline constexpr void
sqr_fft(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch) noexcept
{
  if ( !n ) return;
#if defined(__micron_arbint_fft)
  if !consteval {
    if ( __fft_run<true>(rp, ap, n, ap, n, scratch) ) return;
  }
#endif
  sqr_nussbaumer(rp, ap, n, scratch);
}
};      // namespace micron::math::mpn
