//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Stack benchmark — micron stack family (MICRON SIDE).
//
//   subjects: stack (checked, heap), fstack (fast, heap), istack (persistent
//             cons-list), constack (mutex), sstack (in-object fixed-capacity).
//   kernels:  fill+drain : push N then pop N (ops = 2N), self-resetting (ends empty)
//             peek x1024 : 1024 non-destructive top() on a filled stack
//   sizes:    256, 4096, 65536. Reports ns/op, median of K_MEASUREMENTS. The
//             matching stack_bench_std.cpp runs std::stack / std::vector / a mutex
//             stack with the SAME methodology so the two binaries compare directly
//             (std lives in its own TU: libstdc++ + micron's pthread shim cannot
//             share a translation unit).

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/stacks/constack.hpp"
#include "../src/stacks/fstack.hpp"
#include "../src/stacks/istack.hpp"
#include "../src/stacks/sstack.hpp"
#include "../src/stacks/stack.hpp"

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP = 2;
constexpr u64 SIZES[] = { 256, 1ULL << 12, 1ULL << 16 };

// IMPORTANT: time the kernels with rdtsc (~18 cyc), NOT micron::clock_gettime,
// which is a raw syscall (~1107 cyc / ~316 ns/call). At small N (e.g. 256 → 512
// ops/sample) a 2x-syscall timer adds ~1.2 ns/op of pure artifact and made the
// stack look ~2x slower than std (whose chrono uses the vDSO). g_tsc_ghz converts
// cycles → ns; it is calibrated once against the (slow) wall clock at startup.
f64 g_tsc_ghz = 1.0;

[[gnu::always_inline]] inline u64
rdtsc() noexcept
{
  u32 lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
}

[[gnu::always_inline]] inline u64
wall_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

void
calibrate_tsc() noexcept
{
  const u64 n0 = wall_ns();
  const u64 c0 = rdtsc();
  while ( wall_ns() - n0 < 50000000ULL ) {
  }      // ~50 ms
  const u64 c1 = rdtsc();
  const u64 n1 = wall_ns();
  g_tsc_ghz = static_cast<f64>(c1 - c0) / static_cast<f64>(n1 - n0);      // cycles per ns
}

[[gnu::always_inline]] inline f64
cyc_to_ns(u64 cyc) noexcept
{
  return static_cast<f64>(cyc) / g_tsc_ghz;
}

[[gnu::always_inline]] inline u64
lcg(u64 &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s >> 16;
}

[[gnu::always_inline]] inline void
clobber(u64 v) noexcept
{
  asm volatile("" : : "r"(v));
}

f64
median(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    f64 k = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > k ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = k;
  }
  return xs[n / 2];
}

// print "  <name> <size>:  X.XX ns/op"
void
report(const char *name, u64 size, f64 nsop) noexcept
{
  if ( nsop < 0 ) nsop = 0;
  u64 centi = static_cast<u64>(nsop * 100.0 + 0.5);
  micron::io::print("  ", name, "  N=", size, "  ", centi / 100, ".", (centi % 100 < 10 ? "0" : ""), centi % 100, " ns/op\n");
}

// ── heap-backed (stack / fstack / constack): push N / pop N ─────────────────
template<class Stack>
f64
bench_fill_drain(u64 n, u64 seed) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    Stack s;
    u64 rng = seed + m;
    const u64 t0 = rdtsc();
    for ( u64 i = 0; i < n; ++i ) s.push(static_cast<int>(lcg(rng)));
    u64 acc = 0;
    for ( u64 i = 0; i < n; ++i ) acc += static_cast<u64>(s.pop());
    const u64 t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / static_cast<f64>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

// ── persistent istack: s = s.push(v) / s = s.pop() ──────────────────────────
f64
bench_fill_drain_istack(u64 n, u64 seed) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    micron::istack<int> s;
    u64 rng = seed + m;
    const u64 t0 = rdtsc();
    for ( u64 i = 0; i < n; ++i ) s = s.push(static_cast<int>(lcg(rng)));
    u64 acc = 0;
    for ( u64 i = 0; i < n; ++i ) {
      acc += static_cast<u64>(s.top());
      s = s.pop();
    }
    const u64 t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / static_cast<f64>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

// ── in-object fixed-capacity sstack<int,N> (N must hold the whole fill) ──────
template<usize N>
f64
bench_fill_drain_sstack(u64 seed) noexcept
{
  static micron::sstack<int, N> s;      // static: avoid a huge stack object at large N
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    u64 rng = seed + m;
    const u64 t0 = rdtsc();
    for ( u64 i = 0; i < N; ++i ) s.push(static_cast<int>(lcg(rng)));
    u64 acc = 0;
    for ( u64 i = 0; i < N; ++i ) {
      acc += static_cast<u64>(s.top());
      s.pop();      // sstack::pop() returns void
    }
    const u64 t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / static_cast<f64>(2 * N);
  }
  return median(samples, K_MEASUREMENTS);
}

template<class Stack>
f64
bench_peek(u64 n, u64 seed) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    Stack s;
    u64 rng = seed + m;
    for ( u64 i = 0; i < n; ++i ) s.push(static_cast<int>(lcg(rng)));
    const u64 t0 = rdtsc();
    u64 acc = 0;
    for ( u32 i = 0; i < 1024; ++i ) acc += static_cast<u64>(s.top());
    const u64 t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / 1024.0;
  }
  return median(samples, K_MEASUREMENTS);
}

}      // namespace

int
main()
{
  calibrate_tsc();
  {
    u64 ghz_centi = static_cast<u64>(g_tsc_ghz * 100.0 + 0.5);
    micron::io::print("# TSC calibrated: ", ghz_centi / 100, ".", (ghz_centi % 100 < 10 ? "0" : ""), ghz_centi % 100,
                      " GHz (rdtsc timing)\n");
  }
  micron::io::print("=== micron stack family — fill+drain (push N + pop N), ns/op ===\n");
  for ( u64 sz : SIZES ) {
    report("micron::stack   ", sz, bench_fill_drain<micron::stack<int>>(sz, 0x1111));
    report("micron::fstack  ", sz, bench_fill_drain<micron::fstack<int>>(sz, 0x2222));
    report("micron::istack  ", sz, bench_fill_drain_istack(sz, 0x3333));
    report("micron::constack", sz, bench_fill_drain<micron::constack<int>>(sz, 0x4444));
    micron::io::print("\n");
  }

  micron::io::print("=== in-object fixed-capacity sstack<int,N> — fill+drain, ns/op ===\n");
  report("micron::sstack  ", 256, bench_fill_drain_sstack<256>(0x5555));
  report("micron::sstack  ", 4096, bench_fill_drain_sstack<4096>(0x5556));
  report("micron::sstack  ", 65536, bench_fill_drain_sstack<65536>(0x5557));

  micron::io::print("\n=== peek x1024 (non-destructive top()), ns/op ===\n");
  report("micron::stack   ", 4096, bench_peek<micron::stack<int>>(4096, 0x6661));
  report("micron::fstack  ", 4096, bench_peek<micron::fstack<int>>(4096, 0x6662));
  report("micron::constack", 4096, bench_peek<micron::constack<int>>(4096, 0x6663));

  micron::io::print("\n(note: cactus_stack / fixed_stack are persistent arena stacks with O(depth)\n"
                    " copy-on-push; they are excluded from raw throughput — their value is O(1)\n"
                    " branch / structural sharing, not int push/pop.)\n");
  return 0;
}
