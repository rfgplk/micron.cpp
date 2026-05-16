//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/sets/sets.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <cstdio>

static micron::hstring<char>
make_key(int i)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "key_%04d", i);
  return buf;
}

int
main(void)
{
  sb::print("=== SETS TESTS ===");

  sb::test_case("robin_set - basic insert/contains");
  {
    micron::robin_set<micron::hstring<char>> s(64);
    sb::require(s.empty());
    sb::require(s.insert("a"));
    sb::require(s.insert("b"));
    sb::require(!s.insert("a"));
    sb::require(s.size() == 2ULL);
    sb::require(s.contains("a"));
    sb::require(s.contains("b"));
    sb::require(!s.contains("c"));
  }
  sb::end_test_case();

  sb::test_case("robin_set - erase");
  {
    micron::robin_set<micron::hstring<char>> s(64);
    s.insert("a");
    s.insert("b");
    sb::require(s.erase("a"));
    sb::require(!s.erase("missing"));
    sb::require(!s.contains("a"));
    sb::require(s.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("robin_set - clear");
  {
    micron::robin_set<micron::hstring<char>> s(128);
    for ( int i = 0; i < 20; ++i ) s.insert(make_key(i));
    sb::require(s.size() == 20ULL);
    s.clear();
    sb::require(s.empty());
  }
  sb::end_test_case();

  sb::test_case("swiss_set - basic insert/contains");
  {
    micron::swiss_set<int, 64> s;
    sb::require(s.insert(1));
    sb::require(s.insert(2));
    sb::require(!s.insert(1));
    sb::require(s.contains(1));
    sb::require(!s.contains(99));
  }
  sb::end_test_case();

  sb::test_case("swiss_set - capacity is fixed at template parameter");
  {
    micron::swiss_set<int, 32> s;
    sb::require(s.capacity() == 32ULL);
  }
  sb::end_test_case();

  sb::test_case("heap_swiss_set - growable");
  {
    micron::heap_swiss_set<int> s(16);
    constexpr int N = 500;
    for ( int i = 0; i < N; ++i ) sb::require(s.insert(i));
    sb::require(s.size() == static_cast<unsigned long>(N));
    sb::require(s.capacity() >= static_cast<unsigned long>(N));
    for ( int i = 0; i < N; ++i ) sb::require(s.contains(i));
  }
  sb::end_test_case();

  sb::test_case("heap_swiss_set - duplicates rejected");
  {
    micron::heap_swiss_set<int> s;
    s.insert(42);
    sb::require(!s.insert(42));
    sb::require(s.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("hopscotch_set - insert/contains/erase");
  {
    micron::hopscotch_set<u64> s;
    sb::require(s.insert(1));
    sb::require(s.insert(2));
    sb::require(!s.insert(1));
    sb::require(s.contains(1));
    sb::require(s.erase(1));
    sb::require(!s.contains(1));
  }
  sb::end_test_case();

  sb::test_case("immutable_set - insert returns new set");
  {
    micron::immutable_set<int> a;
    auto b = a.insert(1);
    auto c = b.insert(2);
    sb::require(a.empty());
    sb::require(b.size() == 1ULL);
    sb::require(c.size() == 2ULL);
    sb::require(c.contains(1));
    sb::require(c.contains(2));
    sb::require(!a.contains(1));
  }
  sb::end_test_case();

  sb::test_case("immutable_set - erase returns new set");
  {
    micron::immutable_set<int> a;
    auto b = a.insert(1).insert(2).insert(3);
    auto c = b.erase(2);
    sb::require(b.size() == 3ULL);
    sb::require(c.size() == 2ULL);
    sb::require(b.contains(2));
    sb::require(!c.contains(2));
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
