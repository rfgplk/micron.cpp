// vec8_tests.cpp
// Rigorous snowball test suite for micron::vector_8<T> (vec8, dvec8)

#include "../../src/math/quants/vecs.hpp"

#include "../snowball/snowball.hpp"

#include <cmath>

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ------------------------------------------------------------------ //
//  Tolerance helpers                                                  //
// ------------------------------------------------------------------ //
namespace
{

constexpr float EPS32 = 1e-5f;
constexpr double EPS64 = 1e-10;

bool
feq(float a, float b, float e = EPS32)
{
  float d = a - b;
  return (d < 0 ? -d : d) <= e;
}

bool
feq(double a, double b, double e = EPS64)
{
  double d = a - b;
  return (d < 0 ? -d : d) <= e;
}

bool
veq(const micron::vec8 &a, const micron::vec8 &b, float e = EPS32)
{
  return feq(a.x, b.x, e) && feq(a.y, b.y, e) && feq(a.z, b.z, e) && feq(a.w, b.w, e) && feq(a.a, b.a, e) && feq(a.b, b.b, e)
         && feq(a.c, b.c, e) && feq(a.d, b.d, e);
}

bool
veq(const micron::dvec8 &a, const micron::dvec8 &b, double e = EPS64)
{
  return feq(a.x, b.x, e) && feq(a.y, b.y, e) && feq(a.z, b.z, e) && feq(a.w, b.w, e) && feq(a.a, b.a, e) && feq(a.b, b.b, e)
         && feq(a.c, b.c, e) && feq(a.d, b.d, e);
}

bool
vall_eq(const micron::vec8 &v, float s, float e = EPS32)
{
  return feq(v.x, s, e) && feq(v.y, s, e) && feq(v.z, s, e) && feq(v.w, s, e) && feq(v.a, s, e) && feq(v.b, s, e) && feq(v.c, s, e)
         && feq(v.d, s, e);
}

micron::vec8
V(float a, float b, float c, float d, float e, float f, float g, float h)
{
  return micron::vec8(a, b, c, d, e, f, g, h);
}

micron::dvec8
DV(double a, double b, double c, double d, double e, double f, double g, double h)
{
  return micron::dvec8(a, b, c, d, e, f, g, h);
}

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== VEC8 TESTS ===");

