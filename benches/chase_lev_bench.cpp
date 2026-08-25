//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/queue/chase_lev.hpp"
#include "../src/std.hpp"
#include "../src/thread/thread.hpp"

#if defined(CHASE_LEV_BENCH_PMU)
#include "../external/bbench/bench.hpp"
#endif

namespace
{

constexpr u32 K_MEASUREMENTS = 7;
constexpr u32 WARMUP = 2;

f64 g_tsc_ghz = 1.0;
volatile u64 g_sink = 0;
const char *g_case = nullptr;
u32 g_measurements = K_MEASUREMENTS;
u32 g_cpus[32] = { 0, 1, 2, 3, 4, 5, 6, 7 };
u32 g_cpu_count = 8;
bool g_cpu_set_explicit = false;

bool
eq(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

u64
number(const char *s) noexcept
{
  u64 value = 0;
  while ( *s >= '0' && *s <= '9' ) value = value * 10u + static_cast<u64>(*s++ - '0');
  return value;
}

bool
selected(const char *group, const char *leaf) noexcept
{
  return g_case == nullptr || eq(g_case, group) || eq(g_case, leaf);
}

bool
parse_cpu_list(const char *s) noexcept
{
  u32 count = 0;
  while ( *s ) {
    if ( count == 32 || *s < '0' || *s > '9' ) return false;
    u64 cpu = 0;
    while ( *s >= '0' && *s <= '9' ) cpu = cpu * 10u + static_cast<u64>(*s++ - '0');
    if ( cpu >= 1024 ) return false;
    g_cpus[count++] = static_cast<u32>(cpu);
    if ( *s == '\0' ) break;
    if ( *s++ != ',' ) return false;
  }
  if ( count == 0 ) return false;
  g_cpu_count = count;
  g_cpu_set_explicit = true;
  return true;
}

[[gnu::always_inline]] inline u64
rdtsc() noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  u32 lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
#else
  u64 v;
  asm volatile("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#endif
}

[[gnu::always_inline]] inline u64
wall_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

void
calibrate_tsc() noexcept
{
  const u64 n0 = wall_ns();
  const u64 c0 = rdtsc();
  while ( wall_ns() - n0 < 50000000ULL ) {
  }
  const u64 c1 = rdtsc();
  const u64 n1 = wall_ns();
  g_tsc_ghz = static_cast<f64>(c1 - c0) / static_cast<f64>(n1 - n0);
}

[[gnu::always_inline]] inline f64
cyc_to_ns(u64 cyc) noexcept
{
  return static_cast<f64>(cyc) / g_tsc_ghz;
}

[[gnu::always_inline]] inline void
clobber(u64 v) noexcept
{
  asm volatile("" : : "r"(v) : "memory");
}

f64
median(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 k = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > k ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = k;
  }
  return xs[n / 2];
}

void
row(const char *group, const char *name, u64 n, f64 v, const char *unit) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 centi = static_cast<u64>(v * 100.0 + 0.5);
  micron::io::print("  ", group, "  ", name);
  for ( usize i = micron::strlen(name); i < 18; ++i ) micron::io::print(" ");
  micron::io::print("N=", n);
  for ( usize i = 0; i < (n < 10000 ? 6u : n < 10000000 ? 3u : 1u); ++i ) micron::io::print(" ");
  micron::io::print(centi / 100, ".", (centi % 100 < 10 ? "0" : ""), centi % 100, " ", unit, "\n");
}

template<typename Fn>
inline void
parallel(int n, Fn fn)
{
  micron::__thread_pointer<micron::auto_thread<>> ts[32];
  for ( int i = 0; i < n; ++i ) ts[i] = micron::solo::spawn([fn, i]() { fn(i); });
  for ( int i = 0; i < n; ++i ) micron::solo::join(ts[i]);
}

inline void
pin(u32 cpu) noexcept
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(cpu);
  micron::posix::sched_setaffinity(0, sizeof(set), set);
}

