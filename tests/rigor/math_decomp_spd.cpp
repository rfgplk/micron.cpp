// math_decomp_spd.cpp — Snowball tests for SPD-specific decompositions:
// dynamic Cholesky, inv_sympd, log_det_sympd, chol_pivot, bandwidth,
// chol_banded.

#include "../../src/math/linalg/banded.hpp"
#include "../../src/math/linalg/spd.hpp"
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

// Build SPD: A = MᵀM + 1e-3·I (well-conditioned).
template <usize N>
static mat<f64, N, N>
build_spd(const f64 *M_data) noexcept
{
  mat<f64, N, N> A{};
  for ( usize i = 0; i < N; ++i )
    for ( usize j = 0; j < N; ++j ) {
      f64 s = (i == j) ? 1e-3 : 0.0;
      for ( usize k = 0; k < N; ++k ) s += M_data[k * N + i] * M_data[k * N + j];
      A.data[i * N + j] = s;
    }
  return A;
}

int
main()
{
  print("=== DECOMP SPD ===");

  test_case("dynamic Cholesky: L · Lᵀ = A");
  {
    f64 m[9] = { 1, 2, 1, 0, 1, 3, 1, 0, 1 };
    auto A = build_spd<3>(m);
    auto Ad = dynmat<f64>::from(A);
    auto c = linalg::spd::chol(Ad);
    require_true(c.spd);
    for ( usize i = 0; i < 3; ++i ) {
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += c.L.at(i, k) * c.L.at(j, k);
        require_true(near(s, Ad.at(i, j)));
      }
    }
  }
  end_test_case();

  test_case("inv_sympd: A · A⁻¹ = I (fixed and dynamic)");
  {
    f64 m[9] = { 1, 2, 1, 0, 1, 3, 1, 0, 1 };
    auto A = build_spd<3>(m);
    auto inv_f = linalg::spd::inv_sympd(A);
    require_true(inv_f.spd);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += A.data[i * 3 + k] * inv_f.X.data[k * 3 + j];
        require_true(near(s, (i == j) ? 1.0 : 0.0));
      }
    auto Ad = dynmat<f64>::from(A);
    auto inv_d = linalg::spd::inv_sympd(Ad);
    require_true(inv_d.spd);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += Ad.at(i, k) * inv_d.X.at(k, j);
        require_true(near(s, (i == j) ? 1.0 : 0.0));
      }
  }
  end_test_case();

  test_case("log_det_sympd matches log|det(A)|");
  {
    f64 m[9] = { 1, 2, 1, 0, 1, 3, 1, 0, 1 };
    auto A = build_spd<3>(m);
    auto ld_sp = linalg::spd::log_det_sympd(A);
    auto ld_gen = linalg::decomp::log_det(A);
    require_true(ld_sp.sign == 1);
    require_true(ld_gen.sign == 1);
    require_true(near(ld_sp.log_abs, ld_gen.log_abs, 1e-9));
  }
  end_test_case();

  test_case("inv_sympd reports spd=false on a non-SPD matrix");
  {
    mat<f64, 2, 2> A{ { 1, 2, 2, 1 } };     // indefinite (eigvals 3, -1)
    auto inv = linalg::spd::inv_sympd(A);
    require_true(!inv.spd);
  }
  end_test_case();

  test_case("bandwidth detector identifies tridiagonal / pentadiagonal");
  {
    dynmat<f64> T = dynmat<f64>::zero(5, 5);
    for ( usize i = 0; i < 5; ++i ) T.at(i, i) = 4.0;
    for ( usize i = 0; i + 1 < 5; ++i ) {
      T.at(i, i + 1) = 1.0;
      T.at(i + 1, i) = 1.0;
    }
    auto bw_t = linalg::bandwidth(T);
    require_true(bw_t.kl == 1 && bw_t.ku == 1);

    dynmat<f64> P = dynmat<f64>::zero(5, 5);
    for ( usize i = 0; i < 5; ++i ) P.at(i, i) = 5.0;
    for ( usize i = 0; i + 1 < 5; ++i ) {
      P.at(i, i + 1) = 1.0;
      P.at(i + 1, i) = 1.0;
    }
    for ( usize i = 0; i + 2 < 5; ++i ) {
      P.at(i, i + 2) = 0.5;
      P.at(i + 2, i) = 0.5;
    }
    auto bw_p = linalg::bandwidth(P);
    require_true(bw_p.kl == 2 && bw_p.ku == 2);
  }
  end_test_case();

  test_case("chol_banded matches dense chol for an SPD tridiagonal");
  {
    dynmat<f64> A = dynmat<f64>::zero(5, 5);
    for ( usize i = 0; i < 5; ++i ) A.at(i, i) = 4.0;
    for ( usize i = 0; i + 1 < 5; ++i ) {
      A.at(i, i + 1) = 1.0;
      A.at(i + 1, i) = 1.0;
    }
    auto bw = linalg::bandwidth(A);
    auto c_band = linalg::chol_banded(A, bw.kl);
    auto c_dense = linalg::spd::chol(A);
    require_true(c_band.spd && c_dense.spd);
    for ( usize i = 0; i < 5; ++i )
      for ( usize j = 0; j <= i; ++j ) require_true(near(c_band.L.at(i, j), c_dense.L.at(i, j)));
    // L · Lᵀ == A
    for ( usize i = 0; i < 5; ++i )
      for ( usize j = 0; j < 5; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 5; ++k ) s += c_band.L.at(i, k) * c_band.L.at(j, k);
        require_true(near(s, A.at(i, j)));
      }
  }
  end_test_case();

  test_case("chol_pivot detects rank deficiency in a semidefinite matrix");
  {
    // Rank-2 SPD-singular: A = uuᵀ + vvᵀ with linearly independent u, v.
    f64 u[4] = { 1, 2, 1, 1 };
    f64 v[4] = { 0, 1, 2, 0 };
    dynmat<f64> A(4, 4);
    for ( usize i = 0; i < 4; ++i )
      for ( usize j = 0; j < 4; ++j ) A.at(i, j) = u[i] * u[j] + v[i] * v[j];
    auto cp = linalg::spd::chol_pivot(A);
    require_true(cp.rank == 2);
  }
  end_test_case();

  return 1;
}
