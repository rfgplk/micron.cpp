//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Single-binary, side-by-side pathway benchmark: micron's abcmalloc (TLSF +
// buddy) vs glibc, mimalloc, and jemalloc.  One workload generator drives all
// four allocators through five access patterns plus abc's launder() path:
//
//   [a] hot       — alloc/dealloc round-trip, one block live at a time
//   [b] serial    — alloc N (hold), then bulk-free all in-order
//   [c] random    — alloc N (hold), then free in a shuffled (non-LIFO) order
//   [d] interleav — steady-state churn against a bounded working set
//   [e] fragment  — punch scattered holes, then re-allocate into them
//   [f] launder   — abc::launder()+dealloc (temporal) vs abc::alloc vs the rest
//
// Each pathway is swept over a size-category x count matrix (categories map to
// abc's tier thresholds: precise/small/medium = TLSF, large/huge = buddy).
//
// Each allocator is reached through its own symbol so all four coexist in one
// TU with no interposition clash: abc::alloc, __libc_malloc (glibc), mi_malloc
// (mimalloc), mallocx (jemalloc).  ABCMALLOC_DISABLE keeps abc from interposing
// the process malloc; glibc is taken via __libc_malloc so it stays a true
// baseline even though linking jemalloc interposes ::malloc.  Every kernel is a
// template on (AllocFn, FreeFn) lambdas so the *identical* loop body inlines.
// Build: needs -ljemalloc -lmimalloc.  duck mis-parses -l flags, so link with
// g++ directly using the CLAUDE.md baseline flags, e.g.:
//   g++ <baseline flags> benches/malloc_pathways_bench.cpp -I./src -L./libs/ \
//       -lpthread -ljemalloc -lmimalloc -o bin/malloc_pathways_bench
//
// Metrics, per (impl, phase) row:
//   throughput table — cyc/op (perf cycles), ns/op + Mops/s (monotonic wall),
//                      IPC, branch-miss%, and /abc = impl cyc/op : abc cyc/op.
//   latency table    — p10 / p50 / p90 / p99 / p99.9 / max per-op latency in ns
//                      (rdtsc per op, TSC calibrated to ns at startup).
// Throughput is a clean (untimed-instrumentation) pass; latency is a separate
// rdtsc-instrumented pass so cyc/op stays uncontaminated by the timer.

#define ABCMALLOC_DISABLE 1      // keep ::malloc / ::free on glibc

#include "../external/bbench/bench.hpp"

#include "../src/chrono.hpp"
#include "../src/cmalloc.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/math/rng.hpp"
#include "../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../src/memory/allocation/abcmalloc/config.hpp"
#include "../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../src/std.hpp"

extern "C" __attribute__((malloc, alloc_size(1))) void *__libc_malloc(usize size) noexcept;
extern "C" void __libc_free(void *ptr) noexcept;
extern "C" __attribute__((malloc, alloc_size(1))) void *mi_malloc(usize size) noexcept;
extern "C" void mi_free(void *ptr) noexcept;
extern "C" __attribute__((malloc, alloc_size(1))) void *mallocx(usize size, int flags) noexcept;
extern "C" void dallocx(void *ptr, int flags) noexcept;

