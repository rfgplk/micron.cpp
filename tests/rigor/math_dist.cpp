// math_dist.cpp
// Snowball tests for math::rng::dists templated distribution classes.
// Validates moment estimates from large samples and PDF/CDF/PPF identities.

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
using namespace micron::math::rng::dists;

static bool
near(f64 a, f64 b, f64 eps = 1e-7)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

// Sample a distribution N times and return (mean, var).
template <typename Dist, typename Rng>
static void
moments_of(Dist d, Rng &g, usize N, f64 &mean_out, f64 &var_out)
{
  f64 s = 0, ss = 0;
  for ( usize i = 0; i < N; ++i ) {
    f64 x = f64(d(g));
    s += x;
    ss += x * x;
  }
  mean_out = s / f64(N);
  var_out = (ss - s * s / f64(N)) / f64(N);
}

int
main()
{
  print("=== DIST CLASS TESTS ===");

  // ---------------- engine smoke tests ----------------
  test_case("mt19937 / mwc64 / lcg64 — basic sanity");
  {
    mt19937 m(5489u);     // Matsumoto & Nishimura's reference seed
    // Reference: 10000th output of mt19937(5489) is 4123659995.
    u64 last = 0;
    for ( int i = 0; i < 10000; ++i ) last = m.next32();
    require_true(last == 4123659995u);

    mwc64 w = mwc64::from_seed(0x123456789ABCDEFULL);
    u64 a = w.next();
    u64 b = w.next();
    require_true(a != b);     // distinct outputs

    lcg64 l(0x1234ULL);
    u64 x = l.next();
    u64 y = l.next();
    require_true(x != y);
    require_true(y == lcg64::A * x + lcg64::C);
  }
  end_test_case();

  xoshiro256ss g = xoshiro256ss::from_seed(0xDEADBEEFCAFE);

  // ---------------- gamma ----------------
  test_case("gamma_dist — moments + pdf/cdf");
  {
    gamma_dist<f64> d(2.0, 3.0);     // α=2, θ=3 → mean=6, var=18
    require_true(near(d.mean(), 6.0));
    require_true(near(d.variance(), 18.0));
    f64 m, v;
    moments_of(d, g, 50000, m, v);
    require_true(near(m, 6.0, 0.2));
    require_true(near(v, 18.0, 1.5));

    // pdf(0) = 0 for α > 1
    require_true(near(d.pdf(0.0), 0.0));
    // CDF monotone & in [0, 1]
    require_true(d.cdf(0.0) >= 0.0 && d.cdf(0.0) <= 1.0);
    require_true(d.cdf(100.0) > 0.999);
    require_true(d.cdf(0.5) < d.cdf(5.0));
  }
  end_test_case();

  // ---------------- beta ----------------
  test_case("beta_dist — moments + cdf monotone");
  {
    beta_dist<f64> d(2.0, 5.0);     // mean = 2/7
    require_true(near(d.mean(), 2.0 / 7.0));
    f64 m, v;
    moments_of(d, g, 50000, m, v);
    require_true(near(m, 2.0 / 7.0, 0.01));
    require_true(near(v, d.variance(), 0.005));
    require_true(near(d.cdf(0.0), 0.0));
    require_true(near(d.cdf(1.0), 1.0, 1e-9));
    require_true(d.cdf(0.3) > d.cdf(0.1));
  }
  end_test_case();

  // ---------------- chi2 ----------------
  test_case("chi2_dist — moments");
  {
    chi2_dist<f64> d(4.0);     // mean = 4, var = 8
    require_true(near(d.mean(), 4.0));
    require_true(near(d.variance(), 8.0));
    f64 m, v;
    moments_of(d, g, 50000, m, v);
    require_true(near(m, 4.0, 0.1));
    require_true(near(v, 8.0, 1.0));
  }
  end_test_case();

  // ---------------- student-t ----------------
  test_case("student_t_dist — symmetry + variance");
  {
    student_t_dist<f64> d(10.0);
    require_true(near(d.mean(), 0.0));
    require_true(near(d.variance(), 10.0 / 8.0));
    // CDF symmetric about 0
    require_true(near(d.cdf(0.0), 0.5, 1e-6));
    require_true(near(d.cdf(1.0) + d.cdf(-1.0), 1.0, 1e-6));
  }
  end_test_case();

  // ---------------- F ----------------
  test_case("f_dist — sample positivity + pdf");
  {
    f_dist<f64> d(5.0, 10.0);
    for ( int i = 0; i < 100; ++i ) require_true(d(g) > 0.0);
    require_true(d.pdf(1.0) > 0.0);
    require_true(near(d.cdf(0.0), 0.0));
  }
  end_test_case();

  // ---------------- lognormal ----------------
  test_case("lognormal_dist — ppf round-trip");
  {
    lognormal_dist<f64> d(0.0, 1.0);
    f64 p = 0.3;
    require_true(near(d.cdf(d.ppf(p)), p, 1e-7));
    p = 0.85;
    require_true(near(d.cdf(d.ppf(p)), p, 1e-7));
  }
  end_test_case();

  // ---------------- weibull ----------------
  test_case("weibull_dist — ppf round-trip + closed-form moments");
  {
    weibull_dist<f64> d(2.0, 1.5);
    require_true(near(d.cdf(d.ppf(0.5)), 0.5, 1e-9));
    require_true(d.mean() > 0);
    require_true(d.variance() > 0);
  }
  end_test_case();

  // ---------------- cauchy ----------------
  test_case("cauchy_dist — pdf/cdf identities");
  {
    cauchy_dist<f64> d(0.0, 1.0);
    require_true(near(d.cdf(0.0), 0.5));
    require_true(near(d.pdf(0.0), 1.0 / 3.141592653589793));
    require_true(near(d.cdf(d.ppf(0.7)), 0.7, 1e-9));
  }
  end_test_case();

  // ---------------- geometric ----------------
  test_case("geometric_dist — moments + pmf");
  {
    geometric_dist<i64, f64> d(0.25);
    require_true(near(d.mean(), 3.0));
    require_true(near(d.variance(), 12.0));
    require_true(near(d.pmf(0), 0.25));
    require_true(near(d.cdf(0), 0.25));
  }
  end_test_case();

  // ---------------- binomial ----------------
  test_case("binomial_dist — moments + pmf sums");
  {
    binomial_dist<i64, f64> d(20, 0.4);
    require_true(near(d.mean(), 8.0));
    require_true(near(d.variance(), 4.8));
    f64 sum = 0;
    for ( i64 k = 0; k <= 20; ++k ) sum += d.pmf(k);
    require_true(near(sum, 1.0, 1e-9));
    require_true(near(d.cdf(20), 1.0, 1e-9));
  }
  end_test_case();

  // ---------------- negative_binomial ----------------
  test_case("negative_binomial_dist — moments + pmf");
  {
    negative_binomial_dist<i64, f64> d(5.0, 0.4);
    require_true(near(d.mean(), 5.0 * 0.6 / 0.4));
    require_true(d.pmf(0) > 0);
  }
  end_test_case();

  // ---------------- dirichlet ----------------
  test_case("dirichlet_dist — sample sums to 1");
  {
    f64 alpha[3] = { 1.0, 1.0, 1.0 };
    dirichlet_dist<f64> d(alpha, 3);
    f64 out[3] = { 0 };
    d(g, out);
    require_true(near(out[0] + out[1] + out[2], 1.0, 1e-12));
    require_true(out[0] > 0 && out[1] > 0 && out[2] > 0);
  }
  end_test_case();

  // ---------------- shuffle / permutation / choice ----------------
  test_case("shuffle / permutation / choice");
  {
    int v[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    shuffle<int>(v, v + 10, g);
    int sum = 0;
    for ( int i = 0; i < 10; ++i ) sum += v[i];
    require_true(sum == 45);     // permutation invariant

    int p[8];
    permutation<int>(p, 8, g);
    int psum = 0;
    for ( int i = 0; i < 8; ++i ) psum += p[i];
    require_true(psum == 28);     // 0+1+...+7 = 28

    int c = choice<int>(v, 10, g);
    require_true(c >= 0 && c <= 9);

    f64 w[3] = { 0.0, 1.0, 0.0 };
    int items[3] = { 100, 200, 300 };
    for ( int i = 0; i < 50; ++i ) {
      int pick = choice<int, f64>(items, w, 3, g);
      require_true(pick == 200);     // weight is concentrated on index 1
    }
  }
  end_test_case();

  print("=== dists ok ===");
  return 1;
}
