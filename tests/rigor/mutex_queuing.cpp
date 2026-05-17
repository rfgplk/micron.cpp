//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Note: lock_guard<queuing_mutex> is intentionally NOT supported — the
// mutex's unlock takes an mcs_node argument and lock_guard expects a no-arg
// reset member function. The supported RAII adapter is `scoped_lock` from
// `mutex/locks/queue_lock.hpp`, which has its own test file.

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
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

struct QArgs {
  micron::queuing_mutex *m;
  int *counter;
  int iters;
};

void
q_worker(QArgs *p)
{
  micron::mcs_node node;
  for ( int i = 0; i < p->iters; ++i ) {
    p->m->lock(node);
    ++(*p->counter);
    p->m->unlock(node);
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== QUEUING_MUTEX (MCS) TESTS ===");

  test_case("default ctor unlocked (tail == nullptr)");
  {
    queuing_mutex m;
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("single-thread lock(node) / unlock(node) roundtrip");
  {
    queuing_mutex m;
    mcs_node n;
    m.lock(n);
    require_true(m.is_locked());
    m.unlock(n);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("operator()(node) returns fn-ptr; dispatch unlocks");
  {
    queuing_mutex m;
    mcs_node n;
    auto rptr = m(n);
    require_true(m.is_locked());
    (m.*rptr)(n);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock(node) when free succeeds");
  {
    queuing_mutex m;
    mcs_node n;
    require_true(m.try_lock(n));
    require_true(m.is_locked());
    m.unlock(n);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock(node) when held by another node fails");
  {
    queuing_mutex m;
    mcs_node n1;
    mcs_node n2;
    m.lock(n1);
    require_false(m.try_lock(n2));
    m.unlock(n1);
    require_true(m.try_lock(n2));
    m.unlock(n2);
  }
  end_test_case();

  test_case("8-thread fairness stress — counter == 8 * iters");
  {
    queuing_mutex m;
    int counter = 0;
    constexpr int kIters = 5000;
    QArgs a{ &m, &counter, kIters };
    {
      auto_thread<> t1(q_worker, &a);
      auto_thread<> t2(q_worker, &a);
      auto_thread<> t3(q_worker, &a);
      auto_thread<> t4(q_worker, &a);
      auto_thread<> t5(q_worker, &a);
      auto_thread<> t6(q_worker, &a);
      auto_thread<> t7(q_worker, &a);
      auto_thread<> t8(q_worker, &a);
    }
    require(counter == 8 * kIters);
  }
  end_test_case();

  test_case("queuing_mutex is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<queuing_mutex>);
    static_assert(!is_move_constructible_v<queuing_mutex>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL QUEUING_MUTEX TESTS PASSED ===");
  return 1;
}
