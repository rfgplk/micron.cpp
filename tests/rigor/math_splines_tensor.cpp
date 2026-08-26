// Tensor-product spline value and derivative properties.

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
  print("=== TENSOR SPLINES ===");

  test_case("bilinear scalar surface value, gradient, and Hessian");
  {
    f64 u_knots[4] = { 0, 0, 1, 1 };
    f64 v_knots[4] = { 0, 0, 1, 1 };
    raw_slice<const f64> knots[2] = { { u_knots, 4 }, { v_knots, 4 } };
    usize shape[2] = { 2, 2 };
    u32 degree[2] = { 1, 1 };
    f64 control[4] = { 0, 2, 1, 3 };      // f(u,v) = u + 2v
    auto surface = make_tensor_bspline<f64, f64, 2>(knots, shape, degree, { control, 4 });
    f64 point[2] = { f64(0.25), f64(0.5) };
    require_true(near(evaluate<f64, f64, 2>(surface, point), f64(1.25), f64(2e-12)));
    const auto grad = gradient<f64, 2>(surface, point);
    require_true(near(grad[0], f64(1), f64(2e-12)));
    require_true(near(grad[1], f64(2), f64(2e-12)));
    const auto hess = hessian<f64, 2>(surface, point);
    for ( usize i = 0; i < 4; ++i ) require_true(near(hess[i], f64(0), f64(2e-12)));

    f64 outside[2] = { f64(-0.5), f64(0.5) };
    surface.mode[0] = extension_mode::zero;
    require_true(near(evaluate<f64, f64, 2>(surface, outside), f64(0)));
    surface.mode[0] = extension_mode::clamp;
    require_true(near(evaluate<f64, f64, 2>(surface, outside), f64(1)));
    const u32 du[2] = { 1, 0 };
    require_true(near(partial_derivative<f64, f64, 2>(surface, outside, du), f64(0)));
    surface.mode[0] = extension_mode::linear;
    require_true(near(evaluate<f64, f64, 2>(surface, outside), f64(0.5)));
    surface.mode[0] = extension_mode::polynomial;
    require_true(near(evaluate<f64, f64, 2>(surface, outside), f64(0.5)));
    outside[0] = f64(1.25);
    surface.mode[0] = extension_mode::reflect;
    require_true(near(evaluate<f64, f64, 2>(surface, outside), f64(1.75)));
    require_true(near(partial_derivative<f64, f64, 2>(surface, outside, du), f64(-1)));
  }
  end_test_case();

  test_case("unit-weight tensor NURBS equals B-spline");
  {
    f64 k0[6] = { 0, 0, 0, 1, 1, 1 };
    f64 k1[6] = { 0, 0, 0, 1, 1, 1 };
    raw_slice<const f64> knots[2] = { { k0, 6 }, { k1, 6 } };
    usize shape[2] = { 3, 3 };
    u32 degree[2] = { 2, 2 };
    f64 control[9], weights[9];
    for ( usize i = 0; i < 9; ++i ) {
      control[i] = f64(i) * f64(i) - f64(3) * f64(i);
      weights[i] = 1;
    }
    auto spline = make_tensor_bspline<f64, f64, 2>(knots, shape, degree, { control, 9 });
    auto rational = make_tensor_nurbs<f64, f64, 2>(knots, shape, degree, { control, 9 }, { weights, 9 });
    for ( usize i = 0; i <= 10; ++i )
      for ( usize j = 0; j <= 10; ++j ) {
        f64 point[2] = { f64(i) / f64(10), f64(j) / f64(10) };
        require_true(near(evaluate<f64, f64, 2>(spline, point), evaluate<f64, f64, 2>(rational, point), f64(2e-11)));
      }
  }
  end_test_case();

  test_case("planar vector surface normal");
  {
    f64 k0[4] = { 0, 0, 1, 1 };
    f64 k1[4] = { 0, 0, 1, 1 };
    raw_slice<const f64> knots[2] = { { k0, 4 }, { k1, 4 } };
    usize shape[2] = { 2, 2 };
    u32 degree[2] = { 1, 1 };
    vec<f64, 3> control[4] = { { 0, 0, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 1, 0 } };
    auto surface = make_tensor_bspline<f64, vec<f64, 3>, 2>(knots, shape, degree, { control, 4 });
    f64 point[2] = { f64(0.4), f64(0.7) };
    const auto normal = surface_normal<f64>(surface, point);
    require_true(near(normal[0], f64(0), f64(2e-12)));
    require_true(near(normal[1], f64(0), f64(2e-12)));
    require_true(near(normal[2], f64(1), f64(2e-12)));
  }
  end_test_case();

  test_case("trilinear manifold and batch evaluation");
  {
    f64 k0[4] = { 0, 0, 1, 1 };
    f64 k1[4] = { 0, 0, 1, 1 };
    f64 k2[4] = { 0, 0, 1, 1 };
    raw_slice<const f64> knots[3] = { { k0, 4 }, { k1, 4 }, { k2, 4 } };
    usize shape[3] = { 2, 2, 2 };
    u32 degree[3] = { 1, 1, 1 };
    f64 control[8];
    for ( usize i = 0; i < 2; ++i )
      for ( usize j = 0; j < 2; ++j )
        for ( usize k = 0; k < 2; ++k ) control[(i * 2 + j) * 2 + k] = f64(i) + f64(2 * j) + f64(3 * k);
    auto volume = make_tensor_bspline<f64, f64, 3>(knots, shape, degree, { control, 8 });
    f64 points[6] = { f64(0.25), f64(0.5), f64(0.75), f64(0.8), f64(0.1), f64(0.2) };
    f64 output[2]{};
    evaluate<f64, f64, 3>(volume, points, output, 2);
    require_true(near(output[0], f64(3.5), f64(2e-12)));
    require_true(near(output[1], f64(1.6), f64(2e-12)));
  }
  end_test_case();

  print("=== TENSOR SPLINES PASSED ===");
  return 1;
}
