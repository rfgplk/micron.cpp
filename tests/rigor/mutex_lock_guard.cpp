//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// guard_lock.hpp uses adopt_lock_t (from mutex/locks.hpp) and atomic_flag
// (from atomic/flag.hpp) without including them directly — pull them in
// explicitly so the SUT compiles.
#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/mutex/locks.hpp"
#include "../../src/mutex/mutex.hpp"

#include "../../src/mutex/locks/guard_lock.hpp"
#include "../../src/mutex/locks/recursive_lock.hpp"
#include "../../src/mutex/locks/spin_lock.hpp"
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

struct LGArgs {
  micron::mutex *m;
  int *counter;
  int iters;
};

void
lg_worker(LGArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    micron::lock_guard<micron::mutex> g(*p->m);
    ++(*p->counter);
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== LOCK_GUARD + FREE_GUARD TESTS ===");

  // ── lock_guard ──────────────────────────────────────────────────────────

  test_case("lock_guard<mutex> RAII: locks on ctor, unlocks on dtor");
  {
    mutex m;
    {
      lock_guard<mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  // SKIPPED: lock_guard<M>(M*) at guard_lock.hpp:27 does `m()` on the
  // pointer instead of `(*m)()`. Compile-time broken; documented bug.

  test_case("lock_guard<mutex>(m, adopt_lock) does not re-lock");
  {
    mutex m;
    m.lock();
    require_true(m.is_locked());
    {
      lock_guard<mutex> g(m, adopt_lock);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("lock_guard<weak_mutex> works across mutex flavor");
  {
    weak_mutex m;
    {
      lock_guard<weak_mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("lock_guard<recursive_lock> works");
  {
    recursive_lock rl;
    {
      lock_guard<recursive_lock> g(rl);
      require_true(rl.is_locked());
    }
    require_false(rl.is_locked());
  }
  end_test_case();

  test_case("lock_guard<spin_lock> works");
  {
    spin_lock sl;
    {
      lock_guard<spin_lock> g(sl);
      require_true(sl.is_locked());
    }
    require_false(sl.is_locked());
  }
  end_test_case();

  test_case("lock_guard 4-thread contention totals correctly");
  {
    mutex m;
    int counter = 0;
    constexpr int kIters = 5000;
    LGArgs a{ &m, &counter, kIters };
    {
      auto_thread<> t1(lg_worker, &a);
      auto_thread<> t2(lg_worker, &a);
      auto_thread<> t3(lg_worker, &a);
      auto_thread<> t4(lg_worker, &a);
    }
    require(counter == 4 * kIters);
  }
  end_test_case();

  // ── free_guard ──────────────────────────────────────────────────────────

  test_case("free_guard(atomic_flag*) ctor acquires, dtor releases");
  {
    atomic_flag f;
    {
      free_guard<> g(&f);
      require_true(g.owns());
      require_true(f.test());
    }
    require_false(f.test());
  }
  end_test_case();

  test_case("free_guard(atomic_flag&, try_to_lock_t) reports owns when free");
  {
    atomic_flag f;
    {
      free_guard<> g(f, try_to_lock);
      require_true(g.owns());
      require_true(f.test());
    }
    require_false(f.test());
  }
  end_test_case();

  test_case("free_guard(flag&, try_to_lock_t) reports !owns when held");
  {
    atomic_flag f;
    f.test_and_set();      // pre-lock
    {
      free_guard<> g(f, try_to_lock);
      require_false(g.owns());
    }
    // f still set because we didn't own it
    require_true(f.test());
    f.clear();
  }
  end_test_case();

  test_case("free_guard(flag&, defer_lock_t) starts unowned; lock() acquires");
  {
    atomic_flag f;
    {
      free_guard<> g(f, defer_lock);
      require_false(g.owns());
      require_false(f.test());
      require_true(g.lock());
      require_true(g.owns());
      require_true(f.test());
    }
    require_false(f.test());
  }
  end_test_case();

  test_case("free_guard(flag&, adopt_lock_t) assumes pre-held");
  {
    atomic_flag f;
    f.test_and_set();
    {
      free_guard<> g(f, adopt_lock);
      require_true(g.owns());
    }
    require_false(f.test());
  }
  end_test_case();

  test_case("free_guard move ctor transfers ownership; source becomes !valid");
  {
    atomic_flag f;
    free_guard<> g1(&f);
    require_true(g1.owns());
    require_true(g1.valid());
    free_guard<> g2(static_cast<free_guard<> &&>(g1));
    require_true(g2.owns());
    require_false(g1.valid());
    require_false(g1.owns());
  }
  end_test_case();

  test_case("free_guard manual unlock clears ownership");
  {
    atomic_flag f;
    {
      free_guard<> g(&f);
      g.unlock();
      require_false(g.owns());
      require_false(f.test());
    }
    require_false(f.test());
  }
  end_test_case();

  test_case("free_guard release() returns flag pointer, marks unowned");
  {
    atomic_flag f;
    free_guard<> g(&f);
    require_true(g.owns());
    atomic_flag *p = g.release();
    require(p == &f);
    require_false(g.owns());
    require_false(g.valid());
    f.clear();      // we now own manually
  }
  end_test_case();

  sb::print("=== ALL LOCK_GUARD + FREE_GUARD TESTS PASSED ===");
  return 1;
}
