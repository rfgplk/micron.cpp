//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/maps/conmap.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include "../support/mt.hpp"      // mtest::parallel + micron atomic_token (NOT <thread>/<atomic>)
#include <climits>
#include <cstdio>
#include <vector>

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
  sb::print("=== CONMAP TESTS ===");

  // ── single-threaded basics ───────────────────────────────────────────────

  sb::test_case("construction - default cap, empty");
  {
    micron::conmap<int, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0ULL);
    sb::require(m.stripe_count() == 64ULL);
    sb::require(m.capacity() >= 64ULL * 64ULL);
  }
  sb::end_test_case();

  sb::test_case("construction - explicit capacity per stripe");
  {
    micron::conmap<int, int> m(1024);
    sb::require(m.capacity() >= 1024ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - new key returns true");
  {
    micron::conmap<int, int> m;
    sb::require(m.insert(1, 10));
    sb::require(m.size() == 1ULL);
    sb::require(!m.insert(1, 99));      // duplicate
    sb::require(m.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("insert - keeps original value on duplicate");
  {
    micron::conmap<int, int> m;
    m.insert(1, 10);
    m.insert(1, 99);
    int v = 0;
    sb::require(m.find(1, v));
    sb::require(v == 10);
  }
  sb::end_test_case();

  sb::test_case("insert_or_assign - overwrites duplicate");
  {
    micron::conmap<int, int> m;
    sb::require(m.insert_or_assign(1, 10));
    sb::require(!m.insert_or_assign(1, 99));
    int v = 0;
    sb::require(m.find(1, v));
    sb::require(v == 99);
  }
  sb::end_test_case();

  sb::test_case("find - returns false on miss");
  {
    micron::conmap<int, int> m;
    m.insert(1, 10);
    int v = 0;
    sb::require(!m.find(2, v));
    sb::require(m.find(1, v));
    sb::require(v == 10);
  }
  sb::end_test_case();

  sb::test_case("contains and count");
  {
    micron::conmap<int, int> m;
    m.insert(1, 10);
    sb::require(m.contains(1));
    sb::require(!m.contains(2));
    sb::require(m.count(1) == 1ULL);
    sb::require(m.count(2) == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("erase - returns true on hit, removes");
  {
    micron::conmap<int, int> m;
    m.insert(1, 10);
    sb::require(m.erase(1));
    sb::require(!m.contains(1));
    sb::require(!m.erase(1));
  }
  sb::end_test_case();

  sb::test_case("clear - empties");
  {
    micron::conmap<int, int> m;
    for ( int i = 0; i < 100; ++i ) m.insert(i, i);
    sb::require(m.size() == 100ULL);
    m.clear();
    sb::require(m.empty());
  }
  sb::end_test_case();

  sb::test_case("update - applies fn under lock");
  {
    micron::conmap<int, int> m;
    m.insert(1, 10);
    bool ok = m.update(1, [](int &v) { v += 5; });
    sb::require(ok);
    int v = 0;
    m.find(1, v);
    sb::require(v == 15);
    sb::require(!m.update(99, [](int &) { /* no-op */ }));
  }
  sb::end_test_case();

  sb::test_case("upsert - inserts when missing");
  {
    micron::conmap<int, int> m;
    bool inserted = m.upsert(1, [](int &) { }, 42);
    sb::require(inserted);
    int v = 0;
    m.find(1, v);
    sb::require(v == 42);
    inserted = m.upsert(1, [](int &v) { v += 1; }, 0);
    sb::require(!inserted);
    m.find(1, v);
    sb::require(v == 43);
  }
  sb::end_test_case();

  sb::test_case("for_each - visits all stripes");
  {
    micron::conmap<int, int> m;
    for ( int i = 0; i < 200; ++i ) m.insert(i, i * 2);
    int sum = 0;
    int cnt = 0;
    m.for_each([&](const int &, const int &v) {
      sum += v;
      ++cnt;
    });
    sb::require(cnt == 200);
    int expected = 0;
    for ( int i = 0; i < 200; ++i ) expected += i * 2;
    sb::require(sum == expected);
  }
  sb::end_test_case();

  // ── string keys ──────────────────────────────────────────────────────────

  sb::test_case("string keys - insert/find/erase");
  {
    micron::conmap<micron::hstring<char>, int> m(1024);
    for ( int i = 0; i < 50; ++i ) m.insert(make_key(i), i);
    sb::require(m.size() == 50ULL);
    for ( int i = 0; i < 50; ++i ) {
      int v = -1;
      sb::require(m.find(make_key(i), v));
      sb::require(v == i);
    }
    for ( int i = 0; i < 25; ++i ) sb::require(m.erase(make_key(i)));
    sb::require(m.size() == 25ULL);
  }
  sb::end_test_case();

  // ── concurrent ───────────────────────────────────────────────────────────

  sb::test_case("concurrent inserts - 8 threads, distinct keys");
  {
    micron::conmap<int, int> m(65536);
    constexpr int T = 8;
    constexpr int P = 1000;
    mtest::parallel(T, [&m](int t) {
      for ( int i = 0; i < P; ++i ) m.insert(t * P + i, i);
    });
    sb::require(m.size() == static_cast<unsigned long>(T * P));
    for ( int t = 0; t < T; ++t ) {
      for ( int i = 0; i < P; ++i ) {
        int v = -1;
        sb::require(m.find(t * P + i, v));
        sb::require(v == i);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("concurrent insert+find - mixed workload");
  {
    micron::conmap<int, int> m(65536);
    micron::atomic_token<int> reader_hits{ 0 };
    // 2 writers (t=0,1) + 4 readers (t=2..5) in one pool
    mtest::parallel(6, [&m, &reader_hits](int t) {
      if ( t < 2 ) {
        for ( int i = 0; i < 2000; ++i ) m.insert_or_assign(t * 2000 + i, i);
      } else {
        for ( int i = 0; i < 1000; ++i ) {
          int v = 0;
          if ( m.find(i, v) ) reader_hits.fetch_add(1, micron::memory_order_relaxed);
        }
      }
    });
    sb::require(m.size() == 4000ULL);
  }
  sb::end_test_case();

  sb::test_case("concurrent update - increment counters");
  {
    micron::conmap<int, int> m(1024);
    for ( int i = 0; i < 64; ++i ) m.insert(i, 0);
    constexpr int T = 4;
    constexpr int P = 500;
    mtest::parallel(T, [&m](int) {
      for ( int i = 0; i < P; ++i ) {
        m.update(i % 64, [](int &v) { ++v; });
      }
    });
    int total = 0;
    m.for_each([&](const int &, const int &v) { total += v; });
    sb::require(total == T * P);
  }
  sb::end_test_case();

  // ── move semantics ──────────────────────────────────────────────────────

  sb::test_case("move constructor - transfers");
  {
    micron::conmap<int, int> a;
    a.insert(1, 10);
    a.insert(2, 20);
    micron::conmap<int, int> b = micron::move(a);
    sb::require(b.size() == 2ULL);
    int v = 0;
    sb::require(b.find(1, v));
    sb::require(v == 10);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
