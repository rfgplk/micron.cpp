//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// robin_exhaustive.cpp — exhaustive functional + stress coverage for robin_map.
// Tests every public method with heavy-load workloads, lifetime tracking,
// ground-truth cross-checks against std::unordered_map, edge cases, and
// adversarial hash distributions.

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/robin.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>

static micron::hstring<char>
make_skey(int i)
{
  char buf[40];
  std::snprintf(buf, sizeof(buf), "k_%010d", i);
  return buf;
}

struct counted {
  static inline int live = 0;
  static inline int total_ctor = 0;
  static inline int total_dtor = 0;
  int v = 0;

  counted() noexcept
  {
    ++live;
    ++total_ctor;
  }

  counted(int x) noexcept : v(x)
  {
    ++live;
    ++total_ctor;
  }

  counted(const counted &o) noexcept : v(o.v)
  {
    ++live;
    ++total_ctor;
  }

  counted(counted &&o) noexcept : v(o.v)
  {
    ++live;
    ++total_ctor;
  }

  ~counted() noexcept
  {
    --live;
    ++total_dtor;
  }

  counted &
  operator=(const counted &o) noexcept
  {
    v = o.v;
    return *this;
  }

  counted &
  operator=(counted &&o) noexcept
  {
    v = o.v;
    return *this;
  }

  bool
  operator==(const counted &o) const noexcept
  {
    return v == o.v;
  }

  static void
  reset() noexcept
  {
    live = 0;
    total_ctor = 0;
    total_dtor = 0;
  }
};

static void
__diag_terminate()
{
  micron::io::print("DIAG: terminate during test_case = [", sb::__global_test_case, "]\n\r");
  __builtin_trap();
}

