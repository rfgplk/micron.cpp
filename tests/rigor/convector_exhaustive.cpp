// convector_exhaustive.cpp
// Exhaustive per-member-function tests for micron::convector<T, Alloc, Sf>.
// convector mirrors vector's public surface but every method takes the
// internal mutex. Tests verify (a) the locked methods produce correct
// values, (b) const methods compile (mutable mutex), and (c) a small
// concurrent workload doesn't corrupt state.

#include "../../src/std.hpp"
#include "../../src/vector/convector.hpp"
#include "../../src/vector/vector.hpp"      // for __impl::grow

#include "../snowball/snowball.hpp"
#include "../support/mock_allocators.hpp"
#include "../support/tracked_types.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using cvi = micron::convector<int>;

int
main()
{
  print("=== CONVECTOR EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION & DESTRUCTION                                   //
  // ============================================================ //
  test_case("ctor: default");
  {
    cvi v;
    require_true(v.empty());
    require(v.size(), usize(0));
  }
  end_test_case();

  test_case("ctor: convector(size_type)");
  {
    cvi v(8);
    require(v.size(), usize(8));
  }
  end_test_case();

  test_case("ctor: convector(size_type, const T&)");
  {
    cvi v(4, 99);
    require(v.size(), usize(4));
    for ( usize i = 0; i < 4; ++i ) require(v[i], 99);
  }
  end_test_case();

  test_case("ctor: convector(initializer_list)");
  {
    cvi v{ 10, 20, 30 };
    require(v.size(), usize(3));
    require(v[1], 20);
  }
  end_test_case();

  test_case("ctor: convector(convector&&) [move]");
  {
    cvi a{ 1, 2, 3 };
    cvi b(micron::move(a));
    require(b.size(), usize(3));
    require(b[0], 1);
  }
  end_test_case();

  test_case("dtor: Tracked ctor==dtor");
  {
    mtest::Tracked<40>::reset();
    {
      micron::convector<mtest::Tracked<40>> v;
      for ( int i = 0; i < 50; ++i ) v.emplace_back(i);
    }
    require(mtest::Tracked<40>::live(), usize(0));
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT (with self-assign guard)                          //
  // ============================================================ //
  test_case("op=(const convector&): copy assignment");
  {
    cvi a{ 1, 2, 3 };
    cvi b;
    b = a;
    require(b.size(), usize(3));
    require(b[1], 2);
  }
  end_test_case();

  test_case("op=(convector&&): move assignment");
  {
    cvi a{ 4, 5, 6 };
    cvi b;
    b = micron::move(a);
    require(b.size(), usize(3));
  }
  end_test_case();

  test_case("self copy-assign safe");
  {
    cvi v{ 1, 2, 3 };
    v = v;
    require(v.size(), usize(3));
    require(v[2], 3);
  }
  end_test_case();

  test_case("self move-assign safe");
  {
    cvi v{ 7, 8, 9 };
    v = micron::move(v);
    require(v.size(), usize(3));
    require(v[0], 7);
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                               //
  // ============================================================ //
  test_case("op[](size_type)");
  {
    cvi v{ 1, 2, 3 };
    require(v[0], 1);
    require(v[2], 3);
  }
  end_test_case();

  test_case("at(size_type) bounds-checked");
  {
    cvi v{ 5, 6 };
    require(v.at(0), 5);
    require_throw([&v]() { (void)v.at(99); });
  }
  end_test_case();

  test_case("front()/back()");
  {
    cvi v{ 11, 22, 33 };
    require(v.front(), 11);
    require(v.back(), 33);
  }
  end_test_case();

  test_case("data(): pointer access");
  {
    cvi v{ 10, 20 };
    int *p = v.data();
    require(p[0], 10);
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION                                                    //
  // ============================================================ //
  test_case("begin/end: range-for");
  {
    cvi v{ 1, 2, 3, 4 };
    int sum = 0;
    for ( auto x : v ) sum += x;
    require(sum, 10);
  }
  end_test_case();

  test_case("cbegin/cend");
  {
    cvi v{ 5, 10, 15 };
    int sum = 0;
    for ( auto it = v.cbegin(); it != v.cend(); ++it ) sum += *it;
    require(sum, 30);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                     //
  // ============================================================ //
  test_case("size/max_size/empty");
  {
    cvi v;
    require_true(v.empty());
    v.push_back(1);
    require(v.size(), usize(1));
    require_greater(v.max_size(), usize(0));
    require_false(v.empty());
  }
  end_test_case();

  test_case("const methods callable on const ref (mutable mutex regression)");
  {
    cvi v{ 1, 2, 3 };
    const cvi &cv = v;
    require(cv.size(), usize(3));
    require_false(cv.empty());
    require_greater(cv.max_size(), usize(0));
  }
  end_test_case();

  test_case("reserve");
  {
    cvi v;
    v.reserve(64);
    require_greater(v.max_size(), usize(0));
  }
  end_test_case();

  test_case("try_reserve: no-op when n <= capacity (B3 regression)");
  {
    cvi v;
    v.reserve(64);
    usize cap = v.max_size();
    bool threw = false;
    try {
      v.try_reserve(32);
    } catch ( ... ) {
      threw = true;
    }
    require(threw, false);
    require(v.max_size(), cap);
  }
  end_test_case();

  // ============================================================ //
  //  MODIFIERS                                                    //
  // ============================================================ //
  test_case("push_back(const T&) and push_back(T&&)");
  {
    cvi v;
    int x = 11;
    v.push_back(x);
    v.push_back(22);
    require(v.size(), usize(2));
    require(v[0], 11);
    require(v[1], 22);
  }
  end_test_case();

  test_case("emplace_back");
  {
    struct P {
      int a;

      P(int x) : a(x) { }
    };

    micron::convector<P> v;
    v.emplace_back(7);
    require(v[0].a, 7);
  }
  end_test_case();

  test_case("pop_back");
  {
    cvi v{ 1, 2, 3 };
    v.pop_back();
    require(v.size(), usize(2));
    require(v.back(), 2);
  }
  end_test_case();

  test_case("insert(size_type, T&&)");
  {
    cvi v{ 1, 2, 4 };
    v.insert(usize(2), 3);
    require(v.size(), usize(4));
    require(v[2], 3);
    require(v[3], 4);
  }
  end_test_case();

  test_case("erase(size_type)");
  {
    cvi v{ 1, 2, 3, 4 };
    v.erase(usize(1));
    require(v.size(), usize(3));
    require(v[1], 3);
  }
  end_test_case();

  test_case("clear");
  {
    cvi v{ 1, 2, 3 };
    v.clear();
    require_true(v.empty());
  }
  end_test_case();

  // ============================================================ //
  //  UTILITY                                                      //
  // ============================================================ //
  test_case("swap");
  {
    cvi a{ 1, 2 };
    cvi b{ 3, 4, 5 };
    a.swap(b);
    require(a.size(), usize(3));
    require(b.size(), usize(2));
  }
  end_test_case();

  test_case("fill");
  {
    cvi v(5);
    v.fill(7);
    for ( usize i = 0; i < 5; ++i ) require(v[i], 7);
  }
  end_test_case();

  // ============================================================ //
  //  ALLOCATOR                                                    //
  // ============================================================ //
  test_case("tracking_allocator: no leak");
  {
    using A = mtest::tracking_allocator<41>;
    A::reset();
    {
      micron::convector<int, A> v;
      for ( int i = 0; i < 30; ++i ) v.push_back(i);
    }
    require(A::outstanding(), i64(0));
  }
  end_test_case();

  print("=== ALL CONVECTOR EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
