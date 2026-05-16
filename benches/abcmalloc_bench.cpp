//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Benchmark harness for the abcmalloc public API (src/memory/allocation/abcmalloc/malloc.hpp).
//
//   sweeps:
//     [round-trip]   alloc + dealloc paired (the fast path) at sizes from
//                    16 B (precise tier) up to 16 MiB (gb tier / mmap).
//     [variants]     salloc / balloc / launder / calloc / aligned_alloc
//                    (alignment 64 and 4096) — all measured as round-trips.
//     [pool]         batched: N allocations under measurement, batched
//                    deallocations under a second measurement — exposes
//                    free-list / tombstone-batching effects.
//     [realloc]      alloc(small) -> realloc(big) -> realloc(small) -> free,
//                    three sizes covering same-tier in-place and cross-tier
//                    move paths.
//     [queries]      cheap predicates / introspection: is_present, within,
//                    query_size, musage.
//
//   per (routine, size) cell — cyc/op, IPC, branch-miss%; medians across
//   K_MEASUREMENTS samples; bbench 4-event group.
//
//   memory budget: peak live working set is held under ~128 MiB per cell
//   (BATCH_BUDGET_BYTES). MAX_BATCH caps the pointer array. Round-trip
//   cells hold only one allocation at a time. Total RSS for the whole run
//   stays well below the user-specified 4 GiB ceiling.

#include "../external/bbench/bench.hpp"

#include "../src/cmalloc.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../src/memory/allocation/abcmalloc/config.hpp"
#include "../src/memory/allocation/abcmalloc/malloc.hpp"
#include "../src/std.hpp"

