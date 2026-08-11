//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../tags.hpp"
#include "../../types.hpp"
#include "storage.hpp"
#include "tags.hpp"
#include "unsigned.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// sign and magnitude
// cpp like mechanics

namespace micron
{
namespace math
{

template<usize Bits = 0, arb_solver Solver = solver::automatic, class Alloc = micron::allocator_serial<>> class arbint
{
public:
  using mag_type = arbuint<Bits, Solver, Alloc>;
  using value_type = mpn::limb_t;
  using category_type = micron::numeric_tag;
  using solver_type = Solver;
  using allocator_type = Alloc;

  static constexpr usize width_bits = Bits;
  static constexpr bool bounded = mag_type::bounded;
  static constexpr usize cap_limbs = mag_type::cap_limbs;

private:
  mag_type m;
  bool ng;

  static constexpr usize bit_scratch_limbs = bounded ? 2u * (cap_limbs + 1u) : 1u;
  using bit_scratch = __arb::scratch<bit_scratch_limbs, Alloc, bounded>;

  [[gnu::always_inline]] constexpr void
  __fix_zero() noexcept
  {
    if ( m.is_zero() ) ng = false;
  }

  constexpr void
  __to_twos(mpn::limb_t *out, usize n) const noexcept
  {
    const usize k = m.size();
    mpn::copyi(out, m.limbs(), k);
    mpn::zero(out + k, n - k);
    if ( ng ) (void)mpn::neg(out, out, n);
  }

  constexpr void
  __from_twos(mpn::limb_t *in, usize n)
  {
    const bool negative = (in[n - 1u] & mpn::limb_msb) != 0;
    if ( negative ) (void)mpn::neg(in, in, n);
    m.__assign_limbs(in, n);
    ng = negative;
    __fix_zero();
  }

  enum class bitop : u8 { b_and, b_ior, b_xor };

  constexpr arbint &
  __bitwise(const arbint &o, bitop which)
  {
    const usize n = (m.size() > o.m.size() ? m.size() : o.m.size()) + 1u;
    bit_scratch sc(2u * n);
    mpn::limb_t *a = sc.get();
    mpn::limb_t *b = a + n;
    __to_twos(a, n);
    o.__to_twos(b, n);
    switch ( which ) {
    case bitop::b_and:
      mpn::and_n(a, a, b, n);
      break;
    case bitop::b_ior:
      mpn::ior_n(a, a, b, n);
      break;
    default:
      mpn::xor_n(a, a, b, n);
      break;
    }
    __from_twos(a, n);
    return *this;
  }

public:
  constexpr arbint() noexcept : m(), ng(false) { }

  constexpr arbint(const arbint &) = default;
  constexpr arbint(arbint &&) noexcept = default;
  constexpr arbint &operator=(const arbint &) = default;
  constexpr arbint &operator=(arbint &&) noexcept = default;
  ~arbint() = default;

  template<micron::integral I> constexpr arbint(I v) : m(), ng(false) { __assign_integral(v); }

  constexpr arbint(const mag_type &mag, bool negative) : m(mag), ng(negative) { __fix_zero(); }

  template<micron::integral I>
  constexpr arbint &
  operator=(I v)
  {
    __assign_integral(v);
    return *this;
  }

  [[nodiscard, gnu::always_inline]] constexpr const mag_type &
  magnitude() const noexcept
  {
    return m;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  negative() const noexcept
  {
    return ng;
  }

  [[nodiscard, gnu::always_inline]] constexpr int
  sign() const noexcept
  {
    return m.is_zero() ? 0 : (ng ? -1 : 1);
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  is_zero() const noexcept
  {
    return m.is_zero();
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  size() const noexcept
  {
    return m.size();
  }

  [[nodiscard, gnu::always_inline]] constexpr const mpn::limb_t *
  limbs() const noexcept
  {
    return m.limbs();
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  bit_length() const noexcept
  {
    return m.bit_length();
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  odd() const noexcept
  {
    return m.odd();
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  even() const noexcept
  {
    return m.even();
  }

  [[nodiscard, gnu::always_inline]] constexpr const mpn::limb_t &
  operator[](usize i) const noexcept
  {
    return m[i];
  }

  [[nodiscard, gnu::always_inline]] explicit constexpr
  operator bool() const noexcept
  {
    return !m.is_zero();
  }

  template<micron::integral I>
  [[nodiscard]] explicit constexpr
  operator I() const noexcept
  {
    using U = micron::make_unsigned_t<I>;
    const U u = static_cast<U>(m);
    return static_cast<I>(ng ? static_cast<U>(static_cast<U>(0) - u) : u);
  }

  [[nodiscard]] static constexpr arbint
  zero() noexcept
  {
    return arbint{};
  }

  [[nodiscard]] static constexpr arbint
  one()
  {
    return arbint{ 1 };
  }

  [[nodiscard]] static constexpr arbint
  power_of_two(usize k)
  {
    return arbint(mag_type::power_of_two(k), false);
  }

  [[gnu::always_inline]] constexpr void
  set_zero() noexcept
  {
    m.set_zero();
    ng = false;
  }

  [[gnu::always_inline]] constexpr void
  negate() noexcept
  {
    ng = !ng;
    __fix_zero();
  }

  [[gnu::always_inline]] constexpr void
  swap(arbint &o) noexcept
  {
    m.swap(o.m);
    const bool t = ng;
    ng = o.ng;
    o.ng = t;
  }

  constexpr arbint &
  operator+=(const arbint &o)
  {
    if ( o.m.is_zero() ) return *this;
    if ( m.is_zero() ) {
      m = o.m;
      ng = o.ng;
      return *this;
    }
    if ( ng == o.ng ) {
      m += o.m;
      return *this;
    }
    const int c = cmp(m, o.m);
    if ( c == 0 ) {
      set_zero();
      return *this;
    }
    if ( c > 0 ) {
      m -= o.m;
    } else {
      mag_type t(o.m);
      t -= m;
      m.swap(t);
      ng = o.ng;
    }
    __fix_zero();
    return *this;
  }

  constexpr arbint &
  operator-=(const arbint &o)
  {
    if ( o.m.is_zero() ) return *this;
    if ( this == &o ) {
      set_zero();
      return *this;
    }
    arbint t(o);
    t.ng = !t.ng;
    return *this += t;
  }

  constexpr arbint &
  operator*=(const arbint &o)
  {
    const bool sign = ng != o.ng;
    m *= o.m;
    ng = sign;
    __fix_zero();
    return *this;
  }

  constexpr arbint &
  operator/=(const arbint &o)
  {
    const bool sign = ng != o.ng;
    m /= o.m;
    ng = sign;
    __fix_zero();
    return *this;
  }

  constexpr arbint &
  operator%=(const arbint &o)
  {
    m %= o.m;
    __fix_zero();
    return *this;
  }

  constexpr arbint &
  operator<<=(usize k)
  {
    m <<= k;
    __fix_zero();
    return *this;
  }

  constexpr arbint &
  operator>>=(usize k)
  {
    if ( m.is_zero() || k == 0 ) return *this;
    const bool lost = ng && (mpn::scan1(m.limbs(), m.size()) < k);
    m >>= k;
    if ( lost ) m += mag_type::one();
    __fix_zero();
    return *this;
  }

  constexpr arbint &
  operator&=(const arbint &o)
  {
    return __bitwise(o, bitop::b_and);
  }

  constexpr arbint &
  operator|=(const arbint &o)
  {
    return __bitwise(o, bitop::b_ior);
  }

  constexpr arbint &
  operator^=(const arbint &o)
  {
    return __bitwise(o, bitop::b_xor);
  }

  constexpr arbint &
  operator++()
  {
    return *this += arbint{ 1 };
  }

  constexpr arbint &
  operator--()
  {
    return *this -= arbint{ 1 };
  }

  constexpr arbint
  operator++(int)
  {
    arbint t(*this);
    ++*this;
    return t;
  }

  constexpr arbint
  operator--(int)
  {
    arbint t(*this);
    --*this;
    return t;
  }

  template<micron::integral I>
  constexpr void
  __assign_integral(I v)
  {
    if constexpr ( micron::is_signed_v<I> ) {
      using U = micron::make_unsigned_t<I>;
      const bool negative = v < 0;
      const U u = negative ? static_cast<U>(static_cast<U>(0) - static_cast<U>(v)) : static_cast<U>(v);
      m = u;
      ng = negative;
    } else {
      m = v;
      ng = false;
    }
    __fix_zero();
  }
};

#define __micron_arbint_binop(OP)                                                                                                          \
  template<usize B, arb_solver S, class A>                                                                                                 \
  [[nodiscard]] inline constexpr arbint<B, S, A> operator OP(const arbint<B, S, A> &a, const arbint<B, S, A> &b)                           \
  {                                                                                                                                        \
    arbint<B, S, A> r(a);                                                                                                                  \
    r OP## = b;                                                                                                                            \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard]] inline constexpr arbint<B, S, A> operator OP(const arbint<B, S, A> &a, I v)                                                \
  {                                                                                                                                        \
    arbint<B, S, A> r(a);                                                                                                                  \
    r OP## = arbint<B, S, A>(v);                                                                                                           \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard]] inline constexpr arbint<B, S, A> operator OP(I v, const arbint<B, S, A> &a)                                                \
  {                                                                                                                                        \
    arbint<B, S, A> r(v);                                                                                                                  \
    r OP## = a;                                                                                                                            \
    return r;                                                                                                                              \
  }

__micron_arbint_binop(+) __micron_arbint_binop(-) __micron_arbint_binop(*) __micron_arbint_binop(/) __micron_arbint_binop(%)
    __micron_arbint_binop(&) __micron_arbint_binop(|) __micron_arbint_binop(^)

#undef __micron_arbint_binop

        template<usize B, arb_solver S, class A>
        [[nodiscard]] inline constexpr arbint<B, S, A>
        operator<<(const arbint<B, S, A> &a, usize k)
{
  arbint<B, S, A> r(a);
  r <<= k;
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
operator>>(const arbint<B, S, A> &a, usize k)
{
  arbint<B, S, A> r(a);
  r >>= k;
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
operator-(const arbint<B, S, A> &a)
{
  arbint<B, S, A> r(a);
  r.negate();
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::always_inline]] inline constexpr const arbint<B, S, A> &
operator+(const arbint<B, S, A> &a) noexcept
{
  return a;
}

// ~a == -a - 1, the two's complement identity, and exact at any width
template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
operator~(const arbint<B, S, A> &a)
{
  arbint<B, S, A> r(a);
  r.negate();
  r -= arbint<B, S, A>{ 1 };
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::flatten]] inline constexpr int
cmp(const arbint<B, S, A> &a, const arbint<B, S, A> &b) noexcept
{
  const int sa = a.sign();
  const int sb = b.sign();
  if ( sa != sb ) return sa < sb ? -1 : 1;
  if ( sa == 0 ) return 0;
  const int c = cmp(a.magnitude(), b.magnitude());
  return sa < 0 ? -c : c;
}

template<usize B, arb_solver S, class A, micron::integral I>
[[nodiscard, gnu::flatten]] inline constexpr int
cmp(const arbint<B, S, A> &a, I v)
{
  return cmp(a, arbint<B, S, A>(v));
}

template<usize B, arb_solver S, class A, micron::integral I>
[[nodiscard, gnu::always_inline]] inline constexpr int
cmp(I v, const arbint<B, S, A> &a)
{
  return -cmp(a, v);
}

#define __micron_arbint_cmpop(OP)                                                                                                          \
  template<usize B, arb_solver S, class A>                                                                                                 \
  [[nodiscard, gnu::always_inline]] inline constexpr bool operator OP(const arbint<B, S, A> &a, const arbint<B, S, A> &b) noexcept         \
  {                                                                                                                                        \
    return cmp(a, b) OP 0;                                                                                                                 \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard, gnu::always_inline]] inline constexpr bool operator OP(const arbint<B, S, A> &a, I v)                                       \
  {                                                                                                                                        \
    return cmp(a, v) OP 0;                                                                                                                 \
  }                                                                                                                                        \
  template<usize B, arb_solver S, class A, micron::integral I>                                                                             \
  [[nodiscard, gnu::always_inline]] inline constexpr bool operator OP(I v, const arbint<B, S, A> &a)                                       \
  {                                                                                                                                        \
    return cmp(v, a) OP 0;                                                                                                                 \
  }

__micron_arbint_cmpop(==) __micron_arbint_cmpop(!=) __micron_arbint_cmpop(<) __micron_arbint_cmpop(<=) __micron_arbint_cmpop(>)
    __micron_arbint_cmpop(>=)

#undef __micron_arbint_cmpop

        template<usize B, arb_solver S, class A>
        struct sdiv_result {
  arbint<B, S, A> quot;
  arbint<B, S, A> rem;
};

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr sdiv_result<B, S, A>
divmod(const arbint<B, S, A> &a, const arbint<B, S, A> &b)
{
  sdiv_result<B, S, A> r{ a, a };
  r.quot /= b;
  r.rem %= b;
  return r;
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
abs(const arbint<B, S, A> &a)
{
  return arbint<B, S, A>(a.magnitude(), false);
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::always_inline]] inline constexpr int
sign(const arbint<B, S, A> &a) noexcept
{
  return a.sign();
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
sqr(const arbint<B, S, A> &a)
{
  return arbint<B, S, A>(sqr(a.magnitude()), false);
}

template<usize B, arb_solver S, class A>
[[nodiscard]] inline constexpr arbint<B, S, A>
pow(const arbint<B, S, A> &a, u64 e)
{
  const bool sign = a.negative() && (e & 1u) != 0;
  return arbint<B, S, A>(pow(a.magnitude(), e), sign);
}

template<usize B, arb_solver S, class A>
[[nodiscard, gnu::always_inline]] inline constexpr usize
bit_length(const arbint<B, S, A> &a) noexcept
{
  return a.bit_length();
}

template<usize B, arb_solver S, class A>
[[gnu::always_inline]] inline constexpr void
swap(arbint<B, S, A> &a, arbint<B, S, A> &b) noexcept
{
  a.swap(b);
}

};      // namespace math
};      // namespace micron
