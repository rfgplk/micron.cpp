//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <micron/math/rational.hpp>
#include <snowball/snowball.hpp>

namespace math = micron::math;

namespace
{

i64
gcd(i64 a, i64 b)
{
  if ( a < 0 ) a = -a;
  while ( b ) {
    const auto remainder = a % b;
    a = b;
    b = remainder;
  }
  return a;
}

void
require_value(const math::rational_result &actual, i64 numerator, i64 denominator)
{
  sb::require_true(static_cast<bool>(actual));
  if ( denominator < 0 ) {
    numerator = -numerator;
    denominator = -denominator;
  }
  const i64 common = gcd(numerator, denominator);
  numerator /= common;
  denominator /= common;
  sb::require_true(actual.value.numerator() == numerator);
  sb::require_true(actual.value.denominator() == denominator);
}

}      // namespace

int
main()
{
  sb::test_case("canonical arbitrary precision rationals and invalid denominators");
  require_value(math::make_rational(6, -8), -3, 4);
  require_value(math::make_rational(0, -72), 0, 1);
  require_value(math::make_rational(-12, -4), 3, 1);
  sb::require(math::make_rational(1, 0).error, math::rational_error::zero_denominator);
  const auto huge = math::arbint<>::power_of_two(4096) + 1;
  const auto a = math::make_rational(huge, math::arbint<>(7));
  const auto b = math::make_rational(math::arbint<>(7), huge);
  sb::require_true(static_cast<bool>(a) && static_cast<bool>(b));
  require_value(math::rational_multiply(a.value, b.value), 1, 1);
  require_value(math::rational_divide(a.value, a.value), 1, 1);
  sb::require_true(a.value.numerator() == huge);
  sb::end_test_case();

  sb::test_case("fixed-seed exact operations agree with independent small integer model");
  u64 state = 0x562a9180d47cbe3fULL;
  const auto next = [&]() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  };
  for ( u32 i = 0; i < 3000; ++i ) {
    const i64 an = static_cast<i64>(next() % 2001) - 1000;
    const i64 bn = static_cast<i64>(next() % 2001) - 1000;
    const i64 ad = static_cast<i64>(next() % 97) + 1;
    const i64 bd = static_cast<i64>(next() % 97) + 1;
    const auto left = math::make_rational(an, ad), right = math::make_rational(bn, bd);
    require_value(math::rational_add(left.value, right.value), an * bd + bn * ad, ad * bd);
    require_value(math::rational_subtract(left.value, right.value), an * bd - bn * ad, ad * bd);
    require_value(math::rational_multiply(left.value, right.value), an * bn, ad * bd);
    if ( bn ) require_value(math::rational_divide(left.value, right.value), an * bd, ad * bn);
    const auto order = math::rational_compare(left.value, right.value);
    sb::require_true(static_cast<bool>(order));
    const i64 difference = an * bd - bn * ad;
    sb::require(order.ordering, difference < 0 ? i32{ -1 } : difference > 0 ? i32{ 1 } : i32{ 0 });
    sb::require_true(left.value == math::make_rational(an, ad).value);
    sb::require_true(right.value == math::make_rational(bn, bd).value);
  }
  sb::end_test_case();

  sb::test_case("negative powers and all resource failures retain exact operands");
  const auto fraction = math::make_rational(-2, 3);
  require_value(math::rational_power(fraction.value, -3), -27, 8);
  require_value(math::rational_power(fraction.value, 4), 16, 81);
  require_value(math::rational_power(fraction.value, 0), 1, 1);
  sb::require(math::rational_power(math::rational{}, 0).error, math::rational_error::zero_denominator);
  sb::require(math::rational_power(math::rational{}, -1).error, math::rational_error::zero_denominator);
  sb::require(math::rational_divide(fraction.value, math::rational{}).error, math::rational_error::zero_denominator);
  math::rational_limits small{ 1, 4, 16 };
  const auto wider = math::arbint<>::power_of_two(math::mpn::limb_bits) + 1;
  sb::require(math::make_rational(wider, math::arbint<>(1), small).error, math::rational_error::limb_limit);
  require_value(math::make_rational(wider, wider, small), 1, 1);
  sb::require(math::rational_power(fraction.value, 17, small).error, math::rational_error::exponent_limit);
  sb::require(math::rational_power(fraction.value, (-9223372036854775807LL - 1), small).error, math::rational_error::exponent_limit);
  const auto near = math::make_rational(math::arbint<>::power_of_two(math::mpn::limb_bits - 1), math::arbint<>(1), small);
  sb::require_true(static_cast<bool>(near));
  sb::require(math::rational_add(near.value, near.value, small).error, math::rational_error::limb_limit);
  sb::require_true(near.value.numerator() == math::arbint<>::power_of_two(math::mpn::limb_bits - 1));
  sb::require(math::make_rational(1, 1, { 0, 0, 0 }).error, math::rational_error::limb_limit);
  sb::require(math::rational_compare(near.value, fraction.value, { 0, 0, 0 }).error, math::rational_error::limb_limit);
  sb::end_test_case();
  return 1;
}
