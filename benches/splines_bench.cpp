//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Spline construction and evaluation census.  Build with:
//   duck build benches/splines_bench.cpp -i . --perf --fp --no-ssp --no-lto -o bin/splines-bench
// Run pinned:
//   taskset -c 2 bin/splines-bench/splines_bench

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/splines.hpp"
#include "../src/std.hpp"

#define F64_C(value) static_cast<f64>(value)

namespace
{

using namespace micron;
using namespace micron::math;
using namespace micron::math::splines;

using core_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;
using cache_events = bbench::event_group<bbench::level1d, bbench::level1d_miss, bbench::llcache_miss, bbench::dtlb_miss>;

constexpr usize knot_n = 257;
constexpr usize hot_n = 4096;
constexpr usize l1_n = 1024;
constexpr usize l2_n = 16384;
constexpr usize memory_n = 1u << 20;
constexpr u32 samples = 9;

alignas(64) f64 g_x[knot_n];
alignas(64) f64 g_y[knot_n];
alignas(64) f64 g_d1[knot_n];
alignas(64) f64 g_d2[knot_n];
alignas(64) vec<f64, 6> g_points[knot_n];
alignas(64) f64 g_random[memory_n];
alignas(64) f64 g_sorted_l1[l1_n];
alignas(64) f64 g_sorted_l2[l2_n];
alignas(64) f64 g_sorted[memory_n];
alignas(64) f64 g_fit_x[hot_n];
alignas(64) f64 g_output[memory_n];
alignas(64) f64 g_tensor_coords[hot_n * 3];
alignas(64) f64 g_tensor_output[hot_n];

constant_1d<f64> g_constant;
nearest_1d<f64> g_nearest;
linear_1d<f64> g_linear;
quadratic_spline_1d<f64> g_quadratic;
cubic_spline_1d<f64> g_cubic;
cubic_spline_1d<f64> g_pchip;
cubic_spline_1d<f64> g_akima;
cubic_spline_1d<f64> g_makima;
cubic_spline_1d<f64> g_cardinal;
cubic_spline_1d<f64> g_steffen;
quintic_spline_1d<f64> g_quintic;
piecewise_polynomial_1d<f64> g_power;
periodic_cubic_spline_1d<f64> g_periodic;
bspline<f64> g_bspline1;
bspline<f64> g_bspline2;
bspline<f64> g_bspline3;
bspline<f64> g_bspline5;
cubic_curve_nd<f64, 6> g_curve;
regular_cubic_curve_nd<f64, 6> g_regular_curve;
packed_cubic_curve_nd<f64, 6> g_packed_curve;
bspline_curve_nd<f64, 6> g_bspline_curve;
nurbs_curve_nd<f64, 6> g_nurbs_curve;
bezier_curve_nd<f64, 6> g_bezier;
rational_bezier_curve_nd<f64, 6> g_rational_bezier;
tensor_bspline<f64, f64, 2> g_surface;
tensor_nurbs<f64, f64, 2> g_rational_surface;
tensor_bspline<f64, f64, 3> g_volume;
vector<f64> g_lsq_knots;

static volatile f64 g_sink = 0;

[[gnu::always_inline]] inline u64
next_random(u64 &state) noexcept
{
  state = state * 6364136223846793005ULL + 1442695040888963407ULL;
  return state;
}

[[gnu::always_inline]] inline f64
unit_random(u64 &state) noexcept
{
  return static_cast<f64>(next_random(state) >> 11) * 0x1.0p-53;
}

[[gnu::always_inline]] inline void
clobber(const void *pointer) noexcept
{
  asm volatile("" : : "r"(pointer) : "memory");
}

[[gnu::always_inline]] inline void
consume(f64 value) noexcept
{
  g_sink = value;
  asm volatile("" : : "m"(g_sink) : "memory");
}

[[nodiscard]] f64
median(f64 *values) noexcept
{
  for ( u32 i = 1; i < samples; ++i ) {
    const f64 value = values[i];
    u32 j = i;
    while ( j != 0 && values[j - 1] > value ) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
  return values[samples / 2];
}

struct result {
  const char *name;
  f64 cycles;
  f64 instructions;
  f64 ipc;
  f64 branches;
  f64 branch_miss_percent;
  f64 l1_loads;
  f64 l1_miss_percent;
  f64 llc_misses;
  f64 dtlb_misses;
};

template<typename Kernel>
[[nodiscard, gnu::noinline]] result
measure(const char *name, usize operations, usize repetitions, Kernel &&kernel) noexcept
{
  for ( u32 i = 0; i < 3; ++i ) kernel();
  f64 cycles[samples]{}, instructions[samples]{}, ipc[samples]{}, branches[samples]{}, branch_misses[samples]{};
  f64 l1_loads[samples]{}, l1_misses[samples]{}, llc_misses[samples]{}, dtlb_misses[samples]{};
  const f64 denominator = f64(operations * repetitions);
  for ( u32 sample = 0; sample < samples; ++sample ) {
    core_events events{ bbench::quiet{} };
    events.open();
    events.begin();
    for ( usize repetition = 0; repetition < repetitions; ++repetition ) kernel();
    events.end();
    const u64 cycle_count = static_cast<u64>(events.get<bbench::hardware_cycles>().retrieve());
    const u64 instruction_count = static_cast<u64>(events.get<bbench::hardware_instructions>().retrieve());
    const u64 branch_count = static_cast<u64>(events.get<bbench::branches>().retrieve());
    const u64 miss_count = static_cast<u64>(events.get<bbench::branch_misses>().retrieve());
    cycles[sample] = f64(cycle_count) / denominator;
    instructions[sample] = f64(instruction_count) / denominator;
    ipc[sample] = cycle_count ? f64(instruction_count) / f64(cycle_count) : F64_C(0);
    branches[sample] = f64(branch_count) / denominator;
    branch_misses[sample] = branch_count ? f64(miss_count) * F64_C(100) / f64(branch_count) : F64_C(0);
  }
  for ( u32 sample = 0; sample < samples; ++sample ) {
    cache_events events{ bbench::quiet{} };
    events.open();
    events.begin();
    for ( usize repetition = 0; repetition < repetitions; ++repetition ) kernel();
    events.end();
    const u64 load_count = static_cast<u64>(events.get<bbench::level1d>().retrieve());
    const u64 miss_count = static_cast<u64>(events.get<bbench::level1d_miss>().retrieve());
    l1_loads[sample] = f64(load_count) / denominator;
    l1_misses[sample] = load_count ? f64(miss_count) * F64_C(100) / f64(load_count) : F64_C(0);
    llc_misses[sample] = f64(static_cast<u64>(events.get<bbench::llcache_miss>().retrieve())) / denominator;
    dtlb_misses[sample] = f64(static_cast<u64>(events.get<bbench::dtlb_miss>().retrieve())) / denominator;
  }
  return { name,
           median(cycles),
           median(instructions),
           median(ipc),
           median(branches),
           median(branch_misses),
           median(l1_loads),
           median(l1_misses),
           median(llc_misses),
           median(dtlb_misses) };
}

struct decimal3 {
  u64 whole;
  u32 fraction;
};

[[nodiscard, gnu::always_inline]] inline decimal3
fixed3(f64 value) noexcept
{
  const u64 scaled = static_cast<u64>((value > F64_C(0) ? value : F64_C(0)) * F64_C(1000) + F64_C(0.5));
  return { scaled / 1000, static_cast<u32>(scaled % 1000) };
}

struct line {
  char data[384];
  usize size{ 0 };

  void
  text(const char *value) noexcept
  {
    while ( *value ) data[size++] = *value++;
  }

  void
  comma() noexcept
  {
    data[size++] = ',';
  }

  void
  number(decimal3 value) noexcept
  {
    char digits[24];
    usize count = 0;
    do {
      digits[count++] = static_cast<char>('0' + value.whole % 10);
      value.whole /= 10;
    } while ( value.whole );
    while ( count ) data[size++] = digits[--count];
    data[size++] = '.';
    data[size++] = static_cast<char>('0' + value.fraction / 100);
    data[size++] = static_cast<char>('0' + (value.fraction / 10) % 10);
    data[size++] = static_cast<char>('0' + value.fraction % 10);
  }

  [[nodiscard]] const char *
  c_str() noexcept
  {
    data[size] = '\0';
    return data;
  }
};

void
print(const result &value) noexcept
{
  line out;
  out.text(value.name);
  const f64 fields[] = { value.cycles,   value.instructions,    value.ipc,        value.branches,   value.branch_miss_percent,
                         value.l1_loads, value.l1_miss_percent, value.llc_misses, value.dtlb_misses };
  for ( f64 field : fields ) {
    out.comma();
    out.number(fixed3(field));
  }
  io::println(out.c_str());
}

void
header(const char *name) noexcept
{
  io::println("");
  io::println("[", name, "]");
  io::println("operation,cycles/op,instructions/op,IPC,branches/op,branch_miss_pct,L1D_loads/op,L1D_miss_pct,LLC_misses/op,dTLB_misses/op");
}

template<typename S>
void
scalar_cell(const char *name, const S &spline) noexcept
{
  print(measure(name, hot_n, 128, [&] {
    spline_cursor cursor{};
    f64 sum = F64_C(0);
    for ( usize i = 0; i < hot_n; ++i ) sum += evaluate(spline, g_random[i], cursor);
    consume(sum);
  }));
}

template<typename C>
void
curve_cell(const char *name, const C &curve) noexcept
{
  print(measure(name, hot_n, 64, [&] {
    f64 sum = F64_C(0);
    for ( usize i = 0; i < hot_n; ++i ) sum += evaluate(curve, g_random[i])[0];
    consume(sum);
  }));
}

template<usize D>
[[nodiscard, gnu::noinline]] vector<vec<f64, D>>
legacy_uniform_arc_samples(const cubic_curve_nd<f64, D> &curve, usize count, usize intervals) noexcept
{
  vector<vec<f64, D>> out;
  if ( count == 0 || curve.ts.size() < 2 ) return out;
  out.reserve(count);
  const f64 lo = curve.ts[0];
  const f64 hi = curve.ts[curve.ts.size() - 1];
  const f64 total = arc_length<f64, D>(curve, lo, hi, intervals);
  for ( usize i = 0; i < count; ++i ) {
    const f64 target = count == 1 ? F64_C(0) : total * f64(i) / f64(count - 1);
    f64 left = lo, right = hi;
    for ( u32 iteration = 0; iteration < 40; ++iteration ) {
      const f64 middle = F64_C(0.5) * (left + right);
      if ( arc_length<f64, D>(curve, lo, middle, intervals) < target )
        left = middle;
      else
        right = middle;
    }
    out.emplace_back(evaluate<f64, D>(curve, F64_C(0.5) * (left + right)));
  }
  return out;
}

template<usize D>
[[nodiscard, gnu::noinline]] vec<f64, D>
legacy_materialized_curve_derivative(const bspline_curve_nd<f64, D> &curve, f64 t) noexcept
{
  const auto materialized = derivative_spline(curve);
  return evaluate(materialized, t);
}

void
prepare() noexcept
{
  constexpr f64 tau = F64_C(6.283185307179586476925286766559);
  for ( usize i = 0; i < knot_n; ++i ) {
    g_x[i] = f64(i) / f64(knot_n - 1);
    const f64 angle = tau * g_x[i];
    g_y[i] = mk::trig::sin<f64>(angle);
    g_d1[i] = tau * mk::trig::cos<f64>(angle);
    g_d2[i] = -tau * tau * g_y[i];
    for ( usize d = 0; d < 6; ++d ) g_points[i][d] = f64(d + 1) * F64_C(0.1) * g_x[i] + mk::trig::sin<f64>(angle + f64(d) * F64_C(0.17));
  }
  u64 state = 0x53504C494E45534FULL;
  for ( usize i = 0; i < memory_n; ++i ) {
    g_random[i] = unit_random(state);
    g_sorted[i] = (f64(i) + F64_C(0.5)) / f64(memory_n);
  }
  for ( usize i = 0; i < l1_n; ++i ) g_sorted_l1[i] = (f64(i) + F64_C(0.5)) / f64(l1_n);
  for ( usize i = 0; i < l2_n; ++i ) g_sorted_l2[i] = (f64(i) + F64_C(0.5)) / f64(l2_n);
  for ( usize i = 0; i < hot_n; ++i ) g_fit_x[i] = (f64(i) + F64_C(0.5)) / f64(hot_n);
  for ( usize i = 0; i < hot_n; ++i ) {
    g_tensor_coords[i * 3] = unit_random(state);
    g_tensor_coords[i * 3 + 1] = unit_random(state);
    g_tensor_coords[i * 3 + 2] = unit_random(state);
  }

  const raw_slice<const f64> x{ g_x, knot_n };
  const raw_slice<const f64> y{ g_y, knot_n };
  g_constant = make_constant<f64>(x, y);
  g_nearest = make_nearest<f64>(x, y);
  g_linear = make_linear<f64>(x, y);
  g_quadratic = make_quadratic<f64>(x, y, quadratic_boundary::left_slope, g_d1[0]);
  g_cubic = make_cubic<f64>(x, y, bc_kind::not_a_knot);
  g_pchip = make_pchip<f64>(x, y);
  g_akima = make_akima<f64>(x, y);
  g_makima = make_akima<f64>(x, y, akima_kind::makima);
  g_cardinal = make_cardinal<f64>(x, y, F64_C(0.1));
  g_steffen = make_steffen<f64>(x, y);
  g_quintic = make_quintic_hermite<f64>(x, y, { g_d1, knot_n }, { g_d2, knot_n });
  g_power = to_power_basis<f64>(g_cubic);
  g_periodic = make_periodic_cubic<f64>(x, y);

  vector<f64> controls;
  controls.reserve(128);
  for ( usize i = 0; i < 128; ++i ) controls.emplace_back(g_y[i * 2]);
  auto knots1 = make_uniform_clamped_knots<f64>(controls.size(), 1, F64_C(0), F64_C(1));
  auto knots2 = make_uniform_clamped_knots<f64>(controls.size(), 2, F64_C(0), F64_C(1));
  auto knots3 = make_uniform_clamped_knots<f64>(controls.size(), 3, F64_C(0), F64_C(1));
  auto knots5 = make_uniform_clamped_knots<f64>(controls.size(), 5, F64_C(0), F64_C(1));
  g_bspline1 = make_bspline_from_ctrl<f64>({ knots1.data(), knots1.size() }, { controls.data(), controls.size() }, 1);
  g_bspline2 = make_bspline_from_ctrl<f64>({ knots2.data(), knots2.size() }, { controls.data(), controls.size() }, 2);
  g_bspline3 = make_bspline_from_ctrl<f64>({ knots3.data(), knots3.size() }, { controls.data(), controls.size() }, 3);
  g_bspline5 = make_bspline_from_ctrl<f64>({ knots5.data(), knots5.size() }, { controls.data(), controls.size() }, 5);

  g_curve = make_cubic_curve<f64, 6>(x, g_points, knot_n, bc_kind::not_a_knot);
  g_regular_curve = make_regular_cubic_curve<f64, 6>(F64_C(0), F64_C(1) / f64(knot_n - 1), g_points, knot_n, bc_kind::not_a_knot);
  g_packed_curve = pack<f64, 6>(g_curve);
  vec<f64, 6> curve_controls[128];
  f64 curve_weights[128];
  for ( usize i = 0; i < 128; ++i ) {
    curve_controls[i] = g_points[i * 2];
    curve_weights[i] = F64_C(0.8) + F64_C(0.4) * f64((i * 17) & 31) / F64_C(31);
  }
  g_bspline_curve = make_bspline_curve_from_ctrl<f64, 6>({ knots3.data(), knots3.size() }, curve_controls, 128, 3);
  g_nurbs_curve = make_nurbs_curve<f64, 6>({ knots3.data(), knots3.size() }, curve_controls, { curve_weights, 128 }, 128, 3);
  g_bezier = make_bezier_curve<f64, 6>(curve_controls, 8);
  g_rational_bezier = make_rational_bezier_curve<f64, 6>(curve_controls, { curve_weights, 8 }, 8);

  auto surface_knots0 = make_uniform_clamped_knots<f64>(12, 3, F64_C(0), F64_C(1));
  auto surface_knots1 = make_uniform_clamped_knots<f64>(12, 3, F64_C(0), F64_C(1));
  raw_slice<const f64> surface_knots[2]
      = { { surface_knots0.data(), surface_knots0.size() }, { surface_knots1.data(), surface_knots1.size() } };
  usize surface_shape[2] = { 12, 12 };
  u32 surface_degree[2] = { 3, 3 };
  f64 surface_control[144], surface_weights[144];
  for ( usize i = 0; i < 12; ++i )
    for ( usize j = 0; j < 12; ++j ) {
      const usize index = i * 12 + j;
      surface_control[index] = mk::trig::sin<f64>(f64(i) * F64_C(0.21)) + mk::trig::cos<f64>(f64(j) * F64_C(0.17));
      surface_weights[index] = F64_C(0.9) + F64_C(0.2) * f64((index * 13) & 15) / F64_C(15);
    }
  g_surface = make_tensor_bspline<f64, f64, 2>(surface_knots, surface_shape, surface_degree, { surface_control, 144 });
  g_rational_surface
      = make_tensor_nurbs<f64, f64, 2>(surface_knots, surface_shape, surface_degree, { surface_control, 144 }, { surface_weights, 144 });

  auto volume_knots0 = make_uniform_clamped_knots<f64>(8, 3, F64_C(0), F64_C(1));
  auto volume_knots1 = make_uniform_clamped_knots<f64>(8, 3, F64_C(0), F64_C(1));
  auto volume_knots2 = make_uniform_clamped_knots<f64>(8, 3, F64_C(0), F64_C(1));
  raw_slice<const f64> volume_knots[3] = { { volume_knots0.data(), volume_knots0.size() },
                                           { volume_knots1.data(), volume_knots1.size() },
                                           { volume_knots2.data(), volume_knots2.size() } };
  usize volume_shape[3] = { 8, 8, 8 };
  u32 volume_degree[3] = { 3, 3, 3 };
  f64 volume_control[512];
  for ( usize i = 0; i < 512; ++i ) volume_control[i] = mk::trig::sin<f64>(f64(i) * F64_C(0.013));
  g_volume = make_tensor_bspline<f64, f64, 3>(volume_knots, volume_shape, volume_degree, { volume_control, 512 });
  g_lsq_knots = make_uniform_clamped_knots<f64>(64, 3, F64_C(0), F64_C(1));
}

void
scalar_sweep() noexcept
{
  header("scalar random query, 257 knots, L1/L2 resident");
  scalar_cell("constant nearest", g_constant);
  scalar_cell("nearest", g_nearest);
  scalar_cell("linear", g_linear);
  scalar_cell("quadratic", g_quadratic);
  scalar_cell("cubic not-a-knot", g_cubic);
  scalar_cell("PCHIP", g_pchip);
  scalar_cell("Akima", g_akima);
  scalar_cell("Makima", g_makima);
  scalar_cell("cardinal", g_cardinal);
  scalar_cell("Steffen", g_steffen);
  scalar_cell("quintic Hermite", g_quintic);
  scalar_cell("piecewise power", g_power);
  scalar_cell("periodic cubic", g_periodic);
  scalar_cell("B-spline degree 1", g_bspline1);
  scalar_cell("B-spline degree 2", g_bspline2);
  scalar_cell("B-spline degree 3", g_bspline3);
  scalar_cell("B-spline degree 5", g_bspline5);
}

void
legacy_cubic_sorted(const f64 *query, f64 *output, usize count) noexcept
{
  usize segment = 0;
  for ( usize i = 0; i < count; ++i ) {
    while ( segment + 1 < g_cubic.xs.size() - 1 && query[i] > g_cubic.xs[segment + 1] ) ++segment;
    output[i] = __impl_splines_bits::eval_cubic_local<f64>(g_cubic.seg[segment], query[i] - g_cubic.xs[segment]);
  }
}

void
batch_size(const char *prefix, usize count) noexcept
{
  const f64 *query = count == l1_n ? g_sorted_l1 : (count == l2_n ? g_sorted_l2 : g_sorted);
  usize repetitions = (4u << 20) / count;
  if ( repetitions == 0 ) repetitions = 1;
  char legacy_name[64], batch_name[64], stream_name[64], linear_name[64];
  auto compose = [&](char *out, const char *suffix) {
    usize position = 0;
    for ( const char *p = prefix; *p; ++p ) out[position++] = *p;
    out[position++] = ' ';
    while ( *suffix ) out[position++] = *suffix++;
    out[position] = '\0';
  };
  compose(legacy_name, "cubic legacy scalar");
  compose(batch_name, "cubic SIMD batch");
  compose(stream_name, "cubic streaming stores");
  compose(linear_name, "linear SIMD batch");
  print(measure(legacy_name, count, repetitions, [&] {
    legacy_cubic_sorted(query, g_output, count);
    clobber(g_output);
  }));
  print(measure(batch_name, count, repetitions, [&] {
    evaluate<f64>(g_cubic, query, g_output, count);
    clobber(g_output);
  }));
  print(measure(stream_name, count, repetitions, [&] {
    evaluate_streaming<f64>(g_cubic, query, g_output, count);
    clobber(g_output);
  }));
  print(measure(linear_name, count, repetitions, [&] {
    evaluate<f64>(g_linear, query, g_output, count);
    clobber(g_output);
  }));
}

void
batch_sweep() noexcept
{
  header("sorted batch by cache hierarchy");
  batch_size("L1 1K", l1_n);
  batch_size("L2 16K", l2_n);
  batch_size("memory 1M", memory_n);
}

void
curve_sweep() noexcept
{
  header("curves and tensor products");
  curve_cell("cubic curve D=6", g_curve);
  curve_cell("regular cubic curve D=6", g_regular_curve);
  curve_cell("packed cubic curve D=6", g_packed_curve);
  curve_cell("B-spline curve D=6", g_bspline_curve);
  curve_cell("NURBS curve D=6", g_nurbs_curve);
  curve_cell("Bezier degree 7 D=6", g_bezier);
  curve_cell("rational Bezier degree 7 D=6", g_rational_bezier);
  print(measure("tensor B-spline surface 4x4", hot_n, 32, [&] {
    evaluate<f64, f64, 2>(g_surface, g_tensor_coords, g_tensor_output, hot_n);
    clobber(g_tensor_output);
  }));
  print(measure("tensor NURBS surface 4x4", hot_n, 32, [&] {
    evaluate<f64, f64, 2>(g_rational_surface, g_tensor_coords, g_tensor_output, hot_n);
    clobber(g_tensor_output);
  }));
  print(measure("tensor B-spline volume 4x4x4", hot_n, 16, [&] {
    evaluate<f64, f64, 3>(g_volume, g_tensor_coords, g_tensor_output, hot_n);
    clobber(g_tensor_output);
  }));
}

void
construction_sweep() noexcept
{
  header("construction, normalized per input point");
  const raw_slice<const f64> x{ g_x, knot_n };
  const raw_slice<const f64> y{ g_y, knot_n };
  print(measure("cubic construction", knot_n, 64, [&] {
    auto value = make_cubic<f64>(x, y, bc_kind::not_a_knot);
    consume(f64(value.seg.size()));
  }));
  print(measure("cubic curve D=6 shared solve", knot_n, 32, [&] {
    auto value = make_cubic_curve<f64, 6>(x, g_points, knot_n, bc_kind::not_a_knot);
    consume(f64(value.seg.size()));
  }));
  print(measure("B-spline interpolation", knot_n, 32, [&] {
    auto value = make_bspline_interpolating<f64>(x, y, 3);
    consume(f64(value.ctrl.size()));
  }));
  print(measure("B-spline curve D=6 shared LU", knot_n, 16, [&] {
    auto value = make_bspline_curve_interpolating<f64, 6>(x, g_points, knot_n, 3);
    consume(f64(value.ctrl.size()));
  }));
  print(measure("smoothing lambda=.02", knot_n, 32, [&] {
    auto value = make_smoothing<f64>(x, y, {}, F64_C(0.02));
    consume(f64(value.seg.size()));
  }));
  print(measure("smoothing curve D=6 shared factors", knot_n, 16, [&] {
    auto value = make_smoothing_curve<f64, 6>(x, g_points, knot_n, {}, F64_C(0.02));
    consume(f64(value.seg.size()));
  }));
  print(measure("LSQ B-spline 4K->64", hot_n, 16, [&] {
    auto value = make_lsq_bspline<f64>({ g_fit_x, hot_n }, { g_random, hot_n }, { g_lsq_knots.data(), g_lsq_knots.size() }, 3);
    consume(f64(value.ctrl.size()));
  }));
  print(measure("knot insertion degree 3", 128, 128, [&] {
    auto value = insert_knot<f64>(g_bspline3, F64_C(0.4567));
    consume(f64(value.ctrl.size()));
  }));
}

void
helper_sweep() noexcept
{
  constexpr usize arc_samples = 65;
  constexpr usize arc_intervals = 64;
  constexpr usize derivative_queries = 64;
  header("helper algorithms, normalized per output");
  print(measure("arc sample legacy reintegration", arc_samples, 1, [&] {
    auto value = legacy_uniform_arc_samples<6>(g_curve, arc_samples, arc_intervals);
    consume(value[value.size() - 1][0]);
  }));
  print(measure("arc sample cumulative table", arc_samples, 64, [&] {
    auto value = sample_uniform_arc_length<f64, 6>(g_curve, arc_samples, arc_intervals);
    consume(value[value.size() - 1][0]);
  }));
  print(measure("B-curve derivative legacy materialize", derivative_queries, 1, [&] {
    f64 sum = F64_C(0);
    for ( usize i = 0; i < derivative_queries; ++i ) sum += legacy_materialized_curve_derivative<6>(g_bspline_curve, g_random[i])[0];
    consume(sum);
  }));
  print(measure("B-curve derivative direct basis", derivative_queries, 128, [&] {
    f64 sum = F64_C(0);
    for ( usize i = 0; i < derivative_queries; ++i ) sum += derivative(g_bspline_curve, g_random[i])[0];
    consume(sum);
  }));
}

[[gnu::noinline]] void
perf_random_cubic() noexcept
{
  spline_cursor cursor{};
  f64 sum = F64_C(0);
  for ( usize repetition = 0; repetition < 8192; ++repetition )
    for ( usize i = 0; i < hot_n; ++i ) sum += evaluate(g_cubic, g_random[i], cursor);
  consume(sum);
}

[[gnu::noinline]] void
perf_random_bspline() noexcept
{
  spline_cursor cursor{};
  f64 sum = F64_C(0);
  for ( usize repetition = 0; repetition < 4096; ++repetition )
    for ( usize i = 0; i < hot_n; ++i ) sum += evaluate(g_bspline3, g_random[i], cursor);
  consume(sum);
}

[[gnu::noinline]] void
perf_curve() noexcept
{
  f64 sum = F64_C(0);
  for ( usize repetition = 0; repetition < 4096; ++repetition )
    for ( usize i = 0; i < hot_n; ++i ) sum += evaluate(g_bspline_curve, g_random[i])[0];
  consume(sum);
}

[[gnu::noinline]] void
perf_tensor() noexcept
{
  for ( usize repetition = 0; repetition < 2048; ++repetition ) {
    evaluate<f64, f64, 2>(g_surface, g_tensor_coords, g_tensor_output, hot_n);
    clobber(g_tensor_output);
  }
}

[[gnu::noinline]] void
perf_batch(bool streaming) noexcept
{
  for ( usize repetition = 0; repetition < 128; ++repetition ) {
    if ( streaming )
      evaluate_streaming<f64>(g_cubic, g_sorted, g_output, memory_n);
    else
      evaluate<f64>(g_cubic, g_sorted, g_output, memory_n);
    clobber(g_output);
  }
}

[[gnu::noinline]] void
perf_lsq() noexcept
{
  for ( usize repetition = 0; repetition < 512; ++repetition ) {
    auto value = make_lsq_bspline<f64>({ g_fit_x, hot_n }, { g_random, hot_n }, { g_lsq_knots.data(), g_lsq_knots.size() }, 3);
    consume(f64(value.ctrl.size()));
  }
}

[[nodiscard]] bool
perf_mode(char mode) noexcept
{
  switch ( mode ) {
  case 'r':
    perf_random_cubic();
    return true;
  case 'b':
    perf_random_bspline();
    return true;
  case 'c':
    perf_curve();
    return true;
  case 't':
    perf_tensor();
    return true;
  case 's':
    perf_batch(false);
    return true;
  case 'n':
    perf_batch(true);
    return true;
  case 'q':
    perf_lsq();
    return true;
  default:
    return false;
  }
}

};      // namespace

#undef F64_C

int
main(int argc, char **argv)
{
  micron::posix::cpu_set_t affinity;
  affinity.cpu_zero();
  affinity.cpu_set(2);
  micron::posix::sched_setaffinity(0, sizeof(affinity), affinity);
  prepare();
  if ( argc > 1 && perf_mode(argv[1][0]) ) return 0;
  micron::io::println("=== micron spline microbenchmark ===");
  micron::io::println("CPU 2 pinned; median of 9 core-event and 9 cache-event samples");
  scalar_sweep();
  batch_sweep();
  curve_sweep();
  construction_sweep();
  helper_sweep();
  consume(g_sink);
  return 0;
}
