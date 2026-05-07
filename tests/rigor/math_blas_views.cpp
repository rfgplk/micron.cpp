// math_blas_views.cpp — Snowball tests for BLAS view types

#include "../../src/math/blas/blas.hpp"
#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"
#include "../../src/array/array.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

using namespace micron;
using namespace micron::math;

static bool
near(f64 a, f64 b, f64 eps = 1e-10)
{
  f64 d = a - b;
  return (d < 0 ? -d : d) < eps;
}

int
main()
{
  print("=== BLAS VIEWS TESTS ===");

  test_case("vec_view: round-trip from raw_slice / vector / array / pointer");
  {
    f64 raw[4] = { 1, 2, 3, 4 };

    // pointer + count
    auto v1 = quants::vec_view<f64>::from(raw, 4);
    require_true(v1.n == 4);
    require_true(v1.inc == 1);
    require_true(v1.data == raw);

    // raw_slice
    raw_slice<f64> s{ raw, 4 };
    auto v2 = quants::vec_view<f64>::from(s);
    require_true(v2.data == raw && v2.n == 4);

    // micron::vector
    micron::vector<f64> mv(4, 0.0);
    mv[0] = 10; mv[1] = 20; mv[2] = 30; mv[3] = 40;
    auto v3 = quants::vec_view<f64>::from(mv);
    require_true(v3.n == 4);
    require_true(near(v3[0], 10.0));
    require_true(near(v3[3], 40.0));

    // micron::array
    micron::array<f64, 4> ma{};
    ma[0] = 5; ma[1] = 6; ma[2] = 7; ma[3] = 8;
    auto v4 = quants::vec_view<f64>::from(ma);
    require_true(v4.n == 4);
    require_true(near(v4[2], 7.0));
  }
  end_test_case();

  test_case("vec_view: strided iteration");
  {
    f64 raw[8] = { 1, 9, 2, 9, 3, 9, 4, 9 };
    auto v = quants::vec_view<f64>::strided(raw, 4, 2);
    require_true(near(v[0], 1.0));
    require_true(near(v[1], 2.0));
    require_true(near(v[2], 3.0));
    require_true(near(v[3], 4.0));
  }
  end_test_case();

  test_case("row_view + col_view: at(r,c) honours layout");
  {
    f64 row_buf[6] = { 1, 2, 3, 4, 5, 6 };       // 2x3 row-major
    f64 col_buf[6] = { 1, 4, 2, 5, 3, 6 };       // same matrix, col-major

    auto rv = matrix::row_view<f64>::from(row_buf, 2, 3);
    auto cv = matrix::col_view<f64>::from(col_buf, 2, 3);

    for ( usize i = 0; i < 2; ++i )
      for ( usize j = 0; j < 3; ++j ) require_true(near(rv.at(i, j), cv.at(i, j)));
  }
  end_test_case();

  test_case("transpose_view: zero-copy rebind");
  {
    f64 buf[6] = { 1, 2, 3, 4, 5, 6 };           // 2x3 row-major
    auto rv = matrix::row_view<f64>::from(buf, 2, 3);
    auto tv = matrix::transpose_view(rv);        // now 3x2 col-major over same data

    require_true(tv.rows == 3);
    require_true(tv.cols == 2);
    // tv.at(r, c) reads buf[c*ld + r] which in this case (ld=3) is the same memory
    // arranged so tv represents Aᵀ relative to rv.
    // rv.at(0,0)=1 → tv.at(0,0)=1
    // rv.at(0,1)=2 → tv.at(1,0)=2
    require_true(near(tv.at(0, 0), 1.0));
    require_true(near(tv.at(1, 0), 2.0));
    require_true(near(tv.at(2, 0), 3.0));
    require_true(near(tv.at(0, 1), 4.0));
    require_true(near(tv.at(1, 1), 5.0));
    require_true(near(tv.at(2, 1), 6.0));
  }
  end_test_case();

  test_case("row_view::submat: ld preserved");
  {
    // 4x4 row-major
    f64 buf[16] = {
       1,  2,  3,  4,
       5,  6,  7,  8,
       9, 10, 11, 12,
      13, 14, 15, 16
    };
    auto v = matrix::row_view<f64>::from(buf, 4, 4);
    auto sub = v.submat(1, 1, 2, 2);          // [[6,7];[10,11]]
    require_true(sub.rows == 2);
    require_true(sub.cols == 2);
    require_true(sub.ld == 4);
    require_true(near(sub.at(0, 0), 6.0));
    require_true(near(sub.at(0, 1), 7.0));
    require_true(near(sub.at(1, 0), 10.0));
    require_true(near(sub.at(1, 1), 11.0));
  }
  end_test_case();

  test_case("row_view from int_matrix_base + gemv");
  {
    // 4x4 int matrix from matrix/int4x4.hpp.
    // We use f64 specialisation of int_matrix_base which the template
    // permits since B is is_arithmetic_v.
    int_matrix_base<f64, 4, 4> A{};
    // fill with a deterministic pattern: A(i,j) = i*4 + j + 1
    for ( u32 i = 0; i < 4; ++i )
      for ( u32 j = 0; j < 4; ++j ) A.data[i * 4 + j] = f64(i * 4 + j + 1);

    f64 x[4] = { 1, 1, 1, 1 };
    f64 y[4] = { 0, 0, 0, 0 };
    auto Av = matrix::row_view<f64>::from(A);
    auto xv = quants::vec_view<f64>::from(x, 4);
    auto yv = quants::vec_view<f64>::from(y, 4);
    blas::level2::gemv(f64(1.0), Av, xv, f64(0.0), yv);
    // row sums: 1+2+3+4=10, 5+6+7+8=26, 9+10+11+12=42, 13+14+15+16=58
    require_true(near(y[0], 10.0));
    require_true(near(y[1], 26.0));
    require_true(near(y[2], 42.0));
    require_true(near(y[3], 58.0));
  }
  end_test_case();

  print("=== blas views ok ===");
  return 1;
}
