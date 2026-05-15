// matfunc_pow.cpp
// Rigorous snowball test suite for matrix power (powmat).

#include "../../src/math/linalg/decomp.hpp"
#include "../../src/math/linalg/matfunc.hpp"
#include "../../src/math/linalg/matfunc_schur.hpp"
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

template<typename F, usize N>
static bool
mat_close(const m::mat<F, N, N> &A, const m::mat<F, N, N> &B, F tol) noexcept
{
  for ( usize i = 0; i < N * N; ++i )
    if ( !approx(A.data[i], B.data[i], tol) ) return false;
  return true;
}

template<typename F, usize N>
static m::mat<F, N, N>
gemm(const m::mat<F, N, N> &A, const m::mat<F, N, N> &B) noexcept
{
  m::mat<F, N, N> C{};
  for ( usize i = 0; i < N; ++i )
    for ( usize j = 0; j < N; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < N; ++k ) s += A.data[i * N + k] * B.data[k * N + j];
      C.data[i * N + j] = s;
    }
  return C;
}

int
main()
{
  sb::print("=== MATFUNC_POW TESTS ===");

  // ------------------------------------------------------------------
  test_case("3x3: powm(A, 0) == I");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 2.0;
    A.data[1] = 1.0;
    A.data[2] = 0.5;
    A.data[3] = 1.0;
    A.data[4] = 3.0;
    A.data[5] = 1.0;
    A.data[6] = 0.5;
    A.data[7] = 1.0;
    A.data[8] = 4.0;

    auto r = ml::schur::powmat<F, 3>(A, F(0));
    require_true(r.converged);
    auto I = m::mat<F, 3, 3>::identity();
    require_true(mat_close(r.X, I, F(1e-15)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3: powm(A, 2) == A*A");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 2.0;
    A.data[1] = 1.0;
    A.data[2] = 0.5;
    A.data[3] = 1.0;
    A.data[4] = 3.0;
    A.data[5] = 1.0;
    A.data[6] = 0.5;
    A.data[7] = 1.0;
    A.data[8] = 4.0;

    auto r = ml::schur::powmat<F, 3>(A, F(2));
    require_true(r.converged);
    auto A2 = gemm(A, A);
    require_true(mat_close(r.X, A2, F(1e-12)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3: powm(A, 5) == A*A*A*A*A");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 1.1;
    A.data[1] = 0.2;
    A.data[2] = 0.0;
    A.data[3] = 0.0;
    A.data[4] = 0.9;
    A.data[5] = 0.3;
    A.data[6] = 0.0;
    A.data[7] = 0.0;
    A.data[8] = 1.0;

    auto r = ml::schur::powmat<F, 3>(A, F(5));
    require_true(r.converged);
    auto A2 = gemm(A, A);
    auto A4 = gemm(A2, A2);
    auto A5 = gemm(A4, A);
    require_true(mat_close(r.X, A5, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3: powm(A, -1) == inv(A)");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 2.0;
    A.data[1] = 1.0;
    A.data[2] = 0.5;
    A.data[3] = 1.0;
    A.data[4] = 3.0;
    A.data[5] = 1.0;
    A.data[6] = 0.5;
    A.data[7] = 1.0;
    A.data[8] = 4.0;

    auto r = ml::schur::powmat<F, 3>(A, F(-1));
    require_true(r.converged);
    auto inv_r = ml::decomp::inv<F, 3>(A);
    require_true(!inv_r.singular);
    require_true(mat_close(r.X, inv_r.X, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("SPD 3x3: powm(A, 0.5)^2 == A");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 4.0;
    A.data[1] = 1.0;
    A.data[2] = 0.0;
    A.data[3] = 1.0;
    A.data[4] = 4.0;
    A.data[5] = 1.0;
    A.data[6] = 0.0;
    A.data[7] = 1.0;
    A.data[8] = 4.0;

    auto r = ml::schur::powmat<F, 3>(A, F(0.5));
    require_true(r.converged);
    auto sq = gemm(r.X, r.X);
    require_true(mat_close(sq, A, F(1e-9)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("SPD 3x3: powm(A, 0.5) * powm(A, -0.5) == I");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 4.0;
    A.data[1] = 1.0;
    A.data[2] = 0.0;
    A.data[3] = 1.0;
    A.data[4] = 4.0;
    A.data[5] = 1.0;
    A.data[6] = 0.0;
    A.data[7] = 1.0;
    A.data[8] = 4.0;

    auto r1 = ml::schur::powmat<F, 3>(A, F(0.5));
    auto r2 = ml::schur::powmat<F, 3>(A, F(-0.5));
    require_true(r1.converged);
    require_true(r2.converged);
    auto prod = gemm(r1.X, r2.X);
    auto I = m::mat<F, 3, 3>::identity();
    require_true(mat_close(prod, I, F(1e-9)));
  }
  end_test_case();

  sb::print("=== MATFUNC_POW PASSED ===");
  return 0;
}
