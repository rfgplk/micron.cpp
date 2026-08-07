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

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// amd64 mpn kernels
//
//   T3  (ADX opt-in)        mulx + adcx/adox, two carry chains in parallel
//   T2  __micron_x86_bmi2   mulx + one adc chain, folded then applied      <- default
//   T1  neither             portable
//
// adcx writes only CF and adox writes only OF, so an addmul_1 can run the product high accumulation on one flag
// and the rp[] accumulation on the other without the two serialising
//
// T3 IS NOT THE DEFAULT, DEPENDING ON ARCH ADX LOSES TO ADC FORM BENCHMARK ON DIFFERENT ARCHES FOR CONFIRMATION
// (zen+)          portable    T2 mulx+adc    T3 mulx+adcx/adox
//     addmul_1     2.39         1.50            1.67
//     submul_1     2.76         1.49            2.05
// ADX should pay off for zen3/bwl/skl+

#if defined(__micron_x86_bmi2) && defined(__micron_x86_adx)
#if !defined(MICRON_ARBINT_X86_ADX_KERNEL)
#if defined(__znver3__) || defined(__znver4__) || defined(__znver5__) || defined(__skylake__) || defined(__skylake_avx512__)               \
    || defined(__alderlake__) || defined(__icelake_client__) || defined(__icelake_server__) || defined(__sapphirerapids__)
#define MICRON_ARBINT_X86_ADX_KERNEL 1
#else
#define MICRON_ARBINT_X86_ADX_KERNEL 0
#endif
#endif
#if MICRON_ARBINT_X86_ADX_KERNEL
#define __micron_arbint_x86_adx_kernel 1
#endif
#endif

