//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Array benchmark — exercises every stack/heap array variant under src/array/
// against an extensive per-API battery so the resulting table is a direct
// cross-impl comparison rather than per-impl micro-reports.
//
//   arrays under test:
//     array<T, N>           mutable, fixed stack
//     iarray<T, N>          single-owner ("immutable" handle), fixed stack
//     farray<T, N>          mutable, fundamental-only, fixed stack
//     conarray<T, N>        like array, with thread-safety hooks
//     carray<T, N>          like array, with destroy_fast on dtor
//     constexpr_array<T, N> constexpr-friendly, fixed stack
//     bisect_array<T, N>    auto-sorted, fixed stack, insert/erase API
//     parray<T, K, H>       persistent COW tree, heap-backed
//
//   per (impl, op, N, T) cell — cyc/op, IPC, branch-miss%; medians across
//   K_MEASUREMENTS samples, bbench 4-event group.
//
//   operations covered (every public method that has a meaningful cost):
//     iterate-sum [i]       index-based read sweep
//     iterate-sum (iter)    iterator-based read sweep (where applicable)
//     copy-ctor             container-level deep copy
//     copy-assign           deep-copy assignment
//     move-ctor (pair)      move construct then move-restore (so size stays)
//     fill(value)           micron::ctypeset / memset path
//     generate              Fn-based ctor / generate algo
//     transform             Fn-based in-place transform
//     element write         scalar [i] = v sweep
//     equality              == on equal arrays (worst case)
//     scalar += array       compound scalar add (vectorisable)
//     array  +  array       binary array add (allocates result)
//     sum                   reduction
//     mul_reduce            product reduction
//     all / any             scalar predicate sweep
//     insert (bisect)       bisect_array::insert + matching erase
//     erase  (bisect)       bisect_array::erase one
//     set (parray)          persistent COW set, returns new tree
//     get (parray)          persistent COW indexed read
//     fill (parray)         broadcast-ctor (returns new tree)
//
//   sizes: 64 elements (L1), 1024 elements (L1+), 4096 elements (L1 ↔ L2 split)
//   element types: i32, f64; plus point3 (24 B struct) for non-fundamental impls.

#include "../external/bbench/bench.hpp"

#include "../src/array/array.hpp"
#include "../src/array/bisect_array.hpp"
#include "../src/array/carray.hpp"
#include "../src/array/conarray.hpp"
#include "../src/array/constexprarray.hpp"
#include "../src/array/farray.hpp"
#include "../src/array/iarray.hpp"
#include "../src/array/mdarray.hpp"
#include "../src/array/parray.hpp"
#include "../src/array/soa.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/std.hpp"

