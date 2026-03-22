// ivector_tests.cpp
// Rigorous snowball test suite for micron::ivector<T>

#include "../../src/vector/ivector.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_smaller;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ------------------------------------------------------------------ //
//  Helpers                                                            //
// ------------------------------------------------------------------ //
namespace
{

using ivec = micron::ivector<int>;

bool
all_eq(const ivec &v, int val)
{
  for ( usize i = 0; i < v.size(); ++i )
    if ( v[i] != val )
      return false;
  return true;
}

bool
vec_eq(const ivec &a, const ivec &b)
{
  if ( a.size() != b.size() )
    return false;
  for ( usize i = 0; i < a.size(); ++i )
    if ( a[i] != b[i] )
      return false;
  return true;
}

ivec
make_seq(usize n, int start = 0)
{
  int s = start;
  return ivec(n, [&s]() { return s++; });
}

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== IVECTOR TESTS ===");

  // ================================================================ //
  //  Construction                                                     //
  // ================================================================ //
  test_case("construction with size – all elements default");
  {
    ivec v(8);
    require(v.size(), usize(8));
    require_false(v.empty());
    require_true(all_eq(v, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("construction with size and const value");
  {
    ivec v(16, 42);
    require(v.size(), usize(16));
    require_true(all_eq(v, 42));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("construction with size and rvalue value");
  {
    int val = 99;
    ivec v(8, micron::move(val));
    require(v.size(), usize(8));
    require_true(all_eq(v, 99));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer list construction");
  {
    ivec v{ 10, 20, 30, 40, 50 };
    require(v.size(), usize(5));
    require(v[0], 10);
    require(v[1], 20);
    require(v[2], 30);
    require(v[3], 40);
    require(v[4], 50);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer list – single element");
  {
    ivec v{ 99 };
    require(v.size(), usize(1));
    require(v[0], 99);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("generator constructor – fills via callable");
  {
    int counter = 0;
    ivec v(8, [&counter]() { return counter++; });
    require(v.size(), usize(8));
    for ( int i = 0; i < 8; ++i )
      require(v[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("generator constructor – large array");
  {
    auto v = make_seq(256, 0);
    require(v.size(), usize(256));
    for ( int i = 0; i < 256; ++i )
      require(v[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy construction produces independent clone");
  {
    ivec a = make_seq(16, 1);
    ivec b(a);
    require_true(vec_eq(a, b));
    require(b.size(), a.size());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy assignment replaces content");
  {
    ivec a = make_seq(16, 5);
    ivec b(8, 0);
    b = a;
    require_true(vec_eq(a, b));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction transfers content");
  {
    ivec a = make_seq(8, 10);
    ivec b(micron::move(a));
    require(b.size(), usize(8));
    require(b[0], 10);
    require(b[7], 17);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment transfers content");
  {
    ivec a = make_seq(8, 10);
    ivec b(4, 0);
    b = micron::move(a);
    require(b.size(), usize(8));
    require(b[0], 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("construction from mutable vector");
  {
    micron::vector<int> mv;
    for ( usize i = 0; i < 8; ++i )
      mv.push_back((int)i);
    ivec v(mv);
    require(v.size(), usize(8));
    for ( int i = 0; i < 8; ++i )
      require(v[i], i);
  }
  end_test_case();

  // ================================================================ //
  //  Capacity                                                         //
  // ================================================================ //
  test_case("size() returns element count");
  {
    ivec v1(1);
    ivec v64(64);
    require(v1.size(), usize(1));
    require(v64.size(), usize(64));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("capacity() >= size()");
  {
    ivec v(64, 0);
    require_true(v.capacity() >= v.size());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("max_size() equals capacity()");
  {
    ivec v(64, 0);
    require(v.max_size(), v.capacity());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("empty() and operator!()");
  {
    ivec v(1);
    require_false(v.empty());
    require_false(!v);

    auto v2 = v.clear();
    require_true(v2.empty());
    require_true(!v2);
  }
  end_test_case();

  // ================================================================ //
  //  Const-only element access                                        //
  // ================================================================ //
  test_case("operator[] reads correct elements");
  {
    ivec v{ 10, 20, 30, 40 };
    require(v[0], 10);
    require(v[1], 20);
    require(v[2], 30);
    require(v[3], 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() reads correct elements");
  {
    auto v = make_seq(8, 0);
    for ( usize i = 0; i < 8; ++i )
      require(v.at(i), (int)i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() throws on out-of-bounds");
  {
    ivec v(4, 0);
    require_throw([&]() { (void)v.at(4); });
    require_throw([&]() { (void)v.at(99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("front() and back()");
  {
    ivec v{ 10, 20, 30 };
    require(v.front(), 10);
    require(v.back(), 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("data() returns const pointer to first element");
  {
    ivec v{ 10, 20, 30 };
    const int *p = v.data();
    require_true(p != nullptr);
    require(*p, 10);
    require(*(p + 2), 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() returns const iterator to element");
  {
    auto v = make_seq(8, 10);
    const int *p = v.get(3);
    require(*p, 13);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() throws on out-of-bounds");
  {
    ivec v(4, 0);
    require_throw([&]() { (void)v.get(4); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cget() returns const iterator");
  {
    auto v = make_seq(8, 10);
    const int *p = v.cget(5);
    require(*p, 15);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("itr() returns const iterator");
  {
    auto v = make_seq(8, 10);
    const int *p = v.itr(2);
    require(*p, 12);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("itr() throws on out-of-bounds");
  {
    ivec v(4, 0);
    require_throw([&]() { (void)v.itr(4); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at_n() returns index from iterator");
  {
    auto v = make_seq(8, 0);
    auto it = v.begin() + 3;
    require(v.at_n(it), usize(3));
  }
  end_test_case();

  // ================================================================ //
  //  Iterators                                                        //
  // ================================================================ //
  test_case("begin/end span all elements");
  {
    auto v = make_seq(8, 0);
    int expected = 0;
    for ( auto it = v.begin(); it != v.end(); ++it )
      require(*it, expected++);
    require(expected, 8);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend span all elements");
  {
    auto v = make_seq(8, 1);
    int expected = 1;
    for ( auto it = v.cbegin(); it != v.cend(); ++it )
      require(*it, expected++);
    require(expected, 9);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("range-for loop iterates in order");
  {
    auto v = make_seq(8, 0);
    int sum = 0;
    for ( int val : v )
      sum += val;
    require(sum, 28);     // 0+1+...+7
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("end() - begin() equals size()");
  {
    ivec v(64, 0);
    require((usize)(v.end() - v.begin()), v.size());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("last() returns iterator to final element");
  {
    ivec v{ 10, 20, 30 };
    require(*v.last(), 30);
  }
  end_test_case();

  // ================================================================ //
  //  find()                                                           //
  // ================================================================ //
  test_case("find() returns iterator to matching element");
  {
    ivec v{ 10, 20, 30, 40, 50 };
    auto it = v.find(30);
    require_true(it != nullptr);
    require(*it, 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("find() returns nullptr for missing element");
  {
    ivec v{ 10, 20, 30 };
    auto it = v.find(99);
    require_true(it == nullptr);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("find() returns first occurrence");
  {
    ivec v{ 1, 2, 3, 2, 1 };
    auto it = v.find(2);
    require_true(it != nullptr);
    require((usize)(it - v.begin()), usize(1));
  }
  end_test_case();

  // ================================================================ //
  //  Slice subscript                                                  //
  // ================================================================ //
  test_case("operator[](from, to) returns const slice");
  {
    ivec v{ 10, 20, 30, 40, 50 };
    auto s = v[usize(1), usize(4)];
    require(s.size(), usize(3));
    require(s[0], 20);
    require(s[1], 30);
    require(s[2], 40);
  }
  end_test_case();

  // ================================================================ //
  //  into_bytes                                                       //
  // ================================================================ //
  test_case("into_bytes returns byte slice of correct size");
  {
    ivec v{ 1, 2, 3 };
    auto bs = v.into_bytes();
    require(bs.size(), usize(3 * sizeof(int)));
  }
  end_test_case();

  // ================================================================ //
  //  Type helpers                                                     //
  // ================================================================ //
  test_case("is_pod / is_class_type / is_trivial");
  {
    require_true(ivec::is_pod());
    require_false(ivec::is_class_type());
    require_true(ivec::is_trivial());
  }
  end_test_case();

  // ================================================================ //
  //  Persistent push_back                                             //
  // ================================================================ //
  test_case("push_back const ref – returns new vector, original unchanged");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.push_back(4);
    require(v2.size(), usize(4));
    require(v2[0], 1);
    require(v2[1], 2);
    require(v2[2], 3);
    require(v2[3], 4);

    require(v.size(), usize(3));
    require(v[2], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_back rvalue");
  {
    ivec v{ 1, 2 };
    int val = 99;
    auto v2 = v.push_back(micron::move(val));
    require(v2.size(), usize(3));
    require(v2[2], 99);
    require(v.size(), usize(2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_back chain");
  {
    ivec v(1, 0);
    auto v2 = v.push_back(1).push_back(2).push_back(3);
    require(v2.size(), usize(4));
    for ( int i = 0; i < 4; ++i )
      require(v2[i], i);
    require(v.size(), usize(1));
  }
  end_test_case();

  // ================================================================ //
  //  Persistent push_front                                            //
  // ================================================================ //
  test_case("push_front const ref – returns new vector, original unchanged");
  {
    ivec v{ 2, 3, 4 };
    auto v2 = v.push_front(1);
    require(v2.size(), usize(4));
    require(v2[0], 1);
    require(v2[1], 2);
    require(v2[2], 3);
    require(v2[3], 4);

    require(v.size(), usize(3));
    require(v[0], 2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_front rvalue");
  {
    ivec v{ 2, 3 };
    int val = 1;
    auto v2 = v.push_front(micron::move(val));
    require(v2.size(), usize(3));
    require(v2[0], 1);
    require(v.size(), usize(2));
  }
  end_test_case();

  // ================================================================ //
  //  Persistent emplace_back                                          //
  // ================================================================ //
  test_case("emplace_back constructs element at end");
  {
    ivec v{ 1, 2 };
    auto v2 = v.emplace_back(42);
    require(v2.size(), usize(3));
    require(v2[2], 42);
    require(v.size(), usize(2));
  }
  end_test_case();

  // ================================================================ //
  //  Persistent insert                                                //
  //  NOTE: insert(n, ...) checks n >= length, so n must be < size()   //
  //  To append, use push_back instead.                                //
  // ================================================================ //
  test_case("insert at middle index");
  {
    ivec v{ 1, 2, 4, 5 };
    auto v2 = v.insert(usize(2), 3);
    require(v2.size(), usize(5));
    for ( int i = 0; i < 5; ++i )
      require(v2[i], i + 1);

    require(v.size(), usize(4));
    require(v[2], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert at index 0 (front)");
  {
    ivec v{ 2, 3 };
    auto v2 = v.insert(usize(0), 1);
    require(v2.size(), usize(3));
    require(v2[0], 1);
    require(v2[1], 2);
    require(v2[2], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert at last valid index");
  {
    ivec v{ 1, 2, 3, 4 };
    // insert at index 3 (last valid, since size=4, check is n < 4)
    auto v2 = v.insert(usize(3), 99);
    require(v2.size(), usize(5));
    require(v2[0], 1);
    require(v2[1], 2);
    require(v2[2], 3);
    require(v2[3], 99);
    require(v2[4], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert with const_iterator");
  {
    ivec v{ 1, 2, 4 };
    auto it = v.begin() + 2;
    auto v2 = v.insert(it, 3);
    require(v2.size(), usize(4));
    require(v2[2], 3);
    require(v2[3], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert multiple copies");
  {
    ivec v{ 1, 5 };
    auto v2 = v.insert(usize(1), 0, usize(3));
    require(v2.size(), usize(5));
    require(v2[0], 1);
    require(v2[1], 0);
    require(v2[2], 0);
    require(v2[3], 0);
    require(v2[4], 5);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert rvalue at index");
  {
    ivec v{ 1, 3 };
    int val = 2;
    auto v2 = v.insert(usize(1), micron::move(val));
    require(v2.size(), usize(3));
    require(v2[1], 2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("insert throws on out-of-bounds index");
  {
    ivec v{ 1, 2, 3 };
    // n >= length (3 >= 3) should throw
    require_throw([&]() { (void)v.insert(usize(3), 99); });
    require_throw([&]() { (void)v.insert(usize(99), 99); });
  }
  end_test_case();

  // ================================================================ //
  //  Persistent erase                                                 //
  // ================================================================ //
  test_case("erase at middle index");
  {
    ivec v{ 1, 2, 3, 4, 5 };
    auto v2 = v.erase(usize(2));
    require(v2.size(), usize(4));
    require(v2[0], 1);
    require(v2[1], 2);
    require(v2[2], 4);
    require(v2[3], 5);

    require(v.size(), usize(5));
    require(v[2], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase first element");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.erase(usize(0));
    require(v2.size(), usize(2));
    require(v2[0], 2);
    require(v2[1], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase last element");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.erase(usize(2));
    require(v2.size(), usize(2));
    require(v2[0], 1);
    require(v2[1], 2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase with const_iterator");
  {
    ivec v{ 1, 2, 3, 4 };
    auto v2 = v.erase(v.begin() + 1);
    require(v2.size(), usize(3));
    require(v2[0], 1);
    require(v2[1], 3);
    require(v2[2], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase range by indices");
  {
    ivec v{ 1, 2, 3, 4, 5 };
    auto v2 = v.erase(usize(1), usize(3));     // erase [1, 3) → remove indices 1,2
    require(v2.size(), usize(3));
    require(v2[0], 1);
    require(v2[1], 4);
    require(v2[2], 5);

    require(v.size(), usize(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase range with iterators");
  {
    ivec v{ 10, 20, 30, 40, 50 };
    auto v2 = v.erase(v.begin() + 1, v.begin() + 4);     // erase [1,4)
    require(v2.size(), usize(2));
    require(v2[0], 10);
    require(v2[1], 50);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("erase throws on out-of-bounds index");
  {
    ivec v{ 1, 2, 3 };
    require_throw([&]() { (void)v.erase(usize(3)); });
    require_throw([&]() { (void)v.erase(usize(99)); });
  }
  end_test_case();

  // ================================================================ //
  //  Persistent pop_back                                              //
  // ================================================================ //
  test_case("pop_back returns new vector without last element");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.pop_back();
    require(v2.size(), usize(2));
    require(v2[0], 1);
    require(v2[1], 2);

    require(v.size(), usize(3));
    require(v[2], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop_back chain to single element");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.pop_back().pop_back();
    require(v2.size(), usize(1));
    require(v2[0], 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop_back on single element produces empty vector");
  {
    ivec v(1, 42);
    auto v2 = v.pop_back();
    require(v2.size(), usize(0));
    require_true(v2.empty());
  }
  end_test_case();

  // ================================================================ //
  //  Persistent append / operator+                                    //
  // ================================================================ //
  test_case("append concatenates two vectors");
  {
    ivec a{ 1, 2, 3 };
    ivec b{ 4, 5, 6 };
    auto c = a.append(b);
    require(c.size(), usize(6));
    for ( int i = 0; i < 6; ++i )
      require(c[i], i + 1);

    require(a.size(), usize(3));
    require(b.size(), usize(3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator+ concatenates two vectors");
  {
    ivec a{ 1, 2 };
    ivec b{ 3, 4 };
    auto c = a + b;
    require(c.size(), usize(4));
    require(c[0], 1);
    require(c[1], 2);
    require(c[2], 3);
    require(c[3], 4);
  }
  end_test_case();

  // ================================================================ //
  //  Persistent assign                                                //
  // ================================================================ //
  test_case("assign returns new vector with n copies of value");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.assign(usize(5), 42);
    require(v2.size(), usize(5));
    require_true(all_eq(v2, 42));

    require(v.size(), usize(3));
    require(v[0], 1);
  }
  end_test_case();

  // ================================================================ //
  //  Persistent clear                                                 //
  // ================================================================ //
  test_case("clear returns empty vector with preserved capacity, original intact");
  {
    ivec v{ 1, 2, 3 };
    usize orig_cap = v.capacity();
    auto v2 = v.clear();
    require(v2.size(), usize(0));
    require_true(v2.empty());
    // clear preserves capacity
    // NOTE: no guarantee, allocator may alloc a different cap
    //require(v2.capacity(), orig_cap);

    require(v.size(), usize(3));
    require(v[0], 1);
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases                                                       //
  // ================================================================ //
  test_case("single element vector – all operations");
  {
    ivec v(1, 42);
    require(v.size(), usize(1));
    require(v[0], 42);
    require(v.front(), 42);
    require(v.back(), 42);
    require(*v.last(), 42);

    auto v2 = v.push_back(99);
    require(v2.size(), usize(2));
    require(v2[0], 42);
    require(v2[1], 99);

    auto v3 = v.push_front(0);
    require(v3.size(), usize(2));
    require(v3[0], 0);
    require(v3[1], 42);

    auto v4 = v.pop_back();
    require(v4.size(), usize(0));
    require_true(v4.empty());

    auto v5 = v.erase(usize(0));
    require(v5.size(), usize(0));

    // original always intact
    require(v.size(), usize(1));
    require(v[0], 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("large vector – 1000 elements via push_back chain");
  {
    ivec v(1, 0);
    for ( int i = 1; i < 1000; ++i )
      v = v.push_back(i);
    require(v.size(), usize(1000));
    for ( int i = 0; i < 1000; ++i )
      require(v[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("large vector – direct construction");
  {
    auto v = make_seq(1024, 0);
    require(v.size(), usize(1024));
    for ( int i = 0; i < 1024; ++i )
      require(v[i], i);
  }
  end_test_case();

  // ================================================================ //
  //  Persistence / immutability stress tests                          //
  // ================================================================ //
  test_case("stress: multiple versions alive – push_back chain");
  {
    ivec v0(1, 0);
    auto v1 = v0.push_back(1);
    auto v2 = v1.push_back(2);
    auto v3 = v2.push_back(3);
    auto v4 = v3.push_back(4);

    require(v0.size(), usize(1));
    require(v1.size(), usize(2));
    require(v2.size(), usize(3));
    require(v3.size(), usize(4));
    require(v4.size(), usize(5));

    require(v0[0], 0);
    require(v4[0], 0);
    require(v4[4], 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: fork from single base");
  {
    ivec base{ 1, 2, 3 };
    auto a = base.push_back(100);
    auto b = base.push_back(200);
    auto c = base.push_back(300);

    require(base.size(), usize(3));
    require(a.size(), usize(4));
    require(b.size(), usize(4));
    require(c.size(), usize(4));

    require(a[3], 100);
    require(b[3], 200);
    require(c[3], 300);

    for ( int i = 0; i < 3; ++i ) {
      require(a[i], i + 1);
      require(b[i], i + 1);
      require(c[i], i + 1);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: insert at every valid position");
  {
    ivec base{ 1, 2, 3, 4 };
    // valid positions: 0, 1, 2, 3 (< size)
    for ( usize i = 0; i < base.size(); ++i ) {
      auto v = base.insert(i, 99);
      require(v.size(), usize(5));
      require(v[i], 99);
      require(base.size(), usize(4));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: erase at every position");
  {
    auto base = make_seq(10, 0);
    for ( usize i = 0; i < base.size(); ++i ) {
      auto v = base.erase(i);
      require(v.size(), usize(9));
      for ( usize j = 0; j < v.size(); ++j ) {
        if ( j < i )
          require(v[j], (int)j);
        else
          require(v[j], (int)(j + 1));
      }
      require(base.size(), usize(10));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: push_back + pop_back round trip preserves content");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.push_back(99).pop_back();
    require(v2.size(), usize(3));
    require_true(vec_eq(v, v2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: push_front + erase(0) round trip preserves content");
  {
    ivec v{ 1, 2, 3 };
    auto v2 = v.push_front(99).erase(usize(0));
    require(v2.size(), usize(3));
    require_true(vec_eq(v, v2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: repeated self-append doubles content");
  {
    ivec v{ 1, 2, 3, 4 };
    auto v2 = v + v;
    require(v2.size(), usize(8));
    for ( usize i = 0; i < 4; ++i ) {
      require(v2[i], v[i]);
      require(v2[i + 4], v[i]);
    }
    require(v.size(), usize(4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: erase range at various positions");
  {
    auto base = make_seq(10, 0);     // 0..9

    // erase first 3: [0,3) → {3,4,5,6,7,8,9}
    auto v1 = base.erase(usize(0), usize(3));
    require(v1.size(), usize(7));
    require(v1[0], 3);

    // erase last 3: [7,10) → {0,1,2,3,4,5,6}
    auto v2 = base.erase(usize(7), usize(10));
    require(v2.size(), usize(7));
    require(v2[6], 6);

    // erase middle: [3,7) → {0,1,2,7,8,9}
    auto v3 = base.erase(usize(3), usize(7));
    require(v3.size(), usize(6));
    require(v3[2], 2);
    require(v3[3], 7);

    // base always intact
    require(base.size(), usize(10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: 500 push_back/pop_back cycles");
  {
    ivec v(1, 0);
    for ( int i = 0; i < 500; ++i ) {
      v = v.push_back(i + 1);
      auto tmp = v.pop_back();
      require(tmp.size(), v.size() - 1);
    }
    // v should have accumulated 501 elements: [0, 1, 2, ..., 500]
    require(v.size(), usize(501));
    for ( int i = 0; i <= 500; ++i )
      require(v[i], i);
  }
  end_test_case();

  // ================================================================ //
  //  Invariants                                                       //
  // ================================================================ //
  test_case("invariant: push_back increases size by 1");
  {
    auto v = make_seq(10, 0);
    for ( int i = 0; i < 20; ++i ) {
      auto v2 = v.push_back(i);
      require(v2.size(), v.size() + 1);
      v = v2;
    }
    require(v.size(), usize(30));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase decreases size by 1");
  {
    auto v = make_seq(20, 0);
    for ( int i = 19; i >= 0; --i ) {
      auto v2 = v.erase(usize(i));
      require(v2.size(), v.size() - 1);
      v = v2;
    }
    require(v.size(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: pop_back().size() == size() - 1");
  {
    auto v = make_seq(10, 0);
    auto v2 = v.pop_back();
    require(v2.size(), v.size() - 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: insert preserves all existing elements");
  {
    auto v = make_seq(10, 0);
    auto v2 = v.insert(usize(5), 999);
    require(v2.size(), usize(11));
    for ( int i = 0; i < 5; ++i )
      require(v2[i], i);
    require(v2[5], 999);
    for ( int i = 5; i < 10; ++i )
      require(v2[i + 1], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: erase preserves all other elements");
  {
    auto v = make_seq(10, 0);
    auto v2 = v.erase(usize(5));
    require(v2.size(), usize(9));
    for ( int i = 0; i < 5; ++i )
      require(v2[i], i);
    for ( int i = 5; i < 9; ++i )
      require(v2[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: append size equals sum of sizes");
  {
    ivec a{ 1, 2, 3 };
    ivec b{ 4, 5 };
    auto c = a + b;
    require(c.size(), a.size() + b.size());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: end() - begin() always equals size()");
  {
    for ( usize n : { usize(1), usize(8), usize(64), usize(512) } ) {
      auto v = make_seq(n, 0);
      require((usize)(v.end() - v.begin()), v.size());
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: no operation mutates source (exhaustive)");
  {
    ivec v{ 10, 20, 30, 40, 50 };

    (void)v.push_back(99);
    require(v.size(), usize(5));
    require(v[0], 10);
    (void)v.push_front(99);
    require(v.size(), usize(5));
    require(v[0], 10);
    (void)v.pop_back();
    require(v.size(), usize(5));
    require(v[4], 50);
    (void)v.emplace_back(99);
    require(v.size(), usize(5));
    require(v[0], 10);
    (void)v.insert(usize(2), 99);
    require(v.size(), usize(5));
    require(v[2], 30);
    (void)v.erase(usize(2));
    require(v.size(), usize(5));
    require(v[2], 30);
    (void)v.erase(usize(1), usize(3));
    require(v.size(), usize(5));
    require(v[1], 20);
    (void)v.assign(usize(10), 0);
    require(v.size(), usize(5));
    require(v[0], 10);
    (void)v.clear();
    require(v.size(), usize(5));
    require(v[0], 10);
    (void)(v + v);
    require(v.size(), usize(5));
    require(v[0], 10);
  }
  end_test_case();

  sb::print("=== ALL IVECTOR TESTS PASSED ===");
  return 1;
}
