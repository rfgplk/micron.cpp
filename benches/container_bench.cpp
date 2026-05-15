//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Container benchmark; micron::vector and micron::array families
//
//   vector ops (heap, resizing):
//     push_back, emplace_back, insert (front/middle/back),
//     erase (front/middle/back), pop_back, copy, move, iterate-sum, reserve,
//     resize-grow, resize-shrink, clear
//
//   svector (stack, fixed N):
//     same op set as vector but no reallocation
//
//   array<T,N> / iarray / farray (stack-allocated fixed):
//     copy, move, iterate-sum, fill-by-functor, element write

#include "../external/bbench/bench.hpp"

#include "../src/array/array.hpp"
#include "../src/array/farray.hpp"
#include "../src/array/iarray.hpp"
#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/std.hpp"
#include "../src/vector/fvector.hpp"
#include "../src/vector/svector.hpp"
#include "../src/vector/vector.hpp"

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

constexpr u64 SIZES[] = {
  64,
  1ULL << 10,
  1ULL << 16,
  1ULL << 20,
};

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s;
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
clobber_val(u64 v) noexcept
{
  asm volatile("" : : "r"(v));
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

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

struct cell {
  const char *name;
  u64 size;
  u64 elem_bytes;
  f64 cyc_per_op;
  f64 ns_per_op;

  f64 ipc;
  f64 bmiss_rate;
};

[[gnu::cold]] void
print_col_header()
{
  line h;
  h.s("op");
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

[[gnu::cold]] void
print_cell(const cell &c)
{
  const fmt2 cpo = to_fmt2(c.cyc_per_op);
  const fmt2 ipc = to_fmt2(c.ipc);
  const fmt2 bm = to_fmt2(c.bmiss_rate * 100.0);
  line ln;
  ln.s(c.name);
  ln.u_at(c.size, 32);
  ln.u_at(c.elem_bytes, 42);
  ln.f2_at(cpo, 52);
  ln.f2_at(ipc, 62);
  ln.f2_at(bm, 72);
  micron::io::println(ln.str());
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
               0.0,
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

template<typename T>
void
sweep_vector_t(const char *tag)
{
  print_header(tag);
  for ( u64 N : SIZES ) {

    {
      mc::vector<T> v;
      auto setup = [&] {
        v.clear();
        v.reserve(N);
      };
      auto kernel = [&] {
        for ( u64 i = 0; i < N; ++i ) v.push_back(T{});
        clobber(v.data());
      };
      print_cell(measure("push_back (preres) ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }

    {
      mc::vector<T> v;
      auto setup = [&] {
        v.clear();
        v.reserve(N);
      };
      auto kernel = [&] {
        for ( u64 i = 0; i < N; ++i ) v.emplace_back();
        clobber(v.data());
      };
      print_cell(measure("emplace_back       ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }

    if ( N <= (1ULL << 16) ) {
      mc::vector<T> v;
      auto setup = [&] { v.clear(); };
      auto kernel = [&] {
        for ( u64 i = 0; i < N; ++i ) v.push_back(T{});
        clobber(v.data());
      };

      print_cell(measure("push_back (grow)   ", N, sizeof(T), N, reps_for(N * 2), setup, kernel));
    }

    {
      mc::vector<T> v;
      auto setup = [&] {
        v.clear();
        for ( u64 i = 0; i < N; ++i ) v.push_back(T{});
      };
      auto kernel = [&] {
        while ( !v.empty() ) v.pop_back();
        clobber(v.data());
      };

      print_cell(measure("pop_back (drain)   ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }

    {
      mc::vector<T> v;
      static volatile u64 sink = 0;
      auto setup = [&] {
        v.clear();
        v.reserve(N);
        for ( u64 i = 0; i < N; ++i ) v.push_back(make_from<T>(i));
      };
      auto kernel = [&] {
        u64 s = 0;
        for ( u64 i = 0; i < N; ++i ) s += reduce_one(v[i]);
        sink = s;
      };
      print_cell(measure("iterate-sum [i]    ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }

    {
      mc::vector<T> v;
      static volatile u64 sink = 0;
      auto setup = [&] {
        v.clear();
        v.reserve(N);
        for ( u64 i = 0; i < N; ++i ) v.push_back(make_from<T>(i));
      };
      auto kernel = [&] {
        u64 s = 0;
        for ( auto it = v.begin(); it != v.end(); ++it ) s += reduce_one(*it);
        sink = s;
      };
      print_cell(measure("iterate-sum (iter) ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }

    {
      mc::vector<T> src;
      mc::vector<T> dst;
      auto setup = [&] {
        src.clear();
        src.reserve(N);
        for ( u64 i = 0; i < N; ++i ) src.push_back(make_from<T>(i));
        dst.clear();
      };
      auto kernel = [&] {
        mc::vector<T> tmp(src);
        clobber(tmp.data());
      };
      print_cell(measure("copy-ctor          ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }

    {
      mc::vector<T> src;
      auto setup = [&] {
        src.clear();
        src.reserve(N);
        for ( u64 i = 0; i < N; ++i ) src.push_back(make_from<T>(i));
      };
      auto kernel = [&] {
        mc::vector<T> tmp(micron::move(src));

        src = micron::move(tmp);
        clobber(src.data());
      };
      print_cell(measure("move-ctor (pair)   ", N, sizeof(T), N, reps_for(N) / 4, setup, kernel));
    }

    if ( N <= (1ULL << 12) ) {
      mc::vector<T> v;
      auto setup = [&] {
        v.clear();
        v.reserve(N + 16);
        for ( u64 i = 0; i < N; ++i ) v.push_back(make_from<T>(i));
      };
      auto kernel = [&] {
        v.insert(static_cast<typename mc::vector<T>::size_type>(0), T{});
        v.pop_back();
        clobber(v.data());
      };

      print_cell(measure("insert[0]+popb     ", N, sizeof(T), N, reps_for(N * N), setup, kernel));
    }

    if ( N <= (1ULL << 12) ) {
      mc::vector<T> v;
      auto setup = [&] {
        v.clear();
        v.reserve(N + 16);
        for ( u64 i = 0; i < N; ++i ) v.push_back(make_from<T>(i));
      };
      auto kernel = [&] {
        v.push_back(T{});
        v.erase(static_cast<typename mc::vector<T>::size_type>(0), static_cast<typename mc::vector<T>::size_type>(1));
        clobber(v.data());
      };
      print_cell(measure("erase[0]+push      ", N, sizeof(T), N, reps_for(N * N), setup, kernel));
    }

    if ( N <= (1ULL << 12) ) {
      auto setup = [] { };
      auto kernel = [&] {
        mc::vector<T> v;
        v.reserve(N);
        clobber(v.data());
      };
      print_cell(measure("ctor+reserve       ", N, sizeof(T), 1, reps_for(N * 32), setup, kernel));
    }
  }
}

template<typename T>
void
sweep_fvector_t(const char *tag)
{
  print_header(tag);
  for ( u64 N : SIZES ) {
    {
      mc::fvector<T> v;
      auto setup = [&] {
        v.clear();
        v.reserve(N);
      };
      auto kernel = [&] {
        for ( u64 i = 0; i < N; ++i ) v.push_back(T{});
        clobber(v.data());
      };
      print_cell(measure("push_back (preres) ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }
    {
      mc::fvector<T> v;
      auto setup = [&] {
        v.clear();
        v.reserve(N);
        for ( u64 i = 0; i < N; ++i ) v.push_back(make_from<T>(i));
      };
      static volatile u64 sink = 0;
      auto kernel = [&] {
        u64 s = 0;
        for ( u64 i = 0; i < N; ++i ) s += reduce_one(v[i]);
        sink = s;
      };
      print_cell(measure("iterate-sum [i]    ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }

    {
      mc::fvector<T> src;
      auto setup = [&] {
        src.clear();
        src.reserve(N);
        for ( u64 i = 0; i < N; ++i ) src.push_back(make_from<T>(i));
      };
      auto kernel = [&] {
        mc::fvector<T> tmp;
        tmp.reserve(N);
        for ( u64 i = 0; i < N; ++i ) tmp.push_back(src[i]);
        clobber(tmp.data());
      };
      print_cell(measure("manual-copy        ", N, sizeof(T), N, reps_for(N), setup, kernel));
    }
  }
}

template<typename T, usize Cap>
void
sweep_svector_at(const char *tag)
{
  micron::io::println("");
  micron::io::println("[", tag, " cap=", Cap, "]");
  print_col_header();

  const u64 N = Cap;

  {
    mc::svector<T, Cap> v;
    auto setup = [] { };
    auto kernel = [&] {
      v.clear();
      for ( u64 i = 0; i < Cap; ++i ) v.push_back(T{});
      clobber(v.data());
    };
    print_cell(measure("push_back          ", N, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    mc::svector<T, Cap> v;
    auto setup = [&] {
      v.clear();
      for ( u64 i = 0; i < Cap; ++i ) v.push_back(make_from<T>(i));
    };
    static volatile u64 sink = 0;
    auto kernel = [&] {
      u64 s = 0;
      for ( u64 i = 0; i < Cap; ++i ) s += reduce_one(v[i]);
      sink = s;
    };
    print_cell(measure("iterate-sum        ", N, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    mc::svector<T, Cap> v;
    auto setup = [&] {
      v.clear();
      for ( u64 i = 0; i < Cap; ++i ) v.push_back(make_from<T>(i));
    };
    auto kernel = [&] {
      mc::svector<T, Cap> tmp(v);
      clobber(tmp.data());
    };
    print_cell(measure("copy-ctor          ", N, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_array_at(const char *tag)
{
  micron::io::println("");
  micron::io::println("[", tag, " cap=", Cap, "]");
  print_col_header();

  static mc::array<T, Cap> g_a;
  static mc::array<T, Cap> g_b;

  for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);

  {
    static volatile u64 sink = 0;
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Cap; ++i ) s += reduce_one(g_a[i]);
      sink = s;
    };
    print_cell(measure("iterate-sum [i]    ", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    static volatile u64 sink = 0;
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( auto &v : g_a ) s += reduce_one(v);
      sink = s;
    };
    print_cell(measure("iterate-sum (iter) ", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      g_b = g_a;
      clobber(&g_b);
    };
    print_cell(measure("copy-assign        ", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }

  {
    auto setup = [] { };
    auto kernel = [&] {
      for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);
      clobber(&g_a);
    };
    print_cell(measure("fill (op[]=)       ", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
}

template<typename T, usize Cap>
void
sweep_farray_at(const char *tag)
{
  micron::io::println("");
  micron::io::println("[", tag, " cap=", Cap, "]");
  print_col_header();

  static mc::farray<T, Cap> g_a;
  static mc::farray<T, Cap> g_b;

  for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);

  {
    static volatile u64 sink = 0;
    auto setup = [] { };
    auto kernel = [&] {
      u64 s = 0;
      for ( usize i = 0; i < Cap; ++i ) s += reduce_one(g_a[i]);
      sink = s;
    };
    print_cell(measure("iterate-sum        ", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      g_b = g_a;
      clobber(&g_b);
    };
    print_cell(measure("copy-assign        ", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
  }
  {
    auto setup = [] { };
    auto kernel = [&] {
      for ( usize i = 0; i < Cap; ++i ) g_a[i] = make_from<T>(i);
      clobber(&g_a);
    };
    print_cell(measure("fill (op[]=)       ", Cap, sizeof(T), Cap, reps_for(Cap), setup, kernel));
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

  micron::io::println("=== micron container benchmark ===");
  micron::io::println("vector / fvector sizes: 64, 1K, 64K, 1M elements");
  micron::io::println("svector / array sizes:  64, 1024 (stack-bound)");
  micron::io::println("element types:  i32 (4B), point3 (24B)");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements/cell");

  sweep_vector_t<i32>("vector<i32>");
  sweep_vector_t<point3>("vector<point3>");

  sweep_fvector_t<i32>("fvector<i32>");

  sweep_svector_at<i32, 64>("svector<i32>");
  sweep_svector_at<i32, 1024>("svector<i32>");
  sweep_svector_at<point3, 64>("svector<point3>");

  sweep_array_at<i32, 64>("array<i32>");
  sweep_array_at<i32, 1024>("array<i32>");
  sweep_array_at<point3, 64>("array<point3>");

  sweep_farray_at<i32, 64>("farray<i32>");
  sweep_farray_at<i32, 1024>("farray<i32>");

  micron::io::println("");
  micron::io::println("=== done ===");
  return 0;
}
