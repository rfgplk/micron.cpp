// math_dynvec.cpp — Snowball tests for math::dynvec<T>

#include "../../src/math/blas/blas.hpp"
#include "../../src/math/quants/vecs.hpp"
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

static bool
near(f64 a, f64 b, f64 eps = 1e-12)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== DYNVEC TESTS ===");

  test_case("default-constructed dynvec is empty");
  {
    dynvec<f64> v;
    require_true(v.empty());
    require_true(v.size() == 0);
  }
  end_test_case();

  test_case("sized ctor allocates n elements (value-init)");
  {
    dynvec<f64> v(8);
    require_true(v.size() == 8);
    require_true(v.data() != nullptr);
  }
  end_test_case();

  test_case("fill ctor populates every slot");
  {
    dynvec<f64> v(5, 3.25);
    require_true(v.size() == 5);
    for ( usize i = 0; i < v.size(); ++i ) require_true(near(v[i], 3.25));
  }
  end_test_case();

  test_case("zero factory produces zero-filled vector");
  {
    auto v = dynvec<f64>::zero(7);
    require_true(v.size() == 7);
    for ( usize i = 0; i < v.size(); ++i ) require_true(near(v[i], 0.0));
  }
  end_test_case();

  test_case("operator[] / at() round-trip");
  {
    dynvec<f64> v(4, 0.0);
    v[0] = 1.0;
    v.at(1) = 2.0;
    v[2] = 3.0;
    v.at(3) = 4.0;
    require_true(near(v.at(0), 1.0));
    require_true(near(v[1], 2.0));
    require_true(near(v.at(2), 3.0));
    require_true(near(v[3], 4.0));
  }
  end_test_case();

  test_case("from(vec<T,N>) copies a fixed-size vector");
  {
    vec<f64, 4> fixed{ 1.0, 2.0, 3.0, 4.0 };
    auto dv = dynvec<f64>::from(fixed);
    require_true(dv.size() == 4);
    for ( usize i = 0; i < 4; ++i ) require_true(near(dv[i], fixed.data[i]));
  }
  end_test_case();

  test_case("copy / move semantics");
  {
    dynvec<f64> a(3, 1.5);
    dynvec<f64> b = a;     // copy
    require_true(b.size() == 3);
    require_true(near(b[1], 1.5));
    a[0] = 99.0;     // mutate original; copy must be independent
    require_true(near(b[0], 1.5));

    dynvec<f64> c = micron::move(b);     // move
    require_true(c.size() == 3);
    require_true(near(c[2], 1.5));
  }
  end_test_case();

  test_case("as_view exposes a vec_view aliasing the dynvec");
  {
    dynvec<f64> v(6);
    for ( usize i = 0; i < 6; ++i ) v[i] = f64(i + 1);
    auto vw = as_view(v);
    require_true(vw.n == 6);
    require_true(vw.inc == 1);
    require_true(vw.data == v.data());
    // mutate through the view; observe through the dynvec
    vw[0] = -7.0;
    require_true(near(v[0], -7.0));
  }
  end_test_case();

  test_case("BLAS L1 dot through as_view(dynvec)");
  {
    dynvec<f64> u(5);
    dynvec<f64> v(5);
    for ( usize i = 0; i < 5; ++i ) {
      u[i] = f64(i + 1);         // 1..5
      v[i] = f64(2 * i + 1);     // 1,3,5,7,9
    }
    f64 d = blas::level1::dot(as_view(u), as_view(v));
    // 1*1 + 2*3 + 3*5 + 4*7 + 5*9 = 1+6+15+28+45 = 95
    require_true(near(d, 95.0));
  }
  end_test_case();

  test_case("BLAS L1 axpy through as_view(dynvec)");
  {
    dynvec<f64> x(4, 1.0);
    dynvec<f64> y(4, 2.0);
    auto xv = as_view(x);
    auto yv = as_view(y);
    blas::level1::axpy(f64(3.0), xv, yv);
    // y ← 3·x + y = 3·1 + 2 = 5
    for ( usize i = 0; i < 4; ++i ) require_true(near(y[i], 5.0));
  }
  end_test_case();

  return 0;
}
