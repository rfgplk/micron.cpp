// math_blas_batched.cpp — Snowball tests for batched / extension BLAS routines

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
  print("=== BLAS BATCHED + EXTENSIONS TESTS ===");

  test_case("axpby_batched: 3 independent y_i ← 2·x_i + 0.5·y_i");
  {
    f64 x0[3] = { 1, 1, 1 };
    f64 x1[3] = { 2, 2, 2 };
    f64 x2[3] = { 3, 3, 3 };
    f64 y0[3] = { 10, 10, 10 };
    f64 y1[3] = { 20, 20, 20 };
    f64 y2[3] = { 30, 30, 30 };

    quants::vec_view<f64> xs[3] = {
      quants::vec_view<f64>::from(x0, 3),
      quants::vec_view<f64>::from(x1, 3),
      quants::vec_view<f64>::from(x2, 3),
    };
    quants::vec_view<f64> ys[3] = {
      quants::vec_view<f64>::from(y0, 3),
      quants::vec_view<f64>::from(y1, 3),
      quants::vec_view<f64>::from(y2, 3),
    };
    blas::ext::axpby_batched<f64>(3, f64(2.0), xs, f64(0.5), ys);
    require_true(near(y0[0], 7.0));      // 5 + 2
    require_true(near(y1[0], 14.0));     // 10 + 4
    require_true(near(y2[0], 21.0));     // 15 + 6
  }
  end_test_case();

  test_case("dot_batched: 3 independent dots");
  {
    f64 a[3] = { 1, 2, 3 };
    f64 b[3] = { 4, 5, 6 };
    f64 c[3] = { 1, 0, 0 };

    quants::vec_view<f64> xs[3] = {
      quants::vec_view<f64>::from(a, 3),
      quants::vec_view<f64>::from(a, 3),
      quants::vec_view<f64>::from(b, 3),
    };
    quants::vec_view<f64> ys[3] = {
      quants::vec_view<f64>::from(b, 3),
      quants::vec_view<f64>::from(c, 3),
      quants::vec_view<f64>::from(c, 3),
    };
    f64 out[3] = { 0, 0, 0 };
    blas::ext::dot_batched<f64>(3, xs, ys, out);
    require_true(near(out[0], 32.0));     // a·b = 4+10+18
    require_true(near(out[1], 1.0));      // a·c = 1
    require_true(near(out[2], 4.0));      // b·c = 4
  }
  end_test_case();

  test_case("gemm_batched: 2 independent C_i ← A_i·B_i");
  {
    f64 A0[4] = { 1, 2, 3, 4 };
    f64 A1[4] = { 0, 1, 1, 0 };
    f64 B0[4] = { 5, 6, 7, 8 };
    f64 B1[4] = { 1, 2, 3, 4 };
    f64 C0[4] = { 0, 0, 0, 0 };
    f64 C1[4] = { 0, 0, 0, 0 };

    matrix::row_view<f64> As[2] = {
      matrix::row_view<f64>::from(A0, 2, 2),
      matrix::row_view<f64>::from(A1, 2, 2),
    };
    matrix::row_view<f64> Bs[2] = {
      matrix::row_view<f64>::from(B0, 2, 2),
      matrix::row_view<f64>::from(B1, 2, 2),
    };
    matrix::row_view<f64> Cs[2] = {
      matrix::row_view<f64>::from(C0, 2, 2),
      matrix::row_view<f64>::from(C1, 2, 2),
    };
    blas::ext::gemm_batched<blas::op::none, blas::op::none, f64>(2, f64(1.0), As, Bs, f64(0.0), Cs);
    // C0 = A0·B0 = [[19,22];[43,50]]
    require_true(near(C0[0], 19.0));
    require_true(near(C0[3], 50.0));
    // C1 = [[0,1];[1,0]] · [[1,2];[3,4]] = [[3,4];[1,2]]
    require_true(near(C1[0], 3.0));
    require_true(near(C1[1], 4.0));
    require_true(near(C1[2], 1.0));
    require_true(near(C1[3], 2.0));
  }
  end_test_case();

  test_case("omatcopy: B ← α·Aᵀ");
  {
    f64 A[6] = { 1, 2, 3, 4, 5, 6 };     // 2x3 row-major
    f64 B[6] = { 0, 0, 0, 0, 0, 0 };     // 3x2 row-major
    auto Av = matrix::row_view<f64>::from(A, 2, 3);
    auto Bv = matrix::row_view<f64>::from(B, 3, 2);
    blas::ext::omatcopy<blas::op::trans>(f64(2.0), Av, Bv);
    // Aᵀ = [[1,4];[2,5];[3,6]] → 2*Aᵀ = [[2,8];[4,10];[6,12]]
    require_true(near(B[0], 2.0));
    require_true(near(B[1], 8.0));
    require_true(near(B[2], 4.0));
    require_true(near(B[3], 10.0));
    require_true(near(B[4], 6.0));
    require_true(near(B[5], 12.0));
  }
  end_test_case();

  test_case("gemmt upper: only upper triangle of C is written");
  {
    // A is 3x2, B is 2x3, C is 3x3
    f64 A[6] = { 1, 2, 3, 4, 5, 6 };
    f64 B[6] = { 1, 0, 0, 0, 1, 0 };
    f64 C[9] = { 0, 0, 0, 7, 0, 0, 7, 7, 0 };     // pre-fill strict-lower with 7
    auto Av = matrix::row_view<f64>::from(A, 3, 2);
    auto Bv = matrix::row_view<f64>::from(B, 2, 3);
    auto Cv = matrix::row_view<f64>::from(C, 3, 3);
    blas::ext::gemmt<blas::uplo::upper>(f64(1.0), Av, Bv, f64(0.0), Cv);
    // A·B (full): [[1*1+2*0, 1*0+2*1, 0]; [3*1+4*0, 3*0+4*1, 0]; [5*1+6*0, 5*0+6*1, 0]]
    //          = [[1,2,0];[3,4,0];[5,6,0]]
    // upper-only writes: C[0..2]=[1,2,0]; C[4..5]=[4,0]; C[8]=0; rest unchanged.
    require_true(near(C[0], 1.0));
    require_true(near(C[1], 2.0));
    require_true(near(C[2], 0.0));
    require_true(near(C[4], 4.0));
    require_true(near(C[5], 0.0));
    require_true(near(C[8], 0.0));
    // strict-lower preserved
    require_true(near(C[3], 7.0));
    require_true(near(C[6], 7.0));
    require_true(near(C[7], 7.0));
  }
  end_test_case();

  print("=== blas batched/ext ok ===");
  return 0;
}
