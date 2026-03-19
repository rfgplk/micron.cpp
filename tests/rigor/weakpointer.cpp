//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/memory/pointers/shared.hpp"
#include "../../src/memory/pointers/unique.hpp"
#include "../../src/memory/pointers/weak.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

// ============================================================
//  Instrumentation
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
  sb::print("=== WEAK_POINTER TESTS ===");

  // ============================================================
  //  Section 1: Construction
  // ============================================================

  sb::test_case("weak_pointer<T>: default construction yields null");
  {
    mc::weak_pointer<int> w;
    sb::require(!w);
    sb::require(static_cast<bool>(w) == false);
    sb::require(w.get() == nullptr);     // get() returns nullptr, does not throw
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: nullptr construction yields null");
  {
    mc::weak_pointer<int> w(nullptr);
    sb::require(!w);
    sb::require(w.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: construction from unique_pointer borrows raw address");
  {
    mc::unique_pointer<int> u(42);
    mc::weak_pointer<int> w(u);
    sb::require(static_cast<bool>(w));
    sb::require(w.get() == u.get());
    sb::require(*w == 42);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: construction from shared_pointer borrows raw address");
  {
    mc::shared_pointer<int> s(99);
    mc::weak_pointer<int> w(s);
    sb::require(static_cast<bool>(w));
    sb::require(w.get() == s.get());
    sb::require(*w == 99);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: does NOT increment shared_pointer refcount");
  {
    mc::shared_pointer<int> s(1);
    sb::require(s.refs() == 1);
    mc::weak_pointer<int> w(s);
    sb::require(s.refs() == 1);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: destructor does NOT free the managed object");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> u(5);
    {
      mc::weak_pointer<Tracker> w(u);
    }
    sb::require(Tracker::destructions == 0);
    sb::require(u.active());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 2: get() returns nullptr — not throws (Bug 2 fix)
  // ============================================================

  sb::test_case("weak_pointer<T>: get() on null returns nullptr, does not throw");
  {
    mc::weak_pointer<int> w;
    // call directly — if it throws the test fails via exception propagation
    int *p = w.get();
    sb::require(p == nullptr);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: const get() on null returns nullptr, does not throw");
  {
    const mc::weak_pointer<int> w;
    const int *p = w.get();
    sb::require(p == nullptr);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: get() on valid pointer returns correct address");
  {
    mc::unique_pointer<int> u(77);
    mc::weak_pointer<int> w(u);
    sb::require(w.get() == u.get());
    sb::require(*w.get() == 77);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: get() permits safe null check without exception");
  {
    mc::weak_pointer<int> w;
    bool reached = false;
    if ( w.get() != nullptr )
      reached = true;
    sb::require(reached == false);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 3: Copy semantics
  // ============================================================

  sb::test_case("weak_pointer<T>: copy construction shares raw address");
  {
    mc::unique_pointer<int> u(77);
    mc::weak_pointer<int> a(u);
    mc::weak_pointer<int> b(a);
    sb::require(a.get() == b.get());
    sb::require(*b == 77);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: copy construction does not affect owner lifetime");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> u(3);
    {
      mc::weak_pointer<Tracker> a(u);
      mc::weak_pointer<Tracker> b(a);
    }
    sb::require(Tracker::destructions == 0);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: copy assignment shares raw address");
  {
    mc::unique_pointer<int> u(55);
    mc::weak_pointer<int> a(u);
    mc::weak_pointer<int> b;
    b = a;
    sb::require(b.get() == a.get());
    sb::require(*b == 55);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: copy assignment does not affect owner lifetime");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> u(6);
    mc::weak_pointer<Tracker> a(u);
    {
      mc::weak_pointer<Tracker> b;
      b = a;
    }
    sb::require(Tracker::destructions == 0);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: mutation through copy is visible through original");
  {
    mc::unique_pointer<int> u(10);
    mc::weak_pointer<int> a(u);
    mc::weak_pointer<int> b(a);
    *b = 99;
    sb::require(*a == 99);
    sb::require(*u == 99);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 4: Move semantics + self-move guard (Bug 3 fix)
  // ============================================================

  sb::test_case("weak_pointer<T>: move construction transfers pointer, nulls source");
  {
    mc::unique_pointer<int> u(55);
    mc::weak_pointer<int> a(u);
    int *raw = a.get();
    mc::weak_pointer<int> b(std::move(a));
    sb::require(a.get() == nullptr);
    sb::require(!a);
    sb::require(b.get() == raw);
    sb::require(*b == 55);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: move assignment transfers pointer, nulls source");
  {
    mc::unique_pointer<int> u(66);
    mc::weak_pointer<int> a(u);
    mc::weak_pointer<int> b;
    b = std::move(a);
    sb::require(a.get() == nullptr);
    sb::require(*b == 66);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: self-move-assignment is safe, pointer preserved");
  {
    mc::unique_pointer<int> u(42);
    mc::weak_pointer<int> w(u);
    int *raw = w.get();
    w = std::move(w);     // must not null w
    sb::require(w.get() == raw);
    sb::require(*w == 42);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: move does NOT free the managed object");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> u(4);
    {
      mc::weak_pointer<Tracker> a(u);
      mc::weak_pointer<Tracker> b(std::move(a));
    }
    sb::require(Tracker::destructions == 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 5: Assignment from owning pointer
  // ============================================================

  sb::test_case("weak_pointer<T>: assignment from unique_pointer rebinds");
  {
    mc::unique_pointer<int> u1(11);
    mc::unique_pointer<int> u2(22);
    mc::weak_pointer<int> w(u1);
    sb::require(*w == 11);
    w = u2;
    sb::require(*w == 22);
    sb::require(w.get() == u2.get());
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: assignment from shared_pointer rebinds, refcounts unchanged");
  {
    mc::shared_pointer<int> s1(33);
    mc::shared_pointer<int> s2(44);
    mc::weak_pointer<int> w(s1);
    sb::require(s1.refs() == 1);
    w = s2;
    sb::require(*w == 44);
    sb::require(s1.refs() == 1);
    sb::require(s2.refs() == 1);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 6: Dereference and member access
  // ============================================================

  sb::test_case("weak_pointer<T>: operator* returns mutable reference");
  {
    mc::unique_pointer<int> u(10);
    mc::weak_pointer<int> w(u);
    *w = 42;
    sb::require(*u == 42);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: const operator* returns const reference");
  {
    mc::unique_pointer<int> u(64);
    const mc::weak_pointer<int> w(u);
    sb::require(*w == 64);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator-> accesses member");
  {
    mc::unique_pointer<Point> u(3, 7);
    mc::weak_pointer<Point> w(u);
    sb::require(w->x == 3 && w->y == 7);
    w->x = 99;
    sb::require(u->x == 99);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator* on null throws");
  {
    mc::weak_pointer<int> w;
    sb::require_throw([&]() { (void)*w; });
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator-> on null throws");
  {
    mc::weak_pointer<Point> w;
    sb::require_throw([&]() { (void)w.operator->(); });
  }
  sb::end_test_case();

  // ============================================================
  //  Section 7: active()
  // ============================================================

  sb::test_case("weak_pointer<T>: active() false when null");
  {
    mc::weak_pointer<int> w;
    sb::require(w.active() == false);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: active() true when bound");
  {
    mc::unique_pointer<int> u(1);
    mc::weak_pointer<int> w(u);
    sb::require(w.active() == true);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: active() false after release()");
  {
    mc::unique_pointer<int> u(1);
    mc::weak_pointer<int> w(u);
    w.release();
    sb::require(w.active() == false);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 8: release() and operator()
  // ============================================================

  sb::test_case("weak_pointer<T>: release() returns raw address and nulls self");
  {
    mc::unique_pointer<int> u(88);
    mc::weak_pointer<int> w(u);
    int *raw = w.release();
    sb::require(raw == u.get());
    sb::require(w.get() == nullptr);
    sb::require(!w);
    sb::require(u.active());
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: release() on null returns nullptr");
  {
    mc::weak_pointer<int> w;
    sb::require(w.release() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator() is equivalent to release()");
  {
    mc::unique_pointer<int> u(77);
    mc::weak_pointer<int> w(u);
    int *via_call = w();
    sb::require(via_call == u.get());
    sb::require(!w);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: release() does NOT free the managed object");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> u(2);
    mc::weak_pointer<Tracker> w(u);
    w.release();
    sb::require(Tracker::destructions == 0);
    sb::require(u.active());
  }
  sb::end_test_case();

  // ============================================================
  //  Section 9: assume_ownership() (Bug 1 fix — was convert())
  //
  //  Precondition: original owner must have released first.
  //  Only then does a single owner exist after the call.
  // ============================================================

  sb::test_case("weak_pointer<T>: assume_ownership() after owner releases — single owner, no double-free");
  {
    Tracker::reset();
    {
      mc::unique_pointer<Tracker> u(123);
      mc::weak_pointer<Tracker> w(u);
      u.release();     // owner relinquishes
      mc::unique_pointer<Tracker> promoted = w.assume_ownership();
      sb::require(!w);
      sb::require(promoted.active());
      sb::require(promoted->value == 123);
    }     // exactly one destructor call here
    sb::require(Tracker::balanced());
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: assume_ownership() nulls the weak_pointer");
  {
    mc::unique_pointer<int> u(99);
    mc::weak_pointer<int> w(u);
    u.release();
    mc::unique_pointer<int> p = w.assume_ownership();
    sb::require(!w);
    sb::require(p.active());
    sb::require(*p == 99);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 10: Boolean operators and comparisons
  // ============================================================

  sb::test_case("weak_pointer<T>: operator bool — bound yields true");
  {
    mc::unique_pointer<int> u(1);
    mc::weak_pointer<int> w(u);
    sb::require(static_cast<bool>(w) == true);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator bool — null yields false");
  {
    mc::weak_pointer<int> w;
    sb::require(static_cast<bool>(w) == false);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator! — null yields true");
  {
    mc::weak_pointer<int> w;
    sb::require(!w);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator! — bound yields false");
  {
    mc::unique_pointer<int> u(1);
    mc::weak_pointer<int> w(u);
    sb::require(!(!w));
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator== with matching unique_pointer");
  {
    mc::unique_pointer<int> u(1);
    mc::weak_pointer<int> w(u);
    sb::require(w == u);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: operator== with different unique_pointer returns false");
  {
    mc::unique_pointer<int> u1(1);
    mc::unique_pointer<int> u2(2);
    mc::weak_pointer<int> w(u1);
    sb::require(!(w == u2));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 11: Lifetime and dangling semantics
  //  weak_pointer has no back-channel to owners.
  //  These tests verify the contract without dereferencing
  //  after owner death.
  // ============================================================

  sb::test_case("weak_pointer<T>: address matches owner before destruction");
  {
    mc::unique_pointer<int> u(42);
    mc::weak_pointer<int> w(u);
    sb::require(w.get() == u.get());
    sb::require(*w == 42);
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: shared_pointer death does not null the weak_pointer");
  {
    mc::weak_pointer<int> w;
    {
      mc::shared_pointer<int> s(7);
      w = s;
      sb::require(static_cast<bool>(w));
    }     // s destroyed — w becomes dangling but stays non-null
    sb::require(static_cast<bool>(w));     // no back-channel nulling
  }
  sb::end_test_case();

  sb::test_case("weak_pointer<T>: unique_pointer death does not null the weak_pointer");
  {
    mc::weak_pointer<int> w;
    {
      mc::unique_pointer<int> u(9);
      w = u;
      sb::require(static_cast<bool>(w));
    }
    sb::require(static_cast<bool>(w));     // dangling but non-null — expected
  }
  sb::end_test_case();

  // ============================================================
  //  Section 12: Stress
  // ============================================================

  sb::test_case("Stress: 1000 borrows from unique_pointer, no owner destruction");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> u(0);
    for ( int i = 0; i < 1000; i++ ) {
      mc::weak_pointer<Tracker> w(u);
      sb::require(w.get() == u.get());
      w->value = i;
    }
    sb::require(u->value == 999);
    sb::require(Tracker::destructions == 0);
  }
  sb::end_test_case();

  sb::test_case("Stress: 1000 borrows from shared_pointer, refcount stays at 1");
  {
    mc::shared_pointer<int> s(0);
    for ( int i = 0; i < 1000; i++ ) {
      mc::weak_pointer<int> w(s);
      *w = i;
      sb::require(s.refs() == 1);
    }
    sb::require(*s == 999);
  }
  sb::end_test_case();

  sb::test_case("Stress: 100 move-chain cycles, ownership unaffected");
  {
    Tracker::reset();
    mc::unique_pointer<Tracker> u(42);
    mc::weak_pointer<Tracker> w(u);
    for ( int i = 0; i < 100; i++ ) {
      mc::weak_pointer<Tracker> next(std::move(w));
      w = std::move(next);
    }
    sb::require(w.get() == u.get());
    sb::require(w->value == 42);
    sb::require(Tracker::destructions == 0);
  }
  sb::end_test_case();

  sb::test_case("Stress: 100 copy cycles from shared_pointer, refcount stable");
  {
    mc::shared_pointer<int> s(7);
    mc::weak_pointer<int> root(s);
    for ( int i = 0; i < 100; i++ ) {
      mc::weak_pointer<int> copy(root);
      sb::require(copy.get() == s.get());
      sb::require(s.refs() == 1);
    }
    sb::require(*root == 7);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
