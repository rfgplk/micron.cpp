// math_quaternions.cpp — Snowball tests for math::quaternions::*.

#include "../../src/math/quaternions/quaternions.hpp"
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

using qq = quaternions::quaternion<f64>;
using v3 = micron::vector_3<f64>;

static bool
near(f64 a, f64 b, f64 eps = 1e-12)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

static bool
near_q(const qq &a, const qq &b, f64 eps = 1e-10)
{
  return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps) && near(a.w, b.w, eps);
}

int
main()
{
  print("=== QUATERNIONS ===");

  test_case("identity is the multiplicative identity");
  {
    auto i = quaternions::identity<f64>();
    qq q{ 0.5, -0.3, 0.8, 0.1 };
    require_true(near_q(quaternions::multiply(q, i), q));
    require_true(near_q(quaternions::multiply(i, q), q));
  }
  end_test_case();

  test_case("Hamilton product agrees with the standard formula");
  {
    // i · j = k  =>  (1, 0, 0, 0) ⊗ (0, 1, 0, 0) = (0, 0, 1, 0)
    qq qi{ 1, 0, 0, 0 };
    qq qj{ 0, 1, 0, 0 };
    auto qij = quaternions::multiply(qi, qj);
    require_true(near(qij.x, 0.0) && near(qij.y, 0.0) && near(qij.z, 1.0) && near(qij.w, 0.0));
    // j · i = -k
    auto qji = quaternions::multiply(qj, qi);
    require_true(near(qji.z, -1.0));
  }
  end_test_case();

  test_case("conjugate / inverse round-trip on a non-unit quaternion");
  {
    qq q{ 1.0, 2.0, 3.0, 4.0 };
    auto q_inv = quaternions::inverse(q);
    auto p = quaternions::multiply(q, q_inv);
    require_true(near(p.w, 1.0, 1e-10));
    require_true(near(p.x, 0.0, 1e-10));
    require_true(near(p.y, 0.0, 1e-10));
    require_true(near(p.z, 0.0, 1e-10));
    // For unit q: inverse == conjugate
    auto qu = q.normalized();
    auto qu_inv = quaternions::inverse(qu);
    auto qu_conj = quaternions::conjugate(qu);
    require_true(near_q(qu_inv, qu_conj, 1e-12));
  }
  end_test_case();

  test_case("axis-angle round-trip");
  {
    v3 axis{ 0.6, 0.0, 0.8 };      // unit axis
    f64 angle = 1.234;
    auto q = quaternions::from_axis_angle(axis, angle);
    require_true(near(q.squared_norm(), 1.0, 1e-12));
    auto aa = quaternions::to_axis_angle(q);
    require_true(near(aa.angle, angle, 1e-10));
    require_true(near(aa.axis.x, axis.x, 1e-10));
    require_true(near(aa.axis.y, axis.y, 1e-10));
    require_true(near(aa.axis.z, axis.z, 1e-10));
  }
  end_test_case();

  test_case("rotate(): 90° about z sends (1, 0, 0) to (0, 1, 0)");
  {
    auto q = quaternions::from_axis_angle<f64>(0, 0, 1, 1.5707963267948966);
    v3 v{ 1, 0, 0 };
    auto r = quaternions::rotate(q, v);
    require_true(near(r.x, 0.0, 1e-12));
    require_true(near(r.y, 1.0, 1e-12));
    require_true(near(r.z, 0.0, 1e-12));
  }
  end_test_case();

  test_case("matrix ↔ quaternion round-trip");
  {
    // Build q for 1 rad about (0.6, 0.8, 0).
    auto q = quaternions::from_axis_angle<f64>(0.6, 0.8, 0.0, 1.0);
    auto R = quaternions::to_matrix(q);
    auto q2 = quaternions::from_matrix(R);
    // Quaternion sign is ambiguous (q and -q rotate identically); pick whichever flip aligns with q.
    f64 d = q.dot(q2);
    if ( d < 0 ) {
      q2.x = -q2.x;
      q2.y = -q2.y;
      q2.z = -q2.z;
      q2.w = -q2.w;
    }
    require_true(near_q(q, q2, 1e-10));
  }
  end_test_case();

  test_case("Euler ZYX round-trip away from gimbal lock");
  {
    f64 yaw = 0.4, pitch = 0.2, roll = -0.3;
    auto q = quaternions::from_euler_zyx<f64>(yaw, pitch, roll);
    require_true(near(q.squared_norm(), 1.0, 1e-12));
    auto e = quaternions::to_euler_zyx(q);
    require_true(near(e.x, yaw, 1e-10));
    require_true(near(e.y, pitch, 1e-10));
    require_true(near(e.z, roll, 1e-10));
  }
  end_test_case();

  test_case("composition: q1 ⊗ q2 rotates v as q1(q2(v))");
  {
    auto qz = quaternions::from_axis_angle<f64>(0, 0, 1, 1.5707963267948966);      // 90° z
    auto qx = quaternions::from_axis_angle<f64>(1, 0, 0, 1.5707963267948966);      // 90° x
    auto qzx = quaternions::compose(qz, qx);
    v3 v{ 0, 1, 0 };
    auto r1 = quaternions::rotate(qzx, v);
    auto r2 = quaternions::rotate(qz, quaternions::rotate(qx, v));
    require_true(near(r1.x, r2.x, 1e-12));
    require_true(near(r1.y, r2.y, 1e-12));
    require_true(near(r1.z, r2.z, 1e-12));
  }
  end_test_case();

  test_case("slerp endpoints + halfway angle");
  {
    auto q0 = quaternions::identity<f64>();
    auto q1 = quaternions::from_axis_angle<f64>(0, 0, 1, 1.0);      // 1 rad about z
    require_true(near_q(quaternions::slerp(q0, q1, 0.0), q0, 1e-12));
    require_true(near_q(quaternions::slerp(q0, q1, 1.0), q1, 1e-10));
    // Midpoint should be a rotation of 0.5 rad about z.
    auto mid = quaternions::slerp(q0, q1, 0.5);
    auto expected = quaternions::from_axis_angle<f64>(0, 0, 1, 0.5);
    require_true(near_q(mid, expected, 1e-12));
  }
  end_test_case();

  test_case("nlerp produces a unit-length quaternion");
  {
    auto q0 = quaternions::from_axis_angle<f64>(1, 0, 0, 0.4);
    auto q1 = quaternions::from_axis_angle<f64>(0, 1, 0, 0.7);
    auto m = quaternions::nlerp(q0, q1, 0.3);
    require_true(near(m.squared_norm(), 1.0, 1e-12));
  }
  end_test_case();

  test_case("kinematics: derivative matches ½·q⊗(0,ω)");
  {
    auto q = quaternions::identity<f64>();
    v3 omega{ 0, 0, 1 };
    auto dq = quaternions::derivative<f64>(q, omega);
    // For identity q: dq = (0, 0, 0.5, 0)
    require_true(near(dq.x, 0.0));
    require_true(near(dq.y, 0.0));
    require_true(near(dq.z, 0.5));
    require_true(near(dq.w, 0.0));
  }
  end_test_case();

  test_case("kinematics: integrate ω = (0, 0, π) over dt = 1 → quat for π about z");
  {
    auto q0 = quaternions::identity<f64>();
    v3 omega{ 0, 0, 3.141592653589793 };
    auto q1 = quaternions::integrate<f64>(q0, omega, 1.0);
    // expected (sin(π/2)·z, cos(π/2)) = (0, 0, 1, 0)
    require_true(near(q1.w, 0.0, 1e-10));
    require_true(near(q1.x, 0.0, 1e-10));
    require_true(near(q1.y, 0.0, 1e-10));
    require_true(near(q1.z, 1.0, 1e-10));
  }
  end_test_case();

  test_case("kinematics: angular_velocity recovers the integration ω");
  {
    auto q0 = quaternions::identity<f64>();
    v3 omega{ 0.4, -0.7, 1.2 };
    f64 dt = 0.25;
    auto q1 = quaternions::integrate<f64>(q0, omega, dt);
    auto wr = quaternions::angular_velocity<f64>(q0, q1, dt);
    require_true(near(wr.x, omega.x, 1e-10));
    require_true(near(wr.y, omega.y, 1e-10));
    require_true(near(wr.z, omega.z, 1e-10));
  }
  end_test_case();

  test_case("kinematics: integrate_small_angle matches integrate for tiny ω·dt");
  {
    auto q0 = quaternions::from_axis_angle<f64>(0.3, 0.8, -0.5, 0.27);
    v3 omega{ 0.5, -0.2, 0.3 };
    f64 dt = 0.01;      // |ω·dt| ≈ 6e-3 rad — well inside Taylor regime
    auto q_exact = quaternions::integrate<f64>(q0, omega, dt);
    auto q_taylor = quaternions::integrate_small_angle<f64>(q0, omega, dt);
    require_true(near(q_exact.x, q_taylor.x, 1e-12));
    require_true(near(q_exact.y, q_taylor.y, 1e-12));
    require_true(near(q_exact.z, q_taylor.z, 1e-12));
    require_true(near(q_exact.w, q_taylor.w, 1e-12));
    require_true(near(q_taylor.squared_norm(), 1.0, 1e-12));
  }
  end_test_case();

  test_case("kinematics: integrate_small_angle handles ω = 0 cleanly");
  {
    auto q0 = quaternions::from_axis_angle<f64>(1, 0, 0, 0.5);
    v3 zero{ 0, 0, 0 };
    auto q1 = quaternions::integrate_small_angle<f64>(q0, zero, 0.1);
    require_true(near_q(q0, q1, 1e-14));
  }
  end_test_case();

  test_case("kinematics: integrate_pade is auto-unit and matches exact at moderate ω·dt");
  {
    auto q0 = quaternions::identity<f64>();
    v3 omega{ 0.6, -0.3, 0.9 };
    f64 dt = 0.5;      // |ω·dt| ≈ 0.57 rad — Padé[2/2] still very accurate
    auto q_exact = quaternions::integrate<f64>(q0, omega, dt);
    auto q_pade = quaternions::integrate_pade<f64>(q0, omega, dt);
    require_true(near(q_pade.squared_norm(), 1.0, 1e-12));
    require_true(near(q_exact.x, q_pade.x, 1e-5));
    require_true(near(q_exact.y, q_pade.y, 1e-5));
    require_true(near(q_exact.z, q_pade.z, 1e-5));
    require_true(near(q_exact.w, q_pade.w, 1e-5));
  }
  end_test_case();

  test_case("kinematics: integrate_pade short-circuits to identity for ω = 0");
  {
    auto q0 = quaternions::from_axis_angle<f64>(0, 1, 0, 0.3);
    v3 zero{ 0, 0, 0 };
    auto q1 = quaternions::integrate_pade<f64>(q0, zero, 0.7);
    require_true(near_q(q0, q1, 1e-15));
  }
  end_test_case();

  test_case("kinematics: angular_velocity_pade inverts integrate_pade");
  {
    // Round-trip error is dominated by the order mismatch between the
    // Padé[2/2] forward map and the Padé[1/1] log-map: theoretical
    // bound at this scale is ~(|ω·dt|/2)⁵/720 ≈ 2·10⁻⁸.
    auto q0 = quaternions::identity<f64>();
    v3 omega{ 0.3, 0.4, -0.2 };
    f64 dt = 0.4;
    auto q1 = quaternions::integrate_pade<f64>(q0, omega, dt);
    auto wr = quaternions::angular_velocity_pade<f64>(q0, q1, dt);
    require_true(near(wr.x, omega.x, 1e-7));
    require_true(near(wr.y, omega.y, 1e-7));
    require_true(near(wr.z, omega.z, 1e-7));
  }
  end_test_case();

  test_case("kinematics: log_map_pade vs exact axis-angle for moderate rotation");
  {
    v3 axis{ 0.6, 0.0, 0.8 };      // unit
    f64 angle = 0.7;               // ~ 40°, well inside Padé safe region
    auto q = quaternions::from_axis_angle(axis, angle);
    auto v = quaternions::log_map_pade<f64>(q);
    require_true(near(v.x, axis.x * angle, 1e-4));
    require_true(near(v.y, axis.y * angle, 1e-4));
    require_true(near(v.z, axis.z * angle, 1e-4));
  }
  end_test_case();

  test_case("kinematics: log_map_pade selects short arc for negative-w quaternions");
  {
    // q and -q describe the same rotation; log_map should not depend on sign.
    auto q = quaternions::from_axis_angle<f64>(0, 0, 1, 0.5);
    quaternions::quaternion<f64> qn{ -q.x, -q.y, -q.z, -q.w };
    auto va = quaternions::log_map_pade<f64>(q);
    auto vb = quaternions::log_map_pade<f64>(qn);
    require_true(near(va.x, vb.x, 1e-14));
    require_true(near(va.y, vb.y, 1e-14));
    require_true(near(va.z, vb.z, 1e-14));
  }
  end_test_case();

  test_case("algebra: Hamilton product is associative up to tolerance");
  {
    qq a{ 0.3, -0.7, 1.2, 0.4 };
    qq b{ -1.1, 0.5, -0.2, 0.9 };
    qq c{ 0.6, 0.8, 0.1, -0.5 };
    auto lhs = quaternions::multiply(quaternions::multiply(a, b), c);
    auto rhs = quaternions::multiply(a, quaternions::multiply(b, c));
    require_true(near_q(lhs, rhs, 1e-12));
  }
  end_test_case();

  test_case("algebra: norm multiplicativity |a ⊗ b| = |a| · |b|");
  {
    qq a{ 1.0, 2.0, 3.0, 4.0 };
    qq b{ -0.5, 0.7, -1.3, 2.1 };
    auto p = quaternions::multiply(a, b);
    f64 lhs = p.magnitude();
    f64 rhs = a.magnitude() * b.magnitude();
    require_true(near(lhs, rhs, 1e-12));
  }
  end_test_case();

  test_case("algebra: inverse_unit equals conjugate on unit q and avoids division error");
  {
    auto q = quaternions::from_axis_angle<f64>(0.5, -0.3, 0.8, 1.7);
    auto inv_u = quaternions::inverse_unit(q);
    auto inv_g = quaternions::inverse(q);
    auto conj = quaternions::conjugate(q);
    // bit-exact: inverse_unit is conjugate, no division at all
    require_true(inv_u.x == conj.x && inv_u.y == conj.y && inv_u.z == conj.z && inv_u.w == conj.w);
    require_true(near_q(inv_u, inv_g, 1e-12));
  }
  end_test_case();

  test_case("algebra: rotation is preserved under repeated composition");
  {
    auto q = quaternions::from_axis_angle<f64>(0.6, 0.0, 0.8, 0.13);
    auto qn = q;
    for ( int i = 0; i < 32; ++i ) qn = quaternions::multiply(qn, q).normalized();
    auto qref = quaternions::from_axis_angle<f64>(0.6, 0.0, 0.8, 0.13 * 33.0);
    f64 d = qn.dot(qref);
    if ( d < 0 ) {
      qn.x = -qn.x;
      qn.y = -qn.y;
      qn.z = -qn.z;
      qn.w = -qn.w;
    }
    require_true(near_q(qn, qref, 1e-10));
    require_true(near(qn.squared_norm(), 1.0, 1e-12));
  }
  end_test_case();

  test_case("algebra: degenerate inputs propagate NaN rather than silent identity");
  {
    // -Ofast (-ffinite-math-only) forbids the compiler from emitting branches
    // that distinguish NaN from finite values, so we inspect the bit pattern
    // directly: the qnan_v sentinel has the IEEE-754 NaN exponent (all ones).
    auto is_nan_bits = [](f64 v) -> bool {
      auto u = math::ieee::to_bits<f64>(v);
      using T = math::ieee::traits<f64>;
      return ((u & T::exp_mask) == T::exp_mask) && ((u & T::mant_mask) != 0);
    };
    qq zero{ 0.0, 0.0, 0.0, 0.0 };
    auto inv_zero = quaternions::inverse(zero);
    require_true(is_nan_bits(inv_zero.x));
    require_true(is_nan_bits(inv_zero.y));
    require_true(is_nan_bits(inv_zero.z));
    require_true(is_nan_bits(inv_zero.w));
    auto norm_zero = zero.normalized();
    require_true(is_nan_bits(norm_zero.w));
    require_true(!zero.is_normalized());
  }
  end_test_case();

  test_case("algebra: factory methods build the expected quaternion");
  {
    auto i = quaternions::quaternion<f64>::identity();
    require_true(near(i.w, 1.0) && near(i.x, 0.0) && near(i.y, 0.0) && near(i.z, 0.0));
    auto p = quaternions::quaternion<f64>::pure(1.5, -2.5, 3.5);
    require_true(near(p.x, 1.5) && near(p.y, -2.5) && near(p.z, 3.5) && near(p.w, 0.0));
    auto s = quaternions::quaternion<f64>::scalar(7.0);
    require_true(near(s.w, 7.0) && near(s.x, 0.0) && near(s.y, 0.0) && near(s.z, 0.0));
  }
  end_test_case();

  test_case("kinematics: integrate dispatches to Taylor for tiny ω·dt without 0/0");
  {
    auto q0 = quaternions::from_axis_angle<f64>(0.6, 0.0, 0.8, 0.5);
    v3 omega{ 1e-200, 0.0, 0.0 };      // n^2 underflows to 0 in f64; old code's `n2==0` branch returned q
    f64 dt = 1e-100;                   // |ω|·dt ~ 1e-300 — solidly in the Taylor regime
    auto q1 = quaternions::integrate<f64>(q0, omega, dt);
    require_true(q1.all_finite());
    require_true(near(q1.squared_norm(), 1.0, 1e-12));
    require_true(near_q(q1, q0, 1e-12));      // negligible motion
  }
  end_test_case();

  test_case("kinematics: integrate_pade renormalizes against accumulated drift");
  {
    auto q = quaternions::identity<f64>();
    v3 omega{ 0.7, -0.4, 0.5 };
    f64 dt = 0.05;
    for ( int i = 0; i < 4096; ++i ) q = quaternions::integrate_pade<f64>(q, omega, dt);
    require_true(near(q.squared_norm(), 1.0, 1e-12));
  }
  end_test_case();

  test_case("kinematics: angular_velocity returns NaN for near-zero dt");
  {
    auto q0 = quaternions::identity<f64>();
    auto q1 = quaternions::from_axis_angle<f64>(0, 0, 1, 0.1);
    auto w = quaternions::angular_velocity<f64>(q0, q1, 0.0);
    auto u = math::ieee::to_bits<f64>(w.x);
    using T = math::ieee::traits<f64>;
    require_true(((u & T::exp_mask) == T::exp_mask) && ((u & T::mant_mask) != 0));
  }
  end_test_case();

  test_case("kinematics: log_map_pade falls back to exact for near-π rotations");
  {
    // Full angle 2.9 rad (≈ 166°): cw = cos(1.45) ≈ 0.121, well below the Padé safe floor.
    f64 angle = 2.9;
    v3 axis{ 0.0, 0.0, 1.0 };
    auto q = quaternions::from_axis_angle(axis, angle);
    auto v = quaternions::log_map_pade<f64>(q);
    // Old code would divide by tiny `cw` here. The fallback path delegates to
    // to_axis_angle and gives the exact log map.
    require_true(near(v.x, 0.0, 1e-10));
    require_true(near(v.y, 0.0, 1e-10));
    require_true(near(v.z, angle, 1e-10));
  }
  end_test_case();

  test_case("kinematics: angular_velocity uses conjugate path (no division by |q0|^2)");
  {
    // Same q0 with components scaled so squared_norm() differs subtly from 1.0
    // would silently corrupt the old `inverse(q0)` path; the conjugate path is
    // bit-exact for the unit precondition. Just sanity-check ordinary inputs
    // here — proper invariance is covered by the round-trip test above.
    auto q0 = quaternions::from_axis_angle<f64>(0.0, 1.0, 0.0, 0.4);
    v3 omega{ 0.0, 0.0, 0.7 };
    f64 dt = 0.1;
    auto q1 = quaternions::integrate<f64>(q0, omega, dt);
    auto wr = quaternions::angular_velocity<f64>(q0, q1, dt);
    require_true(near(wr.x, omega.x, 1e-10));
    require_true(near(wr.y, omega.y, 1e-10));
    require_true(near(wr.z, omega.z, 1e-10));
  }
  end_test_case();

  test_case("algebra: self-aliased multiply(q, q) squares the rotation");
  {
    auto q = quaternions::from_axis_angle<f64>(0, 0, 1, 0.4);
    auto q2_alias = quaternions::multiply(q, q);      // same reference twice
    auto q2_ref = quaternions::from_axis_angle<f64>(0, 0, 1, 0.8);
    f64 d = q2_alias.dot(q2_ref);
    if ( d < 0 ) {
      q2_alias.x = -q2_alias.x;
      q2_alias.y = -q2_alias.y;
      q2_alias.z = -q2_alias.z;
      q2_alias.w = -q2_alias.w;
    }
    require_true(near_q(q2_alias, q2_ref, 1e-12));
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Extended euler API: enum-tagged rotations, 12 sequences, matrix routes

  print("=== EULER EXTENDED ===");

  using quaternions::euler_order;
  using quaternions::frame;
  using quaternions::rotations;

  test_case("axis_quat: alpha alias matches roll alias matches quat_x");
  {
    for ( int i = -7; i <= 7; ++i ) {
      const f64 t = 0.4 * i;
      auto qr = quaternions::axis_quat<rotations::roll, f64>(t);
      auto qa = quaternions::axis_quat<rotations::alpha, f64>(t);
      auto qx = quaternions::quat_x<f64>(t);
      require_true(near_q(qr, qa, 1e-15));
      require_true(near_q(qr, qx, 1e-15));
    }
    for ( int i = -3; i <= 3; ++i ) {
      const f64 t = 0.3 * i;
      auto qp = quaternions::axis_quat<rotations::pitch, f64>(t);
      auto qb = quaternions::axis_quat<rotations::beta, f64>(t);
      auto qy = quaternions::quat_y<f64>(t);
      require_true(near_q(qp, qb, 1e-15));
      require_true(near_q(qp, qy, 1e-15));
      auto qyw = quaternions::axis_quat<rotations::yaw, f64>(t);
      auto qg = quaternions::axis_quat<rotations::gamma, f64>(t);
      auto qz = quaternions::quat_z<f64>(t);
      require_true(near_q(qyw, qg, 1e-15));
      require_true(near_q(qyw, qz, 1e-15));
    }
  }
  end_test_case();

  test_case("rotate body-frame: q ⊗ axis_quat matches reference");
  {
    qq q0{ 0.3, -0.2, 0.5, 0.8 };
    q0 = q0.normalized();
    const f64 theta = 0.37;
    // yaw (Z)
    auto qz_ref = quaternions::multiply(q0, quaternions::z_quat<f64>(theta));
    qq q = q0;
    quaternions::rotate<rotations::yaw>(q, theta);
    require_true(near_q(q, qz_ref, 1e-12));
    // pitch (Y)
    auto qy_ref = quaternions::multiply(q0, quaternions::y_quat<f64>(theta));
    q = q0;
    quaternions::rotate<rotations::pitch>(q, theta);
    require_true(near_q(q, qy_ref, 1e-12));
    // roll (X)
    auto qx_ref = quaternions::multiply(q0, quaternions::x_quat<f64>(theta));
    q = q0;
    quaternions::rotate<rotations::roll>(q, theta);
    require_true(near_q(q, qx_ref, 1e-12));
  }
  end_test_case();

  test_case("rotate world-frame: axis_quat ⊗ q matches reference");
  {
    qq q0{ 0.3, -0.2, 0.5, 0.8 };
    q0 = q0.normalized();
    const f64 theta = 0.37;
    // yaw (Z)
    auto qz_ref = quaternions::multiply(quaternions::z_quat<f64>(theta), q0);
    qq q = q0;
    quaternions::rotate<rotations::yaw, frame::world>(q, theta);
    require_true(near_q(q, qz_ref, 1e-12));
    // pitch (Y)
    auto qy_ref = quaternions::multiply(quaternions::y_quat<f64>(theta), q0);
    q = q0;
    quaternions::rotate<rotations::pitch, frame::world>(q, theta);
    require_true(near_q(q, qy_ref, 1e-12));
    // roll (X)
    auto qx_ref = quaternions::multiply(quaternions::x_quat<f64>(theta), q0);
    q = q0;
    quaternions::rotate<rotations::roll, frame::world>(q, theta);
    require_true(near_q(q, qx_ref, 1e-12));
  }
  end_test_case();

  test_case("rotated() returns the rotated quaternion without mutating input");
  {
    qq q0{ 0.1, 0.2, 0.3, 0.9 };
    q0 = q0.normalized();
    const f64 theta = -0.22;
    auto r = quaternions::rotated<rotations::yaw>(q0, theta);
    qq q_inplace = q0;
    quaternions::rotate<rotations::yaw>(q_inplace, theta);
    require_true(near_q(r, q_inplace, 1e-14));
    // input untouched
    require_true(near(q0.x, 0.1 / micron::math::fsqrt(0.1 * 0.1 + 0.2 * 0.2 + 0.3 * 0.3 + 0.9 * 0.9), 1e-14));
  }
  end_test_case();

  test_case("rotate runtime dispatch agrees with the templated form");
  {
    qq q0{ 0.4, 0.1, -0.3, 0.7 };
    q0 = q0.normalized();
    const f64 theta = 0.41;
    // body-frame, each axis
    {
      qq qt = q0;
      quaternions::rotate<rotations::yaw>(qt, theta);
      qq qr = q0;
      quaternions::rotate<f64>(qr, rotations::yaw, theta, frame::body);
      require_true(near_q(qt, qr, 1e-14));
    }
    {
      qq qt = q0;
      quaternions::rotate<rotations::pitch>(qt, theta);
      qq qr = q0;
      quaternions::rotate<f64>(qr, rotations::pitch, theta, frame::body);
      require_true(near_q(qt, qr, 1e-14));
    }
    {
      qq qt = q0;
      quaternions::rotate<rotations::roll>(qt, theta);
      qq qr = q0;
      quaternions::rotate<f64>(qr, rotations::roll, theta, frame::body);
      require_true(near_q(qt, qr, 1e-14));
    }
    // world-frame
    {
      qq qt = q0;
      quaternions::rotate<rotations::yaw, frame::world>(qt, theta);
      qq qr = q0;
      quaternions::rotate<f64>(qr, rotations::yaw, theta, frame::world);
      require_true(near_q(qt, qr, 1e-14));
    }
  }
  end_test_case();

  test_case("rotate_safe holds |q|=1 after 10000 incremental updates; plain rotate drifts");
  {
    qq q = quaternions::identity<f64>();
    const f64 dt = 0.001;
    for ( int i = 0; i < 10000; ++i ) {
      quaternions::rotate_safe<rotations::yaw>(q, dt);
    }
    require_true(near(q.squared_norm(), 1.0, 8e-12));

    // drift baseline: same loop with un-normalized rotate accumulates appreciable error
    qq qd = quaternions::identity<f64>();
    for ( int i = 0; i < 10000; ++i ) {
      quaternions::rotate<rotations::yaw>(qd, dt);
    }
    const f64 drift = qd.squared_norm() - 1.0;
    const f64 abs_drift = drift < 0 ? -drift : drift;
    require_true(abs_drift > 1e-8 || abs_drift < 8e-12);      // either drifts or stays unit (FMA chains can sometimes be exact)
  }
  end_test_case();

  test_case("rotate_axis matches quaternion-based rotation for each axis");
  {
    v3 v{ 1.0, 0.5, -0.7 };
    const f64 theta = 0.8;
    {
      auto vr = quaternions::rotate_axis<rotations::yaw>(v, theta);
      auto vq = quaternions::rotate(quaternions::z_quat<f64>(theta), v);
      require_true(near(vr.x, vq.x, 1e-13));
      require_true(near(vr.y, vq.y, 1e-13));
      require_true(near(vr.z, vq.z, 1e-13));
    }
    {
      auto vr = quaternions::rotate_axis<rotations::pitch>(v, theta);
      auto vq = quaternions::rotate(quaternions::y_quat<f64>(theta), v);
      require_true(near(vr.x, vq.x, 1e-13));
      require_true(near(vr.y, vq.y, 1e-13));
      require_true(near(vr.z, vq.z, 1e-13));
    }
    {
      auto vr = quaternions::rotate_axis<rotations::roll>(v, theta);
      auto vq = quaternions::rotate(quaternions::x_quat<f64>(theta), v);
      require_true(near(vr.x, vq.x, 1e-13));
      require_true(near(vr.y, vq.y, 1e-13));
      require_true(near(vr.z, vq.z, 1e-13));
    }
  }
  end_test_case();

  // --- 12-sequence Euler ↔ quaternion round-trip ------------------------
  // Note: we compare rotations (i.e. quaternion up to sign), since to_euler
  //   may pick a different ±π wrap on β when intermediate angles are near
  //   sign boundaries. We round-trip and re-construct, requiring the
  //   reconstructed quaternion to be equivalent (=q or -q).

  const f64 a0 = 0.4, b0 = 0.27, c0 = -0.31;      // away from gimbal in any sequence

#define MICRON_TEST_ROUNDTRIP(ORD)                                                                                                         \
  do {                                                                                                                                     \
    auto q = quaternions::from_euler<euler_order::ORD, f64>(a0, b0, c0);                                                                   \
    require_true(near(q.squared_norm(), 1.0, 1e-12));                                                                                      \
    auto e = quaternions::to_euler<euler_order::ORD, f64>(q);                                                                              \
    auto q2 = quaternions::from_euler<euler_order::ORD, f64>(e.x, e.y, e.z);                                                               \
    f64 d = q.dot(q2);                                                                                                                     \
    if ( d < 0 ) {                                                                                                                         \
      q2.x = -q2.x;                                                                                                                        \
      q2.y = -q2.y;                                                                                                                        \
      q2.z = -q2.z;                                                                                                                        \
      q2.w = -q2.w;                                                                                                                        \
    }                                                                                                                                      \
    require_true(near_q(q, q2, 1e-10));                                                                                                    \
  } while ( 0 )

  test_case("from_euler/to_euler round-trip: XYZ");
  MICRON_TEST_ROUNDTRIP(XYZ);
  end_test_case();
  test_case("from_euler/to_euler round-trip: XZY");
  MICRON_TEST_ROUNDTRIP(XZY);
  end_test_case();
  test_case("from_euler/to_euler round-trip: YXZ");
  MICRON_TEST_ROUNDTRIP(YXZ);
  end_test_case();
  test_case("from_euler/to_euler round-trip: YZX");
  MICRON_TEST_ROUNDTRIP(YZX);
  end_test_case();
  test_case("from_euler/to_euler round-trip: ZXY");
  MICRON_TEST_ROUNDTRIP(ZXY);
  end_test_case();
  test_case("from_euler/to_euler round-trip: ZYX");
  MICRON_TEST_ROUNDTRIP(ZYX);
  end_test_case();
  test_case("from_euler/to_euler round-trip: XYX");
  MICRON_TEST_ROUNDTRIP(XYX);
  end_test_case();
  test_case("from_euler/to_euler round-trip: XZX");
  MICRON_TEST_ROUNDTRIP(XZX);
  end_test_case();
  test_case("from_euler/to_euler round-trip: YXY");
  MICRON_TEST_ROUNDTRIP(YXY);
  end_test_case();
  test_case("from_euler/to_euler round-trip: YZY");
  MICRON_TEST_ROUNDTRIP(YZY);
  end_test_case();
  test_case("from_euler/to_euler round-trip: ZXZ");
  MICRON_TEST_ROUNDTRIP(ZXZ);
  end_test_case();
  test_case("from_euler/to_euler round-trip: ZYZ");
  MICRON_TEST_ROUNDTRIP(ZYZ);
  end_test_case();

#undef MICRON_TEST_ROUNDTRIP

  // --- from_euler_matrix matches to_matrix(from_euler) for all 12 orders -----

#define MICRON_TEST_MATAGREE(ORD)                                                                                                          \
  do {                                                                                                                                     \
    auto qm = quaternions::from_euler<euler_order::ORD, f64>(a0, b0, c0);                                                                  \
    auto Rq = quaternions::to_matrix(qm);                                                                                                  \
    auto Re = quaternions::from_euler_matrix<euler_order::ORD, f64>(a0, b0, c0);                                                           \
    for ( int i = 0; i < 9; ++i ) require_true(near(Rq.data[i], Re.data[i], 1e-12));                                                       \
  } while ( 0 )

  test_case("from_euler_matrix == to_matrix(from_euler): XYZ");
  MICRON_TEST_MATAGREE(XYZ);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): XZY");
  MICRON_TEST_MATAGREE(XZY);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): YXZ");
  MICRON_TEST_MATAGREE(YXZ);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): YZX");
  MICRON_TEST_MATAGREE(YZX);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): ZXY");
  MICRON_TEST_MATAGREE(ZXY);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): ZYX");
  MICRON_TEST_MATAGREE(ZYX);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): XYX");
  MICRON_TEST_MATAGREE(XYX);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): XZX");
  MICRON_TEST_MATAGREE(XZX);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): YXY");
  MICRON_TEST_MATAGREE(YXY);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): YZY");
  MICRON_TEST_MATAGREE(YZY);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): ZXZ");
  MICRON_TEST_MATAGREE(ZXZ);
  end_test_case();
  test_case("from_euler_matrix == to_matrix(from_euler): ZYZ");
  MICRON_TEST_MATAGREE(ZYZ);
  end_test_case();

