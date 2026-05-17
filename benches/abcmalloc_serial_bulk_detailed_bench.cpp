//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Detailed tail-distribution variant of abcmalloc_serial_bulk_bench.
//
// Same workload as the standard serial-bulk bench: per cell, allocate N
// pointers in a row holding every one of them live, then walk the
// pointer array once at the end and free all N.  The standard bench
// reports median cyc/op per phase; this one keeps each individual alloc
// (and each individual free) cycle sample, sorts both, and prints the
// shape of the tail for each phase.  That matters for bulk workloads
// because:
//   - alloc phase: cost climbs in steps every time a sheet exhausts and
//     __vmap_alloc has to grab a fresh one from the arena.  Those bumps
//     hide inside any median; the p99 / p99.9 / max columns surface them.
//   - free phase: bulk free triggers tombstone batching and sheet
//     collapse on the boundaries.  Per-op latency sits flat for most
//     deallocs and spikes whenever the batch threshold trips.
//
// Sizes: uniform [1, 6144].  Counts: 1k / 10k / 100k.  Peak live ~= count
// * 6 KiB worst case (600 MiB at the largest cell — well inside any
// reasonable budget).
//
// Per-cell output: two rows (alloc + free) with columns
//   count | phase | ops | p50 | p90 | p99 | p99.9 | max | bmiss%
//                                       ... | LLCm | dTLBm | pgflt
// followed by a one-line memory summary (bytes allocated, mean size,
// peak-live), and best/worst-case lines per count after both phases.

#include "../external/bbench/bench.hpp"

#include "../src/cmalloc.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/__asm/rdrand.hpp"      // rdtsc64
#include "../src/math/rng.hpp"
#include "../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../src/memory/allocation/abcmalloc/config.hpp"
#include "../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../src/std.hpp"

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Event groups — same two-pass split as the detailed interleaved bench
// to avoid PMU multiplexing zeroing out the cache events.

using timing_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses,
                                          bbench::page_faults>;

using cache_events = bbench::event_group<bbench::llcache_miss, bbench::dtlb_miss, bbench::page_faults>;

constexpr u64 WARMUP_PASSES = 1;

constexpr u32 SIZE_LO = 1u;
constexpr u32 SIZE_HI = 6u * 1024u;

constexpr u64 COUNTS[] = {
  1'000ULL,
  10'000ULL,
  100'000ULL,
};

constexpr u64 MAX_COUNT = 100'000ULL;

// Prefill / pointer table / per-op cycle buffer.  ~2.4 MiB BSS total.
alignas(64) static u32 g_sizes[MAX_COUNT];
alignas(64) static byte *g_ptrs[MAX_COUNT];
alignas(64) static u64 g_lat[MAX_COUNT];

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Anti-DCE + rdtsc.

