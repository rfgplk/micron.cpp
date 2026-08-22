//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Żmij vs legacy Ryu floating-point formatting.
//
// build: duck build benches/float_format_bench.cpp --x86 --isa v3 --perf --fp --no-ssp --no-lto -o bin/float-format -f
// run:   taskset -c 2 bin/float-format/float_format_bench
// perf:  taskset -c 2 perf stat -r 7 -e cycles,instructions,branches,branch-misses
//          bin/float-format/float_format_bench --case f64-shortest-hot --backend zmij --perf-loop
// batch: bin/float-format/float_format_bench --case batch-f64-hot

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/string/conversions/chars.hpp"
#include "../src/string/conversions/floating_point.hpp"

namespace
{

constexpr usize item_count = 4096;
constexpr usize hot_count = 512;
constexpr u32 measurements = 7;
constexpr u32 warmups = 2;

f64 doubles_hot[item_count];
f64 doubles_uniform[item_count];
f32 floats_hot[item_count];
f32 floats_uniform[item_count];
volatile u64 sink = 0;

enum class case_t : u8 {
  all,
  f64_shortest_hot,
  f32_shortest_hot,
  f64_shortest_stream,
  f32_shortest_stream,
  batch_f64_hot,
  batch_f32_hot,
  fixed_hot,
  scientific_hot,
  general_hot,
  fixed_extreme
};
enum class backend_t : u8 { both, zmij, ryu };

struct config {
  case_t selected_case = case_t::all;
  backend_t backend = backend_t::both;
  bool perf_loop = false;
};

inline bool
same(const char *a, const char *b) noexcept
{
  usize i = 0;
  for ( ; a[i] != 0 && b[i] != 0; ++i )
    if ( a[i] != b[i] ) return false;
  return a[i] == b[i];
}

config
parse_args(int argc, char **argv) noexcept
{
  config c{};
  for ( int i = 1; i < argc; ++i ) {
    if ( same(argv[i], "--perf-loop") ) {
      c.perf_loop = true;
      continue;
    }
    if ( same(argv[i], "--case") && i + 1 < argc ) {
      const char *v = argv[++i];
      if ( same(v, "f64-shortest-hot") )
        c.selected_case = case_t::f64_shortest_hot;
      else if ( same(v, "f32-shortest-hot") )
        c.selected_case = case_t::f32_shortest_hot;
      else if ( same(v, "f64-shortest-stream") )
        c.selected_case = case_t::f64_shortest_stream;
      else if ( same(v, "f32-shortest-stream") )
        c.selected_case = case_t::f32_shortest_stream;
      else if ( same(v, "batch-f64-hot") )
        c.selected_case = case_t::batch_f64_hot;
      else if ( same(v, "batch-f32-hot") )
        c.selected_case = case_t::batch_f32_hot;
      else if ( same(v, "fixed-hot") )
        c.selected_case = case_t::fixed_hot;
      else if ( same(v, "scientific-hot") )
        c.selected_case = case_t::scientific_hot;
      else if ( same(v, "general-hot") )
        c.selected_case = case_t::general_hot;
      else if ( same(v, "fixed-extreme") )
        c.selected_case = case_t::fixed_extreme;
      continue;
    }
    if ( same(argv[i], "--backend") && i + 1 < argc ) {
      const char *v = argv[++i];
      if ( same(v, "zmij") )
        c.backend = backend_t::zmij;
      else if ( same(v, "ryu") )
        c.backend = backend_t::ryu;
    }
  }
  return c;
}

u64
now_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ull + static_cast<u64>(ts.tv_nsec);
}

u64
next(u64 &state) noexcept
{
  state ^= state >> 12;
  state ^= state << 25;
  state ^= state >> 27;
  return state * 0x2545f4914f6cdd1dull;
}

