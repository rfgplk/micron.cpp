//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// RNG engine and sampler throughput census.  Build with:
//   duck build benches/rng_bench.cpp -i . --perf --fp --no-ssp --no-lto -o bin/bench
// Run pinned:
//   taskset -c 2 bin/bench/rng_bench
// Aggregate regression gate:
//   benches/compare_rng_bench.py benches/results/rng_before_native.csv <capture.csv>

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/math/rng.hpp"
#include "../src/std.hpp"

namespace
{

using rng_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

namespace rng = micron::math::rng;
namespace dist = micron::math::rng::dist;
namespace dists = micron::math::rng::dists;
namespace fill = micron::math::rng::fill;

constexpr u32 K_MEASUREMENTS = 7;
constexpr u32 WARMUP_REPS = 3;
constexpr usize ENGINE_N = 16384;
constexpr usize SAMPLE_N = 4096;
constexpr usize BULK_N = 4096;

alignas(64) static u64 g_u64[BULK_N];
alignas(64) static u32 g_u32[BULK_N];
alignas(64) static i32 g_i32[BULK_N];
alignas(64) static f64 g_f64[BULK_N];
alignas(64) static f32 g_f32[BULK_N];
alignas(64) static u8 g_bytes[BULK_N];
alignas(64) static f64 g_dirichlet[16];
static volatile u64 g_sink = 0;

template<typename T>
[[gnu::always_inline]] inline void
consume(T value) noexcept
{
  if constexpr ( micron::is_integral_v<T> ) {
    g_sink = g_sink ^ static_cast<u64>(value);
  } else {
    u64 bits = 0;
    if constexpr ( sizeof(T) == sizeof(u64) )
      __builtin_memcpy(&bits, &value, sizeof(value));
    else {
      u32 lo = 0;
      __builtin_memcpy(&lo, &value, sizeof(value));
      bits = lo;
    }
    g_sink = g_sink ^ bits;
  }
}

[[nodiscard]] f64
median(f64 *values) noexcept
{
  for ( u32 i = 1; i < K_MEASUREMENTS; ++i ) {
    const f64 key = values[i];
    u32 j = i;
    while ( j != 0 && values[j - 1] > key ) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = key;
  }
  return values[K_MEASUREMENTS / 2];
}

struct cell {
  f64 cycles;
  f64 instructions;
  f64 ipc;
  f64 branches;
  f64 miss_pct;
};

template<typename Kernel>
[[nodiscard, gnu::noinline]] cell
measure(usize operations, Kernel &&kernel) noexcept
{
  for ( u32 i = 0; i < WARMUP_REPS; ++i ) kernel();

  f64 cyc[K_MEASUREMENTS]{};
  f64 ins[K_MEASUREMENTS]{};
  f64 ipc[K_MEASUREMENTS]{};
  f64 branches[K_MEASUREMENTS]{};
  f64 misses[K_MEASUREMENTS]{};

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    rng_events events{ bbench::quiet{} };
    events.open();
    events.begin();
    kernel();
    events.end();
    const u64 c = static_cast<u64>(events.get<bbench::hardware_cycles>().retrieve());
    const u64 i = static_cast<u64>(events.get<bbench::hardware_instructions>().retrieve());
    const u64 b = static_cast<u64>(events.get<bbench::branches>().retrieve());
    const u64 bm = static_cast<u64>(events.get<bbench::branch_misses>().retrieve());
    const f64 n = static_cast<f64>(operations);
    cyc[m] = static_cast<f64>(c) / n;
    ins[m] = static_cast<f64>(i) / n;
    ipc[m] = c != 0 ? static_cast<f64>(i) / static_cast<f64>(c) : 0.0;
    branches[m] = static_cast<f64>(b) / n;
    misses[m] = b != 0 ? static_cast<f64>(bm) * 100.0 / static_cast<f64>(b) : 0.0;
  }
  return { median(cyc), median(ins), median(ipc), median(branches), median(misses) };
}

[[nodiscard, gnu::always_inline]] inline u64
scaled(f64 value, u64 scale = 1000) noexcept
{
  return static_cast<u64>(value * static_cast<f64>(scale) + 0.5);
}

void
print_cell(const char *group, const char *name, const cell &c)
{
  micron::io::println(group, ",", name, ",", scaled(c.cycles), ",", scaled(c.instructions), ",", scaled(c.ipc), ",", scaled(c.branches),
                      ",", scaled(c.miss_pct));
}

