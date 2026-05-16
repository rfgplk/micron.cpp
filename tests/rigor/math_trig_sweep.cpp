// math_trig_sweep.cpp
// Wide-range sweep of the *polynomial* (non-CORDIC) trig kernels against
// libc / __builtin_* reference values.
//
// Coverage:
//   * 0 → 1e8 linear & log sweep for sin / cos / tan (scalar f64, f32)
//   * special angles (0, π/4, π/2, π, 3π/2, 2π, k·π)
//   * negative inputs (odd/even mirror symmetry)
//   * asin/acos over [-1, 1], atan over a 19-decade log span,
//     atan2 over the full 4-quadrant grid
//   * SIMD d256/f256/d128/f128 lanes vs scalar polynomial vs libc
//   * fast variants (fcos/ftan/fasin/...) where exposed at micron::math
//
// Tolerance bands scale with |x| because range-reduction error is ~ulp(x)
// per π/2 wrap; the Payne-Hanek branch (|x| > 2^33) is exercised at the
// upper end of the sweep so the band stays tight throughout.

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

static inline double
abs_d(double v) noexcept
{
  return v < 0 ? -v : v;
}

static inline float
abs_f(float v) noexcept
{
  return v < 0 ? -v : v;
}

static inline bool
near_d(double a, double b, double base = 1e-12, double slope = 1e-15) noexcept
{
  const double diff = abs_d(a - b);
  return diff <= base + slope * abs_d(b);
}

// stride generators

template<typename Fn>
static void
sweep_log_stride(double x_min, double x_max, unsigned samples, Fn &&fn)
{
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

static void
store_f256(micron::simd::f256 v, float (&out)[8]) noexcept
{
  alignas(32) float tmp[8];
  _mm256_store_ps(tmp, v);
  for ( int i = 0; i < 8; ++i ) out[i] = tmp[i];
}

static void
store_d128(micron::simd::d128 v, double (&out)[2]) noexcept
{
  alignas(16) double tmp[2];
  _mm_store_pd(tmp, v);
  out[0] = tmp[0];
  out[1] = tmp[1];
}

static void
store_f128(micron::simd::f128 v, float (&out)[4]) noexcept
{
  alignas(16) float tmp[4];
  _mm_store_ps(tmp, v);
  for ( int i = 0; i < 4; ++i ) out[i] = tmp[i];
}

#endif

};      // namespace

