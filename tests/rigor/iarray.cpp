// iarray_tests.cpp
// Rigorous snowball test suite for micron::iarray<T, N>

#include "../../src/array/iarray.hpp"
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

template <typename T, usize N>
bool
all_eq(const micron::iarray<T, N> &a, T val)
{
  for ( usize i = 0; i < N; ++i )
    if ( a[i] != val )
      return false;
  return true;
}

template <typename T, usize N>
bool
arr_eq(const micron::iarray<T, N> &a, const micron::iarray<T, N> &b)
{
  for ( usize i = 0; i < N; ++i )
    if ( a[i] != b[i] )
      return false;
  return true;
}

// make a sequential iarray via generator constructor
template <typename T, usize N>
micron::iarray<T, N>
make_seq(T start = T{})
{
  int offset = static_cast<int>(start);
  return micron::iarray<T, N>([cnt = 0, offset]() mutable -> T { return static_cast<T>(offset + cnt++); });
}

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== IARRAY TESTS ===");

  // ================================================================ //
  //  Construction                                                     //
  // ================================================================ //
  test_case("default construction – all elements zero/default");
  {
    micron::iarray<int, 8> a8;
    micron::iarray<int, 64> a64;
    micron::iarray<int, 128> a128;
    micron::iarray<float, 16> af;
    micron::iarray<char, 32> ac;

    require_true(all_eq(a8, 0));
    require_true(all_eq(a64, 0));
    require_true(all_eq(a128, 0));
    require_true(all_eq(af, 0.0f));
    require_true(all_eq(ac, '\0'));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar constructor fills all elements");
  {
    micron::iarray<int, 16> a(42);
    micron::iarray<float, 16> b(3.14f);
    micron::iarray<char, 8> c('X');
    micron::iarray<int, 1> one(99);
    micron::iarray<int, 64> big(-1);

    require_true(all_eq(a, 42));
    require_true(all_eq(b, 3.14f));
    require_true(all_eq(c, 'X'));
    require_true(all_eq(one, 99));
    require_true(all_eq(big, -1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – partial fill, rest default");
  {
    micron::iarray<int, 8> a{ 1, 2, 3 };
    require(a[0], 1);
    require(a[1], 2);
    require(a[2], 3);
    require(a[3], 0);
    require(a[7], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – exact fill");
  {
    micron::iarray<int, 4> a{ 10, 20, 30, 40 };
    require(a[0], 10);
    require(a[1], 20);
    require(a[2], 30);
    require(a[3], 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – too large throws");
  {
    require_throw([]() { micron::iarray<int, 4> a{ 1, 2, 3, 4, 5 }; });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – single element");
  {
    micron::iarray<int, 8> a{ 99 };
    require(a[0], 99);
    for ( usize i = 1; i < 8; ++i )
      require(a[i], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("generator constructor – fills via callable");
  {
    int counter = 0;
    micron::iarray<int, 8> a([&counter]() { return counter++; });
    for ( int i = 0; i < 8; ++i )
      require(a[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("generator constructor – large array");
  {
    auto a = make_seq<int, 256>(0);
    for ( int i = 0; i < 256; ++i )
      require(a[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy construction produces independent clone");
  {
    auto a = make_seq<int, 16>(1);
    micron::iarray<int, 16> b(a);
    require_true(arr_eq(a, b));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction transfers content");
  {
    auto a = make_seq<int, 8>(10);
    micron::iarray<int, 8> b(micron::move(a));
    require(b[0], 10);
    require(b[7], 17);
  }
  end_test_case();

  // ================================================================ //
  //  Immutability of access                                           //
  // ================================================================ //
  test_case("operator[] returns const reference");
  {
    micron::iarray<int, 4> a{ 1, 2, 3, 4 };
    const int &ref = a[2];
    require(ref, 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() returns const reference");
  {
    micron::iarray<int, 4> a{ 10, 20, 30, 40 };
    const int &ref = a.at(1);
    require(ref, 20);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() throws on out-of-bounds");
  {
    micron::iarray<int, 4> a;
    require_throw([&]() { (void)a.at(4); });
    require_throw([&]() { (void)a.at(99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("data() returns const pointer");
  {
    micron::iarray<int, 8> a(42);
    const int *p = a.data();
    require_true(p != nullptr);
    require(*p, 42);
    require(*(p + 7), 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() returns const iterator, throws on OOB");
  {
    auto a = make_seq<int, 8>(10);
    const int *p = a.get(3);
    require(*p, 13);

    require_throw([&]() { (void)a.get(8); });
    require_throw([&]() { (void)a.get(99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cget() throws on out-of-bounds");
  {
    micron::iarray<int, 4> a;
    require_throw([&]() { (void)a.cget(4); });
  }
  end_test_case();

  // ================================================================ //
  //  Capacity                                                         //
  // ================================================================ //
  test_case("size() and max_size() equal N");
  {
    micron::iarray<int, 1> a1;
    micron::iarray<int, 64> a64;
    micron::iarray<int, 1024> a1024;

    require(a1.size(), usize(1));
    require(a1.max_size(), usize(1));
    require(a64.size(), usize(64));
    require(a1024.size(), usize(1024));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("static constexpr length equals N");
  {
    require(micron::iarray<int, 8>::length, usize(8));
    require(micron::iarray<int, 64>::length, usize(64));
    require(micron::iarray<int, 1>::length, usize(1));
  }
  end_test_case();

  // ================================================================ //
  //  Iterators                                                        //
  // ================================================================ //
  test_case("begin/end span all N elements");
  {
    auto a = make_seq<int, 8>(0);
    int expected = 0;
    for ( auto it = a.begin(); it != a.end(); ++it )
      require(*it, expected++);
    require(expected, 8);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend span all N elements");
  {
    auto a = make_seq<int, 8>(1);
    int expected = 1;
    for ( auto it = a.cbegin(); it != a.cend(); ++it )
      require(*it, expected++);
    require(expected, 9);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("range-for loop iterates in order");
  {
    auto a = make_seq<int, 8>(0);
    int sum = 0;
    for ( int v : a )
      sum += v;
    require(sum, 28);     // 0+1+...+7
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("begin() to end() distance equals N");
  {
    micron::iarray<int, 64> a;
    require((int)(a.end() - a.begin()), 64);
  }
  end_test_case();

  // ================================================================ //
  //  Assignment — replaces entire object                              //
  // ================================================================ //
  test_case("copy assignment replaces content");
  {
    auto a = make_seq<int, 16>(5);
    micron::iarray<int, 16> b;
    b = a;
    require_true(arr_eq(a, b));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment replaces content");
  {
    auto a = make_seq<int, 8>(10);
    micron::iarray<int, 8> b;
    b = micron::move(a);
    require(b[0], 10);
    require(b[7], 17);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar assignment fills all elements");
  {
    micron::iarray<int, 16> a;
    a = 7;
    require_true(all_eq(a, 7));
    a = 0;
    require_true(all_eq(a, 0));
    a = -1;
    require_true(all_eq(a, -1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar assignment – float type");
  {
    micron::iarray<float, 8> a;
    a = 3.14f;
    require_true(all_eq(a, 3.14f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar assignment – char type");
  {
    micron::iarray<char, 8> a;
    a = 'Z';
    require_true(a.all('Z'));
  }
  end_test_case();

  // ================================================================ //
  //  Binary arithmetic — value semantics                              //
  // ================================================================ //
  test_case("operator+ returns new array, operands unchanged");
  {
    micron::iarray<int, 8> a(3);
    micron::iarray<int, 8> b(4);
    auto c = a + b;
    require_true(all_eq(c, 7));
    require_true(all_eq(a, 3));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator- returns new array, operands unchanged");
  {
    micron::iarray<int, 8> a(10);
    micron::iarray<int, 8> b(3);
    auto c = a - b;
    require_true(all_eq(c, 7));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator* returns new array, operands unchanged");
  {
    micron::iarray<int, 8> a(3);
    micron::iarray<int, 8> b(4);
    auto c = a * b;
    require_true(all_eq(c, 12));
    require_true(all_eq(a, 3));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator/ returns new array, operands unchanged");
  {
    micron::iarray<int, 8> a(12);
    micron::iarray<int, 8> b(4);
    auto c = a / b;
    require_true(all_eq(c, 3));
    require_true(all_eq(a, 12));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator% returns new array, operands unchanged");
  {
    micron::iarray<int, 8> a(10);
    micron::iarray<int, 8> b(3);
    auto c = a % b;
    require_true(all_eq(c, 1));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("binary ops with sequential arrays");
  {
    auto a = make_seq<int, 8>(1);
    auto b = make_seq<int, 8>(1);
    auto sum = a + b;
    for ( int i = 0; i < 8; ++i )
      require(sum[i], 2 * (i + 1));
    for ( int i = 0; i < 8; ++i )
      require(a[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("chained binary ops produce correct results");
  {
    micron::iarray<int, 4> a(10);
    micron::iarray<int, 4> b(3);
    micron::iarray<int, 4> c(2);
    auto r = (a + b) * c;     // (10+3)*2 = 26
    require_true(all_eq(r, 26));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
    require_true(all_eq(c, 2));
  }
  end_test_case();

  // ================================================================ //
  //  Compound assignment — scalar (returns new copy)                  //
  // ================================================================ //
  test_case("operator+= scalar returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(10);
    auto b = (a += 5);
    require_true(all_eq(b, 15));
    require_true(all_eq(a, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator-= scalar returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(10);
    auto b = (a -= 3);
    require_true(all_eq(b, 7));
    require_true(all_eq(a, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator*= scalar returns new array, original unchanged");
  {
    auto a = make_seq<int, 8>(1);
    auto b = (a *= 3);
    for ( int i = 0; i < 8; ++i )
      require(b[i], (i + 1) * 3);
    for ( int i = 0; i < 8; ++i )
      require(a[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator/= scalar returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(100);
    auto b = (a /= 4);
    require_true(all_eq(b, 25));
    require_true(all_eq(a, 100));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator%= scalar returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(10);
    auto b = (a %= 3);
    require_true(all_eq(b, 1));
    require_true(all_eq(a, 10));
  }
  end_test_case();

  // ================================================================ //
  //  Compound assignment — array (returns new copy)                   //
  // ================================================================ //
  test_case("operator+= array returns new array, both unchanged");
  {
    micron::iarray<int, 8> a(3);
    micron::iarray<int, 8> b(4);
    auto c = (a += b);
    require_true(all_eq(c, 7));
    require_true(all_eq(a, 3));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator-= array returns new array, both unchanged");
  {
    micron::iarray<int, 8> a(10);
    micron::iarray<int, 8> b(3);
    auto c = (a -= b);
    require_true(all_eq(c, 7));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator*= array returns new array, both unchanged");
  {
    micron::iarray<int, 8> a(3);
    micron::iarray<int, 8> b(4);
    auto c = (a *= b);
    require_true(all_eq(c, 12));
    require_true(all_eq(a, 3));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator/= array returns new array, both unchanged");
  {
    micron::iarray<int, 8> a(12);
    micron::iarray<int, 8> b(4);
    auto c = (a /= b);
    require_true(all_eq(c, 3));
    require_true(all_eq(a, 12));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator%= array returns new array, both unchanged");
  {
    micron::iarray<int, 8> a(10);
    micron::iarray<int, 8> b(3);
    auto c = (a %= b);
    require_true(all_eq(c, 1));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
  }
  end_test_case();

  // ================================================================ //
  //  Named arithmetic — returns new copy                              //
  // ================================================================ //
  test_case("add(n) returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(10);
    auto b = a.add(5);
    require_true(all_eq(b, 15));
    require_true(all_eq(a, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sub(n) returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(10);
    auto b = a.sub(3);
    require_true(all_eq(b, 7));
    require_true(all_eq(a, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul(n) returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(2);
    auto b = a.mul(5);
    require_true(all_eq(b, 10));
    require_true(all_eq(a, 2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("div(n) returns new array, original unchanged");
  {
    micron::iarray<int, 8> a(100);
    auto b = a.div(4);
    require_true(all_eq(b, 25));
    require_true(all_eq(a, 100));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("named arithmetic chaining");
  {
    micron::iarray<int, 4> a(6);
    // (6*3 + 2) / 4 = 20 / 4 = 5
    auto b = a.mul(3).add(2).div(4);
    require_true(all_eq(b, 5));
    require_true(all_eq(a, 6));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul/div named are inverses (exact integer)");
  {
    auto a = make_seq<int, 16>(1);
    auto b = a.mul(7).div(7);
    require_true(arr_eq(a, b));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("add/sub named are inverses");
  {
    auto a = make_seq<int, 16>(0);
    auto b = a.add(100).sub(100);
    require_true(arr_eq(a, b));
  }
  end_test_case();

  // ================================================================ //
  //  Reductions — return T                                            //
  // ================================================================ //
  test_case("sum() returns correct sum");
  {
    auto a = make_seq<int, 8>(1);     // 1..8
    require(a.sum(), 36);

    micron::iarray<int, 8> z;
    require(z.sum(), 0);

    micron::iarray<int, 4> ones(1);
    require(ones.sum(), 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul_reduce() returns correct product");
  {
    micron::iarray<int, 4> a{ 1, 2, 3, 4 };
    require(a.mul_reduce(), 24);

    micron::iarray<int, 4> ones(1);
    require(ones.mul_reduce(), 1);

    micron::iarray<int, 4> z;
    require(z.mul_reduce(), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sum() – large sequential array");
  {
    auto a = make_seq<int, 1024>(0);
    require(a.sum(), 523776);     // 0+1+...+1023
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sum() – uniform fill");
  {
    micron::iarray<int, 100> a(1);
    require(a.sum(), 100);
  }
  end_test_case();

  // ================================================================ //
  //  Queries — all() / any()                                          //
  // ================================================================ //
  test_case("all() – true only when every element equals value");
  {
    micron::iarray<int, 8> a(5);
    require_true(a.all(5));
    require_false(a.all(4));
    require_false(a.all(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("all() – false with one outlier");
  {
    micron::iarray<int, 8> a{ 5, 5, 5, 99, 5, 5, 5, 5 };
    require_false(a.all(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("any() – true when at least one element matches");
  {
    micron::iarray<int, 8> a;
    require_false(a.any(1));

    micron::iarray<int, 8> b{ 0, 0, 0, 0, 1, 0, 0, 0 };
    require_true(b.any(1));
    require_false(b.any(2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("all() and any() on single-element array");
  {
    micron::iarray<int, 1> a(7);
    require_true(a.all(7));
    require_false(a.all(8));
    require_true(a.any(7));
    require_false(a.any(8));
  }
  end_test_case();

  // ================================================================ //
  //  Transforms — returns new copy                                    //
  // ================================================================ //
  test_case("clear() returns default-constructed array");
  {
    micron::iarray<int, 8> a(42);
    auto b = a.clear();
    require_true(all_eq(b, 0));
    require_true(all_eq(a, 42));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fill() returns array filled with value");
  {
    micron::iarray<int, 8> a(0);
    auto b = a.fill(99);
    require_true(all_eq(b, 99));
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sqrt() returns new array with sqrt values");
  {
    micron::iarray<int, 4> a{ 4, 9, 16, 25 };
    auto b = a.sqrt();
    require(b[0], 2);
    require(b[1], 3);
    require(b[2], 4);
    require(b[3], 5);
    require(a[0], 4);
    require(a[1], 9);
  }
  end_test_case();

  // ================================================================ //
  //  Static type helpers                                              //
  // ================================================================ //
  test_case("is_pod / is_class / is_trivial");
  {
    require_true(micron::iarray<int, 8>::is_pod());
    require_false(micron::iarray<int, 8>::is_class());
    require_true(micron::iarray<int, 8>::is_trivial());
    require_true(micron::iarray<float, 8>::is_pod());
    require_true(micron::iarray<char, 8>::is_trivial());
  }
  end_test_case();

  // ================================================================ //
  //  operator& — const byte pointer                                   //
  // ================================================================ //
  test_case("operator& returns const byte* to backing storage");
  {
    micron::iarray<int, 8> a(42);
    const byte *p = &a;
    require_true(p != nullptr);
    require_true(p == reinterpret_cast<const byte *>(a.data()));
  }
  end_test_case();

  // ================================================================ //
  //  Alignment                                                        //
  // ================================================================ //
  test_case("data() respects alignof(T) alignment");
  {
    micron::iarray<int, 64> a;
    require_true((reinterpret_cast<usize>(a.data()) % alignof(int)) == 0);
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases – N = 1                                               //
  // ================================================================ //
  test_case("N=1 – construction, access, transforms");
  {
    micron::iarray<int, 1> a(99);
    require(a[0], 99);
    require(a.at(0), 99);
    require(a.size(), usize(1));

    auto b = a.fill(7);
    require(b[0], 7);
    require(b.sum(), 7);
    require(b.mul_reduce(), 7);
    require_true(b.all(7));
    require_false(b.any(8));

    auto c = b.mul(3);
    require(c[0], 21);
    auto d = c.add(9);
    require(d[0], 30);
    auto e = d.sub(10);
    require(e[0], 20);
    auto f = e.div(4);
    require(f[0], 5);
    auto g = f.clear();
    require(g[0], 0);

    // original chain all untouched
    require(a[0], 99);
    require(b[0], 7);
    require(c[0], 21);
    require(d[0], 30);
    require(e[0], 20);
    require(f[0], 5);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("N=1 – at() throws for index >= 1");
  {
    micron::iarray<int, 1> a;
    require_throw([&]() { (void)a.at(1); });
    require_throw([&]() { (void)a.at(100); });
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases – large N                                             //
  // ================================================================ //
  test_case("N=1024 – default zero, fill, sum, clear");
  {
    micron::iarray<int, 1024> a;
    require_true(all_eq(a, 0));

    auto b = a.fill(3);
    require_true(all_eq(b, 3));
    require(b.sum(), 3072);

    auto c = b.clear();
    require_true(all_eq(c, 0));
    require(c.sum(), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("N=1024 – copy independence");
  {
    auto a = make_seq<int, 1024>(0);
    micron::iarray<int, 1024> b(a);
    require_true(arr_eq(a, b));
  }
  end_test_case();

  // ================================================================ //
  //  Different types                                                  //
  // ================================================================ //
  test_case("iarray<float, 16> – arithmetic");
  {
    micron::iarray<float, 16> a(1.0f);
    auto b = (a += 1.0f);
    require_true(all_eq(b, 2.0f));
    require_true(all_eq(a, 1.0f));

    micron::iarray<float, 4> c{ 1.5f, 2.5f, 3.5f, 4.5f };
    require(c[0], 1.5f);
    require(c[3], 4.5f);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iarray<char, 8> – character storage");
  {
    micron::iarray<char, 8> a{ 'h', 'e', 'l', 'l', 'o', '\0', '\0', '\0' };
    require(a[0], 'h');
    require(a[4], 'o');
    require(a[5], '\0');

    auto b = a.fill('Z');
    require_true(b.all('Z'));
    require(a[0], 'h');
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("iarray<unsigned, 8> – unsigned arithmetic");
  {
    micron::iarray<unsigned, 8> a(10u);
    auto b = a.sub(3);
    require_true(all_eq(b, 7u));

    auto c = b.mul(2);
    require_true(all_eq(c, 14u));

    require(c.sum(), 112u);
  }
  end_test_case();

  // ================================================================ //
  //  Persistence / immutability stress tests                          //
  // ================================================================ //
  test_case("stress: versioning – keep multiple versions alive");
  {
    micron::iarray<int, 16> v0;
    auto v1 = v0.fill(10);
    auto v2 = v1.add(5);
    auto v3 = v2.mul(2);
    auto v4 = v3.sub(10);
    auto v5 = v4.div(2);

    require_true(all_eq(v0, 0));
    require_true(all_eq(v1, 10));
    require_true(all_eq(v2, 15));
    require_true(all_eq(v3, 30));
    require_true(all_eq(v4, 20));
    require_true(all_eq(v5, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: many sequential add/sub cycles preserve value");
  {
    auto a = make_seq<int, 64>(1);
    auto b = a;
    for ( int i = 0; i < 100; ++i ) {
      b = b.add(1);
      b = b.sub(1);
    }
    require_true(arr_eq(a, b));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: mul/div named cycles preserve value");
  {
    micron::iarray<int, 64> a(1);
    auto b = a;
    for ( int i = 0; i < 100; ++i ) {
      b = b.mul(3);
      b = b.div(3);
    }
    require_true(arr_eq(a, b));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: binary ops with sequential arrays");
  {
    auto a = make_seq<int, 64>(1);
    auto b = a;

    auto doubled = a + b;
    for ( int i = 0; i < 64; ++i )
      require(doubled[i], 2 * (i + 1));
    for ( int i = 0; i < 64; ++i )
      require(a[i], i + 1);

    auto restored = doubled - b;
    require_true(arr_eq(restored, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: copy chain maintains independence");
  {
    auto base = make_seq<int, 32>(0);
    micron::iarray<int, 32> a(base), b(a), c(b);
    require_true(arr_eq(base, c));

    auto d = c.fill(-1);
    require_true(all_eq(d, -1));
    require(base[0], 0);
    require(a[0], 0);
    require(b[0], 0);
    require(c[0], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: repeated fill/clear cycles");
  {
    micron::iarray<int, 256> a;
    for ( int round = 0; round < 500; ++round ) {
      auto b = a.fill(round % 127);
      require_true(b.all(round % 127));
      a = b.clear();
      require_true(all_eq(a, 0));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: scalar assignment then verify");
  {
    micron::iarray<int, 128> a;
    for ( int k = -50; k <= 50; ++k ) {
      a = k;
      require_true(all_eq(a, k));
    }
  }
  end_test_case();

  // ================================================================ //
  //  Invariants                                                       //
  // ================================================================ //
  test_case("invariant: sum(fill(k)) == k * N");
  {
    micron::iarray<int, 64> a;
    for ( int k : { 0, 1, 5, 10, -1 } ) {
      auto b = a.fill(k);
      require(b.sum(), k * 64);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: mul_reduce() after fill(2) == 2^N");
  {
    micron::iarray<int, 4> a(2);
    require(a.mul_reduce(), 16);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: all(k) iff fill(k) and no modification");
  {
    micron::iarray<int, 16> base;
    for ( int k : { 0, 1, -1, 42, 1000 } ) {
      auto a = base.fill(k);
      require_true(a.all(k));
      require_false(a.all(k + 1));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: (a + b) - b == a");
  {
    auto a = make_seq<int, 32>(1);
    auto b = make_seq<int, 32>(100);
    auto round_trip = (a + b) - b;
    require_true(arr_eq(round_trip, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: a * ones == a");
  {
    auto a = make_seq<int, 16>(5);
    micron::iarray<int, 16> ones(1);
    auto r = a * ones;
    require_true(arr_eq(r, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: a + zero == a");
  {
    auto a = make_seq<int, 16>(1);
    micron::iarray<int, 16> zero;
    auto r = a + zero;
    require_true(arr_eq(r, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: a - a == zero");
  {
    auto a = make_seq<int, 16>(1);
    auto r = a - a;
    require_true(all_eq(r, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: end() - begin() == N for all sizes");
  {
    micron::iarray<int, 1> a1;
    micron::iarray<int, 8> a8;
    micron::iarray<int, 64> a64;
    micron::iarray<int, 512> a512;

    require((int)(a1.end() - a1.begin()), 1);
    require((int)(a8.end() - a8.begin()), 8);
    require((int)(a64.end() - a64.begin()), 64);
    require((int)(a512.end() - a512.begin()), 512);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: no operation mutates source (exhaustive)");
  {
    micron::iarray<int, 8> a(10);
    micron::iarray<int, 8> b(3);

    (void)(a += 1);
    require_true(all_eq(a, 10));
    (void)(a -= 1);
    require_true(all_eq(a, 10));
    (void)(a *= 2);
    require_true(all_eq(a, 10));
    (void)(a /= 2);
    require_true(all_eq(a, 10));
    (void)(a %= 3);
    require_true(all_eq(a, 10));
    (void)(a += b);
    require_true(all_eq(a, 10));
    (void)(a -= b);
    require_true(all_eq(a, 10));
    (void)(a *= b);
    require_true(all_eq(a, 10));
    (void)(a /= b);
    require_true(all_eq(a, 10));
    (void)(a %= b);
    require_true(all_eq(a, 10));
    (void)(a + b);
    require_true(all_eq(a, 10));
    (void)(a - b);
    require_true(all_eq(a, 10));
    (void)(a * b);
    require_true(all_eq(a, 10));
    (void)(a / b);
    require_true(all_eq(a, 10));
    (void)(a % b);
    require_true(all_eq(a, 10));
    (void)a.add(1);
    require_true(all_eq(a, 10));
    (void)a.sub(1);
    require_true(all_eq(a, 10));
    (void)a.mul(2);
    require_true(all_eq(a, 10));
    (void)a.div(2);
    require_true(all_eq(a, 10));
    (void)a.clear();
    require_true(all_eq(a, 10));
    (void)a.fill(99);
    require_true(all_eq(a, 10));
    (void)a.sqrt();
    require_true(all_eq(a, 10));
    (void)a.sum();
    require_true(all_eq(a, 10));
    (void)a.mul_reduce();
    require_true(all_eq(a, 10));
    (void)a.all(10);
    require_true(all_eq(a, 10));
    (void)a.any(10);
    require_true(all_eq(a, 10));
  }
  end_test_case();

  sb::print("=== ALL IARRAY TESTS PASSED ===");
  return 1;
}