template<typename Engine>
[[nodiscard]] Engine
make_engine(u64 seed) noexcept
{
  if constexpr ( requires { Engine::from_seed(seed); } )
    return Engine::from_seed(seed);
  else
    return Engine(seed);
}

template<typename Engine, typename T>
inline void
generate_or_scalar(Engine &engine, T *out, usize n) noexcept
{
  if constexpr ( requires { engine.generate(out, n); } )
    engine.generate(out, n);
  else
    for ( usize i = 0; i < n; ++i ) out[i] = engine.next();
}

template<typename Engine>
void
bench_engine(const char *name, u64 seed)
{
  Engine scalar = make_engine<Engine>(seed);
  print_cell("engine_scalar", name, measure(ENGINE_N, [&] {
               u64 acc = 0;
               for ( usize i = 0; i < ENGINE_N; ++i ) acc ^= static_cast<u64>(scalar.next());
               consume(acc);
             }));

  Engine streams[4]
      = { make_engine<Engine>(seed), make_engine<Engine>(seed + 1), make_engine<Engine>(seed + 2), make_engine<Engine>(seed + 3) };
  print_cell("engine_stream4", name, measure(ENGINE_N, [&] {
               u64 a = 0, b = 0, c = 0, d = 0;
               for ( usize i = 0; i < ENGINE_N / 4; ++i ) {
                 a ^= static_cast<u64>(streams[0].next());
                 b ^= static_cast<u64>(streams[1].next());
                 c ^= static_cast<u64>(streams[2].next());
                 d ^= static_cast<u64>(streams[3].next());
               }
               consume(a ^ b ^ c ^ d);
             }));

  Engine bulk = make_engine<Engine>(seed);
  using result_type = decltype(bulk.next());
  alignas(64) static result_type outputs[BULK_N];
  print_cell("engine_bulk", name, measure(BULK_N, [&] {
               generate_or_scalar(bulk, outputs, BULK_N);
               consume(outputs[0] ^ outputs[BULK_N - 1]);
             }));
}

template<typename Fn>
void
bench_integral_sampler(const char *name, Fn &&fn)
{
  print_cell("sampler", name, measure(SAMPLE_N, [&] {
               u64 acc = 0;
               for ( usize i = 0; i < SAMPLE_N; ++i ) acc += static_cast<u64>(fn());
               consume(acc);
             }));
}

template<typename Fn>
void
bench_real_sampler(const char *name, Fn &&fn)
{
  print_cell("sampler", name, measure(SAMPLE_N, [&] {
               f64 a = 0.0, b = 0.0, c = 0.0, d = 0.0;
               for ( usize i = 0; i < SAMPLE_N; i += 4 ) {
                 a += static_cast<f64>(fn());
                 b += static_cast<f64>(fn());
                 c += static_cast<f64>(fn());
                 d += static_cast<f64>(fn());
               }
               consume(a + b + c + d);
             }));
}

void
bench_engines()
{
  constexpr u64 seed = 0x9e3779b97f4a7c15ULL;
  bench_engine<rng::splitmix64>("splitmix64", seed);
  bench_engine<rng::xoshiro256ss>("xoshiro256ss", seed);
  bench_engine<rng::xoshiro128ss>("xoshiro128ss", seed);

  rng::pcg64 pcg_scalar = rng::pcg64::make(seed, 7);
  print_cell("engine_scalar", "pcg64", measure(ENGINE_N, [&] {
               u64 acc = 0;
               for ( usize i = 0; i < ENGINE_N; ++i ) acc ^= pcg_scalar.next();
               consume(acc);
             }));
  rng::pcg64 pcg_streams[4]
      = { rng::pcg64::make(seed, 7), rng::pcg64::make(seed + 1, 8), rng::pcg64::make(seed + 2, 9), rng::pcg64::make(seed + 3, 10) };
  print_cell("engine_stream4", "pcg64", measure(ENGINE_N, [&] {
               u64 a = 0, b = 0, c = 0, d = 0;
               for ( usize i = 0; i < ENGINE_N / 4; ++i ) {
                 a ^= pcg_streams[0].next();
                 b ^= pcg_streams[1].next();
                 c ^= pcg_streams[2].next();
                 d ^= pcg_streams[3].next();
               }
               consume(a ^ b ^ c ^ d);
             }));
  rng::pcg64 pcg_bulk = rng::pcg64::make(seed, 7);
  print_cell("engine_bulk", "pcg64", measure(BULK_N, [&] {
               generate_or_scalar(pcg_bulk, g_u64, BULK_N);
               consume(g_u64[0] ^ g_u64[BULK_N - 1]);
             }));

  bench_engine<rng::mt19937>("mt19937", seed);
  bench_engine<rng::mwc64>("mwc64", seed);
  bench_engine<rng::lcg64>("lcg64", seed);
}

