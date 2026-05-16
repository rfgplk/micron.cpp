// math_linalg.cpp
// Rigorous snowball test suite for math::linalg — vec/mat/quat
// types, free function ops, decomp.

#include "../../src/math/linalg.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math::linalg;
using namespace micron::math::linalg::ops;
using namespace micron::math::linalg::decomp;

static bool
near(f64 a, f64 b, f64 eps = 1e-10)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

template<usize N>
static bool
near_v(const vec<f64, N> &a, const vec<f64, N> &b, f64 eps = 1e-10)
{
  for ( usize i = 0; i < N; ++i )
    if ( !near(a.data[i], b.data[i], eps) ) return false;
  return true;
}

template<usize R, usize C>
static bool
near_m(const mat<f64, R, C> &a, const mat<f64, R, C> &b, f64 eps = 1e-10)
{
  for ( usize i = 0; i < R * C; ++i )
    if ( !near(a.data[i], b.data[i], eps) ) return false;
  return true;
}

int
main()
{
  print("=== MATH::LINALG TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // size contracts
  test_case("size contracts — single cache line");
  {
    static_assert(sizeof(vec<f32, 4>) == 16);
    static_assert(sizeof(vec<f64, 4>) == 32);
    static_assert(sizeof(mat<f32, 4, 4>) == 64);
    static_assert(sizeof(mat<f64, 2, 4>) == 64);
    static_assert(sizeof(quat<f32>) == 16);
    static_assert(sizeof(quat<f64>) == 32);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // dot
  test_case("dot — basic");
  {
    vec<f64, 3> a{ 1.0, 2.0, 3.0 };
    vec<f64, 3> b{ 4.0, 5.0, 6.0 };
    require_true(near(dot(a, b), 32.0));
  }
  end_test_case();

  test_case("dot — 8-wide dual accumulator");
  {
    vec<f64, 8> a{ 1, 2, 3, 4, 5, 6, 7, 8 };
    vec<f64, 8> b{ 1, 1, 1, 1, 1, 1, 1, 1 };
    require_true(near(dot(a, b), 36.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // cross
  test_case("cross — orthogonal basis");
  {
    vec<f64, 3> ux{ 1, 0, 0 };
    vec<f64, 3> uy{ 0, 1, 0 };
    auto uz = cross(ux, uy);
    require_true(near_v(uz, vec<f64, 3>{ 0, 0, 1 }));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // norm / normalize
  test_case("norm + normalize");
  {
    vec<f64, 3> v{ 3, 4, 0 };
    require_true(near(norm(v), 5.0));
    require_true(near(norm_l1(v), 7.0));
    require_true(near(norm_inf(v), 4.0));
    auto vn = normalize(v);
    require_true(near(norm(vn), 1.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // hadamard / clamp / abs
  test_case("hadamard / clamp / abs");
  {
    vec<f64, 4> a{ 1, -2, 3, -4 };
    auto h = hadamard(a, a);
    require_true(near_v(h, vec<f64, 4>{ 1, 4, 9, 16 }));
    auto av = abs_v(a);
    require_true(near_v(av, vec<f64, 4>{ 1, 2, 3, 4 }));
    auto cv = clamp_v<f64, 4>(a, -2.0, 2.0);
    require_true(near_v(cv, vec<f64, 4>{ 1, -2, 2, -2 }));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // lerp / reflect / project
  test_case("lerp / reflect / project");
  {
    vec<f64, 3> p0{ 0, 0, 0 }, p1{ 2, 4, 8 };
    auto pm = lerp(p0, p1, 0.5);
    require_true(near_v(pm, vec<f64, 3>{ 1, 2, 4 }));

    vec<f64, 3> i{ 1, -1, 0 };
    vec<f64, 3> n{ 0, 1, 0 };
    auto r = reflect(i, n);
    require_true(near_v(r, vec<f64, 3>{ 1, 1, 0 }));

    vec<f64, 3> u{ 3, 4, 0 };
    vec<f64, 3> v{ 1, 0, 0 };
    auto pr = project(u, v);
    require_true(near_v(pr, vec<f64, 3>{ 3, 0, 0 }));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // gemv / gemm / transpose
  test_case("gemv — identity");
  {
    auto E = mat<f64, 3, 3>::identity();
    vec<f64, 3> v{ 7, 8, 9 };
    require_true(near_v(gemv(E, v), v));
  }
  end_test_case();

  test_case("gemm — identity is neutral");
  {
    mat<f64, 2, 2> M{ 1.0, 2.0, 3.0, 4.0 };
    auto E = mat<f64, 2, 2>::identity();
    require_true(near_m(gemm(M, E), M));
    require_true(near_m(gemm(E, M), M));
  }
  end_test_case();

  test_case("transpose — round-trip");
  {
    mat<f64, 2, 3> M{ 1, 2, 3, 4, 5, 6 };
    auto Mtt = transpose(transpose(M));
    require_true(near_m(Mtt, M));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // det / inv / solve
  test_case("det2 / inv2 / solve2");
  {
    mat<f64, 2, 2> M{ 4.0, 7.0, 2.0, 6.0 };      // det = 24-14 = 10
    require_true(near(det2(M), 10.0));
    auto Mi = inv2(M);
    auto P = gemm(M, Mi);
    require_true(near_m(P, mat<f64, 2, 2>::identity()));
    vec<f64, 2> b{ 1.0, 2.0 };
    auto x = solve2(M, b);
    auto bx = gemv(M, x);
    require_true(near_v(bx, b));
  }
  end_test_case();

  test_case("det3 / inv3 / solve3");
  {
    mat<f64, 3, 3> M{ 2.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 4.0 };
    require_true(near(det3(M), 24.0));
    auto Mi = inv3(M);
    require_true(near_m(gemm(M, Mi), mat<f64, 3, 3>::identity()));
    vec<f64, 3> b{ 4.0, 9.0, 16.0 };
    auto x = solve3(M, b);
    require_true(near_v(x, vec<f64, 3>{ 2.0, 3.0, 4.0 }));
  }
  end_test_case();

  test_case("det4 / inv4");
  {
    auto E = mat<f64, 4, 4>::identity();
    require_true(near(det4(E), 1.0));
    require_true(near_m(inv4(E), E));

    // diagonal
    mat<f64, 4, 4> D = mat<f64, 4, 4>::zero();
    D.data[0] = 2;
    D.data[5] = 3;
    D.data[10] = 4;
    D.data[15] = 5;
    require_true(near(det4(D), 120.0));
    auto Di = inv4(D);
    require_true(near_m(gemm(D, Di), E));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // quaternion ops
  test_case("quat — identity preserves vector");
  {
    auto q = quat<f64>::identity();
    vec<f64, 3> v{ 1.0, 2.0, 3.0 };
    require_true(near_v(rotate(q, v), v));
  }
  end_test_case();

  test_case("quat — multiply identity");
  {
    auto q = quat<f64>::identity();
    auto qm = ops::mul(q, q);
    require_true(near(qm.data[3], 1.0));
    require_true(near(qm.data[0], 0.0));
  }
  end_test_case();

  test_case("quat — slerp endpoints");
  {
    auto q0 = quat<f64>::identity();
    quat<f64> q1{ 0.0, 0.0, 1.0, 0.0 };      // 180 deg about Z
    auto m0 = slerp(q0, q1, 0.0);
    auto m1 = slerp(q0, q1, 1.0);
    require_true(near(m0.data[3], 1.0, 1e-9));
    require_true(near(m1.data[2] * m1.data[2], 1.0, 1e-9));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // decomp
  test_case("LU — reproduces A");
  {
    mat<f64, 3, 3> A{ 4.0, 3.0, 0.0, 3.0, 4.0, -1.0, 0.0, -1.0, 4.0 };
    auto r = lu(A);
    require_false(r.singular);
    auto P = gemm(r.L, r.U);
    require_true(near_m(P, A));
  }
  end_test_case();

  test_case("Cholesky — L Lᵀ reproduces A on SPD");
  {
    mat<f64, 3, 3> A{ 4.0, 3.0, 0.0, 3.0, 4.0, -1.0, 0.0, -1.0, 4.0 };
    auto r = chol(A);
    require_true(r.spd);
    auto Lt = transpose(r.L);
    require_true(near_m(gemm(r.L, Lt), A));
  }
  end_test_case();

  test_case("QR — Q*R reproduces A");
  {
    mat<f64, 3, 3> A{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 10.0 };
    auto r = qr(A);
    require_true(near_m(gemm(r.Q, r.R), A));
  }
  end_test_case();

  test_case("eigen_sym3 — trace is preserved");
  {
    mat<f64, 3, 3> A{ 4.0, 3.0, 0.0, 3.0, 4.0, -1.0, 0.0, -1.0, 4.0 };
    auto e = eigen_sym3(A);
    f64 tr = e.values.data[0] + e.values.data[1] + e.values.data[2];
    require_true(near(tr, 12.0, 1e-9));
  }
  end_test_case();

  test_case("svd2 — reproduces A");
  {
    mat<f64, 2, 2> A{ 3.0, 1.0, 1.0, 3.0 };
    auto s = svd2(A);
    // U * diag(S) * V^T = A
    mat<f64, 2, 2> S = mat<f64, 2, 2>::zero();
    S.data[0] = s.S.data[0];
    S.data[3] = s.S.data[1];
    auto Vt = transpose(s.V);
    auto P = gemm(s.U, gemm(S, Vt));
    require_true(near_m(P, A, 1e-9));
  }
  end_test_case();

  print("=== MATH::LINALG TESTS PASSED ===");
  return 1;
}
