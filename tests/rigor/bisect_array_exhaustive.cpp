// bisect_array_exhaustive.cpp
// Exhaustive per-member-function tests for micron::bisect_array<T, N>.
// Sorted-order array with O(log n) bisect insert and O(n) erase.

#include "../../src/array/bisect_array.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using ba8 = micron::bisect_array<int, 8>;

int
main()
{
  print("=== BISECT_ARRAY EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default empty");
  {
    ba8 v;
    require(v.size(), usize(0));
    require_true(v.empty());
    require_false(v.full());
  }
  end_test_case();

  test_case("ctor: copy preserves contents and length");
  {
    ba8 a;
    a.insert(3);
    a.insert(1);
    a.insert(2);
    require(a.size(), usize(3));
    ba8 b(a);
    require(b.size(), usize(3));
    require(b[0], 1);
    require(b[1], 2);
    require(b[2], 3);
  }
  end_test_case();

  test_case("ctor: move preserves contents and length");
  {
    ba8 a;
    a.insert(5);
    a.insert(1);
    require(a.size(), usize(2));
    ba8 b(micron::move(a));
    require(b.size(), usize(2));
    require(b[0], 1);
    require(b[1], 5);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                     //
  // ============================================================ //
  test_case("size/empty/full");
  {
    ba8 v;
    require_true(v.empty());
    for ( int i = 0; i < 8; ++i ) v.insert(i);
    require(v.size(), usize(8));
    require_true(v.full());
    require_false(v.empty());
  }
  end_test_case();

  test_case("static_size: compile-time N");
  {
    static_assert(ba8::static_size == 8);
    static_assert(ba8::__length == 8);
  }
  end_test_case();

  // ============================================================ //
  //  INSERT (maintains sorted order)                              //
  // ============================================================ //
  test_case("insert: out-of-order values are placed in sorted order");
  {
    ba8 v;
    v.insert(5);
    v.insert(1);
    v.insert(3);
    v.insert(2);
    v.insert(4);
    require(v.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(v[i], i + 1);
  }
  end_test_case();

  test_case("insert: duplicate values allowed, sorted");
  {
    ba8 v;
    v.insert(2);
    v.insert(1);
    v.insert(2);
    v.insert(1);
    require(v.size(), usize(4));
    require(v[0], 1);
    require(v[1], 1);
    require(v[2], 2);
    require(v[3], 2);
  }
  end_test_case();

  test_case("insert: throws when full");
  {
    ba8 v;
    for ( int i = 0; i < 8; ++i ) v.insert(i);
    require_throw([&v]() { v.insert(99); });
  }
  end_test_case();

  // ============================================================ //
  //  ERASE                                                        //
  // ============================================================ //
  test_case("erase(idx): removes and shifts");
  {
    ba8 v;
    v.insert(1);
    v.insert(2);
    v.insert(3);
    v.insert(4);
    v.erase(usize(1));      // remove '2'
    require(v.size(), usize(3));
    require(v[0], 1);
    require(v[1], 3);
    require(v[2], 4);
  }
  end_test_case();

  test_case("erase(idx) out of range throws");
  {
    ba8 v;
    v.insert(1);
    require_throw([&v]() { v.erase(usize(99)); });
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                               //
  // ============================================================ //
  test_case("op[](idx) const: bounds-checked against length");
  {
    ba8 v;
    v.insert(10);
    v.insert(20);
    require(v[0], 10);
    require(v[1], 20);
    require_throw([&v]() { (void)v[2]; });
  }
  end_test_case();

  test_case("get/cget: bounds-checked against N");
  {
    ba8 v;
    v.insert(7);
    auto it = v.get(usize(0));
    require(*it, 7);
    require_throw([&v]() { (void)v.get(usize(99)); });
    require_throw([&v]() { (void)v.cget(usize(99)); });
  }
  end_test_case();

  test_case("data(): mutable/const raw pointer");
  {
    ba8 v;
    v.insert(11);
    int *p = v.data();
    require(p[0], 11);
    const auto &cv = v;
    require(cv.data()[0], 11);
  }
  end_test_case();

  test_case("addr(): returns this");
  {
    ba8 v;
    require_true(v.addr() == micron::addressof(v));
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION                                                    //
  // ============================================================ //
  test_case("begin/end: range over length only");
  {
    ba8 v;
    v.insert(3);
    v.insert(1);
    v.insert(2);
    int sum = 0;
    for ( auto it = v.begin(); it != v.end(); ++it ) sum += *it;
    require(sum, 6);
  }
  end_test_case();

  test_case("cbegin/cend");
  {
    ba8 v;
    v.insert(10);
    v.insert(20);
    int sum = 0;
    for ( auto it = v.cbegin(); it != v.cend(); ++it ) sum += *it;
    require(sum, 30);
  }
  end_test_case();

  // ============================================================ //
  //  TYPE QUERIES                                                 //
  // ============================================================ //
  test_case("is_pod: compile-time");
  {
    static_assert(ba8::is_pod());
    require_true(ba8::is_pod());
  }
  end_test_case();

  print("=== ALL BISECT_ARRAY EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
