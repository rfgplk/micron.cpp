//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Extensive quaternion benchmark for the micron quaternion module.
//
// Covers:
//   algebra:        multiply, conjugate, inverse, normalize, norm
//   rotations:      from_axis_angle, to_axis_angle, to_matrix, from_matrix,
//                   rotate(vec3)
//   euler:          from_euler<Ord>, to_euler<Ord> across all 12 conventions
//                   (Tait-Bryan + proper-Euler)
//   interpolation:  lerp, nlerp, slerp
//   kinematics:     derivative, integrate (full + small-angle + Pade),
//                   angular_velocity (full + Pade)

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/quants/vecs.hpp"
#include "../src/math/quaternions/quaternions.hpp"
#include "../src/std.hpp"

namespace
{

using quat_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u64 N = 256;
constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 4;
constexpr u64 REPS_PER_MEAS = 256;

using quatf = mc::math::quaternions::quaternion<f32>;
using quatd = mc::math::quaternions::quaternion<f64>;
using vec3f = mc::vector_3<f32>;
using vec3d = mc::vector_3<f64>;
using mat3f = mc::math::mat<f32, 3, 3>;
using mat3d = mc::math::mat<f64, 3, 3>;

alignas(64) quatd g_qa_d[N];
alignas(64) quatd g_qb_d[N];
alignas(64) quatd g_qout_d[N];
alignas(64) vec3d g_va_d[N];
alignas(64) vec3d g_vout_d[N];
alignas(64) mat3d g_mout_d[N];
alignas(64) f64 g_angles_d[N];

alignas(64) quatf g_qa_f[N];
alignas(64) quatf g_qb_f[N];
alignas(64) quatf g_qout_f[N];
alignas(64) vec3f g_va_f[N];
alignas(64) vec3f g_vout_f[N];

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

[[gnu::cold]] void
fill_buffers(u64 seed)
{

  for ( u64 i = 0; i < N; ++i ) {
    const f64 ax = lcg_unit(seed) * 2.0 - 1.0;
    const f64 ay = lcg_unit(seed) * 2.0 - 1.0;
    const f64 az = lcg_unit(seed) * 2.0 - 1.0;
    const f64 norm = mc::math::mk::pow_ns::sqrt<f64>(ax * ax + ay * ay + az * az) + 1e-30;
    const f64 nx = ax / norm;
    const f64 ny = ay / norm;
    const f64 nz = az / norm;
    const f64 theta = lcg_unit(seed) * 6.283185307179586;
    g_qa_d[i] = mc::math::quaternions::from_axis_angle<f64>(nx, ny, nz, theta);

    const f64 bx = lcg_unit(seed) * 2.0 - 1.0;
    const f64 by = lcg_unit(seed) * 2.0 - 1.0;
    const f64 bz = lcg_unit(seed) * 2.0 - 1.0;
    const f64 bn = mc::math::mk::pow_ns::sqrt<f64>(bx * bx + by * by + bz * bz) + 1e-30;
    const f64 phi = lcg_unit(seed) * 6.283185307179586;
    g_qb_d[i] = mc::math::quaternions::from_axis_angle<f64>(bx / bn, by / bn, bz / bn, phi);

    g_va_d[i] = vec3d{ ax, ay, az };
    g_angles_d[i] = (lcg_unit(seed) - 0.5) * 2.0;

    g_qa_f[i] = quatf{ static_cast<f32>(g_qa_d[i].x), static_cast<f32>(g_qa_d[i].y), static_cast<f32>(g_qa_d[i].z),
                       static_cast<f32>(g_qa_d[i].w) };
    g_qb_f[i] = quatf{ static_cast<f32>(g_qb_d[i].x), static_cast<f32>(g_qb_d[i].y), static_cast<f32>(g_qb_d[i].z),
                       static_cast<f32>(g_qb_d[i].w) };
    g_va_f[i] = vec3f{ static_cast<f32>(ax), static_cast<f32>(ay), static_cast<f32>(az) };
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
measure(const char *name, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) kernel();

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  const u64 total_ops = REPS_PER_MEAS * N;

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    quat_events evs{ bbench::quiet{} };
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
sweep_algebra()
{
  print_header("algebra (f64 / f32)");

  print_cell(measure("multiply f64       ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::multiply<f64>(g_qa_d[i], g_qb_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("multiply f32       ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_f[i] = mc::math::quaternions::multiply<f32>(g_qa_f[i], g_qb_f[i]);
    clobber(g_qout_f);
  }));
  print_cell(measure("conjugate f64      ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::conjugate<f64>(g_qa_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("inverse f64        ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::inverse<f64>(g_qa_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("inverse_unit f64   ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::inverse_unit<f64>(g_qa_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("normalize f64      ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::normalize<f64>(g_qa_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("norm f64           ", [] {
    static volatile f64 sink = 0;
    for ( u64 i = 0; i < N; ++i ) sink = mc::math::quaternions::norm<f64>(g_qa_d[i]);
    (void)sink;
  }));
  print_cell(measure("norm_sq f64        ", [] {
    static volatile f64 sink = 0;
    for ( u64 i = 0; i < N; ++i ) sink = mc::math::quaternions::norm_sq<f64>(g_qa_d[i]);
    (void)sink;
  }));
  print_cell(measure("batched_mul f64    ", [] {
    mc::math::quaternions::batched_multiply<f64>(g_qa_d, g_qb_d, g_qout_d, N);
    clobber(g_qout_d);
  }));
  print_cell(measure("batched_norm f64   ", [] {
    mc::math::quaternions::batched_normalize<f64>(g_qa_d, g_qout_d, N);
    clobber(g_qout_d);
  }));
}

void
sweep_rotations()
{
  print_header("rotations (f64 / f32)");

  print_cell(measure("from_axis_angle f64", [] {
    for ( u64 i = 0; i < N; ++i )
      g_qout_d[i] = mc::math::quaternions::from_axis_angle<f64>(g_va_d[i].x, g_va_d[i].y, g_va_d[i].z, g_angles_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("from_axis_angle f32", [] {
    for ( u64 i = 0; i < N; ++i )
      g_qout_f[i] = mc::math::quaternions::from_axis_angle<f32>(g_va_f[i].x, g_va_f[i].y, g_va_f[i].z, static_cast<f32>(g_angles_d[i]));
    clobber(g_qout_f);
  }));
  print_cell(measure("to_axis_angle f64  ", [] {
    for ( u64 i = 0; i < N; ++i ) {
      auto r = mc::math::quaternions::to_axis_angle<f64>(g_qa_d[i]);
      g_vout_d[i] = r.axis;
      g_angles_d[i] = r.angle;
    }
    clobber(g_vout_d);
  }));
  print_cell(measure("to_matrix f64      ", [] {
    for ( u64 i = 0; i < N; ++i ) g_mout_d[i] = mc::math::quaternions::to_matrix<f64>(g_qa_d[i]);
    clobber(g_mout_d);
  }));
  print_cell(measure("from_matrix f64    ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::from_matrix<f64>(g_mout_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("rotate vec f64     ", [] {
    for ( u64 i = 0; i < N; ++i ) g_vout_d[i] = mc::math::quaternions::rotate<f64>(g_qa_d[i], g_va_d[i]);
    clobber(g_vout_d);
  }));
  print_cell(measure("rotate vec f32     ", [] {
    for ( u64 i = 0; i < N; ++i ) g_vout_f[i] = mc::math::quaternions::rotate<f32>(g_qa_f[i], g_va_f[i]);
    clobber(g_vout_f);
  }));
}

template<mc::math::quaternions::euler_order Ord>
void
do_euler_pair(const char *name)
{
  cell c1 = measure(name, [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::from_euler<Ord, f64>(g_va_d[i].x, g_va_d[i].y, g_va_d[i].z);
    clobber(g_qout_d);
  });
  print_cell(c1);
}

template<mc::math::quaternions::euler_order Ord>
void
do_to_euler(const char *name)
{
  cell c = measure(name, [] {
    for ( u64 i = 0; i < N; ++i ) g_vout_d[i] = mc::math::quaternions::to_euler<Ord, f64>(g_qa_d[i]);
    clobber(g_vout_d);
  });
  print_cell(c);
}

void
sweep_euler()
{
  print_header("euler from/to (all 12 conventions, f64)");

  do_euler_pair<mc::math::quaternions::euler_order::XYZ>("from_euler<XYZ>    ");
  do_euler_pair<mc::math::quaternions::euler_order::XZY>("from_euler<XZY>    ");
  do_euler_pair<mc::math::quaternions::euler_order::YXZ>("from_euler<YXZ>    ");
  do_euler_pair<mc::math::quaternions::euler_order::YZX>("from_euler<YZX>    ");
  do_euler_pair<mc::math::quaternions::euler_order::ZXY>("from_euler<ZXY>    ");
  do_euler_pair<mc::math::quaternions::euler_order::ZYX>("from_euler<ZYX>    ");

  do_euler_pair<mc::math::quaternions::euler_order::XYX>("from_euler<XYX>    ");
  do_euler_pair<mc::math::quaternions::euler_order::XZX>("from_euler<XZX>    ");
  do_euler_pair<mc::math::quaternions::euler_order::YXY>("from_euler<YXY>    ");
  do_euler_pair<mc::math::quaternions::euler_order::YZY>("from_euler<YZY>    ");
  do_euler_pair<mc::math::quaternions::euler_order::ZXZ>("from_euler<ZXZ>    ");
  do_euler_pair<mc::math::quaternions::euler_order::ZYZ>("from_euler<ZYZ>    ");

  do_to_euler<mc::math::quaternions::euler_order::XYZ>("to_euler<XYZ>      ");
  do_to_euler<mc::math::quaternions::euler_order::XZY>("to_euler<XZY>      ");
  do_to_euler<mc::math::quaternions::euler_order::YXZ>("to_euler<YXZ>      ");
  do_to_euler<mc::math::quaternions::euler_order::YZX>("to_euler<YZX>      ");
  do_to_euler<mc::math::quaternions::euler_order::ZXY>("to_euler<ZXY>      ");
  do_to_euler<mc::math::quaternions::euler_order::ZYX>("to_euler<ZYX>      ");
  do_to_euler<mc::math::quaternions::euler_order::XYX>("to_euler<XYX>      ");
  do_to_euler<mc::math::quaternions::euler_order::XZX>("to_euler<XZX>      ");
  do_to_euler<mc::math::quaternions::euler_order::YXY>("to_euler<YXY>      ");
  do_to_euler<mc::math::quaternions::euler_order::YZY>("to_euler<YZY>      ");
  do_to_euler<mc::math::quaternions::euler_order::ZXZ>("to_euler<ZXZ>      ");
  do_to_euler<mc::math::quaternions::euler_order::ZYZ>("to_euler<ZYZ>      ");
}

void
sweep_interpolation()
{
  print_header("interpolation (f64)");

  print_cell(measure("lerp               ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::lerp<f64>(g_qa_d[i], g_qb_d[i], 0.5);
    clobber(g_qout_d);
  }));
  print_cell(measure("nlerp              ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::nlerp<f64>(g_qa_d[i], g_qb_d[i], 0.5);
    clobber(g_qout_d);
  }));
  print_cell(measure("slerp              ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::slerp<f64>(g_qa_d[i], g_qb_d[i], 0.5);
    clobber(g_qout_d);
  }));
}

void
sweep_kinematics()
{
  print_header("kinematics (f64) — omega = g_va_d, dt = 0.01");

  print_cell(measure("derivative         ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::derivative<f64>(g_qa_d[i], g_va_d[i]);
    clobber(g_qout_d);
  }));
  print_cell(measure("integrate          ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::integrate<f64>(g_qa_d[i], g_va_d[i], 0.01);
    clobber(g_qout_d);
  }));
  print_cell(measure("integrate_small    ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::integrate_small_angle<f64>(g_qa_d[i], g_va_d[i], 0.01);
    clobber(g_qout_d);
  }));
  print_cell(measure("integrate_pade     ", [] {
    for ( u64 i = 0; i < N; ++i ) g_qout_d[i] = mc::math::quaternions::integrate_pade<f64>(g_qa_d[i], g_va_d[i], 0.01);
    clobber(g_qout_d);
  }));
  print_cell(measure("angular_velocity   ", [] {
    for ( u64 i = 0; i < N; ++i ) g_vout_d[i] = mc::math::quaternions::angular_velocity<f64>(g_qa_d[i], g_qb_d[i], 0.01);
    clobber(g_vout_d);
  }));
  print_cell(measure("angular_velocity_p ", [] {
    for ( u64 i = 0; i < N; ++i ) g_vout_d[i] = mc::math::quaternions::angular_velocity_pade<f64>(g_qa_d[i], g_qb_d[i], 0.01);
    clobber(g_vout_d);
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

  micron::io::println("=== micron quaternion benchmark ===");
  micron::io::println("buffer: ", N, " quaternions (~8 KiB f64, ~4 KiB f32 — L1d-resident)");
  micron::io::println("warmup: ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " measurements/cell × ", REPS_PER_MEAS, " reps × ", N, " ops");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");

  sweep_algebra();
  sweep_rotations();
  sweep_euler();
  sweep_interpolation();
  sweep_kinematics();

  micron::io::println("");
  micron::io::println("=== done ===");
  return 0;
}
