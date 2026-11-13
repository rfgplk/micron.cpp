//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "arbint.hpp"

namespace micron::math
{

enum class rational_error : u8 { none, zero_denominator, limb_limit, exponent_limit };

struct rational_limits {
  usize max_limbs = 4096;
  usize max_work_limbs = 8193;
  u64 max_power = 1048576;
};

namespace __impl
{
struct access;
}

// Canonical sign/magnitude fraction
class rational
{
  arbint<> __numerator{};
  arbuint<> __denominator{ 1 };
  friend struct __impl::access;

public:
  rational() = default;
  rational(const rational &) = default;
  rational(rational &&) noexcept = default;
  rational &operator=(const rational &) = default;
  rational &operator=(rational &&) noexcept = default;

  const arbint<> &
  numerator() const noexcept
  {
    return __numerator;
  }

  const arbuint<> &
  denominator() const noexcept
  {
    return __denominator;
  }

  bool
  is_zero() const noexcept
  {
    return __numerator.is_zero();
  }

  bool
  is_integer() const noexcept
  {
    return __denominator == 1;
  }

  int
  sign() const noexcept
  {
    return __numerator.sign();
  }
};

inline bool
operator==(const rational &a, const rational &b) noexcept
{
  return a.numerator() == b.numerator() && a.denominator() == b.denominator();
}

struct rational_result {
  rational value{};
  rational_error error = rational_error::none;

  explicit
  operator bool() const noexcept
  {
    return error == rational_error::none;
  }
};

struct rational_order_result {
  i32 ordering = 0;
  rational_error error = rational_error::none;

