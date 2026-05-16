//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Map benchmark — every map shipped under src/maps/ is exercised against the
// same battery of operations and the same fixed key/value workload, so the
// resulting table is a direct cross-implementation comparison rather than
// a per-impl micro-report.
//
//   maps under test:
//     btree_map (b_map.hpp)       — hash-routed Bε-tree, grows on demand
//     robin_map (robin.hpp)       — fixed-capacity robin-hood w/ SIMD probe
//     hopscotch_map (hopscotch)   — neighbourhood hopscotch, grows on demand
//     stack_swiss_map (swiss)     — SSE/NEON ctrl-byte SIMD, fixed capacity
//     immutable_map (immutable)   — persistent LLRB tree (functional)
//     immutable_table (itable)    — persistent radix/patricia trie
//
//   per (map, op, N, K-type) cell the harness reports
//     cyc/op   IPC   bmiss%
//   medians across K_MEASUREMENTS samples; bbench 4-event group.
//
//   operations (extensive — every public method that has a meaningful cost):
//     insert(N)               build the map from empty
//     insert (replace)        re-insert keys that already exist
//     find (hit)              successful lookups
//     find (miss)             unsuccessful lookups (keys never inserted)
//     contains (hit/miss)     boolean lookup wrappers
//     count (hit/miss)        count = 0/1 wrappers
//     operator[] (hit)        existing-key indexing
//     emplace                 emplace into empty
//     erase                   one-by-one erase of every key
//     iterate                 full begin()/end() walk; sums values
//     copy-ctor               container-level deep/structural copy
//     move-ctor               container-level move
//     clear                   drain to empty
//
//   key/value types:
//     u64 → u64               cheap, trivially copyable, fast hash
//     hstring → i32           heap-owning key (real-world workload shape)
//
//   key generation uses an LCG mixed through a splitmix64-ish finalizer to
//   produce well-distributed, deterministic keys without aliasing the hash
//   used internally by each map.

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/maps/b_map.hpp"
#include "../src/maps/conmap.hpp"
#include "../src/maps/heap_swiss.hpp"
#include "../src/maps/hopscotch.hpp"
#include "../src/maps/immutable.hpp"
#include "../src/maps/itable.hpp"
#include "../src/maps/pmap.hpp"
#include "../src/maps/rb_map.hpp"
#include "../src/maps/robin.hpp"
#include "../src/maps/swiss.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"
#include "../src/trees/art.hpp"

