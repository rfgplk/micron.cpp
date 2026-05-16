//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Side-by-side hot-allocation benchmark: micron's abcmalloc vs the system
// (glibc) malloc, exercised by the same prefilled random-size workload as
// abcmalloc_hot_bench.cpp so the numbers are directly comparable.
//
// Linkage trick (the part the user warned about):
//   abcmalloc normally interposes ::malloc / ::free / ::calloc / ::realloc /
//   ::aligned_alloc via the extern "C" wrappers in malloc-c.hpp. With those
//   wrappers in place, a plain ::malloc(n) call in this TU would resolve to
//   abc::alloc — making any "system malloc" comparison meaningless. To get
//   the real glibc symbol we define ABCMALLOC_DISABLE *before* including
//   any abcmalloc header — the wrappers in malloc-c.hpp guard themselves
//   with #ifndef ABCMALLOC_DISABLE, and malloc_forward.hpp emits the
//   matching `extern "C"` declarations so we can call ::malloc / ::free
//   directly. abc::alloc / abc::dealloc (the namespaced inline definitions
//   in malloc.hpp) are unaffected by the toggle and remain callable.
//
// Workload: identical to abcmalloc_hot_bench.cpp — a static array of
// random sizes is prefilled once per cell via micron::math::rng (untimed),
// then each allocator's kernel walks the array doing alloc/free pairs.
// Output is a wide single-row layout so abc and sys numbers sit next to
// each other along with the abc/sys ratio.

#define ABCMALLOC_DISABLE 1

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

extern "C" __attribute__((malloc, alloc_size(1))) void *malloc(usize size);
extern "C" void free(void *ptr);

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
  h.s_at("ops/rep", 36);
  h.s_at("abc cyc/op", 48);
  h.s_at("sys cyc/op", 60);
  h.s_at("abc/sys", 70);
  h.s_at("abc IPC", 78);
  h.s_at("sys IPC", 86);
  micron::io::println(h.str());
  micron::io::println("---------------------------------------------------------------------------------");
}

struct sample {
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

struct cell {
  const char *bracket_name;
  u64 count;
  u64 ops_per_rep;
  sample abc;
  sample sys;
};

[[gnu::cold]] void
print_cell(const cell &c)
{
  const fmt2 abc_cpo = to_fmt2(c.abc.cyc_per_op);
  const fmt2 sys_cpo = to_fmt2(c.sys.cyc_per_op);
  const f64 ratio = c.sys.cyc_per_op > 0.0 ? c.abc.cyc_per_op / c.sys.cyc_per_op : 0.0;
  const fmt2 ratio_fmt = to_fmt2(ratio);
  const fmt2 abc_ipc = to_fmt2(c.abc.ipc);
  const fmt2 sys_ipc = to_fmt2(c.sys.ipc);
  line ln;
  ln.s_lj_at(c.bracket_name, 14);
  ln.u_at(c.count, 24);
  ln.u_at(c.ops_per_rep, 36);
  ln.f2_at(abc_cpo, 48);
  ln.f2_at(sys_cpo, 60);
  ln.f2_at(ratio_fmt, 70);
  ln.f2_at(abc_ipc, 78);
  ln.f2_at(sys_ipc, 86);
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

template<typename AllocFn, typename FreeFn>
sample
measure_kernel(u64 count, AllocFn alloc_fn, FreeFn free_fn) noexcept
{

  for ( u64 w = 0; w < WARMUP_PASSES; ++w ) {
    for ( u64 i = 0; i < count; ++i ) {
      void *p = alloc_fn(g_sizes[i]);
      clobber_p(p);
      free_fn(p);
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
      void *p = alloc_fn(g_sizes[i]);
      clobber_p(p);
      free_fn(p);
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

  return sample{ median_f64(cpo_samples, K_MEASUREMENTS), median_f64(ipc_samples, K_MEASUREMENTS), median_f64(bm_samples, K_MEASUREMENTS) };
}

cell
bench_bracket(const bracket &b, u64 count) noexcept
{

  const u64 seed = 0x9E3779B97F4A7C15ULL ^ (count * 0xC6BC279692B5C323ULL) ^ static_cast<u64>(b.hi);
  prefill_sizes(b.lo, b.hi, count, seed);

  auto abc_alloc = [](u32 sz) -> void * { return reinterpret_cast<void *>(abc::alloc(sz)); };
  auto abc_free = [](void *p) -> void { abc::dealloc(reinterpret_cast<byte *>(p)); };
  sample abc_s = measure_kernel(count, abc_alloc, abc_free);

  auto sys_alloc = [](u32 sz) -> void * { return ::malloc(sz); };
  auto sys_free = [](void *p) -> void { ::free(p); };
  sample sys_s = measure_kernel(count, sys_alloc, sys_free);

  return cell{ b.name, count, 2 * count, abc_s, sys_s };
}

void
sweep_compare()
{
  micron::io::println("");
  micron::io::println("[compare] alloc(g_sizes[i]) + free(p) per element — abcmalloc vs glibc malloc, identical prefill");
  print_header();

  for ( const bracket &b : BRACKETS ) {
    for ( u64 n : COUNTS ) {
      cell c = bench_bracket(b, n);
      print_cell(c);

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
    byte *warm_abc = abc::alloc(16);
    abc::dealloc(warm_abc);
    void *warm_sys = ::malloc(16);
    ::free(warm_sys);
  }

  micron::io::println("=== abcmalloc vs glibc malloc — hot-alloc comparison ===");
  micron::io::println("sizes prefilled into static array via micron::math::rng::xoshiro256ss + dist::uniform_int");
  micron::io::println("brackets: 1-8 / 8-32 / 32-128 / 128-512 / 1-512 (full mix)");
  micron::io::println("counts: 5k, 10k, 100k, 1M alloc/free pairs per pass");
  micron::io::println("warmup: ", WARMUP_PASSES, " untimed pass; ", K_MEASUREMENTS, " measured passes per cell (median reported)");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses (bbench 4-event group)");
  micron::io::println("ratio abc/sys: < 1.0 = abcmalloc faster, > 1.0 = glibc faster");
  micron::io::println("note: extern C malloc wrappers in abcmalloc/malloc-c.hpp disabled via ABCMALLOC_DISABLE;");
  micron::io::println("      ::malloc therefore resolves to glibc, abc::alloc remains the abcmalloc path.");

  sweep_compare();

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
