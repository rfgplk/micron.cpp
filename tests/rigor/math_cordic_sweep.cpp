// math_cordic_sweep.cpp
// Wide-range sweep of the CORDIC trig kernels (scalar + SIMD)
// against libc / __builtin_* reference values.
//
// Coverage:
//   * 0 → 1e8 linear sweep for sin / cos / tan
//   * special angles (0, π/4, π/2, π, 3π/2, 2π, k·π)
//   * negative inputs (mirror symmetry)
//   * asin/acos/atan/atan2 over [-1, 1] resp. [-1e8, 1e8]
//   * SIMD lanes vs scalar CORDIC vs libc
//
// Tolerance bands scale with |x| because range reduction error is ~ulp(x)
// per π/2 wrap.

#include "../../src/math.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <cmath>

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tolerance model
//
// CORDIC kernel error is ~2^-62. Cody-Waite reduction error grows with
// |x| (roughly ulp(x) once x exceeds 2^20). Payne-Hanek kicks in at 2^33
// and recovers full precision. We model the expected error envelope as:
//
//   |err| <= base + slope * |x|
//
// for x in the range where the iterated reducer is active, and tighter
// once Payne-Hanek takes over.

static inline double
abs_d(double v) noexcept
{
  return v < 0 ? -v : v;
}

static inline bool
near_d(double a, double b, double base = 1e-12, double slope = 1e-15) noexcept
{
  const double diff = abs_d(a - b);
  const double tol = base + slope * abs_d(b);
  return diff <= tol;
}

static inline bool
near_d_relative(double a, double b, double rel = 1e-10, double abs_floor = 1e-12) noexcept
{
  const double diff = abs_d(a - b);
  if ( diff <= abs_floor ) return true;
  return diff <= rel * abs_d(b);
}

// stride generator: ~uniform on a log scale so we hit small / mid / large x
template<typename Fn>
static void
sweep_log_stride(double x_min, double x_max, unsigned samples, Fn &&fn)
{
  // log-uniform sample using integer-bias steps
  const double lmin = std::log(x_min);
  const double lmax = std::log(x_max);
  for ( unsigned i = 0; i < samples; ++i ) {
    const double t = double(i) / double(samples - 1);
    const double x = std::exp(lmin + t * (lmax - lmin));
    fn(x);
  }
}

template<typename Fn>
static void
sweep_linear_stride(double x_min, double x_max, unsigned samples, Fn &&fn)
{
  for ( unsigned i = 0; i < samples; ++i ) {
    const double t = double(i) / double(samples - 1);
    const double x = x_min + t * (x_max - x_min);
    fn(x);
  }
}

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

static void
store_d256(micron::simd::d256 v, double (&out)[4]) noexcept
{
  alignas(32) double tmp[4];
  _mm256_store_pd(tmp, v);
  for ( int i = 0; i < 4; ++i ) out[i] = tmp[i];
}

#endif

};      // namespace

