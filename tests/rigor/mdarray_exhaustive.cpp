// mdarray_exhaustive.cpp
// Exhaustive per-member-function tests for micron::mdarray<T, Rank>.
// Rank-N dense array with row-major layout, SIMD-dispatched arithmetic.

#include "../../src/array/mdarray.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

int
main()
{
  print("=== MDARRAY EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default empty");
  {
    micron::mdarray<int, 2> a;
    require(a.size(), usize(0));
  }
  end_test_case();

  test_case("ctor: rank-1 (Dim)");
  {
    micron::mdarray<int, 1> a(8);
    require(a.size(), usize(8));
    require(a.shape(0), usize(8));
  }
  end_test_case();

  test_case("ctor: rank-2 (D1, D2)");
  {
    micron::mdarray<int, 2> a(3, 4);
    require(a.size(), usize(12));
    require(a.shape(0), usize(3));
    require(a.shape(1), usize(4));
  }
  end_test_case();

  test_case("ctor: rank-3 (D1, D2, D3)");
  {
    micron::mdarray<int, 3> a(2, 3, 4);
    require(a.size(), usize(24));
    require(a.shape(0), usize(2));
    require(a.shape(1), usize(3));
    require(a.shape(2), usize(4));
  }
  end_test_case();

  test_case("ctor: rank-4");
  {
    micron::mdarray<int, 4> a(2, 2, 2, 2);
    require(a.size(), usize(16));
  }
  end_test_case();

  test_case("ctor: zero dimension throws");
  {
    bool threw = false;
    try {
      micron::mdarray<int, 2> a(3, 0);
      (void)a;
    } catch ( ... ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  test_case("ctor: copy");
  {
    micron::mdarray<int, 2> a(3, 4);
    a.fill(7);
    micron::mdarray<int, 2> b(a);
    require(b.size(), usize(12));
    require(b(0, 0), 7);
    require(b(2, 3), 7);
  }
  end_test_case();

  test_case("ctor: move");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(5);
    micron::mdarray<int, 2> b(micron::move(a));
    require(b.size(), usize(6));
    require(b(1, 2), 5);
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                   //
  // ============================================================ //
  test_case("op=(const mdarray&): copy");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(9);
    micron::mdarray<int, 2> b;
    b = a;
    require(b.size(), usize(6));
    require(b(0, 0), 9);
  }
  end_test_case();

  test_case("op=(mdarray&&): move");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(11);
    micron::mdarray<int, 2> b;
    b = micron::move(a);
    require(b.size(), usize(6));
    require(b(1, 2), 11);
  }
  end_test_case();

  test_case("op=(const mdarray&): self-assign safe");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(3);
    a = a;
    require(a(1, 1), 3);
  }
  end_test_case();

  // ============================================================ //
  //  SHAPE/STRIDE                                                 //
  // ============================================================ //
  test_case("shape/stride: row-major layout");
  {
    micron::mdarray<int, 3> a(2, 3, 4);
    require(a.shape(0), usize(2));
    require(a.shape(1), usize(3));
    require(a.shape(2), usize(4));
    require(a.stride(0), usize(12));      // 3*4
    require(a.stride(1), usize(4));       // 4
    require(a.stride(2), usize(1));       // 1
  }
  end_test_case();

  test_case("rank: compile-time constant");
  {
    static_assert(micron::mdarray<int, 3>::rank == 3);
    require_true(micron::mdarray<int, 3>::rank == 3);
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                               //
  // ============================================================ //
  test_case("op()(Idx...): rank-2 indexing");
  {
    micron::mdarray<int, 2> a(3, 4);
    a.fill(0);
    a(1, 2) = 99;
    require(a(1, 2), 99);
    require(a(0, 0), 0);
  }
  end_test_case();

  test_case("op()(Idx...): rank-3 indexing");
  {
    micron::mdarray<int, 3> a(2, 3, 4);
    a.fill(0);
    a(1, 2, 3) = 42;
    require(a(1, 2, 3), 42);
  }
  end_test_case();

  test_case("op()(Idx...) const: read-only");
  {
    micron::mdarray<int, 2> a(2, 2);
    a.fill(7);
    const auto &ca = a;
    require(ca(0, 0), 7);
    require(ca(1, 1), 7);
  }
  end_test_case();

  test_case("at(flat_idx): bounds-checked");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(11);
    require(a.at(0), 11);
    require(a.at(5), 11);
    require_throw([&a]() { (void)a.at(6); });
    require_throw([&a]() { (void)a.at(999); });
  }
  end_test_case();

  test_case("data(): mutable/const");
  {
    micron::mdarray<int, 2> a(2, 2);
    a.fill(3);
    int *p = a.data();
    require(p[0], 3);
    const auto &ca = a;
    const int *cp = ca.data();
    require(cp[3], 3);
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION                                                    //
  // ============================================================ //
  test_case("begin/end: flat iteration");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(5);
    int sum = 0;
    for ( auto x : a ) sum += x;
    require(sum, 30);
  }
  end_test_case();

  test_case("cbegin/cend");
  {
    micron::mdarray<int, 2> a(2, 2);
    a.fill(2);
    int sum = 0;
    for ( auto it = a.cbegin(); it != a.cend(); ++it ) sum += *it;
    require(sum, 8);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                     //
  // ============================================================ //
  test_case("max_size: same as size");
  {
    micron::mdarray<int, 2> a(3, 4);
    require(a.max_size(), usize(12));
  }
  end_test_case();

  // ============================================================ //
  //  MODIFIERS                                                    //
  // ============================================================ //
  test_case("fill: broadcasts value (SIMD path for arithmetic T)");
  {
    micron::mdarray<f32, 2> a(8, 8);
    a.fill(1.5f);
    require_true(a(0, 0) == 1.5f);
    require_true(a(7, 7) == 1.5f);
  }
  end_test_case();

  test_case("clear: zero-fills");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(7);
    a.clear();
    require(a(0, 0), 0);
    require(a(1, 2), 0);
  }
  end_test_case();

  test_case("swap: exchanges shape/data");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(1);
    micron::mdarray<int, 2> b(4, 5);
    b.fill(2);
    a.swap(b);
    require(a.size(), usize(20));
    require(b.size(), usize(6));
    require(a(0, 0), 2);
    require(b(0, 0), 1);
  }
  end_test_case();

  // ============================================================ //
  //  ARITHMETIC                                                   //
  // ============================================================ //
  test_case("op+=: elementwise add");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(10);
    micron::mdarray<int, 2> b(2, 3);
    b.fill(5);
    a += b;
    require(a(0, 0), 15);
    require(a(1, 2), 15);
  }
  end_test_case();

  test_case("op-=: elementwise sub");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(100);
    micron::mdarray<int, 2> b(2, 3);
    b.fill(40);
    a -= b;
    require(a(0, 0), 60);
  }
  end_test_case();

  test_case("op*=(scalar): broadcast multiply");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(3);
    a *= 4;
    require(a(0, 0), 12);
    require(a(1, 2), 12);
  }
  end_test_case();

  test_case("sum: fold over flat");
  {
    micron::mdarray<int, 2> a(2, 3);
    a.fill(2);
    require(a.sum(), 12);
  }
  end_test_case();

  print("=== ALL MDARRAY EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
