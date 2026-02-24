// stack_tests.cpp
// Rigorous snowball test suite for micron::stack<T> and micron::fstack<T>

#include "../../src/stack.hpp"
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

// ---------------------------------------------------------------------------
//  Tracked: counts constructor / destructor calls to catch lifetime bugs
// ---------------------------------------------------------------------------
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
  operator!=(const Tracked &o) const
  {
    return v != o.v;
  }

  bool
  operator<(const Tracked &o) const
  {
    return v < o.v;
  }

  bool
  operator>(const Tracked &o) const
  {
    return v > o.v;
  }

  bool
  operator<=(const Tracked &o) const
  {
    return v <= o.v;
  }

  bool
  operator>=(const Tracked &o) const
  {
    return v >= o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

// ---------------------------------------------------------------------------
//  NoCopy: ensures move-only paths are exercised
// ---------------------------------------------------------------------------
struct NoCopy {
  int v;

  NoCopy() : v(0) {}

  explicit NoCopy(int x) : v(x) {}

  NoCopy(const NoCopy &) = delete;
  NoCopy &operator=(const NoCopy &) = delete;

  NoCopy(NoCopy &&o) noexcept : v(o.v) { o.v = -1; }

  NoCopy &
  operator=(NoCopy &&o) noexcept
  {
    v = o.v;
    o.v = -1;
    return *this;
  }

  bool
  operator==(const NoCopy &o) const
  {
    return v == o.v;
  }

  bool
  operator!=(const NoCopy &o) const
  {
    return v != o.v;
  }

  bool
  operator<(const NoCopy &o) const
  {
    return v < o.v;
  }
};

}     // anonymous namespace

// ===========================================================================
//  micron::stack  (safe / bounds-checked)
// ===========================================================================

