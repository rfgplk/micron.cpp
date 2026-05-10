// math_integrate_derive.cpp — Snowball tests for finite-difference derivatives + grad/jac/hess

#include "../../src/math/integrate/integrate.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps = 1e-10)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== INTEGRATE: DERIVATIVES ===");

  const f64 pi = 3.14159265358979323846;

  test_case("central: d/dx sin(x) at π/4 == cos(π/4)");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::trig::sin<f64>(x); };
    f64 dy = integrate::derive::central<f64>(f, pi / 4.0, 1e-5);
    require_true(near(dy, mk::trig::cos<f64>(pi / 4.0), 1e-9));
  }
  end_test_case();

  test_case("forward / backward: O(h) consistency");
  {
    auto f = [](f64 x) noexcept -> f64 { return x * x * x; };     // f'(x) = 3x²
    f64 fwd = integrate::derive::forward<f64>(f, 2.0, 1e-6);
    f64 bwd = integrate::derive::backward<f64>(f, 2.0, 1e-6);
    require_true(near(fwd, 12.0, 1e-3));
    require_true(near(bwd, 12.0, 1e-3));
  }
  end_test_case();

  test_case("central4: O(h⁴) gives sub-1e-12 on smooth integrand");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::exp_ns::exp<f64>(x); };
    // f'(0.5) = e^0.5
    f64 dy = integrate::derive::central4<f64>(f, 0.5, 1e-3);
    f64 truth = mk::exp_ns::exp<f64>(0.5);
    require_true(near(dy, truth, 1e-12));
  }
  end_test_case();

  test_case("adaptive: Richardson nails high precision automatically");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::trig::cos<f64>(x); };
    f64 dy = integrate::derive::adaptive<f64>(f, 1.0);
    // f'(1) = -sin(1)
    f64 truth = -mk::trig::sin<f64>(1.0);
    require_true(near(dy, truth, 1e-10));
  }
  end_test_case();

  test_case("nth<2>: f''(x) of x⁴ at x=2 = 12·x² = 48");
  {
    auto f = [](f64 x) noexcept -> f64 { return x * x * x * x; };
    f64 d2 = integrate::derive::nth<2, f64>(f, 2.0, 1e-3);
    require_true(near(d2, 48.0, 1e-5));
  }
  end_test_case();

  test_case("gradient: ∇(x·y·z) at (1,2,3) = (yz, xz, xy) = (6,3,2)");
  {
    auto f = [](const f64(&x)[3]) noexcept -> f64 { return x[0] * x[1] * x[2]; };
    f64 x[3] = { 1.0, 2.0, 3.0 };
    auto g = integrate::derive::gradient<3, f64>(f, x, 1e-4);
    require_true(near(g.data[0], 6.0, 1e-6));
    require_true(near(g.data[1], 3.0, 1e-6));
    require_true(near(g.data[2], 2.0, 1e-6));
  }
  end_test_case();

  test_case("jacobian: polar→cartesian (r·cosθ, r·sinθ) at (r=1, θ=π/4)");
  {
    // f: (r, θ) → (r·cosθ, r·sinθ)
    auto f = [](const f64(&x)[2]) noexcept -> vec<f64, 2> {
      const f64 r = x[0], th = x[1];
      return vec<f64, 2>{ { r * mk::trig::cos<f64>(th), r * mk::trig::sin<f64>(th) } };
    };
    f64 x[2] = { 1.0, pi / 4.0 };
    auto J = integrate::derive::jacobian<2, 2, f64>(f, x, 1e-4);
    // analytic Jacobian: [[cosθ, -r·sinθ];[sinθ, r·cosθ]]
    const f64 c = mk::trig::cos<f64>(pi / 4.0);
    const f64 s = mk::trig::sin<f64>(pi / 4.0);
    require_true(near(J.data[0], c, 1e-7));
    require_true(near(J.data[1], -1.0 * s, 1e-7));
    require_true(near(J.data[2], s, 1e-7));
    require_true(near(J.data[3], c, 1e-7));
  }
  end_test_case();

  test_case("hessian: of (x²+y²)/2 at any point = identity");
  {
    auto f = [](const f64(&x)[2]) noexcept -> f64 { return 0.5 * (x[0] * x[0] + x[1] * x[1]); };
    f64 x[2] = { 3.0, 5.0 };
    auto H = integrate::derive::hessian<2, f64>(f, x, 1e-3);
    require_true(near(H.data[0], 1.0, 1e-6));
    require_true(near(H.data[1], 0.0, 1e-6));
    require_true(near(H.data[2], 0.0, 1e-6));
    require_true(near(H.data[3], 1.0, 1e-6));
  }
  end_test_case();

  test_case("hessian: of x*y at any point = [[0,1],[1,0]]");
  {
    auto f = [](const f64(&x)[2]) noexcept -> f64 { return x[0] * x[1]; };
    f64 x[2] = { 7.0, -3.0 };
    auto H = integrate::derive::hessian<2, f64>(f, x, 1e-3);
    require_true(near(H.data[0], 0.0, 1e-5));
    require_true(near(H.data[1], 1.0, 1e-5));
    require_true(near(H.data[2], 1.0, 1e-5));
    require_true(near(H.data[3], 0.0, 1e-5));
  }
  end_test_case();

  test_case("discrete diff: y = x² on uniform grid → dy ≈ 2x");
  {
    constexpr usize N = 9;
    f64 y[N];
    f64 dy[N];
    const f64 dx = 0.1;
    for ( usize i = 0; i < N; ++i ) {
      const f64 xi = f64(i) * dx;
      y[i] = xi * xi;
    }
    integrate::derive::diff<f64>(y, dy, N, dx);
    // Interior: central FD of x² is exact at any h.
    for ( usize i = 1; i + 1 < N; ++i ) {
      const f64 xi = f64(i) * dx;
      require_true(near(dy[i], 2.0 * xi, 1e-12));
    }
  }
  end_test_case();

  test_case("discrete diff non-uniform: y = x² (3-pt formula is exact for parabolas)");
  {
    f64 xs[5] = { 0.0, 0.5, 1.5, 3.5, 7.5 };
    f64 ys[5];
    for ( usize i = 0; i < 5; ++i ) ys[i] = xs[i] * xs[i];
    f64 dy[5];
    integrate::derive::diff<f64>(xs, ys, dy, 5);
    // For y = x² we have y' = 2x.  The 3-point Lagrange-derivative
    // formula is exact for quadratics, so interior points should be
    // bit-equal to 2*x_i within FP error.
    require_true(near(dy[1], 2.0 * 0.5, 1e-12));
    require_true(near(dy[2], 2.0 * 1.5, 1e-12));
    require_true(near(dy[3], 2.0 * 3.5, 1e-12));
  }
  end_test_case();

  test_case("discrete diff2: y = x³ → d²y/dx² = 6x");
  {
    f64 xs[5] = { 0, 1, 2, 3, 4 };
    f64 ys[5];
    for ( usize i = 0; i < 5; ++i ) ys[i] = xs[i] * xs[i] * xs[i];
    f64 d2y[5];
    integrate::derive::diff2<f64>(xs, ys, d2y, 5);
    // Interior: d²(x³) = 6x exactly via 3-point uniform central.
    require_true(near(d2y[1], 6.0 * 1.0, 1e-10));
    require_true(near(d2y[2], 6.0 * 2.0, 1e-10));
    require_true(near(d2y[3], 6.0 * 3.0, 1e-10));
  }
  end_test_case();

  print("=== integrate derive ok ===");
  return 0;
}
