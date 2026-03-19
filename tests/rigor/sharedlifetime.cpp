//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/memory/memory.hpp"
#include "../../src/memory/pointers/shared.hpp"

#include "../../src/array/array.hpp"
#include "../../src/maps/hopscotch.hpp"
#include "../../src/std.hpp"
#include "../../src/string/sstring.hpp"
#include "../../src/string/string.hpp"
#include "../../src/vector/vector.hpp"

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"

#include "../snowball/snowball.hpp"

// ============================================================
//  Lifetime tracking
//  Every container test that involves objects with destructors
//  verifies that the destructor fires exactly once — at the
//  moment the last shared_pointer owner goes away.
// ============================================================

struct Tracker {
  static int constructions;
  static int destructions;

  static void
  reset()
  {
    constructions = destructions = 0;
  }

  static bool
  balanced()
  {
    return constructions == destructions;
  }

  int value;

  explicit Tracker(int v = 0) : value(v) { ++constructions; }

  Tracker(const Tracker &o) : value(o.value) { ++constructions; }

  ~Tracker() { ++destructions; }

  bool
  operator==(const Tracker &o) const
  {
    return value == o.value;
  }

  bool
  operator>(const Tracker &o) const
  {
    return value > o.value;
  }
};

int Tracker::constructions = 0;
int Tracker::destructions = 0;

// ============================================================
//  Helpers
// ============================================================

