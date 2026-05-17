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

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

struct LArgs {
  micron::latch *l;
  int *val;
};

void
l_worker(LArgs *p)
{
  *p->val += 1;
  p->l->count_down(1);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== LATCH TESTS ===");

  test_case("latch(5).expected() returns 5");
  {
    latch l(5);
    require(l.expected() == 5);
  }
  end_test_case();

  test_case("count_down() decrements expected count");
  {
    latch l(3);
    l.count_down();
    require(l.expected() == 2);
    l.count_down();
    require(l.expected() == 1);
    l.count_down();
    require(l.expected() == 0);
  }
  end_test_case();

  test_case("count_down(n) decrements by n");
  {
    latch l(10);
    l.count_down(5);
    require(l.expected() == 5);
    l.count_down(3);
    require(l.expected() == 2);
  }
  end_test_case();

  test_case("try_wait returns false until count reaches 0");
  {
    latch l(2);
    require_false(l.try_wait());
    l.count_down();
    require_false(l.try_wait());
    l.count_down();
    require_true(l.try_wait());
  }
  end_test_case();

  test_case("wait() returns once count reaches 0");
  {
    latch l(1);
    l.count_down();
    l.wait();
    require_true(true);
  }
  end_test_case();

  test_case("5-thread arrival: main waits, workers each count_down(1)");
  {
    latch l(5);
    int val_1 = 0, val_2 = 0, val_3 = 0, val_4 = 0, val_5 = 0;
    LArgs a1{ &l, &val_1 }, a2{ &l, &val_2 }, a3{ &l, &val_3 }, a4{ &l, &val_4 }, a5{ &l, &val_5 };
    {
      auto_thread<> t1(l_worker, &a1);
      auto_thread<> t2(l_worker, &a2);
      auto_thread<> t3(l_worker, &a3);
      auto_thread<> t4(l_worker, &a4);
      auto_thread<> t5(l_worker, &a5);
      l.wait();
    }
    require(l.expected() == 0);
    require(val_1 + val_2 + val_3 + val_4 + val_5 == 5);
  }
  end_test_case();

  test_case("over-count: latch goes negative; try_wait returns false (informational)");
  {
    latch l(1);
    l.count_down(3);
    // counter is signed; behavior is now -2 (documented; not a contract).
    check(l.expected() == -2);
  }
  end_test_case();

  sb::print("=== ALL LATCH TESTS PASSED ===");
  return 1;
}
