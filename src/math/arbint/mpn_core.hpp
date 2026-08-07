//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "bits/carry.hpp"
#include "kernels.hpp"
#include "limb.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// mpn
//
// naked pointers and lengths;
// no ownership;
// no sign;
// no normalization
//
//  .. lengths are in limbs and every routine that takes n requires n >= 1 unless it says otherwise
//  .. inputs need not be normalized; outputs are not normalized either (call normalize())
//  .. rp may equal ap or bp for the in-place families (add/sub/shift/logic); it may not partially overlap
//  .. the multiply outputs take __restrict__ and genuinely require a distinct destination

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// bulk moves
[[gnu::always_inline]] inline constexpr void
copyi(limb_t *rp, const limb_t *ap, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) rp[i] = ap[i];
}

[[gnu::always_inline]] inline constexpr void
copyd(limb_t *rp, const limb_t *ap, usize n) noexcept
{
  for ( usize i = n; i-- > 0; ) rp[i] = ap[i];
}

[[gnu::always_inline]] inline constexpr void
zero(limb_t *rp, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) rp[i] = 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compares

[[nodiscard, gnu::flatten]] inline constexpr int
cmp(const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  for ( usize i = n; i-- > 0; )
    if ( ap[i] != bp[i] ) return ap[i] < bp[i] ? -1 : 1;
  return 0;
}

[[nodiscard, gnu::flatten]] inline constexpr int
cmp_var(const limb_t *ap, usize an, const limb_t *bp, usize bn) noexcept
{
  if ( an != bn ) return an < bn ? -1 : 1;
  return cmp(ap, bp, an);
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
is_zero(const limb_t *ap, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( ap[i] != 0 ) return false;
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// portable reference loops
namespace __portable
{

[[nodiscard, gnu::flatten]] inline constexpr limb_t
add_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) cy = addc(ap[i], bp[i], cy, rp[i]);
  return cy;
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
sub_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  limb_t bw = 0;
  for ( usize i = 0; i < n; ++i ) bw = subb(ap[i], bp[i], bw, rp[i]);
  return bw;
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
mul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) {
    limb_t lo = 0, hi = 0;
    muladd_wide(ap[i], b, cy, lo, hi);
    rp[i] = lo;
    cy = hi;
  }
  return cy;
}

// rp += ap * b. a*b + cy + rp[i] <= (2^w-1)^2 + 2(2^w-1) == 2^2w - 1
[[nodiscard, gnu::flatten]] inline constexpr limb_t
addmul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) {
    limb_t lo = 0, hi = 0;
    muladd2_wide(ap[i], b, cy, rp[i], lo, hi);
    rp[i] = lo;
    cy = hi;
  }
  return cy;
}

// rp -= ap * b. hi + bw cannot wrap
[[nodiscard, gnu::flatten]] inline constexpr limb_t
submul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) {
    limb_t lo = 0, hi = 0;
    muladd_wide(ap[i], b, cy, lo, hi);
    limb_t d = 0;
    const limb_t bw = subb(rp[i], lo, 0, d);
    rp[i] = d;
    cy = static_cast<limb_t>(hi + bw);
  }
  return cy;
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
lshift(limb_t *rp, const limb_t *ap, usize n, usize cnt) noexcept
{
  if ( n == 0 ) return 0;
  const usize tnc = limb_bits - cnt;
  limb_t high = ap[n - 1];
  const limb_t ret = static_cast<limb_t>(high >> tnc);
  for ( usize i = n - 1; i > 0; --i ) {
    const limb_t low = ap[i - 1];
    rp[i] = static_cast<limb_t>((high << cnt) | (low >> tnc));
    high = low;
  }
  rp[0] = static_cast<limb_t>(high << cnt);
  return ret;
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
rshift(limb_t *rp, const limb_t *ap, usize n, usize cnt) noexcept
{
  if ( n == 0 ) return 0;
  const usize tnc = limb_bits - cnt;
  limb_t low = ap[0];
  const limb_t ret = static_cast<limb_t>(low << tnc);
  for ( usize i = 0; i + 1 < n; ++i ) {
    const limb_t high = ap[i + 1];
    rp[i] = static_cast<limb_t>((low >> cnt) | (high << tnc));
    low = high;
  }
  rp[n - 1] = static_cast<limb_t>(low >> cnt);
  return ret;
}

};      // namespace __portable

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// add / subtract

[[nodiscard, gnu::flatten]] inline constexpr limb_t
add_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
#if defined(__micron_arbint_kern_add_n)
  if !consteval {
    return __kern::add_n(rp, ap, bp, n);
  }
#endif
  return __portable::add_n(rp, ap, bp, n);
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
sub_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
#if defined(__micron_arbint_kern_sub_n)
  if !consteval {
    return __kern::sub_n(rp, ap, bp, n);
  }
