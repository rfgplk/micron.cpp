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
// SIMD MULTIPLY EXPERIMENT
//
// decomposition, per limb, with M = 2^32 - 1:
//     m0 = a_lo*b_lo   m1 = a_lo*b_hi   m2 = a_hi*b_lo   m3 = a_hi*b_hi
//
//     t  = m1 + (m0 >> 32)          <= 2^64 - 2^33 + 2^32, so it cannot wrap
//     u  = (t & M) + m2             same bound, same reason
//     lo = (u << 32) | (m0 & M)
//     hi = m3 + (t >> 32) + (u >> 32)

#if defined(__micron_x86_avx2) && defined(__micron_arch_width_64)

#define __micron_arbint_have_simd_mul_experiment 1

namespace micron
{
namespace math
{
namespace mpn
{
namespace __kern
{

typedef long long __ai_v4di __attribute__((vector_size(32)));
typedef int __ai_v8si __attribute__((vector_size(32)));
typedef unsigned long long __ai_v4du __attribute__((vector_size(32)));

// vpmuludq
[[nodiscard, gnu::always_inline]] inline __ai_v4du
__vmul32(__ai_v4du a, __ai_v4du b) noexcept
{
  return (__ai_v4du)__builtin_ia32_pmuludq256((__ai_v8si)a, (__ai_v8si)b);
}

// rp = ap * b + cy
[[nodiscard]] inline limb_t
mul_1_avx2(limb_t *rp, const limb_t *ap, usize n, limb_t b) noexcept
{
  const __ai_v4du mask = { 0xFFFFFFFFull, 0xFFFFFFFFull, 0xFFFFFFFFull, 0xFFFFFFFFull };
  const limb_t bl = b & 0xFFFFFFFFull;
  const limb_t bh = b >> 32;
  const __ai_v4du vbl = { bl, bl, bl, bl };
  const __ai_v4du vbh = { bh, bh, bh, bh };

  limb_t cy = 0;
  usize i = 0;
  alignas(32) limb_t lo[4], hi[4];

  for ( ; i + 4u <= n; i += 4u ) {
    __ai_v4du va;
    __builtin_memcpy(&va, ap + i, sizeof(va));
    const __ai_v4du val = va & mask;
    const __ai_v4du vah = va >> 32;

    const __ai_v4du m0 = __vmul32(val, vbl);
    const __ai_v4du m1 = __vmul32(val, vbh);
    const __ai_v4du m2 = __vmul32(vah, vbl);
    const __ai_v4du m3 = __vmul32(vah, vbh);

    const __ai_v4du t = m1 + (m0 >> 32);
    const __ai_v4du u = (t & mask) + m2;
    const __ai_v4du vlo = (u << 32) | (m0 & mask);
    const __ai_v4du vhi = m3 + (t >> 32) + (u >> 32);

    __builtin_memcpy(lo, &vlo, sizeof(vlo));
    __builtin_memcpy(hi, &vhi, sizeof(vhi));

    for ( usize j = 0; j < 4u; ++j ) {
      limb_t s = 0;
      const limb_t c = addc(lo[j], cy, 0, s);
      rp[i + j] = s;
      cy = static_cast<limb_t>(hi[j] + c);
    }
  }

  for ( ; i < n; ++i ) {
    limb_t l = 0, h = 0;
    muladd_wide(ap[i], b, cy, l, h);
    rp[i] = l;
    cy = h;
  }
  return cy;
}

};      // namespace __kern
};      // namespace mpn
};      // namespace math
};      // namespace micron

#endif      // __micron_x86_avx2 && width 64
