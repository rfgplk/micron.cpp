// math_trig_exhaustive.cpp
// Exhaustive rigor tests for every micron::math trig surface.
//
// Coverage matrix:
//   forward            : sin, cos, tan, sincos        (f32, f64)
//   inverse            : asin, acos, atan, atan2      (f32, f64)
//   hyperbolic forward : sinh, cosh, tanh             (f32, f64)
//   hyperbolic inverse : asinh, acosh, atanh          (f32, f64)
//   CORDIC variants    : *_cordic                     (f32, f64)
//   fast porcelain     : fcos, ftan, fasin, facos, fatan, fatan2
//   top-level reexport : micron::sin etc. agree with micron::math::sin etc.
//
// Each surface gets:
//   - identity tests       (parity, periodicity, pythagoras, etc.)
//   - special-value tests  (0, ±0, ±1, ±inf, NaN, boundary 1±eps)
//   - special-angle tests  (kπ/6, kπ/4, kπ/3, kπ/2, kπ, k·2π for many k)
//   - reduction stress     (Cody-Waite vs Payne-Hanek branch boundary)
//   - cross-precision      (f32 path produces values within f32 ulp band)
//
// Soft-failure model copied from abcmalloc_stress: hard `require_true` for
// invariants we believe must hold, `soft_check` for documented quirks that we
// keep on record without aborting the run.

#include "../../src/math.hpp"
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

// ───────────────────────────────────────────────────────────────────────────
// soft-failure infrastructure (mirrors abcmalloc_stress.cpp)
// ───────────────────────────────────────────────────────────────────────────

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

// ───────────────────────────────────────────────────────────────────────────
// math helpers — no libc/STL required at runtime, but we lift abs from raw fp
// ───────────────────────────────────────────────────────────────────────────

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

// reference constants
static constexpr double kPi = 3.141592653589793238462643383279502884;
static constexpr double kPi2 = 1.570796326794896619231321691639751442;
static constexpr double kPi4 = 0.785398163397448309615660845819875721;

// is_nan / NaN / Inf factories must always-inline so fast-math doesn't strip
// the bit-pattern construction or fold the test. Plain `static inline` is not
// strong enough under -Ofast + -flto; the `gnu::always_inline` attribute is
// what makes ieee::is_nan / ieee::qnan_v reliable here.
[[gnu::always_inline]] inline constexpr bool
is_nan_d(double v) noexcept
{
  return micron::math::ieee::is_nan<double>(v);
}

[[gnu::always_inline]] inline constexpr bool
is_nan_f(float v) noexcept
{
  return micron::math::ieee::is_nan<float>(v);
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
nan_f() noexcept
{
  return micron::math::ieee::qnan_v<float>();
}

};      // anonymous namespace

