// constexprarray_exhaustive.cpp
// Exhaustive per-member-function tests for micron::constexpr_array<T, N>
// AND micron::constarray<T, N> (alias). Both compile-time and runtime paths
// are exercised; constexpr correctness via static_assert.

#include "../../src/array/constarray.hpp"      // brings in constexpr_array via alias
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using ca4 = micron::constexpr_array<int, 4>;
using cn4 = micron::constarray<int, 4>;      // alias for constexpr_array

// Compile-time correctness checks
static_assert(ca4::length == 4);
static_assert(ca4::static_size == 4);
static_assert(micron::is_same_v<ca4, cn4>);      // constarray = constexpr_array

// Constexpr evaluation
constexpr ca4
__build_constexpr()
{
  ca4 a{ 1, 2, 3, 4 };
  return a;
}

constexpr auto __ca_demo = __build_constexpr();
static_assert(__ca_demo[0] == 1);
static_assert(__ca_demo[3] == 4);
static_assert(__ca_demo.size() == 4);
static_assert(__ca_demo.sum() == 10);
static_assert(__ca_demo.all(0) == false);
static_assert(__ca_demo.any(3) == true);

int
main()
{
  print("=== CONSTEXPR_ARRAY EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default");
  {
    ca4 a;
    require(a.size(), usize(4));
  }
  end_test_case();

  test_case("ctor: from value");
  {
    ca4 a(7);
    for ( int i = 0; i < 4; ++i ) require(a[i], 7);
  }
  end_test_case();

  test_case("ctor: initializer_list");
  {
    ca4 a{ 1, 2, 3, 4 };
    for ( int i = 0; i < 4; ++i ) require(a[i], i + 1);
  }
  end_test_case();

  test_case("ctor: copy");
  {
    ca4 a{ 5, 6, 7, 8 };
    ca4 b(a);
    require(b[2], 7);
  }
  end_test_case();

  test_case("ctor: move");
  {
    ca4 a{ 9, 10, 11, 12 };
    ca4 b(micron::move(a));
    require(b[3], 12);
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                   //
  // ============================================================ //
  test_case("op=(const&)");
  {
    ca4 a{ 1, 2, 3, 4 };
    ca4 b;
    b = a;
    require(b[0], 1);
    require(b[3], 4);
  }
  end_test_case();

  test_case("op=(&&)");
  {
    ca4 a{ 5, 6, 7, 8 };
    ca4 b;
    b = micron::move(a);
    require(b[1], 6);
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                               //
  // ============================================================ //
  test_case("op[](idx): mutable/const");
  {
    ca4 a{ 1, 2, 3, 4 };
    require(a[0], 1);
    a[1] = 99;
    require(a[1], 99);
    const ca4 &c = a;
    require(c[2], 3);
  }
  end_test_case();

  test_case("data(): pointer");
  {
    ca4 a{ 10, 20, 30, 40 };
    int *p = a.data();
    require(p[2], 30);
    const ca4 &c = a;
    const int *cp = c.data();
    require(cp[3], 40);
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION                                                    //
  // ============================================================ //
  test_case("begin/end: range-for");
  {
    ca4 a{ 1, 2, 3, 4 };
    int sum = 0;
    for ( auto x : a ) sum += x;
    require(sum, 10);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                     //
  // ============================================================ //
  test_case("size: N");
  {
    ca4 a;
    require(a.size(), usize(4));
  }
  end_test_case();

  // ============================================================ //
  //  MODIFIERS                                                    //
  // ============================================================ //
  test_case("fill: broadcasts");
  {
    ca4 a;
    a.fill(11);
    for ( int i = 0; i < 4; ++i ) require(a[i], 11);
  }
  end_test_case();

  // ============================================================ //
  //  ARITHMETIC                                                   //
  // ============================================================ //
  test_case("op+=(scalar)");
  {
    ca4 a{ 1, 2, 3, 4 };
    a += 10;
    require(a[0], 11);
    require(a[3], 14);
  }
  end_test_case();

  test_case("op-=(scalar)");
  {
    ca4 a{ 10, 20, 30, 40 };
    a -= 5;
    require(a[0], 5);
  }
  end_test_case();

  test_case("op*=(scalar)");
  {
    ca4 a{ 1, 2, 3, 4 };
    a *= 3;
    require(a[3], 12);
  }
  end_test_case();

  test_case("op/=(scalar)");
  {
    ca4 a{ 12, 24, 36, 48 };
    a /= 4;
    require(a[3], 12);
  }
  end_test_case();

  test_case("op+=(array)");
  {
    ca4 a{ 1, 2, 3, 4 };
    ca4 b{ 10, 20, 30, 40 };
    a += b;
    require(a[2], 33);
  }
  end_test_case();

  test_case("op-=(array)");
  {
    ca4 a{ 100, 200, 300, 400 };
    ca4 b{ 1, 2, 3, 4 };
    a -= b;
    require(a[0], 99);
  }
  end_test_case();

  test_case("op*=(array)");
  {
    ca4 a{ 1, 2, 3, 4 };
    ca4 b{ 2, 2, 2, 2 };
    a *= b;
    require(a[3], 8);
  }
  end_test_case();

  test_case("op/=(array)");
  {
    ca4 a{ 10, 20, 30, 40 };
    ca4 b{ 2, 4, 5, 8 };
    a /= b;
    require(a[0], 5);
  }
  end_test_case();

  // ============================================================ //
  //  BINARY ARITHMETIC                                            //
  // ============================================================ //
  test_case("op+(array)");
  {
    ca4 a{ 1, 2, 3, 4 };
    ca4 b{ 10, 20, 30, 40 };
    ca4 c = a + b;
    require(c[0], 11);
    require(c[3], 44);
  }
  end_test_case();

  test_case("op-(array)");
  {
    ca4 a{ 100, 200, 300, 400 };
    ca4 b{ 1, 2, 3, 4 };
    ca4 c = a - b;
    require(c[3], 396);
  }
  end_test_case();

  // ============================================================ //
  //  REDUCTIONS                                                   //
  // ============================================================ //
  test_case("sum");
  {
    ca4 a{ 1, 2, 3, 4 };
    require(a.sum(), 10);
  }
  end_test_case();

  test_case("mul");
  {
    ca4 a{ 1, 2, 3, 4 };
    require(a.mul(), 24);      // 1*1*2*3*4 — uses first as initial
  }
  end_test_case();

  // ============================================================ //
  //  QUERIES                                                      //
  // ============================================================ //
  test_case("all/any");
  {
    ca4 a{ 7, 7, 7, 7 };
    require_true(a.all(7));
    require_false(a.any(0));
    a[0] = 0;
    require_false(a.all(7));
    require_true(a.any(0));
  }
  end_test_case();

  // ============================================================ //
  //  IN-PLACE SQRT                                                //
  // ============================================================ //
  test_case("sqrt: in-place");
  {
    ca4 a{ 1, 4, 9, 16 };
    a.sqrt();
    require(a[0], 1);
    require(a[1], 2);
    require(a[2], 3);
    require(a[3], 4);
  }
  end_test_case();

  // ============================================================ //
  //  TYPE QUERIES                                                 //
  // ============================================================ //
  test_case("is_pod/is_class/is_trivial: compile-time");
  {
    static_assert(ca4::is_pod());
    static_assert(!ca4::is_class());
    static_assert(ca4::is_trivial());
    require_true(ca4::is_trivial());
  }
  end_test_case();

  // ============================================================ //
  //  ALIAS CHECK                                                  //
  // ============================================================ //
  test_case("constarray is alias for constexpr_array");
  {
    cn4 a{ 1, 2, 3, 4 };
    require(a[2], 3);
  }
  end_test_case();

  print("=== ALL CONSTEXPR_ARRAY EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
