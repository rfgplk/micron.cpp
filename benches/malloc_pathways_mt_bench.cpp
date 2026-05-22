//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Multi-threaded companion to malloc_pathways_bench.cpp: how do abcmalloc's
// per-thread arenas scale against glibc, mimalloc, and jemalloc under concurrent
// load?  Each of T worker threads hammers an allocator with the same pathway;
// the headline metric is aggregate throughput (Mops/s) and its scaling vs T=1.
//
//   [a] hot       — alloc/dealloc round-trip per worker (1 live each)
//   [b] interleav — steady-state churn against a per-worker working set
//   [c] serial    — alloc N (hold), then bulk-free, per worker
//
// Workers are launched with the same mmap-stack micron::thread pool the abc MT
// tests use (run_workers, lifted from tests/support/abc_rigor.hpp), and meet at
// a sense-reversing barrier so the timed region starts together; wall time is
// max(per-worker elapsed).  Per-thread perf counters are summed; per-op rdtsc
// latencies from every worker are merged for global p10/p50/p90/p99/p99.9.
//
// Each allocator via its own symbol (abc::alloc, __libc_malloc, mi_malloc,
// mallocx); ABCMALLOC_DISABLE stops abc interposing the process malloc.

#define ABCMALLOC_DISABLE 1

#include "../external/bbench/bench.hpp"

#include "../src/atomic/atomic.hpp"
#include "../src/bits/__pause.hpp"
#include "../src/chrono.hpp"
#include "../src/cmalloc.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/math/__asm/rdrand.hpp"
#include "../src/math/rng.hpp"
#include "../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../src/memory/allocation/abcmalloc/config.hpp"
#include "../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../src/new.hpp"
#include "../src/std.hpp"
#include "../src/thread/cpu.hpp"
#include "../src/thread/threads.hpp"

extern "C" __attribute__((malloc, alloc_size(1))) void *__libc_malloc(usize size) noexcept;
extern "C" void __libc_free(void *ptr) noexcept;
extern "C" __attribute__((malloc, alloc_size(1))) void *mi_malloc(usize size) noexcept;
extern "C" void mi_free(void *ptr) noexcept;
extern "C" __attribute__((malloc, alloc_size(1))) void *mallocx(usize size, int flags) noexcept;
extern "C" void dallocx(void *ptr, int flags) noexcept;

namespace
{

using mem_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u64 MT_OPS = 200'000ull;
constexpr u64 MT_WORKERS_MAX = 8;
constexpr u64 MT_SLICE = MT_OPS;
constexpr u64 MT_LAT_SLICE = 2ull * MT_OPS;
constexpr u64 MT_LAT_CAP = 65536ull;
constexpr u64 LIVE_BUDGET = 512ull << 20;
constexpr u64 WARMUP_OPS = 4096;
constexpr u32 T_LADDER[] = { 1u, 2u, 4u, 8u };

struct category {
  const char *name;
  u32 lo;
  u32 hi;
};

constexpr category MT_CATS[] = {
  { "tiny", 1u, 32u },
  { "small", 32u, 256u },
  { "medium", 512u, 4096u },
  { "large", 4096u, 32768u },
};

alignas(64) static u32 g_sizes[MT_WORKERS_MAX * MT_SLICE];
alignas(64) static void *g_ptrs[MT_WORKERS_MAX * MT_SLICE];
alignas(64) static u64 g_lat[MT_WORKERS_MAX * MT_LAT_SLICE];
alignas(64) static u64 g_pack[MT_WORKERS_MAX * MT_LAT_SLICE];

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

struct pctl {
  f64 p10, p50, p90, p99, p999, max;
};

pctl
pctl_of(u64 *a, u64 n) noexcept
{
  if ( n == 0 ) return pctl{ 0, 0, 0, 0, 0, 0 };
  const f64 k = g_ns_per_tick;
  return pctl{ static_cast<f64>(pick(a, n, 0.10)) * k, static_cast<f64>(pick(a, n, 0.50)) * k,  static_cast<f64>(pick(a, n, 0.90)) * k,
               static_cast<f64>(pick(a, n, 0.99)) * k, static_cast<f64>(pick(a, n, 0.999)) * k, static_cast<f64>(a[n - 1]) * k };
}

struct barrier {
  micron::atomic_token<u32> count{ 0 };
  micron::atomic_token<u32> gen{ 0 };
  u32 n;