namespace
{

using mem_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 3;
constexpr u64 WARMUP_PASSES = 1;
constexpr u64 LIVE_BUDGET = 512ull << 20;
constexpr u64 PCTL_FLOOR = 20'000ull;

constexpr u64 MAX_COUNT = 1'000'000ull;
constexpr u64 MAX_LAT = 2ull * MAX_COUNT;

struct category {
  const char *name;
  u32 lo;
  u32 hi;
  u64 max_count;
};

constexpr category CATS[] = {
  { "tiny", 1u, 32u, 1'000'000ull },       { "small", 32u, 256u, 1'000'000ull },   { "sub-page", 256u, 512u, 1'000'000ull },
  { "medium", 512u, 4096u, 1'000'000ull }, { "large", 4096u, 32768u, 100'000ull }, { "huge", 32768u, 262144u, 10'000ull },
  { "mixed", 1u, 4096u, 1'000'000ull },
};

constexpr u64 COUNTS[] = { 1'000ull, 10'000ull, 100'000ull, 1'000'000ull };

alignas(64) static u32 g_sizes[MAX_COUNT];
alignas(64) static void *g_ptrs[MAX_COUNT];
alignas(64) static u32 g_perm[MAX_COUNT];
alignas(64) static u64 g_lat[MAX_LAT];

static f64 g_ns_per_tick = 1.0;

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

struct sample {
  f64 cyc_per_op;
  f64 ns_per_op;
  f64 mops;
  f64 ipc;
  f64 bmiss_pct;
};

struct raw {
  u64 cyc;
  u64 ins;
  u64 br;
  u64 bm;
  f64 ns;
};

struct pctl {
  f64 p10, p50, p90, p99, p999, max;
};

struct rstat {
  sample agg;
  pctl lat;
};

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

sample
finalize(const raw &r, u64 ops) noexcept
{
  const f64 fo = ops ? static_cast<f64>(ops) : 1.0;
  sample s;
  s.cyc_per_op = static_cast<f64>(r.cyc) / fo;
  s.ns_per_op = r.ns / fo;
  s.mops = r.ns > 0.0 ? (fo * 1000.0) / r.ns : 0.0;
  s.ipc = r.cyc ? static_cast<f64>(r.ins) / static_cast<f64>(r.cyc) : 0.0;
  s.bmiss_pct = r.br ? 100.0 * static_cast<f64>(r.bm) / static_cast<f64>(r.br) : 0.0;
  return s;
}

struct acc {
  f64 cpo[K_MEASUREMENTS], nspo[K_MEASUREMENTS], mop[K_MEASUREMENTS], ipc[K_MEASUREMENTS], bmp[K_MEASUREMENTS];

  void
  set(u32 m, const sample &s) noexcept
  {
    cpo[m] = s.cyc_per_op;
    nspo[m] = s.ns_per_op;
    mop[m] = s.mops;
    ipc[m] = s.ipc;
    bmp[m] = s.bmiss_pct;
  }

  sample
  med() noexcept
  {
    return sample{ median_f64(cpo, K_MEASUREMENTS), median_f64(nspo, K_MEASUREMENTS), median_f64(mop, K_MEASUREMENTS),
                   median_f64(ipc, K_MEASUREMENTS), median_f64(bmp, K_MEASUREMENTS) };
  }
};

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

pctl
pctl_from(u64 n) noexcept
{
  if ( n == 0 ) return pctl{ 0, 0, 0, 0, 0, 0 };
  const f64 k = g_ns_per_tick;
  return pctl{ static_cast<f64>(pick(g_lat, n, 0.10)) * k,  static_cast<f64>(pick(g_lat, n, 0.50)) * k,
               static_cast<f64>(pick(g_lat, n, 0.90)) * k,  static_cast<f64>(pick(g_lat, n, 0.99)) * k,
               static_cast<f64>(pick(g_lat, n, 0.999)) * k, static_cast<f64>(g_lat[n - 1]) * k };
}

[[gnu::always_inline]] inline void
read_evs(mem_events &evs, raw &out, f64 ns) noexcept
{
  out.cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
  out.ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
  out.br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
  out.bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
  out.ns = ns;
}

template<typename Setup, typename Pass, typename Teardown>
sample
agg_run(Setup setup, Pass pass, Teardown teardown, u64 ops) noexcept
{
  for ( u64 w = 0; w < WARMUP_PASSES; ++w ) {
    setup();
    pass();
    teardown();
  }
  acc ac;
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    setup();
    mem_events evs{ bbench::quiet{} };
    evs.open();
    micron::system_clock<micron::system_clocks::monotonic> clk;
    clk.start();
    evs.begin();
    pass();
    evs.end();
    const f64 ns = clk.elapsed<micron::unit::nanoseconds>();
    teardown();
    raw r;
    read_evs(evs, r, ns);
    ac.set(m, finalize(r, ops));
  }
  return ac.med();
}

template<typename Setup, typename Instr, typename Teardown>
pctl
pctl_run(Setup setup, Instr instr, Teardown teardown, u64 per_pass) noexcept
{

  u64 npasses = per_pass ? (PCTL_FLOOR + per_pass - 1) / per_pass : 1;
  if ( npasses < 1 ) npasses = 1;
  while ( per_pass && npasses > 1 && npasses * per_pass > MAX_LAT ) --npasses;

  u64 nsamp = 0;
  for ( u64 p = 0; p < npasses; ++p ) {
    setup();
    nsamp += instr(nsamp);
    teardown();
    if ( nsamp >= MAX_LAT ) break;
  }
  if ( nsamp == 0 ) return pctl{ 0, 0, 0, 0, 0, 0 };
  __shellsort<u64>(g_lat, nsamp);
  return pctl_from(nsamp);
}

void
prefill_sizes(u32 lo, u32 hi, u64 count, u64 seed) noexcept
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = 0; i < count; ++i ) g_sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, lo, hi);
}

