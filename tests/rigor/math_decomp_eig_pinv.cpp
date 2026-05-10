// math_decomp_eig_pinv.cpp — Snowball tests for the symmetric
// tridiag+QL eigendecomposition and the SVD-derived utilities
// (pinv, rank, null, orth).

#include "../../src/math/linalg/decomp_ext.hpp"
#include "../../src/math/linalg/pseudoinv.hpp"
#include "../../src/math/matrix/matrices.hpp"
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
  print("=== DECOMP EIG / PINV ===");

  test_case("eigen_sym_qr on the 4×4 tridiag-Toeplitz [-1, 2, -1]");
  {
    mat<f64, 4, 4> A{};
    A.data[0] = 2;
    A.data[1] = -1;
    A.data[4] = -1;
    A.data[5] = 2;
    A.data[6] = -1;
    A.data[9] = -1;
    A.data[10] = 2;
    A.data[11] = -1;
    A.data[14] = -1;
    A.data[15] = 2;
    auto er = linalg::decomp::eigen_sym_qr(A);
    require_true(er.converged);
    f64 expected[4] = { 0.38196601125010515, 1.3819660112501051, 2.618033988749895, 3.6180339887498949 };
    for ( usize i = 0; i < 4; ++i ) require_true(near(er.values.data[i], expected[i]));
    // A · v = λ · v for every eigenpair
    for ( usize i = 0; i < 4; ++i ) {
      f64 lambda = er.values.data[i];
      for ( usize r = 0; r < 4; ++r ) {
        f64 s = 0;
        for ( usize c = 0; c < 4; ++c ) s += A.data[r * 4 + c] * er.vectors.data[c * 4 + i];
        require_true(near(s, lambda * er.vectors.data[r * 4 + i]));
      }
    }
  }
  end_test_case();

  test_case("eigen_sym_qr (dynamic) on a 6×6 symmetric matrix");
  {
    dynmat<f64> A(6, 6);
    f64 v[36];
    for ( usize i = 0; i < 36; ++i ) v[i] = f64((i * 17 + 5) % 11) - 5.0;
    for ( usize i = 0; i < 6; ++i )
      for ( usize j = 0; j < 6; ++j ) A.at(i, j) = 0.5 * (v[i * 6 + j] + v[j * 6 + i]);
    auto er = linalg::decomp::eigen_sym_qr(A);
    require_true(er.converged);
    // ascending order
    for ( usize i = 0; i + 1 < 6; ++i ) require_true(er.values[i] <= er.values[i + 1] + 1e-12);
    // residual A · v_i = λ_i · v_i
    for ( usize i = 0; i < 6; ++i ) {
      f64 lambda = er.values[i];
      for ( usize r = 0; r < 6; ++r ) {
        f64 s = 0;
        for ( usize c = 0; c < 6; ++c ) s += A.at(r, c) * er.vectors.at(c, i);
        require_true(near(s, lambda * er.vectors.at(r, i), 1e-9));
      }
    }
  }
  end_test_case();

  test_case("pinv (fixed): A · A⁺ · A = A on a tall full-rank matrix");
  {
    mat<f64, 4, 3> A{ { 1, 2, 3, 4, 5, 6, 7, 8, 10, 1, 1, 1 } };
    auto P = linalg::pseudoinv::pinv(A);
    // PA (3x3)
    mat<f64, 3, 3> PA{};
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 4; ++k ) s += P.data[i * 4 + k] * A.data[k * 3 + j];
        PA.data[i * 3 + j] = s;
      }
    // A · PA == A
    for ( usize i = 0; i < 4; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += A.data[i * 3 + k] * PA.data[k * 3 + j];
        require_true(near(s, A.data[i * 3 + j], 1e-9));
      }
  }
  end_test_case();

  test_case("pinv (dynamic): handles wide A by transposing internally");
  {
    // 3x4 matrix — wide; pinv should be 4x3.
    dynmat<f64> A(3, 4);
    f64 vals[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    for ( usize i = 0; i < 12; ++i ) A.data()[i] = vals[i];
    auto P = linalg::pseudoinv::pinv(A);
    require_true(P.rows == 4 && P.cols == 3);
    // A · P · A == A   (note: 3x4 * 4x3 * 3x4 = 3x4)
    dynmat<f64> AP(3, 3);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 4; ++k ) s += A.at(i, k) * P.at(k, j);
        AP.at(i, j) = s;
      }
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 4; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += AP.at(i, k) * A.at(k, j);
        require_true(near(s, A.at(i, j), 1e-9));
      }
  }
  end_test_case();

  test_case("rank correctly identifies a rank-2 4×3 matrix");
  {
    // col 3 = col 1 + col 2 → rank 2
    mat<f64, 4, 3> A{};
    A.data[0] = 1;
    A.data[1] = 2;
    A.data[2] = 3;
    A.data[3] = 4;
    A.data[4] = 5;
    A.data[5] = 9;
    A.data[6] = 7;
    A.data[7] = 8;
    A.data[8] = 15;
    A.data[9] = 1;
    A.data[10] = 1;
    A.data[11] = 2;
    require_true(linalg::pseudoinv::rank(A) == 2);
  }
  end_test_case();

  test_case("null and orth: A · null(A) = 0,  orth(A) is column space");
  {
    dynmat<f64> A(4, 3);
    f64 vals[12] = { 1, 2, 3, 4, 5, 9, 7, 8, 15, 1, 1, 2 };     // rank 2
    for ( usize i = 0; i < 12; ++i ) A.data()[i] = vals[i];
    auto N = linalg::pseudoinv::null(A);
    require_true(N.cols == 1);
    for ( usize i = 0; i < 4; ++i ) {
      f64 s = 0;
      for ( usize k = 0; k < 3; ++k ) s += A.at(i, k) * N.at(k, 0);
      require_true(near(s, 0.0));
    }
    auto O = linalg::pseudoinv::orth(A);
    require_true(O.cols == 2);
    // orthonormality: Oᵀ · O = I_2
    for ( usize i = 0; i < 2; ++i )
      for ( usize j = 0; j < 2; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 4; ++k ) s += O.at(k, i) * O.at(k, j);
        require_true(near(s, (i == j) ? 1.0 : 0.0));
      }
  }
  end_test_case();

  return 0;
}
