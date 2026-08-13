//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Graphics-math kernel benchmark: normalize / cross / inv4 / look_at /
// perspective across linalg (vec<F,N>) and quants (vector_3<T>) families,
// with the SIMD quaternion multiply as the in-house reference kernel.
//
// Harness mirrors quaternion_bench.cpp: bbench perf counters, median of
// K measurements, throughput loops over L1-resident buffers.
//
// reference targets (glm, measured out-of-tree during canvas triage, inst/op):
//   normalize 10.4 · cross 10.5 · inverse4 71 · look_at 33

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/geometry/projection.hpp"
#include "../src/math/linalg.hpp"
#include "../src/math/quants/vecs.hpp"
#include "../src/math/quaternions/quaternions.hpp"
#include "../src/std.hpp"

namespace
{

using geom_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u64 N = 256;
constexpr u64 NM = 96;
constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 4;
constexpr u64 REPS_PER_MEAS = 256;

namespace geo = mc::math::geometry;
namespace lops = mc::math::linalg::ops;

using v3f = mc::math::vec<f32, 3>;
using v3d = mc::math::vec<f64, 3>;
using m4f = mc::math::mat<f32, 4, 4>;
using m4d = mc::math::mat<f64, 4, 4>;
using vec3f = mc::vector_3<f32>;
using quatf = mc::math::quaternions::quaternion<f32>;
using quatd = mc::math::quaternions::quaternion<f64>;
using xform_iso_f = geo::transform<f32, 3, geo::transform_mode::isometry>;
using xform_iso_d = geo::transform<f64, 3, geo::transform_mode::isometry>;
using xform_prj_f = geo::transform<f32, 3, geo::transform_mode::projective>;
using xform_prj_d = geo::transform<f64, 3, geo::transform_mode::projective>;

alignas(64) v3f g_a3f[N];
alignas(64) v3f g_b3f[N];
alignas(64) v3f g_o3f[N];
alignas(64) v3d g_a3d[N];
alignas(64) v3d g_b3d[N];
alignas(64) v3d g_o3d[N];

alignas(64) vec3f g_qa3[N];
alignas(64) vec3f g_qb3[N];
alignas(64) vec3f g_qo3[N];

alignas(64) m4f g_m4f_in[NM];
alignas(64) m4f g_m4f_out[NM];
alignas(64) m4d g_m4d_in[NM];
alignas(64) m4d g_m4d_out[NM];

alignas(64) v3f g_eye_f[NM];
alignas(64) v3f g_tgt_f[NM];
alignas(64) v3f g_up_f[NM];
alignas(64) v3d g_eye_d[NM];
alignas(64) v3d g_tgt_d[NM];
alignas(64) v3d g_up_d[NM];
alignas(64) xform_iso_f g_lt_f[NM];
alignas(64) xform_iso_d g_lt_d[NM];
alignas(64) xform_prj_f g_pp_f[NM];
alignas(64) xform_prj_d g_pp_d[NM];
alignas(64) f32 g_aspect_f[NM];
alignas(64) f64 g_aspect_d[NM];

alignas(64) quatf g_qa_f[N];
alignas(64) quatf g_qb_f[N];
alignas(64) quatf g_qout_f[N];
alignas(64) quatd g_qa_d[N];
alignas(64) quatd g_qb_d[N];
alignas(64) quatd g_qout_d[N];

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s;
}

[[gnu::always_inline]] inline f64
lcg_unit(u64 &s) noexcept
{
  return static_cast<f64>(lcg_next(s) >> 11) * 0x1.0p-53;
}

[[gnu::always_inline]] inline f64
lcg_sym(u64 &s) noexcept
{
  return lcg_unit(s) * 2.0 - 1.0;
}

