// geom_basic.cpp
// Rigorous snowball test suite for geometry primitives.

#include "../../src/math/geometry.hpp"
#include "../../src/math/sqrt.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;
namespace mg = micron::math::geometry;

template <typename F>
static bool
approx(F a, F b, F tol) noexcept
{
  F d = a - b;
  if ( d < F(0) ) d = -d;
  return d <= tol;
}

template <typename F, usize Dim>
static bool
vec_close(const m::vec<F, Dim> &a, const m::vec<F, Dim> &b, F tol) noexcept
{
  for ( usize i = 0; i < Dim; ++i )
    if ( !approx(a.data[i], b.data[i], tol) ) return false;
  return true;
}

int
main()
{
  sb::print("=== GEOMETRY BASIC TESTS ===");

  // ------------------------------------------------------------------
  test_case("translation: apply, inverse, compose");
  {
    using F = double;
    mg::translation<F, 3> t1{ { 1.0, 2.0, 3.0 } };
    mg::translation<F, 3> t2{ { -0.5, 1.5, -2.0 } };

    m::vec<F, 3> p{ 5.0, -1.0, 2.0 };
    auto p1 = t1.apply(p);
    require_true(approx(p1.data[0], 6.0, 1e-15));
    require_true(approx(p1.data[1], 1.0, 1e-15));
    require_true(approx(p1.data[2], 5.0, 1e-15));

    auto inv = t1.inverse();
    auto rt = inv.apply(p1);
    require_true(vec_close(rt, p, F(1e-15)));

    auto comp = t1 * t2;
    require_true(approx(comp.t.data[0], 0.5, 1e-15));
    require_true(approx(comp.t.data[1], 3.5, 1e-15));
    require_true(approx(comp.t.data[2], 1.0, 1e-15));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("scaling: uniform, anisotropic, inverse");
  {
    using F = double;
    auto u = mg::scaling<F, 3>::uniform(2.0);
    m::vec<F, 3> p{ 1.0, 2.0, 3.0 };
    auto q = u.apply(p);
    require_true(vec_close(q, m::vec<F, 3>{ 2.0, 4.0, 6.0 }, F(1e-15)));

    mg::scaling<F, 3> aniso{ { 1.0, 2.0, 4.0 } };
    auto q2 = aniso.apply(p);
    require_true(vec_close(q2, m::vec<F, 3>{ 1.0, 4.0, 12.0 }, F(1e-15)));

    auto inv = aniso.inverse();
    auto back = inv.apply(q2);
    require_true(vec_close(back, p, F(1e-15)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("aligned_box: contains, intersects, extend, corner");
  {
    using F = double;
    mg::aligned_box<F, 3> box{ { 0.0, 0.0, 0.0 }, { 10.0, 10.0, 10.0 } };
    require_true(!box.is_empty());
    require_true(box.contains(m::vec<F, 3>{ 5.0, 5.0, 5.0 }));
    require_true(!box.contains(m::vec<F, 3>{ 11.0, 5.0, 5.0 }));

    mg::aligned_box<F, 3> b2{ { 5.0, 5.0, 5.0 }, { 15.0, 15.0, 15.0 } };
    require_true(box.intersects(b2));

    mg::aligned_box<F, 3> b3{ { 20.0, 20.0, 20.0 }, { 30.0, 30.0, 30.0 } };
    require_true(!box.intersects(b3));

    auto e = mg::aligned_box<F, 3>::empty();
    require_true(e.is_empty());
    e.extend(m::vec<F, 3>{ 1.0, 2.0, 3.0 });
    e.extend(m::vec<F, 3>{ -1.0, 4.0, 0.0 });
    require_true(approx(e.min_corner.data[0], -1.0, 1e-15));
    require_true(approx(e.max_corner.data[1], 4.0, 1e-15));

    // 8 corners of a unit cube
    mg::aligned_box<F, 3> unit{ { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 } };
    auto c5 = unit.corner(5);     // bits 101 -> (max, min, max)
    require_true(approx(c5.data[0], 1.0, 1e-15));
    require_true(approx(c5.data[1], 0.0, 1e-15));
    require_true(approx(c5.data[2], 1.0, 1e-15));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("hyperplane: signed distance, projection");
  {
    using F = double;
    // plane: y = 0 (normal = +y, offset = 0)
    auto h = mg::hyperplane<F, 3>::through(m::vec<F, 3>{ 0.0, 0.0, 0.0 }, m::vec<F, 3>{ 0.0, 1.0, 0.0 });
    F d = h.signed_distance(m::vec<F, 3>{ 1.0, 3.0, 2.0 });
    require_true(approx(d, 3.0, 1e-15));
    auto proj = h.projection(m::vec<F, 3>{ 1.0, 3.0, 2.0 });
    require_true(approx(proj.data[1], 0.0, 1e-15));

    // plane through three 3D points
    auto h2 = mg::hyperplane<F, 3>::through3(m::vec<F, 3>{ 1, 0, 0 }, m::vec<F, 3>{ 0, 1, 0 }, m::vec<F, 3>{ 0, 0, 1 });
    // points should lie on the plane
    require_true(approx(h2.signed_distance(m::vec<F, 3>{ 1, 0, 0 }), 0.0, 1e-14));
    require_true(approx(h2.signed_distance(m::vec<F, 3>{ 0, 1, 0 }), 0.0, 1e-14));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("parametrized_line: distance, intersection");
  {
    using F = double;
    mg::parametrized_line<F, 3> line{ { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 } };
    F d = line.distance(m::vec<F, 3>{ 5.0, 3.0, 4.0 });
    require_true(approx(d, 5.0, 1e-14));     // sqrt(3² + 4²) = 5

    // intersect line (going along +x) with plane y = 0 + ...? not interesting. use plane x = 7.
    auto h = mg::hyperplane<F, 3>::through(m::vec<F, 3>{ 7.0, 0.0, 0.0 }, m::vec<F, 3>{ 1.0, 0.0, 0.0 });
    F t = line.intersection_param(h);
    require_true(approx(t, 7.0, 1e-14));
    auto p = line.intersection_point(h);
    require_true(approx(p.data[0], 7.0, 1e-14));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("unit_orthogonal: dot product is zero");
  {
    using F = double;
    m::vec<F, 3> v{ 1.0, 2.0, 3.0 };
    auto u = mg::unit_orthogonal(v);
    F dot = u.data[0] * v.data[0] + u.data[1] * v.data[1] + u.data[2] * v.data[2];
    require_true(approx(dot, 0.0, 1e-14));
    F nrm = micron::math::fsqrt(u.data[0] * u.data[0] + u.data[1] * u.data[1] + u.data[2] * u.data[2]);
    require_true(approx(nrm, 1.0, 1e-14));
  }
  end_test_case();

  sb::print("=== GEOMETRY BASIC PASSED ===");
  return 0;
}
