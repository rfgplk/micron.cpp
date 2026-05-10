// math_blas2.cpp — Snowball tests for math::blas::level2

#include "../../src/math/blas/blas.hpp"
#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
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
  print("=== BLAS L2 TESTS ===");

  test_case("gemv row-major: y ← α·A·x + β·y");
  {
    // 3 x 4 row-major
    f64 A[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    f64 x[4] = { 1, 1, 1, 1 };
    f64 y[3] = { 0, 0, 0 };
    auto Av = matrix::row_view<f64>::from(A, 3, 4);
    auto xv = quants::vec_view<f64>::from(x, 4);
    auto yv = quants::vec_view<f64>::from(y, 3);
    blas::level2::gemv<blas::op::none>(f64(1.0), Av, xv, f64(0.0), yv);
    require_true(near(y[0], 10.0));     // 1+2+3+4
    require_true(near(y[1], 26.0));     // 5+6+7+8
    require_true(near(y[2], 42.0));     // 9+10+11+12
  }
  end_test_case();

  test_case("gemv row-major transpose: y ← α·Aᵀ·x");
  {
    f64 A[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    f64 x[3] = { 1, 1, 1 };
    f64 y[4] = { 0, 0, 0, 0 };
    auto Av = matrix::row_view<f64>::from(A, 3, 4);
    auto xv = quants::vec_view<f64>::from(x, 3);
    auto yv = quants::vec_view<f64>::from(y, 4);
    blas::level2::gemv<blas::op::trans>(f64(1.0), Av, xv, f64(0.0), yv);
    require_true(near(y[0], 15.0));     // 1+5+9
    require_true(near(y[1], 18.0));     // 2+6+10
    require_true(near(y[2], 21.0));     // 3+7+11
    require_true(near(y[3], 24.0));     // 4+8+12
  }
  end_test_case();

  test_case("gemv col-major: same data, swapped semantics");
  {
    // 3 rows x 4 cols stored col-major: data is [col0; col1; col2; col3]
    f64 A[12] = {
      1, 5, 9,      // col 0
      2, 6, 10,     // col 1
      3, 7, 11,     // col 2
      4, 8, 12      // col 3
    };
    f64 x[4] = { 1, 1, 1, 1 };
    f64 y[3] = { 0, 0, 0 };
    auto Av = matrix::col_view<f64>::from(A, 3, 4);
    auto xv = quants::vec_view<f64>::from(x, 4);
    auto yv = quants::vec_view<f64>::from(y, 3);
    blas::level2::gemv<blas::op::none>(f64(1.0), Av, xv, f64(0.0), yv);
    require_true(near(y[0], 10.0));
    require_true(near(y[1], 26.0));
    require_true(near(y[2], 42.0));
  }
  end_test_case();

  test_case("symv upper: y ← A·x with A symmetric");
  {
    // 3x3 symmetric: A = [[2,1,0],[1,3,1],[0,1,4]]
    f64 A[9] = { 2, 1, 0, 0, 3, 1, 0, 0, 4 };
    f64 x[3] = { 1, 2, 3 };
    f64 y[3] = { 0, 0, 0 };
    matrix::sym_row_view<f64, blas::uplo::upper> Av{ A, 3, 3, 3 };
    auto xv = quants::vec_view<f64>::from(x, 3);
    auto yv = quants::vec_view<f64>::from(y, 3);
    blas::level2::symv(f64(1.0), Av, xv, f64(0.0), yv);
    // [2,1,0]·[1,2,3] = 2+2 = 4
    // [1,3,1]·[1,2,3] = 1+6+3 = 10
    // [0,1,4]·[1,2,3] = 2+12 = 14
    require_true(near(y[0], 4.0));
    require_true(near(y[1], 10.0));
    require_true(near(y[2], 14.0));
  }
  end_test_case();

  test_case("trmv upper non-unit: x ← U·x");
  {
    // U = [[2,3,4],[0,5,6],[0,0,7]]
    f64 U[9] = { 2, 3, 4, 0, 5, 6, 0, 0, 7 };
    f64 x[3] = { 1, 1, 1 };
    matrix::tri_row_view<f64, blas::uplo::upper, blas::diag::non_unit> Uv{ U, 3, 3, 3 };
    auto xv = quants::vec_view<f64>::from(x, 3);
    blas::level2::trmv(Uv, xv);
    require_true(near(x[0], 9.0));      // 2+3+4
    require_true(near(x[1], 11.0));     // 0+5+6
    require_true(near(x[2], 7.0));      // 0+0+7
  }
  end_test_case();

  test_case("trsv upper: solve U·x = b");
  {
    // U = [[2,1,0],[0,3,1],[0,0,4]],  b = [4,7,8]
    // Back-sub:  x[2] = 8/4 = 2
    //            x[1] = (7 - 1*2)/3 = 5/3
    //            x[0] = (4 - 1*5/3)/2 = 7/6
    f64 U[9] = { 2, 1, 0, 0, 3, 1, 0, 0, 4 };
    f64 b[3] = { 4, 7, 8 };
    matrix::tri_row_view<f64, blas::uplo::upper, blas::diag::non_unit> Uv{ U, 3, 3, 3 };
    auto bv = quants::vec_view<f64>::from(b, 3);
    blas::level2::trsv(Uv, bv);
    require_true(near(b[0], 7.0 / 6.0));
    require_true(near(b[1], 5.0 / 3.0));
    require_true(near(b[2], 2.0));
  }
  end_test_case();

  test_case("ger: A ← α·x·yᵀ + A");
  {
    f64 A[6] = { 0, 0, 0, 0, 0, 0 };     // 2x3 row-major
    f64 x[2] = { 1, 2 };
    f64 y[3] = { 4, 5, 6 };
    auto Av = matrix::row_view<f64>::from(A, 2, 3);
    auto xv = quants::vec_view<f64>::from(x, 2);
    auto yv = quants::vec_view<f64>::from(y, 3);
    blas::level2::ger(f64(1.0), xv, yv, Av);
    require_true(near(A[0], 4.0));      // 1*4
    require_true(near(A[1], 5.0));      // 1*5
    require_true(near(A[2], 6.0));      // 1*6
    require_true(near(A[3], 8.0));      // 2*4
    require_true(near(A[4], 10.0));     // 2*5
    require_true(near(A[5], 12.0));     // 2*6
  }
  end_test_case();

  test_case("syr upper: A ← α·x·xᵀ + A");
  {
    f64 A[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    f64 x[3] = { 1, 2, 3 };
    matrix::sym_row_view<f64, blas::uplo::upper> Av{ A, 3, 3, 3 };
    auto xv = quants::vec_view<f64>::from(x, 3);
    blas::level2::syr(f64(1.0), xv, Av);
    // upper triangle should have x_i * x_j
    require_true(near(A[0], 1.0));
    require_true(near(A[1], 2.0));
    require_true(near(A[2], 3.0));
    require_true(near(A[4], 4.0));
    require_true(near(A[5], 6.0));
    require_true(near(A[8], 9.0));
    // strict lower must remain zero
    require_true(near(A[3], 0.0));
    require_true(near(A[6], 0.0));
    require_true(near(A[7], 0.0));
  }
  end_test_case();

  print("=== blas L2 ok ===");
  return 0;
}
