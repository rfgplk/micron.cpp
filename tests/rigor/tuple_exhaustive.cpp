// tuple_exhaustive.cpp
// Exhaustive per-member-function tests for micron::tuple<Ts...> and its
// free functions (make_tuple, forward_as_tuple, tuple_cat, apply, get<I>,
// get<T>, tuple_size, tuple_element).

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
  print("=== TUPLE EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default tuple<>");
  {
    micron::tuple<> t;
    (void)t;
    static_assert(micron::tuple_size_v<micron::tuple<>> == 0);
  }
  end_test_case();

  test_case("ctor: default tuple<int, char>");
  {
    micron::tuple<int, char> t;
    (void)t;
    static_assert(micron::tuple_size_v<micron::tuple<int, char>> == 2);
  }
  end_test_case();

  test_case("ctor: tuple(args...) value-initialized");
  {
    micron::tuple<int, char, long> t(42, 'q', 999L);
    require(micron::get<0>(t), 42);
    require(micron::get<1>(t), 'q');
    require(micron::get<2>(t), 999L);
  }
  end_test_case();

  test_case("ctor: copy");
  {
    micron::tuple<int, char> a(1, 'a');
    micron::tuple<int, char> b(a);
    require(micron::get<0>(b), 1);
    require(micron::get<1>(b), 'a');
  }
  end_test_case();

  test_case("ctor: move");
  {
    micron::tuple<int, char> a(7, 'z');
    micron::tuple<int, char> b(micron::move(a));
    require(micron::get<0>(b), 7);
    require(micron::get<1>(b), 'z');
  }
  end_test_case();

  test_case("ctor: cross-type copy tuple<Us...> -> tuple<Ts...>");
  {
    micron::tuple<int, char> src(11, 'p');
    micron::tuple<long, int> dst(src);
    require(micron::get<0>(dst), long(11));
    require(micron::get<1>(dst), int('p'));
  }
  end_test_case();

  test_case("ctor: cross-type move");
  {
    micron::tuple<int, char> src(5, 'x');
    micron::tuple<long, int> dst(micron::move(src));
    require(micron::get<0>(dst), long(5));
    require(micron::get<1>(dst), int('x'));
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                   //
  // ============================================================ //
  test_case("op=(const tuple&)");
  {
    micron::tuple<int, char> a(3, 'b');
    micron::tuple<int, char> b;
    b = a;
    require(micron::get<0>(b), 3);
    require(micron::get<1>(b), 'b');
  }
  end_test_case();

  test_case("op=(tuple&&)");
  {
    micron::tuple<int, char> a(8, 'e');
    micron::tuple<int, char> b;
    b = micron::move(a);
    require(micron::get<0>(b), 8);
    require(micron::get<1>(b), 'e');
  }
  end_test_case();

  test_case("op=(cross-type tuple)");
  {
    micron::tuple<int, char> src(1, 'q');
    micron::tuple<long, int> dst;
    dst = src;
    require(micron::get<0>(dst), long(1));
    require(micron::get<1>(dst), int('q'));
  }
  end_test_case();

  // ============================================================ //
  //  GET<I>(tuple): lvalue / const / rvalue / const rvalue        //
  // ============================================================ //
  test_case("get<I>(tuple&) lvalue: mutable reference");
  {
    micron::tuple<int, char> t(1, 'a');
    micron::get<0>(t) = 99;
    micron::get<1>(t) = 'Z';
    require(micron::get<0>(t), 99);
    require(micron::get<1>(t), 'Z');
  }
  end_test_case();

  test_case("get<I>(const tuple&): const reference");
  {
    const micron::tuple<int, char> t(5, 'c');
    require(micron::get<0>(t), 5);
    require(micron::get<1>(t), 'c');
  }
  end_test_case();

  test_case("get<I>(tuple&&): rvalue reference");
  {
    micron::tuple<int, char> t(11, 'x');
    int x = micron::get<0>(micron::move(t));
    require(x, 11);
  }
  end_test_case();

  // ============================================================ //
  //  GET<T>(tuple): by type                                       //
  // ============================================================ //
  test_case("get<T>(tuple&): by type lookup");
  {
    micron::tuple<int, char> t(7, 'q');
    require(micron::get<int>(t), 7);
    require(micron::get<char>(t), 'q');
  }
  end_test_case();

  test_case("get<T>(const tuple&)");
  {
    const micron::tuple<int, char> t(13, 'r');
    require(micron::get<int>(t), 13);
    require(micron::get<char>(t), 'r');
  }
  end_test_case();

  // ============================================================ //
  //  TUPLE_SIZE / TUPLE_ELEMENT (compile-time)                    //
  // ============================================================ //
  test_case("tuple_size_v: compile-time size");
  {
    static_assert(micron::tuple_size_v<micron::tuple<>> == 0);
    static_assert(micron::tuple_size_v<micron::tuple<int>> == 1);
    static_assert(micron::tuple_size_v<micron::tuple<int, char, long>> == 3);
    require(micron::tuple_size_v<micron::tuple<int, char, long>>, usize(3));
  }
  end_test_case();

  test_case("tuple_element: compile-time type lookup");
  {
    static_assert(micron::is_same_v<micron::tuple_element_t<0, micron::tuple<int, char>>, int>);
    static_assert(micron::is_same_v<micron::tuple_element_t<1, micron::tuple<int, char>>, char>);
    require_true(true);
  }
  end_test_case();

  // ============================================================ //
  //  SWAP                                                         //
  // ============================================================ //
  test_case("swap(tuple&)");
  {
    micron::tuple<int, char> a(1, 'a');
    micron::tuple<int, char> b(2, 'b');
    a.swap(b);
    require(micron::get<0>(a), 2);
    require(micron::get<1>(a), 'b');
    require(micron::get<0>(b), 1);
    require(micron::get<1>(b), 'a');
  }
  end_test_case();

  test_case("swap on tuple<>: no-op");
  {
    micron::tuple<> a;
    micron::tuple<> b;
    a.swap(b);
    require_true(true);
  }
  end_test_case();

  // ============================================================ //
  //  MAKE_TUPLE / FORWARD_AS_TUPLE                                //
  // ============================================================ //
  test_case("make_tuple: deduces types");
  {
    auto t = micron::make_tuple(1, 'x', 3.14);
    require(micron::get<0>(t), 1);
    require(micron::get<1>(t), 'x');
    static_assert(micron::tuple_size_v<decltype(t)> == 3);
  }
  end_test_case();

  test_case("forward_as_tuple: preserves references");
  {
    int x = 5;
    char y = 'q';
    auto t = micron::forward_as_tuple(x, y);
    // get yields references; mutating through them must change originals.
    micron::get<0>(t) = 99;
    require(x, 99);
  }
  end_test_case();

  // ============================================================ //
  //  TUPLE_CAT                                                    //
  // ============================================================ //
  test_case("tuple_cat: 2 tuples");
  {
    auto a = micron::make_tuple(1, 'a');
    auto b = micron::make_tuple(2L, 3.0);
    auto c = micron::tuple_cat(a, b);
    static_assert(micron::tuple_size_v<decltype(c)> == 4);
    require(micron::get<0>(c), 1);
    require(micron::get<1>(c), 'a');
    require(micron::get<2>(c), 2L);
  }
  end_test_case();

  test_case("tuple_cat: 3 tuples");
  {
    auto a = micron::make_tuple(1);
    auto b = micron::make_tuple('z');
    auto cc = micron::make_tuple(2L, 3L);
    auto c = micron::tuple_cat(a, b, cc);
    static_assert(micron::tuple_size_v<decltype(c)> == 4);
    require(micron::get<0>(c), 1);
    require(micron::get<3>(c), 3L);
  }
  end_test_case();

  test_case("tuple_cat: with empty tuple");
  {
    micron::tuple<> e;
    auto a = micron::make_tuple(7);
    auto c = micron::tuple_cat(e, a);
    static_assert(micron::tuple_size_v<decltype(c)> == 1);
    require(micron::get<0>(c), 7);
  }
  end_test_case();

  // ============================================================ //
  //  APPLY                                                        //
  // ============================================================ //
  test_case("apply: invokes callable with tuple elements");
  {
    auto t = micron::make_tuple(2, 3);
    int product = micron::apply([](int a, int b) { return a * b; }, t);
    require(product, 6);
  }
  end_test_case();

  test_case("apply: with 3-tuple");
  {
    auto t = micron::make_tuple(1, 2, 3);
    int sum = micron::apply([](int a, int b, int c) { return a + b + c; }, t);
    require(sum, 6);
  }
  end_test_case();

  test_case("apply: returns void OK");
  {
    auto t = micron::make_tuple(5, 7);
    int collected = 0;
    micron::apply([&collected](int a, int b) { collected = a + b; }, t);
    require(collected, 12);
  }
  end_test_case();

  print("=== ALL TUPLE EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
