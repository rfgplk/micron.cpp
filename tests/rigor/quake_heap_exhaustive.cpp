

#include "../../src/heap/quake_heap.hpp"
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
  sb::print("=== QUAKE_HEAP TESTS ===");

  test_case("default construction is empty");
  {
    micron::quake_heap<int> h;
    require_true(h.empty());
    require(h.size(), size_t(0));
  }
  end_test_case();

  test_case("insert then max returns the maximum");
  {
    micron::quake_heap<int> h;
    h.insert(5);
    h.insert(9);
    h.insert(2);
    h.insert(7);
    require(h.max(), 9);
    require(h.size(), size_t(4));
    require_false(h.empty());
  }
  end_test_case();

  test_case("insert(const T&) overload");
  {
    micron::quake_heap<int> h;
    int x = 42;
    h.insert(x);
    require(h.max(), 42);
  }
  end_test_case();

  test_case("pop returns elements in descending order");
  {
    micron::quake_heap<int> h;
    int in[] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    for ( int x : in ) h.insert(int(x));
    int prev = 1000000;
    while ( !h.empty() ) {
      int cur = h.pop();
      require_true(cur <= prev);
      prev = cur;
    }
    require(h.size(), size_t(0));
  }
  end_test_case();

  test_case("duplicates");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 5; i++ ) h.insert(7);
    require(h.size(), size_t(5));
    require(h.pop(), 7);
    require(h.pop(), 7);
    require(h.size(), size_t(3));
  }
  end_test_case();

  test_case("pop/max on empty throws");
  {
    micron::quake_heap<int> h;
    require_throw([&] { (void)h.pop(); });
    require_throw([&] { (void)h.max(); });
  }
  end_test_case();

  test_case("move construction transfers, source emptied");
  {
    micron::quake_heap<int> a;
    for ( int i = 0; i < 6; i++ ) a.insert(int(i));
    micron::quake_heap<int> b(micron::move(a));
    require(b.size(), size_t(6));
    require_true(a.empty());
    require(b.max(), 5);
  }
  end_test_case();

  test_case("move-assign onto NON-EMPTY destination (no leak/double-free)");
  {
    micron::quake_heap<int> dst;
    for ( int i = 0; i < 64; i++ ) dst.insert(int(i));
    micron::quake_heap<int> src;
    for ( int i = 0; i < 10; i++ ) src.insert(int(1000 + i));
    dst = micron::move(src);
    require(dst.size(), size_t(10));
    require(dst.max(), 1009);
  }
  end_test_case();

  test_case("self move-assign is safe");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 4; i++ ) h.insert(int(i));
    h = micron::move(h);
    require(h.size(), size_t(4));
  }
  end_test_case();

  test_case("clear empties and stays usable");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 20; i++ ) h.insert(int(i));
    h.clear();
    require_true(h.empty());
    h.insert(42);
    require(h.max(), 42);
  }
  end_test_case();

  test_case("stress: 2000 elements drain fully sorted descending");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 2000; i++ ) h.insert((i * 7919) % 104729);
    require(h.size(), size_t(2000));
    int prev = 1000000, cnt = 0;
    while ( !h.empty() ) {
      int cur = h.pop();
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
      micron::quake_heap<counted> h;
      for ( int i = 0; i < 50; i++ ) h.insert(counted(i));
      for ( int i = 0; i < 20; i++ ) (void)h.pop();
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("lifetime: move-assign over non-empty destroys old elements");
  {
    counted::live = 0;
    {
      micron::quake_heap<counted> dst;
      for ( int i = 0; i < 40; i++ ) dst.insert(counted(i));
      micron::quake_heap<counted> src;
      for ( int i = 0; i < 5; i++ ) src.insert(counted(100 + i));
      dst = micron::move(src);
    }
    require(counted::live, 0L);
  }
  end_test_case();

  sb::print("=== ALL QUAKE_HEAP TESTS PASSED ===");
  return 1;
}
