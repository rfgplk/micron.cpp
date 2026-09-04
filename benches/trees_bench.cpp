//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// trees_bench.cpp -- src/trees/, with counters, at three cache tiers.
//
// Before this file there was no committed baseline for ANY tree in the library, and rb_tree, art
// and radix had no benchmark at all. btree_bench.cpp and spatial_bench.cpp measured wall clock
// with no warmup and a single sample, which cannot separate a real change from run-to-run noise.
//
// WHAT IS MEASURED, AND WHY THESE METRICS
//
//   cyc/op      the headline
//   IPC         the INLINING SENTINEL -- when a descent stops being register-resident, IPC falls
//               before cyc/op rises, and it falls loudly
//   bmiss%      trees are branchy by construction; a branchless descent has to show up here or it
//               did not happen
//   L1d-miss%   / LLC-miss%   the memory metrics. A tree is a pointer chase: these are the numbers
//               that a layout change moves, and the ONLY ones that separate "fewer instructions"
//               from "fewer cache lines"
//   B/elem      resident bytes per element, from the RSS delta across a build. Not a counter and
//               not an estimate -- it is what the process actually costs. It is a gate, not
//               commentary: every layout change in the plan has to move it down
//
// THE SIZE SWEEP *IS* THE CACHE-LOCALITY MEASUREMENT. 1024 fits L1/L2, 65536 fits LLC, 1048576
// does not. A fix that only shows at the largest N is a memory fix; one that shows at all three
// is an instruction-count fix. Do not read a single N and conclude anything.
//
// Two counter passes per cell, not one group of eight: Haswell-E has four general PMU counters,
// so a five-event group multiplexes and scales, and a scaled cache-miss rate is not worth having.
// The same setup+kernel is run twice, once per group.
//
// Build and run EXACTLY:
//   duck build benches/trees_bench.cpp --perf --fp --no-ssp --no-lto -i . -o bin/bench -f
//   taskset -c 0 ./bin/bench/trees_bench
// All four flags matter -- duck defaults to -fstack-protector-all, and a canary on every function
// does not cancel out of a ratio. `-i .` is REQUIRED, not tidiness: external/bbench includes
// <micron/...>, which without it resolves to the stale installed snapshot at /usr/include/micron
// and collides with the working tree. bbench needs kernel.perf_event_paranoid <= 2.
//
// DISCARD THE FIRST RUN. Same-binary spread is ~3%, and 10%+ at N <= 1024.

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/io/sys.hpp"
#include "../src/linux/sys/fcntl.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/maps/b_map.hpp"
#include "../src/maps/rb_map.hpp"
#include "../src/std.hpp"
#include "../src/trees/art.hpp"
#include "../src/trees/b.hpp"
#include "../src/trees/octree.hpp"
#include "../src/trees/quadtree.hpp"
#include "../src/trees/radix.hpp"
#include "../src/trees/rb.hpp"
#include "../src/trees/rtree.hpp"
#include "../src/vector/vector.hpp"

