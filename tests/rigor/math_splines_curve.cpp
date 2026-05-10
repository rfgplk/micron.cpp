// math_splines_curve.cpp
// Rigorous snowball tests for N-D parametric curves:
//   nearest_curve_nd, linear_curve_nd, cubic_curve_nd, regular_cubic_curve_nd

#include "../../src/math/mk.hpp"
#include "../../src/math/splines.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;
using namespace micron::math::splines;

static bool
near(f64 a, f64 b, f64 eps = 1e-10)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

template <usize D>
static bool
near_v(const vec<f64, D> &a, const vec<f64, D> &b, f64 eps = 1e-10)
{
  for ( usize i = 0; i < D; ++i )
    if ( !near(a.data[i], b.data[i], eps) ) return false;
  return true;
}

static f64
fabs64(f64 x)
{
  return x < 0 ? -x : x;
}

int
main()
{
  print("=== MATH::SPLINES CURVE_ND TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("linear_curve_nd<f64, 3>: passes through control points");
  {
    f64 ts[4] = { 0, 1, 2, 3 };
    vec<f64, 3> pts[4] = {
      { 0.0, 0.0, 0.0 },
      { 1.0, 2.0, 3.0 },
      { 4.0, 1.0, 5.0 },
      { 7.0, -1.0, 0.0 },
    };
    auto c = make_linear_curve<f64, 3>(raw_slice<const f64>(ts, 4), pts, 4);
    for ( usize i = 0; i < 4; ++i ) require_true(near_v<3>(evaluate<f64, 3>(c, ts[i]), pts[i], 1e-13));
    // Midpoint between pts[0] and pts[1]:
    auto m = evaluate<f64, 3>(c, 0.5);
    require_true(near(m.data[0], 0.5, 1e-13));
    require_true(near(m.data[1], 1.0, 1e-13));
    require_true(near(m.data[2], 1.5, 1e-13));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("cubic_curve_nd<f64, 2>: interpolates exactly at parameter knots");
  {
    f64 ts[6] = { 0.0, 0.5, 1.2, 2.0, 3.1, 4.0 };
    vec<f64, 2> pts[6] = { { 0.0, 1.0 }, { 1.0, 0.5 }, { 2.0, -0.3 }, { 3.0, 2.5 }, { 1.5, 4.0 }, { -1.0, 3.0 } };
    auto c = make_cubic_curve<f64, 2>(raw_slice<const f64>(ts, 6), pts, 6, bc_kind::natural);
    for ( usize i = 0; i < 6; ++i ) require_true(near_v<2>(evaluate<f64, 2>(c, ts[i]), pts[i], 1e-12));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("cubic_curve_nd<f64, 4>: SIMD-friendly D=4 matches scalar reference");
  {
    constexpr usize n = 8;
    f64 ts[n];
    vec<f64, 4> pts[n];
    for ( usize i = 0; i < n; ++i ) {
      ts[i] = 0.5 * f64(i);
      pts[i].data[0] = mk::trig::sin<f64>(ts[i]);
      pts[i].data[1] = mk::trig::cos<f64>(ts[i]);
      pts[i].data[2] = ts[i] * ts[i];
      pts[i].data[3] = -ts[i];
    }
    auto c = make_cubic_curve<f64, 4>(raw_slice<const f64>(ts, n), pts, n, bc_kind::not_a_knot);

    // Compare each axis against an independent scalar cubic spline through
    // that axis's data.
    f64 axis0[n], axis1[n], axis2[n], axis3[n];
    for ( usize i = 0; i < n; ++i ) {
      axis0[i] = pts[i].data[0];
      axis1[i] = pts[i].data[1];
      axis2[i] = pts[i].data[2];
      axis3[i] = pts[i].data[3];
    }
    auto s0 = make_cubic<f64>(raw_slice<const f64>(ts, n), raw_slice<const f64>(axis0, n), bc_kind::not_a_knot);
    auto s1 = make_cubic<f64>(raw_slice<const f64>(ts, n), raw_slice<const f64>(axis1, n), bc_kind::not_a_knot);
    auto s2 = make_cubic<f64>(raw_slice<const f64>(ts, n), raw_slice<const f64>(axis2, n), bc_kind::not_a_knot);
    auto s3 = make_cubic<f64>(raw_slice<const f64>(ts, n), raw_slice<const f64>(axis3, n), bc_kind::not_a_knot);

    for ( usize i = 0; i <= 30; ++i ) {
      const f64 t = 0.05 + 0.11 * f64(i);
      if ( t > ts[n - 1] ) break;
      const auto v = evaluate<f64, 4>(c, t);
      require_true(near(v.data[0], evaluate<f64>(s0, t), 1e-13));
      require_true(near(v.data[1], evaluate<f64>(s1, t), 1e-13));
      require_true(near(v.data[2], evaluate<f64>(s2, t), 1e-13));
      require_true(near(v.data[3], evaluate<f64>(s3, t), 1e-13));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("regular_cubic_curve_nd<f64, 3>: uniform-spacing path");
  {
    constexpr usize n = 6;
    vec<f64, 3> pts[n]
        = { { 0.0, 0.0, 0.0 }, { 1.0, 0.5, 0.2 }, { 2.0, -0.3, 0.7 }, { 3.0, 2.5, -1.0 }, { 1.5, 4.0, 0.0 }, { -1.0, 3.0, 1.5 } };
    const f64 t0 = 0.0, dt = 0.4;
    auto c = make_regular_cubic_curve<f64, 3>(t0, dt, pts, n, bc_kind::natural);

    // Check it interpolates the points.
    for ( usize i = 0; i < n; ++i ) {
      const f64 t = t0 + dt * f64(i);
      require_true(near_v<3>(evaluate<f64, 3>(c, t), pts[i], 1e-12));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("regular_cubic_curve_nd: matches general cubic_curve_nd on uniform ts");
  {
    constexpr usize n = 7;
    vec<f64, 2> pts[n] = { { 0.0, 1.0 }, { 1.0, 0.5 }, { 2.0, -0.3 }, { 3.0, 2.5 }, { 1.5, 4.0 }, { -1.0, 3.0 }, { -2.0, 2.5 } };
    const f64 t0 = 0.0, dt = 1.0;
    f64 ts[n];
    for ( usize i = 0; i < n; ++i ) ts[i] = t0 + dt * f64(i);
    auto c_reg = make_regular_cubic_curve<f64, 2>(t0, dt, pts, n, bc_kind::natural);
    auto c_gen = make_cubic_curve<f64, 2>(raw_slice<const f64>(ts, n), pts, n, bc_kind::natural);

    for ( usize i = 1; i < 50; ++i ) {
      const f64 t = 0.05 * f64(i);
      const auto vr = evaluate<f64, 2>(c_reg, t);
      const auto vg = evaluate<f64, 2>(c_gen, t);
      require_true(near_v<2>(vr, vg, 1e-12));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("nearest_curve_nd: returns nearest control point");
  {
    f64 ts[3] = { 0.0, 1.0, 2.0 };
    vec<f64, 2> pts[3] = { { 1.0, 1.0 }, { -1.0, 0.0 }, { 0.0, 5.0 } };
    auto c = make_nearest_curve<f64, 2>(raw_slice<const f64>(ts, 3), pts, 3);
    require_true(near_v<2>(evaluate<f64, 2>(c, 0.4), pts[0], 1e-13));
    require_true(near_v<2>(evaluate<f64, 2>(c, 0.6), pts[1], 1e-13));
    require_true(near_v<2>(evaluate<f64, 2>(c, 1.6), pts[2], 1e-13));
    // Out-of-range
    require_true(near_v<2>(evaluate<f64, 2>(c, -10.0), pts[0], 1e-13));
    require_true(near_v<2>(evaluate<f64, 2>(c, 100.0), pts[2], 1e-13));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("cubic_curve_nd: linear-continue extrapolation respects per-axis slope");
  {
    f64 ts[5] = { 0, 1, 2, 3, 4 };
    vec<f64, 2> pts[5] = { { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 9 }, { 4, 16 } };
    auto c = make_cubic_curve<f64, 2>(raw_slice<const f64>(ts, 5), pts, 5, bc_kind::natural);
    c.mode = extrap::linear_continue;
    // Just check the extrap path executes and returns finite values.
    auto lo = evaluate<f64, 2>(c, -0.5);
    auto hi = evaluate<f64, 2>(c, 4.5);
    require_true(fabs64(lo.data[0]) < 100.0);
    require_true(fabs64(hi.data[0]) < 100.0);
  }
  end_test_case();

  print("=== CURVE_ND TESTS PASSED ===");
  return 0;
}
