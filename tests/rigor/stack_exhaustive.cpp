//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Rigor for micron::stack<T> (checked) and micron::fstack<T> (fast/unchecked).
// Randomized push/pop/top is diffed against a std::vector LIFO oracle (STL is
// oracle-only). Targets the audit fixes: moved-from push (was a null-deref),
// copy-assign into raw / shrink-leak, self-move-assign, the class-type
// initializer_list ctor (fstack's never compiled), emplace(rvalue), count-ctor,
// and full element-lifetime balance (tracked) + ASan owning-type coverage.

#include "../../src/stacks/fstack.hpp"
#include "../../src/stacks/stack.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"
#include "../support/stack_rigor.hpp"

#include <vector>

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using stest::move_only;
using stest::owner;
using stest::splitmix64;
using stest::tracked;

namespace
{

// randomized interleaved push/pop diffed against a std::vector LIFO oracle.
// works for both stack and fstack (only pops when non-empty, so no fstack UB).
template<class Stack>
bool
stress(int rounds, u64 seed)
{
  u64 rng = seed;
  for ( int r = 0; r < rounds; ++r ) {
    Stack s;
    std::vector<int> oracle;
    const int ops = 200 + static_cast<int>(splitmix64(rng) % 600);
    for ( int i = 0; i < ops; ++i ) {
      const u64 roll = splitmix64(rng);
      if ( (roll & 3) != 0 || oracle.empty() ) {      // ~75% push (and always when empty)
        const int v = static_cast<int>(splitmix64(rng) & 0xffff);
        s.push(v);
        oracle.push_back(v);
      } else {
        const int got = static_cast<int>(s.pop());
        const int want = oracle.back();
        oracle.pop_back();
        if ( got != want ) return false;
      }
      if ( s.size() != oracle.size() ) return false;
      if ( !oracle.empty() && static_cast<int>(s.top()) != oracle.back() ) return false;
    }
    // drain
    while ( !oracle.empty() ) {
      if ( static_cast<int>(s.pop()) != oracle.back() ) return false;
      oracle.pop_back();
    }
    if ( !s.empty() ) return false;
  }
  return true;
}

}      // namespace