  explicit barrier(u32 nn) noexcept : n(nn) { }
};

[[gnu::always_inline]] inline void
bar_wait(barrier *b) noexcept
{
  if ( b->n <= 1 ) return;
  const u32 g = b->gen.get(micron::memory_order_acquire);
  if ( b->count.fetch_add(1, micron::memory_order_acq_rel) + 1 == b->n ) {
    b->count.store(0, micron::memory_order_relaxed);
    b->gen.fetch_add(1, micron::memory_order_release);
  } else {
    while ( b->gen.get(micron::memory_order_acquire) == g ) __cpu_pause();
  }
}

struct wctx {
  u32 lo, hi;
  u64 ops;
  u64 wset;
  u64 seed;
  u32 *sizes;
  void **ptrs;
  u64 *lat;
  u64 lat_cap;
  barrier *bar;

  u64 cyc, ins, br, bm;
  f64 wall_ns;
  u64 nlat;
};

template<typename Ctx>
inline void
run_workers(void (*fn)(Ctx *), Ctx *ctx, usize n) noexcept
{
  using T = micron::thread<>;
  if ( n > MT_WORKERS_MAX ) n = MT_WORKERS_MAX;
  alignas(T) byte buf[MT_WORKERS_MAX * sizeof(T)];
  T *pool = reinterpret_cast<T *>(buf);
  for ( usize i = 0; i < n; ++i ) ::new (static_cast<void *>(pool + i)) T{ fn, ctx + i };
  for ( usize i = 0; i < n; ++i ) {
    pool[i].join();
    pool[i].~T();
  }
}

[[gnu::always_inline]] inline void
worker_read_perf(mem_events &evs, wctx *c) noexcept
{
  c->cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
  c->ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
  c->br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
  c->bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
}

[[gnu::always_inline]] inline void
worker_prefill(wctx *c) noexcept
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(c->seed);
  for ( u64 i = 0; i < c->ops; ++i ) c->sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, c->lo, c->hi);
}

struct AbcP {
  [[gnu::always_inline]] static void *
  a(u32 s) noexcept
  {
    return reinterpret_cast<void *>(abc::alloc(s));
  }

  [[gnu::always_inline]] static void
  f(void *p) noexcept
  {
    abc::dealloc(reinterpret_cast<byte *>(p));
  }
};

struct SysP {
  [[gnu::always_inline]] static void *
  a(u32 s) noexcept
  {
    return __libc_malloc(s);
  }

  [[gnu::always_inline]] static void
  f(void *p) noexcept
  {
    __libc_free(p);
  }
};

struct MiP {
  [[gnu::always_inline]] static void *
  a(u32 s) noexcept
  {
    return mi_malloc(s);
  }

  [[gnu::always_inline]] static void
  f(void *p) noexcept
  {
    mi_free(p);
  }
};

struct JeP {
  [[gnu::always_inline]] static void *
  a(u32 s) noexcept
  {
    return mallocx(s, 0);
  }

  [[gnu::always_inline]] static void
  f(void *p) noexcept
  {
    dallocx(p, 0);
  }
};

