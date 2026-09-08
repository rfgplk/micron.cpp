// math_defects_quaternions.cpp -- Snowball gates for the quaternion defects in MATH_AUDIT
// (#10, #11, #17, #18, #19, #26).
//
// Every case asserts the property that SHOULD hold, so the file FAILS on a tree carrying
// the defect.  Section (#17) only reaches the SIMD bodies when the translation unit is
// built with -mavx2 -mfma (amd64) or for aarch64/armv7-a NEON; on a scalar build it
// degenerates into a control that the tail loop still agrees with itself.

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
using v3f = micron::vector_3<f32>;
using qd = quaternions::quaternion<f64>;
using qf = quaternions::quaternion<f32>;

static constexpr f64 k_isqrt2 = 0x1.6a09e667f3bcdp-1;      // 1/sqrt(2)
static constexpr f32 k_isqrt2f = 0x1.6a09e6p-1f;

static bool
near(f64 a, f64 b, f64 eps)
{
  const f64 d = a - b;
  return (d < 0 ? -d : d) <= eps;
}

static bool
nearf(f32 a, f32 b, f32 eps)
{
  const f32 d = a - b;
  return (d < 0 ? -d : d) <= eps;
}

// perpendicular a, b of equal magnitude m must always give the same 90 deg rotation about +z
static bool
perp_z90(f64 m)
{
  const auto q = quaternions::from_two_vectors<f64>(v3{ m, 0, 0 }, v3{ 0, m, 0 });
  return near(q.x, 0.0, 1e-15) && near(q.y, 0.0, 1e-15) && near(q.z, k_isqrt2, 1e-15) && near(q.w, k_isqrt2, 1e-15);
}

static bool
perp_z90f(f32 m)
{
  const auto q = quaternions::from_two_vectors<f32>(v3f{ m, 0, 0 }, v3f{ 0, m, 0 });
  return nearf(q.x, 0.0f, 1e-7f) && nearf(q.y, 0.0f, 1e-7f) && nearf(q.z, k_isqrt2f, 1e-7f) && nearf(q.w, k_isqrt2f, 1e-7f);
}

// the SIMD bodies are allowed to round differently from frsqrt (the NEON f32 one is a
// rsqrt estimate plus two Newton steps); what they may NOT do is answer a different KIND
// of value from the scalar path -- a finite quaternion where normalize() says NaN
template<typename T>
static bool
lane_agrees(T a, T b, T tol)
{
  if ( ieee::is_nan<T>(a) || ieee::is_nan<T>(b) ) return ieee::is_nan<T>(a) && ieee::is_nan<T>(b);
  if ( !ieee::is_finite<T>(a) || !ieee::is_finite<T>(b) ) return a == b;
  const T d = a - b;
  const T ab = b < 0 ? -b : b;
  return (d < 0 ? -d : d) <= tol * (ab > T(1) ? ab : T(1));
}

// batched_normalize must answer element-for-element what normalize() answers, at every
// index and for every batch length -- the vector body and its scalar tail cannot disagree
template<typename T>
static bool
batched_matches_scalar(quaternions::quaternion<T> v, T tol)
{
  quaternions::quaternion<T> in[9], out[9];
  for ( usize k = 0; k < 9; ++k ) in[k] = v;
  const auto ref = quaternions::normalize<T>(v);
  for ( usize n = 1; n <= 9; ++n ) {
    for ( usize k = 0; k < 9; ++k ) out[k] = quaternions::quaternion<T>{ T(-7), T(-7), T(-7), T(-7) };
    quaternions::batched_normalize<T>(in, out, n);
    for ( usize k = 0; k < n; ++k ) {
      if ( !lane_agrees<T>(out[k].x, ref.x, tol) ) return false;
      if ( !lane_agrees<T>(out[k].y, ref.y, tol) ) return false;
      if ( !lane_agrees<T>(out[k].z, ref.z, tol) ) return false;
      if ( !lane_agrees<T>(out[k].w, ref.w, tol) ) return false;
    }
  }
  return true;
}

