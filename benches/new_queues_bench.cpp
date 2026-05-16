//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/queue/crossbeam.hpp"
#include "../src/queue/disruptor.hpp"
#include "../src/std.hpp"

#include <atomic>
#include <cstdio>
#include <thread>
#include <time.h>
#include <vector>

namespace
{

static volatile u64 sink = 0;

inline void
print_row(const char *impl, const char *op, usize n, double ns_per_op)
{
  char buf[160];
  unsigned long long whole = static_cast<unsigned long long>(ns_per_op);
  unsigned long long frac = static_cast<unsigned long long>((ns_per_op - static_cast<double>(whole)) * 100.0);
  std::snprintf(buf, sizeof(buf), "  %-12s %-14s N=%-8llu %llu.%02llu ns/op", impl, op, static_cast<unsigned long long>(n), whole, frac);
  micron::io::println(buf);
}

constexpr usize K_N = 1000000;

inline u64
now_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

}      // namespace

int
main()
{
  micron::io::println("=== NEW QUEUES BENCH ===");

  {
    micron::disruptor<u64, 4096> q;
    u64 t0 = now_ns();
    for ( usize i = 0; i < K_N; ++i ) {
      q.publish(i);
      u64 v = 0;
      q.consume(v);
      sink += v;
    }
    u64 t1 = now_ns();
    double ns = static_cast<double>(t1 - t0) / static_cast<double>(K_N);
    print_row("disruptor", "pub+cons", K_N, ns);
  }

  {
    micron::disruptor<u64, 4096> q;
    std::atomic<bool> ready{ false };
    std::thread prod([&]() {
      while ( !ready.load(std::memory_order_acquire) );
      for ( usize i = 0; i < K_N; ) {
        if ( q.publish(i) ) ++i;
      }
    });
    std::thread cons([&]() {
      while ( !ready.load(std::memory_order_acquire) );
      for ( usize i = 0; i < K_N; ) {
        u64 v;
        if ( q.consume(v) ) {
          sink += v;
          ++i;
        }
      }
    });
    u64 t0 = now_ns();
    ready.store(true, std::memory_order_release);
    prod.join();
    cons.join();
    u64 t1 = now_ns();
    double ns = static_cast<double>(t1 - t0) / static_cast<double>(K_N);
    print_row("disruptor", "spsc-2thr", K_N, ns);
  }

  {
    micron::crossbeam<u64, 4096> q;
    u64 t0 = now_ns();
    for ( usize i = 0; i < K_N; ++i ) {
      q.push(i);
      u64 v = 0;
      q.pop(v);
      sink += v;
    }
    u64 t1 = now_ns();
    double ns = static_cast<double>(t1 - t0) / static_cast<double>(K_N);
    print_row("crossbeam", "push+pop", K_N, ns);
  }

  {
    constexpr int P = 4;
    constexpr int C = 4;
    constexpr usize PER = 100000;
    micron::crossbeam<u64, 4096> q;
    std::atomic<usize> consumed{ 0 };
    std::atomic<bool> ready{ false };
    std::vector<std::thread> ts;
    for ( int p = 0; p < P; ++p ) {
      ts.emplace_back([&, p]() {
        while ( !ready.load(std::memory_order_acquire) );
        for ( usize i = 0; i < PER; ) {
          if ( q.push(static_cast<u64>(p) * PER + i) ) ++i;
        }
      });
    }
    for ( int c = 0; c < C; ++c ) {
      ts.emplace_back([&]() {
        while ( !ready.load(std::memory_order_acquire) );
        while ( consumed.load(std::memory_order_relaxed) < P * PER ) {
          u64 v;
          if ( q.pop(v) ) {
            sink += v;
            consumed.fetch_add(1, std::memory_order_relaxed);
          }
        }
      });
    }
    u64 t0 = now_ns();
    ready.store(true, std::memory_order_release);
    for ( auto &t : ts ) t.join();
    u64 t1 = now_ns();
    usize total = P * PER;
    double ns = static_cast<double>(t1 - t0) / static_cast<double>(total);
    print_row("crossbeam", "mpmc-8thr", total, ns);
  }

  micron::io::println("sink=", sink);
  return 0;
}
