//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Nussbaumer multiplication code
//
// exact polynomial transforms in Z[X]/(X^n + 1)

namespace micron
{
namespace math
{
namespace mpn
{

inline constexpr usize __nuss_size_max = static_cast<usize>(~static_cast<usize>(0));
inline constexpr usize __nuss_leaf_n = usize{ 1 } << threshold::nussbaumer_leaf_log;
inline constexpr usize __nuss_max_n = usize{ 1 } << threshold::nussbaumer_max_log;

// checked sizes and transform plan
[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_size_add(usize a, usize b) noexcept
{
  return b > __nuss_size_max - a ? __nuss_size_max : a + b;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_size_mul(usize a, usize b) noexcept
{
  return a != 0u && b > __nuss_size_max / a ? __nuss_size_max : a * b;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_ceil_div(usize a, usize b) noexcept
{
  return a / b + static_cast<usize>((a % b) != 0u);
}

struct __nuss_plan {
  usize digit_bits;
  usize log_n;
  usize n;
  usize a_digits;
  usize b_digits;
  bool valid;
};

[[nodiscard, gnu::flatten]] inline constexpr __nuss_plan
__nuss_make_plan(usize an, usize bn) noexcept
{
  if ( an == 0u || bn == 0u ) return { 0u, 0u, 0u, 0u, 0u, false };
  const usize abits = __nuss_size_mul(an, limb_bits);
  const usize bbits = __nuss_size_mul(bn, limb_bits);
  if ( abits == __nuss_size_max || bbits == __nuss_size_max ) return { 0u, 0u, 0u, 0u, 0u, false };

  for ( usize k = 0u; k <= threshold::nussbaumer_max_log; ++k ) {
    const usize q = limb_bits - 2u - k;
    const usize ad = __nuss_ceil_div(abits, q);
    const usize bd = __nuss_ceil_div(bbits, q);
    const usize need = __nuss_size_add(ad, bd);
    const usize n = usize{ 1 } << k;
    if ( need != __nuss_size_max && need - 1u <= n ) return { q, k, n, ad, bd, true };
  }
  return { 0u, 0u, 0u, 0u, 0u, false };
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_log2(usize n) noexcept
{
  usize k = 0u;
  while ( usize{ 1 } << k != n ) ++k;
  return k;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_inner_n(usize log_n) noexcept
{
  return usize{ 1 } << (log_n - log_n / 2u);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// signed two-limb coefficients

struct __nuss_coeff {
  limb_t lo;
  limb_t hi;
};

struct __nuss_coeffs {
  limb_t *lo;
  limb_t *hi;
  usize n;
};

struct __nuss_const_coeffs {
  const limb_t *lo;
  const limb_t *hi;
  usize n;
};

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeffs
__nuss_coeff_span(limb_t *p, usize n) noexcept
{
  return { p, p + n, n };
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_const_coeffs
__nuss_coeff_span(const limb_t *p, usize n) noexcept
{
  return { p, p + n, n };
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_const_coeffs
__nuss_as_const(__nuss_coeffs p) noexcept
{
  return { p.lo, p.hi, p.n };
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeffs
__nuss_coeff_subspan(__nuss_coeffs p, usize off, usize n) noexcept
{
  return { p.lo + off, p.hi + off, n };
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_const_coeffs
__nuss_coeff_subspan(__nuss_const_coeffs p, usize off, usize n) noexcept
{
  return { p.lo + off, p.hi + off, n };
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_load(__nuss_const_coeffs p, usize i) noexcept
{
  return { p.lo[i], p.hi[i] };
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_load(__nuss_coeffs p, usize i) noexcept
{
  return { p.lo[i], p.hi[i] };
}

[[gnu::always_inline]] inline constexpr void
__nuss_coeff_store(__nuss_coeffs p, usize i, __nuss_coeff v) noexcept
{
  p.lo[i] = v.lo;
  p.hi[i] = v.hi;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
__nuss_coeff_is_zero(__nuss_coeff v) noexcept
{
  return v.lo == 0u && v.hi == 0u;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
__nuss_coeff_negative(__nuss_coeff v) noexcept
{
  return (v.hi & limb_msb) != 0u;
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_add(__nuss_coeff a, __nuss_coeff b) noexcept
{
  __nuss_coeff r{};
  const limb_t cy = addc(a.lo, b.lo, 0u, r.lo);
  (void)addc(a.hi, b.hi, cy, r.hi);
  return r;
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_sub(__nuss_coeff a, __nuss_coeff b) noexcept
{
  __nuss_coeff r{};
  const limb_t bw = subb(a.lo, b.lo, 0u, r.lo);
  (void)subb(a.hi, b.hi, bw, r.hi);
  return r;
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_cneg(__nuss_coeff a, bool negate) noexcept
{
  const limb_t bit = static_cast<limb_t>(negate);
  const limb_t mask = static_cast<limb_t>(0u - bit);
  __nuss_coeff r{ static_cast<limb_t>(a.lo ^ mask), static_cast<limb_t>(a.hi ^ mask) };
  const limb_t cy = addc(r.lo, bit, 0u, r.lo);
  (void)addc(r.hi, 0u, cy, r.hi);
  return r;
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_neg(__nuss_coeff a) noexcept
{
  return __nuss_coeff_cneg(a, true);
}

[[nodiscard, gnu::always_inline]] inline constexpr dlimb_t
__nuss_coeff_mul_bits(__nuss_coeff a, __nuss_coeff b) noexcept
{
  // The plan reserves every transform-growth bit, so leaf operands are signed one-limb values
  // whose high limb is only sign extension.  A native signed widening multiply is therefore exact.
#if defined(__micron_arch_width_64)
  const int128_t product = static_cast<int128_t>(static_cast<slimb_t>(a.lo)) * static_cast<int128_t>(static_cast<slimb_t>(b.lo));
#else
  const i64 product = static_cast<i64>(static_cast<slimb_t>(a.lo)) * static_cast<i64>(static_cast<slimb_t>(b.lo));
#endif
  return static_cast<dlimb_t>(product);
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_mul(__nuss_coeff a, __nuss_coeff b) noexcept
{
  const dlimb_t bits = __nuss_coeff_mul_bits(a, b);
  return { lo_half(bits), hi_half(bits) };
}

[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_coeff_shr(__nuss_coeff a, usize shift) noexcept
{
  if ( shift == 0u ) return a;
  return { static_cast<limb_t>((a.lo >> shift) | (a.hi << (limb_bits - shift))), static_cast<limb_t>(a.hi >> shift) };
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
__nuss_coeff_divexact_pow2(__nuss_coeff &a, usize shift) noexcept
{
  if ( shift == 0u ) return true;
  const limb_t mask = static_cast<limb_t>((limb_t{ 1 } << shift) - 1u);
  const bool exact = (a.lo & mask) == 0u;
  const limb_t sign = static_cast<limb_t>(a.hi >> (limb_bits - 1u));
  const limb_t sign_mask = static_cast<limb_t>(0u - sign);
  a.lo = static_cast<limb_t>((a.lo >> shift) | (a.hi << (limb_bits - shift)));
  a.hi = static_cast<limb_t>((a.hi >> shift) | (sign_mask << (limb_bits - shift)));
  return exact;
}

[[gnu::always_inline]] inline constexpr void
__nuss_zero_coeffs(__nuss_coeffs p) noexcept
{
  zero(p.lo, p.n);
  zero(p.hi, p.n);
}

[[gnu::always_inline]] inline constexpr void
__nuss_copy_coeffs(__nuss_coeffs rp, __nuss_const_coeffs ap) noexcept
{
  copyi(rp.lo, ap.lo, rp.n);
  copyi(rp.hi, ap.hi, rp.n);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// extensionring operations

inline constexpr void
__nuss_copy_cneg(__nuss_coeffs rp, __nuss_const_coeffs ap, bool negate) noexcept
{
#if defined(__micron_arbint_kern_coefficients)
  if ( !__builtin_is_constant_evaluated() ) {
    __kern::coeff_copy_cneg(rp.lo, rp.hi, ap.lo, ap.hi, rp.n, negate);
    return;
  }
#endif
  for ( usize i = 0u; i < rp.n; ++i ) __nuss_coeff_store(rp, i, __nuss_coeff_cneg(__nuss_coeff_load(ap, i), negate));
}

inline constexpr void
__nuss_add_coeffs(__nuss_coeffs rp, __nuss_const_coeffs ap, __nuss_const_coeffs bp) noexcept
{
#if defined(__micron_arbint_kern_coefficients)
  if ( !__builtin_is_constant_evaluated() ) {
    __kern::coeff_add(rp.lo, rp.hi, ap.lo, ap.hi, bp.lo, bp.hi, rp.n);
    return;
  }
#endif
  for ( usize i = 0u; i < rp.n; ++i ) __nuss_coeff_store(rp, i, __nuss_coeff_add(__nuss_coeff_load(ap, i), __nuss_coeff_load(bp, i)));
}

// rp = ap * Z^e in Z[Z]/(Z^r + 1), 0 <= e < 2r
inline constexpr void
__nuss_twiddle_copy(__nuss_coeffs rp, __nuss_const_coeffs ap, usize e) noexcept
{
  const usize r = rp.n;
  e &= 2u * r - 1u;
  const bool outer_neg = e >= r;
  if ( outer_neg ) e -= r;
  if ( e == 0u ) {
    __nuss_copy_cneg(rp, ap, outer_neg);
    return;
  }

  const usize straight = r - e;
  __nuss_copy_cneg(__nuss_coeff_subspan(rp, e, straight), __nuss_coeff_subspan(ap, 0u, straight), outer_neg);
  __nuss_copy_cneg(__nuss_coeff_subspan(rp, 0u, e), __nuss_coeff_subspan(ap, straight, e), !outer_neg);
}

// p *= Z^e in Z[Z]/(Z^r + 1), 0 <= e < 2r
inline constexpr void
__nuss_twiddle(__nuss_coeffs p, usize e) noexcept
{
  const usize r = p.n;
  e &= 2u * r - 1u;
  const bool outer_neg = e >= r;
  if ( outer_neg ) e -= r;

  if ( e == 0u ) {
    if ( outer_neg )
      for ( usize i = 0u; i < r; ++i ) __nuss_coeff_store(p, i, __nuss_coeff_neg(__nuss_coeff_load(p, i)));
    return;
  }

  const usize cycles = usize{ 1 } << limb_ctz(static_cast<limb_t>(e));
  for ( usize start = 0u; start < cycles; ++start ) {
    usize src = start;
    __nuss_coeff carry = __nuss_coeff_load(p, src);
    do {
      usize dst = src + e;
      const bool wrap = dst >= r;
      if ( wrap ) dst -= r;
      const __nuss_coeff next = __nuss_coeff_load(p, dst);
      carry = __nuss_coeff_cneg(carry, wrap != outer_neg);
      __nuss_coeff_store(p, dst, carry);
      carry = next;
      src = dst;
    } while ( src != start );
  }
}

inline constexpr void
__nuss_butterfly_to(__nuss_coeffs a, __nuss_const_coeffs b, __nuss_coeffs diff) noexcept
{
#if defined(__micron_arbint_kern_coefficients)
  if ( !__builtin_is_constant_evaluated() ) {
    __kern::coeff_butterfly_to(a.lo, a.hi, b.lo, b.hi, diff.lo, diff.hi, a.n);
    return;
  }
#endif
  for ( usize i = 0u; i < a.n; ++i ) {
    const __nuss_coeff u = __nuss_coeff_load(a, i);
    const __nuss_coeff v = __nuss_coeff_load(b, i);
    __nuss_coeff_store(a, i, __nuss_coeff_add(u, v));
    __nuss_coeff_store(diff, i, __nuss_coeff_sub(u, v));
  }
}

inline constexpr void
__nuss_butterfly(__nuss_coeffs a, __nuss_coeffs b) noexcept
{
  __nuss_butterfly_to(a, __nuss_as_const(b), b);
}

inline constexpr void
__nuss_forward_node(__nuss_coeffs a, usize base, usize half, usize stride, usize r, usize root_step, __nuss_coeffs temp) noexcept
{
  for ( usize j = 0u; j < half; ++j ) {
    __nuss_coeffs u = __nuss_coeff_subspan(a, (base + j) * r, r);
    __nuss_coeffs v = __nuss_coeff_subspan(a, (base + j + half) * r, r);
    const usize e = j * stride * root_step;
    if ( e == 0u ) {
      __nuss_butterfly(u, v);
    } else {
      __nuss_butterfly_to(u, __nuss_as_const(v), temp);
      __nuss_twiddle_copy(v, __nuss_as_const(temp), e);
    }
  }
}

inline constexpr void
__nuss_forward_block(__nuss_coeffs a, usize base, usize len, usize stride, usize r, usize root_step, __nuss_coeffs temp) noexcept
{
  usize stage_stride = stride;
  for ( usize stage_len = len; stage_len > 1u; stage_len >>= 1u ) {
    const usize half = stage_len >> 1u;
    for ( usize block = base; block < base + len; block += stage_len )
      __nuss_forward_node(a, block, half, stage_stride, r, root_step, temp);
    stage_stride <<= 1u;
  }
}

inline constexpr usize __nuss_cache_block_coeffs = threshold::nussbaumer_cache_block_coeffs;

inline constexpr void
__nuss_forward_rec(__nuss_coeffs a, usize base, usize len, usize stride, usize r, usize root_step, __nuss_coeffs temp) noexcept
{
  if ( len <= 1u ) return;
  if ( len * r <= __nuss_cache_block_coeffs ) {
    __nuss_forward_block(a, base, len, stride, r, root_step, temp);
    return;
  }
  const usize half = len >> 1u;
  __nuss_forward_node(a, base, half, stride, r, root_step, temp);
  __nuss_forward_rec(a, base, half, stride << 1u, r, root_step, temp);
  __nuss_forward_rec(a, base + half, half, stride << 1u, r, root_step, temp);
}

inline constexpr void
__nuss_forward(__nuss_coeffs a, usize outer_n, usize r, __nuss_coeffs temp) noexcept
{
  __nuss_forward_rec(a, 0u, outer_n, 1u, r, (2u * r) / outer_n, temp);
}

inline constexpr void
__nuss_inverse_node(__nuss_coeffs a, usize base, usize half, usize stride, usize r, usize root_step, __nuss_coeffs temp) noexcept
{
  for ( usize j = 0u; j < half; ++j ) {
    __nuss_coeffs u = __nuss_coeff_subspan(a, (base + j) * r, r);
    __nuss_coeffs v = __nuss_coeff_subspan(a, (base + j + half) * r, r);
    const usize e = j * stride * root_step;
    if ( e == 0u ) {
      __nuss_butterfly(u, v);
    } else {
      __nuss_twiddle_copy(temp, __nuss_as_const(v), 2u * r - e);
      __nuss_butterfly_to(u, __nuss_as_const(temp), v);
    }
  }
}

inline constexpr void
__nuss_inverse_block(__nuss_coeffs a, usize base, usize len, usize stride, usize r, usize root_step, __nuss_coeffs temp) noexcept
{
  usize stage_stride = stride * (len >> 1u);
  for ( usize stage_len = 2u; stage_len <= len; stage_len <<= 1u ) {
    const usize half = stage_len >> 1u;
    for ( usize block = base; block < base + len; block += stage_len )
      __nuss_inverse_node(a, block, half, stage_stride, r, root_step, temp);
    if ( stage_len == len ) break;
    stage_stride >>= 1u;
  }
}

inline constexpr void
__nuss_inverse_rec(__nuss_coeffs a, usize base, usize len, usize stride, usize r, usize root_step, __nuss_coeffs temp) noexcept
{
  if ( len <= 1u ) return;
  if ( len * r <= __nuss_cache_block_coeffs ) {
    __nuss_inverse_block(a, base, len, stride, r, root_step, temp);
    return;
  }
  const usize half = len >> 1u;
  __nuss_inverse_rec(a, base, half, stride << 1u, r, root_step, temp);
  __nuss_inverse_rec(a, base + half, half, stride << 1u, r, root_step, temp);
  __nuss_inverse_node(a, base, half, stride, r, root_step, temp);
}

inline constexpr void
__nuss_inverse(__nuss_coeffs a, usize outer_n, usize r, __nuss_coeffs temp) noexcept
{
  __nuss_inverse_rec(a, 0u, outer_n, 1u, r, (2u * r) / outer_n, temp);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// recursive negacyclic products

[[nodiscard, gnu::flatten]] inline constexpr usize
__nuss_mul_rec_itch_coeffs(usize n) noexcept
{
  if ( n <= __nuss_leaf_n ) return n;
  const usize k = __nuss_log2(n);
  const usize r = __nuss_inner_n(k);
  return __nuss_size_add(__nuss_size_mul(4u, n), __nuss_mul_rec_itch_coeffs(r));
}

[[nodiscard, gnu::flatten]] inline constexpr usize
__nuss_sqr_rec_itch_coeffs(usize n) noexcept
{
  if ( n <= __nuss_leaf_n ) return n;
  const usize k = __nuss_log2(n);
  const usize r = __nuss_inner_n(k);
  return __nuss_size_add(__nuss_size_mul(2u, n), __nuss_sqr_rec_itch_coeffs(r));
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_mul_negacyclic_itch(usize n) noexcept
{
  return __nuss_size_mul(2u, __nuss_mul_rec_itch_coeffs(n));
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_sqr_negacyclic_itch(usize n) noexcept
{
  return __nuss_size_mul(2u, __nuss_sqr_rec_itch_coeffs(n));
}

template<bool Wide> struct __nuss_leaf_accumulator;

template<> struct __nuss_leaf_accumulator<true> {
  dlimb_t bits;

  constexpr void
  add(__nuss_coeff a, __nuss_coeff b, bool twice = false) noexcept
  {
    dlimb_t p = __nuss_coeff_mul_bits(a, b);
    if ( twice ) p += p;
    bits += p;
  }

  constexpr void
  sub(__nuss_coeff a, __nuss_coeff b, bool twice = false) noexcept
  {
    dlimb_t p = __nuss_coeff_mul_bits(a, b);
    if ( twice ) p += p;
    bits -= p;
  }

  [[nodiscard]] constexpr __nuss_coeff
  combine(const __nuss_leaf_accumulator &other) const noexcept
  {
    const dlimb_t sum = bits + other.bits;
    return { lo_half(sum), hi_half(sum) };
  }
};

template<> struct __nuss_leaf_accumulator<false> {
  __nuss_coeff value;

  constexpr void
  add(__nuss_coeff a, __nuss_coeff b, bool twice = false) noexcept
  {
    __nuss_coeff p = __nuss_coeff_mul(a, b);
    if ( twice ) p = __nuss_coeff_add(p, p);
    value = __nuss_coeff_add(value, p);
  }

  constexpr void
  sub(__nuss_coeff a, __nuss_coeff b, bool twice = false) noexcept
  {
    __nuss_coeff p = __nuss_coeff_mul(a, b);
    if ( twice ) p = __nuss_coeff_add(p, p);
    value = __nuss_coeff_sub(value, p);
  }

  [[nodiscard]] constexpr __nuss_coeff
  combine(const __nuss_leaf_accumulator &other) const noexcept
  {
    return __nuss_coeff_add(value, other.value);
  }
};

template<bool Wide>
[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_mul_leaf_coefficient(__nuss_const_coeffs ap, __nuss_const_coeffs bp, usize n, usize d) noexcept
{
  __nuss_leaf_accumulator<Wide> acc0{}, acc1{};
  usize i = 0u;
  for ( ; i + 1u <= d; i += 2u ) {
    acc0.add(__nuss_coeff_load(ap, i), __nuss_coeff_load(bp, d - i));
    acc1.add(__nuss_coeff_load(ap, i + 1u), __nuss_coeff_load(bp, d - i - 1u));
  }
  if ( i <= d ) acc0.add(__nuss_coeff_load(ap, i), __nuss_coeff_load(bp, d - i));

  i = d + 1u;
  for ( ; i + 1u < n; i += 2u ) {
    acc0.sub(__nuss_coeff_load(ap, i), __nuss_coeff_load(bp, d + n - i));
    acc1.sub(__nuss_coeff_load(ap, i + 1u), __nuss_coeff_load(bp, d + n - i - 1u));
  }
  if ( i < n ) acc0.sub(__nuss_coeff_load(ap, i), __nuss_coeff_load(bp, d + n - i));
  return acc0.combine(acc1);
}

template<usize N>
[[gnu::noinline]] inline constexpr void
__nuss_mul_leaf_fixed(__nuss_coeffs rp, __nuss_const_coeffs ap, __nuss_const_coeffs bp, limb_t *scratch) noexcept
{
  const usize n = N == 0u ? rp.n : N;
  __nuss_coeffs work = __nuss_coeff_span(scratch, n);
  if constexpr ( N != 0u && N <= 8u ) {
#if defined(__GNUC__)
#pragma GCC unroll 8
#endif
    for ( usize d = 0u; d < N; ++d ) __nuss_coeff_store(work, d, __nuss_mul_leaf_coefficient<true>(ap, bp, N, d));
  } else {
    for ( usize d = 0u; d < n; ++d ) __nuss_coeff_store(work, d, __nuss_mul_leaf_coefficient<false>(ap, bp, n, d));
  }
  for ( usize i = 0u; i < n; ++i ) __nuss_coeff_store(rp, i, __nuss_coeff_load(work, i));
}

inline constexpr void
__nuss_mul_leaf(__nuss_coeffs rp, __nuss_const_coeffs ap, __nuss_const_coeffs bp, limb_t *scratch) noexcept
{
  switch ( rp.n ) {
  case 1u:
    return __nuss_mul_leaf_fixed<1u>(rp, ap, bp, scratch);
  case 2u:
    return __nuss_mul_leaf_fixed<2u>(rp, ap, bp, scratch);
  case 4u:
    return __nuss_mul_leaf_fixed<4u>(rp, ap, bp, scratch);
  case 8u:
    return __nuss_mul_leaf_fixed<8u>(rp, ap, bp, scratch);
  case 16u:
    return __nuss_mul_leaf_fixed<16u>(rp, ap, bp, scratch);
  default:
    return __nuss_mul_leaf_fixed<0u>(rp, ap, bp, scratch);
  }
}

template<bool Wide>
[[nodiscard, gnu::always_inline]] inline constexpr __nuss_coeff
__nuss_sqr_leaf_coefficient(__nuss_const_coeffs ap, usize n, usize d) noexcept
{
  __nuss_leaf_accumulator<Wide> acc0{}, acc1{};
  const usize pos_last = d >> 1u;
  usize i = 0u;
  for ( ; i + 1u <= pos_last; i += 2u ) {
    acc0.add(__nuss_coeff_load(ap, i), __nuss_coeff_load(ap, d - i), i != d - i);
    acc1.add(__nuss_coeff_load(ap, i + 1u), __nuss_coeff_load(ap, d - i - 1u), i + 1u != d - i - 1u);
  }
  if ( i <= pos_last ) acc0.add(__nuss_coeff_load(ap, i), __nuss_coeff_load(ap, d - i), i != d - i);

  const usize neg_last = (d + n) >> 1u;
  i = d + 1u;
  for ( ; i + 1u <= neg_last; i += 2u ) {
    acc0.sub(__nuss_coeff_load(ap, i), __nuss_coeff_load(ap, d + n - i), i != d + n - i);
    acc1.sub(__nuss_coeff_load(ap, i + 1u), __nuss_coeff_load(ap, d + n - i - 1u), i + 1u != d + n - i - 1u);
  }
  if ( i <= neg_last ) acc0.sub(__nuss_coeff_load(ap, i), __nuss_coeff_load(ap, d + n - i), i != d + n - i);
  return acc0.combine(acc1);
}

template<usize N>
[[gnu::noinline]] inline constexpr void
__nuss_sqr_leaf_fixed(__nuss_coeffs rp, __nuss_const_coeffs ap, limb_t *scratch) noexcept
{
  const usize n = N == 0u ? rp.n : N;
  __nuss_coeffs work = __nuss_coeff_span(scratch, n);
  if constexpr ( N != 0u && N <= 8u ) {
#if defined(__GNUC__)
#pragma GCC unroll 8
#endif
    for ( usize d = 0u; d < N; ++d ) __nuss_coeff_store(work, d, __nuss_sqr_leaf_coefficient<true>(ap, N, d));
  } else {
    for ( usize d = 0u; d < n; ++d ) __nuss_coeff_store(work, d, __nuss_sqr_leaf_coefficient<false>(ap, n, d));
  }
  for ( usize i = 0u; i < n; ++i ) __nuss_coeff_store(rp, i, __nuss_coeff_load(work, i));
}

inline constexpr void
__nuss_sqr_leaf(__nuss_coeffs rp, __nuss_const_coeffs ap, limb_t *scratch) noexcept
{
  switch ( rp.n ) {
  case 1u:
    return __nuss_sqr_leaf_fixed<1u>(rp, ap, scratch);
  case 2u:
    return __nuss_sqr_leaf_fixed<2u>(rp, ap, scratch);
  case 4u:
    return __nuss_sqr_leaf_fixed<4u>(rp, ap, scratch);
  case 8u:
    return __nuss_sqr_leaf_fixed<8u>(rp, ap, scratch);
  case 16u:
    return __nuss_sqr_leaf_fixed<16u>(rp, ap, scratch);
  default:
    return __nuss_sqr_leaf_fixed<0u>(rp, ap, scratch);
  }
}

inline constexpr bool __nuss_mul_negacyclic(__nuss_coeffs rp, __nuss_const_coeffs ap, __nuss_const_coeffs bp, limb_t *scratch) noexcept;
inline constexpr bool __nuss_sqr_negacyclic(__nuss_coeffs rp, __nuss_const_coeffs ap, limb_t *scratch) noexcept;

inline constexpr void
__nuss_transpose(__nuss_coeffs rp, __nuss_const_coeffs ap, usize rows, usize cols) noexcept
{
#if defined(__micron_arbint_kern_coefficients)
  if ( !__builtin_is_constant_evaluated() ) {
    constexpr usize lanes = __kern::vec_lanes;
    const usize full_rows = rows - rows % lanes;
    const usize full_cols = cols - cols % lanes;
    for ( usize col = 0u; col < full_cols; col += lanes ) {
      for ( usize row = 0u; row < full_rows; row += lanes ) {
        const usize out = col * rows + row;
        const usize in = row * cols + col;
        __kern::coeff_transpose_square(rp.lo + out, rp.hi + out, rows, ap.lo + in, ap.hi + in, cols);
      }
      for ( usize row = full_rows; row < rows; ++row ) {
        for ( usize i = 0u; i < lanes; ++i ) {
          const usize out = (col + i) * rows + row;
          const usize in = row * cols + col + i;
          rp.lo[out] = ap.lo[in];
          rp.hi[out] = ap.hi[in];
        }
      }
    }
    for ( usize col = full_cols; col < cols; ++col ) {
      for ( usize row = 0u; row < rows; ++row ) {
        const usize out = col * rows + row;
        const usize in = row * cols + col;
        rp.lo[out] = ap.lo[in];
        rp.hi[out] = ap.hi[in];
      }
    }
    return;
  }
#endif
  constexpr usize tile = 8u;
  for ( usize row0 = 0u; row0 < rows; row0 += tile ) {
    const usize row_n = rows - row0 < tile ? rows - row0 : tile;
    for ( usize col0 = 0u; col0 < cols; col0 += tile ) {
      const usize col_n = cols - col0 < tile ? cols - col0 : tile;
      for ( usize col = 0u; col < col_n; ++col ) {
        const usize out = (col0 + col) * rows + row0;
        for ( usize row = 0u; row < row_n; ++row ) {
          const usize in = (row0 + row) * cols + col0 + col;
          rp.lo[out + row] = ap.lo[in];
          rp.hi[out + row] = ap.hi[in];
        }
      }
    }
  }
}

inline constexpr void
__nuss_reshape(__nuss_coeffs rp, __nuss_const_coeffs ap, usize s, usize r) noexcept
{
  __nuss_transpose(__nuss_coeff_subspan(rp, 0u, s * r), ap, r, s);
  __nuss_zero_coeffs(__nuss_coeff_subspan(rp, s * r, s * r));
}

[[nodiscard]] inline constexpr bool
__nuss_divexact_coeffs(__nuss_coeffs a, usize shift) noexcept
{
#if defined(__micron_arbint_kern_coefficients)
  if ( !__builtin_is_constant_evaluated() ) return __kern::coeff_divexact_pow2(a.lo, a.hi, a.n, shift);
#endif
  bool exact = true;
  for ( usize i = 0u; i < a.n; ++i ) {
    __nuss_coeff v = __nuss_coeff_load(a, i);
    exact = __nuss_coeff_divexact_pow2(v, shift) && exact;
    __nuss_coeff_store(a, i, v);
  }
  return exact;
}

inline constexpr bool
__nuss_fold(__nuss_coeffs rp, __nuss_coeffs a, usize s, usize r) noexcept
{
  const usize outer_n = 2u * s;
  const usize div_shift = __nuss_log2(outer_n);
  const bool exact = __nuss_divexact_coeffs(a, div_shift);

  for ( usize i = 0u; i < s; ++i ) {
    __nuss_coeffs lo = __nuss_coeff_subspan(a, i * r, r);
    const __nuss_const_coeffs hi = __nuss_coeff_subspan(__nuss_as_const(a), (i + s) * r, r);
    __nuss_coeff_store(lo, 0u, __nuss_coeff_sub(__nuss_coeff_load(lo, 0u), __nuss_coeff_load(hi, r - 1u)));
    if ( r > 1u )
      __nuss_add_coeffs(__nuss_coeff_subspan(lo, 1u, r - 1u), __nuss_coeff_subspan(__nuss_as_const(lo), 1u, r - 1u),
                        __nuss_coeff_subspan(hi, 0u, r - 1u));
  }
  __nuss_transpose(rp, __nuss_coeff_subspan(__nuss_as_const(a), 0u, s * r), s, r);
  return exact;
}

inline constexpr bool
__nuss_mul_loaded(__nuss_coeffs rp, __nuss_coeffs a, __nuss_coeffs b, usize s, usize r, limb_t *scratch) noexcept
{
  const usize outer_n = 2u * s;
  __nuss_coeffs temp = __nuss_coeff_span(scratch, r);
  __nuss_forward(a, outer_n, r, temp);
  __nuss_forward(b, outer_n, r, temp);

  bool exact = true;
  for ( usize i = 0u; i < outer_n; ++i ) {
    __nuss_coeffs ai = __nuss_coeff_subspan(a, i * r, r);
    const __nuss_const_coeffs bi = __nuss_coeff_subspan(__nuss_as_const(b), i * r, r);
    exact = __nuss_mul_negacyclic(ai, __nuss_as_const(ai), bi, scratch) && exact;
  }

  __nuss_inverse(a, outer_n, r, temp);
  return __nuss_fold(rp, a, s, r) && exact;
}

inline constexpr bool
__nuss_sqr_loaded(__nuss_coeffs rp, __nuss_coeffs a, usize s, usize r, limb_t *scratch) noexcept
{
  const usize outer_n = 2u * s;
  __nuss_coeffs temp = __nuss_coeff_span(scratch, r);
  __nuss_forward(a, outer_n, r, temp);

  bool exact = true;
  for ( usize i = 0u; i < outer_n; ++i ) {
    __nuss_coeffs ai = __nuss_coeff_subspan(a, i * r, r);
    exact = __nuss_sqr_negacyclic(ai, __nuss_as_const(ai), scratch) && exact;
  }

  __nuss_inverse(a, outer_n, r, temp);
  return __nuss_fold(rp, a, s, r) && exact;
}

inline constexpr bool
__nuss_mul_negacyclic(__nuss_coeffs rp, __nuss_const_coeffs ap, __nuss_const_coeffs bp, limb_t *scratch) noexcept
{
  const usize n = rp.n;
  if ( n <= __nuss_leaf_n ) {
    __nuss_mul_leaf(rp, ap, bp, scratch);
    return true;
  }

  const usize k = __nuss_log2(n);
  const usize r = __nuss_inner_n(k);
  const usize s = n / r;
  __nuss_coeffs a = __nuss_coeff_span(scratch, 2u * n);
  __nuss_coeffs b = __nuss_coeff_span(scratch + 4u * n, 2u * n);
  limb_t *const rec = scratch + 8u * n;
  __nuss_reshape(a, ap, s, r);
  __nuss_reshape(b, bp, s, r);
  return __nuss_mul_loaded(rp, a, b, s, r, rec);
}

inline constexpr bool
__nuss_sqr_negacyclic(__nuss_coeffs rp, __nuss_const_coeffs ap, limb_t *scratch) noexcept
{
  const usize n = rp.n;
  if ( n <= __nuss_leaf_n ) {
    __nuss_sqr_leaf(rp, ap, scratch);
    return true;
  }

  const usize k = __nuss_log2(n);
  const usize r = __nuss_inner_n(k);
  const usize s = n / r;
  __nuss_coeffs a = __nuss_coeff_span(scratch, 2u * n);
  limb_t *const rec = scratch + 4u * n;
  __nuss_reshape(a, ap, s, r);
  return __nuss_sqr_loaded(rp, a, s, r, rec);
}

inline constexpr bool
__nuss_mul_negacyclic(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n, limb_t *scratch) noexcept
{
  return __nuss_mul_negacyclic(__nuss_coeff_span(rp, n), __nuss_coeff_span(ap, n), __nuss_coeff_span(bp, n), scratch);
}

inline constexpr bool
__nuss_sqr_negacyclic(limb_t *rp, const limb_t *ap, usize n, limb_t *scratch) noexcept
{
  return __nuss_sqr_negacyclic(__nuss_coeff_span(rp, n), __nuss_coeff_span(ap, n), scratch);
}

// %%%%%%%%%%%%%%%%%%%%%%%
// limb/digit conversion

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
__nuss_low_mask(usize bits) noexcept
{
  return bits >= limb_bits ? limb_max : static_cast<limb_t>((limb_t{ 1 } << bits) - 1u);
}

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
__nuss_digit(const limb_t *ap, usize an, usize digit, usize q) noexcept
{
  const usize bit = digit * q;
  const usize word = bit / limb_bits;
  const usize shift = bit % limb_bits;
  if ( word >= an ) return 0u;
  limb_t v = static_cast<limb_t>(ap[word] >> shift);
  if ( shift != 0u && q > limb_bits - shift && word + 1u < an ) v = static_cast<limb_t>(v | (ap[word + 1u] << (limb_bits - shift)));
  return static_cast<limb_t>(v & __nuss_low_mask(q));
}

struct __nuss_packer {
  limb_t *rp;
  usize rn;
  usize out;
  usize used;
  limb_t acc;
  bool exact;

  constexpr void
  push(limb_t digit, usize q) noexcept
  {
    const usize room = limb_bits - used;
    acc = static_cast<limb_t>(acc | static_cast<limb_t>(digit << used));
    if ( q < room ) {
      used += q;
      return;
    }

    if ( out < rn )
      rp[out] = acc;
    else if ( acc != 0u )
      exact = false;
    ++out;
    if ( q == room ) {
      used = 0u;
      acc = 0u;
    } else {
      used = q - room;
      acc = static_cast<limb_t>(digit >> room);
    }
  }

  constexpr void
  finish() noexcept
  {
    if ( used != 0u ) {
      if ( out < rn )
        rp[out] = acc;
      else if ( acc != 0u )
        exact = false;
      ++out;
    }
    while ( out < rn ) rp[out++] = 0u;
  }
};

inline constexpr bool
__nuss_reassemble(limb_t *rp, usize rn, __nuss_const_coeffs cp, usize q) noexcept
{
  __nuss_coeff carry{};
  __nuss_packer pack{ rp, rn, 0u, 0u, 0u, true };
  for ( usize i = 0u; i < cp.n; ++i ) {
    const __nuss_coeff v = __nuss_coeff_add(__nuss_coeff_load(cp, i), carry);
    if ( __nuss_coeff_negative(v) ) return false;
    pack.push(static_cast<limb_t>(v.lo & __nuss_low_mask(q)), q);
    carry = __nuss_coeff_shr(v, q);
  }
  while ( !__nuss_coeff_is_zero(carry) ) {
    if ( __nuss_coeff_negative(carry) ) return false;
    pack.push(static_cast<limb_t>(carry.lo & __nuss_low_mask(q)), q);
    carry = __nuss_coeff_shr(carry, q);
  }
  pack.finish();
  return pack.exact;
}

inline constexpr bool
__nuss_mul_product(limb_t *rp, const limb_t *ap, usize an, const limb_t *bp, usize bn, const __nuss_plan &plan, limb_t *scratch) noexcept
{
  __nuss_coeffs cp = __nuss_coeff_span(scratch, plan.n);
  limb_t *const work = scratch + 2u * plan.n;
  bool exact = true;

  if ( plan.n <= __nuss_leaf_n ) {
    __nuss_coeffs accum = __nuss_coeff_span(work, plan.n);
    __nuss_zero_coeffs(cp);
    __nuss_zero_coeffs(accum);
    for ( usize i = 0u; i < plan.a_digits; ++i ) {
      const __nuss_coeff a{ __nuss_digit(ap, an, i, plan.digit_bits), 0u };
      __nuss_coeff_store(cp, i, a);
      for ( usize j = 0u; j < plan.b_digits; ++j ) {
        const __nuss_coeff b{ __nuss_digit(bp, bn, j, plan.digit_bits), 0u };
        const usize ij = i + j;
        const __nuss_coeff old = __nuss_coeff_load(accum, ij);
        __nuss_coeff_store(accum, ij, __nuss_coeff_add(old, __nuss_coeff_mul(a, b)));
      }
    }
    __nuss_copy_coeffs(cp, __nuss_as_const(accum));
  } else {
    const usize r = __nuss_inner_n(plan.log_n);
    const usize s = plan.n / r;
    __nuss_coeffs a = __nuss_coeff_span(work, 2u * plan.n);
    __nuss_coeffs b = __nuss_coeff_span(work + 4u * plan.n, 2u * plan.n);
    limb_t *const rec = work + 8u * plan.n;
    __nuss_zero_coeffs(a);
    __nuss_zero_coeffs(b);
    for ( usize d = 0u; d < plan.a_digits; ++d )
      __nuss_coeff_store(a, (d % s) * r + d / s, { __nuss_digit(ap, an, d, plan.digit_bits), 0u });
    for ( usize d = 0u; d < plan.b_digits; ++d )
      __nuss_coeff_store(b, (d % s) * r + d / s, { __nuss_digit(bp, bn, d, plan.digit_bits), 0u });
    exact = __nuss_mul_loaded(cp, a, b, s, r, rec);
  }
  return __nuss_reassemble(rp, an + bn, __nuss_as_const(cp), plan.digit_bits) && exact;
}

inline constexpr bool
__nuss_sqr_product(limb_t *rp, const limb_t *ap, usize an, const __nuss_plan &plan, limb_t *scratch) noexcept
{
  __nuss_coeffs cp = __nuss_coeff_span(scratch, plan.n);
  limb_t *const work = scratch + 2u * plan.n;
  bool exact = true;

  if ( plan.n <= __nuss_leaf_n ) {
    __nuss_zero_coeffs(cp);
    for ( usize i = 0u; i < plan.a_digits; ++i ) __nuss_coeff_store(cp, i, { __nuss_digit(ap, an, i, plan.digit_bits), 0u });
    __nuss_sqr_leaf(cp, __nuss_as_const(cp), work);
  } else {
    const usize r = __nuss_inner_n(plan.log_n);
    const usize s = plan.n / r;
    __nuss_coeffs a = __nuss_coeff_span(work, 2u * plan.n);
    limb_t *const rec = work + 4u * plan.n;
    __nuss_zero_coeffs(a);
    for ( usize d = 0u; d < plan.a_digits; ++d )
      __nuss_coeff_store(a, (d % s) * r + d / s, { __nuss_digit(ap, an, d, plan.digit_bits), 0u });
    exact = __nuss_sqr_loaded(cp, a, s, r, rec);
  }
  return __nuss_reassemble(rp, 2u * an, __nuss_as_const(cp), plan.digit_bits) && exact;
}

// %%%%%%%%%%%%%%%%%%%%%%%%
// transform tiling

[[nodiscard, gnu::flatten]] inline constexpr usize
__nuss_max_balanced_limbs() noexcept
{
  const usize q = limb_bits - 2u - threshold::nussbaumer_max_log;
  usize hi = ((__nuss_max_n + 1u) * q) / (2u * limb_bits) + 2u;
  usize lo = 0u;
  while ( lo + 1u < hi ) {
    const usize mid = lo + (hi - lo) / 2u;
    if ( __nuss_make_plan(mid, mid).valid )
      lo = mid;
    else
      hi = mid;
  }
  return lo;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
__nuss_max_partner(usize fixed, usize wanted) noexcept
{
  usize lo = 0u;
  usize hi = wanted + 1u;
  if ( hi == 0u ) hi = wanted;
  while ( lo + 1u < hi ) {
    const usize mid = lo + (hi - lo) / 2u;
    if ( __nuss_make_plan(mid, fixed).valid )
      lo = mid;
    else
      hi = mid;
  }
  return lo;
}

struct __nuss_mul_shape {
  usize an;
  usize bn;
  __nuss_plan plan;
  bool direct;
};

[[nodiscard, gnu::flatten]] inline constexpr __nuss_mul_shape
__nuss_pick_mul_shape(usize an, usize bn) noexcept
{
  const bool long_axis = an / bn >= 8u;
  const __nuss_plan whole = __nuss_make_plan(an, bn);
  if ( !long_axis && whole.valid ) return { an, bn, whole, true };

  if ( long_axis ) {
    const usize three_bn = __nuss_size_mul(3u, bn);
    const usize block = an < three_bn ? an : three_bn;
    const __nuss_plan p = __nuss_make_plan(block, bn);
    if ( p.valid ) return { block, bn, p, false };
  }

  const usize balanced = __nuss_max_balanced_limbs();
  const usize bblock = bn < balanced ? bn : balanced;
  usize wanted = an;
  if ( long_axis ) {
    const usize three_b = __nuss_size_mul(3u, bblock);
    wanted = an < three_b ? an : three_b;
  }
  const usize ablock = __nuss_max_partner(bblock, wanted);
  return { ablock, bblock, __nuss_make_plan(ablock, bblock), false };
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_plan_mul_itch(const __nuss_plan &p) noexcept
{
  return __nuss_size_mul(2u, __nuss_size_add(p.n, __nuss_mul_rec_itch_coeffs(p.n)));
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__nuss_plan_sqr_itch(const __nuss_plan &p) noexcept
{
  return __nuss_size_mul(2u, __nuss_size_add(p.n, __nuss_sqr_rec_itch_coeffs(p.n)));
}

[[nodiscard, gnu::flatten]] inline constexpr usize
nussbaumer_itch(usize an, usize bn) noexcept
{
  const __nuss_mul_shape shape = __nuss_pick_mul_shape(an, bn);
  return shape.plan.valid ? __nuss_plan_mul_itch(shape.plan) : __nuss_size_max;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_nussbaumer_itch(usize n) noexcept
{
  const __nuss_plan whole = __nuss_make_plan(n, n);
  if ( whole.valid ) return __nuss_plan_sqr_itch(whole);
  const usize block = __nuss_max_balanced_limbs();
  const __nuss_plan p = __nuss_make_plan(block, block);
  return p.valid ? __nuss_plan_mul_itch(p) : __nuss_size_max;
}

inline constexpr void
__nuss_add_partial(limb_t *rp, usize rn, usize off, const limb_t *part, usize pn) noexcept
{
  limb_t cy = add_n(rp + off, rp + off, part, pn);
  usize i = off + pn;
  while ( cy != 0u && i < rn ) {
    cy = addc(rp[i], 0u, cy, rp[i]);
    ++i;
  }
}

inline constexpr void
mul_nussbaumer(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn,
               limb_t *scratch) noexcept
{
  const __nuss_mul_shape shape = __nuss_pick_mul_shape(an, bn);
  if ( shape.direct ) {
    (void)__nuss_mul_product(rp, ap, an, bp, bn, shape.plan, scratch);
    return;
  }

  const usize rn = an + bn;
  zero(rp, rn);
  for ( usize bj = 0u; bj < bn; bj += shape.bn ) {
    const usize btake = bn - bj < shape.bn ? bn - bj : shape.bn;
    for ( usize ai = 0u; ai < an; ai += shape.an ) {
      const usize atake = an - ai < shape.an ? an - ai : shape.an;
      const __nuss_plan p = __nuss_make_plan(atake, btake);
      (void)__nuss_mul_product(scratch, ap + ai, atake, bp + bj, btake, p, scratch);
      __nuss_add_partial(rp, rn, ai + bj, scratch, atake + btake);
    }
  }
}

inline constexpr void
sqr_nussbaumer(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch) noexcept
{
  const __nuss_plan whole = __nuss_make_plan(n, n);
  if ( whole.valid ) {
    (void)__nuss_sqr_product(rp, ap, n, whole, scratch);
    return;
  }

  const usize block = __nuss_max_balanced_limbs();
  const usize rn = 2u * n;
  zero(rp, rn);
  for ( usize i = 0u; i < n; i += block ) {
    const usize itake = n - i < block ? n - i : block;
    const __nuss_plan sp = __nuss_make_plan(itake, itake);
    (void)__nuss_sqr_product(scratch, ap + i, itake, sp, scratch);
    __nuss_add_partial(rp, rn, 2u * i, scratch, 2u * itake);

    for ( usize j = 0u; j < i; j += block ) {
      const usize jtake = n - j < block ? n - j : block;
      const __nuss_plan mp = __nuss_make_plan(itake, jtake);
      (void)__nuss_mul_product(scratch, ap + i, itake, ap + j, jtake, mp, scratch);
      __nuss_add_partial(rp, rn, i + j, scratch, itake + jtake);
      __nuss_add_partial(rp, rn, i + j, scratch, itake + jtake);
    }
  }
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
