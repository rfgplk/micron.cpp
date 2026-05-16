//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/array/soa.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

int
main(void)
{
  sb::print("=== SOA TESTS ===");

  sb::test_case("construction - empty");
  {
    micron::soa<i32, f32, u8> s;
    sb::require(s.empty());
    sb::require(s.size() == 0ULL);
    sb::require(s.capacity() == 0ULL);
    sb::require(micron::soa<i32, f32, u8>::column_count == 3ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - with capacity");
  {
    micron::soa<i32, f32> s(128);
    sb::require(s.capacity() == 128ULL);
    sb::require(s.empty());
  }
  sb::end_test_case();

  sb::test_case("emplace_back into all columns");
  {
    micron::soa<i32, f32> s;
    s.emplace_back(1, 1.5f);
    s.emplace_back(2, 2.5f);
    s.emplace_back(3, 3.5f);
    sb::require(s.size() == 3ULL);
    auto *c0 = s.column<0>();
    auto *c1 = s.column<1>();
    sb::require(c0[0] == 1);
    sb::require(c0[1] == 2);
    sb::require(c0[2] == 3);
    sb::require(c1[0] == 1.5f);
    sb::require(c1[1] == 2.5f);
    sb::require(c1[2] == 3.5f);
  }
  sb::end_test_case();

  sb::test_case("at returns referenced element");
  {
    micron::soa<i32, f32> s;
    for ( int i = 0; i < 5; ++i ) s.emplace_back(i, (float)i * 2.0f);
    sb::require(s.at<0>(2) == 2);
    sb::require(s.at<1>(3) == 6.0f);
    s.at<0>(2) = 999;
    sb::require(s.column<0>()[2] == 999);
  }
  sb::end_test_case();

  sb::test_case("growth - 1000 emplaces");
  {
    micron::soa<i32, f32, u8> s;
    for ( int i = 0; i < 1000; ++i ) s.emplace_back(i, (float)i, (u8)(i & 0xFF));
    sb::require(s.size() == 1000ULL);
    sb::require(s.capacity() >= 1000ULL);
    auto *c0 = s.column<0>();
    auto *c2 = s.column<2>();
    for ( int i = 0; i < 1000; ++i ) {
      sb::require(c0[i] == i);
      sb::require(c2[i] == (u8)(i & 0xFF));
    }
  }
  sb::end_test_case();

  sb::test_case("at - throws on out of range");
  {
    micron::soa<i32, f32> s;
    s.emplace_back(1, 1.0f);
    bool threw = false;
    try {
      s.at<0>(99);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("clear - resets size");
  {
    micron::soa<i32, f32> s;
    for ( int i = 0; i < 10; ++i ) s.emplace_back(i, 1.0f);
    sb::require(s.size() == 10ULL);
    s.clear();
    sb::require(s.empty());
  }
  sb::end_test_case();

  sb::test_case("move constructor");
  {
    micron::soa<i32, f32> a;
    for ( int i = 0; i < 5; ++i ) a.emplace_back(i, 1.0f);
    micron::soa<i32, f32> b(micron::move(a));
    sb::require(b.size() == 5ULL);
    sb::require(a.size() == 0ULL);
    sb::require(b.column<0>()[2] == 2);
  }
  sb::end_test_case();

  sb::test_case("SIMD-friendly: contiguous column");
  {
    micron::soa<f32, f32> s;
    for ( int i = 0; i < 100; ++i ) s.emplace_back((float)i, (float)i * 2.0f);
    auto *c0 = s.column<0>();
    f32 sum = 0;
    for ( int i = 0; i < 100; ++i ) sum += c0[i];
    sb::require(sum == 4950.0f);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