int
main()
{
  print("=== CORDIC SWEEP ===");

  // ================================================================
  // sin / cos over [0, 1e8] — scalar CORDIC vs libc
  // ================================================================
  test_case("scalar sin/cos f64 — log sweep [1e-7, 1e8]");
  {
    unsigned fails_sin = 0, fails_cos = 0, total = 0;
    sweep_log_stride(1e-7, 1e8, 4096, [&](double x) {
      const double s_ref = std::sin(x);
      const double c_ref = std::cos(x);
      const double s_got = micron::sin_cordic<double>(x);
      const double c_got = micron::cos_cordic<double>(x);
      // tolerance: kernel ~2^-62, Payne-Hanek picks up beyond 2^33,
      // iter-cody-waite covers 2^20..2^33. Add a |x|-scaled slope.
      const double tol = 5e-12 + 5e-15 * abs_d(x);
      if ( abs_d(s_got - s_ref) > tol ) ++fails_sin;
      if ( abs_d(c_got - c_ref) > tol ) ++fails_cos;
      ++total;
    });
    require_true(fails_sin == 0);
    require_true(fails_cos == 0);
  }
  end_test_case();

  test_case("scalar sin/cos f64 — linear sweep [0, 1e8] stride 1000");
  {
    unsigned fails = 0;
    for ( double x = 0.0; x <= 1.0e8; x += 1000.0 ) {
      const double s_ref = std::sin(x);
      const double c_ref = std::cos(x);
      const double s_got = micron::sin_cordic<double>(x);
      const double c_got = micron::cos_cordic<double>(x);
      const double tol = 1e-11 + 1e-14 * abs_d(x);
      if ( abs_d(s_got - s_ref) > tol || abs_d(c_got - c_ref) > tol ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar sin/cos f64 — negative mirror");
  {
    unsigned fails = 0;
    for ( double x = -1.0e6; x <= 1.0e6; x += 1.27e3 ) {
      const double s = micron::sin_cordic<double>(x);
      const double c = micron::cos_cordic<double>(x);
      // sin is odd, cos is even
      const double s_neg = micron::sin_cordic<double>(-x);
      const double c_neg = micron::cos_cordic<double>(-x);
      if ( abs_d(s + s_neg) > 1e-12 + 1e-14 * abs_d(x) ) ++fails;
      if ( abs_d(c - c_neg) > 1e-12 + 1e-14 * abs_d(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar sin/cos f64 — special angles");
  {
    const double pi = std::acos(-1.0);

    struct {
      double angle;
      double sin_expect;
      double cos_expect;
    } cases[] = {
      { 0.0, 0.0, 1.0 },
      { pi / 6, 0.5, std::cos(pi / 6) },
      { pi / 4, std::sin(pi / 4), std::cos(pi / 4) },
      { pi / 3, std::sin(pi / 3), 0.5 },
      { pi / 2, 1.0, std::cos(pi / 2) },
      { 2 * pi / 3, std::sin(2 * pi / 3), -0.5 },
      { 3 * pi / 4, std::sin(3 * pi / 4), -std::sin(pi / 4) },
      { pi, std::sin(pi), -1.0 },
      { 5 * pi / 4, -std::sin(pi / 4), -std::cos(pi / 4) },
      { 3 * pi / 2, -1.0, std::cos(3 * pi / 2) },
      { 7 * pi / 4, -std::sin(pi / 4), std::cos(pi / 4) },
      { 2 * pi, std::sin(2 * pi), 1.0 },
      { 100 * pi, std::sin(100 * pi), std::cos(100 * pi) },
      { 1000 * pi, std::sin(1000 * pi), std::cos(1000 * pi) },
      { 12345.6789, std::sin(12345.6789), std::cos(12345.6789) },
    };

    for ( unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i ) {
      const double a = cases[i].angle;
      require_true(near_d(micron::sin_cordic<double>(a), cases[i].sin_expect, 1e-11, 1e-14 * abs_d(a)));
      require_true(near_d(micron::cos_cordic<double>(a), cases[i].cos_expect, 1e-11, 1e-14 * abs_d(a)));
    }
  }
  end_test_case();

  test_case("scalar tan f64 — sweep avoiding poles");
  {
    unsigned fails = 0;
    const double pi = std::acos(-1.0);
    for ( double x = -1.0e5; x <= 1.0e5; x += 0.317 ) {
      // skip poles (where cos≈0); tan blows up so relative tolerance helps
      const double c_ref = std::cos(x);
      if ( abs_d(c_ref) < 1e-4 ) continue;
      const double t_ref = std::tan(x);
      const double t_got = micron::tan_cordic<double>(x);
      // tan is unbounded; use relative tolerance once |t| > 1
      const double scale = (abs_d(t_ref) > 1.0) ? abs_d(t_ref) : 1.0;
      if ( abs_d(t_got - t_ref) > 1e-10 * scale + 1e-13 * abs_d(x) ) ++fails;
      (void)pi;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar sincos f64 — pythagoras identity, [0, 1e8]");
  {
    unsigned fails = 0;
    for ( double x = 0.0; x <= 1.0e8; x += 7321.0 ) {
      double s, c;
      micron::sincos_cordic<double>(x, s, c);
      const double mag = s * s + c * c;
      if ( abs_d(mag - 1.0) > 1e-11 ) ++fails;
      // also check vs libc
      if ( abs_d(s - std::sin(x)) > 1e-11 + 1e-14 * abs_d(x) ) ++fails;
      if ( abs_d(c - std::cos(x)) > 1e-11 + 1e-14 * abs_d(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ================================================================
  // f32 path
  // ================================================================
  test_case("scalar sin/cos f32 — log sweep [1e-3, 1e6]");
  {
    unsigned fails = 0;
    sweep_log_stride(1e-3, 1e6, 1024, [&](double xd) {
      const float x = float(xd);
      const float s_ref = std::sin(x);
      const float c_ref = std::cos(x);
      const float s_got = micron::sin_cordic<float>(x);
      const float c_got = micron::cos_cordic<float>(x);
      const float tol = 5e-5f + 1e-6f * std::fabs(x);
      if ( std::fabs(s_got - s_ref) > tol ) ++fails;
      if ( std::fabs(c_got - c_ref) > tol ) ++fails;
    });
    require_true(fails == 0);
  }
  end_test_case();

  // ================================================================
  // atan / atan2 / asin / acos
  // ================================================================
  test_case("scalar atan f64 — wide range");
  {
    unsigned fails = 0;
    sweep_log_stride(1e-9, 1e9, 2048, [&](double x) {
      const double a_ref = std::atan(x);
      const double a_got = micron::atan_cordic<double>(x);
      if ( !near_d(a_got, a_ref, 1e-11, 1e-13) ) ++fails;
      const double an_ref = std::atan(-x);
      const double an_got = micron::atan_cordic<double>(-x);
      if ( !near_d(an_got, an_ref, 1e-11, 1e-13) ) ++fails;
    });
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar atan2 f64 — quadrant sweep");
  {
    unsigned fails = 0;
    sweep_linear_stride(-100.0, 100.0, 64, [&](double yi) {
      sweep_linear_stride(-100.0, 100.0, 64, [&](double xi) {
        if ( xi == 0 && yi == 0 ) return;
        const double r_ref = std::atan2(yi, xi);
        const double r_got = micron::atan2_cordic<double>(yi, xi);
        if ( !near_d(r_got, r_ref, 5e-11, 1e-13) ) ++fails;
      });
    });
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar asin / acos f64 — [-1, 1] linear sweep");
  {
    unsigned fails = 0;
    sweep_linear_stride(-0.9999, 0.9999, 4096, [&](double x) {
      const double as_ref = std::asin(x);
      const double ac_ref = std::acos(x);
      const double as_got = micron::asin_cordic<double>(x);
      const double ac_got = micron::acos_cordic<double>(x);
      // asin/acos lose a tiny bit of accuracy near |x|=1 due to sqrt
      // condition number; allow a slightly wider band there.
      const double extra = (abs_d(x) > 0.99) ? 1e-9 : 0.0;
      if ( !near_d(as_got, as_ref, 1e-11 + extra, 1e-13) ) ++fails;
      if ( !near_d(ac_got, ac_ref, 1e-11 + extra, 1e-13) ) ++fails;
    });
    require_true(fails == 0);
  }
  end_test_case();

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

  // ================================================================
  // SIMD (d256, 4 lanes f64)
  // ================================================================
  test_case("SIMD sin_cordic d256 — sweep against libc");
  {
    unsigned fails_s = 0, fails_c = 0;
    for ( double base = 0.0; base <= 1.0e6; base += 877.0 ) {
      const double xs[4] = { base, base + 0.31, base + 1.7, base + 11.31 };
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 s = micron::sin_cordic(v);
      const micron::simd::d256 c = micron::cos_cordic(v);
      double sv[4], cv[4];
      store_d256(s, sv);
      store_d256(c, cv);
      for ( int i = 0; i < 4; ++i ) {
        const double tol = 1e-10 + 1e-13 * abs_d(xs[i]);
        if ( abs_d(sv[i] - std::sin(xs[i])) > tol ) ++fails_s;
        if ( abs_d(cv[i] - std::cos(xs[i])) > tol ) ++fails_c;
      }
    }
    require_true(fails_s == 0);
    require_true(fails_c == 0);
  }
  end_test_case();

  test_case("SIMD micron::sin (polynomial) d256 vs libc");
  {
    unsigned fails = 0;
    for ( double base = 0.0; base <= 1.0e6; base += 1213.0 ) {
      const double xs[4] = { base, base + 0.13, base + 5.7, base + 99.31 };
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 s = micron::sin(v);
      double sv[4];
      store_d256(s, sv);
      for ( int i = 0; i < 4; ++i ) {
        const double tol = 1e-10 + 1e-13 * abs_d(xs[i]);
        if ( abs_d(sv[i] - std::sin(xs[i])) > tol ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("SIMD tan_cordic d256 vs libc — bounded inputs");
  {
    unsigned fails = 0;
    // Stay away from cos≈0 to keep tan finite-ish
    for ( double base = -100.0; base <= 100.0; base += 0.17 ) {
      const double xs[4] = { base, base + 0.05, base + 0.11, base + 0.23 };
      bool skip = false;
      for ( int i = 0; i < 4; ++i ) {
        if ( abs_d(std::cos(xs[i])) < 0.05 ) {
          skip = true;
          break;
        }
      }
      if ( skip ) continue;
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 t = micron::tan_cordic(v);
      double tv[4];
      store_d256(t, tv);
      for ( int i = 0; i < 4; ++i ) {
        const double r = std::tan(xs[i]);
        const double scale = (abs_d(r) > 1.0) ? abs_d(r) : 1.0;
        if ( abs_d(tv[i] - r) > 1e-9 * scale + 1e-12 * abs_d(xs[i]) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("SIMD scalar/SIMD agreement — sin_cordic d256");
  {
    unsigned fails = 0;
    for ( double base = 0.0; base <= 1.0e5; base += 13.7 ) {
      const double xs[4] = { base, base + 0.31, base + 1.7, base + 11.31 };
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 s = micron::sin_cordic(v);
      double sv[4];
      store_d256(s, sv);
      for ( int i = 0; i < 4; ++i ) {
        const double scalar = micron::sin_cordic<double>(xs[i]);
        // SIMD uses FP CORDIC, scalar uses fixed-point CORDIC; tolerate ~1ulp
        if ( abs_d(sv[i] - scalar) > 1e-12 + 1e-15 * abs_d(xs[i]) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

#else
  print("(SIMD sweep skipped: AVX2/FMA unavailable)");
#endif

  print("=== cordic sweep ok ===");
  return 1;
}
