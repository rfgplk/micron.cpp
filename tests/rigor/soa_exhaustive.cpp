// soa_exhaustive.cpp
// Exhaustive per-member-function tests for micron::soa<Ts...>.
// Struct-of-arrays container: one heap buffer per column, grows by 2x.

#include "../../src/array/soa.hpp"
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
  print("=== SOA EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default empty");
  {
    micron::soa<int, float, char> s;
    require(s.size(), usize(0));
    require(s.capacity(), usize(0));
    require_true(s.empty());
  }
  end_test_case();

  test_case("ctor: with capacity");
  {
    micron::soa<int, float> s(64);
    require(s.size(), usize(0));
    require(s.capacity(), usize(64));
  }
  end_test_case();

  test_case("ctor: move");
  {
    micron::soa<int, char> a(16);
    a.emplace_back(1, 'a');
    a.emplace_back(2, 'b');
    micron::soa<int, char> b(micron::move(a));
    require(b.size(), usize(2));
    require(a.size(), usize(0));
    require(b.template at<0>(0), 1);
    require(b.template at<1>(1), 'b');
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                   //
  // ============================================================ //
  test_case("op=(soa&&): move");
  {
    micron::soa<int, char> a(8);
    a.emplace_back(5, 'x');
    micron::soa<int, char> b;
    b = micron::move(a);
    require(b.size(), usize(1));
    require(b.template at<0>(0), 5);
  }
  end_test_case();

  test_case("op=(soa&&): self-assign safe");
  {
    micron::soa<int> s(4);
    s.emplace_back(99);
    s = micron::move(s);
    require(s.size(), usize(1));
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                     //
  // ============================================================ //
  test_case("size/capacity/empty/max_size");
  {
    micron::soa<int> s;
    require_true(s.empty());
    s.reserve(32);
    require(s.capacity(), usize(32));
    require(s.max_size(), usize(32));
    s.emplace_back(1);
    require_false(s.empty());
    require(s.size(), usize(1));
  }
  end_test_case();

  test_case("reserve: grows capacity");
  {
    micron::soa<int, float> s;
    s.reserve(100);
    require_true(s.capacity() >= usize(100));
  }
  end_test_case();

  test_case("column_count: compile-time");
  {
    static_assert(micron::soa<int, float, char>::column_count == 3);
    require(micron::soa<int, float, char>::column_count, usize(3));
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                               //
  // ============================================================ //
  test_case("at<I>(idx): typed bounds-checked");
  {
    micron::soa<int, float> s(8);
    s.emplace_back(1, 1.5f);
    s.emplace_back(2, 2.5f);
    require(s.template at<0>(0), 1);
    require(s.template at<0>(1), 2);
    require_true(s.template at<1>(0) == 1.5f);
    require_true(s.template at<1>(1) == 2.5f);
    require_throw([&s]() { (void)s.template at<0>(99); });
  }
  end_test_case();

  test_case("at<I>(idx) const");
  {
    micron::soa<int> s(4);
    s.emplace_back(42);
    const auto &cs = s;
    require(cs.template at<0>(0), 42);
  }
  end_test_case();

  test_case("column<I>(): raw pointer access");
  {
    micron::soa<int, float> s(8);
    for ( int i = 0; i < 5; ++i ) s.emplace_back(i * 10, float(i));
    int *col0 = s.column<0>();
    float *col1 = s.column<1>();
    require(col0[0], 0);
    require(col0[4], 40);
    require_true(col1[2] == 2.0f);
  }
  end_test_case();

  test_case("column<I>() const");
  {
    micron::soa<int> s(4);
    s.emplace_back(7);
    const auto &cs = s;
    const int *col = cs.column<0>();
    require(col[0], 7);
  }
  end_test_case();

  // ============================================================ //
  //  MODIFIERS                                                    //
  // ============================================================ //
  test_case("emplace_back: grows on capacity-exhaust");
  {
    micron::soa<int> s;
    for ( int i = 0; i < 100; ++i ) s.emplace_back(i);
    require(s.size(), usize(100));
    require(s.template at<0>(99), 99);
  }
  end_test_case();

  test_case("pop_back: shrinks size");
  {
    micron::soa<int, char> s(8);
    s.emplace_back(1, 'a');
    s.emplace_back(2, 'b');
    s.pop_back();
    require(s.size(), usize(1));
    require(s.template at<0>(0), 1);
  }
  end_test_case();

  test_case("pop_back on empty throws");
  {
    micron::soa<int> s;
    require_throw([&s]() { s.pop_back(); });
  }
  end_test_case();

  test_case("clear: resets size, retains capacity");
  {
    micron::soa<int, float> s(16);
    for ( int i = 0; i < 5; ++i ) s.emplace_back(i, float(i));
    s.clear();
    require_true(s.empty());
    require(s.size(), usize(0));
    require(s.capacity(), usize(16));
  }
  end_test_case();

  test_case("swap: exchanges all column pointers and size");
  {
    micron::soa<int, char> a(8);
    a.emplace_back(1, 'x');
    a.emplace_back(2, 'y');
    micron::soa<int, char> b(4);
    b.emplace_back(99, 'z');
    a.swap(b);
    require(a.size(), usize(1));
    require(b.size(), usize(2));
    require(a.template at<0>(0), 99);
    require(b.template at<0>(1), 2);
  }
  end_test_case();

  // ============================================================ //
  //  STRESS                                                       //
  // ============================================================ //
  test_case("stress: 1000 rows of 3 columns");
  {
    micron::soa<int, float, char> s;
    for ( int i = 0; i < 1000; ++i ) s.emplace_back(i, float(i * 2), char('a' + (i % 26)));
    require(s.size(), usize(1000));
    require(s.template at<0>(500), 500);
    require_true(s.template at<1>(500) == 1000.0f);
    require(s.template at<2>(0), 'a');
  }
  end_test_case();

  print("=== ALL SOA EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