int
main(void)
{
  print("=== STACK / FSTACK EXHAUSTIVE ===");

  // ── micron::stack (checked) ───────────────────────────────────────────────
  test_case("stack: empty + basic LIFO");
  {
    micron::stack<int> s;
    require(s.empty());
    require(s.size() == 0ULL);
    s.push(1);
    s.push(2);
    s.push(3);
    require(s.size() == 3ULL);
    require(static_cast<int>(s.top()) == 3);
    require(static_cast<int>(s.pop()) == 3);
    require(static_cast<int>(s.pop()) == 2);
    require(static_cast<int>(s.pop()) == 1);
    require(s.empty());
  }
  end_test_case();

  test_case("stack: throw on empty pop / top");
  {
    micron::stack<int> s;
    require_throw([&] { s.pop(); });
    require_throw([&] { (void)s.top(); });
  }
  end_test_case();

  test_case("stack: randomized vs std::vector oracle");
  {
    require(stress<micron::stack<int>>(60, 0xC0FFEEULL));
  }
  end_test_case();

  test_case("stack: moved-from reuse pushes (was a null-deref)");
  {
    micron::stack<int> a;
    for ( int i = 0; i < 10; ++i ) a.push(i);
    micron::stack<int> b(micron::move(a));      // a is now moved-from (cap 0)
    require(b.size() == 10ULL);
    a.push(42);      // must reallocate, not deref null
    a.push(43);
    require(a.size() == 2ULL);
    require(static_cast<int>(a.top()) == 43);
  }
  end_test_case();

  test_case("stack: self-move-assign is a no-op (no UAF)");
  {
    micron::stack<int> s;
    s.push(7);
    s.push(8);
    s = micron::move(s);
    require(s.size() == 2ULL);
    require(static_cast<int>(s.top()) == 8);
  }
  end_test_case();

  test_case("stack: copy-assign shrink + grow, lifetime balanced");
  {
    tracked::reset();
    {
      micron::stack<tracked> big;
      for ( int i = 0; i < 40; ++i ) big.emplace(i);
      micron::stack<tracked> small;
      for ( int i = 0; i < 5; ++i ) small.emplace(100 + i);
      small = big;      // grow dest, no assign-into-raw
      require(small.size() == 40ULL);
      big = micron::stack<tracked>{};      // (move-assign from temp)
      micron::stack<tracked> tiny;
      tiny.emplace(1);
      micron::stack<tracked> src;
      for ( int i = 0; i < 3; ++i ) src.emplace(i);
      // shrink path: assign a 3-elem stack over a 1-elem one and back
      tiny = src;
      require(tiny.size() == 3ULL);
    }
    require(tracked::balanced());
  }
  end_test_case();

  test_case("stack: initializer_list (class type) constructs by copy");
  {
    tracked::reset();
    {
      micron::stack<tracked> s{ tracked(1), tracked(2), tracked(3) };
      require(s.size() == 3ULL);
      require(static_cast<int>(s.top().v) == 3);
    }
    require(tracked::balanced());
  }
  end_test_case();

  test_case("stack: emplace forwards rvalues (constraint removed)");
  {
    micron::stack<int> s;
    s.emplace(42);      // rvalue must compile + work
    require(static_cast<int>(s.top()) == 42);
  }
  end_test_case();

  test_case("stack: count ctor");
  {
    micron::stack<int> s(5);
    require(s.size() == 5ULL);
  }
  end_test_case();

  test_case("stack: owning type, balanced (ASan double-free/leak gate)");
  {
    owner::reset();
    {
      micron::stack<owner> s;
      for ( int i = 0; i < 50; ++i ) s.emplace(i);
      micron::stack<owner> cp(s);
      micron::stack<owner> mv(micron::move(cp));
      cp = s;
      cp = cp;
      for ( int i = 0; i < 10; ++i ) (void)s.pop();
      s.clear();
    }
    require(owner::live == 0);
  }
  end_test_case();

  // ── micron::fstack (fast / unchecked) ─────────────────────────────────────
  test_case("fstack: basic LIFO + oracle stress");
  {
    micron::fstack<int> s;
    s.push(1);
    s.push(2);
    require(static_cast<int>(s.top()) == 2);
    require(static_cast<int>(s.pop()) == 2);
    require(static_cast<int>(s.pop()) == 1);
    require(s.empty());
    require(stress<micron::fstack<int>>(60, 0xBEEF99ULL));
  }
  end_test_case();

  test_case("fstack: initializer_list (class type) now compiles + balances");
  {
    tracked::reset();
    {
      micron::fstack<tracked> s{ tracked(1), tracked(2), tracked(3) };      // never compiled before
      require(s.size() == 3ULL);
      require(static_cast<int>(s.top().v) == 3);
    }
    require(tracked::balanced());
  }
  end_test_case();

  test_case("fstack: moved-from reuse + emplace(rvalue) + owning balance");
  {
    micron::fstack<int> a;
    for ( int i = 0; i < 8; ++i ) a.push(i);
    micron::fstack<int> b(micron::move(a));
    a.push(99);      // moved-from push
    require(a.size() == 1ULL && static_cast<int>(a.top()) == 99);
    micron::fstack<int> e;
    e.emplace(7);
    require(static_cast<int>(e.top()) == 7);

    owner::reset();
    {
      micron::fstack<owner> s;
      for ( int i = 0; i < 30; ++i ) s.emplace(i);
      micron::fstack<owner> cp(s);
      cp = s;
      micron::fstack<owner> mv(micron::move(cp));
      while ( !s.empty() ) (void)s.pop();
    }
    require(owner::live == 0);
  }
  end_test_case();

  test_case("fstack: move-only specialization instantiates + works");
  {
    micron::fstack<move_only> s;
    s.push(move_only(1));
    s.push(move_only(2));
    s.move(move_only(3));
    require(s.size() == 3ULL);
    require(s.top().v == 3);
    micron::fstack<move_only> b(micron::move(s));
    require(b.size() == 3ULL);
    require(b.pop().v == 3);
  }
  end_test_case();

  print("=== ALL TESTS PASSED ===");
  return 1;
}
