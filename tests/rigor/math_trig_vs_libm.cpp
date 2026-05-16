// math_trig_vs_libm.cpp
// Per-function accuracy report for micron's trig surface against the libc /
// __builtin_* reference. Each function is swept over a curated grid of inputs
// (small, medium, large; positive and negative branches) and we collect:
//
//   max absolute error  : worst |micron(x) - libm(x)|
//   max relative error  : worst |Δ| / |libm(x)|
//   max ULP distance    : worst floating-point ULP gap
//
// We then bucket the worst input alongside the worst measurement so the report
// is actionable — a regression test that surfaces *which* x trips the kernel,
// not just a pass/fail bit.
//
// The hard `require_true` gates compare the worst observed deviation against a
// generous engineering budget per function. A few inputs that consistently
// blow past that budget are kept on a soft (FAILS) list to document the
// limitation without aborting the run, as in abcmalloc_stress.cpp.

#include "../../src/math.hpp"
#include "../../src/math/ieee.hpp"
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
// soft-failure infrastructure
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
// math helpers
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

template<typename F>
static inline F
absv(F v) noexcept
{
  return v < 0 ? -v : v;
}

// ULP distance via micron::math::ieee.
template<typename F>
static inline long long
ulp_dist(F a, F b) noexcept
{
  return (long long)(micron::math::ieee::ulp_distance<F>(a, b));
}

// ───────────────────────────────────────────────────────────────────────────
// per-function deviation accumulator
// ───────────────────────────────────────────────────────────────────────────

template<typename F> struct stats {
  F max_abs = 0;
  F max_rel = 0;
  long long max_ulp = 0;
  F worst_abs_x = 0;
  F worst_rel_x = 0;
  F worst_ulp_x = 0;
  unsigned long n = 0;
};

// Both absolute and ULP error are tracked, but ULP is suppressed for nearly-
// zero outputs because ULP distance is meaningless across the dense denormal
// region (e.g. sin(π) = 1.22e-16 vs ref 0.0 is ~17000 ULP). For outputs where
// |ref| < tiny, only the absolute error is meaningful.
template<typename F> static constexpr F ulp_floor_v() noexcept;

template<>
constexpr double
ulp_floor_v<double>() noexcept
{
  return 1e-14;
}

template<>
constexpr float
ulp_floor_v<float>() noexcept
{
  return 1e-6f;
}

template<typename F>
static void
record(stats<F> &s, F x, F got, F ref) noexcept
{
  ++s.n;
  if ( micron::math::ieee::is_nan<F>(got) || micron::math::ieee::is_nan<F>(ref) ) return;
  const F a = absv(F(got - ref));
  const F m = absv(ref);
  const F r = (m > F(0)) ? (a / m) : a;
  if ( a > s.max_abs ) {
    s.max_abs = a;
    s.worst_abs_x = x;
  }
  if ( r > s.max_rel ) {
    s.max_rel = r;
    s.worst_rel_x = x;
  }
  // Skip ULP for tiny |ref| — the denormal ULP step is too fine to be
  // meaningful at the zeros of sin/cos and other functions.
  if ( m > ulp_floor_v<F>() ) {
    const long long u = ulp_dist<F>(got, ref);
    if ( u >= 0 && u > s.max_ulp ) {
      s.max_ulp = u;
      s.worst_ulp_x = x;
    }
  }
}

template<typename F>
static void
report(const char *fn, const stats<F> &s) noexcept
{
  sb::print("    ", fn, " — N=", (long long)s.n);
  sb::print("        max_abs=", (double)s.max_abs, " @ x=", (double)s.worst_abs_x);
  sb::print("        max_rel=", (double)s.max_rel, " @ x=", (double)s.worst_rel_x);
  sb::print("        max_ulp=", (long long)s.max_ulp, " @ x=", (double)s.worst_ulp_x);
}

// ───────────────────────────────────────────────────────────────────────────
// reduced-precision references — __builtin_* uses libm directly
// ───────────────────────────────────────────────────────────────────────────

static double
ref_sin(double x) noexcept
{
  return __builtin_sin(x);
}

