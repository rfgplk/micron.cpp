// math_integrate_mc.cpp — Snowball tests for monte_carlo + quasi_monte_carlo

// Pull monte_carlo directly (NOT via integrate.hpp; the umbrella
// deliberately omits the rng/-dependent module).
#include "../../src/math/integrate/monte_carlo.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== INTEGRATE: MONTE CARLO ===");

  test_case("monte_carlo: ∫∫ 1 dx dy on unit square = 1");
  {
    auto f = [](const f64 (&)[2]) noexcept -> f64 { return 1.0; };
    f64 lo[2] = { 0, 0 };
    f64 hi[2] = { 1, 1 };
    rng::xoshiro256ss g{ rng::xoshiro256ss::from_seed(42u) };
    f64 r = integrate::monte_carlo<2, f64>(f, lo, hi, 10000, g);
    require_true(near(r, 1.0, 1e-12));     // constant integrand → exact
  }
  end_test_case();

  test_case("monte_carlo: ∫∫ x²+y² on unit square ≈ 2/3");
  {
    auto f = [](const f64 (&x)[2]) noexcept -> f64 { return x[0] * x[0] + x[1] * x[1]; };
    f64 lo[2] = { 0, 0 };
    f64 hi[2] = { 1, 1 };
    rng::xoshiro256ss g{ rng::xoshiro256ss::from_seed(123u) };
    f64 r = integrate::monte_carlo<2, f64>(f, lo, hi, 200000, g);
    // ∫_0^1 ∫_0^1 (x² + y²) dx dy = 1/3 + 1/3 = 2/3
    require_true(near(r, 2.0 / 3.0, 5e-3));
  }
  end_test_case();

  test_case("quasi_monte_carlo (Halton): tighter convergence than MC");
  {
    auto f = [](const f64 (&x)[3]) noexcept -> f64 { return x[0] * x[1] * x[2]; };
    f64 lo[3] = { 0, 0, 0 };
    f64 hi[3] = { 1, 1, 1 };
    f64 r = integrate::quasi_monte_carlo<3, f64>(f, lo, hi, 4096);
    // Truth = 1/8.  Halton gets sub-1e-3 with 4k samples.
    require_true(near(r, 0.125, 5e-3));
  }
  end_test_case();

  test_case("monte_carlo: 4D unit hypercube, ∫ x_i sum");
  {
    auto f = [](const f64 (&x)[4]) noexcept -> f64 { return x[0] + x[1] + x[2] + x[3]; };
    f64 lo[4] = { 0, 0, 0, 0 };
    f64 hi[4] = { 1, 1, 1, 1 };
    rng::xoshiro256ss g{ rng::xoshiro256ss::from_seed(0xc0ffeeULL) };
    f64 r = integrate::monte_carlo<4, f64>(f, lo, hi, 200000, g);
    // ∫ (x1+x2+x3+x4) dV = 4 · (1/2) = 2
    require_true(near(r, 2.0, 1e-2));
  }
  end_test_case();

  print("=== integrate monte_carlo ok ===");
  return 1;
}
