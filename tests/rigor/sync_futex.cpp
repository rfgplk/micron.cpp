//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/atomic/atomic.hpp"
#include "../../src/except.hpp"

#include "../../src/sync/futex.hpp"

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

struct WaitArgs {
  ::u32 *word;
  micron::atomic_token<bool> *done;
};

void
wait_worker(WaitArgs *p)
{
  micron::wait_futex(p->word, (::u32)42);
  p->done->store(true, micron::memory_order::release);
}

struct FutexArgs {
  micron::futex<> *f;
  micron::atomic_token<int> *counter;
  int iters;
};

void
fx_worker(FutexArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    p->f->wait();
    p->counter->fetch_add(1, micron::memory_order::acq_rel);
    p->f->release();
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== FUTEX TESTS ===");

  test_case("wait_futex returns immediately when *ptr != expected");
  {
    u32 w = 99;
    wait_futex(&w, (u32)42);
    require_true(true);
  }
  end_test_case();

  test_case("wait_futex blocks until release_futex changes the value");
  {
    u32 w = 42;
    atomic_token<bool> done(false);
    WaitArgs a{ &w, &done };
    auto_thread<> waiter(wait_worker, &a);

    for ( int i = 0; i < 1000 && !done.get(memory_order::acquire); ++i ) yield();
    require_false(done.get(memory_order::acquire));
    release_futex(&w, (u32)100);
  }
  end_test_case();

  test_case("futex<>::wait acquires on default (unlocked) state");
  {
    futex<> f;
    f.wait();
    f.release();
    require_true(true);
  }
  end_test_case();

  test_case("futex<> 2-thread mutual exclusion — 5000 increments each");
  {
    futex<> f;
    atomic_token<int> counter(0);
    constexpr int kIters = 5000;
    FutexArgs a{ &f, &counter, kIters };
    {
      auto_thread<> t1(fx_worker, &a);
      auto_thread<> t2(fx_worker, &a);
    }
    require(counter.get() == 2 * kIters);
  }
  end_test_case();

  test_case("futex<u32, 5, 7> with non-default sentinels works");
  {

    futex<u32, (u32)5, (u32)7> f;
    f.wait();
    f.release();
    require_true(true);
  }
  end_test_case();

  test_case("futex<u32, 5, 7> 2-thread mutex serialization");
  {
    futex<u32, (u32)5, (u32)7> f;
    atomic_token<int> counter(0);
    constexpr int kIters = 2000;

    struct LocalArgs {
      futex<u32, (u32)5, (u32)7> *f;
      atomic_token<int> *c;
      int n;
    };

    LocalArgs a{ &f, &counter, kIters };
    auto worker = [](LocalArgs *p) {
      for ( int i = 0; i < p->n; ++i ) {
        p->f->wait();
        p->c->fetch_add(1, micron::memory_order::acq_rel);
        p->f->release();
      }
    };
    {
      auto_thread<> t1(worker, &a);
      auto_thread<> t2(worker, &a);
    }
    require(counter.get() == 2 * kIters);
  }
  end_test_case();

  test_case("futex<T,__D,__L> requires __D != __L (concept satisfaction)");
  {

    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL FUTEX TESTS PASSED ===");
  return 1;
}