int
main()
{
  sb::print("=== STACK TESTS (safe) ===");

  // -------------------------------------------------------------------------
  test_case("default construction");
  {
    micron::stack<int> s;
    require_true(s.empty());
    require(s.size(), size_t(0));
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("count constructor — default-initialises N elements");
  {
    micron::stack<int> s(8);
    require(s.size(), size_t(8));
    // all values must be zero-initialised
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 0);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("initializer_list constructor");
  {
    micron::stack<int> s{ 1, 2, 3, 4, 5 };
    require(s.size(), size_t(5));
    // operator[](0) is the top (last pushed), i.e. 5
    require(s[0], 5);
    require(s[4], 1);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("push lvalue and top()");
  {
    micron::stack<int> s;
    int val = 42;
    s.push(val);
    require(s.size(), size_t(1));
    require(s.top(), 42);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("push rvalue");
  {
    micron::stack<int> s;
    s.push(100);
    s.push(200);
    require(s.top(), 200);
    require(s.size(), size_t(2));
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("push default (no args)");
  {
    micron::stack<int> s;
    s.push();
    require(s.size(), size_t(1));
    require(s.top(), 0);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("pop() returns moved value");
  {
    micron::stack<int> s;
    s.push(7);
    s.push(13);
    int v = s.pop();
    require(v, 13);
    require(s.size(), size_t(1));
    require(s.top(), 7);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("pop() on empty stack throws");
  {
    micron::stack<int> s;
    require_throw([&]() { (void)s.pop(); });
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("top() on empty stack throws");
  {
    micron::stack<int> s;
    require_throw([&]() { (void)s.top(); });
    const micron::stack<int> cs;
    require_throw([&]() { (void)cs.top(); });
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("operator()() — combined top+pop, returns moved value");
  {
    micron::stack<int> s{ 10, 20, 30 };
    int v = s();
    require(v, 30);
    require(s.size(), size_t(2));
    require(s.top(), 20);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("operator()() on empty stack throws");
  {
    micron::stack<int> s;
    require_throw([&]() { (void)s(); });
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("move() push method");
  {
    micron::stack<int> s;
    int x = 99;
    s.move(micron::move(x));
    require(s.top(), 99);
    require(s.size(), size_t(1));
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("emplace");
  {
    micron::stack<Tracked> s;
    reset_tracked();
    Tracked t(7);
    s.emplace(t);
    require(s.top().v, 7);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("push_range variadic fold");
  {
    micron::stack<int> s;
    s.push_range(1, 2, 3, 4, 5);
    require(s.size(), size_t(5));
    // pushed left-to-right so 5 is at the top
    require(s.top(), 5);
    require(s[0], 5);
    require(s[4], 1);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("pop_range variadic fold");
  {
    micron::stack<int> s;
    s.push_range(10, 20, 30);
    int a, b, c;
    s.pop_range(a, b, c);
    // top is 30, so a=30, b=20, c=10
    require(a, 30);
    require(b, 20);
    require(c, 10);
    require_true(s.empty());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("pop_range throws when not enough elements");
  {
    micron::stack<int> s;
    s.push(1);
    int a, b;
    require_throw([&]() { s.pop_range(a, b); });
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("operator[] bounds checking");
  {
    micron::stack<int> s{ 1, 2, 3 };
    // [0] is fine (top)
    require(s[0], 3);
    // past end must throw
    require_throw([&]() { (void)s[100]; });
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("copy construction");
  {
    micron::stack<int> a{ 1, 2, 3 };
    micron::stack<int> b(a);
    require(b.size(), size_t(3));
    require(b.top() == 3);
    // mutating b must not affect a
    b.push(99);
    require(a.size(), size_t(3));
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("move construction");
  {
    micron::stack<int> a{ 4, 5, 6 };
    micron::stack<int> b(micron::move(a));
    require(b.size(), size_t(3));
    require(b.top(), 6);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("copy assignment");
  {
    micron::stack<int> a{ 1, 2, 3 };
    micron::stack<int> b;
    b = a;
    require(b.size(), size_t(3));
    require(b.top(), 3);
    b.pop();
    require(a.size(), size_t(3));     // a unchanged
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("move assignment");
  {
    micron::stack<int> a{ 7, 8, 9 };
    micron::stack<int> b;
    b = micron::move(a);
    require(b.size(), size_t(3));
    require(b.top(), 9);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("self copy assignment is safe");
  {
    micron::stack<int> s{ 1, 2, 3 };
    s = s;
    require(s.size(), size_t(3));
    require(s.top(), 3);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("reserve grows capacity");
  {
    micron::stack<int> s;
    s.reserve(256);
    require_greater(s.max_size(), size_t(0));
    size_t cap = s.max_size();
    s.reserve(64);     // smaller — no-op
    require(s.max_size(), cap);
    s.reserve(cap + 100);
    require_greater(s.max_size(), cap);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("clear");
  {
    micron::stack<int> s{ 1, 2, 3, 4, 5 };
    s.clear();
    require_true(s.empty());
    require(s.size(), size_t(0));
    // pushing after clear must still work
    s.push(42);
    require(s.top(), 42);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("swap");
  {
    micron::stack<int> a{ 1, 2, 3 };
    micron::stack<int> b{ 10, 20 };
    a.swap(b);
    require(a.size(), size_t(2));
    require(a.top(), 20);
    require(b.size(), size_t(3));
    require(b.top(), 3);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("equality operator ==");
  {
    micron::stack<int> a{ 1, 2, 3 };
    micron::stack<int> b{ 1, 2, 3 };
    micron::stack<int> c{ 1, 2, 4 };
    require_true(a == b);
    require_false(a == c);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("inequality operator !=");
  {
    micron::stack<int> a{ 1, 2, 3 };
    micron::stack<int> b{ 1, 2, 3 };
    micron::stack<int> c{ 9 };
    require_false(a != b);
    require_true(a != c);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("comparison operators < > <= >=");
  {
    micron::stack<int> a{ 1, 2, 3 };
    micron::stack<int> b{ 1, 2, 4 };
    micron::stack<int> c{ 1, 2, 3 };

    require_true(a < b);
    require_false(b < a);
    require_true(b > a);
    require_false(a > b);
    require_true(a <= c);
    require_true(a >= c);
    require_true(a <= b);
    require_false(b <= a);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("size-asymmetric comparison");
  {
    micron::stack<int> shorter{ 1, 2 };
    micron::stack<int> longer{ 1, 2, 0 };
    // shorter < longer because it has fewer elements after equal prefix
    require_true(shorter < longer);
    require_true(longer > shorter);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("iterators (bottom → top order)");
  {
    micron::stack<int> s{ 1, 2, 3 };
    // underlying storage is bottom-first: [1, 2, 3]
    int expected = 1;
    for ( auto it = s.begin(); it != s.end(); ++it )
      require(*it, expected++);
    require(expected, 4);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("const iterators");
  {
    const micron::stack<int> s{ 5, 6, 7 };
    int expected = 5;
    for ( auto it = s.cbegin(); it != s.cend(); ++it )
      require(*it, expected++);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("data() pointer validity");
  {
    micron::stack<int> s{ 10, 20, 30 };
    int *ptr = s.data();
    require(ptr[0], 10);
    require(ptr[2], 30);
    const micron::stack<int> cs{ 1, 2 };
    const int *cptr = cs.data();
    require(cptr[0], 1);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("static type-trait helpers");
  {
    require_true(micron::stack<int>::is_pod());
    require_false(micron::stack<int>::is_class_type());
    require_false(micron::stack<Tracked>::is_pod());
    require_true(micron::stack<Tracked>::is_class_type());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("tracked object lifetime — push/pop balance");
  {
    reset_tracked();
    {
      micron::stack<Tracked> s;
      for ( int i = 0; i < 64; ++i )
        s.push(Tracked(i));
      for ( int i = 0; i < 32; ++i )
        (void)s.pop();
      // destructor called on remaining 32 elements at scope exit
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("tracked object lifetime — initializer_list");
  {
    reset_tracked();
    {
      micron::stack<Tracked> s{ Tracked(1), Tracked(2), Tracked(3) };
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("tracked object lifetime — push_range / pop_range");
  {
    reset_tracked();
    {
      micron::stack<Tracked> s;
      Tracked t1(1), t2(2), t3(3);
      s.push_range(t1, t2, t3);
      require(s.size(), size_t(3));
      Tracked a, b, c;
      s.pop_range(a, b, c);
      require(a.v, 3);
      require(b.v, 2);
      require(c.v, 1);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("large push/pop stress");
  {
    micron::stack<int> s;
    constexpr int N = 100000;
    for ( int i = 0; i < N; ++i )
      s.push(i);
    require(s.size(), size_t(N));
    for ( int i = N - 1; i >= 0; --i ) {
      int v = s.pop();
      require(v, i);
    }
    require_true(s.empty());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("repeated clear + push cycles");
  {
    micron::stack<int> s;
    for ( int r = 0; r < 200; ++r ) {
      for ( int i = 0; i < 500; ++i )
        s.push(i);
      s.clear();
      require_true(s.empty());
    }
    // one final push to verify allocator is still valid after many cycles
    s.push(1);
    require(s.top(), 1);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("interleaved push and pop");
  {
    micron::stack<int> s;
    for ( int i = 0; i < 1000; ++i ) {
      s.push(i);
      if ( i % 3 == 0 ) {
        int v = s.pop();
        require(v, i);
      }
    }
    // stack must not be empty and must be internally consistent
    require_false(s.empty());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("push_range with mixed types (all convertible to T)");
  {
    micron::stack<double> s;
    s.push_range(1.1, 2.2, 3.3, 4.4);
    require(s.size(), size_t(4));
    // top must be 4.4
    require(s.top(), 4.4);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("operator& returns raw byte pointer to memory");
  {
    micron::stack<int> s{ 1, 2, 3 };
    auto *raw = &s;
    require_true(raw != nullptr);
    const micron::stack<int> cs{ 1 };
    const auto *craw = &cs;
    require_true(craw != nullptr);
  }
  end_test_case();

  // ===========================================================================
  sb::print("=== FSTACK TESTS (fast/unsafe) ===");
  // ===========================================================================

  // -------------------------------------------------------------------------
  test_case("[fstack] default construction");
  {
    micron::fstack<int> s;
    require_true(s.empty());
    require(s.size(), size_t(0));
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] count constructor");
  {
    micron::fstack<int> s(6);
    require(s.size(), size_t(6));
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 0);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] initializer_list constructor");
  {
    micron::fstack<int> s{ 10, 20, 30 };
    require(s.size(), size_t(3));
    require(s[0], 30);     // top
    require(s[2], 10);     // bottom
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] push lvalue / rvalue / default");
  {
    micron::fstack<int> s;
    int lv = 5;
    s.push(lv);
    s.push(10);
    s.push();
    require(s.size(), size_t(3));
    require(s.top(), 0);     // push() default-initialises
    require(s[1], 10);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] pop() returns moved value");
  {
    micron::fstack<int> s;
    s.push(55);
    s.push(77);
    int v = s.pop();
    require(v, 77);
    require(s.size(), size_t(1));
    require(s.top(), 55);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] operator()() combined top+pop");
  {
    micron::fstack<int> s{ 1, 2, 3 };
    require(s(), 3);
    require(s.size(), size_t(2));
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] move() named push");
  {
    micron::fstack<int> s;
    int x = 42;
    s.move(micron::move(x));
    require(s.top(), 42);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] push_range variadic");
  {
    micron::fstack<int> s;
    s.push_range(5, 10, 15, 20);
    require(s.size(), size_t(4));
    require(s.top(), 20);
    require(s[3], 5);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] pop_range variadic");
  {
    micron::fstack<int> s;
    s.push_range(100, 200, 300);
    int a, b, c;
    s.pop_range(a, b, c);
    require(a, 300);
    require(b, 200);
    require(c, 100);
    require_true(s.empty());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] copy construction");
  {
    micron::fstack<int> a{ 1, 2, 3 };
    micron::fstack<int> b(a);
    require(b.size(), size_t(3));
    require(b.top(), 3);
    b.push(99);
    require(a.size(), size_t(3));     // a unaffected
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] move construction");
  {
    micron::fstack<int> a{ 7, 8, 9 };
    micron::fstack<int> b(micron::move(a));
    require(b.size(), size_t(3));
    require(b.top(), 9);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] copy assignment");
  {
    micron::fstack<int> a{ 1, 2, 3 };
    micron::fstack<int> b;
    b = a;
    require(b.size(), size_t(3));
    require(b.top(), 3);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] move assignment");
  {
    micron::fstack<int> a{ 4, 5, 6 };
    micron::fstack<int> b;
    b = micron::move(a);
    require(b.size(), size_t(3));
    require(b.top(), 6);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] self copy assignment is safe");
  {
    micron::fstack<int> s{ 1, 2, 3 };
    s = s;
    require(s.size(), size_t(3));
    require(s.top(), 3);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] reserve");
  {
    micron::fstack<int> s;
    s.reserve(128);
    require_greater(s.max_size(), size_t(0));
    size_t cap = s.max_size();
    s.reserve(64);
    require(s.max_size(), cap);
    s.reserve(cap + 50);
    require_greater(s.max_size(), cap);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] clear");
  {
    micron::fstack<int> s{ 1, 2, 3 };
    s.clear();
    require_true(s.empty());
    s.push(7);
    require(s.top(), 7);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] swap");
  {
    micron::fstack<int> a{ 1, 2, 3 };
    micron::fstack<int> b{ 10 };
    a.swap(b);
    require(a.size(), size_t(1));
    require(a.top(), 10);
    require(b.size(), size_t(3));
    require(b.top(), 3);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] equality / inequality");
  {
    micron::fstack<int> a{ 1, 2, 3 };
    micron::fstack<int> b{ 1, 2, 3 };
    micron::fstack<int> c{ 1, 2 };
    require_true(a == b);
    require_false(a == c);
    require_true(a != c);
    require_false(a != b);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] comparison operators");
  {
    micron::fstack<int> a{ 1, 2, 3 };
    micron::fstack<int> b{ 1, 2, 4 };
    require_true(a < b);
    require_true(b > a);
    require_true(a <= b);
    require_false(b <= a);
    require_true(b >= a);
    require_false(a >= b);
    micron::fstack<int> c{ 1, 2, 3 };
    require_true(a <= c);
    require_true(a >= c);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] iterators");
  {
    micron::fstack<int> s{ 1, 2, 3 };
    int expected = 1;
    for ( auto it = s.begin(); it != s.end(); ++it )
      require(*it, expected++);
    const micron::fstack<int> cs{ 4, 5, 6 };
    expected = 4;
    for ( auto it = cs.cbegin(); it != cs.cend(); ++it )
      require(*it, expected++);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] data() pointer");
  {
    micron::fstack<int> s{ 1, 2, 3 };
    int *p = s.data();
    require(p[0], 1);
    require(p[2], 3);
  }
  end_test_case();
  // -------------------------------------------------------------------------
  test_case("[fstack] static type-trait helpers");
  {
    require_true(micron::fstack<int>::is_pod());
    require_false(micron::fstack<int>::is_class_type());
    require_false(micron::fstack<Tracked>::is_pod());
    require_true(micron::fstack<Tracked>::is_class_type());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] tracked lifetime — push/pop");
  {
    reset_tracked();
    {
      micron::fstack<Tracked> s;
      for ( int i = 0; i < 50; ++i )
        s.push(Tracked(i));
      for ( int i = 0; i < 25; ++i )
        (void)s.pop();
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] tracked lifetime — push_range / pop_range");
  {
    reset_tracked();
    {
      micron::fstack<Tracked> s;
      Tracked t1(1), t2(2), t3(3);
      s.push_range(t1, t2, t3);
      Tracked a, b, c;
      s.pop_range(a, b, c);
      require(a.v, 3);
      require(b.v, 2);
      require(c.v, 1);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] large stress push/pop");
  {
    micron::fstack<int> s;
    constexpr int N = 100000;
    for ( int i = 0; i < N; ++i )
      s.push(i);
    require(s.size(), size_t(N));
    for ( int i = N - 1; i >= 0; --i )
      require(s.pop(), i);
    require_true(s.empty());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] repeated clear + push cycles");
  {
    micron::fstack<int> s;
    for ( int r = 0; r < 200; ++r ) {
      for ( int i = 0; i < 500; ++i )
        s.push(i);
      s.clear();
      require_true(s.empty());
    }
    s.push(5);
    require(s.top(), 5);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("[fstack] interleaved push/pop");
  {
    micron::fstack<int> s;
    for ( int i = 0; i < 1000; ++i ) {
      s.push(i);
      if ( i % 2 == 0 ) {
        int v = s.pop();
        require(v, i);
      }
    }
    require_false(s.empty());
  }
  end_test_case();

  // =========================================================================
  sb::print("=== CROSS-CLASS PARITY CHECKS ===");
  // =========================================================================

  // -------------------------------------------------------------------------
  test_case("stack and fstack produce identical results for same push sequence");
  {
    micron::stack<int> safe;
    micron::fstack<int> fast;
    for ( int i = 0; i < 256; ++i ) {
      safe.push(i);
      fast.push(i);
    }
    require(safe.size(), fast.size());
    require(safe.top(), fast.top());
    while ( !safe.empty() ) {
      require(safe.pop(), fast.pop());
    }
    require_true(safe.empty());
    require_true(fast.empty());
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("push_range parity between stack and fstack");
  {
    micron::stack<int> safe;
    micron::fstack<int> fast;
    safe.push_range(1, 2, 3, 4, 5);
    fast.push_range(1, 2, 3, 4, 5);
    require(safe.size(), fast.size());
    for ( size_t i = 0; i < safe.size(); ++i )
      require(safe[i], fast[i]);
  }
  end_test_case();

  // -------------------------------------------------------------------------
  test_case("comparison operators are symmetric across equal stacks");
  {
    micron::stack<int> sa{ 1, 2, 3 };
    micron::stack<int> sb{ 1, 2, 3 };
    micron::fstack<int> fa{ 1, 2, 3 };
    micron::fstack<int> fb{ 1, 2, 3 };

    require_true(sa == sb);
    require_true(fa == fb);
    require_false(sa < sb);
    require_false(fa < fb);
    require_true(sa >= sb);
    require_true(fa >= fb);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
