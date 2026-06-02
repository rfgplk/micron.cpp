// transform_simd_route.cpp — verifies geometry::transform operator*/apply route
// through linalg::ops gemm/gemv (so the f32 Dim=3 4x4 case uses the SSE/NEON
// LinearCombine fast path) and still match a scalar reference. Kept free of
// mk.hpp / simd-trig so it compiles on every arch.

#include "../../src/math/geometry/transform.hpp"
#include "../../src/math/quants/vec.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;
namespace mg = micron::math::geometry;

using xf = mg::transform<f32, 3, mg::transform_mode::affine>;
using pxf = mg::transform<f32, 3, mg::transform_mode::projective>;

static bool
near(f64 a, f64 b, f64 eps = 1e-4)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
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
  print("=== TRANSFORM SIMD ROUTE ===");

  // ------------------------------------------------------------------
  test_case("operator* (f32 4x4) == scalar matrix product reference");
  {
    u64 s = 0x5EED01;
    bool ok = true;
    for ( int t = 0; t < 1000 && ok; ++t ) {
      xf A{}, B{};
      for ( int k = 0; k < 16; ++k ) {
        A.M.data[k] = runit(s);
        B.M.data[k] = runit(s);
      }
      auto C = A * B;      // routes through gemm -> SSE/NEON for f32 4x4
      for ( int i = 0; i < 4; ++i )
        for ( int j = 0; j < 4; ++j ) {
          f64 acc = 0;
          for ( int p = 0; p < 4; ++p ) acc += static_cast<f64>(A.M.data[i * 4 + p]) * static_cast<f64>(B.M.data[p * 4 + j]);
          if ( !near(static_cast<f64>(C.M.data[i * 4 + j]), acc) ) ok = false;
        }
    }
    require_true(ok);
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("affine apply (f32) == scalar M*[p;1] reference");
  {
    u64 s = 0xA11CE;
    bool ok = true;
    for ( int t = 0; t < 1000 && ok; ++t ) {
      xf A{};
      for ( int k = 0; k < 16; ++k ) A.M.data[k] = runit(s);
      m::vec<f32, 3> p{ runit(s), runit(s), runit(s) };
      auto r = A.apply(p);
      for ( int i = 0; i < 3; ++i ) {
        f64 acc = static_cast<f64>(A.M.data[i * 4 + 3]);
        for ( int j = 0; j < 3; ++j ) acc += static_cast<f64>(A.M.data[i * 4 + j]) * static_cast<f64>(p.data[j]);
        if ( !near(static_cast<f64>(r.data[i]), acc) ) ok = false;
      }
    }
    require_true(ok);
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("projective apply (f32) does the perspective divide");
  {
    u64 s = 0xBEE5;
    bool ok = true;
    for ( int t = 0; t < 1000 && ok; ++t ) {
      pxf A{};
      for ( int k = 0; k < 16; ++k ) A.M.data[k] = runit(s);
      // well-conditioned perspective-style w-row so 1/w stays stable in f32
      A.M.data[12] = 0.0f;
      A.M.data[13] = 0.0f;
      A.M.data[14] = -1.0f;
      A.M.data[15] = 2.0f;
      m::vec<f32, 3> p{ runit(s), runit(s), runit(s) };
      auto r = A.apply(p);
      f64 w = static_cast<f64>(A.M.data[3 * 4 + 3]);
      for ( int j = 0; j < 3; ++j ) w += static_cast<f64>(A.M.data[3 * 4 + j]) * static_cast<f64>(p.data[j]);
      f64 inv_w = (w != 0.0) ? 1.0 / w : 1.0;
      for ( int i = 0; i < 3; ++i ) {
        f64 acc = static_cast<f64>(A.M.data[i * 4 + 3]);
        for ( int j = 0; j < 3; ++j ) acc += static_cast<f64>(A.M.data[i * 4 + j]) * static_cast<f64>(p.data[j]);
        if ( !near(static_cast<f64>(r.data[i]), acc * inv_w, 1e-3) ) ok = false;
      }
    }
    require_true(ok);
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("apply_homogeneous (f32) == gemv reference");
  {
    u64 s = 0xF00D;
    pxf A{};
    for ( int k = 0; k < 16; ++k ) A.M.data[k] = runit(s);
    m::vec<f32, 4> h{ runit(s), runit(s), runit(s), 1.0f };
    auto r = A.apply_homogeneous(h);
    for ( int i = 0; i < 4; ++i ) {
      f64 acc = 0;
      for ( int j = 0; j < 4; ++j ) acc += static_cast<f64>(A.M.data[i * 4 + j]) * static_cast<f64>(h.data[j]);
      require_true(near(static_cast<f64>(r.data[i]), acc));
    }
  }
  end_test_case();

  print("=== TRANSFORM SIMD ROUTE PASSED ===");
  return 1;
}
