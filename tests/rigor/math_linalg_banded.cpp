// math_linalg_banded.cpp
// Rigorous snowball tests for math::linalg::banded — Thomas tridiag
// solve and pent_spd_solve (5-banded SPD via LDL^T).

#include "../../src/math/linalg.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math::linalg;

static bool
near(f64 a, f64 b, f64 eps = 1e-10)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Reference matvec: tridiagonal A * x → out.
static void
tri_matvec(const f64 *a, const f64 *b, const f64 *c, const f64 *x, f64 *out, usize n)
{
  for ( usize i = 0; i < n; ++i ) {
    f64 v = b[i] * x[i];
    if ( i > 0 ) v += a[i - 1] * x[i - 1];
    if ( i + 1 < n ) v += c[i] * x[i + 1];
    out[i] = v;
  }
}

// Reference matvec: 5-banded symmetric A * x → out.
static void
pent_matvec(const f64 *d2, const f64 *d1, const f64 *diag, const f64 *x, f64 *out, usize n)
{
  for ( usize i = 0; i < n; ++i ) {
    f64 v = diag[i] * x[i];
    if ( i > 0 ) v += d1[i - 1] * x[i - 1];
    if ( i + 1 < n ) v += d1[i] * x[i + 1];
    if ( i > 1 ) v += d2[i - 2] * x[i - 2];
    if ( i + 2 < n ) v += d2[i] * x[i + 2];
    out[i] = v;
  }
}

static f64
max_abs_diff(const f64 *a, const f64 *b, usize n)
{
  f64 m = 0;
  for ( usize i = 0; i < n; ++i ) {
    f64 d = a[i] - b[i];
    if ( d < 0 ) d = -d;
    if ( d > m ) m = d;
  }
  return m;
}