static double
ref_cos(double x) noexcept
{
  return __builtin_cos(x);
}

static double
ref_tan(double x) noexcept
{
  return __builtin_tan(x);
}

static double
ref_asin(double x) noexcept
{
  return __builtin_asin(x);
}

static double
ref_acos(double x) noexcept
{
  return __builtin_acos(x);
}

static double
ref_atan(double x) noexcept
{
  return __builtin_atan(x);
}

static double
ref_atan2(double y, double x) noexcept
{
  return __builtin_atan2(y, x);
}

static double
ref_sinh(double x) noexcept
{
  return __builtin_sinh(x);
}

static double
ref_cosh(double x) noexcept
{
  return __builtin_cosh(x);
}

static double
ref_tanh(double x) noexcept
{
  return __builtin_tanh(x);
}

static double
ref_asinh(double x) noexcept
{
  return __builtin_asinh(x);
}

static double
ref_acosh(double x) noexcept
{
  return __builtin_acosh(x);
}

static double
ref_atanh(double x) noexcept
{
  return __builtin_atanh(x);
}

static float
ref_sinf(float x) noexcept
{
  return __builtin_sinf(x);
}

static float
ref_cosf(float x) noexcept
{
  return __builtin_cosf(x);
}

static float
ref_tanf(float x) noexcept
{
  return __builtin_tanf(x);
}

static float
ref_asinf(float x) noexcept
{
  return __builtin_asinf(x);
}

static float
ref_acosf(float x) noexcept
{
  return __builtin_acosf(x);
}

static float
ref_atanf(float x) noexcept
{
  return __builtin_atanf(x);
}

// ───────────────────────────────────────────────────────────────────────────
// grid generators
// ───────────────────────────────────────────────────────────────────────────

template<typename Fn>
static void
log_grid(double lo, double hi, unsigned n, Fn &&fn)
{
  const double llo = std::log(lo);
  const double lhi = std::log(hi);
  for ( unsigned i = 0; i < n; ++i ) {
    const double t = double(i) / double(n - 1);
    fn(std::exp(llo + t * (lhi - llo)));
  }
}

template<typename Fn>
static void
linear_grid(double lo, double hi, unsigned n, Fn &&fn)
{
  for ( unsigned i = 0; i < n; ++i ) {
    const double t = double(i) / double(n - 1);
    fn(lo + t * (hi - lo));
  }
}

};      // anonymous namespace

