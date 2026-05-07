// math_log_ext.cpp
// Rigorous snowball test suite for math::log — extended log /
// softmax / sigmoid set.

#include "../../src/math/log.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
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
  print("=== MATH::LOG_EXT TESTS ===");

  test_case("log_base — log_2(8) = 3");
  {
    require_true(near(log_base<f64>(8.0, 2.0), 3.0));
    require_true(near(log_base<f64>(1000.0, 10.0), 3.0));
  }
  end_test_case();

  test_case("log2p1 — small x stable");
  {
    require_true(near(log2p1<f64>(0.0), 0.0));
    require_true(near(log2p1<f64>(1.0), 1.0));
    // log2(1 + 1e-10) ≈ 1.4426950408e-10
    require_true(near(log2p1<f64>(1e-10), 1.4426950408e-10, 1e-18));
  }
  end_test_case();

  test_case("log_sum_exp — pairwise stable");
  {
    require_true(near(log_sum_exp<f64>(0.0, 0.0), constant_ln2<f64>));
    // log_sum_exp(1000, 1000) — must NOT overflow
    f64 r = log_sum_exp<f64>(1000.0, 1000.0);
    require_true(near(r, 1000.0 + constant_ln2<f64>, 1e-9));
  }
  end_test_case();

  test_case("log_sum_exp_n — batch matches identity");
  {
    f64 v[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    f64 r = log_sum_exp_n<f64>(v, 5);
    require_true(near(r, 5.451914395181, 1e-9));
    // single-element span equals the element
    f64 v1[1] = { 7.5 };
    require_true(near(log_sum_exp_n<f64>(v1, 1), 7.5));
  }
  end_test_case();

  test_case("logistic / sigmoid");
  {
    require_true(near(logistic<f64>(0.0), 0.5));
    require_true(near(logistic<f64>(100.0), 1.0, 1e-12));
    require_true(near(logistic<f64>(-100.0), 0.0, 1e-12));
    // log_logistic stable for both signs
    require_true(near(log_logistic<f64>(0.0), -constant_ln2<f64>));
    require_true(log_logistic<f64>(-100.0) < -99.0);
  }
  end_test_case();

  test_case("softplus — overflow guard");
  {
    require_true(near(softplus<f64>(0.0), constant_ln2<f64>));
    require_true(near(softplus<f64>(50.0), 50.0, 1e-12));
    // softplus(-50) ≈ exp(-50)
    require_true(softplus<f64>(-50.0) < 1e-15);
  }
  end_test_case();

  test_case("log1mexp — domain x <= 0");
  {
    // log(1 - e^-1) ≈ -0.4586751453870819
    require_true(near(log1mexp<f64>(-1.0), -0.4586751453870819, 1e-9));
    // log(1 - e^-10) ≈ -4.54e-5
    require_true(near(log1mexp<f64>(-10.0), -4.5400e-5, 1e-7));
  }
  end_test_case();

  test_case("log_diff_exp — exp(2)-exp(1) check");
  {
    require_true(near(log_diff_exp<f64>(2.0, 1.0), 2.0 - 0.4586751453870819, 1e-9));
  }
  end_test_case();

  test_case("xlogx / xlogy — exact 0 case");
  {
    require_true(xlogx<f64>(0.0) == 0.0);
    require_true(xlogy<f64>(0.0, 1e9) == 0.0);
    // xlogx(e) = e
    require_true(near(xlogx<f64>(constant_e<f64>), constant_e<f64>, 1e-12));
  }
  end_test_case();

  test_case("softmax — sums to 1");
  {
    f64 sm[3] = { 1.0, 2.0, 3.0 };
    softmax<f64>(sm, 3);
    f64 s = sm[0] + sm[1] + sm[2];
    require_true(near(s, 1.0));
    // monotonic increasing input → monotonic increasing output
    require_true(sm[0] < sm[1] && sm[1] < sm[2]);
  }
  end_test_case();

  test_case("log_softmax — sums to log_sum_exp");
  {
    f64 v[4] = { -1.0, 0.0, 1.0, 2.0 };
    f64 v2[4] = { -1.0, 0.0, 1.0, 2.0 };
    log_softmax<f64>(v, 4);
    // sum exp(log_softmax) should be 1
    f64 s = 0.0;
    for ( int i = 0; i < 4; ++i ) s += fexp(v[i]);
    require_true(near(s, 1.0, 1e-9));
    (void)v2;
  }
  end_test_case();

  print("=== MATH::LOG_EXT TESTS PASSED ===");
  return 1;
}
