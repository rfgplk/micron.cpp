// Fixed-seed distribution property and histogram fuzzing.

#include "../../src/math/rng.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;
using namespace micron::math::rng;

namespace
{

struct moments {
  f64 mean;
  f64 variance;
};

template<typename Sampler>
[[nodiscard]] moments
sample_moments(usize n, Sampler &&sample) noexcept
{
  f64 sum = 0.0;
  f64 squares = 0.0;
  for ( usize i = 0; i < n; ++i ) {
    const f64 x = f64(sample());
    sum += x;
    squares += x * x;
  }
  const f64 mean = sum / f64(n);
  return { mean, squares / f64(n) - mean * mean };
}

[[nodiscard]] bool
near(f64 a, f64 b, f64 tolerance) noexcept
{
  const f64 delta = a - b;
  return (delta < 0.0 ? -delta : delta) <= tolerance;
}

struct constant64 {
  using result_type = u64;
  u64 value;

  [[nodiscard]] u64
  next() noexcept
  {
    return value;
  }

  [[nodiscard]] u64
  next64() noexcept
  {
    return value;
  }
};

[[nodiscard]] bool
poisson_histogram() noexcept
{
  constexpr usize samples = 240000;
  constexpr usize bins = 18;
  u64 counts[bins]{};
  auto g = xoshiro256ss::from_seed(0xC6BC279692B5CC83ULL);
  dists::poisson_dist<i64> distribution(3.0);
  f64 sum = 0.0;
  f64 squares = 0.0;
  for ( usize i = 0; i < samples; ++i ) {
    const i64 x = distribution(g);
    if ( x < 0 ) return false;
    counts[usize(x) < bins ? usize(x) : bins - 1]++;
    sum += f64(x);
    squares += f64(x * x);
  }
  const f64 mean = sum / samples;
  const f64 variance = squares / samples - mean * mean;
  if ( !near(mean, 3.0, 0.03) || !near(variance, 3.0, 0.07) ) return false;

  f64 probability = math::fexp(-3.0);
  f64 chi_square = 0.0;
  f64 assigned = 0.0;
  for ( usize k = 0; k + 1 < bins; ++k ) {
    const f64 expected = probability * samples;
    if ( expected >= 20.0 ) {
      const f64 delta = f64(counts[k]) - expected;
      chi_square += delta * delta / expected;
    }
    assigned += probability;
    probability *= 3.0 / f64(k + 1);
  }
  const f64 tail_expected = (1.0 - assigned) * samples;
  if ( tail_expected >= 20.0 ) {
    const f64 delta = f64(counts[bins - 1]) - tail_expected;
    chi_square += delta * delta / tail_expected;
  }
  return chi_square < 32.0;
}

template<usize Bins>
[[nodiscard]] bool
binomial_histogram(i64 n, f64 p, usize samples, f64 chi_limit, u64 seed) noexcept
{
  if ( n < 0 || usize(n + 1) > Bins ) return false;
  u64 counts[Bins]{};
  auto g = xoshiro256ss::from_seed(seed);
  dists::binomial_dist<i64, f64> distribution(n, p);
  f64 sum = 0.0;
  f64 squares = 0.0;
  for ( usize i = 0; i < samples; ++i ) {
    const i64 x = distribution(g);
    if ( x < 0 || x > n ) return false;
    counts[usize(x)]++;
    sum += f64(x);
    squares += f64(x * x);
  }

  const f64 target_mean = f64(n) * p;
  const f64 target_variance = target_mean * (1.0 - p);
  const f64 mean = sum / samples;
  const f64 variance = squares / samples - mean * mean;
  if ( !near(mean, target_mean, 0.06) || !near(variance, target_variance, 0.25) ) return false;

  const f64 q = 1.0 - p;
  f64 probability = math::mk::pow_ns::pow<f64>(q, f64(n));
  f64 chi_square = 0.0;
  for ( i64 k = 0; k <= n; ++k ) {
    const f64 expected = probability * samples;
    if ( expected >= 20.0 ) {
      const f64 delta = f64(counts[k]) - expected;
      chi_square += delta * delta / expected;
    }
    if ( k != n ) probability *= (f64(n - k) / f64(k + 1)) * (p / q);
  }
  return chi_square < chi_limit;
}

};      // namespace

