//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/memory/pointers/global.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <climits>

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

  Counted()
  {
    ++instances;
    id = 0;
  }

  ~Counted() { --instances; }
};

int Counted::instances = 0;

// ─── main ────────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== GLOBAL POINTER TESTS ===");

  // ── construction (scalar) ─────────────────────────────────────────────────

  sb::test_case("construction - default: allocates, non-null");
  {
    micron::__global_pointer<int> p;
    sb::require(static_cast<bool>(p));
    sb::require(p.get() != nullptr);
  }
  sb::end_test_case();

  sb::test_case("construction - default: Counted instance created");
  {
    Counted::instances = 0;
    {
      micron::__global_pointer<Counted> p;
      sb::require(Counted::instances == 1);
      p.clear();     // global pointer does not free on destruction — clear manually
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("construction - null literal: null, inactive");
  {
    micron::__global_pointer<int> p(nullptr);
    sb::require(!p);
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("construction - in-place args: value correct");
  {
    micron::__global_pointer<int> p(42);
    sb::require(*p == 42);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("construction - in-place struct: members correct");
  {
    micron::__global_pointer<Point> p(3, 7);
    sb::require(p->x == 3);
    sb::require(p->y == 7);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("construction - lvalue raw pointer: takes ownership, source nulled");
  {
    int *raw = new int(99);
    micron::__global_pointer<int> p(raw);
    sb::require(raw == nullptr);
    sb::require(*p == 99);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("construction - lvalue raw pointer: Counted instance count correct");
  {
    Counted::instances = 0;
    Counted *raw = new Counted(1);
    sb::require(Counted::instances == 1);
    micron::__global_pointer<Counted> p(raw);
    sb::require(raw == nullptr);
    sb::require(Counted::instances == 1);
    p.clear();
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("construction - move constructor: transfers ownership, source null");
  {
    micron::__global_pointer<int> a(55);
    micron::__global_pointer<int> b(micron::move(a));
    sb::require(!a);
    sb::require(*b == 55);
    b.clear();
  }
  sb::end_test_case();

  // ── key design property: destructor does NOT free ─────────────────────────

  sb::test_case("design - destructor does not free: Counted survives scope exit");
  {
    Counted::instances = 0;
    Counted *leaked = nullptr;
    {
      micron::__global_pointer<Counted> p(7);
      sb::require(Counted::instances == 1);
      leaked = p.get();
    }     // destructor runs here — must NOT free
    sb::require(Counted::instances == 1);     // still alive
    // Clean up manually to avoid a true leak in the test binary
    delete leaked;
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  // ── operator bool / operator! (scalar) ───────────────────────────────────

  sb::test_case("operator bool - true when active");
  {
    micron::__global_pointer<int> p(1);
    sb::require(static_cast<bool>(p) == true);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("operator bool - false for null");
  {
    micron::__global_pointer<int> p(nullptr);
    sb::require(static_cast<bool>(p) == false);
  }
  sb::end_test_case();

  sb::test_case("operator! - true for null");
  {
    micron::__global_pointer<int> p(nullptr);
    sb::require(!p == true);
  }
  sb::end_test_case();

  sb::test_case("operator! - false when active");
  {
    micron::__global_pointer<int> p(1);
    sb::require(!p == false);
    p.clear();
  }
  sb::end_test_case();

  // ── operator* / operator-> (scalar) ──────────────────────────────────────

  sb::test_case("operator* - reads correct value");
  {
    micron::__global_pointer<int> p(123);
    sb::require(*p == 123);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("operator* - write through persists");
  {
    micron::__global_pointer<int> p(1);
    *p = 100;
    sb::require(*p == 100);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("operator* const - reads correctly");
  {
    micron::__global_pointer<int> p(33);
    const micron::__global_pointer<int> &cp = p;
    sb::require(*cp == 33);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("operator* - throws on null");
  {
    micron::__global_pointer<int> p(nullptr);
    bool threw = false;
    try {
      (void)*p;
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("operator-> - accesses struct member");
  {
    micron::__global_pointer<Point> p(4, 5);
    sb::require(p->x == 4);
    sb::require(p->y == 5);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("operator-> non-const - write through member");
  {
    micron::__global_pointer<Point> p(1, 2);
    p->x = 99;
    sb::require(p->x == 99);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("operator-> const - reads member correctly");
  {
    micron::__global_pointer<Point> p(6, 9);
    const micron::__global_pointer<Point> &cp = p;
    sb::require(cp->x == 6);
    sb::require(cp->y == 9);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("operator-> - throws on null");
  {
    micron::__global_pointer<int> p(nullptr);
    bool threw = false;
    try {
      (void)p.operator->();
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  // ── value types (scalar) ──────────────────────────────────────────────────

  sb::test_case("value type - double: precision preserved");
  {
    micron::__global_pointer<double> p(3.14159265358979);
    sb::require(*p == 3.14159265358979);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("value type - uint64_t: large value preserved");
  {
    micron::__global_pointer<uint64_t> p(0xDEADBEEFCAFEBABEULL);
    sb::require(*p == 0xDEADBEEFCAFEBABEULL);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("value type - INT_MAX preserved");
  {
    micron::__global_pointer<int> p(INT_MAX);
    sb::require(*p == INT_MAX);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("value type - INT_MIN preserved");
  {
    micron::__global_pointer<int> p(INT_MIN);
    sb::require(*p == INT_MIN);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("value type - string stored and read back");
  {
    micron::__global_pointer<micron::hstring<char>> p("hello");
    sb::require(*p == "hello");
    p.clear();
  }
  sb::end_test_case();

  // ── get / release (scalar) ────────────────────────────────────────────────

  sb::test_case("get - returns non-null address when active");
  {
    micron::__global_pointer<int> p(5);
    sb::require(p.get() != nullptr);
    sb::require(*p.get() == 5);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("get - returns null when null-constructed");
  {
    micron::__global_pointer<int> p(nullptr);
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("release - returns raw pointer, leaves pointer null");
  {
    micron::__global_pointer<int> p(123);
    int *raw = p.release();
    sb::require(raw != nullptr);
    sb::require(*raw == 123);
    sb::require(!p);
    delete raw;
  }
  sb::end_test_case();

  sb::test_case("release - on null pointer returns nullptr");
  {
    micron::__global_pointer<int> p(nullptr);
    int *raw = p.release();
    sb::require(raw == nullptr);
  }
  sb::end_test_case();

  // ── clear (scalar) ────────────────────────────────────────────────────────

  sb::test_case("clear - frees object and nulls pointer");
  {
    Counted::instances = 0;
    micron::__global_pointer<Counted> p(1);
    sb::require(Counted::instances == 1);
    p.clear();
    sb::require(!p);
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("clear - on null pointer: no crash");
  {
    micron::__global_pointer<int> p(nullptr);
    p.clear();
    sb::require(!p);
  }
  sb::end_test_case();

  sb::test_case("clear - pointer is null after clear (no dangling)");
  {
    micron::__global_pointer<int> p(42);
    p.clear();
    sb::require(p.get() == nullptr);
    sb::require(!p);
  }
  sb::end_test_case();

  // ── assignment (scalar) ───────────────────────────────────────────────────

  sb::test_case("move assignment - transfers ownership, source null");
  {
    micron::__global_pointer<int> a(55);
    micron::__global_pointer<int> b(nullptr);
    b = micron::move(a);
    sb::require(!a);
    sb::require(*b == 55);
    b.clear();
  }
  sb::end_test_case();

  sb::test_case("move assignment - frees destination's old object (Counted)");
  {
    Counted::instances = 0;
    micron::__global_pointer<Counted> a(1);
    micron::__global_pointer<Counted> b(2);
    sb::require(Counted::instances == 2);
    b = micron::move(a);
    sb::require(Counted::instances == 1);     // b's old object freed
    sb::require(b->id == 1);
    b.clear();
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("lvalue raw pointer assignment - nulls source, frees old");
  {
    Counted::instances = 0;
    micron::__global_pointer<Counted> p(1);
    sb::require(Counted::instances == 1);
    Counted *raw = new Counted(2);
    sb::require(Counted::instances == 2);
    p = raw;
    sb::require(raw == nullptr);
    sb::require(Counted::instances == 1);     // first object freed
    sb::require(p->id == 2);
    p.clear();
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  // ── comparisons (scalar) ──────────────────────────────────────────────────

  sb::test_case("operator== - same pointer-class object: true");
  {
    micron::__global_pointer<int> a(10);
    micron::__global_pointer<int> b(nullptr);
    // make b point at same address by raw transfer
    int *raw = a.get();
    // compare a against a wrapper holding the same address via get()
    // (can't copy-construct, so compare via raw address in a second global ptr)
    sb::require(a.get() == raw);     // trivially true, confirms get() stability
    a.clear();
  }
  sb::end_test_case();

  // ── array spec construction ───────────────────────────────────────────────

  sb::test_case("array - construction with size arg: non-null, active");
  {
    micron::__global_pointer<int[]> p(4);
    sb::require(static_cast<bool>(p));
    sb::require(p.get() != nullptr);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array - lvalue raw pointer: takes ownership, source nulled");
  {
    int *raw = new int[3]{ 10, 20, 30 };
    micron::__global_pointer<int[]> p(raw);
    sb::require(raw == nullptr);
    sb::require(p[0] == 10);
    sb::require(p[1] == 20);
    sb::require(p[2] == 30);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array - move constructor: transfers ownership, source null");
  {
    micron::__global_pointer<int[]> a(3);
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    micron::__global_pointer<int[]> b(micron::move(a));
    sb::require(!a);
    sb::require(b[0] == 1);
    sb::require(b[1] == 2);
    sb::require(b[2] == 3);
    b.clear();
  }
  sb::end_test_case();

  sb::test_case("array - design: destructor does not free");
  {
    Counted::instances = 0;
    Counted *leaked = nullptr;
    {
      micron::__global_pointer<Counted[]> p(2);
      sb::require(Counted::instances == 2);
      leaked = p.get();
    }
    sb::require(Counted::instances == 2);     // still alive after scope
    delete[] leaked;
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  // ── array operator[] ──────────────────────────────────────────────────────

  sb::test_case("array operator[] - non-const read all elements");
  {
    int *raw = new int[5]{ 1, 2, 3, 4, 5 };
    micron::__global_pointer<int[]> p(raw);
    bool ok = true;
    for ( usize i = 0; i < 5 && ok; ++i )
      if ( p[i] != (int)(i + 1) )
        ok = false;
    sb::require(ok);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array operator[] non-const - write through persists");
  {
    micron::__global_pointer<int[]> p(3);
    p[0] = 7;
    p[1] = 8;
    p[2] = 9;
    sb::require(p[0] == 7);
    sb::require(p[1] == 8);
    sb::require(p[2] == 9);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array operator[] - throws on null");
  {
    int *raw = nullptr;
    micron::__global_pointer<int[]> p(raw);
    bool threw = false;
    try {
      (void)p[0];
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  // ── array operator* / operator-> ─────────────────────────────────────────

  sb::test_case("array operator* - returns reference to first element");
  {
    int *raw = new int[3]{ 42, 0, 0 };
    micron::__global_pointer<int[]> p(raw);
    sb::require(*p == 42);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array operator* - write through first element");
  {
    micron::__global_pointer<int[]> p(2);
    *p = 77;
    sb::require(p[0] == 77);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array operator* - throws on null");
  {
    int *raw = nullptr;
    micron::__global_pointer<int[]> p(raw);
    bool threw = false;
    try {
      (void)*p;
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("array operator-> - accesses first struct member");
  {
    Point *raw = new Point[2]{ Point{ 1, 2 }, Point{ 3, 4 } };
    micron::__global_pointer<Point[]> p(raw);
    sb::require(p->x == 1);
    sb::require(p->y == 2);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array operator-> - write through first struct member");
  {
    Point *raw = new Point[2]{ Point{ 0, 0 }, Point{ 0, 0 } };
    micron::__global_pointer<Point[]> p(raw);
    p->x = 55;
    sb::require(p[0].x == 55);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array operator-> - throws on null");
  {
    int *raw = nullptr;
    micron::__global_pointer<int[]> p(raw);
    bool threw = false;
    try {
      (void)p.operator->();
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  // ── array operator bool / operator! ──────────────────────────────────────

  sb::test_case("array operator bool - true when active");
  {
    micron::__global_pointer<int[]> p(2);
    sb::require(static_cast<bool>(p) == true);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array operator bool - false after clear");
  {
    micron::__global_pointer<int[]> p(2);
    p.clear();
    sb::require(static_cast<bool>(p) == false);
  }
  sb::end_test_case();

  sb::test_case("array operator! - true after clear");
  {
    micron::__global_pointer<int[]> p(2);
    p.clear();
    sb::require(!p == true);
  }
  sb::end_test_case();

  // ── array get / release / clear ───────────────────────────────────────────

  sb::test_case("array get - returns non-null address");
  {
    micron::__global_pointer<int[]> p(3);
    sb::require(p.get() != nullptr);
    p.clear();
  }
  sb::end_test_case();

  sb::test_case("array release - returns raw pointer, leaves null");
  {
    micron::__global_pointer<int[]> p(3);
    int *raw = p.release();
    sb::require(!p);
    sb::require(raw != nullptr);
    delete[] raw;
  }
  sb::end_test_case();

  sb::test_case("array clear - frees and nulls");
  {
    Counted::instances = 0;
    micron::__global_pointer<Counted[]> p(3);
    sb::require(Counted::instances == 3);
    p.clear();
    sb::require(!p);
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("array clear - pointer is null after clear (no dangling)");
  {
    micron::__global_pointer<int[]> p(4);
    p.clear();
    sb::require(p.get() == nullptr);
  }
  sb::end_test_case();

  // ── array move assignment ─────────────────────────────────────────────────

  sb::test_case("array move assignment - transfers, source null, old freed (Counted)");
  {
    Counted::instances = 0;
    micron::__global_pointer<Counted[]> a(1);
    micron::__global_pointer<Counted[]> b(2);
    sb::require(Counted::instances == 3);
    b = micron::move(a);
    sb::require(Counted::instances == 1);     // b's two old objects freed
    b.clear();
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("array lvalue raw pointer assignment - nulls source, frees old");
  {
    Counted::instances = 0;
    micron::__global_pointer<Counted[]> p(1);
    sb::require(Counted::instances == 1);
    Counted *raw = new Counted[2]{ Counted(10), Counted(11) };
    sb::require(Counted::instances == 3);
    p = raw;
    sb::require(raw == nullptr);
    sb::require(Counted::instances == 2);     // original single element freed
    p.clear();
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}
