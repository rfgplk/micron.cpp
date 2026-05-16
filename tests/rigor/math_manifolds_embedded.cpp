// math_manifolds_embedded.cpp
// Snowball tests for math::manifolds — euclidean, sphere, torus,
// SPD, hyperbolic, Stiefel, Grassmann.

#include "../../src/math/manifolds/manifolds.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;
using namespace micron::math::manifolds;

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
  print("=== MATH::MANIFOLDS::EMBEDDED TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Euclidean (flat baseline)
  test_case("euclidean: identity is zero");
  {
    auto e = euclidean<f64, 3>::identity();
    require_true(near_v(e, vec<f64, 3>{ 0, 0, 0 }));
  }
  end_test_case();

  test_case("euclidean: exp(p, v) = p + v");
  {
    vec<f64, 3> p{ 1, 2, 3 };
    vec<f64, 3> v{ 0.1, -0.2, 0.3 };
    auto q = euclidean<f64, 3>::exp_map(p, v);
    require_true(near_v(q, vec<f64, 3>{ 1.1, 1.8, 3.3 }));
  }
  end_test_case();

  test_case("euclidean: log(p, q) = q - p");
  {
    vec<f64, 3> p{ 1, 2, 3 };
    vec<f64, 3> q{ 4, 5, 6 };
    auto v = euclidean<f64, 3>::log_map(p, q);
    require_true(near_v(v, vec<f64, 3>{ 3, 3, 3 }));
  }
  end_test_case();

  test_case("euclidean: distance = ‖q - p‖");
  {
    vec<f64, 3> p{ 0, 0, 0 };
    vec<f64, 3> q{ 1, 2, 2 };
    require_true(near(euclidean<f64, 3>::distance(p, q), 3.0));
  }
  end_test_case();

  test_case("euclidean: composition is addition");
  {
    vec<f64, 3> a{ 1, 2, 3 };
    vec<f64, 3> b{ 4, 5, 6 };
    auto c = euclidean<f64, 3>::compose(a, b);
    require_true(near_v(c, vec<f64, 3>{ 5, 7, 9 }));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Sphere
  test_case("sphere<3>: project_to_manifold gives unit vector");
  {
    vec<f64, 3> x{ 3, 4, 0 };
    auto p = sphere<f64, 3>::project(x);
    require_true(near(linalg::ops::norm(p), 1.0));
    require_true(near_v(p, vec<f64, 3>{ 0.6, 0.8, 0.0 }));
  }
  end_test_case();

  test_case("sphere<3>: project_to_tangent gives orthogonal-to-p");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v{ 0.5, 0.5, 0.0 };
    auto vt = sphere<f64, 3>::project_to_tangent(p, v);
    require_true(near(linalg::ops::dot(p, vt), 0.0));
    require_true(near_v(vt, vec<f64, 3>{ 0.0, 0.5, 0.0 }));
  }
  end_test_case();

  test_case("sphere<3>: tangent projection is idempotent");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v{ 0.3, 0.7, -0.4 };
    auto vt = sphere<f64, 3>::project_to_tangent(p, v);
    auto vtt = sphere<f64, 3>::project_to_tangent(p, vt);
    require_true(near_v(vt, vtt));
  }
  end_test_case();

  test_case("sphere<3>: exp(p, 0) = p");
  {
    vec<f64, 3> p{ 0, 1, 0 };
    auto q = sphere<f64, 3>::exp_map(p, vec<f64, 3>{ 0, 0, 0 });
    require_true(near_v(q, p));
  }
  end_test_case();

  test_case("sphere<3>: exp produces a unit-norm point");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v{ 0, 0.5, 0 };      // tangent at p
    auto q = sphere<f64, 3>::exp_map(p, v);
    require_true(near(linalg::ops::norm(q), 1.0, 1e-12));
  }
  end_test_case();

  test_case("sphere<3>: exp(e1, π/2 · e2) = e2");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    f64 const pi = constant_pi<f64>;
    vec<f64, 3> v{ 0, pi / 2, 0 };      // length π/2 along y
    auto q = sphere<f64, 3>::exp_map(p, v);
    require_true(near_v(q, vec<f64, 3>{ 0, 1, 0 }, 1e-12));
  }
  end_test_case();

  test_case("sphere<3>: log/exp round-trip — non-antipodal");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> q_targets[] = {
      vec<f64, 3>{ 0, 1, 0 },
      vec<f64, 3>{ 0, 0, 1 },
      // 60° away from p in the xy plane
      vec<f64, 3>{ 0.5, math::fsqrt(0.75), 0.0 },
      // generic point
      sphere<f64, 3>::project(vec<f64, 3>{ 0.7, -0.4, 0.5 }),
    };
    for ( auto &q : q_targets ) {
      auto v = sphere<f64, 3>::log_map(p, q);
      // tangent must be orthogonal to p
      require_true(near(linalg::ops::dot(p, v), 0.0, 1e-10));
      auto q2 = sphere<f64, 3>::exp_map(p, v);
      require_true(near_v(q, q2, 1e-10));
    }
  }
  end_test_case();

  test_case("sphere<3>: distance(p, p) = 0");
  {
    vec<f64, 3> p{ 0, 0, 1 };
    require_true(near(sphere<f64, 3>::distance(p, p), 0.0));
  }
  end_test_case();

  test_case("sphere<3>: distance(e1, e2) = π/2");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> q{ 0, 1, 0 };
    f64 const pi = constant_pi<f64>;
    require_true(near(sphere<f64, 3>::distance(p, q), pi / 2));
  }
  end_test_case();

  test_case("sphere<3>: distance is symmetric");
  {
    auto p = sphere<f64, 3>::project(vec<f64, 3>{ 1, 2, 3 });
    auto q = sphere<f64, 3>::project(vec<f64, 3>{ -1, 0.5, 0.7 });
    f64 d_pq = sphere<f64, 3>::distance(p, q);
    f64 d_qp = sphere<f64, 3>::distance(q, p);
    require_true(near(d_pq, d_qp));
  }
  end_test_case();

  test_case("sphere<3>: log_map throws on antipodal points");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> q{ -1, 0, 0 };
    bool threw = false;
    try {
      (void)sphere<f64, 3>::log_map(p, q);
    } catch ( const except::domain_error & ) {
      threw = true;
    }
    require_true(threw);
  }
  end_test_case();

  test_case("sphere<3>: retract produces unit-norm");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v{ 0, 0.4, 0 };      // tangent
    auto q = sphere<f64, 3>::retract(p, v);
    require_true(near(linalg::ops::norm(q), 1.0, 1e-12));
  }
  end_test_case();

  test_case("sphere<3>: parallel transport preserves inner product");
  {
    auto p = vec<f64, 3>{ 1, 0, 0 };
    auto q = sphere<f64, 3>::project(vec<f64, 3>{ 0.6, 0.8, 0 });
    vec<f64, 3> u_amb{ 0, 0.4, 0.3 };
    vec<f64, 3> w_amb{ 0, -0.2, 0.5 };
    auto u = sphere<f64, 3>::project_to_tangent(p, u_amb);
    auto w = sphere<f64, 3>::project_to_tangent(p, w_amb);
    f64 ip_p = sphere<f64, 3>::inner(p, u, w);
    auto u_q = sphere<f64, 3>::parallel_transport(p, q, u);
    auto w_q = sphere<f64, 3>::parallel_transport(p, q, w);
    f64 ip_q = sphere<f64, 3>::inner(q, u_q, w_q);
    require_true(near(ip_p, ip_q, 1e-10));
  }
  end_test_case();

  test_case("sphere<3>: parallel-transported tangent stays in T_q");
  {
    auto p = vec<f64, 3>{ 1, 0, 0 };
    auto q = sphere<f64, 3>::project(vec<f64, 3>{ 0.5, 0.5, 0.5 });
    vec<f64, 3> v_amb{ 0, 0.3, -0.5 };
    auto v = sphere<f64, 3>::project_to_tangent(p, v_amb);
    auto vq = sphere<f64, 3>::parallel_transport(p, q, v);
    require_true(near(linalg::ops::dot(q, vq), 0.0, 1e-10));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Torus
  test_case("torus<3>: identity-rooted exp/log round-trip");
  {
    vec<f64, 3> p{ 0, 0, 0 };
    vec<f64, 3> v{ 0.3, -0.5, 1.1 };
    auto q = torus<f64, 3>::exp_map(p, v);
    auto v2 = torus<f64, 3>::log_map(p, q);
    require_true(near_v(v2, v, 1e-12));
  }
  end_test_case();

  test_case("torus<3>: log gives shortest arc — 350° → -10°");
  {
    f64 const pi = constant_pi<f64>;
    vec<f64, 3> p{ 0, 0, 0 };
    vec<f64, 3> q{ 350.0 * pi / 180.0, 0, 0 };      // canonicalised → -10°
    auto v = torus<f64, 3>::log_map(p, q);
    f64 expected = -10.0 * pi / 180.0;
    require_true(near(v.data[0], expected, 1e-12));
  }
  end_test_case();

  test_case("torus<3>: exp wraps to (−π, π]");
  {
    f64 const pi = constant_pi<f64>;
    vec<f64, 3> p{ pi - 0.1, 0, 0 };
    vec<f64, 3> v{ 0.3, 0, 0 };
    auto q = torus<f64, 3>::exp_map(p, v);
    // p + v = pi + 0.2 → wraps to -pi + 0.2
    require_true(near(q.data[0], -pi + 0.2, 1e-12));
  }
  end_test_case();

  test_case("torus<3>: distance(p, p) = 0; symmetric");
  {
    vec<f64, 3> p{ 0.4, -0.7, 1.2 };
    require_true(near(torus<f64, 3>::distance(p, p), 0.0));
    vec<f64, 3> q{ -0.2, 0.5, -1.0 };
    require_true(near(torus<f64, 3>::distance(p, q), torus<f64, 3>::distance(q, p)));
  }
  end_test_case();

  test_case("torus<3>: inner is the ambient dot");
  {
    vec<f64, 3> p{ 0, 0, 0 };
    vec<f64, 3> u{ 1, 2, 3 };
    vec<f64, 3> v{ 4, 5, 6 };
    require_true(near(torus<f64, 3>::inner(p, u, v), 32.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SPD
  test_case("spd<3>: I is on the manifold");
  {
    auto I = mat<f64, 3, 3>::identity();
    auto P = spd<f64, 3>::project_to_manifold(I);
    require_true(near_m(P, I));
  }
  end_test_case();

  test_case("spd<3>: exp(I, 0) = I");
  {
    auto I = mat<f64, 3, 3>::identity();
    auto Z = mat<f64, 3, 3>::zero();
    auto Q = spd<f64, 3>::exp_map(I, Z);
    require_true(near_m(Q, I, 1e-10));
  }
  end_test_case();

  test_case("spd<3>: log(I, I) = 0");
  {
    auto I = mat<f64, 3, 3>::identity();
    auto V = spd<f64, 3>::log_map(I, I);
    auto Z = mat<f64, 3, 3>::zero();
    require_true(near_m(V, Z, 1e-10));
  }
  end_test_case();

  test_case("spd<3>: AI distance(I, 4·I) = √3 · log(4)");
  {
    auto I = mat<f64, 3, 3>::identity();
    auto A = mat<f64, 3, 3>::zero();
    A.data[0] = 4;
    A.data[4] = 4;
    A.data[8] = 4;
    f64 d = spd<f64, 3>::distance<affine_invariant_metric>(I, A);
    f64 expected = math::fsqrt(3.0) * math::log(4.0);
    require_true(near(d, expected, 1e-9));
  }
  end_test_case();

  test_case("spd<3>: log-Euclidean and AI agree on commuting matrices");
  {
    auto P = mat<f64, 3, 3>::zero();
    P.data[0] = 2.0;
    P.data[4] = 5.0;
    P.data[8] = 0.5;      // diagonal SPD
    auto Q = mat<f64, 3, 3>::zero();
    Q.data[0] = 7.0;
    Q.data[4] = 1.5;
    Q.data[8] = 3.0;      // diagonal SPD
    f64 d_ai = spd<f64, 3>::distance<affine_invariant_metric>(P, Q);
    f64 d_le = spd<f64, 3>::distance<log_euclidean_metric>(P, Q);
    require_true(near(d_ai, d_le, 1e-8));
  }
  end_test_case();

  test_case("spd<3>: distance(P, P) = 0; symmetric");
  {
    auto P = mat<f64, 3, 3>::zero();
    P.data[0] = 2.0;
    P.data[1] = 0.3;
    P.data[3] = 0.3;
    P.data[4] = 1.5;
    P.data[8] = 0.7;
    require_true(near(spd<f64, 3>::distance(P, P), 0.0, 1e-9));
    auto Q = mat<f64, 3, 3>::zero();
    Q.data[0] = 1.2;
    Q.data[4] = 2.1;
    Q.data[8] = 0.9;
    f64 d_pq = spd<f64, 3>::distance(P, Q);
    f64 d_qp = spd<f64, 3>::distance(Q, P);
    require_true(near(d_pq, d_qp, 1e-9));
  }
  end_test_case();

  test_case("spd<3>: project_to_manifold of A + ε·rand stays SPD");
  {
    auto I = mat<f64, 3, 3>::identity();
    // Symmetric perturbation that drives one eigenvalue slightly negative
    auto A = mat<f64, 3, 3>::zero();
    A.data[0] = 1.0;
    A.data[4] = -0.0001;      // negative eigenvalue
    A.data[8] = 1.0;
    auto P = spd<f64, 3>::project_to_manifold(A);
    // verify all diagonal entries (eigenvalues since diagonal) are positive
    require_true(P.data[0] > 0);
    require_true(P.data[4] > 0);
    require_true(P.data[8] > 0);
    (void)I;
  }
  end_test_case();

  test_case("spd<3>: exp/log round-trip — small symmetric V");
  {
    auto P = mat<f64, 3, 3>::zero();
    P.data[0] = 2.0;
    P.data[4] = 1.5;
    P.data[8] = 0.7;
    auto V = mat<f64, 3, 3>::zero();
    V.data[0] = 0.05;
    V.data[1] = 0.02;
    V.data[3] = 0.02;
    V.data[4] = -0.03;
    V.data[8] = 0.04;
    auto Q = spd<f64, 3>::exp_map(P, V);
    auto V2 = spd<f64, 3>::log_map(P, Q);
    require_true(near_m(V, V2, 1e-7));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Hyperbolic (Lorentz model)
  test_case("hyperbolic<2>: origin (1, 0, 0) is on H^2");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    require_true(near(hyperbolic<f64, 2>::minkowski(p, p), -1.0));
  }
  end_test_case();

  test_case("hyperbolic<2>: tangent at origin satisfies ⟨p, v⟩_L = 0");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v_amb{ 0.7, 0.4, 0.5 };
    auto v = hyperbolic<f64, 2>::project_to_tangent(p, v_amb);
    require_true(near(hyperbolic<f64, 2>::minkowski(p, v), 0.0, 1e-12));
  }
  end_test_case();

  test_case("hyperbolic<2>: exp(p, 0) = p");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> z{ 0, 0, 0 };
    auto q = hyperbolic<f64, 2>::exp_map(p, z);
    require_true(near_v(q, p, 1e-12));
  }
  end_test_case();

  test_case("hyperbolic<2>: distance(p, p) = 0");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    require_true(near(hyperbolic<f64, 2>::distance(p, p), 0.0, 1e-10));
  }
  end_test_case();

  test_case("hyperbolic<2>: log/exp round-trip — small tangent");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v_amb{ 0, 0.3, -0.4 };      // already orthogonal to p in Minkowski
    auto v = hyperbolic<f64, 2>::project_to_tangent(p, v_amb);
    auto q = hyperbolic<f64, 2>::exp_map(p, v);
    auto v2 = hyperbolic<f64, 2>::log_map(p, q);
    require_true(near_v(v, v2, 1e-9));
  }
  end_test_case();

  test_case("hyperbolic<2>: exp keeps points on the hyperboloid (⟨q, q⟩_L = -1)");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v{ 0, 0.5, 0.3 };
    auto q = hyperbolic<f64, 2>::exp_map(p, v);
    require_true(near(hyperbolic<f64, 2>::minkowski(q, q), -1.0, 1e-10));
  }
  end_test_case();

  test_case("hyperbolic<2>: cosh(d(p, q)) = -⟨p, q⟩_L");
  {
    vec<f64, 3> p{ 1, 0, 0 };
    vec<f64, 3> v{ 0, 0.6, -0.4 };
    auto q = hyperbolic<f64, 2>::exp_map(p, v);
    f64 d = hyperbolic<f64, 2>::distance(p, q);
    f64 mpq = hyperbolic<f64, 2>::minkowski(p, q);
    require_true(near(math::cosh(d), -mpq, 1e-10));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Stiefel
  test_case("stiefel<5,3>: X = [I_3; 0_2] satisfies X^T X = I");
  {
    mat<f64, 5, 3> X{};
    X.data[0 * 3 + 0] = 1;
    X.data[1 * 3 + 1] = 1;
    X.data[2 * 3 + 2] = 1;
    auto XtX = linalg::ops::gemm(linalg::ops::transpose(X), X);
    require_true(near_m(XtX, mat<f64, 3, 3>::identity(), 1e-12));
  }
  end_test_case();

  test_case("stiefel<5,3>: project_to_tangent — X^T V_T + V_T^T X = 0");
  {
    mat<f64, 5, 3> X{};
    X.data[0] = 1;
    X.data[4] = 1;
    X.data[8] = 1;
    mat<f64, 5, 3> V{};
    for ( usize i = 0; i < 15; ++i ) V.data[i] = 0.01 * f64(i + 1);
    auto VT = stiefel<f64, 5, 3>::project_to_tangent(X, V);
    auto XtVT = linalg::ops::gemm(linalg::ops::transpose(X), VT);
    auto VTtX = linalg::ops::gemm(linalg::ops::transpose(VT), X);
    auto S = mat<f64, 3, 3>::zero();
    for ( usize i = 0; i < 9; ++i ) S.data[i] = XtVT.data[i] + VTtX.data[i];
    require_true(near_m(S, mat<f64, 3, 3>::zero(), 1e-12));
  }
  end_test_case();

  test_case("stiefel<5,3>: tangent projection is idempotent");
  {
    mat<f64, 5, 3> X{};
    X.data[0] = 1;
    X.data[4] = 1;
    X.data[8] = 1;
    mat<f64, 5, 3> V{};
    for ( usize i = 0; i < 15; ++i ) V.data[i] = 0.1 * f64(i) - 0.5;
    auto VT = stiefel<f64, 5, 3>::project_to_tangent(X, V);
    auto VTT = stiefel<f64, 5, 3>::project_to_tangent(X, VT);
    require_true(near_m(VT, VTT, 1e-12));
  }
  end_test_case();

  test_case("stiefel<5,3>: retract_qr produces orthonormal columns");
  {
    mat<f64, 5, 3> X{};
    X.data[0] = 1;
    X.data[4] = 1;
    X.data[8] = 1;
    mat<f64, 5, 3> V{};
    V.data[12] = 0.3;
    V.data[10] = 0.2;
    V.data[14] = 0.1;
    auto VT = stiefel<f64, 5, 3>::project_to_tangent(X, V);
    auto Q = stiefel<f64, 5, 3>::retract_qr(X, VT);
    auto QtQ = linalg::ops::gemm(linalg::ops::transpose(Q), Q);
    require_true(near_m(QtQ, mat<f64, 3, 3>::identity(), 1e-10));
  }
  end_test_case();

  test_case("stiefel<5,3>: retract_polar produces orthonormal columns");
  {
    mat<f64, 5, 3> X{};
    X.data[0] = 1;
    X.data[4] = 1;
    X.data[8] = 1;
    mat<f64, 5, 3> V{};
    V.data[12] = 0.3;
    V.data[10] = 0.2;
    V.data[14] = 0.1;
    auto VT = stiefel<f64, 5, 3>::project_to_tangent(X, V);
    auto Q = stiefel<f64, 5, 3>::retract_polar(X, VT);
    auto QtQ = linalg::ops::gemm(linalg::ops::transpose(Q), Q);
    require_true(near_m(QtQ, mat<f64, 3, 3>::identity(), 1e-10));
  }
  end_test_case();

  test_case("stiefel<5,3>: exp(X, 0) = X");
  {
    mat<f64, 5, 3> X{};
    X.data[0] = 1;
    X.data[4] = 1;
    X.data[8] = 1;
    auto Z = mat<f64, 5, 3>::zero();
    auto Y = stiefel<f64, 5, 3>::exp_map(X, Z);
    require_true(near_m(X, Y, 1e-10));
  }
  end_test_case();

  test_case("stiefel<5,3>: project_to_manifold of perturbed X is orthonormal");
  {
    mat<f64, 5, 3> X{};
    X.data[0] = 1;
    X.data[4] = 1;
    X.data[8] = 1;
    // perturb
    for ( usize i = 0; i < 15; ++i ) X.data[i] += 0.01 * f64(i % 3);
    auto Y = stiefel<f64, 5, 3>::project_to_manifold(X);
    auto YtY = linalg::ops::gemm(linalg::ops::transpose(Y), Y);
    require_true(near_m(YtY, mat<f64, 3, 3>::identity(), 1e-10));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Grassmann
  test_case("grassmann<5,2>: project_to_tangent — X^T V_T = 0");
  {
    mat<f64, 5, 2> X{};
    X.data[0] = 1;
    X.data[3] = 1;      // X = [e1, e2]
    mat<f64, 5, 2> V{};
    for ( usize i = 0; i < 10; ++i ) V.data[i] = 0.01 * f64(i + 1);
    auto VT = grassmann<f64, 5, 2>::project_to_tangent(X, V);
    auto XtVT = linalg::ops::gemm(linalg::ops::transpose(X), VT);
    require_true(near_m(XtVT, mat<f64, 2, 2>::zero(), 1e-12));
  }
  end_test_case();

  test_case("grassmann<5,2>: exp(X, 0) = X");
  {
    mat<f64, 5, 2> X{};
    X.data[0] = 1;
    X.data[3] = 1;
    auto Z = mat<f64, 5, 2>::zero();
    auto Y = grassmann<f64, 5, 2>::exp_map(X, Z);
    require_true(near_m(X, Y, 1e-10));
  }
  end_test_case();

  test_case("grassmann<5,2>: exp produces orthonormal output");
  {
    mat<f64, 5, 2> X{};
    X.data[0] = 1;
    X.data[3] = 1;
    mat<f64, 5, 2> V{};
    V.data[6] = 0.3;
    V.data[8] = 0.2;      // tangent rows 3, 4 — automatically X^T V = 0
    auto Y = grassmann<f64, 5, 2>::exp_map(X, V);
    auto YtY = linalg::ops::gemm(linalg::ops::transpose(Y), Y);
    require_true(near_m(YtY, mat<f64, 2, 2>::identity(), 1e-10));
  }
  end_test_case();

  test_case("grassmann<5,2>: principal_angles(X, X) = 0");
  {
    mat<f64, 5, 2> X{};
    X.data[0] = 1;
    X.data[3] = 1;
    auto a = grassmann<f64, 5, 2>::principal_angles(X, X);
    require_true(near(a.data[0], 0.0, 1e-9));
    require_true(near(a.data[1], 0.0, 1e-9));
  }
  end_test_case();

  test_case("grassmann<5,2>: principal_angles ∈ [0, π/2]");
  {
    mat<f64, 5, 2> X{};
    X.data[0] = 1;
    X.data[3] = 1;
    mat<f64, 5, 2> Y{};
    Y.data[2] = 1;
    Y.data[5] = 1;      // {e_2, e_3}, orthogonal to e_1
    f64 const pi = constant_pi<f64>;
    auto a = grassmann<f64, 5, 2>::principal_angles(X, Y);
    for ( usize i = 0; i < 2; ++i ) {
      require_true(a.data[i] >= 0.0);
      require_true(a.data[i] <= pi / 2 + 1e-10);
    }
  }
  end_test_case();

  test_case("grassmann<5,2>: distance(X, X) = 0; symmetric");
  {
    mat<f64, 5, 2> X{};
    X.data[0] = 1;
    X.data[3] = 1;
    require_true(near(grassmann<f64, 5, 2>::distance(X, X), 0.0, 1e-9));
    mat<f64, 5, 2> Y{};
    Y.data[1] = 1;
    Y.data[4] = 1;
    f64 d_xy = grassmann<f64, 5, 2>::distance(X, Y);
    f64 d_yx = grassmann<f64, 5, 2>::distance(Y, X);
    require_true(near(d_xy, d_yx, 1e-9));
  }
  end_test_case();

  return 1;
}
