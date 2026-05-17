// fvector_exhaustive.cpp
// Exhaustive per-member-function tests for micron::fvector<T, Alloc, Sf>.
// fvector is the move-only "fast vector" — copy ctor/assign are deleted,
// only move semantics. Mirrors vector's structure minus copy paths.

#include "../../src/std.hpp"
#include "../../src/vector/fvector.hpp"
#include "../../src/vector/vector.hpp"      // load __impl::grow first

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
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

using fvi = micron::fvector<int>;

int
main()
{
  print("=== FVECTOR EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                  //
  // ============================================================ //
  test_case("ctor: default fvector()");
  {
    fvi v;
    require_true(v.empty());
    require(v.size(), usize(0));
  }
  end_test_case();

  test_case("ctor: fvector(size_type)");
  {
    fvi v(8);
    require(v.size(), usize(8));
    for ( usize i = 0; i < 8; ++i ) require(v[i], 0);
  }
  end_test_case();

  test_case("ctor: fvector(size_type, const T&)");
  {
    fvi v(5, 99);
    require(v.size(), usize(5));
    for ( usize i = 0; i < 5; ++i ) require(v[i], 99);
  }
  end_test_case();

  test_case("ctor: fvector(size_type, Fn&&) [generator]");
  {
    int c = 0;
    fvi v(4, [&c]() { return c++; });
    require(v.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(v[i], i);
  }
  end_test_case();

  test_case("ctor: fvector(size_type, Args...)");
  {
    struct P {
      int a, b;

      P(int x, int y) : a(x), b(y) { }
    };

    micron::fvector<P> v(3, 4, 5);
    require(v.size(), usize(3));
    require(v[0].a, 4);
    require(v[0].b, 5);
  }
  end_test_case();

  test_case("ctor: fvector(initializer_list)");
  {
    fvi v{ 10, 20, 30 };
    require(v.size(), usize(3));
    require(v[1], 20);
  }
  end_test_case();

  test_case("ctor: fvector(fvector&&) [move]");
  {
    fvi a{ 1, 2, 3 };
    fvi b(micron::move(a));
    require(b.size(), usize(3));
    require(b[2], 3);
  }
  end_test_case();

  // ============================================================ //
  //  DESTRUCTION                                                   //
  // ============================================================ //
  test_case("dtor: Tracked ctor==dtor balance");
  {
    mtest::Tracked<30>::reset();
    {
      micron::fvector<mtest::Tracked<30>> v;
      for ( int i = 0; i < 50; ++i ) v.emplace_back(i);
      require(v.size(), usize(50));
    }
    require(mtest::Tracked<30>::live(), usize(0));
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                    //
  // ============================================================ //
  test_case("op=(fvector&&): move assignment");
  {
    fvi a{ 1, 2, 3 };
    fvi b;
    b = micron::move(a);
    require(b.size(), usize(3));
    require(b[0], 1);
  }
  end_test_case();

  test_case("op=(fvector&&): self-move-assign is safe (Sf=true)");
  {
    fvi v{ 7, 8, 9 };
    v = micron::move(v);
    require(v.size(), usize(3));
    require(v[0], 7);
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS                                                //
  // ============================================================ //
  test_case("op[](size_type): mutable/const");
  {
    fvi v{ 1, 2, 3 };
    require(v[0], 1);
    v[1] = 22;
    const fvi &cv = v;
    require(cv[1], 22);
  }
  end_test_case();

  test_case("at(size_type): unchecked (fvector = fast, no bounds check)");
  {
    // INTENTIONAL: fvector::at() is a fast accessor that does NOT bounds-
    // check even with Sf=true. This is by design (the "f" prefix = fast).
    // If safety becomes desired in fvector, gate at() on Sf via the same
    // __safety_check pattern as vector.
    fvi v{ 5, 6, 7 };
    require(v.at(0), 5);
    require(v.at(2), 7);
  }
  end_test_case();

  test_case("front()/back()");
  {
    fvi v{ 11, 22, 33 };
    require(v.front(), 11);
    require(v.back(), 33);
  }
  end_test_case();

  test_case("data(): pointer access");
  {
    fvi v{ 100, 200 };
    int *p = v.data();
    require(p[0], 100);
    require(p[1], 200);
  }
  end_test_case();

  test_case("itr(size_type): index-to-iterator");
  {
    fvi v{ 1, 2, 3 };
    int *p = v.itr(1);
    require(*p, 2);
  }
  end_test_case();

  test_case("at_n(iterator): iterator-to-index");
  {
    fvi v{ 1, 2, 3 };
    auto it = v.begin() + 2;
    require(v.at_n(it), usize(2));
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION                                                     //
  // ============================================================ //
  test_case("begin/end: range-for");
  {
    fvi v{ 1, 2, 3, 4 };
    int sum = 0;
    for ( auto x : v ) sum += x;
    require(sum, 10);
  }
  end_test_case();

  test_case("cbegin/cend");
  {
    fvi v{ 5, 10, 15 };
    int sum = 0;
    for ( auto it = v.cbegin(); it != v.cend(); ++it ) sum += *it;
    require(sum, 30);
  }
  end_test_case();

  test_case("last(): final element iterator");
  {
    fvi v{ 1, 2, 3 };
    auto l = v.last();
    require(*l, 3);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                      //
  // ============================================================ //
  test_case("size/max_size/empty");
  {
    fvi v;
    require_true(v.empty());
    v.push_back(1);
    require(v.size(), usize(1));
    require_greater(v.max_size(), usize(0));
    require_false(v.empty());
  }
  end_test_case();

  test_case("reserve: grows capacity");
  {
    fvi v;
    v.reserve(64);
    require_greater(v.max_size(), usize(0));
  }
  end_test_case();

  test_case("try_reserve: allocates");
  {
    fvi v;
    bool threw = false;
    try {
      v.try_reserve(32);
    } catch ( ... ) {
      threw = true;
    }
    require(threw, false);
    require_greater(v.max_size(), usize(0));
  }
  end_test_case();

  test_case("set_size: direct length mutation");
  {
    fvi v(10);
    v.set_size(5);
    require(v.size(), usize(5));
  }
  end_test_case();

  // ============================================================ //
  //  MODIFIERS                                                     //
  // ============================================================ //
  test_case("push_back(const T&)");
  {
    fvi v;
    int x = 11;
    v.push_back(x);
    require(v[0], 11);
  }
  end_test_case();

  test_case("push_back(T&&)");
  {
    fvi v;
    v.push_back(22);
    require(v[0], 22);
  }
  end_test_case();

  test_case("inline_push_back: no-grow path");
  {
    fvi v;
    v.reserve(16);
    int x = 5;
    v.inline_push_back(x);
    v.inline_push_back(6);
    require(v.size(), usize(2));
    require(v[1], 6);
  }
  end_test_case();

  test_case("emplace_back");
  {
    struct P {
      int a, b;

      P(int x, int y) : a(x), b(y) { }
    };

    micron::fvector<P> v;
    v.emplace_back(1, 2);
    require(v[0].a, 1);
    require(v[0].b, 2);
  }
  end_test_case();

  test_case("move_back(T&&)");
  {
    fvi v;
    int x = 77;
    v.move_back(micron::move(x));
    require(v[0], 77);
  }
  end_test_case();

  test_case("pop_back");
  {
    fvi v{ 1, 2, 3 };
    v.pop_back();
    require(v.size(), usize(2));
    require(v.back(), 2);
  }
  end_test_case();

  test_case("insert(size_type, const T&)");
  {
    fvi v{ 1, 2, 4 };
    int three = 3;
    v.insert(usize(2), three);
    require(v.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(v[i], i + 1);
  }
  end_test_case();

  test_case("insert(size_type, T&&)");
  {
    fvi v{ 1, 2, 4 };
    v.insert(usize(2), 3);
    require(v.size(), usize(4));
    require(v[2], 3);
  }
  end_test_case();

  test_case("insert(size_type, const T&, size_type cnt)");
  {
    fvi v{ 1, 5 };
    int two = 2;
    v.insert(usize(1), two, usize(3));
    require(v.size(), usize(5));
    require(v[0], 1);
    require(v[1], 2);
    require(v[2], 2);
    require(v[3], 2);
    require(v[4], 5);
  }
  end_test_case();

  test_case("insert(iterator, const T&)");
  {
    fvi v{ 1, 3, 4 };
    int two = 2;
    v.insert(v.begin() + 1, two);
    require(v.size(), usize(4));
    require(v[1], 2);
  }
  end_test_case();

  test_case("erase(size_type)");
  {
    fvi v{ 1, 2, 3, 4 };
    v.erase(usize(1));
    require(v.size(), usize(3));
    require(v[1], 3);
  }
  end_test_case();

  test_case("erase(iterator)");
  {
    fvi v{ 10, 20, 30 };
    v.erase(v.begin() + 1);
    require(v.size(), usize(2));
    require(v[1], 30);
  }
  end_test_case();

  test_case("erase(iterator, iterator)");
  {
    fvi v{ 1, 2, 3, 4, 5 };
    v.erase(v.begin() + 1, v.begin() + 4);
    require(v.size(), usize(2));
    require(v[0], 1);
    require(v[1], 5);
  }
  end_test_case();

  test_case("remove(value): drops all matches");
  {
    fvi v{ 1, 2, 3, 2, 4 };
    v.remove(2);
    require(v.size(), usize(3));
    require(v[0], 1);
    require(v[1], 3);
    require(v[2], 4);
  }
  end_test_case();

  // ============================================================ //
  //  AGGREGATION                                                   //
  // ============================================================ //
  test_case("append(const fvector&)");
  {
    fvi a{ 1, 2 };
    fvi b{ 3, 4 };
    a.append(b);
    require(a.size(), usize(4));
    require(a[3], 4);
  }
  end_test_case();

  test_case("weld(fvector&&)");
  {
    fvi a{ 1, 2 };
    fvi b{ 5, 6 };
    a.weld(micron::move(b));
    require(a.size(), usize(4));
    require(a[3], 6);
  }
  end_test_case();

  // ============================================================ //
  //  UTILITY                                                       //
  // ============================================================ //
  test_case("swap(fvector&)");
  {
    fvi a{ 1, 2 };
    fvi b{ 3, 4, 5 };
    a.swap(b);
    require(a.size(), usize(3));
    require(b.size(), usize(2));
  }
  end_test_case();

  test_case("clear");
  {
    fvi v{ 1, 2, 3 };
    v.clear();
    require_true(v.empty());
  }
  end_test_case();

  test_case("fast_clear");
  {
    fvi v{ 1, 2, 3 };
    v.fast_clear();
    require_true(v.empty());
  }
  end_test_case();

  test_case("fill");
  {
    fvi v(5);
    v.fill(42);
    for ( usize i = 0; i < 5; ++i ) require(v[i], 42);
  }
  end_test_case();

  test_case("resize: grow");
  {
    fvi v{ 1, 2 };
    v.resize(5);
    require(v.size(), usize(5));
  }
  end_test_case();

  test_case("resize(n, value): grow with init");
  {
    fvi v{ 1, 2 };
    v.resize(4, 9);
    require(v[3], 9);
  }
  end_test_case();

  test_case("clone");
  {
    fvi v{ 1, 2, 3 };
    auto c = v.clone();
    require(c.size(), usize(3));
    require(c[1], 2);
  }
  end_test_case();

  test_case("into_bytes");
  {
    fvi v{ 10, 20 };
    auto bs = v.into_bytes();
    require(bs.size(), usize(2 * sizeof(int)));
  }
  end_test_case();

  test_case("get / cget: unchecked iterators (fvector = fast)");
  {
    // fvector::get/cget are unchecked by design (consistent with at()).
    fvi v{ 1, 2, 3 };
    auto it = v.get(1);
    require(*it, 2);
    auto cit = v.cget(2);
    require(*cit, 3);
  }
  end_test_case();

  // ============================================================ //
  //  SEARCH                                                        //
  // ============================================================ //
  test_case("find: present and absent");
  {
    fvi v{ 1, 2, 3 };
    auto p = v.find(2);
    require_true(p != nullptr);
    require(*p, 2);
    require_true(v.find(99) == nullptr);
  }
  end_test_case();

  // ============================================================ //
  //  SORT                                                          //
  // ============================================================ //
  test_case("sort");
  {
    fvi v{ 5, 3, 1, 4, 2 };
    v.sort();
    for ( int i = 0; i < 5; ++i ) require(v[i], i + 1);
  }
  end_test_case();

  test_case("insert_sort");
  {
    fvi v;
    v.insert_sort(5);
    v.insert_sort(1);
    v.insert_sort(3);
    require(v[0], 1);
    require(v[1], 3);
    require(v[2], 5);
  }
  end_test_case();

  test_case("assign");
  {
    fvi v;
    v.assign(usize(4), 7);
    require(v.size(), usize(4));
    for ( usize i = 0; i < 4; ++i ) require(v[i], 7);
  }
  end_test_case();

  // ============================================================ //
  //  ALLOCATOR                                                     //
  // ============================================================ //
  test_case("tracking_allocator: no leak");
  {
    using A = mtest::tracking_allocator<31>;
    A::reset();
    {
      micron::fvector<int, A> v;
      for ( int i = 0; i < 50; ++i ) v.push_back(i);
    }
    require(A::outstanding(), i64(0));
  }
  end_test_case();

  print("=== ALL FVECTOR EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