#endif
  return __portable::sub_n(rp, ap, bp, n);
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
add_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = b;
  usize i = 0;
  for ( ; i < n && cy != 0; ++i ) cy = addc(ap[i], cy, 0, rp[i]);
  if ( rp != ap )
    for ( ; i < n; ++i ) rp[i] = ap[i];
  return cy;
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
sub_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t bw = b;
  usize i = 0;
  for ( ; i < n && bw != 0; ++i ) bw = subb(ap[i], bw, 0, rp[i]);
  if ( rp != ap )
    for ( ; i < n; ++i ) rp[i] = ap[i];
  return bw;
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
add(limb_t *rp, const limb_t *ap, usize an, const limb_t *bp, usize bn) noexcept
{
  const limb_t cy = add_n(rp, ap, bp, bn);
  if ( an == bn ) return cy;
  return add_1(rp + bn, ap + bn, an - bn, cy);
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
sub(limb_t *rp, const limb_t *ap, usize an, const limb_t *bp, usize bn) noexcept
{
  const limb_t bw = sub_n(rp, ap, bp, bn);
  if ( an == bn ) return bw;
  return sub_1(rp + bn, ap + bn, an - bn, bw);
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
neg(limb_t *rp, const limb_t *ap, usize n) noexcept
{
  usize i = 0;
  for ( ; i < n; ++i ) {
    if ( ap[i] != 0 ) {
      rp[i] = static_cast<limb_t>(~ap[i] + 1u);
      for ( ++i; i < n; ++i ) rp[i] = static_cast<limb_t>(~ap[i]);
      return 1;
    }
    rp[i] = 0;
  }
  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// conditional add / subtract
//
// rp = ap +/- (cnd ? bp : 0)

[[nodiscard, gnu::flatten]] inline constexpr limb_t
cnd_add_n(limb_t cnd, limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  const limb_t mask = static_cast<limb_t>(0) - static_cast<limb_t>(cnd != 0);
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) cy = addc(ap[i], static_cast<limb_t>(bp[i] & mask), cy, rp[i]);
  return cy;
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
cnd_sub_n(limb_t cnd, limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  const limb_t mask = static_cast<limb_t>(0) - static_cast<limb_t>(cnd != 0);
  limb_t bw = 0;
  for ( usize i = 0; i < n; ++i ) bw = subb(ap[i], static_cast<limb_t>(bp[i] & mask), bw, rp[i]);
  return bw;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// multiply by one limb
[[nodiscard, gnu::flatten]] inline constexpr limb_t
mul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
#if defined(__micron_arbint_kern_mul_1)
  if !consteval {
    return __kern::mul_1(rp, ap, n, b);
  }
#endif
  return __portable::mul_1(rp, ap, n, b);
}

// rp += ap * b
[[nodiscard, gnu::flatten]] inline constexpr limb_t
addmul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
#if defined(__micron_arbint_kern_addmul_1)
  if !consteval {
    return __kern::addmul_1(rp, ap, n, b);
  }
#endif
  return __portable::addmul_1(rp, ap, n, b);
}

// rp -= ap * b
[[nodiscard, gnu::flatten]] inline constexpr limb_t
submul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
#if defined(__micron_arbint_kern_submul_1)
  if !consteval {
    return __kern::submul_1(rp, ap, n, b);
  }
#endif
  return __portable::submul_1(rp, ap, n, b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// shifts
//
// 0 < cnt < limb_bits

[[nodiscard, gnu::flatten]] inline constexpr limb_t
lshift(limb_t *rp, const limb_t *ap, usize n, usize cnt) noexcept
{
#if defined(__micron_arbint_kern_lshift)
  if !consteval {
    return __kern::lshift(rp, ap, n, cnt);
  }
#endif
  return __portable::lshift(rp, ap, n, cnt);
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
rshift(limb_t *rp, const limb_t *ap, usize n, usize cnt) noexcept
{
#if defined(__micron_arbint_kern_rshift)
  if !consteval {
    return __kern::rshift(rp, ap, n, cnt);
  }
#endif
  return __portable::rshift(rp, ap, n, cnt);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise
// UNSELECTED, gcc already autovectorizes

[[gnu::flatten]] inline constexpr void
and_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) rp[i] = static_cast<limb_t>(ap[i] & bp[i]);
}

[[gnu::flatten]] inline constexpr void
ior_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) rp[i] = static_cast<limb_t>(ap[i] | bp[i]);
}

[[gnu::flatten]] inline constexpr void
xor_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) rp[i] = static_cast<limb_t>(ap[i] ^ bp[i]);
}

[[gnu::flatten]] inline constexpr void
com(limb_t *rp, const limb_t *ap, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) rp[i] = static_cast<limb_t>(~ap[i]);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// queries
[[nodiscard, gnu::always_inline]] inline constexpr usize
normalize(const limb_t *p, usize n) noexcept
{
  while ( n > 0 && p[n - 1] == 0 ) --n;
  return n;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
bitlen(const limb_t *p, usize n) noexcept
{
  n = normalize(p, n);
  if ( n == 0 ) return 0;
  return (n - 1u) * limb_bits + limb_bitlen(p[n - 1]);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
popcount(const limb_t *p, usize n) noexcept
{
  usize c = 0;
  for ( usize i = 0; i < n; ++i ) c += limb_popcount(p[i]);
  return c;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
scan1(const limb_t *p, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] != 0 ) return i * limb_bits + limb_ctz(p[i]);
  return n * limb_bits;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
testbit(const limb_t *p, usize n, usize bit) noexcept
{
  const usize i = bit / limb_bits;
  if ( i >= n ) return false;
  return ((p[i] >> (bit % limb_bits)) & limb_t{ 1 }) != 0;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
