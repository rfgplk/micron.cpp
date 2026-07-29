//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reschedule_fair() on a ONE-WORKER engine.
//
// reschedule() is a LIFO self-push onto the worker's own deque, and engine::__find pops the deque
// bottom before it looks anywhere else -- so a frame looping on it hands itself straight back and
// starves everything else that worker owns. with one worker there is no thief and no escape:
// swapping reschedule_fair() for reschedule() below hangs until the --timeout kills it.
//
// this is the guard for the three EAGAIN/ENOBUFS backoff loops in io/coroutine/__acore.hpp.

#include "../../src/coroio.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;

static int FAILS = 0;

static micron::atomic_token<u32> g_peer_done{ 0 };
static micron::atomic_token<u32> g_spins{ 0 };

// spins on a fair yield until the peer it shares the worker with has finished
static micron::task<void>
yielder(u32 cap)
{
  while ( g_peer_done.get(micron::memory_order_acquire) == 0 ) {
    if ( g_spins.fetch_add(1, micron::memory_order_acq_rel) > cap ) break;      // bail so a regression fails rather than hangs
    co_await coro::reschedule_fair();
  }
}

static micron::task<void>
peer()
{
  g_peer_done.store(1, micron::memory_order_release);
  co_return;
}

static micron::task<void>
root(u32 cap)
{
  co_await coro::fork(coro::discard, yielder)(cap);
  co_await coro::fork(coro::discard, peer)();
  co_await coro::join;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime(1);      // ONE worker: the case reschedule() cannot escape

  sb::test_case("reschedule_fair: a yield loop does not starve its own worker");
  {
    const u32 cap = 200000;
    g_peer_done.store(0, micron::memory_order_relaxed);
    g_spins.store(0, micron::memory_order_relaxed);
    coro::sync_wait(root(cap));
    sb::check(g_peer_done.get(micron::memory_order_acquire) == 1u);
    sb::check(g_spins.get(micron::memory_order_acquire) <= cap);      // did not bail out
  }
  sb::end_test_case();

  sb::test_case("reschedule_fair: io completes while a frame yields");
  {
    if ( !micron::io::coro::available() ) {
      sb::print("no live ring, skipping");
    } else {
      // a file read must complete even though a sibling frame is hammering the yield path on the
      // same (single) worker -- reschedule_fair pumps the reactor on its way out
      g_peer_done.store(0, micron::memory_order_relaxed);
      g_spins.store(0, micron::memory_order_relaxed);

      static max_t rd = 0;
      auto reader = []() -> micron::task<void> {
        micron::buffer out(0);
        rd = co_await micron::io::coro::read_file("/proc/self/cmdline", out);
        g_peer_done.store(1, micron::memory_order_release);
      };
      auto both = [](u32 c, auto rf) -> micron::task<void> {
        co_await coro::fork(coro::discard, yielder)(c);
        co_await coro::fork(coro::discard, rf)();
        co_await coro::join;
      };
      coro::sync_wait(both(200000u, reader));
      sb::check(g_peer_done.get(micron::memory_order_acquire) == 1u);
      sb::check(rd >= 0);
    }
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  if ( FAILS != 0 ) return 0;
  sb::print("=== ALL RESCHEDULE_FAIR TESTS PASSED ===");
  return 1;
}
