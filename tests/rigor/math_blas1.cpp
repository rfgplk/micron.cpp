// math_blas1.cpp
// Snowball tests for math::blas::level1 — axpy/axpby/scal/asum/nrm2/iamax/dot/rotg/rot/sdsdot.

#include "../../src/math/blas/level1.hpp"
#include "../../src/std.hpp"
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
  print("=== BLAS L1 TESTS ===");

  test_case("axpy: y ← α·x + y");
  {
    f64 x[4] = { 1, 2, 3, 4 };
    f64 y[4] = { 10, 20, 30, 40 };
    blas::level1::axpy<f64>(2.0, x, x + 4, y);
    require_true(near(y[0], 12.0));
    require_true(near(y[1], 24.0));
    require_true(near(y[2], 36.0));
    require_true(near(y[3], 48.0));
  }
  end_test_case();

  test_case("axpby: y ← α·x + β·y");
  {
    f64 x[4] = { 1, 2, 3, 4 };
    f64 y[4] = { 10, 20, 30, 40 };
    blas::level1::axpby<f64>(4, 2.0, x, 1, 0.5, y, 1);
    require_true(near(y[0], 7.0));      // 5 + 2 = 7
    require_true(near(y[1], 14.0));     // 10 + 4
    require_true(near(y[2], 21.0));     // 15 + 6
    require_true(near(y[3], 28.0));     // 20 + 8
  }
  end_test_case();

  test_case("scal: x ← α·x");
  {
    f64 x[3] = { 1, 2, 3 };
    blas::level1::scal<f64>(0.5, x, x + 3);
    require_true(near(x[0], 0.5));
    require_true(near(x[1], 1.0));
    require_true(near(x[2], 1.5));
  }
  end_test_case();

  test_case("asum / nrm2");
  {
    f64 v[4] = { 1, -2, 3, -4 };
    require_true(near(blas::level1::asum<f64>(v, v + 4), 10.0));
    require_true(near(blas::level1::nrm2<f64>(v, v + 4), 5.477225575051661, 1e-10));

    // Overflow safety: nrm2 should not overflow even at extreme magnitudes.
    f64 huge[2] = { 1e200, 1e200 };
    require_true(near(blas::level1::nrm2<f64>(huge, huge + 2), 1.4142135623730951e200, 1e190));
  }
  end_test_case();

  test_case("iamax / iamin");
  {
    f64 v[5] = { 1, -3, 2, -4, 0 };
    require(usize(blas::level1::iamax<f64>(v, v + 5)), usize(3));     // |v[3]| = 4
    require(usize(blas::level1::iamin<f64>(v, v + 5)), usize(4));     // |v[4]| = 0
  }
  end_test_case();

  test_case("dot / dot_kahan");
  {
    f64 x[3] = { 1, 2, 3 };
    f64 y[3] = { 4, 5, 6 };
    require_true(near(blas::level1::dot<f64>(x, x + 3, y), 32.0));     // 4+10+18

    f64 a[3] = { 1e10, 1.0, -1e10 };
    f64 b[3] = { 1.0, 1.0, 1.0 };
    require_true(near(blas::level1::dot_kahan<f64>(a, a + 3, b), 1.0, 1e-6));
  }
  end_test_case();

  test_case("sdsdot: f32 dot with f64 accumulator");
  {
    f32 x[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    f32 y[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
    f32 r = blas::level1::sdsdot(4, 0.5f, x, 1, y, 1);
    require_true(near(f64(r), 70.5, 1e-4));     // 0.5 + 5 + 12 + 21 + 32
  }
  end_test_case();

  test_case("strided axpy/dot via inc != 1");
  {
    f64 x[6] = { 1, 9, 2, 9, 3, 9 };        // values at incx=2 are {1,2,3}
    f64 y[6] = { 10, 8, 20, 8, 30, 8 };     // values at incy=2 are {10,20,30}
    blas::level1::axpy<f64>(3, 2.0, x, 2, y, 2);
    require_true(near(y[0], 12.0));
    require_true(near(y[2], 24.0));
    require_true(near(y[4], 36.0));
    require_true(near(y[1], 8.0));     // untouched
    require_true(near(blas::level1::dot<f64>(3, x, 2, y, 2), 12 + 48 + 108));
  }
  end_test_case();

  test_case("rotg + rot: Givens rotation kills b");
  {
    f64 a = 3.0, b = 4.0, c, s;
    blas::level1::rotg<f64>(a, b, c, s);
    require_true(near(a, 5.0, 1e-12));     // r = sqrt(9+16) = 5
    // Apply to 1-element vectors x={3}, y={4} — should produce x={5}, y≈0
    f64 xs[1] = { 3.0 };
    f64 ys[1] = { 4.0 };
    blas::level1::rot<f64>(1, xs, 1, ys, 1, c, s);
    require_true(near(xs[0], 5.0, 1e-12));
    require_true(near(ys[0], 0.0, 1e-12));
  }
  end_test_case();

  test_case("vec_view + container overloads");
  {
    micron::vector<f64> x(3, 0.0);
    micron::vector<f64> y(3, 0.0);
    x[0] = 1;
    x[1] = 2;
    x[2] = 3;
    y[0] = 4;
    y[1] = 5;
    y[2] = 6;
    require_true(near(blas::level1::dot(x, y), 32.0));
    require_true(near(blas::level1::asum(x), 6.0));
    require_true(near(blas::level1::nrm2(y), 8.774964387392123, 1e-10));

    quants::vec_view<f64> vx = quants::vec_view<f64>::from(x);
    quants::vec_view<f64> vy = quants::vec_view<f64>::from(y);
    require_true(near(blas::level1::dot<f64>(vx, vy), 32.0));
    blas::level1::axpy<f64>(2.0, vx, vy);
    require_true(near(y[0], 6.0));      // 4 + 2*1
    require_true(near(y[1], 9.0));      // 5 + 2*2
    require_true(near(y[2], 12.0));     // 6 + 2*3
  }
  end_test_case();

  print("=== blas L1 ok ===");
  return 0;
}
