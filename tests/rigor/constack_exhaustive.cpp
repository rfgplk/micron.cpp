//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Rigor for micron::constack<T> — the mutex-synchronized LIFO (renamed from the
// colliding `stack`). Including the umbrella <micron/stack.hpp> here is itself the
// collision regression test: it pulls stacks/stack.hpp AND stacks/constack.hpp into
// one TU, which only compiles because the duplicate name is gone. Then: full
// single-threaded surface (value-returning observers, ordered-lock assign/swap/
// compare, self-assign), a concurrent disjoint-push gate (no lost/torn updates), a
// deterministic producer/consumer, and lifetime + owning-type ASan balance.
//
// Threading uses micron::auto_thread (NO <thread>/<atomic>, per the pthread shim).

#include "../../src/stack.hpp"      // umbrella: stack + constack in one TU (collision gone)
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"
#include "../../src/thread/thread.hpp"

#include "../snowball/snowball.hpp"
#include "../support/stack_rigor.hpp"

#include <vector>

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_throw;
using sb::test_case;

using stest::owner;
using stest::tracked;

int
main(void)
{
  print("=== CONSTACK (CONCURRENT) EXHAUSTIVE ===");

  test_case("constack: umbrella coexists with micron::stack (collision gone)");
  {
    micron::stack<int> plain;
    plain.push(1);
    micron::constack<int> conc;
    conc.push(1);
    require(plain.size() == 1ULL && conc.size() == 1ULL);
  }
  end_test_case();

  test_case("constack: single-threaded LIFO + value-returning observers");
  {
    micron::constack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    require(s.size() == 3ULL);
    require(s.top() == 3);      // returns by value
    require(s[0] == 3 && s[1] == 2 && s[2] == 1);
    require(s.pop() == 3);
    require(s.operator()() == 2);      // pop+return
    require(s.size() == 1ULL);
    require_throw([&] {
      micron::constack<int> e;
      (void)e.top();
    });
    require_throw([&] {
      micron::constack<int> e;
      (void)e.pop();
    });
  }
  end_test_case();

  test_case("constack: emplace(rvalue), push_range, pop_range, copy/move/assign/swap/compare");
  {
    micron::constack<int> s;
    s.emplace(42);
    require(s.top() == 42);
    s.push_range(1, 2, 3);      // atomic range push
    require(s.size() == 4ULL && s.top() == 3);
    int a = 0, b = 0;
    s.pop_range(a, b);
    require(a == 3 && b == 2 && s.size() == 2ULL);

    micron::constack<int> cp(s);
    require(cp == s);
    micron::constack<int> mv(micron::move(cp));
    require(mv.size() == 2ULL);
    micron::constack<int> as;
    as = s;
    require(as == s);
    as = as;      // self copy-assign
    require(as == s);
    micron::constack<int> x;
    x.push(7);
    x.swap(s);
    require(x.size() == 2ULL);
    require((x < mv) || (mv < x) || (x == mv));
  }
  end_test_case();

  test_case("constack: concurrent disjoint pushes -- no lost / torn updates");
  {
    micron::constack<int> s;
    {
      micron::auto_thread<> t1([&] {
        for ( int i = 0; i < 1000; ++i ) s.push(i);
      });
      micron::auto_thread<> t2([&] {
        for ( int i = 1000; i < 2000; ++i ) s.push(i);
      });
      micron::auto_thread<> t3([&] {
        for ( int i = 2000; i < 3000; ++i ) s.push(i);
      });
    }      // join
    require(s.size() == 3000ULL);
    std::vector<int> seen(3000, 0);
    while ( !s.empty() ) {
      int v = s.pop();
      require(v >= 0 && v < 3000);
      seen[static_cast<usize>(v)]++;
    }
    bool all_once = true;
    for ( int i = 0; i < 3000; ++i )
      if ( seen[static_cast<usize>(i)] != 1 ) all_once = false;
    require(all_once);      // every push survived exactly once
  }
  end_test_case();

  test_case("constack: concurrent producer/consumer smoke (deterministic final size)");
  {
    micron::constack<int> s;
    for ( int i = 0; i < 2000; ++i ) s.push(i);      // seed: consumer can always pop
    {
      micron::auto_thread<> prod([&] {
        for ( int i = 0; i < 2000; ++i ) s.push(i);
      });
      micron::auto_thread<> cons([&] {
        for ( int i = 0; i < 1500; ++i ) (void)s.pop();
      });
    }
    require(s.size() == 2500ULL);      // 2000 + 2000 - 1500
  }
  end_test_case();

  test_case("constack: lifetime balanced + owning type (ASan double-free/leak gate)");
  {
    tracked::reset();
    {
      micron::constack<tracked> s;
      for ( int i = 0; i < 20; ++i ) s.emplace(i);
      micron::constack<tracked> cp(s);
      micron::constack<tracked> mv(micron::move(cp));
      micron::constack<tracked> as;
      as = s;
      for ( int i = 0; i < 5; ++i ) (void)s.pop();
      s.clear();
    }
    require(tracked::balanced());

    owner::reset();
    {
      micron::constack<owner> s;
      for ( int i = 0; i < 30; ++i ) s.emplace(i);
      micron::constack<owner> cp(s);
      cp = s;                                            // copy-assign over non-empty (no leak)
      micron::constack<owner> mv(micron::move(cp));      // move-assign path covered too
      micron::constack<owner> as;
      as = micron::move(mv);
      while ( !s.empty() ) (void)s.pop();
    }
    require(owner::live == 0);
  }
  end_test_case();

  print("=== ALL TESTS PASSED ===");
  return 1;
}
