//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// SO(3) / SE(3) and embedded-manifold throughput. L1-resident scalar, AoS,
// and SoA sweeps expose algebraic kernels, fused actions, maps, and SPD work.

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/linalg/ops.hpp"
#include "../src/math/manifolds/embedded/spd.hpp"
#include "../src/math/manifolds/embedded/sphere.hpp"
#include "../src/math/manifolds/lie/batch.hpp"
#include "../src/math/manifolds/lie/se3.hpp"
#include "../src/math/manifolds/lie/so3.hpp"
#include "../src/std.hpp"

namespace
{

using events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::level1d, bbench::level1d_miss>;

namespace lie = micron::math::manifolds::lie;
namespace lops = micron::math::linalg::ops;

template<typename F> using V3 = micron::math::vec<F, 3>;
template<typename F> using V6 = micron::math::vec<F, 6>;
template<typename F> using M3 = micron::math::mat<F, 3, 3>;
template<typename F> using M4 = micron::math::mat<F, 4, 4>;
template<typename F> using M6 = micron::math::mat<F, 6, 6>;
template<typename F> using SO3 = lie::SO3<F>;
template<typename F> using SE3 = lie::SE3<F>;
template<typename F> using SPD3 = micron::math::manifolds::spd<F, 3>;
template<typename F> using Sphere3 = micron::math::manifolds::sphere<F, 3>;

constexpr u64 N = 256;
constexpr u64 REPS = 256;
constexpr u32 SAMPLES = 7;
constexpr u64 EMBED_N = 32;

alignas(64) SO3<f32> g_ra_f[N];
alignas(64) SO3<f32> g_rb_f[N];
alignas(64) SO3<f32> g_ro_f[N];
alignas(64) SO3<f64> g_ra_d[N];
alignas(64) SO3<f64> g_rb_d[N];
alignas(64) SO3<f64> g_ro_d[N];

alignas(64) SE3<f32> g_ta_f[N];
alignas(64) SE3<f32> g_tb_f[N];
alignas(64) SE3<f32> g_to_f[N];
alignas(64) SE3<f64> g_ta_d[N];
alignas(64) SE3<f64> g_tb_d[N];
alignas(64) SE3<f64> g_to_d[N];

alignas(64) V3<f32> g_v_f[N];
alignas(64) V3<f32> g_vo_f[N];
alignas(64) V3<f64> g_v_d[N];
alignas(64) V3<f64> g_vo_d[N];
alignas(64) f32 g_vx_f[N], g_vy_f[N], g_vz_f[N];
alignas(64) f32 g_vox_f[N], g_voy_f[N], g_voz_f[N];
alignas(64) f64 g_vx_d[N], g_vy_d[N], g_vz_d[N];
alignas(64) f64 g_vox_d[N], g_voy_d[N], g_voz_d[N];
const lie::vec3_soa_const_view<f32> g_soa_in_f{ g_vx_f, g_vy_f, g_vz_f };
const lie::vec3_soa_view<f32> g_soa_out_f{ g_vox_f, g_voy_f, g_voz_f };
const lie::vec3_soa_const_view<f64> g_soa_in_d{ g_vx_d, g_vy_d, g_vz_d };
const lie::vec3_soa_view<f64> g_soa_out_d{ g_vox_d, g_voy_d, g_voz_d };
alignas(64) V6<f32> g_xi_f[N];
alignas(64) V6<f32> g_xio_f[N];
alignas(64) V6<f64> g_xi_d[N];
alignas(64) V6<f64> g_xio_d[N];
alignas(64) M3<f32> g_m3_f[N];
alignas(64) M3<f64> g_m3_d[N];
alignas(64) M4<f32> g_m4_f[N];
alignas(64) M4<f64> g_m4_d[N];
alignas(64) M6<f32> g_m6_f[N];
alignas(64) M6<f64> g_m6_d[N];
alignas(64) M3<f32> g_spd_p_f[EMBED_N];
alignas(64) M3<f32> g_spd_q_f[EMBED_N];
alignas(64) M3<f32> g_spd_v_f[EMBED_N];
alignas(64) M3<f32> g_spd_o_f[EMBED_N];
alignas(64) M3<f64> g_spd_p_d[EMBED_N];
alignas(64) M3<f64> g_spd_q_d[EMBED_N];
alignas(64) M3<f64> g_spd_v_d[EMBED_N];
alignas(64) M3<f64> g_spd_o_d[EMBED_N];

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
lcg_sym(u64 &s, f64 magnitude) noexcept
{
  return (lcg_unit(s) * 2.0 - 1.0) * magnitude;
}

template<typename F>
[[gnu::always_inline]] inline V3<F>
random_v3(u64 &seed, f64 magnitude) noexcept
{
  return V3<F>{ static_cast<F>(lcg_sym(seed, magnitude)), static_cast<F>(lcg_sym(seed, magnitude)),
                static_cast<F>(lcg_sym(seed, magnitude)) };
}

template<typename F>
[[gnu::always_inline]] inline V6<F>
random_v6(u64 &seed) noexcept
{
  return V6<F>{ static_cast<F>(lcg_sym(seed, 2.0)), static_cast<F>(lcg_sym(seed, 2.0)), static_cast<F>(lcg_sym(seed, 2.0)),
                static_cast<F>(lcg_sym(seed, 1.2)), static_cast<F>(lcg_sym(seed, 1.2)), static_cast<F>(lcg_sym(seed, 1.2)) };
}

[[gnu::cold]] void
fill_buffers(u64 seed) noexcept
{
  for ( u64 i = 0; i < N; ++i ) {
    const V6<f64> xa = random_v6<f64>(seed);
    const V6<f64> xb = random_v6<f64>(seed);
    V6<f32> xaf{}, xbf{};
    for ( usize k = 0; k < 6; ++k ) {
      xaf.data[k] = static_cast<f32>(xa.data[k]);
      xbf.data[k] = static_cast<f32>(xb.data[k]);
      g_xi_f[i].data[k] = xaf.data[k];
      g_xi_d[i].data[k] = xa.data[k];
    }
    g_ta_d[i] = SE3<f64>::exp_map(xa);
    g_tb_d[i] = SE3<f64>::exp_map(xb);
    g_ta_f[i] = SE3<f32>::exp_map(xaf);
    g_tb_f[i] = SE3<f32>::exp_map(xbf);
    g_ra_d[i] = g_ta_d[i].R;
    g_rb_d[i] = g_tb_d[i].R;
    g_ra_f[i] = g_ta_f[i].R;
    g_rb_f[i] = g_tb_f[i].R;
    g_v_d[i] = random_v3<f64>(seed, 3.0);
    g_v_f[i] = V3<f32>{ static_cast<f32>(g_v_d[i].data[0]), static_cast<f32>(g_v_d[i].data[1]), static_cast<f32>(g_v_d[i].data[2]) };
    g_vx_f[i] = g_v_f[i].data[0];
    g_vy_f[i] = g_v_f[i].data[1];
    g_vz_f[i] = g_v_f[i].data[2];
    g_vx_d[i] = g_v_d[i].data[0];
    g_vy_d[i] = g_v_d[i].data[1];
    g_vz_d[i] = g_v_d[i].data[2];
  }
  for ( u64 i = 0; i < EMBED_N; ++i ) {
    const f64 d0 = 1.0 + lcg_unit(seed), d1 = 1.5 + lcg_unit(seed), d2 = 2.0 + lcg_unit(seed);
    const f64 a = lcg_sym(seed, 0.05), b = lcg_sym(seed, 0.05), c = lcg_sym(seed, 0.05);
    g_spd_p_d[i] = M3<f64>{ d0, a, b, a, d1, c, b, c, d2 };
    g_spd_v_d[i]
        = M3<f64>{ lcg_sym(seed, 0.04), lcg_sym(seed, 0.02), lcg_sym(seed, 0.02), 0, lcg_sym(seed, 0.04), lcg_sym(seed, 0.02), 0, 0,
                   lcg_sym(seed, 0.04) };
    g_spd_v_d[i].data[3] = g_spd_v_d[i].data[1];
    g_spd_v_d[i].data[6] = g_spd_v_d[i].data[2];
    g_spd_v_d[i].data[7] = g_spd_v_d[i].data[5];
    for ( usize k = 0; k < 9; ++k ) {
      g_spd_p_f[i].data[k] = static_cast<f32>(g_spd_p_d[i].data[k]);
      g_spd_v_f[i].data[k] = static_cast<f32>(g_spd_v_d[i].data[k]);
    }
    g_spd_q_d[i] = SPD3<f64>::exp_map(g_spd_p_d[i], g_spd_v_d[i]);
    g_spd_q_f[i] = SPD3<f32>::exp_map(g_spd_p_f[i], g_spd_v_f[i]);
  }
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

f64
median(f64 *v, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 x = v[i];
    u32 j = i;
    while ( j != 0 && v[j - 1] > x ) {
      v[j] = v[j - 1];
      --j;
    }
    v[j] = x;
  }
  return v[n / 2];
}

struct cell {
  const char *name;
  f64 cycles;
  f64 instructions;
  f64 ipc;
  f64 l1d_loads;
  f64 l1d_miss_percent;
};

template<u64 Repetitions = REPS, u64 Operations = N, typename Kernel>
[[gnu::noinline]] cell
measure(const char *name, Kernel &&kernel) noexcept
{
  for ( u32 i = 0; i < 5; ++i ) kernel();
  f64 cyc[SAMPLES], ins[SAMPLES], ipc[SAMPLES], l1[SAMPLES], l1m[SAMPLES];
  constexpr u64 operations = Repetitions * Operations;
  for ( u32 sample = 0; sample < SAMPLES; ++sample ) {
    events ev{ bbench::quiet{} };
    ev.open();
    ev.begin();
    for ( u64 rep = 0; rep < Repetitions; ++rep ) kernel();
    ev.end();
    const u64 c = static_cast<u64>(ev.get<bbench::hardware_cycles>().retrieve());
    const u64 i = static_cast<u64>(ev.get<bbench::hardware_instructions>().retrieve());
    const u64 loads = static_cast<u64>(ev.get<bbench::level1d>().retrieve());
    const u64 misses = static_cast<u64>(ev.get<bbench::level1d_miss>().retrieve());
    cyc[sample] = static_cast<f64>(c) / static_cast<f64>(operations);
    ins[sample] = static_cast<f64>(i) / static_cast<f64>(operations);
    ipc[sample] = c != 0 ? static_cast<f64>(i) / static_cast<f64>(c) : 0.0;
    l1[sample] = static_cast<f64>(loads) / static_cast<f64>(operations);
    l1m[sample] = loads != 0 ? static_cast<f64>(misses) * 100.0 / static_cast<f64>(loads) : 0.0;
  }
  return cell{ name, median(cyc, SAMPLES), median(ins, SAMPLES), median(ipc, SAMPLES), median(l1, SAMPLES), median(l1m, SAMPLES) };
}

struct fixed2 {
  u64 whole;
  u32 fraction;
};

[[gnu::always_inline]] inline fixed2
fixed(f64 x) noexcept
{
  const u64 v = static_cast<u64>((x > 0.0 ? x : 0.0) * 100.0 + 0.5);
  return fixed2{ v / 100, static_cast<u32>(v % 100) };
}

struct line {
  char data[192];
  usize n{};

