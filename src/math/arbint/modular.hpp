//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__exceptions.hpp"
#include "../../except.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "div_mu.hpp"
#include "limb.hpp"
#include "mont.hpp"
#include "mpn_core.hpp"
#include "powm.hpp"
#include "signed.hpp"
#include "storage.hpp"
#include "traits.hpp"
#include "unsigned.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// modular layer
//
//   montgomery<U>   odd modulus, precomputed once; even moduli throw; the fast path for exponentiation
//   barrett<U>      any modulus, precomputed once
//   powmod/mulmod   one-shot, no context to keep, dispatching on parity
//
// WARNING: montgomery::mul and montgomery::sqr take and return montgomery form,
//          while montgomery::pow takes and returns an ordinary residue
//
//
// NOTE: PINNED SOLVERS DO NOT REACH IN HERE

namespace micron
{
namespace math
{

namespace __arb
{

[[nodiscard, gnu::always_inline]] inline constexpr usize
mod_max(usize a, usize b) noexcept
{
  return a > b ? a : b;
}

template<class U> struct mod_sizes {
  static constexpr bool bounded = U::bounded;
  static constexpr usize cap = U::cap_limbs;

  // __pow2_mod at its widest, k == 2*cap: np(k+1) + qp(k-n+2) + rp(n) + divrem_itch(k+1, n)
  // barrett's ctor carves 2*cap + invert_n_itch(cap) out of the same constant, which is smaller
  static constexpr usize setup = bounded ? (2u * cap + 1u) + (cap + 2u) + cap + mpn::divrem_itch(2u * cap + 1u, cap) : 1u;

  // prod(2cap) + qp(cap+2) + rp(cap) + divrem_itch(2cap, cap)
  // mulmod, sqrmod, __reduced and barrett::reduce all carve out of this one
  static constexpr usize wide = bounded ? 2u * cap + (cap + 2u) + cap + mpn::divrem_itch(2u * cap, cap) : 1u;

  // __mont_op / __bar_op both lay two n-limb operands down first and then hand the rest to the engine
  static constexpr usize op = bounded ? 2u * cap + mod_max(mpn::mont_op_itch(cap), mpn::barrett_op_itch(cap)) : 1u;

  // out(n) + the window. powm_itch_max and NOT powm_itch(cap, ...)
  static constexpr usize powm = bounded ? cap + mpn::powm_itch_max(cap, mpn::threshold::powm_table_limbs) : 1u;
};

};      // namespace __arb

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// montgomery

template<arb_unsigned U> class montgomery
{
public:
  using value_type = U;
  using allocator_type = typename U::allocator_type;

  static constexpr usize width_bits = U::width_bits;
  static constexpr bool bounded = U::bounded;
  static constexpr usize cap_limbs = U::cap_limbs;

private:
  using sizes = __arb::mod_sizes<U>;
  template<usize N> using scratch_for = __arb::scratch<bounded ? N : 1u, allocator_type, bounded>;

  U md;                  // modulus, odd
  U r1;                  // R mod m
  U r2;                  // R^2 mod m
  mpn::limb_t minv;      // -m^-1 mod B
  u32 mn;                // Montgomery width in limbs; R == B^mn

  static_assert(
      !bounded || sizes::powm * sizeof(mpn::limb_t) <= (usize{ 1 } << 18),
      "arbint: a bounded powmod at this width would park more than 256 KiB of frame; use the dynamic arbuint<> for a modulus this large");

  [[nodiscard, gnu::always_inline]] constexpr const mpn::limb_t *
  mlimbs() const noexcept
  {
    return md.limbs();
  }

  // B^k mod m, for k == mn (R) and k == 2*mn (R^2)
  constexpr void
  __pow2_mod(U &out, usize k)
  {
    const usize n = mn;
    scratch_for<sizes::setup> sc(k + 1u + (k - n + 1u) + n + mpn::divrem_itch(k + 1u, n));
    mpn::limb_t *const np = sc.get();               // k + 1
    mpn::limb_t *const qp = np + k + 1u;            // k + 1 - n + 1
    mpn::limb_t *const rp = qp + (k - n + 2u);      // n
    mpn::limb_t *const work = rp + n;

    mpn::zero(np, k);
    np[k] = 1;
    mpn::divrem(qp, rp, np, k + 1u, mlimbs(), n, work);
    out.__assign_limbs(rp, n);
  }

public:
  // ctors
  explicit constexpr montgomery(const U &modulus) : md(modulus), r1(), r2(), minv(0), mn(0)
  {
    if ( md.is_zero() ) micron::exc<micron::except::domain_error>("micron::math::montgomery modulus is zero");
    if ( md.even() ) micron::exc<micron::except::domain_error>("micron::math::montgomery requires an odd modulus");
    mn = static_cast<u32>(md.size());
    minv = static_cast<mpn::limb_t>(static_cast<mpn::limb_t>(0) - mpn::binv_odd(md.limbs()[0]));
    __pow2_mod(r1, mn);
    __pow2_mod(r2, 2u * static_cast<usize>(mn));
  }

  constexpr montgomery(const montgomery &) = default;
  constexpr montgomery(montgomery &&) noexcept = default;
  constexpr montgomery &operator=(const montgomery &) = default;
  constexpr montgomery &operator=(montgomery &&) noexcept = default;
  ~montgomery() = default;

  [[nodiscard, gnu::always_inline]] constexpr const U &
  modulus() const noexcept
  {
    return md;
  }

  [[nodiscard, gnu::always_inline]] constexpr const U &
  one_form() const noexcept
  {
    return r1;
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  width() const noexcept
  {
    return mn;
  }

  [[nodiscard, gnu::always_inline]] constexpr mpn::limb_t
  inv_limb() const noexcept
  {
    return minv;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // named operations

  // a*R mod m. a need not be reduced
  [[nodiscard]] constexpr U
  to_form(const U &a) const
  {
    U t = __reduced(a);
    return __mont_op(t, r2, false);
  }

  // a*R^-1 mod m
  [[nodiscard]] constexpr U
  from_form(const U &a) const
  {
    const usize n = mn;
    scratch_for<sizes::op> sc(2u * n + mpn::from_mont_itch(n));
    mpn::limb_t *const tp = sc.get();
    mpn::limb_t *const out = tp + 2u * n;

    const mpn::limb_t *const ap = a.limbs();
    const usize an = a.size();
    mpn::copyi(out, ap, an < n ? an : n);
    if ( an < n ) mpn::zero(out + an, n - an);

    const mpn::mont_ctx c{ mlimbs(), n, minv };
    mpn::copyi(tp, out, n);
    mpn::zero(tp + n, n);
    mpn::redc(out, tp, c);

    U r;
    r.__assign_limbs(out, n);
    return r;
  }

  [[nodiscard]] constexpr U
  reduce(const U &t) const
  {
    return from_form(t);
  }

  [[nodiscard]] constexpr U
  mul(const U &a, const U &b) const
  {
    return __mont_op(a, b, false);
  }

  [[nodiscard]] constexpr U
  sqr(const U &a) const
  {
    return __mont_op(a, a, true);
  }

  [[nodiscard]] constexpr U
  pow(const U &a, const U &ex) const
  {
    return __powm_into(a, ex.limbs(), ex.size());
  }

  [[nodiscard]] constexpr U
  pow(const U &a, u64 ex) const
  {
    mpn::limb_t buf[(sizeof(u64) + sizeof(mpn::limb_t) - 1u) / sizeof(mpn::limb_t)] = {};
    constexpr usize take = sizeof(buf) / sizeof(buf[0]);
    u64 v = ex;
    for ( usize i = 0; i < take; ++i ) {
      buf[i] = static_cast<mpn::limb_t>(v);
      v = (mpn::limb_bits >= 64u) ? 0u : (v >> (mpn::limb_bits & 63u));
    }
    return __powm_into(a, buf, take);
  }

private:
  [[nodiscard]] constexpr U
  __reduced(const U &a) const
  {
    // a mod m, by the shortest route that is correct for both widths
    const usize n = mn;
    const usize an = a.size();
    if ( an < n || mpn::cmp_var(a.limbs(), an, mlimbs(), n) < 0 ) {
      U t(a);
      return t;
    }
    scratch_for<sizes::wide> sc((an - n + 1u) + n + mpn::divrem_itch(an, n));
    mpn::limb_t *const qp = sc.get();
    mpn::limb_t *const rp = qp + (an - n + 1u);
    mpn::limb_t *const work = rp + n;
    mpn::divrem(qp, rp, a.limbs(), an, mlimbs(), n, work);
    U r;
    r.__assign_limbs(rp, n);
    return r;
  }

  [[nodiscard]] constexpr U
  __mont_op(const U &a, const U &b, bool square) const
  {
    const usize n = mn;
    scratch_for<sizes::op> sc(2u * n + mpn::mont_op_itch(n));
    mpn::limb_t *const av = sc.get();      // n
    mpn::limb_t *const bv = av + n;        // n
    mpn::limb_t *const work = bv + n;

    __spread(av, a);
    if ( !square ) __spread(bv, b);

    const mpn::mont_ctx c{ mlimbs(), n, minv };
    if ( square )
      mpn::mont_sqr(av, av, c, work);
    else
      mpn::mont_mul(av, av, bv, c, work);

    U r;
    r.__assign_limbs(av, n);
    return r;
  }

  [[gnu::always_inline]] constexpr void
  __spread(mpn::limb_t *dst, const U &a) const noexcept
  {
    const usize n = mn;
    const usize an = a.size();
    const usize take = an < n ? an : n;
    mpn::copyi(dst, a.limbs(), take);
    if ( take < n ) mpn::zero(dst + take, n - take);
  }

  [[nodiscard]] constexpr U
  __powm_into(const U &a, const mpn::limb_t *ep, usize en) const
  {
    const usize n = mn;
    const usize an = a.size();
    const usize ebits = mpn::bitlen(ep, en);
    scratch_for<sizes::powm> sc(n + mpn::powm_itch(n, an != 0 ? an : 1u, ebits));
    mpn::limb_t *const out = sc.get();
    mpn::limb_t *const work = out + n;

    mpn::powm_with<mpn::modalgo::redc>(out, a.limbs(), an, ep, en, mlimbs(), n, work);
    U r;
    r.__assign_limbs(out, n);
    return r;
  }
};

// %%%%%%%%%%%%%%%%%
// barrett
template<arb_unsigned U> class barrett
{
public:
  using value_type = U;
  using wide_type = __arb::wide_t<U>;
  using allocator_type = typename U::allocator_type;

  static constexpr usize width_bits = U::width_bits;
  static constexpr bool bounded = U::bounded;
  static constexpr usize cap_limbs = U::cap_limbs;

private:
  using sizes = __arb::mod_sizes<U>;
  template<usize N> using scratch_for = __arb::scratch<bounded ? N : 1u, allocator_type, bounded>;

  U md;
  wide_type mw;      // modulus normalized
  wide_type iv;      // reciprocal
  u32 mn;
  u32 sh;

  static_assert(
      !bounded || sizes::powm * sizeof(mpn::limb_t) <= (usize{ 1 } << 18),
      "arbint: a bounded powmod at this width would park more than 256 KiB of frame; use the dynamic arbuint<> for a modulus this large");

public:
  // ctors
  explicit constexpr barrett(const U &modulus) : md(modulus), mw(), iv(), mn(0), sh(0)
  {
    if ( md.is_zero() ) micron::exc<micron::except::domain_error>("micron::math::barrett modulus is zero");
    mn = static_cast<u32>(md.size());
    const usize n = mn;
    sh = static_cast<u32>(mpn::limb_clz(md.limbs()[n - 1u]));

    scratch_for<sizes::setup> sc(2u * n + mpn::invert_n_itch(n));
    mpn::limb_t *const mp = sc.get();      // n
    mpn::limb_t *const ip = mp + n;        // n
    mpn::limb_t *const work = ip + n;

    if ( sh != 0 )
      (void)mpn::lshift(mp, md.limbs(), n, sh);
    else
      mpn::copyi(mp, md.limbs(), n);
    mpn::invert_n(ip, mp, n, work);

    mw.__assign_limbs(mp, n);
    iv.__assign_limbs(ip, n);
  }

  constexpr barrett(const barrett &) = default;
  constexpr barrett(barrett &&) noexcept = default;
  constexpr barrett &operator=(const barrett &) = default;
  constexpr barrett &operator=(barrett &&) noexcept = default;
  ~barrett() = default;

  [[nodiscard, gnu::always_inline]] constexpr const U &
  modulus() const noexcept
  {
    return md;
  }

  [[nodiscard, gnu::always_inline]] constexpr const wide_type &
  inverse() const noexcept
  {
    return iv;
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  width() const noexcept
  {
    return mn;
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  shift() const noexcept
  {
    return sh;
  }

  [[nodiscard]] constexpr U
  reduce(const U &a) const
  {
    const usize n = mn;
    const usize an = a.size();
    if ( an < n || mpn::cmp_var(a.limbs(), an, md.limbs(), n) < 0 ) return a;
    scratch_for<sizes::wide> sc((an - n + 1u) + n + mpn::divrem_itch(an, n));
    mpn::limb_t *const qp = sc.get();
    mpn::limb_t *const rp = qp + (an - n + 1u);
    mpn::limb_t *const work = rp + n;
    mpn::divrem(qp, rp, a.limbs(), an, md.limbs(), n, work);
    U r;
    r.__assign_limbs(rp, n);
    return r;
  }

  [[nodiscard]] constexpr U
  mul(const U &a, const U &b) const
  {
    return __bar_op(a, b, false);
  }

  [[nodiscard]] constexpr U
  sqr(const U &a) const
  {
    return __bar_op(a, a, true);
  }

  [[nodiscard]] constexpr U
  pow(const U &a, const U &ex) const
  {
    return __powm_into(a, ex.limbs(), ex.size());
  }

  [[nodiscard]] constexpr U
  pow(const U &a, u64 ex) const
  {
    mpn::limb_t buf[(sizeof(u64) + sizeof(mpn::limb_t) - 1u) / sizeof(mpn::limb_t)] = {};
    constexpr usize take = sizeof(buf) / sizeof(buf[0]);
    u64 v = ex;
    for ( usize i = 0; i < take; ++i ) {
      buf[i] = static_cast<mpn::limb_t>(v);
      v = (mpn::limb_bits >= 64u) ? 0u : (v >> (mpn::limb_bits & 63u));
    }
    return __powm_into(a, buf, take);
  }

private:
  [[gnu::always_inline]] constexpr void
  __spread(mpn::limb_t *dst, const U &a) const noexcept
  {
    const usize n = mn;
    const usize an = a.size();
    const usize take = an < n ? an : n;
    mpn::copyi(dst, a.limbs(), take);
    if ( take < n ) mpn::zero(dst + take, n - take);
  }

  [[nodiscard]] constexpr U
  __bar_op(const U &a, const U &b, bool square) const
  {
    const usize n = mn;
    scratch_for<sizes::op> sc(2u * n + mpn::barrett_op_itch(n));
    mpn::limb_t *const av = sc.get();
    mpn::limb_t *const bv = av + n;
    mpn::limb_t *const work = bv + n;

    __spread(av, a);
    if ( !square ) __spread(bv, b);

    const mpn::barrett_ctx c{ mw.limbs(), iv.limbs(), n, sh };
    if ( square )
      mpn::barrett_sqr(av, av, c, work);
    else
      mpn::barrett_mul(av, av, bv, c, work);

    U r;
    r.__assign_limbs(av, n);
    return r;
  }

  [[nodiscard]] constexpr U
  __powm_into(const U &a, const mpn::limb_t *ep, usize en) const
  {
    const usize n = mn;
    const usize an = a.size();
    const usize ebits = mpn::bitlen(ep, en);
    scratch_for<sizes::powm> sc(n + mpn::powm_itch(n, an != 0 ? an : 1u, ebits));
    mpn::limb_t *const out = sc.get();
    mpn::limb_t *const work = out + n;

    mpn::powm_with<mpn::modalgo::barrett>(out, a.limbs(), an, ep, en, md.limbs(), n, work);
    U r;
    r.__assign_limbs(out, n);
    return r;
  }
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// named operations; modular

// a*b mod m
template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
mulmod(const arbuint<B, S, A> &a, const arbuint<B, S, A> &b, const arbuint<B, S, A> &m)
{
  using U = arbuint<B, S, A>;
  using sizes = __arb::mod_sizes<U>;
  const usize n = m.size();
  if ( n == 0 ) micron::exc<micron::except::domain_error>("micron::math::mulmod modulus is zero");

  const usize an = a.size();
  const usize bn = b.size();
  if ( an == 0 || bn == 0 ) return U::zero();

  const usize pn = an + bn;
  __arb::scratch<U::bounded ? sizes::wide : 1u, A, U::bounded> sc(pn + (pn > n ? pn - n + 1u : 1u) + n
                                                                  + mpn::divrem_itch(pn > n ? pn : n, n));
  mpn::limb_t *const prod = sc.get();
  mpn::limb_t *const qp = prod + pn;
  mpn::limb_t *const rp = qp + (pn > n ? pn - n + 1u : 1u);
  mpn::limb_t *const work = rp + n;

  if ( an >= bn )
    U::__mul_into(prod, a.limbs(), an, b.limbs(), bn, work);
  else
    U::__mul_into(prod, b.limbs(), bn, a.limbs(), an, work);

  const usize rn = mpn::normalize(prod, pn);
  U r;
  if ( rn < n || mpn::cmp_var(prod, rn, m.limbs(), n) < 0 ) {
    r.__assign_limbs(prod, rn);
    return r;
  }
  mpn::divrem(qp, rp, prod, rn, m.limbs(), n, work);
  r.__assign_limbs(rp, n);
  return r;
}

// a*a mod m via squaring ladder
template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
sqrmod(const arbuint<B, S, A> &a, const arbuint<B, S, A> &m)
{
  using U = arbuint<B, S, A>;
  using sizes = __arb::mod_sizes<U>;
  const usize n = m.size();
  if ( n == 0 ) micron::exc<micron::except::domain_error>("micron::math::sqrmod modulus is zero");

  const usize an = a.size();
  if ( an == 0 ) return U::zero();

  const usize pn = 2u * an;
  __arb::scratch<U::bounded ? sizes::wide : 1u, A, U::bounded> sc(pn + (pn > n ? pn - n + 1u : 1u) + n
                                                                  + mpn::divrem_itch(pn > n ? pn : n, n));
  mpn::limb_t *const prod = sc.get();
  mpn::limb_t *const qp = prod + pn;
  mpn::limb_t *const rp = qp + (pn > n ? pn - n + 1u : 1u);
  mpn::limb_t *const work = rp + n;

  U::__sqr_into(prod, a.limbs(), an, work);

  const usize rn = mpn::normalize(prod, pn);
  U r;
  if ( rn < n || mpn::cmp_var(prod, rn, m.limbs(), n) < 0 ) {
    r.__assign_limbs(prod, rn);
    return r;
  }
  mpn::divrem(qp, rp, prod, rn, m.limbs(), n, work);
  r.__assign_limbs(rp, n);
  return r;
}

// base^exp mod m via the sliding window
template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
powmod(const arbuint<B, S, A> &a, const arbuint<B, S, A> &ex, const arbuint<B, S, A> &m)
{
  using U = arbuint<B, S, A>;
  using sizes = __arb::mod_sizes<U>;
  const usize n = m.size();
  if ( n == 0 ) micron::exc<micron::except::domain_error>("micron::math::powmod modulus is zero");

  const usize an = a.size();
  const usize en = ex.size();
  const usize ebits = mpn::bitlen(ex.limbs(), en);

  __arb::scratch<U::bounded ? sizes::powm : 1u, A, U::bounded> sc(n + mpn::powm_itch(n, an != 0 ? an : 1u, ebits));
  mpn::limb_t *const out = sc.get();
  mpn::limb_t *const work = out + n;

  mpn::powm(out, a.limbs(), an, ex.limbs(), en, m.limbs(), n, work);
  U r;
  r.__assign_limbs(out, n);
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
powmod(const arbuint<B, S, A> &a, u64 ex, const arbuint<B, S, A> &m)
{
  return powmod(a, arbuint<B, S, A>(ex), m);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// signed
template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
powmod(const arbint<B, S, A> &a, const arbint<B, S, A> &ex, const arbint<B, S, A> &m)
{
  using SI = arbint<B, S, A>;
  using U = arbuint<B, S, A>;
  if ( ex.sign() < 0 ) micron::exc<micron::except::domain_error>("micron::math::powmod negative exponent");

  const U mm = abs(m).magnitude();
  U base = abs(a).magnitude();
  if ( mm.is_zero() ) micron::exc<micron::except::domain_error>("micron::math::powmod modulus is zero");

  base %= mm;
  if ( a.sign() < 0 && !base.is_zero() ) base = mm - base;

  return SI(powmod(base, abs(ex).magnitude(), mm), false);
}

};      // namespace math
};      // namespace micron
