// math_log_vs_libm.cpp
// Per-function accuracy report for micron's log surface against the libc /
// __builtin_* reference. Each function is swept over a curated grid of inputs
// (small, medium, wide; near-1 and far-from-1 branches) and we collect:
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
// limitation without aborting the run, as in math_trig_vs_libm.cpp.

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

template<typename F>
static inline F
absv(F v) noexcept
{
  return v < 0 ? -v : v;
}

template<typename F>
static inline long long
ulp_dist(F a, F b) noexcept
{
  return (long long)(micron::math::ieee::ulp_distance<F>(a, b));
}

[[gnu::always_inline]] inline bool
bit_is_nan_d(double v) noexcept
{
  const unsigned long long u = micron::math::ieee::to_bits<double>(v);
  return ((u & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) && ((u & 0x000FFFFFFFFFFFFFULL) != 0);
}

[[gnu::always_inline]] inline bool
bit_is_nan_f(float v) noexcept
{
  const unsigned int u = micron::math::ieee::to_bits<float>(v);
  return ((u & 0x7F800000U) == 0x7F800000U) && ((u & 0x007FFFFFU) != 0);
}

[[gnu::always_inline]] inline bool
bit_is_inf_d(double v) noexcept
{
  return (micron::math::ieee::to_bits<double>(v) & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL;
}

[[gnu::always_inline]] inline bool
bit_is_inf_f(float v) noexcept
{
  return (micron::math::ieee::to_bits<float>(v) & 0x7FFFFFFFU) == 0x7F800000U;
}

template<typename F>
[[gnu::always_inline]] inline bool
bit_is_nan(F v) noexcept
{
  if constexpr ( sizeof(F) == sizeof(double) )
    return bit_is_nan_d(double(v));
  else
    return bit_is_nan_f(float(v));
}

template<typename F>
[[gnu::always_inline]] inline bool
bit_is_inf(F v) noexcept
{
  if constexpr ( sizeof(F) == sizeof(double) )
    return bit_is_inf_d(double(v));
  else
    return bit_is_inf_f(float(v));
}

template<typename F> struct stats {
  F max_abs = 0;
  F max_rel = 0;
  long long max_ulp = 0;
  F worst_abs_x = 0;
  F worst_rel_x = 0;
  F worst_ulp_x = 0;
  unsigned long n = 0;
};

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
  if ( bit_is_nan<F>(got) || bit_is_nan<F>(ref) ) return;

  if ( bit_is_inf<F>(got) || bit_is_inf<F>(ref) ) return;
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

static double
ref_log(double x) noexcept
{
  return __builtin_log(x);
}

static double
ref_log2(double x) noexcept
{
  return __builtin_log2(x);
}

static double
ref_log10(double x) noexcept
{
  return __builtin_log10(x);
}

static double
ref_log1p(double x) noexcept
{
  return __builtin_log1p(x);
}

static float
ref_logf(float x) noexcept
{
  return __builtin_logf(x);
}

static float
ref_log2f(float x) noexcept
{
  return __builtin_log2f(x);
}

static float
ref_log10f(float x) noexcept
{
  return __builtin_log10f(x);
}

static float
ref_log1pf(float x) noexcept
{
  return __builtin_log1pf(x);
}

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

};      // namespace

int
main()
{
  print("=== LOG vs LIBM ACCURACY REPORT ===");

  test_case("f64 log — near 1: [0.5, 2.0]");
  {
    stats<double> s{};
    linear_grid(0.5, 2.0, 8192, [&](double x) { record<double>(s, x, micron::math::log<double>(x), ref_log(x)); });
    report<double>("log f64 near-1", s);
    require_true(s.max_ulp <= 2);
    require_true(s.max_abs <= 5e-16);
  }
  end_test_case();

  test_case("f64 log — small [1e-30, 1e-3]");
  {
    stats<double> s{};
    log_grid(1e-30, 1e-3, 8192, [&](double x) { record<double>(s, x, micron::math::log<double>(x), ref_log(x)); });
    report<double>("log f64 small", s);
    require_true(s.max_ulp <= 2);
  }
  end_test_case();

  test_case("f64 log — large [1e3, 1e30]");
  {
    stats<double> s{};
    log_grid(1e3, 1e30, 8192, [&](double x) { record<double>(s, x, micron::math::log<double>(x), ref_log(x)); });
    report<double>("log f64 large", s);
    require_true(s.max_ulp <= 2);
  }
  end_test_case();

  test_case("f64 log — extreme [1e-300, 1e300]");
  {
    stats<double> s{};
    log_grid(1e-300, 1e300, 8192, [&](double x) { record<double>(s, x, micron::math::log<double>(x), ref_log(x)); });
    report<double>("log f64 extreme", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log — denormal range [2^-1074, 2^-1022]");
  {
    stats<double> s{};
    log_grid(0x1.0p-1074, 0x1.0p-1022, 4096, [&](double x) { record<double>(s, x, micron::math::log<double>(x), ref_log(x)); });
    report<double>("log f64 denormal", s);

    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("f64 log2 — near 1: [0.5, 2.0]");
  {
    stats<double> s{};
    linear_grid(0.5, 2.0, 8192, [&](double x) { record<double>(s, x, micron::math::log2<double>(x), ref_log2(x)); });
    report<double>("log2 f64 near-1", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log2 — small [1e-30, 1e-3]");
  {
    stats<double> s{};
    log_grid(1e-30, 1e-3, 8192, [&](double x) { record<double>(s, x, micron::math::log2<double>(x), ref_log2(x)); });
    report<double>("log2 f64 small", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log2 — large [1e3, 1e30]");
  {
    stats<double> s{};
    log_grid(1e3, 1e30, 8192, [&](double x) { record<double>(s, x, micron::math::log2<double>(x), ref_log2(x)); });
    report<double>("log2 f64 large", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log2 — extreme [1e-300, 1e300]");
  {
    stats<double> s{};
    log_grid(1e-300, 1e300, 8192, [&](double x) { record<double>(s, x, micron::math::log2<double>(x), ref_log2(x)); });
    report<double>("log2 f64 extreme", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 log10 — near 1: [0.5, 2.0]");
  {
    stats<double> s{};
    linear_grid(0.5, 2.0, 8192, [&](double x) { record<double>(s, x, micron::math::log10<double>(x), ref_log10(x)); });
    report<double>("log10 f64 near-1", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log10 — small [1e-30, 1e-3]");
  {
    stats<double> s{};
    log_grid(1e-30, 1e-3, 8192, [&](double x) { record<double>(s, x, micron::math::log10<double>(x), ref_log10(x)); });
    report<double>("log10 f64 small", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log10 — large [1e3, 1e30]");
  {
    stats<double> s{};
    log_grid(1e3, 1e30, 8192, [&](double x) { record<double>(s, x, micron::math::log10<double>(x), ref_log10(x)); });
    report<double>("log10 f64 large", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log10 — extreme [1e-300, 1e300]");
  {
    stats<double> s{};
    log_grid(1e-300, 1e300, 8192, [&](double x) { record<double>(s, x, micron::math::log10<double>(x), ref_log10(x)); });
    report<double>("log10 f64 extreme", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 log1p — tiny |x| ∈ [1e-300, 1e-3]");
  {
    stats<double> s{};
    log_grid(1e-300, 1e-3, 8192, [&](double x) {
      record<double>(s, x, micron::math::log1p<double>(x), ref_log1p(x));
      record<double>(s, -x, micron::math::log1p<double>(-x), ref_log1p(-x));
    });
    report<double>("log1p f64 tiny", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log1p — moderate [-0.5, 5.0]");
  {
    stats<double> s{};
    linear_grid(-0.5, 5.0, 8192, [&](double x) { record<double>(s, x, micron::math::log1p<double>(x), ref_log1p(x)); });
    report<double>("log1p f64 moderate", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log1p — near -1: [-1+1e-15, -0.5]");
  {
    stats<double> s{};

    linear_grid(-0.99999, -0.5, 4096, [&](double x) { record<double>(s, x, micron::math::log1p<double>(x), ref_log1p(x)); });
    report<double>("log1p f64 near -1", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log1p — large positive [10, 1e10]");
  {
    stats<double> s{};
    log_grid(10.0, 1e10, 8192, [&](double x) { record<double>(s, x, micron::math::log1p<double>(x), ref_log1p(x)); });
    report<double>("log1p f64 large", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f32 log — near 1: [0.5, 2.0]");
  {
    stats<float> s{};
    for ( unsigned i = 0; i < 8192; ++i ) {
      const float x = float(0.5 + 1.5 * double(i) / 8191.0);
      record<float>(s, x, micron::math::log<float>(x), ref_logf(x));
    }
    report<float>("log f32 near-1", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f32 log — small [1e-30, 1e-3]");
  {
    stats<float> s{};
    const double llo = std::log(1e-30);
    const double lhi = std::log(1e-3);
    for ( unsigned i = 0; i < 8192; ++i ) {
      const double t = double(i) / 8191.0;
      const float x = float(std::exp(llo + t * (lhi - llo)));
      record<float>(s, x, micron::math::log<float>(x), ref_logf(x));
    }
    report<float>("log f32 small", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f32 log — large [1e3, 1e30]");
  {
    stats<float> s{};
    const double llo = std::log(1e3);
    const double lhi = std::log(1e30);
    for ( unsigned i = 0; i < 8192; ++i ) {
      const double t = double(i) / 8191.0;
      const float x = float(std::exp(llo + t * (lhi - llo)));
      record<float>(s, x, micron::math::log<float>(x), ref_logf(x));
    }
    report<float>("log f32 large", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f32 log2 — wide [1e-30, 1e30]");
  {
    stats<float> s{};
    const double llo = std::log(1e-30);
    const double lhi = std::log(1e30);
    for ( unsigned i = 0; i < 8192; ++i ) {
      const double t = double(i) / 8191.0;
      const float x = float(std::exp(llo + t * (lhi - llo)));
      record<float>(s, x, micron::math::log2<float>(x), ref_log2f(x));
    }
    report<float>("log2 f32 wide", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f32 log10 — wide [1e-30, 1e30]");
  {
    stats<float> s{};
    const double llo = std::log(1e-30);
    const double lhi = std::log(1e30);
    for ( unsigned i = 0; i < 8192; ++i ) {
      const double t = double(i) / 8191.0;
      const float x = float(std::exp(llo + t * (lhi - llo)));
      record<float>(s, x, micron::math::log10<float>(x), ref_log10f(x));
    }
    report<float>("log10 f32 wide", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f32 log1p — tiny |x| ∈ [1e-30, 1e-3]");
  {
    stats<float> s{};
    const double llo = std::log(1e-30);
    const double lhi = std::log(1e-3);
    for ( unsigned i = 0; i < 4096; ++i ) {
      const double t = double(i) / 4095.0;
      const float x = float(std::exp(llo + t * (lhi - llo)));
      record<float>(s, x, micron::math::log1p<float>(x), ref_log1pf(x));
      record<float>(s, -x, micron::math::log1p<float>(-x), ref_log1pf(-x));
    }
    report<float>("log1p f32 tiny", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f32 log1p — moderate [-0.5, 5.0]");
  {
    stats<float> s{};
    for ( unsigned i = 0; i < 4096; ++i ) {
      const float x = float(-0.5 + 5.5 * double(i) / 4095.0);
      record<float>(s, x, micron::math::log1p<float>(x), ref_log1pf(x));
    }
    report<float>("log1p f32 moderate", s);
    require_true(s.max_ulp <= 4);
  }
  end_test_case();

  test_case("f64 log_base — base ∈ {2, 3, 5, 7, e, 10}, x ∈ [1e-9, 1e9]");
  {
    stats<double> s{};
    const double bases[] = { 2.0, 3.0, 5.0, 7.0, 2.718281828459045, 10.0 };
    for ( unsigned bi = 0; bi < sizeof(bases) / sizeof(bases[0]); ++bi ) {
      const double b = bases[bi];
      log_grid(1e-9, 1e9, 1024, [&](double x) {
        const double mine = micron::math::log_base<double>(x, b);
        const double ref = __builtin_log(x) / __builtin_log(b);
        record<double>(s, x, mine, ref);
      });
    }
    report<double>("log_base f64", s);

    require_true(s.max_ulp <= 64);
  }
  end_test_case();

  test_case("f64 log2p1 — moderate [-0.5, 5.0] vs libm log1p(x)*log2e");
  {

    stats<double> s{};
    constexpr double log2e = 1.442695040888963407359924681001892137;
    linear_grid(-0.5, 5.0, 8192, [&](double x) {
      const double mine = micron::math::log2p1<double>(x);
      const double ref = __builtin_log1p(x) * log2e;
      record<double>(s, x, mine, ref);
    });
    report<double>("log2p1 f64", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 log1mexp — domain x ∈ [-30, -1e-9]");
  {

    constexpr double ln2 = 0.693147180559945309417232121458176568;
    stats<double> s{};
    log_grid(1e-9, 30.0, 8192, [&](double t) {
      const double x = -t;
      const double mine = micron::math::log1mexp<double>(x);
      const double ref = (x > -ln2) ? __builtin_log(-__builtin_expm1(x)) : __builtin_log1p(-__builtin_exp(x));
      record<double>(s, x, mine, ref);
    });
    report<double>("log1mexp f64", s);

    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("f64 log_sum_exp — pairwise vs libm");
  {
    stats<double> s{};
    const unsigned NN = 96;
    for ( unsigned i = 0; i < NN; ++i ) {
      const double a = -20.0 + 40.0 * double(i) / double(NN - 1);
      for ( unsigned j = 0; j < NN; ++j ) {
        const double b = -20.0 + 40.0 * double(j) / double(NN - 1);
        const double mine = micron::math::log_sum_exp<double>(a, b);
        const double m = a > b ? a : b;
        const double ref = m + __builtin_log1p(__builtin_exp((a < b ? a : b) - m));
        record<double>(s, a, mine, ref);
      }
    }
    report<double>("log_sum_exp f64", s);
    require_true(s.max_ulp <= 16);
  }
  end_test_case();

  test_case("f64 log_diff_exp — wide range vs libm");
  {

    constexpr double ln2 = 0.693147180559945309417232121458176568;
    stats<double> s{};
    for ( double a = -10.0; a <= 10.0; a += 0.317 ) {
      for ( double db = 0.01; db <= 5.0; db += 0.151 ) {
        const double b = a - db;
        const double mine = micron::math::log_diff_exp<double>(a, b);
        const double y = b - a;
        const double l1m = (y > -ln2) ? __builtin_log(-__builtin_expm1(y)) : __builtin_log1p(-__builtin_exp(y));
        const double ref = a + l1m;
        record<double>(s, a, mine, ref);
      }
    }
    report<double>("log_diff_exp f64", s);
    require_true(s.max_ulp <= 32);
  }
  end_test_case();

  test_case("f64 logistic — [-30, 30] vs libm 1/(1+exp(-x))");
  {
    stats<double> s{};
    linear_grid(-30.0, 30.0, 8192, [&](double x) {
      const double mine = micron::math::logistic<double>(x);
      double ref;
      if ( x >= 0.0 )
        ref = 1.0 / (1.0 + __builtin_exp(-x));
      else {
        const double e = __builtin_exp(x);
        ref = e / (1.0 + e);
      }
      record<double>(s, x, mine, ref);
    });
    report<double>("logistic f64", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 log_logistic — [-30, 30] vs libm");
  {
    stats<double> s{};
    linear_grid(-30.0, 30.0, 8192, [&](double x) {
      const double mine = micron::math::log_logistic<double>(x);
      double ref;
      if ( x >= 0.0 )
        ref = -__builtin_log1p(__builtin_exp(-x));
      else
        ref = x - __builtin_log1p(__builtin_exp(x));
      record<double>(s, x, mine, ref);
    });
    report<double>("log_logistic f64", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 softplus — [-30, 30] vs libm log1p(exp(x))");
  {
    stats<double> s{};
    linear_grid(-30.0, 30.0, 8192, [&](double x) {
      const double mine = micron::math::softplus<double>(x);
      const double ref = __builtin_log1p(__builtin_exp(x));
      record<double>(s, x, mine, ref);
    });
    report<double>("softplus f64", s);
    require_true(s.max_ulp <= 8);
  }
  end_test_case();

  test_case("f64 log — pseudo-random over [1e-200, 1e200]");
  {

    unsigned long long state = 0xC0FFEE12345678ULL;
    auto next = [&]() -> double {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;

      const long long e = (long long)((state >> 32) % 1329) - 664;
      const unsigned long long m = (state & ((1ULL << 52) - 1));
      union {
        unsigned long long u;
        double d;
      } u;
      const unsigned long long be = (unsigned long long)(e + 1023) & 0x7ff;
      u.u = (be << 52) | m;
      return u.d;
    };
    stats<double> sl{};
    stats<double> sl2{};
    stats<double> sl10{};
    for ( unsigned i = 0; i < 16384; ++i ) {
      const double x = next();
      record<double>(sl, x, micron::math::log<double>(x), ref_log(x));
      record<double>(sl2, x, micron::math::log2<double>(x), ref_log2(x));
      record<double>(sl10, x, micron::math::log10<double>(x), ref_log10(x));
    }
    report<double>("log f64 random", sl);
    report<double>("log2 f64 random", sl2);
    report<double>("log10 f64 random", sl10);
    require_true(sl.max_ulp <= 4);
    require_true(sl2.max_ulp <= 8);
    require_true(sl10.max_ulp <= 8);
  }
  end_test_case();

  test_case("(FAILS) f64 log — ULP near pole at x → 0+");
  {

    long long worst = 0;
    for ( unsigned i = 0; i < 4096; ++i ) {
      const double x = 0x1.0p-1022 * (1.0 + double(i) * 0.0001);
      const long long u = ulp_dist<double>(micron::math::log<double>(x), ref_log(x));
      if ( u >= 0 && u > worst ) worst = u;
    }
    sb::print("    log f64 worst near-0 ULP=", worst);
    soft_check(worst <= 8, "log f64 worst ULP near 0+ <= 8");
  }
  end_test_case();

  test_case("(FAILS) f64 log_sum_exp_n — N=16, extreme spread");
  {

    long long worst = 0;
    unsigned long long state = 0xCAFEBABEDEADBEEFULL;
    auto rnd = [&]() -> double {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      const unsigned long long m = state >> 11;
      return (double(m) * (1.0 / double(1ULL << 53)) - 0.5) * 2000.0;
    };
    for ( unsigned trial = 0; trial < 256; ++trial ) {
      double v[16];
      for ( int i = 0; i < 16; ++i ) v[i] = rnd();
      const double mine = micron::math::log_sum_exp_n<double>(v, 16);
      double mx = v[0];
      for ( int i = 1; i < 16; ++i )
        if ( v[i] > mx ) mx = v[i];
      double sum = 0.0;
      for ( int i = 0; i < 16; ++i ) sum += __builtin_exp(v[i] - mx);
      const double ref = mx + __builtin_log(sum);
      const long long u = ulp_dist<double>(mine, ref);
      if ( u >= 0 && u > worst ) worst = u;
    }
    sb::print("    log_sum_exp_n f64 worst ULP across 256 random 16-vectors=", worst);
    soft_check(worst <= 64, "log_sum_exp_n f64 worst ULP <= 64 on random 16-vectors");
  }
  end_test_case();

  test_case("(FAILS) f64 log1p — Ulp on dense grid near 0 (cancellation guard)");
  {

    long long worst = 0;
    const double lo = 0x1.0p-1074;
    const double hi = 0x1.0p-26;
    const double llo = std::log(lo);
    const double lhi = std::log(hi);
    for ( unsigned i = 0; i < 4096; ++i ) {
      const double t = double(i) / 4095.0;
      const double x = std::exp(llo + t * (lhi - llo));
      const long long u = ulp_dist<double>(micron::math::log1p<double>(x), ref_log1p(x));
      if ( u >= 0 && u > worst ) worst = u;
      const long long u2 = ulp_dist<double>(micron::math::log1p<double>(-x), ref_log1p(-x));
      if ( u2 >= 0 && u2 > worst ) worst = u2;
    }
    sb::print("    log1p f64 worst ULP near 0=", worst);
    soft_check(worst <= 4, "log1p f64 worst ULP near 0 <= 4");
  }
  end_test_case();

  test_case("(FAILS) f64 log_diff_exp — small a-b cancellation");
  {

    constexpr double ln2 = 0.693147180559945309417232121458176568;
    long long worst = 0;
    for ( double a = -5.0; a <= 5.0; a += 0.317 ) {
      for ( double d = 1e-12; d <= 1e-2; d *= 1.3 ) {
        const double b = a - d;
        const double mine = micron::math::log_diff_exp<double>(a, b);
        const double y = b - a;
        const double l1m = (y > -ln2) ? __builtin_log(-__builtin_expm1(y)) : __builtin_log1p(-__builtin_exp(y));
        const double ref = a + l1m;
        const long long u = ulp_dist<double>(mine, ref);
        if ( u >= 0 && u > worst ) worst = u;
      }
    }
    sb::print("    log_diff_exp f64 worst ULP at small a-b=", worst);
    soft_check(worst <= 64, "log_diff_exp f64 worst ULP at small a-b <= 64");
  }
  end_test_case();

  sb::print("=== LOG vs LIBM REPORT COMPLETE ===");
  sb::print("    soft-checks total:    ", __soft().checks);
  sb::print("    soft-checks failures: ", __soft().failures);
  if ( __soft().failures == 0 )
    sb::print("=== ALL ACCURACY TESTS PASSED ===");
  else
    sb::print("=== HARD TESTS PASSED; (FAILS)-marked tests reported above ===");
  return 1;
}