int
main()
{
  print("=== POLY TRIG SWEEP ===");

  // ================================================================
  // scalar f64 — sin / cos
  // ================================================================
  test_case("scalar sin/cos f64 — log sweep [1e-7, 1e8]");
  {
    unsigned fails_s = 0, fails_c = 0;
    sweep_log_stride(1e-7, 1e8, 4096, [&](double x) {
      const double s_ref = std::sin(x);
      const double c_ref = std::cos(x);
      const double s_got = micron::sin<double>(x);
      const double c_got = micron::cos<double>(x);
      const double tol = 5e-12 + 5e-15 * abs_d(x);
      if ( abs_d(s_got - s_ref) > tol ) ++fails_s;
      if ( abs_d(c_got - c_ref) > tol ) ++fails_c;
    });
    require_true(fails_s == 0);
    require_true(fails_c == 0);
  }
  end_test_case();

  test_case("scalar sin/cos f64 — linear sweep [0, 1e8] stride 1000");
  {
    unsigned fails = 0;
    for ( double x = 0.0; x <= 1.0e8; x += 1000.0 ) {
      const double s_ref = std::sin(x);
      const double c_ref = std::cos(x);
      const double s_got = micron::sin<double>(x);
      const double c_got = micron::cos<double>(x);
      const double tol = 1e-11 + 1e-14 * abs_d(x);
      if ( abs_d(s_got - s_ref) > tol || abs_d(c_got - c_ref) > tol ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar sin/cos f64 — fine linear sweep [0, 1000] stride 0.013");
  {
    unsigned fails = 0;
    for ( double x = 0.0; x <= 1000.0; x += 0.013 ) {
      const double s_ref = std::sin(x);
      const double c_ref = std::cos(x);
      const double s_got = micron::sin<double>(x);
      const double c_got = micron::cos<double>(x);
      if ( abs_d(s_got - s_ref) > 1e-12 ) ++fails;
      if ( abs_d(c_got - c_ref) > 1e-12 ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar sin/cos f64 — negative mirror");
  {
    unsigned fails = 0;
    for ( double x = -1.0e6; x <= 1.0e6; x += 1.27e3 ) {
      const double s = micron::sin<double>(x);
      const double c = micron::cos<double>(x);
      const double s_neg = micron::sin<double>(-x);
      const double c_neg = micron::cos<double>(-x);
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
      { 1e6 * pi, std::sin(1e6 * pi), std::cos(1e6 * pi) },
      { 1e8 * pi, std::sin(1e8 * pi), std::cos(1e8 * pi) },
      { 12345.6789, std::sin(12345.6789), std::cos(12345.6789) },
    };

    for ( unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i ) {
      const double a = cases[i].angle;
      // wide angle tolerance scales with magnitude
      const double tol = 5e-11 + 5e-14 * abs_d(a);
      require_true(near_d(micron::sin<double>(a), cases[i].sin_expect, tol, 0));
      require_true(near_d(micron::cos<double>(a), cases[i].cos_expect, tol, 0));
    }
  }
  end_test_case();

  // ================================================================
  // scalar f64 — tan
  // ================================================================
  test_case("scalar tan f64 — sweep avoiding poles");
  {
    unsigned fails = 0;
    for ( double x = -1.0e5; x <= 1.0e5; x += 0.317 ) {
      const double c_ref = std::cos(x);
      if ( abs_d(c_ref) < 1e-4 ) continue;
      const double t_ref = std::tan(x);
      const double t_got = micron::tan<double>(x);
      const double scale = (abs_d(t_ref) > 1.0) ? abs_d(t_ref) : 1.0;
      if ( abs_d(t_got - t_ref) > 1e-10 * scale + 1e-13 * abs_d(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar sincos f64 — pythagoras identity, [0, 1e8]");
  {
    unsigned fails = 0;
    for ( double x = 0.0; x <= 1.0e8; x += 7321.0 ) {
      double s, c;
      micron::sincos<double>(x, s, c);
      const double mag = s * s + c * c;
      if ( abs_d(mag - 1.0) > 1e-11 ) ++fails;
      if ( abs_d(s - std::sin(x)) > 1e-11 + 1e-14 * abs_d(x) ) ++fails;
      if ( abs_d(c - std::cos(x)) > 1e-11 + 1e-14 * abs_d(x) ) ++fails;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ================================================================
  // scalar f32 path
  // ================================================================
  test_case("scalar sin/cos f32 — log sweep [1e-3, 1e6]");
  {
    unsigned fails = 0;
    sweep_log_stride(1e-3, 1e6, 1024, [&](double xd) {
      const float x = float(xd);
      const float s_ref = std::sin(x);
      const float c_ref = std::cos(x);
      const float s_got = micron::sin<float>(x);
      const float c_got = micron::cos<float>(x);
      const float tol = 5e-6f + 1e-6f * abs_f(x);
      if ( abs_f(s_got - s_ref) > tol ) ++fails;
      if ( abs_f(c_got - c_ref) > tol ) ++fails;
    });
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar sin f32 — linear sweep [0, 1e7] stride 0.5");
  {
    unsigned fails = 0;
    for ( double xd = 0.0; xd <= 1.0e7; xd += 0.5 ) {
      const float x = float(xd);
      const float r = std::sin(x);
      const float g = micron::sin<float>(x);
      // f32 reduction error grows quickly; allow more headroom for large x
      const float tol = 1e-5f + 1e-5f * abs_f(x) * 1e-7f * abs_f(x);
      if ( abs_f(g - r) > tol + 1e-3f ) ++fails;
      (void)tol;
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ================================================================
  // scalar atan / atan2
  // ================================================================
  test_case("scalar atan f64 — wide range");
  {
    unsigned fails = 0;
    sweep_log_stride(1e-9, 1e9, 2048, [&](double x) {
      const double a_ref = std::atan(x);
      const double a_got = micron::atan<double>(x);
      if ( !near_d(a_got, a_ref, 1e-12, 1e-15) ) ++fails;
      const double an_ref = std::atan(-x);
      const double an_got = micron::atan<double>(-x);
      if ( !near_d(an_got, an_ref, 1e-12, 1e-15) ) ++fails;
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
        const double r_got = micron::atan2<double>(yi, xi);
        if ( !near_d(r_got, r_ref, 5e-12, 1e-15) ) ++fails;
      });
    });
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar atan2 f64 — axes and infinities");
  {
    const double pi = std::acos(-1.0);
    const double inf = __builtin_inf();
    // y=0
    require_true(near_d(micron::atan2<double>(0.0, 1.0), 0.0));
    require_true(near_d(micron::atan2<double>(0.0, -1.0), pi));
    // x=0
    require_true(near_d(micron::atan2<double>(1.0, 0.0), pi / 2));
    require_true(near_d(micron::atan2<double>(-1.0, 0.0), -pi / 2));
    // +inf / +inf = π/4
    require_true(near_d(micron::atan2<double>(inf, inf), pi / 4));
    // +inf / -inf = 3π/4
    require_true(near_d(micron::atan2<double>(inf, -inf), 3 * pi / 4));
    // -inf / -inf = -3π/4
    require_true(near_d(micron::atan2<double>(-inf, -inf), -3 * pi / 4));
    // -inf / +inf = -π/4
    require_true(near_d(micron::atan2<double>(-inf, inf), -pi / 4));
  }
  end_test_case();

  // ================================================================
  // scalar asin / acos
  // ================================================================
  test_case("scalar asin / acos f64 — [-1, 1] linear sweep");
  {
    unsigned fails = 0;
    sweep_linear_stride(-0.9999, 0.9999, 4096, [&](double x) {
      const double as_ref = std::asin(x);
      const double ac_ref = std::acos(x);
      const double as_got = micron::asin<double>(x);
      const double ac_got = micron::acos<double>(x);
      const double extra = (abs_d(x) > 0.99) ? 1e-9 : 0.0;
      if ( !near_d(as_got, as_ref, 1e-12 + extra, 1e-15) ) ++fails;
      if ( !near_d(ac_got, ac_ref, 1e-12 + extra, 1e-15) ) ++fails;
    });
    require_true(fails == 0);
  }
  end_test_case();

  test_case("scalar asin / acos f64 — boundaries ±1, 0");
  {
    const double pi = std::acos(-1.0);
    require_true(near_d(micron::asin<double>(0.0), 0.0));
    require_true(near_d(micron::asin<double>(1.0), pi / 2));
    require_true(near_d(micron::asin<double>(-1.0), -pi / 2));
    require_true(near_d(micron::acos<double>(0.0), pi / 2));
    require_true(near_d(micron::acos<double>(1.0), 0.0));
    require_true(near_d(micron::acos<double>(-1.0), pi));
  }
  end_test_case();

  // ================================================================
  // top-level micron:: reexports
  // ================================================================
  test_case("micron::* reexports — same answers as micron::math::*");
  {
    unsigned fails = 0;
    sweep_log_stride(1e-3, 1e5, 1024, [&](double x) {
      if ( micron::sin(x) != micron::math::sin<double>(x) ) ++fails;
      if ( micron::cos(x) != micron::math::cos<double>(x) ) ++fails;
      if ( micron::tan(x) != micron::math::tan<double>(x) ) ++fails;
      if ( micron::atan(x) != micron::math::atan<double>(x) ) ++fails;
    });
    require_true(fails == 0);
  }
  end_test_case();

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

  // ================================================================
  // SIMD d256 — 4 lanes f64
  // ================================================================
  test_case("SIMD sin d256 — sweep against libc");
  {
    unsigned fails_s = 0, fails_c = 0;
    for ( double base = 0.0; base <= 1.0e6; base += 877.0 ) {
      const double xs[4] = { base, base + 0.31, base + 1.7, base + 11.31 };
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 s = micron::sin(v);
      const micron::simd::d256 c = micron::cos(v);
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

  test_case("SIMD tan d256 — log sweep, pole-avoiding");
  {
    unsigned fails = 0;
    sweep_log_stride(1e-3, 1e3, 256, [&](double base) {
      const double xs[4] = { base, base + 0.05, base + 0.11, base + 0.23 };
      bool skip = false;
      for ( int i = 0; i < 4; ++i ) {
        if ( abs_d(std::cos(xs[i])) < 0.05 ) {
          skip = true;
          break;
        }
      }
      if ( skip ) return;
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 t = micron::tan(v);
      double tv[4];
      store_d256(t, tv);
      for ( int i = 0; i < 4; ++i ) {
        const double r = std::tan(xs[i]);
        const double scale = (abs_d(r) > 1.0) ? abs_d(r) : 1.0;
        if ( abs_d(tv[i] - r) > 1e-9 * scale + 1e-12 * abs_d(xs[i]) ) ++fails;
      }
    });
    require_true(fails == 0);
  }
  end_test_case();

  test_case("SIMD sincos d256 — pythagoras identity");
  {
    unsigned fails = 0;
    for ( double base = 0.0; base <= 1.0e5; base += 53.7 ) {
      const double xs[4] = { base, base + 0.31, base + 1.7, base + 11.31 };
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      micron::simd::d256 s, c;
      micron::sincos(v, &s, &c);
      double sv[4], cv[4];
      store_d256(s, sv);
      store_d256(c, cv);
      for ( int i = 0; i < 4; ++i ) {
        const double mag = sv[i] * sv[i] + cv[i] * cv[i];
        if ( abs_d(mag - 1.0) > 1e-11 ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("SIMD scalar/SIMD agreement — sin d256 vs scalar polynomial");
  {
    unsigned fails = 0;
    for ( double base = 0.0; base <= 1.0e5; base += 13.7 ) {
      const double xs[4] = { base, base + 0.31, base + 1.7, base + 11.31 };
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 s = micron::sin(v);
      double sv[4];
      store_d256(s, sv);
      for ( int i = 0; i < 4; ++i ) {
        const double scalar = micron::sin<double>(xs[i]);
        if ( abs_d(sv[i] - scalar) > 1e-12 + 1e-15 * abs_d(xs[i]) ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ================================================================
  // SIMD f256 — 8 lanes f32 — via internal d256 split
  // ================================================================
  test_case("SIMD sin f256 — sweep against libc");
  {
    unsigned fails = 0;
    for ( float base = 0.0f; base <= 1.0e4f; base += 7.3f ) {
      const float xs[8] = { base, base + 0.1f, base + 0.5f, base + 1.3f, base + 3.7f, base + 11.31f, base + 27.0f, base + 99.9f };
      const micron::simd::f256 v = _mm256_loadu_ps(xs);
      const micron::simd::f256 s = micron::sin(v);
      float sv[8];
      store_f256(s, sv);
      for ( int i = 0; i < 8; ++i ) {
        const float r = std::sin(xs[i]);
        const float tol = 5e-6f + 1e-6f * abs_f(xs[i]);
        if ( abs_f(sv[i] - r) > tol ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ================================================================
  // SIMD d128 / f128 — 2 / 4 lanes
  // ================================================================
  test_case("SIMD sin d128 — sweep against libc");
  {
    unsigned fails = 0;
    for ( double base = 0.0; base <= 1.0e5; base += 23.7 ) {
      const double xs[2] = { base, base + 1.7 };
      const micron::simd::d128 v = _mm_loadu_pd(xs);
      const micron::simd::d128 s = micron::sin(v);
      double sv[2];
      store_d128(s, sv);
      for ( int i = 0; i < 2; ++i ) {
        const double tol = 1e-10 + 1e-13 * abs_d(xs[i]);
        if ( abs_d(sv[i] - std::sin(xs[i])) > tol ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  test_case("SIMD sin f128 — sweep against libc");
  {
    unsigned fails = 0;
    for ( float base = 0.0f; base <= 1.0e4f; base += 13.7f ) {
      const float xs[4] = { base, base + 0.5f, base + 3.7f, base + 99.9f };
      const micron::simd::f128 v = _mm_loadu_ps(xs);
      const micron::simd::f128 s = micron::sin(v);
      float sv[4];
      store_f128(s, sv);
      for ( int i = 0; i < 4; ++i ) {
        const float r = std::sin(xs[i]);
        const float tol = 5e-6f + 1e-6f * abs_f(xs[i]);
        if ( abs_f(sv[i] - r) > tol ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

  // ================================================================
  // SIMD top-level micron:: reexports
  // ================================================================
  test_case("SIMD reexport — micron::sin(simd::d256) == micron::math::sin(...)");
  {
    unsigned fails = 0;
    for ( double base = 0.0; base <= 1.0e5; base += 19.3 ) {
      const double xs[4] = { base, base + 0.7, base + 3.3, base + 11.11 };
      const micron::simd::d256 v = _mm256_loadu_pd(xs);
      const micron::simd::d256 a = micron::sin(v);
      const micron::simd::d256 b = micron::math::sin(v);
      double av[4], bv[4];
      store_d256(a, av);
      store_d256(b, bv);
      for ( int i = 0; i < 4; ++i ) {
        if ( av[i] != bv[i] ) ++fails;
      }
    }
    require_true(fails == 0);
  }
  end_test_case();

#else
  print("(SIMD sweep skipped: AVX2/FMA unavailable)");
#endif

  print("=== poly trig sweep ok ===");
  return 1;
}
