//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/pause.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/group_thread.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{
constexpr usize GT_STACK = 4ul * 1024 * 1024;
using gthread = micron::group_thread<GT_STACK>;

int
ret_int(int n)
{
  return n * 2;
}

void
ret_void(void)
{
}

micron::thread_attr_t
make_attrs(void)
{
  auto *stack = micron::addrmap(GT_STACK);
  snowball::require(!micron::mmap_failed(stack), true);
  return micron::__thread_attr_with_stack(micron::posix::getpid(), micron::posix::sched_other, stack, GT_STACK);
}
}      // namespace

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== GROUP_THREAD RIGOR ===");

  test_case("type contract: movable, non-copyable");
  {
    static_assert(micron::is_move_constructible_v<group_thread<>>, "group_thread<> must be movable");
    static_assert(!micron::is_copy_constructible_v<group_thread<>>, "group_thread<> must not be copyable");
    require(true);
  }
  end_test_case();

  test_case("bounded-start handshake: ctor returns only after the child reaches its kernel");
  {
    atomic_token<bool> stop{ false };
    gthread t(make_attrs(), [&stop]() {
      while ( !stop.get(memory_order_acquire) ) micron::sleep(1);
    });

    require(t.payload.started.get(memory_order_acquire), true);
    require(t.alive(), true);
    stop.store(true, memory_order_release);
    t.join();
  }
  end_test_case();

  test_case("literal result<int> + join");
  {
    gthread t(make_attrs(), ret_int, 5);
    int got = t.result<int>();
    t.join();
    require(got, 10);
  }
  end_test_case();

  test_case("void worker join + clean destructor (idempotent stack unmap)");
  {
    {
      gthread t(make_attrs(), ret_void);
      int rc = t.join();
      require(rc, 0);
    }
    require(true);
  }
  end_test_case();

  test_case("alive()/active() then reap");
  {
    atomic_token<bool> stop{ false };
    gthread t(make_attrs(), [&stop]() {
      while ( !stop.get(memory_order_acquire) ) micron::sleep(1);
    });
    require(t.alive(), true);
    require(t.active(), true);
    stop.store(true, memory_order_release);
    t.join();
    require(!t.alive(), true);
  }
  end_test_case();

  test_case("sleep()/awaken(): per-thread suspend");
  {
    atomic_token<bool> stop{ false };
    atomic_token<u64> ticks{ 0 };
    gthread t(make_attrs(), [&stop, &ticks]() {
      while ( !stop.get(memory_order_acquire) ) {
        ticks.fetch_add(1, memory_order_acq_rel);
        micron::sleep(1);
      }
    });
    micron::ssleep(1);
    t.sleep();
    micron::sleep(50);
    u64 a = ticks.get(memory_order_acquire);
    micron::sleep(80);
    require(a == ticks.get(memory_order_acquire), true);
    t.awaken();
    micron::sleep(80);
    require(ticks.get(memory_order_acquire) > a, true);
    stop.store(true, memory_order_release);
    t.join();
  }
  end_test_case();

  test_case("terminate(): hard-stop a runaway worker and self-reap");
  {
    atomic_token<bool> never{ false };
    gthread t(make_attrs(), [&never]() {
      while ( !never.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    require(t.alive(), true);
    t.terminate();
    require(!t.alive(), true);
    require(t.try_join(), 0);
  }
  end_test_case();

  test_case("cancel(): unified cancel+join+release at a cancellation point");
  {
    atomic_token<bool> never{ false };
    gthread t(make_attrs(), [&never]() {
      while ( !never.get(memory_order_acquire) ) {
        micron::micthread::cancel();
        micron::sleep(1);
      }
    });
    micron::ssleep(1);
    require(t.alive(), true);
    t.cancel();
    require(!t.alive(), true);
  }
  end_test_case();

  test_case("several group_threads run concurrently and join");
  {
    constexpr int kT = 4;
    atomic_token<u64> c{ 0 };
    auto worker = [&c]() {
      for ( int i = 0; i < 2000; ++i ) c.fetch_add(1, memory_order_acq_rel);
    };
    gthread a(make_attrs(), worker);
    gthread b(make_attrs(), worker);
    gthread d(make_attrs(), worker);
    gthread e(make_attrs(), worker);
    a.join();
    b.join();
    d.join();
    e.join();
    require(c.get(memory_order_acquire), (u64)(kT * 2000));
  }
  end_test_case();

  sb::print("=== ALL GROUP_THREAD TESTS PASSED ===");
  return 1;
}