[[gnu::cold]] void
fill_buffers(u64 seed)
{
  for ( u64 i = 0; i < N; ++i ) {
    const f64 ax = lcg_sym(seed), ay = lcg_sym(seed), az = lcg_sym(seed);
    const f64 bx = lcg_sym(seed), by = lcg_sym(seed), bz = lcg_sym(seed);
    g_a3d[i] = v3d{ ax, ay, az };
    g_b3d[i] = v3d{ bx, by, bz };
    g_a3f[i] = v3f{ static_cast<f32>(ax), static_cast<f32>(ay), static_cast<f32>(az) };
    g_b3f[i] = v3f{ static_cast<f32>(bx), static_cast<f32>(by), static_cast<f32>(bz) };
    g_qa3[i] = vec3f{ static_cast<f32>(ax), static_cast<f32>(ay), static_cast<f32>(az) };
    g_qb3[i] = vec3f{ static_cast<f32>(bx), static_cast<f32>(by), static_cast<f32>(bz) };

    const f64 an = mc::math::mk::pow_ns::sqrt<f64>(ax * ax + ay * ay + az * az) + 1e-30;
    const f64 bn = mc::math::mk::pow_ns::sqrt<f64>(bx * bx + by * by + bz * bz) + 1e-30;
    const f64 th = lcg_unit(seed) * 6.283185307179586;
    const f64 ph = lcg_unit(seed) * 6.283185307179586;
    g_qa_d[i] = mc::math::quaternions::from_axis_angle<f64>(ax / an, ay / an, az / an, th);
    g_qb_d[i] = mc::math::quaternions::from_axis_angle<f64>(bx / bn, by / bn, bz / bn, ph);
    g_qa_f[i] = quatf{ static_cast<f32>(g_qa_d[i].x), static_cast<f32>(g_qa_d[i].y), static_cast<f32>(g_qa_d[i].z),
                       static_cast<f32>(g_qa_d[i].w) };
    g_qb_f[i] = quatf{ static_cast<f32>(g_qb_d[i].x), static_cast<f32>(g_qb_d[i].y), static_cast<f32>(g_qb_d[i].z),
                       static_cast<f32>(g_qb_d[i].w) };
  }

  for ( u64 i = 0; i < NM; ++i ) {

    m4d cand{};
    f64 det = 0.0;
    do {
      for ( usize k = 0; k < 16; ++k ) cand.data[k] = lcg_sym(seed);
      det = lops::det4<f64>(cand);
    } while ( det < 1e-2 && det > -1e-2 );
    g_m4d_in[i] = cand;
    for ( usize k = 0; k < 16; ++k ) g_m4f_in[i].data[k] = static_cast<f32>(cand.data[k]);

    const f64 ex = lcg_sym(seed) * 2.0, ey = lcg_sym(seed) * 2.0, ez = lcg_sym(seed) * 2.0;
    const f64 tx = lcg_sym(seed) * 2.0, ty = lcg_sym(seed) * 2.0, tz = lcg_sym(seed) * 2.0;
    const f64 ux = lcg_sym(seed) * 0.25, uy = 1.0, uz = lcg_sym(seed) * 0.25;
    g_eye_d[i] = v3d{ ex, ey, ez };
    g_tgt_d[i] = v3d{ tx, ty, tz };
    g_up_d[i] = v3d{ ux, uy, uz };
    g_eye_f[i] = v3f{ static_cast<f32>(ex), static_cast<f32>(ey), static_cast<f32>(ez) };
    g_tgt_f[i] = v3f{ static_cast<f32>(tx), static_cast<f32>(ty), static_cast<f32>(tz) };
    g_up_f[i] = v3f{ static_cast<f32>(ux), static_cast<f32>(uy), static_cast<f32>(uz) };

    g_aspect_d[i] = 1.0 + lcg_unit(seed);
    g_aspect_f[i] = static_cast<f32>(g_aspect_d[i]);
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
  f64 cyc_per_op;
  f64 inst_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

[[gnu::cold]] void
print_header(const char *section)
{
  micron::io::println("");
  micron::io::println("[", section, "]");
  line h;
  h.s("op");
  h.s_at("cyc/op", 32);
  h.s_at("inst/op", 42);
  h.s_at("IPC", 52);
  h.s_at("bmiss%", 62);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------");
}

[[gnu::cold]] void
print_cell(const cell &c)
{
  const fmt2 cpo = to_fmt2(c.cyc_per_op);
  const fmt2 ipo = to_fmt2(c.inst_per_op);
  const fmt2 ipc = to_fmt2(c.ipc);
  const fmt2 bm = to_fmt2(c.bmiss_rate * 100.0);
  line ln;
  ln.s(c.name);
  ln.f2_at(cpo, 32);
  ln.f2_at(ipo, 42);
  ln.f2_at(ipc, 52);
  ln.f2_at(bm, 62);
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

  const u64 total_ops = REPS_PER_MEAS * ops_per_call;

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    geom_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < REPS_PER_MEAS; ++i ) kernel();
    evs.end();
    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    cpo_samples[m] = static_cast<f64>(cyc) / static_cast<f64>(total_ops);
    ipo_samples[m] = static_cast<f64>(ins) / static_cast<f64>(total_ops);
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return cell{ name, median_f64(cpo_samples, K_MEASUREMENTS), median_f64(ipo_samples, K_MEASUREMENTS),
               median_f64(ipc_samples, K_MEASUREMENTS), median_f64(bm_samples, K_MEASUREMENTS) };
}

void
sweep_vec3()
{
  print_header("vec3 (linalg vec<F,3> / quants vector_3<f32>)");

  print_cell(measure("normalize v3 f32   ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_o3f[i] = lops::normalize<f32, 3>(g_a3f[i]);
    clobber(g_o3f);
  }));
  print_cell(measure("normalize v3 f64   ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_o3d[i] = lops::normalize<f64, 3>(g_a3d[i]);
    clobber(g_o3d);
  }));
  print_cell(measure("normalize f32 fast ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_o3f[i] = lops::normalize(g_a3f[i], mc::math::policy::fast);
    clobber(g_o3f);
  }));
  print_cell(measure("cross v3 f32       ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_o3f[i] = lops::cross<f32>(g_a3f[i], g_b3f[i]);
    clobber(g_o3f);
  }));
  print_cell(measure("cross v3 f64       ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_o3d[i] = lops::cross<f64>(g_a3d[i], g_b3d[i]);
    clobber(g_o3d);
  }));
  print_cell(measure("vector_3 normalized", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_qo3[i] = g_qa3[i].normalized();
    clobber(g_qo3);
  }));
  print_cell(measure("vector_3 cross     ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_qo3[i] = g_qa3[i].cross(g_qb3[i]);
    clobber(g_qo3);
  }));
}

