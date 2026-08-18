//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Bench for the dynamic-loading layer: dynamic_open cold and warm, dynamic_sym across the three
// scope shapes, and dynamic_call's one-shot overload.
//
// The number that matters most is the WARM open. A cold open is dominated by mmap and page faults
// and is not really micron's to win; a warm one is pure registry work -- fstat, a dev/ino compare,
// a refcount -- and if that is not comfortably under a microsecond, the dedup path is doing
// something it should not be.
//
// Needs the fixtures:  sh tests/support/dl/build.sh x64
// Build with:          duck build benches/dynamic_bench.cpp --perf --fp --no-ssp --no-lto -o bin/b
// and pin it:          taskset -c 2 bin/b/dynamic_bench

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"

#include "../src/dynamic.hpp"

namespace mc = micron;

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 3;
constexpr u64 REPS_OPEN = 200;
constexpr u64 REPS_SYM = 20000;

#if defined(__micron_arch_amd64)
constexpr const char *ARCH_DIR = "x64";
#elif defined(__micron_arch_x86)
constexpr const char *ARCH_DIR = "i386";
#elif defined(__micron_arch_arm32)
constexpr const char *ARCH_DIR = "arm";
#else
constexpr const char *ARCH_DIR = "arm64";
#endif

volatile void *sink_p = nullptr;
volatile int sink_i = 0;

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    i32 j = static_cast<i32>(i) - 1;
    while ( j >= 0 && xs[j] > key ) {
      xs[j + 1] = xs[j];
      --j;
    }
    xs[j + 1] = key;
  }
  return xs[n / 2];
}

void
row(const char *op, u64 n, f64 ns_per_op)
{
  micron::io::print("  ");
  micron::io::print(op);
  for ( usize i = micron::strlen(op); i < 34; ++i ) micron::io::print(" ");
  micron::io::print(static_cast<u64>(n));
  micron::io::print("  ");
  micron::io::print(static_cast<u64>(ns_per_op));
  micron::io::print(".");
  micron::io::print(static_cast<u64>((ns_per_op - static_cast<f64>(static_cast<u64>(ns_per_op))) * 100.0));
  micron::io::println(" ns/op");
}

micron::sstring<512>
fixture(const char *name)
{
  micron::sstring<512> p;
  const char *base = "tests/support/dl/";
  for ( usize i = 0; base[i]; ++i ) p += base[i];
  for ( usize i = 0; ARCH_DIR[i]; ++i ) p += ARCH_DIR[i];
  p += '/';
  for ( usize i = 0; name[i]; ++i ) p += name[i];
  p.null_term();
  return p;
}

}      // namespace

int
main()
{
  micron::io::println("=== DYNAMIC (dlopen) BENCH ===");

  const auto solo = fixture("libmc_dl_solo.so.1");
  const auto top = fixture("libmc_dl_top.so.1");

  if ( !micron::elf::__file_is_native_elf(solo.c_str()) ) {
    micron::io::println("fixtures missing -- run: sh tests/support/dl/build.sh");
    return 0;
  }

  f64 m[K_MEASUREMENTS];

  // ---- cold open: a fresh mmap of a one-object library, every time ----
  for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < REPS_OPEN; ++i ) {
      mc::dynamic_t d = mc::dynamic_open(solo.c_str());
      sink_i += static_cast<int>(d.index);
      mc::dynamic_close(d);
    }
    const u64 t1 = now_ns();
    if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_OPEN);
  }
  row("open+close, no deps (cold)", REPS_OPEN, median_f64(m, K_MEASUREMENTS));

  // ---- cold open of a three-deep DT_NEEDED chain ----
  for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
    const u64 t0 = now_ns();
    for ( u64 i = 0; i < REPS_OPEN; ++i ) {
      mc::dynamic_t d = mc::dynamic_open(top.c_str());
      sink_i += static_cast<int>(d.index);
      mc::dynamic_close(d);
    }
    const u64 t1 = now_ns();
    if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_OPEN);
  }
  row("open+close, 3-deep chain (cold)", REPS_OPEN, median_f64(m, K_MEASUREMENTS));

  // ---- warm open: the dedup path. one fstat, a dev/ino compare and a refcount. ----
  {
    mc::dynamic_t hold = mc::dynamic_open(solo.c_str());
    for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
      const u64 t0 = now_ns();
      for ( u64 i = 0; i < REPS_OPEN * 10; ++i ) {
        mc::dynamic_t d = mc::dynamic_open(solo.c_str());
        sink_i += static_cast<int>(d.index);
        mc::dynamic_close(d);
      }
      const u64 t1 = now_ns();
      if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_OPEN * 10);
    }
    row("open+close, already loaded (warm)", REPS_OPEN * 10, median_f64(m, K_MEASUREMENTS));
    mc::dynamic_close(hold);
  }

  // ---- dynamic_sym across the three scope shapes ----
  {
    mc::dynamic_t d = mc::dynamic_open(top.c_str(), mc::rtld::now | mc::rtld::global);

    for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
      const u64 t0 = now_ns();
      for ( u64 i = 0; i < REPS_SYM; ++i ) sink_p = mc::dynamic_sym(d, "mc_dl_top_call");
      const u64 t1 = now_ns();
      if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_SYM);
    }
    row("sym, hit in the module itself", REPS_SYM, median_f64(m, K_MEASUREMENTS));

    // two hops down the DT_NEEDED closure -- the BFS actually walks here
    for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
      const u64 t0 = now_ns();
      for ( u64 i = 0; i < REPS_SYM; ++i ) sink_p = mc::dynamic_sym(d, "mc_dl_leaf_value");
      const u64 t1 = now_ns();
      if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_SYM);
    }
    row("sym, hit 2 hops down the closure", REPS_SYM, median_f64(m, K_MEASUREMENTS));

    // a miss is the worst case: every module in the scope is searched before giving up
    for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
      const u64 t0 = now_ns();
      for ( u64 i = 0; i < REPS_SYM; ++i ) sink_p = mc::dynamic_sym(d, "no-such-symbol-zzz9");
      const u64 t1 = now_ns();
      if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_SYM);
    }
    row("sym, miss (whole scope searched)", REPS_SYM, median_f64(m, K_MEASUREMENTS));

    for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
      const u64 t0 = now_ns();
      for ( u64 i = 0; i < REPS_SYM; ++i ) sink_p = mc::dynamic_sym(mc::dynamic_default, "mc_dl_top_call");
      const u64 t1 = now_ns();
      if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_SYM);
    }
    row("sym, dynamic_default (global scope)", REPS_SYM, median_f64(m, K_MEASUREMENTS));

    // resolve + indirect call, which is what dynamic_call costs over a raw function pointer
    for ( u32 k = 0; k < K_MEASUREMENTS + WARMUP_REPS; ++k ) {
      const u64 t0 = now_ns();
      for ( u64 i = 0; i < REPS_SYM; ++i ) sink_i += mc::dynamic_call<int>(d, "mc_dl_top_call");
      const u64 t1 = now_ns();
      if ( k >= WARMUP_REPS ) m[k - WARMUP_REPS] = static_cast<f64>(t1 - t0) / static_cast<f64>(REPS_SYM);
    }
    row("dynamic_call (resolve + invoke)", REPS_SYM, median_f64(m, K_MEASUREMENTS));

    mc::dynamic_close(d);
  }

  micron::io::print("sink=");
  micron::io::println(static_cast<u64>(sink_i));
  return 0;
}