namespace
{

using c_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 3;
constexpr u64 WARMUP_REPS = 1;

struct anomaly {
  const char *op;
  const char *impl;
  u64 size;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
  const char *reason;
};

static anomaly g_anomalies[256];
static u32 g_anomaly_count = 0;

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

[[gnu::always_inline]] inline u64
val_u64(u64 i) noexcept
{
  return i * 0x9E3779B1ULL ^ 0xDEADBEEF;
}

static volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
sink_bool(bool b) noexcept
{
  sink_u64 += static_cast<u64>(b);
}

[[gnu::always_inline]] inline void
sink_ptr(const void *p) noexcept
{
  sink_u64 += reinterpret_cast<uintptr_t>(p);
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
print_col_header()
{
  line h;
  h.s("op");
  h.pad_to(24, 0);
  h.s_lj_at("impl", 42);
  h.s_at("N", 52);
  h.s_at("cyc/op", 64);
  h.s_at("IPC", 74);
  h.s_at("bmiss%", 84);
  micron::io::println(h.str());
  micron::io::println("-----------------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_header(const char *section)
{
  micron::io::println("");
  micron::io::println("[", section, "]");
  print_col_header();
}

struct row {
  const char *op;
  const char *impl;
  u64 size;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
  bool unstable;
};

[[gnu::cold]] void
print_row(const row &r)
{
  if ( r.unstable ) {
    line ln;
    ln.s_lj_at(r.op, 24);
    ln.s_lj_at(r.impl, 42);
    ln.u_at(r.size, 52);
    ln.pad_to(56, 0);
    ln.s("(unstable)");
    micron::io::println(ln.str());
    return;
  }
  const fmt2 cpo = to_fmt2(r.cyc_per_op);
  const fmt2 ipc = to_fmt2(r.ipc);
  const fmt2 bm = to_fmt2(r.bmiss_rate * 100.0);
  line ln;
  ln.s_lj_at(r.op, 24);
  ln.s_lj_at(r.impl, 42);
  ln.u_at(r.size, 52);
  ln.f2_at(cpo, 64);
  ln.f2_at(ipc, 74);
  ln.f2_at(bm, 84);
  micron::io::println(ln.str());

  if ( r.unstable ) return;
  const char *reason = nullptr;
  if ( r.bmiss_rate > 0.05 )
    reason = "bmiss%>5 (predictor stress)";
  else if ( r.ipc > 0 && r.ipc < 0.7 )
    reason = "IPC<0.7 (memory-bound)";
  else if ( r.cyc_per_op > 1000.0 && !(r.op[0] == 'i' && r.op[1] == 'n' && r.op[2] == 's' && r.op[3] == 'e')
            && !(r.op[0] == 'e' && r.op[1] == 'r' && r.op[2] == 'a' && r.op[3] == 's') && !(r.op[0] == 'u' && r.op[1] == 'p') )
    reason = "cyc/op>1000 outside path-copy ops";

  if ( reason && g_anomaly_count < 256 ) {
    g_anomalies[g_anomaly_count++] = anomaly{ r.op, r.impl, r.size, r.cyc_per_op, r.ipc, r.bmiss_rate, reason };
  }
}

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

template<typename Setup, typename Kernel>
[[gnu::noinline]] row
measure(const char *op, const char *impl, u64 size, u64 ops_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel) noexcept
{

  try {
    for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
      setup();
      kernel();
    }
  } catch ( ... ) {
    return row{ op, impl, size, 0.0, 0.0, 0.0, true };
  }

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

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
      return row{ op, impl, size, 0.0, 0.0, 0.0, true };
    }
    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    const f64 total_ops = static_cast<f64>(reps_per_meas) * static_cast<f64>(ops_per_rep);
    cpo_samples[m] = total_ops > 0 ? static_cast<f64>(cyc) / total_ops : static_cast<f64>(cyc);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return row{ op,
              impl,
              size,
              median_f64(cpo_samples, K_MEASUREMENTS),
              median_f64(ipc_samples, K_MEASUREMENTS),
              median_f64(bm_samples, K_MEASUREMENTS),
              false };
}

[[gnu::always_inline]] inline u64
reps_for(u64 ops_per_rep) noexcept
{

  constexpr u64 TARGET = 1ULL << 17;
  if ( ops_per_rep == 0 ) return 16;
  u64 r = TARGET / ops_per_rep;
  if ( r < 2 ) r = 2;
  if ( r > 256 ) r = 256;
  return r;
}

template<typename M, typename Trait>
void
sweep_mutable_u64(const char *impl_tag, u64 N)
{

  auto make_empty = [&] { return Trait::make_empty(N); };

  {
    M m = make_empty();
    auto setup = [&] { m = make_empty(); };
    auto kernel = [&] {
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
      clobber(&m);
    };
    print_row(measure("insert (build)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i) + 1);
      clobber(&m);
    };
    print_row(measure("insert (replace)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        const auto *p = Trait::find(m, key_u64(i));
        if ( p ) acc += *p;
      }
      sink_u64 += acc;
    };
    print_row(measure("find (hit)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      u64 acc = 0;

      for ( u64 i = 0; i < N; ++i ) {
        const auto *p = Trait::find(m, key_u64(i + (1ULL << 40)));
        if ( p ) acc += *p;
      }
      sink_u64 += acc;
    };
    print_row(measure("find (miss)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        acc += static_cast<u64>(Trait::contains(m, key_u64(i)));
        acc += static_cast<u64>(Trait::contains(m, key_u64(i + (1ULL << 40))));
      }
      sink_u64 += acc;
    };
    print_row(measure("contains (mix)", impl_tag, N, 2 * N, reps_for(2 * N), setup, kernel));
  }

  if constexpr ( Trait::has_subscript ) {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) acc += Trait::subscript(m, key_u64(i));
      sink_u64 += acc;
    };
    print_row(measure("op[] (hit)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  if constexpr ( Trait::has_emplace ) {
    M m = make_empty();
    auto setup = [&] { m = make_empty(); };
    auto kernel = [&] {
      for ( u64 i = 0; i < N; ++i ) Trait::emplace(m, key_u64(i), val_u64(i));
      clobber(&m);
    };
    print_row(measure("emplace (build)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      for ( u64 i = 0; i < N; ++i ) Trait::erase(m, key_u64(i));
      clobber(&m);
    };
    print_row(measure("erase (drain)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      u64 acc = Trait::iterate_sum(m);
      sink_u64 += acc;
    };
    print_row(measure("iterate", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  if constexpr ( Trait::has_copy ) {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      M tmp(m);
      clobber(&tmp);
    };
    print_row(measure("copy-ctor", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    M src = make_empty();
    auto setup = [&] {
      src = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(src, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      M tmp(micron::move(src));
      src = micron::move(tmp);
      clobber(&src);
    };
    print_row(measure("move-ctor (pair)", impl_tag, N, N, reps_for(N) / 2 + 1, setup, kernel));
  }

  {
    M m = make_empty();
    auto setup = [&] {
      m = make_empty();
      for ( u64 i = 0; i < N; ++i ) Trait::insert(m, key_u64(i), val_u64(i));
    };
    auto kernel = [&] {
      Trait::clear(m);
      clobber(&m);
    };
    print_row(measure("clear (cyc/call)", impl_tag, N, 1, reps_for(N), setup, kernel));
  }
}

template<typename M, typename Trait>
void
sweep_immutable_u64(const char *impl_tag, u64 N)
{

  {
    M base;
    auto setup = [&] { base = M{}; };
    auto kernel = [&] {
      M cur = base;
      for ( u64 i = 0; i < N; ++i ) cur = Trait::insert(cur, key_u64(i), val_u64(i));
      sink_ptr(cur.identity());
    };
    print_row(measure("insert (build)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  M filled;
  for ( u64 i = 0; i < N; ++i ) filled = Trait::insert(filled, key_u64(i), val_u64(i));

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        const auto *p = Trait::find(filled, key_u64(i));
        if ( p ) acc += *p;
      }
      sink_u64 += acc;
    };
    print_row(measure("find (hit)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        const auto *p = Trait::find(filled, key_u64(i + (1ULL << 40)));
        if ( p ) acc += *p;
      }
      sink_u64 += acc;
    };
    print_row(measure("find (miss)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        acc += static_cast<u64>(Trait::contains(filled, key_u64(i)));
        acc += static_cast<u64>(Trait::contains(filled, key_u64(i + (1ULL << 40))));
      }
      sink_u64 += acc;
    };
    print_row(measure("contains (mix)", impl_tag, N, 2 * N, reps_for(2 * N), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) acc += Trait::subscript(filled, key_u64(i));
      sink_u64 += acc;
    };
    print_row(measure("op[] (hit)", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      M next = Trait::erase(filled, key_u64(0));
      sink_ptr(next.identity());
    };
    print_row(measure("erase (1 key)", impl_tag, N, 1, reps_for(1), setup, kernel));
  }

  if constexpr ( Trait::has_iter ) {
    auto setup = [] { };
    auto kernel = [&] {
      u64 acc = Trait::iterate_sum(filled);
      sink_u64 += acc;
    };
    print_row(measure("iterate", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      M tmp(filled);
      sink_ptr(tmp.identity());
    };
    print_row(measure("copy-ctor", impl_tag, N, 1, reps_for(1), setup, kernel));
  }

  if constexpr ( Trait::has_update ) {
    auto setup = [] { };
    auto kernel = [&] {
      M next = Trait::update(filled, key_u64(0));
      sink_ptr(next.identity());
    };
    print_row(measure("update (1 key)", impl_tag, N, 1, reps_for(1), setup, kernel));
  }
}

struct robin_trait_u64 {
  using map_t = micron::robin_map<u64, u64>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64 cap)
  {
    return map_t{ cap * 2 };
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(map_t &m, u64 k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(const map_t &m)
  {

    u64 acc = 0;
    m.for_each([&](const auto &n) { acc += n.value; });
    return acc;
  }
};

struct btree_trait_u64 {
  using map_t = micron::btree_map<u64, u64>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64 cap)
  {
    (void)cap;
    return map_t{};
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(map_t &m, u64 k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(map_t &m)
  {
    u64 acc = 0;
    for ( auto it = m.begin(); it != m.end(); ++it ) acc += (*it).value;
    return acc;
  }
};

struct hopscotch_trait_u64 {
  using map_t = micron::hopscotch_map<u64, u64>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64 cap)
  {
    (void)cap;
    return map_t{};
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(map_t &m, u64 k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(map_t &m)
  {
    u64 acc = 0;
    for ( auto it = m.begin(); it != m.end(); ++it )
      if ( *it ) acc += it->value;
    return acc;
  }
};

template<usize Cap> struct swiss_trait_u64 {
  using map_t = micron::stack_swiss_map<u64, u64, Cap>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = true;

  static map_t
  make_empty(u64)
  {
    return map_t{};
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(map_t &m, u64 k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(map_t &m)
  {

    u64 acc = 0;
    for ( usize i = 0; i < Cap; ++i ) {
      const u8 c = m.__control_bytes[i];
      if ( c != 0xFFu && c != 0xFEu ) acc += m.__entries[i].value;
    }
    return acc;
  }
};

struct immutable_trait_u64 {
  using map_t = micron::immutable_map<u64, u64>;
  static constexpr bool has_iter = true;
  static constexpr bool has_update = true;

  static map_t
  insert(const map_t &m, u64 k, u64 v)
  {
    return m.insert(k, u64{ v });
  }

  static const u64 *
  find(const map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(const map_t &m, u64 k)
  {
    return m[k];
  }

  static map_t
  erase(const map_t &m, u64 k)
  {
    return m.erase(k);
  }

  static u64
  iterate_sum(const map_t &m)
  {
    u64 acc = 0;
    for ( auto it = m.begin(); it != m.end(); ++it ) acc += it.value();
    return acc;
  }

  static map_t
  update(const map_t &m, u64 k)
  {
    return m.update(k, [](const u64 &v) { return v + 1; });
  }
};

struct itable_trait_u64 {
  using map_t = micron::immutable_table<u64, u64>;
  static constexpr bool has_iter = false;
  static constexpr bool has_update = true;

  static map_t
  insert(const map_t &m, u64 k, u64 v)
  {
    return m.insert(k, u64{ v });
  }

  static const u64 *
  find(const map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(const map_t &m, u64 k)
  {
    return m[k];
  }

  static map_t
  erase(const map_t &m, u64 k)
  {
    return m.erase(k);
  }

  static u64
  iterate_sum(const map_t &)
  {
    return 0;
  }

  static map_t
  update(const map_t &m, u64 k)
  {
    return m.update(k, [](const u64 &v) { return v + 1; });
  }
};

struct heap_swiss_trait_u64 {
  using map_t = micron::heap_swiss_map<u64, u64>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = true;

  static map_t
  make_empty(u64 cap)
  {
    return map_t{ cap * 2 };
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(map_t &m, u64 k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(map_t &m)
  {
    u64 acc = 0;
    m.for_each([&](const u64 &, const u64 &v) { acc += v; });
    return acc;
  }
};

struct conmap_trait_u64 {
  using map_t = micron::conmap<u64, u64>;
  static constexpr bool has_subscript = false;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64 cap)
  {
    return map_t{ cap * 4 };
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    static thread_local u64 cache;
    return m.find(k, cache) ? &cache : nullptr;
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(map_t &m)
  {
    u64 acc = 0;
    m.for_each([&](const u64 &, const u64 &v) { acc += v; });
    return acc;
  }
};

struct rb_map_trait_u64 {
  using map_t = micron::rb_map<u64, u64>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64 cap)
  {
    (void)cap;
    return map_t{};
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(map_t &m, u64 k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(map_t &m)
  {
    u64 acc = 0;
    m.for_each([&](const u64 &, const u64 &v) { acc += v; });
    return acc;
  }
};

struct art_trait_u64 {
  using map_t = micron::art<u64, u64>;
  static constexpr bool has_subscript = false;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64)
  {
    return map_t{};
  }

  static void
  insert(map_t &m, u64 k, u64 v)
  {
    m.insert_or_assign(k, u64{ v });
  }

  static u64 *
  find(map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static void
  emplace(map_t &m, u64 k, u64 v)
  {
    m.emplace(k, u64{ v });
  }

  static void
  erase(map_t &m, u64 k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static u64
  iterate_sum(map_t &m)
  {
    u64 acc = 0;
    m.for_each([&](const u64 &, const u64 &v) { acc += v; });
    return acc;
  }
};

struct pmap_trait_u64 {
  using map_t = micron::pmap<u64, u64>;
  static constexpr bool has_iter = true;
  static constexpr bool has_update = false;

  static map_t
  insert(const map_t &m, u64 k, u64 v)
  {
    return m.insert(k, u64{ v });
  }

  static const u64 *
  find(const map_t &m, u64 k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, u64 k)
  {
    return m.contains(k);
  }

  static u64
  subscript(const map_t &m, u64 k)
  {
    const u64 *p = m.find(k);
    return p ? *p : 0;
  }

  static map_t
  erase(const map_t &m, u64 k)
  {
    return m.erase(k);
  }

  static u64
  iterate_sum(const map_t &m)
  {
    u64 acc = 0;
    m.for_each([&](const u64 &, const u64 &v) { acc += v; });
    return acc;
  }
};

[[gnu::always_inline]] inline void
key_str(char *buf, u64 i) noexcept
{

  u64 v = splitmix64(i + 7);
  for ( u32 c = 0; c < 16; ++c ) {
    buf[c] = 'a' + static_cast<char>((v >> (4 * c)) & 0xF);
  }
  buf[16] = '\0';
}

struct robin_trait_hstr {
  using key_t = micron::hstring<char>;
  using val_t = i32;
  using map_t = micron::robin_map<key_t, val_t>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64 cap)
  {
    return map_t{ cap * 2 };
  }

  static void
  insert(map_t &m, const key_t &k, val_t v)
  {
    m.insert(k, val_t{ v });
  }

  static val_t *
  find(map_t &m, const key_t &k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, const key_t &k)
  {
    return m.contains(k);
  }

  static val_t
  subscript(map_t &m, const key_t &k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, const key_t &k, val_t v)
  {
    m.emplace(k, val_t{ v });
  }

  static void
  erase(map_t &m, const key_t &k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static i64
  iterate_sum(const map_t &m)
  {
    i64 acc = 0;
    for ( auto it = m.cbegin(); it != m.cend(); ++it ) acc += it->value;
    return acc;
  }
};

struct hopscotch_trait_hstr {
  using key_t = micron::hstring<char>;
  using val_t = i32;
  using map_t = micron::hopscotch_map<key_t, val_t>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64)
  {
    return map_t{};
  }

  static void
  insert(map_t &m, const key_t &k, val_t v)
  {
    m.insert(k, val_t{ v });
  }

  static val_t *
  find(map_t &m, const key_t &k)
  {
    return m.find(k);
  }

  static bool
  contains(map_t &m, const key_t &k)
  {
    return m.contains(k);
  }

  static val_t
  subscript(map_t &m, const key_t &k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, const key_t &k, val_t v)
  {
    m.emplace(k, val_t{ v });
  }

  static void
  erase(map_t &m, const key_t &k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static i64
  iterate_sum(map_t &m)
  {
    i64 acc = 0;
    for ( auto it = m.begin(); it != m.end(); ++it )
      if ( *it ) acc += it->value;
    return acc;
  }
};

struct btree_trait_hstr {
  using key_t = micron::hstring<char>;
  using val_t = i32;
  using map_t = micron::btree_map<key_t, val_t>;
  static constexpr bool has_subscript = true;
  static constexpr bool has_emplace = true;
  static constexpr bool has_copy = false;

  static map_t
  make_empty(u64)
  {
    return map_t{};
  }

  static void
  insert(map_t &m, const key_t &k, val_t v)
  {
    m.insert(k, val_t{ v });
  }

  static val_t *
  find(map_t &m, const key_t &k)
  {
    return m.find(k);
  }

  static bool
  contains(const map_t &m, const key_t &k)
  {
    return m.contains(k);
  }

  static val_t
  subscript(map_t &m, const key_t &k)
  {
    return m[k];
  }

  static void
  emplace(map_t &m, const key_t &k, val_t v)
  {
    m.emplace(k, val_t{ v });
  }

  static void
  erase(map_t &m, const key_t &k)
  {
    m.erase(k);
  }

  static void
  clear(map_t &m)
  {
    m.clear();
  }

  static i64
  iterate_sum(map_t &m)
  {
    i64 acc = 0;
    for ( auto it = m.begin(); it != m.end(); ++it ) acc += (*it).value;
    return acc;
  }
};

template<typename Trait>
void
sweep_hstr_map(const char *impl_tag, u64 N)
{
  using map_t = typename Trait::map_t;

  {
    map_t m = Trait::make_empty(N);
    auto setup = [&] { m = Trait::make_empty(N); };
    auto kernel = [&] {
      char buf[32];
      for ( u64 i = 0; i < N; ++i ) {
        key_str(buf, i);
        Trait::insert(m, typename Trait::key_t{ buf }, static_cast<typename Trait::val_t>(i));
      }
      clobber(&m);
    };
    print_row(measure("insert (build)", impl_tag, N, N, reps_for(N * 4), setup, kernel));
  }

  map_t filled = Trait::make_empty(N);
  auto rebuild = [&] {
    filled = Trait::make_empty(N);
    char buf[32];
    for ( u64 i = 0; i < N; ++i ) {
      key_str(buf, i);
      Trait::insert(filled, typename Trait::key_t{ buf }, static_cast<typename Trait::val_t>(i));
    }
  };

  {
    auto setup = [&] { rebuild(); };
    auto kernel = [&] {
      char buf[32];
      i64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {
        key_str(buf, i);
        const auto *p = Trait::find(filled, typename Trait::key_t{ buf });
        if ( p ) acc += *p;
      }
      sink_u64 += static_cast<u64>(acc);
    };
    print_row(measure("find (hit)", impl_tag, N, N, reps_for(N * 4), setup, kernel));
  }

  {
    auto setup = [&] { rebuild(); };
    auto kernel = [&] {
      char buf[32];
      i64 acc = 0;
      for ( u64 i = 0; i < N; ++i ) {

        key_str(buf, i + (1ULL << 40));
        const auto *p = Trait::find(filled, typename Trait::key_t{ buf });
        if ( p ) acc += *p;
      }
      sink_u64 += static_cast<u64>(acc);
    };
    print_row(measure("find (miss)", impl_tag, N, N, reps_for(N * 4), setup, kernel));
  }

  {
    auto setup = [&] { rebuild(); };
    auto kernel = [&] {
      char buf[32];
      for ( u64 i = 0; i < N; ++i ) {
        key_str(buf, i);
        Trait::erase(filled, typename Trait::key_t{ buf });
      }
      clobber(&filled);
    };
    print_row(measure("erase (drain)", impl_tag, N, N, reps_for(N * 4), setup, kernel));
  }

  {
    auto setup = [&] { rebuild(); };
    auto kernel = [&] {
      i64 acc = Trait::iterate_sum(filled);
      sink_u64 += static_cast<u64>(acc);
    };
    print_row(measure("iterate", impl_tag, N, N, reps_for(N), setup, kernel));
  }

  {
    auto setup = [&] { rebuild(); };
    auto kernel = [&] {
      Trait::clear(filled);
      clobber(&filled);
    };
    print_row(measure("clear", impl_tag, N, N, reps_for(N), setup, kernel));
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

  micron::io::println("=== micron maps benchmark ===");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements per cell (median)");

  constexpr u64 SIZES_SMALL[] = { 64, 256, 1024 };
  constexpr u64 SIZES_MED[] = { 64, 256, 1024 };
  constexpr u64 SIZES_LARGE[] = { 256, 1024, 4096 };

  print_header("robin_map<u64,u64>");
  for ( u64 N : SIZES_LARGE ) sweep_mutable_u64<typename robin_trait_u64::map_t, robin_trait_u64>("robin", N);

  print_header("heap_swiss_map<u64,u64>");
  for ( u64 N : SIZES_LARGE ) sweep_mutable_u64<typename heap_swiss_trait_u64::map_t, heap_swiss_trait_u64>("heap-swiss", N);

  print_header("conmap<u64,u64>");
  for ( u64 N : SIZES_LARGE ) sweep_mutable_u64<typename conmap_trait_u64::map_t, conmap_trait_u64>("conmap", N);

  print_header("rb_map<u64,u64>");
  for ( u64 N : SIZES_LARGE ) sweep_mutable_u64<typename rb_map_trait_u64::map_t, rb_map_trait_u64>("rb_map", N);

  print_header("art<u64,u64>");
  for ( u64 N : SIZES_LARGE ) sweep_mutable_u64<typename art_trait_u64::map_t, art_trait_u64>("art", N);

  print_header("btree_map<u64,u64>");
  for ( u64 N : SIZES_LARGE ) sweep_mutable_u64<typename btree_trait_u64::map_t, btree_trait_u64>("btree", N);

  print_header("hopscotch_map<u64,u64>");
  for ( u64 N : SIZES_LARGE ) sweep_mutable_u64<typename hopscotch_trait_u64::map_t, hopscotch_trait_u64>("hopscotch", N);

  print_header("stack_swiss_map<u64,u64> N=2048 capacity");
  using swiss_2k = swiss_trait_u64<2048>;
  for ( u64 N : SIZES_SMALL ) sweep_mutable_u64<typename swiss_2k::map_t, swiss_2k>("swiss-2k", N);

  print_header("immutable_map<u64,u64>");
  for ( u64 N : SIZES_MED ) sweep_immutable_u64<typename immutable_trait_u64::map_t, immutable_trait_u64>("immutable", N);

  print_header("immutable_table<u64,u64>");
  for ( u64 N : SIZES_MED ) sweep_immutable_u64<typename itable_trait_u64::map_t, itable_trait_u64>("itable", N);

  print_header("pmap<u64,u64>");
  for ( u64 N : SIZES_MED ) sweep_immutable_u64<typename pmap_trait_u64::map_t, pmap_trait_u64>("pmap", N);

  constexpr u64 HSTR_SIZES[] = { 64, 256 };
  print_header("robin_map<hstring,i32>");
  for ( u64 N : HSTR_SIZES ) sweep_hstr_map<robin_trait_hstr>("robin", N);

  print_header("hopscotch_map<hstring,i32>");
  for ( u64 N : HSTR_SIZES ) sweep_hstr_map<hopscotch_trait_hstr>("hopscotch", N);

  print_header("btree_map<hstring,i32>");
  for ( u64 N : HSTR_SIZES ) sweep_hstr_map<btree_trait_hstr>("btree", N);

  micron::io::println("");
  micron::io::println("[anomalies] (rows flagged during run)");
  if ( g_anomaly_count == 0 ) {
    micron::io::println("  (none)");
  } else {
    line head;
    head.s("  ");
    head.s_at("op", 26);
    head.s_lj_at("impl", 44);
    head.s_at("N", 54);
    head.s_at("cyc/op", 66);
    head.s_at("IPC", 76);
    head.s_at("bmiss%", 86);
    head.s("  ");
    head.s("reason");
    micron::io::println(head.str());
    for ( u32 i = 0; i < g_anomaly_count; ++i ) {
      const auto &a = g_anomalies[i];
      const fmt2 cpo = to_fmt2(a.cyc_per_op);
      const fmt2 ipc = to_fmt2(a.ipc);
      const fmt2 bm = to_fmt2(a.bmiss_rate * 100.0);
      line ln;
      ln.s("  ");
      ln.s_lj_at(a.op, 26);
      ln.s_lj_at(a.impl, 44);
      ln.u_at(a.size, 54);
      ln.f2_at(cpo, 66);
      ln.f2_at(ipc, 76);
      ln.f2_at(bm, 86);
      ln.s("  ");
      ln.s(a.reason);
      micron::io::println(ln.str());
    }
  }
  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}
