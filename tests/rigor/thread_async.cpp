//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/pause.hpp"
#include "../../src/sync/until.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/async_thread.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{
pthread_attr_t
make_attrs(void)
{
  auto a = micron::pthread::prepare_thread(micron::pthread::thread_create_state::joinable, micron::posix::sched_other, 0);
  auto *stack = micron::addrmap(micron::thread_stack_size);
  snowball::require(!micron::mmap_failed(stack), true);
  micron::pthread::set_stack_thread(a, stack, micron::thread_stack_size);
  return a;
}

template<typename T>
concept has_sleep = requires(T &t) { t.sleep(); };
template<typename T>
concept has_awaken = requires(T &t) { t.awaken(); };
template<typename T>
concept has_cancel = requires(T &t) { t.cancel(); };
template<typename T>
concept has_alive = requires(T &t) { t.alive(); };
}      // namespace

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== ASYNC_THREAD RIGOR ===");

  test_case("worker control surface (sleep/awaken/cancel/alive) is =deleted");
  {
    static_assert(!has_sleep<async_thread<>>, "async_thread::sleep must be deleted");
    static_assert(!has_awaken<async_thread<>>, "async_thread::awaken must be deleted");
    static_assert(!has_cancel<async_thread<>>, "async_thread::cancel must be deleted");
    static_assert(!has_alive<async_thread<>>, "async_thread::alive must be deleted");
    require(true);
  }
  end_test_case();

  test_case("operator[] enqueues work that the worker executes");
  {
    atomic_token<u32> done{ 0 };
    async_thread<> w(make_attrs());
    auto job = [&done]() { done.fetch_add(1, memory_order_acq_rel); };
    w[job];
    bool ok = micron::until_timeout(2000.0, [&done]() { return done.get(memory_order_acquire) == 1u; });
    require(ok, true);
    w.join();
  }
  end_test_case();

  test_case("work-on-construct ctor runs the initial job");
  {
    atomic_token<u32> done{ 0 };
    auto job = [&done]() { done.fetch_add(7, memory_order_acq_rel); };
    async_thread<> w(make_attrs(), job);
    bool ok = micron::until_timeout(2000.0, [&done]() { return done.get(memory_order_acquire) == 7u; });
    require(ok, true);
    w.join();
  }
  end_test_case();

  test_case("many sequential jobs all run exactly once");
  {
    atomic_token<u64> sum{ 0 };
    async_thread<> w(make_attrs());
    constexpr u64 N = 200;
    for ( u64 i = 1; i <= N; ++i ) {
      auto job = [&sum, i]() { sum.fetch_add(i, memory_order_acq_rel); };
      w[job];
    }
    const u64 expect = N * (N + 1) / 2;
    bool ok = micron::until_timeout(4000.0, [&sum, expect]() { return sum.get(memory_order_acquire) == expect; });
    require(ok, true);
    w.join();
  }
  end_test_case();

  test_case("get_status()/is_working() and stats() are callable without tearing/crash");
  {
    async_thread<> w(make_attrs());
    atomic_token<bool> ran{ false };
    auto job = [&ran]() { ran.store(true, memory_order_release); };
    w[job];
    micron::until_timeout(2000.0, [&ran]() { return ran.get(memory_order_acquire); });
    (void)w.is_working();
    (void)w.get_status();
    posix::rusage_t u = w.stats();
    (void)u;
    w.join();
    require(true);
  }
  end_test_case();

  test_case("stop() then join() tears down a quiescent worker");
  {
    async_thread<> w(make_attrs());
    w.stop();
    int rc = w.join();
    (void)rc;
    require(true);
  }
  end_test_case();

  sb::print("=== ALL ASYNC_THREAD TESTS PASSED ===");
  return 1;
}