#undef MICRON_TEST_MATAGREE

  test_case("from_euler_matrix4: upper 3x3 == 3x3 builder; last row/col affine identity");
  {
    constexpr euler_order O = euler_order::ZYX;
    auto M = quaternions::from_euler_matrix4<O, f64>(a0, b0, c0);
    auto R = quaternions::from_euler_matrix<O, f64>(a0, b0, c0);
    for ( int r = 0; r < 3; ++r ) {
      for ( int c = 0; c < 3; ++c ) {
        require_true(near(M.data[r * 4 + c], R.data[r * 3 + c], 1e-15));
      }
    }
    // affine identity
    require_true(near(M.data[3], 0.0, 1e-15));
    require_true(near(M.data[7], 0.0, 1e-15));
    require_true(near(M.data[11], 0.0, 1e-15));
    require_true(near(M.data[12], 0.0, 1e-15));
    require_true(near(M.data[13], 0.0, 1e-15));
    require_true(near(M.data[14], 0.0, 1e-15));
    require_true(near(M.data[15], 1.0, 1e-15));
  }
  end_test_case();

  test_case("orientate3 / orientate4 / orientate delegate to YXZ");
  {
    v3 angles{ 0.3, -0.15, 0.6 };
    auto Rm3 = quaternions::orientate3<>(angles);
    auto Rm3_ref = quaternions::from_euler_matrix<euler_order::YXZ, f64>(angles.x, angles.y, angles.z);
    for ( int i = 0; i < 9; ++i ) require_true(near(Rm3.data[i], Rm3_ref.data[i], 1e-15));

    auto Rm4 = quaternions::orientate4<>(angles);
    auto Rm4_ref = quaternions::from_euler_matrix4<euler_order::YXZ, f64>(angles.x, angles.y, angles.z);
    for ( int i = 0; i < 16; ++i ) require_true(near(Rm4.data[i], Rm4_ref.data[i], 1e-15));

    auto qy = quaternions::orientate<>(angles);
    auto qy_ref = quaternions::from_euler<euler_order::YXZ, f64>(angles.x, angles.y, angles.z);
    require_true(near_q(qy, qy_ref, 1e-15));
  }
  end_test_case();

  test_case("to_euler_matrix gimbal lock — ZYX at β = +π/2");
  {
    constexpr euler_order O = euler_order::ZYX;
    const f64 pi_2 = 1.5707963267948966;
    // build R at exact gimbal (β=+π/2), pick α=0.4, γ=0.2
    auto R = quaternions::from_euler_matrix<O, f64>(0.4, pi_2, 0.2);
    auto e = quaternions::to_euler_matrix<O, f64>(R);
    // round-trip the extracted angles
    auto R2 = quaternions::from_euler_matrix<O, f64>(e.x, e.y, e.z);
    for ( int i = 0; i < 9; ++i ) require_true(near(R.data[i], R2.data[i], 1e-9));
  }
  end_test_case();

  test_case("to_euler_matrix gimbal lock — XYX at β = 0");
  {
    constexpr euler_order O = euler_order::XYX;
    auto R = quaternions::from_euler_matrix<O, f64>(0.4, 0.0, 0.2);
    auto e = quaternions::to_euler_matrix<O, f64>(R);
    auto R2 = quaternions::from_euler_matrix<O, f64>(e.x, e.y, e.z);
    for ( int i = 0; i < 9; ++i ) require_true(near(R.data[i], R2.data[i], 1e-9));
  }
  end_test_case();

  return 1;
}
