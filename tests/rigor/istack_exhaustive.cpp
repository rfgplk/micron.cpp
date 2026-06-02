//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Rigor for the rewritten micron::istack<T> — a persistent / COW LIFO stack
// (refcounted immutable cons-list). Verifies: push/pop are non-destructive and
// return new versions; old versions stay valid; versions structurally SHARE
// nodes (copy is O(1), no value copies); a node is freed exactly once when the
// last referencing version dies (owning-type ASan gate); long-chain teardown is
// iterative (no stack overflow); move-only T; build-from-stack; empty throws.

#include "../../src/stacks/istack.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"
#include "../support/stack_rigor.hpp"

#include <vector>

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_throw;
using sb::test_case;

using stest::move_only;
using stest::owner;
using stest::splitmix64;
using stest::tracked;

int
main(void)
{
  print("=== ISTACK (PERSISTENT) EXHAUSTIVE ===");

  test_case("istack: push/pop are non-destructive (persistence)");
  {
    micron::istack<int> a;
    require(a.empty() && a.size() == 0ULL);
    auto b = a.push(1).push(2).push(3);      // b = [3,2,1]
    require(a.empty());                      // a untouched
    require(b.size() == 3ULL && b.top() == 3);
    require(b[0] == 3 && b[1] == 2 && b[2] == 1);
    auto c = b.pop();                               // c = [2,1]
    require(b.size() == 3ULL && b.top() == 3);      // b STILL intact
    require(c.size() == 2ULL && c.top() == 2);
  }
  end_test_case();

  test_case("istack: branching shares the common tail; all versions valid");
  {
    micron::istack<int> base;
    for ( int i = 0; i < 5; ++i ) base = base.push(i);      // [4,3,2,1,0]
    auto x = base.push(100);
    auto y = base.push(200);
    auto z = base.pop().push(300);
    require(base.size() == 5ULL && base.top() == 4);
    require(x.size() == 6ULL && x.top() == 100);
    require(y.size() == 6ULL && y.top() == 200);
    require(z.size() == 5ULL && z.top() == 300 && z[1] == 3);
    require(base.top() == 4);      // base unaffected by all branches
  }
  end_test_case();

  test_case("istack: empty pop / top throw");
  {
    micron::istack<int> e;
    require_throw([&] { (void)e.top(); });
    require_throw([&] { (void)e.pop(); });
    require_throw([&] { (void)e[0]; });
  }
  end_test_case();

  test_case("istack: copy SHARES (no value copies) + move + self-assign + swap");
  {
    tracked::reset();
    {
      micron::istack<tracked> b;
      for ( int i = 0; i < 10; ++i ) b = b.push(tracked(i));
      const long after_build = tracked::live;      // 10 live nodes
      require(after_build == 10);
      micron::istack<tracked> cp(b);              // structural share: must NOT copy values
      require(tracked::live == after_build);      // still 10 — proves O(1) sharing
      micron::istack<tracked> mv(micron::move(cp));
      require(tracked::live == after_build);
      micron::istack<tracked> ca;
      ca = b;
      ca = ca;      // self-assign
      require(ca.size() == 10ULL && tracked::live == after_build);
      micron::istack<tracked> s1 = b.pop();
      micron::istack<tracked> s2;
      s1.swap(s2);
      require(s2.size() == 9ULL && s1.size() == 0ULL);
    }
    require(tracked::balanced());
  }
  end_test_case();

  test_case("istack: vs std::vector LIFO oracle (random push/pop versions)");
  {
    u64 rng = 0x9151ACABULL;
    micron::istack<int> s;
    std::vector<int> oracle;
    for ( int i = 0; i < 4000; ++i ) {
      const u64 roll = splitmix64(rng);
      if ( (roll & 3) != 0 || oracle.empty() ) {
        const int v = static_cast<int>(splitmix64(rng) & 0xffff);
        s = s.push(v);
        oracle.push_back(v);
      } else {
        require(s.top() == oracle.back());
        s = s.pop();
        oracle.pop_back();
      }
      require(s.size() == oracle.size());
      if ( !oracle.empty() ) require(s.top() == oracle.back());
    }
  }
  end_test_case();

  test_case("istack: long chain teardown is iterative (no stack overflow)");
  {
    micron::istack<int> big;
    for ( int i = 0; i < 300000; ++i ) big = big.push(i);
    require(big.size() == 300000ULL && big.top() == 299999);
  }      // destroys 300k shared nodes iteratively here
  end_test_case();

  test_case("istack: build from a mutable micron::stack preserves order");
  {
    micron::stack<int> ms;
    ms.push(10);
    ms.push(20);
    ms.push(30);      // top == 30
    micron::istack<int> is(ms);
    require(is.size() == 3ULL);
    require(is.top() == 30 && is[1] == 20 && is[2] == 10);
  }
  end_test_case();

  test_case("istack: move-only element type");
  {
    micron::istack<move_only> a;
    auto b = a.push(move_only(1)).push(move_only(2));      // push(t&&) only
    auto c = b.pop();
    micron::istack<move_only> shared(b);      // share, no value copy
    require(b.size() == 2ULL && b.top().v == 2);
    require(c.size() == 1ULL && c.top().v == 1);
    require(shared.size() == 2ULL);
  }
  end_test_case();

  test_case("istack: owning type freed exactly once across a version graph (ASan)");
  {
    owner::reset();
    {
      micron::istack<owner> a;
      micron::istack<owner> b = a.push(owner(1)).push(owner(2)).push(owner(3));
      micron::istack<owner> c = b.pop();               // shares 2,1
      micron::istack<owner> d = b.push(owner(4));      // shares 3,2,1
      micron::istack<owner> e = b.pop().pop().push(owner(9));
      require(b.size() == 3ULL && c.size() == 2ULL && d.size() == 4ULL && e.size() == 2ULL);
    }
    require(owner::live == 0);      // every shared node freed once
  }
  end_test_case();

  print("=== ALL TESTS PASSED ===");
  return 1;
}