int
main()
{
  print("=== RNG DISTRIBUTION FUZZ ===");

  test_case("closed and open uniform endpoints");
  {
    constant64 zero{ 0 };
    constant64 ones{ ~u64(0) };
    require_true(dist::uniform_real<f64>(zero) == 0.0);
    require_true(dist::uniform_real<f64>(ones) < 1.0);
    const f64 low = dist::uniform_open_real<f64>(zero);
    const f64 high = dist::uniform_open_real<f64>(ones);
    require_true(low == 0x1.0p-53 && high > 0.0 && high < 1.0);
    require_true(dist::exp_dist<f64>(zero) > 0.0);
  }
  end_test_case();

  test_case("invalid parameters terminate deterministically");
  {
    auto g = xoshiro256ss::from_seed(0x243F6A8885A308D3ULL);
    const f64 nan = math::ieee::qnan_v<f64>();
    dists::gamma_dist<f64> bad_gamma(nan, 1.0);
    dists::beta_dist<f64> bad_beta(1.0, nan);
    dists::binomial_dist<i64, f64> bad_binomial(100, nan);
    require_true(math::ieee::is_nan(bad_gamma(g)));
    require_true(math::ieee::is_nan(bad_beta(g)));
    require_true(bad_binomial(g) == 0);
    require_true(dist::poisson<i64>(g, nan) == 0);
    require_true(dist::poisson<i64>(g, math::ieee::inf_v<f64>(0)) == 0);
  }
  end_test_case();

  test_case("uniform integer exhaustive bounds");
  {
    auto g = xoshiro128ss::from_seed(0xDB4F0B9175AE2165ULL);
    bool ok = true;
    for ( u32 range = 1; range <= 257; ++range ) {
      u32 seen = 0;
      for ( usize i = 0; i < 4096; ++i ) {
        const u32 x = dist::uniform_uint_below<u32>(g, range);
        ok &= x < range;
        seen |= u32(1) << (x & 31u);
      }
      if ( range <= 32 ) ok &= seen == (range == 32 ? ~u32(0) : (u32(1) << range) - 1u);
    }
    for ( usize i = 0; i < 10000; ++i ) {
      const i64 x = dist::uniform_int<i64>(g, numeric_limits<i64>::min(), numeric_limits<i64>::max());
      (void)x;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("normal f32/f64 tails and moments");
  {
    auto g64 = xoshiro256ss::from_seed(0x9E3779B97F4A7C15ULL);
    f64 min64 = 0.0, max64 = 0.0;
    const moments m64 = sample_moments(300000, [&] {
      const f64 x = dist::normal<f64>(g64);
      if ( x < min64 ) min64 = x;
      if ( x > max64 ) max64 = x;
      return x;
    });
    require_true(near(m64.mean, 0.0, 0.012));
    require_true(near(m64.variance, 1.0, 0.025));
    require_true(min64 < -3.5 && max64 > 3.5);

    auto g32 = xoshiro128ss::from_seed(0xA24BAED4963EE407ULL);
    const moments m32 = sample_moments(200000, [&] { return dist::normal<f32>(g32); });
    require_true(near(m32.mean, 0.0, 0.015));
    require_true(near(m32.variance, 1.0, 0.03));
  }
  end_test_case();

  test_case("exact Poisson samplers");
  {
    require_true(poisson_histogram());

    auto g40 = xoshiro256ss::from_seed(0x8CB92BA72F3D8DD7ULL);
    dists::poisson_dist<i64> p40(40.0);
    const moments m40 = sample_moments(180000, [&] { return p40(g40); });
    require_true(near(m40.mean, 40.0, 0.12));
    require_true(near(m40.variance, 40.0, 0.7));

    auto g1000 = xoshiro256ss::from_seed(0x4F1BBCDCBFA54001ULL);
    dists::poisson_dist<i64> p1000(1000.0);
    const moments m1000 = sample_moments(160000, [&] { return p1000(g1000); });
    require_true(near(m1000.mean, 1000.0, 0.35));
    require_true(near(m1000.variance, 1000.0, 7.0));

    auto a = xoshiro256ss::from_seed(0x94D049BB133111EBULL);
    auto b = a;
    dists::poisson_dist<i64> cached(73.0);
    bool same = true;
    for ( usize i = 0; i < 10000; ++i ) same &= dist::poisson<i64>(a, 73.0) == cached(b);
    require_true(same);

    constant64 ones{ ~u64(0) };
    dists::poisson_dist<i64> tail(9.9);
    const i64 extreme = tail(ones);
    require_true(extreme > 31 && extreme < 100);
  }
  end_test_case();

  test_case("exact binomial direct kernel histogram");
  {
    require_true(binomial_histogram<17>(16, 0.35, 240000, 38.0, 0xD1342543DE82EF95ULL));
  }
  end_test_case();

  test_case("small-binomial branchless CDF differential fuzz");
  {
    constexpr f64 probabilities[] = { 0.01, 0.2, 0.35, 0.8, 0.99 };
    bool ok = true;
    for ( i64 n = 1; n <= 16; ++n ) {
      for ( f64 p : probabilities ) {
        auto g = xoshiro256ss::from_seed(0xA4093822299F31D0ULL ^ u64(n) ^ u64(p * 1000.0));
        auto oracle = g;
        dists::binomial_dist<i64, f64> distribution(n, p);
        const bool flip = p > 0.5;
        const f64 r = flip ? 1.0 - p : p;
        const f64 odds = r / (1.0 - r);
        for ( usize sample = 0; sample < 4096; ++sample ) {
          const f64 u = dist::uniform_real<f64>(oracle);
          f64 mass = math::mk::pow_ns::pow<f64>(1.0 - r, f64(n));
          f64 cumulative = mass;
          i64 expected = 0;
          while ( u > cumulative && expected < n ) {
            mass *= f64(n - expected) * odds / f64(expected + 1);
            cumulative += mass;
            ++expected;
          }
          if ( distribution(g) != (flip ? n - expected : expected) ) ok = false;
        }
      }
    }
    require_true(ok);
  }
  end_test_case();

  test_case("exact binomial half kernel histogram");
  {
    require_true(binomial_histogram<101>(100, 0.5, 220000, 60.0, 0xF1357AEA2E62A9C5ULL));
  }
  end_test_case();

  test_case("exact binomial BTRD moments and boundaries");
  {
    auto g = xoshiro256ss::from_seed(0xC13FA9A902A6328FULL);
    dists::binomial_dist<i64, f64> broad(10000, 0.01);
    const moments m = sample_moments(180000, [&] { return broad(g); });
    require_true(near(m.mean, 100.0, 0.10));
    require_true(near(m.variance, 99.0, 0.8));

    dists::binomial_dist<i64, f64> p0(29, 0.0);
    dists::binomial_dist<i64, f64> p1(29, 1.0);
    require_true(p0(g) == 0 && p1(g) == 29);
    require_true(p0.pmf(0) == 1.0 && p1.pmf(29) == 1.0);
    constant64 ones{ ~u64(0) };
    dists::binomial_dist<i64, f64> inversion_edge(17, 0.49);
    const i64 edge = inversion_edge(ones);
    require_true(edge >= 0 && edge <= 17);
  }
  end_test_case();

  test_case("geometric and transformed continuous samplers");
  {
    auto g = xoshiro256ss::from_seed(0x91E10DA5C79E7B1DULL);
    dists::geometric_dist<i64, f64> geometric(0.5);
    const moments gm = sample_moments(220000, [&] { return geometric(g); });
    require_true(near(gm.mean, 1.0, 0.025));
    require_true(near(gm.variance, 2.0, 0.08));

    dists::gamma_dist<f64> gamma_small(0.5, 1.25);
    const moments ga = sample_moments(160000, [&] { return gamma_small(g); });
    require_true(near(ga.mean, 0.625, 0.012));
    require_true(near(ga.variance, 0.78125, 0.04));

    dists::beta_dist<f64> beta(2.0, 5.0);
    const moments be = sample_moments(160000, [&] { return beta(g); });
    require_true(near(be.mean, 2.0 / 7.0, 0.006));
    require_true(near(be.variance, 10.0 / (49.0 * 8.0), 0.002));
  }
  end_test_case();

  print("=== RNG DISTRIBUTION FUZZ PASSED ===");
  return 1;
}
