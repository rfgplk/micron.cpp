// math_defects_manifolds.cpp
// Regression gates for the manifolds/Lie-group defects fixed in this pass:
//   - SE2::exp_map / log_map truncated the V-matrix series two terms short of f64 precision
//   - SOn/SEn::{exp,log}_map and the ops.hpp wrappers were noexcept over throwing bodies
//   - sphere::distance / hyperbolic::distance collapsed to 0 for small separations
//   - grassmann::log_map was a first-order projected difference, not the Riemannian logarithm
//   - spd advertised N >= 1 and four metric tags while supporting N >= 2 and two
//   - torus advertised N >= 1

#include "../../src/math/manifolds/manifolds.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;
using namespace micron::math::manifolds;

namespace mf = micron::math::manifolds;

static bool
rel_within(f64 got, f64 want, f64 tol)
{
  const f64 d = got - want;
  const f64 a = (d < 0 ? -d : d);
  const f64 s = (want < 0 ? -want : want);
  return a <= tol * (s > 1.0 ? s : 1.0);
}

static bool
abs_within(f64 got, f64 want, f64 tol)
{
  const f64 d = got - want;
  return (d < 0 ? -d : d) <= tol;
}

// xorshift64*, seeded with a fixed literal
static u64 __seed = 0xC0FFEE1234ABCD5Full;

static f64
next_signed(void)
{
  __seed ^= __seed >> 12;
  __seed ^= __seed << 25;
  __seed ^= __seed >> 27;
  const u64 m = (__seed * 0x2545F4914F6CDD1Dull) >> 11;
  return static_cast<f64>(static_cast<f64>(m) * (1.0 / 9007199254740992.0) * 2.0 - 1.0);
}

// a requires-expression only softens a failure that comes from substituting a template
// parameter, so N and Metric have to be parameters here rather than spelled inline
template<usize N> constexpr bool __spd_ok = requires { typename mf::spd<f64, N>; };
template<usize N> constexpr bool __torus_ok = requires { typename mf::torus<f64, N>; };
template<typename Metric> constexpr bool __spd_distance_ok = requires(mat<f64, 2, 2> P) { mf::spd<f64, 2>::distance<Metric>(P, P); };
template<typename Metric> constexpr bool __spd_inner_ok = requires(mat<f64, 2, 2> P) { mf::spd<f64, 2>::inner<Metric>(P, P, P); };
template<typename Metric> constexpr bool __spd_exp_ok = requires(mat<f64, 2, 2> P) { mf::spd<f64, 2>::exp_map<Metric>(P, P); };

template<usize R, usize C>
static f64
max_abs_diff(const mat<f64, R, C> &a, const mat<f64, R, C> &b)
{
  f64 m = 0.0;
  for ( usize i = 0; i < R * C; ++i ) {
    f64 e = a.data[i] - b.data[i];
    if ( e < 0 ) e = -e;
    if ( e > m ) m = e;
  }
  return m;
}

template<usize R, usize C>
static f64
frob(const mat<f64, R, C> &a)
{
  f64 s = 0.0;
  for ( usize i = 0; i < R * C; ++i ) s += a.data[i] * a.data[i];
  return math::fsqrt(s);
}

