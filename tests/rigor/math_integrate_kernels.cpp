// math_integrate_kernels.cpp — routed reduction, scan, and batch tails

#include "../../src/math/integrate/integrate.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;
using namespace micron;
using namespace micron::math;

template<typename F>
static bool
near(F a, F b, F tolerance) noexcept
{
  return math::fabs(a - b) <= tolerance;
}

template<typename F>
static void
exercise_tails(F tolerance) noexcept
{
  alignas(64) F storage[160]{};
  alignas(64) F weights[160]{};
  alignas(64) F output[160]{};
  alignas(64) F expected[160]{};
  for ( usize i = 0; i < 160; ++i ) {
    storage[i] = F((i * 37 + 11) % 101) * F(0.03125) - F(1.25);
    weights[i] = F((i * 19 + 7) % 43) * F(0.015625) - F(0.25);
  }

  for ( usize offset = 0; offset < 8; ++offset ) {
    for ( usize count = 0; count <= 79; ++count ) {
      F sum = F(0), weighted = F(0);
      for ( usize i = 0; i < count; ++i ) {
        sum += storage[offset + i];
        weighted += weights[offset + i] * storage[offset + i];
      }
      const F routed_sum = integrate::__integrate_arch::sum_fast(storage + offset, count);
      const F routed_weighted = integrate::__integrate_arch::weighted_sum_fast(weights + offset, storage + offset, count);
      require_true(near(routed_sum, sum, tolerance * F(count + 1)));
      require_true(near(routed_weighted, weighted, tolerance * F(count + 1)));

      integrate::cum_trapezoid<F>(storage + offset, output, count, F(0.125));
      if ( count != 0 ) expected[0] = F(0);
      for ( usize i = 1; i < count; ++i ) expected[i] = expected[i - 1] + F(0.0625) * (storage[offset + i - 1] + storage[offset + i]);
      for ( usize i = 0; i < count; ++i ) require_true(near(output[i], expected[i], tolerance * F(i + 1)));

      integrate::derive::diff<F>(storage + offset, output, count, F(0.125));
      if ( count == 1 ) expected[0] = F(0);
      if ( count >= 2 ) {
        expected[0] = (storage[offset + 1] - storage[offset]) / F(0.125);
        for ( usize i = 1; i + 1 < count; ++i ) expected[i] = (storage[offset + i + 1] - storage[offset + i - 1]) / F(0.25);
        expected[count - 1] = (storage[offset + count - 1] - storage[offset + count - 2]) / F(0.125);
      }
      for ( usize i = 0; i < count; ++i ) require_true(near(output[i], expected[i], tolerance));
    }
  }
}

int
main()
{
  test_case("SIMD reductions, scans, differences, alignment, and tails");
  exercise_tails<f32>(f32(2e-5));
  exercise_tails<f64>(f64(2e-13));
  end_test_case();

  test_case("in-place sampled transforms preserve unread input");
  {
    f64 values[17]{};
    f64 source[17]{};
    for ( usize i = 0; i < 17; ++i ) values[i] = source[i] = f64(i * i) * 0.01;
    integrate::cum_trapezoid<f64>(values, values, 17, 0.1);
    f64 running = 0;
    require_true(values[0] == 0);
    for ( usize i = 1; i < 17; ++i ) {
      running += 0.05 * (source[i - 1] + source[i]);
      require_true(near(values[i], running, f64(2e-14)));
    }
    for ( usize i = 0; i < 17; ++i ) values[i] = source[i];
    integrate::derive::diff<f64>(values, values, 17, 0.1);
    for ( usize i = 1; i + 1 < 17; ++i ) require_true(near(values[i], f64(i) * f64(0.2), f64(2e-14)));
  }
  end_test_case();

  test_case("accurate accumulation recovers cancellation residue");
  {
    const f64 samples[] = { 0.0, 1e16, 1.0, -1e16, 0.0 };
    const f64 fast = integrate::trapezoid<f64, integrate::accumulation_policy::fast>(samples, 5, 1.0);
    const f64 accurate = integrate::trapezoid<f64, integrate::accumulation_policy::accurate>(samples, 5, 1.0);
    require_true(fast == 0.0);
    require_true(accurate == 1.0);
  }
  end_test_case();

  test_case("adaptive batch quadrature submits whole Kronrod rules");
  {
    usize calls = 0;
    usize widest = 0;
    auto batch = [&](const f64 *x, f64 *y, usize count) noexcept {
      ++calls;
      if ( count > widest ) widest = count;
      for ( usize i = 0; i < count; ++i ) y[i] = x[i] * x[i];
    };
    integrate::quad_options<f64> options{};
    options.abs_tol = 1e-12;
    options.rel_tol = 1e-12;
    integrate::quad_workspace<f64, 16> workspace{};
    const auto result = integrate::quad_batch<f64>(batch, 0.0, 1.0, options, workspace);
    require_true(result.status == integrate::quad_status::ok);
    require_true(near(result.value, f64(1.0 / 3.0), f64(2e-14)));
    require_true(calls == 1);
    require_true(widest == 21);
  }
  end_test_case();

  return 1;
}
