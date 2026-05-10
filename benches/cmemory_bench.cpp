//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Benchmark harness for the freestanding micron memory routines:
//   mc::memcpy, mc::memset, mc::memcmp, mc::memmove (scalar cmemory entry points)
//   simd::memcpy{128,256,512}, amemcpy*, ntmemcpy*
//   simd::memset{128,256,512}, amemset*, ntmemset*
//   simd::memcmp{128,256,512}, amemcmp*
//   simd::memmove{128,256,512}, amemmove*
//
// Per (routine, size) pair the harness reports:
//   - cycles per byte (median of K measurements)
//   - IPC (instructions per cycle)
//   - mean call latency in cycles
//
// Cycle counts come from the Linux perf_event_open syscall (CPU_CYCLES +
// INSTRUCTIONS counter group, exclude_kernel + exclude_hv). If perf is
// unavailable (perf_event_paranoid too high, PMU disabled, sandbox blocks
// the syscall), the harness falls back to rdtsc-only mode and reports
// IPC as N/A.
//
// Buffers: 32 MiB of cacheline-aligned BSS (16 MiB src + 16 MiB dst),
// reused across sweeps so the data sits hot in cache for sizes <= L2/L3
// and falls into main-memory streaming territory beyond ~16 MiB.

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"

#include "perf_counter.hpp"

// IMPORTANT: at -Ofast, GCC's tree-loop-distribute-patterns optimization
// will silently turn `mc::memcpy` / `mc::memmove`'s scalar 4-stride byte
// loop into a libc memcpy@plt / memmove@plt call — defeating the purpose
// of benchmarking the freestanding micron path. Disable that pass for the
// whole TU. (The same flag is applied to the freestanding C-ABI fallback
// in src/memory/cmemory/memcpy.hpp:599-609.)
#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Buffers and tunables

