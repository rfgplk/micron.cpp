// math_blas_dynamic.cpp — BLAS L1/L2/L3 round-trip through dynvec/dynmat.

#include "../../src/math/blas/blas.hpp"
#include "../../src/math/matrix/matrices.hpp"
#include "../../src/math/quants/vecs.hpp"
#include "../../src/std.hpp"
#include "../../src/strings.hpp"
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
  print("=== BLAS DYNAMIC (dynvec/dynmat) TESTS ===");

  test_case("L1 dot: dynvec · dynvec");
  {
    dynvec<f64> u(4);
    dynvec<f64> v(4);
    for ( usize i = 0; i < 4; ++i ) {
      u[i] = f64(i + 1);          // 1,2,3,4
      v[i] = f64(2 * i + 1);      // 1,3,5,7
    }
    f64 d = blas::level1::dot(as_view(u), as_view(v));
    require_true(near(d, 1 + 6 + 15 + 28));      // 50
  }
  end_test_case();

  test_case("L1 axpy: y ← α·x + y on dynvec");
  {
    dynvec<f64> x(5, 1.0);
    dynvec<f64> y(5, 10.0);
    blas::level1::axpy(f64(2.0), as_view(x), as_view(y));
    for ( usize i = 0; i < 5; ++i ) require_true(near(y[i], 12.0));
  }
  end_test_case();

  test_case("L1 nrm2: ||dynvec||₂");
  {
    dynvec<f64> v(3);
    v[0] = 3.0;
    v[1] = 4.0;
    v[2] = 0.0;
    f64 n = blas::level1::nrm2(as_view(v));
    require_true(near(n, 5.0));
  }
  end_test_case();

  test_case("L2 gemv: y ← A·x  with A 2x3 dynmat");
  {
    dynmat<f64> A(2, 3);
    A.at(0, 0) = 1;
    A.at(0, 1) = 2;
    A.at(0, 2) = 3;
    A.at(1, 0) = 4;
    A.at(1, 1) = 5;
    A.at(1, 2) = 6;
    dynvec<f64> x(3);
    x[0] = 1;
    x[1] = 1;
    x[2] = 1;
    dynvec<f64> y(2, 0.0);
    blas::level2::gemv(f64(1.0), as_row_view(A), as_view(x), f64(0.0), as_view(y));
    require_true(near(y[0], 6.0));       // 1+2+3
    require_true(near(y[1], 15.0));      // 4+5+6
  }
  end_test_case();

  test_case("L3 gemm: C ← A·B  matches the math_blas3 reference");
  {
    // 2x3 * 3x2 = 2x2 — same numerical case as math_blas3.cpp first test
    dynmat<f64> A(2, 3);
    dynmat<f64> B(3, 2);
    dynmat<f64> C(2, 2, 0.0);
    f64 Avals[6] = { 1, 2, 3, 4, 5, 6 };
    f64 Bvals[6] = { 7, 8, 9, 10, 11, 12 };
    for ( usize i = 0; i < 6; ++i ) A[i] = Avals[i];
    for ( usize i = 0; i < 6; ++i ) B[i] = Bvals[i];
    blas::level3::gemm(f64(1.0), as_row_view(A), as_row_view(B), f64(0.0), as_row_view(C));
    require_true(near(C.at(0, 0), 58.0));
    require_true(near(C.at(0, 1), 64.0));
    require_true(near(C.at(1, 0), 139.0));
    require_true(near(C.at(1, 1), 154.0));
  }
  end_test_case();

  test_case("L3 gemm with α, β: C ← α·A·B + β·C on dynmat");
  {
    dynmat<f64> A(2, 2);
    dynmat<f64> B(2, 2);
    dynmat<f64> C(2, 2, 1.0);
    A[0] = 1;
    A[1] = 2;
    A[2] = 3;
    A[3] = 4;
    B[0] = 5;
    B[1] = 6;
    B[2] = 7;
    B[3] = 8;
    blas::level3::gemm(f64(2.0), as_row_view(A), as_row_view(B), f64(3.0), as_row_view(C));
    // A·B = [[19,22];[43,50]] → 2·A·B + 3·C = [[41,47];[89,103]]
    require_true(near(C.at(0, 0), 41.0));
    require_true(near(C.at(0, 1), 47.0));
    require_true(near(C.at(1, 0), 89.0));
    require_true(near(C.at(1, 1), 103.0));
  }
  end_test_case();

  test_case("dynmat round-trip: mat<T,R,C> → dynmat → row_view → gemm");
  {
    mat<f64, 2, 2> Af{};
    Af.data[0] = 1;
    Af.data[1] = 2;
    Af.data[2] = 3;
    Af.data[3] = 4;
    auto A = dynmat<f64>::from(Af);
    auto B = dynmat<f64>::identity(2);
    dynmat<f64> C(2, 2, 0.0);
    blas::level3::gemm(f64(1.0), as_row_view(A), as_row_view(B), f64(0.0), as_row_view(C));
    // A · I = A
    for ( usize i = 0; i < 4; ++i ) require_true(near(C[i], A[i]));
  }
  end_test_case();

  test_case("submat_view × row_view feeds gemm correctly");
  {
    // Pull a 2x2 sub-block from a 4x4 host and matmul it with another dynmat.
    dynmat<f64> H(4, 4, 0.0);
    // Set H[1..2][1..2] = [[1,2];[3,4]]; rest zero.
    H.at(1, 1) = 1;
    H.at(1, 2) = 2;
    H.at(2, 1) = 3;
    H.at(2, 2) = 4;
    auto S = submat_view(H, 1, 1, 2, 2);      // 2x2, ld=4
    dynmat<f64> B = dynmat<f64>::identity(2);
    dynmat<f64> C(2, 2, 0.0);
    blas::level3::gemm(f64(1.0), S, as_row_view(B), f64(0.0), as_row_view(C));
    require_true(near(C.at(0, 0), 1.0));
    require_true(near(C.at(0, 1), 2.0));
    require_true(near(C.at(1, 0), 3.0));
    require_true(near(C.at(1, 1), 4.0));
  }
  end_test_case();

  return 1;
}