  explicit
  operator bool() const noexcept
  {
    return error == rational_error::none;
  }
};

namespace __impl
{

struct access {
  static rational
  make(arbint<> numerator, arbuint<> denominator)
  {
    rational result;
    result.__numerator = micron::move(numerator);
    result.__denominator = micron::move(denominator);
    return result;
  }
};

inline bool
fits(const rational &a, rational_limits limits) noexcept
{
  return limits.max_limbs && a.numerator().size() <= limits.max_limbs && a.denominator().size() <= limits.max_limbs
         && a.numerator().size() <= limits.max_work_limbs && a.denominator().size() <= limits.max_work_limbs;
}

inline bool
product_fits(usize a, usize b, usize maximum) noexcept
{
  return a == 0 || b == 0 || (a <= maximum && b <= maximum - a);
}

inline rational_result
fail(rational_error error)
{
  return { {}, error };
}

inline rational_result
normalize(arbint<> numerator, arbuint<> denominator, rational_limits limits)
{
  if ( denominator.is_zero() ) return fail(rational_error::zero_denominator);
  if ( !limits.max_limbs || denominator.size() > limits.max_work_limbs || numerator.size() > limits.max_work_limbs )
    return fail(rational_error::limb_limit);
  if ( numerator.is_zero() ) return {};
  const auto common = micron::math::gcd(numerator.magnitude(), denominator);
  if ( common != 1 ) {
    numerator /= arbint<>(common, false);
    denominator /= common;
  }
  if ( numerator.size() > limits.max_limbs || denominator.size() > limits.max_limbs ) return fail(rational_error::limb_limit);
  return { access::make(micron::move(numerator), micron::move(denominator)), rational_error::none };
}

}      // namespace __impl

inline rational_result
make_rational(const arbint<> &numerator, const arbint<> &denominator, rational_limits limits = {})
{
  if ( numerator.size() > limits.max_work_limbs || denominator.size() > limits.max_work_limbs )
    return __impl::fail(rational_error::limb_limit);
  auto adjusted = numerator;
  if ( denominator.negative() ) adjusted.negate();
  return __impl::normalize(micron::move(adjusted), denominator.magnitude(), limits);
}

inline rational_result
make_rational(i64 numerator, i64 denominator = 1, rational_limits limits = {})
{
  return make_rational(arbint<>(numerator), arbint<>(denominator), limits);
}

inline rational_result
rational_negate(const rational &a, rational_limits limits = {})
{
  if ( !__impl::fits(a, limits) ) return __impl::fail(rational_error::limb_limit);
  auto numerator = a.numerator();
  numerator.negate();
  return { __impl::access::make(micron::move(numerator), a.denominator()), rational_error::none };
}

inline rational_result
rational_add(const rational &a, const rational &b, rational_limits limits = {})
{
  using namespace __impl;
  if ( !fits(a, limits) || !fits(b, limits) ) return fail(rational_error::limb_limit);
  if ( a.is_zero() ) return { b };
  if ( b.is_zero() ) return { a };
  if ( a.denominator() == b.denominator() && a.numerator() == -b.numerator() ) return {};
  const auto common = micron::math::gcd(a.denominator(), b.denominator());
  const auto ad = a.denominator() / common;
  const auto bd = b.denominator() / common;
  if ( !product_fits(a.numerator().size(), bd.size(), limits.max_work_limbs)
       || !product_fits(b.numerator().size(), ad.size(), limits.max_work_limbs)
       || !product_fits(ad.size(), b.denominator().size(), limits.max_work_limbs) )
    return fail(rational_error::limb_limit);
  auto left = a.numerator() * arbint<>(bd, false);
  const auto right = b.numerator() * arbint<>(ad, false);
  if ( left.sign() == right.sign() && (left.size() >= limits.max_work_limbs || right.size() >= limits.max_work_limbs) )
    return fail(rational_error::limb_limit);
  left += right;
  return normalize(micron::move(left), ad * b.denominator(), limits);
}

inline rational_result
rational_subtract(const rational &a, const rational &b, rational_limits limits = {})
{
  auto negative = rational_negate(b, limits);
  if ( !negative ) return negative;
  return rational_add(a, negative.value, limits);
}

inline rational_result
rational_multiply(const rational &a, const rational &b, rational_limits limits = {})
{
  using namespace __impl;
  if ( !fits(a, limits) || !fits(b, limits) ) return fail(rational_error::limb_limit);
  if ( a.is_zero() || b.is_zero() ) return {};
  const auto ga = micron::math::gcd(a.numerator().magnitude(), b.denominator());
  const auto gb = micron::math::gcd(b.numerator().magnitude(), a.denominator());
  const auto an = a.numerator() / arbint<>(ga, false);
  const auto bn = b.numerator() / arbint<>(gb, false);
  const auto ad = a.denominator() / gb;
  const auto bd = b.denominator() / ga;
  if ( !product_fits(an.size(), bn.size(), limits.max_work_limbs) || !product_fits(ad.size(), bd.size(), limits.max_work_limbs) )
    return fail(rational_error::limb_limit);
  return normalize(an * bn, ad * bd, limits);
}

inline rational_result
rational_divide(const rational &a, const rational &b, rational_limits limits = {})
{
  using namespace __impl;
  if ( b.is_zero() ) return fail(rational_error::zero_denominator);
  if ( !fits(a, limits) || !fits(b, limits) ) return fail(rational_error::limb_limit);
  if ( a.is_zero() ) return {};
  auto inverse = access::make(arbint<>(b.denominator(), b.numerator().negative()), b.numerator().magnitude());
  return rational_multiply(a, inverse, limits);
}

inline rational_result
rational_power(const rational &a, i64 exponent, rational_limits limits = {})
{
  using namespace __impl;
  const u64 power = exponent < 0 ? static_cast<u64>(-(exponent + 1)) + 1 : static_cast<u64>(exponent);
  if ( power > limits.max_power ) return fail(rational_error::exponent_limit);
  if ( !fits(a, limits) ) return fail(rational_error::limb_limit);
  // 0^0 is deliberately undefined, rather than silently introducing a value
  if ( a.is_zero() && exponent <= 0 ) return fail(rational_error::zero_denominator);
  auto base = exponent < 0 ? rational_divide(make_rational(1).value, a, limits) : rational_result{ a };
  if ( !base ) return base;
  auto result = make_rational(1, 1, limits);
  u64 remaining = power;
  while ( remaining ) {
    if ( remaining & 1 ) {
      result = rational_multiply(result.value, base.value, limits);
      if ( !result ) return result;
    }
    remaining >>= 1;
    if ( remaining ) {
      base = rational_multiply(base.value, base.value, limits);
      if ( !base ) return base;
    }
  }
  return result;
}

inline rational_order_result
rational_compare(const rational &a, const rational &b, rational_limits limits = {})
{
  using namespace __impl;
  if ( !fits(a, limits) || !fits(b, limits) ) return { 0, rational_error::limb_limit };
  if ( a.sign() != b.sign() ) return { a.sign() < b.sign() ? -1 : 1 };
  if ( a.denominator() == b.denominator() ) return { cmp(a.numerator(), b.numerator()) };
  const auto common = micron::math::gcd(a.denominator(), b.denominator());
  const auto ad = a.denominator() / common;
  const auto bd = b.denominator() / common;
  if ( !product_fits(a.numerator().size(), bd.size(), limits.max_work_limbs)
       || !product_fits(b.numerator().size(), ad.size(), limits.max_work_limbs) )
    return { 0, rational_error::limb_limit };
  const auto left = a.numerator() * arbint<>(bd, false);
  const auto right = b.numerator() * arbint<>(ad, false);
  return { cmp(left, right) };
}

};      // namespace micron::math
