//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Hot-path abcmalloc benchmark — random-size alloc/free loops.
//
// A static array of random allocation sizes is prefilled once (outside the
// timed region) using micron::math::rng::xoshiro256ss + dist::uniform_int.
// The measured kernel walks that array doing alloc(size) + dealloc(p) per
// element — so the per-cell number captures only the allocator's
// dispatch + per-tier cost, never the RNG.
//
// Two axes:
//   bracket (size range, drives which tier the allocator routes to):
//     "1-8 B"      — sub-cacheline, entirely in the precise tier (2^8 cap)
//     "8-32 B"     — precise tier
//     "32-128 B"   — precise tier
//     "128-512 B"  — straddles precise (<=256) and small (<=512) tiers
//     "1-512 B"    — full mix; the worst case for tier-dispatch branch
//                    prediction since every iteration's tier is random.
//   count (number of alloc/free pairs in one kernel pass):
//     5k, 10k, 100k, 1M
//
// Each kernel pass executes `count` alloc/free pairs back-to-back, with
// only a single pointer live at any moment. Peak working set per cell is
// bounded by the maximum size in the bracket (so the largest cells hold
// <= ~512 B live + arena cache); total RSS stays well under the 4 GiB
// ceiling regardless of `count`.
//
// Output columns: bracket, count, ops/rep (== 2*count), cyc/op (averaged
// across alloc + dealloc), IPC, branch-miss%. Medians across K samples,
// bbench 4-event group.

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

struct bracket {
  const char *name;
  u32 lo;
  u32 hi;
};

constexpr bracket BRACKETS[] = {
  { "1-8 B", 1u, 8u }, { "8-32 B", 8u, 32u }, { "32-128 B", 32u, 128u }, { "128-512 B", 128u, 512u }, { "1-512 B", 1u, 512u },
};

constexpr u64 COUNTS[] = {
  5'000ULL,
  10'000ULL,
  100'000ULL,
  1'000'000ULL,
};

constexpr u64 MAX_COUNT = 1'000'000ULL;
alignas(64) static u32 g_sizes[MAX_COUNT];

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
  h.s_lj_at("bracket", 14);
  h.s_at("count", 24);
  h.s_at("ops/rep", 38);
  h.s_at("cyc/op", 50);
  h.s_at("IPC", 60);
  h.s_at("bmiss%", 72);
  micron::io::println(h.str());
  micron::io::println("---------------------------------------------------------------------");
}

struct cell {
  const char *bracket_name;
  u64 count;
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
  ln.s_lj_at(c.bracket_name, 14);
  ln.u_at(c.count, 24);
  ln.u_at(c.ops_per_rep, 38);
  ln.f2_at(cpo, 50);
  ln.f2_at(ipc, 60);
  ln.f2_at(bm, 72);
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
prefill_sizes(u32 lo, u32 hi, u64 count, u64 seed) noexcept
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(seed);
  for ( u64 i = 0; i < count; ++i ) {
    g_sizes[i] = micron::math::rng::dist::uniform_int<u32>(rng, lo, hi);
  }
}

cell
bench_bracket(const bracket &b, u64 count) noexcept
{

  const u64 seed = 0x9E3779B97F4A7C15ULL ^ (count * 0xC6BC279692B5C323ULL) ^ static_cast<u64>(b.hi);
  prefill_sizes(b.lo, b.hi, count, seed);

  for ( u64 w = 0; w < WARMUP_PASSES; ++w ) {
    for ( u64 i = 0; i < count; ++i ) {
      byte *p = abc::alloc(g_sizes[i]);
      clobber_p(p);
      abc::dealloc(p);
    }
  }

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    mem_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < count; ++i ) {
      byte *p = abc::alloc(g_sizes[i]);
      clobber_p(p);
      abc::dealloc(p);
    }
    evs.end();

    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());

    const f64 total_ops = 2.0 * static_cast<f64>(count);
    cpo_samples[m] = total_ops > 0 ? static_cast<f64>(cyc) / total_ops : static_cast<f64>(cyc);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return cell{ b.name,
               count,
               2 * count,
               median_f64(cpo_samples, K_MEASUREMENTS),
               median_f64(ipc_samples, K_MEASUREMENTS),
               median_f64(bm_samples, K_MEASUREMENTS) };
}

void
sweep_random_hot()
{
  micron::io::println("");
  micron::io::println("[hot] alloc(g_sizes[i]) + dealloc(p) per element, sizes prefilled via xoshiro256ss");
  print_header();

  for ( const bracket &b : BRACKETS ) {
    for ( u64 n : COUNTS ) {
      print_cell(bench_bracket(b, n));

      sink(static_cast<u64>(g_sizes[n - 1]));
    }
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

  micron::io::println("=== abcmalloc hot-alloc benchmark ===");
  micron::io::println("sizes prefilled into static array via micron::math::rng::xoshiro256ss + dist::uniform_int");
  micron::io::println("brackets: 1-8 / 8-32 / 32-128 / 128-512 / 1-512 (full mix)");
  micron::io::println("counts: 5k, 10k, 100k, 1M alloc/free pairs per pass");
  micron::io::println("warmup: ", WARMUP_PASSES, " untimed pass; ", K_MEASUREMENTS, " measured passes per cell (median reported)");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses (bbench 4-event group)");
  micron::io::println("peak live working set per cell <= ~512 B (one pointer live at a time)");

  sweep_random_hot();

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
