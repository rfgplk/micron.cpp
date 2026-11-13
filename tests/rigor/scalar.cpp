//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <micron/math/scalar.hpp>
#include <snowball/snowball.hpp>

static_assert(micron::math::scalar::add(2.0, 3.0) == 5.0);
static_assert(micron::math::scalar::sub(2.0f, 3.0f) == -1.0f);
static_assert(micron::math::scalar::mul(2.0, 3.0) == 6.0);
static_assert(micron::math::scalar::div(6.0, 3.0) == 2.0);
static_assert(micron::math::scalar::neg(3.0) == -3.0);

int
main()
{
  sb::test_case("strict scalar arithmetic exactly matches individual IEEE operations");
  u64 state = 0x912ee0d743ab132full;
  for ( u32 i = 0; i < 10000; ++i ) {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    f64 left = static_cast<f64>(static_cast<i32>(state >> 32)) / 1024;
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    f64 right = static_cast<f64>(static_cast<i32>(state >> 32)) / 1024;
    if ( right == 0 ) right = 1;
    asm volatile("" : "+m"(left), "+m"(right) : : "memory");
    sb::require(micron::math::scalar::add(left, right), left + right);
    sb::require(micron::math::scalar::sub(left, right), left - right);
    sb::require(micron::math::scalar::mul(left, right), left * right);
    sb::require(micron::math::scalar::div(left, right), left / right);
    sb::require(micron::math::scalar::neg(left), -left);
  }
  const f64 negative_zero = __builtin_bit_cast(f64, u64{ 0x8000000000000000ull });
  sb::require(__builtin_bit_cast(u64, micron::math::scalar::mul(negative_zero, f64{ 2 })), u64{ 0x8000000000000000ull });
  sb::require(__builtin_bit_cast(u64, micron::math::scalar::neg(negative_zero)), u64{ 0 });
  sb::end_test_case();
  return 1;
}
