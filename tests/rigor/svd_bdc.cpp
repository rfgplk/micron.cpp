// svd_bdc.cpp
// Rigorous snowball test suite for bidiagonal-D&C SVD (Golub-Reinsch QR path).

#include "../../src/math/linalg/svd_bdc.hpp"
#include "../../src/math/linalg/pseudoinv.hpp"
#include "../../src/math/sqrt.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;
namespace ml = micron::math::linalg;

template <typename F>
static bool
approx(F a, F b, F tol) noexcept
{
  F d = a - b;
  if ( d < F(0) ) d = -d;
  return d <= tol;
}

template <typename F>
static F
frob_norm(const m::dynmat<F> &A) noexcept
{
  F s = F(0);
  for ( usize i = 0; i < A.rows; ++i )
    for ( usize j = 0; j < A.cols; ++j ) {
      F v = A.at(i, j);
      s += v * v;
    }
  return micron::math::fsqrt(s);
}

template <typename F>
static m::dynmat<F>
gemm_dyn(const m::dynmat<F> &A, const m::dynmat<F> &B) noexcept
{
  m::dynmat<F> C(A.rows, B.cols);
  for ( usize i = 0; i < A.rows; ++i )
    for ( usize j = 0; j < B.cols; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < A.cols; ++k ) s += A.at(i, k) * B.at(k, j);
      C.at(i, j) = s;
    }
  return C;
}

template <typename F>
static m::dynmat<F>
transpose_dyn(const m::dynmat<F> &A) noexcept
{
  m::dynmat<F> T(A.cols, A.rows);
  for ( usize i = 0; i < A.rows; ++i )
    for ( usize j = 0; j < A.cols; ++j ) T.at(j, i) = A.at(i, j);
  return T;
}

template <typename F>
static bool
reconstruct_close(const m::dynmat<F> &A, const m::dynmat<F> &U, const m::dynvec<F> &S, const m::dynmat<F> &V, F tol) noexcept
{
  // build US: R x C with US[i, j] = U[i, j] * S[j]   for j < min(R,C); else 0
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  m::dynmat<F> US(R, C, F(0));
  for ( usize i = 0; i < R; ++i ) {
    for ( usize j = 0; j < K; ++j ) US.at(i, j) = U.at(i, j) * S[j];
  }
  auto Vt = transpose_dyn(V);
  auto rec = gemm_dyn(US, Vt);
  // compare rec vs A
  m::dynmat<F> diff(R, C);
  for ( usize i = 0; i < R; ++i )
    for ( usize j = 0; j < C; ++j ) diff.at(i, j) = rec.at(i, j) - A.at(i, j);
  F nA = frob_norm(A);
  F nD = frob_norm(diff);
  return (nD <= tol * (nA + F(1)));
}

template <typename F>
static bool
orthogonal(const m::dynmat<F> &Q, F tol) noexcept
{
  // Q^T Q ≈ I
  auto Qt = transpose_dyn(Q);
  auto p = gemm_dyn(Qt, Q);
  for ( usize i = 0; i < p.rows; ++i )
    for ( usize j = 0; j < p.cols; ++j ) {
      F exp = (i == j) ? F(1) : F(0);
      if ( !approx(p.at(i, j), exp, tol) ) return false;
    }
  return true;
}

