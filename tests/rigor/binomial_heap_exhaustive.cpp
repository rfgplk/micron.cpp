// binomial_heap_exhaustive.cpp

#include "../../src/heap/binomial.hpp"
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

  bool
  operator==(const counted &o) const
  {
    return v == o.v;
  }
};

static_assert(!micron::is_copy_constructible_v<micron::__binomial_heap<int>>, "__binomial_heap must NOT be copy-constructible");
static_assert(micron::is_move_constructible_v<micron::__binomial_heap<int>>, "__binomial_heap must be move-constructible");
}      // namespace

int
main()
{
  sb::print("=== BINOMIAL_HEAP TESTS ===");

  test_case("default construction is empty (default comparator compiles)");
  {
    micron::__binomial_heap<int> h;
    require_true(h.empty());
    require(h.size(), usize(0));
  }
  end_test_case();

  test_case("insert then find_min returns the minimum");
  {
    micron::__binomial_heap<int> h;
    h.insert(5);
    h.insert(9);
    h.insert(2);
    h.insert(7);
    require(h.find_min(), 2);
    require(h.size(), usize(4));
    require_false(h.empty());
  }
  end_test_case();

  test_case("extract_min pops in strictly ascending order");
  {
    micron::__binomial_heap<int> h;
    int in[] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    for ( int x : in ) h.insert(int(x));
    int prev = -1;
    usize cnt = 0;
    while ( !h.empty() ) {
      int cur = h.extract_min();
      require_true(cur >= prev);
      prev = cur;
      ++cnt;
    }
    require(cnt, usize(8));
  }
  end_test_case();

  test_case("emplace constructs in place");
  {
    micron::__binomial_heap<int> h;
    h.emplace(42);
    h.emplace(7);
    require(h.find_min(), 7);
    require(h.size(), usize(2));
  }
  end_test_case();

  test_case("rvalue insert");
  {
    micron::__binomial_heap<int> h;
    int a = 11, b = 4;
    h.insert(micron::move(a));
    h.insert(micron::move(b));
    require(h.find_min(), 4);
  }
  end_test_case();

  test_case("find_min / extract_min on empty heap throws");
  {
    micron::__binomial_heap<int> h;
    require_throw([&] { (void)h.find_min(); });
    require_throw([&] { (void)h.extract_min(); });
  }
  end_test_case();

  test_case("decrease_key lowers a key and updates the global min");
  {
    micron::__binomial_heap<int> h;
    auto *n10 = h.insert(10);
    auto *n20 = h.insert(20);
    auto *n30 = h.insert(30);
    (void)n10;
    (void)n30;
    require(h.find_min(), 10);
    h.decrease_key(n20, 5);
    require(h.find_min(), 5);
    require(h.extract_min(), 5);
    require(h.extract_min(), 10);
    require(h.extract_min(), 30);
    require_true(h.empty());
  }
  end_test_case();

  test_case("decrease_key to a non-smaller value is a no-op");
  {
    micron::__binomial_heap<int> h;
    auto *n5 = h.insert(5);
    h.insert(8);
    h.decrease_key(n5, 9);
    require(h.find_min(), 5);
    require(h.size(), usize(2));
  }
  end_test_case();

  test_case("delete_node removes the TARGET element, not the global min");
  {
    micron::__binomial_heap<int> h;
    h.insert(10);
    auto *n20 = h.insert(20);
    h.insert(30);
    h.insert(40);
    h.delete_node(n20);
    require(h.size(), usize(3));
    require(h.extract_min(), 10);
    require(h.extract_min(), 30);
    require(h.extract_min(), 40);
    require_true(h.empty());
  }
  end_test_case();

  test_case("meld consumes the other heap; result is the union");
  {
    micron::__binomial_heap<int> a;
    a.insert(3);
    a.insert(7);
    micron::__binomial_heap<int> b;
    b.insert(1);
    b.insert(5);
    b.insert(9);
    a.meld(b);
    require(a.size(), usize(5));
    require_true(b.empty());
    require(a.find_min(), 1);
    int prev = -1;
    while ( !a.empty() ) {
      int cur = a.extract_min();
      require_true(cur >= prev);
      prev = cur;
    }
  }
  end_test_case();

  test_case("find locates present values and returns nullptr for absent");
  {
    micron::__binomial_heap<int> h;
    for ( int i = 0; i < 8; i++ ) h.insert(int(i * 3));
    auto *f = h.find(12);
    require_true(f != nullptr);
    require(f->value, 12);
    require_true(h.find(13) == nullptr);
  }
  end_test_case();

  test_case("move construction transfers, source emptied");
  {
    micron::__binomial_heap<int> a;
    a.insert(1);
    a.insert(2);
    a.insert(3);
    micron::__binomial_heap<int> b(micron::move(a));
    require(b.size(), usize(3));
    require(a.size(), usize(0));
    require_true(a.empty());
    require(b.find_min(), 1);
  }
  end_test_case();

  test_case("move-assign onto non-empty destination (no leak/double-free)");
  {
    micron::__binomial_heap<int> dst;
    for ( int i = 0; i < 20; i++ ) dst.insert(int(i));
    micron::__binomial_heap<int> src;
    for ( int i = 0; i < 5; i++ ) src.insert(int(100 + i));
    dst = micron::move(src);
    require(dst.size(), usize(5));
    require(dst.find_min(), 100);
    require(src.size(), usize(0));
  }
  end_test_case();

  test_case("clear empties and stays usable");
  {
    micron::__binomial_heap<int> h;
    for ( int i = 0; i < 20; i++ ) h.insert(int(i));
    h.clear();
    require(h.size(), usize(0));
    require_true(h.empty());
    h.insert(42);
    require(h.find_min(), 42);
  }
  end_test_case();

  test_case("stress: 2000 elements drain fully sorted ascending");
  {
    micron::__binomial_heap<int> h;
    for ( int i = 0; i < 2000; i++ ) h.insert((i * 7919) % 104729);
    require(h.size(), usize(2000));
    int prev = -1, cnt = 0;
    while ( !h.empty() ) {
      int cur = h.extract_min();
      require_true(cur >= prev);
      prev = cur;
      ++cnt;
    }
    require(cnt, 2000);
  }
  end_test_case();

  test_case("lifetime: non-trivial elements balance ctor/dtor (no double-free)");
  {
    counted::live = 0;
    {
      micron::__binomial_heap<counted> h;
      for ( int i = 0; i < 50; i++ ) h.insert(counted(i));
      for ( int i = 0; i < 20; i++ ) (void)h.extract_min();
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("lifetime: move-assign over non-empty destroys old elements");
  {
    counted::live = 0;
    {
      micron::__binomial_heap<counted> dst;
      for ( int i = 0; i < 40; i++ ) dst.insert(counted(i));
      micron::__binomial_heap<counted> src;
      for ( int i = 0; i < 5; i++ ) src.insert(counted(100 + i));
      dst = micron::move(src);
    }
    require(counted::live, 0L);
  }
  end_test_case();

  sb::print("=== ALL BINOMIAL_HEAP TESTS PASSED ===");
  return 1;
}
