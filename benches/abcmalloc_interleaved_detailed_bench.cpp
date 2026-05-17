//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Detailed tail-distribution variant of abcmalloc_interleaved_bench.
//
// Where the original interleaved bench reports a single median cyc/op across
// K passes — collapsing the entire alloc/free distribution down to one
// number — this one keeps every per-op cycle sample, sorts it, and prints the
// shape of the tail. For an allocator the median says how fast the fast path
// is; the p99 / p99.9 / max say how often the slow path (sheet promotion,
// arena refill, tier crossover spilling out of the per-class free cache,
// mmap on a fresh sheet) actually fires. That's the number that matters for
// any latency-sensitive caller that lives behind abc::alloc.
//
// Each per-op sample = rdtsc cycles for one alloc + one dealloc round-trip
// (one pointer live at a time). Samples are stored in a flat array sized for
// the largest count; sort happens outside the timed region. We use a single
// long pass per cell instead of K medians so the tail estimates aren't
// smoothed away by inter-pass aggregation.
//
// Brackets — same idea as the original, plus a wider 1-64 KiB bracket that
// crosses every tier including a slice of huge so the cell exercises the
// fast path's worst routing case:
//   "1-32 B"       — precise tier only (control; cyc-tight, branch-stable)
//   "1-512 B"      — precise + small boundary
//   "1-4 KiB"      — + medium boundary
//   "1-16 KiB"     — + large boundary
//   "1-64 KiB"     — + huge boundary (largest fast-path bracket)
//   "interleaved"  — curated cross-tier mix biased to hit every tier per pass
//
// Counts: 5k / 10k / 100k / 1M same as the rest of the abcmalloc suite.
//
// Per-cell output columns:
//   bracket | count | ops/rep | p50 | p90 | p99 | p99.9 | max
//                                     ... | bmiss% | LLC-miss | dTLB-miss | page-faults
//
// All three event-derived columns are whole-cell sums — alloc per-op rates
// would be too small to read at any reasonable column width.  Stick with raw
// totals; the user can divide by `count` mentally if they want a per-op
// figure.  perf multiplexing means several HW counters share PMU slots, so
// these numbers are already scaled (value * time_enabled / time_running).
//
// Extra block per bracket: best-case (cell with lowest observed min single-
// round-trip) and worst-case (cell with the largest p99) summary lines.

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
// Event groups — split into two passes.  The kernel multiplexes counters
// when the requested HW event count exceeds the PMU slot count (~4 on a
// typical amd64); events that never run come back as 0, even after
// scaling.  We keep the *timing* pass tight (just the events we need
// alongside the per-op rdtsc samples) and run a second untimed pass
// purely to populate the cache/TLB counters.

// timing pass: cycles + instructions + branches + branch-misses + page-faults
// (4 HW + 1 SW — all fit on a typical Intel/AMD PMU with the fixed
// counters absorbing cycles/instructions).
using timing_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses,
                                          bbench::page_faults>;

// cache pass: LLC-miss + dTLB-miss (2 HW cache events — guaranteed to fit
// regardless of what else is running).  Page-faults included so we can
// sanity-check that the two passes saw equivalent workloads.
using cache_events = bbench::event_group<bbench::llcache_miss, bbench::dtlb_miss, bbench::page_faults>;

constexpr u64 WARMUP_PASSES = 1;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Bracket descriptors.

enum class mode : u32 {
  uniform,
  tier_interleaved,
};

struct bracket {
  const char *name;
  mode kind;
  u32 lo;
  u32 hi;
};

constexpr bracket BRACKETS[] = {
  { "1-32 B", mode::uniform, 1u, 32u },                   // precise control
  { "1-512 B", mode::uniform, 1u, 512u },                 // precise + small
  { "1-4 KiB", mode::uniform, 1u, 4096u },                // + medium
  { "1-16 KiB", mode::uniform, 1u, 16u * 1024u },         // + large
  { "1-64 KiB", mode::uniform, 1u, 64u * 1024u },         // + huge boundary
  { "interleaved", mode::tier_interleaved, 0u, 0u },      // curated cross-tier mix
};

