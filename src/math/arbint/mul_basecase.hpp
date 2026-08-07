//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "bits/carry.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// O(n^2): schoolbook, comba, and comba unrolled at a fixed width
//
//   mul_basecase   ROW-major (operand scanning)
//   mul_comba      COLUMN-major (product scanning)
//
// NOTE: rp is distinct from ap and bp and holds an + bn limbs
// an >= bn >= 1

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// row major (operand scanning)

[[gnu::flatten]] inline constexpr void
mul_basecase(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn) noexcept
{
  rp[an] = mul_1(rp, ap, an, bp[0]);
  for ( usize j = 1; j < bn; ++j ) rp[an + j] = addmul_1(rp + j, ap, an, bp[j]);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// column major (product scanning)

[[gnu::flatten]] inline constexpr void
mul_comba(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn) noexcept
{
  const usize rn = an + bn;
  acc3 acc;
  for ( usize k = 0; k < rn; ++k ) {
    const usize ilo = (k + 1u > bn) ? (k + 1u - bn) : 0u;
    const usize ihi = (k < an - 1u) ? k : (an - 1u);
    for ( usize i = ilo; i <= ihi; ++i ) acc.fma(ap[i], bp[k - i]);
    rp[k] = acc.shift_out();
  }
}

template<usize AN, usize BN>
  requires(AN >= 1 && BN >= 1)
[[gnu::flatten]] inline constexpr void
mul_comba_fixed(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, const limb_t *__restrict__ bp) noexcept
{
  constexpr usize RN = AN + BN;
  acc3 acc;
  for ( usize k = 0; k < RN; ++k ) {
    const usize ilo = (k + 1u > BN) ? (k + 1u - BN) : 0u;
    const usize ihi = (k < AN - 1u) ? k : (AN - 1u);
    for ( usize i = ilo; i <= ihi; ++i ) acc.fma(ap[i], bp[k - i]);
    rp[k] = acc.shift_out();
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// squaring
//
// a^2 == 2 * sum_{i<j} a_i a_j B^(i+j)  +  sum_i a_i^2 B^(2i)
[[gnu::flatten]] inline constexpr void
sqr_basecase(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n) noexcept
{
  if ( n == 1 ) {
    mul_wide(ap[0], ap[0], rp[0], rp[1]);
    return;
  }

  const usize rn = 2u * n;

  // strict upper triangle
  rp[0] = 0;
  rp[n] = mul_1(rp + 1, ap + 1, n - 1u, ap[0]);
  for ( usize i = 1; i + 1u < n; ++i ) rp[n + i] = addmul_1(rp + 2u * i + 1u, ap + i + 1u, n - i - 1u, ap[i]);
  rp[rn - 1u] = 0;

  limb_t bit = 0;
  for ( usize i = 0; i < rn; ++i ) {
    const limb_t w = rp[i];
    rp[i] = static_cast<limb_t>((w << 1) | bit);
    bit = static_cast<limb_t>(w >> (limb_bits - 1u));
  }

  // diagonal
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) {
    limb_t lo = 0, hi = 0, t = 0;
    mul_wide(ap[i], ap[i], lo, hi);
    const limb_t c0 = addc(rp[2u * i], lo, cy, t);
    rp[2u * i] = t;
    cy = addc(rp[2u * i + 1u], hi, c0, t);
    rp[2u * i + 1u] = t;
  }
}

[[gnu::flatten]] inline constexpr void
sqr_comba(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n) noexcept
{
  const usize rn = 2u * n;
  acc3 acc;
  for ( usize k = 0; k < rn; ++k ) {
    const usize ilo = (k + 1u > n) ? (k + 1u - n) : 0u;
    const usize ihi = (k < n - 1u) ? k : (n - 1u);
    for ( usize i = ilo; i <= ihi && 2u * i < k; ++i ) acc.fma2(ap[i], ap[k - i]);
    if ( (k & 1u) == 0u && (k >> 1) <= ihi ) acc.fma(ap[k >> 1], ap[k >> 1]);
    rp[k] = acc.shift_out();
  }
}

template<usize N>
  requires(N >= 1)
[[gnu::flatten]] inline constexpr void
sqr_comba_fixed(limb_t *__restrict__ rp, const limb_t *__restrict__ ap) noexcept
{
  constexpr usize RN = 2u * N;
  acc3 acc;
  for ( usize k = 0; k < RN; ++k ) {
    const usize ilo = (k + 1u > N) ? (k + 1u - N) : 0u;
    const usize ihi = (k < N - 1u) ? k : (N - 1u);
    for ( usize i = ilo; i <= ihi && 2u * i < k; ++i ) acc.fma2(ap[i], ap[k - i]);
    if ( (k & 1u) == 0u && (k >> 1) <= ihi ) acc.fma(ap[k >> 1], ap[k >> 1]);
    rp[k] = acc.shift_out();
  }
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
