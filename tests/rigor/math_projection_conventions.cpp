// math_projection_conventions.cpp

#include "../../src/math/geometry/projection.hpp"
#include "../../src/math/sqrt.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
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

template<typename F>
static bool
is_identity4(const mg::transform<F, 3, mg::transform_mode::projective> &t, F tol) noexcept
{
  for ( int i = 0; i < 4; ++i )
    for ( int j = 0; j < 4; ++j ) {
      const F expect = (i == j) ? F(1) : F(0);
      if ( !approx(t.M.data[i * 4 + j], expect, tol) ) return false;
    }
  return true;
}

int
main()
{
  print("=== PROJECTION CONVENTIONS ===");

  using F = double;
  const F fovy = 1.0, aspect = 1.6, n = 0.5, f = 100.0;

  test_case("perspective: default == explicit right/neg_one_to_one (back-compat)");
  {
    auto a = mg::perspective_projection(fovy, aspect, n, f);
    auto b = mg::perspective_projection<mg::handedness::right, mg::clip_depth::neg_one_to_one>(fovy, aspect, n, f);
    for ( int i = 0; i < 16; ++i ) require_true(approx(a.M.data[i], b.M.data[i], F(0)));
  }
  end_test_case();

  test_case("perspective RH neg_one_to_one: near -> -1, far -> +1");
  {
    auto p = mg::perspective_projection<mg::handedness::right, mg::clip_depth::neg_one_to_one>(fovy, aspect, n, f);
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, -n }).data[2], F(-1), 1e-9));
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, -f }).data[2], F(1), 1e-9));
  }
  end_test_case();

  test_case("perspective RH zero_to_one: near -> 0, far -> +1");
  {
    auto p = mg::perspective_projection<mg::handedness::right, mg::clip_depth::zero_to_one>(fovy, aspect, n, f);
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, -n }).data[2], F(0), 1e-9));
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, -f }).data[2], F(1), 1e-9));
  }
  end_test_case();

  test_case("perspective LH neg_one_to_one: forward +Z, near -> -1, far -> +1");
  {
    auto p = mg::perspective_projection<mg::handedness::left, mg::clip_depth::neg_one_to_one>(fovy, aspect, n, f);
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, n }).data[2], F(-1), 1e-9));
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, f }).data[2], F(1), 1e-9));
  }
  end_test_case();

  test_case("perspective LH zero_to_one: forward +Z, near -> 0, far -> +1");
  {
    auto p = mg::perspective_projection<mg::handedness::left, mg::clip_depth::zero_to_one>(fovy, aspect, n, f);
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, n }).data[2], F(0), 1e-9));
    require_true(approx(p.apply(m::vec<F, 3>{ 0, 0, f }).data[2], F(1), 1e-9));
  }
  end_test_case();

  test_case("handedness: RH sees -Z (w>0), LH sees +Z (w>0); opposite side behind (w<0)");
  {
    auto rh = mg::perspective_projection<mg::handedness::right>(fovy, aspect, n, f);
    auto lh = mg::perspective_projection<mg::handedness::left>(fovy, aspect, n, f);
    require_true(rh.apply_homogeneous(m::vec<F, 4>{ 0, 0, -1, 1 }).data[3] > F(0));
    require_true(rh.apply_homogeneous(m::vec<F, 4>{ 0, 0, 1, 1 }).data[3] < F(0));
    require_true(lh.apply_homogeneous(m::vec<F, 4>{ 0, 0, 1, 1 }).data[3] > F(0));
    require_true(lh.apply_homogeneous(m::vec<F, 4>{ 0, 0, -1, 1 }).data[3] < F(0));
  }
  end_test_case();

  test_case("orthographic RH: NO near->-1 far->+1, ZO near->0 far->+1");
  {
    auto no = mg::orthographic_projection<mg::handedness::right, mg::clip_depth::neg_one_to_one>(F(-2), F(2), F(-2), F(2), n, f);
    require_true(approx(no.apply(m::vec<F, 3>{ 0, 0, -n }).data[2], F(-1), 1e-12));
    require_true(approx(no.apply(m::vec<F, 3>{ 0, 0, -f }).data[2], F(1), 1e-12));
    auto zo = mg::orthographic_projection<mg::handedness::right, mg::clip_depth::zero_to_one>(F(-2), F(2), F(-2), F(2), n, f);
    require_true(approx(zo.apply(m::vec<F, 3>{ 0, 0, -n }).data[2], F(0), 1e-12));
    require_true(approx(zo.apply(m::vec<F, 3>{ 0, 0, -f }).data[2], F(1), 1e-12));
  }
  end_test_case();

  test_case("frustum RH on-axis: NO near->-1 far->+1, ZO near->0 far->+1");
  {
    auto no = mg::frustum<mg::handedness::right, mg::clip_depth::neg_one_to_one>(F(-1), F(1), F(-1), F(1), n, f);
    require_true(approx(no.apply(m::vec<F, 3>{ 0, 0, -n }).data[2], F(-1), 1e-9));
    require_true(approx(no.apply(m::vec<F, 3>{ 0, 0, -f }).data[2], F(1), 1e-9));
    auto zo = mg::frustum<mg::handedness::right, mg::clip_depth::zero_to_one>(F(-1), F(1), F(-1), F(1), n, f);
    require_true(approx(zo.apply(m::vec<F, 3>{ 0, 0, -n }).data[2], F(0), 1e-9));
    require_true(approx(zo.apply(m::vec<F, 3>{ 0, 0, -f }).data[2], F(1), 1e-9));
  }
  end_test_case();

  test_case("look_at RH: canonical -Z view is identity; point maps unchanged");
  {
    auto v = mg::look_at(m::vec<F, 3>{ 0, 0, 0 }, m::vec<F, 3>{ 0, 0, -1 }, m::vec<F, 3>{ 0, 1, 0 });
    auto p = v.apply(m::vec<F, 3>{ 1, 2, -3 });
    require_true(approx(p.data[0], F(1), 1e-12));
    require_true(approx(p.data[1], F(2), 1e-12));
    require_true(approx(p.data[2], F(-3), 1e-12));
  }
  end_test_case();

  test_case("look_at LH: canonical +Z view is identity; point maps unchanged");
  {
    auto v = mg::look_at<mg::handedness::left>(m::vec<F, 3>{ 0, 0, 0 }, m::vec<F, 3>{ 0, 0, 1 }, m::vec<F, 3>{ 0, 1, 0 });
    auto p = v.apply(m::vec<F, 3>{ 1, 2, 3 });
    require_true(approx(p.data[0], F(1), 1e-12));
    require_true(approx(p.data[1], F(2), 1e-12));
    require_true(approx(p.data[2], F(3), 1e-12));
  }
  end_test_case();

  test_case("inv_perspective * perspective == I (RH/LH x NO/ZO), both orders");
  {
    auto a = mg::perspective_projection<mg::handedness::right, mg::clip_depth::neg_one_to_one>(fovy, aspect, n, f);
    auto b = mg::perspective_projection<mg::handedness::right, mg::clip_depth::zero_to_one>(fovy, aspect, n, f);
    auto c = mg::perspective_projection<mg::handedness::left, mg::clip_depth::zero_to_one>(fovy, aspect, n, f);
    require_true(is_identity4(mg::inv_perspective(a) * a, F(1e-9)));
    require_true(is_identity4(a * mg::inv_perspective(a), F(1e-9)));
    require_true(is_identity4(mg::inv_perspective(b) * b, F(1e-9)));
    require_true(is_identity4(mg::inv_perspective(c) * c, F(1e-9)));
  }
  end_test_case();

  test_case("inv_perspective inverts an off-axis frustum (P,Q != 0)");
  {
    auto fr = mg::frustum<mg::handedness::right, mg::clip_depth::zero_to_one>(F(-0.5), F(1.5), F(-1), F(2), n, f);
    require_true(is_identity4(mg::inv_perspective(fr) * fr, F(1e-9)));
    require_true(is_identity4(fr * mg::inv_perspective(fr), F(1e-9)));
  }
  end_test_case();

  test_case("inv_orthographic * orthographic == I (RH NO/ZO), both orders");
  {
    auto no = mg::orthographic_projection<mg::handedness::right, mg::clip_depth::neg_one_to_one>(F(-2), F(3), F(-1), F(4), n, f);
    auto zo = mg::orthographic_projection<mg::handedness::right, mg::clip_depth::zero_to_one>(F(-2), F(3), F(-1), F(4), n, f);
    require_true(is_identity4(mg::inv_orthographic(no) * no, F(1e-12)));
    require_true(is_identity4(no * mg::inv_orthographic(no), F(1e-12)));
    require_true(is_identity4(mg::inv_orthographic(zo) * zo, F(1e-12)));
  }
  end_test_case();

  print("=== PROJECTION CONVENTIONS PASSED ===");
  return 1;
}
