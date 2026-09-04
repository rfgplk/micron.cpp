//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// abcmalloc's per-thread arena claim (tapi.hpp).
//
// NOTE: abc::__init_abcmalloc() is gone and abc::__main_arena with it. The API is fully lazy now --
// __current_arena() (tapi.hpp:208) claims a slot on first use, falling back to __claim_arena_slow().
// __boot_abcmalloc() survives only as an empty compatibility stub for old start files.
#define MICRON_ABC_MT 1      // spawns threads; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/memory/allocation/abcmalloc/tapi.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

constexpr static const u32 N_THREADS = 4;

// deterministic xorshift32; the seed is a fixed literal, never time-based
struct xorshift32 {
  u32 s;
  constexpr xorshift32(u32 seed) noexcept : s(seed ? seed : 0xDEADBEEFu) { }
  u32
  next() noexcept
  {
    u32 x = s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s = x;
    return x;
  }
};

static micron::atomic_token<u32> g_bad{ 0 };
static micron::atomic_token<u64> g_arenas{ 0 };

static void
churn(void)
{
  abc::__arena *a = abc::__current_arena();
  if ( a == nullptr ) {
    g_bad.fetch_add(1, micron::memory_order_acq_rel);
    return;
  }
  g_arenas.fetch_add(reinterpret_cast<uintptr_t>(a) != 0 ? 1u : 0u, micron::memory_order_acq_rel);

  xorshift32 rng(0x1234ABCDu);
  for ( usize n = 0; n < (2 << 8); ++n ) {
    auto mem = a->push(1 + (rng.next() % 1000000u));
    if ( mem.invalid() or mem.ptr == nullptr ) {
      g_bad.fetch_add(1, micron::memory_order_acq_rel);
      return;
    }
    mem.ptr[0] = 0x7E;
    mem.ptr[mem.len - 1] = 0xE7;
    if ( mem.ptr[0] != 0x7E or mem.ptr[mem.len - 1] != 0xE7 ) g_bad.fetch_add(1, micron::memory_order_acq_rel);
  }
}

int
main(void)
{
  test_case("__current_arena claims a slot lazily and answers the same one twice");
  {
    abc::__boot_abcmalloc();      // the compatibility stub: an empty no-op, safe to call

    abc::__arena *a = abc::__current_arena();
    require_true(a != nullptr);
    require_true(abc::__current_arena() == a);      // stable for the life of the thread
  }
  end_test_case();

  test_case("the calling thread's arena serves a randomized push run");
  {
    g_bad.store(0, micron::memory_order_relaxed);
    churn();
    require_true(g_bad.get(micron::memory_order_acquire) == 0u);
  }
  end_test_case();

  test_case("every worker gets a usable arena of its own");
  {
    g_bad.store(0, micron::memory_order_relaxed);
    g_arenas.store(0, micron::memory_order_relaxed);
    {
      micron::auto_thread<> th[N_THREADS]
          = { micron::auto_thread<>(churn), micron::auto_thread<>(churn), micron::auto_thread<>(churn), micron::auto_thread<>(churn) };
      (void)th;
    }      // auto_thread joins in its destructor
    require_true(g_bad.get(micron::memory_order_acquire) == 0u);
    require_true(g_arenas.get(micron::memory_order_acquire) == static_cast<u64>(N_THREADS));
  }
  end_test_case();

  sb::print("=== ALL ABCMALLOC THREAD TESTS PASSED ===");
  return 1;
}
