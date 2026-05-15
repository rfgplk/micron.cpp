//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Extensive trig benchmark harness for the micron math kernel.
//
// Sweeps (impl × function × range), reports per-op cycles / instructions / IPC
// / branch-miss-rate via the bbench performance-event abstraction.
//
//   impls:
//     scalar polynomial    mc::math::mk::trig::{sin,cos,...}<F>
//     scalar CORDIC        mc::math::mk::cordic::{sin,cos,...}<F>
//     SIMD polynomial      mc::sin(simd::d256/d128/f256/f128) (Cody-Waite + Remez)
//     SIMD CORDIC          mc::sin_cordic(simd::d256/d128/f256/f128)
//
//   functions:
//     sin / cos / tan         (unary, full real line)
//     sincos                  (returns both)
//     asin / acos             (|x| <= 1)
//     atan                    (unbounded)
//     atan2                   (binary, all four quadrants)
//
//   ranges:
//     small   [-π, π]            (no range reduction)
//     medium  [-1000, 1000]       (Cody-Waite reduction)
//     large   [-2^33, 2^33]       (Payne-Hanek reduction)

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math.hpp"
#include "../src/simd/aliases.hpp"
#include "../src/std.hpp"

namespace
{

using trig_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u64 N = 1024;
constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 4;
constexpr u64 REPS_PER_MEAS = 64;

alignas(64) f64 g_in_f64[N];
alignas(64) f64 g_in_f64_b[N];
alignas(64) f64 g_out_f64[N];
alignas(64) f64 g_out_f64_b[N];
alignas(64) f32 g_in_f32[N];
alignas(64) f32 g_in_f32_b[N];
alignas(64) f32 g_out_f32[N];
alignas(64) f32 g_out_f32_b[N];

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s;
}

[[gnu::cold]] void
fill_uniform_f64(f64 *p, u64 n, f64 lo, f64 hi, u64 seed) noexcept
{
  for ( u64 i = 0; i < n; ++i ) {
    const f64 r = (lcg_next(seed) >> 11) * 0x1.0p-53;
    p[i] = lo + (hi - lo) * r;
  }
}