template<class P>
void
worker_hot(wctx *c)
{
  worker_prefill(c);
  for ( u64 i = 0; i < WARMUP_OPS && i < c->ops; ++i ) {
    void *p = P::a(c->sizes[i]);
    clobber_p(p);
    P::f(p);
  }
  mem_events evs{ bbench::quiet{} };
  evs.open();
  bar_wait(c->bar);
  micron::system_clock<micron::system_clocks::monotonic> clk;
  clk.start();
  evs.begin();
  for ( u64 i = 0; i < c->ops; ++i ) {
    void *p = P::a(c->sizes[i]);
    clobber_p(p);
    P::f(p);
  }
  evs.end();
  c->wall_ns = clk.elapsed<micron::unit::nanoseconds>();
  worker_read_perf(evs, c);

  bar_wait(c->bar);
  u64 j = 0;
  for ( u64 i = 0; i < c->ops && j + 2 <= c->lat_cap; ++i ) {
    const u64 t0 = tsc();
    void *p = P::a(c->sizes[i]);
    const u64 t1 = tsc();
    clobber_p(p);
    c->lat[j++] = t1 - t0;
    const u64 t2 = tsc();
    P::f(p);
    const u64 t3 = tsc();
    c->lat[j++] = t3 - t2;
  }
  c->nlat = j;
}

template<class P>
void
worker_interleaved(wctx *c)
{
  worker_prefill(c);
  const u64 W = c->wset;
  for ( u64 i = 0; i < W; ++i ) c->ptrs[i] = nullptr;
  const u64 cseed = c->seed ^ 0x55AA55AA55AA55AAull;

  {
    auto r = micron::math::rng::xoshiro256ss::from_seed(cseed);
    for ( u64 i = 0; i < WARMUP_OPS && i < c->ops; ++i ) {
      const u64 s = micron::math::rng::dist::uniform_int<u64>(r, 0ull, W - 1);
      if ( c->ptrs[s] ) {
        P::f(c->ptrs[s]);
        c->ptrs[s] = nullptr;
      } else {
        void *p = P::a(c->sizes[i]);
        clobber_p(p);
        c->ptrs[s] = p;
      }
    }
    for ( u64 i = 0; i < W; ++i )
      if ( c->ptrs[i] ) {
        P::f(c->ptrs[i]);
        c->ptrs[i] = nullptr;
      }
  }

  mem_events evs{ bbench::quiet{} };
  evs.open();
  bar_wait(c->bar);
  micron::system_clock<micron::system_clocks::monotonic> clk;
  clk.start();
  evs.begin();
  {
    auto r = micron::math::rng::xoshiro256ss::from_seed(cseed);
    for ( u64 i = 0; i < c->ops; ++i ) {
      const u64 s = micron::math::rng::dist::uniform_int<u64>(r, 0ull, W - 1);
      if ( c->ptrs[s] ) {
        P::f(c->ptrs[s]);
        c->ptrs[s] = nullptr;
      } else {
        void *p = P::a(c->sizes[i]);
        clobber_p(p);
        c->ptrs[s] = p;
      }
    }
  }
  evs.end();
  c->wall_ns = clk.elapsed<micron::unit::nanoseconds>();
  worker_read_perf(evs, c);
  for ( u64 i = 0; i < W; ++i )
    if ( c->ptrs[i] ) {
      P::f(c->ptrs[i]);
      c->ptrs[i] = nullptr;
    }

  bar_wait(c->bar);
  {
    auto r = micron::math::rng::xoshiro256ss::from_seed(cseed);
    u64 j = 0;
    for ( u64 i = 0; i < c->ops && j < c->lat_cap; ++i ) {
      const u64 s = micron::math::rng::dist::uniform_int<u64>(r, 0ull, W - 1);
      if ( c->ptrs[s] ) {
        const u64 t0 = tsc();
        P::f(c->ptrs[s]);
        const u64 t1 = tsc();
        c->ptrs[s] = nullptr;
        c->lat[j++] = t1 - t0;
      } else {
        const u64 t0 = tsc();
        void *p = P::a(c->sizes[i]);
        const u64 t1 = tsc();
        clobber_p(p);
        c->ptrs[s] = p;
        c->lat[j++] = t1 - t0;
      }
    }
    c->nlat = j;
  }
  for ( u64 i = 0; i < W; ++i )
    if ( c->ptrs[i] ) {
      P::f(c->ptrs[i]);
      c->ptrs[i] = nullptr;
    }
}

