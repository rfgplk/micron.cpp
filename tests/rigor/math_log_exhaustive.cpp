// math_log_exhaustive.cpp
// Exhaustive rigor tests for every micron::math log-family surface.
//
// Coverage matrix:
//   forward                      : log, log2, log10, log1p           (f32, f64)
//   non-template overloads       : log(float), log(double), log(long double), log10, flog, flog10, flog2
//   compositional                : log_base, log2p1                  (f32, f64)
//   exp/log mixed identities     : log1mexp, log_diff_exp, log_sum_exp, log_sum_exp_n
//   neural-net porcelain         : logistic, log_logistic, softplus, xlogx, xlogy
//   batch reductions             : softmax, log_softmax              (in-place and out-of-place)
//   top-level reexport           : micron::log etc. agree with micron::math::log etc.
//
// Each surface gets:
//   - identity tests       (log(exp(x))=x, exp(log(x))=x, log(a*b)=log(a)+log(b))
//   - special-value tests  (log(0)=-inf, log(1)=0, log(e)=1, NaN, ±inf, negatives)
//   - cross-base tests     (log2(2^k)=k, log10(10^k)=k)
//   - subnormal / tiny x   (log1p(x) ≈ x near zero)
//   - cross-precision      (f32 path produces values within f32 ulp band)
//
// Soft-failure model copied from math_trig_vs_libm.cpp: hard `require_true` for
// invariants we believe must hold, `soft_check` for documented quirks that we
// keep on record without aborting the run. Sweep loops accumulate failures and
// report a final count rather than breaking on the first miss, so the
// signal-to-noise of a regression report is high.

#include "../../src/math.hpp"
#include "../../src/math/ieee.hpp"
#include "../../src/math/log.hpp"
#include "../../src/math/mk.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <cmath>

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace
{

struct soft_stats {
  unsigned long failures = 0;
  unsigned long checks = 0;
};

inline soft_stats &
__soft()
{
  static soft_stats s;
  return s;
}

inline void
soft_check(bool ok, const char *what)
{
  ++__soft().checks;
  if ( !ok ) {
    ++__soft().failures;
    sb::print("    [FAILS] ", what);
  }
}

static inline double
adouble(double v) noexcept
{
  return v < 0 ? -v : v;
}

static inline float
afloat(float v) noexcept
{
  return v < 0 ? -v : v;
}

static inline bool
near_d(double a, double b, double abs_eps, double rel_eps) noexcept
{
  const double d = adouble(a - b);
  const double m = adouble(b);
  return d <= abs_eps + rel_eps * m;
}

static inline bool
near_f(float a, float b, float abs_eps, float rel_eps) noexcept
{
  const float d = afloat(a - b);
  const float m = afloat(b);
  return d <= abs_eps + rel_eps * m;
}

static constexpr double kE = 2.718281828459045235360287471352662498;
static constexpr double kLn2 = 0.693147180559945309417232121458176568;
static constexpr double kLn10 = 2.302585092994045684017991454684364208;
static constexpr double kLog2e = 1.442695040888963407359924681001892137;

[[gnu::always_inline]] inline bool
is_nan_d(double v) noexcept
{
  const unsigned long long u = micron::math::ieee::to_bits<double>(v);
  const unsigned long long exp = u & 0x7FF0000000000000ULL;
  const unsigned long long mant = u & 0x000FFFFFFFFFFFFFULL;
  return exp == 0x7FF0000000000000ULL && mant != 0;
}

[[gnu::always_inline]] inline bool
is_nan_f(float v) noexcept
{
  const unsigned int u = micron::math::ieee::to_bits<float>(v);
  const unsigned int exp = u & 0x7F800000U;
  const unsigned int mant = u & 0x007FFFFFU;
  return exp == 0x7F800000U && mant != 0;
}

[[gnu::always_inline]] inline bool
is_inf_d(double v) noexcept
{
  const unsigned long long u = micron::math::ieee::to_bits<double>(v) & 0x7FFFFFFFFFFFFFFFULL;
  return u == 0x7FF0000000000000ULL;
}

[[gnu::always_inline]] inline bool
is_inf_f(float v) noexcept
{
  const unsigned int u = micron::math::ieee::to_bits<float>(v) & 0x7FFFFFFFU;
  return u == 0x7F800000U;
}

[[gnu::always_inline]] inline bool
is_pos_inf_d(double v) noexcept
{
  return micron::math::ieee::to_bits<double>(v) == 0x7FF0000000000000ULL;
}

[[gnu::always_inline]] inline bool
is_neg_inf_d(double v) noexcept
{
  return micron::math::ieee::to_bits<double>(v) == 0xFFF0000000000000ULL;
}

[[gnu::always_inline]] inline bool
is_pos_inf_f(float v) noexcept
{
  return micron::math::ieee::to_bits<float>(v) == 0x7F800000U;
}

[[gnu::always_inline]] inline bool
is_neg_inf_f(float v) noexcept
{
  return micron::math::ieee::to_bits<float>(v) == 0xFF800000U;
}

[[gnu::always_inline]] inline constexpr double
inf_d() noexcept
{
  return micron::math::ieee::inf_v<double>(0);
}

[[gnu::always_inline]] inline constexpr double
ninf_d() noexcept
{
  return micron::math::ieee::inf_v<double>(1);
}

[[gnu::always_inline]] inline constexpr double
nan_d() noexcept
{
  return micron::math::ieee::qnan_v<double>();
}

[[gnu::always_inline]] inline constexpr float
inf_f() noexcept
{
  return micron::math::ieee::inf_v<float>(0);
}

[[gnu::always_inline]] inline constexpr float
ninf_f() noexcept
{
  return micron::math::ieee::inf_v<float>(1);
}

[[gnu::always_inline]] inline constexpr float
nan_f() noexcept
{
  return micron::math::ieee::qnan_v<float>();
}

};      // namespace

