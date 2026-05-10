// math_blas3.cpp — Snowball tests for math::blas::level3

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
  print("=== BLAS L3 TESTS ===");

  test_case("gemm row × row × row: C ← A·B");
  {
    // A = 2x3, B = 3x2, C = 2x2
    f64 A[6] = { 1, 2, 3, 4, 5, 6 };
    f64 B[6] = { 7, 8, 9, 10, 11, 12 };
    f64 C[4] = { 0, 0, 0, 0 };
    auto Av = matrix::row_view<f64>::from(A, 2, 3);
    auto Bv = matrix::row_view<f64>::from(B, 3, 2);
    auto Cv = matrix::row_view<f64>::from(C, 2, 2);
    blas::level3::gemm(f64(1.0), Av, Bv, f64(0.0), Cv);
    // [[1*7+2*9+3*11, 1*8+2*10+3*12]; [4*7+5*9+6*11, 4*8+5*10+6*12]]
    // = [[58, 64]; [139, 154]]
    require_true(near(C[0], 58.0));
    require_true(near(C[1], 64.0));
    require_true(near(C[2], 139.0));
    require_true(near(C[3], 154.0));
  }
  end_test_case();

  test_case("gemm with α, β: C ← α·A·B + β·C");
  {
    f64 A[4] = { 1, 2, 3, 4 };
    f64 B[4] = { 5, 6, 7, 8 };
    f64 C[4] = { 1, 1, 1, 1 };
    auto Av = matrix::row_view<f64>::from(A, 2, 2);
    auto Bv = matrix::row_view<f64>::from(B, 2, 2);
    auto Cv = matrix::row_view<f64>::from(C, 2, 2);
    blas::level3::gemm(f64(2.0), Av, Bv, f64(3.0), Cv);
    // A·B = [[1*5+2*7, 1*6+2*8]; [3*5+4*7, 3*6+4*8]] = [[19,22];[43,50]]
    // 2·A·B + 3·C = [[38+3, 44+3]; [86+3, 100+3]] = [[41,47];[89,103]]
    require_true(near(C[0], 41.0));
    require_true(near(C[1], 47.0));
    require_true(near(C[2], 89.0));
    require_true(near(C[3], 103.0));
  }
  end_test_case();

  test_case("gemm transpose A: C ← Aᵀ·B");
  {
    // A = 3x2 stored row-major, Aᵀ = 2x3
    f64 A[6] = { 1, 4, 2, 5, 3, 6 };
    f64 B[9] = { 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    f64 C[6] = { 0, 0, 0, 0, 0, 0 };
    auto Av = matrix::row_view<f64>::from(A, 3, 2);
    auto Bv = matrix::row_view<f64>::from(B, 3, 3);
    auto Cv = matrix::row_view<f64>::from(C, 2, 3);
    blas::level3::gemm<blas::op::trans, blas::op::none>(f64(1.0), Av, Bv, f64(0.0), Cv);
    // Aᵀ = [[1,2,3];[4,5,6]]
    // Aᵀ·B = [[1*7+2*10+3*13, 1*8+2*11+3*14, 1*9+2*12+3*15];
    //         [4*7+5*10+6*13, 4*8+5*11+6*14, 4*9+5*12+6*15]]
    //      = [[66, 72, 78]; [156, 171, 186]]
    require_true(near(C[0], 66.0));
    require_true(near(C[1], 72.0));
    require_true(near(C[2], 78.0));
    require_true(near(C[3], 156.0));
    require_true(near(C[4], 171.0));
    require_true(near(C[5], 186.0));
  }
  end_test_case();

  test_case("gemm row × col: mixed-layout still computes A·B");
  {
    // A is 2x2 row-major; B is 2x2 col-major (same numeric values arranged differently).
    f64 A[4] = { 1, 2, 3, 4 };         // row-major: [[1,2];[3,4]]
    f64 B_col[4] = { 5, 7, 6, 8 };     // col-major: col0=[5,7], col1=[6,8] → matrix [[5,6];[7,8]]
    f64 C[4] = { 0, 0, 0, 0 };
    auto Av = matrix::row_view<f64>::from(A, 2, 2);
    auto Bv = matrix::col_view<f64>::from(B_col, 2, 2);
    auto Cv = matrix::row_view<f64>::from(C, 2, 2);
    blas::level3::gemm(f64(1.0), Av, Bv, f64(0.0), Cv);
    // [[1,2];[3,4]] · [[5,6];[7,8]] = [[19,22];[43,50]]
    require_true(near(C[0], 19.0));
    require_true(near(C[1], 22.0));
    require_true(near(C[2], 43.0));
    require_true(near(C[3], 50.0));
  }
  end_test_case();

  test_case("syrk upper: C ← α·A·Aᵀ + β·C");
  {
    // A = 2x3 row-major
    f64 A[6] = { 1, 2, 3, 4, 5, 6 };
    f64 C[4] = { 0, 0, 0, 0 };
    auto Av = matrix::row_view<f64>::from(A, 2, 3);
    matrix::sym_row_view<f64, blas::uplo::upper> Cv{ C, 2, 2, 2 };
    blas::level3::syrk(f64(1.0), Av, f64(0.0), Cv);
    // A·Aᵀ = [[1+4+9, 4+10+18];[4+10+18, 16+25+36]] = [[14,32];[32,77]]
    require_true(near(C[0], 14.0));
    require_true(near(C[1], 32.0));
    require_true(near(C[3], 77.0));
    // strict lower untouched
    require_true(near(C[2], 0.0));
  }
  end_test_case();

  test_case("trsm left upper: solve U·X = α·B");
  {
    // U = [[2,1];[0,3]], B = [[4,8];[6,9]], α = 1
    f64 U[4] = { 2, 1, 0, 3 };
    f64 B[4] = { 4, 8, 6, 9 };
    matrix::tri_row_view<f64, blas::uplo::upper, blas::diag::non_unit> Uv{ U, 2, 2, 2 };
    auto Bv = matrix::row_view<f64>::from(B, 2, 2);
    blas::level3::trsm(f64(1.0), Uv, Bv);
    // For col 0: U·x = [4,6]^T
    //   x[1] = 6/3 = 2;  x[0] = (4 - 1*2)/2 = 1
    // For col 1: U·x = [8,9]^T
    //   x[1] = 9/3 = 3;  x[0] = (8 - 1*3)/2 = 5/2
    require_true(near(B[0], 1.0));
    require_true(near(B[1], 2.5));
    require_true(near(B[2], 2.0));
    require_true(near(B[3], 3.0));
  }
  end_test_case();

  test_case("trmm left upper: B ← U·B");
  {
    f64 U[4] = { 2, 3, 0, 4 };
    f64 B[4] = { 1, 1, 1, 1 };
    matrix::tri_row_view<f64, blas::uplo::upper, blas::diag::non_unit> Uv{ U, 2, 2, 2 };
    auto Bv = matrix::row_view<f64>::from(B, 2, 2);
    blas::level3::trmm(f64(1.0), Uv, Bv);
    // U·B per column:
    //  col 0: [2*1+3*1, 0+4*1] = [5, 4]
    //  col 1: [2*1+3*1, 0+4*1] = [5, 4]
    require_true(near(B[0], 5.0));
    require_true(near(B[1], 5.0));
    require_true(near(B[2], 4.0));
    require_true(near(B[3], 4.0));
  }
  end_test_case();

  test_case("symm left upper: C ← A·B  with A symmetric");
  {
    // A = [[2,1];[1,3]],  B = [[1,2];[3,4]]
    // A·B = [[2*1+1*3, 2*2+1*4];[1*1+3*3, 1*2+3*4]] = [[5,8];[10,14]]
    f64 A[4] = { 2, 1, 0, 3 };     // upper-stored
    f64 B[4] = { 1, 2, 3, 4 };
    f64 C[4] = { 0, 0, 0, 0 };
    matrix::sym_row_view<f64, blas::uplo::upper> Av{ A, 2, 2, 2 };
    auto Bv = matrix::row_view<f64>::from(B, 2, 2);
    auto Cv = matrix::row_view<f64>::from(C, 2, 2);
    blas::level3::symm(f64(1.0), Av, Bv, f64(0.0), Cv);
    require_true(near(C[0], 5.0));
    require_true(near(C[1], 8.0));
    require_true(near(C[2], 10.0));
    require_true(near(C[3], 14.0));
  }
  end_test_case();

  test_case("gemm 4x4 f64 — exercises AVX2 register-tile fast path");
  {
    // 4×4 = 4×4 · 4×4. Forces dispatch to gemm_4x4_avx2_f64.
    f64 A[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    f64 B[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    f64 C[16];
    for ( usize i = 0; i < 16; ++i ) C[i] = 0;
    auto Av = matrix::row_view<f64>::from(A, 4, 4);
    auto Bv = matrix::row_view<f64>::from(B, 4, 4);
    auto Cv = matrix::row_view<f64>::from(C, 4, 4);
    blas::level3::gemm(f64(1.0), Av, Bv, f64(0.0), Cv);
    // A · I = A
    for ( usize i = 0; i < 16; ++i ) require_true(near(C[i], A[i]));
  }
  end_test_case();

  test_case("gemm 8x8 f64 — exercises AVX2 register-tile across multiple tiles");
  {
    f64 A[64], B[64], C[64];
    for ( usize i = 0; i < 64; ++i ) {
      A[i] = f64(i + 1);
      B[i] = f64((i * 3) % 17);
      C[i] = 0;
    }
    auto Av = matrix::row_view<f64>::from(A, 8, 8);
    auto Bv = matrix::row_view<f64>::from(B, 8, 8);
    auto Cv = matrix::row_view<f64>::from(C, 8, 8);
    blas::level3::gemm(f64(1.0), Av, Bv, f64(0.0), Cv);

    // Reference: naive scalar gemm
    f64 R[64];
    for ( usize i = 0; i < 8; ++i )
      for ( usize j = 0; j < 8; ++j ) {
        f64 acc = 0;
        for ( usize p = 0; p < 8; ++p ) acc += A[i * 8 + p] * B[p * 8 + j];
        R[i * 8 + j] = acc;
      }
    for ( usize i = 0; i < 64; ++i ) require_true(near(C[i], R[i], 1e-9));
  }
  end_test_case();

  print("=== blas L3 ok ===");
  return 0;
}