[[gnu::always_inline]] inline void
clobber_p(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

static volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline void
sink(u64 v) noexcept
{
  sink_u64 += v;
}

[[gnu::always_inline]] inline u64
tsc() noexcept
{
  asm volatile("" ::: "memory");
  const u64 v = micron::math::__asm_op::rdtsc64();
  asm volatile("" ::: "memory");
  return v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Output formatting (matches abcmalloc_interleaved_detailed_bench).

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 s = static_cast<u64>(v * 100.0 + 0.5);
  return { s / 100, static_cast<u32>(s % 100) };
}

struct line {
  char buf[384];
  u32 pos;

  constexpr line() noexcept : pos(0) { }

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  pad_to(u32 end_col, u32 written) noexcept
  {
    const u32 want = end_col >= written ? end_col - written : 0;
    if ( want < pos )
      buf[pos++] = ' ';
    else
      while ( pos < want ) buf[pos++] = ' ';
  }

  void
  u_at(u64 v, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else {
      u64 vv = v;
      while ( vv ) {
        tmp[n++] = '0' + (vv % 10);
        vv /= 10;
      }
    }
    pad_to(end_col, n);
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      tmp[n++] = '0';
    else
      while ( w ) {
        tmp[n++] = '0' + (w % 10);
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = tmp[--n];
    buf[pos++] = '.';
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 / 10);
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 % 10);
  }

  void
  s_at(const char *p, u32 end_col) noexcept
  {
    u32 n = 0;
    while ( p[n] ) ++n;
    pad_to(end_col, n);
    while ( *p ) buf[pos++] = *p++;
  }

  void
  s_lj_at(const char *p, u32 end_col) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
    while ( pos < end_col ) buf[pos++] = ' ';
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

// columns: phase=16 (lj), count=28, ops=40,
// p50=50, p90=60, p99=70, p99.9=82, max=94,
// bmiss%=106, LLCm=120, dTLBm=134, pgflt=148.
[[gnu::cold]] void
print_header()
{
  line h;
  h.s_lj_at("phase", 16);
  h.s_at("count", 28);
  h.s_at("ops", 40);
  h.s_at("p50", 50);
  h.s_at("p90", 60);
  h.s_at("p99", 70);
  h.s_at("p99.9", 82);
  h.s_at("max", 94);
  h.s_at("bmiss%", 106);
  h.s_at("LLCm", 120);
  h.s_at("dTLBm", 134);
  h.s_at("pgflt", 148);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------------------------------------------------"
                      "------------------------------------");
}

struct cell_stats {
  // identity
  u64 count;
  const char *phase;
  u64 ops;

  // tail-distribution (cycles per op in this phase only)
  u64 p50, p90, p99, p999, max;
  u64 min;

  // event-derived counters (whole-phase sums; bmiss% is a ratio)
  f64 bmiss_rate;
  u64 llc_miss;
  u64 dtlb_miss;
  u64 page_faults;
};

[[gnu::cold]] void
print_cell(const cell_stats &c)
{
  line ln;
  ln.s_lj_at(c.phase, 16);
  ln.u_at(c.count, 28);
  ln.u_at(c.ops, 40);
  ln.u_at(c.p50, 50);
  ln.u_at(c.p90, 60);
  ln.u_at(c.p99, 70);
  ln.u_at(c.p999, 82);
  ln.u_at(c.max, 94);
  ln.f2_at(to_fmt2(c.bmiss_rate * 100.0), 106);
  ln.u_at(c.llc_miss, 120);
  ln.u_at(c.dtlb_miss, 134);
  ln.u_at(c.page_faults, 148);
  micron::io::println(ln.str());
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Shellsort (Ciura) + percentile pick — same as the detailed
// interleaved bench.

template<typename T>
inline void
__shellsort(T *a, u64 n) noexcept
{
  static constexpr u64 gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1 };
  for ( u64 gi = 0; gi < sizeof(gaps) / sizeof(gaps[0]); ++gi ) {
    const u64 gap = gaps[gi];
    if ( gap >= n ) continue;
    for ( u64 i = gap; i < n; ++i ) {
      T tmp = a[i];
      u64 j = i;
      while ( j >= gap && a[j - gap] > tmp ) {
        a[j] = a[j - gap];
        j -= gap;
      }
      a[j] = tmp;
    }
  }
}

[[gnu::always_inline]] inline u64
pick(u64 *sorted, u64 n, f64 q) noexcept
{
  if ( n == 0 ) return 0;
  u64 idx = static_cast<u64>(q * static_cast<f64>(n - 1) + 0.5);
  if ( idx >= n ) idx = n - 1;
  return sorted[idx];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Prefill.  Uniform 1..6 KiB.

void
prefill(u64 count, u64 seed) noexcept
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = 0; i < count; ++i ) {
    g_sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, SIZE_LO, SIZE_HI);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Helpers: fill a cell_stats from the sorted latency buffer + event
// totals from the timing-events pass.

