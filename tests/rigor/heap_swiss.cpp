//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/maps/heap_swiss.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <cstdio>
#include <cstring>

// ─── helpers ──────────────────────────────────────────────────────────────

static micron::hstring<char>
make_key(int i)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "key_%04d", i);
  return buf;
}

// ─── main ─────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== HEAP SWISS MAP TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - default constructor: empty map, min capacity");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
    sb::require(m.capacity() >= 16ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - explicit capacity rounded up to pow2");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m(33);
    sb::require(m.empty());
    sb::require(m.capacity() == 64ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - explicit capacity <= 16 yields min");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m(1);
    sb::require(m.capacity() == 16ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - copy constructor duplicates contents");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> a(64);
    a.insert("alpha", 1);
    a.insert("beta", 2);
    micron::heap_swiss_map<micron::hstring<char>, int> b(a);
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 2ULL);
    sb::require(*b.find("alpha") == 1);
    sb::require(*b.find("beta") == 2);
  }
  sb::end_test_case();

  sb::test_case("construction - copy constructor: maps are independent");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> a;
    a.insert("shared", 100);
    micron::heap_swiss_map<micron::hstring<char>, int> b(a);
    b.erase("shared");
    sb::require(a.contains("shared"));
    sb::require(!b.contains("shared"));
  }
  sb::end_test_case();

  sb::test_case("construction - move constructor transfers contents");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> a;
    a.insert("x", 10);
    a.insert("y", 20);
    micron::heap_swiss_map<micron::hstring<char>, int> b(micron::move(a));
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find("x") == 10);
    sb::require(*b.find("y") == 20);
  }
  sb::end_test_case();

  // ── basic operations ─────────────────────────────────────────────────────

  sb::test_case("insert - new key returns {true, ptr}");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    auto r = m.insert("k", 7);
    sb::require(r.a == true);
    sb::require(r.b != nullptr);
    sb::require(*r.b == 7);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate key returns {false, ptr} and keeps value");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    m.insert("k", 7);
    auto r = m.insert("k", 999);
    sb::require(r.a == false);
    sb::require(*r.b == 7);      // original value preserved
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("insert_or_assign - new key inserts");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    auto r = m.insert_or_assign(micron::hstring<char>("k"), 1);
    sb::require(r.a == true);
    sb::require(*r.b == 1);
  }
  sb::end_test_case();

  sb::test_case("insert_or_assign - existing key overwrites");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    m.insert("k", 1);
    auto r = m.insert_or_assign(micron::hstring<char>("k"), 99);
    sb::require(r.a == false);
    sb::require(*r.b == 99);
    sb::require(*m.find("k") == 99);
  }
  sb::end_test_case();

  sb::test_case("find - returns nullptr on miss");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    m.insert("k", 1);
    sb::require(m.find("missing") == nullptr);
  }
  sb::end_test_case();

  sb::test_case("find - returns ptr to value on hit");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    m.insert("k", 42);
    int *p = m.find("k");
    sb::require(p != nullptr);
    sb::require(*p == 42);
  }
  sb::end_test_case();

  sb::test_case("contains and count");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    m.insert("a", 1);
    sb::require(m.contains("a"));
    sb::require(!m.contains("b"));
    sb::require(m.count("a") == 1ULL);
    sb::require(m.count("b") == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("at - throws on missing key");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    bool threw = false;
    try {
      m.at("missing");
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("operator[] - inserts default on miss");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    int &v = m["new"];
    sb::require(v == 0);
    sb::require(m.contains("new"));
    v = 5;
    sb::require(*m.find("new") == 5);
  }
  sb::end_test_case();

  sb::test_case("erase - returns true and removes");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    m.insert("a", 1);
    sb::require(m.erase("a"));
    sb::require(!m.contains("a"));
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - returns false on missing");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    sb::require(!m.erase("missing"));
  }
  sb::end_test_case();

  sb::test_case("clear - empties the map");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 10; ++i ) m.insert(make_key(i), i);
    m.clear();
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
    for ( int i = 0; i < 10; ++i ) sb::require(!m.contains(make_key(i)));
  }
  sb::end_test_case();

  // ── growth/resize ────────────────────────────────────────────────────────

  sb::test_case("growth - forces multiple rehashes and preserves data");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m(16);
    sb::require(m.capacity() == 16ULL);
    constexpr int N = 1000;
    for ( int i = 0; i < N; ++i ) m.insert(make_key(i), i);
    sb::require(m.size() == static_cast<unsigned long>(N));
    sb::require(m.capacity() >= static_cast<unsigned long>(N));
    for ( int i = 0; i < N; ++i ) {
      int *p = m.find(make_key(i));
      sb::require(p != nullptr);
      sb::require(*p == i);
    }
  }
  sb::end_test_case();

  sb::test_case("reserve - never shrinks, can grow on demand");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m(16);
    m.reserve(256);
    sb::require(m.capacity() >= 256ULL);
    usize before = m.capacity();
    m.reserve(8);      // ignored
    sb::require(m.capacity() == before);
  }
  sb::end_test_case();

  // ── load factor & growth_left correctness ────────────────────────────────

  sb::test_case("load_factor - stays below 7/8");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m(16);
    for ( int i = 0; i < 32; ++i ) m.insert(make_key(i), i);
    sb::require(m.load_factor() <= 7.0f / 8.0f + 0.001f);
  }
  sb::end_test_case();

  // ── erase then reinsert ─────────────────────────────────────────────────

  sb::test_case("erase + reinsert - same key after erase");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    m.insert("k", 1);
    m.erase("k");
    auto r = m.insert("k", 99);
    sb::require(r.a == true);
    sb::require(*r.b == 99);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("erase pattern - alternating insert/erase keeps size correct");
  {
    micron::heap_swiss_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 100; ++i ) {
      m.insert(make_key(i), i);
      if ( i % 2 == 0 ) m.erase(make_key(i));
    }
    sb::require(m.size() == 50ULL);
    for ( int i = 0; i < 100; ++i ) {
      bool exp = (i % 2 == 1);
      sb::require(m.contains(make_key(i)) == exp);
    }
  }
  sb::end_test_case();

  // ── trivial K/V ─────────────────────────────────────────────────────────

  sb::test_case("u64 keys, u64 values");
  {
    micron::heap_swiss_map<u64, u64> m;
    for ( u64 i = 0; i < 256; ++i ) m.insert(i, i * 2);
    sb::require(m.size() == 256ULL);
    for ( u64 i = 0; i < 256; ++i ) {
      u64 *p = m.find(i);
      sb::require(p != nullptr);
      sb::require(*p == i * 2);
    }
  }
  sb::end_test_case();

  // ── iteration ────────────────────────────────────────────────────────────

  sb::test_case("iteration - visits every occupied slot exactly once");
  {
    micron::heap_swiss_map<int, int> m;
    for ( int i = 0; i < 64; ++i ) m.insert(i, i);
    usize seen = 0;
    for ( auto it = m.begin(); it != m.end(); ++it ) ++seen;
    sb::require(seen == 64ULL);
  }
  sb::end_test_case();

  // ── assignment ───────────────────────────────────────────────────────────

  sb::test_case("copy assignment - independent copies");
  {
    micron::heap_swiss_map<int, int> a;
    a.insert(1, 10);
    a.insert(2, 20);
    micron::heap_swiss_map<int, int> b;
    b.insert(99, 99);
    b = a;
    sb::require(b.size() == 2ULL);
    sb::require(*b.find(1) == 10);
    sb::require(!b.contains(99));
    a.insert(3, 30);
    sb::require(!b.contains(3));
  }
  sb::end_test_case();

  sb::test_case("move assignment - transfers ownership");
  {
    micron::heap_swiss_map<int, int> a;
    for ( int i = 0; i < 10; ++i ) a.insert(i, i);
    micron::heap_swiss_map<int, int> b;
    b = micron::move(a);
    sb::require(b.size() == 10ULL);
    sb::require(a.size() == 0ULL);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
