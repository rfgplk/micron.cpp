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
// carry free mpn kernels, vectorized

#if defined(__micron_x86_avx2)
#define __micron_arbint_vec_bytes 32
#elif defined(__micron_x86_sse2) || defined(__micron_arm_neon)
#define __micron_arbint_vec_bytes 16
#endif

#if defined(__micron_arbint_vec_bytes)

namespace micron
{
namespace math
{
namespace mpn
{
namespace __kern
{

typedef limb_t vlimb __attribute__((vector_size(__micron_arbint_vec_bytes)));
typedef slimb_t vslimb __attribute__((vector_size(__micron_arbint_vec_bytes)));

inline constexpr usize vec_lanes = __micron_arbint_vec_bytes / sizeof(limb_t);

[[nodiscard, gnu::always_inline]] inline vlimb
vld(const limb_t *p) noexcept
{
  vlimb v;
  __builtin_memcpy(&v, p, sizeof(v));
  return v;
}

[[gnu::always_inline]] inline void
vst(limb_t *p, vlimb v) noexcept
{
  __builtin_memcpy(p, &v, sizeof(v));
}

[[nodiscard, gnu::always_inline]] inline vslimb
vsigned(vlimb v) noexcept
{
  vslimb out;
  __builtin_memcpy(&out, &v, sizeof(out));
  return out;
}

[[nodiscard, gnu::always_inline]] inline vlimb
vunsigned(vslimb v) noexcept
{
  vlimb out;
  __builtin_memcpy(&out, &v, sizeof(out));
  return out;
}

[[gnu::always_inline]] inline void
__transpose_square(limb_t *rp, usize rs, const limb_t *ap, usize as) noexcept
{
#if defined(__micron_arch_width_64) && __micron_arbint_vec_bytes == 16
  const vlimb x0 = vld(ap);
  const vlimb x1 = vld(ap + as);
  vst(rp, __builtin_shufflevector(x0, x1, 0, 2));
  vst(rp + rs, __builtin_shufflevector(x0, x1, 1, 3));
#elif (defined(__micron_arch_width_64) && __micron_arbint_vec_bytes == 32)                                                                 \
    || (defined(__micron_arch_width_32) && __micron_arbint_vec_bytes == 16)
  const vlimb x0 = vld(ap);
  const vlimb x1 = vld(ap + as);
  const vlimb x2 = vld(ap + 2u * as);
  const vlimb x3 = vld(ap + 3u * as);
  const vlimb t0 = __builtin_shufflevector(x0, x1, 0, 4, 1, 5);
  const vlimb t1 = __builtin_shufflevector(x0, x1, 2, 6, 3, 7);
  const vlimb t2 = __builtin_shufflevector(x2, x3, 0, 4, 1, 5);
  const vlimb t3 = __builtin_shufflevector(x2, x3, 2, 6, 3, 7);
  vst(rp, __builtin_shufflevector(t0, t2, 0, 1, 4, 5));
  vst(rp + rs, __builtin_shufflevector(t0, t2, 2, 3, 6, 7));
  vst(rp + 2u * rs, __builtin_shufflevector(t1, t3, 0, 1, 4, 5));
  vst(rp + 3u * rs, __builtin_shufflevector(t1, t3, 2, 3, 6, 7));
#else
  const vlimb x0 = vld(ap);
  const vlimb x1 = vld(ap + as);
  const vlimb x2 = vld(ap + 2u * as);
  const vlimb x3 = vld(ap + 3u * as);
  const vlimb x4 = vld(ap + 4u * as);
  const vlimb x5 = vld(ap + 5u * as);
  const vlimb x6 = vld(ap + 6u * as);
  const vlimb x7 = vld(ap + 7u * as);
  const vlimb a0 = __builtin_shufflevector(x0, x1, 0, 8, 1, 9, 2, 10, 3, 11);
  const vlimb a1 = __builtin_shufflevector(x0, x1, 4, 12, 5, 13, 6, 14, 7, 15);
  const vlimb a2 = __builtin_shufflevector(x2, x3, 0, 8, 1, 9, 2, 10, 3, 11);
  const vlimb a3 = __builtin_shufflevector(x2, x3, 4, 12, 5, 13, 6, 14, 7, 15);
  const vlimb a4 = __builtin_shufflevector(x4, x5, 0, 8, 1, 9, 2, 10, 3, 11);
  const vlimb a5 = __builtin_shufflevector(x4, x5, 4, 12, 5, 13, 6, 14, 7, 15);
  const vlimb a6 = __builtin_shufflevector(x6, x7, 0, 8, 1, 9, 2, 10, 3, 11);
  const vlimb a7 = __builtin_shufflevector(x6, x7, 4, 12, 5, 13, 6, 14, 7, 15);
  const vlimb b0 = __builtin_shufflevector(a0, a2, 0, 1, 8, 9, 2, 3, 10, 11);
  const vlimb b1 = __builtin_shufflevector(a0, a2, 4, 5, 12, 13, 6, 7, 14, 15);
  const vlimb b2 = __builtin_shufflevector(a4, a6, 0, 1, 8, 9, 2, 3, 10, 11);
  const vlimb b3 = __builtin_shufflevector(a4, a6, 4, 5, 12, 13, 6, 7, 14, 15);
  const vlimb b4 = __builtin_shufflevector(a1, a3, 0, 1, 8, 9, 2, 3, 10, 11);
  const vlimb b5 = __builtin_shufflevector(a1, a3, 4, 5, 12, 13, 6, 7, 14, 15);
  const vlimb b6 = __builtin_shufflevector(a5, a7, 0, 1, 8, 9, 2, 3, 10, 11);
  const vlimb b7 = __builtin_shufflevector(a5, a7, 4, 5, 12, 13, 6, 7, 14, 15);
  vst(rp, __builtin_shufflevector(b0, b2, 0, 1, 2, 3, 8, 9, 10, 11));
  vst(rp + rs, __builtin_shufflevector(b0, b2, 4, 5, 6, 7, 12, 13, 14, 15));
  vst(rp + 2u * rs, __builtin_shufflevector(b1, b3, 0, 1, 2, 3, 8, 9, 10, 11));
  vst(rp + 3u * rs, __builtin_shufflevector(b1, b3, 4, 5, 6, 7, 12, 13, 14, 15));
  vst(rp + 4u * rs, __builtin_shufflevector(b4, b6, 0, 1, 2, 3, 8, 9, 10, 11));
  vst(rp + 5u * rs, __builtin_shufflevector(b4, b6, 4, 5, 6, 7, 12, 13, 14, 15));
  vst(rp + 6u * rs, __builtin_shufflevector(b5, b7, 0, 1, 2, 3, 8, 9, 10, 11));
  vst(rp + 7u * rs, __builtin_shufflevector(b5, b7, 4, 5, 6, 7, 12, 13, 14, 15));
#endif
}

inline void
coeff_transpose_square(limb_t *rlo, limb_t *rhi, usize rs, const limb_t *alo, const limb_t *ahi, usize as) noexcept
{
  __transpose_square(rlo, rs, alo, as);
  __transpose_square(rhi, rs, ahi, as);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// independent two-limb coefficient lanes

#define __micron_arbint_kern_coefficients 1

inline void
coeff_butterfly_to(limb_t *alo, limb_t *ahi, const limb_t *blo, const limb_t *bhi, limb_t *dlo, limb_t *dhi, usize n) noexcept
{
  usize i = 0u;
  vlimb zero{};
  const vlimb one = zero + 1u;
  for ( ; i + vec_lanes <= n; i += vec_lanes ) {
    const vlimb al = vld(alo + i);
    const vlimb ah = vld(ahi + i);
    const vlimb bl = vld(blo + i);
    const vlimb bh = vld(bhi + i);
    const vlimb sum = al + bl;
    const vlimb diff = al - bl;
    const vlimb cy = static_cast<vlimb>(sum < al) & one;
    const vlimb bw = static_cast<vlimb>(al < bl) & one;
    vst(alo + i, sum);
    vst(ahi + i, ah + bh + cy);
    vst(dlo + i, diff);
    vst(dhi + i, ah - bh - bw);
  }
  for ( ; i < n; ++i ) {
    const limb_t al = alo[i];
    const limb_t ah = ahi[i];
    const limb_t bl = blo[i];
    const limb_t bh = bhi[i];
    limb_t sum = 0u, diff = 0u;
    const limb_t cy = addc(al, bl, 0u, sum);
    const limb_t bw = subb(al, bl, 0u, diff);
    alo[i] = sum;
    (void)addc(ah, bh, cy, ahi[i]);
    dlo[i] = diff;
    (void)subb(ah, bh, bw, dhi[i]);
  }
}

inline void
coeff_butterfly(limb_t *alo, limb_t *ahi, limb_t *blo, limb_t *bhi, usize n) noexcept
{
  coeff_butterfly_to(alo, ahi, blo, bhi, blo, bhi, n);
}

inline void
coeff_add(limb_t *rlo, limb_t *rhi, const limb_t *alo, const limb_t *ahi, const limb_t *blo, const limb_t *bhi, usize n) noexcept
{
  usize i = 0u;
  vlimb zero{};
  const vlimb one = zero + 1u;
  for ( ; i + vec_lanes <= n; i += vec_lanes ) {
    const vlimb al = vld(alo + i);
    const vlimb sum = al + vld(blo + i);
    const vlimb cy = static_cast<vlimb>(sum < al) & one;
    vst(rlo + i, sum);
    vst(rhi + i, vld(ahi + i) + vld(bhi + i) + cy);
  }
  for ( ; i < n; ++i ) {
    limb_t lo = 0u;
    const limb_t cy = addc(alo[i], blo[i], 0u, lo);
    rlo[i] = lo;
    (void)addc(ahi[i], bhi[i], cy, rhi[i]);
  }
}

inline void
coeff_copy_cneg(limb_t *rlo, limb_t *rhi, const limb_t *alo, const limb_t *ahi, usize n, bool negate) noexcept
{
  usize i = 0u;
  if ( !negate ) {
    for ( ; i + vec_lanes <= n; i += vec_lanes ) {
      vst(rlo + i, vld(alo + i));
      vst(rhi + i, vld(ahi + i));
    }
    for ( ; i < n; ++i ) {
      rlo[i] = alo[i];
      rhi[i] = ahi[i];
    }
    return;
  }

  vlimb zero{};
  for ( ; i + vec_lanes <= n; i += vec_lanes ) {
    const vlimb al = vld(alo + i);
    const vlimb ah = vld(ahi + i);
    const vlimb lo = zero - al;
    const vlimb zmask = static_cast<vlimb>(al == zero);
    vst(rlo + i, lo);
    vst(rhi + i, ~ah - zmask);
  }
  for ( ; i < n; ++i ) {
    const limb_t lo = static_cast<limb_t>(0u - alo[i]);
    rlo[i] = lo;
    rhi[i] = static_cast<limb_t>(~ahi[i] + static_cast<limb_t>(alo[i] == 0u));
  }
}

[[nodiscard]] inline bool
coeff_divexact_pow2(limb_t *lo, limb_t *hi, usize n, usize shift) noexcept
{
  if ( shift == 0u ) return true;
  const limb_t smask = static_cast<limb_t>((limb_t{ 1 } << shift) - 1u);
  vlimb zero{};
  const vlimb mask = zero + smask;
  vlimb bad{};
  usize i = 0u;
  for ( ; i + vec_lanes <= n; i += vec_lanes ) {
    const vlimb l = vld(lo + i);
    const vlimb h = vld(hi + i);
    bad |= l & mask;
    vst(lo + i, static_cast<vlimb>(l >> shift) | static_cast<vlimb>(h << (limb_bits - shift)));
    vst(hi + i, vunsigned(vsigned(h) >> shift));
  }
  bool exact = true;
  for ( usize lane = 0u; lane < vec_lanes; ++lane ) exact = bad[lane] == 0u && exact;
  for ( ; i < n; ++i ) {
    exact = (lo[i] & smask) == 0u && exact;
    const limb_t sign = static_cast<limb_t>(hi[i] >> (limb_bits - 1u));
    const limb_t sign_mask = static_cast<limb_t>(0u - sign);
    lo[i] = static_cast<limb_t>((lo[i] >> shift) | (hi[i] << (limb_bits - shift)));
    hi[i] = static_cast<limb_t>((hi[i] >> shift) | (sign_mask << (limb_bits - shift)));
  }
  return exact;
}

// %%%%%%%%%%%%%%%
// shifts

#define __micron_arbint_kern_lshift 1
#define __micron_arbint_kern_rshift 1

[[nodiscard]] inline limb_t
lshift(limb_t *rp, const limb_t *ap, usize n, usize cnt) noexcept
{
  if ( n == 0 ) return 0;
  const usize tnc = limb_bits - cnt;
  const limb_t ret = static_cast<limb_t>(ap[n - 1] >> tnc);

  usize i = n - 1u;
  // each pass writes the block ending at i and reads one limb below it, so it needs ap[i - lanes]
  while ( i + 1u >= vec_lanes + 1u ) {
    const usize base = i + 1u - vec_lanes;      // the block is [base, i]
    const vlimb hi = vld(ap + base);
    const vlimb lo = vld(ap + base - 1u);
    vst(rp + base, static_cast<vlimb>(hi << cnt) | static_cast<vlimb>(lo >> tnc));
    i -= vec_lanes;
  }
  for ( ; i > 0; --i ) rp[i] = static_cast<limb_t>((ap[i] << cnt) | (ap[i - 1] >> tnc));
  rp[0] = static_cast<limb_t>(ap[0] << cnt);
  return ret;
}

[[nodiscard]] inline limb_t
rshift(limb_t *rp, const limb_t *ap, usize n, usize cnt) noexcept
{
  if ( n == 0 ) return 0;
  const usize tnc = limb_bits - cnt;
  const limb_t ret = static_cast<limb_t>(ap[0] << tnc);

  usize i = 0;
  // needs ap[i + lanes], so stop while that index is still inside the operand
  while ( i + vec_lanes < n ) {
    const vlimb lo = vld(ap + i);
    const vlimb hi = vld(ap + i + 1u);
    vst(rp + i, static_cast<vlimb>(lo >> cnt) | static_cast<vlimb>(hi << tnc));
    i += vec_lanes;
  }
  for ( ; i + 1u < n; ++i ) rp[i] = static_cast<limb_t>((ap[i] >> cnt) | (ap[i + 1] << tnc));
  rp[n - 1u] = static_cast<limb_t>(ap[n - 1u] >> cnt);
  return ret;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise
//
// gcc already autovectorizes down to this, 0 gain

[[gnu::always_inline]] inline void
__logic3(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n, int op) noexcept
{
  usize i = 0;
  for ( ; i + vec_lanes <= n; i += vec_lanes ) {
    const vlimb a = vld(ap + i);
    const vlimb b = vld(bp + i);
    vst(rp + i, op == 0 ? (a & b) : (op == 1 ? (a | b) : (a ^ b)));
  }
  for ( ; i < n; ++i )
    rp[i] = op == 0 ? static_cast<limb_t>(ap[i] & bp[i])
                    : (op == 1 ? static_cast<limb_t>(ap[i] | bp[i]) : static_cast<limb_t>(ap[i] ^ bp[i]));
}

inline void
and_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  __logic3(rp, ap, bp, n, 0);
}

inline void
ior_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  __logic3(rp, ap, bp, n, 1);
}

inline void
xor_n(limb_t *rp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  __logic3(rp, ap, bp, n, 2);
}

inline void
com(limb_t *rp, const limb_t *ap, usize n) noexcept
{
  usize i = 0;
  vlimb zero = {};
  const vlimb ones = ~zero;
  for ( ; i + vec_lanes <= n; i += vec_lanes ) vst(rp + i, vld(ap + i) ^ ones);
  for ( ; i < n; ++i ) rp[i] = static_cast<limb_t>(~ap[i]);
}

};      // namespace __kern
};      // namespace mpn
};      // namespace math
};      // namespace micron

#endif      // __micron_arbint_vec_bytes
