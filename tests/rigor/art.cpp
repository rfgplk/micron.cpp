//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/trees/art.hpp"
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
  return micron::hstring<char>(static_cast<const char *>(buf));
}

int
main(void)
{
  sb::print("=== ART TESTS ===");

  sb::test_case("construction - empty");
  {
    micron::art<int, int> a;
    sb::require(a.empty());
    sb::require(a.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("insert single");
  {
    micron::art<int, int> a;
    sb::require(a.insert(1, 10));
    sb::require(a.size() == 1ULL);
    sb::require(*a.find(1) == 10);
  }
  sb::end_test_case();

  sb::test_case("insert duplicate updates value");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    a.insert(1, 99);      // duplicate - updates
    sb::require(a.size() == 1ULL);
    sb::require(*a.find(1) == 99);
  }
  sb::end_test_case();

  sb::test_case("find - miss returns nullptr");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    sb::require(a.find(99) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("contains");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    sb::require(a.contains(1));
    sb::require(!a.contains(2));
  }
  sb::end_test_case();

  sb::test_case("erase - returns true on hit");
  {
    micron::art<int, int> a;
    a.insert(1, 10);
    sb::require(a.erase(1));
    sb::require(!a.contains(1));
    sb::require(a.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - returns false on miss");
  {
    micron::art<int, int> a;
    sb::require(!a.erase(99));
  }
  sb::end_test_case();

  sb::test_case("at - throws on miss");
  {
    micron::art<int, int> a;
    bool threw = false;
    try {
      a.at(99);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("clear");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 100; ++i ) a.insert(i, i);
    a.clear();
    sb::require(a.empty());
  }
  sb::end_test_case();

  // ── bulk ─────────────────────────────────────────────────────────────────

  sb::test_case("bulk - 1000 inserts");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 1000; ++i ) a.insert(i, i * 2);
    sb::require(a.size() == 1000ULL);
    for ( int i = 0; i < 1000; ++i ) {
      int *v = a.find(i);
      sb::require(v != nullptr);
      sb::require(*v == i * 2);
    }
  }
  sb::end_test_case();

  sb::test_case("bulk - growth through node types");
  {
    micron::art<int, int> a;
    // 20 keys with the same low byte will fit in n4/n16; 1000 will force n48/n256.
    for ( int i = 0; i < 5000; ++i ) a.insert(i, i + 1);
    sb::require(a.size() == 5000ULL);
    for ( int i = 0; i < 5000; ++i ) sb::require(*a.find(i) == i + 1);
  }
  sb::end_test_case();

  // ── string keys ──────────────────────────────────────────────────────────

  sb::test_case("string keys - insert/find/erase");
  {
    micron::art<micron::hstring<char>, int> a;
    for ( int i = 0; i < 200; ++i ) a.insert(make_key(i), i);
    sb::require(a.size() == 200ULL);
    for ( int i = 0; i < 200; ++i ) {
      int *v = a.find(make_key(i));
      sb::require(v != nullptr);
      sb::require(*v == i);
    }
    for ( int i = 0; i < 100; ++i ) sb::require(a.erase(make_key(i)));
    sb::require(a.size() == 100ULL);
  }
  sb::end_test_case();

  // ── for_each ─────────────────────────────────────────────────────────────

  sb::test_case("for_each");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 100; ++i ) a.insert(i, i * 3);
    int sum = 0;
    int cnt = 0;
    a.for_each([&](const int &, const int &v) {
      sum += v;
      ++cnt;
    });
    sb::require(cnt == 100);
    int expected = 0;
    for ( int i = 0; i < 100; ++i ) expected += i * 3;
    sb::require(sum == expected);
  }
  sb::end_test_case();

  // ── move ─────────────────────────────────────────────────────────────────

  sb::test_case("move constructor");
  {
    micron::art<int, int> a;
    for ( int i = 0; i < 10; ++i ) a.insert(i, i);
    micron::art<int, int> b(micron::move(a));
    sb::require(b.size() == 10ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find(5) == 5);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
