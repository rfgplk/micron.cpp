//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// System (glibc) malloc counterpart to
// abcmalloc_serial_bulk_detailed_bench.
//
// Identical workload + identical output schema, so the rows can be
// diffed cell-for-cell against the abcmalloc detailed numbers: per cell,
// alloc N pointers in a row holding all of them live, walk the pointer
// array once and free everything, then re-run the workload untimed
// while reading the cache events on a separate perf group (PMU-slot
// multiplexing dodge — see the detailed interleaved bench for the
// reasoning).
//
// Linkage: ABCMALLOC_DISABLE before any abcmalloc include keeps the
// extern "C" interposition wrappers out of the link, so ::malloc /
// ::free here resolve to glibc.

#define ABCMALLOC_DISABLE 1

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

extern "C" __attribute__((malloc, alloc_size(1))) void *malloc(usize size);
extern "C" void free(void *ptr);

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Event groups — same split as the abcmalloc detailed counterpart.

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

alignas(64) static u32 g_sizes[MAX_COUNT];
alignas(64) static void *g_ptrs[MAX_COUNT];
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
// Output formatting.

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
  u64 count;
  const char *phase;
  u64 ops;
  u64 p50, p90, p99, p999, max;
  u64 min;
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
// Shellsort + percentile pick.

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

void
prefill(u64 count, u64 seed) noexcept
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = 0; i < count; ++i ) {
    g_sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, SIZE_LO, SIZE_HI);
  }
}

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

struct phase_pair {
  cell_stats alloc;
  cell_stats free_;
  u64 total_bytes;
};

phase_pair
bench_count(u64 count) noexcept
{
  const u64 seed = 0x9E3779B97F4A7C15ULL ^ (count * 0xC6BC279692B5C323ULL) ^ static_cast<u64>(SIZE_HI);
  prefill(count, seed);

  for ( u64 w = 0; w < WARMUP_PASSES; ++w ) {
    for ( u64 i = 0; i < count; ++i ) {
      g_ptrs[i] = ::malloc(g_sizes[i]);
      clobber_p(g_ptrs[i]);
    }
    for ( u64 i = 0; i < count; ++i ) {
      ::free(g_ptrs[i]);
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
      g_ptrs[i] = ::malloc(g_sizes[i]);
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

  // ---- pass 1B: timing for bulk free phase ----
  {
    timing_events tev{ bbench::quiet{} };
    tev.open();
    tev.begin();
    for ( u64 i = 0; i < count; ++i ) {
      const u64 t0 = tsc();
      ::free(g_ptrs[i]);
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

  // ---- pass 2A: cache events for alloc phase (untimed) ----
  {
    cache_events cev{ bbench::quiet{} };
    cev.open();
    cev.begin();
    for ( u64 i = 0; i < count; ++i ) {
      g_ptrs[i] = ::malloc(g_sizes[i]);
      clobber_p(g_ptrs[i]);
    }
    cev.end();
    pp.alloc.llc_miss = static_cast<u64>(cev.get<bbench::llcache_miss>().retrieve());
    pp.alloc.dtlb_miss = static_cast<u64>(cev.get<bbench::dtlb_miss>().retrieve());
  }

  // ---- pass 2B: cache events for bulk free phase (untimed) ----
  {
    cache_events cev{ bbench::quiet{} };
    cev.open();
    cev.begin();
    for ( u64 i = 0; i < count; ++i ) {
      ::free(g_ptrs[i]);
    }
    cev.end();
    pp.free_.llc_miss = static_cast<u64>(cev.get<bbench::llcache_miss>().retrieve());
    pp.free_.dtlb_miss = static_cast<u64>(cev.get<bbench::dtlb_miss>().retrieve());
  }

  pp.total_bytes = total_bytes;
  return pp;
}

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
  micron::io::println("[serial-bulk-detailed] (system malloc) per-op tsc samples — alloc burst + bulk free");
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

  {
    void *w0 = ::malloc(16);
    ::free(w0);
    void *w1 = ::malloc(384);
    ::free(w1);
    void *w2 = ::malloc(2048);
    ::free(w2);
    void *w3 = ::malloc(6000);
    ::free(w3);
  }

  micron::io::println("=== sysmalloc serial-bulk-detailed benchmark ===");
  micron::io::println("allocator: glibc ::malloc / ::free (abcmalloc interposition disabled via ABCMALLOC_DISABLE)");
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