namespace micron
{
namespace math
{
namespace mpn
{
namespace __kern
{

// cycles/limb on znver1:
//
//     n        1     2     3     4     5     6     7     8    10    12    16    32    64
//     speedup 0.88  0.97  1.13  1.29  0.90  0.94  1.02  1.47  1.11  1.51  1.49  1.53  1.53
//
// TODO: add measurements for different arches
inline constexpr usize kern_min_limbs = 8u;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// add / subtract

#define __micron_arbint_kern_add_n 1
#define __micron_arbint_kern_sub_n 1

[[gnu::always_inline]] inline limb_t
__aors_n_blocks(limb_t *rp, const limb_t *ap, const limb_t *bp, usize blocks, bool sub) noexcept
{
  limb_t cy = 0, t0, t1, t2, t3;
  if ( sub ) {
    __asm__ volatile("xorl %k[t0], %k[t0]\n\t"      // CF = 0
                     "1:\n\t"
                     "movq   (%[ap]), %[t0]\n\t"
                     "movq  8(%[ap]), %[t1]\n\t"
                     "movq 16(%[ap]), %[t2]\n\t"
                     "movq 24(%[ap]), %[t3]\n\t"
                     "sbbq   (%[bp]), %[t0]\n\t"
                     "sbbq  8(%[bp]), %[t1]\n\t"
                     "sbbq 16(%[bp]), %[t2]\n\t"
                     "sbbq 24(%[bp]), %[t3]\n\t"
                     "movq %[t0],   (%[rp])\n\t"
                     "movq %[t1],  8(%[rp])\n\t"
                     "movq %[t2], 16(%[rp])\n\t"
                     "movq %[t3], 24(%[rp])\n\t"
                     "leaq 32(%[ap]), %[ap]\n\t"
                     "leaq 32(%[bp]), %[bp]\n\t"
                     "leaq 32(%[rp]), %[rp]\n\t"
                     "leaq -1(%[n]), %[n]\n\t"
                     "jrcxz 2f\n\t"
                     "jmp 1b\n\t"
                     "2:\n\t"
                     "adcq $0, %[cy]\n\t"
                     : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [bp] "+r"(bp), [n] "+c"(blocks), [t0] "=&r"(t0), [t1] "=&r"(t1),
                       [t2] "=&r"(t2), [t3] "=&r"(t3)
                     :
                     : "cc", "memory");
  } else {
    __asm__ volatile("xorl %k[t0], %k[t0]\n\t"
                     "1:\n\t"
                     "movq   (%[ap]), %[t0]\n\t"
                     "movq  8(%[ap]), %[t1]\n\t"
                     "movq 16(%[ap]), %[t2]\n\t"
                     "movq 24(%[ap]), %[t3]\n\t"
                     "adcq   (%[bp]), %[t0]\n\t"
                     "adcq  8(%[bp]), %[t1]\n\t"
                     "adcq 16(%[bp]), %[t2]\n\t"
                     "adcq 24(%[bp]), %[t3]\n\t"
                     "movq %[t0],   (%[rp])\n\t"
                     "movq %[t1],  8(%[rp])\n\t"
                     "movq %[t2], 16(%[rp])\n\t"
                     "movq %[t3], 24(%[rp])\n\t"
                     "leaq 32(%[ap]), %[ap]\n\t"
                     "leaq 32(%[bp]), %[bp]\n\t"
                     "leaq 32(%[rp]), %[rp]\n\t"
                     "leaq -1(%[n]), %[n]\n\t"
                     "jrcxz 2f\n\t"
                     "jmp 1b\n\t"
                     "2:\n\t"
                     "adcq $0, %[cy]\n\t"
                     : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [bp] "+r"(bp), [n] "+c"(blocks), [t0] "=&r"(t0), [t1] "=&r"(t1),
                       [t2] "=&r"(t2), [t3] "=&r"(t3)
                     :
                     : "cc", "memory");
  }
  return cy;
}

[[nodiscard]] inline limb_t
add_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  limb_t cy = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 2) : 0u;
  if ( blocks != 0 ) {
    cy = __aors_n_blocks(rp, ap, bp, blocks, false);
    const usize done = blocks << 2;
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
  const usize blocks = n >= kern_min_limbs ? (n >> 2) : 0u;
  if ( blocks != 0 ) {
    bw = __aors_n_blocks(rp, ap, bp, blocks, true);
    const usize done = blocks << 2;
    rp += done;
    ap += done;
    bp += done;
    n -= done;
  }
  for ( usize i = 0; i < n; ++i ) bw = subb(ap[i], bp[i], bw, rp[i]);
  return bw;
}

#if defined(__micron_x86_bmi2)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// multiply by one limb

#define __micron_arbint_kern_mul_1 1
#define __micron_arbint_kern_addmul_1 1
#define __micron_arbint_kern_submul_1 1

// rp = ap * b + cy, four limbs at a time
[[gnu::always_inline]] inline limb_t
__mul_1_blocks(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t t0, t1, t2, t3;
  __asm__ volatile(
      "xorl %k[t0], %k[t0]\n\t"      // CF = 0
      "1:\n\t"
      "mulx   (%[ap]), %[t0], %[t1]\n\t"
      "mulx  8(%[ap]), %[t2], %[t3]\n\t"
      "adcq %[cy], %[t0]\n\t"
      "adcq %[t1], %[t2]\n\t"
      "movq %[t0],  (%[rp])\n\t"
      "mulx 16(%[ap]), %[t0], %[t1]\n\t"
      "adcq %[t3], %[t0]\n\t"
      "movq %[t2], 8(%[rp])\n\t"
      "mulx 24(%[ap]), %[t2], %[cy]\n\t"
      "adcq %[t1], %[t2]\n\t"
      "adcq $0, %[cy]\n\t"
      "movq %[t0], 16(%[rp])\n\t"
      "movq %[t2], 24(%[rp])\n\t"
      "leaq 32(%[ap]), %[ap]\n\t"
      "leaq 32(%[rp]), %[rp]\n\t"
      "leaq -1(%[n]), %[n]\n\t"
      "jrcxz 2f\n\t"
      "jmp 1b\n\t"
      "2:\n\t"
      : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+c"(blocks), [t0] "=&r"(t0), [t1] "=&r"(t1), [t2] "=&r"(t2), [t3] "=&r"(t3)
      : "d"(b)
      : "cc", "memory");
  return cy;
}

[[nodiscard]] inline limb_t
mul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 2) : 0u;
  if ( blocks != 0 ) {
    cy = __mul_1_blocks(rp, ap, blocks, b, cy);
    const usize done = blocks << 2;
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

#if defined(__micron_arbint_x86_adx_kernel)

// T3
//   adcx (CF)   folds product i-1's HIGH into product i's low
//   adox (OF)   folds rp[i] into product i's low
[[gnu::always_inline]] inline limb_t
__addmul_1_blocks_adx(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t t0, h0, h1, zr;
  __asm__ volatile(
      "xorl %k[zr], %k[zr]\n\t"      // clears CF _and_ OF, and zeroes zr for the fold
      "1:\n\t"
      "mulx   (%[ap]), %[t0], %[h0]\n\t"
      "adcx %[cy], %[t0]\n\t"
      "adox   (%[rp]), %[t0]\n\t"
      "movq %[t0],   (%[rp])\n\t"
      "mulx  8(%[ap]), %[t0], %[h1]\n\t"
      "adcx %[h0], %[t0]\n\t"
      "adox  8(%[rp]), %[t0]\n\t"
      "movq %[t0],  8(%[rp])\n\t"
      "mulx 16(%[ap]), %[t0], %[h0]\n\t"
      "adcx %[h1], %[t0]\n\t"
      "adox 16(%[rp]), %[t0]\n\t"
      "movq %[t0], 16(%[rp])\n\t"
      "mulx 24(%[ap]), %[t0], %[cy]\n\t"
      "adcx %[h0], %[t0]\n\t"
      "adox 24(%[rp]), %[t0]\n\t"
      "movq %[t0], 24(%[rp])\n\t"
      "leaq 32(%[ap]), %[ap]\n\t"
      "leaq 32(%[rp]), %[rp]\n\t"
      "leaq -1(%[n]), %[n]\n\t"
      "jrcxz 2f\n\t"
      "jmp 1b\n\t"
      "2:\n\t"
      "adcx %[zr], %[cy]\n\t"      // fold both chains into the carry word. cannot
      "adox %[zr], %[cy]\n\t"
      : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+c"(blocks), [t0] "=&r"(t0), [h0] "=&r"(h0), [h1] "=&r"(h1), [zr] "=&r"(zr)
      : "d"(b)
      : "cc", "memory");
  return cy;
}

[[gnu::always_inline]] inline limb_t
__submul_1_blocks_adx(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t lo, h0, h1, zr, sd;
  __asm__ volatile("xorl %k[zr], %k[zr]\n\t"      // zr = 0, CF = 0, OF = 0
                   "movl $1, %k[sd]\n\t"          // mov writes no flags
                   "addb $127, %b[sd]\n\t"        // 1 + 127 overflows int8: OF = 1, CF untouched at 0
                   "1:\n\t"
                   "mulx   (%[ap]), %[lo], %[h0]\n\t"
                   "adcx %[cy], %[lo]\n\t"
                   "notq %[lo]\n\t"
                   "adox   (%[rp]), %[lo]\n\t"
                   "movq %[lo],   (%[rp])\n\t"
                   "mulx  8(%[ap]), %[lo], %[h1]\n\t"
                   "adcx %[h0], %[lo]\n\t"
                   "notq %[lo]\n\t"
                   "adox  8(%[rp]), %[lo]\n\t"
                   "movq %[lo],  8(%[rp])\n\t"
                   "mulx 16(%[ap]), %[lo], %[h0]\n\t"
                   "adcx %[h1], %[lo]\n\t"
                   "notq %[lo]\n\t"
                   "adox 16(%[rp]), %[lo]\n\t"
                   "movq %[lo], 16(%[rp])\n\t"
                   "mulx 24(%[ap]), %[lo], %[cy]\n\t"
                   "adcx %[h0], %[lo]\n\t"
                   "notq %[lo]\n\t"
                   "adox 24(%[rp]), %[lo]\n\t"
                   "movq %[lo], 24(%[rp])\n\t"
                   "leaq 32(%[ap]), %[ap]\n\t"
                   "leaq 32(%[rp]), %[rp]\n\t"
                   "leaq -1(%[n]), %[n]\n\t"
                   "jrcxz 2f\n\t"
                   "jmp 1b\n\t"
                   "2:\n\t"
                   "adcx %[zr], %[cy]\n\t"      // cy = H, the product's carry out. adcx leaves OF alone
                   "setno %b[sd]\n\t"           // sd = 1 - carry_out = the borrow
                   "movzbl %b[sd], %k[sd]\n\t"
                   "addq %[sd], %[cy]\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+c"(blocks), [lo] "=&r"(lo), [h0] "=&r"(h0), [h1] "=&r"(h1),
                     [zr] "=&r"(zr), [sd] "=&Q"(sd)
                   : "d"(b)
                   : "cc", "memory");
  return cy;
}

#else      // T2

[[gnu::always_inline]] inline limb_t
__addmul_1_blocks_bmi2(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t l0, l1, l2, l3, h0, h1, h2, t;
  __asm__ volatile("1:\n\t"
                   "mulx   (%[ap]), %[l0], %[h0]\n\t"
                   "mulx  8(%[ap]), %[l1], %[h1]\n\t"
                   "mulx 16(%[ap]), %[l2], %[h2]\n\t"
                   "mulx 24(%[ap]), %[l3], %[t]\n\t"
                   "addq %[cy], %[l0]\n\t"      // fold: lo_i += hi_{i-1}, one chain
                   "adcq %[h0], %[l1]\n\t"
                   "adcq %[h1], %[l2]\n\t"
                   "adcq %[h2], %[l3]\n\t"
                   "adcq $0, %[t]\n\t"
                   "addq %[l0],   (%[rp])\n\t"      // apply: rp[i] += lo_i, a fresh chain
                   "adcq %[l1],  8(%[rp])\n\t"
                   "adcq %[l2], 16(%[rp])\n\t"
                   "adcq %[l3], 24(%[rp])\n\t"
                   "adcq $0, %[t]\n\t"
                   "movq %[t], %[cy]\n\t"
                   "leaq 32(%[ap]), %[ap]\n\t"
                   "leaq 32(%[rp]), %[rp]\n\t"
                   "leaq -1(%[n]), %[n]\n\t"
                   "jrcxz 2f\n\t"
                   "jmp 1b\n\t"
                   "2:\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+c"(blocks), [l0] "=&r"(l0), [l1] "=&r"(l1), [l2] "=&r"(l2),
                     [l3] "=&r"(l3), [h0] "=&r"(h0), [h1] "=&r"(h1), [h2] "=&r"(h2), [t] "=&r"(t)
                   : "d"(b)
                   : "cc", "memory");
  return cy;
}

[[gnu::always_inline]] inline limb_t
__submul_1_blocks_bmi2(limb_t *rp, const limb_t *ap, usize blocks, limb_t b, limb_t cy) noexcept
{
  limb_t l0, l1, l2, l3, h0, h1, h2, t;
  __asm__ volatile("1:\n\t"
                   "mulx   (%[ap]), %[l0], %[h0]\n\t"
                   "mulx  8(%[ap]), %[l1], %[h1]\n\t"
                   "mulx 16(%[ap]), %[l2], %[h2]\n\t"
                   "mulx 24(%[ap]), %[l3], %[t]\n\t"
                   "addq %[cy], %[l0]\n\t"
                   "adcq %[h0], %[l1]\n\t"
                   "adcq %[h1], %[l2]\n\t"
                   "adcq %[h2], %[l3]\n\t"
                   "adcq $0, %[t]\n\t"
                   "subq %[l0],   (%[rp])\n\t"      // the borrow chain, fresh
                   "sbbq %[l1],  8(%[rp])\n\t"
                   "sbbq %[l2], 16(%[rp])\n\t"
                   "sbbq %[l3], 24(%[rp])\n\t"
                   "adcq $0, %[t]\n\t"
                   "movq %[t], %[cy]\n\t"
                   "leaq 32(%[ap]), %[ap]\n\t"
                   "leaq 32(%[rp]), %[rp]\n\t"
                   "leaq -1(%[n]), %[n]\n\t"
                   "jrcxz 2f\n\t"
                   "jmp 1b\n\t"
                   "2:\n\t"
                   : [cy] "+r"(cy), [rp] "+r"(rp), [ap] "+r"(ap), [n] "+c"(blocks), [l0] "=&r"(l0), [l1] "=&r"(l1), [l2] "=&r"(l2),
                     [l3] "=&r"(l3), [h0] "=&r"(h0), [h1] "=&r"(h1), [h2] "=&r"(h2), [t] "=&r"(t)
                   : "d"(b)
                   : "cc", "memory");
  return cy;
}

#endif      // __micron_arbint_x86_adx_kernel

[[nodiscard]] inline limb_t
addmul_1(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  limb_t cy = 0;
  const usize blocks = n >= kern_min_limbs ? (n >> 2) : 0u;
  if ( blocks != 0 ) {
#if defined(__micron_arbint_x86_adx_kernel)
    cy = __addmul_1_blocks_adx(rp, ap, blocks, b, cy);
#else
    cy = __addmul_1_blocks_bmi2(rp, ap, blocks, b, cy);
#endif
    const usize done = blocks << 2;
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
  const usize blocks = n >= kern_min_limbs ? (n >> 2) : 0u;
  if ( blocks != 0 ) {
#if defined(__micron_arbint_x86_adx_kernel)
    cy = __submul_1_blocks_adx(rp, ap, blocks, b, cy);
#else
    cy = __submul_1_blocks_bmi2(rp, ap, blocks, b, cy);
#endif
    const usize done = blocks << 2;
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

#endif      // __micron_x86_bmi2

};      // namespace __kern
};      // namespace mpn
};      // namespace math
};      // namespace micron
