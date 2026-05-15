// svd_bdc_dnc.cpp
// Rigorous snowball test suite for Cuppen-style divide-and-conquer SVD.

#include "../../src/math/linalg/svd_bdc.hpp"
#include "../../src/math/sqrt.hpp"
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

template<typename F>
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

template<typename F>
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

template<typename F>
static m::dynmat<F>
transpose_dyn(const m::dynmat<F> &A) noexcept
{
  m::dynmat<F> T(A.cols, A.rows);
  for ( usize i = 0; i < A.rows; ++i )
    for ( usize j = 0; j < A.cols; ++j ) T.at(j, i) = A.at(i, j);
  return T;
}

template<typename F>
static bool
reconstruct_close(const m::dynmat<F> &A, const m::dynmat<F> &U, const m::dynvec<F> &S, const m::dynmat<F> &V, F tol) noexcept
{
  const usize R = A.rows;
  const usize C = A.cols;
  const usize K = (R < C) ? R : C;
  m::dynmat<F> US(R, C, F(0));
  for ( usize i = 0; i < R; ++i )
    for ( usize j = 0; j < K; ++j ) US.at(i, j) = U.at(i, j) * S[j];
  auto Vt = transpose_dyn(V);
  auto rec = gemm_dyn(US, Vt);
  m::dynmat<F> diff(R, C);
  for ( usize i = 0; i < R; ++i )
    for ( usize j = 0; j < C; ++j ) diff.at(i, j) = rec.at(i, j) - A.at(i, j);
  F nA = frob_norm(A);
  F nD = frob_norm(diff);
  return (nD <= tol * (nA + F(1)));
}

template<typename F>
static bool
orthogonal(const m::dynmat<F> &Q, F tol) noexcept
{
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
  sb::print("=== SVD_BDC_DNC TESTS (Cuppen tridiagonal D&C) ===");

  // ------------------------------------------------------------------
  test_case("4x4 SPD tridiagonal: reconstruction & orthogonality");
  {
    using F = double;
    m::dynmat<F> A(4, 4);
    F vals[16] = { 4, 1, 0, 0, 1, 4, 1, 0, 0, 1, 4, 1, 0, 0, 1, 4 };
    for ( usize i = 0; i < 16; ++i ) A.data()[i] = vals[i];

    // threshold 2 forces actual D&C recursion (rather than direct tqli)
    auto r = ml::decomp::svd_bdc_dnc<F>(A, 2);
    require_true(r.converged);
    for ( usize i = 0; i + 1 < r.S.size(); ++i ) require_true(r.S[i] >= r.S[i + 1]);
    for ( usize i = 0; i < r.S.size(); ++i ) require_true(r.S[i] >= F(0));
    require_true(orthogonal(r.U, F(1e-9)));
    require_true(orthogonal(r.V, F(1e-9)));
    require_true(reconstruct_close(A, r.U, r.S, r.V, F(1e-9)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("5x5 non-symmetric: cross-check vs QR SVD");
  {
    using F = double;
    m::dynmat<F> A(5, 5);
    F vals[25] = {
      3.1, 1.5, 0.8, 0.2, 0.0, 0.7, 4.0, 1.9, 1.1, 0.3, 0.2, 0.9, 2.5, 0.6, 0.4, 0.0, 0.5, 1.3, 3.7, 1.0, 0.1, 0.0, 0.4, 0.9, 2.2,
    };
    for ( usize i = 0; i < 25; ++i ) A.data()[i] = vals[i];

    auto rd = ml::decomp::svd_bdc_dnc<F>(A, 2);
    auto rq = ml::decomp::svd_bdc<F>(A);
    require_true(rd.converged);
    require_true(rq.converged);
    require_true(rd.S.size() == rq.S.size());
    for ( usize i = 0; i < rd.S.size(); ++i ) require_true(approx(rd.S[i], rq.S[i], F(1e-8)));
    require_true(orthogonal(rd.U, F(1e-8)));
    require_true(orthogonal(rd.V, F(1e-8)));
    require_true(reconstruct_close(A, rd.U, rd.S, rd.V, F(1e-8)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("6x4 tall: reconstruction");
  {
    using F = double;
    m::dynmat<F> A(6, 4);
    F vals[24] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 1, 0, 0, 1, 0, 1, 1, 0, 2, 3, 5, 7,
    };
    for ( usize i = 0; i < 24; ++i ) A.data()[i] = vals[i];

    auto r = ml::decomp::svd_bdc_dnc<F>(A, 2);
    require_true(r.converged);
    require_true(r.S.size() == 4);
    for ( usize i = 0; i + 1 < r.S.size(); ++i ) require_true(r.S[i] >= r.S[i + 1]);
    require_true(orthogonal(r.U, F(1e-8)));
    require_true(orthogonal(r.V, F(1e-8)));
    require_true(reconstruct_close(A, r.U, r.S, r.V, F(1e-8)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("4x6 wide: reconstruction (via transpose)");
  {
    using F = double;
    m::dynmat<F> A(4, 6);
    F vals[24] = {
      1, 5, 9, 1, 0, 2, 2, 6, 10, 0, 1, 3, 3, 7, 11, 0, 1, 5, 4, 8, 13, 1, 0, 7,
    };
    for ( usize i = 0; i < 24; ++i ) A.data()[i] = vals[i];

    auto r = ml::decomp::svd_bdc_dnc<F>(A, 2);
    require_true(r.converged);
    require_true(r.S.size() == 4);
    for ( usize i = 0; i + 1 < r.S.size(); ++i ) require_true(r.S[i] >= r.S[i + 1]);
    require_true(orthogonal(r.U, F(1e-8)));
    require_true(orthogonal(r.V, F(1e-8)));
    require_true(reconstruct_close(A, r.U, r.S, r.V, F(1e-8)));
  }
  end_test_case();

  sb::print("=== SVD_BDC_DNC PASSED ===");
  return 0;
}
