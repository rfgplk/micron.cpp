// quat_from_two_vectors.cpp — Snowball tests for the shortest-arc constructors
// quaternions::from_unit_vectors / from_two_vectors.

#include "../../src/math/quaternions/quaternions.hpp"
#include "../../src/std.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

using v3 = micron::vector_3<f64>;

static bool
near(f64 a, f64 b, f64 eps = 1e-12)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

static v3
normd(v3 v)
{
  f64 inv = micron::math::frsqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return { v.x * inv, v.y * inv, v.z * inv };
}

// for unit a, b: from_unit_vectors must rotate a exactly onto b and stay unit.
static bool
unit_rotates_to(const v3 &a, const v3 &b, f64 eps = 1e-12)
{
  auto q = quaternions::from_unit_vectors(a, b);
  auto r = quaternions::rotate(q, a);
  return near(r.x, b.x, eps) && near(r.y, b.y, eps) && near(r.z, b.z, eps) && near(q.squared_norm(), 1.0, 1e-12);
}

int
main()
{
  print("=== QUAT FROM TWO VECTORS ===");

  test_case("from_unit_vectors rotates a -> b for assorted unit pairs");
  {
    require_true(unit_rotates_to(normd({ 1, 2, 3 }), normd({ -2, 1, 0.5 })));
    require_true(unit_rotates_to(normd({ 1, 0, 0 }), normd({ 0, 1, 0 })));
    require_true(unit_rotates_to(normd({ 0, 0, 1 }), normd({ 1, 1, 1 })));
    require_true(unit_rotates_to(normd({ -3, 0.2, 4 }), normd({ 0.1, -5, 2 })));
  }
  end_test_case();

  test_case("from_unit_vectors x -> y is a +90 deg rotation about z");
  {
    v3 x{ 1, 0, 0 }, y{ 0, 1, 0 };
    auto q = quaternions::from_unit_vectors(x, y);
    auto ref = quaternions::from_axis_angle<f64>(0, 0, 1, 1.5707963267948966);
    // q and ref describe the same rotation (allow global sign flip)
    f64 d = q.dot(ref);
    if ( d < 0 ) {
      q.x = -q.x;
      q.y = -q.y;
      q.z = -q.z;
      q.w = -q.w;
    }
    require_true(near(q.x, ref.x, 1e-12) && near(q.y, ref.y, 1e-12) && near(q.z, ref.z, 1e-12) && near(q.w, ref.w, 1e-12));
  }
  end_test_case();

  test_case("from_unit_vectors parallel inputs -> identity rotation");
  {
    v3 a = normd({ 0.3, -0.7, 0.5 });
    auto q = quaternions::from_unit_vectors(a, a);
    require_true(near(q.w, 1.0, 1e-12) && near(q.x, 0.0, 1e-12) && near(q.y, 0.0, 1e-12) && near(q.z, 0.0, 1e-12));
    auto r = quaternions::rotate(q, a);
    require_true(near(r.x, a.x) && near(r.y, a.y) && near(r.z, a.z));
  }
  end_test_case();

  test_case("from_unit_vectors anti-parallel inputs rotate a -> -a, q unit, axis _|_ a");
  {
    v3 a = normd({ 1, 2, -2 });
    v3 b{ -a.x, -a.y, -a.z };
    auto q = quaternions::from_unit_vectors(a, b);
    require_true(near(q.squared_norm(), 1.0, 1e-12));
    // pi rotation has zero scalar part and an axis orthogonal to a
    require_true(near(q.w, 0.0, 1e-12));
    require_true(near(q.x * a.x + q.y * a.y + q.z * a.z, 0.0, 1e-12));
    auto r = quaternions::rotate(q, a);
    require_true(near(r.x, b.x, 1e-12) && near(r.y, b.y, 1e-12) && near(r.z, b.z, 1e-12));
  }
  end_test_case();

  test_case("from_two_vectors aligns directions for non-unit inputs and stays unit");
  {
    v3 a{ 0, 0, 5 };
    v3 b{ 3, 0, 0 };
    auto q = quaternions::from_two_vectors(a, b);
    require_true(near(q.squared_norm(), 1.0, 1e-12));
    auto r = quaternions::rotate(q, normd(a));
    auto bn = normd(b);
    require_true(near(r.x, bn.x, 1e-12) && near(r.y, bn.y, 1e-12) && near(r.z, bn.z, 1e-12));
  }
  end_test_case();

  test_case("from_two_vectors anti-parallel non-unit inputs rotate a-hat -> -a-hat");
  {
    v3 a{ 0, 2, 0 };
    v3 b{ 0, -3, 0 };
    auto q = quaternions::from_two_vectors(a, b);
    require_true(near(q.squared_norm(), 1.0, 1e-12));
    auto r = quaternions::rotate(q, normd(a));
    require_true(near(r.x, 0.0, 1e-12) && near(r.y, -1.0, 1e-12) && near(r.z, 0.0, 1e-12));
  }
  end_test_case();

  test_case("from_two_vectors zero-length input yields finite identity (no NaN)");
  {
    auto q = quaternions::from_two_vectors(v3{ 0, 0, 0 }, v3{ 1, 0, 0 });
    require_true(q.all_finite());
    require_true(near(q.w, 1.0, 1e-12) && near(q.x, 0.0) && near(q.y, 0.0) && near(q.z, 0.0));
  }
  end_test_case();

  print("=== QUAT FROM TWO VECTORS PASSED ===");
  return 1;
}
