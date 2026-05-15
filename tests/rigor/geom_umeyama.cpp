// geom_umeyama.cpp
// Rigorous snowball test suite for Procrustes alignment.

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

int
main()
{
  sb::print("=== GEOM_UMEYAMA TESTS ===");

  // ------------------------------------------------------------------
  test_case("recover rigid transform (no scaling)");
  {
    using F = double;
    // Source points (3 x 5)
    m::dynmat<F> src(3, 5);
    F src_vals[15] = {
      1.0, 2.0, 0.5, -1.0, 0.0, 0.0, 1.0, 1.5, 2.0, -0.5, 0.0, 0.0, 1.0, 0.5, 0.0,
    };
    for ( usize i = 0; i < 15; ++i ) src.data()[i] = src_vals[i];

    // Apply known transform: 90deg rot around z + translation
    // R = [0 -1 0; 1 0 0; 0 0 1]; t = (3, -2, 1)
    m::dynmat<F> dst(3, 5);
    for ( usize j = 0; j < 5; ++j ) {
      F x = src.at(0, j);
      F y = src.at(1, j);
      F z = src.at(2, j);
      dst.at(0, j) = -y + 3.0;
      dst.at(1, j) = x - 2.0;
      dst.at(2, j) = z + 1.0;
    }

    auto T = mg::umeyama<F>(src, dst, false);

    // Verify T applied to src matches dst
    for ( usize j = 0; j < 5; ++j ) {
      m::vec<F, 3> p{ src.at(0, j), src.at(1, j), src.at(2, j) };
      auto q = T.apply(p);
      require_true(approx(q.data[0], dst.at(0, j), F(1e-10)));
      require_true(approx(q.data[1], dst.at(1, j), F(1e-10)));
      require_true(approx(q.data[2], dst.at(2, j), F(1e-10)));
    }
  }
  end_test_case();

  // ------------------------------------------------------------------
  test_case("recover similarity with scaling");
  {
    using F = double;
    m::dynmat<F> src(3, 4);
    F src_vals[12] = {
      1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0,
    };
    for ( usize i = 0; i < 12; ++i ) src.data()[i] = src_vals[i];

    // Apply: scale by 2, no rotation, translate by (5, 0, 0)
    m::dynmat<F> dst(3, 4);
    for ( usize j = 0; j < 4; ++j ) {
      dst.at(0, j) = 2.0 * src.at(0, j) + 5.0;
      dst.at(1, j) = 2.0 * src.at(1, j);
      dst.at(2, j) = 2.0 * src.at(2, j);
    }

    auto T = mg::umeyama<F>(src, dst, true);
    for ( usize j = 0; j < 4; ++j ) {
      m::vec<F, 3> p{ src.at(0, j), src.at(1, j), src.at(2, j) };
      auto q = T.apply(p);
      require_true(approx(q.data[0], dst.at(0, j), F(1e-10)));
      require_true(approx(q.data[1], dst.at(1, j), F(1e-10)));
      require_true(approx(q.data[2], dst.at(2, j), F(1e-10)));
    }
  }
  end_test_case();

  sb::print("=== GEOM_UMEYAMA PASSED ===");
  return 0;
}
