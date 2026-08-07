//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../bits/__arch.hpp"
#include "../../../types.hpp"
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
