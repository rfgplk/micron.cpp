// math_integrate_trapezoid.cpp — Snowball tests for composite trapezoid + simpson + samples

#include "../../src/math/integrate/integrate.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
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
  print("=== INTEGRATE: TRAPEZOID / SIMPSON / SAMPLES ===");

  test_case("trapezoid: ∫_0^π sin x dx = 2");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::trig::sin<f64>(x); };
    const f64 pi = mk::trig::cos<f64>(f64(0)) > f64(0) ? 3.14159265358979323846 : 3.14159265358979323846;
    f64 r = integrate::trapezoid<f64>(f, f64(0), pi, 4096);
    require_true(near(r, 2.0, 1e-6));
  }
  end_test_case();

  test_case("trapezoid: ∫_0^1 x² dx = 1/3");
  {
    auto f = [](f64 x) noexcept -> f64 { return x * x; };
    f64 r = integrate::trapezoid<f64>(f, f64(0), f64(1), 1024);
    require_true(near(r, 1.0 / 3.0, 1e-6));
  }
  end_test_case();

  test_case("trapezoid uniform samples");
  {
    f64 y[5] = { 0, 1, 4, 9, 16 };     // f(x) = x² at x = 0,1,2,3,4 → ∫_0^4 x² dx = 64/3 ≈ 21.333
    f64 r = integrate::trapezoid<f64>(y, 5, 1.0);
    // Trapezoid with 5 samples: (0/2 + 1 + 4 + 9 + 16/2) · 1 = 22
    require_true(near(r, 22.0));
  }
  end_test_case();

  test_case("trapezoid non-uniform xs/ys");
  {
    f64 xs[4] = { 0, 1, 3, 7 };
    f64 ys[4] = { 0, 1, 9, 49 };     // f(x) = x²
    f64 r = integrate::trapezoid<f64>(xs, ys, 4);
    // (0+1)/2·1 + (1+9)/2·2 + (9+49)/2·4 = 0.5 + 10 + 116 = 126.5
    require_true(near(r, 126.5));
  }
  end_test_case();

  test_case("simpson 1/3: ∫_0^1 x³ dx = 1/4 (exact for cubic)");
  {
    auto f = [](f64 x) noexcept -> f64 { return x * x * x; };
    f64 r = integrate::simpson<f64>(f, f64(0), f64(1), 4);
    require_true(near(r, 0.25, 1e-12));
  }
  end_test_case();

  test_case("simpson 1/3: ∫_0^π sin x dx = 2");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::trig::sin<f64>(x); };
    const f64 pi = 3.14159265358979323846;
    f64 r = integrate::simpson<f64>(f, f64(0), pi, 64);
    require_true(near(r, 2.0, 1e-6));
  }
  end_test_case();

  test_case("simpson 3/8: ∫_0^1 x³ dx = 1/4 (also exact)");
  {
    auto f = [](f64 x) noexcept -> f64 { return x * x * x; };
    f64 r = integrate::simpson_38<f64>(f, f64(0), f64(1), 6);
    require_true(near(r, 0.25, 1e-12));
  }
  end_test_case();

  test_case("integrate_samples + cum_trapezoid");
  {
    f64 xs[5] = { 0, 1, 2, 3, 4 };
    f64 ys[5] = { 0, 1, 4, 9, 16 };
    require_true(near(integrate::integrate_samples<f64>(xs, ys, 5), 22.0));

    f64 cum[5];
    integrate::cum_trapezoid<f64>(xs, ys, cum, 5);
    require_true(near(cum[0], 0.0));
    require_true(near(cum[1], 0.5));      // (0+1)/2
    require_true(near(cum[2], 3.0));      // 0.5 + (1+4)/2
    require_true(near(cum[3], 9.5));      // 3 + (4+9)/2
    require_true(near(cum[4], 22.0));     // 9.5 + (9+16)/2
  }
  end_test_case();

  print("=== integrate trapezoid/simpson/samples ok ===");
  return 0;
}
