// math_bessel.cpp
// Rigorous snowball test suite for the Bessel J0/J1/Y0/Y1 set.
// Reference values from <cmath>'s std::cyl_bessel_j / std::cyl_neumann.

#include "../../src/math/mk.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <cmath>

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps = 1e-7)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps + 1e-12 * (b < 0 ? -b : b);
}

int
main()
{
  print("=== BESSEL TESTS ===");

  test_case("j0 — small |x| series");
  {
    require_true(near(mk::special::j0<f64>(0.0), 1.0));
    require_true(near(mk::special::j0<f64>(0.5), std::cyl_bessel_j(0.0, 0.5), 1e-9));
    require_true(near(mk::special::j0<f64>(1.0), std::cyl_bessel_j(0.0, 1.0), 1e-9));
    require_true(near(mk::special::j0<f64>(2.4048), std::cyl_bessel_j(0.0, 2.4048), 1e-7));
    require_true(near(mk::special::j0<f64>(5.0), std::cyl_bessel_j(0.0, 5.0), 1e-6));
    require_true(near(mk::special::j0<f64>(7.5), std::cyl_bessel_j(0.0, 7.5), 1e-5));
    // Negative argument: J0 is even.
    require_true(near(mk::special::j0<f64>(-3.0), std::cyl_bessel_j(0.0, 3.0), 1e-7));
  }
  end_test_case();

  test_case("j0 — asymptotic |x| > 8");
  {
    require_true(near(mk::special::j0<f64>(10.0), std::cyl_bessel_j(0.0, 10.0), 1e-4));
    require_true(near(mk::special::j0<f64>(20.0), std::cyl_bessel_j(0.0, 20.0), 1e-4));
    require_true(near(mk::special::j0<f64>(50.0), std::cyl_bessel_j(0.0, 50.0), 1e-4));
  }
  end_test_case();

  test_case("j1 — small |x| series");
  {
    require_true(near(mk::special::j1<f64>(0.0), 0.0));
    require_true(near(mk::special::j1<f64>(0.5), std::cyl_bessel_j(1.0, 0.5), 1e-9));
    require_true(near(mk::special::j1<f64>(1.0), std::cyl_bessel_j(1.0, 1.0), 1e-9));
    require_true(near(mk::special::j1<f64>(3.8317), std::cyl_bessel_j(1.0, 3.8317), 1e-7));
    require_true(near(mk::special::j1<f64>(5.0), std::cyl_bessel_j(1.0, 5.0), 1e-6));
    // J1 is odd.
    require_true(near(mk::special::j1<f64>(-1.0), -std::cyl_bessel_j(1.0, 1.0), 1e-9));
  }
  end_test_case();

  test_case("j1 — asymptotic");
  {
    require_true(near(mk::special::j1<f64>(10.0), std::cyl_bessel_j(1.0, 10.0), 1e-4));
    require_true(near(mk::special::j1<f64>(20.0), std::cyl_bessel_j(1.0, 20.0), 1e-4));
  }
  end_test_case();

  test_case("y0 / y1 — domain & known values");
  {
    // Y0(x) → -∞ as x → 0+
    require_true(mk::special::y0<f64>(0.0) < -1e10 || std::isinf(mk::special::y0<f64>(0.0)));
    require_true(near(mk::special::y0<f64>(1.0), std::cyl_neumann(0.0, 1.0), 1e-7));
    require_true(near(mk::special::y0<f64>(5.0), std::cyl_neumann(0.0, 5.0), 1e-5));
    require_true(near(mk::special::y0<f64>(10.0), std::cyl_neumann(0.0, 10.0), 1e-4));

    require_true(near(mk::special::y1<f64>(1.0), std::cyl_neumann(1.0, 1.0), 1e-7));
    require_true(near(mk::special::y1<f64>(5.0), std::cyl_neumann(1.0, 5.0), 1e-5));
    require_true(near(mk::special::y1<f64>(10.0), std::cyl_neumann(1.0, 10.0), 1e-4));
  }
  end_test_case();

  print("=== bessel ok ===");
  return 1;
}
