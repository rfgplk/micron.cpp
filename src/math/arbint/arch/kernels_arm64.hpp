//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../bits/__arch.hpp"
#include "../../../types.hpp"
#include "../bits/carry.hpp"
#include "../limb.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// aarch64 mpn kernels
// 64-bit limbs
//
// UNVERIFIED FOR NOW
// aarch64 has no widening multiply-accumulate and no 64x64 in NEON
//     mul   lo, a, b        ; low 64
//     umulh hi, a, b        ; high 64
//

namespace micron
{
namespace math
{
namespace mpn
{
namespace __kern
{

inline constexpr usize kern_min_limbs = 8u;

#define __micron_arbint_kern_add_n 1
#define __micron_arbint_kern_sub_n 1
#define __micron_arbint_kern_mul_1 1
#define __micron_arbint_kern_addmul_1 1
#define __micron_arbint_kern_submul_1 1

// %%%%%%%%%%%%%%%%%%%%
// add / subtract
[[gnu::always_inline]] inline limb_t
__add_n_blocks(limb_t *rp, const limb_t *ap, const limb_t *bp, usize blocks) noexcept
{
  limb_t cy = 0, a0, a1, b0, b1;
  __asm__ volatile("adds xzr, xzr, xzr\n\t"      // C = 0
                   "1:\n\t"
                   "ldp %[a0], %[a1], [%[ap]], #16\n\t"
                   "ldp %[b0], %[b1], [%[bp]], #16\n\t"
                   "adcs %[a0], %[a0], %[b0]\n\t"
                   "adcs %[a1], %[a1], %[b1]\n\t"
                   "stp %[a0], %[a1], [%[rp]], #16\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "cbnz %[n], 1b\n\t"
                   "cset %[cy], cs\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [bp] "+r"(bp), [n] "+r"(blocks), [a0] "=&r"(a0), [a1] "=&r"(a1),
                     [b0] "=&r"(b0), [b1] "=&r"(b1)
                   :
                   : "cc", "memory");
  return cy;
}

[[gnu::always_inline]] inline limb_t
__sub_n_blocks(limb_t *rp, const limb_t *ap, const limb_t *bp, usize blocks) noexcept
{
  limb_t bw = 0, a0, a1, b0, b1;
  __asm__ volatile("subs xzr, xzr, xzr\n\t"      // C = 1, i.e. no borrow
                   "1:\n\t"
                   "ldp %[a0], %[a1], [%[ap]], #16\n\t"
                   "ldp %[b0], %[b1], [%[bp]], #16\n\t"
                   "sbcs %[a0], %[a0], %[b0]\n\t"
                   "sbcs %[a1], %[a1], %[b1]\n\t"
                   "stp %[a0], %[a1], [%[rp]], #16\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "cbnz %[n], 1b\n\t"
                   "cset %[bw], cc\n\t"      // carry CLEAR means a borrow happened
                   : [bw] "+r"(bw), [rp] "+r"(rp), [ap] "+r"(ap), [bp] "+r"(bp), [n] "+r"(blocks), [a0] "=&r"(a0), [a1] "=&r"(a1),
                     [b0] "=&r"(b0), [b1] "=&r"(b1)
                   :
                   : "cc", "memory");
  return bw;
}

[[nodiscard]] inline limb_t
add_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  limb_t cy = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 1) : 0u;
  if ( blocks != 0 ) {
    cy = __add_n_blocks(rp, ap, bp, blocks);
    const usize done = blocks << 1;
    rp += done;
    ap += done;
    bp += done;
    n -= done;
  }
  for ( usize i = 0; i < n; ++i ) cy = addc(ap[i], bp[i], cy, rp[i]);
  return cy;
}

[[nodiscard]] inline limb_t
sub_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  limb_t bw = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 1) : 0u;
  if ( blocks != 0 ) {
    bw = __sub_n_blocks(rp, ap, bp, blocks);
    const usize done = blocks << 1;
    rp += done;
    ap += done;
    bp += done;
    n -= done;
  }
  for ( usize i = 0; i < n; ++i ) bw = subb(ap[i], bp[i], bw, rp[i]);
  return bw;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// multiply by one limb

[[gnu::always_inline]] inline limb_t
__mul_1_blocks(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t a0, a1, l0, l1, h0, h1;
  __asm__ volatile("1:\n\t"
                   "ldp %[a0], %[a1], [%[ap]], #16\n\t"
                   "mul %[l0], %[a0], %[b]\n\t"
                   "umulh %[h0], %[a0], %[b]\n\t"
                   "mul %[l1], %[a1], %[b]\n\t"
                   "umulh %[h1], %[a1], %[b]\n\t"
                   "adds %[l0], %[l0], %[cy]\n\t"
                   "cinc %[h0], %[h0], cs\n\t"
                   "adds %[l1], %[l1], %[h0]\n\t"
                   "cinc %[cy], %[h1], cs\n\t"
                   "stp %[l0], %[l1], [%[rp]], #16\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "cbnz %[n], 1b\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+r"(blocks), [a0] "=&r"(a0), [a1] "=&r"(a1), [l0] "=&r"(l0),
                     [l1] "=&r"(l1), [h0] "=&r"(h0), [h1] "=&r"(h1)
                   : [b] "r"(b)
                   : "cc", "memory");
  return cy;
}

// rp += ap * b; two accumulations per limb
[[gnu::always_inline]] inline limb_t
__addmul_1_blocks(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t a0, a1, l0, l1, h0, h1, d0, d1;
  __asm__ volatile("1:\n\t"
                   "ldp %[a0], %[a1], [%[ap]], #16\n\t"
                   "ldp %[d0], %[d1], [%[rp]]\n\t"
                   "mul %[l0], %[a0], %[b]\n\t"
                   "umulh %[h0], %[a0], %[b]\n\t"
                   "mul %[l1], %[a1], %[b]\n\t"
                   "umulh %[h1], %[a1], %[b]\n\t"
                   "adds %[l0], %[l0], %[cy]\n\t"      // fold the incoming carry
                   "cinc %[h0], %[h0], cs\n\t"
                   "adds %[l1], %[l1], %[h0]\n\t"      // fold product 0's high
                   "cinc %[h1], %[h1], cs\n\t"
                   "adds %[l0], %[l0], %[d0]\n\t"      // now add rp[], a fresh chain
                   "adcs %[l1], %[l1], %[d1]\n\t"
                   "cinc %[cy], %[h1], cs\n\t"
                   "stp %[l0], %[l1], [%[rp]], #16\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "cbnz %[n], 1b\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+r"(blocks), [a0] "=&r"(a0), [a1] "=&r"(a1), [l0] "=&r"(l0),
                     [l1] "=&r"(l1), [h0] "=&r"(h0), [h1] "=&r"(h1), [d0] "=&r"(d0), [d1] "=&r"(d1)
                   : [b] "r"(b)
                   : "cc", "memory");
  return cy;
}

// rp -= ap * b
[[gnu::always_inline]] inline limb_t
__submul_1_blocks(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t a0, a1, l0, l1, h0, h1, d0, d1;
  __asm__ volatile("1:\n\t"
                   "ldp %[a0], %[a1], [%[ap]], #16\n\t"
                   "ldp %[d0], %[d1], [%[rp]]\n\t"
                   "mul %[l0], %[a0], %[b]\n\t"
                   "umulh %[h0], %[a0], %[b]\n\t"
                   "mul %[l1], %[a1], %[b]\n\t"
                   "umulh %[h1], %[a1], %[b]\n\t"
                   "adds %[l0], %[l0], %[cy]\n\t"
                   "cinc %[h0], %[h0], cs\n\t"
                   "adds %[l1], %[l1], %[h0]\n\t"
                   "cinc %[h1], %[h1], cs\n\t"
                   "subs %[d0], %[d0], %[l0]\n\t"
                   "sbcs %[d1], %[d1], %[l1]\n\t"
                   "cinc %[cy], %[h1], cc\n\t"      // carry clear == borrow out
                   "stp %[d0], %[d1], [%[rp]], #16\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "cbnz %[n], 1b\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+r"(blocks), [a0] "=&r"(a0), [a1] "=&r"(a1), [l0] "=&r"(l0),
                     [l1] "=&r"(l1), [h0] "=&r"(h0), [h1] "=&r"(h1), [d0] "=&r"(d0), [d1] "=&r"(d1)
                   : [b] "r"(b)
                   : "cc", "memory");
  return cy;
}

[[nodiscard]] inline limb_t
mul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 1) : 0u;
  if ( blocks != 0 ) {
    cy = __mul_1_blocks(rp, ap, blocks, b, cy);
    const usize done = blocks << 1;
    rp += done;
    ap += done;
    n -= done;
  }
  for ( usize i = 0; i < n; ++i ) {
    limb_t lo = 0, hi = 0;
    muladd_wide(ap[i], b, cy, lo, hi);
    rp[i] = lo;
    cy = hi;
  }
  return cy;
}

[[nodiscard]] inline limb_t
addmul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 1) : 0u;
  if ( blocks != 0 ) {
    cy = __addmul_1_blocks(rp, ap, blocks, b, cy);
    const usize done = blocks << 1;
    rp += done;
    ap += done;
    n -= done;
  }
  for ( usize i = 0; i < n; ++i ) {
    limb_t lo = 0, hi = 0;
    muladd2_wide(ap[i], b, cy, rp[i], lo, hi);
    rp[i] = lo;
    cy = hi;
  }
  return cy;
}

[[nodiscard]] inline limb_t
submul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 1) : 0u;
  if ( blocks != 0 ) {
    cy = __submul_1_blocks(rp, ap, blocks, b, cy);
    const usize done = blocks << 1;
    rp += done;
    ap += done;
    n -= done;
  }
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

};      // namespace __kern
};      // namespace mpn
};      // namespace math
};      // namespace micron
