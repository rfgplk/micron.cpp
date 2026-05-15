// math_simd.cpp
// Snowball tests for math::mk::* SIMD-packed overloads.
// Validates each lane against the scalar kernel for sqrt/exp/log/sin/cos/...

#include "../../src/math/mk.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

static bool
near(f64 a, f64 b, f64 eps = 1e-9)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps + 1e-12 * (b < 0 ? -b : b);
}

static bool
near_f(f32 a, f32 b, f32 eps = 1e-5f)
{
  f32 d = a - b;
  return (d < 0 ? -d : d) < eps + 1e-7f * (b < 0 ? -b : b);
}

template<typename F>
static void
store_d256(simd::d256 v, F (&out)[4])
{
  alignas(32) f64 tmp[4];
  _mm256_store_pd(reinterpret_cast<double *>(tmp), v);
  for ( int i = 0; i < 4; ++i ) out[i] = F(tmp[i]);
}

template<typename F>
static void
store_f256(simd::f256 v, F (&out)[8])
{
  alignas(32) f32 tmp[8];
  _mm256_store_ps(reinterpret_cast<float *>(tmp), v);
  for ( int i = 0; i < 8; ++i ) out[i] = F(tmp[i]);
}

int
main()
{
  print("=== SIMD KERNEL TESTS ===");

  test_case("sqrt — d256 / f256");
  {
    simd::d256 vd = _mm256_set_pd(16.0, 9.0, 4.0, 1.0);
    f64 out[4];
    store_d256<f64>(mk::pow_ns::sqrt<simd::d256>(vd), out);
    require_true(near(out[0], 1.0));
    require_true(near(out[1], 2.0));
    require_true(near(out[2], 3.0));
    require_true(near(out[3], 4.0));

    simd::f256 vf = _mm256_set_ps(64.0f, 49.0f, 36.0f, 25.0f, 16.0f, 9.0f, 4.0f, 1.0f);
    f32 outf[8];
    store_f256<f32>(mk::pow_ns::sqrt<simd::f256>(vf), outf);
    for ( int i = 0; i < 8; ++i ) require_true(near_f(outf[i], f32(i + 1), 1e-6f));
  }
  end_test_case();

  test_case("fabs / floor / ceil / trunc — d256");
  {
    simd::d256 v = _mm256_set_pd(-1.7, 1.5, -2.5, 3.2);
    f64 out[4];
    store_d256<f64>(mk::manip::fabs<simd::d256>(v), out);
    require_true(near(out[0], 3.2));
    require_true(near(out[1], 2.5));
    require_true(near(out[2], 1.5));
    require_true(near(out[3], 1.7));
    store_d256<f64>(mk::round_ns::floor<simd::d256>(v), out);
    require_true(near(out[0], 3.0));
    require_true(near(out[1], -3.0));
    require_true(near(out[3], -2.0));
    store_d256<f64>(mk::round_ns::ceil<simd::d256>(v), out);
    require_true(near(out[0], 4.0));
    require_true(near(out[3], -1.0));
  }
  end_test_case();

  test_case("exp — d256 / f256 vs scalar");
  {
    f64 inputs[4] = { 0.0, 0.5, 1.0, 2.0 };
    simd::d256 v = _mm256_loadu_pd(reinterpret_cast<const double *>(inputs));
    f64 out[4];
    store_d256<f64>(mk::exp_ns::exp<simd::d256>(v), out);
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(out[i], mk::exp_ns::exp<f64>(inputs[i]), 1e-9));
    }

    f32 inputs_f[8] = { -2.0f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 2.0f, 3.0f };
    simd::f256 vf = _mm256_loadu_ps(reinterpret_cast<const float *>(inputs_f));
    f32 outf[8];
    store_f256<f32>(mk::exp_ns::exp<simd::f256>(vf), outf);
    for ( int i = 0; i < 8; ++i ) {
      require_true(near_f(outf[i], mk::exp_ns::exp<f32>(inputs_f[i]), 1e-4f));
    }
  }
  end_test_case();

  test_case("log — d256 / f256 vs scalar");
  {
    f64 inputs[4] = { 0.5, 1.0, 2.0, 10.0 };
    simd::d256 v = _mm256_loadu_pd(reinterpret_cast<const double *>(inputs));
    f64 out[4];
    store_d256<f64>(mk::log_ns::log<simd::d256>(v), out);
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(out[i], mk::log_ns::log<f64>(inputs[i]), 1e-9));
    }
    store_d256<f64>(mk::log_ns::log2<simd::d256>(v), out);
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(out[i], mk::log_ns::log2<f64>(inputs[i]), 1e-9));
    }
  }
  end_test_case();

  test_case("sin / cos — d256 vs scalar");
  {
    f64 inputs[4] = { -1.5, 0.0, 0.5, 1.5 };
    simd::d256 v = _mm256_loadu_pd(reinterpret_cast<const double *>(inputs));
    f64 outs[4], outc[4];
    store_d256<f64>(mk::trig::sin<simd::d256>(v), outs);
    store_d256<f64>(mk::trig::cos<simd::d256>(v), outc);
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(outs[i], mk::trig::sin<f64>(inputs[i]), 1e-12));
      require_true(near(outc[i], mk::trig::cos<f64>(inputs[i]), 1e-12));
    }
    // Pythagorean identity per lane.
    for ( int i = 0; i < 4; ++i ) {
      require_true(near(outs[i] * outs[i] + outc[i] * outc[i], 1.0, 1e-12));
    }
  }
  end_test_case();

  test_case("rsqrt — fast f256");
  {
    simd::f256 v = _mm256_set_ps(64.0f, 49.0f, 36.0f, 25.0f, 16.0f, 9.0f, 4.0f, 1.0f);
    f32 out[8];
    store_f256<f32>(mk::pow_ns::rsqrt<simd::f256>(v), out);
    // ~22-bit accuracy after 1 NR step
    for ( int i = 0; i < 8; ++i ) {
      const f32 ref = 1.0f / mk::pow_ns::sqrt<f32>(f32(i + 1) * f32(i + 1));
      require_true(near_f(out[i], ref, 5e-5f));
    }
  }
  end_test_case();

  print("=== simd ok ===");
  return 0;
}

#else

int
main()
{
  print("=== SIMD KERNEL TESTS — skipped (no AVX2/FMA) ===");
  return 0;
}

#endif
