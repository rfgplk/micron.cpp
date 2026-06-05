// array_exhaustive.cpp
// Exhaustive per-member-function tests for micron::array<T, N>.
// array is stack-allocated, fixed-capacity, mutable. All ctors zero-init
// trailing slots when an initializer_list is shorter than N.

#include "../../src/array/array.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../support/tracked_types.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using a8 = micron::array<int, 8>;
using a4 = micron::array<int, 4>;

int
main()
{
  print("=== ARRAY EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default zero-initializes");
  {
    a8 v;
    for ( usize i = 0; i < 8; ++i ) require(v[i], 0);
  }
  end_test_case();

  test_case("ctor: array(const T&) fills");
  {
    a8 v(7);
    for ( usize i = 0; i < 8; ++i ) require(v[i], 7);
  }
  end_test_case();

  test_case("ctor: array(initializer_list) — full");
  {
    a4 v{ 1, 2, 3, 4 };
    require(v[0], 1);
    require(v[3], 4);
  }
  end_test_case();

  test_case("ctor: array(initializer_list) — short, tail zeroed");
  {
    a4 v{ 9, 8 };
    require(v[0], 9);
    require(v[1], 8);
    require(v[2], 0);
    require(v[3], 0);
  }
  end_test_case();

  test_case("ctor: array(initializer_list too large) throws");
  {
    bool threw = false;
    try {
      a4 v{ 1, 2, 3, 4, 5 };
      (void)v;
    } catch ( ... ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  test_case("ctor: copy");
  {
    a4 a{ 1, 2, 3, 4 };
    a4 b(a);
    for ( int i = 0; i < 4; ++i ) require(b[i], i + 1);
  }
  end_test_case();

  test_case("ctor: move");
  {
    a4 a{ 5, 6, 7, 8 };
    a4 b(micron::move(a));
    for ( int i = 0; i < 4; ++i ) require(b[i], i + 5);
  }
  end_test_case();

  test_case("ctor: array(Fn&&) — generator (requires Fn() return T)");
  {
    int c = 0;
    a4 v([&c]() { return c++; });
    for ( int i = 0; i < 4; ++i ) require(v[i], i);
  }
  end_test_case();

  test_case("dtor: Tracked balance after scope exit");
  {
    mtest::Tracked<60>::reset();
    {
      micron::array<mtest::Tracked<60>, 8> v;
      for ( int i = 0; i < 8; ++i ) v[i].v = i;
    }
    require(mtest::Tracked<60>::live(), usize(0));
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                   //
  // ============================================================ //
  test_case("op=(const array&): copy assignment");
  {
    a4 a{ 1, 2, 3, 4 };
    a4 b;
    b = a;
    for ( int i = 0; i < 4; ++i ) require(b[i], i + 1);
  }
  end_test_case();

  test_case("op=(array&&): move assignment");
  {
    a4 a{ 5, 6, 7, 8 };
    a4 b;
    b = micron::move(a);
    require(b[0], 5);
    require(b[3], 8);
  }
  end_test_case();

  test_case("op=(fundamental): broadcast fill via fill()");
  {
    // Note: array::operator=(F) where F is fundamental is ambiguous with
    // other overloads (operator=(const array&) is the canonical match for
    // direct array-from-int assignment is not the intended path). Use
    // fill() for the broadcast semantic instead.
    a4 v;
    v.fill(42);
    for ( int i = 0; i < 4; ++i ) require(v[i], 42);
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                               //
  // ============================================================ //
  test_case("op[](size_type): unchecked access");
  {
    a4 v{ 10, 20, 30, 40 };
    require(v[0], 10);
    v[2] = 99;
    require(v[2], 99);
  }
  end_test_case();

  test_case("op[](size_type, size_type): slice range");
  {
    a8 v{ 1, 2, 3, 4, 5, 6, 7, 8 };
    auto s = v[2, 6];
    require(s.size(), usize(4));
  }
  end_test_case();

  test_case("at(size_type): bounds-checked, throws");
  {
    a4 v{ 1, 2, 3, 4 };
    require(v.at(0), 1);
    require(v.at(3), 4);
    require_throw([&v]() { (void)v.at(99); });
    require_throw([&v]() { (void)v.at(4); });
  }
  end_test_case();

  test_case("get/cget: bounds-checked iterators");
  {
    a4 v{ 1, 2, 3, 4 };
    auto it = v.get(1);
    require(*it, 2);
    auto cit = v.cget(2);
    require(*cit, 3);
    require_throw([&v]() { (void)v.get(usize(99)); });
    require_throw([&v]() { (void)v.cget(usize(99)); });
  }
  end_test_case();

  test_case("data(): mutable/const raw pointer");
  {
    a4 v{ 1, 2, 3, 4 };
    int *p = v.data();
    require(p[2], 3);
    const a4 &cv = v;
    const int *cp = cv.data();
    require(cp[0], 1);
  }
  end_test_case();

  test_case("addr(): returns this");
  {
    a4 v;
    require_true(v.addr() == micron::addressof(v));
    const a4 &cv = v;
    require_true(cv.addr() == micron::addressof(cv));
  }
  end_test_case();

  test_case("op&(): byte pointer alias");
  {
    a4 v{ 1, 2, 3, 4 };
    byte *bp = &v;
    require_true(bp != nullptr);
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION                                                    //
  // ============================================================ //
  test_case("begin/end: range-for sums");
  {
    a4 v{ 1, 2, 3, 4 };
    int sum = 0;
    for ( auto x : v ) sum += x;
    require(sum, 10);
  }
  end_test_case();

  test_case("cbegin/cend");
  {
    a4 v{ 10, 20, 30, 40 };
    int sum = 0;
    for ( auto it = v.cbegin(); it != v.cend(); ++it ) sum += *it;
    require(sum, 100);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY (compile-time)                                      //
  // ============================================================ //
  test_case("size/max_size/length/static_size: all N");
  {
    a8 v;
    require(v.size(), usize(8));
    require(v.max_size(), usize(8));
    static_assert(a8::length == 8);
    static_assert(a8::static_size == 8);
  }
  end_test_case();

  // ============================================================ //
  //  MODIFIERS                                                    //
  // ============================================================ //
  test_case("clear(): zero + reconstruct");
  {
    a4 v{ 1, 2, 3, 4 };
    v.clear();
    // After clear, T{} is constructed in place; for int that's 0
    require(v[0], 0);
    require(v[3], 0);
  }
  end_test_case();

  test_case("fill(fundamental): broadcast");
  {
    a4 v;
    v.fill(11);
    for ( int i = 0; i < 4; ++i ) require(v[i], 11);
  }
  end_test_case();

  // ============================================================ //
  //  ARITHMETIC (binary, non-mutating)                            //
  // ============================================================ //
  test_case("op+(array): elementwise add");
  {
    a4 a{ 1, 2, 3, 4 };
    a4 b{ 10, 20, 30, 40 };
    a4 c = a + b;
    require(c[0], 11);
    require(c[3], 44);
  }
  end_test_case();

  test_case("op-(array): elementwise sub");
  {
    a4 a{ 100, 200, 300, 400 };
    a4 b{ 1, 2, 3, 4 };
    a4 c = a - b;
    require(c[0], 99);
    require(c[3], 396);
  }
  end_test_case();

  test_case("op*(array): elementwise mul");
  {
    a4 a{ 1, 2, 3, 4 };
    a4 b{ 2, 2, 2, 2 };
    a4 c = a * b;
    require(c[0], 2);
    require(c[3], 8);
  }
  end_test_case();

  test_case("op/(array): elementwise div");
  {
    a4 a{ 10, 20, 30, 40 };
    a4 b{ 2, 4, 5, 8 };
    a4 c = a / b;
    require(c[0], 5);
    require(c[3], 5);
  }
  end_test_case();

  test_case("op%(array): elementwise mod (integral)");
  {
    a4 a{ 10, 11, 12, 13 };
    a4 b{ 3, 3, 5, 5 };
    a4 c = a % b;
    require(c[0], 1);
    require(c[2], 2);
  }
  end_test_case();

  // ============================================================ //
  //  COMPOUND ASSIGNMENT                                          //
  // ============================================================ //
  test_case("op+=(scalar): broadcast");
  {
    a4 v{ 1, 2, 3, 4 };
    v += 10;
    require(v[0], 11);
    require(v[3], 14);
  }
  end_test_case();

  test_case("op-=(scalar)");
  {
    a4 v{ 10, 20, 30, 40 };
    v -= 5;
    require(v[0], 5);
    require(v[3], 35);
  }
  end_test_case();

  test_case("op*=(scalar)");
  {
    a4 v{ 1, 2, 3, 4 };
    v *= 3;
    require(v[3], 12);
  }
  end_test_case();

  test_case("op/=(scalar)");
  {
    a4 v{ 12, 24, 36, 48 };
    v /= 4;
    require(v[0], 3);
    require(v[3], 12);
  }
  end_test_case();

  test_case("op%=(scalar): integral");
  {
    a4 v{ 10, 11, 12, 13 };
    v %= 3;
    require(v[0], 1);
    require(v[3], 1);
  }
  end_test_case();

  test_case("op+=(array): elementwise");
  {
    a4 v{ 1, 2, 3, 4 };
    a4 o{ 10, 20, 30, 40 };
    v += o;
    require(v[2], 33);
  }
  end_test_case();

  // ============================================================ //
  //  REDUCTIONS                                                   //
  // ============================================================ //
  test_case("sum(): aggregate");
  {
    a4 v{ 1, 2, 3, 4 };
    require(v.sum(), 10);
  }
  end_test_case();

  test_case("mul_reduce(): product");
  {
    a4 v{ 1, 2, 3, 4 };
    // Note: mul_reduce uses src[0] as initial, then multiplies by src[i] for i in [1,N).
    // For {1,2,3,4}: 1 * 1 * 2 * 3 * 4 = 24? Actually init=src[0]=1, then m *= src[1]..src[N-1] giving 1*2*3*4=24.
    require(v.mul_reduce(), 24);
  }
  end_test_case();

  // ============================================================ //
  //  IN-PLACE NAMED ARITHMETIC                                    //
  // ============================================================ //
  test_case("add(n)/sub(n)/mul(n)/div(n): broadcast (in-place)");
  {
    a4 v{ 10, 20, 30, 40 };
    v.add(1);
    require(v[0], 11);
    v.sub(1);
    require(v[0], 10);
    v.mul(2);
    require(v[0], 20);
    v.div(2);
    require(v[0], 10);
  }
  end_test_case();

  // ============================================================ //
  //  QUERIES                                                      //
  // ============================================================ //
  test_case("all(): all-equal predicate");
  {
    a4 v{ 7, 7, 7, 7 };
    require_true(v.all(7));
    v[0] = 0;
    require_false(v.all(7));
  }
  end_test_case();

  test_case("any(): any-equal predicate");
  {
    a4 v{ 1, 2, 3, 4 };
    require_true(v.any(3));
    require_false(v.any(99));
  }
  end_test_case();

  // ============================================================ //
  //  TYPE QUERIES                                                 //
  // ============================================================ //
  test_case("is_pod/is_class/is_trivial: compile-time");
  {
    static_assert(a4::is_pod());
    static_assert(!a4::is_class());
    static_assert(a4::is_trivial());
    require_true(a4::is_trivial());
  }
  end_test_case();

  // ============================================================ //
  //  SQRT (mutates in place)                                      //
  // ============================================================ //
  test_case("sqrt(): in-place per-element sqrt");
  {
    a4 v{ 1, 4, 9, 16 };
    v.sqrt();
    require(v[0], 1);
    require(v[1], 2);
    require(v[2], 3);
    require(v[3], 4);
  }
  end_test_case();

  print("=== ALL ARRAY EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
