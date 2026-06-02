// math_step_angle.cpp — Snowball tests for GLSL-style step/smoothstep/smootherstep
// and the radians/degrees/turns angle-unit helpers (scalar + component-wise).

#include "../../src/math/generic.hpp"
#include "../../src/math/quants/vecs.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;

static bool
near(double a, double b, double eps = 1e-12)
{
  double d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== STEP / SMOOTHSTEP / ANGLE ===");

  // ------------------------------------------------------------------
  test_case("scalar step: 0 below edge, 1 at/above edge");
  {
    require_true(m::step(0.5, 0.3) == 0.0);
    require_true(m::step(0.5, 0.7) == 1.0);
    require_true(m::step(0.5, 0.5) == 1.0);      // x >= edge -> 1
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("scalar smoothstep: clamps ends, 0.5 at midpoint, monotone");
  {
    require_true(near(m::smoothstep(0.0, 1.0, -0.5), 0.0));
    require_true(near(m::smoothstep(0.0, 1.0, 1.5), 1.0));
    require_true(near(m::smoothstep(0.0, 1.0, 0.0), 0.0));
    require_true(near(m::smoothstep(0.0, 1.0, 1.0), 1.0));
    require_true(near(m::smoothstep(0.0, 1.0, 0.5), 0.5));
    require_true(m::smoothstep(0.0, 1.0, 0.25) < m::smoothstep(0.0, 1.0, 0.75));
    // works on a non-[0,1] interval too
    require_true(near(m::smoothstep(2.0, 6.0, 4.0), 0.5));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("scalar smootherstep: clamps ends, 0.5 at midpoint, C2 flat ends");
  {
    require_true(near(m::smootherstep(0.0, 1.0, -1.0), 0.0));
    require_true(near(m::smootherstep(0.0, 1.0, 2.0), 1.0));
    require_true(near(m::smootherstep(0.0, 1.0, 0.5), 0.5));
    // 6t^5 - 15t^4 + 10t^3 at t=0.25
    require_true(near(m::smootherstep(0.0, 1.0, 0.25), 0.103515625));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("angle helpers: radians/degrees round-trip + known values");
  {
    const double pi = 3.14159265358979323846;
    require_true(near(m::radians(180.0), pi, 1e-12));
    require_true(near(m::degrees(pi), 180.0, 1e-12));
    require_true(near(m::degrees(m::radians(57.0)), 57.0, 1e-12));
    require_true(near(m::turns(1.0), 2.0 * pi, 1e-12));
    require_true(near(m::turns(0.5), pi, 1e-12));
    // template instantiates for float as well
    require_true(near(static_cast<double>(m::radians(90.0f)), pi / 2.0, 1e-5));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("vector_3 step/smoothstep are component-wise");
  {
    micron::vector_3<double> v{ 0.3, 0.5, 0.7 };
    auto s = v.step(0.5);
    require_true(s.x == 0.0 && s.y == 1.0 && s.z == 1.0);

    micron::vector_3<double> u{ -0.5, 0.5, 1.5 };
    auto ss = u.smoothstep(0.0, 1.0);
    require_true(near(ss.x, 0.0) && near(ss.y, 0.5) && near(ss.z, 1.0));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("vector_4 step/smoothstep are component-wise");
  {
    micron::vector_4<double> v{ 0.1, 0.6, 0.4, 0.9 };
    auto s = v.step(0.5);
    require_true(s.x == 0.0 && s.y == 1.0 && s.z == 0.0 && s.w == 1.0);
    auto ss = micron::vector_4<double>{ 0.0, 0.25, 0.5, 1.0 }.smoothstep(0.0, 1.0);
    require_true(near(ss.x, 0.0) && near(ss.z, 0.5) && near(ss.w, 1.0));
    require_true(near(ss.y, m::smoothstep(0.0, 1.0, 0.25)));
  }
  end_test_case();

  print("=== STEP / SMOOTHSTEP / ANGLE PASSED ===");
  return 1;
}
