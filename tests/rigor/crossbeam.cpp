//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/queue/crossbeam.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <atomic>
#include <thread>
#include <vector>

int
main(void)
{
  sb::print("=== CROSSBEAM MPMC TESTS ===");

  sb::test_case("construction - empty");
  {
    micron::crossbeam<int, 16> q;
    sb::require(q.empty());
    sb::require(q.size() == 0ULL);
    sb::require(q.capacity() == 16ULL);
  }
  sb::end_test_case();

  sb::test_case("push then pop");
  {
    micron::crossbeam<int, 16> q;
    sb::require(q.push(7));
    int v = 0;
    sb::require(q.pop(v));
    sb::require(v == 7);
  }
  sb::end_test_case();

  sb::test_case("push full returns false");
  {
    micron::crossbeam<int, 4> q;
    sb::require(q.push(1));
    sb::require(q.push(2));
    sb::require(q.push(3));
    sb::require(q.push(4));
    sb::require(!q.push(5));
  }
  sb::end_test_case();

  sb::test_case("pop empty returns false");
  {
    micron::crossbeam<int, 16> q;
    int v = 0;
    sb::require(!q.pop(v));
  }
  sb::end_test_case();

  sb::test_case("FIFO order preserved (single producer)");
  {
    micron::crossbeam<int, 64> q;
    for ( int i = 0; i < 20; ++i ) sb::require(q.push(i));
    for ( int i = 0; i < 20; ++i ) {
      int v = -1;
      sb::require(q.pop(v));
      sb::require(v == i);
    }
  }
  sb::end_test_case();

  sb::test_case("wrap-around");
  {
    micron::crossbeam<int, 4> q;
    for ( int round = 0; round < 100; ++round ) {
      sb::require(q.push(round));
      int v = 0;
      sb::require(q.pop(v));
      sb::require(v == round);
    }
  }
  sb::end_test_case();

  // ── concurrent ────────────────────────────────────────────────────────────

  sb::test_case("MPMC - 4 producers x 4 consumers, 40k items");
  {
    micron::crossbeam<int, 1024> q;
    constexpr int P = 4;
    constexpr int C = 4;
    constexpr int PER = 10000;
    std::atomic<int> total_consumed{ 0 };
    std::atomic<long long> sum_consumed{ 0 };
    std::atomic<bool> stop{ false };
    std::vector<std::thread> pt;
    for ( int p = 0; p < P; ++p ) {
      pt.emplace_back([&, p]() {
        for ( int i = 0; i < PER; ) {
          if ( q.push(p * PER + i) ) ++i;
        }
      });
    }
    std::vector<std::thread> ct;
    for ( int c = 0; c < C; ++c ) {
      ct.emplace_back([&]() {
        while ( !stop.load(std::memory_order_acquire) ) {
          int v;
          if ( q.pop(v) ) {
            sum_consumed.fetch_add(v, std::memory_order_relaxed);
            int n = total_consumed.fetch_add(1, std::memory_order_relaxed) + 1;
            if ( n >= P * PER ) {
              stop.store(true, std::memory_order_release);
              break;
            }
          }
        }
      });
    }
    for ( auto &t : pt ) t.join();
    // wait for all consumed
    while ( total_consumed.load() < P * PER );
    stop.store(true);
    for ( auto &t : ct ) t.join();
    long long expected = 0;
    for ( int p = 0; p < P; ++p ) {
      for ( int i = 0; i < PER; ++i ) expected += p * PER + i;
    }
    sb::require(total_consumed.load() == P * PER);
    sb::require(sum_consumed.load() == expected);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
