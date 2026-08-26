// B-spline/NURBS/Bezier, packed, closed, and fitting curve tests.

#include "../../src/math/splines.hpp"
#include "../../src/std.hpp"
#include "../snowball/snowball.hpp"

using namespace micron;
using namespace micron::math;
using namespace micron::math::splines;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

static bool
near(f64 a, f64 b, f64 tolerance = 1e-9)
{
  const f64 difference = a - b;
  return (difference < 0 ? -difference : difference) <= tolerance;
}

int
main()
{
  print("=== RATIONAL AND EXTENDED CURVES ===");

  test_case("NURBS with unit weights equals B-spline curve");
  {
    f64 knots[9] = { 0, 0, 0, 0, f64(0.5), 1, 1, 1, 1 };
    vec<f64, 3> ctrl[5] = { { 0, 0, 0 }, { 1, 2, 0 }, { 2, 0, 1 }, { 3, 1, 2 }, { 4, 0, 0 } };
    f64 weights[5] = { 1, 1, 1, 1, 1 };
    auto spline = make_bspline_curve_from_ctrl<f64, 3>({ knots, 9 }, ctrl, 5, 3);
    auto rational = make_nurbs_curve<f64, 3>({ knots, 9 }, ctrl, { weights, 5 }, 5, 3);
    for ( usize i = 0; i <= 100; ++i ) {
      const f64 t = f64(i) / f64(100);
      const auto a = evaluate<f64, 3>(spline, t);
      const auto b = evaluate<f64, 3>(rational, t);
      for ( usize d = 0; d < 3; ++d ) require_true(near(a[d], b[d], f64(2e-12)));
    }
  }
  end_test_case();

  test_case("curve extension modes and direct B-spline derivatives");
  {
    f64 knots[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };
    vec<f64, 2> ctrl[4] = { { 0, 0 }, { 1, 2 }, { 2, 1 }, { 3, 3 } };
    f64 weights[4] = { 1, 1, 1, 1 };
    auto curve = make_bspline_curve_from_ctrl<f64, 2>({ knots, 8 }, ctrl, 4, 3);
    auto rational = make_nurbs_curve<f64, 2>({ knots, 8 }, ctrl, { weights, 4 }, 4, 3);
    const auto derivative_curve = derivative_spline(curve);
    for ( usize i = 1; i < 100; ++i ) {
      const f64 t = f64(i) / f64(100);
      const auto direct = derivative(curve, t);
      const auto materialized = evaluate(derivative_curve, t);
      for ( usize d = 0; d < 2; ++d ) require_true(near(direct[d], materialized[d], f64(2e-11)));
    }

    curve.mode = extension_mode::zero;
    require_true(near(evaluate(curve, f64(0))[0], f64(0)));
    require_true(near(evaluate(curve, f64(1))[0], f64(3)));
    require_true(near(evaluate(curve, f64(1.1))[0], f64(0)));
    curve.mode = extension_mode::linear;
    const auto edge = evaluate(curve, f64(1));
    const auto slope = derivative(curve, f64(1));
    const auto continued = evaluate(curve, f64(1.25));
    for ( usize d = 0; d < 2; ++d ) require_true(near(continued[d], edge[d] + f64(0.25) * slope[d], f64(2e-12)));
    curve.mode = extension_mode::reflect;
    const auto reflected = derivative(curve, f64(1.2));
    const auto inside = derivative(curve, f64(0.8));
    for ( usize d = 0; d < 2; ++d ) require_true(near(reflected[d], -inside[d], f64(2e-11)));

    rational.mode = extension_mode::linear;
    const auto rational_edge = evaluate(rational, f64(1));
    const auto rational_slope = derivative(rational, f64(1));
    const auto rational_continued = evaluate(rational, f64(1.25));
    for ( usize d = 0; d < 2; ++d ) require_true(near(rational_continued[d], rational_edge[d] + f64(0.25) * rational_slope[d], f64(2e-12)));
  }
  end_test_case();

  test_case("cubic curve derivatives follow endpoint extension modes");
  {
    f64 knots[5] = { 0, f64(0.25), f64(0.5), f64(0.75), 1 };
    vec<f64, 2> points[5] = { { 0, 0 }, { 1, 2 }, { 2, 1 }, { 3, 3 }, { 4, 2 } };
    auto curve = make_cubic_curve<f64, 2>({ knots, 5 }, points, 5, bc_kind::not_a_knot);
    const auto left = derivative(curve, knots[0]);
    const auto right = derivative(curve, knots[4]);

    curve.mode = extrap::linear_continue;
    const auto left_outside = derivative(curve, f64(-2));
    const auto right_outside = derivative(curve, f64(3));
    const auto left_second = derivative(curve, f64(-2), 2);
    const auto right_second = derivative(curve, f64(3), 2);
    for ( usize d = 0; d < 2; ++d ) {
      require_true(near(left_outside[d], left[d], f64(2e-12)));
      require_true(near(right_outside[d], right[d], f64(2e-12)));
      require_true(near(left_second[d], f64(0)) && near(right_second[d], f64(0)));
    }

    curve.mode = extrap::clamp_to_endpoints;
    const auto clamped = derivative(curve, f64(-1));
    curve.mode = extrap::error_value;
    const auto invalid = derivative(curve, f64(2));
    for ( usize d = 0; d < 2; ++d ) require_true(near(clamped[d], f64(0)) && near(invalid[d], f64(0)));
  }
  end_test_case();

  test_case("quadratic NURBS represents a quarter circle");
  {
    f64 knots[6] = { 0, 0, 0, 1, 1, 1 };
    vec<f64, 2> ctrl[3] = { { 1, 0 }, { 1, 1 }, { 0, 1 } };
    const f64 root_half = f64(math::fsqrt(f64(0.5)));
    f64 weights[3] = { 1, root_half, 1 };
    auto circle = make_nurbs_curve<f64, 2>({ knots, 6 }, ctrl, { weights, 3 }, 3, 2);
    const auto middle = evaluate<f64, 2>(circle, f64(0.5));
    require_true(near(middle[0], root_half, f64(2e-12)));
    require_true(near(middle[1], root_half, f64(2e-12)));
    const f64 h = f64(1e-6);
    const auto analytic = derivative<f64, 2>(circle, f64(0.37));
    const auto plus = evaluate<f64, 2>(circle, f64(0.37) + h);
    const auto minus = evaluate<f64, 2>(circle, f64(0.37) - h);
    for ( usize d = 0; d < 2; ++d ) require_true(near(analytic[d], (plus[d] - minus[d]) / (h + h), f64(2e-6)));
  }
  end_test_case();

  test_case("Bezier and rational Bezier agree for unit weights");
  {
    vec<f64, 3> ctrl[4] = { { 0, 0, 0 }, { 1, 2, 0 }, { 2, 2, 1 }, { 3, 0, 1 } };
    f64 weights[4] = { 1, 1, 1, 1 };
    auto a = make_bezier_curve<f64, 3>(ctrl, 4);
    auto b = make_rational_bezier_curve<f64, 3>(ctrl, { weights, 4 }, 4);
    for ( usize i = 0; i <= 50; ++i ) {
      const f64 t = f64(i) / f64(50);
      const auto av = evaluate<f64, 3>(a, t);
      const auto bv = evaluate<f64, 3>(b, t);
      for ( usize d = 0; d < 3; ++d ) require_true(near(av[d], bv[d], f64(2e-12)));
    }
    a.mode = extension_mode::linear;
    b.mode = extension_mode::linear;
    const auto av = evaluate<f64, 3>(a, f64(1.25));
    const auto bv = evaluate<f64, 3>(b, f64(1.25));
    for ( usize d = 0; d < 3; ++d ) require_true(near(av[d], bv[d], f64(2e-12)));
  }
  end_test_case();

  test_case("packed D=6 cubic matches ABI-frozen curve");
  {
    constexpr usize n = 12;
    f64 t[n];
    vec<f64, 6> points[n]{};
    for ( usize i = 0; i < n; ++i ) {
      t[i] = f64(i) * f64(0.2);
      for ( usize d = 0; d < 6; ++d ) points[i][d] = f64(d + 1) * t[i] * t[i] + f64(i & 1) * f64(0.1);
    }
    auto curve = make_cubic_curve<f64, 6>({ t, n }, points, n, bc_kind::not_a_knot);
    auto packed = pack<f64, 6>(curve);
    require_true(packed.coeff.size() == curve.seg.size() * 24);
    for ( usize i = 0; i <= 200; ++i ) {
      const f64 q = t[0] + (t[n - 1] - t[0]) * f64(i) / f64(200);
      const auto a = evaluate<f64, 6>(curve, q);
      const auto b = evaluate<f64, 6>(packed, q);
      for ( usize d = 0; d < 6; ++d ) require_true(near(a[d], b[d], f64(3e-12)));
    }
  }
  end_test_case();

  test_case("closed cubic and closed NURBS wrap at the seam");
  {
    f64 t[5] = { 0, 1, 2, 3, 4 };
    vec<f64, 2> points[5] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 }, { 1, 0 } };
    auto cubic = make_closed_cubic_curve<f64, 2>({ t, 5 }, points, 5);
    const auto before = evaluate<f64, 2>(cubic, f64(-0.2));
    const auto wrapped = evaluate<f64, 2>(cubic, f64(3.8));
    require_true(near(before[0], wrapped[0], f64(2e-12)) && near(before[1], wrapped[1], f64(2e-12)));

    f64 weights[4] = { 1, 1, 1, 1 };
    auto closed = make_closed_nurbs_curve<f64, 2>(points, { weights, 4 }, 4, 2);
    const auto first = evaluate<f64, 2>(closed, f64(0.25));
    const auto repeat = evaluate<f64, 2>(closed, f64(4.25));
    require_true(near(first[0], repeat[0], f64(2e-12)) && near(first[1], repeat[1], f64(2e-12)));
  }
  end_test_case();

  test_case("basis calculus, refinement, design matrix, and LSQ");
  {
    constexpr usize n = 33;
    f64 x[n], y[n];
    for ( usize i = 0; i < n; ++i ) {
      x[i] = f64(i) / f64(n - 1);
      y[i] = f64(1) + f64(2) * x[i] - f64(0.5) * x[i] * x[i];
    }
    auto knots = make_uniform_clamped_knots<f64>(8, 3, f64(0), f64(1));
    build_info<f64> info{};
    auto fitted = make_lsq_bspline<f64>({ x, n }, { y, n }, { knots.data(), knots.size() }, 3, {}, lsq_method::householder_qr, &info);
    require_true(info.status == build_status::ok);
    for ( usize i = 0; i < n; ++i ) require_true(near(evaluate<f64>(fitted, x[i]), y[i], f64(2e-11)));
    auto matrix = design_matrix<f64>(fitted, { x, n });
    for ( usize row = 0; row < n; ++row ) {
      f64 sum = 0;
      for ( u32 index = matrix.outer[row]; index < matrix.outer[row + 1]; ++index ) sum += matrix.values[index];
      require_true(near(sum, f64(1), f64(2e-12)));
    }
    auto refined = insert_knot<f64>(fitted, f64(0.45), 2, &info);
    require_true(info.status == build_status::ok);
    for ( usize i = 0; i <= 100; ++i ) {
      const f64 q = f64(i) / f64(100);
      require_true(near(evaluate<f64>(fitted, q), evaluate<f64>(refined, q), f64(3e-11)));
    }
    auto pieces = split<f64>(fitted, f64(0.45), &info);
    require_true(info.status == build_status::ok);
    for ( usize i = 0; i <= 100; ++i ) {
      const f64 q = f64(i) / f64(100);
      const f64 piece = q <= f64(0.45) ? evaluate<f64>(pieces.left, q) : evaluate<f64>(pieces.right, q);
      require_true(near(evaluate<f64>(fitted, q), piece, f64(3e-11)));
    }
    const auto anti = antiderivative_spline<f64>(fitted);
    const auto recovered = derivative_spline<f64>(anti);
    for ( usize i = 1; i < 100; ++i ) {
      const f64 q = f64(i) / f64(100);
      require_true(near(evaluate<f64>(fitted, q), evaluate<f64>(recovered, q), f64(3e-11)));
    }
  }
  end_test_case();

  test_case("curve curvature and packed extrapolation");
  {
    f64 t[4] = { 0, 1, 2, 3 };
    vec<f64, 2> parabola[4] = { { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 9 } };
    auto curve = make_cubic_curve<f64, 2>({ t, 4 }, parabola, 4, bc_kind::clamped, { 1, 0 }, { 1, 6 });
    const f64 q = f64(1.25);
    const f64 expected = f64(2) / (f64(math::fsqrt(f64(1) + f64(4) * q * q)) * (f64(1) + f64(4) * q * q));
    require_true(near(curvature<f64, 2>(curve, q), expected, f64(2e-10)));

    auto packed = pack<f64, 2>(curve);
    for ( const f64 outside : { f64(-0.5), f64(3.5) } ) {
      const auto a = evaluate<f64, 2>(curve, outside);
      const auto b = evaluate<f64, 2>(packed, outside);
      require_true(near(a[0], b[0], f64(2e-12)) && near(a[1], b[1], f64(2e-12)));
    }
  }
  end_test_case();

  test_case("uniform arc-length sampling reuses a cumulative table");
  {
    f64 t[4] = { 0, 1, 2, 3 };
    vec<f64, 2> points[4] = { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 } };
    auto curve = make_cubic_curve<f64, 2>({ t, 4 }, points, 4);
    const auto samples = sample_uniform_arc_length<f64, 2>(curve, 17, 32);
    require_true(samples.size() == 17);
    for ( usize i = 0; i < samples.size(); ++i ) {
      require_true(near(samples[i][0], f64(3) * f64(i) / f64(16), f64(2e-11)));
      require_true(near(samples[i][1], f64(0), f64(2e-12)));
    }
  }
  end_test_case();

  test_case("multi-coordinate interpolation and LSQ share their factorization");
  {
    constexpr usize n = 25;
    f64 t[n];
    vec<f64, 3> points[n]{};
    f64 axis[3][n]{};
    for ( usize i = 0; i < n; ++i ) {
      t[i] = f64(i) / f64(n - 1);
      points[i] = { f64(1) + t[i], t[i] * t[i] - f64(0.25) * t[i], f64(2) - t[i] * t[i] * t[i] };
      for ( usize d = 0; d < 3; ++d ) axis[d][i] = points[i][d];
    }
    build_info<f64> info{};
    auto interpolated = make_bspline_curve_interpolating<f64, 3>({ t, n }, points, n, 3, &info);
    require_true(info.status == build_status::ok);
    bspline<f64> scalar[3];
    for ( usize d = 0; d < 3; ++d ) scalar[d] = make_bspline_interpolating<f64>({ t, n }, { axis[d], n }, 3);
    for ( usize i = 0; i <= 100; ++i ) {
      const f64 q = f64(i) / f64(100);
      const auto value = evaluate<f64, 3>(interpolated, q);
      for ( usize d = 0; d < 3; ++d ) require_true(near(value[d], evaluate<f64>(scalar[d], q), f64(3e-11)));
    }

    const auto knots = make_uniform_clamped_knots<f64>(9, 3, f64(0), f64(1));
    auto fitted
        = make_lsq_bspline_curve<f64, 3>({ t, n }, points, n, { knots.data(), knots.size() }, 3, {}, lsq_method::householder_qr, &info);
    require_true(info.status == build_status::ok);
    for ( usize d = 0; d < 3; ++d ) scalar[d] = make_lsq_bspline<f64>({ t, n }, { axis[d], n }, { knots.data(), knots.size() }, 3);
    for ( usize i = 0; i <= 100; ++i ) {
      const f64 q = f64(i) / f64(100);
      const auto value = evaluate<f64, 3>(fitted, q);
      for ( usize d = 0; d < 3; ++d ) require_true(near(value[d], evaluate<f64>(scalar[d], q), f64(3e-11)));
    }

    auto smoothed = make_smoothing_curve<f64, 3>({ t, n }, points, n, {}, f64(0.025), &info);
    require_true(info.status == build_status::ok);
    cubic_spline_1d<f64> scalar_smoothing[3];
    for ( usize d = 0; d < 3; ++d ) scalar_smoothing[d] = make_smoothing<f64>({ t, n }, { axis[d], n }, {}, f64(0.025));
    for ( usize i = 0; i <= 100; ++i ) {
      const f64 q = f64(i) / f64(100);
      const auto value = evaluate<f64, 3>(smoothed, q);
      for ( usize d = 0; d < 3; ++d ) require_true(near(value[d], evaluate<f64>(scalar_smoothing[d], q), f64(5e-11)));
    }
    f64 invalid_weights[n];
    for ( usize i = 0; i < n; ++i ) invalid_weights[i] = 1;
    invalid_weights[4] = 0;
    (void)make_smoothing_curve<f64, 3>({ t, n }, points, n, { invalid_weights, n }, f64(0.025), &info);
    require_true(info.status == build_status::invalid_argument);
  }
  end_test_case();

  print("=== RATIONAL AND EXTENDED CURVES PASSED ===");
  return 1;
}
