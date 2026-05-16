//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/array/mdarray.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

int
main(void)
{
  sb::print("=== MDARRAY TESTS ===");

  sb::test_case("construction rank-1");
  {
    micron::mdarray<f32, 1> a(10);
    sb::require(a.size() == 10ULL);
    sb::require(a.shape(0) == 10ULL);
    sb::require(a.stride(0) == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("construction rank-2");
  {
    micron::mdarray<f32, 2> a(3, 4);
    sb::require(a.size() == 12ULL);
    sb::require(a.shape(0) == 3ULL);
    sb::require(a.shape(1) == 4ULL);
    sb::require(a.stride(0) == 4ULL);
    sb::require(a.stride(1) == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("construction rank-3");
  {
    micron::mdarray<i32, 3> a(2, 3, 4);
    sb::require(a.size() == 24ULL);
    sb::require(a.shape(0) == 2ULL);
    sb::require(a.shape(1) == 3ULL);
    sb::require(a.shape(2) == 4ULL);
  }
  sb::end_test_case();

  sb::test_case("indexed access rank-2");
  {
    micron::mdarray<f32, 2> a(3, 4);
    a(1, 2) = 1.5f;
    sb::require(a(1, 2) == 1.5f);
    sb::require(a.data()[1 * 4 + 2] == 1.5f);
  }
  sb::end_test_case();

  sb::test_case("fill - sets every element");
  {
    micron::mdarray<f32, 2> a(10, 10);
    a.fill(3.14f);
    for ( usize i = 0; i < 100; ++i ) sb::require(a.data()[i] == 3.14f);
  }
  sb::end_test_case();

  sb::test_case("operator+= - elementwise add");
  {
    micron::mdarray<f32, 1> a(100);
    micron::mdarray<f32, 1> b(100);
    a.fill(2.0f);
    b.fill(3.0f);
    a += b;
    for ( usize i = 0; i < 100; ++i ) sb::require(a.data()[i] == 5.0f);
  }
  sb::end_test_case();

  sb::test_case("operator-= - elementwise sub");
  {
    micron::mdarray<f32, 1> a(64);
    micron::mdarray<f32, 1> b(64);
    a.fill(10.0f);
    b.fill(3.0f);
    a -= b;
    for ( usize i = 0; i < 64; ++i ) sb::require(a.data()[i] == 7.0f);
  }
  sb::end_test_case();

  sb::test_case("operator*= scalar");
  {
    micron::mdarray<f32, 1> a(64);
    a.fill(2.0f);
    a *= 3.0f;
    for ( usize i = 0; i < 64; ++i ) sb::require(a.data()[i] == 6.0f);
  }
  sb::end_test_case();

  sb::test_case("sum");
  {
    micron::mdarray<i32, 1> a(10);
    for ( usize i = 0; i < 10; ++i ) a.data()[i] = (i32)i;
    sb::require(a.sum() == 45);
  }
  sb::end_test_case();

  sb::test_case("copy constructor");
  {
    micron::mdarray<f32, 2> a(3, 4);
    a.fill(7.0f);
    micron::mdarray<f32, 2> b(a);
    sb::require(b.size() == 12ULL);
    sb::require(b(2, 3) == 7.0f);
    a(0, 0) = 99.0f;
    sb::require(b(0, 0) == 7.0f);      // independent
  }
  sb::end_test_case();

  sb::test_case("move constructor");
  {
    micron::mdarray<f32, 1> a(100);
    a.fill(5.0f);
    micron::mdarray<f32, 1> b(micron::move(a));
    sb::require(b.size() == 100ULL);
    sb::require(a.size() == 0ULL);
    sb::require(b.data()[50] == 5.0f);
  }
  sb::end_test_case();

  sb::test_case("large rank-1 SIMD path - 1024 elements");
  {
    micron::mdarray<f32, 1> a(1024);
    a.fill(1.0f);
    micron::mdarray<f32, 1> b(1024);
    b.fill(2.0f);
    a += b;
    for ( usize i = 0; i < 1024; ++i ) sb::require(a.data()[i] == 3.0f);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
