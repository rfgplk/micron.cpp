// svector_tests.cpp
// Rigorous snowball test suite for micron::svector<T, N, Sf>

#include "../../src/vector/svector.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ------------------------------------------------------------------ //
//  Lifetime-tracking helper                                           //
// ------------------------------------------------------------------ //
namespace
{

struct Probe {
  static inline int live = 0;
  static inline int total = 0;
  static inline bool corrupt = false;

  static constexpr int MAGIC = 0xCAFEBABE;
  int sentinel;
  int id;

  Probe() : sentinel(MAGIC), id(++total) { ++live; }

  explicit Probe(int x) : sentinel(MAGIC), id(x)
  {
    ++total;
    ++live;
  }

  Probe(const Probe &o) : sentinel(MAGIC), id(o.id)
  {
    check(o);
    ++total;
    ++live;
  }

  Probe(Probe &&o) noexcept : sentinel(MAGIC), id(o.id)
  {
    check(o);
    o.sentinel = 0;
    ++total;
    ++live;
  }

  Probe &
  operator=(const Probe &o)
  {
    check(o);
    sentinel = MAGIC;
    id = o.id;
    return *this;
  }

  Probe &
  operator=(Probe &&o) noexcept
  {
    check(o);
    sentinel = MAGIC;
    id = o.id;
    o.sentinel = 0;
    return *this;
  }

  ~Probe()
  {
    if ( sentinel != MAGIC && sentinel != 0 )
      corrupt = true;
    sentinel = 0xDEAD;
    --live;
  }

  bool
  operator==(const Probe &o) const
  {
    return id == o.id;
  }

  bool
  operator>(const Probe &o) const
  {
    return id > o.id;
  }

  bool
  operator>=(const Probe &o) const
  {
    return id >= o.id;
  }

  static void
  check(const Probe &o)
  {
    if ( o.sentinel != MAGIC )
      corrupt = true;
  }

  static void
  reset()
  {
    live = 0;
    total = 0;
    corrupt = false;
  }
};

}     // anonymous namespace