int
main(void)
{
  std::set_terminate(__diag_terminate);
  sb::print("=== ROBIN MAP EXHAUSTIVE TESTS ===");

  sb::test_case("ctor - default produces empty map with min capacity");
  {
    micron::robin_map<int, int> m;
    sb::require(m.empty());
    sb::require(m.size() == 0u);
    sb::require(m.max_size() >= 16u);
    sb::require(m.load_factor() == 0.0f);
  }
  sb::end_test_case();

  sb::test_case("ctor - explicit capacity rounds to power-of-two");
  {
    micron::robin_map<int, int> m(1000);
    sb::require(m.max_size() >= 1000u);
    usize ms = m.max_size();
    sb::require((ms & (ms - 1u)) == 0u);
  }
  sb::end_test_case();

  sb::test_case("ctor - small capacity is bumped to __min_cap = 16");
  {
    micron::robin_map<int, int> m(1);
    sb::require(m.max_size() >= 16u);
  }
  sb::end_test_case();

  sb::test_case("ctor - move ctor transfers contents, leaves source empty");
  {
    micron::robin_map<int, int> a(64);
    for ( int i = 0; i < 30; ++i ) a.insert(i, i * 2);
    usize a_sz = a.size();

    micron::robin_map<int, int> b(micron::move(a));
    sb::require(b.size() == a_sz);
    sb::require(a.size() == 0u);
    sb::require(a.empty());
    for ( int i = 0; i < 30; ++i ) sb::require(*b.find(i) == i * 2);
  }
  sb::end_test_case();

  sb::test_case("move-assign - replaces destination, source empty");
  {
    micron::robin_map<int, int> a(64);
    for ( int i = 0; i < 20; ++i ) a.insert(i, i + 100);

    micron::robin_map<int, int> b(32);
    for ( int i = 0; i < 10; ++i ) b.insert(i + 1000, i);

    b = micron::move(a);
    sb::require(b.size() == 20u);
    sb::require(a.size() == 0u);
    for ( int i = 0; i < 20; ++i ) sb::require(*b.find(i) == i + 100);
    for ( int i = 0; i < 10; ++i ) sb::require(b.find(i + 1000) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("move-assign - self-assign is a no-op");
  {
    micron::robin_map<int, int> m(32);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i);
    auto *self = &m;
    m = micron::move(*self);
    sb::require(m.size() == 10u);
    for ( int i = 0; i < 10; ++i ) sb::require(*m.find(i) == i);
  }
  sb::end_test_case();

  sb::test_case("lifetime - inserts then ~map balances ctor/dtor");
  {
    counted::reset();
    {
      micron::robin_map<int, counted> m(64);
      for ( int i = 0; i < 30; ++i ) m.insert(i, counted(i * 7));
      sb::require(counted::live == 30);
      sb::require(m.size() == 30u);
    }
    sb::require(counted::live == 0);
  }
  sb::end_test_case();

  sb::test_case("lifetime - clear destroys all values");
  {
    counted::reset();
    micron::robin_map<int, counted> m(64);
    for ( int i = 0; i < 25; ++i ) m.insert(i, counted(i));
    sb::require(counted::live == 25);
    m.clear();
    sb::require(counted::live == 0);
    sb::require(m.empty());
    sb::require(m.size() == 0u);
  }
  sb::end_test_case();

  sb::test_case("lifetime - erase drops live count by 1");
  {
    counted::reset();
    micron::robin_map<int, counted> m(64);
    for ( int i = 0; i < 20; ++i ) m.insert(i, counted(i));
    for ( int i = 0; i < 10; ++i ) {
      bool e = m.erase(i);
      sb::require(e == true);
    }
    sb::require(counted::live == 10);
    sb::require(m.size() == 10u);
    for ( int i = 10; i < 20; ++i ) sb::require(m.find(i) != nullptr);
  }
  sb::end_test_case();

  sb::test_case("lifetime - move ctor does not duplicate live count");
  {
    counted::reset();
    {
      micron::robin_map<int, counted> a(32);
      for ( int i = 0; i < 15; ++i ) a.insert(i, counted(i));
      sb::require(counted::live == 15);

      micron::robin_map<int, counted> b(micron::move(a));
      sb::require(counted::live == 15);
      sb::require(a.empty());
    }
    sb::require(counted::live == 0);
  }
  sb::end_test_case();

  sb::test_case("lifetime - update of existing key leaves live count unchanged");
  {
    counted::reset();
    micron::robin_map<int, counted> m(64);
    m.insert(1, counted(10));
    sb::require(counted::live == 1);
    m.insert(1, counted(20));
    sb::require(counted::live == 1);
    sb::require(m.find(1)->v == 20);
  }
  sb::end_test_case();

  sb::test_case("size - tracks every insert/erase exactly");
  {
    micron::robin_map<int, int> m(1024);
    sb::require(m.size() == 0u);
    for ( int i = 0; i < 500; ++i ) {
      m.insert(i, i);
      sb::require(m.size() == (usize)(i + 1));
    }
    for ( int i = 0; i < 250; ++i ) {
      m.erase(i);
      sb::require(m.size() == (usize)(500 - i - 1));
    }
  }
  sb::end_test_case();

  sb::test_case("load_factor - monotonic under insert, capped at 7/8");
  {
    micron::robin_map<int, int> m(1024);
    float prev = -1.0f;
    for ( int i = 0; i < 700; ++i ) {
      m.insert(i, i);
      float lf = m.load_factor();
      sb::require(lf >= prev);
      prev = lf;
    }
    sb::require(prev <= 7.0f / 8.0f);
  }
  sb::end_test_case();

  sb::test_case("empty - flips with insert/clear");
  {
    micron::robin_map<int, int> m(64);
    sb::require(m.empty());
    m.insert(1, 1);
    sb::require(!m.empty());
    m.clear();
    sb::require(m.empty());
  }
  sb::end_test_case();

  sb::test_case("insert - rvalue value overload");
  {
    micron::robin_map<int, micron::hstring<char>> m(64);
    micron::hstring<char> v("hello");
    auto *r = m.insert(1, micron::move(v));
    sb::require(r != nullptr);
    sb::require(*m.find(1) == "hello");
  }
  sb::end_test_case();

  sb::test_case("insert - rvalue key overload");
  {
    micron::robin_map<micron::hstring<char>, int> m(64);
    micron::hstring<char> k("key");
    m.insert(micron::move(k), 42);
    sb::require(*m.find("key") == 42);
  }
  sb::end_test_case();

  sb::test_case("insert - const lvalue value overload (copy path)");
  {
    micron::robin_map<int, micron::hstring<char>> m(64);
    micron::hstring<char> v("hello");
    const micron::hstring<char> &cv = v;
    m.insert(1, cv);
    sb::require(*m.find(1) == "hello");
    sb::require(v == "hello");
  }
  sb::end_test_case();

  sb::test_case("insert - update existing key replaces value, keeps size");
  {
    micron::robin_map<int, int> m(64);
    m.insert(7, 100);
    m.insert(7, 200);
    sb::require(m.size() == 1u);
    sb::require(*m.find(7) == 200);
  }
  sb::end_test_case();

  sb::test_case("emplace - in-place construction with string");
  {
    micron::robin_map<int, micron::hstring<char>> m(64);
    m.emplace(1, "hello");
    sb::require(*m.find(1) == "hello");
  }
  sb::end_test_case();

  sb::test_case("emplace - default-constructed value");
  {
    micron::robin_map<int, int> m(64);
    m.emplace(5);
    sb::require(m.find(5) != nullptr);
    sb::require(*m.find(5) == 0);
  }
  sb::end_test_case();

  sb::test_case("add - returns mutable reference");
  {
    micron::robin_map<int, int> m(64);
    auto &r = m.add(1, 100);
    sb::require(r == 100);
    r = 200;
    sb::require(*m.find(1) == 200);
  }
  sb::end_test_case();

  sb::test_case("find - nullptr for missing key on empty + populated map");
  {
    micron::robin_map<int, int> m(64);
    sb::require(m.find(42) == nullptr);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i);
    sb::require(m.find(999) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("find - returns correct value for every present key");
  {
    micron::robin_map<int, int> m(128);
    for ( int i = 0; i < 80; ++i ) m.insert(i, i * 3);
    for ( int i = 0; i < 80; ++i ) {
      auto *v = m.find(i);
      sb::require(v != nullptr);
      sb::require(*v == i * 3);
    }
  }
  sb::end_test_case();

  sb::test_case("find - const overload on const map");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i + 50);
    const auto &cm = m;
    for ( int i = 0; i < 10; ++i ) sb::require(*cm.find(i) == i + 50);
  }
  sb::end_test_case();

  sb::test_case("find_hash - direct hash lookup matches find");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 50; ++i ) m.insert(i, i + 1000);
    for ( int i = 0; i < 50; ++i ) {
      auto h = micron::hash<micron::hash64_t>(i);
      sb::require(*m.find_hash(h, i) == i + 1000);
    }
  }
  sb::end_test_case();

  sb::test_case("find_hash - const variant");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 30; ++i ) m.insert(i, i);
    const auto &cm = m;
    auto h = micron::hash<micron::hash64_t>(7);
    sb::require(*cm.find_hash(h, 7) == 7);
  }
  sb::end_test_case();

  sb::test_case("contains - matches find semantics");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 30; ++i ) m.insert(i, i);
    for ( int i = 0; i < 30; ++i ) sb::require(m.contains(i));
    for ( int i = 30; i < 60; ++i ) sb::require(!m.contains(i));
  }
  sb::end_test_case();

  sb::test_case("exists / count - 1 or 0 only");
  {
    micron::robin_map<int, int> m(64);
    m.insert(7, 7);
    sb::require(m.exists(7) == 1u);
    sb::require(m.count(7) == 1u);
    sb::require(m.exists(8) == 0u);
    sb::require(m.count(8) == 0u);
  }
  sb::end_test_case();

  sb::test_case("at - returns reference, throws on missing");
  {
    micron::robin_map<int, int> m(64);
    m.insert(1, 100);
    sb::require(m.at(1) == 100);
    bool threw = false;
    try {
      (void)m.at(99);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw == true);
  }
  sb::end_test_case();

  sb::test_case("at - const variant on const map");
  {
    micron::robin_map<int, int> m(64);
    m.insert(1, 100);
    const auto &cm = m;
    sb::require(cm.at(1) == 100);
    bool threw = false;
    try {
      (void)cm.at(99);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw == true);
  }
  sb::end_test_case();

  sb::test_case("at - allows mutation via reference");
  {
    micron::robin_map<int, int> m(64);
    m.insert(1, 100);
    m.at(1) = 999;
    sb::require(*m.find(1) == 999);
  }
  sb::end_test_case();

  sb::test_case("operator[] - default-constructs missing key");
  {
    micron::robin_map<int, int> m(64);
    int &r = m[42];
    sb::require(r == 0);
    sb::require(m.size() == 1u);
  }
  sb::end_test_case();

  sb::test_case("operator[] - persistent reference for repeated access");
  {
    micron::robin_map<int, int> m(64);
    m[7] = 100;
    sb::require(m[7] == 100);
    m[7] = 200;
    sb::require(m[7] == 200);
    sb::require(m.size() == 1u);
  }
  sb::end_test_case();

  sb::test_case("operator[] - 100-iter accumulator over 10 keys");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 100; ++i ) m[i % 10] += 1;
    for ( int i = 0; i < 10; ++i ) sb::require(m[i] == 10);
    sb::require(m.size() == 10u);
  }
  sb::end_test_case();

  sb::test_case("erase - returns false for missing on empty map");
  {
    micron::robin_map<int, int> m(64);
    sb::require(m.erase(42) == false);
    sb::require(m.empty());
  }
  sb::end_test_case();

  sb::test_case("erase - returns true and removes single key");
  {
    micron::robin_map<int, int> m(64);
    m.insert(1, 100);
    sb::require(m.erase(1) == true);
    sb::require(m.empty());
    sb::require(m.find(1) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("erase - re-insert after erase works");
  {
    micron::robin_map<int, int> m(64);
    m.insert(1, 100);
    m.erase(1);
    m.insert(1, 200);
    sb::require(*m.find(1) == 200);
  }
  sb::end_test_case();

  sb::test_case("erase_hash - direct-hash overload matches erase");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 30; ++i ) m.insert(i, i * 2);
    for ( int i = 0; i < 30; ++i ) {
      auto h = micron::hash<micron::hash64_t>(i);
      sb::require(m.erase_hash(h, i) == true);
    }
    sb::require(m.empty());
  }
  sb::end_test_case();

  sb::test_case("erase - chain integrity via backward shift (stride 2)");
  {
    micron::robin_map<int, int> m(128);
    const int N = 60;
    for ( int i = 0; i < N; ++i ) m.insert(i, i);
    for ( int i = 0; i < N; i += 2 ) m.erase(i);
    sb::require(m.size() == (usize)(N / 2));
    for ( int i = 1; i < N; i += 2 ) sb::require(m.find(i) != nullptr && *m.find(i) == i);
    for ( int i = 0; i < N; i += 2 ) sb::require(m.find(i) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("erase - chain integrity via backward shift (stride 3 + 5)");
  {
    micron::robin_map<int, int> m(256);
    const int N = 120;
    for ( int i = 0; i < N; ++i ) m.insert(i, i * 4);
    for ( int i = 0; i < N; i += 3 ) m.erase(i);
    for ( int i = 0; i < N; i += 5 ) m.erase(i);

    for ( int i = 0; i < N; ++i ) {
      bool should_exist = (i % 3 != 0) && (i % 5 != 0);
      auto *v = m.find(i);
      if ( should_exist ) {
        sb::require(v != nullptr);
        sb::require(*v == i * 4);
      } else {
        sb::require(v == nullptr);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("clear - empties, preserves capacity, allows refill");
  {
    micron::robin_map<int, int> m(128);
    usize cap = m.max_size();
    for ( int i = 0; i < 50; ++i ) m.insert(i, i);
    m.clear();
    sb::require(m.empty());
    sb::require(m.max_size() == cap);
    for ( int i = 0; i < 30; ++i ) m.insert(i, i);
    sb::require(m.size() == 30u);
  }
  sb::end_test_case();

  sb::test_case("clear - idempotent under repetition");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i);
    m.clear();
    m.clear();
    m.clear();
    sb::require(m.empty());
  }
  sb::end_test_case();

  sb::test_case("swap - exchanges contents bidirectionally");
  {
    micron::robin_map<int, int> a(64);
    micron::robin_map<int, int> b(64);
    for ( int i = 0; i < 10; ++i ) a.insert(i, i);
    for ( int i = 100; i < 115; ++i ) b.insert(i, i);
    a.swap(b);
    sb::require(a.size() == 15u);
    sb::require(b.size() == 10u);
    for ( int i = 100; i < 115; ++i ) sb::require(*a.find(i) == i);
    for ( int i = 0; i < 10; ++i ) sb::require(*b.find(i) == i);
  }
  sb::end_test_case();

  sb::test_case("swap - of empty maps is safe");
  {
    micron::robin_map<int, int> a(64);
    micron::robin_map<int, int> b(64);
    a.swap(b);
    sb::require(a.empty());
    sb::require(b.empty());
  }
  sb::end_test_case();

  sb::test_case("swap - asymmetric sizes maintained");
  {
    micron::robin_map<int, int> a(64);
    micron::robin_map<int, int> b(64);
    for ( int i = 0; i < 40; ++i ) a.insert(i, i);
    for ( int i = 0; i < 3; ++i ) b.insert(i + 500, i);
    a.swap(b);
    sb::require(a.size() == 3u);
    sb::require(b.size() == 40u);
  }
  sb::end_test_case();

  sb::test_case("iterator - range-for visits all keys exactly once");
  {
    micron::robin_map<int, int> m(64);
    const int N = 25;
    for ( int i = 0; i < N; ++i ) m.insert(i, i * 11);

    std::set<int> seen;
    for ( auto &n : m ) seen.insert(n.value);
    sb::require(seen.size() == (usize)N);
    for ( int i = 0; i < N; ++i ) sb::require(seen.count(i * 11) == 1);
  }
  sb::end_test_case();

  sb::test_case("iterator - const range-for visits same set");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i + 50);
    const auto &cm = m;
    int sum = 0;
    for ( const auto &n : cm ) sum += n.value;
    sb::require(sum == (20 * 19 / 2) + 20 * 50);
  }
  sb::end_test_case();

  sb::test_case("iterator - empty map: begin == end");
  {
    micron::robin_map<int, int> m(64);
    sb::require(m.begin() == m.end());
    const auto &cm = m;
    sb::require(cm.begin() == cm.end());
    sb::require(cm.cbegin() == cm.cend());
  }
  sb::end_test_case();

  sb::test_case("iterator - operator-> reaches members");
  {
    micron::robin_map<int, micron::hstring<char>> m(64);
    m.insert(1, "hello");
    m.insert(2, "world");
    int total = 0;
    for ( auto it = m.begin(); it != m.end(); ++it ) total += static_cast<int>(it->value.size());
    sb::require(total == 10);
  }
  sb::end_test_case();

  sb::test_case("iterator - explicit prefix ++ visits every slot");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i);
    std::set<int> seen;
    auto it = m.begin();
    while ( it != m.end() ) {
      seen.insert(it->value);
      ++it;
    }
    sb::require(seen.size() == 10u);
  }
  sb::end_test_case();

  sb::test_case("raw_begin/end - span the full backing array");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 30; ++i ) m.insert(i, i);
    auto diff = m.raw_end() - m.raw_begin();
    sb::require((usize)diff == m.max_size());
  }
  sb::end_test_case();

  sb::test_case("raw_begin/end - const overloads");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i);
    const auto &cm = m;
    auto diff = cm.raw_end() - cm.raw_begin();
    sb::require((usize)diff == cm.max_size());
  }
  sb::end_test_case();

  sb::test_case("for_each - mutable visits every occupied slot");
  {
    micron::robin_map<int, int> m(256);
    const int N = 100;
    for ( int i = 0; i < N; ++i ) m.insert(i, i);

    int count = 0;
    long long sum = 0;
    m.for_each([&](auto &n) {
      ++count;
      sum += n.value;
    });
    sb::require(count == N);
    sb::require(sum == (long long)(N) * (N - 1) / 2);
  }
  sb::end_test_case();

  sb::test_case("for_each - const variant on const map");
  {
    micron::robin_map<int, int> m(128);
    for ( int i = 0; i < 30; ++i ) m.insert(i, i + 1);
    const auto &cm = m;
    int total = 0;
    cm.for_each([&](const auto &n) { total += n.value; });
    sb::require(total == 30 * 31 / 2);
  }
  sb::end_test_case();

  sb::test_case("for_each - permits in-place mutation");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i);
    m.for_each([](auto &n) { n.value *= 2; });
    for ( int i = 0; i < 20; ++i ) sb::require(*m.find(i) == i * 2);
  }
  sb::end_test_case();

  sb::test_case("ctrl - non-null after construction");
  {
    micron::robin_map<int, int> m(64);
    sb::require(m.ctrl() != nullptr);
  }
  sb::end_test_case();

  sb::test_case("ctrl - 0 = empty, non-zero = occupied; count matches size");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i);
    const auto *c = m.ctrl();
    usize occ = 0;
    for ( usize i = 0; i < m.max_size(); ++i )
      if ( c[i] != 0u ) ++occ;
    sb::require(occ == m.size());
  }
  sb::end_test_case();

  sb::test_case("ctrl - byte 255 never appears under normal load");
  {
    micron::robin_map<int, int> m(2048);
    for ( int i = 0; i < 1500; ++i ) m.insert(i, i);
    const auto *c = m.ctrl();
    for ( usize i = 0; i < m.max_size(); ++i ) sb::require(c[i] != 255u);
  }
  sb::end_test_case();

  sb::test_case("slot_occupied - bounds-checked, returns false out of range");
  {
    micron::robin_map<int, int> m(64);
    m.insert(1, 1);
    sb::require(m.slot_occupied(m.max_size() + 100u) == false);
  }
  sb::end_test_case();

  sb::test_case("slot_occupied / unsafe variant agree in valid range");
  {
    micron::robin_map<int, int> m(64);
    for ( int i = 0; i < 30; ++i ) m.insert(i, i);
    for ( usize i = 0; i < m.max_size(); ++i ) sb::require(m.slot_occupied(i) == m.slot_occupied_unsafe(i));
  }
  sb::end_test_case();

  sb::test_case("heavy - 5000 int keys: insert + lookup");
  {
    const int N = 5000;
    micron::robin_map<int, int> m(8192);
    for ( int i = 0; i < N; ++i ) m.insert(i, i * 3);
    sb::require(m.size() == (usize)N);
    for ( int i = 0; i < N; ++i ) {
      auto *v = m.find(i);
      sb::require(v != nullptr);
      sb::require(*v == i * 3);
    }
    sb::require(m.load_factor() <= 7.0f / 8.0f);
  }
  sb::end_test_case();

  sb::test_case("heavy - 10000 string keys: insert + lookup");
  {
    const int N = 10000;
    micron::robin_map<micron::hstring<char>, int> m(16384);
    for ( int i = 0; i < N; ++i ) m.insert(make_skey(i), i);
    sb::require(m.size() == (usize)N);
    for ( int i = 0; i < N; ++i ) {
      auto k = make_skey(i);
      auto *v = m.find(k);
      sb::require(v != nullptr);
      sb::require(*v == i);
    }
  }
  sb::end_test_case();

  sb::test_case("heavy - 20000-op mix vs std::unordered_map ground truth");
  {
    std::mt19937 rng(0xCAFEFACEu);
    micron::robin_map<int, int> m(8192);
    std::unordered_map<int, int> truth;
    const int OPS = 20000;
    for ( int op = 0; op < OPS; ++op ) {
      int k = std::uniform_int_distribution<int>(0, 1499)(rng);
      int v = std::uniform_int_distribution<int>(0, 1'000'000)(rng);
      int choice = std::uniform_int_distribution<int>(0, 3)(rng);
      if ( choice == 0 ) {
        m.insert(k, v);
        truth[k] = v;
      } else if ( choice == 1 ) {
        bool em = m.erase(k);
        auto et = truth.erase(k);
        sb::require(em == (et > 0));
      } else if ( choice == 2 ) {
        auto *mv = m.find(k);
        auto tit = truth.find(k);
        if ( tit == truth.end() ) {
          sb::require(mv == nullptr);
        } else {
          sb::require(mv != nullptr);
          sb::require(*mv == tit->second);
        }
      } else {
        sb::require(m.contains(k) == (truth.count(k) > 0));
      }
      sb::require(m.size() == truth.size());
    }

    for ( auto &kv : truth ) {
      auto *mv = m.find(kv.first);
      sb::require(mv != nullptr);
      sb::require(*mv == kv.second);
    }
  }
  sb::end_test_case();

  sb::test_case("heavy - clear/refill 50 cycles preserves correctness");
  {
    micron::robin_map<int, int> m(512);
    for ( int cycle = 0; cycle < 50; ++cycle ) {
      for ( int i = 0; i < 100; ++i ) m.insert(i + cycle * 10000, i + cycle * 10000);
      sb::require(m.size() == 100u);
      for ( int i = 0; i < 100; ++i ) sb::require(*m.find(i + cycle * 10000) == i + cycle * 10000);
      m.clear();
      sb::require(m.empty());
    }
  }
  sb::end_test_case();

  sb::test_case("heavy - random half-erase, remaining keys all findable");
  {
    const int N = 3000;
    micron::robin_map<int, int> m(8192);
    for ( int i = 0; i < N; ++i ) m.insert(i, i ^ 0xBEEF);

    std::mt19937 rng(12345);
    std::vector<int> order(N);
    for ( int i = 0; i < N; ++i ) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng);

    for ( int j = 0; j < N / 2; ++j ) {
      sb::require(m.erase(order[j]) == true);
      sb::require(m.size() == (usize)(N - j - 1));
    }

    for ( int j = N / 2; j < N; ++j ) {
      auto *v = m.find(order[j]);
      sb::require(v != nullptr);
      sb::require(*v == (order[j] ^ 0xBEEF));
    }
    for ( int j = 0; j < N / 2; ++j ) sb::require(m.find(order[j]) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("heavy - churn 100 cycles, alternating insert/erase batches");
  {
    micron::robin_map<int, int> m(4096);
    std::mt19937 rng(42);
    int next_key = 0;
    std::vector<int> live;
    for ( int cycle = 0; cycle < 100; ++cycle ) {
      for ( int i = 0; i < 50; ++i ) {
        int k = next_key++;
        m.insert(k, k * 7);
        live.push_back(k);
      }
      std::shuffle(live.begin(), live.end(), rng);
      for ( int i = 0; i < 25; ++i ) {
        sb::require(m.erase(live.back()) == true);
        live.pop_back();
      }
      for ( int k : live ) {
        auto *v = m.find(k);
        sb::require(v != nullptr);
        sb::require(*v == k * 7);
      }
      sb::require(m.size() == live.size());
    }
  }
  sb::end_test_case();

  sb::test_case("heavy - operator[] under 5000 keys");
  {
    micron::robin_map<int, int> m(8192);
    for ( int i = 0; i < 5000; ++i ) m[i] = i + 1;
    sb::require(m.size() == 5000u);
    for ( int i = 0; i < 5000; ++i ) sb::require(m[i] == i + 1);
  }
  sb::end_test_case();

  sb::test_case("heavy - iteration over 4000 elements visits all once");
  {
    const int N = 4000;
    micron::robin_map<int, int> m(8192);
    for ( int i = 0; i < N; ++i ) m.insert(i, i);
    usize count = 0;
    long long sum = 0;
    for ( auto &n : m ) {
      ++count;
      sum += n.value;
    }
    sb::require(count == (usize)N);
    sb::require(sum == (long long)(N) * (N - 1) / 2);
  }
  sb::end_test_case();

  sb::test_case("heavy - for_each over 4000 elements matches range-for");
  {
    const int N = 4000;
    micron::robin_map<int, int> m(8192);
    for ( int i = 0; i < N; ++i ) m.insert(i, i);
    usize count = 0;
    long long sum = 0;
    m.for_each([&](auto &n) {
      ++count;
      sum += n.value;
    });
    sb::require(count == (usize)N);
    sb::require(sum == (long long)(N) * (N - 1) / 2);
  }
  sb::end_test_case();

  sb::test_case("edge - insert past 7/8 load factor throws");
  {
    micron::robin_map<int, int> m(16);
    bool threw = false;
    int inserted = 0;
    try {
      for ( int i = 0; i < 100; ++i ) {
        m.insert(i, i);
        ++inserted;
      }
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw == true);
    sb::require(inserted == 14);
  }
  sb::end_test_case();

  sb::test_case("edge - all queries on empty map are safe");
  {
    micron::robin_map<int, int> m(64);
    sb::require(m.find(0) == nullptr);
    sb::require(!m.contains(0));
    sb::require(m.erase(0) == false);
    sb::require(m.count(0) == 0u);
    sb::require(m.exists(0) == 0u);
    m.clear();
    sb::require(m.empty());
    sb::require(m.begin() == m.end());
    int seen = 0;
    m.for_each([&](auto &) { ++seen; });
    sb::require(seen == 0);
  }
  sb::end_test_case();

  sb::test_case("edge - operator[] on missing yields default-constructed");
  {
    micron::robin_map<int, int> m(64);
    int &r = m[100];
    sb::require(r == 0);
  }
  sb::end_test_case();

  sb::test_case("value types - struct with non-trivial members");
  {
    struct s {
      int a;
      double b;
      micron::hstring<char> c;
    };

    micron::robin_map<int, s> m(64);
    m.insert(1, s{ 10, 3.14, "alpha" });
    auto *v = m.find(1);
    sb::require(v != nullptr);
    sb::require(v->a == 10);
    sb::require(v->b == 3.14);
    sb::require(v->c == "alpha");
  }
  sb::end_test_case();

  sb::test_case("value types - integer extremes");
  {
    micron::robin_map<int, int> m(64);
    m.insert(1, INT_MAX);
    m.insert(2, INT_MIN);
    m.insert(3, 0);
    sb::require(*m.find(1) == INT_MAX);
    sb::require(*m.find(2) == INT_MIN);
    sb::require(*m.find(3) == 0);
  }
  sb::end_test_case();

  sb::test_case("value types - large strings (1023 chars)");
  {
    micron::robin_map<int, micron::hstring<char>> m(64);
    char buf[1024];
    for ( size_t i = 0; i < sizeof(buf) - 1; ++i ) buf[i] = static_cast<char>('a' + (i % 26));
    buf[1023] = '\0';
    m.insert(1, buf);
    sb::require(m.find(1)->size() == 1023u);
  }
  sb::end_test_case();

  sb::test_case("independence - separate maps stay isolated");
  {
    micron::robin_map<int, int> a(64);
    micron::robin_map<int, int> b(64);
    for ( int i = 0; i < 30; ++i ) a.insert(i, i);
    for ( int i = 0; i < 30; ++i ) b.insert(i, i * 100);
    for ( int i = 0; i < 30; ++i ) sb::require(*a.find(i) == i);
    for ( int i = 0; i < 30; ++i ) sb::require(*b.find(i) == i * 100);
    a.clear();
    sb::require(b.size() == 30u);
    for ( int i = 0; i < 30; ++i ) sb::require(*b.find(i) == i * 100);
  }
  sb::end_test_case();

  sb::test_case("iterator - range-for count matches size after random erase");
  {
    micron::robin_map<int, int> m(256);
    for ( int i = 0; i < 100; ++i ) m.insert(i, i);
    std::mt19937 rng(99);
    std::vector<int> keys(100);
    for ( int i = 0; i < 100; ++i ) keys[i] = i;
    std::shuffle(keys.begin(), keys.end(), rng);
    for ( int i = 0; i < 50; ++i ) m.erase(keys[i]);

    usize count = 0;
    for ( auto &n : m ) {
      usize slot = static_cast<usize>(&n - m.raw_begin());
      sb::require(m.slot_occupied(slot));
      ++count;
    }
    sb::require(count == m.size());
    sb::require(count == 50u);
  }
  sb::end_test_case();

  sb::test_case("collisions - 200 colliding keys insert + find + half-erase");
  {
    using K = uint64_t;
    micron::robin_map<K, K> m(1024);
    usize mask = m.max_size() - 1;
    std::vector<K> coll;
    for ( K c = 1; c < 5'000'000ull && coll.size() < 200; ++c )
      if ( (micron::hash<micron::hash64_t>(c) & mask) == 0u ) coll.push_back(c);
    sb::require(coll.size() == 200u);

    for ( auto k : coll ) m.insert(k, k * 13);
    sb::require(m.size() == 200u);
    for ( auto k : coll ) {
      auto *v = m.find(k);
      sb::require(v != nullptr);
      sb::require(*v == k * 13);
    }
    for ( size_t i = 0; i < 100; ++i ) sb::require(m.erase(coll[i]) == true);
    sb::require(m.size() == 100u);
    for ( size_t i = 100; i < 200; ++i ) sb::require(*m.find(coll[i]) == coll[i] * 13);
  }
  sb::end_test_case();

  sb::test_case("collisions - 254 colliding keys is the saturation boundary");
  {
    using K = uint64_t;
    micron::robin_map<K, K> m(2048);
    usize mask = m.max_size() - 1;
    std::vector<K> coll;
    for ( K c = 1; c < 8'000'000ull && coll.size() < 260; ++c )
      if ( (micron::hash<micron::hash64_t>(c) & mask) == 0u ) coll.push_back(c);
    sb::require(coll.size() >= 254u);

    for ( size_t i = 0; i < 254; ++i ) m.insert(coll[i], coll[i]);
    sb::require(m.size() == 254u);
    bool threw = false;
    try {
      m.insert(coll[254], coll[254]);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw == true);
    sb::require(m.size() == 254u);

    for ( size_t i = 0; i < 254; ++i ) {
      auto *v = m.find(coll[i]);
      sb::require(v != nullptr);
      sb::require(*v == coll[i]);
    }

    const auto *c = m.ctrl();
    for ( usize i = 0; i < m.max_size(); ++i ) sb::require(c[i] != 255u);
  }
  sb::end_test_case();

  sb::test_case("lifetime - mixed-op stress keeps live == size at every step");
  {
    counted::reset();
    {
      micron::robin_map<int, counted> m(2048);
      std::mt19937 rng(0xC0FFEEu);
      const int OPS = 2000;
      for ( int i = 0; i < OPS; ++i ) {
        int k = std::uniform_int_distribution<int>(0, 299)(rng);
        int c = std::uniform_int_distribution<int>(0, 2)(rng);
        if ( c == 0 )
          m.insert(k, counted(k * 11));
        else if ( c == 1 )
          m.erase(k);
        else
          (void)m.find(k);
        sb::require(counted::live == (int)m.size());
      }
    }
    sb::require(counted::live == 0);
  }
  sb::end_test_case();

  sb::test_case("lifetime - clear after stress drops to zero");
  {
    counted::reset();
    micron::robin_map<int, counted> m(1024);
    for ( int i = 0; i < 500; ++i ) m.insert(i, counted(i));
    sb::require(counted::live == 500);
    m.clear();
    sb::require(counted::live == 0);

    for ( int i = 0; i < 100; ++i ) m.insert(i, counted(i + 1000));
    sb::require(counted::live == 100);
    sb::require(m.find(42)->v == 1042);
  }
  sb::end_test_case();

  sb::test_case("invariant - ctrl byte 0/!0 matches occupancy after churn");
  {
    micron::robin_map<int, int> m(2048);
    std::mt19937 rng(7);
    std::vector<int> live;
    for ( int i = 0; i < 600; ++i ) {
      m.insert(i, i);
      live.push_back(i);
    }
    int next_new = 1'000'000;
    for ( int cycle = 0; cycle < 5; ++cycle ) {
      std::shuffle(live.begin(), live.end(), rng);
      for ( int i = 0; i < 200; ++i ) sb::require(m.erase(live[i]) == true);
      live.erase(live.begin(), live.begin() + 200);
      for ( int i = 0; i < 200; ++i ) {
        m.insert(next_new, next_new);
        live.push_back(next_new);
        ++next_new;
      }
      sb::require(m.size() == 600u);

      const auto *c = m.ctrl();
      usize counted_occ = 0;
      for ( usize i = 0; i < m.max_size(); ++i ) {
        if ( c[i] != 0u ) ++counted_occ;
        sb::require(c[i] != 255u);
      }
      sb::require(counted_occ == m.size());
    }
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