  void
  text(const char *s) noexcept
  {
    while ( *s != '\0' ) data[n++] = *s++;
  }

  void
  tab() noexcept
  {
    data[n++] = '\t';
  }

  void
  number(fixed2 x) noexcept
  {
    char digits[24];
    usize count = 0;
    do {
      digits[count++] = static_cast<char>('0' + x.whole % 10);
      x.whole /= 10;
    } while ( x.whole != 0 );
    while ( count != 0 ) data[n++] = digits[--count];
    data[n++] = '.';
    data[n++] = static_cast<char>('0' + x.fraction / 10);
    data[n++] = static_cast<char>('0' + x.fraction % 10);
  }

  [[nodiscard]] const char *
  c_str() noexcept
  {
    data[n] = '\0';
    return data;
  }
};

void
print(const cell &c) noexcept
{
  line out;
  out.text(c.name);
  out.tab();
  out.number(fixed(c.cycles));
  out.tab();
  out.number(fixed(c.instructions));
  out.tab();
  out.number(fixed(c.ipc));
  out.tab();
  out.number(fixed(c.l1d_loads));
  out.tab();
  out.number(fixed(c.l1d_miss_percent));
  micron::io::println(out.c_str());
}

void
header(const char *name) noexcept
{
  micron::io::println("");
  micron::io::println("[", name, "]");
  micron::io::println("operation\tcycles/op\tinst/op\tIPC\tL1D loads/op\tL1D miss%");
}

void
sweep_so3() noexcept
{
  header("SO3 algebraic kernels");
  print(measure("compose f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_ro_f[i] = SO3<f32>::compose(g_ra_f[i], g_rb_f[i]);
    clobber(g_ro_f);
  }));
  print(measure("compose f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_ro_d[i] = SO3<f64>::compose(g_ra_d[i], g_rb_d[i]);
    clobber(g_ro_d);
  }));
  print(measure("inverse f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_ro_f[i] = SO3<f32>::inverse(g_ra_f[i]);
    clobber(g_ro_f);
  }));
  print(measure("inverse f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_ro_d[i] = SO3<f64>::inverse(g_ra_d[i]);
    clobber(g_ro_d);
  }));
  print(measure("rotate f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_f[i] = SO3<f32>::rotate(g_ra_f[i], g_v_f[i]);
    clobber(g_vo_f);
  }));
  print(measure("rotate f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_d[i] = SO3<f64>::rotate(g_ra_d[i], g_v_d[i]);
    clobber(g_vo_d);
  }));
  print(measure("to_matrix f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_m3_f[i] = SO3<f32>::to_matrix(g_ra_f[i]);
    clobber(g_m3_f);
  }));
  print(measure("to_matrix f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_m3_d[i] = SO3<f64>::to_matrix(g_ra_d[i]);
    clobber(g_m3_d);
  }));

  header("SO3 maps");
  print(measure("exp_map f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_ro_f[i] = SO3<f32>::exp_map(g_v_f[i]);
    clobber(g_ro_f);
  }));
  print(measure("exp_map f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_ro_d[i] = SO3<f64>::exp_map(g_v_d[i]);
    clobber(g_ro_d);
  }));
  print(measure("log_map f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_f[i] = SO3<f32>::log_map(g_ra_f[i]);
    clobber(g_vo_f);
  }));
  print(measure("log_map f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_d[i] = SO3<f64>::log_map(g_ra_d[i]);
    clobber(g_vo_d);
  }));
}

