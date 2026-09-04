//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// abcmalloc's __arena: push/pop, the internal buffer, and the stats counters.
//
// push() reports failure as ptr == (byte *)-1 (chunk::invalid()), not as nullptr.

#include "../../src/memory/allocation/abcmalloc/arena.hpp"
#include "../../src/memory/allocation/abcmalloc/book.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

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

int
main(void)
{
  test_case("a fresh arena serves a push and hands back a writable block");
  {
    abc::__arena arena;
    auto mem = arena.push(1000);
    require_true(!mem.invalid());
    require_true(mem.ptr != nullptr);
    require_true(mem.len >= 1000);

    micron::byteset(mem.ptr, 0xF3, mem.len);
    require_true(mem.ptr[0] == 0xF3 && mem.ptr[mem.len - 1] == 0xF3);
    require_true(arena.pop(mem));
  }
  end_test_case();

  test_case("push/pop round-trips over a mixed-size run and recycles");
  {
    abc::__arena arena;
    xorshift32 rng(0xBADC0DEDu);
    usize served = 0;
    usize req_total = 0;

    for ( usize n = 0; n < (2 << 10); ++n ) {
      const usize want = 1 + (rng.next() % 1000000u);
      auto mem = arena.push(want);
      require_true(!mem.invalid());
      require_true(mem.ptr != nullptr);
      require_true(mem.len >= want);      // granted is never below requested

      // touch both ends so a short grant would fault under asan
      mem.ptr[0] = 0x11;
      mem.ptr[mem.len - 1] = 0x22;

      require_true(arena.pop(mem));
      req_total += want;
      ++served;
    }
    require_true(served == static_cast<usize>(2 << 10));
    sb::print("arena served ", served, " push/pop pairs, ", req_total, "B requested");
  }
  end_test_case();

  test_case("a block stays valid while later pushes churn around it");
  {
    abc::__arena arena;
    xorshift32 rng(0x5EED0011u);

    auto pinned = arena.push(4096);
    require_true(!pinned.invalid() && pinned.ptr != nullptr);
    pinned.ptr[0] = 0x01;

    for ( usize n = 0; n < (2 << 12); ++n ) {
      auto tmp = arena.push(30 + (rng.next() % 4492));
      require_true(!tmp.invalid());
    }
    require_true(pinned.ptr[0] == 0x01);      // nothing scribbled over a live block
  }
  end_test_case();

  test_case("stats accounting is self-consistent when the counters are compiled in");
  {
    abc::stats_t stats = abc::get_stats();
    if ( !stats.enabled ) {
      sb::skip("MICRON_ABC_STATS is off: the counters are compiled out");
    } else {
      require_true(stats.alloc_requests > 0);
      require_true(stats.dealloc_requests <= stats.alloc_requests);
      require_true(stats.total_memory_throughput >= stats.total_memory_req);
      require_true(stats.total_memory_freed <= stats.total_memory_throughput);
      sb::print("allocs=", stats.alloc_requests, " frees=", stats.dealloc_requests, " req=", stats.total_memory_req,
                " granted=", stats.total_memory_throughput);
    }
  }
  end_test_case();

  sb::print("=== ALL ABCMALLOC ARENA TESTS PASSED ===");
  return 1;
}
