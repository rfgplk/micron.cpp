//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// lazy_bench.cpp -- micron::lz against the floor and the ceiling.
//
// THREE ARMS PER CHAIN, and the middle one has to sit between the other two:
//
//   hand    one hand-written fused loop -- the floor, what the compiler could do if you wrote it out
//   lazy    src | lz::... | terminal    -- the thing being measured
//   eager   the fp:: / micron:: equivalent -- the ceiling it has to beat (house rule 5)
//
// The gates, from the Phase 3/4 handoff:
//   1. lazy <= eager on every chain, every size. No exceptions.
//   2. lazy ~= hand (inside the noise floor) on size-preserving chains.
//   3. IPC must not collapse relative to hand. This is the INLINING SENTINEL: when a view stops
//      being register-resident, IPC falls before cyc/op rises, and it falls loudly.
//
// The three arms share one templated kernel with an `if constexpr` per arm, so they cannot differ
// by an inlining accident in the call shape -- the shape is identical by construction.
//
// Build and run EXACTLY:
//   duck build benches/lazy_bench.cpp --perf --fp --no-ssp --no-lto -o bin -f
//   taskset -c 0 ./bin/lazy_bench
// All four flags matter. duck defaults to -fstack-protector-all, and a canary on every function does
// not cancel out of a ratio. -f matters because this measures HEADERS and duck keys its cache off
// the .cpp mtime. bbench needs kernel.perf_event_paranoid <= 2.
//
// DISCARD THE FIRST RUN. Same-binary spread is ~3%, and 10%+ at N <= 64.

#include "../external/bbench/bench.hpp"

#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/filter.hpp"
#include "../src/algorithm/fold.hpp"
#include "../src/algorithm/fp.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/lz.hpp"
#include "../src/sort/sort.hpp"
#include "../src/vector/vector.hpp"

namespace
{

namespace lz = micron::lz;
using vec_i = micron::vector<i32>;
using vec_d = micron::vector<f64>;
using vec_vi = micron::vector<vec_i>;

using c_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 3;
constexpr u64 SIZES[] = { 64, 1024, 16384 };
constexpr u64 MAXN = 16384;

static volatile u64 sink_u64 = 0;
static volatile f64 sink_f64 = 0.0;

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// formatting -- fixed columns, no float printer needed beyond two decimals

struct fmt2 {
  u64 whole;
  u32 frac;
};

fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  if ( v > 1e15 ) v = 1e15;
  const u64 w = static_cast<u64>(v);
  u32 f = static_cast<u32>((v - static_cast<f64>(w)) * 100.0 + 0.5);
  if ( f >= 100 ) return fmt2{ w + 1, 0 };
  return fmt2{ w, f };
}

struct line {
  char buf[256];
  u32 pos = 0;

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  pad_to(u32 col) noexcept
  {
    while ( pos < col ) buf[pos++] = ' ';
  }

  void
  s_lj_at(const char *p, u32 col) noexcept
  {
    s(p);
    pad_to(col);
  }

  void
  u_at(u64 v, u32 col) noexcept
  {
    char t[24];
    u32 n = 0;
    if ( !v )
      t[n++] = '0';
    else
      for ( u64 x = v; x; x /= 10 ) t[n++] = static_cast<char>('0' + (x % 10));
    while ( pos + n < col ) buf[pos++] = ' ';
    while ( n ) buf[pos++] = t[--n];
  }

  void
  f2_at(f64 v, u32 col) noexcept
  {
    const fmt2 f = to_fmt2(v);
    char t[24];
    u32 n = 0;
    if ( !f.whole )
      t[n++] = '0';
    else
      for ( u64 x = f.whole; x; x /= 10 ) t[n++] = static_cast<char>('0' + (x % 10));
    const u32 width = n + 3;
    while ( pos + width < col ) buf[pos++] = ' ';
    while ( n ) buf[pos++] = t[--n];
    buf[pos++] = '.';
    buf[pos++] = static_cast<char>('0' + (f.frac / 10));
    buf[pos++] = static_cast<char>('0' + (f.frac % 10));
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

f64
median_f64(f64 *xs, u32 n) noexcept
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

struct sample {
  f64 cyc_per_elem;
  f64 ipc;
};

template<typename Kernel>
[[gnu::noinline]] sample
measure(u64 elems_per_rep, u64 reps, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) kernel();

  f64 cpe[K_MEASUREMENTS];
  f64 ipc[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    c_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < reps; ++i ) kernel();
    evs.end();
    const auto cyc = static_cast<f64>(static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve()));
    const auto ins = static_cast<f64>(static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve()));
    const f64 total = static_cast<f64>(reps) * static_cast<f64>(elems_per_rep);
    cpe[m] = total > 0 ? cyc / total : cyc;
    ipc[m] = cyc > 0 ? ins / cyc : 0.0;
  }
  return sample{ median_f64(cpe, K_MEASUREMENTS), median_f64(ipc, K_MEASUREMENTS) };
}

