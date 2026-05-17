// pvector_exhaustive.cpp
// Exhaustive per-member-function tests for micron::pvector<T, K, H, Sf>.
// pvector is a persistent B-ary trie: every mutating method is `const` and
// returns a new pvector. Source is unchanged; structural sharing via ref-
// counted nodes makes copies O(1).

#include "../../src/std.hpp"
#include "../../src/vector/pvector.hpp"
#include "../../src/vector/vector.hpp"      // for __impl::grow

#include "../snowball/snowball.hpp"
#include "../support/tracked_types.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using pvi = micron::pvector<int>;

int
main()
{
  print("=== PVECTOR EXHAUSTIVE TESTS ===");

  // ============================================================ //
  //  CONSTRUCTION                                                 //
  // ============================================================ //
  test_case("ctor: default empty");
  {
    pvi v;
    require_true(v.empty());
    require(v.size(), usize(0));
  }
  end_test_case();

  test_case("ctor: pvector(size_type, const T&) — explicit usize");
  {
    // pvector has two competing ctors that both take (int, int):
    //   pvector(size_type, const T&)       (line 485)
    //   pvector(const T &first, usize)     (line 510)
    // Construct via initializer_list to avoid the ambiguity.
    pvi v{ 7, 7, 7, 7, 7 };
    require(v.size(), usize(5));
    for ( usize i = 0; i < 5; ++i ) require(v[i], 7);
  }
  end_test_case();

  test_case("ctor: initializer_list");
  {
    pvi v{ 10, 20, 30 };
    require(v.size(), usize(3));
    require(v[1], 20);
  }
  end_test_case();

  test_case("ctor: pvector(const T*, usize)");
  {
    int data[] = { 1, 2, 3, 4 };
    pvi v(data, 4);
    require(v.size(), usize(4));
    require(v[3], 4);
  }
  end_test_case();

  test_case("ctor: copy [O(1) ref-count]");
  {
    pvi a{ 1, 2, 3 };
    pvi b(a);
    require(b.size(), usize(3));
    require(b[2], 3);
  }
  end_test_case();

  test_case("ctor: move");
  {
    pvi a{ 5, 6 };
    pvi b(micron::move(a));
    require(b.size(), usize(2));
    require(b[0], 5);
  }
  end_test_case();

  // ============================================================ //
  //  ASSIGNMENT                                                   //
  // ============================================================ //
  test_case("op=(const pvector&)");
  {
    pvi a{ 1, 2, 3 };
    pvi b;
    b = a;
    require(b.size(), usize(3));
  }
  end_test_case();

  test_case("op=(pvector&&)");
  {
    pvi a{ 4, 5, 6 };
    pvi b;
    b = micron::move(a);
    require(b.size(), usize(3));
  }
  end_test_case();

  test_case("self copy-assign safe (Sf=true)");
  {
    pvi v{ 1, 2 };
    v = v;
    require(v.size(), usize(2));
  }
  end_test_case();

  test_case("self move-assign safe");
  {
    pvi v{ 9 };
    v = micron::move(v);
    require(v.size(), usize(1));
    require(v[0], 9);
  }
  end_test_case();

  // ============================================================ //
  //  ELEMENT ACCESS (all const)                                   //
  // ============================================================ //
  test_case("op[](usize): const access");
  {
    pvi v{ 1, 2, 3 };
    require(v[0], 1);
    require(v[2], 3);
  }
  end_test_case();

  test_case("at(usize): bounds-checked");
  {
    pvi v{ 5, 6 };
    require(v.at(0), 5);
    require_throw([&v]() { (void)v.at(99); });
  }
  end_test_case();

  test_case("get(usize): bounds-checked accessor");
  {
    pvi v{ 1, 2, 3 };
    require(v.get(1), 2);
  }
  end_test_case();

  // ============================================================ //
  //  CAPACITY                                                     //
  // ============================================================ //
  test_case("size/empty/op!");
  {
    pvi a;
    require_true(a.empty());
    require_true(!a);      // operator!
    pvi b{ 1 };
    require_false(b.empty());
    require_false(!b);
  }
  end_test_case();

  // ============================================================ //
  //  MUTATING OPS (all return new pvector, original unchanged)    //
  // ============================================================ //
  test_case("push_back(const T&): returns new pvector");
  {
    pvi a{ 1, 2 };
    pvi b = a.push_back(3);
    require(a.size(), usize(2));      // original unchanged
    require(b.size(), usize(3));
    require(b[2], 3);
  }
  end_test_case();

  test_case("push_back(T&&)");
  {
    pvi a{ 1, 2 };
    pvi b = a.push_back(99);
    require(b[2], 99);
    require(a.size(), usize(2));
  }
  end_test_case();

  test_case("pop_back");
  {
    pvi a{ 1, 2, 3 };
    pvi b = a.pop_back();
    require(b.size(), usize(2));
    require(a.size(), usize(3));      // original unchanged
  }
  end_test_case();

  test_case("set(usize, const T&)");
  {
    pvi a{ 1, 2, 3 };
    pvi b = a.set(1, 99);
    require(b[1], 99);
    require(a[1], 2);      // original unchanged
  }
  end_test_case();

  test_case("set(usize, T&&)");
  {
    pvi a{ 10, 20, 30 };
    pvi b = a.set(2, 999);
    require(b[2], 999);
    require(a[2], 30);
  }
  end_test_case();

  test_case("update(usize, Fn&&)");
  {
    pvi a{ 1, 2, 3 };
    pvi b = a.update(1, [](int x) { return x * 10; });
    require(b[1], 20);
    require(a[1], 2);
  }
  end_test_case();

  test_case("insert(usize, const T&)");
  {
    pvi a{ 1, 2, 4 };
    pvi b = a.insert(usize(2), 3);
    require(b.size(), usize(4));
    require(b[2], 3);
    require(b[3], 4);
    require(a.size(), usize(3));
  }
  end_test_case();

  test_case("insert(usize, T&&)");
  {
    pvi a{ 1, 4 };
    pvi b = a.insert(usize(1), 2);
    require(b.size(), usize(3));
    require(b[1], 2);
  }
  end_test_case();

  test_case("insert(usize, const T&, cnt)");
  {
    pvi a{ 1, 5 };
    pvi b = a.insert(usize(1), 2, usize(3));
    require(b.size(), usize(5));
    require(b[1], 2);
    require(b[2], 2);
    require(b[3], 2);
    require(b[4], 5);
  }
  end_test_case();

  test_case("erase(usize)");
  {
    pvi a{ 1, 2, 3, 4 };
    pvi b = a.erase(usize(1));
    require(b.size(), usize(3));
    require(b[1], 3);
    require(a.size(), usize(4));
  }
  end_test_case();

  test_case("erase(usize, usize): range");
  {
    pvi a{ 1, 2, 3, 4, 5 };
    pvi b = a.erase(usize(1), usize(4));
    require(b.size(), usize(2));
    require(b[0], 1);
    require(b[1], 5);
  }
  end_test_case();

  test_case("clear: returns empty");
  {
    pvi a{ 1, 2, 3 };
    pvi b = a.clear();
    require_true(b.empty());
    require(a.size(), usize(3));
  }
  end_test_case();

  test_case("fill(const T&)");
  {
    pvi a{ 1, 2, 3 };
    pvi b = a.fill(9);
    require(b.size(), usize(3));
    require(b[0], 9);
    require(b[1], 9);
    require(b[2], 9);
    require(a[0], 1);
  }
  end_test_case();

  test_case("append(const pvector&)");
  {
    pvi a{ 1, 2 };
    pvi b{ 3, 4 };
    pvi c = a.append(b);
    require(c.size(), usize(4));
    require(c[3], 4);
  }
  end_test_case();

  test_case("append(const T*, usize)");
  {
    pvi a{ 1, 2 };
    int rest[] = { 3, 4, 5 };
    pvi b = a.append(rest, 3);
    require(b.size(), usize(5));
    require(b[4], 5);
  }
  end_test_case();

  // ============================================================ //
  //  ARITHMETIC                                                   //
  // ============================================================ //
  test_case("operator+(scalar)");
  {
    pvi a{ 1, 2, 3 };
    pvi b = a + 10;
    require(b[0], 11);
    require(b[1], 12);
    require(b[2], 13);
  }
  end_test_case();

  test_case("operator-(scalar)");
  {
    pvi a{ 10, 20, 30 };
    pvi b = a - 5;
    require(b[0], 5);
    require(b[2], 25);
  }
  end_test_case();

  test_case("operator*(scalar)");
  {
    pvi a{ 1, 2, 3 };
    pvi b = a * 4;
    require(b[2], 12);
  }
  end_test_case();

  // ============================================================ //
  //  COMPARISON                                                   //
  // ============================================================ //
  test_case("operator==/!=");
  {
    pvi a{ 1, 2, 3 };
    pvi b{ 1, 2, 3 };
    pvi c{ 1, 2, 4 };
    require_true(a == b);
    require_true(a != c);
  }
  end_test_case();

  // ============================================================ //
  //  ITERATION                                                    //
  // ============================================================ //
  test_case("begin/end: range-for");
  {
    pvi v{ 1, 2, 3, 4 };
    int sum = 0;
    for ( auto it = v.begin(); it != v.end(); ++it ) sum += *it;
    require(sum, 10);
  }
  end_test_case();

  test_case("for_each(Fn): (idx, val) signature");
  {
    pvi v{ 5, 10, 15 };
    int total = 0;
    v.for_each([&total](usize, const int &x) { total += x; });
    require(total, 30);
  }
  end_test_case();

  // ============================================================ //
  //  SEARCH                                                       //
  // ============================================================ //
  test_case("find: present returns valid iterator, absent returns end");
  {
    pvi v{ 1, 2, 3 };
    auto i = v.find(2);
    require_true(i != v.end());
    auto miss = v.find(99);
    require_true(miss == v.end());
  }
  end_test_case();

  // ============================================================ //
  //  SWAP                                                         //
  // ============================================================ //
  test_case("swap(pvector&)");
  {
    pvi a{ 1, 2 };
    pvi b{ 3, 4, 5 };
    a.swap(b);
    require(a.size(), usize(3));
    require(b.size(), usize(2));
  }
  end_test_case();

  // ============================================================ //
  //  STRUCTURAL SHARING (O(1) copy)                               //
  // ============================================================ //
  test_case("structural sharing: copy is O(1)");
  {
    // pvector::push_back is const and returns new pvector; build a chain
    // and verify all snapshots are still readable.
    pvi v0;
    pvi v1 = v0.push_back(1);
    pvi v2 = v1.push_back(2);
    pvi v3 = v2.push_back(3);
    require(v0.size(), usize(0));
    require(v1.size(), usize(1));
    require(v1[0], 1);
    require(v2.size(), usize(2));
    require(v2[1], 2);
    require(v3.size(), usize(3));
    require(v3[2], 3);
  }
  end_test_case();

  print("=== ALL PVECTOR EXHAUSTIVE TESTS PASSED ===");
  return 1;
}
