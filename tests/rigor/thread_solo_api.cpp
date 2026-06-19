//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/pause.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"
#include "../../src/thread/thread_types/reg_thread.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{
int
ret_int(int n)
{
  return n * 5;
}
}      // namespace

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== SOLO API RIGOR ===");

  test_case("spawn + result + join clears the handle");
  {
    auto t = solo::spawn<auto_thread<>>(ret_int, 8);
    int got = t->result<int>();
    require(got, 40);
    solo::join(t);
    require(!is_alive_ptr(t), true);
  }
  end_test_case();

  test_case("spawn<thread<>> + wait_for + dismiss clears the handle");
  {
    auto t = solo::spawn<thread<>>(ret_int, 3);
    solo::wait_for(t);
    solo::dismiss(t);
    require(!is_alive_ptr(t), true);
  }
  end_test_case();

  test_case("try_join eventually reaps a finished worker and clears the handle");
  {
    auto t = solo::spawn<auto_thread<>>(ret_int, 1);
    solo::wait_for(t);

    bool cleared = micron::until_timeout(2000.0, [&t]() { return solo::try_join(t) == 1; });
    require(cleared, true);
    require(!is_alive_ptr(t), true);
  }
  end_test_case();

  test_case("is_joinable reflects a live worker");
  {
    atomic_token<bool> stop{ false };
    auto t = solo::spawn<auto_thread<>>([&stop]() {
      while ( !stop.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    require(solo::is_joinable(t), true);
    stop.store(true, memory_order_release);
    solo::join(t);
  }
  end_test_case();

  test_case("yield(handle) is a safe no-op style call on a live worker");
  {
    atomic_token<bool> stop{ false };
    auto t = solo::spawn<auto_thread<>>([&stop]() {
      while ( !stop.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    solo::yield(t);
    stop.store(true, memory_order_release);
    solo::join(t);
    require(true);
  }
  end_test_case();

  test_case("solo::sleep/awaken park and resume a single worker");
  {
    atomic_token<bool> stop{ false };
    atomic_token<u64> ticks{ 0 };
    auto t = solo::spawn<auto_thread<>>([&stop, &ticks]() {
      while ( !stop.get(memory_order_acquire) ) {
        ticks.fetch_add(1, memory_order_acq_rel);
        micron::sleep(1);
      }
    });
    micron::ssleep(1);
    solo::sleep(t);
    micron::sleep(50);
    u64 a = ticks.get(memory_order_acquire);
    micron::sleep(80);
    require(a == ticks.get(memory_order_acquire), true);
    solo::awaken(t);
    micron::sleep(80);
    require(ticks.get(memory_order_acquire) > a, true);
    stop.store(true, memory_order_release);
    solo::join(t);
  }
  end_test_case();

  test_case("solo::terminate self-reaps; dismiss then clears the handle");
  {
    atomic_token<bool> never{ false };
    auto t = solo::spawn<auto_thread<>>([&never]() {
      while ( !never.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    solo::terminate(t);
    require(is_alive_ptr(t), true);
    solo::dismiss(t);
    require(!is_alive_ptr(t), true);
  }
  end_test_case();

  test_case("solo::force_stop self-reaps a runaway worker");
  {
    atomic_token<bool> never{ false };
    auto t = solo::spawn<thread<>>([&never]() {
      while ( !never.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    solo::force_stop(t);
    solo::dismiss(t);
    require(!is_alive_ptr(t), true);
  }
  end_test_case();

  test_case("solo::kill (cancel) stops a worker at an explicit cancellation point");
  {
    atomic_token<bool> never{ false };
    auto t = solo::spawn<auto_thread<>>([&never]() {
      while ( !never.get(memory_order_acquire) ) {
        micron::pthread::cancel();
        micron::sleep(1);
      }
    });
    micron::ssleep(1);
    solo::kill(t);
    solo::dismiss(t);
    require(!is_alive_ptr(t), true);
  }
  end_test_case();

  test_case("variadic join reaps several handles");
  {
    auto a = solo::spawn<auto_thread<>>(ret_int, 1);
    auto b = solo::spawn<auto_thread<>>(ret_int, 2);
    auto c = solo::spawn<auto_thread<>>(ret_int, 3);
    solo::join(a, b, c);
    require(!is_alive_ptr(a) && !is_alive_ptr(b) && !is_alive_ptr(c), true);
  }
  end_test_case();

  sb::print("=== ALL SOLO API TESTS PASSED ===");
  return 1;
}
