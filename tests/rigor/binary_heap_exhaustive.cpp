// binary_heap_exhaustive.cpp
// Rigorous snowball test suite for micron::binary_heap<T> (a max-heap).
// Exercises insert/get/max ordering, capacity, move ctor/assign, clear, stress,
// edge cases, and non-trivial element lifetime balance.

#include "../../src/heap/binary_heap.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace
{
struct counted {
  static inline long live = 0;
  int v;

  counted() : v(0) { ++live; }

  counted(int x) : v(x) { ++live; }

  counted(const counted &o) : v(o.v) { ++live; }

  counted(counted &&o) noexcept : v(o.v) { ++live; }

  counted &
  operator=(const counted &o)
  {
    v = o.v;
    return *this;
  }

  counted &
  operator=(counted &&o) noexcept
  {
    v = o.v;
    return *this;
  }

  ~counted() { --live; }

  bool
  operator<(const counted &o) const
  {
    return v < o.v;
  }

  bool
  operator>(const counted &o) const
  {
    return v > o.v;
  }
};
}      // namespace

int
main()
{
  sb::print("=== BINARY_HEAP TESTS ===");

  test_case("default construction is empty");
  {
    micron::binary_heap<int> h;
    require(h.size(), size_t(0));
  }
  end_test_case();

  test_case("insert then max returns the maximum");
  {
    micron::binary_heap<int> h(64);
    h.insert(5);
    h.insert(9);
    h.insert(2);
    h.insert(7);
    require(h.max(), 9);
    require(h.size(), size_t(4));
  }
  end_test_case();

  test_case("get() pops in strictly descending order");
  {
    micron::binary_heap<int> h(64);
    int in[] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    for ( int x : in ) h.insert(int(x));
    int prev = 1000000;
    while ( h.size() ) {
      int cur = h.get();
      require_true(cur <= prev);
      prev = cur;
    }
    require(h.size(), size_t(0));
  }
  end_test_case();

  test_case("duplicates are preserved and ordered");
  {
    micron::binary_heap<int> h(32);
    for ( int i = 0; i < 5; i++ ) h.insert(7);
    require(h.size(), size_t(5));
    require(h.max(), 7);
    require(h.get(), 7);
    require(h.get(), 7);
    require(h.size(), size_t(3));
  }
  end_test_case();

  test_case("get/max on empty heap throws");
  {
    micron::binary_heap<int> h(8);
    require_throw([&] { (void)h.get(); });
    require_throw([&] { (void)h.max(); });
  }
  end_test_case();

  test_case("fixed capacity: insert beyond max_size is a no-op");
  {
    micron::binary_heap<int> h(4);
    for ( int i = 0; i < 10; i++ ) h.insert(int(i));
    require_true(h.size() <= h.max_size());
  }
  end_test_case();

  test_case("build-from-elements constructor (>= 2 args)");
  {
    micron::binary_heap<int> h(3, 7, 1, 9, 4);
    require(h.size(), size_t(5));
    require(h.max(), 9);
  }
  end_test_case();

  test_case("move construction transfers, source emptied");
  {
    micron::binary_heap<int> a(32);
    for ( int i = 0; i < 6; i++ ) a.insert(int(i));
    micron::binary_heap<int> b(micron::move(a));
    require(b.size(), size_t(6));
    require(a.size(), size_t(0));
    require(b.max(), 5);
  }
  end_test_case();

  test_case("move-assign onto NON-EMPTY destination (no leak/double-free)");
  {
    micron::binary_heap<int> dst(128);
    for ( int i = 0; i < 64; i++ ) dst.insert(int(i));
    micron::binary_heap<int> src(128);
    for ( int i = 0; i < 10; i++ ) src.insert(int(1000 + i));
    dst = micron::move(src);
    require(dst.size(), size_t(10));
    require(dst.max(), 1009);
  }
  end_test_case();

  test_case("self move-assign is safe");
  {
    micron::binary_heap<int> h(16);
    for ( int i = 0; i < 4; i++ ) h.insert(int(i));
    h = micron::move(h);
    require(h.size(), size_t(4));
  }
  end_test_case();

  test_case("clear empties and stays usable");
  {
    micron::binary_heap<int> h(64);
    for ( int i = 0; i < 20; i++ ) h.insert(int(i));
    h.clear();
    require(h.size(), size_t(0));
    h.insert(42);
    require(h.max(), 42);
  }
  end_test_case();

  test_case("stress: 2000 elements drain fully sorted descending");
  {
    micron::binary_heap<int> h(2048);
    for ( int i = 0; i < 2000; i++ ) h.insert((i * 7919) % 104729);
    require(h.size(), size_t(2000));
    int prev = 1000000, cnt = 0;
    while ( h.size() ) {
      int cur = h.get();
      require_true(cur <= prev);
      prev = cur;
      ++cnt;
    }
    require(cnt, 2000);
  }
  end_test_case();

  test_case("lifetime: non-trivial elements balance ctor/dtor");
  {
    counted::live = 0;
    {
      micron::binary_heap<counted> h(128);
      for ( int i = 0; i < 50; i++ ) h.insert(counted(i));
      for ( int i = 0; i < 20; i++ ) (void)h.get();
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("lifetime: move-assign over non-empty destroys old elements");
  {
    counted::live = 0;
    {
      micron::binary_heap<counted> dst(128);
      for ( int i = 0; i < 40; i++ ) dst.insert(counted(i));
      micron::binary_heap<counted> src(128);
      for ( int i = 0; i < 5; i++ ) src.insert(counted(100 + i));
      dst = micron::move(src);
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("capacity: request is NOT sizeof(T)x over-allocated");
  {

    struct wide {
      int k;
      char pad[60];

      bool
      operator<(const wide &o) const
      {
        return k < o.k;
      }

      bool
      operator>(const wide &o) const
      {
        return k > o.k;
      }
    };

    micron::binary_heap<wide> h(64);
    require_true(h.max_size() >= usize(64));
    require_true(h.max_size() < usize(64) * sizeof(wide));
  }
  end_test_case();

  test_case("insert is capped exactly at max_size (no write past capacity)");
  {
    micron::binary_heap<int> h(4);
    const usize cap = h.max_size();
    for ( usize i = 0; i < cap + 64; i++ ) h.insert((int)i);
    require(h.size(), cap);
    require_true(h.size() <= h.max_size());
  }
  end_test_case();

  test_case("get() on a single-element heap (self-move path) is lifetime-balanced");
  {
    counted::live = 0;
    {
      micron::binary_heap<counted> h(8);
      h.insert(counted(42));
      counted got = h.get();
      require(got.v, 42);
      require(h.size(), size_t(0));
    }
    require(counted::live, 0L);
  }
  end_test_case();

  sb::print("=== ALL BINARY_HEAP TESTS PASSED ===");
  return 1;
}
