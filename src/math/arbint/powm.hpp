//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div.hpp"
#include "div_mu.hpp"
#include "limb.hpp"
#include "mont.hpp"
#include "mpn_core.hpp"
#include "mul.hpp"
#include "tags.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// modular exponentiation
// sliding window over Montgomery, or over Barrett for an even modulus
//
// WARNING: NOT CONSTANT TIME

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Barrett

struct barrett_ctx {
  const limb_t *mp;      // n limbs, normalized
  const limb_t *ip;      // n limbs; the reciprocal is B^n + ip
  usize n;
  usize shift;      // 0 .. limb_bits-1
};

[[nodiscard]] inline constexpr barrett_ctx
barrett_make(limb_t *mwork, limb_t *iwork, const limb_t *mp, usize n, limb_t *scratch) noexcept
{
  const usize s = limb_clz(mp[n - 1]);
  if ( s != 0 )
    (void)lshift(mwork, mp, n, s);
  else
    copyi(mwork, mp, n);
  invert_n(iwork, mwork, n, scratch);
  return barrett_ctx{ mwork, iwork, n, s };
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
barrett_make_itch(usize n) noexcept
{
  return invert_n_itch(n);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
barrett_reduce_itch(usize n) noexcept
{
  usize w = mul_itch(n + 1u, n);
  const usize v = mul_itch(n, n);
  if ( v > w ) w = v;
  // xw 2n+1, P 2n+1, T n+2, q3 n, U 2n, rr n+1
  return (2u * n + 1u) + (2u * n + 1u) + (n + 2u) + n + (2u * n) + (n + 1u) + w;
}

// rp = {xp, xn} mod m, both unshifted, rp taking n limbs
inline constexpr void
barrett_reduce(limb_t *rp, const limb_t *xp, usize xn, const barrett_ctx &c, limb_t *scratch) noexcept
{
  const usize n = c.n;
  limb_t *const xw = scratch;               // 2n + 1
  limb_t *const pp = xw + 2u * n + 1u;      // 2n + 1
  limb_t *const tt = pp + 2u * n + 1u;      // n + 2
  limb_t *const q3 = tt + n + 2u;           // n
  limb_t *const uu = q3 + n;                // 2n
  limb_t *const rr = uu + 2u * n;           // n + 1
  limb_t *const work = rr + n + 1u;

  if ( xn > 2u * n ) xn = 2u * n;
  copyi(xw, xp, xn);
  zero(xw + xn, 2u * n + 1u - xn);
  if ( c.shift != 0 ) xw[2u * n] = lshift(xw, xw, 2u * n, c.shift);

  // q1 = xw + n - 1, n + 1 limbs
  mul(pp, xw + n - 1u, n + 1u, c.ip, n, work);

  // T = floor(q2 / B^n) = q1 + floor(q1*ip / B^n)
  copyi(tt, pp + n, n + 1u);
  tt[n + 1u] = add_n(tt, tt, xw + n - 1u, n + 1u);

  // q3 = floor(T / B)
  if ( tt[n + 1u] != 0 ) [[unlikely]]
    for ( usize i = 0; i < n; ++i ) q3[i] = limb_max;
  else
    copyi(q3, tt + 1u, n);

  mul(uu, q3, n, c.mp, n, work);

  (void)sub_n(rr, xw, uu, n + 1u);      // wraps by design

  for ( ;; ) {
    if ( rr[n] == 0 && cmp(rr, c.mp, n) < 0 ) break;
    (void)sub(rr, rr, n + 1u, c.mp, n);
  }

  if ( c.shift != 0 )
    (void)rshift(rp, rr, n, c.shift);
  else
    copyi(rp, rr, n);
}

// rp = a*b mod m, operands reduced, n limbs each
inline constexpr void
barrett_mul(limb_t *rp, const limb_t *ap, const limb_t *bp, const barrett_ctx &c, limb_t *scratch) noexcept
{
  limb_t *const prod = scratch;      // 2n
  limb_t *const work = prod + 2u * c.n;
  mul(prod, ap, c.n, bp, c.n, work);
  barrett_reduce(rp, prod, 2u * c.n, c, work);
}

inline constexpr void
barrett_sqr(limb_t *rp, const limb_t *ap, const barrett_ctx &c, limb_t *scratch) noexcept
{
  limb_t *const prod = scratch;      // 2n
  limb_t *const work = prod + 2u * c.n;
  sqr(prod, ap, c.n, work);
  barrett_reduce(rp, prod, 2u * c.n, c, work);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
barrett_op_itch(usize n) noexcept
{
  usize w = mul_itch(n, n);
  const usize s = sqr_itch(n);
  if ( s > w ) w = s;
  const usize r = barrett_reduce_itch(n);
  if ( r > w ) w = r;
  return 2u * n + w;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
powm_window_for(usize ebits, usize n, usize budget) noexcept
{
  usize k = powm_window(ebits);
  if ( k > threshold::powm_window_cap ) k = threshold::powm_window_cap;
  if ( n == 0 ) n = 1;
  while ( k > 1u && ((usize{ 1 } << (k - 1u)) * n) > budget ) --k;
  return k;
}

// %%%%%%%%%%%%%%%%%%%%%%%%
// scratch
[[nodiscard, gnu::flatten]] inline constexpr usize
__powm_work_itch(usize n, usize bn) noexcept
{
  usize mont = mont_op_itch(n);
  const usize t = to_mont_itch(bn, n);
  if ( t > mont ) mont = t;

  usize bar = barrett_make_itch(n);
  const usize bop = barrett_op_itch(n);
  if ( bop > bar ) bar = bop;
  const usize bred = (bn > n ? (bn - n + 1u) : 1u) + divrem_itch(bn > n ? bn : n, n);
  if ( bred > bar ) bar = bred;
  bar += 2u * n;

  return mont > bar ? mont : bar;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
powm_itch(usize n, usize bn, usize ebits, usize budget = threshold::powm_table_limbs) noexcept
{
  if ( n == 0 ) return 0;
  if ( bn == 0 ) bn = 1;
  const usize k = powm_window_for(ebits, n, budget);
  const usize tbl = (usize{ 1 } << (k - 1u)) * n;
  return tbl + 2u * n + __powm_work_itch(n, bn);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
powm_itch_max(usize cap, usize budget = threshold::powm_table_limbs) noexcept
{
  if ( cap == 0 ) return 0;
  const usize tbl = budget > cap ? budget : cap;
  return tbl + 2u * cap + __powm_work_itch(cap, cap);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// engines

[[nodiscard, gnu::always_inline]] inline constexpr usize
__powm_bits(const limb_t *ep, usize en, usize lo, usize len) noexcept
{
  usize w = 0;
  for ( usize j = len; j-- > 0; ) w = (w << 1) | (testbit(ep, en, lo + j) ? 1u : 0u);
  return w;
}

template<class Engine>
inline constexpr void
__powm_window(limb_t *rp, const limb_t *ep, usize en, usize ebits, usize n, usize k, limb_t *tbl, limb_t *acc, limb_t *tmp,
              Engine &eng) noexcept
{
  const usize entries = usize{ 1 } << (k - 1u);

  // odd powers: T[j] = base^(2j+1)
  if ( entries > 1u ) {
    eng.sqr(tmp, tbl);
    for ( usize j = 1; j < entries; ++j ) eng.mul(tbl + j * n, tbl + (j - 1u) * n, tmp);
  }

  bool first = true;
  usize i = ebits;
  while ( i > 0 ) {
    if ( !testbit(ep, en, i - 1u) ) {
      if ( !first ) eng.sqr(acc, acc);
      --i;
      continue;
    }
    usize len = k < i ? k : i;
    while ( len > 1u && !testbit(ep, en, i - len) ) --len;
    const usize w = __powm_bits(ep, en, i - len, len);

    if ( first ) {
      copyi(acc, tbl + ((w - 1u) >> 1) * n, n);
      first = false;
    } else {
      for ( usize t = 0; t < len; ++t ) eng.sqr(acc, acc);
      eng.mul(acc, acc, tbl + ((w - 1u) >> 1) * n);
    }
    i -= len;
  }
  copyi(rp, acc, n);
}

template<modalgo K>
inline constexpr void
__powm_engine(limb_t *rp, const limb_t *bp, usize bn, const limb_t *ep, usize en, usize ebits, const limb_t *mp, usize n, usize k,
              limb_t *tbl, limb_t *acc, limb_t *tmp, limb_t *work) noexcept
{
  if constexpr ( K == modalgo::redc ) {
    const mont_ctx c = mont_make(mp, n);

    struct eng_t {
      const mont_ctx &c;
      limb_t *w;

      [[gnu::always_inline]] constexpr void
      mul(limb_t *r, const limb_t *a, const limb_t *b) const noexcept
      {
        mont_mul(r, a, b, c, w);
      }

      [[gnu::always_inline]] constexpr void
      sqr(limb_t *r, const limb_t *a) const noexcept
      {
        mont_sqr(r, a, c, w);
      }
    } eng{ c, work };

    to_mont(tbl, bp, bn, c, work);
    if ( is_zero(tbl, n) ) {
      // b == 0 mod m, and e > 0 here, so the answer is 0 whatever the window would do
      zero(rp, n);
      return;
    }
    __powm_window(acc, ep, en, ebits, n, k, tbl, acc, tmp, eng);
    from_mont(rp, acc, c, work);
  } else {
    limb_t *const mw = work;        // n
    limb_t *const iw = mw + n;      // n
    limb_t *const bw = iw + n;      // the rest
    const barrett_ctx c = barrett_make(mw, iw, mp, n, bw);

    struct eng_t {
      const barrett_ctx &c;
      limb_t *w;

      [[gnu::always_inline]] constexpr void
      mul(limb_t *r, const limb_t *a, const limb_t *b) const noexcept
      {
        barrett_mul(r, a, b, c, w);
      }

      [[gnu::always_inline]] constexpr void
      sqr(limb_t *r, const limb_t *a) const noexcept
      {
        barrett_sqr(r, a, c, w);
      }
    } eng{ c, bw };

    if ( bn > n || cmp_var(bp, bn, mp, n) >= 0 ) {
      const usize qn = bn > n ? bn - n + 1u : 1u;
      limb_t *const qp = bw;
      limb_t *const dw = qp + qn;
      if ( bn < n ) {
        copyi(tbl, bp, bn);
        zero(tbl + bn, n - bn);
      } else {
        divrem(qp, tbl, bp, bn, mp, n, dw);
      }
    } else {
      copyi(tbl, bp, bn);
      zero(tbl + bn, n - bn);
    }
    if ( is_zero(tbl, n) ) {
      zero(rp, n);
      return;
    }
    __powm_window(rp, ep, en, ebits, n, k, tbl, acc, tmp, eng);
  }
}

inline constexpr void
powm(limb_t *rp, const limb_t *bp, usize bn, const limb_t *ep, usize en, const limb_t *mp, usize n, limb_t *scratch,
     usize budget = threshold::powm_table_limbs) noexcept
{
  n = normalize(mp, n);
  if ( n == 0 ) return;

  if ( n == 1 && mp[0] == 1 ) {
    rp[0] = 0;
    return;
  }

  en = normalize(ep, en);
  if ( en == 0 ) {
    zero(rp, n);
    rp[0] = 1;
    return;
  }

  bn = normalize(bp, bn);
  if ( bn == 0 ) {
    zero(rp, n);
    return;
  }

  const usize ebits = bitlen(ep, en);
  const usize k = powm_window_for(ebits, n, budget);
  const usize entries = usize{ 1 } << (k - 1u);

  limb_t *const tbl = scratch;                // entries * n
  limb_t *const acc = tbl + entries * n;      // n
  limb_t *const tmp = acc + n;                // n
  limb_t *const work = tmp + n;

  const bool odd = (mp[0] & limb_t{ 1 }) != 0;
  const bool use_mont = (threshold::powm_engine == 0u) ? odd : (threshold::powm_engine == 1u);

  if ( use_mont )
    __powm_engine<modalgo::redc>(rp, bp, bn, ep, en, ebits, mp, n, k, tbl, acc, tmp, work);
  else
    __powm_engine<modalgo::barrett>(rp, bp, bn, ep, en, ebits, mp, n, k, tbl, acc, tmp, work);
}

inline constexpr modalgo mod_engines_built = modalgo::barrett;

// pinned engines
template<modalgo K>
inline constexpr void
powm_with(limb_t *rp, const limb_t *bp, usize bn, const limb_t *ep, usize en, const limb_t *mp, usize n, limb_t *scratch,
          usize budget = threshold::powm_table_limbs) noexcept
{
  static_assert(static_cast<u8>(K) <= static_cast<u8>(mod_engines_built),
                "arbint: this modular engine is not implemented yet -- see mpn::mod_engines_built");
  n = normalize(mp, n);
  if ( n == 0 ) return;
  if ( n == 1 && mp[0] == 1 ) {
    rp[0] = 0;
    return;
  }
  en = normalize(ep, en);
  if ( en == 0 ) {
    zero(rp, n);
    rp[0] = 1;
    return;
  }
  bn = normalize(bp, bn);
  if ( bn == 0 ) {
    zero(rp, n);
    return;
  }

  const usize ebits = bitlen(ep, en);
  const usize k = powm_window_for(ebits, n, budget);
  const usize entries = usize{ 1 } << (k - 1u);

  limb_t *const tbl = scratch;
  limb_t *const acc = tbl + entries * n;
  limb_t *const tmp = acc + n;
  limb_t *const work = tmp + n;

  __powm_engine<K>(rp, bp, bn, ep, en, ebits, mp, n, k, tbl, acc, tmp, work);
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
