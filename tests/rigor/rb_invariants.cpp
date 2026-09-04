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

  sb::test_case("lower_bound / upper_bound / equal_range vs std::set");
  {
    micron::rb_tree<int> t;
    std::set<int> oracle;
    u64 rng = 0x51A7B0UL;
    for ( int i = 0; i < 400; ++i ) {
      const int v = static_cast<int>(splitmix64(rng++) % 500) * 2;      // evens only
      t.insert(v);
      oracle.insert(v);
    }
    // probe evens (present), odds (absent, strictly between), and both ends
    for ( int probe = -3; probe <= 1003; ++probe ) {
      auto lb = t.lower_bound(probe);
      auto ub = t.upper_bound(probe);
      auto olb = oracle.lower_bound(probe);
      auto oub = oracle.upper_bound(probe);

      if ( olb == oracle.end() )
        sb::require(lb == t.end());
      else {
        sb::require(lb != t.end());
        sb::require(*lb == *olb);
      }
      if ( oub == oracle.end() )
        sb::require(ub == t.end());
      else {
        sb::require(ub != t.end());
        sb::require(*ub == *oub);
      }

      auto er = t.equal_range(probe);
      sb::require(er.a == lb);
      sb::require(er.b == ub);
      sb::require(t.count(probe) == (oracle.count(probe) ? 1ULL : 0ULL));
    }
  }
  sb::end_test_case();

  sb::test_case("node pool - erase/insert churn reuses slots, blocks do not grow without bound");
  {
    micron::rb_tree<u64> t;
    for ( u64 i = 0; i < 20000; ++i ) t.insert(splitmix64(i));
    const usize blocks_after_build = t.blocks_allocated();
    sb::require(blocks_after_build > 0ULL);

    // drain and rebuild the same count many times. every freed node must come back off the free
    // list, so the block count must not climb -- that is the whole contract of the pool
    for ( u32 round = 0; round < 8; ++round ) {
      for ( u64 i = 0; i < 20000; ++i ) t.erase(splitmix64(i));
      sb::require(t.size() == 0ULL);
      for ( u64 i = 0; i < 20000; ++i ) t.insert(splitmix64(i));
      sb::require(t.size() == 20000ULL);
    }
    sb::require(t.blocks_allocated() == blocks_after_build);

    // clear() must actually hand the blocks back, not just recycle nodes
    t.clear();
    sb::require(t.size() == 0ULL);
    sb::require(t.blocks_allocated() == 0ULL);

    // and the tree must still work afterwards
    for ( u64 i = 0; i < 1000; ++i ) t.insert(splitmix64(i + 777));
    sb::require(t.size() == 1000ULL);
    for ( u64 i = 0; i < 1000; ++i ) sb::require(t.contains(splitmix64(i + 777)));
  }
  sb::end_test_case();

  sb::test_case("node pool - reserve, and copy builds an independent pool");
  {
    micron::rb_tree<int> a;
    a.reserve(4096);
    for ( int i = 0; i < 4000; ++i ) a.insert(i);
    // everything fit in the reserved run
    sb::require(a.blocks_allocated() == 1ULL);

    micron::rb_tree<int> b(a);
    sb::require(b.size() == a.size());
    // freeing a must not touch b -- separate pools
    a.clear();
    sb::require(a.blocks_allocated() == 0ULL);
    sb::require(b.size() == 4000ULL);
    for ( int i = 0; i < 4000; ++i ) sb::require(b.contains(i));

    // self-assignment must not destroy the tree
    micron::rb_tree<int> &br = b;
    b = br;
    sb::require(b.size() == 4000ULL);
  }
  sb::end_test_case();

  sb::test_case("node pool - non-trivial T is destroyed exactly once");
  {
    // T with a real destructor takes the destroy_subtree path in __destroy_all; a trivially
    // destructible one skips it. Both must free the blocks, and neither may double-destroy.
    struct counted {
      static int &
      live()
      {
        static int n = 0;
        return n;
      }

      int v;

      counted() noexcept : v(0) { ++live(); }

      explicit counted(int x) noexcept : v(x) { ++live(); }

      counted(const counted &o) noexcept : v(o.v) { ++live(); }

      counted(counted &&o) noexcept : v(o.v) { ++live(); }

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

      ~counted() { --live(); }

      bool
      operator<(const counted &o) const noexcept
      {
        return v < o.v;
      }
    };

    const int before = counted::live();
    {
      micron::rb_tree<counted> t;
      for ( int i = 0; i < 3000; ++i ) t.insert(counted(i));
      sb::require(t.size() == 3000ULL);
      for ( int i = 0; i < 1500; ++i ) sb::require(t.erase(counted(i)));
      sb::require(t.size() == 1500ULL);
    }
    sb::require(counted::live() == before);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
