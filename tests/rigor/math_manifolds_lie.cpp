// math_manifolds_lie.cpp
// Snowball tests for math::manifolds — SO2, SO3, SOn, SE2, SE3, SEn,
// hat/vee, Lie bracket, generic algebra_exp/log.

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
using namespace micron::math::manifolds::lie;

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

// quaternion equivalence — q and −q represent the same rotation.
static bool
near_q(const math::quat<f64> &a, const math::quat<f64> &b, f64 eps = 1e-10)
{
  bool same = true, neg = true;
  for ( usize i = 0; i < 4; ++i ) {
    if ( !near(a.data[i], b.data[i], eps) ) same = false;
    if ( !near(a.data[i], -b.data[i], eps) ) neg = false;
  }
  return same || neg;
}

int
main()
{
  print("=== MATH::MANIFOLDS::LIE TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // hat / vee
  test_case("hat_so3 / vee_so3 round-trip");
  {
    vec<f64, 3> w{ 1.5, -2.0, 0.7 };
    auto W = hat_so3<f64>(w);
    auto w2 = vee_so3<f64>(W);
    require_true(near_v(w, w2));
    // skew check: W + W^T = 0
    for ( usize r = 0; r < 3; ++r )
      for ( usize c = 0; c < 3; ++c ) require_true(near(W.data[r * 3 + c] + W.data[c * 3 + r], 0.0));
  }
  end_test_case();

  test_case("hat_se3 / vee_se3 round-trip");
  {
    vec<f64, 6> xi{ 0.3, -0.7, 1.1, 0.2, -0.4, 0.6 };
    auto X = hat_se3<f64>(xi);
    auto xi2 = vee_se3<f64>(X);
    require_true(near_v(xi, xi2));
    // bottom row is zero
    for ( usize c = 0; c < 4; ++c ) require_true(near(X.data[3 * 4 + c], 0.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Lie bracket
  test_case("lie_bracket: [X, X] = 0");
  {
    auto X = hat_so3<f64>(vec<f64, 3>{ 1, 2, 3 });
    auto Z = lie_bracket<f64, 3>(X, X);
    for ( usize i = 0; i < 9; ++i ) require_true(near(Z.data[i], 0.0));
  }
  end_test_case();

  test_case("lie_bracket: anti-symmetric");
  {
    auto X = hat_so3<f64>(vec<f64, 3>{ 1, 0, 0 });
    auto Y = hat_so3<f64>(vec<f64, 3>{ 0, 1, 0 });
    auto XY = lie_bracket<f64, 3>(X, Y);
    auto YX = lie_bracket<f64, 3>(Y, X);
    for ( usize i = 0; i < 9; ++i ) require_true(near(XY.data[i] + YX.data[i], 0.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SO2
  test_case("SO2: identity");
  {
    auto e = SO2<f64>::identity();
    require_true(near(e.theta, 0.0));
  }
  end_test_case();

  test_case("SO2: exp(0) = identity, log(identity) = 0");
  {
    auto g = SO2<f64>::exp_map(0.0);
    require_true(near(g.theta, 0.0));
    auto x = SO2<f64>::log_map(SO2<f64>::identity());
    require_true(near(x, 0.0));
  }
  end_test_case();

  test_case("SO2: exp/log round-trip");
  {
    f64 angles[] = { 0.0, 0.1, 1.5, 3.0, -0.5, -2.5 };
    for ( f64 a : angles ) {
      auto g = SO2<f64>::exp_map(a);
      auto a2 = SO2<f64>::log_map(g);
      require_true(near(a, a2));
    }
  }
  end_test_case();

  test_case("SO2: g · g^-1 = identity");
  {
    SO2<f64> g{ 0.7 };
    auto e = SO2<f64>::compose(g, SO2<f64>::inverse(g));
    require_true(near(e.theta, 0.0));
  }
  end_test_case();

  test_case("SO2: rotate matches to_matrix * v");
  {
    SO2<f64> g{ 0.6 };
    vec<f64, 2> v{ 1.0, 0.0 };
    auto r1 = SO2<f64>::rotate(g, v);
    auto R = SO2<f64>::to_matrix(g);
    auto r2 = linalg::ops::gemv(R, v);
    require_true(near_v(r1, r2));
  }
  end_test_case();

  test_case("SO2: from_matrix(to_matrix(g)) = g");
  {
    SO2<f64> g{ 1.2 };
    auto R = SO2<f64>::to_matrix(g);
    auto g2 = SO2<f64>::from_matrix(R);
    require_true(near(g.theta, g2.theta));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SO3
  test_case("SO3: identity quat is (0,0,0,1)");
  {
    auto e = SO3<f64>::identity();
    require_true(near(e.q.data[0], 0.0));
    require_true(near(e.q.data[1], 0.0));
    require_true(near(e.q.data[2], 0.0));
    require_true(near(e.q.data[3], 1.0));
  }
  end_test_case();

  test_case("SO3: exp(0) = identity, log(identity) = 0");
  {
    auto g = SO3<f64>::exp_map(vec<f64, 3>{ 0, 0, 0 });
    require_true(near_q(g.q, SO3<f64>::identity().q));
    auto x = SO3<f64>::log_map(SO3<f64>::identity());
    require_true(near_v(x, vec<f64, 3>{ 0, 0, 0 }));
  }
  end_test_case();

  test_case("SO3: exp/log round-trip — small/medium/near-π omegas");
  {
    vec<f64, 3> tests[] = {
      vec<f64, 3>{ 1e-8, 0, 0 },            // tiny → Taylor branch
      vec<f64, 3>{ 0.01, 0.01, 0.01 },      // small
      vec<f64, 3>{ 0.5, -0.3, 0.2 },        // medium
      vec<f64, 3>{ 1.5, 0.0, 0.0 },         // larger
      vec<f64, 3>{ 3.0, 0.0, 0.0 },         // near π
    };
    for ( auto &w : tests ) {
      auto g = SO3<f64>::exp_map(w);
      auto w2 = SO3<f64>::log_map(g);
      require_true(near_v(w, w2, 1e-9));
    }
  }
  end_test_case();

  test_case("SO3: exp(omega) matches expm(hat_so3(omega))");
  {
    vec<f64, 3> w{ 0.4, -0.7, 0.2 };
    auto g_closed = SO3<f64>::exp_map(w);
    auto R_closed = SO3<f64>::to_matrix(g_closed);
    auto R_generic = algebra_exp<f64, 3>(hat_so3<f64>(w));
    require_true(near_m(R_closed, R_generic, 1e-10));
  }
  end_test_case();

  test_case("SO3: rotation matrix is orthogonal — R^T R = I");
  {
    auto g = SO3<f64>::exp_map(vec<f64, 3>{ 0.5, 0.3, -0.2 });
    auto R = SO3<f64>::to_matrix(g);
    auto Rt = linalg::ops::transpose(R);
    auto I = linalg::ops::gemm(Rt, R);
    require_true(near_m(I, mat<f64, 3, 3>::identity()));
  }
  end_test_case();

  test_case("SO3: from_matrix(to_matrix(g)) recovers g");
  {
    auto g = SO3<f64>::exp_map(vec<f64, 3>{ 0.4, -0.6, 0.3 });
    auto R = SO3<f64>::to_matrix(g);
    auto g2 = SO3<f64>::from_matrix(R);
    require_true(near_q(g.q, g2.q, 1e-10));
  }
  end_test_case();

  test_case("SO3: rotate(g, v) = to_matrix(g) · v");
  {
    auto g = SO3<f64>::exp_map(vec<f64, 3>{ 0.7, -0.2, 0.5 });
    vec<f64, 3> v{ 1.0, 2.0, 3.0 };
    auto r1 = SO3<f64>::rotate(g, v);
    auto r2 = linalg::ops::gemv(SO3<f64>::to_matrix(g), v);
    require_true(near_v(r1, r2));
  }
  end_test_case();

  test_case("SO3: composition associativity");
  {
    auto a = SO3<f64>::exp_map(vec<f64, 3>{ 0.3, 0, 0 });
    auto b = SO3<f64>::exp_map(vec<f64, 3>{ 0, 0.4, 0 });
    auto c = SO3<f64>::exp_map(vec<f64, 3>{ 0, 0, 0.5 });
    auto ab_c = SO3<f64>::compose(SO3<f64>::compose(a, b), c);
    auto a_bc = SO3<f64>::compose(a, SO3<f64>::compose(b, c));
    require_true(near_q(ab_c.q, a_bc.q));
  }
  end_test_case();

  test_case("SO3: g · g^-1 = identity");
  {
    auto g = SO3<f64>::exp_map(vec<f64, 3>{ 1.0, 0.5, -0.3 });
    auto e = SO3<f64>::compose(g, SO3<f64>::inverse(g));
    require_true(near_q(e.q, SO3<f64>::identity().q, 1e-10));
  }
  end_test_case();

  test_case("SO3: adjoint(g) = R(g)");
  {
    auto g = SO3<f64>::exp_map(vec<f64, 3>{ 0.2, -0.4, 0.6 });
    auto Ad = SO3<f64>::adjoint(g);
    auto R = SO3<f64>::to_matrix(g);
    require_true(near_m(Ad, R));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SOn (generic)
  test_case("SOn<4>: identity is the 4×4 identity matrix");
  {
    auto e = SOn<f64, 4>::identity();
    require_true(near_m(e.R, mat<f64, 4, 4>::identity()));
  }
  end_test_case();

  test_case("SOn<4>: exp(skew) is orthogonal");
  {
    // Build a 4×4 skew matrix.
    mat<f64, 4, 4> X = mat<f64, 4, 4>::zero();
    X.data[1] = 0.3;
    X.data[4] = -0.3;
    X.data[2] = -0.5;
    X.data[8] = 0.5;
    X.data[6] = 0.1;
    X.data[9] = -0.1;
    auto g = SOn<f64, 4>::exp_map(X);
    auto Rt = linalg::ops::transpose(g.R);
    auto I = linalg::ops::gemm(Rt, g.R);
    require_true(near_m(I, mat<f64, 4, 4>::identity(), 1e-9));
  }
  end_test_case();

  test_case("SOn<4>: inverse via transpose · g = identity");
  {
    mat<f64, 4, 4> X = mat<f64, 4, 4>::zero();
    X.data[1] = 0.2;
    X.data[4] = -0.2;
    X.data[7] = 0.4;
    X.data[13] = -0.4;
    auto g = SOn<f64, 4>::exp_map(X);
    auto e = SOn<f64, 4>::compose(g, SOn<f64, 4>::inverse(g));
    require_true(near_m(e.R, mat<f64, 4, 4>::identity(), 1e-9));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SE2
  test_case("SE2: identity");
  {
    auto e = SE2<f64>::identity();
    require_true(near(e.R.theta, 0.0));
    require_true(near(e.t.data[0], 0.0));
    require_true(near(e.t.data[1], 0.0));
  }
  end_test_case();

  test_case("SE2: exp(0) = identity, log(identity) = 0");
  {
    auto g = SE2<f64>::exp_map(vec<f64, 3>{ 0, 0, 0 });
    require_true(near(g.R.theta, 0.0));
    require_true(near(g.t.data[0], 0.0));
    require_true(near(g.t.data[1], 0.0));
    auto xi = SE2<f64>::log_map(SE2<f64>::identity());
    require_true(near_v(xi, vec<f64, 3>{ 0, 0, 0 }));
  }
  end_test_case();

  test_case("SE2: exp/log round-trip");
  {
    vec<f64, 3> tests[] = {
      vec<f64, 3>{ 0.5, -0.3, 0.0 },        // pure translation
      vec<f64, 3>{ 0.0, 0.0, 0.7 },         // pure rotation
      vec<f64, 3>{ 0.5, 0.5, 0.4 },         // mixed
      vec<f64, 3>{ 1e-8, 1e-8, 1e-8 },      // tiny
    };
    for ( auto &xi : tests ) {
      auto g = SE2<f64>::exp_map(xi);
      auto xi2 = SE2<f64>::log_map(g);
      require_true(near_v(xi, xi2, 1e-8));
    }
  }
  end_test_case();

  test_case("SE2: g · g^-1 = identity");
  {
    auto g = SE2<f64>::exp_map(vec<f64, 3>{ 0.4, -0.2, 0.5 });
    auto e = SE2<f64>::compose(g, SE2<f64>::inverse(g));
    require_true(near(e.R.theta, 0.0, 1e-9));
    require_true(near_v(e.t, vec<f64, 2>{ 0, 0 }, 1e-9));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SE3
  test_case("SE3: identity");
  {
    auto e = SE3<f64>::identity();
    require_true(near_q(e.R.q, SO3<f64>::identity().q));
    require_true(near_v(e.t, vec<f64, 3>{ 0, 0, 0 }));
  }
  end_test_case();

  test_case("SE3: exp(0) = identity, log(identity) = 0");
  {
    auto g = SE3<f64>::exp_map(vec<f64, 6>{ 0, 0, 0, 0, 0, 0 });
    require_true(near_q(g.R.q, SO3<f64>::identity().q));
    require_true(near_v(g.t, vec<f64, 3>{ 0, 0, 0 }));
    auto xi = SE3<f64>::log_map(SE3<f64>::identity());
    require_true(near_v(xi, vec<f64, 6>{ 0, 0, 0, 0, 0, 0 }));
  }
  end_test_case();

  test_case("SE3: exp/log round-trip");
  {
    vec<f64, 6> tests[] = {
      vec<f64, 6>{ 1.0, 2.0, 3.0, 0.0, 0.0, 0.0 },            // pure translation
      vec<f64, 6>{ 0.0, 0.0, 0.0, 0.5, -0.2, 0.3 },           // pure rotation
      vec<f64, 6>{ 0.4, -0.2, 0.7, 0.3, 0.1, -0.4 },          // mixed
      vec<f64, 6>{ 1e-9, 1e-9, 1e-9, 1e-9, 1e-9, 1e-9 },      // tiny
    };
    for ( auto &xi : tests ) {
      auto g = SE3<f64>::exp_map(xi);
      auto xi2 = SE3<f64>::log_map(g);
      require_true(near_v(xi, xi2, 1e-8));
    }
  }
  end_test_case();

  test_case("SE3: closed-form exp matches expm(hat_se3(xi))");
  {
    vec<f64, 6> xi{ 0.3, -0.5, 0.2, 0.4, 0.1, -0.3 };
    auto g = SE3<f64>::exp_map(xi);
    auto T_closed = SE3<f64>::to_matrix(g);
    auto T_generic = algebra_exp<f64, 4>(hat_se3<f64>(xi));
    require_true(near_m(T_closed, T_generic, 1e-10));
  }
  end_test_case();

  test_case("SE3: g · g^-1 = identity");
  {
    auto g = SE3<f64>::exp_map(vec<f64, 6>{ 0.5, 0.3, -0.7, 0.2, -0.4, 0.6 });
    auto e = SE3<f64>::compose(g, SE3<f64>::inverse(g));
    require_true(near_q(e.R.q, SO3<f64>::identity().q, 1e-9));
    require_true(near_v(e.t, vec<f64, 3>{ 0, 0, 0 }, 1e-9));
  }
  end_test_case();

  test_case("SE3: composition associativity");
  {
    auto a = SE3<f64>::exp_map(vec<f64, 6>{ 0.1, 0.2, 0.3, 0.1, 0, 0 });
    auto b = SE3<f64>::exp_map(vec<f64, 6>{ 0.4, 0, 0, 0, 0.2, 0 });
    auto c = SE3<f64>::exp_map(vec<f64, 6>{ 0, 0, 0.5, 0, 0, 0.3 });
    auto ab_c = SE3<f64>::compose(SE3<f64>::compose(a, b), c);
    auto a_bc = SE3<f64>::compose(a, SE3<f64>::compose(b, c));
    require_true(near_q(ab_c.R.q, a_bc.R.q, 1e-9));
    require_true(near_v(ab_c.t, a_bc.t, 1e-9));
  }
  end_test_case();

  test_case("SE3: to_matrix has homogeneous structure — bottom row [0,0,0,1]");
  {
    auto g = SE3<f64>::exp_map(vec<f64, 6>{ 0.5, 0.3, -0.2, 0.4, -0.1, 0.6 });
    auto T = SE3<f64>::to_matrix(g);
    require_true(near(T.data[3 * 4 + 0], 0.0));
    require_true(near(T.data[3 * 4 + 1], 0.0));
    require_true(near(T.data[3 * 4 + 2], 0.0));
    require_true(near(T.data[3 * 4 + 3], 1.0));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SEn (generic)
  test_case("SEn<3>: identity is 4×4 identity");
  {
    auto e = SEn<f64, 3>::identity();
    require_true(near_m(e.T, mat<f64, 4, 4>::identity()));
  }
  end_test_case();

  test_case("SEn<3>: g · g^-1 = identity");
  {
    auto g = SE3<f64>::exp_map(vec<f64, 6>{ 0.4, -0.3, 0.5, 0.2, 0.4, -0.1 });
    SEn<f64, 3> sn{ SE3<f64>::to_matrix(g) };
    auto e = SEn<f64, 3>::compose(sn, SEn<f64, 3>::inverse(sn));
    require_true(near_m(e.T, mat<f64, 4, 4>::identity(), 1e-10));
  }
  end_test_case();

  return 0;
}