int
main()
{
  print("=== LOG EXHAUSTIVE TESTS ===");

  test_case("log f64 — log(1)=0, log(e)=1, log(0)=-inf");
  {
    require_true(micron::math::log<double>(1.0) == 0.0);
    require_true(near_d(micron::math::log<double>(kE), 1.0, 1e-15, 1e-15));
    require_true(is_neg_inf_d(micron::math::log<double>(0.0)));
    require_true(is_neg_inf_d(micron::math::log<double>(-0.0)));
  }
  end_test_case();

  test_case("log f64 — NaN, ±inf, negatives");
  {
    require_true(is_nan_d(micron::math::log<double>(nan_d())));
    require_true(is_pos_inf_d(micron::math::log<double>(inf_d())));
    require_true(is_nan_d(micron::math::log<double>(-1.0)));
    require_true(is_nan_d(micron::math::log<double>(-0.5)));
    require_true(is_nan_d(micron::math::log<double>(ninf_d())));
    require_true(is_nan_d(micron::math::log<double>(-1e-300)));
  }
  end_test_case();

  test_case("log f64 — log(2^k) = k * ln2 for integer k");
  {
    unsigned fails = 0;
    for ( int k = -100; k <= 100; ++k ) {
      const double x = __builtin_ldexp(1.0, k);
      const double y = micron::math::log<double>(x);
      const double ref = double(k) * kLn2;
      if ( !near_d(y, ref, 1e-14, 1e-14) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log f64 — log(a*b) = log(a) + log(b) identity sweep");
  {
    unsigned fails = 0;
    for ( double a = 0.1; a <= 100.0; a *= 1.3 ) {
      for ( double b = 0.1; b <= 100.0; b *= 1.3 ) {
        const double lhs = micron::math::log<double>(a * b);
        const double rhs = micron::math::log<double>(a) + micron::math::log<double>(b);
        if ( !near_d(lhs, rhs, 1e-13, 1e-13) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log f64 — exp(log(x)) = x roundtrip sweep");
  {
    unsigned fails = 0;
    for ( double x = 1e-30; x < 1e30; x *= 1.7 ) {
      const double y = micron::math::log<double>(x);
      const double rt = micron::math::exp<double>(y);
      if ( !near_d(rt, x, 1e-13 * adouble(x), 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log f64 — log(exp(x)) = x roundtrip sweep");
  {
    unsigned fails = 0;
    for ( double x = -300.0; x <= 300.0; x += 0.317 ) {
      const double y = micron::math::exp<double>(x);
      if ( !(y > 0.0) ) continue;
      if ( is_inf_d(y) ) continue;
      const double rt = micron::math::log<double>(y);
      if ( !near_d(rt, x, 1e-13, 1e-14) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log f64 — denormal / subnormal inputs");
  {

    const double xs[] = { 0x1.0p-1022, 0x1.0p-1000, 0x1.0p-700, 1e-300, 1e-200 };
    unsigned fails = 0;
    for ( unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); ++i ) {
      const double y = micron::math::log<double>(xs[i]);
      if ( is_nan_d(y) ) ++fails;
      if ( !(y < 0.0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log f64 — near-1 behavior matches small expansion");
  {

    unsigned fails = 0;
    for ( double d = -0.5; d <= 0.5; d += 0.013 ) {
      const double x = 1.0 + d;
      const double y = micron::math::log<double>(x);
      const double ref = __builtin_log(x);
      if ( !near_d(y, ref, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log f32 — boundaries, NaN, ±inf, negatives");
  {
    require_true(micron::math::log<float>(1.0f) == 0.0f);
    require_true(near_f(micron::math::log<float>(float(kE)), 1.0f, 1e-7f, 1e-7f));
    require_true(is_neg_inf_f(micron::math::log<float>(0.0f)));
    require_true(is_nan_f(micron::math::log<float>(nan_f())));
    require_true(is_pos_inf_f(micron::math::log<float>(inf_f())));
    require_true(is_nan_f(micron::math::log<float>(-1.0f)));
    require_true(is_nan_f(micron::math::log<float>(ninf_f())));
  }
  end_test_case();

  test_case("log f32 — log(2^k) = k * ln2 sweep");
  {
    unsigned fails = 0;
    for ( int k = -50; k <= 50; ++k ) {
      const float x = float(__builtin_ldexp(1.0, k));
      const float y = micron::math::log<float>(x);
      const float ref = float(double(k) * kLn2);
      if ( !near_f(y, ref, 1e-5f, 1e-5f) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log f32 — log(a*b) = log(a) + log(b)");
  {
    unsigned fails = 0;
    for ( float a = 0.5f; a <= 100.0f; a *= 1.7f ) {
      for ( float b = 0.5f; b <= 100.0f; b *= 1.7f ) {
        const float lhs = micron::math::log<float>(a * b);
        const float rhs = micron::math::log<float>(a) + micron::math::log<float>(b);
        if ( !near_f(lhs, rhs, 1e-5f, 1e-5f) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log2 f64 — log2(2^k) ≈ k for integer k (kernel bound: k·ε)");
  {

    unsigned fails = 0;
    for ( int k = -100; k <= 100; ++k ) {
      const double x = __builtin_ldexp(1.0, k);
      const double y = micron::math::log2<double>(x);
      const double tol = 4.0 * (k < 0 ? -double(k) : double(k)) * 0x1.0p-52 + 1e-15;
      if ( !near_d(y, double(k), tol, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log2 f64 — boundary values");
  {
    require_true(micron::math::log2<double>(1.0) == 0.0);
    require_true(near_d(micron::math::log2<double>(2.0), 1.0, 1e-15, 0));
    require_true(near_d(micron::math::log2<double>(0.5), -1.0, 1e-15, 0));
    require_true(near_d(micron::math::log2<double>(1024.0), 10.0, 1e-13, 0));
    require_true(is_neg_inf_d(micron::math::log2<double>(0.0)));
    require_true(is_nan_d(micron::math::log2<double>(nan_d())));
    require_true(is_nan_d(micron::math::log2<double>(-1.0)));
    require_true(is_pos_inf_d(micron::math::log2<double>(inf_d())));
  }
  end_test_case();

  test_case("log2 f64 — log2(x) = log(x) / ln2 identity");
  {
    unsigned fails = 0;
    for ( double x = 1e-10; x <= 1e10; x *= 1.5 ) {
      const double l2 = micron::math::log2<double>(x);
      const double ref = micron::math::log<double>(x) * kLog2e;
      if ( !near_d(l2, ref, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log10 f64 — log10(10^k) = k for small integer k");
  {
    unsigned fails = 0;

    for ( int k = -15; k <= 15; ++k ) {
      double x = 1.0;
      for ( int i = 0; i < (k < 0 ? -k : k); ++i ) x *= 10.0;
      if ( k < 0 ) x = 1.0 / x;
      const double y = micron::math::log10<double>(x);
      if ( !near_d(y, double(k), 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log10 f64 — boundary values");
  {
    require_true(micron::math::log10<double>(1.0) == 0.0);
    require_true(near_d(micron::math::log10<double>(10.0), 1.0, 1e-14, 0));
    require_true(near_d(micron::math::log10<double>(100.0), 2.0, 1e-13, 0));
    require_true(is_neg_inf_d(micron::math::log10<double>(0.0)));
    require_true(is_nan_d(micron::math::log10<double>(nan_d())));
    require_true(is_nan_d(micron::math::log10<double>(-1.0)));
    require_true(is_pos_inf_d(micron::math::log10<double>(inf_d())));
  }
  end_test_case();

  test_case("log10 f64 — log10(x) = log(x) / ln10");
  {
    unsigned fails = 0;
    for ( double x = 1e-9; x <= 1e9; x *= 1.7 ) {
      const double l10 = micron::math::log10<double>(x);
      const double ref = micron::math::log<double>(x) / kLn10;
      if ( !near_d(l10, ref, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log2 f32 — log2(2^k) sweep");
  {
    unsigned fails = 0;
    for ( int k = -50; k <= 50; ++k ) {
      const float x = float(__builtin_ldexp(1.0, k));
      const float y = micron::math::log2<float>(x);
      if ( !near_f(y, float(k), 1e-5f, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log10 f32 — boundary values");
  {
    require_true(micron::math::log10<float>(1.0f) == 0.0f);
    require_true(near_f(micron::math::log10<float>(10.0f), 1.0f, 1e-6f, 0));
    require_true(is_neg_inf_f(micron::math::log10<float>(0.0f)));
    require_true(is_nan_f(micron::math::log10<float>(nan_f())));
  }
  end_test_case();

  test_case("log1p f64 — boundary values");
  {
    require_true(micron::math::log1p<double>(0.0) == 0.0);
    require_true(is_neg_inf_d(micron::math::log1p<double>(-1.0)));
    require_true(is_nan_d(micron::math::log1p<double>(-2.0)));
    require_true(is_nan_d(micron::math::log1p<double>(-1.5)));
    require_true(is_nan_d(micron::math::log1p<double>(nan_d())));
    require_true(is_pos_inf_d(micron::math::log1p<double>(inf_d())));
    require_true(near_d(micron::math::log1p<double>(kE - 1.0), 1.0, 1e-15, 1e-15));
  }
  end_test_case();

  test_case("log1p f64 — tiny x: log1p(x) = x exactly");
  {

    const double tinies[] = { 1e-20, 1e-100, 1e-200, 1e-300, 0x1.0p-1022 };
    for ( unsigned i = 0; i < sizeof(tinies) / sizeof(tinies[0]); ++i ) {
      require_true(micron::math::log1p<double>(tinies[i]) == tinies[i]);
      require_true(micron::math::log1p<double>(-tinies[i]) == -tinies[i]);
    }
  }
  end_test_case();

  test_case("log1p f64 — log1p(x) = log(1 + x) identity for moderate x");
  {
    unsigned fails = 0;
    for ( double x = -0.5; x <= 5.0; x += 0.013 ) {
      const double l1p = micron::math::log1p<double>(x);
      const double l = micron::math::log<double>(1.0 + x);
      if ( !near_d(l1p, l, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log1p f64 — small-x accuracy vs libm (catastrophic cancellation guard)");
  {

    unsigned fails = 0;
    for ( double x = 1e-15; x <= 1e-3; x *= 1.3 ) {
      const double mine = micron::math::log1p<double>(x);
      const double ref = __builtin_log1p(x);
      if ( !near_d(mine, ref, 1e-16, 1e-13) ) ++fails;
      const double mine_n = micron::math::log1p<double>(-x);
      const double ref_n = __builtin_log1p(-x);
      if ( !near_d(mine_n, ref_n, 1e-16, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log1p f32 — boundary, tiny-x, identity");
  {
    require_true(micron::math::log1p<float>(0.0f) == 0.0f);
    require_true(is_neg_inf_f(micron::math::log1p<float>(-1.0f)));
    require_true(is_nan_f(micron::math::log1p<float>(-2.0f)));
    require_true(is_nan_f(micron::math::log1p<float>(nan_f())));

    require_true(micron::math::log1p<float>(1e-10f) == 1e-10f);
    require_true(micron::math::log1p<float>(-1e-10f) == -1e-10f);
    unsigned fails = 0;
    for ( float x = -0.5f; x <= 5.0f; x += 0.07f ) {
      if ( !near_f(micron::math::log1p<float>(x), __builtin_log1pf(x), 1e-6f, 1e-5f) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log overloads — float / double / long double agree at moderate x");
  {
    unsigned fails = 0;
    for ( double x = 0.1; x <= 100.0; x += 1.1 ) {
      const double y_d = micron::math::log(x);
      const float y_f = micron::math::log(float(x));
      const long double y_ld = micron::math::log((long double)x);
      if ( y_d != micron::math::log<double>(x) ) ++fails;
      if ( y_f != micron::math::log<float>(float(x)) ) ++fails;

      if ( !near_d((double)y_ld, y_d, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log10 overloads — float / double / long double agree");
  {
    unsigned fails = 0;
    for ( double x = 0.1; x <= 1000.0; x *= 1.5 ) {
      if ( micron::math::log10(x) != micron::math::log10<double>(x) ) ++fails;
      if ( micron::math::log10(float(x)) != micron::math::log10<float>(float(x)) ) ++fails;
      if ( !near_d((double)micron::math::log10((long double)x), micron::math::log10<double>(x), 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("flog / flog10 / flog2 — alias of log / log10 / log2");
  {
    unsigned fails = 0;
    for ( double x = 0.1; x <= 100.0; x += 0.91 ) {
      if ( micron::math::flog(x) != micron::math::log<double>(x) ) ++fails;
      if ( micron::math::flog10(x) != micron::math::log10<double>(x) ) ++fails;
      if ( micron::math::flog2(x) != micron::math::log2<double>(x) ) ++fails;
      if ( micron::math::flog(float(x)) != micron::math::log<float>(float(x)) ) ++fails;
      if ( micron::math::flog10(float(x)) != micron::math::log10<float>(float(x)) ) ++fails;
      if ( micron::math::flog2(float(x)) != micron::math::log2<float>(float(x)) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log_base f64 — integer base, integer power");
  {
    require_true(near_d(micron::math::log_base<double>(8.0, 2.0), 3.0, 1e-13, 0));
    require_true(near_d(micron::math::log_base<double>(1000.0, 10.0), 3.0, 1e-13, 0));
    require_true(near_d(micron::math::log_base<double>(81.0, 3.0), 4.0, 1e-13, 0));
    require_true(near_d(micron::math::log_base<double>(1.0, 7.0), 0.0, 1e-15, 0));
    require_true(near_d(micron::math::log_base<double>(kE, kE), 1.0, 1e-14, 0));
  }
  end_test_case();

  test_case("log_base f64 — change-of-base identity log_b(x) = log(x)/log(b)");
  {
    unsigned fails = 0;
    for ( double b = 2.0; b <= 20.0; b += 1.7 ) {
      for ( double x = 0.1; x <= 1000.0; x *= 1.7 ) {
        const double lhs = micron::math::log_base<double>(x, b);
        const double rhs = micron::math::log<double>(x) / micron::math::log<double>(b);
        if ( !near_d(lhs, rhs, 1e-13, 1e-13) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log2p1 f64 — boundary and small-x");
  {
    require_true(near_d(micron::math::log2p1<double>(0.0), 0.0, 1e-15, 0));
    require_true(near_d(micron::math::log2p1<double>(1.0), 1.0, 1e-14, 0));
    require_true(near_d(micron::math::log2p1<double>(3.0), 2.0, 1e-14, 0));
    require_true(near_d(micron::math::log2p1<double>(7.0), 3.0, 1e-14, 0));

    const double ref = __builtin_log1p(1e-10) * 1.442695040888963407359924681001892137;
    require_true(near_d(micron::math::log2p1<double>(1e-10), ref, 1e-25, 1e-13));
  }
  end_test_case();

  test_case("log2p1 f64 — identity log2p1(x) = log1p(x) * log2e");
  {
    unsigned fails = 0;
    for ( double x = -0.5; x <= 5.0; x += 0.013 ) {
      const double lhs = micron::math::log2p1<double>(x);
      const double rhs = micron::math::log1p<double>(x) * kLog2e;
      if ( !near_d(lhs, rhs, 1e-14, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log1mexp f64 — known values and branch points");
  {

    require_true(near_d(micron::math::log1mexp<double>(-1.0), __builtin_log1p(-__builtin_exp(-1.0)), 1e-13, 1e-13));
    require_true(near_d(micron::math::log1mexp<double>(-10.0), __builtin_log1p(-__builtin_exp(-10.0)), 1e-13, 1e-13));

    require_true(near_d(micron::math::log1mexp<double>(-kLn2), -kLn2, 1e-13, 1e-13));
  }
  end_test_case();

  test_case("log1mexp f64 — branch agreement around -ln2 cusp");
  {

    unsigned fails = 0;
    for ( double dx = 1e-5; dx <= 0.1; dx *= 1.3 ) {
      const double lo = micron::math::log1mexp<double>(-kLn2 - dx);
      const double hi = micron::math::log1mexp<double>(-kLn2 + dx);

      if ( adouble(lo - hi) > 0.5 ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log_diff_exp f64 — log(exp(a)-exp(b))");
  {

    auto ref = [](double a, double b) { return a + __builtin_log1p(-__builtin_exp(b - a)); };
    require_true(near_d(micron::math::log_diff_exp<double>(2.0, 1.0), ref(2.0, 1.0), 1e-12, 1e-13));
    require_true(near_d(micron::math::log_diff_exp<double>(10.0, 9.0), ref(10.0, 9.0), 1e-12, 1e-13));
    require_true(near_d(micron::math::log_diff_exp<double>(1000.0, 999.0), ref(1000.0, 999.0), 1e-9, 1e-12));
  }
  end_test_case();

  test_case("log_sum_exp f64 — pairwise: log(2 e^a) = a + ln2");
  {
    require_true(near_d(micron::math::log_sum_exp<double>(0.0, 0.0), kLn2, 1e-15, 0));
    require_true(near_d(micron::math::log_sum_exp<double>(1.0, 1.0), 1.0 + kLn2, 1e-14, 0));
    require_true(near_d(micron::math::log_sum_exp<double>(-1.0, -1.0), -1.0 + kLn2, 1e-14, 0));
  }
  end_test_case();

  test_case("log_sum_exp f64 — overflow guard at large argument");
  {

    const double r1 = micron::math::log_sum_exp<double>(1000.0, 1000.0);
    require_true(near_d(r1, 1000.0 + kLn2, 1e-9, 0));
    const double r2 = micron::math::log_sum_exp<double>(700.0, -700.0);
    require_true(near_d(r2, 700.0, 1e-9, 0));
  }
  end_test_case();

  test_case("log_sum_exp f64 — commutativity and -inf identity");
  {
    unsigned fails = 0;
    for ( double a = -5.0; a <= 5.0; a += 0.31 ) {
      for ( double b = -5.0; b <= 5.0; b += 0.41 ) {
        const double x = micron::math::log_sum_exp<double>(a, b);
        const double y = micron::math::log_sum_exp<double>(b, a);
        if ( !near_d(x, y, 1e-14, 1e-14) ) ++fails;
      }
    }

    if ( micron::math::log_sum_exp<double>(ninf_d(), 5.0) != 5.0 ) ++fails;
    if ( micron::math::log_sum_exp<double>(5.0, ninf_d()) != 5.0 ) ++fails;
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log_sum_exp_n f64 — sum of 5 vs libm reference");
  {
    double v[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const double r = micron::math::log_sum_exp_n<double>(v, 5);

    double mx = v[0];
    for ( int i = 1; i < 5; ++i )
      if ( v[i] > mx ) mx = v[i];
    double sum = 0.0;
    for ( int i = 0; i < 5; ++i ) sum += __builtin_exp(v[i] - mx);
    const double ref = mx + __builtin_log(sum);
    require_true(near_d(r, ref, 1e-12, 1e-13));
  }
  end_test_case();

  test_case("log_sum_exp_n f64 — single element returns element");
  {
    double v1[1] = { 7.5 };
    double v2[1] = { -1234.5 };
    require_true(micron::math::log_sum_exp_n<double>(v1, 1) == 7.5);
    require_true(micron::math::log_sum_exp_n<double>(v2, 1) == -1234.5);
  }
  end_test_case();

  test_case("log_sum_exp_n f64 — empty span = -inf");
  {
    double v[1] = { 0.0 };
    require_true(is_neg_inf_d(micron::math::log_sum_exp_n<double>(v, 0)));
  }
  end_test_case();

  test_case("log_sum_exp_n f64 — all -inf collapses to -inf");
  {
    double v[4] = { ninf_d(), ninf_d(), ninf_d(), ninf_d() };
    require_true(is_neg_inf_d(micron::math::log_sum_exp_n<double>(v, 4)));
  }
  end_test_case();

  test_case("log_sum_exp_n f64 — pairwise consistency");
  {
    unsigned fails = 0;
    double v[2] = { 0.0, 0.0 };
    for ( double a = -10.0; a <= 10.0; a += 0.71 ) {
      for ( double b = -10.0; b <= 10.0; b += 0.83 ) {
        v[0] = a;
        v[1] = b;
        const double n = micron::math::log_sum_exp_n<double>(v, 2);
        const double p = micron::math::log_sum_exp<double>(a, b);
        if ( !near_d(n, p, 1e-13, 1e-13) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log_sum_exp_n f64 — overflow guard at large magnitudes");
  {

    double v[10];
    for ( int i = 0; i < 10; ++i ) v[i] = 1000.0;
    const double r = micron::math::log_sum_exp_n<double>(v, 10);
    require_true(near_d(r, 1000.0 + __builtin_log(10.0), 1e-9, 0));
  }
  end_test_case();

  test_case("log_add_exp_vec f64 — alias of log_sum_exp_n");
  {
    double v[3] = { 0.5, 1.5, 2.5 };
    require_true(micron::math::log_add_exp_vec<double>(v, 3) == micron::math::log_sum_exp_n<double>(v, 3));
  }
  end_test_case();

  test_case("logistic f64 — boundary and identity");
  {
    require_true(near_d(micron::math::logistic<double>(0.0), 0.5, 1e-15, 0));
    require_true(near_d(micron::math::logistic<double>(100.0), 1.0, 1e-12, 0));
    require_true(near_d(micron::math::logistic<double>(-100.0), 0.0, 1e-12, 0));
    require_true(near_d(micron::math::logistic<double>(inf_d()), 1.0, 0, 0));
    require_true(near_d(micron::math::logistic<double>(ninf_d()), 0.0, 0, 0));

    unsigned fails = 0;
    for ( double x = -10.0; x <= 10.0; x += 0.13 ) {
      const double a = micron::math::logistic<double>(x);
      const double b = micron::math::logistic<double>(-x);
      if ( !near_d(a + b, 1.0, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log_logistic f64 — log(σ(x)) identity");
  {
    require_true(near_d(micron::math::log_logistic<double>(0.0), -kLn2, 1e-15, 0));

    require_true(micron::math::log_logistic<double>(-100.0) < -99.0);
    require_true(micron::math::log_logistic<double>(-1000.0) < -999.0);
    unsigned fails = 0;
    for ( double x = -20.0; x <= 20.0; x += 0.17 ) {
      const double mine = micron::math::log_logistic<double>(x);
      const double ref = __builtin_log(micron::math::logistic<double>(x));

      if ( is_neg_inf_d(ref) ) continue;
      if ( !near_d(mine, ref, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("softplus f64 — boundary and overflow guard");
  {
    require_true(near_d(micron::math::softplus<double>(0.0), kLn2, 1e-15, 0));

    require_true(micron::math::softplus<double>(50.0) == 50.0);
    require_true(micron::math::softplus<double>(1000.0) == 1000.0);

    require_true(micron::math::softplus<double>(-50.0) > 0.0);
    require_true(micron::math::softplus<double>(-50.0) < 1e-15);

    unsigned fails = 0;
    for ( double x = -20.0; x <= 20.0; x += 0.17 ) {
      const double sp = micron::math::softplus<double>(x);
      const double sn = micron::math::softplus<double>(-x);
      if ( !near_d(sp - sn, x, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("softplus f32 — overflow guard at smaller threshold");
  {

    require_true(near_f(micron::math::softplus<float>(0.0f), float(kLn2), 1e-7f, 0));
    require_true(micron::math::softplus<float>(50.0f) == 50.0f);
    require_true(micron::math::softplus<float>(20.0f) == 20.0f);
  }
  end_test_case();

  test_case("xlogx / xlogy f64 — exact 0 case and identity");
  {
    require_true(micron::math::xlogx<double>(0.0) == 0.0);
    require_true(micron::math::xlogy<double>(0.0, 1e9) == 0.0);
    require_true(micron::math::xlogy<double>(0.0, 0.0) == 0.0);

    require_true(near_d(micron::math::xlogx<double>(kE), kE, 1e-13, 1e-13));

    require_true(near_d(micron::math::xlogx<double>(1.0), 0.0, 1e-15, 0));
    require_true(near_d(micron::math::xlogy<double>(1.0, kE), 1.0, 1e-14, 0));

    unsigned fails = 0;
    for ( double x = 0.0; x <= 1.0; x += 0.013 ) {
      const double y = micron::math::xlogx<double>(x);
      if ( is_nan_d(y) ) ++fails;
      if ( y > 1e-15 ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("softmax f64 — in-place sums to 1, preserves order");
  {
    double v[6] = { -2.0, -1.0, 0.0, 1.0, 2.0, 3.0 };
    micron::math::softmax<double>(v, 6);
    double s = 0.0;
    for ( int i = 0; i < 6; ++i ) s += v[i];
    require_true(near_d(s, 1.0, 1e-12, 0));

    for ( int i = 1; i < 6; ++i ) require_true(v[i] > v[i - 1]);

    for ( int i = 0; i < 6; ++i ) require_true(v[i] > 0.0);
  }
  end_test_case();

  test_case("softmax f64 — out-of-place equals in-place");
  {
    double in[5] = { -1.0, 0.0, 1.0, 2.0, 3.0 };
    double cp[5] = { -1.0, 0.0, 1.0, 2.0, 3.0 };
    double out[5];
    micron::math::softmax<double>(in, out, 5);
    micron::math::softmax<double>(cp, 5);
    for ( int i = 0; i < 5; ++i ) require_true(out[i] == cp[i]);
  }
  end_test_case();

  test_case("softmax f64 — overflow guard with large input");
  {
    double v[3] = { 1000.0, 1000.0, 1000.0 };
    micron::math::softmax<double>(v, 3);

    for ( int i = 0; i < 3; ++i ) require_true(near_d(v[i], 1.0 / 3.0, 1e-14, 1e-14));
  }
  end_test_case();

  test_case("softmax f64 — empty span no-op");
  {
    double v[1] = { 42.0 };
    micron::math::softmax<double>(v, 0);
    require_true(v[0] == 42.0);
  }
  end_test_case();

  test_case("log_softmax f64 — exp(log_softmax) sums to 1");
  {
    double v[4] = { -1.0, 0.0, 1.0, 2.0 };
    micron::math::log_softmax<double>(v, 4);
    double s = 0.0;
    for ( int i = 0; i < 4; ++i ) s += micron::math::exp<double>(v[i]);
    require_true(near_d(s, 1.0, 1e-13, 0));
  }
  end_test_case();

  test_case("log_softmax f64 — out-of-place equals in-place");
  {
    double in[4] = { -1.0, 0.0, 1.0, 2.0 };
    double cp[4] = { -1.0, 0.0, 1.0, 2.0 };
    double out[4];
    micron::math::log_softmax<double>(in, out, 4);
    micron::math::log_softmax<double>(cp, 4);
    for ( int i = 0; i < 4; ++i ) require_true(out[i] == cp[i]);
  }
  end_test_case();

  test_case("log_softmax f64 — overflow guard with large input");
  {
    double v[3] = { 1000.0, 1000.0, 1000.0 };
    micron::math::log_softmax<double>(v, 3);

    for ( int i = 0; i < 3; ++i ) require_true(near_d(v[i], -__builtin_log(3.0), 1e-12, 0));
  }
  end_test_case();

  test_case("micron::log/log2/log10/log1p equal micron::math::* bitwise");
  {
    unsigned fails = 0;
    for ( double x = 0.001; x <= 100.0; x *= 1.13 ) {
      if ( micron::log(x) != micron::math::log<double>(x) ) ++fails;
      if ( micron::log2(x) != micron::math::log2<double>(x) ) ++fails;
      if ( micron::log10(x) != micron::math::log10<double>(x) ) ++fails;
      if ( micron::log1p(x) != micron::math::log1p<double>(x) ) ++fails;
    }
    for ( float x = 0.001f; x <= 100.0f; x *= 1.21f ) {
      if ( micron::log(x) != micron::math::log<float>(x) ) ++fails;
      if ( micron::log2(x) != micron::math::log2<float>(x) ) ++fails;
      if ( micron::log10(x) != micron::math::log10<float>(x) ) ++fails;
      if ( micron::log1p(x) != micron::math::log1p<float>(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("log family is usable in constexpr context");
  {
    constexpr double cl1 = micron::math::log<double>(1.0);
    constexpr double cl2 = micron::math::log2<double>(2.0);
    constexpr double cl10 = micron::math::log10<double>(10.0);
    constexpr double cl1p = micron::math::log1p<double>(0.0);
    static_assert(cl1 == 0.0);
    static_assert(cl1p == 0.0);
    require_true(near_d(cl2, 1.0, 1e-14, 0));
    require_true(near_d(cl10, 1.0, 1e-14, 0));
  }
  end_test_case();

  test_case("(FAILS) log2 f64 — log2(2^k) recovers integer k exactly");
  {

    bool any = false;
    int worst_k = 0;
    double worst_diff = 0.0;
    for ( int k = -100; k <= 100; ++k ) {
      const double x = __builtin_ldexp(1.0, k);
      const double y = micron::math::log2<double>(x);
      const double d = y - double(k);
      const double ad = d < 0 ? -d : d;
      if ( ad > worst_diff ) {
        worst_diff = ad;
        worst_k = k;
      }
      if ( ad > 0.0 ) any = true;
    }
    sb::print("    log2(2^k) worst |diff|=", worst_diff, " @ k=", worst_k);
    soft_check(!any, "log2(2^k) is exactly k for all integer k in [-100, 100]");
  }
  end_test_case();

  test_case("(FAILS) log f64 — log(1+eps) accuracy (catastrophic cancellation)");
  {

    bool any = false;
    for ( double eps = 1e-15; eps <= 1e-8; eps *= 1.5 ) {
      const double got = micron::math::log<double>(1.0 + eps);
      const double ref = __builtin_log1p(eps);
      const double r = adouble(got - ref) / (adouble(ref) + 1e-300);
      if ( r > 1e-7 ) any = true;
    }
    soft_check(!any, "log(1+eps) within 1e-7 relative of log1p(eps) for eps ≥ 1e-15");
  }
  end_test_case();

  test_case("(FAILS) log_sum_exp_n f64 — extreme spread");
  {

    double v[10] = { 1000.0, -500.0, 750.0, -300.0, 999.999, -1.0, 0.0, 500.0, -999.0, 1000.0 };
    const double r = micron::math::log_sum_exp_n<double>(v, 10);

    double mx = v[0];
    for ( int i = 1; i < 10; ++i )
      if ( v[i] > mx ) mx = v[i];
    double sum = 0.0;
    for ( int i = 0; i < 10; ++i ) sum += __builtin_exp(v[i] - mx);
    const double ref = mx + __builtin_log(sum);
    soft_check(adouble(r - ref) <= 1e-9, "log_sum_exp_n stays within 1e-9 of libm on extreme spread");
  }
  end_test_case();

  test_case("(FAILS) log10 f64 — log10(10^k) exact integer recovery for large k");
  {

    bool any = false;
    for ( int k = 100; k <= 280; k += 20 ) {
      double x = 1.0;
      for ( int i = 0; i < k; ++i ) x *= 10.0;
      const double got = micron::math::log10<double>(x);
      if ( adouble(got - double(k)) > 1e-12 ) any = true;
    }
    soft_check(!any, "log10(10^k) recovers k within 1e-12 for k up to 280");
  }
  end_test_case();

  sb::print("=== LOG EXHAUSTIVE COMPLETE ===");
  sb::print("    soft-checks total:    ", __soft().checks);
  sb::print("    soft-checks failures: ", __soft().failures);
  if ( __soft().failures == 0 )
    sb::print("=== ALL LOG EXHAUSTIVE TESTS PASSED ===");
  else
    sb::print("=== HARD TESTS PASSED; (FAILS)-marked tests reported above ===");
  return 1;
}
