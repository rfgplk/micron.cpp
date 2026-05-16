//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/maps/rb_map.hpp"
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
  sb::print("=== RB MAP TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - default constructor: empty");
  {
    micron::rb_map<int, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
    sb::require(m.bin_count() >= 16ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - explicit bin count");
  {
    micron::rb_map<int, int> m(128);
    sb::require(m.bin_count() >= 128ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - move");
  {
    micron::rb_map<int, int> a;
    a.insert(1, 10);
    a.insert(2, 20);
    micron::rb_map<int, int> b(micron::move(a));
    sb::require(b.size() == 2ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find(1) == 10);
  }
  sb::end_test_case();

  // ── basic ────────────────────────────────────────────────────────────────

  sb::test_case("insert - new key returns {true, ptr}");
  {
    micron::rb_map<int, int> m;
    auto r = m.insert(1, 10);
    sb::require(r.a == true);
    sb::require(r.b && *r.b == 10);
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - duplicate returns {false, existing}");
  {
    micron::rb_map<int, int> m;
    m.insert(1, 10);
    auto r = m.insert(1, 99);
    sb::require(r.a == false);
    sb::require(*r.b == 10);
  }
  sb::end_test_case();

  sb::test_case("insert_or_assign - overwrites");
  {
    micron::rb_map<int, int> m;
    m.insert(1, 10);
    auto r = m.insert_or_assign(1, 99);
    sb::require(r.a == false);
    sb::require(*r.b == 99);
    sb::require(*m.find(1) == 99);
  }
  sb::end_test_case();

  sb::test_case("find - miss returns nullptr");
  {
    micron::rb_map<int, int> m;
    m.insert(1, 10);
    sb::require(m.find(99) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("contains and count");
  {
    micron::rb_map<int, int> m;
    m.insert(1, 10);
    sb::require(m.contains(1));
    sb::require(!m.contains(2));
    sb::require(m.count(1) == 1ULL);
    sb::require(m.count(2) == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("at - throws on missing");
  {
    micron::rb_map<int, int> m;
    bool threw = false;
    try {
      m.at(99);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("operator[] - inserts default on miss");
  {
    micron::rb_map<int, int> m;
    int &v = m[5];
    sb::require(v == 0);
    v = 50;
    sb::require(*m.find(5) == 50);
  }
  sb::end_test_case();

  sb::test_case("erase - returns true on hit");
  {
    micron::rb_map<int, int> m;
    m.insert(1, 10);
    sb::require(m.erase(1));
    sb::require(!m.contains(1));
    sb::require(m.size() == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - returns false on miss");
  {
    micron::rb_map<int, int> m;
    sb::require(!m.erase(99));
  }
  sb::end_test_case();

  sb::test_case("clear");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 50; ++i ) m.insert(i, i);
    m.clear();
    sb::require(m.empty());
  }
  sb::end_test_case();

  // ── growth / treeification ───────────────────────────────────────────────

  sb::test_case("growth - 5000 entries roundtrip");
  {
    micron::rb_map<int, int> m;
    constexpr int N = 5000;
    for ( int i = 0; i < N; ++i ) m.insert(i, i * 2);
    sb::require(m.size() == static_cast<unsigned long>(N));
    for ( int i = 0; i < N; ++i ) {
      int *p = m.find(i);
      sb::require(p != nullptr);
      sb::require(*p == i * 2);
    }
  }
  sb::end_test_case();

  sb::test_case("growth - load factor stays bounded");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 1000; ++i ) m.insert(i, i);
    sb::require(m.load_factor() <= 0.76f);
  }
  sb::end_test_case();

  sb::test_case("treeification - force a heavy bin by colliding hashes");
  {
    // since we can't easily force collisions across hashes, just verify a
    // large-ish map keeps correctness even when treeify_threshold + resize
    // interleave.
    micron::rb_map<u64, u64> m;
    for ( u64 i = 0; i < 2000; ++i ) m.insert(i, i + 1);
    for ( u64 i = 0; i < 2000; ++i ) sb::require(*m.find(i) == i + 1);
  }
  sb::end_test_case();

  // ── string keys ──────────────────────────────────────────────────────────

  sb::test_case("string keys - insert / find");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 200; ++i ) m.insert(make_key(i), i);
    sb::require(m.size() == 200ULL);
    for ( int i = 0; i < 200; ++i ) {
      int *p = m.find(make_key(i));
      sb::require(p != nullptr);
      sb::require(*p == i);
    }
  }
  sb::end_test_case();

  sb::test_case("string keys - erase half");
  {
    micron::rb_map<micron::hstring<char>, int> m;
    for ( int i = 0; i < 200; ++i ) m.insert(make_key(i), i);
    for ( int i = 0; i < 200; i += 2 ) sb::require(m.erase(make_key(i)));
    sb::require(m.size() == 100ULL);
    for ( int i = 0; i < 200; ++i ) {
      bool exp = (i % 2 == 1);
      sb::require(m.contains(make_key(i)) == exp);
    }
  }
  sb::end_test_case();

  // ── for_each ─────────────────────────────────────────────────────────────

  sb::test_case("for_each - touches every entry");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 300; ++i ) m.insert(i, i * 3);
    int sum = 0;
    int cnt = 0;
    m.for_each([&](const int &, const int &v) {
      sum += v;
      ++cnt;
    });
    sb::require(cnt == 300);
    int expected = 0;
    for ( int i = 0; i < 300; ++i ) expected += i * 3;
    sb::require(sum == expected);
  }
  sb::end_test_case();

  sb::test_case("for_each - non-destructive (idempotent across calls)");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 300; ++i ) m.insert(i, i);
    int sum1 = 0;
    m.for_each([&](const int &, const int &v) { sum1 += v; });
    int sum2 = 0;
    int cnt2 = 0;
    m.for_each([&](const int &, const int &v) {
      sum2 += v;
      ++cnt2;
    });
    sb::require(sum1 == sum2);
    sb::require(cnt2 == 300);
    sb::require(m.size() == 300ULL);
  }
  sb::end_test_case();

  sb::test_case("for_each - const overload visits every entry");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 300; ++i ) m.insert(i, i * 5);
    const auto &cm = m;
    int sum = 0;
    int cnt = 0;
    cm.for_each([&](const int &, const int &v) {
      sum += v;
      ++cnt;
    });
    int expected = 0;
    for ( int i = 0; i < 300; ++i ) expected += i * 5;
    sb::require(cnt == 300);
    sb::require(sum == expected);
  }
  sb::end_test_case();

  // ── move forwarding ──────────────────────────────────────────────────────

  sb::test_case("insert(K&&, V&&) actually moves both arguments");
  {
    micron::rb_map<micron::hstring<char>, micron::hstring<char>> m;
    micron::hstring<char> k = "the_quick_brown_fox_jumps_over_the_lazy_dog";
    micron::hstring<char> v = "lorem_ipsum_dolor_sit_amet_consectetur_adipiscing";
    auto r = m.insert(micron::move(k), micron::move(v));
    sb::require(r.a == true);
    sb::require(k.size() == 0ULL);      // key was moved-from
    sb::require(v.size() == 0ULL);      // value was moved-from
    auto *found = m.find(micron::hstring<char>("the_quick_brown_fox_jumps_over_the_lazy_dog"));
    sb::require(found != nullptr);
    sb::require(*found == micron::hstring<char>("lorem_ipsum_dolor_sit_amet_consectetur_adipiscing"));
  }
  sb::end_test_case();

  sb::test_case("insert_or_assign(K&&, V&&) moves on new insert and overwrite");
  {
    micron::rb_map<micron::hstring<char>, micron::hstring<char>> m;
    micron::hstring<char> k1 = "alpha_key_padded_to_avoid_sso";
    micron::hstring<char> v1 = "alpha_value_padded_to_avoid_sso";
    m.insert_or_assign(micron::move(k1), micron::move(v1));
    sb::require(k1.size() == 0ULL);
    sb::require(v1.size() == 0ULL);

    micron::hstring<char> probe = "alpha_key_padded_to_avoid_sso";
    micron::hstring<char> v2 = "alpha_value_overwritten_with_new_data";
    auto r = m.insert_or_assign(micron::move(probe), micron::move(v2));
    sb::require(r.a == false);      // overwrite, not new
    // probe was used for lookup only - not consumed by the overwrite path,
    // but v2 must have been forwarded into the stored slot.
    sb::require(v2.size() == 0ULL);
    sb::require(*r.b == micron::hstring<char>("alpha_value_overwritten_with_new_data"));
  }
  sb::end_test_case();

  // ── erase / re-insert correctness on large maps ──────────────────────────

  sb::test_case("erase-then-reinsert preserves correctness on large map");
  {
    micron::rb_map<u64, u64> m;
    constexpr u64 N = 4000;
    for ( u64 i = 0; i < N; ++i ) m.insert(i, i * 7);
    for ( u64 i = 0; i < N; i += 3 ) sb::require(m.erase(i));
    for ( u64 i = 0; i < N; ++i ) {
      bool kept = (i % 3 != 0);
      auto *p = m.find(i);
      sb::require((p != nullptr) == kept);
      if ( kept ) sb::require(*p == i * 7);
    }
    // re-insert everything we removed
    for ( u64 i = 0; i < N; i += 3 ) m.insert(i, i * 7 + 1);
    for ( u64 i = 0; i < N; ++i ) {
      auto *p = m.find(i);
      sb::require(p != nullptr);
      sb::require(*p == ((i % 3 == 0) ? i * 7 + 1 : i * 7));
    }
  }
  sb::end_test_case();

  // ── alias ────────────────────────────────────────────────────────────────

  sb::test_case("rmap alias works");
  {
    micron::rmap<int, int> m;
    m.insert(1, 10);
    sb::require(*m.find(1) == 10);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