void
sweep_se3() noexcept
{
  header("SE3 algebraic kernels");
  print(measure("compose f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_f[i] = SE3<f32>::compose(g_ta_f[i], g_tb_f[i]);
    clobber(g_to_f);
  }));
  print(measure("compose f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_d[i] = SE3<f64>::compose(g_ta_d[i], g_tb_d[i]);
    clobber(g_to_d);
  }));
  print(measure("inverse f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_f[i] = SE3<f32>::inverse(g_ta_f[i]);
    clobber(g_to_f);
  }));
  print(measure("inverse f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_d[i] = SE3<f64>::inverse(g_ta_d[i]);
    clobber(g_to_d);
  }));
  print(measure("to_matrix f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_m4_f[i] = SE3<f32>::to_matrix(g_ta_f[i]);
    clobber(g_m4_f);
  }));
  print(measure("to_matrix f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_m4_d[i] = SE3<f64>::to_matrix(g_ta_d[i]);
    clobber(g_m4_d);
  }));
  print(measure("adjoint f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_m6_f[i] = SE3<f32>::adjoint(g_ta_f[i]);
    clobber(g_m6_f);
  }));
  print(measure("adjoint f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_m6_d[i] = SE3<f64>::adjoint(g_ta_d[i]);
    clobber(g_m6_d);
  }));

  header("SE3 fused actions");
  print(measure("between legacy f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_f[i] = SE3<f32>::compose(SE3<f32>::inverse(g_ta_f[i]), g_tb_f[i]);
    clobber(g_to_f);
  }));
  print(measure("between f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_f[i] = SE3<f32>::between(g_ta_f[i], g_tb_f[i]);
    clobber(g_to_f);
  }));
  print(measure("between legacy f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_d[i] = SE3<f64>::compose(SE3<f64>::inverse(g_ta_d[i]), g_tb_d[i]);
    clobber(g_to_d);
  }));
  print(measure("between f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_d[i] = SE3<f64>::between(g_ta_d[i], g_tb_d[i]);
    clobber(g_to_d);
  }));
  print(measure("adjoint+gemv f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_xio_f[i] = lops::gemv(SE3<f32>::adjoint(g_ta_f[i]), g_xi_f[i]);
    clobber(g_xio_f);
  }));
  print(measure("adjoint_apply f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_xio_f[i] = SE3<f32>::adjoint_apply(g_ta_f[i], g_xi_f[i]);
    clobber(g_xio_f);
  }));
  print(measure("adjoint+gemv f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_xio_d[i] = lops::gemv(SE3<f64>::adjoint(g_ta_d[i]), g_xi_d[i]);
    clobber(g_xio_d);
  }));
  print(measure("adjoint_apply f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_xio_d[i] = SE3<f64>::adjoint_apply(g_ta_d[i], g_xi_d[i]);
    clobber(g_xio_d);
  }));

  header("SE3 maps");
  print(measure("exp_map f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_f[i] = SE3<f32>::exp_map(g_xi_f[i]);
    clobber(g_to_f);
  }));
  print(measure("exp_map f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_to_d[i] = SE3<f64>::exp_map(g_xi_d[i]);
    clobber(g_to_d);
  }));
  print(measure("log_map f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_xio_f[i] = SE3<f32>::log_map(g_ta_f[i]);
    clobber(g_xio_f);
  }));
  print(measure("log_map f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_xio_d[i] = SE3<f64>::log_map(g_ta_d[i]);
    clobber(g_xio_d);
  }));
}

