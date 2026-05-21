

#include "../../src/queue/conqueue.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
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
  operator==(const counted &o) const
  {
    return v == o.v;
  }
};
}      // namespace

int
main()
{
  sb::print("=== CONQUEUE TESTS ===");

  test_case("default construction is empty");
  {
    micron::conqueue<int> q;
    require_true(q.empty());
    require(q.size(), size_t(0));
  }
  end_test_case();

  test_case("push increases size, pop decreases it");
  {
    micron::conqueue<int> q;
    for ( int i = 0; i < 10; i++ ) q.push(i);
    require(q.size(), size_t(10));
    require_false(q.empty());
    q.pop();
    q.pop();
    require(q.size(), size_t(8));
  }
  end_test_case();

  test_case("FIFO: last() is next-to-pop (oldest), front() is newest");
  {
    micron::conqueue<int> q;
    q.push(11);
    q.push(22);
    q.push(33);
    require(q.last(), 11);
    require(q.front(), 33);
    q.pop();
    require(q.last(), 22);
    q.pop();
    require(q.last(), 33);
  }
  end_test_case();

  test_case("push lvalue, rvalue, and default overloads");
  {
    micron::conqueue<int> q;
    int x = 7;
    q.push(x);
    q.push(99);
    q.push();
    require(q.size(), size_t(3));
  }
  end_test_case();

  test_case("pop on empty is a no-op (does not underflow)");
  {
    micron::conqueue<int> q;
    q.pop();
    q.pop();
    require(q.size(), size_t(0));
    require_true(q.empty());
  }
  end_test_case();

  test_case("clear empties the queue");
  {
    micron::conqueue<int> q;
    for ( int i = 0; i < 20; i++ ) q.push(i);
    q.clear();
    require(q.size(), size_t(0));
    require_true(q.empty());
    q.push(5);
    require(q.size(), size_t(1));
  }
  end_test_case();

  test_case("initializer_list constructor");
  {
    micron::conqueue<int> q{ 1, 2, 3, 4, 5 };
    require(q.size(), size_t(5));
  }
  end_test_case();

  test_case("move construction transfers contents, source emptied");
  {
    micron::conqueue<int> a;
    for ( int i = 0; i < 6; i++ ) a.push(i);
    micron::conqueue<int> b(micron::move(a));
    require(b.size(), size_t(6));
    require(a.size(), size_t(0));
  }
  end_test_case();

  test_case("move-assign onto NON-EMPTY destination (no leak/no double-free)");
  {
    micron::conqueue<int> dst;
    for ( int i = 0; i < 50; i++ ) dst.push(i);
    micron::conqueue<int> src;
    for ( int i = 0; i < 12; i++ ) src.push(1000 + i);
    dst = micron::move(src);
    require(dst.size(), size_t(12));
  }
  end_test_case();

  test_case("self move-assign is safe");
  {
    micron::conqueue<int> q;
    for ( int i = 0; i < 5; i++ ) q.push(i);
    q = micron::move(q);
    require(q.size(), size_t(5));
  }
  end_test_case();

  test_case("copy construction deep-copies");
  {
    micron::conqueue<int> a;
    for ( int i = 0; i < 8; i++ ) a.push(i);
    micron::conqueue<int> b(a);
    require(b.size(), a.size());
  }
  end_test_case();

  test_case("stress: many push/pop cycles keep size consistent");
  {
    micron::conqueue<int> q;
    for ( int i = 0; i < 500; i++ ) q.push(i);
    require(q.size(), size_t(500));
    for ( int i = 0; i < 300; i++ ) q.pop();
    require(q.size(), size_t(200));
    for ( int i = 0; i < 1000; i++ ) q.push(i);
    require(q.size(), size_t(1200));
  }
  end_test_case();

  test_case("lifetime: non-trivial elements balance ctor/dtor (destroy)");
  {
    counted::live = 0;
    {
      micron::conqueue<counted> q;
      for ( int i = 0; i < 30; i++ ) q.push(counted(i));
      q.pop();
      q.pop();
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("lifetime: move-assign over non-empty destroys old elements");
  {
    counted::live = 0;
    {
      micron::conqueue<counted> dst;
      for ( int i = 0; i < 20; i++ ) dst.push(counted(i));
      micron::conqueue<counted> src;
      for ( int i = 0; i < 5; i++ ) src.push(counted(100 + i));
      dst = micron::move(src);
    }
    require(counted::live, 0L);
  }
  end_test_case();

  sb::print("=== ALL CONQUEUE TESTS PASSED ===");
  return 1;
}