int
main()
{
  print("=== QUATERNION DEFECT GATES ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("#11 from_two_vectors is scale-free: small but well-conditioned inputs");
  {
    require_true(perp_z90(1.0));
    require_true(perp_z90(1e-3));
    require_true(perp_z90(1e-6));
    require_true(perp_z90(1e-7));
    require_true(perp_z90(1e-30));
    require_true(perp_z90(1e-160));
    require_true(perp_z90f(1e-2f));
    require_true(perp_z90f(1e-4f));
    require_true(perp_z90f(1e-10f));
    require_true(perp_z90f(1e-30f));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("#19 from_two_vectors does not square its arguments into the exponent ceiling");
  {
    require_true(perp_z90(1e70));
    require_true(perp_z90(1.2e77));
    require_true(perp_z90(1e100));
    require_true(perp_z90(1e155));
    require_true(perp_z90(1e200));
    require_true(perp_z90(1e300));
    require_true(perp_z90f(1e9f));
    require_true(perp_z90f(4e9f));
    require_true(perp_z90f(1e19f));
    require_true(perp_z90f(1e30f));

    // wildly mismatched magnitudes: the answer depends only on the two directions
    const auto qm = quaternions::from_two_vectors<f64>(v3{ 1e-300, 0, 0 }, v3{ 0, 1e300, 0 });
    require_true(qm.all_finite());
    require_true(near(qm.z, k_isqrt2, 1e-15) && near(qm.w, k_isqrt2, 1e-15));

    // anti-parallel at a magnitude that overflows the unscaled form is still a 180 deg flip
    const auto qa = quaternions::from_two_vectors<f64>(v3{ 1e200, 0, 0 }, v3{ -1e200, 0, 0 });
    require_true(near(qa.squared_norm(), 1.0, 1e-12));
    const auto ra = quaternions::rotate<f64>(qa, v3{ 1, 0, 0 });
    require_true(near(ra.x, -1.0, 1e-12) && near(ra.y, 0.0, 1e-12) && near(ra.z, 0.0, 1e-12));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("from_two_vectors keeps its documented degenerate answers");
  {
    const auto qz = quaternions::from_two_vectors<f64>(v3{ 0, 0, 0 }, v3{ 1, 0, 0 });
    require_true(qz.all_finite() && near(qz.w, 1.0, 1e-15) && near(qz.x, 0.0, 1e-15));
    const auto qz2 = quaternions::from_two_vectors<f64>(v3{ 1, 0, 0 }, v3{ 0, 0, 0 });
    require_true(qz2.all_finite() && near(qz2.w, 1.0, 1e-15));
    const auto qp = quaternions::from_two_vectors<f64>(v3{ 3, 0, 0 }, v3{ 5, 0, 0 });
    require_true(near(qp.w, 1.0, 1e-15) && near(qp.x, 0.0, 1e-15));
    const auto qn = quaternions::from_two_vectors<f64>(v3{ 1, 0, 0 }, v3{ -1, 0, 0 });
    require_true(near(qn.z, 1.0, 1e-15) && near(qn.w, 0.0, 1e-15));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("#10 log_map_pade's fallback branch takes the short arc for q.w < 0");
  {
    // rotation angles whose cos(half) sits below the Pade floor, i.e. inside the fallback branch
    const f64 angles[] = { 2.1, 2.2, 2.6, 3.0, 3.1 };
    for ( const f64 a : angles ) {
      const auto q = quaternions::from_axis_angle<f64>(0, 0, 1, a);
      const qd qn2{ -q.x, -q.y, -q.z, -q.w };
      const auto va = quaternions::log_map_pade<f64>(q);
      const auto vb = quaternions::log_map_pade<f64>(qn2);
      require_true(near(va.x, vb.x, 1e-14));
      require_true(near(va.y, vb.y, 1e-14));
      require_true(near(va.z, vb.z, 1e-14));
      // and the shared answer is the short arc, never the 2*pi complement
      require_true(near(va.z, a, 1e-9));
    }
    // the same invariance across the whole range, on both sides of the branch
    for ( int i = 1; i <= 600; ++i ) {
      const f64 a = 3.14 * f64(i) / 600.0;
      const auto q = quaternions::from_axis_angle<f64>(0, 1, 0, a);
      const qd qn2{ -q.x, -q.y, -q.z, -q.w };
      const auto va = quaternions::log_map_pade<f64>(q);
      const auto vb = quaternions::log_map_pade<f64>(qn2);
      require_true(near(va.y, vb.y, 1e-14));
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("#18 log_map_pade is continuous across its branch and stays inside 3e-4 relative");
  {
    // straddle cos(half) = 0.5, the old floor: 4e-6 rad of input used to move the output
    // by 1.3e-1 rad
    const auto q1 = quaternions::from_axis_angle<f64>(1, 0, 0, 2.094392);
    const auto q2 = quaternions::from_axis_angle<f64>(1, 0, 0, 2.094396);
    const auto l1 = quaternions::log_map_pade<f64>(q1);
    const auto l2 = quaternions::log_map_pade<f64>(q2);
    require_true(near(l1.x, l2.x, 1e-5));
    require_true(near(l1.x, 2.094392, 1e-6));
    require_true(near(l2.x, 2.094396, 1e-6));

    // whole range: no rotation angle may be reported more than 3e-4 relative off
    f64 worst = 0.0;
    for ( int i = 1; i <= 3000; ++i ) {
      const f64 a = 3.14159 * f64(i) / 3000.0;
      const auto q = quaternions::from_axis_angle<f64>(0, 1, 0, a);
      const auto l = quaternions::log_map_pade<f64>(q);
      const f64 e = (l.y - a) / a;
      const f64 ae = e < 0 ? -e : e;
      if ( ae > worst ) worst = ae;
    }
    require_true(worst <= 3e-4);

    // the public entry point: 2 rad/s used to read as 2.089
    const auto w0 = quaternions::identity<f64>();
    const auto w1 = quaternions::from_axis_angle<f64>(1, 0, 0, 2.0);
    const auto wp = quaternions::angular_velocity_pade<f64>(w0, w1, 1.0);
    require_true(near(wp.x, 2.0, 6e-4));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("#26 to_axis_angle has no deadband: atan2 resolves every representable |v|");
  {
    const f64 angles[] = { 2e-12, 1.9e-12, 1e-12, 1e-14, 1e-30, 1e-120 };
    for ( const f64 a : angles ) {
      const qd q{ a * 0.5, 0, 0, 1.0 };
      const auto aa = quaternions::to_axis_angle<f64>(q);
      require_true(near(aa.angle, a, a * 1e-6));
      require_true(near(aa.axis.x, 1.0, 1e-12));
      // 1 kHz sample rate: the rate must not read as an exact zero
      const auto wv = quaternions::angular_velocity<f64>(quaternions::identity<f64>(), q, 1e-3);
      require_true(near(wv.x, a * 1e3, a * 1e3 * 1e-6));
    }
    // 1e-15 is the floor here: below ~2e-19 the f32 |v|^2 itself goes subnormal
    const f32 anglesf[] = { 2e-6f, 1.9e-6f, 1e-6f, 1e-10f, 1e-15f };
    for ( const f32 a : anglesf ) {
      const qf q{ a * 0.5f, 0, 0, 1.0f };
      const auto aa = quaternions::to_axis_angle<f32>(q);
      require_true(nearf(aa.angle, a, a * 1e-3f));
      require_true(nearf(aa.axis.x, 1.0f, 1e-6f));
    }

    // the exact-zero contract the guard is actually there for
    const auto ai = quaternions::to_axis_angle<f64>(quaternions::identity<f64>());
    require_true(ai.angle == 0.0 && ai.axis.x == 1.0 && ai.axis.y == 0.0 && ai.axis.z == 0.0);
    const auto an = quaternions::to_axis_angle<f64>(qd{ 0, 0, 0, -1.0 });
    require_true(an.angle == 0.0 && an.axis.x == 1.0);
    const auto az = quaternions::to_axis_angle<f64>(qd{ 0, 0, 0, 0 });
    require_true(az.angle == 0.0 && az.axis.x == 1.0);

    // a real rotation still round-trips
    const auto qr = quaternions::from_axis_angle<f64>(0.6, 0.0, 0.8, 1.234);
    const auto ar = quaternions::to_axis_angle<f64>(qr);
    require_true(near(ar.angle, 1.234, 1e-12));
    require_true(near(ar.axis.x, 0.6, 1e-12) && near(ar.axis.z, 0.8, 1e-12));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("#17 batched_normalize answers the same at every index in the batch");
  {
    const f64 inf = ieee::inf_v<f64>();
    const f32 inff = ieee::inf_v<f32>();
    require_true(batched_matches_scalar<f64>(qd{ 1e-160, 0, 0, 0 }, 1e-14));
    require_true(batched_matches_scalar<f64>(qd{ 0, 0, 0, 0 }, 1e-14));
    require_true(batched_matches_scalar<f64>(qd{ inf, 0, 0, 1 }, 1e-14));
    require_true(batched_matches_scalar<f64>(qd{ 1e-200, 1e-200, 0, 0 }, 1e-14));
    require_true(batched_matches_scalar<f32>(qf{ 1e-30f, 0, 0, 0 }, 1e-6f));
    require_true(batched_matches_scalar<f32>(qf{ 0, 0, 0, 0 }, 1e-6f));
    require_true(batched_matches_scalar<f32>(qf{ inff, 0, 0, 1 }, 1e-6f));
    // finite components whose SQUARES overflow: normalize() answers (0,0,0,0) via frsqrt(inf),
    // the NEON f32 rsqrt estimate answers NaN.  Only this shape reaches the armv7-a body
    require_true(batched_matches_scalar<f64>(qd{ 1e200, 0, 0, 0 }, 1e-14));
    require_true(batched_matches_scalar<f64>(qd{ 1e200, 1e200, 0, 1 }, 1e-14));
    require_true(batched_matches_scalar<f32>(qf{ 1e30f, 0, 0, 0 }, 1e-6f));
    require_true(batched_matches_scalar<f32>(qf{ 2e19f, 2e19f, 2e19f, 2e19f }, 1e-6f));

    // control: the fast path is still the fast path for ordinary data
    require_true(batched_matches_scalar<f64>(qd{ 0.1, 0.2, 0.3, 0.4 }, 1e-14));
    require_true(batched_matches_scalar<f64>(qd{ -3.0, 0.0, 0.0, 4.0 }, 1e-14));
    require_true(batched_matches_scalar<f32>(qf{ 0.1f, 0.2f, 0.3f, 0.4f }, 1e-6f));

    qd in[8], out[8];
    for ( usize k = 0; k < 8; ++k ) in[k] = qd{ 0.1 * f64(k + 1), 0.2, -0.3, 0.4 };
    quaternions::batched_normalize<f64>(in, out, 8);
    for ( usize k = 0; k < 8; ++k ) require_true(near(out[k].squared_norm(), 1.0, 1e-14));
  }
  end_test_case();

  print("=== QUATERNION DEFECT GATES PASSED ===");
  return 1;
}
