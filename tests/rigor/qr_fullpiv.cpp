// qr_fullpiv.cpp
// Rigorous snowball test suite for full-pivoting QR

#include "../../src/math/linalg/decomp.hpp"
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

int
main()
{
  sb::print("=== QR_FULLPIV TESTS ===");

  // ------------------------------------------------------------------
  test_case("3x3 full-rank: A * x_solved == b");
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

    m::vec<F, 3> x_true{ 5.0, -3.0, 7.0 };
    m::vec<F, 3> b{};
    for ( usize i = 0; i < 3; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 3; ++j ) s += A.data[i * 3 + j] * x_true.data[j];
      b.data[i] = s;
    }

    auto r = ml::decomp::qr_householder_fullpiv<F, 3, 3>(A);
    require_true(r.rank == 3);
    auto x = ml::decomp::qr_fullpiv_solve<F, 3, 3>(r, b);
    require_true(approx(x.data[0], x_true.data[0], F(1e-10)));
    require_true(approx(x.data[1], x_true.data[1], F(1e-10)));
    require_true(approx(x.data[2], x_true.data[2], F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3 rank-1 (outer product matrix): rank == 1");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    F u[3] = { 1.0, 2.0, 3.0 };
    F v[3] = { 4.0, 5.0, 6.0 };
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) A.data[i * 3 + j] = u[i] * v[j];

    auto r = ml::decomp::qr_householder_fullpiv<F, 3, 3>(A, F(1e-12));
    require_true(r.rank == 1);
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("solve full-rank 4x4: dynamic");
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
    auto r = ml::decomp::qr_householder_fullpiv<F>(A);
    require_true(r.rank == 4);
    auto x = ml::decomp::qr_fullpiv_solve<F>(r, b);
    for ( usize i = 0; i < 4; ++i ) require_true(approx(x[i], x_true[i], F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("4x4 dynamic rank-2: rank detected");
  {
    using F = double;
    m::dynmat<F> A(4, 4);
    F vals[16] = { 1, 2, 3, 4, 2, 4, 6, 8, 5, 7, 9, 11, 10, 14, 18, 22 };
    for ( usize i = 0; i < 16; ++i ) A.data()[i] = vals[i];

    auto r = ml::decomp::qr_householder_fullpiv<F>(A, F(1e-10));
    require_true(r.rank == 2);
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("non-symmetric general 4x4: solve");
  {
    using F = double;
    m::mat<F, 4, 4> A{};
    F vals[16] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9, 3 };
    for ( usize i = 0; i < 16; ++i ) A.data[i] = vals[i];

    m::vec<F, 4> x_true{ 1.0, -2.5, 0.7, 3.0 };
    m::vec<F, 4> b{};
    for ( usize i = 0; i < 4; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 4; ++j ) s += A.data[i * 4 + j] * x_true.data[j];
      b.data[i] = s;
    }
    auto r = ml::decomp::qr_householder_fullpiv<F, 4, 4>(A);
    require_true(r.rank == 4);
    auto x = ml::decomp::qr_fullpiv_solve<F, 4, 4>(r, b);
    for ( usize i = 0; i < 4; ++i ) require_true(approx(x.data[i], x_true.data[i], F(1e-9)));
  }
  end_test_case();

  sb::print("=== QR_FULLPIV PASSED ===");
  return 0;
}