#if defined(CHASE_LEV_BENCH_FIXED)
using deque_t = micron::chase_lev<u64, 1024>;
constexpr const char *K_VARIANT = "chase_lev (fixed)";
constexpr const char *K_IMPL = "fixed";
#else
using deque_t = micron::chase_lev_grow<u64, 1024>;
constexpr const char *K_VARIANT = "chase_lev_grow (runtime default)";
constexpr const char *K_IMPL = "grow";
#endif

f64
bench_pingpong(u64 n) noexcept
{
  f64 s[K_MEASUREMENTS];
  for ( u32 m = 0; m < g_measurements + WARMUP; ++m ) {
    deque_t d;
    u64 acc = 0;
    const u64 c0 = rdtsc();
    for ( u64 i = 1; i <= n; ++i ) {
      d.push_bottom(i);
      acc += d.pop_bottom();
    }
    const u64 c1 = rdtsc();
    clobber(acc);
    g_sink += acc;
    if ( m >= WARMUP ) s[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(n);
  }
  return median(s, g_measurements);
}

f64
bench_deep(u64 depth, u64 rounds) noexcept
{
  f64 s[K_MEASUREMENTS];
  for ( u32 m = 0; m < g_measurements + WARMUP; ++m ) {
    deque_t d;
    u64 acc = 0;
    const u64 c0 = rdtsc();
    for ( u64 r = 0; r < rounds; ++r ) {
      for ( u64 i = 1; i <= depth; ++i ) d.push_bottom(i);
      for ( u64 i = 0; i < depth; ++i ) acc += d.pop_bottom();
    }
    const u64 c1 = rdtsc();
    clobber(acc);
    g_sink += acc;
    if ( m >= WARMUP ) s[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(rounds * depth * 2ULL);
  }
  return median(s, g_measurements);
}

template<class D>
[[gnu::noinline]] u64
steal_sweep(D *victims, u32 nv, u32 self, u32 &seed) noexcept
{
  for ( u32 sweep = 0; sweep < 2u; ++sweep ) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    u32 v = static_cast<u32>((static_cast<u64>(seed) * nv) >> 32);
    for ( u32 i = 0; i < nv; ++i ) {
      if ( v != self ) {
        const u64 c = victims[v].steal_top();
        if ( c != 0 ) return c;
      }
      if ( ++v == nv ) v = 0;
    }
  }
  return 0;
}

template<usize S, bool Pad = (S > sizeof(deque_t))> struct strided;

template<usize S> struct alignas(64) strided<S, true> {
  deque_t d;
  char pad[S - sizeof(deque_t)];
};

template<usize S> struct alignas(64) strided<S, false> {
  deque_t d;
};

template<usize S>
f64
bench_probe_strided(u32 nv, u64 sweeps) noexcept
{
  f64 s[K_MEASUREMENTS];
  strided<S> *w = new strided<S>[nv];
  for ( u32 m = 0; m < g_measurements + WARMUP; ++m ) {
    u32 seed = 0x9E3779B9u;
    u64 acc = 0;
    const u64 c0 = rdtsc();
    for ( u64 r = 0; r < sweeps; ++r ) {
      for ( u32 i = 0; i < nv; ++i ) {

        for ( u32 sw = 0; sw < 2u; ++sw ) {
          seed ^= seed << 13;
          seed ^= seed >> 17;
          seed ^= seed << 5;
          u32 v = static_cast<u32>((static_cast<u64>(seed) * nv) >> 32);
          for ( u32 k = 0; k < nv; ++k ) {
            if ( v != i ) acc += w[v].d.steal_top();
            if ( ++v == nv ) v = 0;
          }
        }
      }
    }
    const u64 c1 = rdtsc();
    clobber(acc);
    g_sink += acc;

    const f64 probes = static_cast<f64>(sweeps) * static_cast<f64>(nv) * 2.0 * static_cast<f64>(nv - 1);
    if ( m >= WARMUP ) s[m - WARMUP] = cyc_to_ns(c1 - c0) / probes;
  }
  delete[] w;
  return median(s, g_measurements);
}

f64
bench_probe_mixed(u32 nv, u64 sweeps) noexcept
{
  f64 s[K_MEASUREMENTS];
  deque_t *w = new deque_t[nv];
  for ( u32 m = 0; m < g_measurements + WARMUP; ++m ) {
    for ( u32 i = 0; i < nv; ++i )
      if ( (i & 7u) == 0u )
        for ( u64 j = 1; j <= 64; ++j ) w[i].push_bottom(j);
    u32 seed = 0x9E3779B9u;
    u64 acc = 0;
    const u64 c0 = rdtsc();
    for ( u64 r = 0; r < sweeps; ++r ) acc += steal_sweep(w, nv, r % nv, seed);
    const u64 c1 = rdtsc();
    clobber(acc);
    g_sink += acc;
    for ( u32 i = 0; i < nv; ++i )
      while ( w[i].pop_bottom() != 0 ) {
      }
    if ( m >= WARMUP ) s[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(sweeps);
  }
  delete[] w;
  return median(s, g_measurements);
}

f64
bench_grow(u64 n, u64 &final_cap) noexcept
{
  f64 s[K_MEASUREMENTS];
  for ( u32 m = 0; m < g_measurements + WARMUP; ++m ) {
    micron::chase_lev_grow<u64, 4> d;
    const u64 c0 = rdtsc();
    for ( u64 i = 1; i <= n; ++i ) d.push_bottom(i);
    const u64 c1 = rdtsc();
    final_cap = d.capacity();
    u64 acc = 0;
    while ( const u64 v = d.pop_bottom() ) acc += v;
    g_sink += acc;
    if ( m >= WARMUP ) s[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(n);
  }
  return median(s, g_measurements);
}

template<bool Hist>
void
fj(deque_t &d, u32 depth, u64 &tag, u32 cur, u64 *hist) noexcept
{
  if ( depth == 0 ) return;
  d.push_bottom(++tag);
  fj<Hist>(d, depth - 1, tag, cur + 1, hist);
  if constexpr ( Hist ) ++hist[cur];
  g_sink += d.pop_bottom();
  fj<Hist>(d, depth - 1, tag, cur + 1, hist);
}

f64
bench_forkjoin(u32 depth, u64 &nodes) noexcept
{
  nodes = (1ULL << depth) - 1ULL;
  f64 s[K_MEASUREMENTS];
  for ( u32 m = 0; m < g_measurements + WARMUP; ++m ) {
    deque_t d;
    u64 tag = 0;
    const u64 c0 = rdtsc();
    fj<false>(d, depth, tag, 0, nullptr);
    const u64 c1 = rdtsc();
    if ( m >= WARMUP ) s[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(nodes * 2ULL);
  }
  return median(s, g_measurements);
}

void
forkjoin_depths(u32 depth) noexcept
{
  deque_t d;
  u64 tag = 0;
  u64 hist[64] = {};
  fj<true>(d, depth, tag, 0, hist);
  u64 tot = 0;
  for ( u32 i = 0; i < 64; ++i ) tot += hist[i];
  u64 acc = 0, p50 = 0, p95 = 0, mx = 0;
  for ( u32 i = 0; i < 64; ++i ) {
    if ( hist[i] == 0 ) continue;
    mx = i + 1;
    acc += hist[i];
    if ( p50 == 0 && acc * 2 >= tot ) p50 = i + 1;
    if ( p95 == 0 && acc * 100 >= tot * 95 ) p95 = i + 1;
  }
  micron::io::print("  fj depth at pop: p50=", p50, " p95=", p95, " max=", mx, " (nodes=", tot, ")\n");
}

struct alignas(64) thief_stat {
  u64 got = 0;
  u64 lost = 0;
  u64 empty = 0;
  u64 pad[5];
};

enum class owner_mode { churn, produce };

void
bench_steal(u32 K, u64 depth, u64 owner_ops, owner_mode mode, const char *label) noexcept
{
  f64 own_s[K_MEASUREMENTS];
  f64 rate_s[K_MEASUREMENTS];
  f64 got_s[K_MEASUREMENTS];
  f64 lost_s[K_MEASUREMENTS];

  for ( u32 m = 0; m < g_measurements + WARMUP; ++m ) {
    deque_t d;
    thief_stat *st = new thief_stat[K + 1];
    micron::atomic_token<u32> done{ 0 };
    micron::atomic_token<u64> owner_cyc{ 0 };

    for ( u64 i = 1; i <= depth; ++i ) d.push_bottom(i);

    parallel(static_cast<int>(K) + 1, [&](int tid) {
      pin(g_cpus[static_cast<u32>(tid) % g_cpu_count]);
      if ( tid == 0 ) {
        u64 acc = 0;
        const u64 c0 = rdtsc();
        if ( mode == owner_mode::churn ) {
          for ( u64 i = 0; i < owner_ops; ++i ) {
            d.push_bottom(depth + i + 1);
            acc += d.pop_bottom();
          }
        } else {
          for ( u64 i = 0; i < owner_ops; ++i ) {
            d.push_bottom(depth + 2 * i + 1);
            d.push_bottom(depth + 2 * i + 2);
            acc += d.pop_bottom();
          }
        }
        const u64 c1 = rdtsc();
        g_sink += acc;
        owner_cyc.store(c1 - c0, micron::memory_order_release);
        done.store(1, micron::memory_order_release);
      } else {
        thief_stat &me = st[tid];
        while ( done.get(micron::memory_order_acquire) == 0 ) {
          micron::steal_status s = micron::steal_status::empty;
          const u64 v = d.steal_top(s);
          if ( s == micron::steal_status::got ) {
            me.got += (v != 0);
          } else if ( s == micron::steal_status::lost )
            ++me.lost;
          else
            ++me.empty;
        }
      }
    });

    u64 got = 0, lost = 0, emp = 0;
    for ( u32 i = 1; i <= K; ++i ) {
      got += st[i].got;
      lost += st[i].lost;
      emp += st[i].empty;
    }
    const f64 ocyc = static_cast<f64>(owner_cyc.get(micron::memory_order_acquire));
    const f64 ons = cyc_to_ns(static_cast<u64>(ocyc));
    const u64 probes = got + lost + emp;
    if ( m >= WARMUP ) {
      own_s[m - WARMUP] = ons / static_cast<f64>(owner_ops);
      rate_s[m - WARMUP] = ons > 0 ? static_cast<f64>(got) * 1000.0 / ons : 0.0;
      got_s[m - WARMUP] = probes ? 100.0 * static_cast<f64>(got) / static_cast<f64>(probes) : 0.0;
      lost_s[m - WARMUP] = probes ? 100.0 * static_cast<f64>(lost) / static_cast<f64>(probes) : 0.0;
    }
    while ( d.pop_bottom() != 0 ) {
    }
    delete[] st;
  }

  const u64 centi_o = static_cast<u64>(median(own_s, g_measurements) * 100.0 + 0.5);
  const u64 centi_r = static_cast<u64>(median(rate_s, g_measurements) * 100.0 + 0.5);
  micron::io::print("  steal  ", label);
  for ( usize i = micron::strlen(label); i < 18; ++i ) micron::io::print(" ");
  micron::io::print("owner ", centi_o / 100, ".", (centi_o % 100 < 10 ? "0" : ""), centi_o % 100, " ns/op   steals ", centi_r / 100, ".",
                    (centi_r % 100 < 10 ? "0" : ""), centi_r % 100, " M/s   got=", static_cast<u64>(median(got_s, g_measurements) + 0.5),
                    "% lost=", static_cast<u64>(median(lost_s, g_measurements) + 0.5), "%\n");
}

bool
valid_case(const char *value) noexcept
{
  return eq(value, "owner") || eq(value, "pingpong") || eq(value, "deep8") || eq(value, "deep64") || eq(value, "deep512")
         || eq(value, "deep1000") || eq(value, "probe") || eq(value, "probe-v4") || eq(value, "probe-v8") || eq(value, "probe-v16")
         || eq(value, "probe-v32") || eq(value, "probe-mixed") || eq(value, "probe-stride256") || eq(value, "probe-stride8448")
         || eq(value, "growth") || eq(value, "forkjoin") || eq(value, "steal") || eq(value, "steal-churn") || eq(value, "steal-produce");
}

bool
parse_args(int argc, char **argv) noexcept
{
  if ( (argc & 1) == 0 ) return false;
  for ( int i = 1; i < argc; i += 2 ) {
    const char *key = argv[i];
    const char *value = argv[i + 1];
    if ( eq(key, "--case") ) {
      if ( eq(value, "all") )
        g_case = nullptr;
      else if ( valid_case(value) )
        g_case = value;
      else
        return false;
    } else if ( eq(key, "--impl") ) {
      if ( !eq(value, "all") && !eq(value, K_IMPL) ) return false;
    } else if ( eq(key, "--capacity") ) {
      if ( number(value) != 1024 ) return false;
    } else if ( eq(key, "--reps") ) {
      g_measurements = static_cast<u32>(number(value));
    } else if ( eq(key, "--cpu") ) {
      const u64 cpu = number(value);
      if ( cpu >= 1024 ) return false;
      g_cpus[0] = static_cast<u32>(cpu);
      g_cpu_count = 1;
      g_cpu_set_explicit = true;
    } else if ( eq(key, "--cpus") ) {
      if ( !parse_cpu_list(value) ) return false;
    } else if ( eq(key, "--topology") ) {
      if ( eq(value, "physical") ) {
        if ( !g_cpu_set_explicit ) {
          for ( u32 cpu = 0; cpu < 8; ++cpu ) g_cpus[cpu] = cpu;
          g_cpu_count = 8;
        }
      } else if ( eq(value, "smt") ) {
        if ( !g_cpu_set_explicit ) {
          for ( u32 core = 0; core < 8; ++core ) {
            g_cpus[2 * core] = core;
            g_cpus[2 * core + 1] = core + 8;
          }
          g_cpu_count = 16;
        }
      } else
        return false;
    } else
      return false;
  }
  return g_measurements >= 5 && g_measurements <= K_MEASUREMENTS;
}

}      // namespace

int
main(int argc, char **argv)
{
  if ( !parse_args(argc, argv) ) {
    micron::io::print("usage: chase_lev_bench --impl fixed|grow|all --case owner|pingpong|deep8|deep64|deep512|deep1000|"
                      "probe|probe-v4|probe-v8|probe-v16|probe-v32|probe-mixed|probe-stride256|probe-stride8448|growth|forkjoin|"
                      "steal|steal-churn|steal-produce|all --capacity 1024 --topology physical|smt --cpu N --cpus N,N,... --reps 5..7\n");
    return 1;
  }
  pin(g_cpus[0]);
  calibrate_tsc();

  micron::io::print("=== chase_lev bench === variant: ", K_VARIANT, "\n");
  micron::io::print("TSC ", static_cast<u64>(g_tsc_ghz * 1000.0), " MHz | sizeof(deque)=", sizeof(deque_t), " alignof=", alignof(deque_t),
                    " cap=", static_cast<u64>(1024), "\n");
  micron::io::print("median of ", static_cast<u64>(g_measurements), " (warmup ", static_cast<u64>(WARMUP), ")\n\n");

  if ( selected("owner", "pingpong") || selected("owner", "deep8") || selected("owner", "deep64") || selected("owner", "deep512")
       || selected("owner", "deep1000") ) {
    micron::io::print("[owner] single-threaded paths -- reproduce to ~2%, a >2% delta is real\n");
    if ( selected("owner", "pingpong") ) row("owner ", "pingpong-d1", 5000000, bench_pingpong(5000000), "ns/pair");
    if ( selected("owner", "deep8") ) row("owner ", "deep-d8", 8ULL * 400000, bench_deep(8, 400000), "ns/op");
    if ( selected("owner", "deep64") ) row("owner ", "deep-d64", 64ULL * 60000, bench_deep(64, 60000), "ns/op");
    if ( selected("owner", "deep512") ) row("owner ", "deep-d512", 512ULL * 8000, bench_deep(512, 8000), "ns/op");
    if ( selected("owner", "deep1000") ) row("owner ", "deep-d1000", 1000ULL * 4000, bench_deep(1000, 4000), "ns/op");
  }

  if ( selected("probe", "probe-v4") || selected("probe", "probe-v8") || selected("probe", "probe-v16") || selected("probe", "probe-v32")
       || selected("probe", "probe-mixed") || selected("probe", "probe-stride256") || selected("probe", "probe-stride8448") ) {
    micron::io::print("\n[probe] the literal cl_sched::__steal sweep over EMPTY victims\n");
    if ( selected("probe", "probe-v4") ) row("probe ", "sweep-v4", 4, bench_probe_strided<192>(4, 40000), "ns/victim");
    if ( selected("probe", "probe-v8") ) row("probe ", "sweep-v8", 8, bench_probe_strided<192>(8, 20000), "ns/victim");
    if ( selected("probe", "probe-v16") ) row("probe ", "sweep-v16", 16, bench_probe_strided<192>(16, 8000), "ns/victim");
    if ( selected("probe", "probe-v32") ) row("probe ", "sweep-v32", 32, bench_probe_strided<192>(32, 2500), "ns/victim");
    if ( selected("probe", "probe-mixed") ) row("probe ", "sweep-v16-mix", 16, bench_probe_mixed(16, 200000), "ns/sweep");
    if ( selected("probe", "probe-stride256") ) row("probe ", "stride256-v16", 16, bench_probe_strided<256>(16, 8000), "ns/victim");
    if ( selected("probe", "probe-stride8448") ) row("probe ", "stride8448-v16", 16, bench_probe_strided<8448>(16, 8000), "ns/victim");
  }

  if ( selected("growth", "growth") ) {
    micron::io::print("\n[grow] segment doubling, init 4\n");
    u64 cap = 0;
    const f64 v = bench_grow(1u << 18, cap);
    row("grow  ", "4->2^18", 1u << 18, v, "ns/push");
    micron::io::print("        final capacity ", cap, "\n");
  }

  if ( selected("forkjoin", "forkjoin") ) {
    micron::io::print("\n[forkjoin] binary recursion, the shape the runtime makes\n");
    u64 nodes = 0;
    const f64 v = bench_forkjoin(20, nodes);
    row("fj    ", "solo-d20", nodes * 2ULL, v, "ns/op");
    forkjoin_depths(20);
  }

  if ( selected("steal", "steal-churn") || selected("steal", "steal-produce") ) {
    micron::io::print("\n[steal] 1 owner + K thieves -- NOISY, median of >=5 whole runs, quote a range\n");
    micron::io::print("        churn   = owner push+pop (net 0): read the OWNER ns/op and lost%\n");
    micron::io::print("        produce = owner push+push+pop (net +1): read the steal rate and got%\n");
    if ( selected("steal", "steal-churn") ) {
      bench_steal(1, 4, 2000000, owner_mode::churn, "K=1 churn");
      bench_steal(3, 4, 2000000, owner_mode::churn, "K=3 churn");
      bench_steal(7, 4, 2000000, owner_mode::churn, "K=7 churn");
    }
    if ( selected("steal", "steal-produce") ) {
      bench_steal(1, 4, 1000000, owner_mode::produce, "K=1 produce");
      bench_steal(3, 4, 1000000, owner_mode::produce, "K=3 produce");
      bench_steal(7, 4, 1000000, owner_mode::produce, "K=7 produce");
    }
  }

#if defined(CHASE_LEV_BENCH_PMU)
  micron::io::print("\n[pmu] see the PMU block; single-threaded cells only\n");
#endif

  micron::io::print("\n(anti-DCE sink: ", g_sink, ")\n");
  return 0;
}