namespace
{

constexpr u64 BUF_BYTES = 16ULL << 20;     // 16 MiB
alignas(64) static u8 g_src[BUF_BYTES];
alignas(64) static u8 g_dst[BUF_BYTES];

// Total bytes to process per measurement, distributed across reps. Higher =
// less per-measurement noise, more total wall time.
constexpr u64 TARGET_BYTES_PER_MEAS = 1ULL << 28;     // 256 MiB
constexpr u64 MIN_REPS = 16;
constexpr u64 WARMUP_REPS = 8;
constexpr u32 K_MEASUREMENTS = 7;

struct row {
  const char *name;
  u64 size;
  u64 bytes_per_op;
  f64 cyc_per_byte;
  f64 ipc;
  u64 latency_cyc;
  bool perf_ok;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Output formatting

[[gnu::cold]] void
print_header()
{
  micron::io::println("size(B)       routine                bytes/op   cyc/byte    IPC      lat(cyc)");
  micron::io::println("-------------------------------------------------------------------------------");
}

// Format f64 to "X.YY" via integer math (avoids relying on the project's
// f64 printer's default precision, which differs from what we want here).
struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v)
{
  if ( v < 0 ) v = 0;
  u64 scaled = static_cast<u64>(v * 100.0 + 0.5);
  return { scaled / 100, static_cast<u32>(scaled % 100) };
}

[[gnu::cold]] void
print_row(const row &r)
{
  fmt2 cpb = to_fmt2(r.cyc_per_byte);
  fmt2 ipc = to_fmt2(r.ipc);
  const char *cpb_pad = cpb.frac_x100 < 10 ? "0" : "";
  const char *ipc_pad = ipc.frac_x100 < 10 ? "0" : "";

  micron::io::println(r.size, "\t", r.name, "\t", r.bytes_per_op, "\t", cpb.whole, ".", cpb_pad, cpb.frac_x100, "\t",
                      r.perf_ok ? ipc.whole : static_cast<u64>(0), ".", ipc_pad, ipc.frac_x100,
                      r.perf_ok ? "" : " (N/A)", "\t", r.latency_cyc);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Anti-DCE clobber. Forces the compiler to assume `p`'s pointed-to memory
// changed during the asm — prevents hoisting the call out of the loop.

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
clobber_val(u64 v) noexcept
{
  asm volatile("" : : "r"(v));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Median selection over a small array.

u64
median_u64(u64 *xs, u32 n) noexcept
{
  // simple insertion sort — n <= K_MEASUREMENTS
  for ( u32 i = 1; i < n; i++ ) {
    u64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; i++ ) {
    f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Single benchmark cell: run `fn` `reps` times, bracketed by perf
// counters. Returns one (cyc, inst) sample.

struct sample {
  u64 cyc;
  u64 inst;
};

template <typename Fn>
[[gnu::noinline]] sample
measure_once(bench::perf_counter &pc, Fn &&fn, u64 reps) noexcept
{
  if ( pc.ok() ) {
    pc.reset();
    pc.enable();
    for ( u64 i = 0; i < reps; i++ ) fn();
    pc.disable();
    auto rd = pc.read();
    return { rd.cycles, rd.instructions };
  } else {
    u64 t0 = micron::math::__asm_op::rdtsc64();
    for ( u64 i = 0; i < reps; i++ ) fn();
    u64 t1 = micron::math::__asm_op::rdtsc64();
    return { t1 - t0, 0 };
  }
}

template <typename Fn>
row
bench_routine(bench::perf_counter &pc, const char *name, u64 size, u64 bytes_per_op, Fn &&fn) noexcept
{
  // Warmup
  for ( u64 i = 0; i < WARMUP_REPS; i++ ) fn();

  // Calibrate reps to hit ~TARGET_BYTES_PER_MEAS per measurement.
  u64 reps = TARGET_BYTES_PER_MEAS / (bytes_per_op == 0 ? 1 : bytes_per_op);
  if ( reps < MIN_REPS ) reps = MIN_REPS;

  f64 cpb_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  u64 lat_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; m++ ) {
    sample s = measure_once(pc, fn, reps);
    f64 total_bytes = static_cast<f64>(reps) * static_cast<f64>(bytes_per_op);
    cpb_samples[m] = static_cast<f64>(s.cyc) / total_bytes;
    ipc_samples[m] = pc.ok() && s.cyc > 0 ? static_cast<f64>(s.inst) / static_cast<f64>(s.cyc) : 0.0;
    lat_samples[m] = s.cyc / reps;
  }

  return row{
    name, size, bytes_per_op,
    median_f64(cpb_samples, K_MEASUREMENTS),
    median_f64(ipc_samples, K_MEASUREMENTS),
    median_u64(lat_samples, K_MEASUREMENTS),
    pc.ok(),
  };
}

};     // namespace

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Sweeps. Each routine is wrapped in a lambda that calls it on the
// global buffers and applies a clobber to defeat DCE.

static constexpr u64 SIZES[] = {
  16,
  64,
  256,
  1ULL << 10,         // 1 KiB
  4ULL << 10,         // 4 KiB
  64ULL << 10,        // 64 KiB
  1ULL << 20,         // 1 MiB
  16ULL << 20,        // 16 MiB
};

void
sweep_memcpy(bench::perf_counter &pc)
{
  micron::io::println("");
  micron::io::println("[memcpy] dst=src+1MiB, hot cache, dst aligned 64, src aligned 64");
  print_header();

  for ( u64 sz : SIZES ) {
    if ( sz > BUF_BYTES / 2 ) continue;     // need 2 buffers of `sz`

    {
      auto fn = [sz]() {
        mc::memcpy(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "mc::memcpy        ", sz, sz, fn));
    }
    {
      auto fn = [sz]() {
        mc::memcpy128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memcpy128   ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::memcpy256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memcpy256   ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() {
        mc::memcpy512(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memcpy512   ", sz, sz, fn));
    }
#endif
#endif
    {
      auto fn = [sz]() {
        mc::amemcpy128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::amemcpy128  ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::amemcpy256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::amemcpy256  ", sz, sz, fn));
    }
#endif
    if ( sz >= 1024 ) {
      // NT stores only worthwhile for buffers larger than L1.
      auto fn = [sz]() {
        mc::ntmemcpy128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::ntmemcpy128 ", sz, sz, fn));

#if defined(__micron_arch_x86_any)
      auto fn256 = [sz]() {
        mc::ntmemcpy256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::ntmemcpy256 ", sz, sz, fn256));
#endif
    }
  }
}

void
sweep_memset(bench::perf_counter &pc)
{
  micron::io::println("");
  micron::io::println("[memset] dst aligned 64, hot cache");
  print_header();

  for ( u64 sz : SIZES ) {
    {
      auto fn = [sz]() {
        mc::memset(g_dst, static_cast<byte>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "mc::memset        ", sz, sz, fn));
    }
    {
      auto fn = [sz]() {
        mc::memset128(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memset128   ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::memset256(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memset256   ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() {
        mc::memset512(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memset512   ", sz, sz, fn));
    }
#endif
#endif
    {
      auto fn = [sz]() {
        mc::amemset128(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::amemset128  ", sz, sz, fn));
    }
    if ( sz >= 1024 ) {
      auto fn = [sz]() {
        mc::ntmemset128(g_dst, static_cast<u8>(0xAB), sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::ntmemset128 ", sz, sz, fn));
    }
  }
}

void
sweep_memcmp(bench::perf_counter &pc)
{
  micron::io::println("");
  micron::io::println("[memcmp] equal buffers (worst case for early-exit), hot cache");
  print_header();

  // For honest worst-case latency, set src == dst so memcmp must scan the
  // whole buffer (no early exit on first mismatch).
  mc::memcpy(g_dst, g_src, BUF_BYTES);

  static volatile i64 sink = 0;

  for ( u64 sz : SIZES ) {
    {
      auto fn = [sz]() {
        sink = static_cast<i64>(mc::bytecmp(g_src, g_dst, sz));
      };
      print_row(bench_routine(pc, "mc::bytecmp       ", sz, sz, fn));
    }
    {
      auto fn = [sz]() {
        sink = mc::memcmp128(g_src, g_dst, sz);
      };
      print_row(bench_routine(pc, "simd::memcmp128   ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        sink = mc::memcmp256(g_src, g_dst, sz);
      };
      print_row(bench_routine(pc, "simd::memcmp256   ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() {
        sink = mc::memcmp512(g_src, g_dst, sz);
      };
      print_row(bench_routine(pc, "simd::memcmp512   ", sz, sz, fn));
    }
#endif
#endif
    clobber_val(static_cast<u64>(sink));
  }
}

void
sweep_memmove(bench::perf_counter &pc)
{
  micron::io::println("");
  micron::io::println("[memmove] non-overlapping (forward path), hot cache");
  print_header();

  for ( u64 sz : SIZES ) {
    if ( sz > BUF_BYTES / 2 ) continue;

    {
      auto fn = [sz]() {
        mc::memmove(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "mc::memmove       ", sz, sz, fn));
    }
    {
      auto fn = [sz]() {
        mc::memmove128(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memmove128  ", sz, sz, fn));
    }
#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn = [sz]() {
        mc::memmove256(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memmove256  ", sz, sz, fn));
    }
#if defined(__micron_x86_avx512f)
    if ( sz >= 64 ) {
      auto fn = [sz]() {
        mc::memmove512(g_dst, g_src, sz);
        clobber(g_dst);
      };
      print_row(bench_routine(pc, "simd::memmove512  ", sz, sz, fn));
    }
#endif
#endif
  }

  // Overlap path: dst > src, dst-src < sz → backward scan in memmove.
  micron::io::println("");
  micron::io::println("[memmove] overlapping (dst = src + 64, backward path)");
  print_header();

  for ( u64 sz : { (u64)1024, (u64)4096, (u64)65536, (u64)1ULL << 20 } ) {
    auto fn = [sz]() {
      mc::memmove(g_src + 64, g_src, sz);
      clobber(g_src);
    };
    print_row(bench_routine(pc, "mc::memmove[ovlp] ", sz, sz, fn));

    auto fn128 = [sz]() {
      mc::memmove128(g_src + 64, g_src, sz);
      clobber(g_src);
    };
    print_row(bench_routine(pc, "simd::memmove128[ovlp]", sz, sz, fn128));

#if defined(__micron_arch_x86_any)
    if ( sz >= 32 ) {
      auto fn256 = [sz]() {
        mc::memmove256(g_src + 64, g_src, sz);
        clobber(g_src);
      };
      print_row(bench_routine(pc, "simd::memmove256[ovlp]", sz, sz, fn256));
    }
#endif
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int
main(void)
{
  // Pin to CPU 0 to reduce migration noise. Best-effort; ignore failure.
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  // Initialize buffers with deterministic non-zero content.
  for ( u64 i = 0; i < BUF_BYTES; i++ ) {
    g_src[i] = static_cast<u8>(i * 0x9Eu + 0x37u);
    g_dst[i] = static_cast<u8>(i * 0x6Bu + 0x9Du);
  }

  bench::perf_counter pc;
  bool perf_avail = pc.open();

  micron::io::println("=== micron memory benchmark ===");
  micron::io::println("buffers: 16 MiB src + 16 MiB dst, 64-byte aligned");
  micron::io::println("warmup ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements per cell; median reported");
  if ( perf_avail )
    micron::io::println("perf: HW counters CPU_CYCLES + INSTRUCTIONS (exclude_kernel + exclude_hv)");
  else
    micron::io::println("perf: UNAVAILABLE (perf_event_paranoid?), falling back to rdtsc; IPC marked N/A");

  sweep_memcpy(pc);
  sweep_memset(pc);
  sweep_memcmp(pc);
  sweep_memmove(pc);

  micron::io::println("");
  micron::io::println("=== done ===");

  return 0;
}

#pragma GCC pop_options