void
build_perm(u64 n, u64 seed) noexcept
{
  for ( u64 i = 0; i < n; ++i ) g_perm[i] = static_cast<u32>(i);
  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = n; i > 1; --i ) {
    const u64 j = micron::math::rng::dist::uniform_int<u64>(rng, 0ull, i - 1);
    const u32 t = g_perm[i - 1];
    g_perm[i - 1] = g_perm[j];
    g_perm[j] = t;
  }
}

[[gnu::always_inline]] inline bool
cap_ok(u32 hi, u64 n) noexcept
{
  return static_cast<u64>(hi) * n <= LIVE_BUDGET;
}

[[gnu::always_inline]] inline u64
seed_for(u32 hi, u64 n, u64 salt) noexcept
{
  return 0x9E3779B97F4A7C15ull ^ (n * 0xC6BC279692B5C323ull) ^ (static_cast<u64>(hi) << 7) ^ salt;
}

[[gnu::always_inline]] inline f64
cyc_ratio(const rstat &a, const rstat &b) noexcept
{
  return b.agg.cyc_per_op > 0.0 ? a.agg.cyc_per_op / b.agg.cyc_per_op : 0.0;
}

template<typename A, typename F>
rstat
measure_roundtrip(u64 count, u32 lo, u32 hi, u64 seed, A a, F f) noexcept
{
  prefill_sizes(lo, hi, count, seed);
  auto noop = []() { };
  auto pass = [&]() {
    for ( u64 i = 0; i < count; ++i ) {
      void *p = a(g_sizes[i]);
      clobber_p(p);
      f(p);
    }
  };
  auto instr = [&](u64 base) -> u64 {
    u64 j = base;
    for ( u64 i = 0; i < count; ++i ) {
      const u64 t0 = tsc();
      void *p = a(g_sizes[i]);
      const u64 t1 = tsc();
      clobber_p(p);
      g_lat[j++] = t1 - t0;
      const u64 t2 = tsc();
      f(p);
      const u64 t3 = tsc();
      g_lat[j++] = t3 - t2;
    }
    return 2 * count;
  };
  rstat r;
  r.agg = agg_run(noop, pass, noop, 2 * count);
  r.lat = pctl_run(noop, instr, noop, 2 * count);
  return r;
}

template<typename A, typename F>
void
measure_two_phase(u64 count, u32 lo, u32 hi, u64 seed, bool rnd, A a, F f, rstat &out_a, rstat &out_f) noexcept
{
  prefill_sizes(lo, hi, count, seed);
  if ( rnd ) build_perm(count, seed ^ 0xABCDEF1234567890ull);
  auto fidx = [&](u64 i) -> u64 { return rnd ? static_cast<u64>(g_perm[i]) : i; };
  auto alloc_all = [&]() {
    for ( u64 i = 0; i < count; ++i ) {
      g_ptrs[i] = a(g_sizes[i]);
      clobber_p(g_ptrs[i]);
    }
  };
  auto free_all = [&]() {
    for ( u64 i = 0; i < count; ++i ) f(g_ptrs[fidx(i)]);
  };
  auto noop = []() { };

  auto alloc_instr = [&](u64 base) -> u64 {
    for ( u64 i = 0; i < count; ++i ) {
      const u64 t0 = tsc();
      g_ptrs[i] = a(g_sizes[i]);
      const u64 t1 = tsc();
      clobber_p(g_ptrs[i]);
      g_lat[base + i] = t1 - t0;
    }
    return count;
  };
  out_a.agg = agg_run(noop, alloc_all, free_all, count);
  out_a.lat = pctl_run(noop, alloc_instr, free_all, count);

  auto free_instr = [&](u64 base) -> u64 {
    for ( u64 i = 0; i < count; ++i ) {
      const u64 ii = fidx(i);
      const u64 t0 = tsc();
      f(g_ptrs[ii]);
      const u64 t1 = tsc();
      g_lat[base + i] = t1 - t0;
    }
    return count;
  };
  out_f.agg = agg_run(alloc_all, free_all, noop, count);
  out_f.lat = pctl_run(alloc_all, free_instr, noop, count);
}

