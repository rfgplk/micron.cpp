// pair_exhaustive.cpp
// Exhaustive per-member-function tests for micron::pair<T, F>.

#include "../../src/array/array.hpp"
#include "../../src/std.hpp"
#include "../../src/tuple.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"
#include "../support/tracked_types.hpp"

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

  // ============================================================ //
  //  MOVE LIFETIMES                                               //
  //                                                               //
  //  The move ctor used to destroy its source (o.a.~T()) while    //
  //  ~pair() is defaulted, so every moved-from pair was destroyed //
  //  twice. These count destructions instead of relying on ASan.  //
  // ============================================================ //
  test_case("move ctor: destroys the source exactly once");
  {
    mtest::Tracked<0>::reset();
    {
      micron::pair<mtest::Tracked<0>, mtest::Tracked<0>> a(mtest::Tracked<0>(1), mtest::Tracked<0>(2));
      micron::pair<mtest::Tracked<0>, mtest::Tracked<0>> b(micron::move(a));
      require(b.a.v, 1);
      require(b.b.v, 2);
    }
    require(mtest::Tracked<0>::live(), usize(0));
    require(mtest::Tracked<0>::dtor, mtest::Tracked<0>::ctor + mtest::Tracked<0>::copy_ctor + mtest::Tracked<0>::move_ctor);
  }
  end_test_case();

  test_case("move assign after move ctor: source is alive, not destroyed");
  {
    mtest::Tracked<1>::reset();
    {
      micron::pair<mtest::Tracked<1>, int> a(mtest::Tracked<1>(5), 6);
      micron::pair<mtest::Tracked<1>, int> tmp(micron::move(a));
      a = micron::pair<mtest::Tracked<1>, int>(mtest::Tracked<1>(7), 8);      // writes into a, which must still be live
      require(a.a.v, 7);
      require(tmp.a.v, 5);
    }
    require(mtest::Tracked<1>::live(), usize(0));
    require(mtest::Tracked<1>::dtor, mtest::Tracked<1>::ctor + mtest::Tracked<1>::copy_ctor + mtest::Tracked<1>::move_ctor);
  }
  end_test_case();

  test_case("move ctor: owning members survive and free once");
  {
    micron::pair<micron::string, micron::string> a(micron::string("alpha"), micron::string("beta"));
    micron::pair<micron::string, micron::string> b(micron::move(a));
    require(b.a.size(), usize(5));
    require(b.b.size(), usize(4));
    require_true(b.a == micron::string("alpha"));
    require_true(b.b == micron::string("beta"));
  }
  end_test_case();

  test_case("micron::swap: move-ctor then move-assign into the moved-from object");
  {
    micron::pair<micron::string, int> x(micron::string("left"), 1);
    micron::pair<micron::string, int> y(micron::string("right"), 2);
    micron::swap(x, y);
    require_true(x.a == micron::string("right"));
    require(x.b, 2);
    require_true(y.a == micron::string("left"));
    require(y.b, 1);
  }
  end_test_case();

  test_case("array<pair> move: deep_move pairs T(move(src)) with src.~T()");
  {
    mtest::Tracked<2>::reset();
    {
      micron::array<micron::pair<mtest::Tracked<2>, int>, 4> a;
      for ( int i = 0; i < 4; ++i ) a[i] = micron::pair<mtest::Tracked<2>, int>(mtest::Tracked<2>(i + 1), i);
      micron::array<micron::pair<mtest::Tracked<2>, int>, 4> b(micron::move(a));
      for ( int i = 0; i < 4; ++i ) require(b[i].a.v, i + 1);
    }
    require(mtest::Tracked<2>::live(), usize(0));
    require(mtest::Tracked<2>::dtor, mtest::Tracked<2>::ctor + mtest::Tracked<2>::copy_ctor + mtest::Tracked<2>::move_ctor);
  }
  end_test_case();

  test_case("vector<pair> weld: element-wise relocation of owning members");
  {
    micron::vector<micron::pair<micron::string, int>> v;
    micron::vector<micron::pair<micron::string, int>> w;
    v.push_back(micron::pair<micron::string, int>(micron::string("one"), 1));
    w.push_back(micron::pair<micron::string, int>(micron::string("two"), 2));
    w.push_back(micron::pair<micron::string, int>(micron::string("three"), 3));
    v.weld(micron::move(w));
    require(v.size(), usize(3));
    require_true(v[0].a == micron::string("one"));
    require_true(v[1].a == micron::string("two"));
    require_true(v[2].a == micron::string("three"));
  }
  end_test_case();

  // ============================================================ //
  //  OVERLOAD RESOLUTION                                          //
  // ============================================================ //
  test_case("ctor from lvalues copies, never steals");
  {
    micron::string s("abc");
    int n = 7;
    micron::pair<micron::string, int> p(s, n);
    require(s.size(), usize(3));      // the unconstrained pair(K&&,L&&) used to move out of s
    require_true(s == micron::string("abc"));
    require(p.a.size(), usize(3));
    require(p.b, 7);
  }
  end_test_case();

  test_case("reference members: move must not write through the reference");
  {
    int k = 11;
    int v = 22;
    micron::pair<const int &, int &> p(k, v);
    micron::pair<const int &, int &> q(micron::move(p));
    require(v, 22);      // o.b = 0x0 used to zero the referent
    require(k, 11);
    require(q.a, 11);
    require(q.b, 22);
  }
  end_test_case();

  test_case("cross-type converting ctor casts to the destination types");
  {
    micron::pair<int, char> src(9, 'b');
    micron::pair<long, int> dst(micron::move(src));
    require(dst.a, long(9));
    require(dst.b, int('b'));
  }
  end_test_case();

  test_case("pair<int,int> is a literal type: constructible and movable at compile time");
  {
    constexpr auto build = []() constexpr {
      micron::pair<int, int> x(3, 4);
      micron::pair<int, int> y(micron::move(x));
      return y.a + y.b;
    };
    static_assert(build() == 7, "pair must be usable in a constant expression");
    require(build(), 7);
  }
  end_test_case();

  print("=== ALL PAIR EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
