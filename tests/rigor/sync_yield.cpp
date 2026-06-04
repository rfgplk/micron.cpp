//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/chrono.hpp"
#include "../../src/sync/pause.hpp"
#include "../../src/sync/yield.hpp"

#include "../../src/std.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/linux/process/signals.hpp"
#include "../../src/linux/sys/system.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

struct PauseSigArgs {
  micron::posix::pid_t target_tid;       // the thread that is inside pause()
  micron::atomic_token<int> *armed;      // set once pause() is known to be waiting
};

// helper: wait until the main thread reports it is about to wait, then deliver SIGCONT to it
// specifically (tgkill → unambiguous target). Repeats so a single dropped delivery can't hang the
// test; with the sigprocmask-block fix one delivery is enough.
void
pause_signaller(PauseSigArgs *p)
{
  while ( p->armed->get(micron::memory_order::acquire) == 0 ) micron::yield();
  for ( int i = 0; i < 200; ++i ) {
    micron::posix::tgkill(micron::posix::getpid(), p->target_tid, micron::posix::sig_cont);
    micron::sleep_for(5);
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== YIELD / PAUSE / SLEEP TESTS ===");

  test_case("yield() returns control without crash");
  {
    for ( int i = 0; i < 1000; ++i ) yield();
    require_true(true);
  }
  end_test_case();

  test_case("cpu_pause<1000>() returns (smoke)");
  {
    cpu_pause<1000>();      // ~1us
    require_true(true);
  }
  end_test_case();

  test_case("sleep_for(10) sleeps for at least 5ms (loose)");
  {
    auto t0 = system_clock<>::now();
    sleep_for(10);
    auto t1 = system_clock<>::now();
    auto dt = t1 - t0;
    // loose: at least 5ms — clock granularity tolerance
    check(dt >= (fduration_t)5);
  }
  end_test_case();

  test_case("sleep_nano(1_000_000) returns (smoke)");
  {
    sleep_nano(1000000);
    require_true(true);
  }
  end_test_case();

  // Fix: pause(F, args...) now sigprocmask-BLOCKs SIGCONT before sigwait()ing (like the no-arg
  // pause()/await()); previously an unblocked SIGCONT could be taken by default disposition and
  // never dequeued by sigwait → the callback would never run / the wait would hang. The callback
  // also only runs when sigwait actually succeeds, and carries the real signal number.
  test_case("pause(callback, arg) blocks+waits for SIGCONT, then invokes callback with the signal");
  {
    atomic_token<int> got_sig(-1);
    atomic_token<int> armed(0);
    PauseSigArgs sa{ posix::gettid(), &armed };
    {
      auto_thread<> helper(pause_signaller, &sa);
      armed.store(1, memory_order::release);      // tell the helper we are about to wait
      int marker = 1234;
      // pause() blocks SIGCONT, waits, then calls our lambda(sig, marker). Must return (no hang)
      // and hand us a real signal number plus the forwarded arg.
      pause(
          [&](int sig, int m) {
            if ( m == 1234 ) got_sig.store(sig, memory_order::release);
          },
          marker);
    }
    require(got_sig.get(memory_order::acquire) == posix::sig_cont);
  }
  end_test_case();

  sb::print("=== ALL YIELD/PAUSE/SLEEP TESTS PASSED ===");
  return 1;
}
