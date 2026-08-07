//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__exceptions.hpp"
#include "../../except.hpp"
#include "../../types.hpp"
#include "gcd.hpp"
#include "gcdext.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "signed.hpp"
#include "storage.hpp"
#include "traits.hpp"
#include "unsigned.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// number theory et al

namespace micron
{
namespace math
{

namespace __arb
{

[[nodiscard, gnu::always_inline]] inline constexpr usize
mod_gcd_max(usize a, usize b) noexcept
{
  return a > b ? a : b;
}

template<class U> struct gcd_sizes {
  static constexpr bool bounded = U::bounded;
  static constexpr usize cap = U::cap_limbs;

  static constexpr usize gcd
      = bounded ? cap + cap + (cap + 1u) + mod_gcd_max(mpn::divrem_itch(cap, cap), mpn::gcd_tier_itch(cap, cap)) : 1u;

  static constexpr usize inv = bounded ? cap + mpn::invmod_itch(cap, cap) : 1u;
};

};      // namespace __arb

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
gcd(const arbuint<B, S, A> &a, const arbuint<B, S, A> &b)
{
  using U = arbuint<B, S, A>;
  using sizes = __arb::gcd_sizes<U>;
  const usize an = a.size();
  const usize bn = b.size();
  if ( an == 0 ) return b;
  if ( bn == 0 ) return a;

  const usize gn = an < bn ? an : bn;
  __arb::scratch<U::bounded ? sizes::gcd : 1u, A, U::bounded> sc(gn + mpn::gcd_itch(an, bn));
  mpn::limb_t *const gp = sc.get();
  mpn::limb_t *const work = gp + gn;

  const usize rn = mpn::gcd(gp, a.limbs(), an, b.limbs(), bn, work);
  U r;
  r.__assign_limbs(gp, rn);
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
lcm(const arbuint<B, S, A> &a, const arbuint<B, S, A> &b)
{
  using U = arbuint<B, S, A>;
  if ( a.is_zero() || b.is_zero() ) return U::zero();
  const U g = gcd(a, b);
  return (a / g) * b;
}

template<usize B, arb_solver S, class A> struct inv_result {
  arbuint<B, S, A> value;
  bool exists;
};

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr inv_result<B, S, A>
invmod(const arbuint<B, S, A> &a, const arbuint<B, S, A> &m)
{
  using U = arbuint<B, S, A>;
  using sizes = __arb::gcd_sizes<U>;
  const usize mn = m.size();
  if ( mn == 0 ) micron::exc<micron::except::domain_error>("micron::math::invmod modulus is zero");

  const usize an = a.size();
  __arb::scratch<U::bounded ? sizes::inv : 1u, A, U::bounded> sc(mn + mpn::invmod_itch(an != 0 ? an : 1u, mn));
  mpn::limb_t *const rp = sc.get();
  mpn::limb_t *const work = rp + mn;

  usize rn = 0;
  const bool ok = mpn::invmod(rp, rn, a.limbs(), an, m.limbs(), mn, work);
  U r;
  if ( ok ) r.__assign_limbs(rp, rn);
  return inv_result<B, S, A>{ r, ok };
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
gcd(const arbint<B, S, A> &a, const arbint<B, S, A> &b)
{
  return arbint<B, S, A>(gcd(a.magnitude(), b.magnitude()), false);
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
lcm(const arbint<B, S, A> &a, const arbint<B, S, A> &b)
{
  return arbint<B, S, A>(lcm(a.magnitude(), b.magnitude()), false);
}

template<usize B, arb_solver S, class A> struct sinv_result {
  arbint<B, S, A> value;
  bool exists;
};

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr sinv_result<B, S, A>
invmod(const arbint<B, S, A> &a, const arbint<B, S, A> &m)
{
  using U = arbuint<B, S, A>;
  const U mm = m.magnitude();
  if ( mm.is_zero() ) micron::exc<micron::except::domain_error>("micron::math::invmod modulus is zero");

  U base = a.magnitude();
  base %= mm;
  if ( a.sign() < 0 && !base.is_zero() ) base = mm - base;

  const auto r = invmod(base, mm);
  return sinv_result<B, S, A>{ arbint<B, S, A>(r.value, false), r.exists };
}

};      // namespace math
};      // namespace micron
