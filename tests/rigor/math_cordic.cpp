// math_cordic.cpp
// Snowball tests for the CORDIC shift+add trig kernel (scalar + SIMD)
// and the micron::* re-exports.

#include "../../src/math.hpp"
#include "../../src/math/mk.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

static bool
near(f64 a, f64 b, f64 eps = 1e-12)
{
  f64 d = a - b;
  if ( d < 0 ) d = -d;
  f64 mag = b < 0 ? -b : b;
  return d < eps + 1e-12 * mag;
}

static bool
near_f(f32 a, f32 b, f32 eps = 5e-6f)
{
  f32 d = a - b;
  if ( d < 0 ) d = -d;
  f32 mag = b < 0 ? -b : b;
  return d < eps + 1e-6f * mag;
}

int
main()
{
  print("=== CORDIC TESTS ===");

  test_case("scalar sin_cordic f64 / f32");
  {
    const f64 inputs[] = { 0.0, 0.25, 0.5, 0.78539816339744830962, 1.0, 1.5, -0.5, -1.5, 2.5, -3.14 };
    for ( unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i ) {
      const f64 x = inputs[i];
      require_true(near(micron::math::sin_cordic<f64>(x), micron::math::sin<f64>(x), 1e-11));
      require_true(near(micron::math::cos_cordic<f64>(x), micron::math::cos<f64>(x), 1e-11));
      const f32 xf = f32(x);
      require_true(near_f(micron::math::sin_cordic<f32>(xf), micron::math::sin<f32>(xf), 1e-5f));
      require_true(near_f(micron::math::cos_cordic<f32>(xf), micron::math::cos<f32>(xf), 1e-5f));
    }
  }
  end_test_case();

  test_case("scalar tan_cordic / sincos_cordic");
  {
    const f64 angles[] = { 0.1, 0.3, 0.7, 1.0, 1.2, -0.4, -1.1 };
    for ( unsigned i = 0; i < sizeof(angles) / sizeof(angles[0]); ++i ) {
      const f64 a = angles[i];
      require_true(near(micron::math::tan_cordic<f64>(a), micron::math::tan<f64>(a), 1e-10));
      f64 s, c;
      micron::math::sincos_cordic<f64>(a, s, c);
      require_true(near(s, micron::math::sin<f64>(a), 1e-11));
      require_true(near(c, micron::math::cos<f64>(a), 1e-11));
      // pythagoras identity
      require_true(near(s * s + c * c, 1.0, 1e-12));
    }
  }
  end_test_case();

  test_case("scalar atan_cordic / atan2_cordic / asin_cordic / acos_cordic");
  {
    const f64 inputs[] = { 0.0, 0.5, 0.75, -0.5, -0.9, 0.99 };
    for ( unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i ) {
      const f64 x = inputs[i];
      require_true(near(micron::math::atan_cordic<f64>(x), micron::math::atan<f64>(x), 5e-10));
      require_true(near(micron::math::asin_cordic<f64>(x), micron::math::asin<f64>(x), 5e-10));
      require_true(near(micron::math::acos_cordic<f64>(x), micron::math::acos<f64>(x), 5e-10));
    }
    require_true(near(micron::math::atan2_cordic<f64>(1.0, 1.0), 0.785398163397448, 1e-10));
    require_true(near(micron::math::atan2_cordic<f64>(-1.0, -1.0), -2.35619449019234, 1e-10));
    require_true(near(micron::math::atan2_cordic<f64>(1.0, 0.0), 1.5707963267948966, 1e-12));
  }
  end_test_case();

  test_case("re-exports: micron::sin / micron::cos / micron::tan");
  {
    require_true(near(micron::sin(0.5), micron::math::sin<f64>(0.5)));
    require_true(near(micron::cos(0.5), micron::math::cos<f64>(0.5)));
    require_true(near(micron::tan(0.5), micron::math::tan<f64>(0.5)));
    require_true(near(micron::sin_cordic(0.5), micron::math::sin_cordic<f64>(0.5)));
    require_true(near(micron::cos_cordic(0.5), micron::math::cos_cordic<f64>(0.5)));
    require_true(near(micron::atan2(1.0, 1.0), 0.785398163397448, 1e-12));
    require_true(near(micron::sqrt(4.0), 2.0));
    require_true(near(micron::exp(0.0), 1.0));
  }
  end_test_case();

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

  test_case("SIMD sin / cos / tan via micron::math:: (no mk::)");
  {
    const f64 inp[4] = { -1.5, 0.0, 0.5, 1.5 };
    const micron::simd::d256 v = _mm256_loadu_pd(reinterpret_cast<const double *>(inp));
    const micron::simd::d256 s = micron::math::sin(v);
    const micron::simd::d256 c = micron::math::cos(v);
    alignas(32) f64 so[4], co[4];
    _mm256_store_pd(reinterpret_cast<double *>(so), s);
    _mm256_store_pd(reinterpret_cast<double *>(co), c);
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(so[i], micron::math::sin<f64>(inp[i]), 1e-12));
      require_true(near(co[i], micron::math::cos<f64>(inp[i]), 1e-12));
    }
  }
  end_test_case();

  test_case("SIMD sin_cordic / cos_cordic via micron::math::");
  {
    const f64 inp[4] = { -1.2, 0.0, 0.3, 1.4 };
    const micron::simd::d256 v = _mm256_loadu_pd(reinterpret_cast<const double *>(inp));
    const micron::simd::d256 s = micron::math::sin_cordic(v);
    const micron::simd::d256 c = micron::math::cos_cordic(v);
    alignas(32) f64 so[4], co[4];
    _mm256_store_pd(reinterpret_cast<double *>(so), s);
    _mm256_store_pd(reinterpret_cast<double *>(co), c);
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(so[i], micron::math::sin<f64>(inp[i]), 1e-10));
      require_true(near(co[i], micron::math::cos<f64>(inp[i]), 1e-10));
      require_true(near(so[i] * so[i] + co[i] * co[i], 1.0, 1e-12));
    }
  }
  end_test_case();

  test_case("SIMD via top-level micron::sin / micron::cos");
  {
    const f64 inp[4] = { -1.5, 0.0, 0.5, 1.5 };
    const micron::simd::d256 v = _mm256_loadu_pd(reinterpret_cast<const double *>(inp));
    const micron::simd::d256 s = micron::sin(v);
    const micron::simd::d256 c = micron::cos(v);
    alignas(32) f64 so[4], co[4];
    _mm256_store_pd(reinterpret_cast<double *>(so), s);
    _mm256_store_pd(reinterpret_cast<double *>(co), c);
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(so[i], micron::math::sin<f64>(inp[i]), 1e-12));
      require_true(near(co[i], micron::math::cos<f64>(inp[i]), 1e-12));
    }
  }
  end_test_case();

#else
  print("(SIMD tests skipped: AVX2/FMA unavailable)");
#endif

  print("=== cordic ok ===");
  return 1;
}
