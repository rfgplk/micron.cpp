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

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// armv7-a mpn kernels
// 32-bit limbs
//
// essentially via
//     umaal  Rlo, Rhi, Rn, Rm       ->   Rhi:Rlo = Rn * Rm + Rlo + Rhi
//
// a 32x32 multiply with two 32-bit addends
//
// NOTE: no ldm/stm; the assembler sorts a register list by REGISTER NUMBER, not by the order written, so
// `ldmia %[ap]!, {%[a0], %[a1]}` loads in whatever order gcc's allocation happens to imply
//
// WARNING: THUMB-2 HAZARD. src/linux/sys/syscall_arm32.hpp:87-92 documents a register asm("r7") that breaks
// under -fno-omit-frame-pointer; every operand here is "r"/"=&r" and gcc does the allocation, so the hazard cannot arise

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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// add / subtract
[[gnu::always_inline]] inline limb_t
__add_n_blocks(limb_t *rp, const limb_t *ap, const limb_t *bp, usize blocks) noexcept
{
  limb_t cy = 0, t0, t1;
  __asm__ volatile("cmn %[n], #0\n\t"
                   "1:\n\t"
                   "ldr %[t0], [%[ap]]\n\t"
                   "ldr %[t1], [%[bp]]\n\t"
                   "adcs %[t0], %[t0], %[t1]\n\t"
                   "str %[t0], [%[rp]]\n\t"
                   "ldr %[t0], [%[ap], #4]\n\t"
                   "ldr %[t1], [%[bp], #4]\n\t"
                   "adcs %[t0], %[t0], %[t1]\n\t"
                   "str %[t0], [%[rp], #4]\n\t"
                   "add %[ap], %[ap], #8\n\t"
                   "add %[bp], %[bp], #8\n\t"
                   "add %[rp], %[rp], #8\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "teq %[n], #0\n\t"
                   "bne 1b\n\t"
                   "adc %[cy], %[cy], #0\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [bp] "+r"(bp), [n] "+r"(blocks), [t0] "=&r"(t0), [t1] "=&r"(t1)
                   :
                   : "cc", "memory");
  return cy;
}

[[gnu::always_inline]] inline limb_t
__sub_n_blocks(limb_t *rp, const limb_t *ap, const limb_t *bp, usize blocks) noexcept
{
  limb_t bw = 0, t0, t1;
  __asm__ volatile("cmp %[n], %[n]\n\t"
                   "1:\n\t"
                   "ldr %[t0], [%[ap]]\n\t"
                   "ldr %[t1], [%[bp]]\n\t"
                   "sbcs %[t0], %[t0], %[t1]\n\t"
                   "str %[t0], [%[rp]]\n\t"
                   "ldr %[t0], [%[ap], #4]\n\t"
                   "ldr %[t1], [%[bp], #4]\n\t"
                   "sbcs %[t0], %[t0], %[t1]\n\t"
                   "str %[t0], [%[rp], #4]\n\t"
                   "add %[ap], %[ap], #8\n\t"
                   "add %[bp], %[bp], #8\n\t"
                   "add %[rp], %[rp], #8\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "teq %[n], #0\n\t"
                   "bne 1b\n\t"
                   "sbc %[bw], %[bw], #0\n\t"      // 0 if C (no borrow), -1 otherwise
                   "rsb %[bw], %[bw], #0\n\t"      // -> 0 or 1
                   : [bw] "+r"(bw), [rp] "+r"(rp), [ap] "+r"(ap), [bp] "+r"(bp), [n] "+r"(blocks), [t0] "=&r"(t0), [t1] "=&r"(t1)
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// multiply by one limb

// rp = ap * b + cy
[[gnu::always_inline]] inline limb_t
__mul_1_blocks(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t a0, l0;
  __asm__ volatile("1:\n\t"
                   "ldr %[a0], [%[ap]]\n\t"
                   "mov %[l0], #0\n\t"
                   "umaal %[l0], %[cy], %[a0], %[b]\n\t"
                   "str %[l0], [%[rp]]\n\t"
                   "ldr %[a0], [%[ap], #4]\n\t"
                   "mov %[l0], #0\n\t"
                   "umaal %[l0], %[cy], %[a0], %[b]\n\t"
                   "str %[l0], [%[rp], #4]\n\t"
                   "add %[ap], %[ap], #8\n\t"
                   "add %[rp], %[rp], #8\n\t"
                   "subs %[n], %[n], #1\n\t"
                   "bne 1b\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+r"(blocks), [a0] "=&r"(a0), [l0] "=&r"(l0)
                   : [b] "r"(b)
                   : "cc", "memory");
  return cy;
}

// rp += ap * b; an umaal per limb
[[gnu::always_inline]] inline limb_t
__addmul_1_blocks(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t a0, r0;
  __asm__ volatile("1:\n\t"
                   "ldr %[a0], [%[ap]]\n\t"
                   "ldr %[r0], [%[rp]]\n\t"
                   "umaal %[r0], %[cy], %[a0], %[b]\n\t"
                   "str %[r0], [%[rp]]\n\t"
                   "ldr %[a0], [%[ap], #4]\n\t"
                   "ldr %[r0], [%[rp], #4]\n\t"
                   "umaal %[r0], %[cy], %[a0], %[b]\n\t"
                   "str %[r0], [%[rp], #4]\n\t"
                   "add %[ap], %[ap], #8\n\t"
                   "add %[rp], %[rp], #8\n\t"
                   "subs %[n], %[n], #1\n\t"
                   "bne 1b\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+r"(blocks), [a0] "=&r"(a0), [r0] "=&r"(r0)
                   : [b] "r"(b)
                   : "cc", "memory");
  return cy;
}

// rp -= ap * b
[[gnu::always_inline]] inline limb_t
__submul_1_blocks(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t t0, l0;
  __asm__ volatile("cmp %[n], %[n]\n\t"      // C = 1: no borrow yet
                   "1:\n\t"
                   "ldr %[t0], [%[ap]]\n\t"
                   "mov %[l0], #0\n\t"
                   "umaal %[l0], %[cy], %[t0], %[b]\n\t"
                   "ldr %[t0], [%[rp]]\n\t"
                   "sbcs %[t0], %[t0], %[l0]\n\t"
                   "str %[t0], [%[rp]]\n\t"
                   "ldr %[t0], [%[ap], #4]\n\t"
                   "mov %[l0], #0\n\t"
                   "umaal %[l0], %[cy], %[t0], %[b]\n\t"
                   "ldr %[t0], [%[rp], #4]\n\t"
                   "sbcs %[t0], %[t0], %[l0]\n\t"
                   "str %[t0], [%[rp], #4]\n\t"
                   "add %[ap], %[ap], #8\n\t"
                   "add %[rp], %[rp], #8\n\t"
                   "sub %[n], %[n], #1\n\t"
                   "teq %[n], #0\n\t"
                   "bne 1b\n\t"
                   "sbc %[l0], %[l0], %[l0]\n\t"      // 0 if C (no borrow), -1 if borrow
                   "sub %[cy], %[cy], %[l0]\n\t"      // cy += borrow
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+r"(blocks), [t0] "=&r"(t0), [l0] "=&r"(l0)
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
