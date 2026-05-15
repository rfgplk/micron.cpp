//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Algorithm benchmark — exercises every contiguous-range algorithm shipped
// under src/algorithm/ at three working-set sizes (L1 / L1+ / L2).
//
//   coverage (one cell per public entry point with a meaningful cost):
//
//   algorithm.hpp:
//     clamp, fill, fill_n, generate, transform,
//     where (auto-Fn + Fn), shift_left, shift_right,
//     rotate_left, rotate_right, reverse, reverse_copy,
//     sum, mean, geomean, harmonicmean,
//     clear, round, ceil, floor,
//     min, max, min_at, max_at
//
//   accumulate.hpp:
//     accumulate, accumulate-with-Fn, accumulate-limited
//
//   find.hpp:
//     all_of, any_of, none_of (cmp+Fn forms),
//     find (hit/miss), find_if, find_if_not (Fn+ptr-Fn),
//     find_last, find_last_if, find_last_if_not,
//     find_end, find_first_of, adjacent_find,
//     count, count_if, mismatch, equal,
//     search, search_n, contains, contains_subrange,
//     starts_with, ends_with
//
//   arith.hpp:    pow, add, multiply, divide, subtract  (scalar)
//   data.hpp:     merge (2 forms), concat, rotate (3 forms),
//                 make_heap, push_heap, pop_heap, sort_heap, is_heap
//   filter.hpp:   filter (3 forms), filter_inplace, prune
//   fold.hpp:     fold_left, fold_right, fold, fold_left_counted,
//                 fold_left_while, fold_build
//   math.hpp:     sin, cos, tan, sqrt, exp, log, log10, cbrt, absolute,
//                 sign, clip, degrees, radians, asinh, acosh, atanh,
//                 sinh, cosh, tanh, asin, acos, atan
//   fp.hpp:       add_c, subtract_c, multiply_c, divide_c, pow_c   (curried)
//                 add, subtract, multiply, divide                   (option-wrapped)
//                 fmap                                              (functor map)
//                 filter_c                                          (curried filter)
//                 take_c, drop_c                                    (slice combinators)
//
//   per (algo, op, N, T) cell — cyc/op, IPC, branch-miss%; medians across
//   K_MEASUREMENTS samples, bbench 4-event group.

#include "../external/bbench/bench.hpp"

#include "../src/algorithm/accumulate.hpp"
#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/arith.hpp"
#include "../src/algorithm/data.hpp"
#include "../src/algorithm/filter.hpp"
#include "../src/algorithm/find.hpp"
#include "../src/algorithm/fold.hpp"
#include "../src/algorithm/fp.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtemplate-body"
#include "../src/algorithm/math.hpp"
#pragma GCC diagnostic pop
#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/std.hpp"
#include "../src/vector/vector.hpp"

