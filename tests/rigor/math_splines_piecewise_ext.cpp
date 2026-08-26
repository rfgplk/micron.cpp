// Additive 1-D spline families and batch/cursor invariants.

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
near(f64 a, f64 b, f64 tolerance = 1e-10)
{
  const f64 difference = a - b;
  return (difference < 0 ? -difference : difference) <= tolerance;
}

int
main()
{
  print("=== ADDITIVE PIECEWISE SPLINES ===");

  test_case("constant sides and extension mapping");
  {
    f64 x[3] = { 0, 1, 2 };
    f64 y[3] = { 10, 20, 30 };
    auto previous = make_constant<f64>({ x, 3 }, { y, 3 }, constant_side::previous);
    auto next = make_constant<f64>({ x, 3 }, { y, 3 }, constant_side::next);
    auto nearest = make_constant<f64>({ x, 3 }, { y, 3 });
    require_true(near(evaluate<f64>(previous, f64(0.75)), f64(10)));
    require_true(near(evaluate<f64>(next, f64(0.75)), f64(20)));
    require_true(near(evaluate<f64>(nearest, f64(0.75)), f64(20)));
    spline_cursor cursor{};
    require_true(near(evaluate<f64>(previous, f64(1), cursor), f64(20)));
    require_true(near(evaluate<f64>(next, f64(1), cursor), f64(20)));
    previous.mode = extension_mode::zero;
    require_true(near(evaluate<f64>(previous, f64(0)), f64(10)));
    require_true(near(evaluate<f64>(previous, f64(2)), f64(30)));
    require_true(near(evaluate<f64>(previous, f64(-0.01)), f64(0)));
    previous.mode = extension_mode::periodic;
    require_true(near(evaluate<f64>(previous, f64(2.75)), f64(10)));
    previous.mode = extension_mode::reflect;
    require_true(near(evaluate<f64>(previous, f64(2.25)), f64(20)));
  }
  end_test_case();

  test_case("extension modes preserve endpoint and derivative contracts");
  {
    f64 x[3] = { 0, 1, 2 };
    f64 y[3] = { 0, 1, 4 };
    auto quadratic = make_quadratic<f64>({ x, 3 }, { y, 3 }, quadratic_boundary::left_slope, f64(0));
    quadratic.mode = extension_mode::zero;
    require_true(near(evaluate<f64>(quadratic, f64(0)), f64(0)));
    require_true(near(evaluate<f64>(quadratic, f64(2)), f64(4)));
    require_true(near(evaluate<f64>(quadratic, f64(2.1)), f64(0)));
    require_true(near(derivative<f64>(quadratic, f64(2.1)), f64(0)));
    quadratic.mode = extension_mode::linear;
    require_true(near(evaluate<f64>(quadratic, f64(3)), f64(8)));
    require_true(near(derivative<f64>(quadratic, f64(3)), f64(4)));
    require_true(near(derivative<f64>(quadratic, f64(3), 2), f64(0)));
    quadratic.mode = extension_mode::polynomial;
    require_true(near(evaluate<f64>(quadratic, f64(-1)), f64(1)));
    require_true(near(derivative<f64>(quadratic, f64(-1)), f64(-2)));
    quadratic.mode = extension_mode::reflect;
    require_true(near(evaluate<f64>(quadratic, f64(-0.25)), f64(0.0625)));
    require_true(near(derivative<f64>(quadratic, f64(-0.25)), f64(-0.5)));

    auto cubic = make_cubic<f64>({ x, 3 }, { y, 3 });
    cubic.mode = extrap::clamp_to_endpoints;
    require_true(to_power_basis(cubic).mode == extension_mode::clamp);
    cubic.mode = extrap::linear_continue;
    require_true(to_power_basis(cubic).mode == extension_mode::linear);
    cubic.mode = extrap::error_value;
    require_true(to_power_basis(cubic).mode == extension_mode::zero);
  }
  end_test_case();

  test_case("quadratic and cubic Hermite reproduce polynomials");
  {
    f64 x[5] = { -1, 0, 1, 2, 3 };
    f64 y2[5], y3[5], d3[5];
    for ( usize i = 0; i < 5; ++i ) {
      y2[i] = x[i] * x[i] + f64(2) * x[i] + f64(3);
      y3[i] = x[i] * x[i] * x[i] - f64(2) * x[i] + f64(1);
      d3[i] = f64(3) * x[i] * x[i] - f64(2);
    }
    auto quadratic = make_quadratic<f64>({ x, 5 }, { y2, 5 }, quadratic_boundary::left_slope, f64(0));
    auto hermite = make_cubic_hermite<f64>({ x, 5 }, { y3, 5 }, { d3, 5 });
    for ( usize i = 0; i < 81; ++i ) {
      const f64 q = f64(-1) + f64(i) * f64(0.05);
      require_true(near(evaluate<f64>(quadratic, q), q * q + f64(2) * q + f64(3), f64(2e-12)));
      require_true(near(evaluate<f64>(hermite, q), q * q * q - f64(2) * q + f64(1), f64(2e-12)));
    }
  }
  end_test_case();

  test_case("quintic Hermite reproduces a quintic");
  {
    f64 x[4] = { -1, 0, 1, 2 };
    f64 y[4], d1[4], d2[4];
    for ( usize i = 0; i < 4; ++i ) {
      const f64 q = x[i];
      y[i] = q * q * q * q * q - f64(2) * q * q * q + q;
      d1[i] = f64(5) * q * q * q * q - f64(6) * q * q + f64(1);
      d2[i] = f64(20) * q * q * q - f64(12) * q;
    }
    auto spline = make_quintic_hermite<f64>({ x, 4 }, { y, 4 }, { d1, 4 }, { d2, 4 });
    for ( usize i = 0; i < 61; ++i ) {
      const f64 q = f64(-1) + f64(i) * f64(0.05);
      const f64 truth = q * q * q * q * q - f64(2) * q * q * q + q;
      require_true(near(evaluate<f64>(spline, q), truth, f64(2e-11)));
    }
  }
  end_test_case();

  test_case("Akima, Makima, cardinal, and Steffen interpolate knots");
  {
    f64 x[7] = { 0, 1, 2, 3, 4, 5, 6 };
    f64 y[7] = { 0, 1, f64(1.1), f64(1.2), 4, f64(4.1), f64(4.2) };
    auto akima = make_akima<f64>({ x, 7 }, { y, 7 });
    auto makima = make_akima<f64>({ x, 7 }, { y, 7 }, akima_kind::makima);
    auto cardinal = make_cardinal<f64>({ x, 7 }, { y, 7 }, f64(0.25));
    auto steffen = make_steffen<f64>({ x, 7 }, { y, 7 });
    for ( usize i = 0; i < 7; ++i ) {
      require_true(near(evaluate<f64>(akima, x[i]), y[i], f64(2e-12)));
      require_true(near(evaluate<f64>(makima, x[i]), y[i], f64(2e-12)));
      require_true(near(evaluate<f64>(cardinal, x[i]), y[i], f64(2e-12)));
      require_true(near(evaluate<f64>(steffen, x[i]), y[i], f64(2e-12)));
    }
    f64 previous = evaluate<f64>(steffen, x[0]);
    for ( usize i = 1; i <= 600; ++i ) {
      const f64 value = evaluate<f64>(steffen, f64(6) * f64(i) / f64(600));
      require_true(value >= previous - f64(2e-12));
      previous = value;
    }
  }
  end_test_case();

  test_case("periodic cubic closes through second derivative");
  {
    constexpr usize n = 9;
    f64 x[n], y[n];
    for ( usize i = 0; i < n; ++i ) {
      x[i] = f64(i) * f64(6.2831853071795864769) / f64(n - 1);
      y[i] = mk::trig::sin<f64>(x[i]);
    }
    auto periodic = make_periodic_cubic<f64>({ x, n }, { y, n });
    require_true(near(evaluate<f64>(periodic, f64(-0.2)), evaluate<f64>(periodic, x[n - 1] - f64(0.2)), f64(2e-12)));
    require_true(near(derivative<f64>(periodic, x[0], 1), derivative<f64>(periodic, x[n - 1], 1), f64(2e-12)));
    require_true(near(derivative<f64>(periodic, x[0], 2), derivative<f64>(periodic, x[n - 1], 2), f64(2e-12)));
  }
  end_test_case();

  test_case("power basis calculus, roots, and solve");
  {
    f64 breaks[2] = { 0, 4 };
    f64 coefficients[4] = { 0, -1, 0, 1 };
    auto polynomial = make_piecewise_polynomial<f64>({ breaks, 2 }, { coefficients, 4 }, 3);
    auto found = roots<f64>(polynomial);
    require_true(found.size() == 2);
    require_true(near(found[0], f64(0), f64(2e-11)));
    require_true(near(found[1], f64(1), f64(2e-11)));
    auto solved = solve<f64>(polynomial, f64(6));
    require_true(solved.size() == 1);
    require_true(near(evaluate<f64>(polynomial, solved[0]), f64(6), f64(2e-10)));
    require_true(near(integral<f64>(polynomial, f64(0), f64(2)), f64(2), f64(2e-12)));
  }
  end_test_case();

  test_case("sorted batch is independent of stale cursor");
  {
    f64 x[8], y[8];
    for ( usize i = 0; i < 8; ++i ) {
      x[i] = f64(i);
      y[i] = x[i] * x[i];
    }
    auto spline = make_cubic<f64>({ x, 8 }, { y, 8 });
    spline.last_hit = 6;
    f64 query[8] = { f64(0.1), f64(0.2), f64(0.8), f64(1.1), f64(2.3), f64(3.7), f64(5.5), f64(6.8) };
    f64 output[8];
    evaluate<f64>(spline, query, output, 8);
    for ( usize i = 0; i < 8; ++i ) require_true(near(output[i], evaluate_stateless(spline, query[i]), f64(2e-12)));
    const usize before = spline.last_hit;
    (void)evaluate_stateless(spline, f64(0.3));
    require_true(spline.last_hit == before);
    alignas(64) f64 streaming[8];
    evaluate_streaming<f64>(spline, query, streaming, 8);
    require_true(spline.last_hit == before);
    for ( usize i = 0; i < 8; ++i ) require_true(near(output[i], streaming[i], f64(2e-12)));

    auto linear = make_linear<f64>({ x, 8 }, { y, 8 });
    evaluate_streaming<f64>(linear, query, streaming, 8);
    for ( usize i = 0; i < 8; ++i ) require_true(near(streaming[i], evaluate_stateless(linear, query[i]), f64(2e-12)));
  }
  end_test_case();

  print("=== ADDITIVE PIECEWISE SPLINES PASSED ===");
  return 1;
}