void
sweep_batch() noexcept
{
  header("shared-frame point batches");
  print(measure("rotate loop f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_f[i] = SO3<f32>::rotate(g_ra_f[0], g_v_f[i]);
    clobber(g_vo_f);
  }));
  print(measure("rotate_many f32", [] {
    lie::rotate_many(g_ra_f[0], g_v_f, g_vo_f, N);
    clobber(g_vo_f);
  }));
  print(measure("rotate loop f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_d[i] = SO3<f64>::rotate(g_ra_d[0], g_v_d[i]);
    clobber(g_vo_d);
  }));
  print(measure("rotate_many f64", [] {
    lie::rotate_many(g_ra_d[0], g_v_d, g_vo_d, N);
    clobber(g_vo_d);
  }));
  print(measure("transform loop f32", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_f[i] = SO3<f32>::rotate(g_ta_f[0].R, g_v_f[i]) + g_ta_f[0].t;
    clobber(g_vo_f);
  }));
  print(measure("transform_many f32", [] {
    lie::transform_many(g_ta_f[0], g_v_f, g_vo_f, N);
    clobber(g_vo_f);
  }));
  print(measure("transform loop f64", [] {
    for ( u64 i = 0; i < N; ++i ) g_vo_d[i] = SO3<f64>::rotate(g_ta_d[0].R, g_v_d[i]) + g_ta_d[0].t;
    clobber(g_vo_d);
  }));
  print(measure("transform_many f64", [] {
    lie::transform_many(g_ta_d[0], g_v_d, g_vo_d, N);
    clobber(g_vo_d);
  }));

  header("shared-frame SoA point batches");
  print(measure("rotate SoA loop f32", [] {
    for ( u64 i = 0; i < N; ++i ) {
      const auto r = SO3<f32>::rotate(g_ra_f[0], V3<f32>{ g_vx_f[i], g_vy_f[i], g_vz_f[i] });
      g_vox_f[i] = r.data[0];
      g_voy_f[i] = r.data[1];
      g_voz_f[i] = r.data[2];
    }
    clobber(g_vox_f);
  }));
  print(measure("rotate_many_soa f32", [] {
    lie::rotate_many_soa(g_ra_f[0], g_soa_in_f, g_soa_out_f, N);
    clobber(g_vox_f);
  }));
  print(measure("rotate SoA loop f64", [] {
    for ( u64 i = 0; i < N; ++i ) {
      const auto r = SO3<f64>::rotate(g_ra_d[0], V3<f64>{ g_vx_d[i], g_vy_d[i], g_vz_d[i] });
      g_vox_d[i] = r.data[0];
      g_voy_d[i] = r.data[1];
      g_voz_d[i] = r.data[2];
    }
    clobber(g_vox_d);
  }));
  print(measure("rotate_many_soa f64", [] {
    lie::rotate_many_soa(g_ra_d[0], g_soa_in_d, g_soa_out_d, N);
    clobber(g_vox_d);
  }));
  print(measure("transform SoA loop f32", [] {
    for ( u64 i = 0; i < N; ++i ) {
      const auto r = SO3<f32>::rotate(g_ta_f[0].R, V3<f32>{ g_vx_f[i], g_vy_f[i], g_vz_f[i] }) + g_ta_f[0].t;
      g_vox_f[i] = r.data[0];
      g_voy_f[i] = r.data[1];
      g_voz_f[i] = r.data[2];
    }
    clobber(g_vox_f);
  }));
  print(measure("transform_many_soa f32", [] {
    lie::transform_many_soa(g_ta_f[0], g_soa_in_f, g_soa_out_f, N);
    clobber(g_vox_f);
  }));
  print(measure("transform SoA loop f64", [] {
    for ( u64 i = 0; i < N; ++i ) {
      const auto r = SO3<f64>::rotate(g_ta_d[0].R, V3<f64>{ g_vx_d[i], g_vy_d[i], g_vz_d[i] }) + g_ta_d[0].t;
      g_vox_d[i] = r.data[0];
      g_voy_d[i] = r.data[1];
      g_voz_d[i] = r.data[2];
    }
    clobber(g_vox_d);
  }));
  print(measure("transform_many_soa f64", [] {
    lie::transform_many_soa(g_ta_d[0], g_soa_in_d, g_soa_out_d, N);
    clobber(g_vox_d);
  }));
}