template<typename A, typename F>
rstat
measure_interleaved(u64 opsn, u32 lo, u32 hi, u64 seed, A a, F f) noexcept
{
  u64 W = (64ull << 20) / (hi ? hi : 1u);
  if ( W < 64 ) W = 64;
  if ( W > opsn ) W = opsn;
  if ( W > MAX_COUNT ) W = MAX_COUNT;
  prefill_sizes(lo, hi, opsn, seed);
  for ( u64 i = 0; i < W; ++i ) g_ptrs[i] = nullptr;
  const u64 cseed = seed ^ 0x55AA55AA55AA55AAull;

  auto cleanup = [&]() {
    for ( u64 i = 0; i < W; ++i )
      if ( g_ptrs[i] ) {
        f(g_ptrs[i]);
        g_ptrs[i] = nullptr;
      }
  };
  auto noop = []() { };
  auto pass = [&]() {
    auto rng = micron::math::rng::xoshiro256ss::from_seed(cseed);
    for ( u64 i = 0; i < opsn; ++i ) {
      const u64 slot = micron::math::rng::dist::uniform_int<u64>(rng, 0ull, W - 1);
      if ( g_ptrs[slot] ) {
        f(g_ptrs[slot]);
        g_ptrs[slot] = nullptr;
      } else {
        void *p = a(g_sizes[i]);
        clobber_p(p);
        g_ptrs[slot] = p;
      }
    }
  };
  auto instr = [&](u64 base) -> u64 {
    auto rng = micron::math::rng::xoshiro256ss::from_seed(cseed);
    for ( u64 i = 0; i < opsn; ++i ) {
      const u64 slot = micron::math::rng::dist::uniform_int<u64>(rng, 0ull, W - 1);
      if ( g_ptrs[slot] ) {
        const u64 t0 = tsc();
        f(g_ptrs[slot]);
        const u64 t1 = tsc();
        g_ptrs[slot] = nullptr;
        g_lat[base + i] = t1 - t0;
      } else {
        const u64 t0 = tsc();
        void *p = a(g_sizes[i]);
        const u64 t1 = tsc();
        clobber_p(p);
        g_ptrs[slot] = p;
        g_lat[base + i] = t1 - t0;
      }
    }
    return opsn;
  };
  rstat r;
  r.agg = agg_run(noop, pass, cleanup, opsn);
  r.lat = pctl_run(noop, instr, cleanup, opsn);
  return r;
}

template<typename A, typename F>
rstat
measure_fragmented(u64 N, u32 lo, u32 hi, u64 seed, A a, F f) noexcept
{
  u64 M = N / 2;
  if ( M == 0 ) M = 1;
  prefill_sizes(lo, hi, N, seed);
  build_perm(N, seed ^ 0x0F0F0F0F0F0F0F0Full);

  for ( u64 i = 0; i < N; ++i ) {
    g_ptrs[i] = a(g_sizes[i]);
    clobber_p(g_ptrs[i]);
  }
  for ( u64 k = 0; k < M; ++k ) {
    const u32 idx = g_perm[k];
    f(g_ptrs[idx]);
    g_ptrs[idx] = nullptr;
  }

  auto noop = []() { };
  auto pass = [&]() {
    for ( u64 k = 0; k < M; ++k ) {
      const u32 idx = g_perm[k];
      void *p = a(g_sizes[k]);
      clobber_p(p);
      g_ptrs[idx] = p;
    }
    for ( u64 k = 0; k < M; ++k ) {
      const u32 idx = g_perm[k];
      f(g_ptrs[idx]);
      g_ptrs[idx] = nullptr;
    }
  };
  auto instr = [&](u64 base) -> u64 {
    u64 j = base;
    for ( u64 k = 0; k < M; ++k ) {
      const u32 idx = g_perm[k];
      const u64 t0 = tsc();
      void *p = a(g_sizes[k]);
      const u64 t1 = tsc();
      clobber_p(p);
      g_ptrs[idx] = p;
      g_lat[j++] = t1 - t0;
    }
    for ( u64 k = 0; k < M; ++k ) {
      const u32 idx = g_perm[k];
      const u64 t0 = tsc();
      f(g_ptrs[idx]);
      const u64 t1 = tsc();
      g_ptrs[idx] = nullptr;
      g_lat[j++] = t1 - t0;
    }
    return 2 * M;
  };
  rstat r;
  r.agg = agg_run(noop, pass, noop, 2 * M);
  r.lat = pctl_run(noop, instr, noop, 2 * M);

  for ( u64 i = 0; i < N; ++i )
    if ( g_ptrs[i] ) {
      f(g_ptrs[i]);
      g_ptrs[i] = nullptr;
    }
  return r;
}