[[gnu::always_inline]] inline u64
reps_for(u64 n) noexcept
{
  constexpr u64 TARGET = 1ULL << 21;
  u64 r = n ? TARGET / n : 64;
  if ( r < 4 ) r = 4;
  if ( r > 1ULL << 16 ) r = 1ULL << 16;
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the data

static vec_i g_a;
static vec_i g_b;
static vec_d g_f;
static vec_vi g_nested;

void
setup_data()
{
  u64 s = 0x9E3779B97F4A7C15ULL;
  auto nxt = [&]() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
  };
  for ( u64 i = 0; i < MAXN; ++i ) {
    g_a.push_back(static_cast<i32>(nxt() % 2000) - 1000);
    g_b.push_back(static_cast<i32>(nxt() % 2000) - 1000);
    g_f.push_back(static_cast<f64>(static_cast<i32>(nxt() % 2000) - 1000) * 0.125);
  }
  for ( u64 i = 0; i < MAXN / 8; ++i ) {
    vec_i row;
    for ( u64 k = 0; k < 8; ++k ) row.push_back(static_cast<i32>(i * 8 + k));
    g_nested.push_back(micron::move(row));
  }
}

// micron::vector has no iterator-pair constructor, and the eager arms all need a fresh prefix copy
// of the input -- which is itself part of what they cost, so it stays inside the timed region.
template<typename C>
[[gnu::always_inline]] inline C
prefix(const C &__src, u64 __n)
{
  C __o(__n);
  for ( u64 i = 0; i < __n; ++i ) __o[i] = __src[i];
  return __o;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the chains

enum class arm : int { hand = 0, lazy = 1, eager = 2 };

enum class chain : int {
  fmap_collect,
  fmap2_collect,
  fmap_fold,
  sum_i32,
  sum_f64,
  count_if,
  any_of_late,
  filter_collect,
  filter_fmap_take,
  add_scalar,
  zip_fold,
  enumerate_filter,
  reverse_sum,
  sort_collect,
  chunk_count,
  flatten_sum,
  COUNT
};

const char *
chain_name(chain c) noexcept
{
  switch ( c ) {
  case chain::fmap_collect: return "fmap|collect";
  case chain::fmap2_collect: return "fmap|fmap|collect";
  case chain::fmap_fold: return "fmap|fold";
  case chain::sum_i32: return "sum<i32>";
  case chain::sum_f64: return "sum<f64>";
  case chain::count_if: return "count_if";
  case chain::any_of_late: return "any_of (late hit)";
  case chain::filter_collect: return "filter|collect";
  case chain::filter_fmap_take: return "filter|fmap|take(k)";
  case chain::add_scalar: return "add(y)|collect";
  case chain::zip_fold: return "zip_with|fold";
  case chain::enumerate_filter: return "enumerate|filter|collect";
  case chain::reverse_sum: return "reverse|sum";
  case chain::sort_collect: return "sort|collect";
  case chain::chunk_count: return "chunk(16)|count";
  case chain::flatten_sum: return "flatten|sum";
  default: return "?";
  }
}

template<chain C, arm A>
[[gnu::always_inline]] inline void
run(u64 n)
{
  const i32 *__restrict a = g_a.begin();
  const i32 *__restrict b = g_b.begin();
  const f64 *__restrict f = g_f.begin();
  auto av = lz::ptr_view<i32>{ a, a + n };
  auto bv = lz::ptr_view<i32>{ b, b + n };
  auto fv = lz::ptr_view<f64>{ f, f + n };

  if constexpr ( C == chain::fmap_collect ) {
    if constexpr ( A == arm::hand ) {
      vec_i out(n);
      i32 *__restrict d = out.begin();
      for ( u64 i = 0; i < n; ++i ) d[i] = a[i] * 3 + 1;
      clobber(out.begin());
    } else if constexpr ( A == arm::lazy ) {
      auto out = av | lz::fmap([](i32 x) { return x * 3 + 1; }) | lz::collect<vec_i>();
      clobber(out.begin());
    } else {
      auto out = micron::fp::fmap([](i32 x) { return x * 3 + 1; }, prefix(g_a, n));
      clobber(out.begin());
    }
  } else if constexpr ( C == chain::fmap2_collect ) {
    if constexpr ( A == arm::hand ) {
      vec_i out(n);
      i32 *__restrict d = out.begin();
      for ( u64 i = 0; i < n; ++i ) d[i] = (a[i] * 3 + 1) ^ 0x5a;
      clobber(out.begin());
    } else if constexpr ( A == arm::lazy ) {
      auto out = av | lz::fmap([](i32 x) { return x * 3 + 1; }) | lz::fmap([](i32 x) { return x ^ 0x5a; })
                 | lz::collect<vec_i>();
      clobber(out.begin());
    } else {
      auto out = micron::fp::fmap([](i32 x) { return x ^ 0x5a; },
                                  micron::fp::fmap([](i32 x) { return x * 3 + 1; }, prefix(g_a, n)));
      clobber(out.begin());
    }
  } else if constexpr ( C == chain::fmap_fold ) {
    if constexpr ( A == arm::hand ) {
      i64 acc = 0;
      for ( u64 i = 0; i < n; ++i ) acc += a[i] * 3 + 1;
      sink_u64 += static_cast<u64>(acc);
    } else if constexpr ( A == arm::lazy ) {
      sink_u64 += static_cast<u64>(av | lz::fmap([](i32 x) { return static_cast<i64>(x) * 3 + 1; })
                                   | lz::fold(static_cast<i64>(0), [](i64 s, i64 x) { return s + x; }));
    } else {
      auto m = micron::fp::fmap([](i32 x) { return x * 3 + 1; }, prefix(g_a, n));
      sink_u64 += static_cast<u64>(micron::fold_left(m, static_cast<i64>(0), [](i64 s, const i32 *x) { return s + *x; }));
    }
  } else if constexpr ( C == chain::sum_i32 ) {
    if constexpr ( A == arm::hand ) {
      umax_t w = 0, x = 0, y = 0, z = 0;
      u64 i = 0;
      for ( ; i + 4 <= n; i += 4 ) {
        w += static_cast<umax_t>(a[i]);
        x += static_cast<umax_t>(a[i + 1]);
        y += static_cast<umax_t>(a[i + 2]);
        z += static_cast<umax_t>(a[i + 3]);
      }
      for ( ; i < n; ++i ) w += static_cast<umax_t>(a[i]);
      sink_u64 += static_cast<u64>((w + x) + (y + z));
    } else if constexpr ( A == arm::lazy ) {
      sink_u64 += static_cast<u64>(av | lz::sum());
    } else {
      sink_u64 += static_cast<u64>(micron::sum(prefix(g_a, n)));
    }
  } else if constexpr ( C == chain::sum_f64 ) {
    if constexpr ( A == arm::hand ) {
      // Neumaier, four lanes -- the SAME algorithm micron::sum runs. a bare `acc += f[i]` would
      // vectorise to one add chain and read ~3x faster, but it computes a different, less accurate
      // answer; timing it as the floor would be measuring the wrong thing.
      f64 s4[4] = { 0, 0, 0, 0 };
      f64 c4[4] = { 0, 0, 0, 0 };
      u64 i = 0;
      for ( ; i + 4 <= n; i += 4 )
        for ( u32 k = 0; k < 4; ++k ) micron::__impl::__neumaier_add(s4[k], c4[k], f[i + k]);
      for ( ; i < n; ++i ) micron::__impl::__neumaier_add(s4[0], c4[0], f[i]);
      f64 acc = 0, comp = 0;
      for ( u32 k = 0; k < 4; ++k ) {
        micron::__impl::__neumaier_add(acc, comp, s4[k]);
        comp += c4[k];
      }
      sink_f64 += acc + comp;
    } else if constexpr ( A == arm::lazy ) {
      sink_f64 += static_cast<f64>(fv | lz::sum());
    } else {
      sink_f64 += static_cast<f64>(micron::sum(prefix(g_f, n)));
    }
  } else if constexpr ( C == chain::count_if ) {
    if constexpr ( A == arm::hand ) {
      u64 c = 0;
      for ( u64 i = 0; i < n; ++i ) c += (a[i] > 0);
      sink_u64 += c;
    } else if constexpr ( A == arm::lazy ) {
      sink_u64 += static_cast<u64>(av | lz::count_if([](i32 x) { return x > 0; }));
    } else {
      sink_u64 += static_cast<u64>(micron::count_if(prefix(g_a, n), [](const i32 *x) { return *x > 0; }));
    }
  } else if constexpr ( C == chain::any_of_late ) {
    // the hit is in the last quarter, so short-circuiting is measurable but not degenerate
    const i32 target = a[n - (n / 4) - 1];
    if constexpr ( A == arm::hand ) {
      bool hit = false;
      for ( u64 i = 0; i < n; ++i )
        if ( a[i] == target ) {
          hit = true;
          break;
        }
      sink_u64 += hit;
    } else if constexpr ( A == arm::lazy ) {
      sink_u64 += static_cast<u64>(av | lz::any_of([target](i32 x) { return x == target; }));
    } else {
      sink_u64 += static_cast<u64>(micron::fp::any_of_c([target](i32 x) { return x == target; })(prefix(g_a, n)));
    }
  } else if constexpr ( C == chain::filter_collect ) {
    if constexpr ( A == arm::hand ) {
      vec_i out(n);
      i32 *__restrict d = out.begin();
      u64 k = 0;
      for ( u64 i = 0; i < n; ++i )
        if ( a[i] > 0 ) d[k++] = a[i];
      out.resize(k);
      clobber(out.begin());
    } else if constexpr ( A == arm::lazy ) {
      auto out = av | lz::filter([](i32 x) { return x > 0; }) | lz::collect<vec_i>();
      clobber(out.begin());
    } else {
      auto out = micron::filter(prefix(g_a, n), [](const i32 *x) { return *x > 0; });
      clobber(out.begin());
    }
  } else if constexpr ( C == chain::filter_fmap_take ) {
    const u64 k = n / 8;
    if constexpr ( A == arm::hand ) {
      vec_i out(k);
      i32 *__restrict d = out.begin();
      u64 w = 0;
      for ( u64 i = 0; i < n && w < k; ++i )
        if ( a[i] > 0 ) d[w++] = a[i] * 3;
      out.resize(w);
      clobber(out.begin());
    } else if constexpr ( A == arm::lazy ) {
      auto out = av | lz::filter([](i32 x) { return x > 0; }) | lz::fmap([](i32 x) { return x * 3; }) | lz::take(k)
                 | lz::collect<vec_i>();
      clobber(out.begin());
    } else {
      // this is the shape the whole layer exists for: eager filters ALL n, maps ALL of them, then
      // throws away everything past k -- three containers for one eighth of an answer
      auto out = micron::fp::take(micron::fp::fmap([](i32 x) { return x * 3; },
                                                   micron::filter(prefix(g_a, n),
                                                                  [](const i32 *x) { return *x > 0; })),
                                  k);
      clobber(out.begin());
    }
  } else if constexpr ( C == chain::add_scalar ) {
    if constexpr ( A == arm::hand ) {
      vec_i out(n);
      i32 *__restrict d = out.begin();
      for ( u64 i = 0; i < n; ++i ) d[i] = a[i] + 7;
      clobber(out.begin());
    } else if constexpr ( A == arm::lazy ) {
      auto out = av | lz::add(7) | lz::collect<vec_i>();
      clobber(out.begin());
    } else {
      auto out = micron::fp::add_c(7)(prefix(g_a, n));
      clobber(out.begin());
    }
  } else if constexpr ( C == chain::zip_fold ) {
    if constexpr ( A == arm::hand ) {
      i64 acc = 0;
      for ( u64 i = 0; i < n; ++i ) acc += static_cast<i64>(a[i]) * static_cast<i64>(b[i]);
      sink_u64 += static_cast<u64>(acc);
    } else if constexpr ( A == arm::lazy ) {
      sink_u64 += static_cast<u64>(av | lz::inner_product<i64>(bv, static_cast<i64>(0)));
    } else {
      sink_u64 += static_cast<u64>(micron::fp::inner_product<vec_i, i64>(prefix(g_a, n),
                                                                         prefix(g_b, n),
                                                                         static_cast<i64>(0)));
    }
  } else if constexpr ( C == chain::enumerate_filter ) {
    if constexpr ( A == arm::hand ) {
      vec_i out(n);
      i32 *__restrict d = out.begin();
      u64 w = 0;
      for ( u64 i = 0; i < n; ++i )
        if ( a[i] > 0 ) d[w++] = static_cast<i32>(i);
      out.resize(w);
      clobber(out.begin());
    } else if constexpr ( A == arm::lazy ) {
      auto out = av | lz::enumerate() | lz::filter([](auto t) { return micron::get<1>(t) > 0; })
                 | lz::fmap([](auto t) { return static_cast<i32>(micron::get<0>(t)); }) | lz::collect<vec_i>();
      clobber(out.begin());
    } else {
      using vt = micron::vector<micron::tuple<usize, i32>>;
      auto en = micron::fp::enumerate<vt>(prefix(g_a, n));
      vec_i out;
      out.reserve(n);
      for ( usize i = 0; i < en.size(); ++i )
        if ( micron::get<1>(en[i]) > 0 ) out.push_back(static_cast<i32>(micron::get<0>(en[i])));
      clobber(out.begin());
    }
  } else if constexpr ( C == chain::reverse_sum ) {
    if constexpr ( A == arm::hand ) {
      i64 acc = 0;
      for ( u64 i = n; i-- > 0; ) acc += a[i];
      sink_u64 += static_cast<u64>(acc);
    } else if constexpr ( A == arm::lazy ) {
      sink_u64 += static_cast<u64>(av | lz::reverse() | lz::sum());
    } else {
      sink_u64 += static_cast<u64>(micron::sum(micron::fp::reverse_c()(prefix(g_a, n))));
    }
  } else if constexpr ( C == chain::sort_collect ) {
    if constexpr ( A == arm::hand ) {
      vec_i out = prefix(g_a, n);
      micron::sort::sort(out);
      clobber(out.begin());
    } else if constexpr ( A == arm::lazy ) {
      auto out = av | lz::sort() | lz::collect<vec_i>();
      clobber(out.begin());
    } else {
      auto out = micron::fp::sort_c()(prefix(g_a, n));
      clobber(out.begin());
    }
  } else if constexpr ( C == chain::chunk_count ) {
    if constexpr ( A == arm::hand ) {
      u64 c = 0;
      for ( u64 i = 0; i < n; i += 16 ) ++c;
      sink_u64 += c;
    } else if constexpr ( A == arm::lazy ) {
      sink_u64 += static_cast<u64>(av | lz::chunk(16) | lz::count());
    } else {
      auto out = micron::fp::chunk_into<vec_vi>(prefix(g_a, n), 16);
      sink_u64 += out.size();
    }
  } else if constexpr ( C == chain::flatten_sum ) {
    const u64 rows = n / 8;
    if constexpr ( A == arm::hand ) {
      i64 acc = 0;
      for ( u64 r = 0; r < rows; ++r ) {
        const i32 *__restrict p = g_nested[r].begin();
        for ( u64 k = 0; k < 8; ++k ) acc += p[k];
      }
      sink_u64 += static_cast<u64>(acc);
    } else if constexpr ( A == arm::lazy ) {
      auto nv = lz::ptr_view<vec_i>{ g_nested.begin(), g_nested.begin() + rows };
      sink_u64 += static_cast<u64>(nv | lz::flatten() | lz::sum());
    } else {
      vec_vi sub;
      sub.reserve(rows);
      for ( u64 r = 0; r < rows; ++r ) sub.push_back(g_nested[r]);
      sink_u64 += static_cast<u64>(micron::sum(micron::fp::flatten(sub)));
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reporting

void
print_header()
{
  micron::io::println("");
  line h;
  h.s_lj_at("chain", 26);
  h.s_lj_at("N", 34);
  h.s_lj_at("hand", 45);
  h.s_lj_at("lazy", 56);
  h.s_lj_at("eager", 67);
  h.s_lj_at("lazy/hand", 79);
  h.s_lj_at("lazy/eager", 92);
  h.s_lj_at("IPCh", 99);
  h.s_lj_at("IPCl", 106);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------------------------------------");
}

void
print_row(chain c, u64 n, sample h, sample l, sample e)
{
  line ln;
  ln.s_lj_at(chain_name(c), 26);
  ln.u_at(n, 33);
  ln.f2_at(h.cyc_per_elem, 45);
  ln.f2_at(l.cyc_per_elem, 56);
  ln.f2_at(e.cyc_per_elem, 67);
  ln.f2_at(h.cyc_per_elem > 0 ? l.cyc_per_elem / h.cyc_per_elem : 0.0, 77);
  ln.f2_at(e.cyc_per_elem > 0 ? l.cyc_per_elem / e.cyc_per_elem : 0.0, 90);
  ln.f2_at(h.ipc, 99);
  ln.f2_at(l.ipc, 106);
  micron::io::println(ln.str());
}

template<chain C>
void
sweep_chain()
{
  for ( u64 si = 0; si < sizeof(SIZES) / sizeof(SIZES[0]); ++si ) {
    const u64 n = SIZES[si];
    const u64 r = reps_for(n);
    const sample h = measure(n, r, [n]() { run<C, arm::hand>(n); });
    const sample l = measure(n, r, [n]() { run<C, arm::lazy>(n); });
    const sample e = measure(n, r, [n]() { run<C, arm::eager>(n); });
    print_row(C, n, h, l, e);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// verification -- the three arms must agree BEFORE any of them is timed. a bench whose arms compute
// different things is worse than no bench.

bool
verify()
{
  const u64 n = 1024;
  const i32 *a = g_a.begin();
  auto av = lz::ptr_view<i32>{ a, a + n };

  bool ok = true;
  {
    vec_i hand(n);
    for ( u64 i = 0; i < n; ++i ) hand[i] = a[i] * 3 + 1;
    auto lazy = av | lz::fmap([](i32 x) { return x * 3 + 1; }) | lz::collect<vec_i>();
    auto eager = micron::fp::fmap([](i32 x) { return x * 3 + 1; }, prefix(g_a, n));
    if ( lazy.size() != n || eager.size() != n ) ok = false;
    for ( u64 i = 0; i < n && ok; ++i )
      if ( hand[i] != lazy[i] || hand[i] != eager[i] ) ok = false;
  }
  {
    i64 hand = 0;
    for ( u64 i = 0; i < n; ++i ) hand += a[i];
    if ( static_cast<umax_t>(hand) != (av | lz::sum()) ) ok = false;
    if ( (av | lz::sum()) != micron::sum(prefix(g_a, n)) ) ok = false;
  }
  {
    u64 hand = 0;
    for ( u64 i = 0; i < n; ++i ) hand += (a[i] > 0);
    if ( hand != (av | lz::count_if([](i32 x) { return x > 0; })) ) ok = false;
  }
  {
    auto lazy = av | lz::filter([](i32 x) { return x > 0; }) | lz::collect<vec_i>();
    auto eager = micron::filter(prefix(g_a, n), [](const i32 *x) { return *x > 0; });
    if ( lazy.size() != eager.size() ) ok = false;
    for ( usize i = 0; i < lazy.size() && ok; ++i )
      if ( lazy[i] != eager[i] ) ok = false;
  }
  {
    auto lazy = av | lz::sort() | lz::collect<vec_i>();
    auto eager = micron::fp::sort_c()(prefix(g_a, n));
    if ( lazy.size() != eager.size() ) ok = false;
    for ( usize i = 0; i < lazy.size() && ok; ++i )
      if ( lazy[i] != eager[i] ) ok = false;
  }
  {
    if ( (av | lz::reverse() | lz::sum()) != (av | lz::sum()) ) ok = false;
  }
  return ok;
}

};      // namespace

int
main()
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  setup_data();

  if ( !verify() ) {
    micron::io::println("VERIFY FAILED -- the arms disagree; the numbers below would be meaningless");
    return 1;
  }

  micron::io::println("=== MICRON LZ vs HAND vs EAGER (median-of-5, cyc/elem) ===");
  micron::io::println("");
  micron::io::println("gate 1: lazy/eager must be <= 1.00 everywhere");
  micron::io::println("gate 2: lazy/hand must be ~1.00 on the size-preserving chains (*)");
  micron::io::println("gate 3: IPCl must not collapse against IPCh");

  print_header();
  sweep_chain<chain::fmap_collect>();
  sweep_chain<chain::fmap2_collect>();
  sweep_chain<chain::fmap_fold>();
  sweep_chain<chain::sum_i32>();
  sweep_chain<chain::sum_f64>();
  sweep_chain<chain::count_if>();
  sweep_chain<chain::add_scalar>();
  sweep_chain<chain::zip_fold>();
  sweep_chain<chain::reverse_sum>();

  micron::io::println("");
  micron::io::println("[size-changing / buffered chains -- gate 2 does not apply]");
  print_header();
  sweep_chain<chain::any_of_late>();
  sweep_chain<chain::filter_collect>();
  sweep_chain<chain::filter_fmap_take>();
  sweep_chain<chain::enumerate_filter>();
  sweep_chain<chain::sort_collect>();
  sweep_chain<chain::chunk_count>();
  sweep_chain<chain::flatten_sum>();

  micron::io::println("");
  micron::io::print("[done] sink=");
  micron::io::print(static_cast<u64>(sink_u64));
  micron::io::print(" f=");
  micron::io::println(static_cast<i64>(sink_f64));
  return 0;
}
