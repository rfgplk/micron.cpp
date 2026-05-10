// math_linalg_generic.cpp
// Snowball tests for math::linalg generic NxN operations.

#include "../../src/math/linalg.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math::linalg;

static bool
near(f64 a, f64 b, f64 eps = 1e-9)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== LINALG-GENERIC TESTS ===");

  test_case("trace");
  {
    mat<f64, 3, 3> A{};
    A.at(0, 0) = 1.0;
    A.at(1, 1) = 5.0;
    A.at(2, 2) = 9.0;
    require_true(near(trace<f64, 3>(A), 15.0));
  }
  end_test_case();

  test_case("det / inv 4×4 identity");
  {
    auto I = mat<f64, 4, 4>::identity();
    require_true(near(det<f64, 4>(I), 1.0));
    auto Ii = inv<f64, 4>(I);
    for ( usize i = 0; i < 4; ++i )
      for ( usize j = 0; j < 4; ++j ) require_true(near(Ii.at(i, j), I.at(i, j)));
  }
  end_test_case();

  test_case("solve 4×4");
  {
    mat<f64, 4, 4> A = mat<f64, 4, 4>::zero();
    A.at(0, 0) = 4;
    A.at(0, 1) = 1;
    A.at(0, 2) = 0;
    A.at(0, 3) = 0;
    A.at(1, 0) = 1;
    A.at(1, 1) = 4;
    A.at(1, 2) = 1;
    A.at(1, 3) = 0;
    A.at(2, 0) = 0;
    A.at(2, 1) = 1;
    A.at(2, 2) = 4;
    A.at(2, 3) = 1;
    A.at(3, 0) = 0;
    A.at(3, 1) = 0;
    A.at(3, 2) = 1;
    A.at(3, 3) = 4;
    vec<f64, 4> b{ 1.0, 0.0, 0.0, 1.0 };
    auto x = solve<f64, 4>(A, b);
    // Verify A·x ≈ b
    for ( usize i = 0; i < 4; ++i ) {
      f64 s = 0;
      for ( usize j = 0; j < 4; ++j ) s += A.at(i, j) * x.data[j];
      require_true(near(s, b.data[i], 1e-9));
    }
  }
  end_test_case();

  test_case("frobenius / inf / l1 norms");
  {
    mat<f64, 2, 2> A{};
    A.at(0, 0) = 1;
    A.at(0, 1) = 2;
    A.at(1, 0) = 3;
    A.at(1, 1) = 4;
    require_true(near(frobenius_norm(A), 5.477225575051661, 1e-10));
    require_true(near(norm_inf_mat(A), 7.0));     // max row-sum
    require_true(near(norm_l1_mat(A), 6.0));      // max col-sum
  }
  end_test_case();

  test_case("kron 2×2 ⊗ 2×2");
  {
    mat<f64, 2, 2> A{};
    A.at(0, 0) = 1;
    A.at(0, 1) = 2;
    A.at(1, 0) = 3;
    A.at(1, 1) = 4;
    mat<f64, 2, 2> B{};
    B.at(0, 0) = 0;
    B.at(0, 1) = 5;
    B.at(1, 0) = 6;
    B.at(1, 1) = 7;
    auto K = kron<f64, 2, 2, 2, 2>(A, B);
    // Expected (row-major):
    // [[ 0*1, 5*1, 0*2, 5*2],
    //  [ 6*1, 7*1, 6*2, 7*2],
    //  [ 0*3, 5*3, 0*4, 5*4],
    //  [ 6*3, 7*3, 6*4, 7*4]]
    require_true(near(K.at(0, 0), 0));
    require_true(near(K.at(0, 1), 5));
    require_true(near(K.at(1, 0), 6));
    require_true(near(K.at(2, 1), 15));
    require_true(near(K.at(3, 3), 28));
  }
  end_test_case();

  print("=== linalg-generic ok ===");
  return 0;
}