template<class P>
void
worker_serial(wctx *c)
{
  worker_prefill(c);
  {
    for ( u64 i = 0; i < c->ops; ++i ) {
      c->ptrs[i] = P::a(c->sizes[i]);
      clobber_p(c->ptrs[i]);
    }
    for ( u64 i = 0; i < c->ops; ++i ) P::f(c->ptrs[i]);
  }
  mem_events evs{ bbench::quiet{} };
  evs.open();
  bar_wait(c->bar);
  micron::system_clock<micron::system_clocks::monotonic> clk;
  clk.start();
  evs.begin();
  for ( u64 i = 0; i < c->ops; ++i ) {
    c->ptrs[i] = P::a(c->sizes[i]);
    clobber_p(c->ptrs[i]);
  }
  for ( u64 i = 0; i < c->ops; ++i ) P::f(c->ptrs[i]);
  evs.end();
  c->wall_ns = clk.elapsed<micron::unit::nanoseconds>();
  worker_read_perf(evs, c);

  bar_wait(c->bar);
  u64 j = 0;
  for ( u64 i = 0; i < c->ops && j < c->lat_cap; ++i ) {
    const u64 t0 = tsc();
    c->ptrs[i] = P::a(c->sizes[i]);
    const u64 t1 = tsc();
    clobber_p(c->ptrs[i]);
    c->lat[j++] = t1 - t0;
  }
  for ( u64 i = 0; i < c->ops && j < c->lat_cap; ++i ) {
    const u64 t0 = tsc();
    P::f(c->ptrs[i]);
    const u64 t1 = tsc();
    c->lat[j++] = t1 - t0;
  }
  c->nlat = j;
}

struct cfgres {
  u64 total_ops;
  f64 mops;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss;
  pctl lat;
};

static wctx g_ctx[MT_WORKERS_MAX];

cfgres
run_cfg(void (*worker)(wctx *), u32 lo, u32 hi, u32 T, u64 pw_ops, u64 wset, u64 ops_mult, u64 seed) noexcept
{
  barrier bar(T);
  for ( u32 i = 0; i < T; ++i ) {
    wctx &c = g_ctx[i];
    c.lo = lo;
    c.hi = hi;
    c.ops = pw_ops;
    c.wset = wset;
    c.seed = seed ^ (0x9E3779B97F4A7C15ull * (i + 1));
    c.sizes = g_sizes + static_cast<u64>(i) * MT_SLICE;
    c.ptrs = g_ptrs + static_cast<u64>(i) * MT_SLICE;
    c.lat = g_lat + static_cast<u64>(i) * MT_LAT_SLICE;
    c.lat_cap = MT_LAT_CAP;
    c.bar = &bar;
    c.cyc = c.ins = c.br = c.bm = 0;
    c.wall_ns = 0;
    c.nlat = 0;
  }

  run_workers(worker, g_ctx, T);

  u64 sc = 0, si = 0, sb = 0, sm = 0, npack = 0;
  f64 wall = 0;
  for ( u32 i = 0; i < T; ++i ) {
    sc += g_ctx[i].cyc;
    si += g_ctx[i].ins;
    sb += g_ctx[i].br;
    sm += g_ctx[i].bm;
    if ( g_ctx[i].wall_ns > wall ) wall = g_ctx[i].wall_ns;
    for ( u64 k = 0; k < g_ctx[i].nlat; ++k ) g_pack[npack++] = g_ctx[i].lat[k];
  }
  const u64 total_ops = ops_mult * pw_ops * T;

  cfgres r;
  r.total_ops = total_ops;
  r.mops = wall > 0.0 ? static_cast<f64>(total_ops) * 1000.0 / wall : 0.0;
  r.cyc_per_op = total_ops ? static_cast<f64>(sc) / static_cast<f64>(total_ops) : 0.0;
  r.ipc = sc ? static_cast<f64>(si) / static_cast<f64>(sc) : 0.0;
  r.bmiss = sb ? 100.0 * static_cast<f64>(sm) / static_cast<f64>(sb) : 0.0;
  __shellsort<u64>(g_pack, npack);
  r.lat = pctl_of(g_pack, npack);
  return r;
}