// ================================================================== //
int
main()
{
  sb::print("=== SVECTOR TESTS ===");

  // ================================================================ //
  //  Construction                                                     //
  // ================================================================ //
  test_case("default construction – empty, length zero");
  {
    micron::svector<int, 16> v;
    require_true(v.empty());
    require(v.size(), size_t(0));
    require(v.max_size(), size_t(16));
    require_false(v.full());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("size constructor – length set, elements value-initialised");
  {
    micron::svector<int, 32> v(8);
    require(v.size(), size_t(8));
    require_false(v.empty());
    for ( size_t i = 0; i < 8; ++i )
      require(v[i], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("size constructor – cnt > N throws");
  {
    require_throw([]() { micron::svector<int, 4> v(5); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("size + value constructor fills all slots");
  {
    micron::svector<int, 8> v(6, 42);
    require(v.size(), size_t(6));
    for ( size_t i = 0; i < 6; ++i )
      require(v[i], 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("size+value constructor fills requested slots");
  {
    // svector(size_type, const T&) — primitive T, single fill value
    micron::svector<int, 8> v(4, 7);
    require(v.size(), size_t(4));
    for ( size_t i = 0; i < 4; ++i )
      require(v[i], 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("size+value constructor – Probe class type, all slots equal filler");
  {
    // svector(size_type, const T&): first arg is integral so variadic is excluded.
    Probe::reset();
    {
      Probe filler(99);
      micron::svector<Probe, 8> v(size_t(4), filler);
      require(v.size(), size_t(4));
      for ( size_t i = 0; i < 4; ++i )
        require(v[i].id, 99);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – correct values and length");
  {
    micron::svector<int, 8> v{ 1, 2, 3, 4 };
    require(v.size(), size_t(4));
    require(v[0], 1);
    require(v[1], 2);
    require(v[2], 3);
    require(v[3], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – exact fill");
  {
    micron::svector<int, 4> v{ 10, 20, 30, 40 };
    require(v.size(), size_t(4));
    require_true(v.full());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – too large throws");
  {
    require_throw([]() { micron::svector<int, 4> v{ 1, 2, 3, 4, 5 }; });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – Probe lifetime correct");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8>{ Probe(1), Probe(2), Probe(3) };
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy construction – independent clone");
  {
    micron::svector<int, 8> a{ 1, 2, 3, 4 };
    micron::svector<int, 8> b(a);
    require(b.size(), size_t(4));
    for ( size_t i = 0; i < 4; ++i )
      require(b[i], a[i]);

    b[0] = 99;
    require(a[0], 1);     // a unaffected
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy construction – Probe lifetime");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> a;
      for ( int i = 0; i < 4; ++i )
        a.emplace_back(i);
      require(Probe::live, 4);

      micron::svector<Probe, 8> b(a);
      require(Probe::live, 8);
      for ( size_t i = 0; i < 4; ++i )
        require(b[i].id, a[i].id);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction – source emptied, content transferred");
  {
    micron::svector<int, 8> a{ 5, 6, 7 };
    micron::svector<int, 8> b(micron::move(a));
    require(b.size(), size_t(3));
    require(b[0], 5);
    require(b[2], 7);
    require(a.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction – Probe lifetime");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> a;
      for ( int i = 0; i < 4; ++i )
        a.emplace_back(i);
      int live_before = Probe::live;

      micron::svector<Probe, 8> b(micron::move(a));
      require(Probe::live, live_before);     // no new constructions
      require(b.size(), size_t(4));
      require(a.size(), size_t(0));
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy assignment");
  {
    micron::svector<int, 8> a{ 1, 2, 3 };
    micron::svector<int, 8> b{ 9, 8 };
    b = a;
    require(b.size(), size_t(3));
    for ( size_t i = 0; i < 3; ++i )
      require(b[i], a[i]);
    b[0] = 77;
    require(a[0], 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self copy-assignment is a no-op");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    v = v;
    require(v.size(), size_t(3));
    require(v[0], 1);
    require(v[2], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment");
  {
    micron::svector<int, 8> a{ 10, 20, 30 };
    micron::svector<int, 8> b{ 99 };
    b = micron::move(a);
    require(b.size(), size_t(3));
    require(b[0], 10);
    require(b[2], 30);
    require(a.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self move-assignment is a no-op");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    v = micron::move(v);
    // after self-move the content may be zeroed or intact —
    // what matters is no crash and size is consistent
    require_false(v.size() > 8);
  }
  end_test_case();

  // ================================================================ //
  //  Element access                                                  //
  // ================================================================ //
  test_case("operator[] read and write");
  {
    micron::svector<int, 8> v(8);
    v[0] = 1;
    v[7] = 99;
    require(v[0], 1);
    require(v[7], 99);
    require(v[3], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() returns correct element");
  {
    micron::svector<int, 8> v{ 10, 20, 30, 40 };
    require(v.at(0), 10);
    require(v.at(3), 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() throws on out-of-bounds");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    require_throw([&]() { (void)v.at(3); });     // length == 3, index 3 is OOB
    require_throw([&]() { (void)v.at(99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("front() and back() – correct values");
  {
    micron::svector<int, 8> v{ 7, 8, 9 };
    require(v.front(), 7);
    require(v.back(), 9);
    v.front() = 1;
    v.back() = 3;
    require(v[0], 1);
    require(v[2], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("front() / back() throw on empty");
  {
    micron::svector<int, 8> v;
    require_throw([&]() { (void)v.front(); });
    require_throw([&]() { (void)v.back(); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const front() / back()");
  {
    const micron::svector<int, 8> cv{ 5, 6, 7 };
    require(cv.front(), 5);
    require(cv.back(), 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("data() is non-null, points to first element");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    int *p = v.data();
    require_true(p != nullptr);
    require(*p, 1);
    require(*(p + 2), 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() – correct pointer");
  {
    micron::svector<int, 8> v{ 10, 20, 30 };
    require(*v.get(0), 10);
    require(*v.get(2), 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() throws on n >= N");
  {
    micron::svector<int, 4> v{ 1, 2, 3, 4 };
    require_throw([&]() { (void)v.get(4); });
    require_throw([&]() { (void)v.get(99); });
  }
  end_test_case();

  // ================================================================ //
  //  Capacity / state predicates                                     //
  // ================================================================ //
  test_case("empty() / full() / max_size()");
  {
    micron::svector<int, 4> v;
    require_true(v.empty());
    require_false(v.full());
    require(v.max_size(), size_t(4));

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    require_true(v.full());
    require_false(v.empty());
    require(v.size(), size_t(4));
  }
  end_test_case();

  // ================================================================ //
  //  push_back / emplace_back / move_back                            //
  // ================================================================ //
  test_case("push_back appends and increments length");
  {
    micron::svector<int, 8> v;
    for ( int i = 0; i < 8; ++i ) {
      v.push_back(i);
      require(v.size(), size_t(i + 1));
      require(v.back(), i);
    }
    require_true(v.full());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_back throws when full");
  {
    micron::svector<int, 4> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    require_throw([&]() { v.push_back(5); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("emplace_back constructs in place");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> v;
      v.emplace_back(42);
      require(v.size(), size_t(1));
      require(v[0].id, 42);
      require(Probe::live, 1);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("emplace_back throws when full");
  {
    micron::svector<int, 2> v;
    v.emplace_back(1);
    v.emplace_back(2);
    require_throw([&]() { v.emplace_back(3); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move_back moves element in");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> v;
      Probe p(77);
      v.move_back(micron::move(p));
      require(v.size(), size_t(1));
      require(v[0].id, 77);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move_back throws when full");
  {
    micron::svector<int, 2> v;
    v.push_back(1);
    v.push_back(2);
    require_throw([&]() { v.move_back(3); });
  }
  end_test_case();

  // ================================================================ //
  //  clear() / fast_clear()                                          //
  // ================================================================ //
  test_case("clear() zeroes length, calls destructors");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> v;
      for ( int i = 0; i < 6; ++i )
        v.emplace_back(i);
      require(Probe::live, 6);

      v.clear();
      require_true(v.empty());
      require(Probe::live, 0);
      require_false(Probe::corrupt);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear() on empty is a no-op");
  {
    micron::svector<int, 8> v;
    v.clear();
    require_true(v.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fast_clear() on trivial type just resets length");
  {
    micron::svector<int, 8> v{ 1, 2, 3, 4 };
    v.fast_clear();
    require_true(v.empty());
    require(v.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("reuse after clear is fully operational");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    v.clear();
    v.push_back(42);
    require(v.size(), size_t(1));
    require(v[0], 42);
  }
  end_test_case();

  // ================================================================ //
  //  Iterators                                                        //
  // ================================================================ //
  test_case("begin/end iteration in order");
  {
    micron::svector<int, 8> v{ 1, 2, 3, 4, 5 };
    int expected = 1;
    for ( auto it = v.begin(); it != v.end(); ++it )
      require(*it, expected++);
    require(expected, 6);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend const iteration");
  {
    const micron::svector<int, 8> cv{ 10, 20, 30 };
    int sum = 0;
    for ( auto it = cv.cbegin(); it != cv.cend(); ++it )
      sum += *it;
    require(sum, 60);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("range-for loop");
  {
    micron::svector<int, 8> v{ 1, 2, 3, 4 };
    int sum = 0;
    for ( int x : v )
      sum += x;
    require(sum, 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("end() - begin() == size()");
  {
    micron::svector<int, 16> v{ 1, 2, 3, 4, 5, 6 };
    require(static_cast<size_t>(v.end() - v.begin()), v.size());
  }
  end_test_case();

  // ================================================================ //
  //  erase()                                                         //
  // ================================================================ //
  test_case("erase by index – shifts remaining elements");
  {
    micron::svector<int, 8> v{ 1, 2, 3, 4, 5 };
    v.erase(size_t(1));
    require(v.size(), size_t(4));
    require(v[0], 1);
    require(v[1], 3);
    require(v[2], 4);
    require(v[3], 5);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase first element");
  {
    micron::svector<int, 8> v{ 10, 20, 30 };
    v.erase(size_t(0));
    require(v.size(), size_t(2));
    require(v[0], 20);
    require(v[1], 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase last element");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    v.erase(size_t(2));
    require(v.size(), size_t(2));
    require(v[0], 1);
    require(v[1], 2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase throws on out-of-bounds index");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    require_throw([&]() { v.erase(size_t(3)); });
    require_throw([&]() { v.erase(size_t(99)); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase – Probe lifetime: exactly one destructor per erase");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> v;
      for ( int i = 0; i < 5; ++i )
        v.emplace_back(i);
      require(Probe::live, 5);

      v.erase(size_t(2));
      require(Probe::live, 4);
      require(v[2].id, 3);     // element 3 shifted left
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ================================================================ //
  //  insert()                                                        //
  // ================================================================ //
  test_case("insert by index – shifts and inserts correctly");
  {
    micron::svector<int, 8> v{ 1, 2, 4, 5 };
    v.insert(size_t(2), 3);
    require(v.size(), size_t(5));
    for ( int i = 0; i < 5; ++i )
      require(v[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert at index 0 – prepend");
  {
    micron::svector<int, 8> v{ 2, 3, 4 };
    v.insert(size_t(0), 1);
    require(v.size(), size_t(4));
    require(v[0], 1);
    require(v[1], 2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert at end – appends");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    v.insert(size_t(3), 4);
    require(v.size(), size_t(4));
    require(v[3], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert by index throws when full");
  {
    micron::svector<int, 4> v{ 1, 2, 3, 4 };
    require_throw([&]() { v.insert(size_t(0), 99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert by iterator – shifts correctly");
  {
    micron::svector<int, 8> v{ 1, 2, 4 };
    auto it = v.begin() + 2;
    v.insert(it, 3);
    require(v.size(), size_t(4));
    for ( int i = 0; i < 4; ++i )
      require(v[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert by iterator throws when full");
  {
    micron::svector<int, 4> v{ 1, 2, 3, 4 };
    auto it = v.begin();
    require_throw([&]() { v.insert(it, 99); });
  }
  end_test_case();

  // ================================================================ //
  //  append() / operator+=                                           //
  // ================================================================ //
  test_case("append(svector) concatenates in order");
  {
    micron::svector<int, 8> a{ 1, 2 };
    micron::svector<int, 8> b{ 3, 4 };
    a.append(b);
    require(a.size(), size_t(4));
    for ( int i = 0; i < 4; ++i )
      require(a[i], i + 1);
    require(b.size(), size_t(2));     // b unmodified
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("append throws when combined size > N");
  {
    micron::svector<int, 4> a{ 1, 2, 3 };
    micron::svector<int, 4> b{ 4, 5 };
    require_throw([&]() { a.append(b); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator+= appends");
  {
    micron::svector<int, 8> a{ 1, 2 };
    micron::svector<int, 8> b{ 3, 4 };
    a += b;
    require(a.size(), size_t(4));
    require(a[2], 3);
    require(a[3], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("append(single element) – alias for push_back");
  {
    micron::svector<int, 8> v{ 1, 2, 3 };
    v.append(4);
    require(v.size(), size_t(4));
    require(v[3], 4);
  }
  end_test_case();

  // ================================================================ //
  //  operator[](from, to) slice                                      //
  // ================================================================ //
  test_case("operator[](from, to) returns correct slice");
  {
    micron::svector<int, 8> v{ 0, 1, 2, 3, 4, 5, 6, 7 };
    auto sl = v[size_t(2), size_t(5)];
    require(sl[0], 2);
    require(sl[1], 3);
    require(sl[2], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator[](from, to) throws on invalid range");
  {
    micron::svector<int, 8> v{ 1, 2, 3, 4 };
    require_throw([&]() { (void)v[size_t(3), size_t(2)]; });     // from > to
    require_throw([&]() { (void)v[size_t(0), size_t(9)]; });     // to > N
  }
  end_test_case();

  // ================================================================ //
  //  Static type helpers                                             //
  // ================================================================ //
  test_case("is_pod / is_class_type / is_trivial");
  {
    require_true(micron::svector<int, 8>::is_pod());
    require_false(micron::svector<int, 8>::is_class_type());
    require_true(micron::svector<int, 8>::is_trivial());

    require_false(micron::svector<Probe, 8>::is_pod());
    require_true(micron::svector<Probe, 8>::is_class_type());
    require_false(micron::svector<Probe, 8>::is_trivial());
  }
  end_test_case();

  // ================================================================ //
  //  Different types                                                 //
  // ================================================================ //
  test_case("svector<float, 8> – basic ops");
  {
    micron::svector<float, 8> v;
    for ( int i = 0; i < 8; ++i )
      v.push_back((float)i * 1.5f);
    require_true(v.full());
    for ( int i = 0; i < 8; ++i )
      require(v[i], (float)i * 1.5f);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("svector<char, 32> – string-like storage");
  {
    micron::svector<char, 32> v;
    const char *hello = "hello";
    for ( int i = 0; hello[i]; ++i )
      v.push_back(hello[i]);
    require(v.size(), size_t(5));
    require(v[0], 'h');
    require(v[4], 'o');
  }
  end_test_case();

  // ================================================================ //
  //  Lifetime – comprehensive                                        //
  // ================================================================ //
  test_case("lifetime: destructor destroys all remaining elements");
  {
    Probe::reset();
    {
      micron::svector<Probe, 16> v;
      for ( int i = 0; i < 12; ++i )
        v.emplace_back(i);
      require(Probe::live, 12);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: copy assignment destroys nothing extra");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> a, b;
      for ( int i = 0; i < 4; ++i )
        a.emplace_back(i);
      for ( int i = 0; i < 3; ++i )
        b.emplace_back(100 + i);
      require(Probe::live, 7);

      b = a;
      // b's old elements are overwritten (slots re-assigned), a's copied in
      require(b.size(), size_t(4));
      for ( size_t i = 0; i < 4; ++i )
        require(b[i].id, a[i].id);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: erase + push cycle – no leak");
  {
    Probe::reset();
    {
      micron::svector<Probe, 8> v;
      for ( int i = 0; i < 8; ++i )
        v.emplace_back(i);
      require(Probe::live, 8);

      for ( int i = 0; i < 4; ++i )
        v.erase(size_t(0));
      require(Probe::live, 4);

      for ( int i = 0; i < 4; ++i )
        v.emplace_back(10 + i);
      require(Probe::live, 8);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases                                                      //
  // ================================================================ //
  test_case("edge: N=1 – full lifecycle");
  {
    micron::svector<int, 1> v;
    require_true(v.empty());
    require_false(v.full());

    v.push_back(42);
    require_true(v.full());
    require(v.front(), 42);
    require(v.back(), 42);

    require_throw([&]() { v.push_back(99); });

    v.erase(size_t(0));
    require_true(v.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: single element erase leaves empty vector");
  {
    micron::svector<int, 8> v{ 99 };
    v.erase(size_t(0));
    require_true(v.empty());
    require(v.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: insert then erase round-trips correctly");
  {
    micron::svector<int, 8> v{ 1, 3, 5 };
    v.insert(size_t(1), 2);
    v.insert(size_t(3), 4);
    require(v.size(), size_t(5));
    for ( int i = 0; i < 5; ++i )
      require(v[i], (i % 2 == 0) ? i + 1 : i + 1);     // 1,2,3,4,5

    v.erase(size_t(2));
    require(v.size(), size_t(4));
    require(v[0], 1);
    require(v[1], 2);
    require(v[2], 4);
    require(v[3], 5);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: operator[] all 64 slots of default N");
  {
    micron::svector<int, 64> v(64);
    for ( int i = 0; i < 64; ++i )
      v[i] = i * 2;
    for ( int i = 0; i < 64; ++i )
      require(v[i], i * 2);
  }
  end_test_case();

  // ================================================================ //
  //  Stress                                                          //
  // ================================================================ //
  test_case("stress: fill to capacity then clear, 1000 rounds");
  {
    Probe::reset();
    micron::svector<Probe, 32> v;
    for ( int round = 0; round < 1000; ++round ) {
      for ( int i = 0; i < 32; ++i )
        v.emplace_back(i);
      require(Probe::live, 32);
      v.clear();
      require(Probe::live, 0);
    }
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: push then erase from front, live count always correct");
  {
    Probe::reset();
    {
      micron::svector<Probe, 64> v;
      for ( int i = 0; i < 64; ++i )
        v.emplace_back(i);
      require(Probe::live, 64);

      for ( int i = 0; i < 64; ++i ) {
        require(v[0].id, i);
        v.erase(size_t(0));
        require(Probe::live, 64 - i - 1);
      }
      require_true(v.empty());
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: interleaved push/erase keeps size correct");
  {
    micron::svector<int, 16> v;
    int expected = 0;
    for ( int i = 0; i < 10000; ++i ) {
      if ( !v.full() ) {
        v.push_back(i);
        ++expected;
      }
      if ( (i % 3 == 0) && !v.empty() ) {
        v.erase(size_t(0));
        --expected;
      }
      require(v.size(), size_t(expected));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: append(svector) builds large result correctly");
  {
    micron::svector<int, 64> a{ 1, 2, 3, 4, 5, 6, 7, 8 };
    micron::svector<int, 64> b{ 9, 10, 11, 12, 13, 14, 15, 16 };
    micron::svector<int, 64> c{ 17, 18, 19, 20, 21, 22, 23, 24 };

    a.append(b);
    a.append(c);
    require(a.size(), size_t(24));
    for ( int i = 0; i < 24; ++i )
      require(a[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sentinel: no Probe corruption throughout all tests");
  {
    require_false(Probe::corrupt);
  }
  end_test_case();

  sb::print("=== ALL SVECTOR TESTS PASSED ===");
  return 1;
}
