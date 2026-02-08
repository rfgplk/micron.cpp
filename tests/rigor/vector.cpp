// vector_tests.cpp
// Rigorous snowball test suite for micron::vector<T>

#include "../../src/vector/vector.hpp"
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

}     // namespace

int
main()
{

  // ------------------------------------------------------------
  test_case("default construction");
  {
    micron::vector<int> v;
    require_true(v.empty());
    require(v.size(), size_t(0));
    // require(v.max_size(), size_t(0));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("size constructor and value initialization");
  {
    micron::vector<int> v(10);
    require(v.size(), size_t(10));
    for ( size_t i = 0; i < v.size(); ++i )
      require(v[i], 0);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("initializer_list constructor");
  {
    micron::vector<int> v{ 1, 2, 3, 4 };
    require(v.size(), size_t(4));
    require(v[0], 1);
    require(v[3], 4);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("copy and move construction");
  {
    micron::vector<int> a{ 1, 2, 3 };
    micron::vector<int> b(a);
    require(b.size(), size_t(3));
    require(b[1], 2);

    micron::vector<int> c(micron::move(a));
    require(c.size(), size_t(3));
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("push_back and pop_back");
  {
    micron::vector<int> v;
    for ( int i = 0; i < 1000; ++i )
      v.push_back(i);
    require(v.size(), size_t(1000));
    for ( int i = 999; i >= 0; --i ) {
      require(v.back(), i);
      v.pop_back();
    }
    require_true(v.empty());
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("reserve and capacity growth");
  {
    micron::vector<int> v;
    v.reserve(128);
    require_greater(v.max_size(), size_t(0));
    size_t cap = v.max_size();
    v.reserve(64);
    require(v.max_size(), cap);
    v.reserve(cap + 10);
    require_greater(v.max_size(), cap);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("at() bounds");
  {
    micron::vector<int> v(4);
    require_throw([&]() { (void)v.at(5); });
    require_throw([&]() { (void)v.at(4); });
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("front/back access");
  {
    micron::vector<int> v{ 7, 8, 9 };
    require(v.front(), 7);
    require(v.back(), 9);
    v.front() = 1;
    v.back() = 2;
    require(v[0], 1);
    require(v[2], 2);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("insert by index");
  {
    micron::vector<int> v{ 1, 2, 4 };
    v.insert(size_t(2), 3);
    require(v.size(), size_t(4));
    for ( int i = 0; i < 4; ++i )
      require(v[i], i + 1);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("insert iterator variants");
  {
    micron::vector<int> v{ 1, 3, 4 };
    auto it = v.begin() + 1;
    v.insert(it, 2);
    require(v.size(), size_t(4));
    for ( int i = 0; i < 4; ++i )
      require(v[i], i + 1);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("erase single and range");
  {
    micron::vector<int> v{ 1, 2, 3, 4, 5 };
    v.erase(size_t(1));
    require(v.size(), size_t(4));
    require(v[1], 3);

    v.erase(size_t(1), size_t(3));
    require(v.size(), size_t(2));
    require(v[0], 1);
    require(v[1], 5);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("find and remove");
  {
    micron::vector<int> v{ 1, 2, 3, 2, 4 };
    require_true(v.find(3) != nullptr);
    v.remove(2);
    require(v.size(), size_t(3));
    require(v[0], 1);
    require(v[1], 3);
    require(v[2], 4);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("resize semantics");
  {
    micron::vector<int> v{ 1, 2 };
    v.resize(5);
    require(v.size(), size_t(5));
    require(v[0], 1);
    require(v[1], 2);
    require(v[4], 0);

    v.resize(8, 7);
    require(v[7], 7);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("append and weld");
  {
    micron::vector<int> a{ 1, 2 };
    micron::vector<int> b{ 3, 4 };
    a.append(b);
    require(a.size(), size_t(4));
    require(a[3], 4);

    micron::vector<int> c{ 5, 6 };
    a.weld(micron::move(c));
    require(a.size(), size_t(6));
    require(a[5], 6);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("sort and insert_sort");
  {
    micron::vector<int> v{ 5, 1, 4, 3, 2 };
    v.sort();
    for ( int i = 0; i < 5; ++i )
      require(v[i], i + 1);

    micron::vector<int> w;
    w.insert_sort(3);
    w.insert_sort(1);
    w.insert_sort(2);
    require(w[0], 1);
    require(w[1], 2);
    require(w[2], 3);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("tracked object lifetime");
  {
    reset_tracked();
    {
      micron::vector<Tracked> v;
      for ( int i = 0; i < 100; ++i )
        v.emplace_back(i);
      require(v.size(), size_t(100));
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ------------------------------------------------------------
  test_case("stress push/erase cycles");
  {
    micron::vector<int> v;
    for ( int r = 0; r < 100; ++r ) {
      for ( int i = 0; i < 1000; ++i )
        v.push_back(i);
      for ( int i = 0; i < 500; ++i )
        v.erase(size_t(0));
      v.clear();
      require_true(v.empty());
    }
  }
  end_test_case();

  return 0;
}
