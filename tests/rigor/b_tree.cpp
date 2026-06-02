//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"
#include "../src/trees/b.hpp"

#include "../snowball/snowball.hpp"

#include <cstdio>
#include <map>
#include <vector>

namespace
{

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

struct moveonly {
  int v;

  moveonly() noexcept : v(0) { }

  explicit moveonly(int x) noexcept : v(x) { }

  moveonly(const moveonly &) = delete;
  moveonly &operator=(const moveonly &) = delete;

  moveonly(moveonly &&o) noexcept : v(o.v) { o.v = -1; }

  moveonly &
  operator=(moveonly &&o) noexcept
  {
    v = o.v;
    o.v = -1;
    return *this;
  }

  ~moveonly() = default;
};

template<class Tree>
bool
matches(Tree &t, const std::map<int, int> &oracle)
{
  if ( t.size() != oracle.size() ) return false;
  std::vector<int> ks, vs;
  t.for_each([&](const int &k, const int &v) {
    ks.push_back(k);
    vs.push_back(v);
  });
  if ( ks.size() != oracle.size() ) return false;
  size_t i = 0;
  for ( auto &kv : oracle ) {
    if ( ks[i] != kv.first || vs[i] != kv.second ) return false;
    ++i;
  }

  for ( size_t j = 1; j < ks.size(); ++j )
    if ( !(ks[j - 1] < ks[j]) ) return false;
  return true;
}

template<class Tree>
bool
stress(int rounds, int keyspace, u64 seed)
{
  u64 rng = seed;
  for ( int r = 0; r < rounds; ++r ) {
    Tree t;
    std::map<int, int> oracle;
    const int ops = 100 + static_cast<int>(splitmix64(rng++) % 400);

    for ( int i = 0; i < ops; ++i ) {
      int k = static_cast<int>(splitmix64(rng++) % keyspace);
      int val = static_cast<int>(splitmix64(rng++) & 0xffff);
      if ( splitmix64(rng++) & 1 ) {
        bool was = oracle.count(k) != 0;
        bool ins = t.insert(k, val);
        if ( ins == was ) return false;
        if ( !was ) oracle[k] = val;
      } else {
        t.insert_or_assign(k, val);
        oracle[k] = val;
      }
      if ( t.size() != oracle.size() ) return false;
      int *p = t.find(k);
      if ( !p || *p != oracle[k] ) return false;
    }
    if ( !matches(t, oracle) ) return false;

    int guard = 0;
    while ( !oracle.empty() && guard < keyspace * 4 ) {
      int k = static_cast<int>(splitmix64(rng++) % keyspace);
      bool was = oracle.count(k) != 0;
      bool er = t.erase(k);
      if ( er != was ) return false;
      if ( was ) oracle.erase(k);
      if ( t.size() != oracle.size() ) return false;
      if ( t.find(k) != nullptr ) return false;
      ++guard;
    }

    std::vector<int> rem;
    for ( auto &kv : oracle ) rem.push_back(kv.first);
    for ( int k : rem ) {
      if ( !t.erase(k) ) return false;
      oracle.erase(k);
    }
    if ( !t.empty() || !matches(t, oracle) ) return false;
  }
  return true;
}

}      // namespace

