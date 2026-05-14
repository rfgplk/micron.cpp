// lu_fullpiv.cpp
// Rigorous snowball test suite for full-pivoting LU

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
  sb::print("=== LU_FULLPIV TESTS ===");

  // ------------------------------------------------------------------
  test_case("3x3 full-rank: solve recovers x");
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

    m::vec<F, 3> x_true{ 1.0, 2.0, 3.0 };
    m::vec<F, 3> b{};
    for ( usize i = 0; i < 3; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < 3; ++j ) s += A.data[i * 3 + j] * x_true.data[j];
      b.data[i] = s;
    }

    auto r = ml::decomp::lu_pivot_full<F, 3>(A);
    require_true(!r.singular);
    require_true(r.rank == 3);

    auto x = ml::decomp::lu_full_solve<F, 3>(r, b);
    require_true(approx(x.data[0], x_true.data[0], F(1e-10)));
    require_true(approx(x.data[1], x_true.data[1], F(1e-10)));
    require_true(approx(x.data[2], x_true.data[2], F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3 rank-2 (singular): rank detected");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    // row 2 = row 0 + row 1
    A.data[0] = 1.0;
    A.data[1] = 2.0;
    A.data[2] = 3.0;
    A.data[3] = 4.0;
    A.data[4] = 5.0;
    A.data[5] = 6.0;
    A.data[6] = 5.0;
    A.data[7] = 7.0;
    A.data[8] = 9.0;

    auto r = ml::decomp::lu_pivot_full<F, 3>(A, F(1e-12));
    require_true(r.singular);
    require_true(r.rank == 2);
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("4x4 dynamic: solve recovers x");
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
    auto r = ml::decomp::lu_pivot_full<F>(A);
    require_true(!r.singular);
    require_true(r.rank == 4);
    auto x = ml::decomp::lu_full_solve<F>(r, b);
    for ( usize i = 0; i < 4; ++i ) require_true(approx(x[i], x_true[i], F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("4x4 dynamic rank-3: rank detected");
  {
    using F = double;
    m::dynmat<F> A(4, 4);
    // col 3 = col 0 + col 1
    F vals[16] = { 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 2 };
    for ( usize i = 0; i < 16; ++i ) A.data()[i] = vals[i];

    auto r = ml::decomp::lu_pivot_full<F>(A, F(1e-12));
    require_true(r.singular);
    require_true(r.rank == 3);
  }
  end_test_case();

  sb::print("=== LU_FULLPIV PASSED ===");
  return 0;
}