namespace
{

using mem_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

constexpr u64 SIZES[] = {
  16, 64, 256, 1ULL << 10, 4ULL << 10, 16ULL << 10, 64ULL << 10, 256ULL << 10, 1ULL << 20, 4ULL << 20, 16ULL << 20,
};

constexpr u64 BATCH_BUDGET_BYTES = 32ULL << 20;
constexpr u64 MAX_BATCH = 1024;
constexpr u64 TIER_SAFE_BATCH = 32;

[[gnu::always_inline]] inline u64
round_trip_reps(u64 sz) noexcept
{

  if ( sz <= (4ULL << 10) ) return 4096;
  if ( sz <= (64ULL << 10) ) return 1024;
  if ( sz <= (256ULL << 10) ) return 256;
  if ( sz <= (4ULL << 20) ) return 64;
  return 16;
}

[[gnu::always_inline]] inline u64
batch_for(u64 sz) noexcept
{
  u64 b = BATCH_BUDGET_BYTES / (sz == 0 ? 1 : sz);
  if ( b > MAX_BATCH ) b = MAX_BATCH;

  if ( sz >= (16ULL << 10) && b > TIER_SAFE_BATCH ) b = TIER_SAFE_BATCH;
  if ( b < 1 ) b = 1;
  return b;
}

alignas(64) static byte *g_ptrs[MAX_BATCH];

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
  h.s_at("size(B)", 10);
  h.pad_to(12, 0);
  h.s_lj_at("routine", 36);
  h.s_at("ops/rep", 46);
  h.s_at("cyc/op", 58);
  h.s_at("IPC", 68);
  h.s_at("bmiss%", 78);
  micron::io::println(h.str());
  micron::io::println("------------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_section(const char *title)
{
  micron::io::println("");
  micron::io::println(title);
  print_header();
}

struct cell {
  const char *name;
  u64 size;
  u64 ops_per_rep;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

[[gnu::cold]] void
print_cell(const cell &c)
{
  const fmt2 cpo = to_fmt2(c.cyc_per_op);
  const fmt2 ipc = to_fmt2(c.ipc);
  const fmt2 bm = to_fmt2(c.bmiss_rate * 100.0);
  line ln;
  ln.u_at(c.size, 10);
  ln.pad_to(12, 0);
  ln.s_lj_at(c.name, 36);
  ln.u_at(c.ops_per_rep, 46);
  ln.f2_at(cpo, 58);
  ln.f2_at(ipc, 68);
  ln.f2_at(bm, 78);
  micron::io::println(ln.str());
}

[[gnu::always_inline]] inline void
clobber_p(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
clobber_arr() noexcept
{
  asm volatile("" : : "r"(g_ptrs) : "memory");
}

static volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline void
sink(u64 v) noexcept
{
  sink_u64 += v;
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

template<typename Setup, typename Kernel, typename Cleanup>
[[gnu::noinline]] cell
measure(const char *name, u64 size, u64 ops_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel, Cleanup &&cleanup) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
    setup();
    for ( u64 j = 0; j < reps_per_meas; ++j ) kernel();
    cleanup();
  }

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    setup();

    mem_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
    evs.end();

    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());

    cleanup();

    const f64 total_ops = static_cast<f64>(reps_per_meas) * static_cast<f64>(ops_per_rep);
    cpo_samples[m] = total_ops > 0 ? static_cast<f64>(cyc) / total_ops : static_cast<f64>(cyc);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return cell{ name,
               size,
               ops_per_rep,
               median_f64(cpo_samples, K_MEASUREMENTS),
               median_f64(ipc_samples, K_MEASUREMENTS),
               median_f64(bm_samples, K_MEASUREMENTS) };
}

[[gnu::always_inline]] inline void
nop_setup() noexcept
{
}

[[gnu::always_inline]] inline void
nop_cleanup() noexcept
{
}

void
sweep_round_trip()
{
  print_section("[round-trip] alloc(sz) + dealloc(p) paired — hot path");

  for ( u64 sz : SIZES ) {
    const u64 reps = round_trip_reps(sz);

    auto kernel = [sz]() {
      byte *p = abc::alloc(sz);
      clobber_p(p);
      abc::dealloc(p);
    };
    print_cell(measure("alloc + dealloc", sz, 2, reps, nop_setup, kernel, nop_cleanup));
  }
}

void
sweep_variants()
{
  print_section("[variants] salloc / balloc / launder / calloc / aligned_alloc — round-trip");

  for ( u64 sz : SIZES ) {
    const u64 reps = round_trip_reps(sz);

    {
      auto kernel = [sz]() {
        byte *p = abc::salloc(sz);
        clobber_p(p);
        abc::dealloc(p);
      };
      print_cell(measure("salloc + dealloc", sz, 2, reps, nop_setup, kernel, nop_cleanup));
    }

    {
      auto kernel = [sz]() {
        micron::__chunk<byte> c = abc::balloc(sz);
        clobber_p(c.ptr);
        sink(c.len);
        abc::dealloc(c.ptr);
      };
      print_cell(measure("balloc + dealloc", sz, 2, reps, nop_setup, kernel, nop_cleanup));
    }

    {
      auto kernel = [sz]() {
        byte *p = abc::launder(sz);
        clobber_p(p);
        abc::dealloc(p);
      };
      print_cell(measure("launder + dealloc", sz, 2, reps, nop_setup, kernel, nop_cleanup));
    }

    {
      auto kernel = [sz]() {
        void *p = abc::calloc(1, sz);
        clobber_p(p);
        abc::free(p);
      };
      print_cell(measure("calloc(1,sz) + free", sz, 2, reps, nop_setup, kernel, nop_cleanup));
    }

    if ( sz >= 64 && (sz % 64) == 0 ) {
      auto kernel = [sz]() {
        void *p = abc::aligned_alloc(64, sz);
        clobber_p(p);
        abc::aligned_free(p);
      };
      print_cell(measure("aligned_alloc(64) + a_free", sz, 2, reps, nop_setup, kernel, nop_cleanup));
    }

    if ( sz >= 4096 && (sz % 4096) == 0 ) {
      auto kernel = [sz]() {
        void *p = abc::aligned_alloc(4096, sz);
        clobber_p(p);
        abc::aligned_free(p);
      };
      print_cell(measure("aligned_alloc(4096) + a_free", sz, 2, reps, nop_setup, kernel, nop_cleanup));
    }
  }
}

void
sweep_pool()
{
  print_section("[pool] N allocs then N frees — alloc/free isolated");

  for ( u64 sz : SIZES ) {
    const u64 batch = batch_for(sz);

    {
      auto setup = nop_setup;
      auto kernel = [sz, batch]() {
        for ( u64 i = 0; i < batch; ++i ) g_ptrs[i] = abc::alloc(sz);
        clobber_arr();
      };
      auto cleanup = [batch]() {
        for ( u64 i = 0; i < batch; ++i ) abc::dealloc(g_ptrs[i]);
      };
      print_cell(measure("pool-alloc (alloc x N)", sz, batch, 1, setup, kernel, cleanup));
    }

    {
      auto setup = [sz, batch]() {
        for ( u64 i = 0; i < batch; ++i ) g_ptrs[i] = abc::alloc(sz);
      };
      auto kernel = [batch]() {
        for ( u64 i = 0; i < batch; ++i ) abc::dealloc(g_ptrs[i]);
        clobber_arr();
      };
      auto cleanup = nop_cleanup;
      print_cell(measure("pool-free  (dealloc x N)", sz, batch, 1, setup, kernel, cleanup));
    }

    {
      auto setup = nop_setup;
      auto kernel = [sz, batch]() {
        for ( u64 i = 0; i < batch; ++i ) g_ptrs[i] = abc::alloc(sz);
        clobber_arr();
        for ( u64 i = 0; i < batch; ++i ) abc::dealloc(g_ptrs[i]);
      };
      auto cleanup = nop_cleanup;
      print_cell(measure("pool-roundtrip (a+f) x N", sz, 2 * batch, 1, setup, kernel, cleanup));
    }
  }
}

void
sweep_realloc()
{
  print_section("[realloc] alloc(128) -> realloc(big) -> realloc(128) -> free");

  constexpr u64 SMALL = 128;
  for ( u64 big : SIZES ) {
    if ( big <= SMALL ) continue;
    const u64 reps = round_trip_reps(big);

    auto kernel = [big]() {
      void *p = abc::malloc(SMALL);
      p = abc::realloc(p, big);
      p = abc::realloc(p, SMALL);
      abc::free(p);
    };

    print_cell(measure("malloc/realloc x 2/free", big, 4, reps, nop_setup, kernel, nop_cleanup));
  }
}

void
sweep_queries()
{
  print_section("[queries] is_present / within / query_size / musage");

  for ( u64 sz : SIZES ) {
    const u64 reps = round_trip_reps(sz);

    {
      byte *p = nullptr;
      auto setup = [&p, sz]() { p = abc::alloc(sz); };
      auto kernel = [&p]() {
        bool b = abc::is_present(p);
        sink(static_cast<u64>(b));
      };
      auto cleanup = [&p]() { abc::dealloc(p); };
      print_cell(measure("is_present", sz, 1, reps, setup, kernel, cleanup));
    }

    {
      byte *p = nullptr;
      auto setup = [&p, sz]() { p = abc::alloc(sz); };
      auto kernel = [&p]() {
        bool b = abc::within(p);
        sink(static_cast<u64>(b));
      };
      auto cleanup = [&p]() { abc::dealloc(p); };
      print_cell(measure("within", sz, 1, reps, setup, kernel, cleanup));
    }

    {
      byte *p = nullptr;
      auto setup = [&p, sz]() { p = abc::alloc(sz); };
      auto kernel = [&p]() {
        usize q = abc::query_size(p);
        sink(q);
      };
      auto cleanup = [&p]() { abc::dealloc(p); };
      print_cell(measure("query_size", sz, 1, reps, setup, kernel, cleanup));
    }
  }

  {
    auto setup = nop_setup;
    auto kernel = []() { sink(abc::musage()); };
    auto cleanup = nop_cleanup;
    print_cell(measure("musage()", 0, 1, 4096, setup, kernel, cleanup));
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
    byte *warm = abc::alloc(16);
    abc::dealloc(warm);
  }

  micron::io::println("=== abcmalloc benchmark ===");
  micron::io::println("sizes: 16 B .. 16 MiB (spans precise -> gb tier)");
  micron::io::println("warmup: ", WARMUP_REPS, " kernel reps; ", K_MEASUREMENTS, " measurements per cell (median)");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses (bbench 4-event group)");
  micron::io::println("memory budget: peak live <= 32 MiB per cell; total RSS << 4 GiB");

  sweep_round_trip();
  sweep_variants();
  sweep_pool();
  sweep_realloc();
  sweep_queries();

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