void
__fill_stats(cell_stats &c, u64 count, u64 ops, const char *phase, u64 br, u64 bm, u64 pgf) noexcept
{
  c.count = count;
  c.phase = phase;
  c.ops = ops;
  c.min = count > 0 ? g_lat[0] : 0;
  c.p50 = pick(g_lat, count, 0.50);
  c.p90 = pick(g_lat, count, 0.90);
  c.p99 = pick(g_lat, count, 0.99);
  c.p999 = pick(g_lat, count, 0.999);
  c.max = count > 0 ? g_lat[count - 1] : 0;
  c.bmiss_rate = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  c.page_faults = pgf;
  c.llc_miss = 0;
  c.dtlb_miss = 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// One cell = one alloc burst (per-op tsc samples + perf) then one bulk
// free pass (per-op tsc samples + perf), then an untimed alloc+free
// pair for the cache-events pass.  total_bytes / mean / peak filled in
// from the prefilled g_sizes.

struct phase_pair {
  cell_stats alloc;
  cell_stats free_;
  u64 total_bytes;      // sum of g_sizes[i]
};

phase_pair
bench_count(u64 count) noexcept
{
  const u64 seed = 0x9E3779B97F4A7C15ULL ^ (count * 0xC6BC279692B5C323ULL) ^ static_cast<u64>(SIZE_HI);
  prefill(count, seed);

  // warmup full cycle so sheet bootstrap doesn't land in the first
  // measured sample.
  for ( u64 w = 0; w < WARMUP_PASSES; ++w ) {
    for ( u64 i = 0; i < count; ++i ) {
      g_ptrs[i] = abc::alloc(g_sizes[i]);
      clobber_p(g_ptrs[i]);
    }
    for ( u64 i = 0; i < count; ++i ) {
      abc::dealloc(g_ptrs[i]);
    }
  }

  u64 total_bytes = 0;
  for ( u64 i = 0; i < count; ++i ) total_bytes += g_sizes[i];

  phase_pair pp{};

  // ---- pass 1A: timing for alloc phase ----
  {
    timing_events tev{ bbench::quiet{} };
    tev.open();
    tev.begin();
    for ( u64 i = 0; i < count; ++i ) {
      const u64 t0 = tsc();
      g_ptrs[i] = abc::alloc(g_sizes[i]);
      clobber_p(g_ptrs[i]);
      const u64 t1 = tsc();
      g_lat[i] = t1 - t0;
    }
    tev.end();
    const auto br = static_cast<u64>(tev.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(tev.get<bbench::branch_misses>().retrieve());
    const auto pgf = static_cast<u64>(tev.get<bbench::page_faults>().retrieve());
    __shellsort<u64>(g_lat, count);
    __fill_stats(pp.alloc, count, count, "alloc (hold)", br, bm, pgf);
  }

  // ---- pass 1B: timing for bulk free phase (pointers still live) ----
  {
    timing_events tev{ bbench::quiet{} };
    tev.open();
    tev.begin();
    for ( u64 i = 0; i < count; ++i ) {
      const u64 t0 = tsc();
      abc::dealloc(g_ptrs[i]);
      const u64 t1 = tsc();
      g_lat[i] = t1 - t0;
    }
    tev.end();
    const auto br = static_cast<u64>(tev.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(tev.get<bbench::branch_misses>().retrieve());
    const auto pgf = static_cast<u64>(tev.get<bbench::page_faults>().retrieve());
    __shellsort<u64>(g_lat, count);
    __fill_stats(pp.free_, count, count, "bulk-free", br, bm, pgf);
  }

  // ---- pass 2: cache events for alloc phase (untimed) ----
  {
    cache_events cev{ bbench::quiet{} };
    cev.open();
    cev.begin();
    for ( u64 i = 0; i < count; ++i ) {
      g_ptrs[i] = abc::alloc(g_sizes[i]);
      clobber_p(g_ptrs[i]);
    }
    cev.end();
    pp.alloc.llc_miss = static_cast<u64>(cev.get<bbench::llcache_miss>().retrieve());
    pp.alloc.dtlb_miss = static_cast<u64>(cev.get<bbench::dtlb_miss>().retrieve());
  }

  // ---- pass 3: cache events for bulk free phase (untimed) ----
  {
    cache_events cev{ bbench::quiet{} };
    cev.open();
    cev.begin();
    for ( u64 i = 0; i < count; ++i ) {
      abc::dealloc(g_ptrs[i]);
    }
    cev.end();
    pp.free_.llc_miss = static_cast<u64>(cev.get<bbench::llcache_miss>().retrieve());
    pp.free_.dtlb_miss = static_cast<u64>(cev.get<bbench::dtlb_miss>().retrieve());
  }

  pp.total_bytes = total_bytes;
  return pp;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void
print_mem_summary(u64 count, u64 total_bytes)
{
  const u64 mean_b = count > 0 ? total_bytes / count : 0;
  micron::io::println("  [mem] count=", count, " total=", total_bytes, " B (", total_bytes / 1024, " KiB)", " mean/alloc=", mean_b, " B",
                      " peak-live <= ", count * static_cast<u64>(SIZE_HI), " B");
}

void
print_best_worst(const phase_pair &pp)
{
  micron::io::println("  alloc best  (min single-op): ", pp.alloc.min, " cyc;  worst (max): ", pp.alloc.max, " cyc;  p99=", pp.alloc.p99,
                      " p99.9=", pp.alloc.p999);
  micron::io::println("  free  best  (min single-op): ", pp.free_.min, " cyc;  worst (max): ", pp.free_.max, " cyc;  p99=", pp.free_.p99,
                      " p99.9=", pp.free_.p999);
}

void
sweep_serial_bulk()
{
  micron::io::println("");
  micron::io::println("[serial-bulk-detailed] per-op tsc samples — alloc burst + bulk free, full tail distribution");
  print_header();

  for ( u64 n : COUNTS ) {
    const phase_pair pp = bench_count(n);
    print_cell(pp.alloc);
    print_cell(pp.free_);
    print_mem_summary(n, pp.total_bytes);
    print_best_worst(pp);
    micron::io::println("");
    sink(static_cast<u64>(g_sizes[n - 1]));
    sink(pp.alloc.max);
    sink(pp.free_.max);
  }
}

};      // namespace

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  // touch each tier the bracket covers once.
  {
    byte *w0 = abc::alloc(16);
    abc::dealloc(w0);
    byte *w1 = abc::alloc(384);
    abc::dealloc(w1);
    byte *w2 = abc::alloc(2048);
    abc::dealloc(w2);
    byte *w3 = abc::alloc(6000);
    abc::dealloc(w3);
  }

  micron::io::println("=== abcmalloc serial-bulk-detailed benchmark ===");
  micron::io::println("per-op latency = rdtsc cycles for one alloc (or one dealloc) — phases timed separately");
  micron::io::println("size range: 1..6 KiB uniform; counts: 1k, 10k, 100k");
  micron::io::println("workload: alloc N pointers in a row (no frees) then dealloc all at the end (bulk free)");
  micron::io::println("warmup: ", WARMUP_PASSES, " untimed full cycle per cell; then 2 timing passes + 2 cache-events passes per cell");
  micron::io::println("perf events (2 passes): pass1 = cycles + instructions + branches + branch-misses + page-faults");
  micron::io::println("                        pass2 = LLC-miss + dTLB-miss + page-faults  (PMU-slot multiplexing avoided)");
  micron::io::println("p50/p90/p99/p99.9/max are cycles per op in the named phase only");
  micron::io::println("LLCm + dTLBm + pgflt are whole-phase sums; mem summary line reports total bytes + peak-live upper bound");

  sweep_serial_bulk();

  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
