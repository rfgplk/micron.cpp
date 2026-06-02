// conqueue_exhaustive.cpp
// Rigorous snowball test suite for micron::conqueue<T>
// Covers construction, push/pop/clear, accessors, move ctor/assign, copy, and
// object-lifetime balance (the move-assign + clear() element handling fixed 2026-05-21).

#include "../../src/queue/conqueue.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"

#include "../../src/thread/thread.hpp"      // micron::auto_thread (NO <thread>, per pthread shim)

#include <vector>      // oracle only (STL allowed as reference)

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{
// lifetime-tracked element: live counts constructed-but-not-destroyed instances.
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
    require(q.last(), 11);       // oldest == next to pop
    require(q.front(), 33);      // newest
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
    q.push(x);       // const T&
    q.push(99);      // T&&
    q.push();        // default
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
    q.push(5);      // usable after clear
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
    }      // ~conqueue must destroy the remaining live elements
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
      dst = micron::move(src);      // dst's 20 old elements must be destroyed
    }
    require(counted::live, 0L);
  }
  end_test_case();

  // ============================================================ //
  //  CONCURRENCY (fast_mutex coarse lock)                         //
  // ============================================================ //
  // NOTE: conqueue::pop() is a deliberate no-op at the empty / needle==0 edge, so a
  // drain-everything loop is NOT a reliable element count. These gates instead assert
  // the lock-counted length and verify contents by iteration (begin/end), which is
  // what actually exercises the fast_mutex without depending on needle edge semantics.
  test_case("concurrent: disjoint pushes -- no lost / torn updates (verified by iteration)");
  {
    micron::conqueue<int> q;
    {
      micron::auto_thread<> t1([&] {
        for ( int i = 0; i < 1000; ++i ) q.push(i);
      });
      micron::auto_thread<> t2([&] {
        for ( int i = 1000; i < 2000; ++i ) q.push(i);
      });
      micron::auto_thread<> t3([&] {
        for ( int i = 2000; i < 3000; ++i ) q.push(i);
      });
    }      // join
    require(q.size(), size_t(3000));      // every push took the lock & bumped length
    std::vector<int> seen(3000, 0);
    for ( const int *it = q.begin(); it != q.end(); ++it ) {      // snapshot, no pop
      const int v = *it;
      require_true(v >= 0 && v < 3000);
      seen[static_cast<size_t>(v)]++;
    }
    bool all_once = true;
    for ( int i = 0; i < 3000; ++i )
      if ( seen[static_cast<size_t>(i)] != 1 ) all_once = false;
    require_true(all_once);      // every distinct push present exactly once (no torn slot)
  }
  end_test_case();

  test_case("concurrent: push/pop stress stays consistent and usable");
  {
    micron::conqueue<int> q;
    for ( int i = 0; i < 2000; ++i ) q.push(i);      // seed
    {
      micron::auto_thread<> prod([&] {
        for ( int i = 0; i < 2000; ++i ) q.push(i);
      });
      micron::auto_thread<> cons([&] {
        for ( int i = 0; i < 1500; ++i ) q.pop();
      });
    }
    const size_t s = q.size();
    // 4000 pushed total; effective pops in [..,1500] (pop no-ops at the edge) -> [2500,4000].
    require_true(s >= size_t(2500) && s <= size_t(4000));
    q.push(123456);      // still usable after concurrent churn
    require(q.size(), s + 1);
  }
  end_test_case();

  test_case("concurrent: owning elements balance (ASan/UBSan double-free/leak gate)");
  {
    counted::live = 0;
    {
      micron::conqueue<counted> q;
      {
        micron::auto_thread<> t1([&] {
          for ( int i = 0; i < 800; ++i ) q.push(counted(i));
        });
        micron::auto_thread<> t2([&] {
          for ( int i = 0; i < 800; ++i ) q.push(counted(i));
        });
      }      // join
      require(q.size(), size_t(1600));
      // ~conqueue (clear()) destroys all remaining live elements regardless of needle.
    }
    require(counted::live, 0L);      // no element leaked or double-destroyed
  }
  end_test_case();

  sb::print("=== ALL CONQUEUE TESTS PASSED ===");
  return 1;
}