namespace
{

using c_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;
using m_events = bbench::event_group<bbench::level1d, bbench::level1d_miss, bbench::llcache, bbench::llcache_miss>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 1;

// L1/L2-resident · LLC-resident · out-of-cache. see the header comment.
constexpr u64 SIZES[] = { 1024, 65536, 1048576 };
constexpr u64 SPATIAL_SIZES[] = { 1024, 65536, 262144 };
constexpr u64 MAXN = 1048576;

// rb_tree costs ~8.2 KiB of RESIDENT memory per element at HEAD (measured, see the baseline), so
// a 1048576-element build is 8.6 GiB and each of the 5 measurements rebuilds it. Capped until the
// node pool lands; the cap lifting is itself part of the proof.
constexpr u64 RB_TREE_MAX_N = 65536;

constexpr usize SPATIAL_QUERIES = 2000;
constexpr float SPAN = 4000.f;

static volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

using vec2 = micron::math::vec<float, 2>;
using box2 = micron::math::geometry::aligned_box<float, 2>;
using vec3 = micron::math::vec<float, 3>;
using box3 = micron::math::geometry::aligned_box<float, 3>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// keys -- fixed hex seeds, never time-based

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

[[gnu::always_inline]] inline u64
key_u64(u64 i) noexcept
{
  return splitmix64(i + 1);
}

// disjoint from every key_u64(i) for any i < 2^40
[[gnu::always_inline]] inline u64
miss_u64(u64 i) noexcept
{
  return splitmix64(i + (1ULL << 40));
}

// keys are precomputed rather than generated in the kernel: splitmix64 is ~5 cycles, which is 5%
// of a warm b_tree find and would be charged to the tree.
micron::vector<u64> g_keys;
micron::vector<u64> g_misses;

void
build_key_tables()
{
  g_keys.reserve(MAXN);
  g_misses.reserve(MAXN);
  for ( u64 i = 0; i < MAXN; ++i ) {
    g_keys.push_back(key_u64(i));
    g_misses.push_back(miss_u64(i));
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// resident-set size, for the B/elem column

u64
rss_kib() noexcept
{
  const i32 fd = micron::posix::open("/proc/self/statm", micron::posix::o_rdonly);
  if ( fd < 0 ) return 0;
  char b[128];
  const max_t n = micron::posix::read(fd, b, sizeof(b) - 1);
  micron::posix::close(fd);
  if ( n <= 0 ) return 0;
  b[n] = '\0';
  // "size resident shared text lib data dt" -- field 2, in pages
  u32 i = 0;
  while ( b[i] && b[i] != ' ' ) ++i;
  while ( b[i] == ' ' ) ++i;
  u64 v = 0;
  while ( b[i] >= '0' && b[i] <= '9' ) v = v * 10 + static_cast<u64>(b[i++] - '0');
  return v * (micron::page_size / 1024);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// formatting -- fixed columns, no float printer beyond two decimals

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
        tmp[n++] = static_cast<char>('0' + (vv % 10));
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
        tmp[n++] = static_cast<char>('0' + (w % 10));
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = tmp[--n];
    buf[pos++] = '.';
    buf[pos++] = static_cast<char>('0' + (f.frac_x100 / 10));
    buf[pos++] = static_cast<char>('0' + (f.frac_x100 % 10));
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cell filters

const char *g_tree_filter = nullptr;
const char *g_op_filter = nullptr;
u64 g_size_filter = 0;
bool g_skip_footprint = false;

[[gnu::always_inline]] inline bool
streq(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

[[gnu::always_inline]] inline bool
starts_with(const char *s, const char *prefix) noexcept
{
  while ( *prefix )
    if ( *s++ != *prefix++ ) return false;
  return true;
}

[[gnu::always_inline]] inline bool
wanted_tree(const char *impl) noexcept
{
  return g_tree_filter == nullptr || streq(g_tree_filter, impl);
}

[[gnu::always_inline]] inline bool
wanted_cell(const char *op, u64 size) noexcept
{
  return (g_op_filter == nullptr || streq(g_op_filter, op)) && (g_size_filter == 0 || g_size_filter == size);
}

u64
parse_u64(const char *s) noexcept
{
  u64 value = 0;
  if ( *s == '\0' ) return 0;
  while ( *s ) {
    if ( *s < '0' || *s > '9' ) return 0;
    value = value * 10 + static_cast<u64>(*s++ - '0');
  }
  return value;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// measurement

struct row {
  const char *op;
  const char *impl;
  u64 size;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
  f64 l1d_miss_rate;
  f64 llc_miss_rate;
  bool unstable;
  bool skipped;
};

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

[[gnu::cold]] void
print_col_header()
{
  line h;
  h.s("op");
  h.pad_to(26, 0);
  h.s_lj_at("impl", 44);
  h.s_at("N", 54);
  h.s_at("cyc/op", 68);
  h.s_at("IPC", 77);
  h.s_at("bmiss%", 87);
  h.s_at("L1dmiss%", 98);
  h.s_at("LLCmiss%", 109);
  micron::io::println(h.str());
  micron::io::println("---------------------------------------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_header(const char *section)
{
  micron::io::println("");
  micron::io::println("[", section, "]");
  print_col_header();
}

[[gnu::cold]] void
print_row(const row &r)
{
  if ( r.skipped ) return;
  line ln;
  ln.s(r.op);
  ln.pad_to(26, 0);
  ln.s_lj_at(r.impl, 44);
  ln.u_at(r.size, 54);
  if ( r.unstable ) {
    ln.s_at("(unstable)", 68);
    micron::io::println(ln.str());
    return;
  }
  ln.f2_at(to_fmt2(r.cyc_per_op), 68);
  ln.f2_at(to_fmt2(r.ipc), 77);
  ln.f2_at(to_fmt2(r.bmiss_rate * 100.0), 87);
  ln.f2_at(to_fmt2(r.l1d_miss_rate * 100.0), 98);
  ln.f2_at(to_fmt2(r.llc_miss_rate * 100.0), 109);
  micron::io::println(ln.str());
}

template<typename Setup, typename Kernel>
[[gnu::noinline]] row
measure(const char *op, const char *impl, u64 size, u64 ops_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel) noexcept
{
  if ( !wanted_cell(op, size) ) return row{ op, impl, size, 0, 0, 0, 0, 0, false, true };

  try {
    for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
      setup();
      kernel();
    }
  } catch ( ... ) {
    return row{ op, impl, size, 0, 0, 0, 0, 0, true, false };
  }

  f64 cpo[K_MEASUREMENTS];
  f64 ipc[K_MEASUREMENTS];
  f64 bms[K_MEASUREMENTS];
  f64 l1m[K_MEASUREMENTS];
  f64 llm[K_MEASUREMENTS];

  const f64 total_ops = static_cast<f64>(reps_per_meas) * static_cast<f64>(ops_per_rep);

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    c_events evs{ bbench::quiet{} };
    evs.open();
    try {
      setup();
      evs.begin();
      for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
      evs.end();
    } catch ( ... ) {
      evs.end();
      return row{ op, impl, size, 0, 0, 0, 0, 0, true, false };
    }
    const auto cyc = static_cast<u64>(evs.template get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.template get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.template get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.template get<bbench::branch_misses>().retrieve());
    cpo[m] = total_ops > 0 ? static_cast<f64>(cyc) / total_ops : static_cast<f64>(cyc);
    ipc[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bms[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  // second pass, same setup+kernel: four cache events would multiplex against the four above
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    m_events evs{ bbench::quiet{} };
    evs.open();
    try {
      setup();
      evs.begin();
      for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
      evs.end();
    } catch ( ... ) {
      evs.end();
      l1m[m] = 0.0;
      llm[m] = 0.0;
      continue;
    }
    const auto l1a = static_cast<u64>(evs.template get<bbench::level1d>().retrieve());
    const auto l1x = static_cast<u64>(evs.template get<bbench::level1d_miss>().retrieve());
    const auto lla = static_cast<u64>(evs.template get<bbench::llcache>().retrieve());
    const auto llx = static_cast<u64>(evs.template get<bbench::llcache_miss>().retrieve());
    l1m[m] = l1a > 0 ? static_cast<f64>(l1x) / static_cast<f64>(l1a) : 0.0;
    llm[m] = lla > 0 ? static_cast<f64>(llx) / static_cast<f64>(lla) : 0.0;
  }

  return row{ op,
              impl,
              size,
              median_f64(cpo, K_MEASUREMENTS),
              median_f64(ipc, K_MEASUREMENTS),
              median_f64(bms, K_MEASUREMENTS),
              median_f64(l1m, K_MEASUREMENTS),
              median_f64(llm, K_MEASUREMENTS),
              false,
              false };
}

[[gnu::always_inline]] inline u64
reps_for(u64 ops_per_rep) noexcept
{
  constexpr u64 TARGET = 1ULL << 18;
  if ( ops_per_rep == 0 ) return 16;
  u64 r = TARGET / ops_per_rep;
  if ( r < 2 ) r = 2;
  if ( r > 256 ) r = 256;
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// footprint -- resident bytes per element, measured, not derived

[[gnu::cold]] void
print_footprint_header(const char *what)
{
  micron::io::println("");
  micron::io::println("[", what, "]");
  line h;
  h.s("impl");
  h.pad_to(30, 0);
  h.s_at("N", 42);
  h.s_at("B/elem", 54);
  h.s_at("node KiB", 66);
  h.s_at("RSS B/elem", 80);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_footprint(const char *impl, u64 n, u64 node_bytes, u64 rss_kib_delta)
{
  line ln;
  ln.s(impl);
  ln.pad_to(30, 0);
  ln.u_at(n, 42);
  ln.u_at(n ? node_bytes / n : 0, 54);
  ln.u_at(node_bytes / 1024, 66);
  ln.u_at(n ? (rss_kib_delta * 1024) / n : 0, 80);
  micron::io::println(ln.str());
}

// exact node-storage accounting, per container. RSS delta alone is NOT trustworthy here and the
// baseline says so: the measurements run in one process, so an entry that follows a large free
// reads artificially low. At HEAD, rb_tree's build grew and released ~480 MiB immediately before
// art's, and art consequently reported 37 B/elem against a true 55. These accessors are exact and
// order-independent; the RSS column is kept only as a cross-check.
template<typename T>
u64
owned_node_bytes(const T &t) noexcept
{
  if constexpr ( requires { t.pool_bytes(); } )
    return t.pool_bytes();
  else if constexpr ( requires { t.nodes_used(); T::node_bytes(); } )
    return t.nodes_used() * T::node_bytes();
  else
    return 0;
}

template<typename Make, typename Fill>
[[gnu::noinline]] void
footprint(const char *impl, u64 n, Make &&make, Fill &&fill) noexcept
{
  if ( g_skip_footprint || !wanted_tree(impl) ) return;
  u64 kib = 0;
  u64 nb = 0;
  try {
    const u64 before = rss_kib();
    {
      auto t = make();
      fill(t, n);
      clobber(&t);
      const u64 after = rss_kib();
      kib = after > before ? after - before : 0;
      nb = owned_node_bytes(t);
    }
  } catch ( ... ) {
    return;
  }
  print_footprint(impl, n, nb, kib);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ordered / trie sweeps

// rb_tree is a SET of T; the others are maps. one op battery, two shapes.
void
sweep_rb_tree(u64 N)
{
  const char *impl = "rb_tree<u64>";
  micron::rb_tree<u64> m;

  auto build = [&](micron::rb_tree<u64> &t) {
    for ( u64 i = 0; i < N; ++i ) t.insert(g_keys[i]);
  };

  print_row(measure(
      "insert (build)", impl, N, N, 1, [&] { m = micron::rb_tree<u64>{}; },
      [&] {
        build(m);
        clobber(&m);
      }));

  m = micron::rb_tree<u64>{};
  build(m);

  print_row(measure(
      "find (hit)", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        for ( u64 i = 0; i < N; ++i ) {
          const u64 *p = m.find(g_keys[i]);
          if ( p ) acc += *p;
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "find (miss)", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        for ( u64 i = 0; i < N; ++i ) {
          const u64 *p = m.find(g_misses[i]);
          if ( p ) acc += *p;
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "iterate", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        m.for_each([&](const u64 &e) { acc += e; });
        sink_u64 += acc;
      }));

  print_row(measure(
      "erase (drain)", impl, N, N, 1,
      [&] {
        m = micron::rb_tree<u64>{};
        build(m);
      },
      [&] {
        for ( u64 i = 0; i < N; ++i ) sink_u64 += m.erase(g_keys[i]) ? 1u : 0u;
        clobber(&m);
      }));

  // churn: steady-state size, alternating erase/insert over a sliding window
  print_row(measure(
      "churn (erase+insert)", impl, N, N, 1,
      [&] {
        m = micron::rb_tree<u64>{};
        build(m);
      },
      [&] {
        for ( u64 i = 0; i < N; ++i ) {
          m.erase(g_keys[i]);
          m.insert(g_misses[i]);
        }
        clobber(&m);
      }));
}

template<typename Map, typename Insert>
void
sweep_map_u64(const char *impl, u64 N, Insert &&ins)
{
  Map m;

  auto build = [&](Map &t) {
    for ( u64 i = 0; i < N; ++i ) ins(t, g_keys[i], i);
  };

  print_row(measure(
      "insert (build)", impl, N, N, 1, [&] { m = Map{}; },
      [&] {
        build(m);
        clobber(&m);
      }));

  m = Map{};
  build(m);

  print_row(measure(
      "find (hit)", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        for ( u64 i = 0; i < N; ++i ) {
          const u64 *p = m.find(g_keys[i]);
          if ( p ) acc += *p;
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "find (miss)", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        for ( u64 i = 0; i < N; ++i ) {
          const u64 *p = m.find(g_misses[i]);
          if ( p ) acc += *p;
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "iterate", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        m.for_each([&](const u64 &, u64 &v) { acc += v; });
        sink_u64 += acc;
      }));

  print_row(measure(
      "erase (drain)", impl, N, N, 1,
      [&] {
        m = Map{};
        build(m);
      },
      [&] {
        for ( u64 i = 0; i < N; ++i ) sink_u64 += m.erase(g_keys[i]) ? 1u : 0u;
        clobber(&m);
      }));

  print_row(measure(
      "churn (erase+insert)", impl, N, N, 1,
      [&] {
        m = Map{};
        build(m);
      },
      [&] {
        for ( u64 i = 0; i < N; ++i ) {
          m.erase(g_keys[i]);
          ins(m, g_misses[i], i);
        }
        clobber(&m);
      }));
}

// b_tree only: the ordered ops the hash-shaped maps cannot answer
template<typename BT>
void
sweep_btree_ordered(const char *impl, u64 N)
{
  BT m;
  for ( u64 i = 0; i < N; ++i ) m.insert(g_keys[i], i);

  print_row(measure(
      "lower_bound", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        for ( u64 i = 0; i < N; ++i ) {
          auto it = m.lower_bound(g_misses[i]);
          if ( it != m.end() ) acc += (*it).value;
        }
        sink_u64 += acc;
      }));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// radix -- string keys, deliberately prefix-dense so edge splitting is actually exercised

micron::vector<micron::string> g_skeys;

void
build_string_keys(u64 n)
{
  g_skeys.reserve(n);
  for ( u64 i = 0; i < n; ++i ) {
    char buf[24];
    u64 v = g_keys[i];
    // a two-symbol alphabet: keys collide on long prefixes, which is the shape a radix trie is for
    u32 len = 8 + static_cast<u32>(v % 8);
    for ( u32 j = 0; j < len; ++j ) {
      buf[j] = static_cast<char>('a' + (v & 1u));
      v >>= 1;
    }
    buf[len] = '\0';
    g_skeys.push_back(micron::string(static_cast<const char *>(buf)));
  }
}

void
sweep_radix(u64 N)
{
  const char *impl = "radix<string,u64>";
  using RT = micron::radix_tree<micron::string, u64>;
  RT m;

  auto build = [&](RT &t) {
    for ( u64 i = 0; i < N; ++i ) t.insert_or_assign(g_skeys[i], i);
  };

  print_row(measure(
      "insert (build)", impl, N, N, 1, [&] { m = RT{}; },
      [&] {
        build(m);
        clobber(&m);
      }));

  m = RT{};
  build(m);

  print_row(measure(
      "find (hit)", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        for ( u64 i = 0; i < N; ++i ) {
          const u64 *p = m.find(g_skeys[i]);
          if ( p ) acc += *p;
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "longest_prefix", impl, N, N, reps_for(N), [] { },
      [&] {
        u64 acc = 0;
        for ( u64 i = 0; i < N; ++i ) {
          const u64 *p = m.longest_prefix_match(g_skeys[i]);
          if ( p ) acc += *p;
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "erase (drain)", impl, N, N, 1,
      [&] {
        m = RT{};
        build(m);
      },
      [&] {
        for ( u64 i = 0; i < N; ++i ) sink_u64 += m.erase(g_skeys[i]) ? 1u : 0u;
        clobber(&m);
      }));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// spatial sweeps

void
sweep_rtree(u64 N)
{
  const char *impl = "rtree<u32,f32,2>";
  micron::rtree<u32, float, 2> m;

  auto build = [&](micron::rtree<u32, float, 2> &t) {
    for ( u64 i = 0; i < N; ++i ) {
      const float x = static_cast<float>(g_keys[i] % 4000u);
      const float y = static_cast<float>((g_keys[i] >> 20) % 4000u);
      box2 b;
      b.min_corner = vec2{ x, y };
      b.max_corner = vec2{ x + 3.f, y + 3.f };
      t.insert(b, static_cast<u32>(i));
    }
  };

  print_row(measure(
      "insert (build)", impl, N, N, 1, [&] { m = micron::rtree<u32, float, 2>{}; },
      [&] {
        build(m);
        clobber(&m);
      }));

  m = micron::rtree<u32, float, 2>{};
  build(m);

  print_row(measure(
      "range query", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          const float x = static_cast<float>(g_keys[q] % 4000u);
          const float y = static_cast<float>((g_keys[q] >> 20) % 4000u);
          box2 qb;
          qb.min_corner = vec2{ x, y };
          qb.max_corner = vec2{ x + 100.f, y + 100.f };
          m.query(qb, [&](const box2 &, const u32 &v) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "knn (k=10)", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          const float x = static_cast<float>(g_keys[q] % 4000u);
          const float y = static_cast<float>((g_keys[q] >> 20) % 4000u);
          m.nearest(vec2{ x, y }, 10, [&](const box2 &, const u32 &v, float) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "erase (drain)", impl, N, N, 1,
      [&] {
        m = micron::rtree<u32, float, 2>{};
        build(m);
      },
      [&] {
        for ( u64 i = 0; i < N; ++i ) {
          const float x = static_cast<float>(g_keys[i] % 4000u);
          const float y = static_cast<float>((g_keys[i] >> 20) % 4000u);
          box2 b;
          b.min_corner = vec2{ x, y };
          b.max_corner = vec2{ x + 3.f, y + 3.f };
          sink_u64 += m.erase(b, static_cast<u32>(i)) ? 1u : 0u;
        }
        clobber(&m);
      }));
}

void
sweep_quadtree(u64 N)
{
  const char *impl = "quadtree<u32>";
  box2 uni;
  uni.min_corner = vec2{ 0, 0 };
  uni.max_corner = vec2{ SPAN, SPAN };

  micron::quadtree<u32> m(uni);

  auto build = [&](micron::quadtree<u32> &t) {
    for ( u64 i = 0; i < N; ++i )
      t.insert(vec2{ static_cast<float>(g_keys[i] % 4000u), static_cast<float>((g_keys[i] >> 20) % 4000u) }, static_cast<u32>(i));
  };

  print_row(measure(
      "insert (build)", impl, N, N, 1, [&] { m = micron::quadtree<u32>{ uni }; },
      [&] {
        build(m);
        clobber(&m);
      }));

  m = micron::quadtree<u32>{ uni };
  build(m);

  print_row(measure(
      "range query", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          const float x = static_cast<float>(g_keys[q] % 4000u);
          const float y = static_cast<float>((g_keys[q] >> 20) % 4000u);
          box2 qb;
          qb.min_corner = vec2{ x, y };
          qb.max_corner = vec2{ x + 100.f, y + 100.f };
          m.query(qb, [&](const vec2 &, const u32 &v) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "radius query", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          const float x = static_cast<float>(g_keys[q] % 4000u);
          const float y = static_cast<float>((g_keys[q] >> 20) % 4000u);
          m.query_radius(vec2{ x, y }, 50.f, [&](const vec2 &, const u32 &v) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "knn (k=10)", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          const float x = static_cast<float>(g_keys[q] % 4000u);
          const float y = static_cast<float>((g_keys[q] >> 20) % 4000u);
          m.nearest(vec2{ x, y }, 10, [&](const vec2 &, const u32 &v, float) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));

  // pr_tree::erase reclaims nothing at HEAD -- nodes_used() only goes up. this cell plus the
  // footprint table is what that defect shows up in.
  print_row(measure(
      "churn (erase+insert)", impl, N, N, 1,
      [&] {
        m = micron::quadtree<u32>{ uni };
        build(m);
      },
      [&] {
        for ( u64 i = 0; i < N; ++i ) {
          const vec2 p{ static_cast<float>(g_keys[i] % 4000u), static_cast<float>((g_keys[i] >> 20) % 4000u) };
          m.erase(p, static_cast<u32>(i));
          m.insert(p, static_cast<u32>(i));
        }
        clobber(&m);
      }));
}

void
sweep_octree(u64 N)
{
  const char *impl = "octree<u32>";
  box3 uni;
  uni.min_corner = vec3{ 0, 0, 0 };
  uni.max_corner = vec3{ 400, 400, 400 };

  micron::octree<u32> m(uni);

  auto pt = [&](u64 i) {
    return vec3{ static_cast<float>(g_keys[i] % 400u), static_cast<float>((g_keys[i] >> 16) % 400u),
                 static_cast<float>((g_keys[i] >> 32) % 400u) };
  };

  auto build = [&](micron::octree<u32> &t) {
    for ( u64 i = 0; i < N; ++i ) t.insert(pt(i), static_cast<u32>(i));
  };

  print_row(measure(
      "insert (build)", impl, N, N, 1, [&] { m = micron::octree<u32>{ uni }; },
      [&] {
        build(m);
        clobber(&m);
      }));

  m = micron::octree<u32>{ uni };
  build(m);

  print_row(measure(
      "range query", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          const vec3 c = pt(q);
          box3 qb;
          qb.min_corner = c;
          qb.max_corner = vec3{ c.data[0] + 50.f, c.data[1] + 50.f, c.data[2] + 50.f };
          m.query(qb, [&](const vec3 &, const u32 &v) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "radius query", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          m.query_radius(pt(q), 25.f, [&](const vec3 &, const u32 &v) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));

  print_row(measure(
      "knn (k=10)", impl, N, SPATIAL_QUERIES, 2, [] { },
      [&] {
        u64 acc = 0;
        for ( usize q = 0; q < SPATIAL_QUERIES; ++q ) {
          m.nearest(pt(q), 10, [&](const vec3 &, const u32 &v, float) noexcept { acc += v; });
        }
        sink_u64 += acc;
      }));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// verification -- every structure must agree on the same key set BEFORE anything is timed. a
// bench whose arms hold different data is worse than no bench.

bool
verify()
{
  constexpr u64 n = 4096;

  micron::rb_tree<u64> rb;
  micron::b_tree<u64, u64> bt;
  micron::art<u64, u64> at;
  micron::rb_map<u64, u64> rm;
  micron::btree_map<u64, u64> bm;

  for ( u64 i = 0; i < n; ++i ) {
    rb.insert(g_keys[i]);
    bt.insert(g_keys[i], i);
    at.insert(g_keys[i], i);
    rm.insert_or_assign(g_keys[i], i);
    bm.insert(g_keys[i], i);
  }

  if ( rb.size() != n || bt.size() != n || at.size() != n ) return false;

  for ( u64 i = 0; i < n; ++i ) {
    const u64 *r = rb.find(g_keys[i]);
    const u64 *b = bt.find(g_keys[i]);
    const u64 *a = at.find(g_keys[i]);
    const u64 *p = rm.find(g_keys[i]);
    const u64 *q = bm.find(g_keys[i]);
    if ( !r || !b || !a || !p || !q ) return false;
    if ( *r != g_keys[i] ) return false;
    if ( *b != i || *a != i || *p != i || *q != i ) return false;
    if ( rb.find(g_misses[i]) || bt.find(g_misses[i]) || at.find(g_misses[i]) ) return false;
  }

  // b_tree iterates in key order; that is the whole point of it over the hash-shaped maps
  u64 prev = 0;
  u64 seen = 0;
  bool ordered = true;
  bt.for_each([&](const u64 &k, u64 &) {
    if ( seen && k <= prev ) ordered = false;
    prev = k;
    ++seen;
  });
  if ( !ordered || seen != n ) return false;

  // spatial arms against a brute-force scan of the same points
  box2 uni;
  uni.min_corner = vec2{ 0, 0 };
  uni.max_corner = vec2{ SPAN, SPAN };
  micron::quadtree<u32> qt(uni);
  micron::rtree<u32, float, 2> rt;
  constexpr u64 sn = 1024;
  float px[sn];
  float py[sn];
  for ( u64 i = 0; i < sn; ++i ) {
    px[i] = static_cast<float>(g_keys[i] % 4000u);
    py[i] = static_cast<float>((g_keys[i] >> 20) % 4000u);
    qt.insert(vec2{ px[i], py[i] }, static_cast<u32>(i));
    box2 b;
    b.min_corner = vec2{ px[i], py[i] };
    b.max_corner = vec2{ px[i], py[i] };
    rt.insert(b, static_cast<u32>(i));
  }

  for ( u64 q = 0; q < 32; ++q ) {
    const float x = static_cast<float>(g_keys[q] % 4000u);
    const float y = static_cast<float>((g_keys[q] >> 20) % 4000u);
    box2 qb;
    qb.min_corner = vec2{ x, y };
    qb.max_corner = vec2{ x + 100.f, y + 100.f };

    u64 brute = 0;
    for ( u64 i = 0; i < sn; ++i )
      if ( px[i] >= x && px[i] <= x + 100.f && py[i] >= y && py[i] <= y + 100.f ) ++brute;

    u64 qhits = 0;
    qt.query(qb, [&](const vec2 &, const u32 &) noexcept { ++qhits; });
    u64 rhits = 0;
    rt.query(qb, [&](const box2 &, const u32 &) noexcept { ++rhits; });

    if ( qhits != brute || rhits != brute ) return false;
  }

  return true;
}

};      // namespace

int
main(int argc, char **argv)
{
  for ( int i = 1; i < argc; ++i ) {
    if ( starts_with(argv[i], "--tree=") )
      g_tree_filter = argv[i] + 7;
    else if ( starts_with(argv[i], "--op=") )
      g_op_filter = argv[i] + 5;
    else if ( starts_with(argv[i], "--size=") )
      g_size_filter = parse_u64(argv[i] + 7);
    else if ( streq(argv[i], "--no-footprint") )
      g_skip_footprint = true;
    else {
      micron::io::println("usage: trees_bench [--tree=TAG] [--op=NAME] [--size=N] [--no-footprint]");
      return 2;
    }
  }

  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  build_key_tables();
  build_string_keys(1u << 17);

  if ( !verify() ) {
    micron::io::println("VERIFY FAILED -- the arms disagree; the numbers below would be meaningless");
    return 1;
  }

  micron::io::println("=== micron src/trees benchmark ===");
  micron::io::println("pass 1: cycles + instructions + branches + branch-misses");
  micron::io::println("pass 2: L1d + L1d-miss + LLC + LLC-miss   (two passes: 4 PMU counters, no multiplexing)");
  micron::io::println("warmup ", WARMUP_REPS, " rep; ", K_MEASUREMENTS, " measurements per cell (median)");
  micron::io::println("sizes: 1024 (L1/L2) - 65536 (LLC) - 1048576 (out of cache)");
  micron::io::println("verify: PASS (all arms agree on 4096 keys; spatial arms agree with brute force)");

  if ( wanted_tree("rb_tree<u64>") ) {
    print_header("rb_tree<u64>  -- src/trees/rb.hpp");
    for ( u64 N : SIZES ) {
      if ( N > RB_TREE_MAX_N ) {
        micron::io::println("  N=", N, " SKIPPED: ~8.2 KiB/element at HEAD would need ", (N * 8407ULL) >> 20, " MiB per rebuild");
        continue;
      }
      sweep_rb_tree(N);
    }
  }

  if ( wanted_tree("b_tree<u64,u64>") ) {
    print_header("b_tree<u64,u64>  -- src/trees/b.hpp, auto degree (T=16, 31 keys/node)");
    for ( u64 N : SIZES ) {
      sweep_map_u64<micron::b_tree<u64, u64>>("b_tree<u64,u64>", N,
                                              [](micron::b_tree<u64, u64> &t, u64 k, u64 v) { t.insert(k, v); });
      sweep_btree_ordered<micron::b_tree<u64, u64>>("b_tree<u64,u64>", N);
    }
  }

  if ( wanted_tree("b_tree T=8") ) {
    print_header("b_tree<u64,u64,T=8>  -- 15 keys/node, degree sweep");
    using bt8 = micron::b_tree<u64, u64, micron::b_default_less<u64>, 8>;
    for ( u64 N : SIZES ) sweep_map_u64<bt8>("b_tree T=8", N, [](bt8 &t, u64 k, u64 v) { t.insert(k, v); });
  }

  if ( wanted_tree("b_tree T=32") ) {
    print_header("b_tree<u64,u64,T=32>  -- 63 keys/node, degree sweep");
    using bt32 = micron::b_tree<u64, u64, micron::b_default_less<u64>, 32>;
    for ( u64 N : SIZES ) sweep_map_u64<bt32>("b_tree T=32", N, [](bt32 &t, u64 k, u64 v) { t.insert(k, v); });
  }

  if ( wanted_tree("art<u64,u64>") ) {
    print_header("art<u64,u64>  -- src/trees/art.hpp");
    for ( u64 N : SIZES )
      sweep_map_u64<micron::art<u64, u64>>("art<u64,u64>", N, [](micron::art<u64, u64> &t, u64 k, u64 v) { t.insert(k, v); });
  }

  if ( wanted_tree("rb_map<u64,u64>") ) {
    print_header("rb_map<u64,u64>  -- src/maps/rb_map.hpp (continuity with btree_bench)");
    for ( u64 N : SIZES )
      sweep_map_u64<micron::rb_map<u64, u64>>("rb_map<u64,u64>", N,
                                              [](micron::rb_map<u64, u64> &t, u64 k, u64 v) { t.insert_or_assign(k, v); });
  }

  if ( wanted_tree("btree_map<u64,u64>") ) {
    print_header("btree_map<u64,u64>  -- src/maps/b_map.hpp (continuity with btree_bench)");
    for ( u64 N : SIZES )
      sweep_map_u64<micron::btree_map<u64, u64>>("btree_map<u64,u64>", N,
                                                 [](micron::btree_map<u64, u64> &t, u64 k, u64 v) { t.insert(k, v); });
  }

  if ( wanted_tree("radix<string,u64>") ) {
    print_header("radix_tree<string,u64>  -- src/trees/radix.hpp (prefix-dense keys)");
    constexpr u64 RSIZES[] = { 1024, 65536 };
    for ( u64 N : RSIZES ) sweep_radix(N);
  }

  if ( wanted_tree("rtree<u32,f32,2>") ) {
    print_header("rtree<u32,f32,2>  -- src/trees/rtree.hpp");
    for ( u64 N : SPATIAL_SIZES ) sweep_rtree(N);
  }

  if ( wanted_tree("quadtree<u32>") ) {
    print_header("quadtree<u32>  -- src/trees/__subdiv_tree.hpp, Dim=2");
    for ( u64 N : SPATIAL_SIZES ) sweep_quadtree(N);
  }

  if ( wanted_tree("octree<u32>") ) {
    print_header("octree<u32>  -- src/trees/__subdiv_tree.hpp, Dim=3");
    for ( u64 N : SPATIAL_SIZES ) sweep_octree(N);
  }

  if ( !g_skip_footprint ) {
    print_footprint_header("node storage after a clean build -- exact, plus RSS delta as a cross-check");
    constexpr u64 FN = 65536;

    footprint(
        "rb_tree<u64>", FN, [] { return micron::rb_tree<u64>{}; },
        [](micron::rb_tree<u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert(g_keys[i]);
        });
    footprint(
        "b_tree<u64,u64>", FN, [] { return micron::b_tree<u64, u64>{}; },
        [](micron::b_tree<u64, u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert(g_keys[i], i);
        });
    footprint(
        "art<u64,u64>", FN, [] { return micron::art<u64, u64>{}; },
        [](micron::art<u64, u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert(g_keys[i], i);
        });
    footprint(
        "rb_map<u64,u64>", FN, [] { return micron::rb_map<u64, u64>{}; },
        [](micron::rb_map<u64, u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert_or_assign(g_keys[i], i);
        });
    footprint(
        "btree_map<u64,u64>", FN, [] { return micron::btree_map<u64, u64>{}; },
        [](micron::btree_map<u64, u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert(g_keys[i], i);
        });
    footprint(
        "radix<string,u64>", FN, [] { return micron::radix_tree<micron::string, u64>{}; },
        [](micron::radix_tree<micron::string, u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert_or_assign(g_skeys[i], i);
        });
    footprint(
        "rtree<u32,f32,2>", FN, [] { return micron::rtree<u32, float, 2>{}; },
        [](micron::rtree<u32, float, 2> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) {
            const float x = static_cast<float>(g_keys[i] % 4000u);
            const float y = static_cast<float>((g_keys[i] >> 20) % 4000u);
            box2 b;
            b.min_corner = vec2{ x, y };
            b.max_corner = vec2{ x + 3.f, y + 3.f };
            t.insert(b, static_cast<u32>(i));
          }
        });

    box2 uni2;
    uni2.min_corner = vec2{ 0, 0 };
    uni2.max_corner = vec2{ SPAN, SPAN };
    footprint(
        "quadtree<u32>", FN, [&] { return micron::quadtree<u32>{ uni2 }; },
        [](micron::quadtree<u32> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i )
            t.insert(vec2{ static_cast<float>(g_keys[i] % 4000u), static_cast<float>((g_keys[i] >> 20) % 4000u) },
                     static_cast<u32>(i));
        });

    box3 uni3;
    uni3.min_corner = vec3{ 0, 0, 0 };
    uni3.max_corner = vec3{ 400, 400, 400 };
    footprint(
        "octree<u32>", FN, [&] { return micron::octree<u32>{ uni3 }; },
        [](micron::octree<u32> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i )
            t.insert(vec3{ static_cast<float>(g_keys[i] % 400u), static_cast<float>((g_keys[i] >> 16) % 400u),
                           static_cast<float>((g_keys[i] >> 32) % 400u) },
                     static_cast<u32>(i));
        });

    // churned footprint: build, drain, rebuild. a structure that reclaims nothing on erase reads
    // roughly 2x its clean number here. pr_tree::erase and art's missing node shrink are the two
    // this is aimed at.
    micron::io::println("");
    print_footprint_header("node storage after CHURN -- build N, erase all, rebuild N. a reclaim failure shows as ~2x");

    footprint(
        "art<u64,u64>", FN, [] { return micron::art<u64, u64>{}; },
        [](micron::art<u64, u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert(g_keys[i], i);
          for ( u64 i = 0; i < n; ++i ) t.erase(g_keys[i]);
          for ( u64 i = 0; i < n; ++i ) t.insert(g_misses[i], i);
        });
    footprint(
        "quadtree<u32>", FN, [&] { return micron::quadtree<u32>{ uni2 }; },
        [](micron::quadtree<u32> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i )
            t.insert(vec2{ static_cast<float>(g_keys[i] % 4000u), static_cast<float>((g_keys[i] >> 20) % 4000u) },
                     static_cast<u32>(i));
          for ( u64 i = 0; i < n; ++i )
            t.erase(vec2{ static_cast<float>(g_keys[i] % 4000u), static_cast<float>((g_keys[i] >> 20) % 4000u) },
                    static_cast<u32>(i));
          for ( u64 i = 0; i < n; ++i )
            t.insert(vec2{ static_cast<float>(g_misses[i] % 4000u), static_cast<float>((g_misses[i] >> 20) % 4000u) },
                     static_cast<u32>(i));
        });
    footprint(
        "b_tree<u64,u64>", FN, [] { return micron::b_tree<u64, u64>{}; },
        [](micron::b_tree<u64, u64> &t, u64 n) {
          for ( u64 i = 0; i < n; ++i ) t.insert(g_keys[i], i);
          for ( u64 i = 0; i < n; ++i ) t.erase(g_keys[i]);
          for ( u64 i = 0; i < n; ++i ) t.insert(g_misses[i], i);
        });
  }

  micron::io::println("");
  micron::io::println("=== done (anti-DCE sink: ", sink_u64, ") ===");
  return 0;
}