[[gnu::cold]] void
fill_uniform_f32(f32 *p, u64 n, f32 lo, f32 hi, u64 seed) noexcept
{
  for ( u64 i = 0; i < n; ++i ) {
    const f32 r = static_cast<f32>((lcg_next(seed) >> 11) * 0x1.0p-53);
    p[i] = lo + (hi - lo) * r;
  }
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
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

struct cell {
  const char *impl;
  u64 ops_per_call;
  f64 cyc_per_op;
  f64 inst_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 scaled = static_cast<u64>(v * 100.0 + 0.5);
  return { scaled / 100, static_cast<u32>(scaled % 100) };
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

[[gnu::cold]] void
print_header(const char *fn_name, const char *range_name)
{
  micron::io::println("");
  micron::io::println("[", fn_name, "]  range: ", range_name);
  line h;
  h.s("impl");
  h.s_at("cyc/op", 35);
  h.s_at("inst/op", 45);
  h.s_at("IPC", 55);
  h.s_at("bmiss%", 65);
  micron::io::println(h.str());
  micron::io::println("-----------------------------------------------------------------");
}

[[gnu::cold]] void
print_cell(const cell &c)
{
  const fmt2 cpo = to_fmt2(c.cyc_per_op);
  const fmt2 ipo = to_fmt2(c.inst_per_op);
  const fmt2 ipc = to_fmt2(c.ipc);
  const fmt2 bm = to_fmt2(c.bmiss_rate * 100.0);
  line ln;
  ln.s(c.impl);
  ln.f2_at(cpo, 35);
  ln.f2_at(ipo, 45);
  ln.f2_at(ipc, 55);
  ln.f2_at(bm, 65);
  micron::io::println(ln.str());
}

template<typename Kernel>
[[gnu::noinline]] cell
measure(const char *name, u64 ops_per_call, Kernel &&kernel) noexcept
{

  for ( u64 i = 0; i < WARMUP_REPS; ++i ) kernel();

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  const u64 total_ops = REPS_PER_MEAS * (N / ops_per_call) * ops_per_call;

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    trig_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < REPS_PER_MEAS; ++i ) kernel();
    evs.end();

    const auto cyc = evs.get<bbench::hardware_cycles>().retrieve();
    const auto ins = evs.get<bbench::hardware_instructions>().retrieve();
    const auto br = evs.get<bbench::branches>().retrieve();
    const auto bm = evs.get<bbench::branch_misses>().retrieve();

    cpo_samples[m] = static_cast<f64>(cyc) / static_cast<f64>(total_ops);
    ipo_samples[m] = static_cast<f64>(ins) / static_cast<f64>(total_ops);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return cell{ name,
               ops_per_call,
               median_f64(cpo_samples, K_MEASUREMENTS),
               median_f64(ipo_samples, K_MEASUREMENTS),
               median_f64(ipc_samples, K_MEASUREMENTS),
               median_f64(bm_samples, K_MEASUREMENTS) };
}

enum class range_kind {
  small,
  medium,
  large,
  unit,
  signed_pos,
};

[[gnu::cold]] void
fill_range(range_kind k, u64 seed_off = 0)
{
  const u64 s1 = 0xDEADBEEFCAFEBABEULL ^ seed_off;
  const u64 s2 = 0x0123456789ABCDEFULL ^ seed_off;
  switch ( k ) {
  case range_kind::small:
    fill_uniform_f64(g_in_f64, N, -3.14159265358979, 3.14159265358979, s1);
    fill_uniform_f32(g_in_f32, N, -3.14159265358979f, 3.14159265358979f, s2);
    fill_uniform_f64(g_in_f64_b, N, -3.14159265358979, 3.14159265358979, s1 + 1);
    fill_uniform_f32(g_in_f32_b, N, -3.14159265358979f, 3.14159265358979f, s2 + 1);
    break;
  case range_kind::medium:
    fill_uniform_f64(g_in_f64, N, -1000.0, 1000.0, s1);
    fill_uniform_f32(g_in_f32, N, -1000.0f, 1000.0f, s2);
    fill_uniform_f64(g_in_f64_b, N, -1000.0, 1000.0, s1 + 1);
    fill_uniform_f32(g_in_f32_b, N, -1000.0f, 1000.0f, s2 + 1);
    break;
  case range_kind::large:
    fill_uniform_f64(g_in_f64, N, -8589934592.0, 8589934592.0, s1);
    fill_uniform_f32(g_in_f32, N, -1048576.0f, 1048576.0f, s2);
    fill_uniform_f64(g_in_f64_b, N, -8589934592.0, 8589934592.0, s1 + 1);
    fill_uniform_f32(g_in_f32_b, N, -1048576.0f, 1048576.0f, s2 + 1);
    break;
  case range_kind::unit:
    fill_uniform_f64(g_in_f64, N, -1.0, 1.0, s1);
    fill_uniform_f32(g_in_f32, N, -1.0f, 1.0f, s2);
    fill_uniform_f64(g_in_f64_b, N, -1.0, 1.0, s1 + 1);
    fill_uniform_f32(g_in_f32_b, N, -1.0f, 1.0f, s2 + 1);
    break;
  case range_kind::signed_pos:
    fill_uniform_f64(g_in_f64, N, -8589934592.0, 8589934592.0, s1);
    fill_uniform_f32(g_in_f32, N, -1048576.0f, 1048576.0f, s2);
    fill_uniform_f64(g_in_f64_b, N, -8589934592.0, 8589934592.0, s1 + 1);
    fill_uniform_f32(g_in_f32_b, N, -1048576.0f, 1048576.0f, s2 + 1);
    break;
  }
}

const char *
range_label(range_kind k) noexcept
{
  switch ( k ) {
  case range_kind::small:
    return "small [-pi, pi]";
  case range_kind::medium:
    return "medium [-1e3, 1e3]";
  case range_kind::large:
    return "large [-2^33, 2^33]";
  case range_kind::unit:
    return "unit [-1, 1]";
  case range_kind::signed_pos:
    return "signed [-2^33, 2^33]";
  }
  return "?";
}

template<typename Fn>
[[gnu::noinline]] void
k_scalar_f64(Fn f) noexcept
{
  for ( u64 i = 0; i < N; ++i ) g_out_f64[i] = f(g_in_f64[i]);
  clobber(g_out_f64);
}

template<typename Fn>
[[gnu::noinline]] void
k_scalar_f32(Fn f) noexcept
{
  for ( u64 i = 0; i < N; ++i ) g_out_f32[i] = f(g_in_f32[i]);
  clobber(g_out_f32);
}

template<typename Fn>
[[gnu::noinline]] void
k_scalar_f64_2(Fn f) noexcept
{
  for ( u64 i = 0; i < N; ++i ) g_out_f64[i] = f(g_in_f64[i], g_in_f64_b[i]);
  clobber(g_out_f64);
}

template<typename Fn>
[[gnu::noinline]] void
k_scalar_f32_2(Fn f) noexcept
{
  for ( u64 i = 0; i < N; ++i ) g_out_f32[i] = f(g_in_f32[i], g_in_f32_b[i]);
  clobber(g_out_f32);
}

template<typename Fn>
[[gnu::noinline]] void
k_scalar_f64_sc(Fn f) noexcept
{
  for ( u64 i = 0; i < N; ++i ) f(g_in_f64[i], g_out_f64[i], g_out_f64_b[i]);
  clobber(g_out_f64);
  clobber(g_out_f64_b);
}

template<typename Fn>
[[gnu::noinline]] void
k_scalar_f32_sc(Fn f) noexcept
{
  for ( u64 i = 0; i < N; ++i ) f(g_in_f32[i], g_out_f32[i], g_out_f32_b[i]);
  clobber(g_out_f32);
  clobber(g_out_f32_b);
}

[[gnu::always_inline]] inline mc::simd::d256
ld_d256(const f64 *p) noexcept
{
  return mc::simd::avx::load_f64(reinterpret_cast<const double *>(p));
}

[[gnu::always_inline]] inline void
st_d256(f64 *p, mc::simd::d256 v) noexcept
{
  mc::simd::avx::store_f64(reinterpret_cast<double *>(p), v);
}

[[gnu::always_inline]] inline mc::simd::d128
ld_d128(const f64 *p) noexcept
{
  return mc::simd::sse::load_f64(reinterpret_cast<const double *>(p));
}

[[gnu::always_inline]] inline void
st_d128(f64 *p, mc::simd::d128 v) noexcept
{
  mc::simd::sse::store_f64(reinterpret_cast<double *>(p), v);
}

[[gnu::always_inline]] inline mc::simd::f256
ld_f256(const f32 *p) noexcept
{
  return mc::simd::avx::load_f32(reinterpret_cast<const float *>(p));
}

[[gnu::always_inline]] inline void
st_f256(f32 *p, mc::simd::f256 v) noexcept
{
  mc::simd::avx::store_f32(reinterpret_cast<float *>(p), v);
}

[[gnu::always_inline]] inline mc::simd::f128
ld_f128(const f32 *p) noexcept
{
  return mc::simd::sse::load_f32(reinterpret_cast<const float *>(p));
}

[[gnu::always_inline]] inline void
st_f128(f32 *p, mc::simd::f128 v) noexcept
{
  mc::simd::sse::store_f32(reinterpret_cast<float *>(p), v);
}

template<typename Fn>
[[gnu::noinline]] void
k_d256(Fn f) noexcept
{
  for ( u64 i = 0; i < N; i += 4 ) {
    mc::simd::d256 v = ld_d256(g_in_f64 + i);
    v = f(v);
    st_d256(g_out_f64 + i, v);
  }
  clobber(g_out_f64);
}

template<typename Fn>
[[gnu::noinline]] void
k_d256_sc(Fn f) noexcept
{
  for ( u64 i = 0; i < N; i += 4 ) {
    mc::simd::d256 v = ld_d256(g_in_f64 + i);
    mc::simd::d256 s, c;
    f(v, &s, &c);
    st_d256(g_out_f64 + i, s);
    st_d256(g_out_f64_b + i, c);
  }
  clobber(g_out_f64);
  clobber(g_out_f64_b);
}

template<typename Fn>
[[gnu::noinline]] void
k_d128(Fn f) noexcept
{
  for ( u64 i = 0; i < N; i += 2 ) {
    mc::simd::d128 v = ld_d128(g_in_f64 + i);
    v = f(v);
    st_d128(g_out_f64 + i, v);
  }
  clobber(g_out_f64);
}

template<typename Fn>
[[gnu::noinline]] void
k_f256(Fn f) noexcept
{
  for ( u64 i = 0; i < N; i += 8 ) {
    mc::simd::f256 v = ld_f256(g_in_f32 + i);
    v = f(v);
    st_f256(g_out_f32 + i, v);
  }
  clobber(g_out_f32);
}

template<typename Fn>
[[gnu::noinline]] void
k_f256_sc(Fn f) noexcept
{
  for ( u64 i = 0; i < N; i += 8 ) {
    mc::simd::f256 v = ld_f256(g_in_f32 + i);
    mc::simd::f256 s, c;
    f(v, &s, &c);
    st_f256(g_out_f32 + i, s);
    st_f256(g_out_f32_b + i, c);
  }
  clobber(g_out_f32);
  clobber(g_out_f32_b);
}

template<typename Fn>
[[gnu::noinline]] void
k_f128(Fn f) noexcept
{
  for ( u64 i = 0; i < N; i += 4 ) {
    mc::simd::f128 v = ld_f128(g_in_f32 + i);
    v = f(v);
    st_f128(g_out_f32 + i, v);
  }
  clobber(g_out_f32);
}

void
sweep_unary_sin(range_kind rk)
{
  print_header("sin", range_label(rk));
  fill_range(rk);

  print_cell(measure("mk::trig    f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::trig::sin<f64>(x); }); }));
  print_cell(measure("mk::trig    f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::trig::sin<f32>(x); }); }));
  print_cell(measure("mk::cordic  f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::cordic::sin<f64>(x); }); }));
  print_cell(measure("mk::cordic  f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::cordic::sin<f32>(x); }); }));
  print_cell(measure("mk::trig    d256 (4xf64)", 4, [] { k_d256([](mc::simd::d256 v) { return mc::sin(v); }); }));
  print_cell(measure("mk::trig    d128 (2xf64)", 2, [] { k_d128([](mc::simd::d128 v) { return mc::sin(v); }); }));
  print_cell(measure("mk::trig    f256 (8xf32)", 8, [] { k_f256([](mc::simd::f256 v) { return mc::sin(v); }); }));
  print_cell(measure("mk::trig    f128 (4xf32)", 4, [] { k_f128([](mc::simd::f128 v) { return mc::sin(v); }); }));
  print_cell(measure("mk::cordic  d256 (4xf64)", 4, [] { k_d256([](mc::simd::d256 v) { return mc::sin_cordic(v); }); }));
  print_cell(measure("mk::cordic  d128 (2xf64)", 2, [] { k_d128([](mc::simd::d128 v) { return mc::sin_cordic(v); }); }));
  print_cell(measure("mk::cordic  f256 (8xf32)", 8, [] { k_f256([](mc::simd::f256 v) { return mc::sin_cordic(v); }); }));
  print_cell(measure("mk::cordic  f128 (4xf32)", 4, [] { k_f128([](mc::simd::f128 v) { return mc::sin_cordic(v); }); }));
}

void
sweep_unary_cos(range_kind rk)
{
  print_header("cos", range_label(rk));
  fill_range(rk);

  print_cell(measure("mk::trig    f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::trig::cos<f64>(x); }); }));
  print_cell(measure("mk::trig    f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::trig::cos<f32>(x); }); }));
  print_cell(measure("mk::cordic  f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::cordic::cos<f64>(x); }); }));
  print_cell(measure("mk::cordic  f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::cordic::cos<f32>(x); }); }));
  print_cell(measure("mk::trig    d256 (4xf64)", 4, [] { k_d256([](mc::simd::d256 v) { return mc::cos(v); }); }));
  print_cell(measure("mk::trig    d128 (2xf64)", 2, [] { k_d128([](mc::simd::d128 v) { return mc::cos(v); }); }));
  print_cell(measure("mk::trig    f256 (8xf32)", 8, [] { k_f256([](mc::simd::f256 v) { return mc::cos(v); }); }));
  print_cell(measure("mk::trig    f128 (4xf32)", 4, [] { k_f128([](mc::simd::f128 v) { return mc::cos(v); }); }));
  print_cell(measure("mk::cordic  d256 (4xf64)", 4, [] { k_d256([](mc::simd::d256 v) { return mc::cos_cordic(v); }); }));
  print_cell(measure("mk::cordic  d128 (2xf64)", 2, [] { k_d128([](mc::simd::d128 v) { return mc::cos_cordic(v); }); }));
  print_cell(measure("mk::cordic  f256 (8xf32)", 8, [] { k_f256([](mc::simd::f256 v) { return mc::cos_cordic(v); }); }));
  print_cell(measure("mk::cordic  f128 (4xf32)", 4, [] { k_f128([](mc::simd::f128 v) { return mc::cos_cordic(v); }); }));
}

void
sweep_unary_tan(range_kind rk)
{
  print_header("tan", range_label(rk));
  fill_range(rk);

  print_cell(measure("mk::trig    f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::trig::tan<f64>(x); }); }));
  print_cell(measure("mk::trig    f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::trig::tan<f32>(x); }); }));
  print_cell(measure("mk::cordic  f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::cordic::tan<f64>(x); }); }));
  print_cell(measure("mk::cordic  f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::cordic::tan<f32>(x); }); }));

  print_cell(measure("mk::trig    d256 (4xf64)", 4, [] { k_d256([](mc::simd::d256 v) { return mc::tan(v); }); }));

  print_cell(measure("mk::cordic  d256 (4xf64)", 4, [] { k_d256([](mc::simd::d256 v) { return mc::tan_cordic(v); }); }));
  print_cell(measure("mk::cordic  d128 (2xf64)", 2, [] { k_d128([](mc::simd::d128 v) { return mc::tan_cordic(v); }); }));
  print_cell(measure("mk::cordic  f256 (8xf32)", 8, [] { k_f256([](mc::simd::f256 v) { return mc::tan_cordic(v); }); }));
  print_cell(measure("mk::cordic  f128 (4xf32)", 4, [] { k_f128([](mc::simd::f128 v) { return mc::tan_cordic(v); }); }));
}

void
sweep_sincos(range_kind rk)
{
  print_header("sincos", range_label(rk));
  fill_range(rk);

  print_cell(measure("mk::trig    f64 scalar ", 1,
                     [] { k_scalar_f64_sc([](f64 x, f64 &s, f64 &c) { mc::math::mk::trig::sincos<f64>(x, s, c); }); }));
  print_cell(measure("mk::trig    f32 scalar ", 1,
                     [] { k_scalar_f32_sc([](f32 x, f32 &s, f32 &c) { mc::math::mk::trig::sincos<f32>(x, s, c); }); }));
  print_cell(measure("mk::cordic  f64 scalar ", 1,
                     [] { k_scalar_f64_sc([](f64 x, f64 &s, f64 &c) { mc::math::mk::cordic::sincos<f64>(x, s, c); }); }));
  print_cell(measure("mk::cordic  f32 scalar ", 1,
                     [] { k_scalar_f32_sc([](f32 x, f32 &s, f32 &c) { mc::math::mk::cordic::sincos<f32>(x, s, c); }); }));

  print_cell(measure("mk::trig    d256 (4xf64)", 4,
                     [] { k_d256_sc([](mc::simd::d256 v, mc::simd::d256 *s, mc::simd::d256 *c) { mc::math::mk::sincos(v, s, c); }); }));

  print_cell(measure("mk::cordic  d256 (4xf64)", 4, [] {
    k_d256_sc([](mc::simd::d256 v, mc::simd::d256 *s, mc::simd::d256 *c) { mc::math::mk::sincos_cordic(v, s, c); });
  }));
  print_cell(measure("mk::cordic  f256 (8xf32)", 8, [] {
    k_f256_sc([](mc::simd::f256 v, mc::simd::f256 *s, mc::simd::f256 *c) { mc::math::mk::sincos_cordic(v, s, c); });
  }));
}

void
sweep_inverse_unit(const char *fn_name, range_kind rk)
{
  print_header(fn_name, range_label(rk));
  fill_range(rk);

  if ( mc::strcmp(fn_name, "asin") == 0 ) {
    print_cell(measure("mk::trig    f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::trig::asin<f64>(x); }); }));
    print_cell(measure("mk::trig    f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::trig::asin<f32>(x); }); }));
    print_cell(measure("mk::cordic  f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::cordic::asin<f64>(x); }); }));
    print_cell(measure("mk::cordic  f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::cordic::asin<f32>(x); }); }));
  } else {
    print_cell(measure("mk::trig    f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::trig::acos<f64>(x); }); }));
    print_cell(measure("mk::trig    f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::trig::acos<f32>(x); }); }));
    print_cell(measure("mk::cordic  f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::cordic::acos<f64>(x); }); }));
    print_cell(measure("mk::cordic  f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::cordic::acos<f32>(x); }); }));
  }
}

void
sweep_atan(range_kind rk)
{
  print_header("atan", range_label(rk));
  fill_range(rk);

  print_cell(measure("mk::trig    f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::trig::atan<f64>(x); }); }));
  print_cell(measure("mk::trig    f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::trig::atan<f32>(x); }); }));
  print_cell(measure("mk::cordic  f64 scalar ", 1, [] { k_scalar_f64([](f64 x) { return mc::math::mk::cordic::atan<f64>(x); }); }));
  print_cell(measure("mk::cordic  f32 scalar ", 1, [] { k_scalar_f32([](f32 x) { return mc::math::mk::cordic::atan<f32>(x); }); }));
}

void
sweep_atan2(range_kind rk)
{
  print_header("atan2", range_label(rk));
  fill_range(rk);

  print_cell(
      measure("mk::trig    f64 scalar ", 1, [] { k_scalar_f64_2([](f64 y, f64 x) { return mc::math::mk::trig::atan2<f64>(y, x); }); }));
  print_cell(
      measure("mk::trig    f32 scalar ", 1, [] { k_scalar_f32_2([](f32 y, f32 x) { return mc::math::mk::trig::atan2<f32>(y, x); }); }));
  print_cell(
      measure("mk::cordic  f64 scalar ", 1, [] { k_scalar_f64_2([](f64 y, f64 x) { return mc::math::mk::cordic::atan2<f64>(y, x); }); }));
  print_cell(
      measure("mk::cordic  f32 scalar ", 1, [] { k_scalar_f32_2([](f32 y, f32 x) { return mc::math::mk::cordic::atan2<f32>(y, x); }); }));
}

};      // namespace

int
main(void)
{

  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  micron::io::println("=== micron trig benchmark ===");
  micron::io::println("buffer: ", N, " elements (", N * 8, " B f64, ", N * 4, " B f32 — L1d-resident)");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements/cell × ", REPS_PER_MEAS, " reps/measurement");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses (4-event group)");

  micron::io::println("");
  micron::io::println("# sin -- full unary sweep across scalar/SIMD × poly/CORDIC × 3 ranges");
  sweep_unary_sin(range_kind::small);
  sweep_unary_sin(range_kind::medium);
  sweep_unary_sin(range_kind::large);

  micron::io::println("");
  micron::io::println("# cos");
  sweep_unary_cos(range_kind::small);
  sweep_unary_cos(range_kind::medium);
  sweep_unary_cos(range_kind::large);

  micron::io::println("");
  micron::io::println("# tan");
  sweep_unary_tan(range_kind::small);
  sweep_unary_tan(range_kind::medium);
  sweep_unary_tan(range_kind::large);

  micron::io::println("");
  micron::io::println("# sincos -- combined sin & cos");
  sweep_sincos(range_kind::small);
  sweep_sincos(range_kind::medium);
  sweep_sincos(range_kind::large);

  micron::io::println("");
  micron::io::println("# inverse trig over [-1, 1]");
  sweep_inverse_unit("asin", range_kind::unit);
  sweep_inverse_unit("acos", range_kind::unit);

  micron::io::println("");
  micron::io::println("# atan -- full real line");
  sweep_atan(range_kind::small);
  sweep_atan(range_kind::medium);
  sweep_atan(range_kind::large);

  micron::io::println("");
  micron::io::println("# atan2 -- four-quadrant arctan");
  sweep_atan2(range_kind::small);
  sweep_atan2(range_kind::medium);
  sweep_atan2(range_kind::large);

  micron::io::println("");
  micron::io::println("=== done ===");
  return 0;
}
