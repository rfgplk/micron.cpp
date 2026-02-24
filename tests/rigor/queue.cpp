// queue_tests.cpp
// Rigorous snowball test suite for micron::queue<T>

#include "../../src/queue/queue.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace
{

struct Tracked {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
    ++ctor;
  }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    return *this;
  }

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }

  bool
  operator>(const Tracked &o) const
  {
    return v > o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

}     // anonymous namespace

int
main()
{
  sb::print("=== QUEUE TESTS ===");

  // ------------------------------------------------------------
  test_case("default construction");
  {
    micron::queue<int> q;
    require_true(q.empty());
    require(q.size(), size_t(0));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("size + value constructor");
  {
    micron::queue<int> q(8, 42);
    require(q.size(), size_t(8));
    // every element should be 42
    for ( auto it = q.begin(); it != q.end(); ++it )
      require(*it, 42);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("default-value size constructor");
  {
    micron::queue<int> q(5);
    require(q.size(), size_t(5));
    for ( auto it = q.begin(); it != q.end(); ++it )
      require(*it, 0);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("initializer_list constructor");
  {
    micron::queue<int> q{ 10, 20, 30, 40 };
    require(q.size(), size_t(4));
    // front() is the first pushed element, last() is the most-recently pushed
    // initializer_list fills from capacity-1 downward, so iteration goes
    // begin → end in push order
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("move construction");
  {
    micron::queue<int> a{ 7, 8, 9 };
    micron::queue<int> b(micron::move(a));
    require(b.size(), size_t(3));
    require_true(a.empty());
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("push lvalue and rvalue");
  {
    micron::queue<int> q;
    int val = 99;
    q.push(val);
    require(q.size(), size_t(1));
    require(q.last(), 99);

    q.push(100);
    require(q.size(), size_t(2));
    require(q.last(), 99);       // last() is back/oldest-pushed end
    require(q.front(), 100);     // front() is the most-recently pushed element
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("pop removes from last (oldest) end");
  {
    micron::queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    // memory layout: [_ _ 1 2 3] with needle at capacity-1
    // last() == element at needle (oldest push = 1)
    // front() == most recent push = 3
    require(q.size(), size_t(3));
    require(q.last(), 1);
    q.pop();
    require(q.size(), size_t(2));
    require(q.last(), 2);
    q.pop();
    require(q.size(), size_t(1));
    require(q.last(), 3);
    q.pop();
    require_true(q.empty());
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("pop on empty queue is a no-op");
  {
    micron::queue<int> q;
    q.pop();     // must not crash
    require_true(q.empty());
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("front and last accessors");
  {
    micron::queue<int> q;
    for ( int i = 1; i <= 5; ++i )
      q.push(i);
    // push order: 1, 2, 3, 4, 5
    // last() is oldest = 1, front() is newest = 5
    require(q.last(), 1);
    require(q.front(), 5);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("const front and last");
  {
    const micron::queue<int> q{ 10, 20, 30 };
    require(q.size(), size_t(3));
    // just verify they compile and return sensible values
    (void)q.front();
    (void)q.last();
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("clear resets the queue");
  {
    micron::queue<int> q{ 1, 2, 3, 4, 5 };
    require(q.size(), size_t(5));
    q.clear();
    require_true(q.empty());
    require(q.size(), size_t(0));
    // can still push after clear
    q.push(42);
    require(q.size(), size_t(1));
    require(q.last(), 42);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("reserve grows capacity");
  {
    micron::queue<int> q(4, 0);
    size_t cap_before = q.max_size();
    q.reserve(cap_before + 32);
    require_greater(q.max_size(), cap_before);
    // elements must still be intact after reserve
    require(q.size() == size_t(4));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("reserve preserves element order");
  {
    micron::queue<int> q;
    for ( int i = 0; i < 10; ++i )
      q.push(i);
    q.reserve(q.max_size() + 64);
    // iteration must still yield 0..9 in push order (begin→end)
    int expected = 0;
    for ( auto it = q.begin(); it != q.end(); ++it )
      require(*it == expected++);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("swap exchanges contents");
  {
    micron::queue<int> a{ 1, 2, 3 };
    micron::queue<int> b{ 9, 8 };
    a.swap(b);
    require(a.size(), size_t(2));
    require(b.size(), size_t(3));
    require(b.last(), 1);
    require(a.last(), 9);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("iterator begin/end traversal");
  {
    micron::queue<int> q;
    for ( int i = 0; i < 20; ++i )
      q.push(i);
    int expected = 0;
    for ( auto it = q.begin(); it != q.end(); ++it )
      require(*it, expected++);
    require(expected == 20);
  }
  end_test_case();
  // ------------------------------------------------------------
  test_case("size and max_size are consistent");
  {
    micron::queue<int> q;
    for ( int i = 0; i < 50; ++i )
      q.push(i);
    require(q.size(), size_t(50));
    require_greater(q.max_size(), q.size());
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("push triggers automatic reserve");
  {
    // start with a tiny queue and keep pushing past capacity
    micron::queue<int, 2> q;
    for ( int i = 0; i < 512; ++i )
      q.push(i);
    require(q.size(), size_t(512));
    // verify content integrity
    int expected = 0;
    for ( auto it = q.begin(); it != q.end(); ++it )
      require(*it, expected++);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("large push/pop cycle");
  {
    micron::queue<int> q;
    constexpr int N = 10000;
    for ( int i = 0; i < N; ++i )
      q.push(i);
    require(q.size(), size_t(N));
    for ( int i = 0; i < N; ++i ) {
      require(q.last(), i);
      q.pop();
    }
    require_true(q.empty());
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("interleaved push and pop");
  {
    micron::queue<int> q;
    // push 5, pop 2, push 5, pop 3 ...
    for ( int round = 0; round < 50; ++round ) {
      for ( int i = 0; i < 5; ++i )
        q.push(round * 5 + i);
      q.pop();
      q.pop();
    }
    // 50 rounds × (5 push − 2 pop) = 150 elements remain
    require(q.size(), size_t(150));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("push default (no-arg) overload");
  {
    micron::queue<int> q;
    q.push();
    q.push();
    q.push();
    require(q.size(), size_t(3));
    for ( auto it = q.begin(); it != q.end(); ++it )
      require(*it, 0);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("tracked object lifetime — push/pop");
  {
    reset_tracked();
    {
      micron::queue<Tracked> q;
      for ( int i = 0; i < 64; ++i )
        q.push(Tracked(i));
      require(q.size(), size_t(64));
      for ( int i = 0; i < 32; ++i )
        q.pop();
      require(q.size(), size_t(32));
    }     // destructor runs here
    // every constructed object must have been destroyed exactly once
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("tracked object lifetime — clear");
  {
    reset_tracked();
    {
      micron::queue<Tracked> q;
      for ( int i = 0; i < 100; ++i )
        q.push(Tracked(i));
      q.clear();
      require_true(q.empty());
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("stress: repeated clear and refill");
  {
    micron::queue<int> q;
    for ( int round = 0; round < 100; ++round ) {
      for ( int i = 0; i < 500; ++i )
        q.push(i);
      q.clear();
      require_true(q.empty());
    }
    // final fill and integrity check
    for ( int i = 0; i < 200; ++i )
      q.push(i);
    require(q.size(), size_t(200));
    int expected = 0;
    for ( auto it = q.begin(); it != q.end(); ++it )
      require(*it, expected++);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("queue of floats");
  {
    micron::queue<float> q;
    for ( int i = 0; i < 10; ++i )
      q.push(static_cast<float>(i) * 0.5f);
    require(q.size(), size_t(10));
    float expected = 0.0f;
    for ( auto it = q.begin(); it != q.end(); ++it ) {
      require(*it, expected);
      expected += 0.5f;
    }
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("queue of pointers");
  {
    int arr[5] = { 10, 20, 30, 40, 50 };
    micron::queue<int *> q;
    for ( int i = 0; i < 5; ++i )
      q.push(&arr[i]);
    require(q.size(), size_t(5));
    int expected = 10;
    for ( auto it = q.begin(); it != q.end(); ++it ) {
      require(**it, expected);
      expected += 10;
    }
  }
  end_test_case();

  sb::print("=== ALL QUEUE TESTS PASSED ===");
  return 1;
}
