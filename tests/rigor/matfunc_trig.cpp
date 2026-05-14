// matfunc_trig.cpp
// Rigorous snowball test suite for matrix sin/cos/sinh/cosh.

#include "../../src/math/linalg/decomp.hpp"
#include "../../src/math/linalg/matfunc.hpp"
#include "../../src/math/linalg/matfunc_schur.hpp"
#include "../../src/math/linalg/ops.hpp"
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

template <typename F, usize N>
static bool
mat_close(const m::mat<F, N, N> &A, const m::mat<F, N, N> &B, F tol) noexcept
{
  for ( usize i = 0; i < N * N; ++i )
    if ( !approx(A.data[i], B.data[i], tol) ) return false;
  return true;
}

template <typename F, usize N>
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

template <typename F, usize N>
static m::mat<F, N, N>
add(const m::mat<F, N, N> &A, const m::mat<F, N, N> &B) noexcept
{
  m::mat<F, N, N> C{};
  for ( usize i = 0; i < N * N; ++i ) C.data[i] = A.data[i] + B.data[i];
  return C;
}

template <typename F, usize N>
static m::mat<F, N, N>
sub(const m::mat<F, N, N> &A, const m::mat<F, N, N> &B) noexcept
{
  m::mat<F, N, N> C{};
  for ( usize i = 0; i < N * N; ++i ) C.data[i] = A.data[i] - B.data[i];
  return C;
}

template <typename F, usize N>
static m::mat<F, N, N>
scal(const m::mat<F, N, N> &A, F s) noexcept
{
  m::mat<F, N, N> C{};
  for ( usize i = 0; i < N * N; ++i ) C.data[i] = A.data[i] * s;
  return C;
}

int
main()
{
  sb::print("=== MATFUNC_TRIG TESTS ===");

  // ------------------------------------------------------------------
  test_case("symmetric 3x3: sin^2(A) + cos^2(A) == I");
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

    auto sr = ml::schur::sinmat<F, 3>(A);
    auto cr = ml::schur::cosmat<F, 3>(A);
    require_true(sr.converged);
    require_true(cr.converged);

    auto s2 = gemm<F, 3>(sr.X, sr.X);
    auto c2 = gemm<F, 3>(cr.X, cr.X);
    auto sum = add(s2, c2);
    auto I = m::mat<F, 3, 3>::identity();
    require_true(mat_close(sum, I, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("symmetric 3x3: cos(2A) == cos^2(A) - sin^2(A)");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 0.5;
    A.data[1] = 0.1;
    A.data[2] = 0.0;
    A.data[3] = 0.1;
    A.data[4] = 0.7;
    A.data[5] = 0.2;
    A.data[6] = 0.0;
    A.data[7] = 0.2;
    A.data[8] = 0.3;

    auto sr = ml::schur::sinmat<F, 3>(A);
    auto cr = ml::schur::cosmat<F, 3>(A);
    require_true(sr.converged);
    require_true(cr.converged);

    auto A2 = scal(A, F(2));
    auto cr2 = ml::schur::cosmat<F, 3>(A2);
    require_true(cr2.converged);

    auto c2_minus_s2 = sub(gemm<F, 3>(cr.X, cr.X), gemm<F, 3>(sr.X, sr.X));
    require_true(mat_close(cr2.X, c2_minus_s2, F(1e-9)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("symmetric 3x3: cosh^2(A) - sinh^2(A) == I");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 0.1;
    A.data[1] = 0.05;
    A.data[2] = 0.0;
    A.data[3] = 0.05;
    A.data[4] = 0.2;
    A.data[5] = 0.05;
    A.data[6] = 0.0;
    A.data[7] = 0.05;
    A.data[8] = 0.15;

    auto shr = ml::schur::sinhmat<F, 3>(A);
    auto chr = ml::schur::coshmat<F, 3>(A);
    require_true(shr.converged);
    require_true(chr.converged);

    auto sh2 = gemm<F, 3>(shr.X, shr.X);
    auto ch2 = gemm<F, 3>(chr.X, chr.X);
    auto diff = sub(ch2, sh2);
    auto I = m::mat<F, 3, 3>::identity();
    require_true(mat_close(diff, I, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3: cosh(A) == (expm(A) + expm(-A))/2");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    A.data[0] = 0.3;
    A.data[1] = 0.1;
    A.data[2] = 0.0;
    A.data[3] = 0.0;
    A.data[4] = 0.4;
    A.data[5] = 0.2;
    A.data[6] = 0.0;
    A.data[7] = 0.0;
    A.data[8] = 0.5;

    auto chr = ml::schur::coshmat<F, 3>(A);
    require_true(chr.converged);

    auto exp_pos = ml::schur::expmat<F, 3>(A);
    auto neg = scal(A, F(-1));
    auto exp_neg = ml::schur::expmat<F, 3>(neg);
    require_true(exp_pos.converged);
    require_true(exp_neg.converged);
    auto half = scal(add(exp_pos.X, exp_neg.X), F(0.5));
    require_true(mat_close(chr.X, half, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("3x3 non-symmetric: sin(2A) == 2 sin(A) cos(A)");
  {
    using F = double;
    m::mat<F, 3, 3> A{};
    // mild non-symmetric with real eigenvalues
    A.data[0] = 0.3;
    A.data[1] = 0.5;
    A.data[2] = 0.1;
    A.data[3] = 0.0;
    A.data[4] = 0.6;
    A.data[5] = 0.4;
    A.data[6] = 0.0;
    A.data[7] = 0.0;
    A.data[8] = 0.2;

    auto sr = ml::schur::sinmat<F, 3>(A);
    auto cr = ml::schur::cosmat<F, 3>(A);
    require_true(sr.converged);
    require_true(cr.converged);

    auto A2 = scal(A, F(2));
    auto sr2 = ml::schur::sinmat<F, 3>(A2);
    require_true(sr2.converged);

    auto two_sc = scal(gemm<F, 3>(sr.X, cr.X), F(2));
    require_true(mat_close(sr2.X, two_sc, F(1e-9)));
  }
  end_test_case();

  sb::print("=== MATFUNC_TRIG PASSED ===");
  return 0;
}
