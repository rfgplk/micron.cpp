// math_dynmat.cpp — Snowball tests for math::dynmat<T>

#include "../../src/math/matrix/matrices.hpp"
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
  print("=== DYNMAT TESTS ===");

  test_case("default-constructed dynmat is empty");
  {
    dynmat<f64> M;
    require_true(M.rows == 0);
    require_true(M.cols == 0);
    require_true(M.ld == 0);
    require_true(M.empty());
  }
  end_test_case();

  test_case("sized ctor: rows, cols, ld = cols by default");
  {
    dynmat<f64> M(3, 4);
    require_true(M.rows == 3);
    require_true(M.cols == 4);
    require_true(M.ld == 4);
    require_true(M.size() == 12);
  }
  end_test_case();

  test_case("fill ctor populates every slot");
  {
    dynmat<f64> M(2, 3, 7.0);
    for ( usize r = 0; r < 2; ++r )
      for ( usize c = 0; c < 3; ++c ) require_true(near(M.at(r, c), 7.0));
  }
  end_test_case();

  test_case("with_ld factory: buf sized rows*lda, ld preserved");
  {
    auto M = dynmat<f64>::with_ld(3, 4, /*lda*/ 8);
    require_true(M.rows == 3);
    require_true(M.cols == 4);
    require_true(M.ld == 8);
    require_true(M.size() == 24);     // rows * lda
    // at(r,c) walks ld; only the first 4 cols are logical
    M.at(0, 0) = 1.0;
    M.at(1, 0) = 2.0;
    M.at(2, 0) = 3.0;
    require_true(near(M[0 * 8 + 0], 1.0));
    require_true(near(M[1 * 8 + 0], 2.0));
    require_true(near(M[2 * 8 + 0], 3.0));
  }
  end_test_case();

  test_case("zero / filled / identity factories");
  {
    auto Z = dynmat<f64>::zero(3, 3);
    for ( usize i = 0; i < 9; ++i ) require_true(near(Z[i], 0.0));

    auto F = dynmat<f64>::filled(2, 2, -1.5);
    for ( usize i = 0; i < 4; ++i ) require_true(near(F[i], -1.5));

    auto I = dynmat<f64>::identity(4);
    for ( usize r = 0; r < 4; ++r )
      for ( usize c = 0; c < 4; ++c ) require_true(near(I.at(r, c), r == c ? 1.0 : 0.0));
  }
  end_test_case();

  test_case("from(mat<T,R,C>) copies a fixed-size matrix");
  {
    mat<f64, 2, 3> fixed{};
    fixed.data[0] = 1;
    fixed.data[1] = 2;
    fixed.data[2] = 3;
    fixed.data[3] = 4;
    fixed.data[4] = 5;
    fixed.data[5] = 6;
    auto D = dynmat<f64>::from(fixed);
    require_true(D.rows == 2);
    require_true(D.cols == 3);
    require_true(D.ld == 3);
    for ( usize i = 0; i < 6; ++i ) require_true(near(D[i], fixed.data[i]));
  }
  end_test_case();

  test_case("at(r,c) is row-major and honours ld");
  {
    dynmat<f64> M(2, 3);
    M.at(0, 0) = 1;
    M.at(0, 1) = 2;
    M.at(0, 2) = 3;
    M.at(1, 0) = 4;
    M.at(1, 1) = 5;
    M.at(1, 2) = 6;
    f64 expected[6] = { 1, 2, 3, 4, 5, 6 };
    for ( usize i = 0; i < 6; ++i ) require_true(near(M[i], expected[i]));
  }
  end_test_case();

  test_case("copy and move preserve contents");
  {
    dynmat<f64> A = dynmat<f64>::identity(3);
    dynmat<f64> B = A;
    require_true(B.rows == 3 && B.cols == 3);
    require_true(near(B.at(1, 1), 1.0));
    A.at(0, 0) = 42.0;     // copy must be independent
    require_true(near(B.at(0, 0), 1.0));

    dynmat<f64> C = micron::move(B);
    require_true(C.rows == 3);
    require_true(near(C.at(2, 2), 1.0));
  }
  end_test_case();

  test_case("as_row_view exposes a row_view aliasing the dynmat");
  {
    dynmat<f64> M(2, 3);
    for ( usize i = 0; i < 6; ++i ) M[i] = f64(i + 1);
    auto V = as_row_view(M);
    require_true(V.rows == 2);
    require_true(V.cols == 3);
    require_true(V.ld == 3);
    require_true(V.data == M.data());
    // mutate through the view; observe through the dynmat
    V.at(0, 0) = -42.0;
    require_true(near(M.at(0, 0), -42.0));
  }
  end_test_case();

  test_case("submat_view aliases a sub-rectangle and walks parent ld");
  {
    // 4x4 with sequential entries; carve out the bottom-right 2x2
    dynmat<f64> M(4, 4);
    for ( usize r = 0; r < 4; ++r )
      for ( usize c = 0; c < 4; ++c ) M.at(r, c) = f64(r * 4 + c);
    auto S = submat_view(M, 2, 2, 2, 2);
    require_true(S.rows == 2);
    require_true(S.cols == 2);
    require_true(S.ld == 4);     // parent ld preserved
    require_true(near(S.at(0, 0), M.at(2, 2)));
    require_true(near(S.at(0, 1), M.at(2, 3)));
    require_true(near(S.at(1, 0), M.at(3, 2)));
    require_true(near(S.at(1, 1), M.at(3, 3)));
    // mutate through sub-view; parent observes
    S.at(0, 0) = 100.0;
    require_true(near(M.at(2, 2), 100.0));
  }
  end_test_case();

  return 1;
}