constexpr u64 COUNTS[] = {
  5'000ULL,
  10'000ULL,
  100'000ULL,
  1'000'000ULL,
};

constexpr u64 MAX_COUNT = 1'000'000ULL;

// Prefilled size table.  64 KiB fits in u32 trivially; 4 MiB BSS.
alignas(64) static u32 g_sizes[MAX_COUNT];

// Per-op latency samples (cycles).  8 MiB BSS at the largest count — sized
// once for the longest cell so we never allocate inside the timed region.
alignas(64) static u64 g_lat[MAX_COUNT];

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Anti-DCE.

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

// rdtsc + memory clobber — serializes the compiler around the timing point.
// On amd64 we don't bother with cpuid serialization; the alloc/dealloc work
// between the two reads is large enough that ooo skew is in the noise for
// p50/p99/max characterization.
[[gnu::always_inline]] inline u64
tsc() noexcept
{
  asm volatile("" ::: "memory");
  const u64 v = micron::math::__asm_op::rdtsc64();
  asm volatile("" ::: "memory");
  return v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Output formatting — line builder w/ fixed columns, same flavor as the
// other micron benches.  Two formatters: fmt2 (two decimal places) and
// integers right-aligned at a target column.

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

// columns: bracket=14 (lj), count=24, ops/rep=36,
// p50=46, p90=56, p99=66, p99.9=78, max=90,
// bmiss%=102, LLCm=116, dTLBm=130, pgflt=144.
[[gnu::cold]] void
print_header()
{
  line h;
  h.s_lj_at("bracket", 14);
  h.s_at("count", 24);
  h.s_at("ops/rep", 36);
  h.s_at("p50", 46);
  h.s_at("p90", 56);
  h.s_at("p99", 66);
  h.s_at("p99.9", 78);
  h.s_at("max", 90);
  h.s_at("bmiss%", 102);
  h.s_at("LLCm", 116);
  h.s_at("dTLBm", 130);
  h.s_at("pgflt", 144);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------------------------------------------------"
                      "------------------------");
}

struct cell_stats {
  // identity
  const char *bracket_name;
  u64 count;
  u64 ops_per_rep;      // == 2 * count

  // tail-distribution (cycles per alloc+free pair)
  u64 p50, p90, p99, p999, max;
  u64 min;      // best-case single round-trip seen in this cell

  // event-derived counters (whole-cell sums)
  f64 bmiss_rate;       // branch_misses / branches (one ratio is small enough to keep)
  u64 llc_miss;         // llcache_miss whole-cell
  u64 dtlb_miss;        // dtlb_miss whole-cell
  u64 page_faults;      // whole-cell sum
};

[[gnu::cold]] void
print_cell(const cell_stats &c)
{
  line ln;
  ln.s_lj_at(c.bracket_name, 14);
  ln.u_at(c.count, 24);
  ln.u_at(c.ops_per_rep, 36);
  ln.u_at(c.p50, 46);
  ln.u_at(c.p90, 56);
  ln.u_at(c.p99, 66);
  ln.u_at(c.p999, 78);
  ln.u_at(c.max, 90);
  ln.f2_at(to_fmt2(c.bmiss_rate * 100.0), 102);
  ln.u_at(c.llc_miss, 116);
  ln.u_at(c.dtlb_miss, 130);
  ln.u_at(c.page_faults, 144);
  micron::io::println(ln.str());
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Quickselect for percentile — sorts in-place enough to expose the picks
// we need.  We just full-sort with insertion for small inputs and a
// shellsort-style gap pattern for large, because we want max + min + 4
// percentiles out of the same buffer; total cost is amortized fine since
// it happens outside the timed region.  For the 1M cell the sort dominates
// post-bench wall time (~50 ms on a modern desktop), which is well below
// what humans wait for anyway.

template<typename T>
inline void
__shellsort(T *a, u64 n) noexcept
{
  // Ciura gap sequence, truncated to what fits in u64.
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
// Size-table prefill — identical layout to abcmalloc_interleaved_bench so
// the two are directly comparable cell-by-cell.

void
prefill_uniform(u32 lo, u32 hi, u64 count, u64 seed) noexcept
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = 0; i < count; ++i ) {
    g_sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, lo, hi);
  }
}

