// qr_cod.cpp
// Rigorous snowball test suite for Complete Orthogonal Decomposition

#include "../../src/math/linalg/decomp.hpp"
#include "../../src/math/linalg/ops.hpp"
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
norm_dyn(const m::dynvec<F> &v) noexcept
{
  F s = F(0);
  for ( usize i = 0; i < v.size(); ++i ) s += v[i] * v[i];
  return micron::math::fsqrt(s);
}

int
main()
{
  sb::print("=== QR_COD TESTS ===");

  // ------------------------------------------------------------------
  test_case("full-rank 3x3: solve recovers x exactly");
  {
    using F = double;
    m::dynmat<F> A(3, 3);
    F vals[9] = { 2.0, -1.0, 0.0, -1.0, 2.0, -1.0, 0.0, -1.0, 2.0 };
    for ( usize i = 0; i < 9; ++i ) A.data()[i] = vals[i];

    m::dynvec<F> x_true(3);
    x_true[0] = 1.0;
    x_true[1] = 2.0;
    x_true[2] = 3.0;
    m::dynvec<F> b(3, F(0));
    for ( usize i = 0; i < 3; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 3; ++j ) s += A.at(i, j) * x_true[j];
      b[i] = s;
    }

    auto c = ml::decomp::cod<F>(A);
    require_true(c.rank == 3);
    auto x = ml::decomp::cod_solve<F>(c, b);
    for ( usize i = 0; i < 3; ++i ) require_true(approx(x[i], x_true[i], F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("rank-deficient: COD gives min-norm solution");
  {
    using F = double;
    // A is 4x3 with rank 2 (col 2 = col 0 + col 1).
    m::dynmat<F> A(4, 3);
    F vals[12] = { 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 2.0, 2.0, -1.0, 1.0 };
    for ( usize i = 0; i < 12; ++i ) A.data()[i] = vals[i];

    // b is in range of A:  b = A * [1; 2; 0]  (some valid x).
    m::dynvec<F> b(4, F(0));
    F x_pick[3] = { 1.0, 2.0, 0.0 };
    for ( usize i = 0; i < 4; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 3; ++j ) s += A.at(i, j) * x_pick[j];
      b[i] = s;
    }

    auto c = ml::decomp::cod<F>(A, F(1e-10));
    require_true(c.rank == 2);

    auto x_cod = ml::decomp::cod_solve<F>(c, b);

    // residual: ||A*x - b|| should be small (b is in range)
    m::dynvec<F> r(4, F(0));
    for ( usize i = 0; i < 4; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 3; ++j ) s += A.at(i, j) * x_cod[j];
      r[i] = s - b[i];
    }
    F res_norm = norm_dyn(r);
    require_true(res_norm < F(1e-9));

    // Minimum norm property: ||x_cod|| should be ≤ ||x_pick||
    // (any other valid x has the form x_pick + null-space vector)
    F x_cod_norm = norm_dyn(x_cod);
    F x_pick_norm = micron::math::fsqrt(F(1.0) + F(4.0));
    require_true(x_cod_norm <= x_pick_norm + F(1e-9));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("zero matrix: rank 0");
  {
    using F = double;
    m::dynmat<F> A = m::dynmat<F>::zero(3, 3);
    auto c = ml::decomp::cod<F>(A, F(1e-12));
    require_true(c.rank == 0);
    m::dynvec<F> b(3, F(0));
    b[0] = 1.0;
    b[1] = 2.0;
    b[2] = 3.0;
    auto x = ml::decomp::cod_solve<F>(c, b);
    // x must be 0 for rank 0
    for ( usize i = 0; i < 3; ++i ) require_true(approx(x[i], F(0), F(1e-15)));
  }
  end_test_case();

  sb::print("=== QR_COD PASSED ===");
  return 0;
}
