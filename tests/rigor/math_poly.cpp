// math_poly.cpp — Snowball tests for the polynomial module
// (polyval / polyder / polyint / polyfit / roots).

#include "../../src/math/linalg/poly.hpp"
#include "../../src/math/matrix/matrices.hpp"
#include "../../src/std.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
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
  print("=== POLY ===");

  test_case("polyval: Horner evaluation");
  {
    // p(x) = 2x^2 - 3x + 1
    dynvec<f64> c(3);
    c[0] = 2;
    c[1] = -3;
    c[2] = 1;
    require_true(near(linalg::poly::polyval(c, f64(0.0)), 1.0));
    require_true(near(linalg::poly::polyval(c, f64(1.0)), 0.0));
    require_true(near(linalg::poly::polyval(c, f64(2.0)), 3.0));       // 8 - 6 + 1
    require_true(near(linalg::poly::polyval(c, f64(-1.0)), 6.0));      // 2 + 3 + 1
  }
  end_test_case();

  test_case("polyder: derivative coefficient-wise");
  {
    // p = 2x^3 - 3x^2 + 4x - 5; p' = 6x^2 - 6x + 4
    dynvec<f64> c(4);
    c[0] = 2;
    c[1] = -3;
    c[2] = 4;
    c[3] = -5;
    auto dc = linalg::poly::polyder(c);
    require_true(dc.size() == 3);
    require_true(near(dc[0], 6.0));
    require_true(near(dc[1], -6.0));
    require_true(near(dc[2], 4.0));
  }
  end_test_case();

  test_case("polyint: antiderivative with integration constant");
  {
    // p = 6x^2 - 6x + 4; ∫p = 2x^3 - 3x^2 + 4x + k0
    dynvec<f64> c(3);
    c[0] = 6;
    c[1] = -6;
    c[2] = 4;
    auto ic = linalg::poly::polyint(c, f64(7.0));
    require_true(ic.size() == 4);
    require_true(near(ic[0], 2.0));
    require_true(near(ic[1], -3.0));
    require_true(near(ic[2], 4.0));
    require_true(near(ic[3], 7.0));
  }
  end_test_case();

  test_case("polyfit: exact recovery on a degree-2 polynomial");
  {
    // y = 1 + 2x + 3x^2 sampled at five points → fit must recover exactly.
    dynvec<f64> xs(5), ys(5);
    for ( usize i = 0; i < 5; ++i ) {
      xs[i] = f64(i);
      ys[i] = f64(1) + f64(2) * xs[i] + f64(3) * xs[i] * xs[i];
    }
    auto cf = linalg::poly::polyfit(xs, ys, 2);
    require_true(cf.size() == 3);
    require_true(near(cf[0], 3.0));
    require_true(near(cf[1], 2.0));
    require_true(near(cf[2], 1.0));
  }
  end_test_case();

  test_case("roots: real distinct");
  {
    // p(x) = 2x^2 - 3x + 1 = 2(x - 1)(x - 1/2) → roots {1, 0.5}
    dynvec<f64> c(3);
    c[0] = 2;
    c[1] = -3;
    c[2] = 1;
    auto rs = linalg::poly::roots(c);
    require_true(rs.converged);
    require_true(rs.re.size() == 2);
    f64 r0 = rs.re[0], r1 = rs.re[1];
    if ( r0 > r1 ) {
      f64 t = r0;
      r0 = r1;
      r1 = t;
    }
    require_true(near(r0, 0.5));
    require_true(near(r1, 1.0));
    require_true(rs.im[0] == 0.0 && rs.im[1] == 0.0);
  }
  end_test_case();

  test_case("roots: complex conjugate pair");
  {
    // p(x) = x^2 + 1 → roots ±i
    dynvec<f64> c(3);
    c[0] = 1;
    c[1] = 0;
    c[2] = 1;
    auto rs = linalg::poly::roots(c);
    require_true(rs.converged);
    f64 abs_im = rs.im[0];
    if ( abs_im < 0 ) abs_im = -abs_im;
    require_true(near(abs_im, 1.0));
    f64 abs_re = rs.re[0];
    if ( abs_re < 0 ) abs_re = -abs_re;
    require_true(near(abs_re, 0.0));
  }
  end_test_case();

  test_case("roots: cubic with three real roots");
  {
    // p(x) = (x-1)(x-2)(x-3) = x^3 - 6x^2 + 11x - 6
    dynvec<f64> c(4);
    c[0] = 1;
    c[1] = -6;
    c[2] = 11;
    c[3] = -6;
    auto rs = linalg::poly::roots(c);
    require_true(rs.converged);
    require_true(rs.re.size() == 3);
    // sort and verify
    f64 r[3] = { rs.re[0], rs.re[1], rs.re[2] };
    for ( usize i = 0; i + 1 < 3; ++i )
      for ( usize j = i + 1; j < 3; ++j )
        if ( r[i] > r[j] ) {
          f64 t = r[i];
          r[i] = r[j];
          r[j] = t;
        }
    require_true(near(r[0], 1.0));
    require_true(near(r[1], 2.0));
    require_true(near(r[2], 3.0));
  }
  end_test_case();

  return 0;
}
