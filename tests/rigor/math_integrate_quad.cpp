// math_integrate_quad.cpp — Snowball tests for adaptive quad / nquad / dblquad / tplquad / gauss / romberg

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
  print("=== INTEGRATE: ADAPTIVE / GAUSS / ROMBERG / NQUAD ===");

  const f64 pi = 3.14159265358979323846;

  test_case("quad: ∫_0^π sin x dx = 2 (adaptive GK 7/15)");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::trig::sin<f64>(x); };
    auto r = integrate::quad<f64>(f, f64(0), pi, f64(1e-10), f64(1e-8));
    require_true(near(r.value, 2.0, 1e-10));
    require_true(r.status == integrate::quad_status::ok);
  }
  end_test_case();

  test_case("quad: ∫_-1^1 1/(1+25x²) dx (Runge function)");
  {
    auto f = [](f64 x) noexcept -> f64 { return 1.0 / (1.0 + 25.0 * x * x); };
    auto r = integrate::quad<f64>(f, f64(-1), f64(1), f64(1e-12), f64(1e-10));
    // exact = (2/5)·atan(5) ≈ 0.5493603...
    const f64 truth = 2.0 / 5.0 * mk::trig::atan<f64>(5.0);
    require_true(near(r.value, truth, 1e-9));
  }
  end_test_case();

  test_case("quad: ∫_0^4 x³ dx = 64 (smooth polynomial)");
  {
    auto f = [](f64 x) noexcept -> f64 { return x * x * x; };
    auto r = integrate::quad<f64>(f, f64(0), f64(4), f64(1e-12), f64(1e-10));
    require_true(near(r.value, 64.0, 1e-10));
  }
  end_test_case();

  test_case("gauss_legendre: order 8 exact for degree 15 polynomial");
  {
    // ∫_0^1 x^15 dx = 1/16; Gauss-8 exact for degrees ≤ 15
    auto f = [](f64 x) noexcept -> f64 {
      f64 r = x;
      for ( int i = 0; i < 14; ++i ) r *= x;
      return r;
    };
    f64 r = integrate::gauss_legendre<8, f64>(f, f64(0), f64(1));
    require_true(near(r, 1.0 / 16.0, 1e-12));
  }
  end_test_case();

  test_case("gauss_legendre: order 16 on smooth integrand");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::exp_ns::exp<f64>(-x * x); };
    f64 r = integrate::gauss_legendre<16, f64>(f, f64(0), f64(1));
    // ∫_0^1 e^(-x²) dx = (sqrt(π)/2)·erf(1) ≈ 0.7468241328...
    require_true(near(r, 0.7468241328124271, 1e-10));
  }
  end_test_case();

  test_case("clenshaw_curtis: matches gauss on smooth integrand");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::trig::cos<f64>(x); };
    f64 cc = integrate::clenshaw_curtis<32, f64>(f, f64(0), pi);
    f64 gl = integrate::gauss_legendre<16, f64>(f, f64(0), pi);
    // ∫_0^π cos x dx = 0.  Both should be very close.
    require_true(near(cc, 0.0, 1e-10));
    require_true(near(gl, 0.0, 1e-10));
  }
  end_test_case();

  test_case("romberg: ∫_0^1 e^x dx = e − 1");
  {
    auto f = [](f64 x) noexcept -> f64 { return mk::exp_ns::exp<f64>(x); };
    f64 r = integrate::romberg<f64>(f, f64(0), f64(1), 1e-12, 12);
    const f64 truth = 2.718281828459045 - 1.0;
    require_true(near(r, truth, 1e-10));
  }
  end_test_case();

  test_case("dblquad rectangular: ∫∫ x²·y dx dy on [0,1]²  = 1/6");
  {
    auto f = [](f64 x, f64 y) noexcept -> f64 { return x * x * y; };
    auto r = integrate::dblquad<f64>(f, f64(0), f64(1), f64(0), f64(1), f64(1e-9), f64(1e-7));
    require_true(near(r.value, 1.0 / 6.0, 1e-7));
  }
  end_test_case();

  test_case("dblquad variable bounds: ∫∫ over triangle x ∈ [0,1], y ∈ [0, 1-x] of 1 = 1/2");
  {
    auto f = [](f64, f64) noexcept -> f64 { return 1.0; };
    auto yl = [](f64) noexcept -> f64 { return 0.0; };
    auto yu = [](f64 x) noexcept -> f64 { return 1.0 - x; };
    auto r = integrate::dblquad<f64>(f, f64(0), f64(1), yl, yu, f64(1e-9), f64(1e-7));
    require_true(near(r.value, 0.5, 1e-7));
  }
  end_test_case();

  test_case("tplquad: ∫∫∫ 1 dV over unit cube = 1");
  {
    auto f = [](f64, f64, f64) noexcept -> f64 { return 1.0; };
    auto r = integrate::tplquad<f64>(f, f64(0), f64(1), f64(0), f64(1), f64(0), f64(1), f64(1e-8), f64(1e-6));
    require_true(near(r.value, 1.0, 1e-7));
  }
  end_test_case();

  test_case("nquad<2>: matches dblquad");
  {
    auto f = [](const f64 (&x)[2]) noexcept -> f64 { return x[0] * x[0] * x[1]; };
    f64 lo[2] = { 0, 0 };
    f64 hi[2] = { 1, 1 };
    auto r = integrate::nquad<2, f64>(f, lo, hi, f64(1e-9), f64(1e-7));
    require_true(near(r.value, 1.0 / 6.0, 1e-7));
  }
  end_test_case();

  test_case("nquad<3>: ∫∫∫ x·y·z over unit cube = 1/8");
  {
    auto f = [](const f64 (&x)[3]) noexcept -> f64 { return x[0] * x[1] * x[2]; };
    f64 lo[3] = { 0, 0, 0 };
    f64 hi[3] = { 1, 1, 1 };
    auto r = integrate::nquad<3, f64>(f, lo, hi, f64(1e-8), f64(1e-6));
    require_true(near(r.value, 0.125, 1e-7));
  }
  end_test_case();

  print("=== integrate quad/nquad/gauss/romberg ok ===");
  return 1;
}