struct rowrec {
  const char *impl;
  const char *cat;
  const char *phase;
  u64 count;
  u64 ops;
  sample agg;
  pctl lat;
  f64 ratio;
};

static rowrec g_rows[512];
static u32 g_nrows = 0;

void
reset_rows() noexcept
{
  g_nrows = 0;
}

void
add_row(const char *impl, const char *cat, const char *phase, u64 count, u64 ops, const rstat &r, f64 ratio) noexcept
{
  rowrec &w = g_rows[g_nrows++];
  w.impl = impl;
  w.cat = cat;
  w.phase = phase;
  w.count = count;
  w.ops = ops;
  w.agg = r.agg;
  w.lat = r.lat;
  w.ratio = ratio;
}

[[gnu::always_inline]] inline bool
is_abc_impl(const char *s) noexcept
{
  return s[0] == 'a' && s[1] == 'b' && s[2] == 'c';
}

[[gnu::cold]] void
emit_row(const char *impl, const char *text)
{
  const bool b = is_abc_impl(impl);
  if ( b ) micron::set_color(micron::color::reset, micron::style::bold);
  micron::io::println(text);
  if ( b ) micron::set_color_reset();
}

[[gnu::cold]] void
print_tput_table()
{
  line h;
  h.s_lj_at("impl", 10);
  h.s_lj_at("cat", 18);
  h.s_at("phase", 27);
  h.s_at("count", 37);
  h.s_at("ops", 48);
  h.s_at("cyc/op", 59);
  h.s_at("ns/op", 69);
  h.s_at("Mops/s", 81);
  h.s_at("IPC", 89);
  h.s_at("bmiss%", 98);
  h.s_at("/abc", 106);
  micron::io::println("  throughput  (cyc/op = perf cycles; ns/op + Mops/s = wall; /abc = impl cyc/op : abc cyc/op, <1 beats abc)");
  micron::io::println(h.str());
  micron::io::println("  ----------------------------------------------------------------------------------------------------------");
  for ( u32 i = 0; i < g_nrows; ++i ) {
    const rowrec &r = g_rows[i];
    line ln;
    ln.s_lj_at("  ", 2);
    ln.s_lj_at(r.impl, 10);
    ln.s_lj_at(r.cat, 18);
    ln.s_at(r.phase, 27);
    ln.u_at(r.count, 37);
    ln.u_at(r.ops, 48);
    ln.f2_at(to_fmt2(r.agg.cyc_per_op), 59);
    ln.f2_at(to_fmt2(r.agg.ns_per_op), 69);
    ln.f2_at(to_fmt2(r.agg.mops), 81);
    ln.f2_at(to_fmt2(r.agg.ipc), 89);
    ln.f2_at(to_fmt2(r.agg.bmiss_pct), 98);
    if ( r.ratio >= 0.0 ) ln.f2_at(to_fmt2(r.ratio), 106);
    emit_row(r.impl, ln.str());
  }
}

[[gnu::cold]] void
print_lat_table()
{
  line h;
  h.s_lj_at("impl", 10);
  h.s_lj_at("cat", 18);
  h.s_at("phase", 27);
  h.s_at("count", 37);
  h.s_at("p10", 49);
  h.s_at("p50", 60);
  h.s_at("p90", 71);
  h.s_at("p99", 83);
  h.s_at("p99.9", 95);
  h.s_at("max", 107);
  micron::io::println("  per-op latency, nanoseconds  (rdtsc per op, TSC-calibrated)");
  micron::io::println(h.str());
  micron::io::println("  ----------------------------------------------------------------------------------------------------------");
  for ( u32 i = 0; i < g_nrows; ++i ) {
    const rowrec &r = g_rows[i];
    line ln;
    ln.s_lj_at("  ", 2);
    ln.s_lj_at(r.impl, 10);
    ln.s_lj_at(r.cat, 18);
    ln.s_at(r.phase, 27);
    ln.u_at(r.count, 37);
    ln.f2_at(to_fmt2(r.lat.p10), 49);
    ln.f2_at(to_fmt2(r.lat.p50), 60);
    ln.f2_at(to_fmt2(r.lat.p90), 71);
    ln.f2_at(to_fmt2(r.lat.p99), 83);
    ln.f2_at(to_fmt2(r.lat.p999), 95);
    ln.f2_at(to_fmt2(r.lat.max), 107);
    emit_row(r.impl, ln.str());
  }
}

