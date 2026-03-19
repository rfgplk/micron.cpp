//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/memory/pointers/atomic.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>
#include <thread>
#include <vector>

// ─── helpers ─────────────────────────────────────────────────────────────────

struct Point {
  int x, y;

  bool
  operator==(const Point &o) const
  {
    return x == o.x && y == o.y;
  }
};

struct Counted {
  static int instances;
  int id;

  explicit Counted(int i) : id(i) { ++instances; }

  ~Counted() { --instances; }
};

int Counted::instances = 0;

// ─── main ────────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== ATOMIC POINTER TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - default: null, inactive");
  {
    micron::atomic_pointer<int> p;
    sb::require(!p);
    sb::require(p.active() == false);
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("construction - null literal: null, inactive");
  {
    micron::atomic_pointer<int> p(nullptr);
    sb::require(!p);
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("construction - in-place args: active, correct value");
  {
    micron::atomic_pointer<int> p(42);
    sb::require(static_cast<bool>(p));
    sb::require(p.active());
    sb::require(*p == 42);
  }
  sb::end_test_case();

  sb::test_case("construction - in-place struct: members correct");
  {
    micron::atomic_pointer<Point> p(3, 7);
    sb::require(p.active());
    sb::require(p->x == 3);
    sb::require(p->y == 7);
  }
  sb::end_test_case();

  sb::test_case("construction - rvalue raw pointer: takes ownership, source nulled");
  {
    int *raw = new int(99);
    micron::atomic_pointer<int> p(micron::move(raw));
    sb::require(raw == nullptr);
    sb::require(*p == 99);
  }
  sb::end_test_case();

  sb::test_case("construction - move constructor: transfers ownership");
  {
    micron::atomic_pointer<int> a(10);
    micron::atomic_pointer<int> b(micron::move(a));
    sb::require(!a);
    sb::require(*b == 10);
  }
  sb::end_test_case();

  sb::test_case("construction - destructor frees object (Counted)");
  {
    Counted::instances = 0;
    {
      micron::atomic_pointer<Counted> p(1);
      sb::require(Counted::instances == 1);
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("construction - default destructor on null: no crash");
  {
    micron::atomic_pointer<int> p;
    // destructor runs here — must not crash
    sb::require(true);
  }
  sb::end_test_case();

  // ── assignment ───────────────────────────────────────────────────────────

  sb::test_case("assignment - move: transfers ownership, source becomes null");
  {
    micron::atomic_pointer<int> a(55);
    micron::atomic_pointer<int> b;
    b = micron::move(a);
    sb::require(!a);
    sb::require(*b == 55);
  }
  sb::end_test_case();

  sb::test_case("assignment - move: old object in destination is freed (Counted)");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> a(1);
    micron::atomic_pointer<Counted> b(2);
    sb::require(Counted::instances == 2);
    b = micron::move(a);
    sb::require(Counted::instances == 1);     // b's old object freed
    sb::require(b->id == 1);
  }
  sb::end_test_case();

  sb::test_case("assignment - nullptr: clears pointer and frees (Counted)");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(7);
    sb::require(Counted::instances == 1);
    p = nullptr;
    sb::require(!p);
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("assignment - rvalue raw pointer: frees old, takes new");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(1);
    sb::require(Counted::instances == 1);
    Counted *raw = new Counted(2);
    sb::require(Counted::instances == 2);
    p = micron::move(raw);
    sb::require(raw == nullptr);
    sb::require(Counted::instances == 1);     // first object freed
    sb::require(p->id == 2);
  }
  sb::end_test_case();

  // ── observers ────────────────────────────────────────────────────────────

  sb::test_case("operator* - returns correct value");
  {
    micron::atomic_pointer<int> p(77);
    sb::require(*p == 77);
  }
  sb::end_test_case();

  sb::test_case("operator* - write through dereference persists");
  {
    micron::atomic_pointer<int> p(1);
    *p = 100;
    sb::require(*p == 100);
  }
  sb::end_test_case();

  sb::test_case("operator* const - reads correctly");
  {
    micron::atomic_pointer<int> p(33);
    const micron::atomic_pointer<int> &cp = p;
    sb::require(*cp == 33);
  }
  sb::end_test_case();

  sb::test_case("operator-> - accesses member correctly");
  {
    micron::atomic_pointer<Point> p(4, 5);
    sb::require(p->x == 4);
    sb::require(p->y == 5);
  }
  sb::end_test_case();

  sb::test_case("operator-> const - accesses member correctly");
  {
    micron::atomic_pointer<Point> p(6, 9);
    const micron::atomic_pointer<Point> &cp = p;
    sb::require(cp->x == 6);
    sb::require(cp->y == 9);
  }
  sb::end_test_case();

  sb::test_case("operator-> - throws on null pointer");
  {
    micron::atomic_pointer<int> p;
    bool threw = false;
    try {
      (void)p.operator->();
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("operator* - throws on null pointer");
  {
    micron::atomic_pointer<int> p;
    bool threw = false;
    try {
      (void)*p;
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("operator bool - true when active");
  {
    micron::atomic_pointer<int> p(1);
    sb::require(static_cast<bool>(p) == true);
  }
  sb::end_test_case();

  sb::test_case("operator bool - false when null");
  {
    micron::atomic_pointer<int> p;
    sb::require(static_cast<bool>(p) == false);
  }
  sb::end_test_case();

  sb::test_case("operator! - true when null");
  {
    micron::atomic_pointer<int> p;
    sb::require(!p == true);
  }
  sb::end_test_case();

  sb::test_case("operator! - false when active");
  {
    micron::atomic_pointer<int> p(1);
    sb::require(!p == false);
  }
  sb::end_test_case();

  sb::test_case("get - returns non-null when active");
  {
    micron::atomic_pointer<int> p(5);
    sb::require(p.get() != nullptr);
    sb::require(*p.get() == 5);
  }
  sb::end_test_case();

  sb::test_case("get - returns null when empty");
  {
    micron::atomic_pointer<int> p;
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("active - true when managing object");
  {
    micron::atomic_pointer<int> p(1);
    sb::require(p.active());
  }
  sb::end_test_case();

  sb::test_case("active - false after clear");
  {
    micron::atomic_pointer<int> p(1);
    p.clear();
    sb::require(!p.active());
  }
  sb::end_test_case();

  // ── modifiers ────────────────────────────────────────────────────────────

  sb::test_case("release - returns raw pointer, leaves null");
  {
    micron::atomic_pointer<int> p(123);
    int *raw = p.release();
    sb::require(raw != nullptr);
    sb::require(*raw == 123);
    sb::require(!p);
    delete raw;
  }
  sb::end_test_case();

  sb::test_case("release - on null returns nullptr");
  {
    micron::atomic_pointer<int> p;
    int *raw = p.release();
    sb::require(raw == nullptr);
  }
  sb::end_test_case();

  sb::test_case("reset - nullptr: frees and nulls (Counted)");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(1);
    sb::require(Counted::instances == 1);
    p.reset();
    sb::require(!p);
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("reset - raw pointer: frees old, takes new");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(1);
    Counted *raw = new Counted(2);
    sb::require(Counted::instances == 2);
    p.reset(raw);
    sb::require(Counted::instances == 1);
    sb::require(p->id == 2);
  }
  sb::end_test_case();

  sb::test_case("clear - frees object and nulls (Counted)");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(5);
    sb::require(Counted::instances == 1);
    p.clear();
    sb::require(!p);
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("clear - on null pointer: no crash");
  {
    micron::atomic_pointer<int> p;
    p.clear();
    sb::require(!p);
  }
  sb::end_test_case();

  sb::test_case("exchange - swaps in new raw pointer, returns old");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(1);
    Counted *raw = new Counted(2);
    sb::require(Counted::instances == 2);
    Counted *old = p.exchange(micron::move(raw));
    sb::require(raw == nullptr);
    sb::require(old != nullptr);
    sb::require(old->id == 1);
    sb::require(p->id == 2);
    sb::require(Counted::instances == 2);     // caller holds old
    delete old;
    sb::require(Counted::instances == 1);
  }
  sb::end_test_case();

  sb::test_case("swap - two pointers exchange ownership");
  {
    micron::atomic_pointer<int> a(1);
    micron::atomic_pointer<int> b(2);
    a.swap(b);
    sb::require(*a == 2);
    sb::require(*b == 1);
  }
  sb::end_test_case();

  sb::test_case("swap - with null: one side becomes null");
  {
    micron::atomic_pointer<int> a(99);
    micron::atomic_pointer<int> b;
    a.swap(b);
    sb::require(!a);
    sb::require(*b == 99);
  }
  sb::end_test_case();

  sb::test_case("free swap - mirrors member swap");
  {
    micron::atomic_pointer<int> a(10);
    micron::atomic_pointer<int> b(20);
    micron::swap(a, b);
    sb::require(*a == 20);
    sb::require(*b == 10);
  }
  sb::end_test_case();

  sb::test_case("compare_exchange_strong - succeeds when pointer matches expected");
  {
    micron::atomic_pointer<int> p(7);
    int *expected = p.get();
    int *desired = new int(8);
    bool ok = p.compare_exchange_strong(expected, micron::move(desired));
    sb::require(ok == true);
    sb::require(desired == nullptr);     // ownership transferred
    sb::require(*p == 8);
  }
  sb::end_test_case();

  sb::test_case("compare_exchange_strong - fails when pointer does not match, loads current");
  {
    micron::atomic_pointer<int> p(7);
    int *wrong = nullptr;     // does not match p's pointer
    int *desired = new int(8);
    bool ok = p.compare_exchange_strong(wrong, micron::move(desired));
    sb::require(ok == false);
    sb::require(wrong == p.get());     // updated to current
    sb::require(*p == 7);              // unchanged
    delete desired;
  }
  sb::end_test_case();

  // ── make_atomic ──────────────────────────────────────────────────────────

  sb::test_case("make_atomic - int: correct value");
  {
    auto p = micron::make_atomic<int>(42);
    sb::require(*p == 42);
  }
  sb::end_test_case();

  sb::test_case("make_atomic - struct: members correct");
  {
    auto p = micron::make_atomic<Point>(1, 2);
    sb::require(p->x == 1);
    sb::require(p->y == 2);
  }
  sb::end_test_case();

  sb::test_case("make_atomic - Counted: instance tracking correct");
  {
    Counted::instances = 0;
    {
      auto p = micron::make_atomic<Counted>(9);
      sb::require(Counted::instances == 1);
      sb::require(p->id == 9);
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  // ── value types ──────────────────────────────────────────────────────────

  sb::test_case("value type - double: precision preserved");
  {
    micron::atomic_pointer<double> p(3.14159265358979);
    sb::require(*p == 3.14159265358979);
  }
  sb::end_test_case();

  sb::test_case("value type - uint64_t: large value preserved");
  {
    micron::atomic_pointer<uint64_t> p(0xDEADBEEFCAFEBABEULL);
    sb::require(*p == 0xDEADBEEFCAFEBABEULL);
  }
  sb::end_test_case();

  sb::test_case("value type - int: INT_MIN and INT_MAX");
  {
    micron::atomic_pointer<int> a(INT_MIN);
    micron::atomic_pointer<int> b(INT_MAX);
    sb::require(*a == INT_MIN);
    sb::require(*b == INT_MAX);
  }
  sb::end_test_case();

  sb::test_case("value type - string: stored and read back");
  {
    micron::atomic_pointer<micron::hstring<char>> p("hello");
    sb::require(*p == "hello");
  }
  sb::end_test_case();

  // ── array specialisation ─────────────────────────────────────────────────

  sb::test_case("array - construction and element access");
  {
    micron::atomic_pointer<int[]> p(4);     // allocates array of 4
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

  sb::test_case("array - active and get");
  {
    micron::atomic_pointer<int[]> p(2);
    sb::require(p.active());
    sb::require(p.get() != nullptr);
  }
  sb::end_test_case();

  sb::test_case("array - release leaves null, caller responsible");
  {
    micron::atomic_pointer<int[]> p(2);
    int *raw = p.release();
    sb::require(!p);
    sb::require(raw != nullptr);
    delete[] raw;
  }
  sb::end_test_case();

  sb::test_case("array - clear frees and nulls");
  {
    micron::atomic_pointer<int[]> p(4);
    p.clear();
    sb::require(!p);
  }
  sb::end_test_case();

  sb::test_case("array - reset nulls and frees");
  {
    micron::atomic_pointer<int[]> p(4);
    p.reset();
    sb::require(!p);
  }
  sb::end_test_case();

  sb::test_case("array - move constructor transfers ownership");
  {
    micron::atomic_pointer<int[]> a(3);
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    micron::atomic_pointer<int[]> b(micron::move(a));
    sb::require(!a);
    sb::require(b[0] == 1);
    sb::require(b[1] == 2);
    sb::require(b[2] == 3);
  }
  sb::end_test_case();

  sb::test_case("array - swap exchanges ownership");
  {
    micron::atomic_pointer<int[]> a(2);
    micron::atomic_pointer<int[]> b(2);
    a[0] = 1;
    a[1] = 2;
    b[0] = 3;
    b[1] = 4;
    a.swap(b);
    sb::require(a[0] == 3);
    sb::require(b[0] == 1);
  }
  sb::end_test_case();

  // ── concurrency ──────────────────────────────────────────────────────────

  sb::test_case("concurrency - concurrent resets from multiple threads: no crash, no leak");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(0);
    sb::require(Counted::instances == 1);

    constexpr int NTHREADS = 8;
    std::vector<std::thread> threads;
    threads.reserve(NTHREADS);

    for ( int i = 0; i < NTHREADS; ++i ) {
      threads.emplace_back([&p, i]() {
        Counted *raw = new Counted(i + 1);
        p.reset(raw);
      });
    }
    for ( auto &t : threads )
      t.join();

    // Exactly one Counted should survive (the last winner of the resets)
    sb::require(p.active());
    sb::require(Counted::instances == 1);
  }
  sb::end_test_case();

  sb::test_case("concurrency - concurrent release and reset: no double-free");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(99);

    std::thread t1([&p]() {
      Counted *raw = p.release();
      if ( raw )
        delete raw;
    });

    std::thread t2([&p]() { p.reset(new Counted(1)); });

    t1.join();
    t2.join();

    // Either t2's object survived or p is null — either is correct.
    // What must NOT happen: a crash or Counted::instances < 0.
    sb::require(Counted::instances >= 0);
    p.clear();
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("concurrency - N threads each exchange once: all old values freed");
  {
    Counted::instances = 0;
    micron::atomic_pointer<Counted> p(0);

    constexpr int NTHREADS = 6;
    std::vector<Counted *> released(NTHREADS, nullptr);

    std::vector<std::thread> threads;
    threads.reserve(NTHREADS);

    for ( int i = 0; i < NTHREADS; ++i ) {
      threads.emplace_back([&p, &released, i]() {
        Counted *raw = new Counted(i + 1);
        released[i] = p.exchange(micron::move(raw));
      });
    }
    for ( auto &t : threads )
      t.join();

    // Free all the pointers threads got back from exchange
    for ( auto *r : released )
      if ( r )
        delete r;

    // One Counted remains in p
    p.clear();
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}
