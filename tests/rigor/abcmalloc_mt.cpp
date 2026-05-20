//  Copyright (c) 2025 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/cmalloc.hpp"
#include "../../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../../src/memory/allocation/abcmalloc/config.hpp"
#include "../../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../../src/std.hpp"

#include "../../src/bits/__pause.hpp"
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

constexpr usize NUM_BLOCKS = 256;
constexpr usize BLOCK_SIZE = 128;

struct mt_state {
  byte *ptrs[NUM_BLOCKS];
  micron::atomic_token<u32> ready{ 0 };
};

void
worker_free_all(mt_state *s)
{
  while ( s->ready.get(micron::memory_order_acquire) != 1u ) __cpu_pause();
  for ( usize i = 0; i < NUM_BLOCKS; ++i ) {
    abc::dealloc(s->ptrs[i]);
  }
  s->ready.store(2u, micron::memory_order_release);
}

};      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== ABCMALLOC MT TESTS ===");

  test_case("cross-thread free via MPSC queue + spin-retry on full");
  {
    mt_state s;
    for ( usize i = 0; i < NUM_BLOCKS; ++i ) {
      s.ptrs[i] = abc::alloc(BLOCK_SIZE);
      require_true(s.ptrs[i] != nullptr);
    }

    {
      auto_thread<> th{ worker_free_all, &s };
      s.ready.store(1u, memory_order_release);

      while ( s.ready.get(memory_order_acquire) != 2u ) {
        abc::__current_arena()->__maybe_drain();
        __cpu_pause();
      }
    }

    abc::__current_arena()->__maybe_drain();

    for ( usize i = 0; i < 32; ++i ) {
      byte *q = abc::alloc(BLOCK_SIZE);
      require_true(q != nullptr);
      abc::dealloc(q);
    }
  }
  end_test_case();

  test_case("bidirectional cross-thread alloc + free");
  {
    constexpr usize ROUNDS = 64;
    mt_state s;
    micron::atomic_token<u32> phase{ 0 };

    auto worker = [](micron::atomic_token<u32> *p, mt_state *st) {
      while ( p->get(memory_order_acquire) != 1u ) __cpu_pause();
      for ( usize i = 0; i < ROUNDS; ++i ) abc::dealloc(st->ptrs[i]);

      for ( usize i = 0; i < ROUNDS; ++i ) {
        st->ptrs[i] = abc::alloc(BLOCK_SIZE);
      }
      p->store(2u, memory_order_release);
    };

    for ( usize i = 0; i < ROUNDS; ++i ) s.ptrs[i] = abc::alloc(BLOCK_SIZE);

    {
      auto_thread<> th{ worker, &phase, &s };
      phase.store(1u, memory_order_release);
      while ( phase.get(memory_order_acquire) != 2u ) {
        abc::__current_arena()->__maybe_drain();
        __cpu_pause();
      }
    }
    abc::__current_arena()->__maybe_drain();

    for ( usize i = 0; i < ROUNDS; ++i ) {
      require_true(s.ptrs[i] != nullptr);
      abc::dealloc(s.ptrs[i]);
    }
  }
  end_test_case();

  test_case("musage() aggregates across thread arenas");
  {
    constexpr usize NUM_WORKERS = 4;
    constexpr usize K = 64;
    constexpr usize ALLOC_SIZE = 128;

    struct agg_state {
      byte *blocks[NUM_WORKERS][K];
      micron::atomic_token<u32> ready{ 0 };
      micron::atomic_token<u32> go_free{ 0 };
      micron::atomic_token<u32> freed{ 0 };
    };

    agg_state s;

    auto worker = [](agg_state *st, u32 widx) {
      for ( usize i = 0; i < K; ++i ) st->blocks[widx][i] = abc::alloc(ALLOC_SIZE);
      st->ready.fetch_add(1u, memory_order_acq_rel);
      while ( st->go_free.get(memory_order_acquire) == 0u ) __cpu_pause();
      for ( usize i = 0; i < K; ++i ) abc::dealloc(st->blocks[widx][i]);
      st->freed.fetch_add(1u, memory_order_acq_rel);
    };

    const usize base_usage = abc::musage();

    {

      auto_thread<> w0{ worker, &s, u32(0) };
      auto_thread<> w1{ worker, &s, u32(1) };
      auto_thread<> w2{ worker, &s, u32(2) };
      auto_thread<> w3{ worker, &s, u32(3) };

      while ( s.ready.get(memory_order_acquire) != NUM_WORKERS ) __cpu_pause();

      const usize alloc_phase_usage = abc::musage();
      const u32 pool_next = abc::__arena_pool_next.get(memory_order_acquire);
      require_true(alloc_phase_usage > base_usage);

      require_true(pool_next >= NUM_WORKERS);

      const usize precise_total = abc::musage<abc::__class_precise>();
      require_true(precise_total > 0);

      s.go_free.store(1u, memory_order_release);
      while ( s.freed.get(memory_order_acquire) != NUM_WORKERS ) {
        abc::__current_arena()->__maybe_drain();
        __cpu_pause();
      }
    }

    abc::__current_arena()->__maybe_drain();

    require_true(abc::musage() >= base_usage);
  }
  end_test_case();

  sb::print("=== ALL ABCMALLOC MT TESTS PASSED ===");
  return 1;
}
