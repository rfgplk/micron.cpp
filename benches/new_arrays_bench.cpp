//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Bench for the new array containers: soa, mdarray.

#include "../src/array/mdarray.hpp"
#include "../src/array/soa.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include <cstdio>
#include <time.h>

namespace
{

static volatile float sink_f = 0;

inline void
print_row(const char *impl, const char *op, usize n, double ns_per_op)
{
  char buf[160];
  unsigned long long whole = static_cast<unsigned long long>(ns_per_op);
  unsigned long long frac = static_cast<unsigned long long>((ns_per_op - static_cast<double>(whole)) * 100.0);
  std::snprintf(buf, sizeof(buf), "  %-12s %-18s N=%-9llu %llu.%02llu ns/op", impl, op, static_cast<unsigned long long>(n), whole, frac);
  micron::io::println(buf);
}

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
  micron::io::println("=== NEW ARRAYS BENCH ===");

  {
    constexpr usize N = 1000000;
    micron::soa<f32, f32, u32> s(N);
    u64 t0 = now_ns();
    for ( usize i = 0; i < N; ++i ) s.emplace_back(static_cast<f32>(i), static_cast<f32>(i) * 2.0f, static_cast<u32>(i));
    u64 t1 = now_ns();
    double ns_ins = static_cast<double>(t1 - t0) / static_cast<double>(N);
    print_row("soa", "emplace_back", N, ns_ins);

    auto *c0 = s.column<0>();
    u64 t2 = now_ns();
    f32 acc = 0;
    for ( usize i = 0; i < N; ++i ) acc += c0[i];
    u64 t3 = now_ns();
    sink_f += acc;
    double ns_sum = static_cast<double>(t3 - t2) / static_cast<double>(N);
    print_row("soa", "sum-col0", N, ns_sum);
  }

  {
    constexpr usize N = 1000000;
    micron::mdarray<f32, 1> a(N);
    a.fill(1.0f);
    micron::mdarray<f32, 1> b(N);
    b.fill(2.0f);
    u64 t0 = now_ns();
    a += b;
    u64 t1 = now_ns();
    sink_f += a.data()[0];
    double ns = static_cast<double>(t1 - t0) / static_cast<double>(N);
    print_row("mdarray", "elemwise-add", N, ns);

    u64 t2 = now_ns();
    a *= 3.0f;
    u64 t3 = now_ns();
    sink_f += a.data()[0];
    double ns_mul = static_cast<double>(t3 - t2) / static_cast<double>(N);
    print_row("mdarray", "scalar-mul", N, ns_mul);

    u64 t4 = now_ns();
    f32 s = a.sum();
    u64 t5 = now_ns();
    sink_f += s;
    double ns_sum_t = static_cast<double>(t5 - t4) / static_cast<double>(N);
    print_row("mdarray", "sum", N, ns_sum_t);
  }

  {
    constexpr usize R = 1024;
    constexpr usize C = 1024;
    micron::mdarray<f32, 2> a(R, C);
    a.fill(1.0f);
    micron::mdarray<f32, 2> b(R, C);
    b.fill(0.5f);
    u64 t0 = now_ns();
    a += b;
    u64 t1 = now_ns();
    sink_f += a(0, 0);
    double ns = static_cast<double>(t1 - t0) / static_cast<double>(R * C);
    print_row("mdarray-2d", "add(1024x1024)", R * C, ns);
  }

  char buf[40];
  std::snprintf(buf, sizeof(buf), "sink_f=%g", static_cast<double>(sink_f));
  micron::io::println(buf);
  return 0;
}
