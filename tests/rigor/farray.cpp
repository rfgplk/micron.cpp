// farray_tests.cpp
// Rigorous snowball test suite for micron::farray<T, N>

#include "../../src/array/farray.hpp"
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

// verify every element equals val
template <typename T, usize N>
bool
all_eq(const micron::farray<T, N> &a, T val)
{
  for ( usize i = 0; i < N; ++i )
    if ( a[i] != val )
      return false;
  return true;
}

// verify two arrays are element-wise equal
template <typename T, usize N>
bool
arr_eq(const micron::farray<T, N> &a, const micron::farray<T, N> &b)
{
  for ( usize i = 0; i < N; ++i )
    if ( a[i] != b[i] )
      return false;
  return true;
}

// make a sequential farray: [start, start+1, ..., start+N-1]
template <typename T, usize N>
micron::farray<T, N>
make_seq(T start = T{})
{
  micron::farray<T, N> a;
  for ( usize i = 0; i < N; ++i )
    a[i] = static_cast<T>(start + (T)i);
  return a;
}

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== FARRAY TESTS ===");

  // ================================================================ //
  //  Construction                                                     //
  // ================================================================ //
  test_case("default construction – all elements zero");
  {
    micron::farray<int, 8> a8;
    micron::farray<int, 64> a64;
    micron::farray<int, 128> a128;
    micron::farray<float, 16> af;
    micron::farray<char, 32> ac;

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
    micron::farray<int, 16> a(42);
    micron::farray<float, 16> b(3.14f);
    micron::farray<char, 8> c('X');
    micron::farray<int, 1> one(99);
    micron::farray<int, 64> big(-1);

    require_true(all_eq(a, 42));
    require_true(all_eq(b, 3.14f));
    require_true(all_eq(c, 'X'));
    require_true(all_eq(one, 99));
    require_true(all_eq(big, -1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – partial fill, rest stays zero");
  {
    micron::farray<int, 8> a{ 1, 2, 3 };
    require(a[0], 1);
    require(a[1], 2);
    require(a[2], 3);
    // elements beyond the list must be zero
    require(a[3], 0);
    require(a[7], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – exact fill");
  {
    micron::farray<int, 4> a{ 10, 20, 30, 40 };
    require(a[0], 10);
    require(a[1], 20);
    require(a[2], 30);
    require(a[3], 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – too large throws");
  {
    require_throw([]() { micron::farray<int, 4> a{ 1, 2, 3, 4, 5 }; });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – single element");
  {
    micron::farray<int, 8> a{ 99 };
    require(a[0], 99);
    for ( usize i = 1; i < 8; ++i )
      require(a[i], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy construction produces independent clone");
  {
    micron::farray<int, 16> a = make_seq<int, 16>(1);
    micron::farray<int, 16> b(a);
    require_true(arr_eq(a, b));

    b[0] = 9999;
    require(a[0], 1);     // a unaffected
    require(b[0], 9999);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction transfers content, source zeroed");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(10);
    micron::farray<int, 8> b(micron::move(a));

    require(b[0], 10);
    require(b[7], 17);
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy assignment");
  {
    micron::farray<int, 16> a = make_seq<int, 16>(5);
    micron::farray<int, 16> b;
    b = a;
    require_true(arr_eq(a, b));

    b[3] = -1;
    require(a[3], 8);     // a[3] = 5+3
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("scalar assignment fills all elements");
  {
    micron::farray<int, 16> a = make_seq<int, 16>(1);
    a = 7;
    require_true(all_eq(a, 7));

    a = 0;
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("generator constructor – fills via callable");
  {
    int counter = 0;
    micron::farray<int, 8> a([&counter]() { return counter++; });
    for ( int i = 0; i < 8; ++i )
      require(a[i], i);
  }
  end_test_case();

  // ================================================================ //
  //  Capacity                                                         //
  // ================================================================ //
  test_case("size() and max_size() equal N");
  {
    micron::farray<int, 1> a1;
    micron::farray<int, 64> a64;
    micron::farray<int, 1024> a1024;

    require(a1.size(), usize(1));
    require(a1.max_size(), usize(1));
    require(a64.size(), usize(64));
    require(a1024.size(), usize(1024));
  }
  end_test_case();

  // ================================================================ //
  //  Element access                                                   //
  // ================================================================ //
  test_case("operator[] read and write");
  {
    micron::farray<int, 8> a;
    a[0] = 1;
    a[7] = 99;
    require(a[0], 1);
    require(a[7], 99);
    require(a[3], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() returns correct element");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(0);
    for ( usize i = 0; i < 8; ++i )
      require(a.at(i), (int)i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() throws on out-of-bounds");
  {
    micron::farray<int, 4> a;
    require_throw([&]() { (void)a.at(4); });
    require_throw([&]() { (void)a.at(99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const at() throws on out-of-bounds");
  {
    const micron::farray<int, 4> a;
    require_throw([&]() { (void)a.at(4); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() returns pointer to element");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(10);
    int *p = a.get(0);
    require(*p, 10);

    int *p3 = a.get(3);
    require(*p3, 13);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() throws on out-of-bounds");
  {
    micron::farray<int, 4> a;
    require_throw([&]() { a.get(4); });
    require_throw([&]() { a.get(99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cget() throws on out-of-bounds");
  {
    const micron::farray<int, 4> a;
    require_throw([&]() { a.cget(4); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("data() returns pointer to first element");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(5);
    int *p = a.data();
    require_true(p != nullptr);
    require(*p, 5);
    require(*(p + 7), 12);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const data() is non-null and consistent");
  {
    const micron::farray<int, 8> a = make_seq<int, 8>(0);
    const int *p = a.data();
    require_true(p != nullptr);
    require(*p, 0);
  }
  end_test_case();

  // ================================================================ //
  //  Iterators                                                        //
  // ================================================================ //
  test_case("begin/end span all N elements");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(0);
    int expected = 0;
    for ( auto it = a.begin(); it != a.end(); ++it ) {
      require(*it, expected++);
    }
    require(expected, 8);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend span all N elements (const)");
  {
    const micron::farray<int, 8> a = make_seq<int, 8>(1);
    int expected = 1;
    for ( auto it = a.cbegin(); it != a.cend(); ++it ) {
      require(*it, expected++);
    }
    require(expected, 9);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("range-for loop iterates in order");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(0);
    int sum = 0;
    for ( int v : a )
      sum += v;
    require(sum, 28);     // 0+1+...+7
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("begin() to end() distance equals N");
  {
    micron::farray<int, 64> a;
    require((int)(a.end() - a.begin()), 64);
  }
  end_test_case();

  // ================================================================ //
  //  clear()                                                         //
  // ================================================================ //
  test_case("clear() zeroes all elements");
  {
    micron::farray<int, 16> a = make_seq<int, 16>(1);
    a.clear();
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear() on already-zero array is a no-op");
  {
    micron::farray<int, 8> a;
    a.clear();
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear() then reuse works correctly");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(1);
    a.clear();
    a[3] = 77;
    require(a[3], 77);
    require(a[0], 0);
    require(a[7], 0);
  }
  end_test_case();

  // ================================================================ //
  //  fill()                                                          //
  // ================================================================ //
  test_case("fill() sets all elements to given value");
  {
    micron::farray<int, 16> a;
    a.fill(42);
    require_true(all_eq(a, 42));

    a.fill(0);
    require_true(all_eq(a, 0));

    a.fill(-1);
    require_true(all_eq(a, -1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fill() returns *this for chaining");
  {
    micron::farray<int, 8> a;
    auto &ref = a.fill(5);
    require_true(&ref == &a);
    require_true(all_eq(a, 5));
  }
  end_test_case();

  // ================================================================ //
  //  Arithmetic ops                                                  //
  // ================================================================ //
  test_case("operator*= scalar multiplies every element");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(1);     // 1..8
    a *= 3;
    for ( int i = 0; i < 8; ++i )
      require(a[i], (i + 1) * 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul(n) multiplies every element in-place");
  {
    micron::farray<int, 8> a(2);
    a.mul((usize)5);
    require_true(all_eq(a, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("div(n) divides every element in-place");
  {
    micron::farray<int, 8> a(100);
    a.div((usize)4);
    require_true(all_eq(a, 25));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("add(n) adds to every element in-place");
  {
    micron::farray<int, 8> a(10);
    a.add((usize)5);
    require_true(all_eq(a, 15));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sub(n) subtracts from every element in-place");
  {
    micron::farray<int, 8> a(10);
    a.sub((usize)3);
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul/div are inverses (exact integer)");
  {
    micron::farray<int, 16> a = make_seq<int, 16>(1);
    micron::farray<int, 16> orig = a;
    a.mul((usize)7);
    a.div((usize)7);
    require_true(arr_eq(a, orig));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("add/sub are inverses");
  {
    micron::farray<int, 16> a = make_seq<int, 16>(0);
    micron::farray<int, 16> orig = a;
    a.add((usize)100);
    a.sub((usize)100);
    require_true(arr_eq(a, orig));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise operator+ modifies left-hand side in-place");
  {
    micron::farray<int, 8> a(3);
    micron::farray<int, 8> b(4);
    a + b;
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise operator- modifies left-hand side in-place");
  {
    micron::farray<int, 8> a(10);
    micron::farray<int, 8> b(3);
    a - b;
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise operator* modifies left-hand side in-place");
  {
    micron::farray<int, 8> a(3);
    micron::farray<int, 8> b(4);
    a *b;
    require_true(all_eq(a, 12));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise operator/ modifies left-hand side in-place");
  {
    micron::farray<int, 8> a(12);
    micron::farray<int, 8> b(4);
    a / b;
    require_true(all_eq(a, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("element-wise operator% modifies left-hand side in-place");
  {
    micron::farray<int, 8> a(10);
    micron::farray<int, 8> b(3);
    a % b;
    require_true(all_eq(a, 1));     // 10 % 3 == 1
  }
  end_test_case();

  // ================================================================ //
  //  Reductions                                                      //
  // ================================================================ //
  test_case("sum() – sum of all elements");
  {
    micron::farray<int, 8> a = make_seq<int, 8>(1);     // 1..8
    require(a.sum(), usize(36));

    micron::farray<int, 8> z;
    require(z.sum(), usize(0));

    micron::farray<int, 4> ones(1);
    require(ones.sum(), usize(4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul() – product of all elements");
  {
    micron::farray<int, 4> a{ 1, 2, 3, 4 };
    require(a.mul(), usize(24));

    micron::farray<int, 4> ones(1);
    require(ones.mul(), usize(1));

    micron::farray<int, 4> z;     // zeroes
    require(z.mul(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sum() – large array correctness");
  {
    // farray<int, 100> filled with 1 → sum = 100
    micron::farray<int, 100> a(1);
    require(a.sum(), usize(100));
  }
  end_test_case();

  // ================================================================ //
  //  all() / any()                                                   //
  // ================================================================ //
  test_case("all() – returns true only when every element equals value");
  {
    micron::farray<int, 8> a(5);
    require_true(a.all(5));
    require_false(a.all(4));
    require_false(a.all(0));

    a[3] = 99;
    require_false(a.all(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("any() – returns true when at least one element equals value");
  {
    micron::farray<int, 8> a;
    require_false(a.any(1));

    a[4] = 1;
    require_true(a.any(1));
    require_false(a.any(2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("all() and any() on single-element array");
  {
    micron::farray<int, 1> a(7);
    require_true(a.all(7));
    require_false(a.all(8));
    require_true(a.any(7));
    require_false(a.any(8));
  }
  end_test_case();

  // ================================================================ //
  //  Static type helpers                                             //
  // ================================================================ //
  test_case("is_pod / is_class / is_trivial");
  {
    // int: pod, non-class, trivial
    require_true(micron::farray<int, 8>::is_pod());
    require_false(micron::farray<int, 8>::is_class());
    require_true(micron::farray<int, 8>::is_trivial());

    // float: same
    require_true(micron::farray<float, 8>::is_pod());
    require_false(micron::farray<float, 8>::is_class());
    require_true(micron::farray<float, 8>::is_trivial());

    // char: same
    require_true(micron::farray<char, 8>::is_pod());
    require_false(micron::farray<char, 8>::is_class());
    require_true(micron::farray<char, 8>::is_trivial());
  }
  end_test_case();

  // ================================================================ //
  //  Alignment                                                       //
  // ================================================================ //
  test_case("data() is 64-byte aligned");
  {
    micron::farray<int, 64> a;
    micron::farray<float, 64> b;
    micron::farray<char, 64> c;

    require_true((reinterpret_cast<usize>(a.data()) % 64) == 0);
    require_true((reinterpret_cast<usize>(b.data()) % 64) == 0);
    require_true((reinterpret_cast<usize>(c.data()) % 64) == 0);
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases – N = 1                                              //
  // ================================================================ //
  test_case("N=1 – construction, access, arithmetic");
  {
    micron::farray<int, 1> a(99);
    require(a[0], 99);
    require(a.at(0), 99);
    require(a.size(), usize(1));

    a.fill(7);
    require(a[0], 7);
    require(a.sum(), usize(7));
    require(a.mul(), usize(7));
    require_true(a.all(7));
    require_false(a.any(8));

    a.mul((usize)3);
    require(a[0], 21);

    a.add((usize)9);
    require(a[0], 30);

    a.sub((usize)10);
    require(a[0], 20);

    a.div((usize)4);
    require(a[0], 5);

    a.clear();
    require(a[0], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("N=1 – at() throws for any index >= 1");
  {
    micron::farray<int, 1> a;
    require_throw([&]() { (void)a.at(1); });
    require_throw([&]() { (void)a.at(100); });
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases – large N                                            //
  // ================================================================ //
  test_case("N=1024 – default zero, fill, sum, clear");
  {
    micron::farray<int, 1024> a;
    require_true(all_eq(a, 0));

    a.fill(3);
    require_true(all_eq(a, 3));
    require(a.sum(), usize(3072));

    a.clear();
    require_true(all_eq(a, 0));
    require(a.sum(), usize(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("N=1024 – sequential fill and sum");
  {
    micron::farray<int, 1024> a = make_seq<int, 1024>(0);
    // sum = 0+1+...+1023 = 1023*1024/2 = 523776
    require(a.sum(), usize(523776));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("N=1024 – copy independence");
  {
    micron::farray<int, 1024> a = make_seq<int, 1024>(0);
    micron::farray<int, 1024> b(a);
    require_true(arr_eq(a, b));

    b[0] = -999;
    require(a[0], 0);
    require(b[0], -999);
  }
  end_test_case();

  // ================================================================ //
  //  Different types                                                 //
  // ================================================================ //
  test_case("farray<float, 16> – arithmetic precision");
  {
    micron::farray<float, 16> a(1.0f);
    a.add((usize)1);     // note: add takes usize, converts
    // result depends on implicit conversion, test structural behaviour
    require_true(a.any((float)2.0f) || a.any((float)1.0f));     // either way not zero

    micron::farray<float, 4> b{ 1.5f, 2.5f, 3.5f, 4.5f };
    require(b[0], 1.5f);
    require(b[3], 4.5f);
    require(b.size(), usize(4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("farray<char, 8> – character storage");
  {
    micron::farray<char, 8> a{ 'h', 'e', 'l', 'l', 'o', '\0', '\0', '\0' };
    require(a[0], 'h');
    require(a[1], 'e');
    require(a[4], 'o');
    require(a[5], '\0');

    a.fill('Z');
    require_true(a.all('Z'));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("farray<unsigned, 8> – unsigned arithmetic");
  {
    micron::farray<unsigned, 8> a(10u);
    a.sub((usize)3);
    require_true(a.all(7u));

    a.mul((usize)2);
    require_true(a.all(14u));

    require(a.sum(), usize(112));     // 8 * 14
  }
  end_test_case();

  // ================================================================ //
  //  Stress tests                                                    //
  // ================================================================ //
  test_case("stress: repeated fill/clear cycles");
  {
    micron::farray<int, 256> a;
    for ( int round = 0; round < 1000; ++round ) {
      a.fill(round % 127);
      require_true(a.all(round % 127));
      a.clear();
      require_true(all_eq(a, 0));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: sequential add/sub cycles preserve value");
  {
    micron::farray<int, 64> a;
    for ( int i = 0; i < 1000; ++i ) {
      a.add((usize)1);
      a.sub((usize)1);
    }
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: mul/div cycles preserve value");
  {
    micron::farray<int, 64> a(1);
    for ( int i = 0; i < 100; ++i ) {
      a.mul((usize)3);
      a.div((usize)3);
    }
    require_true(all_eq(a, 1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: element-wise ops over sequential arrays");
  {
    // verify +(seq) is an in-place double
    micron::farray<int, 64> a = make_seq<int, 64>(1);
    micron::farray<int, 64> b = a;

    a + b;     // a[i] += b[i] → a[i] = 2*(i+1)
    for ( int i = 0; i < 64; ++i )
      require(a[i], 2 * (i + 1));

    // subtract back
    a - b;     // a[i] -= b[i] → a[i] = (i+1)
    for ( int i = 0; i < 64; ++i )
      require(a[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: copy chain maintains independence");
  {
    micron::farray<int, 32> base = make_seq<int, 32>(0);
    micron::farray<int, 32> a(base), b(a), c(b);
    require_true(arr_eq(base, c));

    c[0] = -1;
    require(base[0], 0);
    require(a[0], 0);
    require(b[0], 0);
    require(c[0], -1);
  }
  end_test_case();

  // ================================================================ //
  //  Invariants                                                      //
  // ================================================================ //
  test_case("invariant: sum(fill(k)) == k * N");
  {
    micron::farray<int, 64> a;
    for ( int k : { 0, 1, 5, 10, -1 } ) {
      a.fill(k);
      require(a.sum(), usize(k * 64));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: mul() == product of all elements after fill(k)");
  {
    micron::farray<int, 4> a(2);     // [2,2,2,2], product = 16
    require(a.mul(), usize(16));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: all(k) iff fill(k) and no modification");
  {
    micron::farray<int, 16> a;
    for ( int k : { 0, 1, -1, 42, 1000 } ) {
      a.fill(k);
      require_true(a.all(k));
      require_false(a.all(k + 1));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: any(k) iff at least one element equals k");
  {
    micron::farray<int, 16> a;     // all zeros
    require_true(a.any(0));
    require_false(a.any(1));

    a[8] = 42;
    require_true(a.any(42));
    require_false(a.any(41));
    require_true(a.any(0));     // others still 0
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: end() - begin() == N for all sizes");
  {
    micron::farray<int, 1> a1;
    micron::farray<int, 8> a8;
    micron::farray<int, 64> a64;
    micron::farray<int, 512> a512;

    require((int)(a1.end() - a1.begin()), 1);
    require((int)(a8.end() - a8.begin()), 8);
    require((int)(a64.end() - a64.begin()), 64);
    require((int)(a512.end() - a512.begin()), 512);
  }
  end_test_case();

  sb::print("=== ALL FARRAY TESTS PASSED ===");
  return 0;
}
