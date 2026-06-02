//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Audit-regression rigor for micron::cactus_stack<T,N> and micron::fixed_stack<T,N>
// (persistent arena stacks). Complements the broad existing cactus_stack.cpp.
// Targets the fixes: empty pop/top/try_pop/pop_range no longer read __slot(-1)/
// __val(-1) (now empty-pop => empty, observers throw), copy/move spine handling
// is exactly-once (lifetime + owning ASan), and operator< keeps its bottom-aligned
// (parent < child) semantics. The N-sized per-call scratch arrays are gone (proved
// separately with -Wstack-usage); here we just confirm large-N ops stay correct.

#include "../../src/stacks/cactus.hpp"
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

using stest::owner;
using stest::splitmix64;
using stest::tracked;

namespace
{

// persistence + branch + try_pop/pop_range + ordering; works for both arena stacks.
template<class Stack>
bool
correctness()
{
  Stack a;
  if ( !a.is_empty() || a.len() != 0 ) return false;
  Stack b = a.push(1).push(2).push(3);      // [3,2,1]
  if ( !a.is_empty() ) return false;        // persistent
  if ( b.len() != 3 || b.top() != 3 ) return false;
  Stack c = b.pop();
  if ( b.len() != 3 || b.top() != 3 ) return false;      // b intact
  if ( c.len() != 2 || c.top() != 2 ) return false;
  Stack d = b.push(99);      // branch
  if ( d.len() != 4 || d.top() != 99 ) return false;
  if ( b.len() != 3 || b.top() != 3 ) return false;      // still intact
  // try_pop
  auto pr = b.try_pop();
  if ( pr.value != 3 || pr.stack.len() != 2 ) return false;
  // pop_range
  int x = 0, y = 0;
  Stack g = b.pop_range(x, y);
  if ( x != 3 || y != 2 || g.len() != 1 || g.top() != 1 ) return false;
  // ordering: a parent (bottom-prefix) is < its extension
  if ( !(b < d) ) return false;      // b is a bottom-prefix of d
  if ( b < b ) return false;         // irreflexive
  return true;
}

template<class Stack>
bool
oracle_chain(u64 seed, int n)
{
  u64 rng = seed;
  Stack s;
  std::vector<int> oracle;
  for ( int i = 0; i < n; ++i ) {
    const u64 roll = splitmix64(rng);
    if ( (roll & 3) != 0 || oracle.empty() ) {
      const int v = static_cast<int>(splitmix64(rng) & 0xffff);
      s = s.push(v);
      oracle.push_back(v);
    } else {
      if ( s.top() != oracle.back() ) return false;
      s = s.pop();
      oracle.pop_back();
    }
    if ( s.len() != oracle.size() ) return false;
    if ( !oracle.empty() && s.top() != oracle.back() ) return false;
  }
  return true;
}

}      // namespace

int
main(void)
{
  print("=== CACTUS / FIXED STACK EXHAUSTIVE ===");

  test_case("cactus_stack: persistence / branch / try_pop / pop_range / ordering");
  {
    require(correctness<micron::cactus_stack<int, 256>>());
  }
  end_test_case();

  test_case("fixed_stack: persistence / branch / try_pop / pop_range / ordering");
  {
    require(correctness<micron::fixed_stack<int, 256>>());
  }
  end_test_case();

  test_case("cactus_stack: empty pop -> empty (no __slot(-1) OOB); observers throw");
  {
    micron::cactus_stack<int, 64> e;
    auto p1 = e.pop();      // must NOT read __slot(-1)
    require(p1.is_empty());
    auto p2 = e.pop().pop();      // repeated empty pop is still safe
    require(p2.is_empty());
    require_throw([&] { (void)e.top(); });
    require_throw([&] { (void)e.val(); });
    require_throw([&] {
      auto r = e.try_pop();
      (void)r.value;
    });
    micron::cactus_stack<int, 64> two = e.push(1).push(2);
    int a = 0, b = 0, c = 0;
    require_throw([&] { (void)two.pop_range(a, b, c); });      // only 2 elements, asked 3
  }
  end_test_case();

  test_case("fixed_stack: empty pop -> empty; observers throw; pop_range guarded");
  {
    micron::fixed_stack<int, 64> e;
    require(e.pop().is_empty());
    require_throw([&] { (void)e.top(); });
    require_throw([&] { (void)e.val(); });
    micron::fixed_stack<int, 64> two = e.push(1).push(2);
    int a = 0, b = 0, c = 0;
    require_throw([&] { (void)two.pop_range(a, b, c); });
  }
  end_test_case();

  test_case("cactus_stack: random push/pop chain vs std::vector oracle");
  {
    require(oracle_chain<micron::cactus_stack<int, 300>>(0xCAC705ULL, 250));
  }
  end_test_case();

  test_case("fixed_stack: random push/pop chain vs std::vector oracle");
  {
    require(oracle_chain<micron::fixed_stack<int, 300>>(0xF15EDULL, 250));
  }
  end_test_case();

  test_case("cactus_stack: lifetime balanced across branch/copy/move (tracked)");
  {
    tracked::reset();
    {
      micron::cactus_stack<tracked, 64> a;
      auto b = a.push(tracked(1)).push(tracked(2)).push(tracked(3));
      auto c = b.pop();
      auto d = b.push(tracked(4));
      micron::cactus_stack<tracked, 64> cp(b);
      micron::cactus_stack<tracked, 64> mv(micron::move(cp));
      require(b.len() == 3 && c.len() == 2 && d.len() == 4 && mv.len() == 3);
    }
    require(tracked::balanced());
  }
  end_test_case();

  test_case("cactus_stack + fixed_stack: owning type freed exactly once (ASan)");
  {
    owner::reset();
    {
      micron::cactus_stack<owner, 64> a;
      auto b = a.push(owner(1)).push(owner(2)).push(owner(3));
      auto c = b.pop();
      auto d = b.push(owner(4));
      micron::cactus_stack<owner, 64> cp(b);
      micron::cactus_stack<owner, 64> mv(micron::move(cp));
      (void)c;
      (void)d;
      (void)mv;
    }
    require(owner::live == 0);

    owner::reset();
    {
      micron::fixed_stack<owner, 64> a;
      auto b = a.push(owner(1)).push(owner(2));
      auto c = b.pop();
      micron::fixed_stack<owner, 64> mv(micron::move(b));
      (void)c;
      (void)mv;
    }
    require(owner::live == 0);
  }
  end_test_case();

  test_case("cactus_stack: large-N ops stay correct (no per-call scratch array)");
  {
    // N is large (the OLD code put i32[N] / 2x i32[N] on the stack per copy/compare);
    // depth is kept small so the O(depth) per-push copy is cheap. Just confirm the
    // no-array paths still produce correct results at large N.
    micron::cactus_stack<int, 20000> a;
    auto b = a.push(10).push(20).push(30).push(40);
    auto c = b.push(50);
    require(b.len() == 4 && b.top() == 40);
    require(c.len() == 5 && c.top() == 50);
    require(b < c);                              // bottom-prefix ordering, no as_[20000] frame
    micron::cactus_stack<int, 20000> cp(b);      // copy_spine, no spine[20000] frame
    require(cp == b);
  }
  end_test_case();

  print("=== ALL TESTS PASSED ===");
  return 1;
}