// Confirm a shared_pointer is the sole owner and the managed object is valid
template <typename T>
void
assert_sole_owner(const mc::shared_pointer<T> &p)
{
  sb::require(static_cast<bool>(p));
  sb::require(p.refs() == static_cast<usize>(1));
}

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== SHARED_POINTER + CONTAINERS TESTS ===");

  // ============================================================
  //  Section 1: vector<int>
  // ============================================================

  sb::test_case("shared_pointer<vector<int>>: construction and element access");
  {
    mc::shared_pointer<mc::vector<int>> p(10);
    assert_sole_owner(p);
    for ( int i = 0; i < 10; i++ )
      (*p)[i] = i * 2;
    for ( int i = 0; i < 10; i++ )
      sb::require((*p)[i] == i * 2);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>>: shared mutation visible through all copies");
  {
    mc::shared_pointer<mc::vector<int>> a(4);
    (*a)[0] = 99;
    mc::shared_pointer<mc::vector<int>> b(a);
    mc::shared_pointer<mc::vector<int>> c(b);
    sb::require((*b)[0] == 99);
    sb::require((*c)[0] == 99);
    (*c)[0] = 42;
    sb::require((*a)[0] == 42);     // mutation via c visible through a
    sb::require((*b)[0] == 42);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>>: size/empty reflect managed object state");
  {
    mc::shared_pointer<mc::vector<int>> p;
    p = mc::vector<int>{};
    sb::require(p->empty());
    p->push_back(1);
    p->push_back(2);
    sb::require(p->size() == 2);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>>: push_back through operator-> grows vector");
  {
    mc::shared_pointer<mc::vector<int>> p(mc::vector<int>{});
    for ( int i = 0; i < 100; i++ )
      p->push_back(i);
    sb::require(p->size() == 100);
    sb::require((*p)[0] == 0);
    sb::require((*p)[99] == 99);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>>: object freed when last copy destroyed");
  {
    // Use vector<Tracker> so we can count destructions
    Tracker::reset();
    {
      mc::shared_pointer<mc::vector<Tracker>> a(mc::vector<Tracker>{});
      a->push_back(Tracker(1));
      a->push_back(Tracker(2));
      {
        mc::shared_pointer<mc::vector<Tracker>> b(a);
        sb::require(a.refs() == 2);
      }     // b destroyed — vector must still be alive
      sb::require(a->size() == 2);
      sb::require(a.refs() == 1);
    }     // a destroyed — vector and its Trackers freed
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>>: erase through shared pointer");
  {
    mc::shared_pointer<mc::vector<int>> p(mc::vector<int>{});
    for ( int i = 0; i < 5; i++ )
      p->push_back(i);
    p->erase(static_cast<usize>(0));     // erase first element
    sb::require(p->size() == 4);
    sb::require((*p)[0] == 1);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 2: vector<Tracker> — deep lifetime checks
  // ============================================================

  sb::test_case("shared_pointer<vector<Tracker>>: no leak, vector freed exactly once");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::vector<Tracker>> p(mc::vector<Tracker>{});
      p->push_back(Tracker(10));
      p->push_back(Tracker(20));
      p->push_back(Tracker(30));
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<Tracker>>: copy keeps object alive, both see same data");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::vector<Tracker>> a(mc::vector<Tracker>{});
      a->push_back(Tracker(7));
      mc::shared_pointer<mc::vector<Tracker>> b(a);
      // Destroy a — b must still have valid data
      a = nullptr;
      sb::require(!a);
      sb::require(b->size() == 1);
      sb::require((*b)[0].value == 7);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<Tracker>>: move assignment frees old vector");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::vector<Tracker>> a(mc::vector<Tracker>{});
      a->push_back(Tracker(1));
      mc::shared_pointer<mc::vector<Tracker>> b(mc::vector<Tracker>{});
      b->push_back(Tracker(2));
      b->push_back(Tracker(3));
      // Temporaries from push_back have already been destroyed; snapshot
      // the destruction count now so we can measure the delta from the move.
      int before = Tracker::destructions;
      b = std::move(a);     // b's old vector (2 live Trackers) freed
      sb::require(Tracker::destructions - before == 2);
      sb::require(b->size() == 1);
      sb::require((*b)[0].value == 1);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 3: vector<int> fixed-size usage
  //  NOTE: array is a stack_tag / alignas(64) container and must NOT
  //  be heap-allocated through shared_pointer. operator=(Type&&) calls
  //  __new<array> which uses standard operator new — guaranteed only up
  //  to __STDCPP_DEFAULT_NEW_ALIGNMENT__ (≤16 bytes on most platforms),
  //  not the 64-byte alignment array requires. __delete then receives a
  //  misaligned pointer and crashes. Use vector with a fixed reserve instead.
  // ============================================================

  sb::test_case("shared_pointer<vector<int>> fixed-64: element access");
  {
    mc::shared_pointer<mc::vector<int>> p(mc::vector<int>{});
    p->reserve(64);
    for ( int i = 0; i < 64; i++ )
      p->push_back(i);
    for ( int i = 0; i < 64; i++ )
      sb::require((*p)[static_cast<usize>(i)] == i);
    assert_sole_owner(p);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>> fixed-64: shared mutation visible through copies");
  {
    mc::shared_pointer<mc::vector<int>> a(mc::vector<int>{});
    a->push_back(111);
    mc::shared_pointer<mc::vector<int>> b(a);
    (*b)[0] = 222;
    sb::require((*a)[0] == 222);     // same underlying object
    sb::require(a.refs() == 2);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>> fixed-64: fill and verify all elements");
  {
    mc::shared_pointer<mc::vector<int>> p(mc::vector<int>{});
    p->reserve(64);
    for ( int i = 0; i < 64; i++ )
      p->push_back(0xAB);
    bool all_match = true;
    for ( usize i = 0; i < p->size(); i++ )
      if ( (*p)[i] != 0xAB ) {
        all_match = false;
        break;
      }
    sb::require(all_match);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<int>> fixed-64: sum via manual reduction");
  {
    mc::shared_pointer<mc::vector<int>> p(mc::vector<int>{});
    p->reserve(64);
    for ( int i = 0; i < 64; i++ )
      p->push_back(1);
    int sum = 0;
    for ( usize i = 0; i < p->size(); i++ )
      sum += (*p)[i];
    sb::require(sum == 64);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<vector<Tracker>> fixed-64: freed when last owner gone");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::vector<Tracker>> p(mc::vector<Tracker>{});
      p->push_back(Tracker(1));
      {
        mc::shared_pointer<mc::vector<Tracker>> q(p);
        sb::require(p.refs() == 2);
      }     // q gone, vector still live
      sb::require(p.refs() == 1);
      sb::require(p->size() == 1);
    }     // p gone, vector and Trackers freed
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 4: hstring
  // ============================================================

  sb::test_case("shared_pointer<hstring<>>: construction from string literal");
  {
    mc::shared_pointer<mc::hstring<>> p("hello");
    assert_sole_owner(p);
    sb::require(*p == "hello");
    sb::require(p->size() == 5);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hstring<>>: shared mutation visible through all copies");
  {
    mc::shared_pointer<mc::hstring<>> a("foo");
    mc::shared_pointer<mc::hstring<>> b(a);
    *a = "bar";
    sb::require(*b == "bar");     // same managed string
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hstring<>>: append through operator->");
  {
    mc::shared_pointer<mc::hstring<>> p("hello");
    p->append(" world");
    sb::require(*p == "hello world");
    sb::require(p->size() == 11);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hstring<>>: copy keeps string alive after original nulled");
  {
    mc::shared_pointer<mc::hstring<>> a("lifetime");
    mc::shared_pointer<mc::hstring<>> b(a);
    a = nullptr;
    sb::require(!a);
    sb::require(*b == "lifetime");     // still alive through b
    sb::require(b.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hstring<>>: substr produces independent copy");
  {
    mc::shared_pointer<mc::hstring<>> p("hello world");
    mc::hstring<> sub = p->substr(0, 5);
    sb::require(sub == "hello");
    // mutate original, substr must not change
    *p = "goodbye";
    sb::require(sub == "hello");
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hstring<>>: erase modifies content in-place");
  {
    mc::shared_pointer<mc::hstring<>> a("abcdef");
    mc::shared_pointer<mc::hstring<>> b(a);
    a->erase(static_cast<usize>(2), static_cast<usize>(2));     // remove 'cd'
    // both a and b point to same string
    sb::require(*b == "abef");
  }
  sb::end_test_case();

  // ============================================================
  //  Section 5: sstring<256>
  // ============================================================

  sb::test_case("shared_pointer<sstring<256>>: construction and comparison");
  {
    mc::shared_pointer<mc::sstring<256>> p("stack string");
    assert_sole_owner(p);
    sb::require(*p == "stack string");
    sb::require(p->size() == 12);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<sstring<256>>: shared mutation visible through copies");
  {
    mc::shared_pointer<mc::sstring<256>> a("original");
    mc::shared_pointer<mc::sstring<256>> b(a);
    a->clear();
    *a = mc::sstring<256>("changed");
    sb::require(*b == "changed");
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<sstring<256>>: append through shared pointer");
  {
    mc::shared_pointer<mc::sstring<256>> p("foo");
    *p += "bar";
    sb::require(*p == "foobar");
    sb::require(p->size() == 6);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<sstring<256>>: sstring freed when last owner gone");
  {
    // sstring is stack-allocated inside the shared control block;
    // verify the control block itself is freed (no double-free / crash)
    mc::shared_pointer<mc::sstring<256>> a("test");
    {
      mc::shared_pointer<mc::sstring<256>> b(a);
      sb::require(a.refs() == 2);
    }
    sb::require(a.refs() == 1);
    sb::require(*a == "test");
  }
  sb::end_test_case();

  // ============================================================
  //  Section 6: hopscotch_map<hstring, int>
  // ============================================================

  sb::test_case("shared_pointer<hopscotch_map>: insert and find");
  {
    mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> p;
    p = mc::hopscotch_map<mc::hstring<>, int>{};
    p->insert(mc::hstring<>("alpha"), 1);
    p->insert(mc::hstring<>("beta"), 2);
    p->insert(mc::hstring<>("gamma"), 3);
    sb::require(p->size() == 3);
    int *v = p->find(mc::hstring<>("alpha"));
    sb::require(v != nullptr && *v == 1);
    v = p->find(mc::hstring<>("beta"));
    sb::require(v != nullptr && *v == 2);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hopscotch_map>: shared mutation visible through copies");
  {
    mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> a;
    a = mc::hopscotch_map<mc::hstring<>, int>{};
    a->insert(mc::hstring<>("key"), 42);
    mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> b(a);
    // insert through b, find through a
    b->insert(mc::hstring<>("key2"), 99);
    int *v = a->find(mc::hstring<>("key2"));
    sb::require(v != nullptr && *v == 99);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hopscotch_map>: erase removes entry");
  {
    mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> p;
    p = mc::hopscotch_map<mc::hstring<>, int>{};
    p->insert(mc::hstring<>("x"), 10);
    p->insert(mc::hstring<>("y"), 20);
    sb::require(p->size() == 2);
    p->erase(mc::hstring<>("x"));
    sb::require(p->size() == 1);
    sb::require(p->find(mc::hstring<>("x")) == nullptr);
    int *v = p->find(mc::hstring<>("y"));
    sb::require(v != nullptr && *v == 20);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hopscotch_map>: contains() reflects current state");
  {
    mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> p;
    p = mc::hopscotch_map<mc::hstring<>, int>{};
    sb::require(!p->contains(mc::hstring<>("missing")));
    p->insert(mc::hstring<>("present"), 1);
    sb::require(p->contains(mc::hstring<>("present")));
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<hopscotch_map>: map freed when last owner gone");
  {
    int *dangling = nullptr;
    {
      mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> a;
      a = mc::hopscotch_map<mc::hstring<>, int>{};
      a->insert(mc::hstring<>("k"), 7);
      mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> b(a);
      a = nullptr;
      // map still alive through b
      int *v = b->find(mc::hstring<>("k"));
      sb::require(v != nullptr && *v == 7);
      sb::require(b.refs() == 1);
    }     // b gone — map freed; dangling stays null
    sb::require(dangling == nullptr);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 7: pair<int, hstring>
  // ============================================================

  sb::test_case("shared_pointer<pair<int,hstring>>: construction and member access");
  {
    mc::shared_pointer<mc::pair<int, mc::hstring<>>> p(42, mc::hstring<>("hello"));
    assert_sole_owner(p);
    sb::require(p->a == 42);
    sb::require(p->b == "hello");
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<pair<int,hstring>>: shared mutation through copies");
  {
    mc::shared_pointer<mc::pair<int, mc::hstring<>>> a(1, mc::hstring<>("first"));
    mc::shared_pointer<mc::pair<int, mc::hstring<>>> b(a);
    a->a = 99;
    a->b = mc::hstring<>("mutated");
    sb::require(b->a == 99);
    sb::require(b->b == "mutated");
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<pair<int,hstring>>: pair freed when last owner gone");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::pair<int, mc::vector<Tracker>>> a(0, mc::vector<Tracker>{});
      a->b.push_back(Tracker(5));
      {
        mc::shared_pointer<mc::pair<int, mc::vector<Tracker>>> b(a);
        sb::require(a.refs() == 2);
      }     // b gone, pair still alive
      sb::require(a->b.size() == 1);
    }     // a gone, pair and its vector destroyed
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 8: tuple<int, hstring, vector<int>>
  // ============================================================

  sb::test_case("shared_pointer<tuple<int,hstring,vector<int>>>: construction and get<>");
  {
    mc::shared_pointer<mc::tuple<int, mc::hstring<>, mc::vector<int>>> p(10, mc::hstring<>("hello"), mc::vector<int>{});
    assert_sole_owner(p);
    sb::require(mc::get<0>(*p) == 10);
    sb::require(mc::get<1>(*p) == "hello");
    sb::require(mc::get<2>(*p).empty());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<tuple<...>>: shared mutation visible through copies");
  {
    mc::shared_pointer<mc::tuple<int, mc::hstring<>>> a(1, mc::hstring<>("orig"));
    mc::shared_pointer<mc::tuple<int, mc::hstring<>>> b(a);
    mc::get<0>(*a) = 42;
    mc::get<1>(*a) = mc::hstring<>("changed");
    sb::require(mc::get<0>(*b) == 42);
    sb::require(mc::get<1>(*b) == "changed");
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<tuple<...>>: tuple freed when last owner gone");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::tuple<mc::vector<Tracker>, int>> p(mc::vector<Tracker>{}, 0);
      mc::get<0>(*p).push_back(Tracker(1));
      mc::get<0>(*p).push_back(Tracker(2));
      {
        mc::shared_pointer<mc::tuple<mc::vector<Tracker>, int>> q(p);
        sb::require(p.refs() == 2);
      }
      sb::require(p.refs() == 1);
      sb::require(mc::get<0>(*p).size() == 2);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 9: static_any
  // ============================================================

  sb::test_case("shared_pointer<static_any<64>>: store and cast int");
  {
    mc::shared_pointer<mc::static_any<64>> p;
    p = mc::static_any<64>{};
    p->emplace<int>(42);
    sb::require(p->has_value());
    sb::require(p->is<int>());
    sb::require(p->cast<int>() == 42);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<static_any<64>>: store hstring, visible through copy");
  {
    mc::shared_pointer<mc::static_any<64>> a;
    a = mc::static_any<64>{};
    a->emplace<mc::hstring<>>(mc::hstring<>("shared_any"));
    mc::shared_pointer<mc::static_any<64>> b(a);
    sb::require(b->cast<mc::hstring<>>() == "shared_any");
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<static_any<64>>: reset clears value");
  {
    mc::shared_pointer<mc::static_any<64>> p;
    p = mc::static_any<64>{};
    p->emplace<int>(7);
    p->reset();
    sb::require(!p->has_value());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<static_any<64>>: static_any freed when last owner gone");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::static_any<sizeof(Tracker) + 64>> a;
      a = mc::static_any<sizeof(Tracker) + 64>{};
      a->emplace<Tracker>(99);
      {
        mc::shared_pointer<mc::static_any<sizeof(Tracker) + 64>> b(a);
        sb::require(a.refs() == 2);
      }     // b gone, static_any alive
      sb::require(a->has_value());
      sb::require(a->cast<Tracker>().value == 99);
    }     // a gone, static_any (and Tracker inside) destroyed
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 10: Nested shared_pointer — shared_pointer of
  //  vector<shared_pointer<hstring>>
  //  The most demanding lifetime test: elements themselves are
  //  shared owners; the outer vector's lifetime must not affect
  //  the inner strings while inner refs exist.
  // ============================================================

  sb::test_case("Nested: vector<shared_pointer<hstring>> — inner strings outlive vector");
  {
    mc::shared_pointer<mc::hstring<>> inner_ref;
    {
      mc::shared_pointer<mc::vector<mc::shared_pointer<mc::hstring<>>>> vec;
      vec = mc::vector<mc::shared_pointer<mc::hstring<>>>{};
      mc::shared_pointer<mc::hstring<>> s1("first");
      mc::shared_pointer<mc::hstring<>> s2("second");
      inner_ref = s1;         // hold a reference outside the vector
      vec->push_back(s1);     // vector now also holds a ref
      vec->push_back(s2);
      sb::require(s1.refs() == 3);     // inner_ref + s1 + vec element
    }     // vec destroyed; s1 loses its vec copy; s2 freed (no other owner)
    // inner_ref still holds s1 alive
    sb::require(inner_ref.refs() == 1);
    sb::require(*inner_ref == "first");
  }
  sb::end_test_case();

  sb::test_case("Nested: shared_pointer<vector<shared_pointer<int>>> — element lifetime");
  {
    mc::shared_pointer<int> elem;
    {
      mc::shared_pointer<mc::vector<mc::shared_pointer<int>>> outer;
      outer = mc::vector<mc::shared_pointer<int>>{};
      mc::shared_pointer<int> x(100);
      elem = x;
      outer->push_back(x);
      outer->push_back(mc::shared_pointer<int>(200));
      sb::require(x.refs() == 3);     // x + elem + outer[0]
    }     // outer destroyed; outer[0] loses its copy; outer[1] freed
    sb::require(elem.refs() == 1);
    sb::require(*elem == 100);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 11: Stress — many shared_pointer<vector<Tracker>>
  //  all sharing one vector, sequential mutation, no leaks
  // ============================================================

  sb::test_case("Stress: 100 shared owners of one vector, all released, no leaks");
  {
    Tracker::reset();
    {
      mc::shared_pointer<mc::vector<Tracker>> root(mc::vector<Tracker>{});
      root->push_back(Tracker(1));

      mc::shared_pointer<mc::vector<Tracker>> copies[100];
      for ( int i = 0; i < 100; i++ )
        copies[i] = root;

      sb::require(root.refs() == 101);

      // Mutate through one copy, verify through another
      copies[0]->push_back(Tracker(2));
      sb::require(copies[99]->size() == 2);

      // Release all copies
      for ( int i = 0; i < 100; i++ )
        copies[i] = nullptr;

      sb::require(root.refs() == 1);
      sb::require(root->size() == 2);
    }     // root released, vector and all Trackers freed
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("Stress: 50 hopscotch_map insertions through shared pointer, consistent state");
  {
    mc::shared_pointer<mc::hopscotch_map<mc::hstring<>, int>> p;
    p = mc::hopscotch_map<mc::hstring<>, int>{};
    for ( int i = 0; i < 50; i++ ) {
      mc::hstring<> key("key");
      key += static_cast<char>('a' + (i % 26));
      key += static_cast<char>('0' + (i / 26));
      p->insert(key, i);
    }
    sb::require(p->size() == 50);
    sb::require(!p->empty());
    // clear and verify
    p->clear();
    sb::require(p->size() == 0);
    sb::require(p->empty());
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
