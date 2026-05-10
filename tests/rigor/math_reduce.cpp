// math_reduce.cpp
// Snowball tests for math::reduce — sum/mean/var/std/min/max/median/cumsum.

#include "../../src/math/reduce.hpp"
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
near(f64 a, f64 b, f64 eps = 1e-12)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== REDUCE TESTS ===");

  test_case("sum / prod");
  {
    f64 a[5] = { 1, 2, 3, 4, 5 };
    require_true(near(sum<f64>(a, a + 5), 15.0));
    require_true(near(prod<f64>(a, a + 5), 120.0));

    f64 empty[1];
    require_true(near(sum<f64>(empty, empty), 0.0));
  }
  end_test_case();

  test_case("kahan_sum / neumaier_sum stability");
  {
    // Classic ill-conditioned sum: 1 + (many tiny) + (-1).
    f64 v[100002];
    v[0] = 1e16;
    for ( int i = 1; i <= 100000; ++i ) v[i] = 1.0;
    v[100001] = -1e16;
    f64 ks = kahan_sum<f64>(v, v + 100002);
    f64 ns = neumaier_sum<f64>(v, v + 100002);
    require_true(near(ks, 100000.0, 1e-6));
    require_true(near(ns, 100000.0, 1e-6));
  }
  end_test_case();

  test_case("mean / var / std_dev (Welford)");
  {
    f64 v[5] = { 2, 4, 4, 4, 5 };
    require_true(near(mean<f64>(v, v + 5), 3.8));
    // Population variance: Σ(xᵢ−μ)² / n = 4.8 / 5 = 0.96
    require_true(near(var<f64>(v, v + 5), 0.96, 1e-12));
    require_true(near(std_dev<f64>(v, v + 5), 0.9797958971132712, 1e-9));
  }
  end_test_case();

  test_case("min / max / argmin / argmax / ptp");
  {
    f64 v[6] = { 3, 1, 4, 1, 5, 9 };
    require_true(near(micron::math::min<f64>(v, v + 6), 1.0));
    require_true(near(micron::math::max<f64>(v, v + 6), 9.0));
    require(usize(argmin<f64>(v, v + 6)), usize(1));
    require(usize(argmax<f64>(v, v + 6)), usize(5));
    require_true(near(ptp<f64>(v, v + 6), 8.0));
  }
  end_test_case();

  test_case("median / quantile");
  {
    f64 odd[5] = { 5, 1, 4, 2, 3 };
    require_true(near(median<f64>(odd, odd + 5), 3.0));

    f64 even[6] = { 5, 1, 4, 2, 3, 6 };
    require_true(near(median<f64>(even, even + 6), 3.5));

    f64 q[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    require_true(near(quantile<f64>(q, q + 10, 0.5), 4.5, 1e-10));
    require_true(near(quantile<f64>(q, q + 10, 0.0), 0.0, 1e-10));
    require_true(near(quantile<f64>(q, q + 10, 1.0), 9.0, 1e-10));
  }
  end_test_case();

  test_case("cumsum / cumprod");
  {
    f64 in[4] = { 1, 2, 3, 4 };
    f64 out[4] = { 0 };
    cumsum<f64>(in, in + 4, out);
    require_true(near(out[0], 1.0));
    require_true(near(out[1], 3.0));
    require_true(near(out[2], 6.0));
    require_true(near(out[3], 10.0));

    f64 op[4] = { 0 };
    cumprod<f64>(in, in + 4, op);
    require_true(near(op[3], 24.0));
  }
  end_test_case();

  test_case("container overloads — vector<f64>");
  {
    micron::vector<f64> v(5, 0.0);
    for ( usize i = 0; i < 5; ++i ) v[i] = f64(i + 1);
    require_true(near(f64(sum(v)), 15.0));
    require_true(near(f64(micron::math::mean(v)), 3.0));
    require_true(near(micron::math::min(v), 1.0));
    require_true(near(micron::math::max(v), 5.0));
  }
  end_test_case();

  print("=== reduce ok ===");
  return 0;
}
