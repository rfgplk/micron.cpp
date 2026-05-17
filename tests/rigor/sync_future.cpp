//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// future.hpp uses fduration_t / system_clock (from chrono.hpp) and lock_guard
// (from mutex/locks.hpp) without including them — pull both in first.
#include "../../src/atomic/atomic.hpp"
#include "../../src/chrono.hpp"
#include "../../src/mutex/locks.hpp"

#include "../../src/sync/future.hpp"
#include "../../src/sync/promises.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace
{

struct ProducerArgs {
  micron::promise<int> *p;
  int v;
};

void
producer_int(ProducerArgs *p)
{
  p->p->set_value(p->v);
}

struct VoidProducerArgs {
  micron::promise<void> *p;
};

void
producer_void(VoidProducerArgs *p)
{
  p->p->set_value();
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== FUTURE<T> TESTS ===");

  test_case("future<int> default ctor: !valid()");
  {
    future<int> f;
    require_false(f.valid());
  }
  end_test_case();

  test_case("future<int> from promise.get_future() is valid()");
  {
    promise<int> p;
    auto f = p.get_future();
    require_true(f.valid());
  }
  end_test_case();

  test_case("future<int>::get() returns set value");
  {
    promise<int> p;
    auto f = p.get_future();
    p.set_value(42);
    require(f.get() == 42);
  }
  end_test_case();

  test_case("future<int>::get() second call throws future_error");
  {
    promise<int> p;
    auto f = p.get_future();
    p.set_value(7);
    (void)f.get();
    require_throw([&]() { f.get(); });
  }
  end_test_case();

  test_case("future<int>::wait() returns after promise.set_value() from another thread");
  {
    promise<int> p;
    auto f = p.get_future();
    ProducerArgs a{ &p, 99 };
    {
      auto_thread<> producer(producer_int, &a);
    }
    f.wait();
    require(f.get() == 99);
  }
  end_test_case();

  test_case("future<int>::wait_for(0ns) returns timeout when not ready");
  {
    promise<int> p;
    auto f = p.get_future();
    require(f.wait_for(fduration_t(0)) == future_status::timeout);
  }
  end_test_case();

  test_case("future<int>::wait_for returns ready once set");
  {
    promise<int> p;
    auto f = p.get_future();
    p.set_value(1);
    require(f.wait_for(fduration_t(0)) == future_status::ready);
  }
  end_test_case();

  test_case("future<int> move ctor transfers state; source becomes !valid");
  {
    promise<int> p;
    auto f = p.get_future();
    require_true(f.valid());
    future<int> f2(static_cast<future<int> &&>(f));
    require_false(f.valid());
    require_true(f2.valid());
    p.set_value(5);
    require(f2.get() == 5);
  }
  end_test_case();

  test_case("future<int>::get on default-constructed throws future_error");
  {
    future<int> f;
    require_throw([&]() { f.get(); });
  }
  end_test_case();

  // ── future<void> ────────────────────────────────────────────────────────

  test_case("future<void>: set_value then get() completes; second get throws");
  {
    promise<void> p;
    auto f = p.get_future();
    p.set_value();
    f.get();
    require_throw([&]() { f.get(); });
  }
  end_test_case();

  test_case("future<void> threaded wait");
  {
    promise<void> p;
    auto f = p.get_future();
    VoidProducerArgs a{ &p };
    {
      auto_thread<> producer(producer_void, &a);
    }
    f.wait();
    f.get();
    require_true(true);
  }
  end_test_case();

  // ── future<T&> ──────────────────────────────────────────────────────────

  test_case("future<T&>: set_value(ref) and get() returns same address");
  {
    promise<int &> p;
    auto f = p.get_future();
    int v = 17;
    p.set_value(v);
    int &out = f.get();
    require(&out == &v);
    require(out == 17);
  }
  end_test_case();

  sb::print("=== ALL FUTURE TESTS PASSED ===");
  return 1;
}