void
bench_primitive_samplers()
{
  auto g = rng::xoshiro256ss::from_seed(0x123456789abcdef0ULL);
  bench_real_sampler("uniform_f32", [&] { return dist::uniform_real<f32>(g); });
  bench_real_sampler("uniform_f64", [&] { return dist::uniform_real<f64>(g); });
  bench_integral_sampler("uniform_u8_197", [&] { return dist::uniform_int<u8>(g, 3, 199); });
  bench_integral_sampler("uniform_u16_60001", [&] { return dist::uniform_int<u16>(g, 7, 60007); });
  bench_integral_sampler("uniform_u32_pow2", [&] { return dist::uniform_int<u32>(g, 0, 65535); });
  bench_integral_sampler("uniform_u32_1e9", [&] { return dist::uniform_int<u32>(g, 17, 1000000017); });
  bench_integral_sampler("uniform_u64_1e12", [&] { return dist::uniform_int<u64>(g, 19, 1000000000019ULL); });
  bench_integral_sampler("bernoulli_half", [&] { return dist::bernoulli(g, 0.5); });
  bench_integral_sampler("bernoulli_rare", [&] { return dist::bernoulli(g, 0.001); });
  bench_real_sampler("normal_default", [&] { return dist::normal<f64>(g); });
  bench_real_sampler("normal_ziggurat", [&] { return dist::normal_ziggurat<f64>(g); });
  bench_real_sampler("exponential", [&] { return dist::exp_dist<f64>(g, 1.25); });
  bench_integral_sampler("poisson_3", [&] { return dist::poisson(g, 3.0); });
  bench_integral_sampler("poisson_40", [&] { return dist::poisson(g, 40.0); });
  bench_integral_sampler("poisson_1000", [&] { return dist::poisson(g, 1000.0); });
}

void
bench_fills()
{
  auto g = rng::xoshiro256ss::from_seed(0xa0761d6478bd642fULL);
  print_cell("fill", "uniform_f32", measure(BULK_N, [&] {
               fill::fill_uniform(g_f32, BULK_N, g);
               consume(g_f32[0] + g_f32[BULK_N - 1]);
             }));
  print_cell("fill", "uniform_f64", measure(BULK_N, [&] {
               fill::fill_uniform(g_f64, BULK_N, g);
               consume(g_f64[0] + g_f64[BULK_N - 1]);
             }));
  print_cell("fill", "uniform_i32", measure(BULK_N, [&] {
               fill::fill_uniform_int(g_i32, BULK_N, g, -1000000, 1000000);
               consume(g_i32[0] + g_i32[BULK_N - 1]);
             }));
  print_cell("fill", "bytes", measure(BULK_N, [&] {
               fill::fill_bytes(g_bytes, BULK_N, g);
               consume(g_bytes[0] ^ g_bytes[BULK_N - 1]);
             }));
  print_cell("fill", "normal_f32", measure(BULK_N, [&] {
               fill::fill_normal(g_f32, BULK_N, g);
               consume(g_f32[0] + g_f32[BULK_N - 1]);
             }));
  print_cell("fill", "normal_f64", measure(BULK_N, [&] {
               fill::fill_normal(g_f64, BULK_N, g);
               consume(g_f64[0] + g_f64[BULK_N - 1]);
             }));
}

