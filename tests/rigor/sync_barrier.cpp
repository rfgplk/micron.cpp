//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/latch.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

struct BArgs {
  micron::barrier *b;
  micron::atomic_token<int> *counter;
  int generations;
};

void
b_worker(BArgs *p)
{
  for ( int g = 0; g < p->generations; ++g ) {
    p->b->arrive_and_wait();
    p->counter->fetch_add(1, micron::memory_order::acq_rel);
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== BARRIER TESTS ===");

  test_case("barrier(N).expected() returns N initially");
  {
    barrier b(4);
    require(b.expected() == 4);
  }
  end_test_case();

  test_case("single-thread barrier(1).arrive_and_wait passes through; counter restored");
  {
    barrier b(1);
    int rv = b.arrive_and_wait();
    require(rv == 0);
    require(b.expected() == 1);
  }
  end_test_case();

  test_case("4-thread single-generation barrier — all threads cleared past");
  {
    barrier b(4);
    atomic_token<int> ctr(0);
    BArgs a{ &b, &ctr, 1 };
    {
      auto_thread<> t1(b_worker, &a);
      auto_thread<> t2(b_worker, &a);
      auto_thread<> t3(b_worker, &a);
      auto_thread<> t4(b_worker, &a);
    }
    require(ctr.get() == 4);
    require(b.expected() == 4);
  }
  end_test_case();

  test_case("4-thread x 50-generation multi-cycle correctness");
  {
    barrier b(4);
    atomic_token<int> ctr(0);
    constexpr int G = 50;
    BArgs a{ &b, &ctr, G };
    {
      auto_thread<> t1(b_worker, &a);
      auto_thread<> t2(b_worker, &a);
      auto_thread<> t3(b_worker, &a);
      auto_thread<> t4(b_worker, &a);
    }
    require(ctr.get() == 4 * G);
    require(b.expected() == 4);
  }
  end_test_case();

  test_case("8-thread x 200-generation tight loop — sole stress test");
  {
    barrier b(8);
    atomic_token<int> ctr(0);
    constexpr int G = 200;
    BArgs a{ &b, &ctr, G };
    {
      auto_thread<> t1(b_worker, &a);
      auto_thread<> t2(b_worker, &a);
      auto_thread<> t3(b_worker, &a);
      auto_thread<> t4(b_worker, &a);
      auto_thread<> t5(b_worker, &a);
      auto_thread<> t6(b_worker, &a);
      auto_thread<> t7(b_worker, &a);
      auto_thread<> t8(b_worker, &a);
    }
    require(ctr.get() == 8 * G);
    require(b.expected() == 8);
  }
  end_test_case();

  test_case("arrive_and_drop decrements count without waiting");
  {
    barrier b(3);
    require(b.expected() == 3);
    b.arrive_and_drop();
    require(b.expected() == 2);
    b.arrive_and_drop();
    require(b.expected() == 1);
    b.arrive_and_drop();

    require(b.expected() == 3);
  }
  end_test_case();

  sb::print("=== ALL BARRIER TESTS PASSED ===");
  return 1;
}