int
main()
{
  sb::print("=== SVD_BDC TESTS ===");

  // ------------------------------------------------------------------
  test_case("3x3 SPD: reconstruction & orthogonality");
  {
    using F = double;
    m::dynmat<F> A(3, 3);
    F vals[9] = { 4.0, 1.0, 0.0, 1.0, 5.0, 1.0, 0.0, 1.0, 6.0 };
    for ( usize i = 0; i < 9; ++i ) A.data()[i] = vals[i];

    auto r = ml::decomp::svd_bdc<F>(A);
    require_true(r.converged);
    // singular values descending and non-negative
    require_true(r.S[0] >= r.S[1]);
    require_true(r.S[1] >= r.S[2]);
    require_true(r.S[2] >= F(0));
    require_true(orthogonal(r.U, F(1e-10)));
    require_true(orthogonal(r.V, F(1e-10)));
    require_true(reconstruct_close(A, r.U, r.S, r.V, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("5x5 non-symmetric: reconstruction");
  {
    using F = double;
    m::dynmat<F> A(5, 5);
    F vals[25] = {
      3.1, 1.5, 0.8, 0.2, 0.0, 0.7, 4.0, 1.9, 1.1, 0.3, 0.2, 0.9, 2.5, 0.6, 0.4, 0.0, 0.5, 1.3, 3.7, 1.0, 0.1, 0.0, 0.4, 0.9, 2.2,
    };
    for ( usize i = 0; i < 25; ++i ) A.data()[i] = vals[i];

    auto r = ml::decomp::svd_bdc<F>(A);
    require_true(r.converged);
    for ( usize i = 0; i + 1 < r.S.size(); ++i ) require_true(r.S[i] >= r.S[i + 1]);
    for ( usize i = 0; i < r.S.size(); ++i ) require_true(r.S[i] >= F(0));
    require_true(orthogonal(r.U, F(1e-9)));
    require_true(orthogonal(r.V, F(1e-9)));
    require_true(reconstruct_close(A, r.U, r.S, r.V, F(1e-9)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("6x4 tall: reconstruction");
  {
    using F = double;
    m::dynmat<F> A(6, 4);
    F vals[24] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13,     // perturbed to avoid singular
      1, 0, 0, 1, 0, 1, 1, 0, 2, 3,  5,  7,
    };
    for ( usize i = 0; i < 24; ++i ) A.data()[i] = vals[i];

    auto r = ml::decomp::svd_bdc<F>(A);
    require_true(r.converged);
    require_true(r.S.size() == 4);
    for ( usize i = 0; i + 1 < r.S.size(); ++i ) require_true(r.S[i] >= r.S[i + 1]);
    require_true(orthogonal(r.U, F(1e-9)));
    require_true(orthogonal(r.V, F(1e-9)));
    require_true(reconstruct_close(A, r.U, r.S, r.V, F(1e-9)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("cross-check vs Jacobi pseudoinv::svd on a 4x4");
  {
    using F = double;
    m::dynmat<F> A(4, 4);
    F vals[16] = {
      2, 1, 0, 0, 1, 3, 1, 0, 0, 1, 4, 1, 0, 0, 1, 5,
    };
    for ( usize i = 0; i < 16; ++i ) A.data()[i] = vals[i];

    auto rb = ml::decomp::svd_bdc<F>(A);
    auto rj = ml::pseudoinv::svd<F>(A);
    require_true(rb.converged);
    // Jacobi's converged flag is strict (off-diag sum < machine eps); singular
    // values are correct regardless.  Compare the values only.
    require_true(rb.S.size() == rj.S.size());
    for ( usize i = 0; i < rb.S.size(); ++i ) require_true(approx(rb.S[i], rj.S[i], F(1e-9)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("rank-deficient 4x4: small singular value");
  {
    using F = double;
    m::dynmat<F> A(4, 4);
    // A = u v^T + tiny perturbation gives rank ~ 1 with three tiny svs
    F u[4] = { 1.0, 2.0, 3.0, 4.0 };
    F v[4] = { 5.0, 6.0, 7.0, 8.0 };
    for ( usize i = 0; i < 4; ++i )
      for ( usize j = 0; j < 4; ++j ) A.at(i, j) = u[i] * v[j];

    auto r = ml::decomp::svd_bdc<F>(A);
    require_true(r.converged);
    // largest singular value ≈ |u|·|v|
    F nu = micron::math::fsqrt(F(1.0 + 4.0 + 9.0 + 16.0));
    F nv = micron::math::fsqrt(F(25.0 + 36.0 + 49.0 + 64.0));
    require_true(approx(r.S[0], nu * nv, F(1e-9)));
    // others small
    require_true(r.S[1] < F(1e-9));
    require_true(r.S[2] < F(1e-9));
    require_true(r.S[3] < F(1e-9));
    require_true(reconstruct_close(A, r.U, r.S, r.V, F(1e-9)));
  }
  end_test_case();

  sb::print("=== SVD_BDC PASSED ===");
  return 0;
}
