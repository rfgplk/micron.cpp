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

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// i386 mpn kernels
// 32-bit limbs
//
// unverified yet

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

[[gnu::always_inline]] inline limb_t
__aors_n_idx(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n, bool sub) noexcept
{
  limb_t t0;
  rp += n;
  ap += n;
  bp += n;
  usize i = static_cast<usize>(0) - n;

  if ( sub ) {
    __asm__ volatile("xorl %[t0], %[t0]\n\t"      // CF = 0, and t0 = 0
                     "1:\n\t"
                     "movl (%[ap],%[i],4), %[t0]\n\t"
                     "sbbl (%[bp],%[i],4), %[t0]\n\t"
                     "movl %[t0], (%[rp],%[i],4)\n\t"
                     "incl %[i]\n\t"      // inc leaves CF alone; that is the entire trick
                     "jnz 1b\n\t"
                     "setc %b[t0]\n\t"      // CF here is still the chain's borrow
                     "movzbl %b[t0], %[t0]\n\t"
                     : [t0] "=&q"(t0), [i] "+r"(i)
                     : [rp] "r"(rp), [ap] "r"(ap), [bp] "r"(bp)
                     : "cc", "memory");
  } else {
    __asm__ volatile("xorl %[t0], %[t0]\n\t"
                     "1:\n\t"
                     "movl (%[ap],%[i],4), %[t0]\n\t"
                     "adcl (%[bp],%[i],4), %[t0]\n\t"
                     "movl %[t0], (%[rp],%[i],4)\n\t"
                     "incl %[i]\n\t"
                     "jnz 1b\n\t"
                     "setc %b[t0]\n\t"
                     "movzbl %b[t0], %[t0]\n\t"
                     : [t0] "=&q"(t0), [i] "+r"(i)
                     : [rp] "r"(rp), [ap] "r"(ap), [bp] "r"(bp)
                     : "cc", "memory");
  }
  return t0;
}

[[nodiscard]] inline limb_t
add_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  if ( n >= kern_min_limbs ) return __aors_n_idx(rp, ap, bp, n, false);
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) cy = addc(ap[i], bp[i], cy, rp[i]);
  return cy;
}

[[nodiscard]] inline limb_t
sub_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  if ( n >= kern_min_limbs ) return __aors_n_idx(rp, ap, bp, n, true);
  limb_t bw = 0;
  for ( usize i = 0; i < n; ++i ) bw = subb(ap[i], bp[i], bw, rp[i]);
  return bw;
}

};      // namespace __kern
};      // namespace mpn
};      // namespace math
};      // namespace micron
