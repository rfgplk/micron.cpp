// conarray_exhaustive.cpp
// Exhaustive per-member-function tests for micron::conarray<T, N>.
// Concurrent fixed-size array — all mutating ops take the internal mutex.

#include "../../src/array/conarray.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using ca8 = micron::conarray<int, 8>;
using ca4 = micron::conarray<int, 4>;

int
main()
{
  print("=== CONARRAY EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default");
  {
    ca8 v;
    require(v.size(), usize(8));
  }
  end_test_case();

  test_case("ctor: from fill value");
  {
    ca4 v(7);
    require(v[0], 7);
    require(v[3], 7);
  }
  end_test_case();

  test_case("ctor: initializer_list (short, tail zeroed)");
  {
    ca4 v{ 1, 2 };
    require(v[0], 1);
    require(v[1], 2);
    require(v[2], 0);
    require(v[3], 0);
  }
  end_test_case();

  test_case("ctor: copy (locks source)");
  {
    ca4 a{ 1, 2, 3, 4 };
    ca4 b(a);
    require(b[0], 1);
    require(b[3], 4);
  }
  end_test_case();

  test_case("ctor: move (locks source)");
  {
    ca4 a{ 5, 6, 7, 8 };
    ca4 b(micron::move(a));
    require(b[0], 5);
    require(b[3], 8);
  }
  end_test_case();

  test_case("ctor: array too large throws");
  {
    bool threw = false;
    try {
      ca4 v{ 1, 2, 3, 4, 5 };
      (void)v;
    } catch ( ... ) {
      threw = true;
    }
    require(threw, true);
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                               //
  // ============================================================ //
  test_case("at(idx) const: locks, returns by value");
  {
    ca4 v{ 10, 20, 30, 40 };
    require(v.at(0), 10);
    require(v.at(3), 40);
    require_throw([&v]() { (void)v.at(99); });
  }
  end_test_case();

  test_case("at(idx, val): locks, sets");
  {
    ca4 v;
    v.at(2, 99);
    require(v[2], 99);
    require_throw([&v]() { v.at(99, 1); });
  }
  end_test_case();

  test_case("op[](idx): no-lock direct access");
  {
    ca4 v{ 1, 2, 3, 4 };
    require(v[0], 1);
    v[1] = 22;
    require(v[1], 22);
    const ca4 &cv = v;
    require(cv[2], 3);
  }
  end_test_case();

  test_case("get/cget: bounds-checked");
  {
    ca4 v{ 1, 2, 3, 4 };
    auto it = v.get(2);
    require(*it, 3);
    auto cit = v.cget(0);
    require(*cit, 1);
    require_throw([&v]() { (void)v.get(usize(99)); });
    require_throw([&v]() { (void)v.cget(usize(99)); });
  }
  end_test_case();

  test_case("data() const: raw pointer");
  {
    ca4 v{ 1, 2, 3, 4 };
    const auto &cv = v;
    const int *p = cv.data();
    require(p[2], 3);
  }
  end_test_case();

  test_case("view() const: same as data()");
  {
    ca4 v{ 5, 6, 7, 8 };
    const auto &cv = v;
    const int *p = cv.view();
    require(p[1], 6);
  }
  end_test_case();

  test_case("addr(): returns this");
  {
    ca4 v;
    require_true(v.addr() == micron::addressof(v));
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION (const-only public iteration)                     //
  // ============================================================ //
  test_case("begin/end const: range-for sum");
  {
    ca4 v{ 1, 2, 3, 4 };
    int sum = 0;
    for ( auto it = v.begin(); it != v.end(); ++it ) sum += *it;
    require(sum, 10);
  }
  end_test_case();

  test_case("cbegin/cend");
  {
    ca4 v{ 10, 20, 30, 40 };
    int sum = 0;
    for ( auto it = v.cbegin(); it != v.cend(); ++it ) sum += *it;
    require(sum, 100);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY (compile-time)                                      //
  // ============================================================ //
  test_case("size/max_size: N");
  {
    ca8 v;
    require(v.size(), usize(8));
    require(v.max_size(), usize(8));
    static_assert(ca8::length == 8);
  }
  end_test_case();

  // ============================================================ //
  //  MODIFIERS                                                    //
  // ============================================================ //
  test_case("clear: zero + reconstruct");
  {
    ca4 v{ 7, 7, 7, 7 };
    v.clear();
    require(v[0], 0);
    require(v[3], 0);
  }
  end_test_case();

  // ============================================================ //
  //  EXCLUSIVE ACCESS (get + release pair)                       //
  // ============================================================ //
  test_case("get() + release(): manual lock acquire");
  {
    ca4 v{ 1, 2, 3, 4 };
    int *p = v.get();
    p[0] = 99;
    v.release();
    require(v[0], 99);
  }
  end_test_case();

  print("=== ALL CONARRAY EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
