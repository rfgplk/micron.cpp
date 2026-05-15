// qr_colpiv.cpp
// Rigorous snowball test suite for column-pivoting QR + HouseholderSequence

#include "../../src/math/linalg/decomp.hpp"
#include "../../src/math/linalg/householder_seq.hpp"
#include "../../src/math/linalg/ops.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;
namespace ml = micron::math::linalg;

template<typename F>
static bool
approx(F a, F b, F tol) noexcept
{
  F d = a - b;
  if ( d < F(0) ) d = -d;
  return d <= tol;
}

template<typename F, usize R, usize C>
static bool
mat_approx(const m::mat<F, R, C> &a, const m::mat<F, R, C> &b, F tol) noexcept
{
  for ( usize i = 0; i < R * C; ++i )
    if ( !approx(a.data[i], b.data[i], tol) ) return false;
  return true;
}

template<typename F, usize R, usize C>
static m::mat<F, R, C>
permute_cols(const m::mat<F, R, C> &A, const usize *perm) noexcept
{
  m::mat<F, R, C> AP{};
  for ( usize i = 0; i < R; ++i )
    for ( usize j = 0; j < C; ++j ) AP.data[i * C + j] = A.data[i * C + perm[j]];
  return AP;
}

// dense GEMM for verification: C = A * B (no BLAS dep)
template<typename F, usize M, usize K, usize N>
static m::mat<F, M, N>
gemm_naive(const m::mat<F, M, K> &A, const m::mat<F, K, N> &B) noexcept
{
  m::mat<F, M, N> C{};
  for ( usize i = 0; i < M; ++i )
    for ( usize j = 0; j < N; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < K; ++k ) s += A.data[i * K + k] * B.data[k * N + j];
      C.data[i * N + j] = s;
    }
  return C;
}

int
main()
{
  sb::print("=== QR_COLPIV TESTS ===");

  // ------------------------------------------------------------------
  test_case("3x3 full-rank: A*P == Q*R");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 2.0;
    A.data[1] = 1.0;
    A.data[2] = 0.0;
    A.data[3] = 1.0;
    A.data[4] = 3.0;
    A.data[5] = 1.0;
    A.data[6] = 0.0;
    A.data[7] = 1.0;
    A.data[8] = 2.0;

    auto r = ml::decomp::qr_householder_colpiv<F, 3, 3>(A);
    require_true(r.rank == 3);

    auto Q = r.Q.explicit_form();
    auto QR = gemm_naive<F, 3, 3, 3>(Q, r.R);
    auto AP = permute_cols(A, r.perm);

    require_true(mat_approx(AP, QR, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3 rank-2 (singular column): rank detected, A*P == Q*R");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    // col 0 + col 1 = col 2  →  rank 2
    A.data[0] = 1.0;
    A.data[1] = 0.0;
    A.data[2] = 1.0;
    A.data[3] = 0.0;
    A.data[4] = 1.0;
    A.data[5] = 1.0;
    A.data[6] = 1.0;
    A.data[7] = 1.0;
    A.data[8] = 2.0;

    auto r = ml::decomp::qr_householder_colpiv<F, 3, 3>(A, F(1e-14));
    require_true(r.rank == 2);

    auto Q = r.Q.explicit_form();
    auto QR = gemm_naive<F, 3, 3, 3>(Q, r.R);
    auto AP = permute_cols(A, r.perm);
    require_true(mat_approx(AP, QR, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("Q^T * Q == I (orthogonality)");
  {
    using F = double;
    m::mat<F, 4, 3> A{};
    F vals[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 13, 17 };
    for ( int i = 0; i < 12; ++i ) A.data[i] = vals[i];

    auto r = ml::decomp::qr_householder_colpiv<F, 4, 3>(A);
    auto Q = r.Q.explicit_form();
    m::mat<F, 4, 4> Qt{};
    for ( usize i = 0; i < 4; ++i )
      for ( usize j = 0; j < 4; ++j ) Qt.data[i * 4 + j] = Q.data[j * 4 + i];
    auto QtQ = gemm_naive<F, 4, 4, 4>(Qt, Q);
    auto I = m::mat<F, 4, 4>::identity();
    require_true(mat_approx(QtQ, I, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("solve: full-rank 3x3 linear system");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 2.0;
    A.data[1] = -1.0;
    A.data[2] = 0.0;
    A.data[3] = -1.0;
    A.data[4] = 2.0;
    A.data[5] = -1.0;
    A.data[6] = 0.0;
    A.data[7] = -1.0;
    A.data[8] = 2.0;

    m::vec<F, 3> b{ 1.0, 0.0, 1.0 };
    auto r = ml::decomp::qr_householder_colpiv<F, 3, 3>(A);
    auto x = ml::decomp::qr_colpiv_solve<F, 3, 3>(r, b);

    // verify A * x == b
    m::vec<F, 3> Ax{};
    for ( usize i = 0; i < 3; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 3; ++j ) s += A.data[i * 3 + j] * x.data[j];
      Ax.data[i] = s;
    }
    require_true(approx(Ax.data[0], b.data[0], F(1e-10)));
    require_true(approx(Ax.data[1], b.data[1], F(1e-10)));
    require_true(approx(Ax.data[2], b.data[2], F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("dynamic: 4x4 SPD, solve recovers x");
  {
    using F = double;
    m::dynmat<F> A(4, 4);
    F vals[16] = { 4, 1, 0, 0, 1, 4, 1, 0, 0, 1, 4, 1, 0, 0, 1, 4 };
    for ( usize i = 0; i < 16; ++i ) A.data()[i] = vals[i];

    m::dynvec<F> x_true(4, F(0));
    x_true[0] = 1.0;
    x_true[1] = -2.0;
    x_true[2] = 3.0;
    x_true[3] = -4.0;
    m::dynvec<F> b(4, F(0));
    for ( usize i = 0; i < 4; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 4; ++j ) s += A.at(i, j) * x_true[j];
      b[i] = s;
    }
    auto r = ml::decomp::qr_householder_colpiv<F>(A);
    require_true(r.rank == 4);
    auto x = ml::decomp::qr_colpiv_solve<F>(r, b);
    for ( usize i = 0; i < 4; ++i ) require_true(approx(x[i], x_true[i], F(1e-10)));
  }
  end_test_case();

  sb::print("=== QR_COLPIV PASSED ===");
  return 0;
}
