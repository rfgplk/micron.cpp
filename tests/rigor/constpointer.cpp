//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/memory/pointers/constant.hpp"
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
  sb::print("=== CONST POINTER TESTS ===");

  // ── construction (scalar) ─────────────────────────────────────────────────

  sb::test_case("construction - default: allocates, non-null, active");
  {
    micron::const_pointer<int> p;
    sb::require(static_cast<bool>(p));
    sb::require(!(!p));
    sb::require(p.data() != nullptr);
  }
  sb::end_test_case();

  sb::test_case("construction - default: Counted instance created");
  {
    Counted::instances = 0;
    {
      micron::const_pointer<Counted> p;
      sb::require(Counted::instances == 1);
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("construction - in-place args: value correct");
  {
    micron::const_pointer<int> p(42);
    sb::require(*p == 42);
  }
  sb::end_test_case();

  sb::test_case("construction - in-place struct: members correct");
  {
    micron::const_pointer<Point> p(3, 7);
    sb::require(p->x == 3);
    sb::require(p->y == 7);
  }
  sb::end_test_case();

  sb::test_case("construction - lvalue raw pointer: takes ownership, source nulled");
  {
    int *raw = new int(99);
    micron::const_pointer<int> p(raw);
    sb::require(raw == nullptr);     // caller's pointer was nulled
    sb::require(*p == 99);
  }
  sb::end_test_case();

  sb::test_case("construction - lvalue raw pointer: Counted instance count correct");
  {
    Counted::instances = 0;
    Counted *raw = new Counted(1);
    sb::require(Counted::instances == 1);
    {
      micron::const_pointer<Counted> p(raw);
      sb::require(raw == nullptr);
      sb::require(Counted::instances == 1);
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  sb::test_case("construction - destructor frees object");
  {
    Counted::instances = 0;
    {
      micron::const_pointer<Counted> p(5);
      sb::require(Counted::instances == 1);
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  // ── operator bool / operator! (scalar) ───────────────────────────────────

  sb::test_case("operator bool - true for default-constructed (always allocates)");
  {
    micron::const_pointer<int> p;
    sb::require(static_cast<bool>(p) == true);
  }
  sb::end_test_case();

  sb::test_case("operator bool - true for in-place constructed");
  {
    micron::const_pointer<int> p(1);
    sb::require(static_cast<bool>(p) == true);
  }
  sb::end_test_case();

  sb::test_case("operator! - false for live pointer");
  {
    micron::const_pointer<int> p(1);
    sb::require(!p == false);
  }
  sb::end_test_case();

  // ── operator() / data() (scalar) ─────────────────────────────────────────

  sb::test_case("operator() - returns non-null address");
  {
    micron::const_pointer<int> p(7);
    sb::require(p() != nullptr);
  }
  sb::end_test_case();

  sb::test_case("operator() - address matches data()");
  {
    micron::const_pointer<int> p(7);
    sb::require(p() == p.data());
  }
  sb::end_test_case();

  sb::test_case("data() - returns correct non-null address");
  {
    micron::const_pointer<int> p(55);
    sb::require(p.data() != nullptr);
    sb::require(*p.data() == 55);
  }
  sb::end_test_case();

  // ── operator* / operator-> (scalar) ──────────────────────────────────────

  sb::test_case("operator* - reads correct value");
  {
    micron::const_pointer<int> p(123);
    sb::require(*p == 123);
  }
  sb::end_test_case();

  sb::test_case("operator* - does not modify object");
  {
    micron::const_pointer<int> p(77);
    (void)*p;
    sb::require(*p == 77);
  }
  sb::end_test_case();

  sb::test_case("operator-> - accesses struct member correctly");
  {
    micron::const_pointer<Point> p(4, 5);
    sb::require(p->x == 4);
    sb::require(p->y == 5);
  }
  sb::end_test_case();

  sb::test_case("operator* - double: precision preserved");
  {
    micron::const_pointer<double> p(3.14159265358979);
    sb::require(*p == 3.14159265358979);
  }
  sb::end_test_case();

  sb::test_case("operator* - INT_MAX preserved");
  {
    micron::const_pointer<int> p(INT_MAX);
    sb::require(*p == INT_MAX);
  }
  sb::end_test_case();

  sb::test_case("operator* - INT_MIN preserved");
  {
    micron::const_pointer<int> p(INT_MIN);
    sb::require(*p == INT_MIN);
  }
  sb::end_test_case();

  sb::test_case("operator* - uint64_t large value preserved");
  {
    micron::const_pointer<uint64_t> p(0xDEADBEEFCAFEBABEULL);
    sb::require(*p == 0xDEADBEEFCAFEBABEULL);
  }
  sb::end_test_case();

  // ── operator== (scalar) ───────────────────────────────────────────────────

  sb::test_case("operator== (T*) - matches own address");
  {
    micron::const_pointer<int> p(9);
    sb::require(p == p.data());
  }
  sb::end_test_case();

  sb::test_case("operator== (T*) - does not match different address");
  {
    micron::const_pointer<int> a(1);
    micron::const_pointer<int> b(2);
    sb::require(!(a == b.data()));
  }
  sb::end_test_case();

  sb::test_case("operator== (uintptr_t) - matches own address as integer");
  {
    micron::const_pointer<int> p(5);
    uintptr_t addr = reinterpret_cast<uintptr_t>(p.data());
    sb::require(p == addr);
  }
  sb::end_test_case();

  sb::test_case("operator== (uintptr_t) - does not match unrelated address");
  {
    micron::const_pointer<int> p(5);
    sb::require(!(p == uintptr_t(0)));
  }
  sb::end_test_case();

  // ── immutability (scalar) ─────────────────────────────────────────────────

  sb::test_case("immutability - value unchanged across multiple reads");
  {
    micron::const_pointer<int> p(42);
    sb::require(*p == 42);
    sb::require(*p == 42);
    sb::require(*p == 42);
  }
  sb::end_test_case();

  sb::test_case("immutability - string value unchanged across reads");
  {
    micron::const_pointer<micron::hstring<char>> p("immutable");
    sb::require(*p == "immutable");
    sb::require(*p == "immutable");
  }
  sb::end_test_case();

  // ── multiple independent instances (scalar) ───────────────────────────────

  sb::test_case("independence - two const_pointers hold distinct objects");
  {
    micron::const_pointer<int> a(10);
    micron::const_pointer<int> b(20);
    sb::require(*a == 10);
    sb::require(*b == 20);
    sb::require(a.data() != b.data());
  }
  sb::end_test_case();

  sb::test_case("independence - Counted: two instances, two destructions");
  {
    Counted::instances = 0;
    {
      micron::const_pointer<Counted> a(1);
      micron::const_pointer<Counted> b(2);
      sb::require(Counted::instances == 2);
      sb::require(a->id == 1);
      sb::require(b->id == 2);
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  // ── array spec construction ───────────────────────────────────────────────

  sb::test_case("array - default: null, size 0");
  {
    micron::const_pointer<int[]> p;
    sb::require(!p);
    sb::require(p.size() == 0ULL);
    sb::require(p.data() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("array - explicit size: non-null, correct size");
  {
    micron::const_pointer<int[]> p(4);
    sb::require(static_cast<bool>(p));
    sb::require(p.size() == 4ULL);
    sb::require(p.data() != nullptr);
  }
  sb::end_test_case();

  sb::test_case("array - lvalue raw pointer: takes ownership, source nulled, size stored");
  {
    int *raw = new int[3]{ 10, 20, 30 };
    micron::const_pointer<int[]> p(raw, 3);
    sb::require(raw == nullptr);
    sb::require(p.size() == 3ULL);
    sb::require(p[0] == 10);
    sb::require(p[1] == 20);
    sb::require(p[2] == 30);
  }
  sb::end_test_case();

  sb::test_case("array - destructor frees array (Counted)");
  {
    Counted::instances = 0;
    {
      micron::const_pointer<Counted[]> p(3);
      sb::require(Counted::instances == 3);
    }
    sb::require(Counted::instances == 0);
  }
  sb::end_test_case();

  // ── array operator[] ──────────────────────────────────────────────────────

  sb::test_case("array operator[] - reads all elements correctly");
  {
    int *raw = new int[5]{ 1, 2, 3, 4, 5 };
    micron::const_pointer<int[]> p(raw, 5);
    bool ok = true;
    for ( usize i = 0; i < 5 && ok; ++i )
      if ( p[i] != (int)(i + 1) )
        ok = false;
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("array operator[] - out-of-bounds throws");
  {
    micron::const_pointer<int[]> p(2);
    bool threw = false;
    try {
      (void)p[2];     // index == size, out of bounds
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  sb::test_case("array operator[] - access on null throws");
  {
    micron::const_pointer<int[]> p;     // default: null
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
    micron::const_pointer<int[]> p(raw, 3);
    sb::require(*p == 42);
  }
  sb::end_test_case();

  sb::test_case("array operator-> - accesses first struct member");
  {
    Point *raw = new Point[2]{ Point{ 1, 2 }, Point{ 3, 4 } };
    micron::const_pointer<Point[]> p(raw, 2);
    sb::require(p->x == 1);
    sb::require(p->y == 2);
  }
  sb::end_test_case();

  sb::test_case("array operator* - throws on null array");
  {
    micron::const_pointer<int[]> p;
    bool threw = false;
    try {
      (void)*p;
    } catch ( ... ) {
      threw = true;
    }
    sb::require(threw);
  }
  sb::end_test_case();

  // ── array operator() / data() ─────────────────────────────────────────────

  sb::test_case("array operator() - returns same address as data()");
  {
    micron::const_pointer<int[]> p(4);
    sb::require(p() == p.data());
  }
  sb::end_test_case();

  sb::test_case("array data() - returns non-null for live array");
  {
    micron::const_pointer<int[]> p(2);
    sb::require(p.data() != nullptr);
  }
  sb::end_test_case();

  // ── array operator== ──────────────────────────────────────────────────────

  sb::test_case("array operator== (T*) - matches own address");
  {
    micron::const_pointer<int[]> p(3);
    sb::require(p == p.data());
  }
  sb::end_test_case();

  sb::test_case("array operator== (uintptr_t) - matches own address as integer");
  {
    micron::const_pointer<int[]> p(3);
    uintptr_t addr = reinterpret_cast<uintptr_t>(p.data());
    sb::require(p == addr);
  }
  sb::end_test_case();

  // ── array immutability ────────────────────────────────────────────────────

  sb::test_case("array immutability - repeated reads return same values");
  {
    int *raw = new int[3]{ 7, 8, 9 };
    micron::const_pointer<int[]> p(raw, 3);
    sb::require(p[0] == 7);
    sb::require(p[1] == 8);
    sb::require(p[2] == 9);
    sb::require(p[0] == 7);     // re-read
    sb::require(p[2] == 9);
  }
  sb::end_test_case();

  sb::test_case("array immutability - size is stable");
  {
    micron::const_pointer<int[]> p(6);
    sb::require(p.size() == 6ULL);
    (void)p[0];
    sb::require(p.size() == 6ULL);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}
