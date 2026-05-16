// geom_transform.cpp
// Rigorous snowball test suite for transform<F, Dim, Mode>.

#include "../../src/math/geometry.hpp"
#include "../../src/math/sqrt.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace m = micron::math;
namespace mg = micron::math::geometry;

template<typename F>
static bool
approx(F a, F b, F tol) noexcept
{
  F d = a - b;
  if ( d < F(0) ) d = -d;
  return d <= tol;
}

template<typename F, usize Dim>
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
  sb::print("=== GEOM_TRANSFORM TESTS ===");

  // ------------------------------------------------------------------
  test_case("identity transform leaves points unchanged");
  {
    using F = double;
    auto T = mg::transform<F, 3, mg::transform_mode::affine>::identity();
    m::vec<F, 3> p{ 1.0, 2.0, 3.0 };
    auto q = T.apply(p);
    require_true(vec_close(q, p, F(1e-15)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("translation+scaling compose");
  {
    using F = double;
    auto T = mg::transform<F, 3, mg::transform_mode::affine>::from_translation(mg::translation<F, 3>{ { 1.0, 2.0, 3.0 } });
    auto S = mg::transform<F, 3, mg::transform_mode::affine>::from_scaling(mg::scaling<F, 3>{ { 2.0, 3.0, 4.0 } });

    // (T * S) applied to p: first scale, then translate
    auto TS = T * S;
    m::vec<F, 3> p{ 1.0, 1.0, 1.0 };
    auto q = TS.apply(p);
    // scale → (2, 3, 4), then translate → (3, 5, 7)
    require_true(approx(q.data[0], 3.0, 1e-14));
    require_true(approx(q.data[1], 5.0, 1e-14));
    require_true(approx(q.data[2], 7.0, 1e-14));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("inverse: T * T.inverse() = identity (affine)");
  {
    using F = double;
    auto T = mg::transform<F, 3, mg::transform_mode::affine>::from_translation(mg::translation<F, 3>{ { 4.0, -2.0, 1.5 } });
    auto S = mg::transform<F, 3, mg::transform_mode::affine>::from_scaling(mg::scaling<F, 3>{ { 2.0, 3.0, 0.5 } });
    auto M = T * S;
    auto Minv = M.inverse();
    auto I = M * Minv;
    auto p = m::vec<F, 3>{ 7.0, -3.0, 11.0 };
    auto q = I.apply(p);
    require_true(vec_close(q, p, F(1e-12)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("isometry inverse: analytic R^T form");
  {
    using F = double;
    // Build isometry from a 90-degree rotation around z and a translation
    mg::transform<F, 3, mg::transform_mode::isometry> R{};
    R.M = m::mat<F, 4, 4>::zero();
    R.M.data[0 * 4 + 0] = 0.0;
    R.M.data[0 * 4 + 1] = -1.0;
    R.M.data[0 * 4 + 2] = 0.0;
    R.M.data[0 * 4 + 3] = 5.0;
    R.M.data[1 * 4 + 0] = 1.0;
    R.M.data[1 * 4 + 1] = 0.0;
    R.M.data[1 * 4 + 2] = 0.0;
    R.M.data[1 * 4 + 3] = -2.0;
    R.M.data[2 * 4 + 0] = 0.0;
    R.M.data[2 * 4 + 1] = 0.0;
    R.M.data[2 * 4 + 2] = 1.0;
    R.M.data[2 * 4 + 3] = 3.0;
    R.M.data[3 * 4 + 0] = 0.0;
    R.M.data[3 * 4 + 1] = 0.0;
    R.M.data[3 * 4 + 2] = 0.0;
    R.M.data[3 * 4 + 3] = 1.0;

    auto Rinv = R.inverse();
    auto p = m::vec<F, 3>{ 1.0, 0.0, 0.0 };
    auto q = R.apply(p);
    auto back = Rinv.apply(q);
    require_true(vec_close(back, p, F(1e-14)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("projective: orthographic projection of unit cube corners");
  {
    using F = double;
    auto P = mg::orthographic_projection<F>(-1.0, 1.0, -1.0, 1.0, 0.1, 100.0);
    // origin maps to (0, 0, normalized depth)
    auto origin = m::vec<F, 3>{ 0.0, 0.0, -10.0 };
    auto p = P.apply(origin);
    require_true(approx(p.data[0], 0.0, 1e-14));
    require_true(approx(p.data[1], 0.0, 1e-14));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("look_at: eye is at origin in camera frame");
  {
    using F = double;
    auto eye = m::vec<F, 3>{ 5.0, 5.0, 5.0 };
    auto target = m::vec<F, 3>{ 0.0, 0.0, 0.0 };
    auto up = m::vec<F, 3>{ 0.0, 1.0, 0.0 };
    auto L = mg::look_at<F>(eye, target, up);
    auto eye_cam = L.apply(eye);
    require_true(vec_close(eye_cam, m::vec<F, 3>{ 0.0, 0.0, 0.0 }, F(1e-13)));
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("from_quat / inverse round-trip");
  {
    using F = double;
    // quat representing 90deg rot around z: (x,y,z,w) = (0, 0, sin(pi/4), cos(pi/4))
    m::quat<F> q{};
    q.data[0] = 0.0;
    q.data[1] = 0.0;
    q.data[2] = micron::math::fsqrt(0.5);
    q.data[3] = micron::math::fsqrt(0.5);
    auto T = mg::transform<F, 3, mg::transform_mode::isometry>::from_quat(q);
    auto Tinv = T.inverse();
    auto p = m::vec<F, 3>{ 1.0, 2.0, 3.0 };
    auto q_p = T.apply(p);
    auto back = Tinv.apply(q_p);
    require_true(vec_close(back, p, F(1e-13)));
  }
  end_test_case();

  sb::print("=== GEOM_TRANSFORM PASSED ===");
  return 1;
}
