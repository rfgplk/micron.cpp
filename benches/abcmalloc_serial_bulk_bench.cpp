//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Bulk alloc-then-free abcmalloc benchmark — companion to the hot/
// interleaved benches.  Where those round-trip a single pointer per
// iteration (one alloc + one dealloc back-to-back, peak live ~= one
// block), this one inverts the lifetime: each cell allocates N blocks in
// a row without freeing, then frees the whole batch at the end.  That
// captures a different slice of allocator cost:
//   - alloc phase: sheet exhaustion + sheet bind cost actually shows up
//     here, because the per-class free cache stays empty (no deallocs to
//     refill it).  Forces __vmap_alloc to walk to the arena for new
//     sheets repeatedly.
//   - free phase: bulk-free path — tombstone sweep batching, sheet
//     reclamation, and any back-to-arena promotion all fire in one
//     burst.  Useful for measuring whether the allocator amortises bulk
//     destruction or pays per-block bookkeeping.
//
// Sizes: uniform in [1, 6144] (== 1..6 KiB inclusive).  That spans the
// precise (<=256), small (<=512), medium (<=4096) tiers and dips into
// the large (<=32768) tier — every alloc dispatches differently.
//
// Counts: 1k, 10k, 100k.  Peak live ~= count * 6 KiB worst case, so the
// largest cell pins ~600 MiB live before the bulk-free hits.  Well
// inside any reasonable memory budget.
//
// Output columns (same flavour as abcmalloc_interleaved_bench): one row
// per (count, phase) pair — `phase` is `alloc` or `free`.

#include "../external/bbench/bench.hpp"

#include "../src/cmalloc.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/rng.hpp"
#include "../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../src/memory/allocation/abcmalloc/config.hpp"
#include "../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../src/std.hpp"

namespace
{

using mem_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 5;
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
alignas(64) static byte *g_ptrs[MAX_COUNT];

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
  char buf[256];
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
  h.s_at("ops/rep", 40);
  h.s_at("cyc/op", 52);
  h.s_at("IPC", 62);
  h.s_at("bmiss%", 74);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------");
}

struct cell {
  u64 count;
  const char *phase;
  u64 ops_per_rep;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

[[gnu::cold]] void
print_cell(const cell &c)
{
  line ln;
  ln.s_lj_at(c.phase, 16);
  ln.u_at(c.count, 28);
  ln.u_at(c.ops_per_rep, 40);
  ln.f2_at(to_fmt2(c.cyc_per_op), 52);
  ln.f2_at(to_fmt2(c.ipc), 62);
  ln.f2_at(to_fmt2(c.bmiss_rate * 100.0), 74);
  micron::io::println(ln.str());
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
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

void
prefill(u64 count, u64 seed) noexcept
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = 0; i < count; ++i ) {
    g_sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, SIZE_LO, SIZE_HI);
  }
}

struct phase_result {
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

[[gnu::always_inline]] inline phase_result
__collect(mem_events &evs, u64 ops) noexcept
{
  const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
  const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
  const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
  const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
  return { ops > 0 ? static_cast<f64>(cyc) / static_cast<f64>(ops) : static_cast<f64>(cyc),
           cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0, br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0 };
}

void
bench_count(u64 count, cell &alloc_cell, cell &free_cell) noexcept
{
  const u64 seed = 0x9E3779B97F4A7C15ULL ^ (count * 0xC6BC279692B5C323ULL) ^ static_cast<u64>(SIZE_HI);
  prefill(count, seed);

  for ( u64 w = 0; w < WARMUP_PASSES; ++w ) {
    for ( u64 i = 0; i < count; ++i ) {
      g_ptrs[i] = abc::alloc(g_sizes[i]);
      clobber_p(g_ptrs[i]);
    }
    for ( u64 i = 0; i < count; ++i ) {
      abc::dealloc(g_ptrs[i]);
    }
  }

  f64 a_cpo[K_MEASUREMENTS], a_ipc[K_MEASUREMENTS], a_bm[K_MEASUREMENTS];
  f64 f_cpo[K_MEASUREMENTS], f_ipc[K_MEASUREMENTS], f_bm[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {

    mem_events ae{ bbench::quiet{} };
    ae.open();
    ae.begin();
    for ( u64 i = 0; i < count; ++i ) {
      g_ptrs[i] = abc::alloc(g_sizes[i]);
      clobber_p(g_ptrs[i]);
    }
    ae.end();
    const phase_result ap = __collect(ae, count);
    a_cpo[m] = ap.cyc_per_op;
    a_ipc[m] = ap.ipc;
    a_bm[m] = ap.bmiss_rate;

    mem_events fe{ bbench::quiet{} };
    fe.open();
    fe.begin();
    for ( u64 i = 0; i < count; ++i ) {
      abc::dealloc(g_ptrs[i]);
    }
    fe.end();
    const phase_result fp = __collect(fe, count);
    f_cpo[m] = fp.cyc_per_op;
    f_ipc[m] = fp.ipc;
    f_bm[m] = fp.bmiss_rate;
  }

  alloc_cell = cell{
    count, "alloc (hold)", count, median_f64(a_cpo, K_MEASUREMENTS), median_f64(a_ipc, K_MEASUREMENTS), median_f64(a_bm, K_MEASUREMENTS)
  };
  free_cell = cell{
    count, "bulk-free", count, median_f64(f_cpo, K_MEASUREMENTS), median_f64(f_ipc, K_MEASUREMENTS), median_f64(f_bm, K_MEASUREMENTS)
  };
}

void
sweep_serial_bulk()
{
  micron::io::println("");
  micron::io::println("[serial-bulk] alloc N (no frees) then dealloc all — sizes uniform 1..6 KiB");
  print_header();

  for ( u64 n : COUNTS ) {
    cell ac, fc;
    bench_count(n, ac, fc);
    print_cell(ac);
    print_cell(fc);
    sink(static_cast<u64>(g_sizes[n - 1]));
  }
}

};      // namespace

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

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

  micron::io::println("=== abcmalloc serial-bulk benchmark ===");
  micron::io::println("sizes prefilled into static array via micron::math::rng::xoshiro256ss + dist::uniform_int");
  micron::io::println("size range: 1..6 KiB uniform (spans precise + small + medium tiers + large tier edge)");
  micron::io::println("workload: alloc N pointers in a row (no frees) then dealloc all at the end (bulk free)");
  micron::io::println("warmup: ", WARMUP_PASSES, " untimed full cycle; ", K_MEASUREMENTS, " measured cycles per cell (median reported)");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses (bbench 4-event group)");
  micron::io::println("each alloc and dealloc counts as one op; cyc/op is the per-op cost in that phase only");

  sweep_serial_bulk();

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