int
main(void)
{
  sb::print("=== B-TREE TESTS ===");

  using bt = micron::b_tree<int, int>;
  using bt2 = micron::b_tree<int, int, micron::b_default_less<int>, 2>;

  sb::test_case("construction - empty");
  {
    bt t;
    sb::require(t.empty());
    sb::require(t.size() == 0ULL);
    sb::require(t.find(5) == nullptr);
    sb::require(!t.erase(5));
  }
  sb::end_test_case();

  sb::test_case("insert / find / duplicate / insert_or_assign / operator[]");
  {
    bt t;
    sb::require(t.insert(1, 10));
    sb::require(!t.insert(1, 99));
    sb::require(*t.find(1) == 10);
    sb::require(!t.insert_or_assign(1, 99));
    sb::require(*t.find(1) == 99);
    sb::require(t.insert_or_assign(2, 20));
    sb::require(t[2] == 20);
    sb::require(t[3] == 0);
    sb::require(t.size() == 3ULL);
    sb::require(t.contains(2) && !t.contains(99));
    sb::require(t.count(2) == 1ULL && t.count(99) == 0ULL);
  }
  sb::end_test_case();

  sb::test_case("at - throws on miss");
  {
    bt t;
    t.insert(7, 70);
    sb::require(t.at(7) == 70);
    bool threw = false;
    try {
      t.at(8);
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("ordered iteration matches sorted order");
  {
    bt t;
    std::map<int, int> oracle;
    for ( int i = 0; i < 500; ++i ) {
      int k = static_cast<int>(splitmix64(static_cast<u64>(i)) % 1000);
      t.insert_or_assign(k, k * 2);
      oracle[k] = k * 2;
    }
    sb::require(matches(t, oracle));

    std::vector<int> via_it;
    for ( auto it = t.begin(); it != t.end(); ++it ) via_it.push_back((*it).key);
    sb::require(via_it.size() == oracle.size());
    bool ok = true;
    size_t j = 0;
    for ( auto &kv : oracle ) {
      if ( via_it[j++] != kv.first ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("lower_bound / upper_bound");
  {
    bt t;
    for ( int i = 0; i < 100; ++i ) t.insert(i * 2, i);

    sb::require((*t.lower_bound(50)).key == 50);
    sb::require((*t.lower_bound(51)).key == 52);
    sb::require((*t.upper_bound(50)).key == 52);
    sb::require(t.lower_bound(1000) == t.end());
  }
  sb::end_test_case();

  sb::test_case("randomized vs std::map oracle - minimum degree (heavy split/merge)");
  {
    sb::require(stress<bt2>(120, 160, 0xC0FFEEULL));
  }
  sb::end_test_case();

  sb::test_case("randomized vs std::map oracle - default degree");
  {
    sb::require(stress<bt>(40, 4000, 0xBEEF1234ULL));
  }
  sb::end_test_case();

  sb::test_case("bulk - 50000 insert / find / erase");
  {
    bt t;
    const int N = 50000;
    for ( int i = 0; i < N; ++i ) t.insert(static_cast<int>(splitmix64(static_cast<u64>(i))), i);
    sb::require(t.size() == static_cast<usize>(N));
    for ( int i = 0; i < N; ++i ) {
      int *v = t.find(static_cast<int>(splitmix64(static_cast<u64>(i))));
      sb::require(v != nullptr && *v == i);
    }
    for ( int i = 0; i < N; i += 2 ) sb::require(t.erase(static_cast<int>(splitmix64(static_cast<u64>(i)))));
    sb::require(t.size() == static_cast<usize>(N / 2));
    for ( int i = 0; i < N; ++i ) {
      int *v = t.find(static_cast<int>(splitmix64(static_cast<u64>(i))));
      if ( i % 2 == 0 )
        sb::require(v == nullptr);
      else
        sb::require(v != nullptr && *v == i);
    }
    t.clear();
    sb::require(t.empty());
  }
  sb::end_test_case();

  sb::test_case("non-trivial value (hstring) - no corruption");
  {
    micron::b_tree<int, micron::hstring<char>, micron::b_default_less<int>, 2> t;
    for ( int i = 0; i < 300; ++i ) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "val_%05d", i);
      t.insert(i, micron::hstring<char>(static_cast<const char *>(buf)));
    }
    sb::require(t.size() == 300ULL);
    for ( int i = 0; i < 300; i += 3 ) sb::require(t.erase(i));
    sb::require(t.size() == 200ULL);
    for ( int i = 0; i < 300; ++i ) {
      auto *s = t.find(i);
      if ( i % 3 == 0 ) {
        sb::require(s == nullptr);
      } else {
        sb::require(s != nullptr);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "val_%05d", i);
        sb::require(*s == micron::hstring<char>(static_cast<const char *>(buf)));
      }
    }
  }
  sb::end_test_case();

  sb::test_case("move-only value");
  {
    micron::b_tree<int, moveonly, micron::b_default_less<int>, 2> t;
    for ( int i = 0; i < 400; ++i ) t.insert(i, moveonly(i * 10));
    sb::require(t.size() == 400ULL);
    for ( int i = 0; i < 400; ++i ) {
      moveonly *m = t.find(i);
      sb::require(m != nullptr && m->v == i * 10);
    }
    for ( int i = 0; i < 400; i += 2 ) sb::require(t.erase(i));
    sb::require(t.size() == 200ULL);
    for ( int i = 1; i < 400; i += 2 ) {
      moveonly *m = t.find(i);
      sb::require(m != nullptr && m->v == i * 10);
    }
  }
  sb::end_test_case();

  sb::test_case("move ctor / move assign");
  {
    bt a;
    for ( int i = 0; i < 1000; ++i ) a.insert(i, i + 1);
    bt b(micron::move(a));
    sb::require(b.size() == 1000ULL);
    sb::require(a.size() == 0ULL);
    sb::require(*b.find(500) == 501);

    bt c;
    for ( int i = 0; i < 50; ++i ) c.insert(-i, i);
    c = micron::move(b);
    sb::require(c.size() == 1000ULL);
    sb::require(*c.find(999) == 1000);
    sb::require(b.size() == 0ULL);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
