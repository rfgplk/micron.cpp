

#include "../../src/math/linalg/ops.hpp"
#include "../../src/math/matrix/mat.hpp"
#include "../../src/math/quants/vec.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace ml = micron::math;
namespace lo = micron::math::linalg::ops;

using mat4f = ml::mat<f32, 4, 4>;
using vec4f = ml::vec<f32, 4>;

static bool
near(f64 a, f64 b, f64 eps = 1e-5)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

template<typename M>
static M
ref_gemm(const M &a, const M &b, int n)
{
  M c = M::zero();
  for ( int i = 0; i < n; ++i )
    for ( int j = 0; j < n; ++j ) {
      f64 acc = 0;
      for ( int p = 0; p < n; ++p ) acc += static_cast<f64>(a.data[i * n + p]) * static_cast<f64>(b.data[p * n + j]);
      c.data[i * n + j] = static_cast<typename M::value_type>(acc);
    }
  return c;
}

static u64
lcg(u64 &s)
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s;
}

static f32
runit(u64 &s)
{
  return static_cast<f32>(static_cast<f64>(lcg(s) >> 11) * 0x1.0p-53 - 0.5);
}

int
main()
{
  print("=== MAT4 SIMD FAST PATHS ===");

  test_case("gemm f32 4x4 (runtime SSE) matches scalar reference over random inputs");
  {
    u64 s = 0xC0FFEEULL;
    bool ok = true;
    for ( int t = 0; t < 2000 && ok; ++t ) {
      mat4f A, B;
      for ( int k = 0; k < 16; ++k ) {
        A.data[k] = runit(s);
        B.data[k] = runit(s);
      }
      auto got = lo::gemm(A, B);
      auto ref = ref_gemm(A, B, 4);
      for ( int k = 0; k < 16; ++k )
        if ( !near(static_cast<f64>(got.data[k]), static_cast<f64>(ref.data[k])) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("gemv f32 4x4 (runtime SSE) matches scalar reference over random inputs");
  {
    u64 s = 0xBEEF77ULL;
    bool ok = true;
    for ( int t = 0; t < 2000 && ok; ++t ) {
      mat4f M;
      vec4f v;
      for ( int k = 0; k < 16; ++k ) M.data[k] = runit(s);
      for ( int k = 0; k < 4; ++k ) v.data[k] = runit(s);
      auto got = lo::gemv(M, v);
      vec4f ref{};
      for ( int i = 0; i < 4; ++i ) {
        f64 acc = 0;
        for ( int j = 0; j < 4; ++j ) acc += static_cast<f64>(M.data[i * 4 + j]) * static_cast<f64>(v.data[j]);
        ref.data[i] = static_cast<f32>(acc);
      }
      for ( int k = 0; k < 4; ++k )
        if ( !near(static_cast<f64>(got.data[k]), static_cast<f64>(ref.data[k])) ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("identity laws: gemm(I,A)==A, gemv(I,v)==v");
  {
    auto I = mat4f::identity();
    u64 s = 0x1357ULL;
    mat4f A;
    vec4f v;
    for ( int k = 0; k < 16; ++k ) A.data[k] = runit(s);
    for ( int k = 0; k < 4; ++k ) v.data[k] = runit(s);
    auto Ai = lo::gemm(I, A);
    auto vi = lo::gemv(I, v);
    for ( int k = 0; k < 16; ++k ) require_true(near(static_cast<f64>(Ai.data[k]), static_cast<f64>(A.data[k]), 1e-6));
    for ( int k = 0; k < 4; ++k ) require_true(near(static_cast<f64>(vi.data[k]), static_cast<f64>(v.data[k]), 1e-6));
  }
  end_test_case();

  test_case("other sizes/types untouched: f64 4x4 and f32 3x3 still match reference");
  {
    using mat4d = ml::mat<f64, 4, 4>;
    using mat3f = ml::mat<f32, 3, 3>;
    u64 s = 0x99AA;
    mat4d A, B;
    for ( int k = 0; k < 16; ++k ) {
      A.data[k] = static_cast<f64>(lcg(s) >> 11) * 0x1.0p-53 - 0.5;
      B.data[k] = static_cast<f64>(lcg(s) >> 11) * 0x1.0p-53 - 0.5;
    }
    auto gd = lo::gemm(A, B);
    auto rd = ref_gemm(A, B, 4);
    for ( int k = 0; k < 16; ++k ) require_true(near(gd.data[k], rd.data[k], 1e-12));

    mat3f P, Q;
    for ( int k = 0; k < 9; ++k ) {
      P.data[k] = runit(s);
      Q.data[k] = runit(s);
    }
    auto gp = lo::gemm(P, Q);
    auto rp = ref_gemm(P, Q, 3);
    for ( int k = 0; k < 9; ++k ) require_true(near(static_cast<f64>(gp.data[k]), static_cast<f64>(rp.data[k])));
  }
  end_test_case();

  print("=== MAT4 SIMD FAST PATHS PASSED ===");
  return 1;
}