[[gnu::cold]] void
section(const char *title)
{
  micron::io::println("");
  micron::io::println(title);
}

auto ABC_A = [](u32 s) -> void * { return reinterpret_cast<void *>(abc::alloc(s)); };
auto ABC_F = [](void *p) { abc::dealloc(reinterpret_cast<byte *>(p)); };
auto ABC_LND = [](u32 s) -> void * { return reinterpret_cast<void *>(abc::launder(s)); };
auto SYS_A = [](u32 s) -> void * { return __libc_malloc(s); };
auto SYS_F = [](void *p) { __libc_free(p); };
auto MI_A = [](u32 s) -> void * { return mi_malloc(s); };
auto MI_F = [](void *p) { mi_free(p); };
auto JE_A = [](u32 s) -> void * { return mallocx(s, 0); };
auto JE_F = [](void *p) { dallocx(p, 0); };

void
run_hot()
{
  reset_rows();
  for ( const category &c : CATS ) {
    for ( u64 n : COUNTS ) {
      if ( n > c.max_count ) continue;
      const u64 seed = seed_for(c.hi, n, 0xA1);
      rstat ra = measure_roundtrip(n, c.lo, c.hi, seed, ABC_A, ABC_F);
      rstat rs = measure_roundtrip(n, c.lo, c.hi, seed, SYS_A, SYS_F);
      rstat rm = measure_roundtrip(n, c.lo, c.hi, seed, MI_A, MI_F);
      rstat rj = measure_roundtrip(n, c.lo, c.hi, seed, JE_A, JE_F);
      add_row("abc", c.name, "rt", n, 2 * n, ra, -1.0);
      add_row("sys", c.name, "rt", n, 2 * n, rs, cyc_ratio(rs, ra));
      add_row("mi", c.name, "rt", n, 2 * n, rm, cyc_ratio(rm, ra));
      add_row("je", c.name, "rt", n, 2 * n, rj, cyc_ratio(rj, ra));
      sink(static_cast<u64>(g_sizes[n - 1]));
    }
  }
  section("[a] HOT alloc/dealloc -- round-trip, 1 live; op = single allocator call");
  print_tput_table();
  print_lat_table();
}

void
run_serial_bulk()
{
  reset_rows();
  for ( const category &c : CATS ) {
    for ( u64 n : COUNTS ) {
      if ( n > c.max_count || !cap_ok(c.hi, n) ) continue;
      const u64 seed = seed_for(c.hi, n, 0xB2);
      rstat aA, aF, sA, sF, mA, mF, jA, jF;
      measure_two_phase(n, c.lo, c.hi, seed, false, ABC_A, ABC_F, aA, aF);
      measure_two_phase(n, c.lo, c.hi, seed, false, SYS_A, SYS_F, sA, sF);
      measure_two_phase(n, c.lo, c.hi, seed, false, MI_A, MI_F, mA, mF);
      measure_two_phase(n, c.lo, c.hi, seed, false, JE_A, JE_F, jA, jF);
      add_row("abc", c.name, "alloc", n, n, aA, -1.0);
      add_row("sys", c.name, "alloc", n, n, sA, cyc_ratio(sA, aA));
      add_row("mi", c.name, "alloc", n, n, mA, cyc_ratio(mA, aA));
      add_row("je", c.name, "alloc", n, n, jA, cyc_ratio(jA, aA));
      add_row("abc", c.name, "bulk-free", n, n, aF, -1.0);
      add_row("sys", c.name, "bulk-free", n, n, sF, cyc_ratio(sF, aF));
      add_row("mi", c.name, "bulk-free", n, n, mF, cyc_ratio(mF, aF));
      add_row("je", c.name, "bulk-free", n, n, jF, cyc_ratio(jF, aF));
      sink(static_cast<u64>(g_sizes[n - 1]));
    }
  }
  section("[b] SERIAL alloc -> BULK dealloc -- alloc N (hold), free all in-order");
  print_tput_table();
  print_lat_table();
}

