// Fixed-seed fuzz oracles for additive spline families and lookup kernels.

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

static u64 fuzz_state = 0xA17E5C4D39B206F1ULL;

static u64
next_random() noexcept
{
  fuzz_state ^= fuzz_state >> 12;
  fuzz_state ^= fuzz_state << 25;
  fuzz_state ^= fuzz_state >> 27;
  return fuzz_state * 2685821657736338717ULL;
}

static f64
unit_random() noexcept
{
  return f64(next_random() >> 11) * 0x1.0p-53;
}

static f64
signed_random() noexcept
{
  return f64(2) * unit_random() - f64(1);
}

static f64
absolute(f64 value) noexcept
{
  return value < 0 ? -value : value;
}

static bool
near(f64 actual, f64 expected, f64 tolerance = 1e-10) noexcept
{
  return absolute(actual - expected) <= tolerance * (f64(1) + absolute(expected));
}

static f64
polynomial(const f64 *coefficients, usize count, f64 x) noexcept
{
  f64 out = coefficients[count - 1];
  for ( usize i = count - 1; i-- > 0; ) out = out * x + coefficients[i];
  return out;
}

static f64
polynomial_derivative(const f64 *coefficients, usize count, f64 x) noexcept
{
  f64 out = f64(count - 1) * coefficients[count - 1];
  for ( usize i = count - 1; i-- > 1; ) out = out * x + f64(i) * coefficients[i];
  return out;
}

static f64
polynomial_second_derivative(const f64 *coefficients, usize count, f64 x) noexcept
{
  f64 out = f64((count - 1) * (count - 2)) * coefficients[count - 1];
  for ( usize i = count - 1; i-- > 2; ) out = out * x + f64(i * (i - 1)) * coefficients[i];
  return out;
}

