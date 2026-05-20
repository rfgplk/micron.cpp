// rigor_algo_math.cpp — snowball suite for src/algorithm/math.hpp
//
// Coverage:
//   sin / cos / tan / asin / acos / atan      (container, in-place)
//   sinh / cosh / tanh / asinh / acosh / atanh
//   exp / log / log10 / sqrt / cbrt
//   absolute / sign / clip / degrees / radians
//   derivative                                (numerical, central diff)
//   jacobian / hessian                        (vector<f64> only)
//
// Transcendentals are tested via mathematical identities rather than
// bit-exact comparison, with floating-point tolerances:
//   sin² + cos² = 1
//   exp(log(x)) = x  for x > 0
//   sqrt(x)² = x  for x >= 0
//   asin(sin(x)) = x  for x ∈ [-π/2, π/2]
// Container in-place semantics mean we apply f, then apply f⁻¹, and check
// equality.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/math.hpp"

#include "../support/algo_rigor.hpp"

using namespace mtest::rigor;
using mtest::prng;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

// helper: build f64 vector from buf
static micron::vector<f64>
to_vec(const f64 *buf, usize n)
{
  micron::vector<f64> v(n, 0.0);
  for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
  return v;
}

static constexpr f64 pi64 = 3.14159265358979323846;

int
main()
{
  sb::print("=== ALGO/MATH RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // Trig — sin / cos / tan
  // ════════════════════════════════════════════════════════════════════

  test_case("sin(0) == 0, sin(π/2) == 1, sin(π) ~ 0");
  {
    f64 data[3] = { 0.0, pi64 / 2.0, pi64 };
    auto v = to_vec(data, 3);
    micron::sin(v);
    require_true(near<f64>(v[0], 0.0, 1e-6));
    require_true(near<f64>(v[1], 1.0, 1e-6));
    require_true(near<f64>(v[2], 0.0, 1e-4));
  }
  end_test_case();

  test_case("cos(0) == 1, cos(π/2) ~ 0, cos(π) == -1");
  {
    f64 data[3] = { 0.0, pi64 / 2.0, pi64 };
    auto v = to_vec(data, 3);
    micron::cos(v);
    require_true(near<f64>(v[0], 1.0, 1e-6));
    require_true(near<f64>(v[1], 0.0, 1e-4));
    require_true(near<f64>(v[2], -1.0, 1e-6));
  }
  end_test_case();

  test_case("tan(0) == 0");
  {
    f64 data[1] = { 0.0 };
    auto v = to_vec(data, 1);
    micron::tan(v);
    require_true(near<f64>(v[0], 0.0, 1e-6));
  }
  end_test_case();

  test_case("sin² + cos² == 1 (identity, container)");
  {
    f64 angles[8] = { 0.0, pi64 / 6.0, pi64 / 4.0, pi64 / 3.0, pi64 / 2.0, 2.0 * pi64 / 3.0, pi64, -pi64 / 4.0 };
    auto s = to_vec(angles, 8);
    auto c = to_vec(angles, 8);
    micron::sin(s);
    micron::cos(c);
    for ( int i = 0; i < 8; ++i ) {
      f64 ident = s[i] * s[i] + c[i] * c[i];
      require_true(near<f64>(ident, 1.0, 1e-4));
    }
  }
  end_test_case();

  test_case("asin(sin(x)) == x for x in [-π/2, π/2]");
  {
    f64 data[5] = { -1.0, -0.5, 0.0, 0.5, 1.0 };
    auto v = to_vec(data, 5);
    micron::sin(v);
    micron::asin(v);
    for ( int i = 0; i < 5; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  test_case("acos(cos(x)) == x for x in [0, π]");
  {
    f64 data[4] = { 0.1, 0.5, 1.0, 2.0 };
    auto v = to_vec(data, 4);
    micron::cos(v);
    micron::acos(v);
    for ( int i = 0; i < 4; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  test_case("atan(tan(x)) == x for x in (-π/2, π/2)");
  {
    f64 data[4] = { -1.0, -0.3, 0.3, 1.0 };
    auto v = to_vec(data, 4);
    micron::tan(v);
    micron::atan(v);
    for ( int i = 0; i < 4; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Hyperbolic
  // ════════════════════════════════════════════════════════════════════

  test_case("sinh / cosh / tanh basic");
  {
    f64 data[3] = { 0.0, 1.0, -1.0 };
    auto s = to_vec(data, 3);
    auto c = to_vec(data, 3);
    auto t = to_vec(data, 3);
    micron::sinh(s);
    micron::cosh(c);
    micron::tanh(t);
    require_true(near<f64>(s[0], 0.0, 1e-6));
    require_true(near<f64>(c[0], 1.0, 1e-6));
    require_true(near<f64>(t[0], 0.0, 1e-6));
  }
  end_test_case();

  test_case("cosh² - sinh² == 1 (identity)");
  {
    f64 data[5] = { -2.0, -1.0, 0.0, 1.0, 2.0 };
    auto s = to_vec(data, 5);
    auto c = to_vec(data, 5);
    micron::sinh(s);
    micron::cosh(c);
    for ( int i = 0; i < 5; ++i ) {
      f64 ident = c[i] * c[i] - s[i] * s[i];
      require_true(near<f64>(ident, 1.0, 1e-3));
    }
  }
  end_test_case();

  test_case("asinh(sinh(x)) == x");
  {
    f64 data[5] = { -2.0, -1.0, 0.0, 1.0, 2.0 };
    auto v = to_vec(data, 5);
    micron::sinh(v);
    micron::asinh(v);
    for ( int i = 0; i < 5; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  test_case("acosh(cosh(x)) == |x|");
  {
    f64 data[4] = { 0.5, 1.0, 1.5, 2.0 };
    auto v = to_vec(data, 4);
    micron::cosh(v);
    micron::acosh(v);
    for ( int i = 0; i < 4; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  test_case("atanh(tanh(x)) == x for |x| < 1");
  {
    f64 data[4] = { -0.9, -0.3, 0.3, 0.9 };
    auto v = to_vec(data, 4);
    micron::tanh(v);
    micron::atanh(v);
    for ( int i = 0; i < 4; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // exp / log / log10
  // ════════════════════════════════════════════════════════════════════

  test_case("exp(0) == 1, exp(1) == e");
  {
    f64 data[2] = { 0.0, 1.0 };
    auto v = to_vec(data, 2);
    micron::exp(v);
    require_true(near<f64>(v[0], 1.0, 1e-6));
    require_true(near<f64>(v[1], 2.71828182845904, 1e-6));
  }
  end_test_case();

  test_case("log(1) == 0, log(e) ~ 1");
  {
    f64 data[2] = { 1.0, 2.71828182845904 };
    auto v = to_vec(data, 2);
    micron::log(v);
    require_true(near<f64>(v[0], 0.0, 1e-6));
    require_true(near<f64>(v[1], 1.0, 1e-6));
  }
  end_test_case();

  test_case("exp(log(x)) == x for x > 0 (identity)");
  {
    f64 data[5] = { 0.5, 1.0, 2.0, 5.0, 10.0 };
    auto v = to_vec(data, 5);
    micron::log(v);
    micron::exp(v);
    for ( int i = 0; i < 5; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  test_case("log10(1) == 0, log10(10) ~ 1, log10(100) ~ 2");
  {
    f64 data[3] = { 1.0, 10.0, 100.0 };
    auto v = to_vec(data, 3);
    micron::log10(v);
    require_true(near<f64>(v[0], 0.0, 1e-6));
    require_true(near<f64>(v[1], 1.0, 1e-6));
    require_true(near<f64>(v[2], 2.0, 1e-6));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // sqrt / cbrt
  // ════════════════════════════════════════════════════════════════════

  test_case("sqrt basic values");
  {
    f64 data[4] = { 0.0, 1.0, 4.0, 9.0 };
    auto v = to_vec(data, 4);
    micron::sqrt(v);
    require_true(near<f64>(v[0], 0.0, 1e-6));
    require_true(near<f64>(v[1], 1.0, 1e-6));
    require_true(near<f64>(v[2], 2.0, 1e-6));
    require_true(near<f64>(v[3], 3.0, 1e-6));
  }
  end_test_case();

  test_case("sqrt(x)² == x for x >= 0 (identity)");
  {
    f64 data[6] = { 0.25, 1.0, 2.0, 7.5, 16.0, 100.0 };
    auto v = to_vec(data, 6);
    micron::sqrt(v);
    for ( int i = 0; i < 6; ++i ) {
      f64 sq = v[i] * v[i];
      require_true(near<f64>(sq, data[i], 1e-3));
    }
  }
  end_test_case();

  test_case("cbrt basic values");
  {
    f64 data[3] = { 1.0, 8.0, 27.0 };
    auto v = to_vec(data, 3);
    micron::cbrt(v);
    require_true(near<f64>(v[0], 1.0, 1e-6));
    require_true(near<f64>(v[1], 2.0, 1e-6));
    require_true(near<f64>(v[2], 3.0, 1e-6));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // absolute / sign / clip / degrees / radians
  // ════════════════════════════════════════════════════════════════════

  test_case("absolute flips negative");
  {
    f64 data[4] = { -3.5, -1.0, 0.0, 5.0 };
    auto v = to_vec(data, 4);
    micron::absolute(v);
    require_true(near<f64>(v[0], 3.5, 1e-6));
    require_true(near<f64>(v[1], 1.0, 1e-6));
    require_true(near<f64>(v[2], 0.0, 1e-6));
    require_true(near<f64>(v[3], 5.0, 1e-6));
  }
  end_test_case();

  test_case("sign returns -1 / 0 / +1");
  {
    f64 data[4] = { -3.0, 0.0, 5.0, -100.0 };
    auto v = to_vec(data, 4);
    micron::sign(v);
    require_true(near<f64>(v[0], -1.0, 1e-9));
    require_true(near<f64>(v[1], 0.0, 1e-9));
    require_true(near<f64>(v[2], 1.0, 1e-9));
    require_true(near<f64>(v[3], -1.0, 1e-9));
  }
  end_test_case();

  test_case("clip restricts to range");
  {
    f64 data[5] = { -10.0, -1.0, 0.5, 3.0, 100.0 };
    auto v = to_vec(data, 5);
    micron::clip(v, -1.0, 2.0);
    require_true(near<f64>(v[0], -1.0, 1e-9));
    require_true(near<f64>(v[1], -1.0, 1e-9));
    require_true(near<f64>(v[2], 0.5, 1e-9));
    require_true(near<f64>(v[3], 2.0, 1e-9));
    require_true(near<f64>(v[4], 2.0, 1e-9));
  }
  end_test_case();

  test_case("degrees converts radians to degrees");
  {
    f64 data[3] = { 0.0, pi64, pi64 / 2.0 };
    auto v = to_vec(data, 3);
    micron::degrees(v);
    require_true(near<f64>(v[0], 0.0, 1e-6));
    require_true(near<f64>(v[1], 180.0, 1e-4));
    require_true(near<f64>(v[2], 90.0, 1e-4));
  }
  end_test_case();

  test_case("radians converts degrees to radians");
  {
    f64 data[3] = { 0.0, 180.0, 90.0 };
    auto v = to_vec(data, 3);
    micron::radians(v);
    require_true(near<f64>(v[0], 0.0, 1e-6));
    require_true(near<f64>(v[1], pi64, 1e-4));
    require_true(near<f64>(v[2], pi64 / 2.0, 1e-4));
  }
  end_test_case();

  test_case("degrees + radians is identity");
  {
    f64 data[4] = { 0.5, 1.0, 2.0, pi64 };
    auto v = to_vec(data, 4);
    micron::degrees(v);
    micron::radians(v);
    for ( int i = 0; i < 4; ++i ) require_true(near<f64>(v[i], data[i], 1e-4));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // derivative
  // ════════════════════════════════════════════════════════════════════

  test_case("derivative of x² at x=3 is ~ 6");
  {
    auto f = [](f64 x) { return x * x; };
    f64 d = micron::derivative(f, f64(3.0));
    require_true(near<f64>(d, 6.0, 1e-3));
  }
  end_test_case();

  test_case("derivative of sin(x) at x=0 is ~ 1 (cos(0))");
  {
    auto f = [](f64 x) { return micron::math::sin(x); };
    f64 d = micron::derivative(f, f64(0.0));
    require_true(near<f64>(d, 1.0, 1e-3));
  }
  end_test_case();

  test_case("derivative of exp(x) at x=0 is ~ 1 (exp(0))");
  {
    auto f = [](f64 x) { return micron::math::exp(x); };
    f64 d = micron::derivative(f, f64(0.0));
    require_true(near<f64>(d, 1.0, 1e-3));
  }
  end_test_case();

  test_case("derivative of x³ at x=2 is ~ 12");
  {
    auto f = [](f64 x) { return x * x * x; };
    f64 d = micron::derivative(f, f64(2.0));
    require_true(near<f64>(d, 12.0, 1e-3));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // 10k property tests — identity check
  // ════════════════════════════════════════════════════════════════════

  property_test(
      "sin² + cos² == 1 for random angles (10k)",
      [](u32 raw_n) {
        f64 x = static_cast<f64>(static_cast<i32>(raw_n)) / 1e7;
        f64 s = micron::math::sin(x);
        f64 c = micron::math::cos(x);
        f64 ident = s * s + c * c;
        require_true(near<f64>(ident, 1.0, 1e-6));
      },
      10000);

  property_test(
      "exp(log(x)) ~ x for x > 0 (10k)",
      [](u32 raw_n) {
        f64 x = 0.001 + (static_cast<f64>(raw_n & 0xfffff) / 1000.0);      // 0.001..~1049
        f64 v = micron::math::log(x);
        v = micron::math::exp(v);
        require_true(near_rel<f64>(v, x, 1e-4));
      },
      10000);

  property_test(
      "sqrt(x²) == |x| (10k)",
      [](u32 raw_n) {
        f64 x = static_cast<f64>(static_cast<i32>(raw_n)) / 1e6;
        f64 sq = x * x;
        f64 r = micron::math::sqrt(sq);
        f64 absx = x < 0 ? -x : x;
        require_true(near_rel<f64>(r, absx, 1e-4));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // jacobian / hessian (vector<f64> only — require V::rebind)
  // ════════════════════════════════════════════════════════════════════

  test_case("jacobian of f(x) = sum(x_i²) at x = [1,2,3] is [2,4,6]");
  {
    auto f = [](const micron::vector<f64> &x) -> f64 {
      f64 s = 0;
      for ( usize i = 0; i < x.size(); ++i ) s += x[i] * x[i];
      return s;
    };
    micron::vector<f64> x(3, 0.0);
    x[0] = 1.0;
    x[1] = 2.0;
    x[2] = 3.0;
    auto g = micron::jacobian(f, x);
    require_true(near<f64>(g[0], 2.0, 1e-3));
    require_true(near<f64>(g[1], 4.0, 1e-3));
    require_true(near<f64>(g[2], 6.0, 1e-3));
  }
  end_test_case();

  sb::print("=== ALGO/MATH RIGOR SUITE PASSED ===");
  return 1;
}
