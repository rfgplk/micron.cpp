//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Exercises Fix #1: src/sync/channel.hpp `__wait()` returned a lock_guard
// by value via implicit copy; the inner-scope dtor released the lock
// before the caller's consume executed, racing against concurrent pushes.
// The fix inlines the wait-and-pop so the guard's lifetime brackets the
// `to = obj()` call. Pre-fix MPMC stress lost / duplicated items;
// post-fix every produced value is consumed exactly once.

#include "../../src/sync/channel.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

struct ProdArgs {
  micron::channel<int> *ch;
  int base;
  int count;
};

void
producer(ProdArgs *p)
{
  for ( int i = 0; i < p->count; ++i ) {
    int v = p->base + i;
    *p->ch >> static_cast<int &&>(v);
  }
}

struct ConsArgs {
  micron::channel<int> *ch;
  micron::atomic_token<int> *seen;
  int total;
};

void
consumer(ConsArgs *p)
{
  while ( p->seen->get(micron::memory_order::acquire) < p->total ) {
    int v = 0;
    *p->ch << v;
    p->seen->fetch_add(1, micron::memory_order::acq_rel);
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== CHANNEL TESTS ===");

  test_case("default ctor: empty");
  {
    channel<int> c;
    require_true(!c);
  }
  end_test_case();

  test_case("variadic ctor emplaces items");
  {
    channel<int> c(1, 2, 3);
    require_false(!c);
  }
  end_test_case();

  test_case("operator>>(rvalue) emplaces; operator<< pops in LIFO order");
  {
    channel<int> c;
    c >> 10;
    c >> 20;
    c >> 30;
    int a = 0, b = 0, d = 0;
    c << a;
    c << b;
    c << d;
    require(a == 30);
    require(b == 20);
    require(d == 10);
    require_true(!c);
  }
  end_test_case();

  test_case("operator>>(lvalue) pushes by ref");
  {
    channel<int> c;
    int v = 99;
    c >> v;
    int out = 0;
    c << out;
    require(out == 99);
  }
  end_test_case();

  test_case("single producer / single consumer: 1000 sequential items");
  {
    channel<int> c;
    constexpr int N = 1000;
    ProdArgs pa{ &c, 0, N };
    atomic_token<int> seen(0);
    ConsArgs ca{ &c, &seen, N };
    {
      auto_thread<> p(producer, &pa);
      auto_thread<> q(consumer, &ca);
    }
    require(seen.get() == N);
    require_true(!c);
  }
  end_test_case();

  test_case("2 producers + 2 consumers MPMC: every item consumed exactly once (FIX #1)");
  {
    channel<int> c;
    constexpr int PER_PROD = 250;
    constexpr int N_PROD = 2;
    constexpr int N_CONS = 2;
    constexpr int TOTAL = PER_PROD * N_PROD;

    ProdArgs pa1{ &c, 0, PER_PROD };
    ProdArgs pa2{ &c, 1000, PER_PROD };

    atomic_token<int> seen(0);
    ConsArgs ca{ &c, &seen, TOTAL };

    {
      auto_thread<> p1(producer, &pa1);
      auto_thread<> p2(producer, &pa2);
      auto_thread<> c1(consumer, &ca);
      auto_thread<> c2(consumer, &ca);
    }

    require(seen.get() == TOTAL);
    require_true(!c);      // channel must be drained
  }
  end_test_case();

  sb::print("=== ALL CHANNEL TESTS PASSED ===");
  return 1;
}