int
main()
{
  print("=== MATH::LINALG::BANDED TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Tridiagonal: hand-verified 4x4 system.
  test_case("tridiag_solve: 4×4 hand-verified system");
  {
    // A = [[2, -1, 0, 0],
    //      [-1, 2, -1, 0],
    //      [0, -1, 2, -1],
    //      [0, 0, -1, 2]]
    // x = [1, 2, 3, 4]; b = A*x = [0, 0, 0, 5]
    f64 a[3] = { -1, -1, -1 };
    f64 b[4] = { 2, 2, 2, 2 };
    f64 c[3] = { -1, -1, -1 };
    f64 d[4] = { 0, 0, 0, 5 };
    tridiag_solve<f64>(a, b, c, d, 4);
    require_true(near(d[0], 1.0, 1e-12));
    require_true(near(d[1], 2.0, 1e-12));
    require_true(near(d[2], 3.0, 1e-12));
    require_true(near(d[3], 4.0, 1e-12));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("tridiag_solve: degenerate n=1");
  {
    f64 b[1] = { 3.0 };
    f64 d[1] = { 12.0 };
    tridiag_solve<f64>(nullptr, b, nullptr, d, 1);
    require_true(near(d[0], 4.0, 1e-15));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("tridiag_solve: n=2 closed-form");
  {
    // [[3, 1], [1, 4]] x = [5, 6]   →   x = [14/11, 13/11]
    f64 a[1] = { 1 };
    f64 b[2] = { 3, 4 };
    f64 c[1] = { 1 };
    f64 d[2] = { 5, 6 };
    tridiag_solve<f64>(a, b, c, d, 2);
    require_true(near(d[0], 14.0 / 11.0, 1e-13));
    require_true(near(d[1], 13.0 / 11.0, 1e-13));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("tridiag_solve: round-trip on 64×64 SDD system");
  {
    constexpr usize n = 64;
    f64 a[n - 1], b[n], c[n - 1];
    f64 x_truth[n], rhs[n], rhs_copy[n];
    // strictly diagonally dominant
    for ( usize i = 0; i < n; ++i ) {
      b[i] = 4.0 + 0.01 * f64(i);
      x_truth[i] = 1.0 - 0.5 * f64(i % 7) + 0.25 * f64(i % 3);
    }
    for ( usize i = 0; i + 1 < n; ++i ) {
      a[i] = -1.0 + 0.001 * f64(i);
      c[i] = -1.0 - 0.002 * f64(i);
    }
    tri_matvec(a, b, c, x_truth, rhs, n);
    for ( usize i = 0; i < n; ++i ) rhs_copy[i] = rhs[i];

    // solve modifies a, b, c, d; preserve a copy of a, b for matvec re-check
    f64 a_save[n - 1], b_save[n], c_save[n - 1];
    for ( usize i = 0; i < n; ++i ) b_save[i] = b[i];
    for ( usize i = 0; i + 1 < n; ++i ) {
      a_save[i] = a[i];
      c_save[i] = c[i];
    }
    tridiag_solve<f64>(a, b, c, rhs, n);

    require_true(max_abs_diff(rhs, x_truth, n) < 1e-10);

    // residual check: ‖A x_solved - rhs_copy‖∞ < eps
    f64 check[n];
    tri_matvec(a_save, b_save, c_save, rhs, check, n);
    require_true(max_abs_diff(check, rhs_copy, n) < 1e-9);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("pent_spd_solve: 5×5 SPD hand-verified");
  {
    // A is SPD: build from B B^T-ish. Use the well-known "biharmonic"
    // band [[5,-4,1], [-4,6,-4,1], [1,-4,6,-4,1], [1,-4,6,-4], [1,-4,5]]
    // (typical 4th-order finite-difference operator on a beam).
    // Take x = [1, 2, 3, 4, 5], compute b = A x, solve, verify.
    constexpr usize n = 5;
    f64 d2[n - 2] = { 1, 1, 1 };
    f64 d1[n - 1] = { -4, -4, -4, -4 };
    f64 diag[n] = { 5, 6, 6, 6, 5 };
    f64 x_truth[n] = { 1, 2, 3, 4, 5 };
    f64 rhs[n];
    pent_matvec(d2, d1, diag, x_truth, rhs, n);

    // copy band data because solve overwrites
    f64 d2c[n - 2], d1c[n - 1], diagc[n];
    for ( usize i = 0; i < n - 2; ++i ) d2c[i] = d2[i];
    for ( usize i = 0; i < n - 1; ++i ) d1c[i] = d1[i];
    for ( usize i = 0; i < n; ++i ) diagc[i] = diag[i];

    bool ok = pent_spd_solve<f64>(d2c, d1c, diagc, rhs, n);
    require_true(ok);
    require_true(max_abs_diff(rhs, x_truth, n) < 1e-10);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("pent_spd_solve: degenerate n=1 and n=2");
  {
    {
      f64 diag[1] = { 4.0 };
      f64 rhs[1] = { 12.0 };
      bool ok = pent_spd_solve<f64>(nullptr, nullptr, diag, rhs, 1);
      require_true(ok);
      require_true(near(rhs[0], 3.0, 1e-15));
    }
    {
      // [[4, -1], [-1, 3]]  x = [3, 4]   →   x = (13/11, 19/11)
      f64 d1[1] = { -1 };
      f64 diag[2] = { 4, 3 };
      f64 rhs[2] = { 3, 4 };
      bool ok = pent_spd_solve<f64>(nullptr, d1, diag, rhs, 2);
      require_true(ok);
      require_true(near(rhs[0], 13.0 / 11.0, 1e-13));
      require_true(near(rhs[1], 19.0 / 11.0, 1e-13));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("pent_spd_solve: 32×32 SPD round-trip");
  {
    constexpr usize n = 32;
    f64 d2[n - 2], d1[n - 1], diag[n];
    f64 x_truth[n], rhs[n];
    // SPD by construction: diag dominates ∑|off-diag|.
    for ( usize i = 0; i < n - 2; ++i ) d2[i] = 0.5;
    for ( usize i = 0; i < n - 1; ++i ) d1[i] = -1.0;
    for ( usize i = 0; i < n; ++i ) diag[i] = 4.0;
    for ( usize i = 0; i < n; ++i ) x_truth[i] = 0.3 * f64(i) - 0.7 * f64(i % 5);
    pent_matvec(d2, d1, diag, x_truth, rhs, n);

    bool ok = pent_spd_solve<f64>(d2, d1, diag, rhs, n);
    require_true(ok);
    require_true(max_abs_diff(rhs, x_truth, n) < 1e-10);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("pent_spd_solve: rejects non-SPD with negative pivot");
  {
    // diag negative ⇒ first pivot fails.
    f64 d2[1] = { 0.0 };
    f64 d1[2] = { 0.0, 0.0 };
    f64 diag[3] = { -1.0, 1.0, 1.0 };
    f64 rhs[3] = { 1, 2, 3 };
    bool ok = pent_spd_solve<f64>(d2, d1, diag, rhs, 3);
    require_false(ok);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("pent_spd_factor + solve_factored: equivalent to pent_spd_solve");
  {
    constexpr usize n = 16;
    f64 d2_a[n - 2], d1_a[n - 1], diag_a[n], rhs_a[n];
    f64 d2_b[n - 2], d1_b[n - 1], diag_b[n], rhs_b[n];
    f64 x_truth[n];
    for ( usize i = 0; i < n - 2; ++i ) d2_a[i] = d2_b[i] = 0.5;
    for ( usize i = 0; i < n - 1; ++i ) d1_a[i] = d1_b[i] = -1.0;
    for ( usize i = 0; i < n; ++i ) {
      diag_a[i] = diag_b[i] = 4.0;
      x_truth[i] = 0.7 * f64(i) - 0.3 * f64(i % 4);
    }
    pent_matvec(d2_a, d1_a, diag_a, x_truth, rhs_a, n);
    for ( usize i = 0; i < n; ++i ) rhs_b[i] = rhs_a[i];

    // path A: combined solve
    bool ok_a = pent_spd_solve<f64>(d2_a, d1_a, diag_a, rhs_a, n);
    // path B: factor + solve_factored
    bool ok_b = pent_spd_factor<f64>(d2_b, d1_b, diag_b, n);
    pent_spd_solve_factored<f64>(d2_b, d1_b, diag_b, rhs_b, n);
    require_true(ok_a);
    require_true(ok_b);
    require_true(max_abs_diff(rhs_a, rhs_b, n) < 1e-15);
    require_true(max_abs_diff(rhs_a, x_truth, n) < 1e-10);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("pent_spd_takahashi: matches diag(C^{-1}) on a 5×5 SPD");
  {
    constexpr usize n = 5;
    // The biharmonic matrix (positive definite).
    f64 d2[n - 2] = { 1, 1, 1 };
    f64 d1[n - 1] = { -4, -4, -4, -4 };
    f64 diag[n] = { 5, 6, 6, 6, 5 };

    // Compute diag(C^{-1}) the brute-force way: solve C e_k = e_k for
    // each k; the k-th component of the solution is C^{-1}[k, k].
    f64 z_truth[n];
    f64 z_d1_truth[n - 1];
    f64 z_d2_truth[n - 2];
    for ( usize k = 0; k < n; ++k ) {
      f64 d2c[n - 2], d1c[n - 1], diagc[n], e[n];
      for ( usize i = 0; i < n - 2; ++i ) d2c[i] = d2[i];
      for ( usize i = 0; i < n - 1; ++i ) d1c[i] = d1[i];
      for ( usize i = 0; i < n; ++i ) {
        diagc[i] = diag[i];
        e[i] = (i == k) ? 1.0 : 0.0;
      }
      bool ok = pent_spd_solve<f64>(d2c, d1c, diagc, e, n);
      require_true(ok);
      z_truth[k] = e[k];
      if ( k + 1 < n ) z_d1_truth[k] = e[k + 1];
      if ( k + 2 < n ) z_d2_truth[k] = e[k + 2];
    }

    // Now factor + Takahashi.
    f64 d2c[n - 2], d1c[n - 1], diagc[n];
    for ( usize i = 0; i < n - 2; ++i ) d2c[i] = d2[i];
    for ( usize i = 0; i < n - 1; ++i ) d1c[i] = d1[i];
    for ( usize i = 0; i < n; ++i ) diagc[i] = diag[i];
    bool ok = pent_spd_factor<f64>(d2c, d1c, diagc, n);
    require_true(ok);
    f64 z_diag[n], z_d1[n - 1], z_d2[n - 2];
    pent_spd_takahashi<f64>(d2c, d1c, diagc, n, z_diag, z_d1, z_d2);
    for ( usize k = 0; k < n; ++k ) require_true(near(z_diag[k], z_truth[k], 1e-12));
    for ( usize k = 0; k + 1 < n; ++k ) require_true(near(z_d1[k], z_d1_truth[k], 1e-12));
    for ( usize k = 0; k + 2 < n; ++k ) require_true(near(z_d2[k], z_d2_truth[k], 1e-12));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("gband_lu_solve: 7-banded (kl=ku=3) on a diagonally-dominant system");
  {
    constexpr usize n = 12;
    constexpr usize kl = 3, ku = 3;
    constexpr usize nb = kl + ku + 1;
    // Build a diagonally-dominant 7-banded matrix.  Reference matvec
    // walks the same band layout to validate the solver.
    f64 AB[nb * n];
    for ( usize i = 0; i < nb * n; ++i ) AB[i] = 0.0;
    f64 x_truth[n], rhs[n];
    for ( usize i = 0; i < n; ++i ) {
      x_truth[i] = 0.5 * f64(i % 7) - f64(i % 3);
    }
    auto set = [&](usize r, usize c, f64 v) {
      const usize band = (r + ku) - c;
      AB[band * n + c] = v;
    };
    auto get = [&](usize r, usize c) -> f64 {
      if ( r > c + ku || c > r + kl ) return 0.0;
      const usize band = (r + ku) - c;
      return AB[band * n + c];
    };
    for ( usize r = 0; r < n; ++r ) {
      // Diagonal weighted to dominate the off-diagonals.
      f64 abs_off = 0.0;
      for ( usize c = (r > kl ? r - kl : 0); c < n && c <= r + ku; ++c ) {
        if ( c == r ) continue;
        f64 v = 0.1 * f64((r + 3 * c + 1) % 5) - 0.05;
        set(r, c, v);
        if ( v < 0 ) v = -v;
        abs_off += v;
      }
      set(r, r, 5.0 + abs_off);
    }

    // Reference matvec: rhs = A * x_truth.
    for ( usize r = 0; r < n; ++r ) {
      f64 s = 0.0;
      for ( usize c = (r > kl ? r - kl : 0); c < n && c <= r + ku; ++c ) {
        s += get(r, c) * x_truth[c];
      }
      rhs[r] = s;
    }

    // Save AB, solve.
    bool ok = gband_lu_solve<f64>(AB, kl, ku, n, rhs);
    require_true(ok);
    require_true(max_abs_diff(rhs, x_truth, n) < 1e-9);
  }
  end_test_case();

  print("=== BANDED SOLVER TESTS PASSED ===");
  return 1;
}
