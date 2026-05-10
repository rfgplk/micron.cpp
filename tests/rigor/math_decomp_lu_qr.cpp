// math_decomp_lu_qr.cpp — Snowball tests for pivoted LU, Householder QR,
// det / log_det, inverse, lu_solve, rcond.

#include "../../src/math/linalg/decomp.hpp"
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
  print("=== DECOMP LU / QR / DET / INV / RCOND ===");

  test_case("pivoted LU on a matrix where unpivoted LU diverges (zero leading pivot)");
  {
    // First pivot is 0: the unpivoted Doolittle would divide-by-zero.
    mat<f64, 3, 3> A{ { 0, 1, 2, 1, 2, 3, 2, 3, 5 } };
    auto lu = linalg::decomp::lu_pivot(A);
    require_true(!lu.singular);
    // PA == LU reconstruction
    for ( usize i = 0; i < 3; ++i ) {
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k <= ((i < j) ? i : j); ++k ) {
          f64 li = (k == i) ? 1.0 : (k < i) ? lu.LU.data[i * 3 + k] : 0.0;
          f64 uj = (k <= j) ? lu.LU.data[k * 3 + j] : 0.0;
          s += li * uj;
        }
        require_true(near(s, A.data[lu.perm[i] * 3 + j]));
      }
    }
  }
  end_test_case();

  test_case("lu_solve recovers x for A x = b");
  {
    mat<f64, 3, 3> A{ { 2, 1, 1, 4, 3, 3, 8, 7, 9 } };
    vec<f64, 3> b{ { 5, 10, 24 } };
    auto lu = linalg::decomp::lu_pivot(A);
    require_true(!lu.singular);
    vec<f64, 3> x = b;
    linalg::decomp::lu_solve(lu, x);
    // residual A x - b
    for ( usize i = 0; i < 3; ++i ) {
      f64 s = 0;
      for ( usize j = 0; j < 3; ++j ) s += A.data[i * 3 + j] * x.data[j];
      require_true(near(s, b.data[i]));
    }
  }
  end_test_case();

  test_case("det matches a hand-computed value");
  {
    mat<f64, 3, 3> A{ { 2, 1, 1, 4, 3, 3, 8, 7, 9 } };
    require_true(near(linalg::decomp::det(A), 4.0));
    // Identity → det = 1
    require_true(near(linalg::decomp::det(mat<f64, 4, 4>::identity()), 1.0));
    // Singular → det = 0
    mat<f64, 2, 2> S{ { 1, 2, 2, 4 } };
    require_true(near(linalg::decomp::det(S), 0.0));
  }
  end_test_case();

  test_case("log_det reconstructs |det| with the correct sign");
  {
    mat<f64, 3, 3> A{ { 2, 1, 1, 4, 3, 3, 8, 7, 9 } };
    auto ld = linalg::decomp::log_det(A);
    require_true(ld.sign == 1);
    f64 d = math::exp(ld.log_abs);
    require_true(near(d, 4.0));
  }
  end_test_case();

  test_case("inv: A · A⁻¹ = I");
  {
    mat<f64, 4, 4> A{ { 4, 3, 2, 1, 1, 4, 3, 2, 2, 1, 4, 3, 3, 2, 1, 4 } };
    auto inv = linalg::decomp::inv(A);
    require_true(!inv.singular);
    for ( usize i = 0; i < 4; ++i ) {
      for ( usize j = 0; j < 4; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 4; ++k ) s += A.data[i * 4 + k] * inv.X.data[k * 4 + j];
        require_true(near(s, (i == j) ? 1.0 : 0.0));
      }
    }
  }
  end_test_case();

  test_case("Householder QR: Q · R = A and Qᵀ · Q = I");
  {
    mat<f64, 4, 3> A{ { 1, 2, 3, 4, 5, 6, 7, 8, 10, 1, 1, 1 } };
    auto qr = linalg::decomp::qr_householder(A);
    // Q * R == A
    for ( usize i = 0; i < 4; ++i ) {
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 4; ++k ) s += qr.Q.data[i * 4 + k] * qr.R.data[k * 3 + j];
        require_true(near(s, A.data[i * 3 + j]));
      }
    }
    // Qᵀ * Q == I (orthonormality)
    for ( usize i = 0; i < 4; ++i ) {
      for ( usize j = 0; j < 4; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 4; ++k ) s += qr.Q.data[k * 4 + i] * qr.Q.data[k * 4 + j];
        require_true(near(s, (i == j) ? 1.0 : 0.0));
      }
    }
    // R upper triangular
    for ( usize i = 0; i < 4; ++i )
      for ( usize j = 0; j < 3 && j < i; ++j ) require_true(near(qr.R.data[i * 3 + j], 0.0));
  }
  end_test_case();

  test_case("dynamic LU/det/inv match the fixed-size results");
  {
    mat<f64, 3, 3> A{ { 2, 1, 1, 4, 3, 3, 8, 7, 9 } };
    auto Ad = dynmat<f64>::from(A);
    require_true(near(linalg::decomp::det(Ad), 4.0));
    auto inv_d = linalg::decomp::inv(Ad);
    require_true(!inv_d.singular);
    for ( usize i = 0; i < 3; ++i ) {
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += Ad.at(i, k) * inv_d.X.at(k, j);
        require_true(near(s, (i == j) ? 1.0 : 0.0));
      }
    }
  }
  end_test_case();

  test_case("dynamic Householder QR: Q · R = A");
  {
    dynmat<f64> A(4, 3);
    f64 v[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 10, 1, 1, 1 };
    for ( usize i = 0; i < 12; ++i ) A.data()[i] = v[i];
    auto qr = linalg::decomp::qr_householder(A);
    require_true(qr.Q.rows == 4 && qr.Q.cols == 4);
    require_true(qr.R.rows == 4 && qr.R.cols == 3);
    for ( usize i = 0; i < 4; ++i ) {
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 4; ++k ) s += qr.Q.at(i, k) * qr.R.at(k, j);
        require_true(near(s, A.at(i, j)));
      }
    }
  }
  end_test_case();

  test_case("rcond: identity = 1, singular = 0, Hilbert(3) ≈ 1/524");
  {
    require_true(near(linalg::decomp::rcond(mat<f64, 3, 3>::identity()), 1.0));
    mat<f64, 2, 2> S{ { 1, 2, 2, 4 } };
    require_true(linalg::decomp::rcond(S) == 0.0);
    // Hilbert 3x3: cond_1 ≈ 524, so rcond ≈ 0.0019
    mat<f64, 3, 3> H{ { 1.0, 0.5, 1.0 / 3.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 3.0, 0.25, 0.2 } };
    f64 r = linalg::decomp::rcond(H);
    require_true(r > 0.0 && r < 0.01);
  }
  end_test_case();

  return 0;
}