void
sweep_mat4()
{
  print_header("mat4 inverse");

  print_cell(measure("inv4 f32           ", NM, [] {
    for ( u64 i = 0; i < NM; ++i ) g_m4f_out[i] = lops::inv4<f32>(g_m4f_in[i]);
    clobber(g_m4f_out);
  }));
  print_cell(measure("inv4 f64           ", NM, [] {
    for ( u64 i = 0; i < NM; ++i ) g_m4d_out[i] = lops::inv4<f64>(g_m4d_in[i]);
    clobber(g_m4d_out);
  }));
}

void
sweep_geometry()
{
  print_header("geometry (look_at / perspective)");

  print_cell(measure("look_at f32        ", NM, [] {
    for ( u64 i = 0; i < NM; ++i ) g_lt_f[i] = geo::look_at<geo::handedness::right, f32>(g_eye_f[i], g_tgt_f[i], g_up_f[i]);
    clobber(g_lt_f);
  }));
  print_cell(measure("look_at f64        ", NM, [] {
    for ( u64 i = 0; i < NM; ++i ) g_lt_d[i] = geo::look_at<geo::handedness::right, f64>(g_eye_d[i], g_tgt_d[i], g_up_d[i]);
    clobber(g_lt_d);
  }));
  print_cell(measure("look_at f32 fast   ", NM, [] {
    for ( u64 i = 0; i < NM; ++i )
      g_lt_f[i] = geo::look_at<geo::handedness::right, f32>(g_eye_f[i], g_tgt_f[i], g_up_f[i], mc::math::policy::fast);
    clobber(g_lt_f);
  }));
  print_cell(measure("perspective f32    ", NM, [] {
    for ( u64 i = 0; i < NM; ++i )
      g_pp_f[i]
          = geo::perspective_projection<geo::handedness::right, geo::clip_depth::neg_one_to_one, f32>(0.9f, g_aspect_f[i], 0.1f, 100.0f);
    clobber(g_pp_f);
  }));
  print_cell(measure("perspective f64    ", NM, [] {
    for ( u64 i = 0; i < NM; ++i )
      g_pp_d[i] = geo::perspective_projection<geo::handedness::right, geo::clip_depth::neg_one_to_one, f64>(0.9, g_aspect_d[i], 0.1, 100.0);
    clobber(g_pp_d);
  }));
}

void
sweep_reference()
{
  print_header("reference (SIMD quaternion multiply)");

  print_cell(measure("quat multiply f64  ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::multiply<f64>(g_qa_d[i], g_qb_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("quat multiply f32  ", N, [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_f[i] = mc::math::quaternions::multiply<f32>(g_qa_f[i], g_qb_f[i]);
    clobber(g_qout_f);
  }));
}

};      // namespace

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  fill_buffers(0xC0FFEE);

  micron::io::println("=== micron math geometry benchmark ===");
  micron::io::println("buffers: ", N, " vec3/quat rows, ", NM, " mat4/transform rows (L1d-resident)");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements/cell × ", REPS_PER_MEAS, " reps");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("glm reference (out-of-tree canvas triage, inst/op): normalize 10.4 · cross 10.5 · inv4 71 · look_at 33");

  sweep_vec3();
  sweep_mat4();
  sweep_geometry();
  sweep_reference();

  micron::io::println("");
  micron::io::println("=== done ===");
  return 0;
}
