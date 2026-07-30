//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../src/queue/static_mpmc.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include "../support/mt.hpp"

// layout guarantees the whole point of this type rests on
static_assert(alignof(micron::static_mpmc<int, 16>) >= micron::cache_line_size(), "static_mpmc must be cache-line aligned");
static_assert(sizeof(micron::static_mpmc<int, 16>) >= 18 * micron::cache_line_size(),
              "each cell must own a line, and head/tail a line each");
static_assert(micron::static_mpmc<int, 16>::capacity() == 16ULL, "capacity is the template argument, not a rounded one");

// a ring must be constructible at namespace scope with no allocator in sight -- the reason this
// type exists at all (crossbeam allocates through abcmalloc and cannot do this)
static micron::static_mpmc<void *, 64> __global_pool;

int
main(void)
{
  sb::print("=== STATIC_MPMC TESTS ===");

  sb::test_case("construction - empty");
  {
    micron::static_mpmc<int, 16> q;
    sb::require(q.empty());
    sb::require(q.size() == 0ULL);
    sb::require(q.capacity() == 16ULL);
    sb::require(q.max_size() == 16ULL);
    sb::require(!q.maybe_nonempty());
  }
  sb::end_test_case();

  sb::test_case("push then pop");
  {
    micron::static_mpmc<int, 16> q;
    sb::require(q.push(7));
    sb::require(q.maybe_nonempty());
    int v = 0;
    sb::require(q.pop(v));
    sb::require(v == 7);
    sb::require(!q.maybe_nonempty());
  }
  sb::end_test_case();

  sb::test_case("push full returns false");
  {
    micron::static_mpmc<int, 4> q;
    sb::require(q.push(1));
    sb::require(q.push(2));
    sb::require(q.push(3));
    sb::require(q.push(4));
    sb::require(!q.push(5));
    sb::require(q.size() == 4ULL);
  }
  sb::end_test_case();

  sb::test_case("pop empty returns false");
  {
    micron::static_mpmc<int, 16> q;
    int v = 0;
    sb::require(!q.pop(v));
  }
  sb::end_test_case();

  sb::test_case("FIFO order preserved (single producer)");
  {
    micron::static_mpmc<int, 64> q;
    for ( int i = 0; i < 20; ++i ) sb::require(q.push(i));
    for ( int i = 0; i < 20; ++i ) {
      int v = -1;
      sb::require(q.pop(v));
      sb::require(v == i);
    }
    sb::require(q.empty());
  }
  sb::end_test_case();

  // 300k rounds on a 4-cell ring: drives the cell tags far past any 16-bit boundary and keeps the
  // 32-bit-usize counter arithmetic (i386/arm32) honest
  sb::test_case("wrap-around");
  {
    micron::static_mpmc<int, 4> q;
    for ( int round = 0; round < 300000; ++round ) {
      sb::require(q.push(round));
      int v = 0;
      sb::require(q.pop(v));
      sb::require(v == round);
    }
    sb::require(q.empty());
  }
  sb::end_test_case();

  // the same, but keeping the ring partly full so head and tail wrap at different cells
  sb::test_case("wrap-around, ring kept half full");
  {
    micron::static_mpmc<u32, 8> q;
    for ( u32 i = 0; i < 4; ++i ) sb::require(q.push(i));
    for ( u32 round = 4; round < 200000; ++round ) {
      sb::require(q.push(round));
      u32 v = 0;
      sb::require(q.pop(v));
      sb::require(v == round - 4);
    }
    sb::require(q.size() == 4ULL);
  }
  sb::end_test_case();

  sb::test_case("pointer sentinel pop() overload");
  {
    micron::static_mpmc<int *, 8> q;
    int a = 1, b = 2;
    sb::require(q.pop() == nullptr);      // empty
    sb::require(q.push(&a));
    sb::require(q.push(&b));
    sb::require(q.pop() == &a);
    sb::require(q.pop() == &b);
    sb::require(q.pop() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("drain");
  {
    micron::static_mpmc<int, 16> q;
    for ( int i = 0; i < 10; ++i ) sb::require(q.push(i));
    int sum = 0;
    u32 n = q.drain([&sum](int v) { sum += v; });
    sb::require(n == 10U);
    sb::require(sum == 45);
    sb::require(q.empty());
    sb::require(q.drain([](int) { }) == 0U);
  }
  sb::end_test_case();

  sb::test_case("clear");
  {
    micron::static_mpmc<int, 16> q;
    for ( int i = 0; i < 12; ++i ) sb::require(q.push(i));
    q.clear();
    sb::require(q.empty());
    sb::require(q.push(99));      // still usable afterwards
    int v = 0;
    sb::require(q.pop(v));
    sb::require(v == 99);
  }
  sb::end_test_case();

  sb::test_case("namespace-scope instance is usable");
  {
    int a = 0;
    sb::require(__global_pool.empty());
    sb::require(__global_pool.push(&a));
    sb::require(__global_pool.pop() == &a);
  }
  sb::end_test_case();

  sb::test_case("MPMC - 4 producers x 4 consumers, 40k items");
  {
    micron::static_mpmc<int, 1024> q;
    constexpr int P = 4;
    constexpr int PER = 10000;
    micron::atomic_token<int> total_consumed{ 0 };
    micron::atomic_token<long long> sum_consumed{ 0 };
    micron::atomic_token<bool> stop{ false };

    auto produce = [&](int p) {
      for ( int i = 0; i < PER; ) {
        if ( q.push(p * PER + i) ) ++i;
      }
    };
    auto consume = [&]() {
      while ( !stop.get(micron::memory_order_acquire) ) {
        int v;
        if ( q.pop(v) ) {
          sum_consumed.fetch_add(v, micron::memory_order_relaxed);
          int n = total_consumed.fetch_add(1, micron::memory_order_relaxed) + 1;
          if ( n >= P * PER ) {
            stop.store(true, micron::memory_order_release);
            break;
          }
        }
      }
    };

    mtest::parallel(2 * P, [&](int t) {
      if ( t < P )
        produce(t);
      else
        consume();
    });
    long long expected = 0;
    for ( int p = 0; p < P; ++p ) {
      for ( int i = 0; i < PER; ++i ) expected += p * PER + i;
    }
    sb::require(total_consumed.get() == P * PER);
    sb::require(sum_consumed.get() == expected);
  }
  sb::end_test_case();

  // the seg-pool profile: a small ring under heavy contention, so full/empty and CAS retry are the
  // common case rather than the exception
  sb::test_case("MPMC - cap 64 under 8-way contention, pointer payload");
  {
    micron::static_mpmc<void *, 64> q;
    constexpr int P = 4;
    constexpr int PER = 20000;
    micron::atomic_token<int> total_consumed{ 0 };
    micron::atomic_token<bool> stop{ false };
    static int slab[P * PER];

    auto produce = [&](int p) {
      for ( int i = 0; i < PER; ) {
        if ( q.push(static_cast<void *>(&slab[p * PER + i])) ) ++i;
      }
    };
    auto consume = [&]() {
      while ( !stop.get(micron::memory_order_acquire) ) {
        void *v = q.pop();
        if ( v != nullptr ) {
          *static_cast<int *>(v) += 1;
          int n = total_consumed.fetch_add(1, micron::memory_order_relaxed) + 1;
          if ( n >= P * PER ) {
            stop.store(true, micron::memory_order_release);
            break;
          }
        }
      }
    };

    mtest::parallel(2 * P, [&](int t) {
      if ( t < P )
        produce(t);
      else
        consume();
    });
    sb::require(total_consumed.get() == P * PER);
    for ( int i = 0; i < P * PER; ++i ) sb::require(slab[i] == 1);      // each pointer delivered exactly once
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
