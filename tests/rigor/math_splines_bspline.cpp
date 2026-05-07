// math_splines_bspline.cpp
// Rigorous snowball tests for math::splines::bspline.

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

static f64
fabs64(f64 x)
{
  return x < 0 ? -x : x;
}

int
main()
{
  print("=== MATH::SPLINES::BSPLINE TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline degree=3: partition of unity (all P_i = 1 ⇒ S(x) = 1)");
  {
    auto knots = make_uniform_clamped_knots<f64>(8, 3, 0.0, 1.0);
    f64 ctrl[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    raw_slice<const f64> ks(knots.data(), knots.size());
    raw_slice<const f64> cs(ctrl, 8);
    auto s = make_bspline_from_ctrl<f64>(ks, cs, 3);
    for ( usize i = 0; i <= 100; ++i ) {
      const f64 t = 0.01 * f64(i);
      require_true(near(evaluate<f64>(s, t), 1.0, 1e-13));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline degree=3: clamped endpoints reproduce first/last ctrl");
  {
    auto knots = make_uniform_clamped_knots<f64>(6, 3, 0.0, 1.0);
    f64 ctrl[6] = { 0.0, 1.5, -2.0, 3.7, -0.4, 5.0 };
    raw_slice<const f64> ks(knots.data(), knots.size());
    raw_slice<const f64> cs(ctrl, 6);
    auto s = make_bspline_from_ctrl<f64>(ks, cs, 3);
    require_true(near(evaluate<f64>(s, 0.0), ctrl[0], 1e-13));
    require_true(near(evaluate<f64>(s, 1.0), ctrl[5], 1e-13));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline degree=1: piecewise-linear interpolation through ctrl points");
  {
    // Uniform clamped degree-1 reduces to linear interp through ctrl points
    // (each ctrl point lies on the curve at the associated knot).
    auto knots = make_uniform_clamped_knots<f64>(5, 1, 0.0, 4.0);
    f64 ctrl[5] = { 0.0, 1.0, 4.0, 9.0, 16.0 };
    raw_slice<const f64> ks(knots.data(), knots.size());
    raw_slice<const f64> cs(ctrl, 5);
    auto s = make_bspline_from_ctrl<f64>(ks, cs, 1);
    // At each interior knot the value is the corresponding control point.
    require_true(near(evaluate<f64>(s, 0.0), 0.0, 1e-13));
    require_true(near(evaluate<f64>(s, 1.0), 1.0, 1e-13));
    require_true(near(evaluate<f64>(s, 2.0), 4.0, 1e-13));
    require_true(near(evaluate<f64>(s, 3.0), 9.0, 1e-13));
    require_true(near(evaluate<f64>(s, 4.0), 16.0, 1e-13));
    // Midpoints are linear interpolations.
    require_true(near(evaluate<f64>(s, 0.5), 0.5, 1e-13));
    require_true(near(evaluate<f64>(s, 2.5), 6.5, 1e-13));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline derivative_spline: matches finite-difference of S");
  {
    auto knots = make_uniform_clamped_knots<f64>(8, 3, 0.0, 1.0);
    f64 ctrl[8] = { 0.0, 0.5, -0.3, 1.2, 0.8, -0.5, 1.4, 0.0 };
    raw_slice<const f64> ks(knots.data(), knots.size());
    raw_slice<const f64> cs(ctrl, 8);
    auto s = make_bspline_from_ctrl<f64>(ks, cs, 3);
    auto sd = derivative_spline<f64>(s);
    require_true(sd.degree == 2);
    // Check at several non-knot positions.
    const f64 h = 1e-6;
    for ( usize i = 1; i < 10; ++i ) {
      const f64 t = 0.05 + 0.09 * f64(i);
      const f64 d_analytic = evaluate<f64>(sd, t);
      const f64 d_fd = (evaluate<f64>(s, t + h) - evaluate<f64>(s, t - h)) / (2.0 * h);
      require_true(fabs64(d_analytic - d_fd) < 1e-7);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline degree=2: continuous and bounded between ctrl extremes");
  {
    auto knots = make_uniform_clamped_knots<f64>(7, 2, 0.0, 1.0);
    f64 ctrl[7] = { 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0 };
    raw_slice<const f64> ks(knots.data(), knots.size());
    raw_slice<const f64> cs(ctrl, 7);
    auto s = make_bspline_from_ctrl<f64>(ks, cs, 2);
    // For convex combination of [0, 1] control points the curve must lie in [0, 1].
    f64 lo = 1e9, hi = -1e9;
    for ( usize i = 0; i <= 200; ++i ) {
      const f64 t = 0.005 * f64(i);
      const f64 v = evaluate<f64>(s, t);
      if ( v < lo ) lo = v;
      if ( v > hi ) hi = v;
    }
    require_true(lo >= -1e-13);
    require_true(hi <= 1.0 + 1e-13);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline: build_info reports invalid_argument on bad sizes");
  {
    f64 knots[5] = { 0, 0, 0, 1, 1 };     // wrong size for n_ctrl=4 deg=3 (need 8)
    f64 ctrl[4] = { 0, 1, 2, 3 };
    build_info<f64> info{};
    auto s = make_bspline_from_ctrl<f64>(raw_slice<const f64>(knots, 5),
                                          raw_slice<const f64>(ctrl, 4), 3, &info);
    require_true(info.status == build_status::size_mismatch);
    // Returned struct should be empty (knots / ctrl not populated).
    require_true(s.knots.size() == 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline: degree 0 rejected (invalid_argument)");
  {
    f64 knots[3] = { 0, 0, 1 };
    f64 ctrl[2] = { 1, 2 };
    build_info<f64> info{};
    auto s = make_bspline_from_ctrl<f64>(raw_slice<const f64>(knots, 3),
                                          raw_slice<const f64>(ctrl, 2), 0, &info);
    require_true(info.status == build_status::invalid_argument);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline degree=3: out-of-range queries clamp to endpoints");
  {
    auto knots = make_uniform_clamped_knots<f64>(6, 3, 0.0, 1.0);
    f64 ctrl[6] = { 1, 2, 3, 4, 5, 6 };
    auto s = make_bspline_from_ctrl<f64>(raw_slice<const f64>(knots.data(), knots.size()),
                                          raw_slice<const f64>(ctrl, 6), 3);
    require_true(near(evaluate<f64>(s, -10.0), 1.0, 1e-13));
    require_true(near(evaluate<f64>(s, 10.0), 6.0, 1e-13));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline interpolation: degree=3 passes through every data point");
  {
    constexpr usize n = 8;
    f64 xs[n], ys[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = 0.5 * f64(i);
      ys[i] = mk::trig::sin<f64>(xs[i]);
    }
    build_info<f64> info{};
    auto s = make_bspline_interpolating<f64>(raw_slice<const f64>(xs, n),
                                              raw_slice<const f64>(ys, n), 3, &info);
    require_true(info.status == build_status::ok);
    require_true(s.degree == 3);
    require_true(s.ctrl.size() == n);
    require_true(s.knots.size() == n + 4);
    for ( usize i = 0; i < n; ++i ) require_true(near(evaluate<f64>(s, xs[i]), ys[i], 1e-10));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline interpolation: degree=2 passes through every data point");
  {
    constexpr usize n = 7;
    f64 xs[n] = { 0.0, 0.7, 1.5, 2.4, 3.1, 4.0, 5.0 };
    f64 ys[n] = { 1.0, -0.5, 2.3, 0.0, -1.2, 3.7, 2.0 };
    auto s = make_bspline_interpolating<f64>(raw_slice<const f64>(xs, n),
                                              raw_slice<const f64>(ys, n), 2);
    for ( usize i = 0; i < n; ++i ) require_true(near(evaluate<f64>(s, xs[i]), ys[i], 1e-10));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline interpolation: degree=3 reproduces a smooth function");
  {
    // y = e^{-x²} on [-1.5, 1.5]
    constexpr usize n = 24;
    f64 xs[n], ys[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = -1.5 + 3.0 * f64(i) / f64(n - 1);
      ys[i] = mk::exp_ns::exp<f64>(-xs[i] * xs[i]);
    }
    auto s = make_bspline_interpolating<f64>(raw_slice<const f64>(xs, n),
                                              raw_slice<const f64>(ys, n), 3);
    // Sampled max-error on a fine grid: cubic B-spline interpolation is
    // O(h^4) for a smooth function; 24 points over a span of 3 ⇒ h ≈ 0.13,
    // h^4 ≈ 3e-4.  Allow 5e-3 to absorb the f^(4)/24 prefactor.
    f64 worst = 0.0;
    for ( usize i = 0; i <= 200; ++i ) {
      const f64 x = xs[0] + (xs[n - 1] - xs[0]) * f64(i) / 200.0;
      const f64 truth = mk::exp_ns::exp<f64>(-x * x);
      const f64 e = fabs64(evaluate<f64>(s, x) - truth);
      if ( e > worst ) worst = e;
    }
    require_true(worst < 5e-3);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline interpolation: rejects non-monotonic xs");
  {
    f64 xs[5] = { 0, 1, 0.5, 2, 3 };     // non-monotone
    f64 ys[5] = { 1, 2, 3, 4, 5 };
    build_info<f64> info{};
    auto s = make_bspline_interpolating<f64>(raw_slice<const f64>(xs, 5),
                                              raw_slice<const f64>(ys, 5), 3, &info);
    require_true(info.status == build_status::non_monotonic_x);
    require_true(s.knots.size() == 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("bspline interpolation: too few points");
  {
    f64 xs[3] = { 0, 1, 2 };
    f64 ys[3] = { 0, 1, 4 };
    build_info<f64> info{};
    auto s = make_bspline_interpolating<f64>(raw_slice<const f64>(xs, 3),
                                              raw_slice<const f64>(ys, 3), 3, &info);
    require_true(info.status == build_status::too_few_points);
  }
  end_test_case();

  print("=== BSPLINE TESTS PASSED ===");
  return 1;
}