void
sweep_embedded() noexcept
{
  header("embedded manifold kernels");
  print(measure("sphere exp f32", [] {
    const V3<f32> p{ 1.0f, 0.0f, 0.0f };
    for ( u64 i = 0; i < N; ++i ) g_vo_f[i] = Sphere3<f32>::exp_map(p, V3<f32>{ 0.0f, g_v_f[i].data[1], g_v_f[i].data[2] });
    clobber(g_vo_f);
  }));
  print(measure("sphere exp f64", [] {
    const V3<f64> p{ 1.0, 0.0, 0.0 };
    for ( u64 i = 0; i < N; ++i ) g_vo_d[i] = Sphere3<f64>::exp_map(p, V3<f64>{ 0.0, g_v_d[i].data[1], g_v_d[i].data[2] });
    clobber(g_vo_d);
  }));
  print(measure<8, EMBED_N>("SPD exp f32", [] {
    for ( u64 i = 0; i < EMBED_N; ++i ) g_spd_o_f[i] = SPD3<f32>::exp_map(g_spd_p_f[i], g_spd_v_f[i]);
    clobber(g_spd_o_f);
  }));
  print(measure<8, EMBED_N>("SPD exp f64", [] {
    for ( u64 i = 0; i < EMBED_N; ++i ) g_spd_o_d[i] = SPD3<f64>::exp_map(g_spd_p_d[i], g_spd_v_d[i]);
    clobber(g_spd_o_d);
  }));
  print(measure<8, EMBED_N>("SPD log f32", [] {
    for ( u64 i = 0; i < EMBED_N; ++i ) g_spd_o_f[i] = SPD3<f32>::log_map(g_spd_p_f[i], g_spd_q_f[i]);
    clobber(g_spd_o_f);
  }));
  print(measure<8, EMBED_N>("SPD log f64", [] {
    for ( u64 i = 0; i < EMBED_N; ++i ) g_spd_o_d[i] = SPD3<f64>::log_map(g_spd_p_d[i], g_spd_q_d[i]);
    clobber(g_spd_o_d);
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
  fill_buffers(0x4D414E49464F4C44ULL);
  micron::io::println("=== micron manifold benchmark ===");
  micron::io::println("CPU-pinned, N=256 L1-resident rows, median of 7 perf-event samples");
  sweep_so3();
  sweep_se3();
  sweep_batch();
  sweep_embedded();
  return 0;
}
