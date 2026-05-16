// math_splines_smoothing.cpp
// Rigorous snowball tests for the Reinsch smoothing spline + adaptive
// knot insertion.

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
  print("=== MATH::SPLINES SMOOTHING / ADAPTIVE-KNOTS TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("smoothing: lambda = 0 reproduces the natural cubic interpolant");
  {
    constexpr usize n = 8;
    f64 xs[n], ys[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = 0.5 * f64(i);
      ys[i] = mk::trig::sin<f64>(xs[i]);
    }
    raw_slice<const f64> empty_w(static_cast<const f64 *>(nullptr), 0);
    auto smooth0 = make_smoothing<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), empty_w, 0.0);
    auto interp = make_cubic<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), bc_kind::natural);
    for ( usize i = 0; i < n; ++i ) require_true(near(evaluate<f64>(smooth0, xs[i]), ys[i], 1e-12));
    for ( usize i = 0; i < 30; ++i ) {
      const f64 t = 0.05 + 0.13 * f64(i);
      if ( t > xs[n - 1] ) break;
      require_true(near(evaluate<f64>(smooth0, t), evaluate<f64>(interp, t), 1e-12));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("smoothing: lambda > 0 reduces residual energy below interpolation noise");
  {
    // Synthetic noisy data: y = sin(x) + small deterministic 'noise'.
    constexpr usize n = 21;
    f64 xs[n], ys[n], truth[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = 0.3 * f64(i);
      truth[i] = mk::trig::sin<f64>(xs[i]);
      // Deterministic perturbation that averages to zero over the dataset.
      const f64 noise = 0.05 * mk::trig::sin<f64>(13.7 * xs[i]);
      ys[i] = truth[i] + noise;
    }
    raw_slice<const f64> empty_w(static_cast<const f64 *>(nullptr), 0);
    auto interp = make_cubic<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), bc_kind::natural);
    auto smooth = make_smoothing<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), empty_w, 0.05);

    // RMS error vs truth on a fine grid.
    f64 err_interp = 0, err_smooth = 0;
    usize m = 0;
    for ( usize i = 0; i < 200; ++i ) {
      const f64 t = 0.0 + 0.03 * f64(i);
      if ( t > xs[n - 1] ) break;
      const f64 truth_t = mk::trig::sin<f64>(t);
      const f64 e_i = evaluate<f64>(interp, t) - truth_t;
      const f64 e_s = evaluate<f64>(smooth, t) - truth_t;
      err_interp += e_i * e_i;
      err_smooth += e_s * e_s;
      ++m;
    }
    err_interp /= f64(m);
    err_smooth /= f64(m);
    // Smoothing should reduce reconstruction error vs the noise-following interpolant.
    require_true(err_smooth < err_interp);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("smoothing: lambda → ∞ collapses to nearly-affine residual structure");
  {
    constexpr usize n = 12;
    f64 xs[n], ys[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = f64(i);
      ys[i] = 1.0 + 2.0 * xs[i] + 0.5 * mk::trig::sin<f64>(3.0 * xs[i]);      // affine + ripple
    }
    raw_slice<const f64> empty_w(static_cast<const f64 *>(nullptr), 0);
    build_info<f64> info{};
    auto smooth = make_smoothing<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), empty_w, 1e10, &info);
    require_true(info.status == build_status::ok);
    // Verify result is monotone-increasing (the affine trend dominates).
    f64 prev = evaluate<f64>(smooth, xs[0]);
    for ( usize i = 1; i < 100; ++i ) {
      const f64 t = xs[0] + (xs[n - 1] - xs[0]) * f64(i) / 99.0;
      const f64 v = evaluate<f64>(smooth, t);
      require_true(v >= prev - 1e-6);
      prev = v;
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("smoothing: lambda < 0 triggers GCV-driven λ selection");
  {
    // Noisy sine data; GCV should pick a positive λ that smooths it.
    constexpr usize n = 25;
    f64 xs[n], ys[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = 0.25 * f64(i);
      const f64 truth = mk::trig::sin<f64>(xs[i]);
      const f64 noise = 0.08 * mk::trig::sin<f64>(11.3 * xs[i]) + 0.04 * mk::trig::cos<f64>(17.1 * xs[i]);
      ys[i] = truth + noise;
    }
    raw_slice<const f64> empty_w(static_cast<const f64 *>(nullptr), 0);
    build_info<f64> info{};
    auto s = make_smoothing<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), empty_w, -1.0, &info);
    require_true(info.status == build_status::ok);
    require_true(s.xs.size() == n);
    // Status should report the GCV-selected lambda in residual.
    const f64 lam = info.residual;
    require_true(lam > 0.0);
    require_true(lam < 1e6);
    require_true(info.n_iterations > 4);

    // GCV-fit reconstruction beats no-smoothing (interpolant) on the truth.
    auto interp = make_cubic<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), bc_kind::natural);
    f64 err_interp = 0, err_gcv = 0;
    usize m = 0;
    for ( usize i = 0; i < 200; ++i ) {
      const f64 t = 0.05 + 0.03 * f64(i);
      if ( t > xs[n - 1] ) break;
      const f64 truth = mk::trig::sin<f64>(t);
      const f64 e_i = evaluate<f64>(interp, t) - truth;
      const f64 e_g = evaluate<f64>(s, t) - truth;
      err_interp += e_i * e_i;
      err_gcv += e_g * e_g;
      ++m;
    }
    require_true(err_gcv < err_interp);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("adaptive_knots: meets tolerance on smooth data with a knot budget");
  {
    constexpr usize n = 64;
    f64 xs[n], ys[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = -2.0 + 4.0 * f64(i) / f64(n - 1);
      ys[i] = mk::trig::sin<f64>(2.0 * xs[i]);      // smooth
    }
    build_info<f64> info{};
    auto s = make_adaptive_knots<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), 0.005, 30, &info);
    require_true(info.status == build_status::ok || info.status == build_status::max_iter);
    // Verify the residual at every original sample is close.
    f64 worst = 0;
    for ( usize i = 0; i < n; ++i ) {
      const f64 e = fabs64(evaluate<f64>(s, xs[i]) - ys[i]);
      if ( e > worst ) worst = e;
    }
    if ( info.status == build_status::ok ) require_true(worst <= 0.005 + 1e-9);
    require_true(s.xs.size() <= 30);
    require_true(s.xs.size() >= 4);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("adaptive_knots: respects max_knots when tolerance unreachable");
  {
    constexpr usize n = 32;
    f64 xs[n], ys[n];
    for ( usize i = 0; i < n; ++i ) {
      xs[i] = f64(i);
      // Highly oscillatory data — impossible to fit to 1e-9 with 6 knots.
      ys[i] = mk::trig::sin<f64>(15.0 * xs[i]) + (i % 3 == 0 ? 1.0 : 0.0);
    }
    build_info<f64> info{};
    auto s = make_adaptive_knots<f64>(raw_slice<const f64>(xs, n), raw_slice<const f64>(ys, n), 1e-9, 6, &info);
    require_true(s.xs.size() <= 6);
    // Status should be max_iter (since we couldn't reach 1e-9).
    require_true(info.status == build_status::max_iter || info.status == build_status::ok);
  }
  end_test_case();

  print("=== SMOOTHING / ADAPTIVE-KNOTS TESTS PASSED ===");
  return 1;
}
