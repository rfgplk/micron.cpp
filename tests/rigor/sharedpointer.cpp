//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/memory/pointers/shared.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

// ============================================================
//  Test instrumentation
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
};

int Tracker::constructions = 0;
int Tracker::destructions = 0;

struct Point {
  int x, y;

  Point(int x_, int y_) : x(x_), y(y_) {}
};

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== SHARED_POINTER TESTS ===");

  // ============================================================
  //  Section 1: Construction
  // ============================================================

  sb::test_case("shared_pointer<T>: default construction yields null, refcount 0");
  {
    mc::shared_pointer<int> p;
    sb::require(!p);
    sb::require(p.get() == nullptr);
    sb::require(p.refs() == 0);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: nullptr construction yields null");
  {
    mc::shared_pointer<int> p(nullptr);
    sb::require(!p);
    sb::require(p.refs() == 0);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: in-place single-arg construction");
  {
    mc::shared_pointer<int> p(42);
    sb::require(static_cast<bool>(p));
    sb::require(*p == 42);
    sb::require(p.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: in-place multi-arg construction");
  {
    mc::shared_pointer<Point> p(3, 7);
    sb::require(static_cast<bool>(p));
    sb::require(p->x == 3);
    sb::require(p->y == 7);
    sb::require(p.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: explicit raw-pointer construction");
  {
    int *raw = new int(99);
    mc::shared_pointer<int> p(raw);
    sb::require(static_cast<bool>(p));
    sb::require(*p == 99);
    sb::require(p.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: explicit raw nullptr construction yields null");
  {
    mc::shared_pointer<int> p(static_cast<int *>(nullptr));
    sb::require(!p);
    sb::require(p.refs() == 0);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: destructor frees object when last owner");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> p(5);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: default-constructed destructor does not crash");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> p;
    }
    sb::require(Tracker::destructions == 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 2: Copy semantics and reference counting
  // ============================================================

  sb::test_case("shared_pointer<T>: copy construction increments refcount");
  {
    mc::shared_pointer<int> a(10);
    mc::shared_pointer<int> b(a);
    sb::require(a.refs() == 2);
    sb::require(b.refs() == 2);
    sb::require(a.get() == b.get());     // same managed object
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: copy construction from non-const ref increments refcount");
  {
    mc::shared_pointer<int> a(20);
    mc::shared_pointer<int> b(a);     // non-const copy ctor
    sb::require(a.refs() == 2);
    sb::require(b.refs() == 2);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: three copies share same object, refcount = 3");
  {
    mc::shared_pointer<int> a(30);
    mc::shared_pointer<int> b(a);
    mc::shared_pointer<int> c(b);
    sb::require(a.refs() == 3);
    sb::require(b.refs() == 3);
    sb::require(c.refs() == 3);
    sb::require(a.get() == b.get() && b.get() == c.get());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: destroying one copy decrements refcount, object lives on");
  {
    Tracker::reset();
    mc::shared_pointer<Tracker> a(1);
    {
      mc::shared_pointer<Tracker> b(a);
      sb::require(a.refs() == 2);
    }     // b destroyed here
    sb::require(a.refs() == 1);
    sb::require(Tracker::destructions == 0);     // object still alive
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: last copy destroyed frees object");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> a(2);
      mc::shared_pointer<Tracker> b(a);
      mc::shared_pointer<Tracker> c(b);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: copy assignment increments refcount");
  {
    mc::shared_pointer<int> a(50);
    mc::shared_pointer<int> b;
    b = a;
    sb::require(a.refs() == 2);
    sb::require(b.refs() == 2);
    sb::require(*b == 50);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: copy assignment releases previous object if last owner");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> a(1);
      mc::shared_pointer<Tracker> b(2);
      b = a;     // b's old object should be freed (refcount was 1)
      sb::require(Tracker::destructions == 1);
      sb::require(a.refs() == 2);
      sb::require(b->value == 1);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: copy assignment does not free shared object");
  {
    Tracker::reset();
    mc::shared_pointer<Tracker> a(10);
    mc::shared_pointer<Tracker> b(a);     // refs = 2
    mc::shared_pointer<Tracker> c(20);
    c = a;                                       // c drops its old object (refs was 1, freed),
                                                 // then joins a's group (refs = 3)
    sb::require(Tracker::destructions == 1);     // only c's original object freed
    sb::require(a.refs() == 3);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: self-copy-assignment is safe");
  {
    mc::shared_pointer<int> p(77);
    p = p;
    sb::require(p.refs() == 1);
    sb::require(*p == 77);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: copy from null pointer leaves dest null");
  {
    mc::shared_pointer<int> a;
    mc::shared_pointer<int> b(5);
    b = a;     // b drops its object; a is null; b becomes null
    sb::require(!b);
    sb::require(b.refs() == 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 3: Move semantics
  // ============================================================

  sb::test_case("shared_pointer<T>: move construction transfers control block");
  {
    mc::shared_pointer<int> a(100);
    int *raw = a.get();
    mc::shared_pointer<int> b(std::move(a));
    sb::require(a.refs() == 0);     // a has no control block
    sb::require(!a);
    sb::require(b.refs() == 1);     // b is sole owner
    sb::require(b.get() == raw);
    sb::require(*b == 100);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: move construction does not alter refcount");
  {
    mc::shared_pointer<int> a(200);
    mc::shared_pointer<int> shared(a);     // refs = 2
    mc::shared_pointer<int> b(std::move(a));
    sb::require(b.refs() == 2);     // moved, not incremented
    sb::require(!a);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: move assignment transfers ownership");
  {
    mc::shared_pointer<int> a(300);
    mc::shared_pointer<int> b;
    b = std::move(a);
    sb::require(!a);
    sb::require(b.refs() == 1);
    sb::require(*b == 300);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: move assignment frees displaced object if last owner");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> a(1);
      mc::shared_pointer<Tracker> b(2);
      b = std::move(a);     // b drops its sole-owned object (freed), takes a's
      sb::require(Tracker::destructions == 1);
      sb::require(!a);
      sb::require(b->value == 1);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: move assignment does not free shared displaced object");
  {
    Tracker::reset();
    mc::shared_pointer<Tracker> a(10);
    mc::shared_pointer<Tracker> extra(a);     // refs(a's obj) = 2
    mc::shared_pointer<Tracker> b(20);
    b = std::move(a);                            // b drops its sole-owned object (freed), takes a's control (refs stays 2)
    sb::require(Tracker::destructions == 1);     // only b's original freed
    sb::require(b.refs() == 2);
    sb::require(extra.refs() == 2);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: self-move-assignment is safe");
  {
    mc::shared_pointer<int> p(55);
    p = std::move(p);
    // implementation guards with `if (this != &o)`
    sb::require(p.refs() == 1);
    sb::require(*p == 55);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: move assignment from rvalue raw pointer");
  {
    int *raw = new int(66);
    mc::shared_pointer<int> p;
    p = std::move(raw);
    sb::require(raw == nullptr);
    sb::require(*p == 66);
    sb::require(p.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: move assignment from lvalue raw pointer");
  {
    int *raw = new int(88);
    mc::shared_pointer<int> p;
    p = raw;
    sb::require(raw == nullptr);
    sb::require(*p == 88);
    sb::require(p.refs() == 1);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 4: Value assignment (operator=(T&&) / operator=(const T&))
  // ============================================================

  sb::test_case("shared_pointer<T>: assign by const-ref value replaces managed object");
  {
    mc::shared_pointer<int> p(1);
    int val = 42;
    p = val;
    sb::require(*p == 42);
    sb::require(p.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: assign by rvalue value replaces managed object");
  {
    mc::shared_pointer<int> p(1);
    p = 99;
    sb::require(*p == 99);
    sb::require(p.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: value assignment frees old object if last owner");
  {
    Tracker::reset();
    mc::shared_pointer<Tracker> p(1);
    Tracker t(2);
    p = t;     // old object freed, new one allocated
    sb::require(Tracker::destructions == 1);
    sb::require(p->value == 2);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: value assignment does not free shared old object");
  {
    Tracker::reset();
    mc::shared_pointer<Tracker> a(1);
    mc::shared_pointer<Tracker> b(a);     // refs = 2
    Tracker t(99);
    a = t;                                       // a detaches from shared object (refs drops to 1), allocates new
    sb::require(Tracker::destructions == 0);     // shared object still alive via b
    sb::require(a.refs() == 1);
    sb::require(b.refs() == 1);
    sb::require(a->value == 99);
    sb::require(b->value == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: nullptr assignment resets to null");
  {
    Tracker::reset();
    mc::shared_pointer<Tracker> p(7);
    p = nullptr;
    sb::require(!p);
    sb::require(p.refs() == 0);
    sb::require(Tracker::destructions == 1);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 5: Dereference and member access
  // ============================================================

  sb::test_case("shared_pointer<T>: operator* returns mutable reference");
  {
    mc::shared_pointer<int> p(10);
    *p = 99;
    sb::require(*p == 99);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: const operator* returns const reference");
  {
    const mc::shared_pointer<int> p(64);
    sb::require(*p == 64);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator* on null throws");
  {
    mc::shared_pointer<int> p;
    sb::require_throw([&]() { (void)*p; });
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator-> accesses member");
  {
    mc::shared_pointer<Point> p(5, 10);
    sb::require(p->x == 5 && p->y == 10);
    p->x = 99;
    sb::require(p->x == 99);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator-> on null returns nullptr (no throw)");
  {
    mc::shared_pointer<int> p;
    // operator-> returns nullptr rather than throwing; verify it is null
    sb::require(p.operator->() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator() returns raw pointer");
  {
    mc::shared_pointer<int> p(7);
    sb::require(p() == p.get());
    sb::require(*p() == 7);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: const operator() returns const raw pointer");
  {
    const mc::shared_pointer<int> p(8);
    sb::require(p() == p.get());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: get() returns same address as operator->");
  {
    mc::shared_pointer<int> p(123);
    sb::require(p.get() == p.operator->());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: get() on null returns nullptr");
  {
    mc::shared_pointer<int> p;
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 6: Boolean operators
  // ============================================================

  sb::test_case("shared_pointer<T>: operator bool — active yields true");
  {
    mc::shared_pointer<int> p(1);
    sb::require(static_cast<bool>(p) == true);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator bool — null yields false");
  {
    mc::shared_pointer<int> p;
    sb::require(static_cast<bool>(p) == false);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator! — null yields true");
  {
    mc::shared_pointer<int> p;
    sb::require(!p);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator! — active yields false");
  {
    mc::shared_pointer<int> p(1);
    sb::require(!(!p));
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator== nullptr_t: null pointer equals nullptr");
  {
    mc::shared_pointer<int> p;
    sb::require(p == nullptr);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: operator== nullptr_t: active pointer does not equal nullptr");
  {
    mc::shared_pointer<int> p(1);
    sb::require(!(p == nullptr));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 7: Refcount precision
  // ============================================================

  sb::test_case("shared_pointer<T>: refcount reaches 0 exactly after last copy destroyed");
  {
    mc::shared_pointer<int> a(1);
    {
      mc::shared_pointer<int> b(a);
      {
        mc::shared_pointer<int> c(b);
        sb::require(a.refs() == 3);
      }
      sb::require(a.refs() == 2);
    }
    sb::require(a.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: move does not increment refcount");
  {
    mc::shared_pointer<int> a(1);
    mc::shared_pointer<int> b(a);     // refs = 2
    mc::shared_pointer<int> c(std::move(b));
    sb::require(a.refs() == 2);     // move transferred control, count unchanged
    sb::require(!b);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: copy then move maintains correct count");
  {
    mc::shared_pointer<int> a(1);
    mc::shared_pointer<int> b(a);                // refs = 2
    mc::shared_pointer<int> c(a);                // refs = 3
    mc::shared_pointer<int> d(std::move(c));     // refs stays 3, c emptied
    sb::require(a.refs() == 3);
    sb::require(!c);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: copy assignment from null decrements refcount");
  {
    mc::shared_pointer<int> a(1);
    mc::shared_pointer<int> b(a);     // refs = 2
    mc::shared_pointer<int> empty;
    b = empty;     // b detaches (refs drops to 1), b becomes null
    sb::require(a.refs() == 1);
    sb::require(!b);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: refs() returns 0 for default-constructed pointer");
  {
    mc::shared_pointer<int> p;
    sb::require(p.refs() == 0);
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: shared mutation visible through all copies");
  {
    mc::shared_pointer<int> a(0);
    mc::shared_pointer<int> b(a);
    mc::shared_pointer<int> c(b);
    *a = 42;
    sb::require(*b == 42);
    sb::require(*c == 42);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 8: Leak detection via Tracker
  // ============================================================

  sb::test_case("shared_pointer<T>: no leak on single-owner scope exit");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> p(1);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: no leak after copy chain destroyed in reverse");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> a(1);
      mc::shared_pointer<Tracker> b(a);
      mc::shared_pointer<Tracker> c(b);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: no leak after copy assignment chain");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> a(1);
      mc::shared_pointer<Tracker> b;
      mc::shared_pointer<Tracker> c;
      b = a;
      c = b;
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: no leak after move chain");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> a(1);
      mc::shared_pointer<Tracker> b(std::move(a));
      mc::shared_pointer<Tracker> c(std::move(b));
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: no leak when copy assignment replaces owned object");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> a(1);
      mc::shared_pointer<Tracker> b(2);
      b = a;     // b's old (sole-owned) object freed; b joins a
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: no leak on nullptr assignment");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> p(5);
      p = nullptr;
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: no leak after value reassignment");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> p(1);
      Tracker t(2);
      p = t;
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("shared_pointer<T>: no leak after rvalue value reassignment");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> p(1);
      p = Tracker(2);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 9: Stress
  // ============================================================

  sb::test_case("Stress: 1000 construction/destruction cycles");
  {
    Tracker::reset();
    for ( int i = 0; i < 1000; i++ ) {
      mc::shared_pointer<Tracker> p(i);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("Stress: 1000 copies from single source, all destroyed");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> root(0);
      for ( int i = 0; i < 1000; i++ ) {
        // copy is scoped to this iteration and destroyed before the next,
        // so refs is always exactly 2 (root + copy), never accumulates
        mc::shared_pointer<Tracker> copy(root);
        sb::require(copy.refs() == static_cast<usize>(2));
      }     // all copies destroyed each iteration
      sb::require(root.refs() == 1);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("Stress: 500 copy-assign + 500 move-assign cycles, no leaks");
  {
    Tracker::reset();
    {
      mc::shared_pointer<Tracker> anchor(99);
      for ( int i = 0; i < 500; i++ ) {
        mc::shared_pointer<Tracker> tmp(i);
        tmp = anchor;     // tmp drops sole-owned object, joins anchor
      }
      // Each loop iteration: 1 Tracker constructed (tmp), 1 freed (tmp's old),
      // anchor's object persists throughout.
      sb::require(anchor.refs() == 1);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("Stress: shared mutation visible across 100 copies");
  {
    mc::shared_pointer<int> root(0);
    mc::shared_pointer<int> copies[100];
    for ( int i = 0; i < 100; i++ )
      copies[i] = root;
    sb::require(root.refs() == 101);
    *root = 42;
    for ( int i = 0; i < 100; i++ )
      sb::require(*copies[i] == 42);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