struct mtrow {
  const char *impl;
  const char *cat;
  u32 thr;
  u64 total_ops;
  f64 mops;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss;
  f64 ratio;
  f64 scaling;
  pctl lat;
};

static mtrow g_rows[256];
static u32 g_nrows = 0;

void
reset_rows() noexcept
{
  g_nrows = 0;
}

void
add_row(const char *impl, const char *cat, u32 thr, const cfgres &r, f64 ratio) noexcept
{
  mtrow &w = g_rows[g_nrows++];
  w.impl = impl;
  w.cat = cat;
  w.thr = thr;
  w.total_ops = r.total_ops;
  w.mops = r.mops;
  w.cyc_per_op = r.cyc_per_op;
  w.ipc = r.ipc;
  w.bmiss = r.bmiss;
  w.ratio = ratio;
  w.scaling = 0.0;
  w.lat = r.lat;
}

bool
streq(const char *a, const char *b) noexcept
{
  while ( *a && *b ) {
    if ( *a != *b ) return false;
    ++a;
    ++b;
  }
  return *a == *b;
}

void
fill_scaling() noexcept
{
  for ( u32 i = 0; i < g_nrows; ++i ) {
    f64 base = 0.0;
    for ( u32 j = 0; j < g_nrows; ++j ) {
      if ( g_rows[j].thr == 1 && streq(g_rows[j].impl, g_rows[i].impl) && streq(g_rows[j].cat, g_rows[i].cat) ) {
        base = g_rows[j].mops;
        break;
      }
    }
    g_rows[i].scaling = base > 0.0 ? g_rows[i].mops / base : 0.0;
  }
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
  h.s_at("thr", 25);
  h.s_at("total-ops", 37);
  h.s_at("Mops/s", 49);
  h.s_at("scaling", 59);
  h.s_at("cyc/op", 70);
  h.s_at("/abc", 78);
  h.s_at("IPC", 86);
  h.s_at("bmiss%", 95);
  micron::io::println(
      "  throughput  (Mops/s aggregate; scaling = Mops/s vs that allocator's 1-thread run; /abc = impl:abc cyc/op, <1 beats abc)");
  micron::io::println(h.str());
  micron::io::println("  -------------------------------------------------------------------------------------------------");
  for ( u32 i = 0; i < g_nrows; ++i ) {
    const mtrow &r = g_rows[i];
    line ln;
    ln.s_lj_at("  ", 2);
    ln.s_lj_at(r.impl, 10);
    ln.s_lj_at(r.cat, 18);
    ln.u_at(r.thr, 25);
    ln.u_at(r.total_ops, 37);
    ln.f2_at(to_fmt2(r.mops), 49);
    ln.f2_at(to_fmt2(r.scaling), 59);
    ln.f2_at(to_fmt2(r.cyc_per_op), 70);
    if ( r.ratio >= 0.0 ) ln.f2_at(to_fmt2(r.ratio), 78);
    ln.f2_at(to_fmt2(r.ipc), 86);
    ln.f2_at(to_fmt2(r.bmiss), 95);
    emit_row(r.impl, ln.str());
  }
}

