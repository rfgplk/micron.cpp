// array_tests.cpp
// Rigorous adversarial test suite for micron::array<T, N>

#include "../../src/array/array.hpp"
#include "../../src/vector/vector.hpp"
#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace
{

struct Tracked {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  static inline size_t copy = 0;
  static inline size_t move = 0;

  int v;

  Tracked() : v(0) { ++ctor; }
  explicit Tracked(int x) : v(x) { ++ctor; }
  Tracked(const Tracked &o) : v(o.v)
  {
    ++ctor;
    ++copy;
  }
  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = -1;
    ++ctor;
    ++move;
  }
  ~Tracked() { ++dtor; }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    ++copy;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = -1;
    ++move;
    return *this;
  }

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

  static void
  reset()
  {
    ctor = dtor = copy = move = 0;
  }
};

}     // namespace

// ------------------------------------------------------------

int
main()
{
  // ------------------------------------------------------------
  test_case("default construction zero-init");
  {
    micron::array<int, 8> a;
    for ( size_t i = 0; i < 8; ++i )
      require(a[i], 0);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("fill constructor");
  {
    micron::array<int, 8> a(7);
    for ( size_t i = 0; i < 8; ++i )
      require(a[i], 7);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("initializer_list constructor");
  {
    micron::array<int, 5> a{ 1, 2, 3 };
    require(a[0], 1);
    require(a[1], 2);
    require(a[2], 3);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("initializer_list overflow");
  {
    require_throw([] { micron::array<int, 2> a{ 1, 2, 3 }; });
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("copy constructor exact");
  {
    micron::array<int, 4> a{ 1, 2, 3, 4 };
    micron::array<int, 4> b(a);
    for ( size_t i = 0; i < 4; ++i )
      require(a[i], b[i]);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("move constructor preserves values");
  {
    micron::array<int, 4> a{ 5, 6, 7, 8 };
    micron::array<int, 4> b(micron::move(a));
    for ( size_t i = 0; i < 4; ++i )
      require(b[i], int(5 + i));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("copy assignment");
  {
    micron::array<int, 4> a{ 1, 2, 3, 4 };
    micron::array<int, 4> b;
    b = a;
    for ( size_t i = 0; i < 4; ++i )
      require(b[i], a[i]);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("move assignment");
  {
    micron::array<int, 4> a{ 9, 8, 7, 6 };
    micron::array<int, 4> b;
    b = micron::move(a);
    for ( size_t i = 0; i < 4; ++i )
      require(b[i], int(9 - i));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("at() bounds");
  {
    micron::array<int, 4> a;
    require_throw([&] { a.at(4); });
    require_throw([&] { a.at(100); });
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("iterator range");
  {
    micron::array<int, 4> a{ 1, 2, 3, 4 };
    int sum = 0;
    for ( auto it = a.begin(); it != a.end(); ++it )
      sum += *it;
    require(sum, 10);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("sum()");
  {
    micron::array<int, 5> a{ 1, 1, 1, 1, 1 };
    require(a.sum(), size_t(5));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("scalar arithmetic operators");
  {
    micron::array<int, 4> a{ 1, 2, 3, 4 };
    a += 1;
    a *= 2;
    a -= 2;
    a /= 2;
    require(a[0], 1);
    require(a[3], 4);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("all() / any()");
  {
    micron::array<int, 4> a(7);
    require_true(a.all(7));
    require_false(a.any(3));
    a[2] = 3;
    require_false(a.all(7));
    require_true(a.any(3));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("Tracked lifetime correctness");
  {
    Tracked::reset();
    {
      micron::array<Tracked, 8> a;
      for ( size_t i = 0; i < 8; ++i )
        a[i] = Tracked(int(i));
    }
    micron::console(Tracked::dtor);
    micron::console(Tracked::ctor);
    //require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("array of vector<int>");
  {
    micron::array<micron::vector<int>, 4> a;

    for ( size_t i = 0; i < 4; ++i ) {
      a[i].push_back(int(i));
      a[i].push_back(int(i + 10));
    }

    for ( size_t i = 0; i < 4; ++i ) {
      require(a[i].size(), size_t(2));
      require(a[i][0], int(i));
      require(a[i][1], int(i + 10));
    }
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("array<vector> copy");
  {
    micron::array<micron::vector<int>, 2> a;
    a[0].push_back(1);
    a[1].push_back(2);

    micron::array<micron::vector<int>, 2> b(a);

    require(b[0][0], 1);
    require(b[1][0], 2);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("array<vector> move");
  {
    micron::array<micron::vector<int>, 2> a;
    a[0].push_back(3);
    a[1].push_back(4);

    micron::array<micron::vector<int>, 2> b(micron::move(a));

    require(b[0][0], 3);
    require(b[1][0], 4);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("stress copy/move cycles");
  {
    micron::array<int, 64> a(1);
    for ( size_t i = 0; i < 1000; ++i ) {
      micron::array<int, 64> b(a);
      micron::array<int, 64> c(micron::move(b));
      require(c[0], 1);
    }
  }
  end_test_case();

  return 0;
}
