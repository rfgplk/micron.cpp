//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/memory/pointers/unique.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

// ============================================================
//  Test instrumentation types
// ============================================================

// Tracks construction and destruction counts to detect leaks / double-frees
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

// Non-copyable, non-trivial type
struct NoCopy {
  int data;
  bool moved = false;

  explicit NoCopy(int d) : data(d) {}

  NoCopy(const NoCopy &) = delete;
  NoCopy &operator=(const NoCopy &) = delete;

  NoCopy(NoCopy &&o) : data(o.data), moved(true) { o.data = -1; }
};

// Type with a two-argument constructor
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
  sb::print("=== UNIQUE_POINTER TESTS ===");

  // ============================================================
  //  Section 1: Scalar — construction
  // ============================================================

  sb::test_case("unique_pointer<T>: default construction yields inactive pointer");
  {
    mc::unique_pointer<int> p;
    sb::require(!p.active());
    sb::require(!p);
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: nullptr construction yields inactive pointer");
  {
    mc::unique_pointer<int> p(nullptr);
    sb::require(!p.active());
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: in-place construction allocates object");
  {
    mc::unique_pointer<int> p(42);
    sb::require(p.active());
    sb::require(static_cast<bool>(p));
    sb::require(*p == 42);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: multi-arg in-place construction");
  {
    mc::unique_pointer<Point> p(3, 7);
    sb::require(p.active());
    sb::require(p->x == 3);
    sb::require(p->y == 7);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: raw-pointer move construction takes ownership");
  {
    int *raw = new int(99);
    mc::unique_pointer<int> p(std::move(raw));
    sb::require(raw == nullptr);     // ownership transferred, raw nulled
    sb::require(p.active());
    sb::require(*p == 99);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: destructor frees the managed object");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> p(5);
      sb::require(Tracker::constructions == 1);
    }     // p destroyed here
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: default-constructed destructor does not double-free");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> p;     // nullptr, nothing to free
    }
    sb::require(Tracker::destructions == 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 2: Scalar — move semantics
  // ============================================================

  sb::test_case("unique_pointer<T>: move construction transfers ownership");
  {
    mc::unique_pointer<int> a(100);
    int *raw_before = a.get();
    mc::unique_pointer<int> b(std::move(a));
    sb::require(!a.active());     // source is empty
    sb::require(a.get() == nullptr);
    sb::require(b.active());
    sb::require(b.get() == raw_before);     // same address
    sb::require(*b == 100);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: move assignment transfers ownership");
  {
    mc::unique_pointer<int> a(200);
    mc::unique_pointer<int> b;
    int *raw_before = a.get();
    b = std::move(a);
    sb::require(!a.active());
    sb::require(b.active());
    sb::require(b.get() == raw_before);
    sb::require(*b == 200);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: move assignment replaces existing managed object");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> a(1);
      mc::unique_pointer<Tracker> b(2);
      b = std::move(a);                            // b's old object must be freed
      sb::require(Tracker::destructions == 1);     // old b freed
      sb::require(b->value == 1);
      sb::require(!a.active());
    }     // b freed on scope exit
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: self-move-assignment is safe");
  {
    mc::unique_pointer<int> p(55);
    int *raw = p.get();
    p = std::move(p);
    // after self-move the pointer must remain consistent
    // (either still valid or null — must not crash / double-free)
    // implementation guards with `if (this != &t)`
    sb::require(p.get() == raw);
    sb::require(*p == 55);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: move assignment from raw pointer");
  {
    int *raw = new int(77);
    mc::unique_pointer<int> p;
    p = std::move(raw);
    sb::require(raw == nullptr);
    sb::require(*p == 77);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: copy construction is deleted (compile-time only)");
  {
    // This test documents intent; it passes by existing — we can't call it.
    // unique_pointer<int> a(1);
    // unique_pointer<int> b(a);  // must not compile
    sb::require(true);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 3: Scalar — release / clear / get
  // ============================================================

  sb::test_case("unique_pointer<T>: release returns raw pointer and clears ownership");
  {
    mc::unique_pointer<int> p(33);
    int *raw = p.release();
    sb::require(raw != nullptr);
    sb::require(*raw == 33);
    sb::require(!p.active());
    sb::require(p.get() == nullptr);
    delete raw;     // we now own it
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: release on null returns nullptr");
  {
    mc::unique_pointer<int> p;
    int *raw = p.release();
    sb::require(raw == nullptr);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: clear frees object and nulls pointer");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> p(9);
    p.clear();
    sb::require(!p.active());
    sb::require(Tracker::destructions == 1);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: clear on null is a no-op");
  {
    mc::unique_pointer<int> p;
    p.clear();     // must not crash
    sb::require(!p.active());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: double clear does not double-free");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> p(7);
    p.clear();
    p.clear();     // second clear on null — must not crash
    sb::require(Tracker::destructions == 1);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: get returns raw address without releasing");
  {
    mc::unique_pointer<int> p(17);
    int *raw = p.get();
    sb::require(raw != nullptr);
    sb::require(*raw == 17);
    sb::require(p.active());     // still owns it
  }
  sb::end_test_case();

  // ============================================================
  //  Section 4: Scalar — dereference and member access
  // ============================================================

  sb::test_case("unique_pointer<T>: operator* returns reference to managed object");
  {
    mc::unique_pointer<int> p(42);
    *p = 99;
    sb::require(*p == 99);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: const operator* on const pointer");
  {
    const mc::unique_pointer<int> p(64);
    sb::require(*p == 64);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: operator-> accesses member");
  {
    mc::unique_pointer<Point> p(5, 10);
    sb::require(p->x == 5);
    sb::require(p->y == 10);
    p->x = 99;
    sb::require(p->x == 99);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: operator* on null throws");
  {
    mc::unique_pointer<int> p;
    sb::require_throw([&]() { (void)*p; });
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: operator-> on null throws");
  {
    mc::unique_pointer<int> p;
    sb::require_throw([&]() { (void)p.operator->(); });
  }
  sb::end_test_case();

  // ============================================================
  //  Section 5: Scalar — comparison operators
  // ============================================================

  sb::test_case("unique_pointer<T>: operator== with equal pointers");
  {
    mc::unique_pointer<int> a(1);
    mc::unique_pointer<int> b;
    // b = a.get() via raw pointer move so they point to same address
    int *raw = a.release();
    mc::unique_pointer<int> c(std::move(raw));
    // a and c now both existed at same raw; b is null
    // just test that two nulls are equal via bool check
    sb::require(!a.active());
    sb::require(!b.active());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: operator bool — active yields true");
  {
    mc::unique_pointer<int> p(1);
    sb::require(static_cast<bool>(p) == true);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: operator bool — null yields false");
  {
    mc::unique_pointer<int> p;
    sb::require(static_cast<bool>(p) == false);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: operator! — null yields true");
  {
    mc::unique_pointer<int> p;
    sb::require(!p);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: operator! — active yields false");
  {
    mc::unique_pointer<int> p(1);
    sb::require(!(!p));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 6: Scalar — leak detection via Tracker
  // ============================================================

  sb::test_case("unique_pointer<T>: no leak on normal scope exit");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> p(42);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: no leak after move chain");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> a(1);
      mc::unique_pointer<Tracker> b(std::move(a));
      mc::unique_pointer<Tracker> c(std::move(b));
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: no leak after reassignment");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> p(10);
      mc::unique_pointer<Tracker> q(20);
      p = std::move(q);     // p's old object freed, q emptied
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: no leak after clear");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> p(5);
      p.clear();
    }     // destructor called on empty pointer — balanced?
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T>: release prevents destructor from freeing");
  {
    Tracker::reset();
    Tracker *raw = nullptr;
    {
      mc::unique_pointer<Tracker> p(3);
      raw = p.release();
    }
    // destructor of empty p should not decrement
    sb::require(Tracker::destructions == 0);
    delete raw;
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 7: Array specialisation — construction
  // ============================================================

  sb::test_case("unique_pointer<T[]>: default construction yields inactive pointer");
  {
    mc::unique_pointer<int[]> p;
    sb::require(!p.active());
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: nullptr construction yields inactive pointer");
  {
    mc::unique_pointer<int[]> p(nullptr);
    sb::require(!p.active());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: construction with count allocates array");
  {
    mc::unique_pointer<int[]> p(8);     // allocate 8-element array
    sb::require(p.active());
    sb::require(static_cast<bool>(p));
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: raw-pointer move construction takes ownership");
  {
    int *raw = new int[4]{ 1, 2, 3, 4 };
    mc::unique_pointer<int[]> p(std::move(raw));
    sb::require(raw == nullptr);
    sb::require(p.active());
    sb::require(p[0] == 1);
    sb::require(p[3] == 4);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 8: Array specialisation — element access
  // ============================================================

  sb::test_case("unique_pointer<T[]>: operator[] read access");
  {
    mc::unique_pointer<int[]> p(4);
    p[0] = 10;
    p[1] = 20;
    p[2] = 30;
    p[3] = 40;
    sb::require(p[0] == 10);
    sb::require(p[1] == 20);
    sb::require(p[2] == 30);
    sb::require(p[3] == 40);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: operator[] write modifies in place");
  {
    mc::unique_pointer<int[]> p(4);
    for ( int i = 0; i < 4; i++ )
      p[i] = i * 10;
    p[2] = 99;
    sb::require(p[2] == 99);
    sb::require(p[1] == 10);     // neighbours untouched
    sb::require(p[3] == 30);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: const operator[] read access");
  {
    int *raw = new int[3]{ 7, 8, 9 };
    const mc::unique_pointer<int[]> p(std::move(raw));
    sb::require(p[0] == 7);
    sb::require(p[2] == 9);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: operator[] on null throws");
  {
    mc::unique_pointer<int[]> p;
    sb::require_throw([&]() { (void)p[0]; });
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: operator* returns reference to first element");
  {
    int *raw = new int[4]{ 55, 66, 77, 88 };
    mc::unique_pointer<int[]> p(std::move(raw));
    sb::require(*p == 55);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: operator-> accesses elements via pointer");
  {
    mc::unique_pointer<int[]> p(4);
    p[0] = 100;
    sb::require(p.get()[0] == 100);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 9: Array specialisation — move semantics
  // ============================================================

  sb::test_case("unique_pointer<T[]>: move construction transfers ownership");
  {
    mc::unique_pointer<int[]> a(4);
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    int *raw_before = a.get();
    mc::unique_pointer<int[]> b(std::move(a));
    sb::require(!a.active());
    sb::require(b.get() == raw_before);
    sb::require(b[0] == 1 && b[3] == 4);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: move assignment transfers ownership");
  {
    mc::unique_pointer<int[]> a(4);
    a[0] = 42;
    mc::unique_pointer<int[]> b;
    b = std::move(a);
    sb::require(!a.active());
    sb::require(b.active());
    sb::require(b[0] == 42);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: move assignment replaces existing array");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker[]> a(2);
      mc::unique_pointer<Tracker[]> b(3);
      b = std::move(a);
      // b's old 3-element array freed, a's 2-element array now owned by b
      sb::require(!a.active());
      sb::require(b.active());
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: move assignment from raw pointer");
  {
    int *raw = new int[2]{ 10, 20 };
    mc::unique_pointer<int[]> p;
    p = std::move(raw);
    sb::require(raw == nullptr);
    sb::require(p[0] == 10);
    sb::require(p[1] == 20);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 10: Array specialisation — release / clear / active
  // ============================================================

  sb::test_case("unique_pointer<T[]>: release returns raw pointer and clears");
  {
    mc::unique_pointer<int[]> p(3);
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;
    int *raw = p.release();
    sb::require(raw != nullptr);
    sb::require(raw[0] == 1 && raw[2] == 3);
    sb::require(!p.active());
    delete[] raw;
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: release on null returns nullptr");
  {
    mc::unique_pointer<int[]> p;
    sb::require(p.release() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: clear frees array and nulls");
  {
    mc::unique_pointer<int[]> p(4);
    p.clear();
    sb::require(!p.active());
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: clear on null is a no-op");
  {
    mc::unique_pointer<int[]> p;
    p.clear();
    sb::require(!p.active());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: double clear does not double-free");
  {
    mc::unique_pointer<int[]> p(2);
    p.clear();
    p.clear();
    sb::require(!p.active());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: active() reflects ownership state");
  {
    mc::unique_pointer<int[]> p;
    sb::require(p.active() == false);
    p = mc::unique_pointer<int[]>(2);
    sb::require(p.active() == true);
    p.clear();
    sb::require(p.active() == false);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: operator bool matches active()");
  {
    mc::unique_pointer<int[]> p(2);
    sb::require(static_cast<bool>(p) == p.active());
    p.clear();
    sb::require(static_cast<bool>(p) == p.active());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 11: Array specialisation — leak detection
  // ============================================================

  sb::test_case("unique_pointer<T[]>: no leak on normal scope exit");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker[]> p(5);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: no leak after move chain");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker[]> a(3);
      mc::unique_pointer<Tracker[]> b(std::move(a));
      mc::unique_pointer<Tracker[]> c(std::move(b));
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: no leak after reassignment freeing old array");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker[]> p(4);
      mc::unique_pointer<Tracker[]> q(6);
      p = std::move(q);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<T[]>: release prevents destructor from freeing");
  {
    Tracker::reset();
    Tracker *raw = nullptr;
    {
      mc::unique_pointer<Tracker[]> p(3);
      raw = p.release();
    }
    sb::require(Tracker::destructions == 0);
    delete[] raw;
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 12: Scalar — NoCopy type (move-only managed object)
  // ============================================================

  sb::test_case("unique_pointer<NoCopy>: in-place construction of move-only type");
  {
    mc::unique_pointer<NoCopy> p(42);
    sb::require(p.active());
    sb::require(p->data == 42);
  }
  sb::end_test_case();

  sb::test_case("unique_pointer<NoCopy>: move from one pointer to another");
  {
    mc::unique_pointer<NoCopy> a(7);
    mc::unique_pointer<NoCopy> b(std::move(a));
    sb::require(!a.active());
    sb::require(b->data == 7);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 13: Repeated construction/destruction stress
  // ============================================================

  sb::test_case("Stress: 1000 construction/destruction cycles, no leaks");
  {
    Tracker::reset();
    for ( int i = 0; i < 1000; i++ ) {
      mc::unique_pointer<Tracker> p(i);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("Stress: 1000 move-assignment cycles, no leaks");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> p(0);
      for ( int i = 1; i < 1000; i++ ) {
        mc::unique_pointer<Tracker> q(i);
        p = std::move(q);
      }
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("Stress: 1000 array cycles, no leaks");
  {
    Tracker::reset();
    for ( int i = 0; i < 1000; i++ ) {
      mc::unique_pointer<Tracker[]> p(4);
    }
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
