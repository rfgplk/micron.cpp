// math_matfunc_schur.cpp — Snowball tests for namespace schur::
// (expmat, sqrtmat, logmat).

#include "../../src/math/linalg/matfunc_schur.hpp"
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
near(f64 a, f64 b, f64 eps = 1e-9)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== MATFUNC SCHUR ===");

  test_case("expmat: diagonal matrix → diagonal of exponentials");
  {
    mat<f64, 2, 2> D{ { 1, 0, 0, 2 } };
    auto r = linalg::schur::expmat(D);
    require_true(r.converged);
    require_true(near(r.X.data[0], math::exp<f64>(1.0)));
    require_true(near(r.X.data[3], math::exp<f64>(2.0)));
    require_true(near(r.X.data[1], 0.0));
    require_true(near(r.X.data[2], 0.0));
  }
  end_test_case();

  test_case("expmat: 2×2 rotation generator → rotation matrix");
  {
    // exp([[0, -t], [t, 0]]) = [[cos t, -sin t], [sin t, cos t]]
    f64 t = 0.7;
    mat<f64, 2, 2> R{ { 0, -t, t, 0 } };
    auto r = linalg::schur::expmat(R);
    require_true(r.converged);
    f64 ct = math::cos<f64>(t);
    f64 st = math::sin<f64>(t);
    require_true(near(r.X.data[0], ct));
    require_true(near(r.X.data[1], -st));
    require_true(near(r.X.data[2], st));
    require_true(near(r.X.data[3], ct));
  }
  end_test_case();

  test_case("expmat: nilpotent 3×3 → finite Taylor expansion");
  {
    // N = [[0,1,0],[0,0,1],[0,0,0]]; exp(N) = I + N + N²/2
    mat<f64, 3, 3> N{ { 0, 1, 0, 0, 0, 1, 0, 0, 0 } };
    auto r = linalg::schur::expmat(N);
    require_true(r.converged);
    // expected: [[1, 1, 0.5], [0, 1, 1], [0, 0, 1]]
    require_true(near(r.X.data[0], 1.0));
    require_true(near(r.X.data[1], 1.0));
    require_true(near(r.X.data[2], 0.5));
    require_true(near(r.X.data[3], 0.0));
    require_true(near(r.X.data[4], 1.0));
    require_true(near(r.X.data[5], 1.0));
    require_true(near(r.X.data[6], 0.0));
    require_true(near(r.X.data[7], 0.0));
    require_true(near(r.X.data[8], 1.0));
  }
  end_test_case();

  test_case("expmat: known 2×2 with distinct real eigenvalues");
  {
    // M = [[1, 1], [0, 2]] (upper triangular).  Eigenvalues 1, 2.
    // exp(M) = [[e, e²-e], [0, e²]]
    mat<f64, 2, 2> M{ { 1, 1, 0, 2 } };
    auto r = linalg::schur::expmat(M);
    require_true(r.converged);
    f64 e = math::exp<f64>(1.0);
    f64 e2 = math::exp<f64>(2.0);
    require_true(near(r.X.data[0], e));
    require_true(near(r.X.data[1], e2 - e));
    require_true(near(r.X.data[2], 0.0));
    require_true(near(r.X.data[3], e2));
  }
  end_test_case();

  test_case("sqrtmat: diagonal SPD → element-wise sqrt");
  {
    mat<f64, 2, 2> S{ { 4, 0, 0, 9 } };
    auto r = linalg::schur::sqrtmat(S);
    require_true(r.converged && r.real_sqrt_exists);
    require_true(near(r.X.data[0], 2.0));
    require_true(near(r.X.data[3], 3.0));
  }
  end_test_case();

  test_case("sqrtmat: roundtrip A = X · X for an SPD A");
  {
    // SPD A = MᵀM where M is some 3×3 invertible
    mat<f64, 3, 3> A{};
    f64 m[9] = { 1, 2, 3, 0, 1, 4, 0, 0, 5 };
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += m[k * 3 + i] * m[k * 3 + j];
        A.data[i * 3 + j] = s;
      }
    auto r = linalg::schur::sqrtmat(A);
    require_true(r.converged && r.real_sqrt_exists);
    // X · X == A
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) {
        f64 s = 0;
        for ( usize k = 0; k < 3; ++k ) s += r.X.data[i * 3 + k] * r.X.data[k * 3 + j];
        require_true(near(s, A.data[i * 3 + j]));
      }
  }
  end_test_case();

  test_case("logmat: diagonal of exponentials → diagonal of integers");
  {
    mat<f64, 2, 2> L{ { math::exp<f64>(1.0), 0, 0, math::exp<f64>(2.0) } };
    auto r = linalg::schur::logmat(L);
    require_true(r.converged);
    require_true(near(r.X.data[0], 1.0));
    require_true(near(r.X.data[3], 2.0));
  }
  end_test_case();

  test_case("logmat: roundtrip exp(log(A)) = A on a 2×2 with positive eigenvalues");
  {
    mat<f64, 2, 2> A{ { 4, 1, 0, 3 } };
    auto rl = linalg::schur::logmat(A);
    require_true(rl.converged);
    auto re = linalg::schur::expmat(rl.X);
    require_true(re.converged);
    for ( usize i = 0; i < 4; ++i ) require_true(near(re.X.data[i], A.data[i]));
  }
  end_test_case();

  return 1;
}
