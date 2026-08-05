//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: the atexit table's claim/publish handshake.
//
// The broken scheme claimed an index by LOWERING __atexit_count and only then read the entry, so a
// concurrent __push (any function-local static ctor on any thread) reserved the SAME index and
// overwrote it underneath the drainer. Three distinct failures fell out of that one race:
//
//   1. the drainer paired the OLD func with the NEW arg -- a destructor run on an unrelated object
//   2. it then nulled the new registration's func, dropping that handler entirely
//   3. the count was back where it started, so it re-claimed the same index and spun forever on
//      `while (f == nullptr)` -- a process that never exits (graded 124, not 6)
//
// Every registration here encodes its expected callee in its own arg, so (1) is caught directly
// rather than inferred. (2) is caught by the exactly-once tally, (3) by the fact that this test
// terminates at all.

#define MICRON_ABC_MT 1

#include "../../src/atomic/atomic.hpp"
#include "../../src/exit.hpp"
#include "../../src/sync/yield.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace
{

constexpr u32 __pushers = 6;
constexpr u32 __per_pusher = 240;
constexpr u32 __total = __pushers * __per_pusher;

// arg is the registration id; its low bit says which of the two handlers must receive it
micron::atomic_token<u32> g_seen[__total]{};
micron::atomic_token<u32> g_ran{ 0 };
micron::atomic_token<u32> g_mispaired{ 0 };      // func/arg pairing broken: THE bug
micron::atomic_token<u32> g_gate{ 0 };

inline void
record(void *p, u32 want_parity) noexcept
{
  const u64 id = reinterpret_cast<u64>(p);
  if ( id >= __total ) {      // an arg that was never registered at all
    g_mispaired.fetch_add(1, micron::memory_order_acq_rel);
    return;
  }
  if ( (id & 1u) != want_parity ) g_mispaired.fetch_add(1, micron::memory_order_acq_rel);
  g_seen[id].fetch_add(1, micron::memory_order_acq_rel);
  g_ran.fetch_add(1, micron::memory_order_acq_rel);
}

void
handler_even(void *p) noexcept
{
  record(p, 0u);
}

void
handler_odd(void *p) noexcept
{
  record(p, 1u);
}

}      // namespace

int
main(int, char **)
{
  sb::print("=== ATEXIT CLAIM/PUBLISH RIGOR ===");

  test_case("a drain concurrent with registrations neither mispairs, drops, nor hangs");
  {
    // one thread drains in a loop while the others register; the drain must terminate every time
    mtest::parallel(static_cast<int>(__pushers) + 1, [](int w) {
      if ( w == 0 ) {
        while ( g_gate.get(micron::memory_order_acquire) == 0 ) {
          micron::__drain_atexit_table();
          micron::yield();
        }
        micron::__drain_atexit_table();
        return;
      }
      const u32 base = static_cast<u32>(w - 1) * __per_pusher;
      for ( u32 i = 0; i < __per_pusher; ++i ) {
        const u64 id = base + i;
        (void)micron::__exit_internal::__push((id & 1u) ? &handler_odd : &handler_even, reinterpret_cast<void *>(id));
        if ( (i & 15u) == 15u ) micron::yield();
      }
      if ( static_cast<u32>(w) == __pushers ) g_gate.store(1, micron::memory_order_release);
    });

    micron::__drain_atexit_table();      // sweep up anything the racing drainer missed

    require(g_mispaired.get(micron::memory_order_acquire) == 0u);

    u32 ran = 0, twice = 0, never = 0;
    for ( u32 i = 0; i < __total; ++i ) {
      const u32 n = g_seen[i].get(micron::memory_order_acquire);
      if ( n == 0 )
        ++never;
      else if ( n > 1 )
        ++twice;
      else
        ++ran;
    }
    sb::print("registered ", __total, ", ran ", ran, ", dropped ", never, ", doubled ", twice, ", dropped-by-drain ",
              micron::__exit_internal::__atexit_dropped);

    require(twice == 0u);      // exactly-once
    require(never == 0u);      // nothing silently swallowed
    require(ran == __total);
    require(micron::__exit_internal::__atexit_dropped == 0u);
  }
  end_test_case();

  test_case("a handler that registers another handler still gets drained");
  {
    static micron::atomic_token<u32> nested{ 0 };
    struct chain {
      static void
      inner(void *) noexcept
      {
        nested.fetch_add(1, micron::memory_order_acq_rel);
      }
      static void
      outer(void *) noexcept
      {
        nested.fetch_add(1, micron::memory_order_acq_rel);
        (void)micron::__exit_internal::__push(&chain::inner, nullptr);
      }
    };
    (void)micron::__exit_internal::__push(&chain::outer, nullptr);
    micron::__drain_atexit_table();
    require(nested.get(micron::memory_order_acquire) == 2u);
  }
  end_test_case();

  test_case("the allocation cursor never moves backwards");
  {
    // reuse is what made the pairing unsafe; the cursor is monotonic on purpose
    const u32 before = micron::__exit_internal::__atexit_count;
    (void)micron::__exit_internal::__push(&handler_even, reinterpret_cast<void *>(u64{ 0 }));
    const u32 mid = micron::__exit_internal::__atexit_count;
    micron::__drain_atexit_table();
    const u32 after = micron::__exit_internal::__atexit_count;
    require(mid > before);
    require(after >= mid);
  }
  end_test_case();

  sb::print("=== ATEXIT CLAIM/PUBLISH PASSED ===");
  return 1;
}