void
bench_distributions()
{
  auto g = rng::xoshiro256ss::from_seed(0xe7037ed1a0b428dbULL);
  dists::gamma_dist<f64> gamma_small(0.5, 1.25);
  dists::gamma_dist<f64> gamma_mid(2.0, 3.0);
  dists::gamma_dist<f64> gamma_large(20.0, 0.5);
  dists::beta_dist<f64> beta(2.0, 5.0);
  dists::chi2_dist<f64> chi2(10.0);
  dists::student_t_dist<f64> student(10.0);
  dists::f_dist<f64> fisher(5.0, 10.0);
  dists::lognormal_dist<f64> lognormal(0.25, 1.25);
  dists::weibull_dist<f64> weibull(2.0, 1.5);
  dists::cauchy_dist<f64> cauchy(0.0, 1.0);
  dists::geometric_dist<i64, f64> geometric_half(0.5);
  dists::geometric_dist<i64, f64> geometric_rare(0.01);
  dists::binomial_dist<i64, f64> binomial_small(16, 0.35);
  dists::binomial_dist<i64, f64> binomial_mid(100, 0.5);
  dists::binomial_dist<i64, f64> binomial_large(10000, 0.01);
  dists::poisson_dist<i64> poisson_small(3.0);
  dists::poisson_dist<i64> poisson_mid(40.0);
  dists::poisson_dist<i64> poisson_large(1000.0);
  dists::negative_binomial_dist<i64, f64> negative_binomial(5.0, 0.4);

  bench_real_sampler("gamma_a0_5", [&] { return gamma_small(g); });
  bench_real_sampler("gamma_a2", [&] { return gamma_mid(g); });
  bench_real_sampler("gamma_a20", [&] { return gamma_large(g); });
  bench_real_sampler("beta", [&] { return beta(g); });
  bench_real_sampler("chi2", [&] { return chi2(g); });
  bench_real_sampler("student_t", [&] { return student(g); });
  bench_real_sampler("f", [&] { return fisher(g); });
  bench_real_sampler("lognormal", [&] { return lognormal(g); });
  bench_real_sampler("weibull", [&] { return weibull(g); });
  bench_real_sampler("cauchy", [&] { return cauchy(g); });
  bench_integral_sampler("geometric_p0_5", [&] { return geometric_half(g); });
  bench_integral_sampler("geometric_p0_01", [&] { return geometric_rare(g); });
  bench_integral_sampler("binomial_n16", [&] { return binomial_small(g); });
  bench_integral_sampler("binomial_n100", [&] { return binomial_mid(g); });
  bench_integral_sampler("binomial_n10000", [&] { return binomial_large(g); });
  bench_integral_sampler("poisson_cached_3", [&] { return poisson_small(g); });
  bench_integral_sampler("poisson_cached_40", [&] { return poisson_mid(g); });
  bench_integral_sampler("poisson_cached_1000", [&] { return poisson_large(g); });
  bench_integral_sampler("negative_binomial", [&] { return negative_binomial(g); });

  const f64 alpha3[3] = { 0.5, 1.5, 3.0 };
  const f64 alpha16[16] = { 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5, 2.75, 3.0, 3.25, 3.5, 3.75, 4.0, 4.25 };
  dists::dirichlet_dist<f64, 3> dirichlet3(alpha3);
  dists::dirichlet_dist<f64, 16> dirichlet16(alpha16);
  print_cell("sampler", "dirichlet_k3", measure(SAMPLE_N * 3, [&] {
               for ( usize i = 0; i < SAMPLE_N; ++i ) dirichlet3(g, g_dirichlet);
               consume(g_dirichlet[0]);
             }));
  print_cell("sampler", "dirichlet_k16", measure(SAMPLE_N * 16, [&] {
               for ( usize i = 0; i < SAMPLE_N; ++i ) dirichlet16(g, g_dirichlet);
               consume(g_dirichlet[0]);
             }));

  for ( usize i = 0; i < 256; ++i ) g_u32[i] = static_cast<u32>(i);
  print_cell("helper", "shuffle_256", measure(256, [&] {
               dists::shuffle(g_u32, g_u32 + 256, g);
               consume(g_u32[0]);
             }));
  bench_integral_sampler("choice_256", [&] { return dists::choice(g_u32, 256, g); });
}

void
bench_hardware()
{
#if defined(__micron_x86_rdrnd) && defined(__micron_arch_amd64)
  print_cell("hardware", "rdrand64", measure(SAMPLE_N, [] {
               u64 acc = 0;
               for ( usize i = 0; i < SAMPLE_N; ++i ) {
                 u64 value = 0;
                 (void)rng::hardware::rdrand64(value);
                 acc ^= value;
               }
               consume(acc);
             }));
#endif
#if defined(__micron_x86_rdseed) && defined(__micron_arch_amd64)
  print_cell("hardware", "rdseed64", measure(SAMPLE_N, [] {
               u64 acc = 0;
               for ( usize i = 0; i < SAMPLE_N; ++i ) {
                 u64 value = 0;
                 (void)rng::hardware::rdseed64(value);
                 acc ^= value;
               }
               consume(acc);
             }));
#endif
}

};      // namespace

int
main()
{
  micron::io::println("group,name,cycles_x1000,instructions_x1000,ipc_x1000,branches_x1000,branch_miss_pct_x1000");
  bench_engines();
  bench_primitive_samplers();
  bench_fills();
  bench_distributions();
  bench_hardware();
  consume(g_sink);
  return 1;
}
