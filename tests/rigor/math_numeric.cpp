// math_numeric.cpp
// Snowball tests for math::numeric — polyval/diff/gradient/interp/conv/lcm/...

#include "../../src/math/numeric.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps = 1e-12)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== NUMERIC TESTS ===");

  test_case("polyval — Horner");
  {
    // p(x) = 2 + 3x + x²
    f64 c[3] = { 2.0, 3.0, 1.0 };
    require_true(near(polyval<f64>(c, 3, 0.0), 2.0));
    require_true(near(polyval<f64>(c, 3, 1.0), 6.0));
    require_true(near(polyval<f64>(c, 3, 2.0), 12.0));
  }
  end_test_case();

  test_case("diff");
  {
    f64 in[5] = { 1, 4, 9, 16, 25 };
    f64 out[4] = { 0 };
    diff<f64>(in, 5, out);
    require_true(near(out[0], 3.0));
    require_true(near(out[1], 5.0));
    require_true(near(out[2], 7.0));
    require_true(near(out[3], 9.0));
  }
  end_test_case();

  test_case("gradient — uniform");
  {
    f64 y[5] = { 1, 2, 4, 7, 11 };
    f64 g[5] = { 0 };
    gradient<f64>(y, 5, g);
    require_true(near(g[0], 1.0));     // forward
    require_true(near(g[1], 1.5));     // central (4-1)/2
    require_true(near(g[2], 2.5));     // (7-2)/2
    require_true(near(g[3], 3.5));     // (11-4)/2
    require_true(near(g[4], 4.0));     // backward
  }
  end_test_case();

  test_case("interp — linear");
  {
    f64 x[4] = { 0, 1, 2, 3 };
    f64 y[4] = { 0, 10, 30, 60 };
    require_true(near(interp<f64>(0.5, x, y, 4), 5.0));
    require_true(near(interp<f64>(1.5, x, y, 4), 20.0));
    require_true(near(interp<f64>(2.5, x, y, 4), 45.0));
    // Clamps at endpoints.
    require_true(near(interp<f64>(-1.0, x, y, 4), 0.0));
    require_true(near(interp<f64>(10.0, x, y, 4), 60.0));
  }
  end_test_case();

  test_case("convolve — box ⨯ box");
  {
    f64 a[3] = { 1, 1, 1 };
    f64 b[3] = { 1, 1, 1 };
    f64 out[5] = { 0 };
    convolve<f64>(a, 3, b, 3, out);
    require_true(near(out[0], 1.0));
    require_true(near(out[1], 2.0));
    require_true(near(out[2], 3.0));
    require_true(near(out[3], 2.0));
    require_true(near(out[4], 1.0));
  }
  end_test_case();

  test_case("lcm / divmod");
  {
    require(lcm<int>(4, 6), 12);
    require(lcm<int>(7, 5), 35);
    require(lcm<int>(0, 5), 0);

    auto dm = divmod<int>(7, 3);
    require(dm.quot, 2);
    require(dm.rem, 1);

    auto dm2 = divmod<int>(-7, 3);
    require(dm2.quot, -3);
    require(dm2.rem, 2);
  }
  end_test_case();

  test_case("factorial / comb / perm");
  {
    require(u64(factorial<u64>(0)), u64(1));
    require(u64(factorial<u64>(5)), u64(120));
    require(u64(factorial<u64>(10)), u64(3628800));

    require(u64(comb<u64>(5, 2)), u64(10));
    require(u64(comb<u64>(10, 5)), u64(252));
    require(u64(comb<u64>(5, 0)), u64(1));
    require(u64(comb<u64>(5, 6)), u64(0));

    require(u64(perm<u64>(5, 3)), u64(60));
  }
  end_test_case();

  test_case("bincount / digitize");
  {
    int x[8] = { 0, 1, 1, 2, 2, 2, 3, 0 };
    u64 cnt[5] = { 0 };
    bincount<int>(x, 8, cnt, 5);
    require(u64(cnt[0]), u64(2));
    require(u64(cnt[1]), u64(2));
    require(u64(cnt[2]), u64(3));
    require(u64(cnt[3]), u64(1));
    require(u64(cnt[4]), u64(0));

    f64 edges[3] = { 0.0, 1.0, 2.0 };
    require(usize(digitize<f64>(-0.5, edges, 3)), usize(0));
    require(usize(digitize<f64>(0.5, edges, 3)), usize(1));
    require(usize(digitize<f64>(1.5, edges, 3)), usize(2));
    require(usize(digitize<f64>(2.5, edges, 3)), usize(3));
  }
  end_test_case();

  print("=== numeric ok ===");
  return 0;
}