void
build_corpus() noexcept
{
  u64 state = 0x8c37d42a69f105beull;
  for ( usize i = 0; i < item_count; ++i ) {
    u64 dbits = next(state);
    if ( ((dbits >> 52) & 0x7ffu) == 0x7ffu ) dbits ^= 1ull << 52;
    doubles_uniform[i] = micron::math::ieee::from_bits<f64>(dbits);

    const u64 dexp = 990u + next(state) % 68u;
    const u64 dhot = (dbits & (1ull << 63 | ((1ull << 52) - 1))) | dexp << 52;
    doubles_hot[i] = micron::math::ieee::from_bits<f64>(dhot);

    u32 fbits = static_cast<u32>(next(state));
    if ( ((fbits >> 23) & 0xffu) == 0xffu ) fbits ^= 1u << 23;
    floats_uniform[i] = micron::math::ieee::from_bits<f32>(fbits);

    const u32 fexp = 95u + static_cast<u32>(next(state) % 65u);
    const u32 fhot = (fbits & (1u << 31 | ((1u << 23) - 1))) | fexp << 23;
    floats_hot[i] = micron::math::ieee::from_bits<f32>(fhot);
  }
}

f64
median(f64 (&samples)[measurements]) noexcept
{
  for ( u32 i = 1; i < measurements; ++i ) {
    const f64 value = samples[i];
    u32 j = i;
    while ( j > 0 && samples[j - 1] > value ) {
      samples[j] = samples[j - 1];
      --j;
    }
    samples[j] = value;
  }
  return samples[measurements / 2];
}

inline u64
observe(char *out, usize n) noexcept
{
  asm volatile("" : : "r"(out), "r"(n) : "memory");
  return static_cast<u64>(n) + static_cast<u8>(out[0]) + static_cast<u8>(out[n - 1]);
}

[[gnu::noinline]] u64
batch4_f64(usize group, char *out) noexcept
{
  const f64(&values)[4] = *reinterpret_cast<const f64(*)[4]>(doubles_hot + group * 4);
  const micron::chars4_result lengths = micron::to_chars4(out, micron::f64_shortest_chars_capacity, values);
  u64 result = 0;
  for ( usize lane = 0; lane < 4; ++lane ) result += observe(out + lane * micron::f64_shortest_chars_capacity, lengths[lane]);
  return result;
}

[[gnu::noinline]] u64
scalar4_f64(usize group, char *out) noexcept
{
  u64 result = 0;
  for ( usize lane = 0; lane < 4; ++lane ) {
    const usize n = micron::__impl::__zmij::d2s_buffered(doubles_hot[group * 4 + lane], out + lane * micron::f64_shortest_chars_capacity);
    result += observe(out + lane * micron::f64_shortest_chars_capacity, n);
  }
  return result;
}

[[gnu::noinline]] u64
batch4_f32(usize group, char *out) noexcept
{
  const f32(&values)[4] = *reinterpret_cast<const f32(*)[4]>(floats_hot + group * 4);
  const micron::chars4_result lengths = micron::to_chars4(out, micron::f32_shortest_chars_capacity, values);
  u64 result = 0;
  for ( usize lane = 0; lane < 4; ++lane ) result += observe(out + lane * micron::f32_shortest_chars_capacity, lengths[lane]);
  return result;
}

[[gnu::noinline]] u64
scalar4_f32(usize group, char *out) noexcept
{
  u64 result = 0;
  for ( usize lane = 0; lane < 4; ++lane ) {
    const usize n = micron::__impl::__zmij::f2s_buffered(floats_hot[group * 4 + lane], out + lane * micron::f32_shortest_chars_capacity);
    result += observe(out + lane * micron::f32_shortest_chars_capacity, n);
  }
  return result;
}

template<typename Fn>
f64
measure_once(Fn &&format, usize count, u32 repetitions) noexcept
{
  u64 bytes = 0;
  const u64 start = now_ns();
  for ( u32 repeat = 0; repeat < repetitions; ++repeat )
    for ( usize i = 0; i < count; ++i ) bytes += format(i);
  const u64 stop = now_ns();
  sink = bytes;
  return static_cast<f64>(stop - start) / static_cast<f64>(repetitions * count);
}

