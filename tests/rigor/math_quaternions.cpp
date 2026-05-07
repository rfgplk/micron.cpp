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
    v3 axis{ 0.6, 0.0, 0.8 };     // unit axis
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
    auto qz = quaternions::from_axis_angle<f64>(0, 0, 1, 1.5707963267948966);     // 90° z
    auto qx = quaternions::from_axis_angle<f64>(1, 0, 0, 1.5707963267948966);     // 90° x
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
    auto q1 = quaternions::from_axis_angle<f64>(0, 0, 1, 1.0);     // 1 rad about z
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
    f64 dt = 0.01;     // |ω·dt| ≈ 6e-3 rad — well inside Taylor regime
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
    f64 dt = 0.5;     // |ω·dt| ≈ 0.57 rad — Padé[2/2] still very accurate
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
    v3 axis{ 0.6, 0.0, 0.8 };     // unit
    f64 angle = 0.7;              // ~ 40°, well inside Padé safe region
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

  return 1;
}
