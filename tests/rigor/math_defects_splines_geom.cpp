// math_defects_splines_geom.cpp
// Regression suite for the wave-3 splines/geometry defects:
//   cubic_1d integral()      -- an interval entirely outside the knot range must integrate to 0
//   cubic_1d derivative()    -- must be the derivative of evaluate() under EVERY extrap mode
//   splines/bits/impl.hpp    -- binary_advance must build for a 16-byte float on amd64
//   geometry/umeyama         -- the linear part must be a scaled ROTATION for rank-deficient input
//   geometry/unit_orthogonal -- never the zero vector for a non-zero v, at any N
//   geometry/parametrized_line::distance -- stable, and correct for a non-unit direction

#include "../../src/math/geometry/ortho.hpp"
#include "../../src/math/geometry/parametrized_line.hpp"
#include "../../src/math/geometry/umeyama.hpp"
#include "../../src/math/splines/cubic_1d.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;
using namespace micron::math::splines;
namespace geo = micron::math::geometry;

static f64
abs64(f64 x) noexcept
{
  return x < 0 ? -x : x;
}

static bool
near(f64 a, f64 b, f64 eps = 1e-12) noexcept
{
  return abs64(a - b) < eps;
}

// the asm arm of binary_advance is gated on __OPTIMIZE__ && amd64 and used to feed a 16-byte
// float to ucomiss; instantiating the evaluator on long double is the whole assertion
static long double
long_double_spline_probe() noexcept
{
  long double xs[4] = { 0.0L, 1.0L, 2.0L, 3.0L };
  long double ys[4] = { 0.0L, 1.0L, 0.0L, 2.0L };
  auto s = make_cubic<long double>(raw_slice<const long double>(xs, 4), raw_slice<const long double>(ys, 4), bc_kind::natural);
  return evaluate<long double>(s, 1.5L) + derivative<long double>(s, 1.5L, 1) + integral<long double>(s, 0.0L, 3.0L);
}

static u64 rng_state = 0x5eed1234abcd9876ULL;

static f64
next_unit() noexcept
{
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 7;
  rng_state ^= rng_state << 17;
  return f64(rng_state >> 11) / f64(u64(1) << 53) * 2.0 - 1.0;
}

