// math_tests.cpp
// Rigorous snowball test suite for micron::math
// Covers wide codomains for every function in the header.

#include "../../src/math/generic.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <cmath>     // reference values via std/builtin

// NOTE: -Ofast -ffast-math BREAKS this partially, be careful!

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_smaller;
using sb::require_true;
using sb::test_case;

namespace
{

// tolerance helpers
constexpr double EPS32 = 1e-5;
constexpr double EPS64 = 1e-10;
constexpr double EPSBIG = 1e-7;

bool
feq32(float a, float b, float eps = (float)EPS32)
{
  float d = a - b;
  return (d < 0 ? -d : d) <= eps;
}

bool
feq64(double a, double b, double eps = EPS64)
{
  double d = a - b;
  return (d < 0 ? -d : d) <= eps;
}

// integer exact equality shortcuts already handled by sb::require

}     // namespace

int
main()
{
  sb::print("=== MATH TESTS ===");

  // ================================================================ //
  //  gcd                                                             //
  // ================================================================ //
  test_case("gcd – positive pairs");
  {
    require(micron::math::gcd(12, 8), 4);
    require(micron::math::gcd(100, 75), 25);
    require(micron::math::gcd(17, 13), 1);     // coprime
    require(micron::math::gcd(1024, 768), 256);
    require(micron::math::gcd(999999937, 999999929), 1);           // large coprimes
    require(micron::math::gcd(2 * 3 * 5 * 7, 3 * 5 * 11), 15);     // lcm-style check
    require(micron::math::gcd(1, 1), 1);
    require(micron::math::gcd(1, 999), 1);
    require(micron::math::gcd(0, 7), 7);
    require(micron::math::gcd(7, 0), 7);
    require(micron::math::gcd(0, 0), 0);
  }
  end_test_case();

  test_case("gcd – negative inputs treated as absolute value");
  {
    require(micron::math::gcd(-12, 8), 4);
    require(micron::math::gcd(12, -8), 4);
    require(micron::math::gcd(-12, -8), 4);
    require(micron::math::gcd(-100, 75), 25);
    require(micron::math::gcd(0, -5), 5);
    require(micron::math::gcd(-5, 0), 5);
  }
  end_test_case();

  test_case("gcd – same-value inputs");
  {
    require(micron::math::gcd(7, 7), 7);
    require(micron::math::gcd(128, 128), 128);
    require(micron::math::gcd(1000000, 1000000), 1000000);
  }
  end_test_case();

  test_case("gcd – powers of two");
  {
    require(micron::math::gcd(256, 64), 64);
    require(micron::math::gcd(1024, 32), 32);
    require(micron::math::gcd(2, 1), 1);
    require(micron::math::gcd(65536, 4096), 4096);
  }
  end_test_case();

  // ================================================================ //
  //  abs (integral)                                                  //
  // ================================================================ //
  test_case("abs<int> – full signed range sample");
  {
    require(micron::math::abs(0), 0);
    require(micron::math::abs(1), 1);
    require(micron::math::abs(-1), 1);
    require(micron::math::abs(127), 127);
    require(micron::math::abs(-127), 127);
    require(micron::math::abs(2147483647), 2147483647);
    require(micron::math::abs(-2147483647), 2147483647);
  }
  end_test_case();

  test_case("abs<unsigned> – no-op on unsigned");
  {
    require(micron::math::abs((unsigned)0), (unsigned)0);
    require(micron::math::abs((unsigned)42), (unsigned)42);
    require(micron::math::abs((unsigned)4294967295u), (unsigned)4294967295u);
  }
  end_test_case();

  test_case("abs<i64> – large values");
  {
    require(micron::math::abs((long long)0), (long long)0);
    require(micron::math::abs((long long)-1), (long long)1);
    require(micron::math::abs((long long)-9000000000LL), (long long)9000000000LL);
    require(micron::math::abs((long long)9000000000LL), (long long)9000000000LL);
  }
  end_test_case();

  // ================================================================ //
  //  digits (integral)                                               //
  // ================================================================ //
  test_case("digits<int> – decimal digit count");
  {
    require(micron::math::digits(0), 1);
    require(micron::math::digits(1), 1);
    require(micron::math::digits(9), 1);
    require(micron::math::digits(10), 2);
    require(micron::math::digits(99), 2);
    require(micron::math::digits(100), 3);
    require(micron::math::digits(999), 3);
    require(micron::math::digits(1000), 4);
    require(micron::math::digits(9999), 4);
    require(micron::math::digits(1000000), 7);
    require(micron::math::digits(2147483647), 10);
  }
  end_test_case();

  test_case("digits<int> – negative inputs use absolute value");
  {
    require(micron::math::digits(-1), 1);
    require(micron::math::digits(-9), 1);
    require(micron::math::digits(-10), 2);
    require(micron::math::digits(-100), 3);
    require(micron::math::digits(-2147483647), 10);
  }
  end_test_case();

  test_case("digits<i64> – large 64-bit counts");
  {
    require(micron::math::digits((long long)1000000000LL), 10);
    require(micron::math::digits((long long)9999999999LL), 10);
    require(micron::math::digits((long long)10000000000LL), 11);
    require(micron::math::digits((long long)1000000000000LL), 13);
    require(micron::math::digits((long long)9999999999999999LL), 16);
  }
  end_test_case();

  // ================================================================ //
  //  power_loop                                                      //
  // ================================================================ //
  test_case("power_loop – small bases and exponents");
  {
    require(micron::math::power_loop(2, 0), 1);
    require(micron::math::power_loop(2, 1), 2);
    require(micron::math::power_loop(2, 10), 1024);
    require(micron::math::power_loop(3, 5), 243);
    require(micron::math::power_loop(10, 6), 1000000);
    require(micron::math::power_loop(1, 100), 1);
    require(micron::math::power_loop(0, 5), 0);
  }
  end_test_case();

  test_case("power_loop – negative exponent returns 0");
  {
    require(micron::math::power_loop(2, -1), 0);
    require(micron::math::power_loop(5, -3), 0);
  }
  end_test_case();

  test_case("power_loop – negative base");
  {
    require(micron::math::power_loop(-2, 3), -8);
    require(micron::math::power_loop(-3, 4), 81);
    require(micron::math::power_loop(-1, 99), -1);
    require(micron::math::power_loop(-1, 100), 1);
  }
  end_test_case();

  // ================================================================ //
  //  power (recursive fast exponentiation)                           //
  // ================================================================ //
  test_case("power – matches power_loop over wide domain");
  {
    for ( int base : { -5, -3, -2, -1, 0, 1, 2, 3, 5, 7, 10 } ) {
      for ( int exp = 0; exp <= 10; ++exp ) {
        require(micron::math::power(base, exp), micron::math::power_loop(base, exp));
      }
    }
  }
  end_test_case();

  test_case("power – zero exponent always yields 1");
  {
    for ( int base : { -100, -1, 0, 1, 100, 999 } )
      require(micron::math::power(base, 0), 1);
  }
  end_test_case();

  test_case("power – negative exponent returns 0");
  {
    require(micron::math::power(2, -1), 0);
    require(micron::math::power(99, -5), 0);
  }
  end_test_case();

  // ================================================================ //
  //  pow (iterative, binary exponentiation)                          //
  // ================================================================ //
  test_case("pow – matches power_loop over wide domain");
  {
    for ( int base : { 0, 1, 2, 3, 5, 7, 10, 12 } ) {
      for ( int exp = 0; exp <= 12; ++exp ) {
        require(micron::math::pow(base, exp), micron::math::power_loop(base, exp));
      }
    }
  }
  end_test_case();

  test_case("pow – negative exponent returns 0");
  {
    require(micron::math::pow(2, -1), 0);
    require(micron::math::pow(5, -3), 0);
  }
  end_test_case();

  // ================================================================ //
  //  fpow (floating point)                                           //
  // ================================================================ //
  test_case("fpow<float> – wide range of bases and exponents");
  {
    // exact integer powers
    require_true(feq32(micron::math::fpow(2.0f, 8.0f), 256.0f));
    require_true(feq32(micron::math::fpow(3.0f, 4.0f), 81.0f));
    require_true(feq32(micron::math::fpow(10.0f, 3.0f), 1000.0f));
    // fractional exponents
    require_true(feq32(micron::math::fpow(4.0f, 0.5f), 2.0f));
    require_true(feq32(micron::math::fpow(9.0f, 0.5f), 3.0f));
    require_true(feq32(micron::math::fpow(27.0f, 1.0f / 3.0f), 3.0f, 1e-4f));
    // x^0 == 1
    require_true(feq32(micron::math::fpow(999.0f, 0.0f), 1.0f));
    // 1^y == 1
    require_true(feq32(micron::math::fpow(1.0f, 999.0f), 1.0f));
    // 0^positive == 0
    require_true(feq32(micron::math::fpow(0.0f, 5.0f), 0.0f));
    // negative exponent
    require_true(feq32(micron::math::fpow(2.0f, -1.0f), 0.5f));
    require_true(feq32(micron::math::fpow(2.0f, -8.0f), 1.0f / 256.0f));
  }
  end_test_case();

  test_case("fpow<double> – precision and special values");
  {
    require_true(feq64(micron::math::fpow(2.0, 32.0), 4294967296.0));
    require_true(feq64(micron::math::fpow(10.0, 15.0), 1e15));
    require_true(feq64(micron::math::fpow(1.0001, 10000.0), std::pow(1.0001, 10000.0), 1e-3));
    require_true(feq64(micron::math::fpow(0.5, 10.0), 1.0 / 1024.0));
    require_true(feq64(micron::math::fpow(1.0, 1e15), 1.0));
  }
  end_test_case();

  // ================================================================ //
  //  fexp                                                            //
  // ================================================================ //
  test_case("fexp<float> – wide exponent domain");
  {
    require_true(feq32(micron::math::fexp(0.0f), 1.0f));
    require_true(feq32(micron::math::fexp(1.0f), 2.71828182f, 1e-5f));
    require_true(feq32(micron::math::fexp(-1.0f), 1.0f / 2.71828182f, 1e-5f));
    require_true(feq32(micron::math::fexp(2.0f), 7.38905609f, 1e-4f));
    require_true(feq32(micron::math::fexp(10.0f), 22026.4657948f, 0.1f));
    require_true(feq32(micron::math::fexp(-10.0f), (float)std::exp(-10.0f), 1e-8f));
  }
  end_test_case();

  test_case("fexp<double> – matches std::exp");
  {
    for ( double x : { -20.0, -10.0, -5.0, -1.0, -0.5, 0.0, 0.5, 1.0, 5.0, 10.0, 20.0 } ) {
      require_true(feq64(micron::math::fexp(x), std::exp(x), std::abs(std::exp(x)) * 1e-10 + 1e-15));
    }
  }
  end_test_case();

  // ================================================================ //
  //  log (float/double via logf32/logf64)                            //
  // ================================================================ //
  test_case("logf32 – positive reals");
  {
    require_true(feq32(micron::math::logf32(1.0f), 0.0f));
    require_true(feq32(micron::math::logf32(2.71828f), 1.0f, 1e-4f));
    require_true(feq32(micron::math::logf32(1.0f / 2.71828f), -1.0f, 1e-4f));
    require_true(feq32(micron::math::logf32(100.0f), (float)std::log(100.0f), 1e-4f));
    require_true(feq32(micron::math::logf32(1e-10f), (float)std::log(1e-10f), 1e-3f));
  }
  end_test_case();

  test_case("logf64 – matches std::log over wide domain");
  {
    for ( double x : { 1e-15, 1e-5, 0.1, 0.5, 1.0, 2.0, 10.0, 1e5, 1e15 } ) {
      require_true(feq64(micron::math::logf64(x), std::log(x), std::abs(std::log(x)) * 1e-12 + 1e-15));
    }
  }
  end_test_case();

  // ================================================================ //
  //  flog2                                                           //
  // ================================================================ //
  test_case("flog2<double> – exact powers of two");
  {
    for ( int i = 0; i <= 20; ++i ) {
      double x = (double)(1 << i);
      require_true(feq64(micron::math::flog2(x), (double)i));
    }
  }
  end_test_case();

  test_case("flog2<float> – fractional values");
  {
    require_true(feq32(micron::math::flog2(1.0f), 0.0f));
    require_true(feq32(micron::math::flog2(0.5f), -1.0f));
    require_true(feq32(micron::math::flog2(0.25f), -2.0f));
    require_true(feq32(micron::math::flog2(3.0f), (float)std::log2(3.0), 1e-5f));
  }
  end_test_case();

  // ================================================================ //
  //  log2 / log2ll (integer)                                         //
  // ================================================================ //
  test_case("log2<i32> – floor(log2) for positive integers");
  {
    require(micron::math::log2(1), 0);
    require(micron::math::log2(2), 1);
    require(micron::math::log2(3), 1);
    require(micron::math::log2(4), 2);
    require(micron::math::log2(7), 2);
    require(micron::math::log2(8), 3);
    require(micron::math::log2(255), 7);
    require(micron::math::log2(256), 8);
    require(micron::math::log2(1024), 10);
    require(micron::math::log2(2147483647), 30);
  }
  end_test_case();

  test_case("log2<i32> – zero and negative return -1");
  {
    require(micron::math::log2(0), -1);
    require(micron::math::log2(-1), -1);
    require(micron::math::log2(-100), -1);
  }
  end_test_case();

  test_case("log2ll<i64> – large values");
  {
    require(micron::math::log2ll((long long)1), (long long)0);
    require(micron::math::log2ll((long long)(1LL << 32)), (long long)32);
    require(micron::math::log2ll((long long)(1LL << 62)), (long long)62);
    require(micron::math::log2ll((long long)0), (long long)-1);
    require(micron::math::log2ll((long long)-1), (long long)-1);
  }
  end_test_case();

  // ================================================================ //
  //  log10f32 / log10f64                                             //
  // ================================================================ //
  test_case("log10f64 – matches std::log10 over decades");
  {
    for ( int i = -10; i <= 20; ++i ) {
      double x = std::pow(10.0, i);
      require_true(feq64(micron::math::log10f64(x), (double)i, 1e-9));
    }
  }
  end_test_case();

  test_case("log10f32 – spot checks");
  {
    require_true(feq32(micron::math::log10f32(1.0f), 0.0f));
    require_true(feq32(micron::math::log10f32(10.0f), 1.0f));
    require_true(feq32(micron::math::log10f32(100.0f), 2.0f));
    require_true(feq32(micron::math::log10f32(1000.0f), 3.0f));
    require_true(feq32(micron::math::log10f32(0.1f), -1.0f, 1e-5f));
  }
  end_test_case();

  // ================================================================ //
  //  round / ceil / floor                                            //
  // ================================================================ //
  test_case("round<double> – half-up rounding across negative and positive");
  {
    require_true(feq64(micron::math::round(0.0), 0.0));
    require_true(feq64(micron::math::round(0.4), 0.0));
    require_true(feq64(micron::math::round(0.5), 1.0));
    require_true(feq64(micron::math::round(0.9), 1.0));
    require_true(feq64(micron::math::round(1.5), 2.0));
    require_true(feq64(micron::math::round(-0.4), 0.0));
    require_true(feq64(micron::math::round(-0.5), -1.0));
    require_true(feq64(micron::math::round(-0.9), -1.0));
    require_true(feq64(micron::math::round(-1.5), -2.0));
    require_true(feq64(micron::math::round(100.5), 101.0));
    require_true(feq64(micron::math::round(-100.5), -101.0));
  }
  end_test_case();

  test_case("round<float> – spot checks");
  {
    require_true(feq32(micron::math::round(2.5f), 3.0f));
    require_true(feq32(micron::math::round(-2.5f), -3.0f));
    require_true(feq32(micron::math::round(0.0f), 0.0f));
  }
  end_test_case();

  test_case("ceil<double> – ceiling over wide range");
  {
    require_true(feq64(micron::math::ceil(0.0), 0.0));
    require_true(feq64(micron::math::ceil(0.1), 1.0));
    require_true(feq64(micron::math::ceil(0.9), 1.0));
    require_true(feq64(micron::math::ceil(1.0), 1.0));
    require_true(feq64(micron::math::ceil(-0.1), 0.0));
    require_true(feq64(micron::math::ceil(-0.9), 0.0));
    require_true(feq64(micron::math::ceil(-1.0), -1.0));
    require_true(feq64(micron::math::ceil(-1.5), -1.0));
    require_true(feq64(micron::math::ceil(100.01), 101.0));
    require_true(feq64(micron::math::ceil(-100.01), -100.0));
  }
  end_test_case();

  test_case("floor<double> – floor over wide range");
  {
    require_true(feq64(micron::math::floor(0.0), 0.0));
    require_true(feq64(micron::math::floor(0.9), 0.0));
    require_true(feq64(micron::math::floor(1.0), 1.0));
    require_true(feq64(micron::math::floor(1.9), 1.0));
    require_true(feq64(micron::math::floor(-0.1), -1.0));
    require_true(feq64(micron::math::floor(-0.9), -1.0));
    require_true(feq64(micron::math::floor(-1.0), -1.0));
    require_true(feq64(micron::math::floor(-1.001), -2.0));
    require_true(feq64(micron::math::floor(100.99), 100.0));
    require_true(feq64(micron::math::floor(-100.001), -101.0));
  }
  end_test_case();

  test_case("ceil/floor/round are mutually consistent");
  {
    for ( double x : { -3.7, -2.5, -1.1, -0.5, 0.0, 0.5, 1.1, 2.5, 3.7 } ) {
      double c = micron::math::ceil(x);
      double f = micron::math::floor(x);
      require_true(c >= f);
      require_true(c - f <= 1.0 + 1e-12);
      // for non-integer x, floor < x < ceil
      if ( x != micron::math::floor(x) ) {
        require_true(f < x);
        require_true(c > x);
      }
    }
  }
  end_test_case();

  // ================================================================ //
  //  ftrunc                                                          //
  // ================================================================ //
  test_case("ftrunc – truncation toward zero");
  {
    require_true(feq64(micron::math::ftrunc(0.0), 0.0));
    require_true(feq64(micron::math::ftrunc(1.9), 1.0));
    require_true(feq64(micron::math::ftrunc(-1.9), -1.0));
    require_true(feq64(micron::math::ftrunc(99.99), 99.0));
    require_true(feq64(micron::math::ftrunc(-99.99), -99.0));
    require_true(feq64(micron::math::ftrunc(0.001), 0.0));
    require_true(feq64(micron::math::ftrunc(-0.999), 0.0));
  }
  end_test_case();

  // ================================================================ //
  //  ffract                                                          //
  // ================================================================ //
  test_case("ffract – fractional part");
  {
    require_true(feq64(micron::math::ffract(0.0), 0.0));
    require_true(feq64(micron::math::ffract(1.75), 0.75));
    require_true(feq64(micron::math::ffract(-1.75), -0.75));
    require_true(feq64(micron::math::ffract(3.0), 0.0));
    require_true(feq64(micron::math::ffract(0.999), 0.999, 1e-9));
    require_true(feq64(micron::math::ffract(100.25), 0.25, 1e-9));
    // invariant: trunc + fract == original
    for ( double x : { -7.3, -2.0, -0.5, 0.0, 0.5, 2.0, 7.3, 123.456 } ) {
      require_true(feq64(micron::math::ftrunc(x) + micron::math::ffract(x), x, 1e-10));
    }
  }
  end_test_case();

  // ================================================================ //
  //  fclamp                                                          //
  // ================================================================ //
  test_case("fclamp<double> – values below, inside, and above range");
  {
    require_true(feq64(micron::math::fclamp(0.5, 1.0, 2.0), 1.0));     // below
    require_true(feq64(micron::math::fclamp(1.5, 1.0, 2.0), 1.5));     // inside
    require_true(feq64(micron::math::fclamp(2.5, 1.0, 2.0), 2.0));     // above
    require_true(feq64(micron::math::fclamp(1.0, 1.0, 2.0), 1.0));     // on lo
    require_true(feq64(micron::math::fclamp(2.0, 1.0, 2.0), 2.0));     // on hi
    require_true(feq64(micron::math::fclamp(-100.0, -1.0, 1.0), -1.0));
    require_true(feq64(micron::math::fclamp(100.0, -1.0, 1.0), 1.0));
    require_true(feq64(micron::math::fclamp(0.0, -1.0, 1.0), 0.0));
    // degenerate: lo == hi
    require_true(feq64(micron::math::fclamp(5.0, 3.0, 3.0), 3.0));
  }
  end_test_case();

  test_case("fclamp<float> – spot checks");
  {
    require_true(feq32(micron::math::fclamp(0.0f, 0.0f, 1.0f), 0.0f));
    require_true(feq32(micron::math::fclamp(0.5f, 0.0f, 1.0f), 0.5f));
    require_true(feq32(micron::math::fclamp(2.0f, 0.0f, 1.0f), 1.0f));
    require_true(feq32(micron::math::fclamp(-999.0f, -1.0f, 1.0f), -1.0f));
  }
  end_test_case();

  // ================================================================ //
  //  fmin / fmax                                                     //
  // ================================================================ //
  test_case("fmin<double> – all orderings");
  {
    require_true(feq64(micron::math::fmin(1.0, 2.0), 1.0));
    require_true(feq64(micron::math::fmin(2.0, 1.0), 1.0));
    require_true(feq64(micron::math::fmin(0.0, 0.0), 0.0));
    require_true(feq64(micron::math::fmin(-3.0, 3.0), -3.0));
    require_true(feq64(micron::math::fmin(-3.0, -5.0), -5.0));
    require_true(feq64(micron::math::fmin(1e15, 1e-15), 1e-15));
  }
  end_test_case();

  test_case("maxf64 – all orderings");
  {
    require_true(feq64(micron::math::maxf64(1.0, 2.0), 2.0));
    require_true(feq64(micron::math::maxf64(2.0, 1.0), 2.0));
    require_true(feq64(micron::math::maxf64(0.0, 0.0), 0.0));
    require_true(feq64(micron::math::maxf64(-3.0, 3.0), 3.0));
    require_true(feq64(micron::math::maxf64(-3.0, -5.0), -3.0));
    require_true(feq64(micron::math::maxf64(1e15, 1e-15), 1e15));
  }
  end_test_case();

  test_case("fabs<double> – absolute value of floats");
  {
    require_true(feq64(micron::math::fabs(0.0), 0.0));
    require_true(feq64(micron::math::fabs(1.0), 1.0));
    require_true(feq64(micron::math::fabs(-1.0), 1.0));
    require_true(feq64(micron::math::fabs(-1e300), 1e300));
    require_true(feq64(micron::math::fabs(1e-300), 1e-300));
  }
  end_test_case();

  // ================================================================ //
  //  nearest_pow2 / nearest_pow2ll                                   //
  // ================================================================ //
  test_case("nearest_pow2<i32> – rounds to closest power of two");
  {
    // exact powers stay
    require(micron::math::nearest_pow2(1), 1);
    require(micron::math::nearest_pow2(2), 2);
    require(micron::math::nearest_pow2(4), 4);
    require(micron::math::nearest_pow2(8), 8);
    require(micron::math::nearest_pow2(16), 16);
    require(micron::math::nearest_pow2(32), 32);
    require(micron::math::nearest_pow2(64), 64);
    require(micron::math::nearest_pow2(128), 128);
    require(micron::math::nearest_pow2(256), 256);
    // tie-break uses <=: equidistant values round DOWN to the lower power
    require(micron::math::nearest_pow2(3), 2);         // 3-2=1 == 4-3=1  → lower (2)
    require(micron::math::nearest_pow2(5), 4);         // 5-4=1  <  8-5=3 → lower (4)
    require(micron::math::nearest_pow2(6), 4);         // 6-4=2 == 8-6=2  → lower (4)
    require(micron::math::nearest_pow2(7), 8);         // 7-4=3  >  8-7=1 → upper (8)
    require(micron::math::nearest_pow2(9), 8);         // 9-8=1  <  16-9=7 → lower (8)
    require(micron::math::nearest_pow2(12), 8);        // 12-8=4 == 16-12=4 → lower (8)
    require(micron::math::nearest_pow2(100), 128);     // 100-64=36 > 128-100=28 → upper (128)
    require(micron::math::nearest_pow2(200), 256);     // 200-128=72 > 256-200=56 → upper (256)
    // edge: 0 and negatives → 1
    require(micron::math::nearest_pow2(0), 1);
    require(micron::math::nearest_pow2(-5), 1);
  }
  end_test_case();

  test_case("nearest_pow2ll<i64> – large values");
  {
    require(micron::math::nearest_pow2ll((long long)(1LL << 30)), (long long)(1LL << 30));
    require(micron::math::nearest_pow2ll((long long)0), (long long)1);
    require(micron::math::nearest_pow2ll((long long)-1), (long long)1);
    require(micron::math::nearest_pow2ll((long long)1000000000LL), (long long)(1LL << 30));
    require(micron::math::nearest_pow2ll((long long)1500000000LL), (long long)(1LL << 30));     // d_lo=426M < d_hi=647M -> lower (2^30)
    require(micron::math::nearest_pow2ll((long long)1610612737LL), (long long)(1LL << 31));     // just past crossover -> upper (2^31)
  }
  end_test_case();

  // ================================================================ //
  //  isfinite / isinf / isnan / isnormal                             //
  // ================================================================ //
  test_case("isfinite / isinf / isnan / isnormal – f64 classification");
  {
    double inf = __builtin_inf();
    double ninf = -__builtin_inf();
    double nan = __builtin_nan("");
    double norm = 1.0;
    double tiny_ = 5e-324;     // denormal (subnormal)

    require_true(micron::math::isfinite(norm));
    require_false(micron::math::isfinite(inf));
    require_false(micron::math::isfinite(ninf));
    require_false(micron::math::isfinite(nan));

    require_true(micron::math::isinf(inf));
    require_true(micron::math::isinf(ninf));
    require_false(micron::math::isinf(norm));
    require_false(micron::math::isinf(nan));

    require_true(micron::math::isnan(nan));
    require_false(micron::math::isnan(norm));
    require_false(micron::math::isnan(inf));

    require_true(micron::math::isnormal(norm));
    require_false(micron::math::isnormal(inf));
    require_false(micron::math::isnormal(nan));
    require_false(micron::math::isnormal(tiny_));     // subnormal
    require_false(micron::math::isnormal(0.0));
  }
  end_test_case();

  test_case("isfinite / isinf / isnan – f32 classification");
  {
    float inf = __builtin_inff();
    float nan = __builtin_nanf("");
    float norm = 1.0f;

    require_true(micron::math::isfinite(norm));
    require_false(micron::math::isfinite(inf));
    require_false(micron::math::isfinite(nan));

    require_true(micron::math::isinf(inf));
    require_false(micron::math::isinf(norm));

    require_true(micron::math::isnan(nan));
    require_false(micron::math::isnan(norm));
  }
  end_test_case();

  // ================================================================ //
  //  signbit                                                         //
  // ================================================================ //
  test_case("signbit – positive, negative, zero, special values");
  {
    require_false(micron::math::signbit(1.0));
    require_false(micron::math::signbit(0.0));
    require_false(micron::math::signbit(1e300));
    require_true(micron::math::signbit(-1.0));
    require_true(micron::math::signbit(-0.0));
    require_true(micron::math::signbit(-1e300));
    require_true(micron::math::signbit(-__builtin_inf()));     // -inf
    require_false(micron::math::signbit(__builtin_inf()));     // +inf

    require_false(micron::math::signbit(1.0f));
    require_true(micron::math::signbit(-1.0f));
  }
  end_test_case();

  // ================================================================ //
  //  isign                                                           //
  // ================================================================ //
  test_case("isign – three-way sign for integer and float");
  {
    require(micron::math::isign(5), 1);
    require(micron::math::isign(-5), -1);
    require(micron::math::isign(0), 0);
    require(micron::math::isign(1), 1);
    require(micron::math::isign(-1), -1);

    require(micron::math::isign(1.0), 1);
    require(micron::math::isign(-1.0), -1);
    require(micron::math::isign(0.0), 0);
    require(micron::math::isign(1e-300), 1);
    require(micron::math::isign(-1e-300), -1);
  }
  end_test_case();

  // ================================================================ //
  //  isgreater / isless / isgreaterequal / islessequal               //
  //  islessgreater / isunordered                                     //
  // ================================================================ //
  test_case("ordered comparisons – f64 normal values");
  {
    require_true(micron::math::isgreater(2.0, 1.0));
    require_false(micron::math::isgreater(1.0, 2.0));
    require_false(micron::math::isgreater(1.0, 1.0));

    require_true(micron::math::isgreaterequal(2.0, 2.0));
    require_true(micron::math::isgreaterequal(3.0, 2.0));
    require_false(micron::math::isgreaterequal(1.0, 2.0));

    require_true(micron::math::isless(1.0, 2.0));
    require_false(micron::math::isless(2.0, 1.0));
    require_false(micron::math::isless(1.0, 1.0));

    require_true(micron::math::islessequal(1.0, 1.0));
    require_true(micron::math::islessequal(1.0, 2.0));
    require_false(micron::math::islessequal(2.0, 1.0));

    require_true(micron::math::islessgreater(1.0, 2.0));
    require_true(micron::math::islessgreater(2.0, 1.0));
    require_false(micron::math::islessgreater(1.0, 1.0));
  }
  end_test_case();

  test_case("ordered comparisons – NaN produces false / isunordered true");
  {
    double nan = __builtin_nan("");
    require_false(micron::math::isgreater(nan, 1.0));
    require_false(micron::math::isless(nan, 1.0));
    require_false(micron::math::isgreaterequal(nan, 1.0));
    require_false(micron::math::islessequal(nan, 1.0));
    require_false(micron::math::islessgreater(nan, 1.0));
    require_true(micron::math::isunordered(nan, 1.0));
    require_true(micron::math::isunordered(nan, nan));
    require_false(micron::math::isunordered(1.0, 2.0));
  }
  end_test_case();

  // ================================================================ //
  //  copysign                                                        //
  // ================================================================ //
  test_case("copysign – magnitude from x, sign from y");
  {
    require_true(feq64(micron::math::copysign(3.0, 1.0), 3.0));
    require_true(feq64(micron::math::copysign(3.0, -1.0), -3.0));
    require_true(feq64(micron::math::copysign(-3.0, 1.0), 3.0));
    require_true(feq64(micron::math::copysign(-3.0, -1.0), -3.0));
    require_true(feq32(micron::math::copysign(5.0f, -0.0f), -5.0f));
    require_true(feq64(micron::math::copysign(1e300, -0.0), -1e300));
  }
  end_test_case();

  // ================================================================ //
  //  remainder                                                       //
  // ================================================================ //
  test_case("remainder<f64> – IEEE remainder (not fmod)");
  {
    // IEEE remainder: result in (-y/2, y/2]
    double r = micron::math::remainder(5.0, 3.0);
    require_true(r >= -1.5 - 1e-12 && r <= 1.5 + 1e-12);

    r = micron::math::remainder(0.5, 1.0);
    require_true(feq64(r, 0.5));

    r = micron::math::remainder(7.0, 4.0);     // 7 - 2*4 = -1
    require_true(feq64(r, -1.0));

    r = micron::math::remainder(-7.0, 4.0);     // -7 + 2*4 = 1
    require_true(feq64(r, 1.0));
  }
  end_test_case();

  // ================================================================ //
  //  rint / lrint / llrint / nearbyint                               //
  // ================================================================ //
  test_case("rint<f64> – rounds to nearest integer (ties to even)");
  {
    require_true(feq64(micron::math::rint(0.0), 0.0));
    require_true(feq64(micron::math::rint(0.4), 0.0));
    require_true(feq64(micron::math::rint(0.6), 1.0));
    require_true(feq64(micron::math::rint(-0.6), -1.0));
    require_true(feq64(micron::math::rint(1.5), 2.0));     // ties to even
    require_true(feq64(micron::math::rint(2.5), 2.0));     // ties to even
    require_true(feq64(micron::math::rint(3.5), 4.0));
  }
  end_test_case();

  test_case("lrint / llrint – integer return types");
  {
    require(micron::math::lrint(1.6), 2L);
    require(micron::math::lrint(-1.6), -2L);
    require(micron::math::llrint(2.5), 2LL);     // ties to even
    require(micron::math::llrint(3.5), 4LL);
  }
  end_test_case();

  test_case("nearbyint<f64> – does not raise FE_INEXACT (value check)");
  {
    require_true(feq64(micron::math::nearbyint(0.9), 1.0));
    require_true(feq64(micron::math::nearbyint(-0.9), -1.0));
    require_true(feq64(micron::math::nearbyint(2.5), 2.0));
  }
  end_test_case();

  // ================================================================ //
  //  scalbn / scalbln                                                //
  // ================================================================ //
  test_case("scalbn<f64> – multiply by 2^n");
  {
    require_true(feq64(micron::math::scalbn(1.0, 0), 1.0));
    require_true(feq64(micron::math::scalbn(1.0, 1), 2.0));
    require_true(feq64(micron::math::scalbn(1.0, -1), 0.5));
    require_true(feq64(micron::math::scalbn(1.5, 10), 1536.0));
    require_true(feq64(micron::math::scalbn(3.0, -2), 0.75));
    require_true(feq64(micron::math::scalbn(1.0, 53), (double)(1LL << 53)));
  }
  end_test_case();

  test_case("scalbln<f64> – long exponent variant");
  {
    require_true(feq64(micron::math::scalbln(1.0, 10L), 1024.0));
    require_true(feq64(micron::math::scalbln(1.0, -10L), 1.0 / 1024.0));
  }
  end_test_case();

  // ================================================================ //
  //  ffma                                                            //
  // ================================================================ //
  test_case("ffma<double> – fused multiply-add accuracy");
  {
    // exact cases
    require_true(feq64(micron::math::ffma(2.0, 3.0, 4.0), 10.0));
    require_true(feq64(micron::math::ffma(0.0, 999.0, 5.0), 5.0));
    require_true(feq64(micron::math::ffma(1.0, 1.0, 0.0), 1.0));
    require_true(feq64(micron::math::ffma(-2.0, 3.0, 10.0), 4.0));
    // precision check: a=(1+2^-27), b=(1-2^-27), c=-1
    // exact a*b-1 = -(2^-54); naive a*b rounds to 1.0 losing the term,
    // a proper FMA preserves it and returns the nonzero exact result
    double a_ = 1.0 + std::ldexp(1.0, -27);
    double b_ = 1.0 - std::ldexp(1.0, -27);
    double fma_result = micron::math::ffma(a_, b_, -1.0);
    double expected_fma = -(std::ldexp(1.0, -54));     // exact: -(2^-54)
    require_true(feq64(fma_result, expected_fma, std::ldexp(1.0, -60)));
  }
  end_test_case();

  test_case("ffmaf<float> – fused multiply-add float");
  {
    require_true(feq32(micron::math::ffmaf(2.0f, 3.0f, 4.0f), 10.0f));
    require_true(feq32(micron::math::ffmaf(-1.0f, 5.0f, 5.0f), 0.0f));
  }
  end_test_case();

  // ================================================================ //
  //  roundf32 / roundf64 (builtin wrappers)                         //
  // ================================================================ //
  test_case("roundf32 / roundf64 – delegate to builtins");
  {
    require_true(feq32(micron::math::roundf32(2.5f), 3.0f));
    require_true(feq32(micron::math::roundf32(-2.5f), -3.0f));
    require_true(feq64(micron::math::roundf64(2.5), 3.0));
    require_true(feq64(micron::math::roundf64(-2.5), -3.0));
  }
  end_test_case();

  // ================================================================ //
  //  expf32 / expf64 / expf128                                       //
  // ================================================================ //
  test_case("expf32 – matches std::expf over key values");
  {
    for ( float x : { -20.0f, -5.0f, -1.0f, 0.0f, 1.0f, 5.0f, 20.0f } ) {
      float ref = std::expf(x);
      require_true(feq32(micron::math::expf32(x), ref, ref * 1e-5f + 1e-30f));
    }
  }
  end_test_case();

  // ================================================================ //
  //  Cross-function invariants                                       //
  // ================================================================ //
  test_case("invariant: floor(x) <= x <= ceil(x)");
  {
    for ( double x : { -10.7, -1.0, -0.1, 0.0, 0.1, 1.0, 10.7, 1e6 + 0.3 } ) {
      require_true(micron::math::floor(x) <= x + 1e-12);
      require_true(micron::math::ceil(x) >= x - 1e-12);
    }
  }
  end_test_case();

  test_case("invariant: exp(log(x)) == x for positive x");
  {
    for ( double x : { 0.001, 0.5, 1.0, 2.0, 10.0, 1000.0, 1e10 } ) {
      double roundtrip = micron::math::fexp(micron::math::logf64(x));
      require_true(feq64(roundtrip, x, x * 1e-10 + 1e-15));
    }
  }
  end_test_case();

  test_case("invariant: pow(x,n) == x*x*...*x for small n");
  {
    for ( int base : { 2, 3, 5, 7 } ) {
      for ( int exp = 1; exp <= 8; ++exp ) {
        int brute = 1;
        for ( int k = 0; k < exp; ++k )
          brute *= base;
        require(micron::math::pow(base, exp), brute);
      }
    }
  }
  end_test_case();

  test_case("invariant: gcd(a,b) divides both a and b");
  {
    auto pairs
        = { std::make_pair(12, 8), std::make_pair(100, 75), std::make_pair(17, 13), std::make_pair(1024, 768), std::make_pair(360, 420) };
    for ( auto [a, b] : pairs ) {
      int g = micron::math::gcd(a, b);
      require(a % g, 0);
      require(b % g, 0);
    }
  }
  end_test_case();

  test_case("invariant: nearest_pow2 result is a power of two");
  {
    auto is_pow2 = [](int x) { return x > 0 && (x & (x - 1)) == 0; };
    for ( int v = 1; v <= 1000; ++v )
      require_true(is_pow2(micron::math::nearest_pow2(v)));
  }
  end_test_case();

  sb::print("=== ALL MATH TESTS PASSED ===");
  return 0;
}