void
prefill_interleaved(u64 count, u64 seed) noexcept
{
  // five representative bands across every tier we exercise.  The huge
  // band (the last) is what makes this distinct from a uniform mix — it
  // forces __vmap_alloc to hit the wide branch each pass.
  struct band {
    u32 lo, hi;
  };

  constexpr band BANDS[] = {
    { 1u, 200u },            // precise tier (sz <= 256)
    { 257u, 480u },          // still precise, past the 256 break
    { 600u, 3500u },         // small tier (sz < 4096)
    { 5000u, 16000u },       // medium tier (sz <= 32768)
    { 33000u, 62000u },      // edge of huge tier (sz <= 262144)
  };
  constexpr u32 BAND_COUNT = sizeof(BANDS) / sizeof(BANDS[0]);

  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = 0; i < count; ++i ) {
    const u32 b = micron::math::rng::dist::uniform_int<u32>(rng, 0u, BAND_COUNT - 1);
    g_sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, BANDS[b].lo, BANDS[b].hi);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Measured pass: for each prefilled size, record rdtsc(alloc+dealloc) into
// g_lat[i].  events run across the whole pass so we get whole-cell sums of
// branch-misses, cache-misses, page-faults, dTLB-misses, LLC-misses.

cell_stats
bench_bracket(const bracket &b, u64 count) noexcept
{
  const u64 seed = 0x9E3779B97F4A7C15ULL ^ (count * 0xC6BC279692B5C323ULL) ^ static_cast<u64>(b.hi)
                   ^ (static_cast<u64>(b.kind) * 0xD1B54A32D192ED03ULL);

  if ( b.kind == mode::uniform )
    prefill_uniform(b.lo, b.hi, count, seed);
  else
    prefill_interleaved(count, seed);

  // warmup: prime tier sheets / per-class free cache so the first few
  // measured samples don't pay for first-time bootstrap of a tier the
  // previous bracket never touched.
  for ( u64 w = 0; w < WARMUP_PASSES; ++w ) {
    for ( u64 i = 0; i < count; ++i ) {
      byte *p = abc::alloc(g_sizes[i]);
      clobber_p(p);
      abc::dealloc(p);
    }
  }

  // ---- pass 1: timing + branches/page-faults ----
  timing_events tev{ bbench::quiet{} };
  tev.open();
  tev.begin();

  for ( u64 i = 0; i < count; ++i ) {
    const u64 t0 = tsc();
    byte *p = abc::alloc(g_sizes[i]);
    clobber_p(p);
    abc::dealloc(p);
    const u64 t1 = tsc();
    g_lat[i] = t1 - t0;
  }

  tev.end();

  const auto br = static_cast<u64>(tev.get<bbench::branches>().retrieve());
  const auto bm = static_cast<u64>(tev.get<bbench::branch_misses>().retrieve());
  const auto pgf = static_cast<u64>(tev.get<bbench::page_faults>().retrieve());

  // ---- pass 2: cache + TLB (untimed; rdtsc samples already captured) ----
  // Re-uses the same prefilled g_sizes so the workload is byte-identical.
  // The per-op rdtsc loop would skew the counts (the rdtsc itself touches
  // the TSC line each iteration), so this pass omits it.
  cache_events cev{ bbench::quiet{} };
  cev.open();
  cev.begin();
  for ( u64 i = 0; i < count; ++i ) {
    byte *p = abc::alloc(g_sizes[i]);
    clobber_p(p);
    abc::dealloc(p);
  }
  cev.end();

  const auto llc_m = static_cast<u64>(cev.get<bbench::llcache_miss>().retrieve());
  const auto dtlb_m = static_cast<u64>(cev.get<bbench::dtlb_miss>().retrieve());

  __shellsort<u64>(g_lat, count);

  cell_stats c{};
  c.bracket_name = b.name;
  c.count = count;
  c.ops_per_rep = 2 * count;
  c.min = count > 0 ? g_lat[0] : 0;
  c.p50 = pick(g_lat, count, 0.50);
  c.p90 = pick(g_lat, count, 0.90);
  c.p99 = pick(g_lat, count, 0.99);
  c.p999 = pick(g_lat, count, 0.999);
  c.max = count > 0 ? g_lat[count - 1] : 0;
  c.bmiss_rate = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  c.llc_miss = llc_m;
  c.dtlb_miss = dtlb_m;
  c.page_faults = pgf;
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Sweep — for each bracket, run every count and print one row per cell.
// Track the per-bracket best/worst cell (best = smallest p50, worst =
// largest p99) so we can summarise the shape after the bracket.

void
print_best_worst(const char *bracket_name, const cell_stats &best, const cell_stats &worst)
{
  micron::io::println("  best-case  (lowest p50):   bracket=", bracket_name, " count=", best.count, " min=", best.min, " p50=", best.p50,
                      " p99=", best.p99);
  micron::io::println("  worst-case (highest p99):  bracket=", bracket_name, " count=", worst.count, " p99=", worst.p99,
                      " p99.9=", worst.p999, " max=", worst.max, " pgflt=", worst.page_faults);
}

void
sweep_interleaved()
{
  micron::io::println("");
  micron::io::println("[interleave-detailed] per-op tsc samples — full distribution + cache/TLB/page-fault counters");
  print_header();

  for ( const bracket &b : BRACKETS ) {
    cell_stats best{};
    cell_stats worst{};
    bool first = true;

    for ( u64 n : COUNTS ) {
      const cell_stats c = bench_bracket(b, n);
      print_cell(c);
      sink(static_cast<u64>(g_sizes[n - 1]));
      sink(c.max);

      if ( first ) {
        best = c;
        worst = c;
        first = false;
      } else {
        if ( c.p50 < best.p50 ) best = c;
        if ( c.p99 > worst.p99 ) worst = c;
      }
    }
    print_best_worst(b.name, best, worst);
    micron::io::println("");
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

  // touch each tier once, including huge, so the first measured cell isn't
  // paying for first-time sheet bootstrap on a cold tier.
  {
    byte *w0 = abc::alloc(16);
    abc::dealloc(w0);
    byte *w1 = abc::alloc(384);
    abc::dealloc(w1);
    byte *w2 = abc::alloc(2048);
    abc::dealloc(w2);
    byte *w3 = abc::alloc(12000);
    abc::dealloc(w3);
    byte *w4 = abc::alloc(48000);
    abc::dealloc(w4);
  }

  micron::io::println("=== abcmalloc interleaved-detailed benchmark ===");
  micron::io::println("per-op latency = rdtsc cycles for one alloc + one dealloc round-trip");
  micron::io::println("brackets: 1-32 B, 1-512 B, 1-4 KiB, 1-16 KiB, 1-64 KiB, interleaved (curated)");
  micron::io::println("counts:   5k, 10k, 100k, 1M");
  micron::io::println("warmup: ", WARMUP_PASSES, " untimed pass per cell; then 1 timing pass + 1 cache-events pass per cell");
  micron::io::println("perf events (2 passes): pass1 = cycles + instructions + branches + branch-misses + page-faults");
  micron::io::println("                        pass2 = LLC-miss + dTLB-miss + page-faults  (PMU-slot multiplexing avoided)");
  micron::io::println("p50/p90/p99/p99.9/max are cycles per alloc+free pair; bmiss% = branch-misses / branches");
  micron::io::println("LLCm + dTLBm + pgflt are whole-cell sums (perf-scaled when multiplexed across PMU slots)");

  sweep_interleaved();

  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