  // ---------------------------------------------------------------- //
  test_case("default construction – all components zero");
  {
    micron::vec8 v;
    require_true(vall_eq(v, 0.0f));

    micron::dvec8 dv;
    require_true(feq(dv.x, 0.0) && feq(dv.d, 0.0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("component constructor preserves all eight values");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(feq(v.x, 1) && feq(v.y, 2) && feq(v.z, 3) && feq(v.w, 4));
    require_true(feq(v.a, 5) && feq(v.b, 6) && feq(v.c, 7) && feq(v.d, 8));

    // s0..s7 accessors mirror the named components
    require_true(feq(v.s0(), 1) && feq(v.s1(), 2) && feq(v.s2(), 3) && feq(v.s3(), 4));
    require_true(feq(v.s4(), 5) && feq(v.s5(), 6) && feq(v.s6(), 7) && feq(v.s7(), 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list constructor preserves order");
  {
    micron::vec8 v{ 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f };
    require_true(feq(v.x, 1) && feq(v.y, 2) && feq(v.z, 3) && feq(v.w, 4));
    require_true(feq(v.a, 5) && feq(v.b, 6) && feq(v.c, 7) && feq(v.d, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list wrong size throws");
  {
    require_throw([]() { micron::vec8 v{ 1.f, 2.f, 3.f }; });
    require_throw([]() { micron::vec8 v{ 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f }; });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy and move construction");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(a);
    require_true(a == b);

    micron::vec8 c(micron::move(a));
    require_true(b == c);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy and move assignment");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b;
    b = a;
    require_true(a == b);

    micron::vec8 c;
    c = micron::move(b);
    require_true(a == c);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator== and operator!=");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 c(1, 2, 3, 4, 5, 6, 7, 9);
    require_true(a == b);
    require_false(a == c);
    require_true(a != c);
    require_false(a != b);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("almost_equal with epsilon");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(1.000001f, 2.000001f, 3.000001f, 4.000001f, 5.000001f, 6.000001f, 7.000001f, 8.000001f);
    require_true(a.almost_equal(b, 1e-4f));
    require_false(a.almost_equal(b, 1e-8f));
  }
  end_test_case();

  // ================================================================ //
  //  Arithmetic                                                       //
  // ================================================================ //
  test_case("element-wise operator+ / operator-");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(8, 7, 6, 5, 4, 3, 2, 1);
    micron::vec8 s = a + b;
    require_true(vall_eq(s, 9.0f));

    micron::vec8 d = a - b;
    require_true(feq(d.x, -7) && feq(d.y, -5) && feq(d.z, -3) && feq(d.w, -1));
    require_true(feq(d.a, 1) && feq(d.b, 3) && feq(d.c, 5) && feq(d.d, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise operator* (Hadamard product)");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(2, 2, 2, 2, 2, 2, 2, 2);
    micron::vec8 r = a * b;
    require_true(feq(r.x, 2) && feq(r.y, 4) && feq(r.z, 6) && feq(r.w, 8));
    require_true(feq(r.a, 10) && feq(r.b, 12) && feq(r.c, 14) && feq(r.d, 16));

    // product() is an alias
    require_true(veq(a.product(b), r));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar operator* and operator/ (both orderings)");
  {
    micron::vec8 a(2, 4, 6, 8, 10, 12, 14, 16);
    micron::vec8 r1 = a * 2.0f;
    micron::vec8 r2 = 2.0f * a;
    require_true(veq(r1, r2));
    require_true(feq(r1.x, 4) && feq(r1.d, 32));

    micron::vec8 d1 = a / 2.0f;
    require_true(feq(d1.x, 1) && feq(d1.d, 8));

    // s / v  (reciprocal broadcast)
    micron::vec8 inv = 1.0f / V(1, 2, 4, 8, 16, 32, 64, 128);
    require_true(feq(inv.x, 1.0f) && feq(inv.y, 0.5f) && feq(inv.z, 0.25f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar operator+ and operator- (both orderings)");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 r1 = a + 10.0f;
    require_true(feq(r1.x, 11) && feq(r1.d, 18));

    micron::vec8 r2 = 10.0f + a;
    require_true(veq(r1, r2));

    micron::vec8 r3 = a - 1.0f;
    require_true(feq(r3.x, 0) && feq(r3.d, 7));

    micron::vec8 r4 = 10.0f - a;
    require_true(feq(r4.x, 9) && feq(r4.d, 2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("unary negation");
  {
    micron::vec8 a(1, -2, 3, -4, 5, -6, 7, -8);
    micron::vec8 n = -a;
    require_true(feq(n.x, -1) && feq(n.y, 2) && feq(n.z, -3) && feq(n.w, 4));
    require_true(feq(n.a, -5) && feq(n.b, 6) && feq(n.c, -7) && feq(n.d, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator+= and operator-=");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(1, 1, 1, 1, 1, 1, 1, 1);
    a += b;
    require_true(feq(a.x, 2) && feq(a.d, 9));
    a -= b;
    require_true(feq(a.x, 1) && feq(a.d, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator*= scalar");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    a *= 3.0f;
    require_true(feq(a.x, 3) && feq(a.y, 6) && feq(a.d, 24));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator/= scalar");
  {
    micron::vec8 a(2, 4, 6, 8, 10, 12, 14, 16);
    a /= 2.0f;
    require_true(feq(a.x, 1) && feq(a.y, 2) && feq(a.d, 8));
  }
  end_test_case();

  // ================================================================ //
  //  Geometric operations                                            //
  // ================================================================ //
  test_case("dot product – orthogonal, parallel, general");
  {
    // parallel unit-ish
    micron::vec8 a(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 b(1, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(a.dot(b), 1.0f));

    // opposite
    micron::vec8 c(-1, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(a.dot(c), -1.0f));

    // orthogonal
    micron::vec8 ox(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 oy(0, 1, 0, 0, 0, 0, 0, 0);
    require_true(feq(ox.dot(oy), 0.0f));

    // general: (1,2,3,4,5,6,7,8)·(1,2,3,4,5,6,7,8) = 1+4+9+16+25+36+49+64 = 204
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(feq(v.dot(v), 204.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("squared_norm and magnitude");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(feq(v.squared_norm(), 204.0f));
    require_true(feq(v.magnitude(), std::sqrt(204.0f), 1e-4f));

    // axis-aligned unit vector
    micron::vec8 ux(1, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(ux.magnitude(), 1.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("normalized – result has unit magnitude");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 n = v.normalized();
    require_true(feq(n.magnitude(), 1.0f, 1e-5f));
    require_true(n.is_normalized(1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("normalize() in-place – modifies and returns unit vector");
  {
    micron::vec8 v(3, 1, 4, 1, 5, 9, 2, 6);
    v.normalize();
    require_true(feq(v.magnitude(), 1.0f, 1e-5f));
    require_true(v.is_normalized(1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("is_normalized – true only for unit vectors");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_false(v.is_normalized());

    micron::vec8 u = v.normalized();
    require_true(u.is_normalized(1e-4f));

    micron::vec8 ax(1, 0, 0, 0, 0, 0, 0, 0);
    require_true(ax.is_normalized());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("distance and squared_distance");
  {
    micron::vec8 a(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 b(4, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(a.distance(b), 3.0f));
    require_true(feq(a.squared_distance(b), 9.0f));

    // symmetry
    require_true(feq(a.distance(b), b.distance(a)));

    // zero distance to self
    require_true(feq(a.distance(a), 0.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("l1_norm – sum of absolute values");
  {
    micron::vec8 v(1, -2, 3, -4, 5, -6, 7, -8);
    require_true(feq(v.l1_norm(), 36.0f));

    micron::vec8 z;
    require_true(feq(z.l1_norm(), 0.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("linf_norm – maximum absolute component");
  {
    micron::vec8 v(1, -2, 3, -4, 5, -6, 7, -8);
    require_true(feq(v.linf_norm(), 8.0f));

    micron::vec8 w(10, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(w.linf_norm(), 10.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lp_norm – p=1 matches l1, p=2 matches magnitude");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(feq(v.lp_norm(1.0f), v.l1_norm(), 1e-4f));
    require_true(feq(v.lp_norm(2.0f), v.magnitude(), 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cos_angle – known angles");
  {
    micron::vec8 a(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 b(1, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(a.cos_angle(b), 1.0f, 1e-5f));     // 0°

    micron::vec8 c(-1, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(a.cos_angle(c), -1.0f, 1e-5f));     // 180°

    micron::vec8 d(0, 1, 0, 0, 0, 0, 0, 0);
    require_true(feq(a.cos_angle(d), 0.0f, 1e-5f));     // 90°
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("angle – returns value in [0, pi]");
  {
    micron::vec8 a(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 b(0, 1, 0, 0, 0, 0, 0, 0);
    float ang = a.angle(b);
    require_true(feq(ang, (float)(M_PI / 2.0), 1e-4f));

    micron::vec8 c(-1, 0, 0, 0, 0, 0, 0, 0);
    require_true(feq(a.angle(c), (float)M_PI, 1e-4f));

    // self-angle is 0
    require_true(feq(a.angle(a), 0.0f, 1e-5f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("project_onto – onto axis gives correct scalar");
  {
    micron::vec8 v(3, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 axis(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 proj = v.project_onto(axis);
    require_true(veq(proj, v, 1e-5f));     // projects fully onto x axis

    micron::vec8 w(3, 4, 0, 0, 0, 0, 0, 0);
    micron::vec8 px = w.project_onto(axis);
    require_true(feq(px.x, 3.0f) && feq(px.y, 0.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("reject_from – orthogonal to axis after rejection");
  {
    micron::vec8 v(3, 4, 0, 0, 0, 0, 0, 0);
    micron::vec8 axis(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 rej = v.reject_from(axis);

    // rejection from x-axis should have x=0
    require_true(feq(rej.x, 0.0f, 1e-5f));
    require_true(feq(rej.y, 4.0f, 1e-5f));

    // projection + rejection reconstructs original
    micron::vec8 proj = v.project_onto(axis);
    require_true(veq(proj + rej, v, 1e-5f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar_project – projection scalar onto axis");
  {
    micron::vec8 v(6, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 axis(2, 0, 0, 0, 0, 0, 0, 0);
    // scalar projection = dot(v,axis)/|axis| = 12/2 = 6
    require_true(feq(v.scalar_project(axis), 6.0f, 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("reflect – reflection across a normal");
  {
    // vector pointing right, normal pointing up in x-y
    micron::vec8 v(1, -1, 0, 0, 0, 0, 0, 0);
    micron::vec8 n(0, 1, 0, 0, 0, 0, 0, 0);
    micron::vec8 r = v.reflect(n);
    require_true(feq(r.x, 1.0f, 1e-5f));
    require_true(feq(r.y, 1.0f, 1e-5f));

    // magnitude preserved
    require_true(feq(r.magnitude(), v.magnitude(), 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("refract – total internal reflection returns zero vector");
  {
    micron::vec8 v(1, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 n(0, 1, 0, 0, 0, 0, 0, 0);
    // eta >> 1 guarantees k < 0 (total internal reflection)
    micron::vec8 r = v.refract(n, 100.0f);
    require_true(vall_eq(r, 0.0f));
  }
  end_test_case();

  // ================================================================ //
  //  Reduction operations                                            //
  // ================================================================ //
  test_case("sum – sum of all components");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(feq(v.sum(), 36.0f));

    micron::vec8 z;
    require_true(feq(z.sum(), 0.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mean – arithmetic mean of all components");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(feq(v.mean(), 4.5f));

    micron::vec8 c(3, 3, 3, 3, 3, 3, 3, 3);
    require_true(feq(c.mean(), 3.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("prod – product of all components");
  {
    micron::vec8 v(1, 2, 1, 2, 1, 2, 1, 2);
    require_true(feq(v.prod(), 16.0f));     // 2^4

    micron::vec8 ones(1, 1, 1, 1, 1, 1, 1, 1);
    require_true(feq(ones.prod(), 1.0f));

    micron::vec8 z;
    require_true(feq(z.prod(), 0.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("min_component and max_component");
  {
    micron::vec8 v(3, 1, 4, 1, 5, 9, 2, 6);
    require_true(feq(v.min_component(), 1.0f));
    require_true(feq(v.max_component(), 9.0f));

    // all equal
    micron::vec8 c(7, 7, 7, 7, 7, 7, 7, 7);
    require_true(feq(c.min_component(), 7.0f));
    require_true(feq(c.max_component(), 7.0f));

    // negative values
    micron::vec8 n(-5, -3, -1, -2, -4, -6, -7, -8);
    require_true(feq(n.min_component(), -8.0f));
    require_true(feq(n.max_component(), -1.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("argmin and argmax – correct index returned");
  {
    micron::vec8 v(3, 1, 4, 1, 5, 9, 2, 6);
    // min=1 at index 1 (first occurrence wins)
    require(v.argmin(), 1);
    // max=9 at index 5
    require(v.argmax(), 5);

    // single dominant value at each end
    micron::vec8 lo(-99, 0, 0, 0, 0, 0, 0, 0);
    require(lo.argmin(), 0);

    micron::vec8 hi(0, 0, 0, 0, 0, 0, 0, 99.0f);
    require(hi.argmax(), 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("all() and any()");
  {
    micron::vec8 all_nz(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(all_nz.all());
    require_true(all_nz.any());

    micron::vec8 one_zero(1, 2, 3, 0, 5, 6, 7, 8);
    require_false(one_zero.all());
    require_true(one_zero.any());

    micron::vec8 all_zero;
    require_false(all_zero.all());
    require_false(all_zero.any());
  }
  end_test_case();

  // ================================================================ //
  //  Per-element unary math                                          //
  // ================================================================ //
  test_case("abs() – absolute value of each component");
  {
    micron::vec8 v(1, -2, 3, -4, 5, -6, 7, -8);
    micron::vec8 a = v.abs();
    require_true(feq(a.x, 1) && feq(a.y, 2) && feq(a.z, 3) && feq(a.w, 4));
    require_true(feq(a.a, 5) && feq(a.b, 6) && feq(a.c, 7) && feq(a.d, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("floor / ceil / round / trunc / frac – per-component");
  {
    micron::vec8 v(1.2f, 2.7f, -1.2f, -2.7f, 0.5f, -0.5f, 3.0f, -3.0f);

    micron::vec8 fl = v.floor();
    require_true(feq(fl.x, 1) && feq(fl.y, 2) && feq(fl.z, -2) && feq(fl.w, -3));
    require_true(feq(fl.d, -3));

    micron::vec8 cl = v.ceil();
    require_true(feq(cl.x, 2) && feq(cl.y, 3) && feq(cl.z, -1) && feq(cl.w, -2));

    micron::vec8 tr = v.trunc();
    require_true(feq(tr.x, 1) && feq(tr.y, 2) && feq(tr.z, -1) && feq(tr.w, -2));
    require_true(feq(tr.d, -3));

    // frac: v - trunc(v)
    micron::vec8 fr = v.frac();
    micron::vec8 reconstructed;
    reconstructed += tr;
    reconstructed += fr;
    require_true(veq(reconstructed, v, 1e-5f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sqrt / rsqrt – per-component");
  {
    micron::vec8 v(1, 4, 9, 16, 25, 36, 49, 64);
    micron::vec8 s = v.sqrt();
    require_true(feq(s.x, 1) && feq(s.y, 2) && feq(s.z, 3) && feq(s.w, 4));
    require_true(feq(s.a, 5) && feq(s.b, 6) && feq(s.c, 7) && feq(s.d, 8));

    // rsqrt * sqrt ≈ 1
    micron::vec8 rs = v.rsqrt();
    micron::vec8 ones = s * rs;
    require_true(vall_eq(ones, 1.0f, 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("exp / log – per-component round-trip");
  {
    micron::vec8 v(0, 1, 2, 3, -1, -2, 0.5f, -0.5f);
    micron::vec8 e = v.exp();
    micron::vec8 r = e.log();
    require_true(veq(r, v, 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("log2 – exact powers of two");
  {
    micron::vec8 v(1, 2, 4, 8, 16, 32, 64, 128);
    micron::vec8 l = v.log2();
    require_true(feq(l.x, 0) && feq(l.y, 1) && feq(l.z, 2) && feq(l.w, 3));
    require_true(feq(l.a, 4) && feq(l.b, 5) && feq(l.c, 6) && feq(l.d, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("log10 – exact powers of ten");
  {
    micron::vec8 v(1, 10, 100, 1000, 10000, 100000, 1000000, 10000000);
    micron::vec8 l = v.log10();
    for ( int i = 0; i < 8; ++i ) {
      float expected = (float)i;
      float actual = (&l.x)[i];     // components laid out sequentially
      require_true(feq(actual, expected, 1e-4f));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sin / cos / tan – key angles");
  {
    const float pi = (float)M_PI;
    const float pi2 = pi / 2.0f;
    micron::vec8 angles(0, pi2, pi, -pi2, pi / 4.0f, -pi / 4.0f, pi / 6.0f, pi / 3.0f);

    micron::vec8 s = angles.sin();
    micron::vec8 c = angles.cos();

    // sin(0)=0, sin(pi/2)=1, sin(pi)≈0, sin(-pi/2)=-1
    require_true(feq(s.x, 0.0f, 1e-5f));
    require_true(feq(s.y, 1.0f, 1e-5f));
    require_true(feq(s.z, 0.0f, 1e-4f));
    require_true(feq(s.w, -1.0f, 1e-5f));

    // cos(0)=1, cos(pi/2)≈0, cos(pi)=-1, cos(-pi/2)≈0
    require_true(feq(c.x, 1.0f, 1e-5f));
    require_true(feq(c.y, 0.0f, 1e-4f));
    require_true(feq(c.z, -1.0f, 1e-5f));

    // sin^2 + cos^2 == 1 for all components
    micron::vec8 identity = s * s + c * c;
    require_true(vall_eq(identity, 1.0f, 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("asin / acos / atan – round-trip");
  {
    micron::vec8 v(0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 0.25f, -0.25f, 0.75f);
    micron::vec8 rt = v.asin().sin();
    require_true(veq(rt, v, 1e-4f));

    micron::vec8 rt2 = v.acos().cos();
    require_true(veq(rt2.abs(), v.abs(), 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sinh / cosh / tanh – identities");
  {
    micron::vec8 v(0, 0.5f, -0.5f, 1, -1, 2, -2, 0.1f);

    // cosh^2 - sinh^2 == 1
    micron::vec8 ch = v.cosh();
    micron::vec8 sh = v.sinh();
    micron::vec8 diff = ch * ch - sh * sh;
    require_true(vall_eq(diff, 1.0f, 1e-4f));

    // tanh(x) == sinh(x)/cosh(x)
    micron::vec8 th = v.tanh();
    micron::vec8 ratio = sh / ch;
    require_true(veq(th, ratio, 1e-5f));
  }
  end_test_case();

  // ================================================================ //
  //  Clamp, saturate, sign                                           //
  // ================================================================ //
  test_case("clamp(lo, hi) – scalar bounds");
  {
    micron::vec8 v(-2, -1, 0, 0.5f, 1, 1.5f, 2, 3);
    micron::vec8 c = v.clamp(0.0f, 1.0f);
    require_true(feq(c.x, 0) && feq(c.y, 0) && feq(c.z, 0) && feq(c.w, 0.5f));
    require_true(feq(c.a, 1) && feq(c.b, 1) && feq(c.c, 1) && feq(c.d, 1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clamp(vec, vec) – per-component bounds");
  {
    micron::vec8 v(-1, 0.5f, 2, -3, 10, -10, 0.3f, 0.7f);
    micron::vec8 lo(0, 0, 0, 0, 0, 0, 0, 0);
    micron::vec8 hi(1, 1, 1, 1, 1, 1, 1, 1);
    micron::vec8 c = v.clamp(lo, hi);
    require_true(feq(c.x, 0) && feq(c.y, 0.5f) && feq(c.z, 1) && feq(c.w, 0));
    require_true(feq(c.a, 1) && feq(c.b, 0) && feq(c.c, 0.3f) && feq(c.d, 0.7f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("saturate – clamps to [0,1]");
  {
    micron::vec8 v(-1, 0, 0.5f, 1, 2, -0.5f, 0.99f, 1.01f);
    micron::vec8 s = v.saturate();
    require_true(feq(s.x, 0) && feq(s.y, 0) && feq(s.z, 0.5f) && feq(s.w, 1));
    require_true(feq(s.a, 1) && feq(s.b, 0) && feq(s.c, 0.99f) && feq(s.d, 1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sign – returns -1, 0, or 1 per component");
  {
    micron::vec8 v(-5, -0.001f, 0, 0.001f, 1, -1, 100, -100);
    micron::vec8 s = v.sign();
    require_true(feq(s.x, -1) && feq(s.y, -1) && feq(s.z, 0) && feq(s.w, 1));
    require_true(feq(s.a, 1) && feq(s.b, -1) && feq(s.c, 1) && feq(s.d, -1));
  }
  end_test_case();

  // ================================================================ //
  //  min / max / pow / atan2 element-wise binary                     //
  // ================================================================ //
  test_case("min / max – element-wise binary");
  {
    micron::vec8 a(1, 5, 3, 7, 2, 8, 4, 6);
    micron::vec8 b(4, 2, 6, 1, 8, 3, 7, 5);
    micron::vec8 mn = a.min(b);
    micron::vec8 mx = a.max(b);
    require_true(feq(mn.x, 1) && feq(mn.y, 2) && feq(mn.z, 3) && feq(mn.w, 1));
    require_true(feq(mx.x, 4) && feq(mx.y, 5) && feq(mx.z, 6) && feq(mx.w, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pow (vector exponent) – element-wise");
  {
    micron::vec8 base(2, 3, 4, 2, 3, 4, 2, 3);
    micron::vec8 exp_(1, 2, 3, 4, 1, 2, 3, 4);
    micron::vec8 r = base.pow(exp_);
    require_true(feq(r.x, 2) && feq(r.y, 9) && feq(r.z, 64) && feq(r.w, 16));
    require_true(feq(r.a, 3) && feq(r.b, 16) && feq(r.c, 8) && feq(r.d, 81));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pow (scalar exponent)");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 r = v.pow(2.0f);
    require_true(feq(r.x, 1) && feq(r.y, 4) && feq(r.z, 9) && feq(r.w, 16));
    require_true(feq(r.a, 25) && feq(r.b, 36) && feq(r.c, 49) && feq(r.d, 64));
  }
  end_test_case();

  // ================================================================ //
  //  fma / mul_add                                                   //
  // ================================================================ //
  test_case("fma(v,u) – fused multiply-add per component");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(2, 2, 2, 2, 2, 2, 2, 2);
    micron::vec8 c(10, 10, 10, 10, 10, 10, 10, 10);
    micron::vec8 r = a.fma(b, c);     // a*b + c
    require_true(feq(r.x, 12) && feq(r.y, 14) && feq(r.z, 16) && feq(r.w, 18));
    require_true(feq(r.a, 20) && feq(r.b, 22) && feq(r.c, 24) && feq(r.d, 26));

    // mul_add is an alias
    require_true(veq(a.mul_add(b, c), r));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fma(scalar, u) – broadcast scalar multiply");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 u(1, 1, 1, 1, 1, 1, 1, 1);
    micron::vec8 r = a.fma(3.0f, u);     // a*3 + 1
    require_true(feq(r.x, 4) && feq(r.y, 7) && feq(r.z, 10) && feq(r.w, 13));
    require_true(feq(r.a, 16) && feq(r.b, 19) && feq(r.c, 22) && feq(r.d, 25));
  }
  end_test_case();

  // ================================================================ //
  //  Swizzle / sub-vector accessors                                  //
  // ================================================================ //
  test_case("lo() / hi() – lower and upper halves");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec4 lo = v.lo();
    micron::vec4 hi = v.hi();
    require_true(feq(lo.x, 1) && feq(lo.y, 2) && feq(lo.z, 3) && feq(lo.w, 4));
    require_true(feq(hi.x, 5) && feq(hi.y, 6) && feq(hi.z, 7) && feq(hi.w, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lo_xy / lo_zw / hi_ab / hi_cd – quarter views");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec2 xy = v.lo_xy();
    require_true(feq(xy.x, 1) && feq(xy.y, 2));
    micron::vec2 zw = v.lo_zw();
    require_true(feq(zw.x, 3) && feq(zw.y, 4));
    micron::vec2 ab = v.hi_ab();
    require_true(feq(ab.x, 5) && feq(ab.y, 6));
    micron::vec2 cd = v.hi_cd();
    require_true(feq(cd.x, 7) && feq(cd.y, 8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("even() / odd() – interleaved component selection");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec4 ev = v.even();     // x,z,a,c = 1,3,5,7
    micron::vec4 od = v.odd();      // y,w,b,d = 2,4,6,8
    require_true(feq(ev.x, 1) && feq(ev.y, 3) && feq(ev.z, 5) && feq(ev.w, 7));
    require_true(feq(od.x, 2) && feq(od.y, 4) && feq(od.z, 6) && feq(od.w, 8));
  }
  end_test_case();

  // ================================================================ //
  //  Comparison vector ops (< <= > >=)                               //
  // ================================================================ //
  test_case("comparison operators return 0/1 per component");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(4, 4, 4, 4, 4, 4, 4, 4);

    micron::vec8 lt = a < b;
    require_true(feq(lt.x, 1) && feq(lt.y, 1) && feq(lt.z, 1) && feq(lt.w, 0));
    require_true(feq(lt.a, 0) && feq(lt.b, 0) && feq(lt.c, 0) && feq(lt.d, 0));

    micron::vec8 gt = a > b;
    require_true(feq(gt.x, 0) && feq(gt.y, 0) && feq(gt.z, 0) && feq(gt.w, 0));
    require_true(feq(gt.a, 1) && feq(gt.b, 1) && feq(gt.c, 1) && feq(gt.d, 1));

    micron::vec8 le = a <= 4.0f;
    require_true(feq(le.x, 1) && feq(le.y, 1) && feq(le.z, 1) && feq(le.w, 1));
    require_true(feq(le.a, 0));

    micron::vec8 ge = 4.0f >= a;
    require_true(feq(ge.x, 1) && feq(ge.d, 0));
  }
  end_test_case();

  // ================================================================ //
  //  dvec8 (double precision)                                        //
  // ================================================================ //
  test_case("dvec8 – basic construction and dot product");
  {
    micron::dvec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(feq(v.dot(v), 204.0, 1e-12));
    require_true(feq(v.magnitude(), std::sqrt(204.0), 1e-12));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("dvec8 – normalize round-trip");
  {
    micron::dvec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::dvec8 n = v.normalized();
    require_true(feq(n.magnitude(), 1.0, 1e-12));
    require_true(n.is_normalized(1e-10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("dvec8 – sin^2 + cos^2 == 1 (higher precision)");
  {
    micron::dvec8 v(0, 0.5, 1.0, -0.5, -1.0, 1.5, -1.5, 2.0);
    micron::dvec8 s = v.sin();
    micron::dvec8 c = v.cos();
    micron::dvec8 id = s * s + c * c;
    for ( int i = 0; i < 8; ++i )
      require_true(feq((&id.x)[i], 1.0, 1e-12));
  }
  end_test_case();

  // ================================================================ //
  //  Cross-function invariants                                       //
  // ================================================================ //
  test_case("invariant: dot(v,v) == squared_norm");
  {
    micron::vec8 v(2, 3, 1, 4, 5, 1, 2, 3);
    require_true(feq(v.dot(v), v.squared_norm()));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: (a+b) == (b+a) commutativity");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(8, 7, 6, 5, 4, 3, 2, 1);
    require_true(veq(a + b, b + a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: (a*s)/s == a for nonzero scalar");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    require_true(veq((a * 7.0f) / 7.0f, a, 1e-5f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: projection + rejection == original");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 axis = V(1, 0, 0, 0, 0, 0, 0, 0);
    require_true(veq(v.project_onto(axis) + v.reject_from(axis), v, 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: fma(v,u) == v*a + u element-wise");
  {
    micron::vec8 a(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 b(2, 3, 4, 5, 6, 7, 8, 9);
    micron::vec8 c(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f);
    require_true(veq(a.fma(b, c), a * b + c, 1e-4f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: sum == dot(v, ones)");
  {
    micron::vec8 v(1, 2, 3, 4, 5, 6, 7, 8);
    micron::vec8 ones(1, 1, 1, 1, 1, 1, 1, 1);
    require_true(feq(v.sum(), v.dot(ones)));
  }
  end_test_case();

  sb::print("=== ALL VEC8 TESTS PASSED ===");
  return 0;
}