[[gnu::cold]] void
print_lat_table()
{
  line h;
  h.s_lj_at("impl", 10);
  h.s_lj_at("cat", 18);
  h.s_at("thr", 25);
  h.s_at("total-ops", 37);
  h.s_at("p10", 49);
  h.s_at("p50", 60);
  h.s_at("p90", 71);
  h.s_at("p99", 83);
  h.s_at("p99.9", 95);
  h.s_at("max", 107);
  micron::io::println("  per-op latency, nanoseconds  (rdtsc per op, merged across all worker threads)");
  micron::io::println(h.str());
  micron::io::println("  ----------------------------------------------------------------------------------------------------------");
  for ( u32 i = 0; i < g_nrows; ++i ) {
    const mtrow &r = g_rows[i];
    line ln;
    ln.s_lj_at("  ", 2);
    ln.s_lj_at(r.impl, 10);
    ln.s_lj_at(r.cat, 18);
    ln.u_at(r.thr, 25);
    ln.u_at(r.total_ops, 37);
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

u32 g_tset[8];
u32 g_ntset = 0;

void
run_hot_mt()
{
  reset_rows();
  for ( const category &c : MT_CATS ) {
    for ( u32 ti = 0; ti < g_ntset; ++ti ) {
      const u32 T = g_tset[ti];
      const u64 seed = 0xA1A1ull ^ (static_cast<u64>(c.hi) << 7) ^ (static_cast<u64>(T) << 1);
      cfgres ra = run_cfg(worker_hot<AbcP>, c.lo, c.hi, T, MT_OPS, 0, 2, seed);
      cfgres rs = run_cfg(worker_hot<SysP>, c.lo, c.hi, T, MT_OPS, 0, 2, seed);
      cfgres rm = run_cfg(worker_hot<MiP>, c.lo, c.hi, T, MT_OPS, 0, 2, seed);
      cfgres rj = run_cfg(worker_hot<JeP>, c.lo, c.hi, T, MT_OPS, 0, 2, seed);
      const f64 ab = ra.cyc_per_op > 0.0 ? ra.cyc_per_op : 1.0;
      add_row("abc", c.name, T, ra, -1.0);
      add_row("sys", c.name, T, rs, rs.cyc_per_op / ab);
      add_row("mi", c.name, T, rm, rm.cyc_per_op / ab);
      add_row("je", c.name, T, rj, rj.cyc_per_op / ab);
    }
  }
  fill_scaling();
  section("[a] HOT alloc/dealloc -- per-worker round-trip, op = single allocator call");
  print_tput_table();
  print_lat_table();
}

void
run_interleaved_mt()
{
  reset_rows();
  for ( const category &c : MT_CATS ) {
    for ( u32 ti = 0; ti < g_ntset; ++ti ) {
      const u32 T = g_tset[ti];
      u64 W = (LIVE_BUDGET / T) / (c.hi ? c.hi : 1u);
      if ( W < 64 ) W = 64;
      if ( W > MT_SLICE ) W = MT_SLICE;
      const u64 seed = 0xD4D4ull ^ (static_cast<u64>(c.hi) << 7) ^ (static_cast<u64>(T) << 1);
      cfgres ra = run_cfg(worker_interleaved<AbcP>, c.lo, c.hi, T, MT_OPS, W, 1, seed);
      cfgres rs = run_cfg(worker_interleaved<SysP>, c.lo, c.hi, T, MT_OPS, W, 1, seed);
      cfgres rm = run_cfg(worker_interleaved<MiP>, c.lo, c.hi, T, MT_OPS, W, 1, seed);
      cfgres rj = run_cfg(worker_interleaved<JeP>, c.lo, c.hi, T, MT_OPS, W, 1, seed);
      const f64 ab = ra.cyc_per_op > 0.0 ? ra.cyc_per_op : 1.0;
      add_row("abc", c.name, T, ra, -1.0);
      add_row("sys", c.name, T, rs, rs.cyc_per_op / ab);
      add_row("mi", c.name, T, rm, rm.cyc_per_op / ab);
      add_row("je", c.name, T, rj, rj.cyc_per_op / ab);
    }
  }
  fill_scaling();
  section("[b] INTERLEAVED -- per-worker steady-state churn (working set <= 512 MiB / T)");
  print_tput_table();
  print_lat_table();
}

void
run_serial_mt()
{
  reset_rows();
  for ( const category &c : MT_CATS ) {
    for ( u32 ti = 0; ti < g_ntset; ++ti ) {
      const u32 T = g_tset[ti];
      u64 pw = LIVE_BUDGET / (static_cast<u64>(T) * c.hi);
      if ( pw > MT_OPS ) pw = MT_OPS;
      if ( pw < 1 ) pw = 1;
      const u64 seed = 0xB2B2ull ^ (static_cast<u64>(c.hi) << 7) ^ (static_cast<u64>(T) << 1);
      cfgres ra = run_cfg(worker_serial<AbcP>, c.lo, c.hi, T, pw, 0, 2, seed);
      cfgres rs = run_cfg(worker_serial<SysP>, c.lo, c.hi, T, pw, 0, 2, seed);
      cfgres rm = run_cfg(worker_serial<MiP>, c.lo, c.hi, T, pw, 0, 2, seed);
      cfgres rj = run_cfg(worker_serial<JeP>, c.lo, c.hi, T, pw, 0, 2, seed);
      const f64 ab = ra.cyc_per_op > 0.0 ? ra.cyc_per_op : 1.0;
      add_row("abc", c.name, T, ra, -1.0);
      add_row("sys", c.name, T, rs, rs.cyc_per_op / ab);
      add_row("mi", c.name, T, rm, rm.cyc_per_op / ab);
      add_row("je", c.name, T, rj, rj.cyc_per_op / ab);
    }
  }
  fill_scaling();
  section("[c] SERIAL alloc -> BULK dealloc -- per worker, alloc N (hold) then free all");
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
  calibrate_tsc();

  {
    byte *wa = abc::alloc(16);
    abc::dealloc(wa);
    __libc_free(__libc_malloc(16));
    mi_free(mi_malloc(16));
    dallocx(mallocx(16, 0), 0);
  }

  u32 ncpu = static_cast<u32>(micron::cpu_count());
  if ( ncpu < 1 ) ncpu = 1;
  for ( u32 t : T_LADDER )
    if ( t <= ncpu && g_ntset < 8 ) g_tset[g_ntset++] = t;
  if ( g_ntset == 0 ) g_tset[g_ntset++] = 1;

  const fmt2 ghz = to_fmt2(g_ns_per_tick > 0.0 ? 1.0 / g_ns_per_tick : 0.0);
  line gl;
  gl.s_lj_at("tsc calibration: ", 17);
  gl.f2_at(ghz, 22);
  gl.s_lj_at(" GHz", 27);

  micron::io::println("=== abcmalloc (TLSF+buddy) vs glibc / mimalloc / jemalloc -- pathway suite (multi-threaded) ===");
  micron::io::println("each worker drives the pathway concurrently; aggregate throughput + scaling vs 1 thread");
  micron::io::print("online cpus: ");
  micron::io::print(ncpu);
  micron::io::print("   thread counts: ");
  for ( u32 i = 0; i < g_ntset; ++i ) micron::io::print(g_tset[i], " ");
  micron::io::println("");
  micron::io::println(gl.str());
  micron::io::print("per-worker ops: ");
  micron::io::println(static_cast<u64>(MT_OPS), " (serial/interleaved capped by 512 MiB live budget)");
  micron::io::println("/abc ratio on sys/mi/je rows: < 1.0 = beats abcmalloc; higher Mops/s + scaling is better");
  micron::io::println("symbols: abc=abc::alloc  glibc=__libc_malloc  mi=mi_malloc  je=mallocx (each its own, no interpose clash)");

  run_hot_mt();
  run_interleaved_mt();
  run_serial_mt();

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