int
main()
{
  print("=== SPLINE EXTENSION FIXED-SEED FUZZ ===");

  test_case("branchless segment lookup matches a linear oracle");
  {
    for ( usize trial = 0; trial < 256; ++trial ) {
      constexpr usize count = 17;
      f64 knots[count];
      knots[0] = f64(-4) + signed_random();
      for ( usize i = 1; i < count; ++i ) knots[i] = knots[i - 1] + f64(0.01) + unit_random();
      usize last = usize(next_random() % (count + 9));
      for ( usize query = 0; query < 64; ++query ) {
        const f64 x = knots[0] - f64(1) + unit_random() * (knots[count - 1] - knots[0] + f64(2));
        usize expected = 0;
        if ( x >= knots[count - 1] )
          expected = count - 2;
        else if ( x > knots[0] )
          while ( expected + 1 < count - 1 && knots[expected + 1] < x ) ++expected;
        const usize found = __impl_splines_bits::locate_segment<f64>(knots, count, x, last);
        require_true(found == expected);
        require_true(last == expected);
      }
      require_true(__impl_splines_bits::locate_segment<f64>(knots, count, knots[0] - f64(1), last) == 0);
      require_true(__impl_splines_bits::locate_segment<f64>(knots, count, knots[count - 1] + f64(1), last) == count - 2);
    }
  }
  end_test_case();

  test_case("cubic and quintic Hermite match polynomial oracles");
  {
    for ( usize trial = 0; trial < 128; ++trial ) {
      constexpr usize count = 9;
      f64 x[count], cubic_y[count], cubic_d1[count];
      f64 quintic_y[count], quintic_d1[count], quintic_d2[count];
      f64 cubic_coefficients[4], quintic_coefficients[6];
      for ( f64 &coefficient : cubic_coefficients ) coefficient = f64(2) * signed_random();
      for ( f64 &coefficient : quintic_coefficients ) coefficient = signed_random();
      x[0] = f64(-1) + f64(0.25) * signed_random();
      for ( usize i = 1; i < count; ++i ) x[i] = x[i - 1] + f64(0.05) + f64(0.25) * unit_random();
      for ( usize i = 0; i < count; ++i ) {
        cubic_y[i] = polynomial(cubic_coefficients, 4, x[i]);
        cubic_d1[i] = polynomial_derivative(cubic_coefficients, 4, x[i]);
        quintic_y[i] = polynomial(quintic_coefficients, 6, x[i]);
        quintic_d1[i] = polynomial_derivative(quintic_coefficients, 6, x[i]);
        quintic_d2[i] = polynomial_second_derivative(quintic_coefficients, 6, x[i]);
      }
      const auto cubic = make_cubic_hermite<f64>({ x, count }, { cubic_y, count }, { cubic_d1, count });
      const auto quintic = make_quintic_hermite<f64>({ x, count }, { quintic_y, count }, { quintic_d1, count }, { quintic_d2, count });
      spline_cursor cubic_cursor{}, quintic_cursor{};
      for ( usize query = 0; query < 64; ++query ) {
        const f64 q = x[0] + unit_random() * (x[count - 1] - x[0]);
        require_true(near(evaluate(cubic, q, cubic_cursor), polynomial(cubic_coefficients, 4, q), f64(2e-10)));
        require_true(near(evaluate(quintic, q, quintic_cursor), polynomial(quintic_coefficients, 6, q), f64(2e-9)));
      }
    }
  }
  end_test_case();

  test_case("B-spline basis and derivative agree with direct sums");
  {
    for ( usize trial = 0; trial < 192; ++trial ) {
      const u32 degree = u32(2 + next_random() % 4);
      const usize n_ctrl = usize(degree) + 3 + usize(next_random() % 6);
      f64 control[16];
      for ( usize i = 0; i < n_ctrl; ++i ) control[i] = f64(3) * signed_random();
      const auto knots = make_uniform_clamped_knots<f64>(n_ctrl, degree, f64(-1), f64(2));
      const auto spline = make_bspline_from_ctrl<f64>({ knots.data(), knots.size() }, { control, n_ctrl }, degree);
      const auto derivative_spline_value = derivative_spline(spline);
      for ( usize query = 0; query < 32; ++query ) {
        const f64 q = f64(-0.999) + f64(2.998) * unit_random();
        const auto values = basis(spline, q);
        const auto derivatives = basis_derivatives(spline, q, 1);
        f64 partition = 0, derivative_partition = 0, value = 0, derivative_value = 0;
        for ( usize i = 0; i < n_ctrl; ++i ) {
          partition += values[i];
          derivative_partition += derivatives[n_ctrl + i];
          value += values[i] * control[i];
          derivative_value += derivatives[n_ctrl + i] * control[i];
        }
        require_true(near(partition, f64(1), f64(2e-12)));
        require_true(near(derivative_partition, f64(0), f64(2e-11)));
        require_true(near(value, evaluate(spline, q), f64(2e-12)));
        require_true(near(derivative_value, evaluate(derivative_spline_value, q), f64(2e-10)));
      }
    }
  }
  end_test_case();

  test_case("banded least-squares solvers recover a generated spline");
  {
    for ( usize trial = 0; trial < 64; ++trial ) {
      const u32 degree = u32(1 + next_random() % 5);
      const usize n_ctrl = usize(degree) + 3 + usize(next_random() % 5);
      const usize sample_count = n_ctrl * 4 + 1;
      f64 control[16], x[65], y[65], weights[65];
      for ( usize i = 0; i < n_ctrl; ++i ) control[i] = f64(3) * signed_random();
      const auto knots = make_uniform_clamped_knots<f64>(n_ctrl, degree, f64(0), f64(1));
      const auto oracle = make_bspline_from_ctrl<f64>({ knots.data(), knots.size() }, { control, n_ctrl }, degree);
      for ( usize i = 0; i < sample_count; ++i ) {
        x[i] = f64(i) / f64(sample_count - 1);
        y[i] = evaluate(oracle, x[i]);
        weights[i] = f64(0.25) + unit_random();
      }
      const auto qr = make_lsq_bspline<f64>({ x, sample_count }, { y, sample_count }, { knots.data(), knots.size() }, degree,
                                            { weights, sample_count }, lsq_method::givens_qr);
      const auto normal = make_lsq_bspline<f64>({ x, sample_count }, { y, sample_count }, { knots.data(), knots.size() }, degree,
                                                { weights, sample_count }, lsq_method::normal_equations);
      require_true(qr.ctrl.size() == n_ctrl && normal.ctrl.size() == n_ctrl);
      for ( usize query = 0; query < 32; ++query ) {
        const f64 q = unit_random();
        const f64 expected = evaluate(oracle, q);
        require_true(near(evaluate(qr, q), expected, f64(2e-9)));
        require_true(near(evaluate(normal, q), expected, f64(2e-8)));
      }
    }
  }
  end_test_case();

  test_case("unit-weight NURBS curves match the B-spline oracle");
  {
    for ( usize trial = 0; trial < 96; ++trial ) {
      constexpr usize dimensions = 3;
      const u32 degree = u32(1 + next_random() % 5);
      const usize n_ctrl = usize(degree) + 2 + usize(next_random() % 6);
      vec<f64, dimensions> control[16];
      f64 weights[16];
      for ( usize i = 0; i < n_ctrl; ++i ) {
        weights[i] = 1;
        for ( usize axis = 0; axis < dimensions; ++axis ) control[i][axis] = signed_random();
      }
      const auto knots = make_uniform_clamped_knots<f64>(n_ctrl, degree, f64(0), f64(1));
      auto curve = make_bspline_curve_from_ctrl<f64, dimensions>({ knots.data(), knots.size() }, control, n_ctrl, degree);
      auto rational = make_nurbs_curve<f64, dimensions>({ knots.data(), knots.size() }, control, { weights, n_ctrl }, n_ctrl, degree);
      const usize cache_size = (n_ctrl - degree) * __impl_bspline::basis_inverse_width(degree);
      require_true(curve.__basis_inverse.size() == cache_size);
      require_true(rational.__basis_inverse.size() == cache_size);
      const f64 cache_probe = unit_random();
      const auto cached = evaluate(curve, cache_probe);
      invalidate_basis_cache(curve);
      const auto uncached = evaluate(curve, cache_probe);
      for ( usize axis = 0; axis < dimensions; ++axis ) require_true(near(cached[axis], uncached[axis], f64(3e-12)));
      rebuild_basis_cache(curve);
      spline_cursor curve_cursor{}, rational_cursor{};
      for ( usize query = 0; query < 48; ++query ) {
        const f64 q = unit_random();
        const auto expected = evaluate(curve, q, curve_cursor);
        const auto actual = evaluate(rational, q, rational_cursor);
        for ( usize axis = 0; axis < dimensions; ++axis ) require_true(near(actual[axis], expected[axis], f64(3e-12)));
      }
    }
  }
  end_test_case();

  test_case("tensor surface matches a handwritten bilinear oracle");
  {
    f64 axis0[4] = { 0, 0, 1, 1 };
    f64 axis1[4] = { 0, 0, 1, 1 };
    raw_slice<const f64> knots[2] = { { axis0, 4 }, { axis1, 4 } };
    usize shape[2] = { 2, 2 };
    u32 degree[2] = { 1, 1 };
    for ( usize trial = 0; trial < 128; ++trial ) {
      f64 control[4];
      for ( f64 &value : control ) value = f64(4) * signed_random();
      auto surface = make_tensor_bspline<f64, f64, 2>(knots, shape, degree, { control, 4 });
      const f64 cache_point[2] = { unit_random(), unit_random() };
      const f64 cached = evaluate(surface, cache_point);
      invalidate_basis_cache(surface);
      require_true(near(evaluate(surface, cache_point), cached, f64(2e-12)));
      rebuild_basis_cache(surface);
      for ( usize query = 0; query < 64; ++query ) {
        const f64 u = unit_random();
        const f64 v = unit_random();
        const f64 expected = (f64(1) - u) * ((f64(1) - v) * control[0] + v * control[1]) + u * ((f64(1) - v) * control[2] + v * control[3]);
        const f64 point[2] = { u, v };
        require_true(near(evaluate(surface, point), expected, f64(2e-12)));
      }
    }
  }
  end_test_case();

  print("=== SPLINE EXTENSION FIXED-SEED FUZZ PASSED ===");
  return 1;
}