template<typename Fn>
f64
measure(Fn &&format, usize count, u32 repetitions) noexcept
{
  f64 samples[measurements];
  for ( u32 sample = 0; sample < measurements + warmups; ++sample ) {
    const f64 elapsed = measure_once(format, count, repetitions);
    if ( sample >= warmups ) samples[sample - warmups] = elapsed;
  }
  return median(samples);
}

template<typename A, typename B>
void
measure_pair(A &&a, B &&b, usize count, u32 repetitions, f64 &a_median, f64 &b_median) noexcept
{
  f64 a_samples[measurements];
  f64 b_samples[measurements];
  for ( u32 sample = 0; sample < measurements + warmups; ++sample ) {
    f64 a_elapsed;
    f64 b_elapsed;
    if ( (sample & 1) == 0 ) {
      a_elapsed = measure_once(a, count, repetitions);
      b_elapsed = measure_once(b, count, repetitions);
    } else {
      b_elapsed = measure_once(b, count, repetitions);
      a_elapsed = measure_once(a, count, repetitions);
    }
    if ( sample >= warmups ) {
      a_samples[sample - warmups] = a_elapsed;
      b_samples[sample - warmups] = b_elapsed;
    }
  }
  a_median = median(a_samples);
  b_median = median(b_samples);
}

template<typename Fn>
void
perf_loop(Fn &&format, usize count, u32 repeats) noexcept
{
  u64 bytes = 0;
  for ( u32 repeat = 0; repeat < repeats; ++repeat )
    for ( usize i = 0; i < count; ++i ) bytes += format(i);
  sink = bytes;
}

const char *
case_name(case_t c) noexcept
{
  switch ( c ) {
  case case_t::f64_shortest_hot:
    return "f64-shortest-hot";
  case case_t::f32_shortest_hot:
    return "f32-shortest-hot";
  case case_t::f64_shortest_stream:
    return "f64-shortest-stream";
  case case_t::f32_shortest_stream:
    return "f32-shortest-stream";
  case case_t::batch_f64_hot:
    return "f64-shortest-batch4-hot";
  case case_t::batch_f32_hot:
    return "f32-shortest-batch4-hot";
  case case_t::fixed_hot:
    return "f64-fixed.6-hot";
  case case_t::scientific_hot:
    return "f64-scientific.6-hot";
  case case_t::general_hot:
    return "f64-general.6-hot";
  case case_t::fixed_extreme:
    return "f64-fixed.6-extreme";
  default:
    return "all";
  }
}

template<typename Batch, typename Scalar>
void
run_batch_case(const config &cfg, case_t which, Batch &&batch, Scalar &&scalar) noexcept
{
  if ( cfg.selected_case != case_t::all && cfg.selected_case != which ) return;
  constexpr usize groups = hot_count / 4;
  if ( cfg.perf_loop ) {
    if ( cfg.backend == backend_t::ryu )
      perf_loop(scalar, groups, 16384u);
    else
      perf_loop(batch, groups, 16384u);
    return;
  }
  if ( cfg.backend == backend_t::zmij ) {
    micron::io::println(case_name(which), "  batch=", measure(batch, groups, 4096) * 0.25, " ns/value");
    return;
  }
  if ( cfg.backend == backend_t::ryu ) {
    micron::io::println(case_name(which), "  scalar=", measure(scalar, groups, 4096) * 0.25, " ns/value");
    return;
  }
  f64 batch_ns;
  f64 scalar_ns;
  measure_pair(batch, scalar, groups, 4096, batch_ns, scalar_ns);
  batch_ns *= 0.25;
  scalar_ns *= 0.25;
  micron::io::println(case_name(which), "  batch=", batch_ns, " ns  scalar=", scalar_ns, " ns  speedup=", scalar_ns / batch_ns);
}

