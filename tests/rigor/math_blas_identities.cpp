// math_blas_identities.cpp — Snowball tests for BLAS identity builders

#include "../../src/math/blas/blas.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps = 1e-10)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== BLAS IDENTITIES TESTS ===");

  test_case("from_quat: identity quaternion → I_3");
  {
    quat<f64> q = quat<f64>::identity();      // {0,0,0,1}
    auto R = blas::identities::from_quat<f64>(q);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) require_true(near(R.data[i * 3 + j], (i == j) ? 1.0 : 0.0));
  }
  end_test_case();

  test_case("from_quat: 90° around z axis rotates x → y");
  {
    // q = (0, 0, sin(45°), cos(45°)) for 90° rotation about z
    const f64 s = 0.7071067811865476;      // sin(45°) = cos(45°)
    quat<f64> q{ { 0.0, 0.0, s, s } };
    auto R = blas::identities::from_quat<f64>(q);
    // Apply R · [1,0,0]ᵀ → should be [0,1,0]ᵀ
    f64 x[3] = { 1.0, 0.0, 0.0 };
    f64 y[3] = { 0.0, 0.0, 0.0 };
    auto Av = matrix::row_view<f64>::from(R.data, 3, 3);
    auto xv = quants::vec_view<f64>::from(x, 3);
    auto yv = quants::vec_view<f64>::from(y, 3);
    blas::level2::gemv(f64(1.0), Av, xv, f64(0.0), yv);
    require_true(near(y[0], 0.0, 1e-12));
    require_true(near(y[1], 1.0, 1e-12));
    require_true(near(y[2], 0.0, 1e-12));
  }
  end_test_case();

  test_case("skew(v) · w == v × w");
  {
    vec<f64, 3> v{ { 1.0, 2.0, 3.0 } };
    vec<f64, 3> w{ { 4.0, 5.0, 6.0 } };
    // v × w = (2*6-3*5, 3*4-1*6, 1*5-2*4) = (-3, 6, -3)
    auto S = blas::identities::skew<f64>(v);
    f64 wbuf[3] = { 4.0, 5.0, 6.0 };
    f64 out[3] = { 0.0, 0.0, 0.0 };
    auto Sv = matrix::row_view<f64>::from(S.data, 3, 3);
    auto wv = quants::vec_view<f64>::from(wbuf, 3);
    auto ov = quants::vec_view<f64>::from(out, 3);
    blas::level2::gemv(f64(1.0), Sv, wv, f64(0.0), ov);
    require_true(near(out[0], -3.0));
    require_true(near(out[1], 6.0));
    require_true(near(out[2], -3.0));
  }
  end_test_case();

  test_case("from_axis_angle == from_quat for matched rotations");
  {
    // 90° about z: axis = (0,0,1), theta = π/2
    const f64 pi = 3.14159265358979323846;
    vec<f64, 3> axis{ { 0.0, 0.0, 1.0 } };
    auto Raa = blas::identities::from_axis_angle<f64>(axis, pi / 2.0);

    // Equivalent quat: (0, 0, sin(π/4), cos(π/4))
    const f64 c = mk::trig::cos<f64>(pi / 4.0);
    const f64 s = mk::trig::sin<f64>(pi / 4.0);
    quat<f64> q{ { 0.0, 0.0, s, c } };
    auto Rq = blas::identities::from_quat<f64>(q);

    for ( usize i = 0; i < 9; ++i ) require_true(near(Raa.data[i], Rq.data[i], 1e-12));
  }
  end_test_case();

  test_case("householder: H is orthogonal (H·Hᵀ = I)");
  {
    // v = (1, 1, 1), β = 2/3
    f64 v_data[3] = { 1.0, 1.0, 1.0 };
    f64 H[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    auto vv = quants::vec_view<f64>::from(v_data, 3);
    auto Hv = matrix::row_view<f64>::from(H, 3, 3);
    blas::identities::householder<f64>(vv, Hv);

    // Check H·Hᵀ = I via gemm
    f64 HHT[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    auto Hv2 = matrix::row_view<f64>::from(H, 3, 3);
    auto HHTv = matrix::row_view<f64>::from(HHT, 3, 3);
    blas::level3::gemm<blas::op::none, blas::op::trans>(f64(1.0), Hv, Hv2, f64(0.0), HHTv);

    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) require_true(near(HHT[i * 3 + j], (i == j) ? 1.0 : 0.0, 1e-10));
  }
  end_test_case();

  test_case("givens: 2-plane rotation in 4D");
  {
    f64 G[16] = { 0 };
    auto Gv = matrix::row_view<f64>::from(G, 4, 4);
    const f64 c = 0.6, s = 0.8;
    blas::identities::givens<f64>(0, 2, c, s, Gv);
    // Expected: identity except (0,0)=(2,2)=0.6, (0,2)=0.8, (2,0)=-0.8
    require_true(near(G[0], 0.6));
    require_true(near(G[2], 0.8));
    require_true(near(G[8], -0.8));
    require_true(near(G[10], 0.6));
    require_true(near(G[5], 1.0));
    require_true(near(G[15], 1.0));
    // c² + s² = 1 ⇒ Givens is orthogonal
    require_true(near(c * c + s * s, 1.0));
  }
  end_test_case();

  test_case("outer: x ⊗ y → A");
  {
    f64 xb[2] = { 1.0, 2.0 };
    f64 yb[3] = { 3.0, 4.0, 5.0 };
    f64 A[6] = { 0 };
    auto xv = quants::vec_view<f64>::from(xb, 2);
    auto yv = quants::vec_view<f64>::from(yb, 3);
    auto Av = matrix::row_view<f64>::from(A, 2, 3);
    blas::identities::outer<f64>(f64(1.0), xv, yv, Av);
    require_true(near(A[0], 3.0));
    require_true(near(A[1], 4.0));
    require_true(near(A[2], 5.0));
    require_true(near(A[3], 6.0));
    require_true(near(A[4], 8.0));
    require_true(near(A[5], 10.0));
  }
  end_test_case();

  print("=== blas identities ok ===");
  return 1;
}
