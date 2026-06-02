// heapq_exhaustive.cpp

#include "../../src/heap/heapq.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"
#include "../../src/thread/thread.hpp"

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
};
}      // namespace

int
main()
{
  sb::print("=== HEAPQ TESTS ===");

  test_case("default construction is empty");
  {
    micron::heapq<int> h;
    require_true(h.empty());
    require(h.size(), usize(0));
  }
  end_test_case();

  test_case("push then top returns the minimum");
  {
    micron::heapq<int> h;
    h.push(5);
    h.push(9);
    h.push(2);
    h.push(7);
    require(h.top(), 2);
    require(h.size(), usize(4));
    require_false(h.empty());
  }
  end_test_case();

  test_case("pop returns in strictly ascending order");
  {
    micron::heapq<int> h;
    int in[] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    for ( int x : in ) h.push(x);
    int prev = -1;
    usize cnt = 0;
    while ( !h.empty() ) {
      int cur = h.pop();
      require_true(cur >= prev);
      prev = cur;
      ++cnt;
    }
    require(cnt, usize(8));
  }
  end_test_case();

  test_case("duplicates are preserved and ordered");
  {
    micron::heapq<int> h;
    for ( int i = 0; i < 5; i++ ) h.push(7);
    require(h.size(), usize(5));
    require(h.top(), 7);
    require(h.pop(), 7);
    require(h.pop(), 7);
    require(h.size(), usize(3));
  }
  end_test_case();

  test_case("pop / top on empty heap throws");
  {
    micron::heapq<int> h;
    require_throw([&] { (void)h.pop(); });
    require_throw([&] { (void)h.top(); });
  }
  end_test_case();

  test_case("heapify-from-fvector constructor");
  {
    micron::fvector<int> v;
    int d[] = { 4, 1, 7, 3, 8, 2, 6, 5 };
    for ( int x : d ) v.push_back(x);
    micron::heapq<int> h(v);
    require(h.size(), usize(8));
    require(h.top(), 1);
    int prev = -1;
    while ( !h.empty() ) {
      int cur = h.pop();
      require_true(cur >= prev);
      prev = cur;
    }
  }
  end_test_case();

  test_case("copy construction snapshots; source is unaffected");
  {
    micron::heapq<int> h;
    h.push(5);
    h.push(2);
    h.push(8);
    micron::heapq<int> c(h);
    require(c.size(), usize(3));
    require(c.top(), 2);
    require(h.size(), usize(3));
    require(h.top(), 2);
  }
  end_test_case();

  test_case("move construction transfers; source emptied");
  {
    micron::heapq<int> h;
    h.push(5);
    h.push(2);
    micron::heapq<int> m(micron::move(h));
    require(m.size(), usize(2));
    require(m.top(), 2);
    require(h.size(), usize(0));
  }
  end_test_case();

  test_case("copy-assign and move-assign");
  {
    micron::heapq<int> a;
    a.push(3);
    a.push(1);
    micron::heapq<int> b;
    b.push(99);
    b = a;
    require(b.size(), usize(2));
    require(b.top(), 1);
    require(a.size(), usize(2));
    micron::heapq<int> d;
    d = micron::move(b);
    require(d.size(), usize(2));
    require(d.top(), 1);
  }
  end_test_case();

  test_case("self copy-assign and self move-assign are safe");
  {
    micron::heapq<int> h;
    h.push(4);
    h.push(2);
    h.push(6);
    h = h;
    require(h.size(), usize(3));
    require(h.top(), 2);
    h = micron::move(h);
    require(h.size(), usize(3));
    require(h.top(), 2);
  }
  end_test_case();

  test_case("data() exposes the backing store");
  {
    micron::heapq<int> h;
    h.push(10);
    h.push(20);
    require(h.data().size(), usize(2));
  }
  end_test_case();

  test_case("lifetime: non-trivial elements balance ctor/dtor");
  {
    counted::live = 0;
    {
      micron::heapq<counted> h;
      for ( int i = 0; i < 30; i++ ) h.push(counted(i));
      for ( int i = 0; i < 10; i++ ) (void)h.pop();
      micron::heapq<counted> c(h);
      (void)c;
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("thread-safety smoke: 2 threads push disjoint ranges concurrently");
  {
    micron::heapq<int> h;
    {
      micron::auto_thread<> t1([&] {
        for ( int i = 0; i < 500; i++ ) h.push(i);
      });
      micron::auto_thread<> t2([&] {
        for ( int i = 500; i < 1000; i++ ) h.push(i);
      });
    }
    require(h.size(), usize(1000));
    int prev = -1;
    usize cnt = 0;
    bool ascending = true;
    while ( !h.empty() ) {
      int cur = h.pop();
      if ( cur < prev ) ascending = false;
      prev = cur;
      ++cnt;
    }
    require_true(ascending);
    require(cnt, usize(1000));
  }
  end_test_case();

  sb::print("=== ALL HEAPQ TESTS PASSED ===");
  return 1;
}
