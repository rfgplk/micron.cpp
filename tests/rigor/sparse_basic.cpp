// sparse_basic.cpp
// Rigorous snowball test suite for the sparse module.

#include "../../src/math/sparse.hpp"
#include "../../src/math/sqrt.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;
namespace ms = micron::math::sparse;

template<typename F>
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
  sb::print("=== SPARSE BASIC TESTS ===");

  // ------------------------------------------------------------------
  test_case("csc construction from sorted triplets");
  {
    using F = double;
    using I = u32;
    // A 3x3 diagonal + one off-diag entry
    //   A = [4 0 1
    //        0 5 0
    //        0 0 6]
    I rows[] = { 0, 1, 0, 2 };
    I cols[] = { 0, 1, 2, 2 };
    F vals[] = { 4.0, 5.0, 1.0, 6.0 };
    auto A = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, vals, 4);
    require_true(A.nnz() == 4);
    require_true(A.col_nnz(0) == 1);
    require_true(A.col_nnz(1) == 1);
    require_true(A.col_nnz(2) == 2);
    require_true(approx(A.values.data()[0], 4.0, 1e-15));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("SIMD axpy: y := alpha*x + y for length 100");
  {
    using F = double;
    micron::vector<F, micron::allocator_serial<>, false> x(100, F(0));
    micron::vector<F, micron::allocator_serial<>, false> y(100, F(0));
    for ( usize i = 0; i < 100; ++i ) {
      x.data()[i] = F(i);
      y.data()[i] = F(i) * F(0.5);
    }
    ms::axpy_values<F>(2.0, x.data(), y.data(), 100);
    // y[i] should equal 2*i + 0.5*i = 2.5*i
    for ( usize i = 0; i < 100; ++i ) require_true(approx(y.data()[i], F(2.5) * F(i), F(1e-12)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("SIMD dot: sum_i x[i]*y[i]");
  {
    using F = double;
    micron::vector<F, micron::allocator_serial<>, false> x(50, F(0));
    micron::vector<F, micron::allocator_serial<>, false> y(50, F(0));
    F expected = F(0);
    for ( usize i = 0; i < 50; ++i ) {
      x.data()[i] = F(i) + F(1);
      y.data()[i] = F(50 - i);
      expected += x.data()[i] * y.data()[i];
    }
    F got = ms::dot_values<F>(x.data(), y.data(), 50);
    require_true(approx(got, expected, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("SIMD norm_sq + frobenius");
  {
    using F = double;
    micron::vector<F, micron::allocator_serial<>, false> a(33, F(0));
    F expected = F(0);
    for ( usize i = 0; i < 33; ++i ) {
      a.data()[i] = F(i) - F(16);
      expected += a.data()[i] * a.data()[i];
    }
    F got = ms::norm_sq_values<F>(a.data(), 33);
    require_true(approx(got, expected, F(1e-10)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("SIMD f32 axpy");
  {
    using F = float;
    micron::vector<F, micron::allocator_serial<>, false> x(64, F(0));
    micron::vector<F, micron::allocator_serial<>, false> y(64, F(0));
    for ( usize i = 0; i < 64; ++i ) {
      x.data()[i] = F(i);
      y.data()[i] = F(1);
    }
    ms::axpy_values<F>(F(0.5), x.data(), y.data(), 64);
    for ( usize i = 0; i < 64; ++i ) require_true(approx(y.data()[i], F(0.5) * F(i) + F(1), F(1e-5)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("spmv: y := A * x  (3x3 diagonal + off-diag)");
  {
    using F = double;
    using I = u32;
    // A = [4 0 1; 0 5 0; 0 0 6]; x = [1, 2, 3] → y = [4+3, 10, 18] = [7, 10, 18]
    I rows[] = { 0, 1, 0, 2 };
    I cols[] = { 0, 1, 2, 2 };
    F vals[] = { 4.0, 5.0, 1.0, 6.0 };
    auto A = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, vals, 4);

    m::dynvec<F> x(3, F(0));
    x[0] = 1.0;
    x[1] = 2.0;
    x[2] = 3.0;
    m::dynvec<F> y(3, F(0));
    ms::spmv<F, I>(F(1), A, x, F(0), y);
    require_true(approx(y[0], 7.0, 1e-12));
    require_true(approx(y[1], 10.0, 1e-12));
    require_true(approx(y[2], 18.0, 1e-12));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("spmv_transposed: y := A^T * x");
  {
    using F = double;
    using I = u32;
    // A = [4 0 1; 0 5 0; 0 0 6]; A^T = [4 0 0; 0 5 0; 1 0 6]; x = [1, 2, 3]
    // A^T x = [4, 10, 1+18] = [4, 10, 19]
    I rows[] = { 0, 1, 0, 2 };
    I cols[] = { 0, 1, 2, 2 };
    F vals[] = { 4.0, 5.0, 1.0, 6.0 };
    auto A = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, vals, 4);

    m::dynvec<F> x(3, F(0));
    x[0] = 1.0;
    x[1] = 2.0;
    x[2] = 3.0;
    m::dynvec<F> y(3, F(0));
    ms::spmv_transposed<F, I>(F(1), A, x, F(0), y);
    require_true(approx(y[0], 4.0, 1e-12));
    require_true(approx(y[1], 10.0, 1e-12));
    require_true(approx(y[2], 19.0, 1e-12));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("spgemm: A * A");
  {
    using F = double;
    using I = u32;
    // A = [2 1 0; 1 2 1; 0 1 2]  (tridiagonal SPD)
    // A^2 = A*A = [4+1 2+2 1; 2+2 1+4+1 2+2; 1 2+2 1+4] = [5 4 1; 4 6 4; 1 4 5]
    I rows[] = { 0, 1, 0, 1, 2, 1, 2 };
    I cols[] = { 0, 0, 1, 1, 1, 2, 2 };
    F vals[] = { 2.0, 1.0, 1.0, 2.0, 1.0, 1.0, 2.0 };
    auto A = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, vals, 7);
    auto A2 = ms::spgemm<F, I>(A, A);
    require_true(A2.rows == 3);
    require_true(A2.cols == 3);
    // verify by spmv:  A2 * e_1 = (4, 6, 4)
    m::dynvec<F> e1(3, F(0));
    e1[1] = 1.0;
    m::dynvec<F> y(3, F(0));
    ms::spmv<F, I>(F(1), A2, e1, F(0), y);
    require_true(approx(y[0], 4.0, 1e-12));
    require_true(approx(y[1], 6.0, 1e-12));
    require_true(approx(y[2], 4.0, 1e-12));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("triangular: L*x = b forward solve");
  {
    using F = double;
    using I = u32;
    // L (non-unit) = [2 0 0; 1 3 0; 4 0 5]
    I rows[] = { 0, 1, 2, 1, 2 };
    I cols[] = { 0, 0, 0, 1, 2 };
    F vals[] = { 2.0, 1.0, 4.0, 3.0, 5.0 };
    auto L = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, vals, 5);
    // pick x = (1, 2, 3); b = L*x = (2, 1+6, 4+15) = (2, 7, 19)
    m::dynvec<F> b(3, F(0));
    b[0] = 2.0;
    b[1] = 7.0;
    b[2] = 19.0;
    bool ok = ms::solve_lower<F, I>(L, b, ms::triangular_tag::lower_non_unit{});
    require_true(ok);
    require_true(approx(b[0], 1.0, 1e-12));
    require_true(approx(b[1], 2.0, 1e-12));
    require_true(approx(b[2], 3.0, 1e-12));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("triangular: U*x = b backward solve");
  {
    using F = double;
    using I = u32;
    // U (non-unit) = [2 1 4; 0 3 0; 0 0 5]
    I rows[] = { 0, 0, 1, 0, 2 };
    I cols[] = { 0, 1, 1, 2, 2 };
    F vals[] = { 2.0, 1.0, 3.0, 4.0, 5.0 };
    auto U = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, vals, 5);
    // x = (1, 2, 3); b = U*x = (2+2+12, 6, 15) = (16, 6, 15)
    m::dynvec<F> b(3, F(0));
    b[0] = 16.0;
    b[1] = 6.0;
    b[2] = 15.0;
    bool ok = ms::solve_upper<F, I>(U, b, ms::triangular_tag::upper_non_unit{});
    require_true(ok);
    require_true(approx(b[0], 1.0, 1e-12));
    require_true(approx(b[1], 2.0, 1e-12));
    require_true(approx(b[2], 3.0, 1e-12));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("csr ↔ csc round-trip");
  {
    using F = double;
    using I = u32;
    I rows[] = { 0, 1, 0, 2 };
    I cols[] = { 0, 1, 2, 2 };
    F vals[] = { 4.0, 5.0, 1.0, 6.0 };
    auto A = ms::csc<F, I>::from_triplets_sorted(3, 3, rows, cols, vals, 4);
    auto B = ms::to_csr(A);
    auto C = ms::to_csc(B);
    require_true(C.rows == A.rows);
    require_true(C.cols == A.cols);
    require_true(C.nnz() == A.nnz());
    // spmv must agree
    m::dynvec<F> x(3, F(0));
    x[0] = 1.0;
    x[1] = 2.0;
    x[2] = 3.0;
    m::dynvec<F> ya(3, F(0)), yc(3, F(0));
    ms::spmv<F, I>(F(1), A, x, F(0), ya);
    ms::spmv<F, I>(F(1), C, x, F(0), yc);
    for ( usize i = 0; i < 3; ++i ) require_true(approx(ya[i], yc[i], F(1e-12)));
  }
  end_test_case();

  sb::print("=== SPARSE BASIC PASSED ===");
  return 1;
}
