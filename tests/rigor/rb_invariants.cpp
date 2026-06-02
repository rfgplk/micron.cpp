//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Property / invariant coverage for the red-black tree (src/trees/rb.hpp).

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/trees/rb.hpp"

#include "../snowball/snowball.hpp"

#include <cstdio>
#include <set>
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

bool
matches_oracle(micron::rb_tree<int> &t, const std::set<int> &oracle)
{
  std::vector<int> got;
  t.for_each([&](const int &v) { got.push_back(v); });
  if ( got.size() != oracle.size() ) return false;

  for ( size_t i = 1; i < got.size(); ++i )
    if ( !(got[i - 1] < got[i]) ) return false;

  std::vector<int> ov(oracle.begin(), oracle.end());
  for ( size_t i = 0; i < got.size(); ++i )
    if ( got[i] != ov[i] ) return false;
  return true;
}

}      // namespace

int
main(void)
{
  sb::print("=== RB INVARIANTS TESTS ===");

  sb::test_case("randomized insert/erase keeps order + membership");
  {
    u64 rng = 0x9E3779B1ULL;
    for ( int round = 0; round < 120; ++round ) {
      micron::rb_tree<int> t;
      std::set<int> oracle;

      const int ins = 150 + static_cast<int>(splitmix64(rng++) % 250);
      for ( int i = 0; i < ins; ++i ) {
        int k = static_cast<int>(splitmix64(rng++) % 1000);
        t.insert(k);
        oracle.insert(k);
        sb::require(t.size() == oracle.size());
        sb::require(t.contains(k));
      }
      sb::require(matches_oracle(t, oracle));
      if ( !oracle.empty() ) {
        sb::require(t.min() != nullptr && *t.min() == *oracle.begin());
        sb::require(t.max() != nullptr && *t.max() == *oracle.rbegin());
      }

      int since_check = 0;
      while ( !oracle.empty() ) {
        int k = static_cast<int>(splitmix64(rng++) % 1000);
        bool present = oracle.count(k) != 0;
        sb::require(t.erase(k) == present);
        if ( present ) oracle.erase(k);
        sb::require(t.size() == oracle.size());
        sb::require(!t.contains(k));
        if ( ++since_check >= 32 ) {
          since_check = 0;
          sb::require(matches_oracle(t, oracle));
        }
      }
      sb::require(t.empty());
      sb::require(t.size() == 0ULL);
      sb::require(t.min() == nullptr);
      sb::require(t.max() == nullptr);
    }
  }
  sb::end_test_case();

  sb::test_case("extract_min drains sorted; throws when empty");
  {
    micron::rb_tree<int> t;
    for ( int i = 0; i < 500; ++i ) t.insert(static_cast<int>(splitmix64(static_cast<u64>(i)) % 100000));

    int last = 0;
    bool first = true;
    while ( !t.empty() ) {
      int m = t.extract_min();
      if ( !first ) sb::require(last <= m);
      last = m;
      first = false;
    }
    sb::require(t.empty());

    bool threw = false;
    try {
      (void)t.extract_min();
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("repeated min-erase (root churn)");
  {
    micron::rb_tree<int> t;
    for ( int i = 0; i < 1000; ++i ) t.insert(i);
    for ( int i = 0; i < 1000; ++i ) {
      int *mn = t.min();
      sb::require(mn != nullptr);
      sb::require(*mn == i);
      sb::require(t.erase(*mn));
    }
    sb::require(t.empty());
  }
  sb::end_test_case();

  sb::test_case("copy + move + move-assign onto non-empty");
  {
    micron::rb_tree<int> a;
    for ( int i = 0; i < 100; ++i ) a.insert(i * 3);

    micron::rb_tree<int> b(a);
    sb::require(b.size() == a.size());

    micron::rb_tree<int> c(micron::move(a));
    sb::require(c.size() == 100ULL);
    sb::require(a.size() == 0ULL);

    micron::rb_tree<int> d;
    for ( int i = 0; i < 50; ++i ) d.insert(-i);
    d = micron::move(b);
    sb::require(d.size() == 100ULL);

    std::vector<int> got;
    d.for_each([&](const int &v) { got.push_back(v); });
    bool ok = got.size() == 100;
    for ( size_t i = 1; ok && i < got.size(); ++i )
      if ( !(got[i - 1] < got[i]) ) ok = false;
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("large tree teardown (iterative free)");
  {
    micron::rb_tree<int> t;
    for ( int i = 0; i < 100000; ++i ) t.insert(static_cast<int>(splitmix64(static_cast<u64>(i))));
    const usize n = t.size();
    sb::require(n > 0ULL);
  }
  sb::require(true);
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