int
main()
{
  print("=== SPLINES / GEOMETRY DEFECT REGRESSIONS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("cubic integral(): an interval outside the knot range integrates to 0");
  {
    f64 xs[3] = { 0.0, 1.0, 2.0 };
    f64 ys[3] = { 0.0, 1.0, 0.0 };
    auto s = make_cubic<f64>(raw_slice<const f64>(xs, 3), raw_slice<const f64>(ys, 3), bc_kind::natural);

    require_true(near(integral<f64>(s, 0.0, 2.0), 1.25));

    require_true(near(integral<f64>(s, -3.0, -1.0), 0.0));
    require_true(near(integral<f64>(s, -5.0, -4.0), 0.0));
    require_true(near(integral<f64>(s, -1e6, -1.0), 0.0));
    require_true(near(integral<f64>(s, 3.0, 5.0), 0.0));
    require_true(near(integral<f64>(s, 5.0, 3.0), 0.0));
    require_true(near(integral<f64>(s, 10.0, 11.0), 0.0));
    require_true(near(integral<f64>(s, 2.0, 1e6), 0.0));

    // the partially-outside convention is unchanged: clamp to the domain
    require_true(near(integral<f64>(s, -1.0, 0.5), integral<f64>(s, 0.0, 0.5)));
    require_true(near(integral<f64>(s, 1.5, 9.0), integral<f64>(s, 1.5, 2.0)));
    require_true(near(integral<f64>(s, -7.0, 9.0), 1.25));
    // antisymmetry
    require_true(near(integral<f64>(s, 0.5, 0.0), -integral<f64>(s, 0.0, 0.5)));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("cubic derivative(): honours extrap mode outside the knot range");
  {
    f64 xs[3] = { 0.0, 1.0, 2.0 };
    f64 ys[3] = { 0.0, 1.0, 0.0 };

    // clamp_to_endpoints: evaluate() is constant outside, so every derivative order is 0
    auto clamped = make_cubic<f64>(raw_slice<const f64>(xs, 3), raw_slice<const f64>(ys, 3), bc_kind::clamped, 0.0, 0.0);
    clamped.mode = extrap::clamp_to_endpoints;
    require_true(near(derivative<f64>(clamped, -1.0, 1), 0.0));
    require_true(near(derivative<f64>(clamped, -5.0, 1), 0.0));
    require_true(near(derivative<f64>(clamped, -1.0, 2), 0.0));
    require_true(near(derivative<f64>(clamped, 3.0, 1), 0.0));
    require_true(near(derivative<f64>(clamped, 3.0, 2), 0.0));
    // order 0 is the value itself
    require_true(near(derivative<f64>(clamped, -1.0, 0), evaluate<f64>(clamped, -1.0)));
    require_true(near(derivative<f64>(clamped, 7.0, 0), evaluate<f64>(clamped, 7.0)));

    // error_value: evaluate() is identically 0 outside
    auto zeroed = make_cubic<f64>(raw_slice<const f64>(xs, 3), raw_slice<const f64>(ys, 3), bc_kind::clamped, 0.0, 0.0);
    zeroed.mode = extrap::error_value;
    require_true(near(derivative<f64>(zeroed, -1.0, 1), 0.0));
    require_true(near(derivative<f64>(zeroed, -1.0, 2), 0.0));
    require_true(near(derivative<f64>(zeroed, 3.0, 1), 0.0));
    require_true(near(derivative<f64>(zeroed, 3.0, 2), 0.0));

    // linear_continue: the extrapolant is a straight line -- order 1 is the endpoint slope,
    // order 2 and up are 0 even where the end segment has curvature
    auto linear = make_cubic<f64>(raw_slice<const f64>(xs, 3), raw_slice<const f64>(ys, 3), bc_kind::clamped, 0.0, 0.0);
    linear.mode = extrap::linear_continue;
    require_true(!near(linear.seg[0].data[2], 0.0));      // end segment really is curved
    require_true(near(derivative<f64>(linear, -1.0, 2), 0.0));
    require_true(near(derivative<f64>(linear, 3.0, 2), 0.0));
    require_true(near(derivative<f64>(linear, -1.0, 1), linear.seg[0].data[1]));

    // every mode: derivative() agrees with a central difference of evaluate() outside the domain
    const extrap modes[3] = { extrap::clamp_to_endpoints, extrap::linear_continue, extrap::error_value };
    for ( usize m = 0; m < 3; ++m ) {
      auto s = make_cubic<f64>(raw_slice<const f64>(xs, 3), raw_slice<const f64>(ys, 3), bc_kind::clamped, 0.7, -0.4);
      s.mode = modes[m];
      const f64 probes[4] = { -4.0, -1.5, 3.5, 9.0 };
      for ( usize i = 0; i < 4; ++i ) {
        const f64 x = probes[i];
        const f64 h = 1e-4;
        const f64 d1 = (evaluate<f64>(s, x + h) - evaluate<f64>(s, x - h)) / (2.0 * h);
        const f64 d2 = (evaluate<f64>(s, x + h) - 2.0 * evaluate<f64>(s, x) + evaluate<f64>(s, x - h)) / (h * h);
        require_true(near(derivative<f64>(s, x, 1), d1, 1e-7));
        require_true(near(derivative<f64>(s, x, 2), d2, 1e-6));
      }
    }

    // knots are untouched: a clamped spline still reports its prescribed end slopes there
    auto pinned = make_cubic<f64>(raw_slice<const f64>(xs, 3), raw_slice<const f64>(ys, 3), bc_kind::clamped, 0.7, -0.4);
    require_true(near(derivative<f64>(pinned, xs[0], 1), 0.7, 1e-12));
    require_true(near(derivative<f64>(pinned, xs[2], 1), -0.4, 1e-12));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("splines instantiate on a 16-byte float (binary_advance asm gate)");
  {
    const long double v = long_double_spline_probe();
    require_true(v == v);
    require_true(v > -1e30L && v < 1e30L);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("unit_orthogonal: never the zero vector for a non-zero v");
  {
    // exhaustive over the {-1,0,1} cube -- the axis-aligned cases are the ones that failed
    usize checked = 0;
    for ( usize n = 2; n <= 7; ++n ) {
      long lim = 1;
      for ( usize i = 0; i < n; ++i ) lim *= 3;
      for ( long code = 0; code < lim; ++code ) {
        f64 comp[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        long c = code;
        bool nonzero = false;
        for ( usize i = 0; i < n; ++i ) {
          const int d = int(c % 3) - 1;
          c /= 3;
          comp[i] = f64(d);
          if ( d != 0 ) nonzero = true;
        }
        if ( !nonzero ) continue;
        f64 nrm = 0, dot = 0;
        if ( n == 2 ) {
          math::vec<f64, 2> v{ comp[0], comp[1] };
          auto u = geo::unit_orthogonal<f64, 2>(v);
          for ( usize i = 0; i < 2; ++i ) {
            nrm += u.data[i] * u.data[i];
            dot += u.data[i] * v.data[i];
          }
        } else if ( n == 3 ) {
          math::vec<f64, 3> v{ comp[0], comp[1], comp[2] };
          auto u = geo::unit_orthogonal<f64, 3>(v);
          for ( usize i = 0; i < 3; ++i ) {
            nrm += u.data[i] * u.data[i];
            dot += u.data[i] * v.data[i];
          }
        } else if ( n == 4 ) {
          math::vec<f64, 4> v{ comp[0], comp[1], comp[2], comp[3] };
          auto u = geo::unit_orthogonal<f64, 4>(v);
          for ( usize i = 0; i < 4; ++i ) {
            nrm += u.data[i] * u.data[i];
            dot += u.data[i] * v.data[i];
          }
        } else if ( n == 5 ) {
          math::vec<f64, 5> v{ comp[0], comp[1], comp[2], comp[3], comp[4] };
          auto u = geo::unit_orthogonal<f64, 5>(v);
          for ( usize i = 0; i < 5; ++i ) {
            nrm += u.data[i] * u.data[i];
            dot += u.data[i] * v.data[i];
          }
        } else if ( n == 6 ) {
          math::vec<f64, 6> v{ comp[0], comp[1], comp[2], comp[3], comp[4], comp[5] };
          auto u = geo::unit_orthogonal<f64, 6>(v);
          for ( usize i = 0; i < 6; ++i ) {
            nrm += u.data[i] * u.data[i];
            dot += u.data[i] * v.data[i];
          }
        } else {
          math::vec<f64, 7> v{ comp[0], comp[1], comp[2], comp[3], comp[4], comp[5], comp[6] };
          auto u = geo::unit_orthogonal<f64, 7>(v);
          for ( usize i = 0; i < 7; ++i ) {
            nrm += u.data[i] * u.data[i];
            dot += u.data[i] * v.data[i];
          }
        }
        require_true(near(nrm, 1.0, 1e-12));
        require_true(near(dot, 0.0, 1e-12));
        ++checked;
      }
    }
    require_true(checked == 8 + 26 + 80 + 242 + 728 + 2186);

    // the two named axis cases
    auto e0 = geo::unit_orthogonal<f64, 4>(math::vec<f64, 4>{ 1, 0, 0, 0 });
    auto e3 = geo::unit_orthogonal<f64, 4>(math::vec<f64, 4>{ 0, 0, 0, 1 });
    require_true(near(e0.data[0] * e0.data[0] + e0.data[1] * e0.data[1] + e0.data[2] * e0.data[2] + e0.data[3] * e0.data[3], 1.0));
    require_true(near(e3.data[0] * e3.data[0] + e3.data[1] * e3.data[1] + e3.data[2] * e3.data[2] + e3.data[3] * e3.data[3], 1.0));

    // random f64 sweep, N = 4
    for ( usize trial = 0; trial < 4000; ++trial ) {
      math::vec<f64, 4> v{ next_unit(), next_unit(), next_unit(), next_unit() };
      // sparsify so zero coordinates are common
      for ( usize i = 0; i < 4; ++i )
        if ( v.data[i] > 0.4 ) v.data[i] = 0.0;
      f64 mag = 0;
      for ( usize i = 0; i < 4; ++i ) mag += v.data[i] * v.data[i];
      if ( mag == 0.0 ) continue;
      auto u = geo::unit_orthogonal<f64, 4>(v);
      f64 nrm = 0, dot = 0;
      for ( usize i = 0; i < 4; ++i ) {
        nrm += u.data[i] * u.data[i];
        dot += u.data[i] * v.data[i];
      }
      require_true(near(nrm, 1.0, 1e-12));
      require_true(near(dot, 0.0, 1e-12));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("umeyama: rank-deficient input still yields a scaled rotation");
  {
    // R = Rz(1.1) * Ry(-0.7) * Rx(0.3)
    const f64 R[9] = { 0.34692945, -0.93775824, -0.01579353, 0.68163299, 0.26366945, -0.68253563, 0.64421769, 0.22602632, 0.73068165 };
    const f64 t[3] = { 5.0, -2.0, 3.0 };

    // the minimal rigid-alignment case: 3 points, which always span a plane
    const f64 P[3][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };
    dynmat<f64> src(3, 3), dst(3, 3);
    for ( usize j = 0; j < 3; ++j )
      for ( usize i = 0; i < 3; ++i ) {
        src.at(i, j) = P[j][i];
        f64 v = t[i];
        for ( usize k = 0; k < 3; ++k ) v += R[i * 3 + k] * P[j][k];
        dst.at(i, j) = v;
      }
    auto T = geo::umeyama<f64>(src, dst, true);

    // the recovered linear part IS the rotation, so a point off the fitted plane maps correctly
    for ( usize i = 0; i < 3; ++i ) {
      for ( usize j = 0; j < 3; ++j ) require_true(near(T.M.data[i * 4 + j], R[i * 3 + j], 1e-7));
      require_true(near(T.M.data[i * 4 + 3], t[i], 1e-7));
    }
    const f64 det = T.M.data[0] * (T.M.data[5] * T.M.data[10] - T.M.data[6] * T.M.data[9])
                    - T.M.data[1] * (T.M.data[4] * T.M.data[10] - T.M.data[6] * T.M.data[8])
                    + T.M.data[2] * (T.M.data[4] * T.M.data[9] - T.M.data[5] * T.M.data[8]);
    require_true(near(det, 1.0, 1e-6));

    // seeded fuzz: coplanar, collinear and N <= 3 sets must all come back as similarities
    for ( usize trial = 0; trial < 300; ++trial ) {
      const usize kind = trial % 3;      // 0 coplanar, 1 collinear, 2 N == 3 / N == 2
      const usize N = (kind == 2) ? (2 + (trial % 2)) : (3 + (trial % 5));
      f64 a[3] = { next_unit(), next_unit(), next_unit() };
      f64 b[3] = { next_unit(), next_unit(), next_unit() };
      dynmat<f64> s2(3, N), d2(3, N);
      for ( usize j = 0; j < N; ++j ) {
        f64 p[3];
        if ( kind == 0 ) {
          const f64 w0 = next_unit() * 3.0, w1 = next_unit() * 3.0;
          for ( usize i = 0; i < 3; ++i ) p[i] = w0 * a[i] + w1 * b[i];
        } else if ( kind == 1 ) {
          const f64 w0 = next_unit() * 3.0;
          for ( usize i = 0; i < 3; ++i ) p[i] = w0 * a[i];
        } else {
          for ( usize i = 0; i < 3; ++i ) p[i] = next_unit() * 3.0;
        }
        for ( usize i = 0; i < 3; ++i ) s2.at(i, j) = p[i];
        for ( usize i = 0; i < 3; ++i ) {
          f64 v = t[i];
          for ( usize k = 0; k < 3; ++k ) v += R[i * 3 + k] * p[k];
          d2.at(i, j) = v;
        }
      }
      auto T2 = geo::umeyama<f64>(s2, d2, true);
      // L * L^T == c^2 * I  <=>  L is a scaled orthogonal matrix
      f64 g[9];
      for ( usize i = 0; i < 3; ++i )
        for ( usize j = 0; j < 3; ++j ) {
          f64 acc = 0;
          for ( usize k = 0; k < 3; ++k ) acc += T2.M.data[i * 4 + k] * T2.M.data[j * 4 + k];
          g[i * 3 + j] = acc;
        }
      const f64 c2 = (g[0] + g[4] + g[8]) / 3.0;
      require_true(c2 > 1e-6);
      for ( usize i = 0; i < 3; ++i )
        for ( usize j = 0; j < 3; ++j ) require_true(near(g[i * 3 + j], i == j ? c2 : 0.0, 1e-9 * c2 + 1e-12));
      // and it still reproduces the pairs it was fitted to
      for ( usize j = 0; j < N; ++j )
        for ( usize i = 0; i < 3; ++i ) {
          f64 v = T2.M.data[i * 4 + 3];
          for ( usize k = 0; k < 3; ++k ) v += T2.M.data[i * 4 + k] * s2.at(k, j);
          require_true(near(v, d2.at(i, j), 1e-6));
        }
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("parametrized_line::distance is stable and unit-direction agnostic");
  {
    geo::parametrized_line<f32, 3> lf{ math::vec<f32, 3>{ 0, 0, 0 }, math::vec<f32, 3>{ 1, 0, 0 } };
    const f32 far32[4] = { 1e4f, 1e5f, 1e6f, 1e7f };
    for ( usize i = 0; i < 4; ++i ) require_true(abs64(f64(lf.distance(math::vec<f32, 3>{ far32[i], 1.0f, 0.0f })) - 1.0) < 1e-3);

    geo::parametrized_line<f64, 3> ld{ math::vec<f64, 3>{ 0, 0, 0 }, math::vec<f64, 3>{ 1, 0, 0 } };
    const f64 far64[4] = { 1e8, 1e9, 1e12, 1e15 };
    for ( usize i = 0; i < 4; ++i ) require_true(near(ld.distance(math::vec<f64, 3>{ far64[i], 1.0, 0.0 }), 1.0, 1e-6));

    // a non-unit direction is not a precondition anywhere in the type
    geo::parametrized_line<f64, 3> scaled{ math::vec<f64, 3>{ 0, 0, 0 }, math::vec<f64, 3>{ 2, 0, 0 } };
    require_true(near(scaled.distance(math::vec<f64, 3>{ 1.0, 1.0, 0.0 }), 1.0, 1e-12));
    require_true(near(scaled.distance(math::vec<f64, 3>{ 7.0, 3.0, 4.0 }), 5.0, 1e-12));

    // a zero direction degenerates to |p - origin|, as before
    geo::parametrized_line<f64, 3> degenerate{ math::vec<f64, 3>{ 0, 0, 0 }, math::vec<f64, 3>{ 0, 0, 0 } };
    require_true(near(degenerate.distance(math::vec<f64, 3>{ 3.0, 4.0, 0.0 }), 5.0, 1e-12));

    // seeded fuzz against the explicit projection oracle, with the offset small relative to the
    // along-line coordinate -- exactly the regime the Pythagorean form loses
    for ( usize trial = 0; trial < 5000; ++trial ) {
      math::vec<f64, 3> o{ next_unit() * 4.0, next_unit() * 4.0, next_unit() * 4.0 };
      math::vec<f64, 3> d{ next_unit(), next_unit(), next_unit() };
      f64 dn = 0;
      for ( usize i = 0; i < 3; ++i ) dn += d.data[i] * d.data[i];
      if ( dn < 1e-6 ) continue;
      dn = math::fsqrt(dn);
      for ( usize i = 0; i < 3; ++i ) d.data[i] /= dn;
      // an exactly-known perpendicular: any unit vector orthogonal to d
      auto perp = geo::unit_orthogonal<f64, 3>(d);
      const f64 offset = 1.0 + abs64(next_unit());
      const f64 along = next_unit() * 1e7;
      math::vec<f64, 3> p{};
      for ( usize i = 0; i < 3; ++i ) p.data[i] = o.data[i] + along * d.data[i] + offset * perp.data[i];
      const f64 scale = 1.0 + abs64(next_unit()) * 3.0;      // non-unit direction
      math::vec<f64, 3> ds{ d.data[0] * scale, d.data[1] * scale, d.data[2] * scale };
      geo::parametrized_line<f64, 3> line{ o, ds };
      require_true(near(line.distance(p), offset, 1e-6 * offset + 1e-7));
    }
  }
  end_test_case();

  print("=== SPLINES / GEOMETRY DEFECT REGRESSIONS PASSED ===");
  return 1;
}