void
run_random()
{
  reset_rows();
  for ( const category &c : CATS ) {
    for ( u64 n : COUNTS ) {
      if ( n > c.max_count || !cap_ok(c.hi, n) ) continue;
      const u64 seed = seed_for(c.hi, n, 0xC3);
      rstat aA, aF, sA, sF, mA, mF, jA, jF;
      measure_two_phase(n, c.lo, c.hi, seed, true, ABC_A, ABC_F, aA, aF);
      measure_two_phase(n, c.lo, c.hi, seed, true, SYS_A, SYS_F, sA, sF);
      measure_two_phase(n, c.lo, c.hi, seed, true, MI_A, MI_F, mA, mF);
      measure_two_phase(n, c.lo, c.hi, seed, true, JE_A, JE_F, jA, jF);
      add_row("abc", c.name, "alloc", n, n, aA, -1.0);
      add_row("sys", c.name, "alloc", n, n, sA, cyc_ratio(sA, aA));
      add_row("mi", c.name, "alloc", n, n, mA, cyc_ratio(mA, aA));
      add_row("je", c.name, "alloc", n, n, jA, cyc_ratio(jA, aA));
      add_row("abc", c.name, "rnd-free", n, n, aF, -1.0);
      add_row("sys", c.name, "rnd-free", n, n, sF, cyc_ratio(sF, aF));
      add_row("mi", c.name, "rnd-free", n, n, mF, cyc_ratio(mF, aF));
      add_row("je", c.name, "rnd-free", n, n, jF, cyc_ratio(jF, aF));
      sink(static_cast<u64>(g_sizes[n - 1]));
    }
  }
  section("[c] RANDOMIZED -- alloc N (hold), free in a shuffled (non-LIFO) order");
  print_tput_table();
  print_lat_table();
}

void
run_interleaved()
{
  reset_rows();
  for ( const category &c : CATS ) {
    for ( u64 n : COUNTS ) {
      if ( n > c.max_count ) continue;
      const u64 seed = seed_for(c.hi, n, 0xD4);
      rstat ra = measure_interleaved(n, c.lo, c.hi, seed, ABC_A, ABC_F);
      rstat rs = measure_interleaved(n, c.lo, c.hi, seed, SYS_A, SYS_F);
      rstat rm = measure_interleaved(n, c.lo, c.hi, seed, MI_A, MI_F);
      rstat rj = measure_interleaved(n, c.lo, c.hi, seed, JE_A, JE_F);
      add_row("abc", c.name, "churn", n, n, ra, -1.0);
      add_row("sys", c.name, "churn", n, n, rs, cyc_ratio(rs, ra));
      add_row("mi", c.name, "churn", n, n, rm, cyc_ratio(rm, ra));
      add_row("je", c.name, "churn", n, n, rj, cyc_ratio(rj, ra));
      sink(static_cast<u64>(g_sizes[n - 1]));
    }
  }
  section("[d] INTERLEAVED -- steady-state churn against a bounded (<=64 MiB) working set");
  print_tput_table();
  print_lat_table();
}

void
run_fragmented()
{
  reset_rows();
  for ( const category &c : CATS ) {
    for ( u64 n : COUNTS ) {
      if ( n > c.max_count || !cap_ok(c.hi, n) ) continue;
      const u64 seed = seed_for(c.hi, n, 0xE5);
      rstat ra = measure_fragmented(n, c.lo, c.hi, seed, ABC_A, ABC_F);
      rstat rs = measure_fragmented(n, c.lo, c.hi, seed, SYS_A, SYS_F);
      rstat rm = measure_fragmented(n, c.lo, c.hi, seed, MI_A, MI_F);
      rstat rj = measure_fragmented(n, c.lo, c.hi, seed, JE_A, JE_F);
      const u64 fops = 2 * (n / 2 ? n / 2 : 1);
      add_row("abc", c.name, "refill", n, fops, ra, -1.0);
      add_row("sys", c.name, "refill", n, fops, rs, cyc_ratio(rs, ra));
      add_row("mi", c.name, "refill", n, fops, rm, cyc_ratio(rm, ra));
      add_row("je", c.name, "refill", n, fops, rj, cyc_ratio(rj, ra));
      sink(static_cast<u64>(g_sizes[n - 1]));
    }
  }
  section("[e] FRAGMENTED -- punch scattered holes (~50%), then re-allocate into them");
  print_tput_table();
  print_lat_table();
}

