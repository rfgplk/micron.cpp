// test_fma.cpp
// Behavioral coverage for `micron::simd::fma::*`.

#include "../../src/simd/aliases/fma.hpp"
#include "../snowball/snowball.hpp"

namespace mf = ::micron::simd::fma;

using ::sb::end_test_case;
using ::sb::print;
using ::sb::require_true;
using ::sb::test_case;

[[gnu::always_inline]] inline bool
near_eq(float a, float b, float tol = 1e-5f) noexcept
{
  float d = a - b;
  return (d < tol) && (d > -tol);
}

int
main()
{
  print("=== TEST FMA ===");

  test_case("fma: 128-bit f32 fma / fms / fnma / fnms");
  __m128 a = _mm_setr_ps(1, 2, 3, 4);
  __m128 b = _mm_setr_ps(0.5f, 1.5f, 2.5f, 3.5f);
  __m128 c = _mm_setr_ps(10, 20, 30, 40);
  alignas(16) float r[4];

  _mm_storeu_ps(r, mf::fma_f32(a, b, c));
  for ( int i = 0; i < 4; ++i ) require_true(near_eq(r[i], (float)(i + 1) * (0.5f + i) + (10.0f * (i + 1))));

  _mm_storeu_ps(r, mf::fms_f32(a, b, c));
  for ( int i = 0; i < 4; ++i ) require_true(near_eq(r[i], (float)(i + 1) * (0.5f + i) - (10.0f * (i + 1))));

  _mm_storeu_ps(r, mf::fnma_f32(a, b, c));
  for ( int i = 0; i < 4; ++i ) require_true(near_eq(r[i], -(float)(i + 1) * (0.5f + i) + (10.0f * (i + 1))));

  _mm_storeu_ps(r, mf::fnms_f32(a, b, c));
  for ( int i = 0; i < 4; ++i ) require_true(near_eq(r[i], -(float)(i + 1) * (0.5f + i) - (10.0f * (i + 1))));
  end_test_case();

  test_case("fma: 256-bit f32 fma + addsub");
  __m256 va = _mm256_setr_ps(1, 2, 3, 4, 5, 6, 7, 8);
  __m256 vb = _mm256_setr_ps(0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f);
  __m256 vc = _mm256_setr_ps(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f);
  alignas(32) float vr[8];
  _mm256_storeu_ps(vr, mf::fma_f32(va, vb, vc));
  for ( int i = 0; i < 8; ++i ) require_true(near_eq(vr[i], (float)(i + 1) * (0.5f + i) + (0.1f * (i + 1))));

  _mm256_storeu_ps(vr, mf::fma_addsub_f32(va, vb, vc));
  // even lanes: a*b - c, odd lanes: a*b + c
  for ( int i = 0; i < 8; ++i ) {
    float ab = (float)(i + 1) * (0.5f + i);
    float cv = 0.1f * (i + 1);
    float exp = (i & 1) ? (ab + cv) : (ab - cv);
    require_true(near_eq(vr[i], exp));
  }
  end_test_case();

  test_case("fma: 128-bit f64 fma");
  __m128d da = _mm_setr_pd(2.0, 3.0);
  __m128d db = _mm_setr_pd(4.0, 5.0);
  __m128d dc = _mm_setr_pd(1.0, 1.0);
  alignas(16) double dr[2];
  _mm_storeu_pd(dr, mf::fma_f64(da, db, dc));
  require_true(dr[0] == 9.0);       // 2*4+1
  require_true(dr[1] == 16.0);      // 3*5+1
  end_test_case();

  print("[TEST FMA OK]");
  return 0;
}
