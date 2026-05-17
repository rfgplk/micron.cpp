//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/mutex/locks.hpp"
#include "../../src/mutex/mutex.hpp"

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

struct SLArgs {
  micron::queuing_mutex *m;
  int *counter;
  int iters;
};

void
sl_worker(SLArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    micron::scoped_lock g(*p->m);
    ++(*p->counter);
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== SCOPED_LOCK TESTS ===");

  test_case("scoped_lock(qmutex&) ctor acquires");
  {
    queuing_mutex m;
    {
      scoped_lock g(m);
      require_true(g.owns());
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("default ctor + acquire(qm) usage");
  {
    queuing_mutex m;
    {
      scoped_lock g;
      require_false(g.owns());
      g.acquire(m);
      require_true(g.owns());
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_acquire(qm) when free succeeds");
  {
    queuing_mutex m;
    {
      scoped_lock g;
      require_true(g.try_acquire(m));
      require_true(g.owns());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("manual release() inside scope; dtor does not double-release");
  {
    queuing_mutex m;
    {
      scoped_lock g(m);
      require_true(g.owns());
      g.release();
      require_false(g.owns());
      require_false(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("scoped_lock is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<scoped_lock>);
    static_assert(!is_move_constructible_v<scoped_lock>);
    require_true(true);
  }
  end_test_case();

  test_case("8 threads on shared counter via scoped_lock");
  {
    queuing_mutex m;
    int counter = 0;
    constexpr int kIters = 5000;
    SLArgs a{ &m, &counter, kIters };
    {
      auto_thread<> t1(sl_worker, &a);
      auto_thread<> t2(sl_worker, &a);
      auto_thread<> t3(sl_worker, &a);
      auto_thread<> t4(sl_worker, &a);
      auto_thread<> t5(sl_worker, &a);
      auto_thread<> t6(sl_worker, &a);
      auto_thread<> t7(sl_worker, &a);
      auto_thread<> t8(sl_worker, &a);
    }
    require(counter == 8 * kIters);
  }
  end_test_case();

  sb::print("=== ALL SCOPED_LOCK TESTS PASSED ===");
  return 1;
}
