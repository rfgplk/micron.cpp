// math_rng.cpp
// Rigorous snowball test suite for math::rng — engines and
// distributions.  Statistical sanity checks at moderate N.

#include "../../src/math/rng.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps = 1e-10)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== MATH::RNG TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // engine size contracts
  test_case("engine state sizes");
  {
    static_assert(sizeof(rng::splitmix64) == 8);
    static_assert(sizeof(rng::xoshiro256ss) == 32);
    static_assert(sizeof(rng::xoshiro128ss) == 16);
    static_assert(sizeof(rng::pcg64) == 32);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // splitmix64
  test_case("splitmix64 — known sequence from seed 0");
  {
    rng::splitmix64 sm{ 0 };
    // Vigna's reference: first three outputs from seed 0
    u64 a = sm.next();
    u64 b = sm.next();
    u64 c = sm.next();
    require(a, 0xE220A8397B1DCDAFULL);
    require(b, 0x6E789E6AA1B965F4ULL);
    require(c, 0x06C45D188009454FULL);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // xoshiro256ss
  test_case("xoshiro256ss — non-trivial output");
  {
    auto g = rng::xoshiro256ss::from_seed(1234567);
    u64 a = g.next();
    u64 b = g.next();
    require_false(a == b);
    require_false(a == 0 && b == 0);
  }
  end_test_case();

  test_case("xoshiro256ss — jump diverges");
  {
    auto g1 = rng::xoshiro256ss::from_seed(42);
    auto g2 = g1;
    g2.jump();
    // first output after jump must differ from base
    require_false(g1.next() == g2.next());
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // pcg64
  test_case("pcg64 — non-trivial, distinct streams");
  {
    auto a = rng::pcg64::make(7, 1);
    auto b = rng::pcg64::make(7, 2);
    u64 av = a.next();
    u64 bv = b.next();
    require_false(av == bv);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // uniform distributions
  test_case("uniform_real — within [0,1)");
  {
    auto g = rng::xoshiro256ss::from_seed(7);
    for ( int i = 0; i < 10000; ++i ) {
      f64 u = rng::dist::uniform_real<f64>(g);
      require_true(u >= 0.0 && u < 1.0);
    }
  }
  end_test_case();

  test_case("uniform_int — bounded");
  {
    auto g = rng::xoshiro256ss::from_seed(3);
    int hits[6] = { 0 };
    for ( int i = 0; i < 60000; ++i ) {
      i32 v = rng::dist::uniform_int<i32>(g, 5, 10);
      require_true(v >= 5 && v <= 10);
      hits[v - 5]++;
    }
    // each bucket ~10000 +/- a few hundred
    for ( int i = 0; i < 6; ++i ) {
      require_true(hits[i] > 9000 && hits[i] < 11000);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // bernoulli
  test_case("bernoulli — p ≈ 0.5");
  {
    auto g = rng::xoshiro256ss::from_seed(5);
    int hits = 0;
    for ( int i = 0; i < 100000; ++i )
      if ( rng::dist::bernoulli(g, 0.5) ) ++hits;
    require_true(hits > 49000 && hits < 51000);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // normal
  test_case("normal (Marsaglia polar) — mean ≈ 0, var ≈ 1");
  {
    auto g = rng::xoshiro256ss::from_seed(11);
    int N = 50000;
    f64 sum = 0.0;
    f64 sumsq = 0.0;
    for ( int i = 0; i < N; ++i ) {
      f64 x = rng::dist::normal<f64>(g);
      sum += x;
      sumsq += x * x;
    }
    f64 mean = sum / N;
    f64 var = sumsq / N - mean * mean;
    require_true(near(mean, 0.0, 0.05));
    require_true(near(var, 1.0, 0.05));
  }
  end_test_case();

  test_case("normal_ziggurat — mean ≈ 0, var ≈ 1");
  {
    auto g = rng::xoshiro256ss::from_seed(13);
    int N = 50000;
    f64 sum = 0.0;
    f64 sumsq = 0.0;
    for ( int i = 0; i < N; ++i ) {
      f64 x = rng::dist::normal_ziggurat<f64>(g);
      sum += x;
      sumsq += x * x;
    }
    f64 mean = sum / N;
    f64 var = sumsq / N - mean * mean;
    require_true(near(mean, 0.0, 0.1));
    require_true(near(var, 1.0, 0.1));
  }
  end_test_case();

  test_case("exp_dist — mean = 1/lambda");
  {
    auto g = rng::xoshiro256ss::from_seed(17);
    int N = 50000;
    f64 sum = 0.0;
    for ( int i = 0; i < N; ++i ) sum += rng::dist::exp_dist<f64>(g, 1.0);
    f64 mean = sum / N;
    require_true(near(mean, 1.0, 0.05));
  }
  end_test_case();

  test_case("poisson — mean ≈ lambda for small λ");
  {
    auto g = rng::xoshiro256ss::from_seed(19);
    int N = 5000;
    i64 sum = 0;
    for ( int i = 0; i < N; ++i ) sum += rng::dist::poisson(g, 3.0);
    f64 mean = f64(sum) / N;
    require_true(near(mean, 3.0, 0.2));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // fill batch APIs
  test_case("fill::fill_uniform — span fully populated");
  {
    auto g = rng::xoshiro256ss::from_seed(23);
    f64 buf[256];
    rng::fill::fill_uniform<f64>(buf, 256, g);
    for ( int i = 0; i < 256; ++i ) require_true(buf[i] >= 0.0 && buf[i] < 1.0);
  }
  end_test_case();

  test_case("fill::fill_normal — population stats");
  {
    auto g = rng::xoshiro256ss::from_seed(29);
    constexpr int N = 8192;
    f64 buf[N];
    rng::fill::fill_normal<f64>(buf, N, g);
    f64 sum = 0.0, sumsq = 0.0;
    for ( int i = 0; i < N; ++i ) {
      sum += buf[i];
      sumsq += buf[i] * buf[i];
    }
    f64 mean = sum / N;
    f64 var = sumsq / N - mean * mean;
    require_true(near(mean, 0.0, 0.1));
    require_true(near(var, 1.0, 0.1));
  }
  end_test_case();

  test_case("fill::fill_bytes — non-zero output");
  {
    auto g = rng::xoshiro256ss::from_seed(31);
    u8 buf[256] = { 0 };
    rng::fill::fill_bytes(buf, 256, g);
    int nonzero = 0;
    for ( int i = 0; i < 256; ++i )
      if ( buf[i] != 0 ) ++nonzero;
    // overwhelmingly likely to be all non-zero or close to it
    require_true(nonzero > 240);
  }
  end_test_case();

  print("=== MATH::RNG TESTS PASSED ===");
  return 1;
}
