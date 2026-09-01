//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__exceptions.hpp"
#include "../../concepts.hpp"
#include "../../except.hpp"
#include "../../tags.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "div_mu.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "mul.hpp"
#include "storage.hpp"
#include "tags.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// arbuint

namespace micron
{
namespace math
{

template<usize Bits = 0, arb_solver Solver = solver::automatic, class Alloc = micron::allocator_serial<>> class arbuint
{
public:
  using store_type = __arb::store<Bits, Alloc>;
  using value_type = mpn::limb_t;
  using category_type = micron::numeric_tag;
  using solver_type = Solver;
  using allocator_type = Alloc;

  static constexpr usize width_bits = Bits;
  static constexpr bool bounded = store_type::bounded;
  static constexpr usize cap_limbs = store_type::cap_limbs;

private:
  store_type s;

  template<usize N> using scratch_for = __arb::scratch<bounded ? N : 1u, Alloc, bounded>;

  static constexpr usize mul_cap_itch = bounded ? mpn::mul_solver_cap_itch<Solver>(cap_limbs) : 0u;
  static constexpr usize sqr_cap_itch = bounded ? mpn::sqr_solver_cap_itch<Solver>(cap_limbs) : 0u;
  static constexpr usize mul_scratch_limbs
      = bounded ? 2u * cap_limbs + 2u + (mul_cap_itch > sqr_cap_itch ? mul_cap_itch : sqr_cap_itch) : 1u;

  static constexpr usize div_scratch_limbs = bounded ? (cap_limbs + 1u) + cap_limbs + mpn::divrem_itch(2u * cap_limbs, cap_limbs) : 1u;

  template<typename T> friend class __arb_friend_tag;

public:
  constexpr arbuint() noexcept : s() { }

  constexpr arbuint(const arbuint &) = default;
  constexpr arbuint(arbuint &&) noexcept = default;
  constexpr arbuint &operator=(const arbuint &) = default;
  constexpr arbuint &operator=(arbuint &&) noexcept = default;
  ~arbuint() = default;

  template<micron::integral I> constexpr arbuint(I v) : s() { __assign_integral(v); }

  template<micron::integral I>
  constexpr arbuint &
  operator=(I v)
  {
    s.finish(0);
    __assign_integral(v);
    return *this;
  }

  [[nodiscard, gnu::always_inline]] constexpr const mpn::limb_t *
  limbs() const noexcept
  {
    return s.limbs();
  }

  [[nodiscard, gnu::always_inline]] constexpr mpn::limb_t *
  limbs() noexcept
  {
    return s.limbs();
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  size() const noexcept
  {
    return s.size();
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  capacity() const noexcept
  {
    return s.capacity();
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  is_zero() const noexcept
  {
    return s.size() == 0;
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  bit_length() const noexcept
  {
    return mpn::bitlen(s.limbs(), s.size());
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  popcount() const noexcept
  {
    return mpn::popcount(s.limbs(), s.size());
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  testbit(usize i) const noexcept
  {
    return mpn::testbit(s.limbs(), s.size(), i);
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  odd() const noexcept
  {
    return s.size() != 0 && (s.limbs()[0] & mpn::limb_t{ 1 }) != 0;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  even() const noexcept
  {
    return !odd();
  }

  [[nodiscard, gnu::always_inline]] constexpr mpn::limb_t &
  operator[](usize i) noexcept
  {
    return s.limbs()[i];
  }

  [[nodiscard, gnu::always_inline]] constexpr const mpn::limb_t &
  operator[](usize i) const noexcept
  {
    return s.limbs()[i];
  }

  [[nodiscard, gnu::always_inline]] explicit constexpr
  operator bool() const noexcept
  {
    return s.size() != 0;
  }

  template<micron::integral I>
  [[nodiscard]] explicit constexpr
  operator I() const noexcept
  {
    using U = micron::make_unsigned_t<I>;
    U acc = 0;
    const usize take = (sizeof(U) + sizeof(mpn::limb_t) - 1u) / sizeof(mpn::limb_t);
    const usize n = s.size() < take ? s.size() : take;
    for ( usize i = n; i-- > 0; )
      acc = static_cast<U>((sizeof(U) > sizeof(mpn::limb_t) ? (acc << mpn::limb_bits) : U{ 0 }) | static_cast<U>(s.limbs()[i]));
    return static_cast<I>(acc);
  }

  [[nodiscard]] static constexpr arbuint
  zero() noexcept
  {
    return arbuint{};
  }

  [[nodiscard]] static constexpr arbuint
  one()
  {
    return arbuint{ 1u };
  }

  [[nodiscard]] static constexpr arbuint
  power_of_two(usize k)
  {
    arbuint r;
    const usize w = k / mpn::limb_bits;
    if ( !r.s.ensure(w + 1u) ) return r;
    mpn::zero(r.s.limbs(), w + 1u);
    r.s.limbs()[w] = static_cast<mpn::limb_t>(mpn::limb_t{ 1 } << (k % mpn::limb_bits));
    r.s.finish(w + 1u);
    return r;
  }

  [[gnu::always_inline]] constexpr void
  set_zero() noexcept
  {
    s.finish(0);
  }

  [[gnu::always_inline]] constexpr void
  swap(arbuint &o) noexcept
  {
    s.swap_with(o.s);
  }

  constexpr arbuint &
  operator+=(const arbuint &o)
  {
    const usize an = s.size();
    const usize bn = o.s.size();
    if ( bn == 0 ) return *this;
    const usize mx = an > bn ? an : bn;
    const usize mn = an > bn ? bn : an;

    (void)s.ensure(mx + 1u);

    mpn::limb_t *r = s.limbs();
    const mpn::limb_t *b = o.s.limbs();
    const usize cap = s.capacity();

    mpn::limb_t cy = mpn::add_n(r, r, b, mn);
    if ( an > bn )
      cy = mpn::add_1(r + mn, r + mn, an - mn, cy);
    else if ( bn > an )
      cy = mpn::add_1(r + mn, b + mn, bn - mn, cy);

    usize n = mx;
    if ( cy != 0 && n < cap ) {
      r[n] = cy;
      ++n;
    }
    s.finish(n);
    return *this;
  }

  constexpr arbuint &
  operator-=(const arbuint &o)
  {
    const usize an = s.size();
    const usize bn = o.s.size();
    if ( bn == 0 ) return *this;

    const int c = mpn::cmp_var(s.limbs(), an, o.s.limbs(), bn);
    if ( c == 0 ) {
      s.finish(0);
      return *this;
    }
    if ( c > 0 ) {
      (void)mpn::sub(s.limbs(), s.limbs(), an, o.s.limbs(), bn);
      s.finish(an);
      return *this;
    }

    if constexpr ( !bounded ) {
      s.finish(0);
      return *this;
    } else {

      (void)s.ensure(cap_limbs);
      mpn::limb_t *r = s.limbs();
      mpn::zero(r + an, cap_limbs - an);
      mpn::limb_t ext[cap_limbs] = {};
      mpn::copyi(ext, o.s.limbs(), bn);
      (void)mpn::sub_n(r, r, ext, cap_limbs);
      s.finish(cap_limbs);
      return *this;
    }
  }

  constexpr arbuint &
  operator*=(const arbuint &o)
  {
    const usize an = s.size();
    const usize bn = o.s.size();
    if ( an == 0 || bn == 0 ) {
      s.finish(0);
      return *this;
    }
    if ( &o == this ) return __square();

    const usize rn = an + bn;
    const usize itch = an >= bn ? mpn::mul_solver_itch<Solver>(an, bn) : mpn::mul_solver_itch<Solver>(bn, an);
    scratch_for<mul_scratch_limbs> sc(rn + itch);
    mpn::limb_t *t = sc.get();
    mpn::limb_t *work = t + rn;

    if ( an >= bn )
      __mul_into(t, s.limbs(), an, o.s.limbs(), bn, work);
    else
      __mul_into(t, o.s.limbs(), bn, s.limbs(), an, work);

    (void)s.ensure(rn);
    const usize cap = s.capacity();
    mpn::copyi(s.limbs(), t, rn < cap ? rn : cap);
    s.finish(rn);
    return *this;
  }

  constexpr arbuint &
  operator/=(const arbuint &o)
  {
    __divmod_into(o, true);
    return *this;
  }

  constexpr arbuint &
  operator%=(const arbuint &o)
  {
    __divmod_into(o, false);
    return *this;
  }

  constexpr arbuint &
  operator<<=(usize k)
  {
    const usize an = s.size();
    if ( an == 0 || k == 0 ) return *this;
    const usize wh = k / mpn::limb_bits;
    const usize bi = k % mpn::limb_bits;

    (void)s.ensure(an + wh + 1u);
    const usize cap = s.capacity();
    const usize room = (an + wh + 1u) < cap ? (an + wh + 1u) : cap;
    if ( room <= wh ) {
      s.finish(0);
      return *this;
    }
    const usize keep = room - wh;
    const usize src = keep < an ? keep : an;

    mpn::limb_t *r = s.limbs();
    usize n = wh + src;
    if ( bi != 0 ) {
      const mpn::limb_t top = mpn::lshift(r + wh, r, src, bi);
      if ( top != 0 && n < room ) {
        r[n] = top;
        ++n;
      }
    } else {
      mpn::copyd(r + wh, r, src);
    }
    mpn::zero(r, wh);
    s.finish(n);
    return *this;
  }

  constexpr arbuint &
  operator>>=(usize k)
  {
    const usize an = s.size();
    if ( an == 0 || k == 0 ) return *this;
    const usize wh = k / mpn::limb_bits;
    const usize bi = k % mpn::limb_bits;
    if ( wh >= an ) {
      s.finish(0);
      return *this;
    }
    const usize keep = an - wh;
    mpn::limb_t *r = s.limbs();
    if ( bi != 0 )
      (void)mpn::rshift(r, r + wh, keep, bi);
    else
      mpn::copyi(r, r + wh, keep);
    s.finish(keep);
    return *this;
  }

  constexpr arbuint &
  operator&=(const arbuint &o)
  {
    const usize mn = s.size() < o.s.size() ? s.size() : o.s.size();
    mpn::and_n(s.limbs(), s.limbs(), o.s.limbs(), mn);
    s.finish(mn);
    return *this;
  }

  constexpr arbuint &
  operator|=(const arbuint &o)
  {
    const usize an = s.size();
    const usize bn = o.s.size();
    if ( bn == 0 ) return *this;
    const usize mx = an > bn ? an : bn;
    const usize mn = an > bn ? bn : an;
    (void)s.ensure(mx);
    mpn::limb_t *r = s.limbs();
    const mpn::limb_t *b = o.s.limbs();
    const usize cap = s.capacity();
    const usize lim = mx < cap ? mx : cap;
    mpn::ior_n(r, r, b, mn < lim ? mn : lim);
    for ( usize i = mn; i < lim; ++i ) r[i] = (i < an) ? r[i] : b[i];
    s.finish(lim);
    return *this;
  }

  constexpr arbuint &
  operator^=(const arbuint &o)
  {
    const usize an = s.size();
    const usize bn = o.s.size();
    if ( bn == 0 ) return *this;
    const usize mx = an > bn ? an : bn;
    const usize mn = an > bn ? bn : an;
    (void)s.ensure(mx);
    mpn::limb_t *r = s.limbs();
    const mpn::limb_t *b = o.s.limbs();
    const usize cap = s.capacity();
    const usize lim = mx < cap ? mx : cap;
    mpn::xor_n(r, r, b, mn < lim ? mn : lim);
    for ( usize i = mn; i < lim; ++i ) r[i] = (i < an) ? r[i] : b[i];
    s.finish(lim);
    return *this;
  }

  constexpr arbuint &
  operator++()
  {
    return *this += arbuint{ 1u };
  }

  constexpr arbuint &
  operator--()
  {
    return *this -= arbuint{ 1u };
  }

  constexpr arbuint
  operator++(int)
  {
    arbuint t(*this);
    ++*this;
    return t;
  }

  constexpr arbuint
  operator--(int)
  {
    arbuint t(*this);
    --*this;
    return t;
  }

  template<micron::integral I>
  constexpr void
  __assign_integral(I v)
  {
    using U = micron::make_unsigned_t<I>;
    U u = static_cast<U>(v);
    if ( u == 0 ) {
      s.finish(0);
      return;
    }
    constexpr usize take = (sizeof(U) + sizeof(mpn::limb_t) - 1u) / sizeof(mpn::limb_t);
    if ( !s.ensure(take) ) {

      (void)s.ensure(cap_limbs);
    }
    const usize cap = s.capacity();
    const usize lim = take < cap ? take : cap;
    mpn::limb_t *r = s.limbs();
    for ( usize i = 0; i < lim; ++i ) {
      r[i] = static_cast<mpn::limb_t>(u);
      if constexpr ( sizeof(U) > sizeof(mpn::limb_t) )
        u = static_cast<U>(u >> mpn::limb_bits);
      else
        u = 0;
    }
    s.finish(lim);
  }

  constexpr void
  __assign_limbs(const mpn::limb_t *p, usize n)
  {
    (void)s.ensure(n);
    const usize cap = s.capacity();
    mpn::copyi(s.limbs(), p, n < cap ? n : cap);
    s.finish(n);
  }

  constexpr arbuint &
  __mul_add_1(mpn::limb_t mul, mpn::limb_t add)
  {
    const usize n = s.size();
    (void)s.ensure(n + 1u);
    const usize cap = s.capacity();
    mpn::limb_t *r = s.limbs();
    mpn::limb_t hi = (n != 0) ? mpn::mul_1(r, r, n, mul) : mpn::limb_t{ 0 };
    if ( n != 0 ) hi = static_cast<mpn::limb_t>(hi + mpn::add_1(r, r, n, add));
    usize k = n;
    if ( n == 0 ) {
      if ( add != 0 && cap > 0 ) {
        r[0] = add;
        k = 1;
      }
    } else if ( hi != 0 && k < cap ) {
      r[k] = hi;
      ++k;
    }
    s.finish(k);
    return *this;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  __raw_ensure(usize n)
  {
    return s.ensure(n);
  }

  [[gnu::always_inline]] constexpr void
  __raw_finish(usize n) noexcept
  {
    s.finish(n);
  }

  [[gnu::always_inline]] static constexpr void
  __mul_into(mpn::limb_t *__restrict__ t, const mpn::limb_t *__restrict__ ap, usize an, const mpn::limb_t *__restrict__ bp, usize bn,
             mpn::limb_t *work) noexcept
  {
    if constexpr ( bounded )
      mpn::mul_fixed<cap_limbs, cap_limbs, Solver>(t, ap, an, bp, bn, work);
    else if constexpr ( !micron::is_same_v<Solver, solver::automatic> )
      mpn::mul_with<mpn::pinned_algo<Solver>()>(t, ap, an, bp, bn, work);
    else
      mpn::mul(t, ap, an, bp, bn, work);
  }

  [[gnu::always_inline]] static constexpr void
  __sqr_into(mpn::limb_t *__restrict__ t, const mpn::limb_t *__restrict__ ap, usize an, mpn::limb_t *work) noexcept
  {
    if constexpr ( bounded )
      mpn::sqr_fixed<cap_limbs, Solver>(t, ap, an, work);
    else if constexpr ( !micron::is_same_v<Solver, solver::automatic> )
      mpn::sqr_with<mpn::pinned_algo<Solver>()>(t, ap, an, work);
    else
      mpn::sqr(t, ap, an, work);
  }

  constexpr arbuint &
  __square()
  {
    const usize an = s.size();
    if ( an == 0 ) {
      s.finish(0);
      return *this;
    }
    const usize rn = 2u * an;
    scratch_for<mul_scratch_limbs> sc(rn + mpn::sqr_solver_itch<Solver>(an));
    mpn::limb_t *t = sc.get();
    __sqr_into(t, s.limbs(), an, t + rn);

    (void)s.ensure(rn);
    const usize cap = s.capacity();
    mpn::copyi(s.limbs(), t, rn < cap ? rn : cap);
    s.finish(rn);
    return *this;
  }

  constexpr void
  __divmod_both(const arbuint &o, arbuint &q)
  {
    const usize an = s.size();
    const usize bn = o.s.size();
    if ( bn == 0 ) micron::exc<micron::except::domain_error>("micron::math::arbuint division by zero");
    if ( an < bn ) {
      q.s.finish(0);
      return;
    }

    const usize qn = an - bn + 1u;
    const usize itch = mpn::divrem_itch(an, bn);
    scratch_for<div_scratch_limbs> sc(qn + bn + itch);
    mpn::limb_t *qp = sc.get();
    mpn::limb_t *rp = qp + qn;
    mpn::limb_t *work = rp + bn;

    mpn::divrem(qp, rp, s.limbs(), an, o.s.limbs(), bn, work);

    (void)q.s.ensure(qn);
    const usize qcap = q.s.capacity();
    mpn::copyi(q.s.limbs(), qp, qn < qcap ? qn : qcap);
    q.s.finish(qn);

    (void)s.ensure(bn);
    const usize rcap = s.capacity();
    mpn::copyi(s.limbs(), rp, bn < rcap ? bn : rcap);
    s.finish(bn);
  }

  constexpr void
  __divmod_into(const arbuint &o, bool want_quotient)
  {
    const usize an = s.size();
    const usize bn = o.s.size();
    if ( bn == 0 ) micron::exc<micron::except::domain_error>("micron::math::arbuint division by zero");
    if ( an < bn ) {
      if ( want_quotient ) s.finish(0);
      return;
    }

    const usize qn = an - bn + 1u;
    const usize itch = mpn::divrem_itch(an, bn);
    scratch_for<div_scratch_limbs> sc(qn + bn + itch);
    mpn::limb_t *qp = sc.get();
    mpn::limb_t *rp = qp + qn;
    mpn::limb_t *work = rp + bn;

    mpn::divrem(qp, rp, s.limbs(), an, o.s.limbs(), bn, work);

    const mpn::limb_t *src = want_quotient ? qp : rp;
    const usize n = want_quotient ? qn : bn;
    (void)s.ensure(n);
    const usize cap = s.capacity();
    mpn::copyi(s.limbs(), src, n < cap ? n : cap);
    s.finish(n);
  }

  constexpr arbuint &
  flip_bits() noexcept
    requires(Bits != 0)
  {
    (void)s.ensure(cap_limbs);
    mpn::limb_t *r = s.limbs();
    const usize n = s.size();
    mpn::com(r, r, n);
    for ( usize i = n; i < cap_limbs; ++i ) r[i] = mpn::limb_max;
    s.finish(cap_limbs);
    return *this;
  }
};

#define __micron_arbuint_binop(OP)                                                                                                         \
  template<usize B, arb_solver S, class A>                                                                                                 \
  [[nodiscard]] inline constexpr arbuint<B, S, A> operator OP(const arbuint<B, S, A> &a, const arbuint<B, S, A> &b)                        \
  {                                                                                                                                        \
    arbuint<B, S, A> r(a);                                                                                                                 \
    r OP## = b;                                                                                                                            \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard]] inline constexpr arbuint<B, S, A> operator OP(const arbuint<B, S, A> &a, I v)                                              \
  {                                                                                                                                        \
    arbuint<B, S, A> r(a);                                                                                                                 \
    r OP## = arbuint<B, S, A>(v);                                                                                                          \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard]] inline constexpr arbuint<B, S, A> operator OP(I v, const arbuint<B, S, A> &a)                                              \
  {                                                                                                                                        \
    arbuint<B, S, A> r(v);                                                                                                                 \
    r OP## = a;                                                                                                                            \
    return r;                                                                                                                              \
  }

__micron_arbuint_binop(+) __micron_arbuint_binop(-) __micron_arbuint_binop(*) __micron_arbuint_binop(/) __micron_arbuint_binop(%)
    __micron_arbuint_binop(&) __micron_arbuint_binop(|) __micron_arbuint_binop(^)

#undef __micron_arbuint_binop

        template<usize B, arb_solver S, class A>
        [[nodiscard]] inline constexpr arbuint<B, S, A>
        operator<<(const arbuint<B, S, A> &a, usize k)
{
  arbuint<B, S, A> r(a);
  r <<= k;
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
operator>>(const arbuint<B, S, A> &a, usize k)
{
  arbuint<B, S, A> r(a);
  r >>= k;
  return r;
}

template<usize B, arb_solver S, class A>
  requires(B != 0)
[[nodiscard]] inline constexpr arbuint<B, S, A>
operator~(const arbuint<B, S, A> &a)
{
  arbuint<B, S, A> r(a);
  r.flip_bits();
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::always_inline]] inline constexpr const arbuint<B, S, A> &
operator+(const arbuint<B, S, A> &a) noexcept
{
  return a;
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::always_inline]] inline constexpr int
cmp(const arbuint<B, S, A> &a, const arbuint<B, S, A> &b) noexcept
{
  return mpn::cmp_var(a.limbs(), a.size(), b.limbs(), b.size());
}

template<usize B, arb_solver S, class A, micron::integral I>
[[nodiscard, gnu::flatten]] inline constexpr int
cmp(const arbuint<B, S, A> &a, I v) noexcept
{
  using U = micron::make_unsigned_t<I>;
  constexpr usize take = (sizeof(U) + sizeof(mpn::limb_t) - 1u) / sizeof(mpn::limb_t);
  U u = static_cast<U>(v);
  mpn::limb_t tmp[take] = {};
  for ( usize i = 0; i < take; ++i ) {
    tmp[i] = static_cast<mpn::limb_t>(u);
    if constexpr ( sizeof(U) > sizeof(mpn::limb_t) )
      u = static_cast<U>(u >> mpn::limb_bits);
    else
      u = 0;
  }
  return mpn::cmp_var(a.limbs(), a.size(), tmp, mpn::normalize(tmp, take));
}

template<usize B, arb_solver S, class A, micron::integral I>
[[nodiscard, gnu::always_inline]] inline constexpr int
cmp(I v, const arbuint<B, S, A> &a) noexcept
{
  return -cmp(a, v);
}

#define __micron_arbuint_cmpop(OP)                                                                                                         \
  template<usize B, arb_solver S, class A>                                                                                                 \
  [[nodiscard, gnu::always_inline]] inline constexpr bool operator OP(const arbuint<B, S, A> &a, const arbuint<B, S, A> &b) noexcept       \
  {                                                                                                                                        \
    return cmp(a, b) OP 0;                                                                                                                 \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard, gnu::always_inline]] inline constexpr bool operator OP(const arbuint<B, S, A> &a, I v) noexcept                             \
  {                                                                                                                                        \
    return cmp(a, v) OP 0;                                                                                                                 \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard, gnu::always_inline]] inline constexpr bool operator OP(I v, const arbuint<B, S, A> &a) noexcept                             \
  {                                                                                                                                        \
    return cmp(v, a) OP 0;                                                                                                                 \
  }

__micron_arbuint_cmpop(==) __micron_arbuint_cmpop(!=) __micron_arbuint_cmpop(<) __micron_arbuint_cmpop(<=) __micron_arbuint_cmpop(>)
    __micron_arbuint_cmpop(>=)

#undef __micron_arbuint_cmpop

        template<usize B, arb_solver S, class A>
        struct div_result {
  arbuint<B, S, A> quot;
  arbuint<B, S, A> rem;
};

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr div_result<B, S, A>
divmod(const arbuint<B, S, A> &a, const arbuint<B, S, A> &b)
{
  div_result<B, S, A> r{ arbuint<B, S, A>{}, a };
  r.rem.__divmod_both(b, r.quot);
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
sqr(const arbuint<B, S, A> &a)
{
  arbuint<B, S, A> r(a);
  r.__square();
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbuint<B, S, A>
pow(const arbuint<B, S, A> &a, u64 ex)
{
  if ( ex == 0 ) return arbuint<B, S, A>::one();
  arbuint<B, S, A> base(a);
  arbuint<B, S, A> acc = arbuint<B, S, A>::one();
  while ( ex != 0 ) {
    if ( (ex & 1u) != 0 ) acc *= base;
    ex >>= 1;
    if ( ex != 0 ) base *= base;
  }
  return acc;
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::always_inline]] inline constexpr usize
bit_length(const arbuint<B, S, A> &a) noexcept
{
  return a.bit_length();
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::always_inline]] inline constexpr usize
popcount(const arbuint<B, S, A> &a) noexcept
{
  return a.popcount();
}

template<usize B, arb_solver S, class A>
[[gnu::always_inline]] inline constexpr void
swap(arbuint<B, S, A> &a, arbuint<B, S, A> &b) noexcept
{
  a.swap(b);
}

};      // namespace math
};      // namespace micron