int
main()
{
  print("=== MATH MANIFOLD DEFECT GATES ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SE2: the |omega| < 0.1 branch must reach f64 precision, not 8.3e-7
  test_case("SE2::exp_map series matches sin(w)/w in its own branch");
  {
    // (1 - cos w)/w is the cancelling half, so only sin(w)/w is compared against the closed form
    const f64 ws[] = { 0.001, 0.005, 0.01, 0.02, 0.05, 0.09, 0.0999 };
    for ( usize i = 0; i < 7; ++i ) {
      volatile f64 wv = ws[i];
      const f64 w = wv;
      f64 s, c;
      math::sincos<f64>(w, s, c);
      const auto g = lie::SE2<f64>::exp_map(vec<f64, 3>{ 1.0, 0.0, w });
      require_true(rel_within(g.t.data[0], s / w, 1e-14));
    }
  }
  end_test_case();

  test_case("SE2::exp_map is continuous across its |omega| < 0.1 branch");
  {
    volatile f64 lo = 0.099999999999999992, hi = 0.10000000000000001;
    const auto a = lie::SE2<f64>::exp_map(vec<f64, 3>{ 1.0, 0.0, (f64)lo });
    const auto b = lie::SE2<f64>::exp_map(vec<f64, 3>{ 1.0, 0.0, (f64)hi });
    require_true(abs_within(a.t.data[0], b.t.data[0], 1e-14));
    require_true(abs_within(a.t.data[1], b.t.data[1], 1e-14));
  }
  end_test_case();

  test_case("SE2::to_matrix(exp_map) agrees with the matrix exponential in the series branch");
  {
    volatile f64 wv = 0.09;
    const f64 w = wv;
    const auto M = lie::SE2<f64>::to_matrix(lie::SE2<f64>::exp_map(vec<f64, 3>{ 1.0, -0.5, w }));
    mat<f64, 3, 3> hat{};
    hat.data[1] = -w;
    hat.data[2] = 1.0;
    hat.data[3] = w;
    hat.data[5] = -0.5;
    const auto E = linalg::matfunc::expm<f64, 3>(hat).X;
    require_true(max_abs_diff<3, 3>(M, E) <= 1e-13);
  }
  end_test_case();

  test_case("SE2 exp/log round-trip in the series branch");
  {
    const f64 ws[] = { 1e-8, 0.02, 0.05, 0.09, 0.0999 };
    for ( usize i = 0; i < 5; ++i ) {
      volatile f64 wv = ws[i];
      const f64 w = wv;
      const auto xi = vec<f64, 3>{ 1.0, -0.5, w };
      const auto r = lie::SE2<f64>::log_map(lie::SE2<f64>::exp_map(xi));
      require_true(abs_within(r.data[0], 1.0, 1e-14));
      require_true(abs_within(r.data[1], -0.5, 1e-14));
      require_true(abs_within(r.data[2], w, 1e-14));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // noexcept over a throwing body is std::terminate, not a catchable error
  test_case("SOn/SEn maps do not claim noexcept over algebra_exp/algebra_log");
  {
    const mat<f64, 3, 3> M{};
    const lie::SOn<f64, 3> g{ M };
    require_true(noexcept(lie::SOn<f64, 3>::log_map(g)) == noexcept(lie::algebra_log<f64, 3>(M)));
    require_true(noexcept(lie::SOn<f64, 3>::exp_map(M)) == noexcept(lie::algebra_exp<f64, 3>(M)));
    require_true(noexcept(lie::SOn<f64, 3>::distance(g, g)) == noexcept(lie::algebra_log<f64, 3>(M)));
    require_true(noexcept(lie::SOn<f64, 3>::squared_distance(g, g)) == noexcept(lie::algebra_log<f64, 3>(M)));
    require_true(noexcept(lie::SOn<f64, 3>::interpolate(g, g, 0.5)) == noexcept(lie::algebra_log<f64, 3>(M)));
    const mat<f64, 4, 4> T{};
    const lie::SEn<f64, 3> h{ T };
    require_true(noexcept(lie::SEn<f64, 3>::log_map(h)) == noexcept(lie::algebra_log<f64, 4>(T)));
    require_true(noexcept(lie::SEn<f64, 3>::exp_map(T)) == noexcept(lie::algebra_exp<f64, 4>(T)));
  }
  end_test_case();

  test_case("manifolds:: wrappers do not claim noexcept over a throwing manifold");
  {
    const vec<f64, 3> p{ 1.0, 0.0, 0.0 }, q{ -1.0, 0.0, 0.0 };
    require_true(noexcept(mf::log_map<sphere<f64, 3>>(p, q)) == noexcept(sphere<f64, 3>::log_map(p, q)));
    require_true(noexcept(mf::project_to_manifold<hyperbolic<f64, 2>>(p)) == noexcept(hyperbolic<f64, 2>::project_to_manifold(p)));
    require_true(noexcept(mf::retract<hyperbolic<f64, 2>>(p, q)) == noexcept(hyperbolic<f64, 2>::retract(p, q)));
    require_true(noexcept(mf::inverse_retract<hyperbolic<f64, 2>>(p, q)) == noexcept(hyperbolic<f64, 2>::inverse_retract(p, q)));
    require_true(noexcept(mf::interpolate<sphere<f64, 3>>(p, q, 0.5)) == false);
    const mat<f64, 3, 3> M{};
    const lie::SOn<f64, 3> g{ M };
    require_true(noexcept(mf::log_map<lie::SOn<f64, 3>>(g)) == noexcept(lie::SOn<f64, 3>::log_map(g)));
    require_true(noexcept(mf::exp_map<lie::SOn<f64, 3>>(M)) == noexcept(lie::SOn<f64, 3>::exp_map(M)));
  }
  end_test_case();

#if !defined(__micron_freestanding) || defined(__micron_eh)
  test_case("a rotation by pi reports a catchable error instead of aborting");
  {
    const auto R = lie::SO3<f64>::to_matrix(lie::SO3<f64>::exp_map(vec<f64, 3>{ math::constant_pi<f64>, 0.0, 0.0 }));
    const auto g = lie::SOn<f64, 3>::from_matrix(R);
    bool caught = false;
    try {
      const auto L = lie::SOn<f64, 3>::log_map(g);
      (void)L;
    } catch ( ... ) {
      caught = true;
    }
    require_true(caught);
    caught = false;
    try {
      const auto L = mf::log_map<lie::SOn<f64, 3>>(g);
      (void)L;
    } catch ( ... ) {
      caught = true;
    }
    require_true(caught);
  }
  end_test_case();

  test_case("antipodal sphere log through the ops wrapper is catchable");
  {
    const vec<f64, 3> p{ 1.0, 0.0, 0.0 }, q{ -1.0, 0.0, 0.0 };
    bool caught = false;
    try {
      const auto v = mf::log_map<sphere<f64, 3>>(p, q);
      (void)v;
    } catch ( ... ) {
      caught = true;
    }
    require_true(caught);
    caught = false;
    const vec<f64, 3> bad{ 0.0, 1.0, 0.0 };
    try {
      const auto v = mf::project_to_manifold<hyperbolic<f64, 2>>(bad);
      (void)v;
    } catch ( ... ) {
      caught = true;
    }
    require_true(caught);
  }
  end_test_case();
#endif

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // spd/torus advertised template arguments they cannot instantiate, and spd
  // silently answered the affine-invariant question for two other metric tags
  test_case("spd rejects N = 1 and the metrics it does not implement");
  {
    require_true(!__spd_ok<1>);
    require_true(__spd_ok<2>);
    require_true(!__spd_distance_ok<euclidean_metric>);
    require_true(!__spd_distance_ok<bures_metric>);
    require_true(!__spd_inner_ok<euclidean_metric>);
    require_true(!__spd_exp_ok<bures_metric>);
    require_true(__spd_distance_ok<affine_invariant_metric>);
    require_true(__spd_distance_ok<log_euclidean_metric>);
  }
  end_test_case();

  test_case("torus rejects N = 1");
  {
    require_true(!__torus_ok<1>);
    require_true(__torus_ok<2>);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // distance must not cancel away a small separation that log_map resolves
  test_case("sphere::distance resolves separations below the acos cancellation floor");
  {
    const f64 ts[] = { 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-2, 0.5, 1.0, 2.0, 3.0 };
    for ( usize i = 0; i < 11; ++i ) {
      volatile f64 tv = ts[i];
      const f64 t = tv;
      f64 s, c;
      math::sincos<f64>(t, s, c);
      const vec<f64, 3> p{ 1.0, 0.0, 0.0 }, q{ c, s, 0.0 };
      const f64 d = sphere<f64, 3>::distance(p, q);
      require_true(rel_within(d, t, 1e-13));
      require_true(rel_within(d, sphere<f64, 3>::norm(p, sphere<f64, 3>::log_map(p, q)), 1e-12));
      require_true(sphere<f64, 3>::squared_distance(p, q) > 0.0);
    }
    // N != 3 shares the code path
    volatile f64 tv = 1e-9;
    f64 s, c;
    math::sincos<f64>((f64)tv, s, c);
    const vec<f64, 5> p5{ 1.0, 0.0, 0.0, 0.0, 0.0 }, q5{ c, s, 0.0, 0.0, 0.0 };
    require_true(rel_within(sphere<f64, 5>::distance(p5, q5), (f64)tv, 1e-13));
  }
  end_test_case();

  test_case("hyperbolic::distance resolves separations below the acosh cancellation floor");
  {
    const f64 ts[] = { 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-2, 0.5, 1.0, 1.3, 2.0, 5.0, 10.0 };
    for ( usize i = 0; i < 13; ++i ) {
      volatile f64 tv = ts[i];
      const f64 t = tv;
      const vec<f64, 3> p{ 1.0, 0.0, 0.0 }, q{ math::cosh<f64>(t), math::sinh<f64>(t), 0.0 };
      const f64 d = hyperbolic<f64, 2>::distance(p, q);
      require_true(rel_within(d, t, 1e-13));
      require_true(rel_within(d, hyperbolic<f64, 2>::norm(p, hyperbolic<f64, 2>::log_map(p, q)), 1e-12));
      require_true(hyperbolic<f64, 2>::squared_distance(p, q) > 0.0);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // grassmann::log_map must be the exact logarithm, whose norm is the distance
  test_case("grassmann::log_map recovers the tangent that exp_map consumed");
  {
    mat<f64, 4, 2> X{};
    X.data[0] = 1.0;
    X.data[3] = 1.0;
    const f64 ms[] = { 0.05, 0.2, 1.0, 1.5 };
    for ( usize i = 0; i < 4; ++i ) {
      mat<f64, 4, 2> V{};
      V.data[4] = ms[i];
      const auto Y = grassmann<f64, 4, 2>::exp_map(X, V);
      const auto L = grassmann<f64, 4, 2>::log_map(X, Y);
      require_true(max_abs_diff<4, 2>(L, V) <= 1e-12);
      require_true(rel_within(grassmann<f64, 4, 2>::norm(X, L), grassmann<f64, 4, 2>::distance(X, Y), 1e-11));
    }
  }
  end_test_case();

  test_case("grassmann<6,3>: |log_X(Y)| equals distance(X, Y) over a seeded sweep");
  {
    f64 worst = 0.0;
    for ( usize it = 0; it < 120; ++it ) {
      mat<f64, 6, 3> A{};
      for ( usize i = 0; i < 18; ++i ) A.data[i] = next_signed();
      const auto X = grassmann<f64, 6, 3>::project_to_manifold(A);
      mat<f64, 6, 3> B{};
      for ( usize i = 0; i < 18; ++i ) B.data[i] = next_signed();
      const auto V0 = grassmann<f64, 6, 3>::project_to_tangent(X, B);
      // keep the largest principal angle inside the injectivity radius (pi/2)
      const auto sv = linalg::decomp::svd<f64, 6, 3>(V0);
      if ( sv.S.data[0] < 1e-8 ) continue;
      const f64 target = 0.02 + 1.4 * (static_cast<f64>(it % 20) / 20.0);
      const f64 k = target / sv.S.data[0];
      mat<f64, 6, 3> V{};
      for ( usize i = 0; i < 18; ++i ) V.data[i] = k * V0.data[i];
      const auto Y = grassmann<f64, 6, 3>::exp_map(X, V);
      const auto L = grassmann<f64, 6, 3>::log_map(X, Y);
      const f64 nv = frob<6, 3>(V), nl = frob<6, 3>(L);
      f64 e = (nl - nv) / nv;
      if ( e < 0 ) e = -e;
      if ( e > worst ) worst = e;
    }
    print("worst | |log|_F / |V|_F - 1 | = ", worst);
    require_true(worst <= 1e-9);
  }
  end_test_case();

  test_case("grassmann::inverse_retract keeps the first-order projected difference");
  {
    mat<f64, 4, 2> X{};
    X.data[0] = 1.0;
    X.data[3] = 1.0;
    mat<f64, 4, 2> V{};
    V.data[4] = 1.0;
    const auto Y = grassmann<f64, 4, 2>::exp_map(X, V);
    const auto ir = grassmann<f64, 4, 2>::inverse_retract(X, Y);
    // |Pi_X(Y - X)| is sin(theta), which is exactly what the log used to answer
    require_true(rel_within(grassmann<f64, 4, 2>::norm(X, ir), math::sin<f64>(1.0), 1e-12));
  }
  end_test_case();

  print("=== ALL MANIFOLD DEFECT GATES PASSED ===");
  return 1;
}
