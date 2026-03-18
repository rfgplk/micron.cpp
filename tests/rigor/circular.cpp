// circle_vector_tests.cpp
// Rigorous snowball test suite for micron::circle_vector<T, N>

#include "../../src/circle_buffer.hpp"
#include "../../src/std.hpp"

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

// ------------------------------------------------------------------ //
//  Lifetime-tracking helper                                           //
// ------------------------------------------------------------------ //
struct Tracked {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = -1;
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
    o.v = -1;
    return *this;
  }

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

// ------------------------------------------------------------------ //
//  Non-trivial class to exercise deep-copy path                       //
// ------------------------------------------------------------------ //
struct StrBox {
  int id;

  bool
  operator==(const StrBox &o) const
  {
    return id == o.id;
  }
};

}     // anonymous namespace

int
main()
{
  sb::print("=== CIRCLE_VECTOR TESTS ===");

  // ---------------------------------------------------------------- //
  test_case("default construction – empty, zero size");
  {
    micron::circle_vector<int, 8> cv;
    require_true(cv.empty());
    require_false(cv.full());
    require(cv.size(), size_t(0));
    require(cv.capacity(), size_t(8));
    require(cv.max_size(), size_t(8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("single push then front/back and pop_front");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(42);
    require_false(cv.empty());
    require(cv.size(), size_t(1));
    require(cv.front(), 42);
    require(cv.back(), 42);

    cv.pop_front();
    require_true(cv.empty());
    require(cv.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_back is an alias for push");
  {
    micron::circle_vector<int, 8> cv;
    cv.push_back(7);
    require(cv.size(), size_t(1));
    require(cv.front(), 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move_back moves element in");
  {
    micron::circle_vector<Tracked, 8> cv;
    reset_tracked();
    Tracked t(55);
    cv.move_back(micron::move(t));
    require(cv.front().v, 55);
    require(cv.size(), size_t(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("emplace_back constructs in place");
  {
    micron::circle_vector<Tracked, 8> cv;
    reset_tracked();
    cv.emplace_back(99);
    require(cv.front().v, 99);
    require(cv.size(), size_t(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fill to capacity – full() becomes true");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 8; ++i )
      cv.push(i);
    require_true(cv.full());
    require(cv.size(), size_t(8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("FIFO ordering – push N elements, pop N in order");
  {
    micron::circle_vector<int, 16> cv;
    for ( int i = 0; i < 16; ++i )
      cv.push(i);
    for ( int i = 0; i < 16; ++i ) {
      require(cv.front(), i);
      cv.pop_front();
    }
    require_true(cv.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[] logical indexing");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 5; ++i )
      cv.push(i * 10);
    for ( size_t i = 0; i < 5; ++i )
      require(cv[i], (int)(i * 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() returns correct element");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 4; ++i )
      cv.push(i + 1);
    require(cv.at(0), 1);
    require(cv.at(3), 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() throws on out-of-bounds index");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(1);
    cv.push(2);
    require_throw([&]() { (void)cv.at(2); });
    require_throw([&]() { (void)cv.at(99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const at() throws on out-of-bounds");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(10);
    const auto &ccv = cv;
    require_throw([&]() { (void)ccv.at(1); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("overwrite behaviour – pushing past capacity advances tail");
  {
    micron::circle_vector<int, 4> cv;
    // fill: [0,1,2,3]
    for ( int i = 0; i < 4; ++i )
      cv.push(i);
    require_true(cv.full());
    require(cv.size(), size_t(4));

    // push one more – oldest (0) is silently overwritten
    cv.push(99);
    require_true(cv.full());
    require(cv.size(), size_t(4));
    require(cv.front(), 1);     // 0 was evicted
    require(cv.back(), 99);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("overwrite N times preserves last N elements in order");
  {
    micron::circle_vector<int, 4> cv;
    for ( int i = 0; i < 20; ++i )
      cv.push(i);

    // After 20 pushes into cap-4, the last 4 values are 16,17,18,19
    require(cv.size(), size_t(4));
    for ( int i = 0; i < 4; ++i )
      require(cv[i], 16 + i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop() returns moved value and decrements size");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(10);
    cv.push(20);
    cv.push(30);
    int v = cv.pop();
    require(v, 10);
    require(cv.size(), size_t(2));
    v = cv.pop();
    require(v, 20);
    require(cv.size(), size_t(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop_front on empty does not underflow size");
  {
    micron::circle_vector<int, 8> cv;
    cv.pop_front();     // must not crash or wrap size
    require_true(cv.empty());
    require(cv.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear resets to empty");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 8; ++i )
      cv.push(i);
    cv.clear();
    require_true(cv.empty());
    require_false(cv.full());
    require(cv.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("reuse after clear works correctly");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 8; ++i )
      cv.push(i);
    cv.clear();
    for ( int i = 100; i < 108; ++i )
      cv.push(i);
    require_true(cv.full());
    for ( int i = 0; i < 8; ++i )
      require(cv[i], 100 + i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("front() and back() after partial fill");
  {
    micron::circle_vector<int, 16> cv;
    cv.push(1);
    cv.push(2);
    cv.push(3);
    require(cv.front(), 1);
    require(cv.back(), 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("front() and back() update after pop_front");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(10);
    cv.push(20);
    cv.push(30);
    cv.pop_front();
    require(cv.front(), 20);
    require(cv.back(), 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const front() and back() are accessible");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(5);
    cv.push(6);
    const auto &ccv = cv;
    require(ccv.front(), 5);
    require(ccv.back(), 6);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy construction produces independent clone");
  {
    micron::circle_vector<int, 8> a;
    for ( int i = 0; i < 5; ++i )
      a.push(i);

    micron::circle_vector<int, 8> b(a);
    require(b.size(), a.size());
    for ( size_t i = 0; i < b.size(); ++i )
      require(b[i], a[i]);

    // mutate b – a must be unaffected
    b.push(99);
    require(a.size(), size_t(5));
    require(b.size(), size_t(6));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy assignment produces independent clone");
  {
    micron::circle_vector<int, 8> a;
    for ( int i = 0; i < 4; ++i )
      a.push(i * 3);

    micron::circle_vector<int, 8> b;
    b = a;
    require(b.size(), a.size());
    for ( size_t i = 0; i < b.size(); ++i )
      require(b[i], a[i]);

    b.clear();
    require(a.size(), size_t(4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self-assignment is a no-op");
  {
    micron::circle_vector<int, 8> a;
    a.push(1);
    a.push(2);
    a = a;
    require(a.size(), size_t(2));
    require(a[0], 1);
    require(a[1], 2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction transfers content and clears source");
  {
    micron::circle_vector<int, 8> a;
    for ( int i = 0; i < 5; ++i )
      a.push(i);

    micron::circle_vector<int, 8> b(micron::move(a));
    require(b.size(), size_t(5));
    for ( int i = 0; i < 5; ++i )
      require(b[i], i);

    require_true(a.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment transfers content and clears source");
  {
    micron::circle_vector<int, 8> a;
    for ( int i = 0; i < 6; ++i )
      a.push(i + 10);

    micron::circle_vector<int, 8> b;
    b = micron::move(a);
    require(b.size(), size_t(6));
    for ( int i = 0; i < 6; ++i )
      require(b[i], i + 10);

    require_true(a.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator begin/end covers all elements in order");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 6; ++i )
      cv.push(i * 2);

    int expected = 0;
    for ( auto it = cv.begin(); it != cv.end(); ++it ) {
      require(*it, expected);
      expected += 2;
    }
    require(expected, 12);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("range-for loop iterates correctly");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 1; i <= 5; ++i )
      cv.push(i);

    int sum = 0;
    for ( auto v : cv )
      sum += v;
    require(sum, 15);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const iterator (cbegin/cend) works");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(3);
    cv.push(6);
    cv.push(9);

    int product = 1;
    for ( auto it = cv.cbegin(); it != cv.cend(); ++it )
      product *= *it;
    require(product, 162);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator pre/post increment and decrement");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 4; ++i )
      cv.push(i);

    auto it = cv.begin();
    require(*it, 0);
    ++it;
    require(*it, 1);
    auto old = it++;
    require(*old, 1);
    require(*it, 2);
    --it;
    require(*it, 1);
    auto old2 = it--;
    require(*old2, 1);
    require(*it, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator arithmetic and subscript");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 6; ++i )
      cv.push(i);

    auto it = cv.begin();
    require(*(it + 3), 3);
    require(it[4], 4);

    auto it2 = cv.end();
    require(*(it2 - 1), 5);
    require(it2 - it, ptrdiff_t(6));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator comparison operators");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 4; ++i )
      cv.push(i);

    auto a = cv.begin();
    auto b = cv.begin() + 2;

    require_true(a < b);
    require_true(a <= b);
    require_false(a > b);
    require_false(a >= b);
    require_true(a != b);
    require_false(a == b);

    auto c = cv.begin();
    require_true(a == c);
    require_true(a <= c);
    require_true(a >= c);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iterator wrap-around after overwrite cycle");
  {
    // Push 4+3 into cap-4 so head has wrapped; iterate and verify order
    micron::circle_vector<int, 4> cv;
    for ( int i = 0; i < 7; ++i )
      cv.push(i);
    // last 4 pushed: 3,4,5,6
    int expected = 3;
    for ( auto v : cv ) {
      require(v, expected);
      ++expected;
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("data() pointer is non-null");
  {
    micron::circle_vector<int, 8> cv;
    require_true(cv.data() != nullptr);

    const auto &ccv = cv;
    require_true(ccv.data() != nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("is_pod / is_class_type / is_trivial static helpers");
  {
    // int is a POD, non-class, trivial
    require_true(micron::circle_vector<int, 8>::is_pod());
    require_false(micron::circle_vector<int, 8>::is_class_type());
    require_true(micron::circle_vector<int, 8>::is_trivial());

    // Tracked is a class, non-trivial, non-POD
    require_false(micron::circle_vector<Tracked, 8>::is_pod());
    require_true(micron::circle_vector<Tracked, 8>::is_class_type());
    require_false(micron::circle_vector<Tracked, 8>::is_trivial());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: push/pop_front alternating keeps size at 1");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 10000; ++i ) {
      cv.push(i);
      require(cv.size(), size_t(1));
      require(cv.front(), i);
      cv.pop_front();
    }
    require_true(cv.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: repeated fill and overwrite preserves last N");
  {
    constexpr size_t CAP = 16;
    micron::circle_vector<int, CAP> cv;
    for ( int i = 0; i < 10000; ++i )
      cv.push(i);

    for ( size_t i = 0; i < CAP; ++i )
      require(cv[i], (int)(10000 - CAP + i));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: clear and refill many times");
  {
    micron::circle_vector<int, 8> cv;
    for ( int round = 0; round < 1000; ++round ) {
      for ( int i = 0; i < 8; ++i )
        cv.push(i);
      require_true(cv.full());
      cv.clear();
      require_true(cv.empty());
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("Tracked copy construction – deep copy path exercised");
  {
    reset_tracked();
    micron::circle_vector<Tracked, 8> a;
    for ( int i = 0; i < 5; ++i )
      a.emplace_back(i);

    micron::circle_vector<Tracked, 8> b(a);
    for ( size_t i = 0; i < 5; ++i )
      require(b[i].v, (int)i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("emplace_back with multi-arg construction");
  {
    struct Pair {
      int x, y;
    };

    micron::circle_vector<Pair, 8> cv;
    cv.emplace_back(3, 4);
    require(cv.front().x, 3);
    require(cv.front().y, 4);
  }
  end_test_case();

  sb::print("=== ALL CIRCLE_VECTOR TESTS PASSED ===");
  return 0;
}