namespace
{

using c_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 4;

struct point3 {
  f64 x;
  f64 y;
  f64 z;
};

template<typename T>
[[gnu::always_inline]] inline u64
reduce_one(const T &v) noexcept
{
  if constexpr ( micron::is_arithmetic_v<T> )
    return (u64)v;
  else
    return (u64)v.x;
}

template<typename T>
[[gnu::always_inline]] inline T
make_from(u64 i) noexcept
{
  if constexpr ( micron::is_arithmetic_v<T> )
    return (T)i;
  else
    return T{ static_cast<f64>(i), static_cast<f64>(i + 1), static_cast<f64>(i + 2) };
}

static volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
sink_t(u64 v) noexcept
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
print_col_header()
{
  line h;
  h.s("op");
  h.pad_to(24, 0);
  h.s_at("N", 32);
  h.s_at("elem_B", 42);
  h.s_at("cyc/op", 52);
  h.s_at("IPC", 62);
  h.s_at("bmiss%", 72);
  micron::io::println(h.str());
  micron::io::println("------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_header(const char *section)
{
  micron::io::println("");
  micron::io::println("[", section, "]");
  print_col_header();
}

struct cell {
  const char *name;
  u64 size;
  u64 elem_bytes;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

struct anomaly {
  const char *name;
  u64 size;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
  const char *reason;
};

static anomaly g_anomalies[512];
static u32 g_anomaly_count = 0;

[[gnu::cold]] void
print_cell(const cell &c)
{
  const fmt2 cpo = to_fmt2(c.cyc_per_op);
  const fmt2 ipc = to_fmt2(c.ipc);
  const fmt2 bm = to_fmt2(c.bmiss_rate * 100.0);
  line ln;
  ln.s_lj_at(c.name, 24);
  ln.u_at(c.size, 32);
  ln.u_at(c.elem_bytes, 42);
  ln.f2_at(cpo, 52);
  ln.f2_at(ipc, 62);
  ln.f2_at(bm, 72);
  micron::io::println(ln.str());

  const char *reason = nullptr;
  if ( c.bmiss_rate > 0.05 )
    reason = "bmiss%>5 (predictor stress)";
  else if ( c.ipc > 0 && c.ipc < 0.7 )
    reason = "IPC<0.7 (memory-bound)";
  else if ( c.cyc_per_op > 1000.0 && !(c.name[0] == 'c' && c.name[1] == 'o' && c.name[2] == 'p') )
    reason = "cyc/op>1000 outside copy ops";

  if ( reason && g_anomaly_count < 512 ) {
    g_anomalies[g_anomaly_count++] = anomaly{ c.name, c.size, c.cyc_per_op, c.ipc, c.bmiss_rate, reason };
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
[[gnu::noinline]] cell
measure(const char *name, u64 size, u64 elem_bytes, u64 ops_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
    setup();
    kernel();
  }

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    c_events evs{ bbench::quiet{} };
    evs.open();
    setup();
    evs.begin();
    for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
    evs.end();
    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    const f64 total_ops = static_cast<f64>(reps_per_meas) * static_cast<f64>(ops_per_rep);
    cpo_samples[m] = total_ops > 0 ? static_cast<f64>(cyc) / total_ops : static_cast<f64>(cyc);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return cell{ name,
               size,
               elem_bytes,
               median_f64(cpo_samples, K_MEASUREMENTS),
               median_f64(ipc_samples, K_MEASUREMENTS),
               median_f64(bm_samples, K_MEASUREMENTS) };
}

[[gnu::always_inline]] inline u64
reps_for(u64 ops_per_rep) noexcept
{
  constexpr u64 TARGET = 1ULL << 22;
  if ( ops_per_rep == 0 ) return 64;
  u64 r = TARGET / ops_per_rep;
  if ( r < 4 ) r = 4;
  if ( r > 1ULL << 18 ) r = 1ULL << 18;
  return r;
}

template<template<typename, usize> class A, typename T, usize Cap>
void
sweep_mutable_array(const char *tag)
{
  print_header(tag);

  static A<T, Cap> g_a;
  static A<T, Cap> g_b;

  for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Cap; ++i ) s += reduce_one(g_a[i]);
      sink_t(s);
    };
    print_cell(measure("iterate-sum [i]", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( auto &v : g_a ) s += reduce_one(v);
      sink_t(s);
    };
    print_cell(measure("iterate-sum (iter)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      A<T, Cap> tmp(g_a);
      clobber(&tmp);
    };
    print_cell(measure("copy-ctor", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      g_b = g_a;
      clobber(&g_b);
    };
    print_cell(measure("copy-assign", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      A<T, Cap> tmp(micron::move(g_a));
      g_a = micron::move(tmp);
      clobber(&g_a);
    };
    print_cell(measure("move-ctor (pair)", Cap, sizeof(T), Cap, reps_for(Cap) / 4 + 1, setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      if constexpr ( micron::is_arithmetic_v<T> )
        g_a.fill(T(0));
      else
        g_a.fill(make_from<T>(0));
      clobber(&g_a);
    };
    print_cell(measure("fill(value)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 c = 0;
      A<T, Cap> tmp([&]() noexcept { return make_from<T>(++c); });
      clobber(&tmp);
    };
    print_cell(measure("generate (ctor)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      A<T, Cap> tmp([&](const T *p) noexcept { return *p; });
      clobber(&tmp);
    };
    print_cell(measure("transform (ctor)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);
      clobber(&g_a);
    };
    print_cell(measure("fill (op[]=)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [&] { g_b = g_a; };
    auto kernel = [&] {
      bool eq = true;
      for ( usize i = 0; i < Cap; ++i )
        if ( reduce_one(g_a[i]) != reduce_one(g_b[i]) ) {
          eq = false;
          break;
        }
      sink_t(static_cast<u64>(eq));
    };
    print_cell(measure("equality (worst)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  if constexpr ( micron::is_arithmetic_v<T> ) {
    auto setup = [] { };
    auto kernel = [&] {
      auto s = g_a.sum();
      sink_t(static_cast<u64>(s));
    };
    print_cell(measure("sum (reduce)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_conarray(const char *tag)
{
  print_header(tag);

  static mc::conarray<T, Cap> g_a;
  for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Cap; ++i ) s += reduce_one(g_a[i]);
      sink_t(s);
    };
    print_cell(measure("iterate-sum [i]", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( auto &v : g_a ) s += reduce_one(v);
      sink_t(s);
    };
    print_cell(measure("iterate-sum (iter)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      mc::conarray<T, Cap> tmp(g_a);
      clobber(&tmp);
    };
    print_cell(measure("copy-ctor", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      g_a.fill(T(0));
      clobber(&g_a);
    };
    print_cell(measure("fill(value)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);
      clobber(&g_a);
    };
    print_cell(measure("fill (op[]=)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  if constexpr ( micron::is_arithmetic_v<T> ) {
    auto setup = [] { };
    auto kernel = [&] {
      auto s = g_a.sum();
      sink_t(static_cast<u64>(s));
    };
    print_cell(measure("sum (reduce)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_farray(const char *tag)
{
  print_header(tag);

  static mc::farray<T, Cap> g_a;
  static mc::farray<T, Cap> g_b;

  for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Cap; ++i ) s += reduce_one(g_a[i]);
      sink_t(s);
    };
    print_cell(measure("iterate-sum [i]", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      mc::farray<T, Cap> tmp(g_a);
      clobber(&tmp);
    };
    print_cell(measure("copy-ctor", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      g_b = g_a;
      clobber(&g_b);
    };
    print_cell(measure("copy-assign", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      mc::farray<T, Cap> tmp(micron::move(g_a));
      g_a = micron::move(tmp);
      clobber(&g_a);
    };
    print_cell(measure("move-ctor (pair)", Cap, sizeof(T), Cap, reps_for(Cap) / 4 + 1, setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      g_a.fill(T(0));
      clobber(&g_a);
    };
    print_cell(measure("fill(value)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);
      clobber(&g_a);
    };
    print_cell(measure("fill (op[]=)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_iarray(const char *tag)
{
  print_header(tag);

  static mc::iarray<T, Cap> g_a([](T *p) noexcept {
    *p = T{};
    return *p;
  });

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Cap; ++i ) s += reduce_one(g_a[i]);
      sink_t(s);
    };
    print_cell(measure("iterate-sum [i]", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( auto it = g_a.cbegin(); it != g_a.cend(); ++it ) s += reduce_one(*it);
      sink_t(s);
    };
    print_cell(measure("iterate-sum (iter)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      mc::iarray<T, Cap> tmp(g_a);
      clobber(&tmp);
    };
    print_cell(measure("copy-ctor", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 c = 0;
      mc::iarray<T, Cap> tmp([&]() noexcept { return make_from<T>(++c); });
      clobber(&tmp);
    };
    print_cell(measure("generate (ctor)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_cexpr_array(const char *tag)
{
  print_header(tag);

  static mc::constexpr_array<T, Cap> g_a;
  for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Cap; ++i ) s += reduce_one(g_a[i]);
      sink_t(s);
    };
    print_cell(measure("iterate-sum [i]", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      mc::constexpr_array<T, Cap> tmp(g_a);
      clobber(&tmp);
    };
    print_cell(measure("copy-ctor", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      g_a.fill(T(0));
      clobber(&g_a);
    };
    print_cell(measure("fill(value)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  if constexpr ( micron::is_arithmetic_v<T> ) {
    auto setup = [] { };
    auto kernel = [&] {
      auto s = g_a.sum();
      sink_t(static_cast<u64>(s));
    };
    print_cell(measure("sum (reduce)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_array_arith(const char *tag)
{
  print_header(tag);

  static mc::array<T, Cap> g_a;
  static mc::array<T, Cap> g_b;

  auto reset = [&] {
    for ( usize i = 0; i < Cap; ++i ) {
      g_a[i] = make_from<T>(i + 1);
      g_b[i] = make_from<T>(i + 2);
    }
  };

  reset();

  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      g_a += T(1);
      clobber(&g_a);
    };
    print_cell(measure("scalar +=", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      g_a -= T(1);
      clobber(&g_a);
    };
    print_cell(measure("scalar -=", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      g_a *= T(2);
      clobber(&g_a);
    };
    print_cell(measure("scalar *=", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  if constexpr ( micron::is_floating_point_v<T> || micron::is_integral_v<T> ) {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      g_a /= T(2);
      clobber(&g_a);
    };
    print_cell(measure("scalar /=", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      g_a += g_b;
      clobber(&g_a);
    };
    print_cell(measure("array +=", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      g_a *= g_b;
      clobber(&g_a);
    };
    print_cell(measure("array *=", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto c = g_a + g_b;
      clobber(&c);
    };
    print_cell(measure("a + b (alloc)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto s = g_a.sum();
      sink_t(static_cast<u64>(s));
    };
    print_cell(measure("sum (reduce)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  if constexpr ( micron::is_integral_v<T> ) {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto p = g_a.mul_reduce();
      sink_t(static_cast<u64>(p));
    };
    print_cell(measure("mul_reduce", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [&] {
      reset();
      for ( usize i = 0; i < Cap; ++i ) g_a[i] = T(0);
    };
    auto kernel = [&] {
      bool a = g_a.all(T(0));
      sink_t(static_cast<u64>(a));
    };
    print_cell(measure("all(0)", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [&] {
      reset();
      g_a[Cap - 1] = T(0);
    };
    auto kernel = [&] {
      bool a = g_a.any(T(0));
      sink_t(static_cast<u64>(a));
    };
    print_cell(measure("any(0) tail", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_bisect_array(const char *tag)
{
  print_header(tag);

  static mc::bisect_array<T, Cap> g_a;

  auto fill_half = [&] {
    while ( g_a.size() ) g_a.erase(0);
    for ( usize i = 0; i < Cap / 2; ++i ) g_a.insert(make_from<T>(i));
  };

  fill_half();

  {
    auto setup = [&] { fill_half(); };
    auto kernel = [&] {
      g_a.insert(make_from<T>(Cap / 4));
      g_a.erase(g_a.size() / 2);
      clobber(&g_a);
    };
    print_cell(measure("insert+erase (mid)", Cap, sizeof(T), 1, reps_for(Cap * 2), setup, kernel));
  }

  {
    auto setup = [&] { fill_half(); };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < g_a.size(); ++i ) s += reduce_one(g_a[i]);
      sink_t(s);
    };
    print_cell(measure("iterate-sum [i]", Cap / 2, sizeof(T), Cap / 2, reps_for(Cap / 2), setup, kernel));
  }

  {
    auto setup = [&] { fill_half(); };
    auto kernel = [&] {
      g_a.erase(g_a.size() - 1);
      g_a.insert(make_from<T>(0));
      clobber(&g_a);
    };
    print_cell(measure("erase(tail)+insert", Cap / 2, sizeof(T), 1, reps_for(Cap * 2), setup, kernel));
  }
  {
    auto setup = [&] { fill_half(); };
    auto kernel = [&] {
      g_a.erase(0);
      g_a.insert(make_from<T>(0));
      clobber(&g_a);
    };
    print_cell(measure("erase(head)+insert", Cap / 2, sizeof(T), 1, reps_for(Cap * 2), setup, kernel));
  }
}

template<typename T, usize K, usize H>
void
sweep_parray(const char *tag)
{
  using PA = mc::parray<T, K, H>;
  static constexpr usize Length = PA::length;
  print_header(tag);

  PA filled;
  for ( usize i = 0; i < Length; ++i ) filled = filled.set(i, make_from<T>(i));

  {
    auto setup = [] { };
    auto kernel = [&] {
      PA p;
      for ( usize i = 0; i < Length; ++i ) p = p.set(i, make_from<T>(i));
      clobber(&p);
    };
    print_cell(measure("build (set chain)", Length, sizeof(T), Length, reps_for(Length * 4), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Length; ++i ) s += reduce_one(filled.get(i));
      sink_t(s);
    };
    print_cell(measure("get (read sweep)", Length, sizeof(T), Length, reps_for(Length * 2), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      filled.for_each([&](usize, const T &v) { s += reduce_one(v); });
      sink_t(s);
    };
    print_cell(measure("for_each", Length, sizeof(T), Length, reps_for(Length * 2), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      PA next = filled.set(Length / 2, make_from<T>(42));
      clobber(&next);
    };
    print_cell(measure("set (1 key)", Length, sizeof(T), 1, reps_for(64), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      PA tmp(filled);
      clobber(&tmp);
    };
    print_cell(measure("copy-ctor (O(1))", Length, sizeof(T), 1, reps_for(64), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      PA next = filled.fill(make_from<T>(0));
      clobber(&next);
    };
    print_cell(measure("fill (broadcast)", Length, sizeof(T), Length, reps_for(Length * 4), setup, kernel));
  }

  if constexpr ( micron::is_arithmetic_v<T> ) {
    auto setup = [] { };
    auto kernel = [&] {
      auto s = filled.sum();
      sink_t(static_cast<u64>(s));
    };
    print_cell(measure("sum (reduce)", Length, sizeof(T), Length, reps_for(Length * 2), setup, kernel));
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

  micron::io::println("=== micron arrays benchmark ===");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements per cell (median)");

  sweep_mutable_array<mc::array, i32, 64>("array<i32, 64>");
  sweep_mutable_array<mc::array, i32, 1024>("array<i32, 1024>");
  sweep_mutable_array<mc::array, f64, 1024>("array<f64, 1024>");
  sweep_mutable_array<mc::array, point3, 64>("array<point3, 64>");
  sweep_mutable_array<mc::array, point3, 1024>("array<point3, 1024>");

  sweep_mutable_array<mc::carray, i32, 64>("carray<i32, 64>");
  sweep_mutable_array<mc::carray, i32, 1024>("carray<i32, 1024>");

  sweep_conarray<i32, 64>("conarray<i32, 64>");
  sweep_conarray<i32, 1024>("conarray<i32, 1024>");

  sweep_farray<i32, 64>("farray<i32, 64>");
  sweep_farray<i32, 1024>("farray<i32, 1024>");
  sweep_farray<f64, 1024>("farray<f64, 1024>");

  sweep_iarray<i32, 64>("iarray<i32, 64>");
  sweep_iarray<i32, 1024>("iarray<i32, 1024>");
  sweep_iarray<point3, 64>("iarray<point3, 64>");

  sweep_cexpr_array<i32, 64>("constexpr_array<i32, 64>");
  sweep_cexpr_array<i32, 1024>("constexpr_array<i32, 1024>");

  sweep_array_arith<i32, 1024>("array<i32, 1024> arith");
  sweep_array_arith<f64, 1024>("array<f64, 1024> arith");

  sweep_bisect_array<i32, 256>("bisect_array<i32, 256>");
  sweep_bisect_array<i32, 1024>("bisect_array<i32, 1024>");

  sweep_parray<i32, 5, 2>("parray<i32, 5, 2>");
  sweep_parray<f64, 5, 2>("parray<f64, 5, 2>");

  for ( u64 N : { 64ULL, 1024ULL, 4096ULL } ) {
    {
      micron::soa<f32, f32, u32> s(N);
      auto setup = [&] { s.clear(); };
      auto kernel = [&] {
        for ( u64 i = 0; i < N; ++i ) s.emplace_back(static_cast<f32>(i), static_cast<f32>(i) * 2.0f, static_cast<u32>(i));
        s.clear();
      };
      print_cell(measure("soa emplace_back", N, sizeof(f32) + sizeof(f32) + sizeof(u32), N, reps_for(N), setup, kernel));
    }
    {
      micron::soa<f32, f32, u32> s(N);
      for ( u64 i = 0; i < N; ++i ) s.emplace_back(static_cast<f32>(i), 0.0f, 0);
      auto setup = [] { };
      auto kernel = [&] {
        f32 acc = 0;
        auto *c0 = s.column<0>();
        for ( u64 i = 0; i < N; ++i ) acc += c0[i];
        sink_u64 += static_cast<u64>(acc);
      };
      print_cell(measure("soa col-sum", N, sizeof(f32), N, reps_for(N), setup, kernel));
    }
    {
      micron::soa<f32, f32, u32> s(N);
      auto setup = [&] {
        s.clear();
        for ( u64 i = 0; i < N; ++i ) s.emplace_back(static_cast<f32>(i), 0.0f, 0);
      };
      auto kernel = [&] {
        while ( !s.empty() ) s.pop_back();
        clobber(&s);
      };
      print_cell(measure("soa pop_back", N, sizeof(f32) + sizeof(f32) + sizeof(u32), N, reps_for(N), setup, kernel));
    }
  }

  for ( u64 N : { 256ULL, 1024ULL, 4096ULL } ) {
    micron::mdarray<f32, 1> a(static_cast<usize>(N));
    micron::mdarray<f32, 1> b(static_cast<usize>(N));
    a.fill(1.0f);
    b.fill(2.0f);
    {
      auto setup = [&] { a.fill(1.0f); };
      auto kernel = [&] {
        a.fill(3.0f);
        clobber(&a);
      };
      print_cell(measure("mdarray-1d fill", N, sizeof(f32), N, reps_for(N), setup, kernel));
    }
    {
      auto setup = [&] { a.fill(1.0f); };
      auto kernel = [&] {
        a += b;
        clobber(&a);
      };
      print_cell(measure("mdarray-1d add", N, sizeof(f32), N, reps_for(N), setup, kernel));
    }
    {
      auto setup = [&] { a.fill(1.0f); };
      auto kernel = [&] {
        a *= 3.0f;
        clobber(&a);
      };
      print_cell(measure("mdarray-1d mul-scalar", N, sizeof(f32), N, reps_for(N), setup, kernel));
    }
    {
      auto setup = [] { };
      auto kernel = [&] { sink_u64 += static_cast<u64>(a.sum()); };
      print_cell(measure("mdarray-1d sum", N, sizeof(f32), N, reps_for(N), setup, kernel));
    }
    {
      auto setup = [] { };
      auto kernel = [&] {
        micron::mdarray<f32, 1> tmp(a);
        clobber(&tmp);
      };
      print_cell(measure("mdarray-1d copy-ctor", N, sizeof(f32), 1, reps_for(N), setup, kernel));
    }
  }

  for ( u64 side : { 32ULL, 64ULL, 128ULL } ) {
    const u64 total = side * side;
    micron::mdarray<f32, 2> a(static_cast<usize>(side), static_cast<usize>(side));
    micron::mdarray<f32, 2> b(static_cast<usize>(side), static_cast<usize>(side));
    a.fill(1.0f);
    b.fill(0.5f);
    {
      auto setup = [&] { a.fill(1.0f); };
      auto kernel = [&] {
        a += b;
        clobber(&a);
      };
      print_cell(measure("mdarray-2d add", total, sizeof(f32), total, reps_for(total), setup, kernel));
    }
    {
      auto setup = [] { };
      auto kernel = [&] { sink_u64 += static_cast<u64>(a.sum()); };
      print_cell(measure("mdarray-2d sum", total, sizeof(f32), total, reps_for(total), setup, kernel));
    }
  }

  micron::io::println("");
  micron::io::println("[anomalies] (rows flagged during run)");
  if ( g_anomaly_count == 0 ) {
    micron::io::println("  (none)");
  } else {
    line head;
    head.s("  ");
    head.s_at("op", 26);
    head.s_at("N", 36);
    head.s_at("cyc/op", 48);
    head.s_at("IPC", 58);
    head.s_at("bmiss%", 68);
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
      ln.s_lj_at(a.name, 26);
      ln.u_at(a.size, 36);
      ln.f2_at(cpo, 48);
      ln.f2_at(ipc, 58);
      ln.f2_at(bm, 68);
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