int
main()
{
  print("=== TRIG vs LIBM ACCURACY REPORT ===");

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ f64: forward trig                                                    ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("f64 sin — small range [-π, π]");
  {
    stats<double> s{};
    linear_grid(-3.14159265358979, 3.14159265358979, 8192,
                [&](double x) { record<double>(s, x, micron::math::sin<double>(x), ref_sin(x)); });
    report<double>("sin f64 small", s);
    require_true(s.max_ulp <= 2);
    require_true(s.max_abs <= 5e-16);
  }
  end_test_case();

  test_case("f64 sin — medium range [-100, 100]");
  {
    stats<double> s{};
    linear_grid(-100.0, 100.0, 8192, [&](double x) { record<double>(s, x, micron::math::sin<double>(x), ref_sin(x)); });
    report<double>("sin f64 medium", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 sin — Cody-Waite range [100, 2^20]");
  {
    stats<double> s{};
    log_grid(100.0, 1.0e6, 8192, [&](double x) { record<double>(s, x, micron::math::sin<double>(x), ref_sin(x)); });
    report<double>("sin f64 cody-waite", s);
    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("(FAILS) f64 sin — Payne-Hanek range [2^33, 1e15] vs libm");
  {
    // The Payne-Hanek reduction now agrees with libm to ~1e-13 absolute
    // (post-fix; previously drifted by ~1 in absolute terms). Residual ULP
    // distance is from the downstream f64 reconstruction (`f` is materialized
    // as a single double from the 128-bit accumulator, capping precision at
    // 53 bits before the π/2 multiply). True libm parity would require dd64
    // throughout the f-to-r path.
    stats<double> s{};
    log_grid(1.0e10, 1.0e15, 4096, [&](double x) { record<double>(s, x, micron::math::sin<double>(x), ref_sin(x)); });
    report<double>("sin f64 payne-hanek", s);
    soft_check(s.max_abs <= 1e-10, "sin f64 Payne-Hanek max abs <= 1e-10 vs libm");
    soft_check(s.max_ulp <= 32, "sin f64 Payne-Hanek max ULP <= 32 (limited by f64 reconstruction)");
  }
  end_test_case();

  test_case("f64 cos — small range [-π, π]");
  {
    stats<double> s{};
    linear_grid(-3.14159265358979, 3.14159265358979, 8192,
                [&](double x) { record<double>(s, x, micron::math::cos<double>(x), ref_cos(x)); });
    report<double>("cos f64 small", s);
    require_true(s.max_ulp <= 2);
  }
  end_test_case();

  test_case("f64 cos — medium range [-100, 100]");
  {
    stats<double> s{};
    linear_grid(-100.0, 100.0, 8192, [&](double x) { record<double>(s, x, micron::math::cos<double>(x), ref_cos(x)); });
    report<double>("cos f64 medium", s);
    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("f64 tan — small range [-1.5, 1.5] pole-avoiding");
  {
    stats<double> s{};
    linear_grid(-1.5, 1.5, 8192, [&](double x) {
      if ( adouble(__builtin_cos(x)) < 1e-4 ) return;
      record<double>(s, x, micron::math::tan<double>(x), ref_tan(x));
    });
    report<double>("tan f64 small", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 tan — wide range [-1e5, 1e5] pole-avoiding");
  {
    stats<double> s{};
    linear_grid(-1.0e5, 1.0e5, 8192, [&](double x) {
      if ( adouble(__builtin_cos(x)) < 1e-3 ) return;
      record<double>(s, x, micron::math::tan<double>(x), ref_tan(x));
    });
    report<double>("tan f64 wide", s);
    require_true(s.max_ulp <= 64);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ f64: inverse trig                                                    ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("f64 asin — [-1, 1]");
  {
    stats<double> s{};
    linear_grid(-0.99999, 0.99999, 8192, [&](double x) { record<double>(s, x, micron::math::asin<double>(x), ref_asin(x)); });
    // boundary points exactly
    record<double>(s, 1.0, micron::math::asin<double>(1.0), ref_asin(1.0));
    record<double>(s, -1.0, micron::math::asin<double>(-1.0), ref_asin(-1.0));
    report<double>("asin f64", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 acos — [-1, 1]");
  {
    stats<double> s{};
    linear_grid(-0.99999, 0.99999, 8192, [&](double x) { record<double>(s, x, micron::math::acos<double>(x), ref_acos(x)); });
    record<double>(s, 1.0, micron::math::acos<double>(1.0), ref_acos(1.0));
    record<double>(s, -1.0, micron::math::acos<double>(-1.0), ref_acos(-1.0));
    report<double>("acos f64", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 atan — wide range");
  {
    stats<double> s{};
    log_grid(1e-10, 1e10, 8192, [&](double x) {
      record<double>(s, x, micron::math::atan<double>(x), ref_atan(x));
      record<double>(s, -x, micron::math::atan<double>(-x), ref_atan(-x));
    });
    report<double>("atan f64", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 atan2 — full 4-quadrant grid");
  {
    stats<double> s{};
    const unsigned NN = 96;
    for ( unsigned i = 0; i < NN; ++i ) {
      const double y = -10.0 + 20.0 * double(i) / double(NN - 1);
      for ( unsigned j = 0; j < NN; ++j ) {
        const double x = -10.0 + 20.0 * double(j) / double(NN - 1);
        if ( x == 0.0 && y == 0.0 ) continue;
        record<double>(s, x, micron::math::atan2<double>(y, x), ref_atan2(y, x));
      }
    }
    report<double>("atan2 f64", s);
    require_true(s.max_ulp <= 6);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ f64: hyperbolic                                                      ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("f64 sinh — range [-50, 50]");
  {
    stats<double> s{};
    linear_grid(-50.0, 50.0, 8192, [&](double x) { record<double>(s, x, micron::math::sinh<double>(x), ref_sinh(x)); });
    report<double>("sinh f64", s);
    // sinh chains expm1 with a couple of arithmetic ops, so a few ulps of
    // drift across the full magnitude range is expected.
    require_true(s.max_ulp <= 32);
    require_true(s.max_rel <= 1e-14);
  }
  end_test_case();

  test_case("f64 cosh — range [-50, 50]");
  {
    stats<double> s{};
    linear_grid(-50.0, 50.0, 8192, [&](double x) { record<double>(s, x, micron::math::cosh<double>(x), ref_cosh(x)); });
    report<double>("cosh f64", s);
    require_true(s.max_ulp <= 32);
    require_true(s.max_rel <= 1e-14);
  }
  end_test_case();

  test_case("f64 tanh — range [-30, 30]");
  {
    stats<double> s{};
    linear_grid(-30.0, 30.0, 8192, [&](double x) { record<double>(s, x, micron::math::tanh<double>(x), ref_tanh(x)); });
    report<double>("tanh f64", s);
    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("f64 asinh — wide range");
  {
    stats<double> s{};
    log_grid(1e-9, 1e9, 4096, [&](double x) {
      record<double>(s, x, micron::math::asinh<double>(x), ref_asinh(x));
      record<double>(s, -x, micron::math::asinh<double>(-x), ref_asinh(-x));
    });
    report<double>("asinh f64", s);
    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("f64 acosh — range [1, 1e6]");
  {
    stats<double> s{};
    log_grid(1.0001, 1.0e6, 4096, [&](double x) { record<double>(s, x, micron::math::acosh<double>(x), ref_acosh(x)); });
    report<double>("acosh f64", s);
    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("f64 atanh — range (-1, 1)");
  {
    stats<double> s{};
    linear_grid(-0.99999, 0.99999, 4096, [&](double x) { record<double>(s, x, micron::math::atanh<double>(x), ref_atanh(x)); });
    report<double>("atanh f64", s);
    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ f32                                                                   ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("f32 sin — range [-π, π]");
  {
    stats<float> s{};
    for ( unsigned i = 0; i < 8192; ++i ) {
      const float x = float(-3.14159265358979 + 6.28318530717958 * double(i) / 8191.0);
      record<float>(s, x, micron::math::sin<float>(x), ref_sinf(x));
    }
    report<float>("sin f32", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f32 cos — range [-π, π]");
  {
    stats<float> s{};
    for ( unsigned i = 0; i < 8192; ++i ) {
      const float x = float(-3.14159265358979 + 6.28318530717958 * double(i) / 8191.0);
      record<float>(s, x, micron::math::cos<float>(x), ref_cosf(x));
    }
    report<float>("cos f32", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f32 sin — Payne-Hanek path");
  {
    stats<float> s{};
    for ( unsigned i = 0; i < 4096; ++i ) {
      const float x = float(1e7 + 1.0 * double(i));
      record<float>(s, x, micron::math::sin<float>(x), ref_sinf(x));
    }
    report<float>("sin f32 large", s);
    require_true(s.max_ulp <= 32);
  }
  end_test_case();

  test_case("f32 tan — [-1.5, 1.5] pole-avoiding");
  {
    stats<float> s{};
    for ( unsigned i = 0; i < 4096; ++i ) {
      const float x = float(-1.5 + 3.0 * double(i) / 4095.0);
      if ( afloat(__builtin_cosf(x)) < 1e-3f ) continue;
      record<float>(s, x, micron::math::tan<float>(x), ref_tanf(x));
    }
    report<float>("tan f32", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f32 asin/acos — [-1, 1]");
  {
    stats<float> sa{};
    stats<float> sc{};
    for ( unsigned i = 0; i < 4096; ++i ) {
      const float x = float(-0.9999 + 1.9998 * double(i) / 4095.0);
      record<float>(sa, x, micron::math::asin<float>(x), ref_asinf(x));
      record<float>(sc, x, micron::math::acos<float>(x), ref_acosf(x));
    }
    report<float>("asin f32", sa);
    report<float>("acos f32", sc);
    require_true(sa.max_ulp <= 8);
    require_true(sc.max_ulp <= 8);
  }
  end_test_case();

  test_case("f32 atan — wide");
  {
    stats<float> s{};
    for ( unsigned i = 0; i < 4096; ++i ) {
      const float x = float(-1e6 + 2e6 * double(i) / 4095.0);
      record<float>(s, x, micron::math::atan<float>(x), ref_atanf(x));
    }
    report<float>("atan f32", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ stress: random sampling across full f64 range                        ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("f64 sin/cos — pseudo-random over [-1e6, 1e6]");
  {
    // tiny xorshift; deterministic seeding so the report is reproducible.
    unsigned long long state = 0xDEADBEEF12345678ULL;
    auto next = [&]() -> double {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      // convert top 53 bits to a double in [-1, 1) and scale to [-1e6, 1e6]
      const unsigned long long m = state >> 11;
      const double u = double(m) * (1.0 / double(1ULL << 53));      // [0, 1)
      return (u - 0.5) * 2.0e6;
    };
    stats<double> ss{};
    stats<double> sc{};
    for ( unsigned i = 0; i < 16384; ++i ) {
      const double x = next();
      record<double>(ss, x, micron::math::sin<double>(x), ref_sin(x));
      record<double>(sc, x, micron::math::cos<double>(x), ref_cos(x));
    }
    report<double>("sin f64 random", ss);
    report<double>("cos f64 random", sc);
    require_true(ss.max_ulp <= 32);
    require_true(sc.max_ulp <= 32);
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ DOCUMENTED-FAILURE: extreme Payne-Hanek band                          ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  test_case("(FAILS) f64 sin extreme: pythagoras within 1e-14 above 2^100");
  {
    // The pythagoras invariant sin²+cos²=1 begins to drift past the very far
    // end of the Payne-Hanek range. Documented here, never aborted.
    bool any = false;
    const double xs[] = { 0x1.0p100, 0x1.0p200, 0x1.0p400, 0x1.0p600, 0x1.0p900 };
    for ( unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); ++i ) {
      const double s = micron::math::sin<double>(xs[i]);
      const double c = micron::math::cos<double>(xs[i]);
      const double mag = s * s + c * c;
      if ( adouble(mag - 1.0) > 1e-14 ) any = true;
    }
    soft_check(!any, "sin²+cos²−1 < 1e-14 for x in [2^100, 2^900]");
  }
  end_test_case();

  test_case("(FAILS) f64 tan worst-case ULP near edge of polynomial accuracy");
  {
    // tan grows fast and the bare s/c reciprocation amplifies any sin/cos
    // error. We track the worst-case observed but do not abort, since it
    // depends on libm's particular rounding that may differ across systems.
    long long worst = 0;
    for ( unsigned i = 0; i < 4096; ++i ) {
      const double t = -1.5 + 3.0 * double(i) / 4095.0;
      if ( adouble(__builtin_cos(t)) < 1e-5 ) continue;
      const long long u = ulp_dist<double>(micron::math::tan<double>(t), ref_tan(t));
      if ( u >= 0 && u > worst ) worst = u;
    }
    sb::print("    tan f64 worst near-edge ULP=", worst);
    soft_check(worst <= 8, "tan f64 worst ULP near edge <= 8");
  }
  end_test_case();

  // ╔══════════════════════════════════════════════════════════════════════╗
  // ║ summary                                                              ║
  // ╚══════════════════════════════════════════════════════════════════════╝

  sb::print("=== TRIG vs LIBM REPORT COMPLETE ===");
  sb::print("    soft-checks total:    ", __soft().checks);
  sb::print("    soft-checks failures: ", __soft().failures);
  if ( __soft().failures == 0 )
    sb::print("=== ALL ACCURACY TESTS PASSED ===");
  else
    sb::print("=== HARD TESTS PASSED; (FAILS)-marked tests reported above ===");
  return 1;
}
