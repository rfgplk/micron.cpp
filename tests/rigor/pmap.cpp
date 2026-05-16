//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/maps/pmap.hpp"
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
  sb::print("=== PMAP TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - default is empty");
  {
    micron::pmap<int, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("copy - shares structure (no crash, same contents)");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10).insert(2, 20);
    micron::pmap<int, int> c = b;
    sb::require(c.size() == 2ULL);
    sb::require(*c.find(1) == 10);
    sb::require(*c.find(2) == 20);
  }
  sb::end_test_case();

  sb::test_case("move - transfers");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10);
    micron::pmap<int, int> c = micron::move(b);
    sb::require(c.size() == 1ULL);
    sb::require(b.size() == 0ULL);
  }
  sb::end_test_case();

  // ── persistent insert ────────────────────────────────────────────────────

  sb::test_case("insert - returns new map; original unchanged");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10);
    sb::require(a.empty());
    sb::require(b.size() == 1ULL);
    sb::require(*b.find(1) == 10);
    sb::require(a.find(1) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("insert - chains");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10).insert(2, 20).insert(3, 30);
    sb::require(b.size() == 3ULL);
    sb::require(*b.find(1) == 10);
    sb::require(*b.find(2) == 20);
    sb::require(*b.find(3) == 30);
  }
  sb::end_test_case();

  sb::test_case("insert - replace returns same size");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10);
    auto c = b.insert(1, 99);
    sb::require(c.size() == 1ULL);
    sb::require(*c.find(1) == 99);
    // older version unchanged
    sb::require(*b.find(1) == 10);
  }
  sb::end_test_case();

  // ── persistent erase ─────────────────────────────────────────────────────

  sb::test_case("erase - returns new map without key");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10).insert(2, 20);
    auto c = b.erase(1);
    sb::require(c.size() == 1ULL);
    sb::require(!c.contains(1));
    sb::require(c.contains(2));
    // b unchanged
    sb::require(b.size() == 2ULL);
    sb::require(b.contains(1));
  }
  sb::end_test_case();

  sb::test_case("erase - missing key leaves same size");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10);
    auto c = b.erase(99);
    sb::require(c.size() == b.size());
  }
  sb::end_test_case();

  // ── find / contains / at ─────────────────────────────────────────────────

  sb::test_case("find - miss returns nullptr");
  {
    micron::pmap<int, int> a;
    auto b = a.insert(1, 10);
    sb::require(b.find(99) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("at - throws on missing");
  {
    micron::pmap<int, int> a;
    bool threw = false;
    try {
      a.at(99);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  // ── bulk ─────────────────────────────────────────────────────────────────

  sb::test_case("bulk - 1000 inserts roundtrip");
  {
    micron::pmap<int, int> m;
    for ( int i = 0; i < 1000; ++i ) m = m.insert(i, i * 2);
    sb::require(m.size() == 1000ULL);
    for ( int i = 0; i < 1000; ++i ) sb::require(*m.find(i) == i * 2);
  }
  sb::end_test_case();

  sb::test_case("bulk - 5000 inserts and 2500 erases");
  {
    micron::pmap<int, int> m;
    for ( int i = 0; i < 5000; ++i ) m = m.insert(i, i);
    sb::require(m.size() == 5000ULL);
    for ( int i = 0; i < 5000; i += 2 ) m = m.erase(i);
    sb::require(m.size() == 2500ULL);
    for ( int i = 0; i < 5000; ++i ) {
      bool exp = (i % 2 == 1);
      sb::require(m.contains(i) == exp);
    }
  }
  sb::end_test_case();

  // ── string keys ──────────────────────────────────────────────────────────

  sb::test_case("string keys - persistent insert and find");
  {
    micron::pmap<micron::hstring<char>, int> m;
    for ( int i = 0; i < 200; ++i ) m = m.insert(make_key(i), i);
    sb::require(m.size() == 200ULL);
    for ( int i = 0; i < 200; ++i ) {
      const int *p = m.find(make_key(i));
      sb::require(p != nullptr);
      sb::require(*p == i);
    }
  }
  sb::end_test_case();

  // ── structural sharing semantics ─────────────────────────────────────────

  sb::test_case("structural sharing - older versions remain valid");
  {
    micron::pmap<int, int> v0;
    auto v1 = v0.insert(1, 10);
    auto v2 = v1.insert(2, 20);
    auto v3 = v2.insert(3, 30);
    auto v4 = v3.erase(1);
    sb::require(v0.empty());
    sb::require(v1.size() == 1ULL && v1.contains(1));
    sb::require(v2.size() == 2ULL && v2.contains(1) && v2.contains(2));
    sb::require(v3.size() == 3ULL && v3.contains(1) && v3.contains(3));
    sb::require(v4.size() == 2ULL && !v4.contains(1) && v4.contains(3));
  }
  sb::end_test_case();

  // ── for_each ─────────────────────────────────────────────────────────────

  sb::test_case("for_each - touches every entry");
  {
    micron::pmap<int, int> m;
    for ( int i = 0; i < 100; ++i ) m = m.insert(i, i * 5);
    int sum = 0;
    int cnt = 0;
    m.for_each([&](const int &, const int &v) {
      sum += v;
      ++cnt;
    });
    sb::require(cnt == 100);
    int expected = 0;
    for ( int i = 0; i < 100; ++i ) expected += i * 5;
    sb::require(sum == expected);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
