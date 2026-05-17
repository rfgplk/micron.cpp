// pair_exhaustive.cpp
// Exhaustive per-member-function tests for micron::pair<T, F>.

#include "../../src/std.hpp"
#include "../../src/tuple.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

int
main()
{
  print("=== PAIR EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default zero-initializes both fields");
  {
    micron::pair<int, int> p;
    require(p.a, 0);
    require(p.b, 0);
  }
  end_test_case();

  test_case("ctor: pair(const T&, const F&)");
  {
    micron::pair<int, char> p(42, 'x');
    require(p.a, 42);
    require(p.b, 'x');
  }
  end_test_case();

  test_case("ctor: pair(T&&, F&&) rvalue");
  {
    micron::pair<int, char> p(7, 'y');
    require(p.a, 7);
    require(p.b, 'y');
  }
  end_test_case();

  test_case("ctor: pair(initializer_list)");
  {
    micron::pair<int, int> p{ 10, 20 };
    require(p.a, 10);
    require(p.b, 20);
  }
  end_test_case();

  test_case("ctor: pair(initializer_list) partial - second left default");
  {
    micron::pair<int, int> p{ 99 };
    require(p.a, 99);
    require(p.b, 0);      // default-init
  }
  end_test_case();

  test_case("ctor: copy");
  {
    micron::pair<int, int> a(1, 2);
    micron::pair<int, int> b(a);
    require(b.a, 1);
    require(b.b, 2);
  }
  end_test_case();

  test_case("ctor: move");
  {
    micron::pair<int, int> a(5, 10);
    micron::pair<int, int> b(micron::move(a));
    require(b.a, 5);
    require(b.b, 10);
  }
  end_test_case();

  test_case("ctor: cross-type copy pair<K,L> -> pair<T,F>");
  {
    micron::pair<int, char> src(7, 'a');
    micron::pair<long, int> dst(src);
    require(dst.a, long(7));
    require(dst.b, int('a'));
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                   //
  // ============================================================ //
  test_case("op=(const pair&)");
  {
    micron::pair<int, int> a(3, 4);
    micron::pair<int, int> b;
    b = a;
    require(b.a, 3);
    require(b.b, 4);
  }
  end_test_case();

  test_case("op=(pair&&)");
  {
    micron::pair<int, int> a(11, 22);
    micron::pair<int, int> b;
    b = micron::move(a);
    require(b.a, 11);
    require(b.b, 22);
  }
  end_test_case();

  test_case("op=(initializer_list)");
  {
    micron::pair<int, int> p(0, 0);
    p = { 7, 8 };
    require(p.a, 7);
    require(p.b, 8);
  }
  end_test_case();

  test_case("op=(cross-type pair)");
  {
    micron::pair<int, char> src(1, 'z');
    micron::pair<long, int> dst;
    dst = src;
    require(dst.a, long(1));
    require(dst.b, int('z'));
  }
  end_test_case();

  // ============================================================ //
  //  MEMBER ACCESS                                                //
  // ============================================================ //
  test_case("public members a/b directly mutable");
  {
    micron::pair<int, int> p(0, 0);
    p.a = 99;
    p.b = 88;
    require(p.a, 99);
    require(p.b, 88);
  }
  end_test_case();

  test_case("get(): returns copy");
  {
    micron::pair<int, char> p(5, 'q');
    auto c = p.get();
    require(c.a, 5);
    require(c.b, 'q');
    c.a = 99;
    require(p.a, 5);      // original unchanged
  }
  end_test_case();

  // ============================================================ //
  //  FREE FUNCTION: tie                                           //
  // ============================================================ //
  test_case("tie(initializer_list): builds pair<C,C>");
  {
    auto p = micron::tie<int>({ 10, 20 });
    require(p.a, 10);
    require(p.b, 20);
  }
  end_test_case();

  // ============================================================ //
  //  EDGE CASES                                                   //
  // ============================================================ //
  test_case("pair with same T,F types");
  {
    micron::pair<int, int> p(1, 1);
    require(p.a, p.b);
  }
  end_test_case();

  test_case("pair with class-type members");
  {
    micron::pair<micron::string, int> p(micron::string("hello"), 42);
    require(p.a.size(), usize(5));
    require(p.b, 42);
  }
  end_test_case();

  print("=== ALL PAIR EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