void
run_launder()
{
  reset_rows();
  for ( const category &c : CATS ) {
    for ( u64 n : COUNTS ) {
      if ( n > c.max_count ) continue;
      const u64 seed = seed_for(c.hi, n, 0xF6);
      rstat rl = measure_roundtrip(n, c.lo, c.hi, seed, ABC_LND, ABC_F);
      rstat rab = measure_roundtrip(n, c.lo, c.hi, seed, ABC_A, ABC_F);
      rstat rs = measure_roundtrip(n, c.lo, c.hi, seed, SYS_A, SYS_F);
      rstat rm = measure_roundtrip(n, c.lo, c.hi, seed, MI_A, MI_F);
      rstat rj = measure_roundtrip(n, c.lo, c.hi, seed, JE_A, JE_F);
      add_row("abc-lnd", c.name, "rt", n, 2 * n, rl, cyc_ratio(rl, rab));
      add_row("abc-alc", c.name, "rt", n, 2 * n, rab, -1.0);
      add_row("sys", c.name, "rt", n, 2 * n, rs, cyc_ratio(rs, rab));
      add_row("mi", c.name, "rt", n, 2 * n, rm, cyc_ratio(rm, rab));
      add_row("je", c.name, "rt", n, 2 * n, rj, cyc_ratio(rj, rab));
      sink(static_cast<u64>(g_sizes[n - 1]));
    }
  }
  section("[f] LAUNDER (temporal) -- abc::launder vs abc::alloc vs glibc/mi/je; /abc = vs abc::alloc");
  micron::io::println("    (temporal overhead = abc-lnd /abc column; baseline row is abc-alc)");
  print_tput_table();
  print_lat_table();
}

[[gnu::cold]] void
calibrate_tsc()
{
  micron::system_clock<micron::system_clocks::monotonic> clk;
  volatile u64 s = 0;
  clk.start();
  const u64 t0 = tsc();
  for ( u64 i = 0; i < 200'000'000ull; ++i ) s = s + i;
  const u64 t1 = tsc();
  const f64 ns = clk.elapsed<micron::unit::nanoseconds>();
  const u64 dt = t1 - t0;
  g_ns_per_tick = dt > 0 ? ns / static_cast<f64>(dt) : 1.0;
  sink(static_cast<u64>(s));
}

};      // namespace

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  calibrate_tsc();

  {
    byte *wa = abc::alloc(16);
    abc::dealloc(wa);
    byte *wl = abc::launder(16);
    abc::dealloc(wl);
    __libc_free(__libc_malloc(16));
    mi_free(mi_malloc(16));
    dallocx(mallocx(16, 0), 0);
  }

  const fmt2 ghz = to_fmt2(g_ns_per_tick > 0.0 ? 1.0 / g_ns_per_tick : 0.0);
  line gl;
  gl.s_lj_at("tsc calibration: ", 17);
  gl.f2_at(ghz, 22);
  gl.s_lj_at(" GHz", 27);

  micron::io::println("=== abcmalloc (TLSF+buddy) vs glibc / mimalloc / jemalloc -- pathway suite (single-threaded) ===");
  micron::io::println("sizes prefilled via micron::math::rng::xoshiro256ss + dist::uniform_int (same seed for all allocators)");
  micron::io::println("categories: tiny/small/sub-page/medium = TLSF, large/huge = buddy; counts: 1k/10k/100k/1M");
  micron::io::println("hold pathways capped at 512 MiB peak live; large/huge counts capped for runtime");
  micron::io::println(gl.str());
  micron::io::println("/abc ratio on sys/mi/je rows: < 1.0 = beats abcmalloc, > 1.0 = abcmalloc faster (abc row = baseline)");
  micron::io::println("symbols: abc=abc::alloc  glibc=__libc_malloc  mi=mi_malloc  je=mallocx (each its own, no interpose clash)");

  run_hot();
  run_serial_bulk();
  run_random();
  run_interleaved();
  run_fragmented();
  run_launder();

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