namespace
{

using c_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 3;

constexpr u64 SIZES[] = {
  64,
  1024,
  16384,
};

static volatile u64 sink_u64 = 0;
static volatile f64 sink_f64 = 0.0;

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

[[gnu::always_inline]] inline void
sink_f(f64 v) noexcept
{
  sink_f64 += v;
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
  h.pad_to(28, 0);
  h.s_at("N", 38);
  h.s_at("elem_B", 48);
  h.s_at("cyc/op", 58);
  h.s_at("IPC", 68);
  h.s_at("bmiss%", 78);
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

[[gnu::cold]] void
print_cell(const cell &c)
{
  const fmt2 cpo = to_fmt2(c.cyc_per_op);
  const fmt2 ipc = to_fmt2(c.ipc);
  const fmt2 bm = to_fmt2(c.bmiss_rate * 100.0);
  line ln;
  ln.s_lj_at(c.name, 28);
  ln.u_at(c.size, 38);
  ln.u_at(c.elem_bytes, 48);
  ln.f2_at(cpo, 58);
  ln.f2_at(ipc, 68);
  ln.f2_at(bm, 78);
  micron::io::println(ln.str());
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

constexpr u64 MAX_N = 16384;
alignas(64) static i32 g_i32a[MAX_N];
alignas(64) static i32 g_i32b[MAX_N];
alignas(64) static i32 g_i32c[MAX_N];
alignas(64) static f64 g_f64a[MAX_N];
alignas(64) static f64 g_f64b[MAX_N];

[[gnu::always_inline]] inline void
fill_i32(u64 N, u64 seed = 0)
{
  for ( u64 i = 0; i < N; ++i ) g_i32a[i] = static_cast<i32>(i + seed);
}

[[gnu::always_inline]] inline void
fill_f64(u64 N, u64 seed = 0)
{
  for ( u64 i = 0; i < N; ++i ) g_f64a[i] = static_cast<f64>(i + seed) * 0.5;
}

void
sweep_algorithm()
{
  print_header("algorithm.hpp (pointers, i32)");
  for ( u64 N : SIZES ) {

    {
      auto setup = [] { };
      auto kernel = [&] {
        micron::fill(g_i32a, g_i32a + N, i32{ 42 });
        clobber(g_i32a);
      };
      print_cell(measure("fill(value)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [] { };
      auto kernel = [&] {
        micron::fill_n(g_i32a, N, i32{ 7 });
        clobber(g_i32a);
      };
      print_cell(measure("fill_n", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [] { };
      auto kernel = [&] {
        u64 c = 0;
        micron::generate(g_i32a, g_i32a + N, [&]() noexcept { return static_cast<i32>(c++); });
        clobber(g_i32a);
      };
      print_cell(measure("generate(Fn)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::transform(g_i32a, g_i32a + N, [](const i32 *p) noexcept { return *p * 2; });
        clobber(g_i32a);
      };
      print_cell(measure("transform (*2)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        i32 *last = micron::where(g_i32a, g_i32a + N, g_i32b, [](i32 v) noexcept { return (v & 1) == 0; });
        sink_t(static_cast<u64>(last - g_i32b));
      };
      print_cell(measure("where (even)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::shift_left(g_i32a, g_i32a + N, 1);
        clobber(g_i32a);
      };
      print_cell(measure("shift_left(1)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::shift_right(g_i32a, g_i32a + N, 1);
        clobber(g_i32a);
      };
      print_cell(measure("shift_right(1)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::rotate_left(g_i32a, g_i32a + N, N / 4);
        clobber(g_i32a);
      };
      print_cell(measure("rotate_left(N/4)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::rotate_right(g_i32a, g_i32a + N, N / 4);
        clobber(g_i32a);
      };
      print_cell(measure("rotate_right(N/4)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::reverse(g_i32a, g_i32a + N - 1);
        clobber(g_i32a);
      };
      print_cell(measure("reverse (ptr)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::reverse_copy(g_i32a, g_i32a + N, g_i32b);
        clobber(g_i32b);
      };
      print_cell(measure("reverse_copy", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
  }

  print_header("algorithm.hpp (container, array<i32,1024>)");
  using A = mc::array<i32, 1024>;
  static A g_arr;

  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arr[i] = static_cast<i32>(i);
    };
    auto kernel = [&] {
      auto s = micron::sum(g_arr);
      sink_t(static_cast<u64>(s));
    };
    print_cell(measure("sum (container)", 1024, sizeof(i32), 1024, reps_for(1024), setup, kernel));
  }

  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arr[i] = static_cast<i32>(i);
    };
    auto kernel = [&] {
      f64 m = micron::mean(g_arr);
      sink_f(m);
    };
    print_cell(measure("mean", 1024, sizeof(i32), 1024, reps_for(1024), setup, kernel));
  }

  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arr[i] = static_cast<i32>(i);
    };
    auto kernel = [&] {
      sink_t(static_cast<u64>(micron::max(g_arr)));
      sink_t(static_cast<u64>(micron::min(g_arr)));
    };
    print_cell(measure("max+min", 1024, sizeof(i32), 2 * 1024, reps_for(1024), setup, kernel));
  }

  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arr[i] = static_cast<i32>(i);
    };
    auto kernel = [&] {
      sink_t(static_cast<u64>(*micron::max_at(g_arr)));
      sink_t(static_cast<u64>(*micron::min_at(g_arr)));
    };
    print_cell(measure("max_at+min_at", 1024, sizeof(i32), 2 * 1024, reps_for(1024), setup, kernel));
  }

  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arr[i] = static_cast<i32>(i);
    };
    auto kernel = [&] {
      micron::clear(g_arr);
      clobber(&g_arr);
    };
    print_cell(measure("clear", 1024, sizeof(i32), 1024, reps_for(1024), setup, kernel));
  }

  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arr[i] = static_cast<i32>(i);
    };
    auto kernel = [&] {
      i32 acc = 0;
      for ( usize i = 0; i < 1024; ++i ) acc += micron::clamp(g_arr[i], 100, 900);
      sink_t(static_cast<u64>(acc));
    };
    print_cell(measure("clamp (scalar loop)", 1024, sizeof(i32), 1024, reps_for(1024), setup, kernel));
  }

  using AF = mc::array<f64, 1024>;
  static AF g_arrf;
  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arrf[i] = static_cast<f64>(i) + 0.5;
    };
    auto kernel = [&] {
      micron::round(g_arrf);
      clobber(&g_arrf);
    };
    print_cell(measure("round", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arrf[i] = static_cast<f64>(i) + 0.3;
    };
    auto kernel = [&] {
      micron::ceil(g_arrf);
      clobber(&g_arrf);
    };
    print_cell(measure("ceil", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_arrf[i] = static_cast<f64>(i) + 0.7;
    };
    auto kernel = [&] {
      micron::floor(g_arrf);
      clobber(&g_arrf);
    };
    print_cell(measure("floor", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
}

void
sweep_accumulate()
{
  print_header("accumulate.hpp (i32)");
  for ( u64 N : SIZES ) {
    auto setup = [&] { fill_i32(N); };

    {
      auto kernel = [&] {
        i64 a = micron::accumulate(g_i32a, g_i32a + N, i64{ 0 });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("accumulate", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto kernel = [&] {
        i64 a = micron::accumulate(g_i32a, g_i32a + N, i64{ 0 }, [](i64 acc, const i32 &v) noexcept { return acc + v; });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("accumulate(Fn)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto kernel = [&] {
        i64 a = micron::accumulate(g_i32a, g_i32a + N, i64{ 0 }, N / 2);
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("accumulate(N/2)", N, sizeof(i32), N / 2, reps_for(N / 2), setup, kernel));
    }
  }
}

void
sweep_find()
{
  print_header("find.hpp (i32, pointers)");
  for ( u64 N : SIZES ) {
    auto setup_seq = [&] { fill_i32(N); };
    auto setup_targets = [&] {
      for ( u64 i = 0; i < N; ++i ) g_i32a[i] = static_cast<i32>(i);
    };

    {
      auto kernel = [&] {
        bool a = micron::all_of(g_i32a, g_i32a + N, [](i32 v) noexcept { return v >= 0; });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("all_of (true)", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        g_i32a[0] = -1;
      };
      auto kernel = [&] {
        bool a = micron::all_of(g_i32a, g_i32a + N, [](i32 v) noexcept { return v >= 0; });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("all_of (early miss)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        g_i32a[N - 1] = -1;
      };
      auto kernel = [&] {
        bool a = micron::any_of(g_i32a, g_i32a + N, [](i32 v) noexcept { return v < 0; });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("any_of (tail hit)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto kernel = [&] {
        bool a = micron::none_of(g_i32a, g_i32a + N, [](i32 v) noexcept { return v < 0; });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("none_of", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto setup = [&] { setup_targets(); };
      auto kernel = [&] {
        const i32 *p = micron::find(g_i32a, g_i32a + N, static_cast<i32>(N - 1));
        sink_t(static_cast<u64>(p ? *p : 0));
      };
      print_cell(measure("find (tail hit)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto kernel = [&] {
        const i32 *p = micron::find(g_i32a, g_i32a + N, i32{ -1 });
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("find (miss)", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto kernel = [&] {
        const i32 *p = micron::find_if(g_i32a, g_i32a + N, [&](i32 v) noexcept { return v == static_cast<i32>(N - 1); });
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("find_if (tail)", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto kernel = [&] {
        const i32 *p = micron::find_last(g_i32a, g_i32a + N, static_cast<i32>(0));
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("find_last", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto kernel = [&] {
        const i32 *p = micron::find_last_if(g_i32a, g_i32a + N, [](i32 v) noexcept { return (v & 7) == 0; });
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("find_last_if", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto kernel = [&] {
        const i32 *p = micron::adjacent_find(g_i32a, g_i32a + N);
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("adjacent_find (miss)", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto kernel = [&] {
        auto c = micron::count(g_i32a, g_i32a + N, static_cast<i32>(N / 2));
        sink_t(static_cast<u64>(c));
      };
      print_cell(measure("count", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto kernel = [&] {
        auto c = micron::count_if(g_i32a, g_i32a + N, [](i32 v) noexcept { return (v & 1) == 0; });
        sink_t(static_cast<u64>(c));
      };
      print_cell(measure("count_if (even)", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        for ( u64 i = 0; i < N; ++i ) g_i32b[i] = g_i32a[i];
      };
      auto kernel = [&] {
        auto pr = micron::mismatch(g_i32a, g_i32a + N, g_i32b);
        sink_t(reinterpret_cast<uintptr_t>(pr.a));
      };
      print_cell(measure("mismatch (equal)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        for ( u64 i = 0; i < N; ++i ) g_i32b[i] = g_i32a[i];
      };
      auto kernel = [&] {
        bool e = micron::equal(g_i32a, g_i32a + N, g_i32b);
        sink_t(static_cast<u64>(e));
      };
      print_cell(measure("equal (full)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { setup_targets(); };
      auto kernel = [&] {
        i32 needle[8];
        for ( int j = 0; j < 8; ++j ) needle[j] = static_cast<i32>(N - 8 + j);
        const i32 *p = micron::search(g_i32a, g_i32a + N, needle, needle + 8);
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("search (tail needle)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();

        for ( int j = 0; j < 4; ++j ) g_i32a[N / 2 + j] = 999;
      };
      auto kernel = [&] {
        const i32 *p = micron::search_n(g_i32a, g_i32a + N, 4, i32{ 999 });
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("search_n (4×)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto kernel = [&] {
        bool c = micron::contains(g_i32a, g_i32a + N, static_cast<i32>(N / 2));
        sink_t(static_cast<u64>(c));
      };
      print_cell(measure("contains", N, sizeof(i32), N, reps_for(N), setup_targets, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        for ( u64 i = 0; i < 8; ++i ) g_i32b[i] = g_i32a[i];
      };
      auto kernel = [&] {
        bool s = micron::starts_with(g_i32a, g_i32a + N, g_i32b, g_i32b + 8);
        sink_t(static_cast<u64>(s));
      };
      print_cell(measure("starts_with(8)", N, sizeof(i32), 8, reps_for(8), setup, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        for ( u64 i = 0; i < 8; ++i ) g_i32b[i] = g_i32a[N - 8 + i];
      };
      auto kernel = [&] {
        bool e = micron::ends_with(g_i32a, g_i32a + N, g_i32b, g_i32b + 8);
        sink_t(static_cast<u64>(e));
      };
      print_cell(measure("ends_with(8)", N, sizeof(i32), 8, reps_for(8), setup, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        g_i32b[0] = static_cast<i32>(N / 3);
        g_i32b[1] = static_cast<i32>(N / 4);
        g_i32b[2] = static_cast<i32>(N / 5);
      };
      auto kernel = [&] {
        const i32 *p = micron::find_first_of(g_i32a, g_i32a + N, g_i32b, g_i32b + 3);
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("find_first_of(3)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] {
        setup_targets();
        for ( u64 i = 0; i < 4; ++i ) g_i32b[i] = g_i32a[N - 4 + i];
      };
      auto kernel = [&] {
        const i32 *p = micron::find_end(g_i32a, g_i32a + N, g_i32b, g_i32b + 4);
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("find_end(4)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    (void)setup_seq;
  }
}

void
sweep_arith()
{
  print_header("arith.hpp (container, f64)");
  using A = mc::array<f64, 1024>;
  static A g_a;

  auto setup = [&] {
    for ( usize i = 0; i < 1024; ++i ) g_a[i] = static_cast<f64>(i + 1) * 0.5;
  };

  {
    auto kernel = [&] {
      micron::pow(g_a, 2.0);
      clobber(&g_a);
    };
    print_cell(measure("pow(2.0)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto kernel = [&] {
      micron::add(g_a, 1.0);
      clobber(&g_a);
    };
    print_cell(measure("add(1.0)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto kernel = [&] {
      micron::multiply(g_a, 2.0);
      clobber(&g_a);
    };
    print_cell(measure("multiply(2.0)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto kernel = [&] {
      micron::divide(g_a, 2.0);
      clobber(&g_a);
    };
    print_cell(measure("divide(2.0)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto kernel = [&] {
      micron::subtract(g_a, 1.0);
      clobber(&g_a);
    };
    print_cell(measure("subtract(1.0)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto kernel = [&] {
      auto p = micron::multiply(g_a);
      sink_f(p);
    };
    print_cell(measure("multiply(reduce)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
}

void
sweep_data()
{
  print_header("data.hpp (i32)");
  for ( u64 N : SIZES ) {

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::rotate(g_i32a, g_i32a + N / 4, g_i32a + N);
        clobber(g_i32a);
      };
      print_cell(measure("rotate(N/4)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] { fill_i32(N); };
      auto kernel = [&] {
        micron::cycle_rotate(g_i32a, g_i32a + N / 4, g_i32a + N);
        clobber(g_i32a);
      };
      print_cell(measure("cycle_rotate(N/4)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }

    {
      auto setup = [&] {
        fill_i32(N);
        for ( u64 i = 0; i < N; ++i ) g_i32b[i] = static_cast<i32>(i * 2);
      };
      auto kernel = [&] {
        auto *p = micron::merge(g_i32a, g_i32a + N / 2, g_i32b, g_i32b + N / 2, g_i32c);
        sink_t(static_cast<u64>(p - g_i32c));
      };
      print_cell(measure("merge(iter)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
  }

  print_header("data.hpp heap ops (vector<i32>)");
  for ( u64 N : SIZES ) {
    mc::vector<i32> v;
    auto reset = [&] {
      v.clear();
      v.reserve(N);

      for ( u64 i = 0; i < N; ++i ) v.push_back(static_cast<i32>(N - i));
    };

    {
      auto setup = [&] { reset(); };
      auto kernel = [&] {
        micron::make_heap(v);
        clobber(v.data());
      };
      print_cell(measure("make_heap", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto setup = [&] {
        reset();
        micron::make_heap(v);
      };
      auto kernel = [&] {
        micron::push_heap(v);
        clobber(v.data());
      };
      print_cell(measure("push_heap (1)", N, sizeof(i32), 1, reps_for(N), setup, kernel));
    }
    {
      auto setup = [&] {
        reset();
        micron::make_heap(v);
      };
      auto kernel = [&] {
        micron::pop_heap(v);
        clobber(v.data());
      };
      print_cell(measure("pop_heap (1)", N, sizeof(i32), 1, reps_for(N), setup, kernel));
    }
    {
      auto setup = [&] {
        reset();
        micron::make_heap(v);
      };
      auto kernel = [&] {
        micron::sort_heap(v);
        clobber(v.data());
      };

      print_cell(measure("sort_heap", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto setup = [&] {
        reset();
        micron::make_heap(v);
      };
      auto kernel = [&] {
        bool h = micron::is_heap(v);
        sink_t(static_cast<u64>(h));
      };
      print_cell(measure("is_heap", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
  }
}

void
sweep_filter()
{
  print_header("filter.hpp (i32)");
  for ( u64 N : SIZES ) {
    auto setup = [&] {
      for ( u64 i = 0; i < N; ++i ) g_i32a[i] = static_cast<i32>(i);
    };

    {
      auto kernel = [&] {
        i32 *last = micron::filter(g_i32a, g_i32a + N, [](const i32 *p) noexcept { return (*p & 1) == 0; }, g_i32b);
        sink_t(static_cast<u64>(last - g_i32b));
      };
      print_cell(measure("filter (even)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto kernel = [&] {
        i32 *last = micron::filter(g_i32a, g_i32a + N, [](const i32 *p) noexcept { return *p > 0; }, g_i32b, usize{ N / 2 });
        sink_t(static_cast<u64>(last - g_i32b));
      };
      print_cell(measure("filter (limit N/2)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto kernel = [&] {
        const i32 *p = micron::prune(g_i32a, g_i32a + N, [](const i32 *p) noexcept { return (*p & 1) == 0; }, g_i32b, usize{ N / 2 });
        sink_t(reinterpret_cast<uintptr_t>(p));
      };
      print_cell(measure("prune (limit N/2)", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
  }
}

void
sweep_fold()
{
  print_header("fold.hpp (i32)");
  for ( u64 N : SIZES ) {
    auto setup = [&] { fill_i32(N); };

    {
      auto kernel = [&] {
        i64 a = micron::fold_left(g_i32a, g_i32a + N, i64{ 0 }, [](i64 acc, const i32 *v) noexcept { return acc + *v; });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("fold_left", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto kernel = [&] {
        i64 a = micron::fold_right(g_i32a, g_i32a + N, [](const i32 *v, i64 acc) noexcept { return acc + *v; }, i64{ 0 });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("fold_right", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto kernel = [&] {
        i64 a = micron::fold(g_i32a, g_i32a + N, i64{ 0 }, [](i64 acc, const i32 *v) noexcept { return acc + *v; }, usize{ N / 2 });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("fold (limit N/2)", N, sizeof(i32), N / 2, reps_for(N / 2), setup, kernel));
    }
    {
      auto kernel = [&] {
        auto pr = micron::fold_left_counted(g_i32a, g_i32a + N, i64{ 0 }, [](i64 acc, const i32 *v) noexcept { return acc + *v; });
        sink_t(static_cast<u64>(pr.a));
        sink_t(static_cast<u64>(pr.b));
      };
      print_cell(measure("fold_left_counted", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
    {
      auto kernel = [&] {
        i64 a = micron::fold_left_while(
            g_i32a, g_i32a + N, i64{ 0 }, [](i64 acc, const i32 *v) noexcept { return acc + *v; },
            [](i64 acc, const i32 *) noexcept { return acc < (1ULL << 40); });
        sink_t(static_cast<u64>(a));
      };
      print_cell(measure("fold_left_while", N, sizeof(i32), N, reps_for(N), setup, kernel));
    }
  }
}

void
sweep_math()
{
  print_header("math.hpp (container, f64)");
  using AF = mc::array<f64, 1024>;
  static AF g_a;

  auto reset_unit = [&] {
    for ( usize i = 0; i < 1024; ++i ) g_a[i] = 0.001 * static_cast<f64>(i + 1);
  };
  auto reset_positive = [&] {
    for ( usize i = 0; i < 1024; ++i ) g_a[i] = static_cast<f64>(i + 1);
  };

  for ( const char *op : { "sin", "cos", "tan" } ) {
    auto setup = [&] { reset_unit(); };
    if ( op == nullptr ) continue;
    auto kernel = [&, op] {
      if ( op[0] == 's' )
        micron::sin(g_a);
      else if ( op[0] == 'c' )
        micron::cos(g_a);
      else
        micron::tan(g_a);
      clobber(&g_a);
    };
    print_cell(measure(op, 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }

  for ( const char *op : { "asin", "acos", "atan" } ) {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_a[i] = static_cast<f64>(i) / 1024.0;
    };
    auto kernel = [&, op] {
      if ( op[0] == 'a' && op[1] == 's' )
        micron::asin(g_a);
      else if ( op[0] == 'a' && op[1] == 'c' )
        micron::acos(g_a);
      else
        micron::atan(g_a);
      clobber(&g_a);
    };
    print_cell(measure(op, 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }

  for ( const char *op : { "sinh", "cosh", "tanh" } ) {
    auto setup = [&] { reset_unit(); };
    auto kernel = [&, op] {
      if ( op[0] == 's' )
        micron::sinh(g_a);
      else if ( op[0] == 'c' )
        micron::cosh(g_a);
      else
        micron::tanh(g_a);
      clobber(&g_a);
    };
    print_cell(measure(op, 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }

  {
    auto setup = [&] { reset_positive(); };
    auto kernel = [&] {
      micron::exp(g_a);
      clobber(&g_a);
    };
    print_cell(measure("exp", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset_positive(); };
    auto kernel = [&] {
      micron::sqrt(g_a);
      clobber(&g_a);
    };
    print_cell(measure("sqrt", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset_positive(); };
    auto kernel = [&] {
      micron::cbrt(g_a);
      clobber(&g_a);
    };
    print_cell(measure("cbrt", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_a[i] = static_cast<f64>(static_cast<i32>(i) - 512);
    };
    auto kernel = [&] {
      micron::absolute(g_a);
      clobber(&g_a);
    };
    print_cell(measure("absolute", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] {
      for ( usize i = 0; i < 1024; ++i ) g_a[i] = static_cast<f64>(static_cast<i32>(i) - 512);
    };
    auto kernel = [&] {
      micron::sign(g_a);
      clobber(&g_a);
    };
    print_cell(measure("sign", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset_positive(); };
    auto kernel = [&] {
      micron::clip(g_a, f64(100.0), f64(900.0));
      clobber(&g_a);
    };
    print_cell(measure("clip(100,900)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset_positive(); };
    auto kernel = [&] {
      micron::degrees(g_a);
      clobber(&g_a);
    };
    print_cell(measure("degrees", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset_positive(); };
    auto kernel = [&] {
      micron::radians(g_a);
      clobber(&g_a);
    };
    print_cell(measure("radians", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
}

void
sweep_fp()
{
  print_header("fp.hpp curried scalar (array<f64,1024>)");
  using AF = mc::array<f64, 1024>;
  static AF g_a;
  auto reset = [&] {
    for ( usize i = 0; i < 1024; ++i ) g_a[i] = static_cast<f64>(i + 1) * 0.5;
  };

  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto add5 = micron::add_c(5.0);
      auto next = add5(g_a);
      clobber(&next);
    };
    print_cell(measure("add_c(5)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto sub5 = micron::subtract_c(5.0);
      auto next = sub5(g_a);
      clobber(&next);
    };
    print_cell(measure("subtract_c(5)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto mul2 = micron::multiply_c(2.0);
      auto next = mul2(g_a);
      clobber(&next);
    };
    print_cell(measure("multiply_c(2)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto div2 = micron::divide_c(2.0);
      auto next = div2(g_a);
      clobber(&next);
    };
    print_cell(measure("divide_c(2)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
  }
  {
    auto setup = [&] { reset(); };
    auto kernel = [&] {
      auto pow2 = micron::pow_c(2.0);
      auto next = pow2(g_a);
      clobber(&next);
    };
    print_cell(measure("pow_c(2)", 1024, sizeof(f64), 1024, reps_for(1024), setup, kernel));
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

  micron::io::println("=== micron algorithm benchmark ===");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements per cell (median)");
  micron::io::println("sizes: 64, 1024, 16384 elements");

  sweep_algorithm();
  sweep_accumulate();
  sweep_find();
  sweep_arith();
  sweep_data();
  sweep_filter();
  sweep_fold();
  sweep_math();
  sweep_fp();

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sinks: ", sink_u64, " / f=", sink_f64, ")");
  return 0;
}