template<typename Zmij, typename Ryu>
void
run_case(const config &cfg, case_t which, usize count, Zmij &&zmij, Ryu &&ryu) noexcept
{
  if ( cfg.selected_case != case_t::all && cfg.selected_case != which ) return;
  const u32 measurement_repetitions = which == case_t::fixed_extreme ? 2u : count == hot_count ? 2048u : 64u;
  if ( cfg.perf_loop ) {
    const u32 repeats = which == case_t::fixed_extreme ? 32u : count == hot_count ? 16384u : 2048u;
    if ( cfg.backend == backend_t::ryu )
      perf_loop(ryu, count, repeats);
    else
      perf_loop(zmij, count, repeats);
    return;
  }
  if ( cfg.backend == backend_t::zmij ) {
    micron::io::println(case_name(which), "  zmij=", measure(zmij, count, measurement_repetitions), " ns/value");
    return;
  }
  if ( cfg.backend == backend_t::ryu ) {
    micron::io::println(case_name(which), "  ryu=", measure(ryu, count, measurement_repetitions), " ns/value");
    return;
  }
  f64 z;
  f64 r;
  measure_pair(zmij, ryu, count, measurement_repetitions, z, r);
  micron::io::println(case_name(which), "  zmij=", z, " ns  ryu=", r, " ns  zmij/ryu=", z / r);
}

void
bench(const config &cfg) noexcept
{
  char out[1100];
  char out4[4 * micron::f64_shortest_chars_capacity];
  run_case(
      cfg, case_t::f64_shortest_hot, hot_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::d2s_buffered(doubles_hot[i], out);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::d2s_buffered(doubles_hot[i], out);
        return observe(out, n);
      });
  run_case(
      cfg, case_t::f32_shortest_hot, hot_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::f2s_buffered(floats_hot[i], out);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::__f32::f2s_buffered(floats_hot[i], out);
        return observe(out, n);
      });
  run_case(
      cfg, case_t::f64_shortest_stream, item_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::d2s_buffered(doubles_uniform[i], out);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::d2s_buffered(doubles_uniform[i], out);
        return observe(out, n);
      });
  run_case(
      cfg, case_t::f32_shortest_stream, item_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::f2s_buffered(floats_uniform[i], out);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::__f32::f2s_buffered(floats_uniform[i], out);
        return observe(out, n);
      });
  run_batch_case(
      cfg, case_t::batch_f64_hot, [&](usize group) { return batch4_f64(group, out4); },
      [&](usize group) { return scalar4_f64(group, out4); });
  run_batch_case(
      cfg, case_t::batch_f32_hot, [&](usize group) { return batch4_f32(group, out4); },
      [&](usize group) { return scalar4_f32(group, out4); });
  run_case(
      cfg, case_t::fixed_hot, hot_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::d2f_buffered(doubles_hot[i], out, sizeof(out), 6);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::d2f_buffered(doubles_hot[i], out, sizeof(out), 6);
        return observe(out, n);
      });
  run_case(
      cfg, case_t::scientific_hot, hot_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::d2e_buffered(doubles_hot[i], out, sizeof(out), 6);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::d2e_buffered(doubles_hot[i], out, sizeof(out), 6);
        return observe(out, n);
      });
  run_case(
      cfg, case_t::general_hot, hot_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::d2g_buffered(doubles_hot[i], out, sizeof(out), 6, false, false);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::d2g_buffered(doubles_hot[i], out, sizeof(out), 6, false, false);
        return observe(out, n);
      });
  run_case(
      cfg, case_t::fixed_extreme, item_count,
      [&](usize i) {
        const usize n = micron::__impl::__zmij::d2f_buffered(doubles_uniform[i], out, sizeof(out), 6);
        return observe(out, n);
      },
      [&](usize i) {
        const usize n = micron::__impl::__ryu::d2f_buffered(doubles_uniform[i], out, sizeof(out), 6);
        return observe(out, n);
      });
}

};      // namespace

int
main(int argc, char **argv)
{
  build_corpus();
  const config cfg = parse_args(argc, argv);
  if ( !cfg.perf_loop ) micron::io::println("=== FLOAT FORMAT: ZMIJ VS RYU (isolated median-of-7, ns/value) ===");
  bench(cfg);
  if ( !cfg.perf_loop ) micron::io::fflush(micron::io::stdout);
  return sink == 0;
}