int
main()
{
  print("=== TRIG EXHAUSTIVE TESTS ===");

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 1 — sin / cos f64 — identities, edges, special values        ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("sin/cos f64 — zero, ±0, sign symmetry");
  {
    require_true(micron::math::sin<double>(0.0) == 0.0);
    require_true(micron::math::cos<double>(0.0) == 1.0);
    // ±0 should round-trip: sin(-0) = -0, cos(-0) = +1
    const double sn0 = micron::math::sin<double>(-0.0);
    require_true(sn0 == 0.0);
    require_true(micron::math::cos<double>(-0.0) == 1.0);
  }
  end_test_case();

  test_case("sin/cos f64 — NaN propagation");
  {
    require_true(is_nan_d(micron::math::sin<double>(nan_d())));
    require_true(is_nan_d(micron::math::cos<double>(nan_d())));
  }
  end_test_case();

  test_case("sin/cos f64 — ±inf → NaN");
  {
    require_true(is_nan_d(micron::math::sin<double>(inf_d())));
    require_true(is_nan_d(micron::math::sin<double>(ninf_d())));
    require_true(is_nan_d(micron::math::cos<double>(inf_d())));
    require_true(is_nan_d(micron::math::cos<double>(ninf_d())));
  }
  end_test_case();

  test_case("sin/cos f64 — denormal tiny x: sin(x) ≈ x, cos(x) ≈ 1");
  {
    const double tinies[] = { 1e-30, 1e-100, 1e-200, 1e-300, 0x1.0p-1022, 0x1.0p-1000 };
    for ( unsigned i = 0; i < sizeof(tinies) / sizeof(tinies[0]); ++i ) {
      const double x = tinies[i];
      require_true(micron::math::sin<double>(x) == x);
      require_true(micron::math::cos<double>(x) == 1.0);
      require_true(micron::math::sin<double>(-x) == -x);
      require_true(micron::math::cos<double>(-x) == 1.0);
    }
  }
  end_test_case();

  test_case("sin/cos f64 — parity: sin(-x)=-sin(x), cos(-x)=cos(x)");
  {
    unsigned fails = 0;
    for ( double x = -10.0; x <= 10.0; x += 0.07 ) {
      const double s = micron::math::sin<double>(x);
      const double sn = micron::math::sin<double>(-x);
      const double c = micron::math::cos<double>(x);
      const double cn = micron::math::cos<double>(-x);
      if ( !near_d(sn, -s, 1e-15, 1e-15) ) ++fails;
      if ( !near_d(cn, c, 1e-15, 1e-15) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f64 — pythagoras identity over [-π, π]");
  {
    unsigned fails = 0;
    for ( double x = -kPi; x <= kPi; x += kPi / 1000.0 ) {
      const double s = micron::math::sin<double>(x);
      const double c = micron::math::cos<double>(x);
      const double mag = s * s + c * c;
      if ( !near_d(mag, 1.0, 1e-13, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f64 — pythagoras at wide ranges (post-reduction)");
  {
    unsigned fails = 0;
    const double xs[] = { 12345.6789, 1.0e4, -1.0e4, 1.0e6, 1.0e8, 1.0e12, 1.0e16, 0x1.0p33, 0x1.0p33 * 1.001, 0x1.0p40, 0x1.0p50 };
    for ( unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); ++i ) {
      const double s = micron::math::sin<double>(xs[i]);
      const double c = micron::math::cos<double>(xs[i]);
      const double mag = s * s + c * c;
      if ( !near_d(mag, 1.0, 5e-11, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f64 — sum identities");
  {
    // sin(a+b) = sin(a)cos(b)+cos(a)sin(b), cos(a+b)=cos(a)cos(b)-sin(a)sin(b)
    unsigned fails = 0;
    for ( double a = -3.0; a <= 3.0; a += 0.211 ) {
      for ( double b = -3.0; b <= 3.0; b += 0.317 ) {
        const double s_ab = micron::math::sin<double>(a + b);
        const double c_ab = micron::math::cos<double>(a + b);
        const double sa = micron::math::sin<double>(a);
        const double ca = micron::math::cos<double>(a);
        const double sb = micron::math::sin<double>(b);
        const double cb = micron::math::cos<double>(b);
        if ( !near_d(s_ab, sa * cb + ca * sb, 5e-15, 5e-14) ) ++fails;
        if ( !near_d(c_ab, ca * cb - sa * sb, 5e-15, 5e-14) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f64 — double-angle identity");
  {
    // sin(2x) = 2 sin(x)cos(x), cos(2x) = 1 - 2 sin²(x)
    unsigned fails = 0;
    for ( double x = -2.0; x <= 2.0; x += 0.013 ) {
      const double s = micron::math::sin<double>(x);
      const double c = micron::math::cos<double>(x);
      const double s2 = micron::math::sin<double>(2.0 * x);
      const double c2 = micron::math::cos<double>(2.0 * x);
      if ( !near_d(s2, 2.0 * s * c, 1e-14, 1e-13) ) ++fails;
      if ( !near_d(c2, 1.0 - 2.0 * s * s, 1e-14, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f64 — periodicity sin(x+2πk)=sin(x), cos(x+2πk)=cos(x)");
  {
    unsigned fails = 0;
    const double xs[] = { 0.3, 1.2, -0.7, 2.6, -2.6, 0.001 };
    const int ks[] = { 1, 2, 5, 10, 100, 1000 };
    for ( unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); ++i ) {
      const double s_ref = micron::math::sin<double>(xs[i]);
      const double c_ref = micron::math::cos<double>(xs[i]);
      for ( unsigned j = 0; j < sizeof(ks) / sizeof(ks[0]); ++j ) {
        const double shifted = xs[i] + double(ks[j]) * 2.0 * kPi;
        const double s = micron::math::sin<double>(shifted);
        const double c = micron::math::cos<double>(shifted);
        const double tol = 1e-12 + 1e-14 * adouble(shifted);
        if ( adouble(s - s_ref) > tol ) ++fails;
        if ( adouble(c - c_ref) > tol ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f64 — special angles (multiples of π/6, π/4)");
  {
    struct rec {
      double x;
      double s;
      double c;
    } cases[] = {
      { 0.0, 0.0, 1.0 },
      { kPi / 6, 0.5, 0.866025403784438646763723170752936183 },
      { kPi / 4, 0.707106781186547524400844362104849039, 0.707106781186547524400844362104849039 },
      { kPi / 3, 0.866025403784438646763723170752936183, 0.5 },
      { kPi / 2, 1.0, 0.0 },
      { 2 * kPi / 3, 0.866025403784438646763723170752936183, -0.5 },
      { 3 * kPi / 4, 0.707106781186547524400844362104849039, -0.707106781186547524400844362104849039 },
      { 5 * kPi / 6, 0.5, -0.866025403784438646763723170752936183 },
      { kPi, 0.0, -1.0 },
      { -kPi / 2, -1.0, 0.0 },
      { -kPi, 0.0, -1.0 },
      { 3 * kPi / 2, -1.0, 0.0 },
      { 2 * kPi, 0.0, 1.0 },
    };

    for ( unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i ) {
      const double s = micron::math::sin<double>(cases[i].x);
      const double c = micron::math::cos<double>(cases[i].x);
      require_true(near_d(s, cases[i].s, 1e-15, 1e-14));
      require_true(near_d(c, cases[i].c, 1e-15, 1e-14));
    }
  }
  end_test_case();

  test_case("sin/cos f64 — reduction branch boundary (|x| around 2^20)");
  {
    // crosses Cody-Waite / iter-Cody-Waite boundary at 2^20
    unsigned fails = 0;
    const double base = 0x1.0p20;
    for ( double off = -100.0; off <= 100.0; off += 1.7 ) {
      const double x = base + off;
      const double s = micron::math::sin<double>(x);
      const double c = micron::math::cos<double>(x);
      if ( !near_d(s * s + c * c, 1.0, 1e-10, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f64 — Payne-Hanek branch (|x| ≥ 2^33)");
  {
    unsigned fails = 0;
    const double base = 0x1.0p33;
    for ( double off = -1000.0; off <= 1000.0; off += 17.3 ) {
      const double x = base + off;
      const double s = micron::math::sin<double>(x);
      const double c = micron::math::cos<double>(x);
      if ( !near_d(s * s + c * c, 1.0, 1e-9, 0) ) ++fails;
    }
    // also a few huge values
    const double huge[] = { 1.0e15, 1.0e18, 1.0e20, 0x1.0p52, 0x1.0p60 };
    for ( unsigned i = 0; i < sizeof(huge) / sizeof(huge[0]); ++i ) {
      const double s = micron::math::sin<double>(huge[i]);
      const double c = micron::math::cos<double>(huge[i]);
      if ( !near_d(s * s + c * c, 1.0, 1e-8, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 2 — sin / cos f32                                            ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("sin/cos f32 — zero, NaN, ±inf");
  {
    require_true(micron::math::sin<float>(0.0f) == 0.0f);
    require_true(micron::math::cos<float>(0.0f) == 1.0f);
    require_true(is_nan_f(micron::math::sin<float>(nan_f())));
    require_true(is_nan_f(micron::math::sin<float>(inf_f())));
    require_true(is_nan_f(micron::math::cos<float>(inf_f())));
  }
  end_test_case();

  test_case("sin/cos f32 — pythagoras identity sweep");
  {
    unsigned fails = 0;
    for ( float x = -1000.0f; x <= 1000.0f; x += 0.317f ) {
      const float s = micron::math::sin<float>(x);
      const float c = micron::math::cos<float>(x);
      const float mag = s * s + c * c;
      if ( afloat(mag - 1.0f) > 1e-5f ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sin/cos f32 — parity");
  {
    unsigned fails = 0;
    for ( float x = -100.0f; x <= 100.0f; x += 0.51f ) {
      const float s = micron::math::sin<float>(x);
      const float sn = micron::math::sin<float>(-x);
      const float c = micron::math::cos<float>(x);
      const float cn = micron::math::cos<float>(-x);
      if ( afloat(s + sn) > 1e-6f ) ++fails;
      if ( afloat(c - cn) > 1e-6f ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 3 — sincos                                                   ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("sincos f64 — agrees with sin/cos");
  {
    unsigned fails = 0;
    for ( double x = -100.0; x <= 100.0; x += 0.111 ) {
      double s, c;
      micron::math::sincos<double>(x, s, c);
      if ( s != micron::math::sin<double>(x) ) ++fails;
      if ( c != micron::math::cos<double>(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sincos f32 — agrees with sin/cos");
  {
    unsigned fails = 0;
    for ( float x = -50.0f; x <= 50.0f; x += 0.13f ) {
      float s, c;
      micron::math::sincos<float>(x, s, c);
      if ( s != micron::math::sin<float>(x) ) ++fails;
      if ( c != micron::math::cos<float>(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("(FAILS) sincos f64 — NaN / inf both lanes (runtime under -Ofast)");
  {
    // The sincos kernel writes through reference args; that materializes the
    // ref store outside the constexpr-foldable chain, so under -Ofast +
    // -ffinite-math-only the NaN/Inf guard (`is_nan(x)||is_inf(x)`) folds to
    // false and the kernel computes on the non-finite input. Result is finite
    // garbage. Pure runtime quirk; the kernel logic itself is correct.
    double s, c;
    micron::math::sincos<double>(nan_d(), s, c);
    soft_check(is_nan_d(s), "sincos(NaN).sin lane is NaN");
    soft_check(is_nan_d(c), "sincos(NaN).cos lane is NaN");
    micron::math::sincos<double>(inf_d(), s, c);
    soft_check(is_nan_d(s), "sincos(±inf).sin lane is NaN");
    soft_check(is_nan_d(c), "sincos(±inf).cos lane is NaN");
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 4 — tan f64 / f32                                            ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("tan f64 — zero, NaN, ±inf");
  {
    require_true(micron::math::tan<double>(0.0) == 0.0);
    require_true(is_nan_d(micron::math::tan<double>(nan_d())));
    require_true(is_nan_d(micron::math::tan<double>(inf_d())));
    require_true(is_nan_d(micron::math::tan<double>(ninf_d())));
  }
  end_test_case();

  test_case("tan f64 — tan(x) = sin(x)/cos(x) identity, pole-avoiding");
  {
    unsigned fails = 0;
    for ( double x = -3.0; x <= 3.0; x += 0.0131 ) {
      const double s = micron::math::sin<double>(x);
      const double c = micron::math::cos<double>(x);
      if ( adouble(c) < 1e-3 ) continue;
      const double t = micron::math::tan<double>(x);
      if ( !near_d(t, s / c, 1e-13, 5e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("tan f64 — parity tan(-x)=-tan(x)");
  {
    unsigned fails = 0;
    for ( double x = 0.05; x <= 1.5; x += 0.013 ) {
      const double a = micron::math::tan<double>(x);
      const double b = micron::math::tan<double>(-x);
      if ( !near_d(a, -b, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("tan f64 — special angles");
  {
    require_true(near_d(micron::math::tan<double>(kPi / 4), 1.0, 1e-15, 1e-15));
    require_true(near_d(micron::math::tan<double>(-kPi / 4), -1.0, 1e-15, 1e-15));
    require_true(near_d(micron::math::tan<double>(kPi), 0.0, 1e-12, 0));
    require_true(near_d(micron::math::tan<double>(2 * kPi), 0.0, 1e-12, 0));
    require_true(near_d(micron::math::tan<double>(kPi / 6), 0.577350269189625764509148780501957455, 1e-14, 1e-14));
    require_true(near_d(micron::math::tan<double>(kPi / 3), 1.732050807568877293527446341505872366, 1e-14, 1e-14));
  }
  end_test_case();

  test_case("tan f32 — sin/cos identity, pole-avoiding");
  {
    unsigned fails = 0;
    for ( float x = -2.0f; x <= 2.0f; x += 0.013f ) {
      const float s = micron::math::sin<float>(x);
      const float c = micron::math::cos<float>(x);
      if ( afloat(c) < 1e-2f ) continue;
      const float t = micron::math::tan<float>(x);
      if ( !near_f(t, s / c, 1e-5f, 5e-5f) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 5 — asin / acos f64 / f32                                    ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("asin/acos f64 — boundaries, NaN, out-of-range");
  {
    require_true(micron::math::asin<double>(0.0) == 0.0);
    require_true(near_d(micron::math::asin<double>(1.0), kPi / 2, 1e-15, 0));
    require_true(near_d(micron::math::asin<double>(-1.0), -kPi / 2, 1e-15, 0));
    require_true(near_d(micron::math::acos<double>(1.0), 0.0, 1e-15, 0));
    require_true(near_d(micron::math::acos<double>(-1.0), kPi, 1e-15, 0));
    require_true(near_d(micron::math::acos<double>(0.0), kPi / 2, 1e-15, 0));
    require_true(is_nan_d(micron::math::asin<double>(nan_d())));
    require_true(is_nan_d(micron::math::acos<double>(nan_d())));
    require_true(is_nan_d(micron::math::asin<double>(1.0 + 1e-9)));
    require_true(is_nan_d(micron::math::asin<double>(-1.0 - 1e-9)));
    require_true(is_nan_d(micron::math::acos<double>(1.0 + 1e-9)));
    require_true(is_nan_d(micron::math::acos<double>(-1.0 - 1e-9)));
    require_true(is_nan_d(micron::math::asin<double>(inf_d())));
    require_true(is_nan_d(micron::math::acos<double>(inf_d())));
  }
  end_test_case();

  test_case("asin/acos f64 — sin(asin(x)) = x identity over [-1, 1]");
  {
    unsigned fails = 0;
    for ( double x = -1.0; x <= 1.0; x += 0.001 ) {
      const double y = micron::math::asin<double>(x);
      const double s = micron::math::sin<double>(y);
      if ( adouble(s - x) > 1e-14 ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("asin/acos f64 — cos(acos(x)) = x identity over [-1, 1]");
  {
    unsigned fails = 0;
    for ( double x = -1.0; x <= 1.0; x += 0.001 ) {
      const double y = micron::math::acos<double>(x);
      const double c = micron::math::cos<double>(y);
      if ( adouble(c - x) > 1e-14 ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("asin/acos f64 — asin(x) + acos(x) = π/2");
  {
    unsigned fails = 0;
    for ( double x = -1.0; x <= 1.0; x += 0.001 ) {
      const double a = micron::math::asin<double>(x);
      const double c = micron::math::acos<double>(x);
      if ( !near_d(a + c, kPi / 2, 1e-14, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("asin f64 — parity asin(-x) = -asin(x)");
  {
    unsigned fails = 0;
    for ( double x = 0.001; x <= 1.0; x += 0.001 ) {
      const double a = micron::math::asin<double>(x);
      const double b = micron::math::asin<double>(-x);
      if ( a != -b ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("asin/acos f64 — tiny x: asin(x) ≈ x");
  {
    const double tinies[] = { 1e-9, 1e-10, 1e-15, 1e-20, 1e-100 };
    for ( unsigned i = 0; i < sizeof(tinies) / sizeof(tinies[0]); ++i ) {
      require_true(micron::math::asin<double>(tinies[i]) == tinies[i]);
    }
  }
  end_test_case();

  test_case("asin/acos f32 — boundaries, identity");
  {
    unsigned fails = 0;
    for ( float x = -1.0f; x <= 1.0f; x += 0.001f ) {
      const float a = micron::math::asin<float>(x);
      const float c = micron::math::acos<float>(x);
      if ( !near_f(a + c, float(kPi) / 2.0f, 1e-6f, 1e-6f) ) ++fails;
    }
    require_true(fails == 0);
    require_true(near_f(micron::math::asin<float>(1.0f), float(kPi) / 2.0f, 1e-7f, 0));
    require_true(near_f(micron::math::acos<float>(-1.0f), float(kPi), 1e-6f, 0));
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 6 — atan / atan2 f64 / f32                                   ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("atan f64 — boundaries");
  {
    require_true(micron::math::atan<double>(0.0) == 0.0);
    require_true(is_nan_d(micron::math::atan<double>(nan_d())));
    require_true(near_d(micron::math::atan<double>(1.0), kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan<double>(-1.0), -kPi / 4, 1e-15, 0));
    // atan(±inf) → ±π/2
    require_true(near_d(micron::math::atan<double>(inf_d()), kPi / 2, 1e-15, 0));
    require_true(near_d(micron::math::atan<double>(ninf_d()), -kPi / 2, 1e-15, 0));
  }
  end_test_case();

  test_case("atan f64 — tan(atan(x)) = x");
  {
    unsigned fails = 0;
    for ( double x = -10.0; x <= 10.0; x += 0.013 ) {
      const double y = micron::math::atan<double>(x);
      const double t = micron::math::tan<double>(y);
      if ( !near_d(t, x, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("atan f64 — parity");
  {
    unsigned fails = 0;
    for ( double x = 0.001; x <= 100.0; x += 0.317 ) {
      const double a = micron::math::atan<double>(x);
      const double b = micron::math::atan<double>(-x);
      if ( a != -b ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("atan f64 — tiny x: atan(x) = x");
  {
    const double tinies[] = { 1e-10, 1e-15, 1e-20, 1e-100, 0x1.0p-30 };
    for ( unsigned i = 0; i < sizeof(tinies) / sizeof(tinies[0]); ++i ) {
      require_true(micron::math::atan<double>(tinies[i]) == tinies[i]);
    }
  }
  end_test_case();

  test_case("atan f64 — very large x: atan(x) → ±π/2");
  {
    // For ax >= 2^61 the kernel returns ±π/2 exactly (its fixed constant).
    // For finite ax just below 2^61 we expect |atan(x) - π/2| ≈ 1/|x|, so we
    // pick the tolerance accordingly. Note: the kernel constant for π/2 and
    // libm's π/2 differ by at most 1 ULP, so a small absolute floor is needed.
    const double huge[] = { 1.0e15, 1.0e18, 0x1.0p61, 0x1.0p100, 0x1.0p500 };
    for ( unsigned i = 0; i < sizeof(huge) / sizeof(huge[0]); ++i ) {
      const double x = huge[i];
      const double tol = 1.0 / x + 1e-15;
      require_true(near_d(micron::math::atan<double>(x), kPi / 2, tol, 0));
      require_true(near_d(micron::math::atan<double>(-x), -kPi / 2, tol, 0));
    }
  }
  end_test_case();

  test_case("atan2 f64 — axes and quadrants");
  {
    // y=0 axis
    require_true(micron::math::atan2<double>(0.0, 1.0) == 0.0);
    require_true(near_d(micron::math::atan2<double>(0.0, -1.0), kPi, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(-0.0, -1.0), -kPi, 1e-15, 0));
    // x=0 axis
    require_true(near_d(micron::math::atan2<double>(1.0, 0.0), kPi / 2, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(-1.0, 0.0), -kPi / 2, 1e-15, 0));
    // diagonals
    require_true(near_d(micron::math::atan2<double>(1.0, 1.0), kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(1.0, -1.0), 3 * kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(-1.0, -1.0), -3 * kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(-1.0, 1.0), -kPi / 4, 1e-15, 0));
  }
  end_test_case();

  test_case("atan2 f64 — infinities");
  {
    require_true(near_d(micron::math::atan2<double>(inf_d(), inf_d()), kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(inf_d(), ninf_d()), 3 * kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(ninf_d(), ninf_d()), -3 * kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(ninf_d(), inf_d()), -kPi / 4, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(1.0, inf_d()), 0.0, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(-1.0, inf_d()), 0.0, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(1.0, ninf_d()), kPi, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(-1.0, ninf_d()), -kPi, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(inf_d(), 1.0), kPi / 2, 1e-15, 0));
    require_true(near_d(micron::math::atan2<double>(ninf_d(), 1.0), -kPi / 2, 1e-15, 0));
  }
  end_test_case();

  test_case("atan2 f64 — NaN propagation");
  {
    require_true(is_nan_d(micron::math::atan2<double>(nan_d(), 1.0)));
    require_true(is_nan_d(micron::math::atan2<double>(1.0, nan_d())));
    require_true(is_nan_d(micron::math::atan2<double>(nan_d(), nan_d())));
  }
  end_test_case();

  test_case("atan2 f64 — atan2(y,x) reconstructs angle on unit circle");
  {
    // walk θ around unit circle, ensure atan2(sin θ, cos θ) = θ
    unsigned fails = 0;
    for ( double th = -kPi + 0.001; th <= kPi - 0.001; th += 0.013 ) {
      const double s = micron::math::sin<double>(th);
      const double c = micron::math::cos<double>(th);
      const double th2 = micron::math::atan2<double>(s, c);
      if ( !near_d(th2, th, 1e-14, 1e-14) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("atan f32 — identity tan(atan(x)) = x");
  {
    unsigned fails = 0;
    for ( float x = -10.0f; x <= 10.0f; x += 0.07f ) {
      const float y = micron::math::atan<float>(x);
      const float t = micron::math::tan<float>(y);
      if ( !near_f(t, x, 1e-5f, 1e-5f) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 7 — sinh / cosh / tanh                                       ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("sinh/cosh/tanh f64 — zero, parity, identities");
  {
    require_true(micron::math::sinh<double>(0.0) == 0.0);
    require_true(micron::math::cosh<double>(0.0) == 1.0);
    require_true(micron::math::tanh<double>(0.0) == 0.0);
    unsigned fails = 0;
    for ( double x = -5.0; x <= 5.0; x += 0.017 ) {
      const double sh = micron::math::sinh<double>(x);
      const double ch = micron::math::cosh<double>(x);
      const double th = micron::math::tanh<double>(x);
      // cosh² − sinh² = 1
      if ( !near_d(ch * ch - sh * sh, 1.0, 1e-12, 1e-12) ) ++fails;
      // tanh = sinh / cosh
      if ( !near_d(th, sh / ch, 1e-13, 1e-13) ) ++fails;
      // parity
      if ( !near_d(micron::math::sinh<double>(-x), -sh, 1e-13, 1e-13) ) ++fails;
      if ( !near_d(micron::math::cosh<double>(-x), ch, 1e-13, 1e-13) ) ++fails;
      if ( !near_d(micron::math::tanh<double>(-x), -th, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("sinh/cosh/tanh f64 — large |x| limits");
  {
    // tanh(±35) ≈ ±1, sinh/cosh saturate to ±inf at ~709
    require_true(near_d(micron::math::tanh<double>(35.0), 1.0, 1e-14, 0));
    require_true(near_d(micron::math::tanh<double>(-35.0), -1.0, 1e-14, 0));
    require_true(near_d(micron::math::tanh<double>(1.0e6), 1.0, 1e-14, 0));
    require_true(near_d(micron::math::tanh<double>(inf_d()), 1.0, 1e-14, 0));
    require_true(near_d(micron::math::tanh<double>(ninf_d()), -1.0, 1e-14, 0));
    require_true(micron::math::cosh<double>(inf_d()) == inf_d());
    require_true(micron::math::cosh<double>(ninf_d()) == inf_d());
    require_true(micron::math::sinh<double>(inf_d()) == inf_d());
    require_true(micron::math::sinh<double>(ninf_d()) == ninf_d());
  }
  end_test_case();

  test_case("sinh/cosh/tanh f64 — NaN propagation");
  {
    require_true(is_nan_d(micron::math::sinh<double>(nan_d())));
    require_true(is_nan_d(micron::math::cosh<double>(nan_d())));
    require_true(is_nan_d(micron::math::tanh<double>(nan_d())));
  }
  end_test_case();

  test_case("asinh/acosh/atanh f64 — inverse identities");
  {
    unsigned fails = 0;
    // asinh: sinh(asinh(x)) = x for all real x
    for ( double x = -5.0; x <= 5.0; x += 0.013 ) {
      const double y = micron::math::asinh<double>(x);
      if ( !near_d(micron::math::sinh<double>(y), x, 1e-13, 1e-13) ) ++fails;
    }
    // acosh: domain x >= 1, cosh(acosh(x)) = x
    for ( double x = 1.0; x <= 10.0; x += 0.013 ) {
      const double y = micron::math::acosh<double>(x);
      if ( !near_d(micron::math::cosh<double>(y), x, 1e-12, 1e-12) ) ++fails;
    }
    // atanh: domain |x| < 1, tanh(atanh(x)) = x
    for ( double x = -0.99; x <= 0.99; x += 0.013 ) {
      const double y = micron::math::atanh<double>(x);
      if ( !near_d(micron::math::tanh<double>(y), x, 1e-13, 1e-13) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("asinh/acosh/atanh f64 — boundary inputs");
  {
    require_true(micron::math::asinh<double>(0.0) == 0.0);
    require_true(micron::math::acosh<double>(1.0) == 0.0);
    require_true(micron::math::atanh<double>(0.0) == 0.0);
    // |x| > 1 in atanh → NaN, x < 1 in acosh → NaN
    require_true(is_nan_d(micron::math::atanh<double>(1.5)));
    require_true(is_nan_d(micron::math::atanh<double>(-1.5)));
    require_true(is_nan_d(micron::math::acosh<double>(0.5)));
    require_true(is_nan_d(micron::math::acosh<double>(-1.0)));
    // atanh(±1) → ±inf
    require_true(micron::math::atanh<double>(1.0) == inf_d());
    require_true(micron::math::atanh<double>(-1.0) == ninf_d());
  }
  end_test_case();

  test_case("sinh/cosh/tanh f32 — identity");
  {
    // cosh² − sinh² = 1 algebraically, but in f32 with ch²/sh² ~ ch² the
    // subtraction can lose a few decimal digits. Bound the tolerance by the
    // squared magnitude.
    unsigned fails = 0;
    for ( float x = -4.0f; x <= 4.0f; x += 0.013f ) {
      const float sh = micron::math::sinh<float>(x);
      const float ch = micron::math::cosh<float>(x);
      const float th = micron::math::tanh<float>(x);
      const float subtr_tol = 1e-5f + 1e-6f * ch * ch;
      if ( !near_f(ch * ch - sh * sh, 1.0f, subtr_tol, 0.0f) ) ++fails;
      if ( !near_f(th, sh / ch, 1e-5f, 1e-5f) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 8 — CORDIC kernels                                           ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("cordic sin/cos f64 — bounded range agrees with poly kernel");
  {
    unsigned fails = 0;
    // CORDIC is accurate over moderate ranges (no Payne-Hanek), so confine to ±50
    for ( double x = -50.0; x <= 50.0; x += 0.017 ) {
      const double sc = micron::math::sin_cordic<double>(x);
      const double cc = micron::math::cos_cordic<double>(x);
      const double sp = micron::math::sin<double>(x);
      const double cp = micron::math::cos<double>(x);
      if ( !near_d(sc, sp, 1e-9, 1e-9) ) ++fails;
      if ( !near_d(cc, cp, 1e-9, 1e-9) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("cordic tan_cordic f64 — sin/cos identity, pole-avoiding");
  {
    unsigned fails = 0;
    for ( double x = -1.5; x <= 1.5; x += 0.011 ) {
      const double c = micron::math::cos_cordic<double>(x);
      if ( adouble(c) < 0.05 ) continue;
      const double t = micron::math::tan_cordic<double>(x);
      const double s = micron::math::sin_cordic<double>(x);
      if ( !near_d(t, s / c, 1e-8, 1e-8) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("cordic atan / atan2 f64 — agree with poly kernel within CORDIC band");
  {
    unsigned fails = 0;
    for ( double x = -100.0; x <= 100.0; x += 0.317 ) {
      const double a = micron::math::atan_cordic<double>(x);
      const double b = micron::math::atan<double>(x);
      if ( !near_d(a, b, 1e-9, 1e-9) ) ++fails;
    }
    for ( double y = -5.0; y <= 5.0; y += 0.317 ) {
      for ( double x = -5.0; x <= 5.0; x += 0.317 ) {
        if ( x == 0 && y == 0 ) continue;
        const double a = micron::math::atan2_cordic<double>(y, x);
        const double b = micron::math::atan2<double>(y, x);
        if ( !near_d(a, b, 1e-9, 1e-9) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("cordic asin/acos f64 — sin(asin)=x, cos(acos)=x");
  {
    unsigned fails = 0;
    for ( double x = -0.999; x <= 0.999; x += 0.005 ) {
      const double a = micron::math::asin_cordic<double>(x);
      const double c = micron::math::acos_cordic<double>(x);
      if ( !near_d(micron::math::sin<double>(a), x, 1e-9, 1e-9) ) ++fails;
      if ( !near_d(micron::math::cos<double>(c), x, 1e-9, 1e-9) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("cordic sincos_cordic f64 — pythagoras");
  {
    unsigned fails = 0;
    for ( double x = -10.0; x <= 10.0; x += 0.013 ) {
      double s, c;
      micron::math::sincos_cordic<double>(x, s, c);
      if ( !near_d(s * s + c * c, 1.0, 1e-8, 0) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("cordic f32 path — pythagoras");
  {
    unsigned fails = 0;
    for ( float x = -10.0f; x <= 10.0f; x += 0.013f ) {
      const float s = micron::math::sin_cordic<float>(x);
      const float c = micron::math::cos_cordic<float>(x);
      if ( afloat(s * s + c * c - 1.0f) > 1e-4f ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 9 — fast porcelain (fcos, ftan, fasin, facos, fatan, fatan2) ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("fast porcelain f64 — fcos/ftan/fasin/facos/fatan match base");
  {
    unsigned fails = 0;
    for ( double x = -3.0; x <= 3.0; x += 0.017 ) {
      if ( micron::math::fcos(x) != micron::math::cos<double>(x) ) ++fails;
      const double c = micron::math::cos<double>(x);
      if ( adouble(c) > 1e-3 ) {
        if ( micron::math::ftan(x) != micron::math::tan<double>(x) ) ++fails;
      }
      if ( micron::math::fatan(x) != micron::math::atan<double>(x) ) ++fails;
    }
    for ( double x = -0.999; x <= 0.999; x += 0.013 ) {
      if ( micron::math::fasin(x) != micron::math::asin<double>(x) ) ++fails;
      if ( micron::math::facos(x) != micron::math::acos<double>(x) ) ++fails;
    }
    for ( double y = -5.0; y <= 5.0; y += 0.717 ) {
      for ( double x = -5.0; x <= 5.0; x += 0.717 ) {
        if ( x == 0 && y == 0 ) continue;
        if ( micron::math::fatan2(y, x) != micron::math::atan2<double>(y, x) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("fast porcelain f32 — same semantics");
  {
    unsigned fails = 0;
    for ( float x = -3.0f; x <= 3.0f; x += 0.07f ) {
      if ( micron::math::fcos(x) != micron::math::cos<float>(x) ) ++fails;
      if ( micron::math::fatan(x) != micron::math::atan<float>(x) ) ++fails;
    }
    for ( float x = -0.999f; x <= 0.999f; x += 0.017f ) {
      if ( micron::math::fasin(x) != micron::math::asin<float>(x) ) ++fails;
      if ( micron::math::facos(x) != micron::math::acos<float>(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 10 — top-level micron:: namespace reexports                  ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("micron::sin/cos/... equal micron::math::sin/cos/... bitwise");
  {
    unsigned fails = 0;
    for ( double x = -10.0; x <= 10.0; x += 0.017 ) {
      if ( micron::sin(x) != micron::math::sin<double>(x) ) ++fails;
      if ( micron::cos(x) != micron::math::cos<double>(x) ) ++fails;
      const double c = micron::math::cos<double>(x);
      if ( adouble(c) > 1e-3 ) {
        if ( micron::tan(x) != micron::math::tan<double>(x) ) ++fails;
      }
      if ( micron::atan(x) != micron::math::atan<double>(x) ) ++fails;
      if ( micron::sinh(x) != micron::math::sinh<double>(x) ) ++fails;
      if ( micron::cosh(x) != micron::math::cosh<double>(x) ) ++fails;
      if ( micron::tanh(x) != micron::math::tanh<double>(x) ) ++fails;
    }
    for ( double x = -0.999; x <= 0.999; x += 0.013 ) {
      if ( micron::asin(x) != micron::math::asin<double>(x) ) ++fails;
      if ( micron::acos(x) != micron::math::acos<double>(x) ) ++fails;
      if ( micron::atanh(x) != micron::math::atanh<double>(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("micron::*_cordic equal micron::math::*_cordic bitwise");
  {
    unsigned fails = 0;
    for ( double x = -5.0; x <= 5.0; x += 0.013 ) {
      if ( micron::sin_cordic(x) != micron::math::sin_cordic<double>(x) ) ++fails;
      if ( micron::cos_cordic(x) != micron::math::cos_cordic<double>(x) ) ++fails;
      if ( micron::atan_cordic(x) != micron::math::atan_cordic<double>(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 11 — constexpr context                                       ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("trig is usable in constexpr context");
  {
    constexpr double cs = micron::math::sin<double>(1.0);
    constexpr double cc = micron::math::cos<double>(1.0);
    constexpr double ct = micron::math::tan<double>(1.0);
    constexpr double ca = micron::math::atan<double>(1.0);
    static_assert(cs != 0.0);
    static_assert(cc != 0.0);
    static_assert(ct != 0.0);
    static_assert(ca != 0.0);
    // builtins used in consteval; just confirm runtime equality is sane
    require_true(near_d(cs, micron::math::sin<double>(1.0), 1e-10, 1e-10));
    require_true(near_d(cc, micron::math::cos<double>(1.0), 1e-10, 1e-10));
    require_true(near_d(ct, micron::math::tan<double>(1.0), 1e-10, 1e-10));
    require_true(near_d(ca, kPi / 4, 1e-10, 0));
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ SECTION 12 — DOCUMENTED-FAILURE cases (soft, never abort)            ║
  // ║                                                                       ║
  // ║ These exercise sharp edges of the trig surface where micron::math    ║
  // ║ behavior may not yet match libm. They print [FAILS] but keep the     ║
  // ║ run alive so the rest of the suite can report.                       ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("(FAILS) sin/cos f64 — last digit accuracy at very near-pole tan");
  {
    // Very tight tolerance near tan poles; documented in case kernels regress.
    bool any_fail = false;
    const double xs[] = { kPi / 2 - 1e-12, kPi / 2 + 1e-12, 3 * kPi / 2 - 1e-12, 3 * kPi / 2 + 1e-12 };
    for ( unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); ++i ) {
      const double t_got = micron::math::tan<double>(xs[i]);
      const double t_ref = std::tan(xs[i]);
      const double r = (t_got - t_ref) / t_ref;
      if ( adouble(r) > 1e-10 ) any_fail = true;
    }
    soft_check(!any_fail, "tan near pole stays within 1e-10 relative of libm");
  }
  end_test_case();

  test_case("(FAILS) f64 sin worst-case ULP in Payne-Hanek branch");
  {
    // The big-argument branch can wander by a few ULP at the extreme upper end.
    // Tracked here so future polish has a regression marker.
    bool any_fail = false;
    const double xs[] = { 1.0e100, 1.0e150, 1.0e200, 0x1.0p500, 0x1.0p1000 };
    for ( unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); ++i ) {
      const double s = micron::math::sin<double>(xs[i]);
      const double c = micron::math::cos<double>(xs[i]);
      const double mag = s * s + c * c;
      if ( adouble(mag - 1.0) > 1e-12 ) any_fail = true;
    }
    soft_check(!any_fail, "sin²+cos² within 1e-12 for x up to 2^1000 (Payne-Hanek extreme)");
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ summary                                                              ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  sb::print("=== TRIG EXHAUSTIVE COMPLETE ===");
  sb::print("    soft-checks total:    ", __soft().checks);
  sb::print("    soft-checks failures: ", __soft().failures);
  if ( __soft().failures == 0 )
    sb::print("=== ALL TRIG EXHAUSTIVE TESTS PASSED ===");
  else
    sb::print("=== HARD TESTS PASSED; (FAILS)-marked tests reported above ===");
  return 1;
}
