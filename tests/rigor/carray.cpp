// carray_tests.cpp
// Rigorous snowball test suite for micron::carray<T, N>

#include "../../src/array/carray.hpp"
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
all_eq(const micron::carray<T, N> &a, T val)
{
  for ( usize i = 0; i < N; ++i )
    if ( a[i] != val )
      return false;
  return true;
}

template <typename T, usize N>
bool
arr_eq(const micron::carray<T, N> &a, const micron::carray<T, N> &b)
{
  for ( usize i = 0; i < N; ++i )
    if ( a[i] != b[i] )
      return false;
  return true;
}

template <typename T, usize N>
micron::carray<T, N>
make_seq(T start = T{})
{
  micron::carray<T, N> a;
  for ( usize i = 0; i < N; ++i )
    a[i] = static_cast<T>(start + (T)i);
  return a;
}

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== CARRAY TESTS ===");

  // ================================================================ //
  //  Construction                                                     //
  // ================================================================ //
  test_case("default construction – all elements default");
  {
    micron::carray<int, 8> a8;
    micron::carray<int, 64> a64;
    micron::carray<int, 128> a128;
    micron::carray<float, 16> af;
    micron::carray<char, 32> ac;

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
    micron::carray<int, 16> a(42);
    micron::carray<float, 16> b(3.14f);
    micron::carray<char, 8> c('X');
    micron::carray<int, 1> one(99);
    micron::carray<int, 64> big(-1);

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
    micron::carray<int, 8> a{ 1, 2, 3 };
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
    micron::carray<int, 4> a{ 10, 20, 30, 40 };
    require(a[0], 10);
    require(a[1], 20);
    require(a[2], 30);
    require(a[3], 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – too large throws");
  {
    require_throw([]() { micron::carray<int, 4> a{ 1, 2, 3, 4, 5 }; });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list – single element");
  {
    micron::carray<int, 8> a{ 99 };
    require(a[0], 99);
    for ( usize i = 1; i < 8; ++i )
      require(a[i], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("generator constructor – fills via callable");
  {
    int counter = 0;
    micron::carray<int, 8> a([&counter]() { return counter++; });
    for ( int i = 0; i < 8; ++i )
      require(a[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("copy construction produces independent clone");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(1);
    micron::carray<int, 16> b(a);
    require_true(arr_eq(a, b));

    b[0] = 9999;
    require(a[0], 1);
    require(b[0], 9999);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move construction transfers content");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(10);
    micron::carray<int, 8> b(micron::move(a));
    require(b[0], 10);
    require(b[7], 17);
  }
  end_test_case();

  // ================================================================ //
  //  Capacity                                                         //
  // ================================================================ //
  test_case("size() and max_size() equal N");
  {
    micron::carray<int, 1> a1;
    micron::carray<int, 64> a64;
    micron::carray<int, 1024> a1024;

    require(a1.size(), usize(1));
    require(a1.max_size(), usize(1));
    require(a64.size(), usize(64));
    require(a1024.size(), usize(1024));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("static constexpr length equals N");
  {
    require(micron::carray<int, 8>::length, usize(8));
    require(micron::carray<int, 64>::length, usize(64));
    require(micron::carray<int, 1>::length, usize(1));
  }
  end_test_case();

  // ================================================================ //
  //  Element access — no bounds checks (cache class)                  //
  // ================================================================ //
  test_case("operator[] read and write");
  {
    micron::carray<int, 8> a;
    a[0] = 1;
    a[7] = 99;
    require(a[0], 1);
    require(a[7], 99);
    require(a[3], 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("at() read and write (unchecked)");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(0);
    for ( usize i = 0; i < 8; ++i )
      require(a.at(i), (int)i);

    a.at(3) = 77;
    require(a.at(3), 77);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const at() returns correct element");
  {
    const micron::carray<int, 4> a{ 10, 20, 30, 40 };
    require(a.at(0), 10);
    require(a.at(3), 40);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("get() returns pointer to element");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(10);
    int *p = a.get(0);
    require(*p, 10);
    int *p3 = a.get(3);
    require(*p3, 13);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cget() returns const pointer");
  {
    const micron::carray<int, 8> a = make_seq<int, 8>(5);
    const int *p = a.cget(2);
    require(*p, 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("data() returns pointer to first element");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(5);
    int *p = a.data();
    require_true(p != nullptr);
    require(*p, 5);
    require(*(p + 7), 12);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const data() is non-null and consistent");
  {
    const micron::carray<int, 8> a = make_seq<int, 8>(0);
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
    micron::carray<int, 8> a = make_seq<int, 8>(0);
    int expected = 0;
    for ( auto it = a.begin(); it != a.end(); ++it )
      require(*it, expected++);
    require(expected, 8);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("cbegin/cend span all N elements (const)");
  {
    const micron::carray<int, 8> a = make_seq<int, 8>(1);
    int expected = 1;
    for ( auto it = a.cbegin(); it != a.cend(); ++it )
      require(*it, expected++);
    require(expected, 9);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("range-for loop iterates in order");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(0);
    int sum = 0;
    for ( int v : a )
      sum += v;
    require(sum, 28);     // 0+1+...+7
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("begin() to end() distance equals N");
  {
    micron::carray<int, 64> a;
    require((int)(a.end() - a.begin()), 64);
  }
  end_test_case();

  // ================================================================ //
  //  clear()                                                          //
  // ================================================================ //
  test_case("clear() zeroes all elements and leaves valid state");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(1);
    a.clear();
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear() on already-zero array is a no-op");
  {
    micron::carray<int, 8> a;
    a.clear();
    require_true(all_eq(a, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear() then reuse works correctly");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(1);
    a.clear();
    a[3] = 77;
    require(a[3], 77);
    require(a[0], 0);
    require(a[7], 0);
  }
  end_test_case();

  // ================================================================ //
  //  fill()                                                           //
  // ================================================================ //
  test_case("fill() sets all elements to given value");
  {
    micron::carray<int, 16> a;
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
    micron::carray<int, 8> a;
    auto &ref = a.fill(5);
    require_true(&ref == &a);
    require_true(all_eq(a, 5));
  }
  end_test_case();

  // ================================================================ //
  //  Assignment                                                       //
  // ================================================================ //
  test_case("copy assignment");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(5);
    micron::carray<int, 16> b;
    b = a;
    require_true(arr_eq(a, b));

    b[3] = -1;
    require(a[3], 8);     // a[3] = 5+3
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move assignment");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(10);
    micron::carray<int, 8> b;
    b = micron::move(a);
    require(b[0], 10);
    require(b[7], 17);
  }
  end_test_case();

  // ================================================================ //
  //  Compound assignment — scalar (in-place)                          //
  // ================================================================ //
  test_case("operator+= scalar adds to every element");
  {
    micron::carray<int, 8> a(10);
    a += 5;
    require_true(all_eq(a, 15));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator-= scalar subtracts from every element");
  {
    micron::carray<int, 8> a(10);
    a -= 3;
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator*= scalar multiplies every element");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(1);     // 1..8
    a *= 3;
    for ( int i = 0; i < 8; ++i )
      require(a[i], (i + 1) * 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator/= scalar divides every element");
  {
    micron::carray<int, 8> a(100);
    a /= 4;
    require_true(all_eq(a, 25));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator%= scalar modulos every element");
  {
    micron::carray<int, 8> a(10);
    a %= 3;
    require_true(all_eq(a, 1));
  }
  end_test_case();

  // ================================================================ //
  //  Compound assignment — array (in-place)                           //
  // ================================================================ //
  test_case("operator+= array adds element-wise in place");
  {
    micron::carray<int, 8> a(3);
    micron::carray<int, 8> b(4);
    a += b;
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator-= array subtracts element-wise in place");
  {
    micron::carray<int, 8> a(10);
    micron::carray<int, 8> b(3);
    a -= b;
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator*= array multiplies element-wise in place");
  {
    micron::carray<int, 8> a(3);
    micron::carray<int, 8> b(4);
    a *= b;
    require_true(all_eq(a, 12));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator/= array divides element-wise in place");
  {
    micron::carray<int, 8> a(12);
    micron::carray<int, 8> b(4);
    a /= b;
    require_true(all_eq(a, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator%= array modulos element-wise in place");
  {
    micron::carray<int, 8> a(10);
    micron::carray<int, 8> b(3);
    a %= b;
    require_true(all_eq(a, 1));
  }
  end_test_case();

  // ================================================================ //
  //  Named in-place arithmetic                                        //
  // ================================================================ //
  test_case("mul(n) multiplies every element in-place");
  {
    micron::carray<int, 8> a(2);
    a.mul((usize)5);
    require_true(all_eq(a, 10));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("div(n) divides every element in-place");
  {
    micron::carray<int, 8> a(100);
    a.div((usize)4);
    require_true(all_eq(a, 25));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("add(n) adds to every element in-place");
  {
    micron::carray<int, 8> a(10);
    a.add((usize)5);
    require_true(all_eq(a, 15));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sub(n) subtracts from every element in-place");
  {
    micron::carray<int, 8> a(10);
    a.sub((usize)3);
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul/div are inverses (exact integer)");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(1);
    micron::carray<int, 16> orig = a;
    a.mul((usize)7);
    a.div((usize)7);
    require_true(arr_eq(a, orig));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("add/sub are inverses");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(0);
    micron::carray<int, 16> orig = a;
    a.add((usize)100);
    a.sub((usize)100);
    require_true(arr_eq(a, orig));
  }
  end_test_case();

  // ================================================================ //
  //  Binary arithmetic — value semantics                              //
  //  operator+/-/*/ /% return NEW arrays, originals unchanged          //
  // ================================================================ //
  test_case("operator+ returns new array, operands unchanged");
  {
    micron::carray<int, 8> a(3);
    micron::carray<int, 8> b(4);
    auto c = a + b;
    require_true(all_eq(c, 7));
    require_true(all_eq(a, 3));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator- returns new array, operands unchanged");
  {
    micron::carray<int, 8> a(10);
    micron::carray<int, 8> b(3);
    auto c = a - b;
    require_true(all_eq(c, 7));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator* returns new array, operands unchanged");
  {
    micron::carray<int, 8> a(3);
    micron::carray<int, 8> b(4);
    auto c = a * b;
    require_true(all_eq(c, 12));
    require_true(all_eq(a, 3));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator/ returns new array, operands unchanged");
  {
    micron::carray<int, 8> a(12);
    micron::carray<int, 8> b(4);
    auto c = a / b;
    require_true(all_eq(c, 3));
    require_true(all_eq(a, 12));
    require_true(all_eq(b, 4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("operator% returns new array, operands unchanged");
  {
    micron::carray<int, 8> a(10);
    micron::carray<int, 8> b(3);
    auto c = a % b;
    require_true(all_eq(c, 1));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("binary ops with sequential arrays");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(1);
    micron::carray<int, 8> b = make_seq<int, 8>(1);
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
    micron::carray<int, 4> a(10);
    micron::carray<int, 4> b(3);
    micron::carray<int, 4> c(2);
    auto r = (a + b) * c;     // (10+3)*2 = 26
    require_true(all_eq(r, 26));
    require_true(all_eq(a, 10));
    require_true(all_eq(b, 3));
    require_true(all_eq(c, 2));
  }
  end_test_case();

  // ================================================================ //
  //  Reductions — return T                                            //
  // ================================================================ //
  test_case("sum() returns correct sum (returns T)");
  {
    micron::carray<int, 8> a = make_seq<int, 8>(1);     // 1..8
    require(a.sum(), 36);

    micron::carray<int, 8> z;
    require(z.sum(), 0);

    micron::carray<int, 4> ones(1);
    require(ones.sum(), 4);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("mul_reduce() returns correct product (returns T)");
  {
    micron::carray<int, 4> a{ 1, 2, 3, 4 };
    require(a.mul_reduce(), 24);

    micron::carray<int, 4> ones(1);
    require(ones.mul_reduce(), 1);

    micron::carray<int, 4> z;
    require(z.mul_reduce(), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sum() – large array correctness");
  {
    micron::carray<int, 100> a(1);
    require(a.sum(), 100);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sum() – sequential large array");
  {
    micron::carray<int, 1024> a = make_seq<int, 1024>(0);
    require(a.sum(), 523776);     // 0+1+...+1023
  }
  end_test_case();

  // ================================================================ //
  //  all() / any()                                                    //
  // ================================================================ //
  test_case("all() – true only when every element equals value");
  {
    micron::carray<int, 8> a(5);
    require_true(a.all(5));
    require_false(a.all(4));
    require_false(a.all(0));

    a[3] = 99;
    require_false(a.all(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("any() – true when at least one element equals value");
  {
    micron::carray<int, 8> a;
    require_false(a.any(1));

    a[4] = 1;
    require_true(a.any(1));
    require_false(a.any(2));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("all() and any() on single-element array");
  {
    micron::carray<int, 1> a(7);
    require_true(a.all(7));
    require_false(a.all(8));
    require_true(a.any(7));
    require_false(a.any(8));
  }
  end_test_case();

  // ================================================================ //
  //  Static type helpers                                              //
  // ================================================================ //
  test_case("is_pod / is_class / is_trivial");
  {
    require_true(micron::carray<int, 8>::is_pod());
    require_false(micron::carray<int, 8>::is_class());
    require_true(micron::carray<int, 8>::is_trivial());
    require_true(micron::carray<float, 8>::is_pod());
    require_true(micron::carray<char, 8>::is_trivial());
  }
  end_test_case();

  // ================================================================ //
  //  Alignment                                                        //
  // ================================================================ //
  test_case("data() is 64-byte aligned");
  {
    micron::carray<int, 64> a;
    micron::carray<float, 64> b;
    micron::carray<char, 64> c;

    require_true((reinterpret_cast<usize>(a.data()) % 64) == 0);
    require_true((reinterpret_cast<usize>(b.data()) % 64) == 0);
    require_true((reinterpret_cast<usize>(c.data()) % 64) == 0);
  }
  end_test_case();

  // ================================================================ //
  //  operator&                                                        //
  // ================================================================ //
  test_case("operator& returns byte* to backing storage");
  {
    micron::carray<int, 8> a(42);
    byte *p = &a;
    require_true(p != nullptr);
    require_true(p == reinterpret_cast<byte *>(a.data()));

    const micron::carray<int, 8> b(42);
    const byte *cp = &b;
    require_true(cp == reinterpret_cast<const byte *>(b.data()));
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases – N = 1                                               //
  // ================================================================ //
  test_case("N=1 – construction, access, arithmetic");
  {
    micron::carray<int, 1> a(99);
    require(a[0], 99);
    require(a.at(0), 99);
    require(a.size(), usize(1));

    a.fill(7);
    require(a[0], 7);
    require(a.sum(), 7);
    require(a.mul_reduce(), 7);
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

  // ================================================================ //
  //  Edge cases – large N                                             //
  // ================================================================ //
  test_case("N=1024 – default zero, fill, sum, clear");
  {
    micron::carray<int, 1024> a;
    require_true(all_eq(a, 0));

    a.fill(3);
    require_true(all_eq(a, 3));
    require(a.sum(), 3072);

    a.clear();
    require_true(all_eq(a, 0));
    require(a.sum(), 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("N=1024 – copy independence");
  {
    micron::carray<int, 1024> a = make_seq<int, 1024>(0);
    micron::carray<int, 1024> b(a);
    require_true(arr_eq(a, b));

    b[0] = -999;
    require(a[0], 0);
    require(b[0], -999);
  }
  end_test_case();

  // ================================================================ //
  //  SIMD correctness — various sizes                                 //
  //  These sizes exercise different code paths:                       //
  //    4: single SSE vector (or unroll)                               //
  //    8: two SSE / one AVX vector                                    //
  //   16: multiple vectors                                            //
  //   17: vector bulk + scalar tail                                   //
  //   64: multiple AVX vectors                                        //
  //  100: bulk + non-trivial tail                                     //
  //  256: deep SIMD loop + prefetch-eligible                          //
  // ================================================================ //
  test_case("SIMD correctness – N=4 compound += scalar");
  {
    micron::carray<int, 4> a(10);
    a += 5;
    require_true(all_eq(a, 15));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – N=8 compound += array");
  {
    micron::carray<int, 8> a(3);
    micron::carray<int, 8> b(4);
    a += b;
    require_true(all_eq(a, 7));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – N=16 operator+ value semantic");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(1);
    micron::carray<int, 16> b = make_seq<int, 16>(1);
    auto c = a + b;
    for ( int i = 0; i < 16; ++i )
      require(c[i], 2 * (i + 1));
    for ( int i = 0; i < 16; ++i )
      require(a[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – N=17 (bulk + tail)");
  {
    micron::carray<int, 17> a(10);
    a += 5;
    require_true(all_eq(a, 15));

    micron::carray<int, 17> b(3);
    a -= b;
    require_true(all_eq(a, 12));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – N=64 operator*= scalar");
  {
    micron::carray<int, 64> a = make_seq<int, 64>(1);
    a *= 2;
    for ( int i = 0; i < 64; ++i )
      require(a[i], (i + 1) * 2);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – N=100 (bulk + tail) round trip");
  {
    micron::carray<int, 100> a = make_seq<int, 100>(0);
    micron::carray<int, 100> orig = a;
    a += 50;
    a -= 50;
    require_true(arr_eq(a, orig));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – N=256 binary op");
  {
    micron::carray<int, 256> a(7);
    micron::carray<int, 256> b(3);
    auto c = a + b;
    require_true(all_eq(c, 10));
    require_true(all_eq(a, 7));
    require_true(all_eq(b, 3));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – float N=64 +=/-= round trip");
  {
    micron::carray<float, 64> a(1.0f);
    a += 2.5f;
    require_true(all_eq(a, 3.5f));
    a -= 2.5f;
    require_true(all_eq(a, 1.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – float N=64 *= scalar");
  {
    micron::carray<float, 64> a(2.0f);
    a *= 3.0f;
    require_true(all_eq(a, 6.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – float N=64 /= scalar");
  {
    micron::carray<float, 64> a(12.0f);
    a /= 4.0f;
    require_true(all_eq(a, 3.0f));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("SIMD correctness – float N=64 binary operator*");
  {
    micron::carray<float, 64> a(3.0f);
    micron::carray<float, 64> b(4.0f);
    auto c = a * b;
    require_true(all_eq(c, 12.0f));
    require_true(all_eq(a, 3.0f));
  }
  end_test_case();

  // ================================================================ //
  //  Different types                                                  //
  // ================================================================ //
  test_case("carray<float, 16> – arithmetic");
  {
    micron::carray<float, 16> a(1.0f);
    a += 1.0f;
    require_true(all_eq(a, 2.0f));

    micron::carray<float, 4> b{ 1.5f, 2.5f, 3.5f, 4.5f };
    require(b[0], 1.5f);
    require(b[3], 4.5f);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("carray<char, 8> – character storage");
  {
    micron::carray<char, 8> a{ 'h', 'e', 'l', 'l', 'o', '\0', '\0', '\0' };
    require(a[0], 'h');
    require(a[4], 'o');
    require(a[5], '\0');

    a.fill('Z');
    require_true(a.all('Z'));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("carray<unsigned, 8> – unsigned arithmetic");
  {
    micron::carray<unsigned, 8> a(10u);
    a.sub((usize)3);
    require_true(a.all(7u));

    a.mul((usize)2);
    require_true(a.all(14u));

    require(a.sum(), 112u);
  }
  end_test_case();

  // ================================================================ //
  //  Stress tests                                                     //
  // ================================================================ //
  test_case("stress: repeated fill/clear cycles");
  {
    micron::carray<int, 256> a;
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
    micron::carray<int, 64> a;
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
    micron::carray<int, 64> a(1);
    for ( int i = 0; i < 100; ++i ) {
      a.mul((usize)3);
      a.div((usize)3);
    }
    require_true(all_eq(a, 1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: compound += over sequential arrays");
  {
    micron::carray<int, 64> a = make_seq<int, 64>(1);
    micron::carray<int, 64> b = a;

    a += b;
    for ( int i = 0; i < 64; ++i )
      require(a[i], 2 * (i + 1));

    a -= b;
    for ( int i = 0; i < 64; ++i )
      require(a[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: value-semantic ops preserve originals");
  {
    micron::carray<int, 64> a = make_seq<int, 64>(1);
    micron::carray<int, 64> b = a;

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
    micron::carray<int, 32> base = make_seq<int, 32>(0);
    micron::carray<int, 32> a(base), b(a), c(b);
    require_true(arr_eq(base, c));

    c[0] = -1;
    require(base[0], 0);
    require(a[0], 0);
    require(b[0], 0);
    require(c[0], -1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: compound scalar += on large SIMD-eligible array");
  {
    micron::carray<int, 512> a(0);
    for ( int i = 0; i < 100; ++i )
      a += 1;
    require_true(all_eq(a, 100));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: compound *= on float array (SIMD path)");
  {
    micron::carray<float, 128> a(2.0f);
    a *= 0.5f;
    require_true(all_eq(a, 1.0f));

    for ( int i = 0; i < 50; ++i ) {
      a *= 2.0f;
      a *= 0.5f;
    }
    require_true(all_eq(a, 1.0f));
  }
  end_test_case();

  // ================================================================ //
  //  Invariants                                                       //
  // ================================================================ //
  test_case("invariant: sum(fill(k)) == k * N");
  {
    micron::carray<int, 64> a;
    for ( int k : { 0, 1, 5, 10, -1 } ) {
      a.fill(k);
      require(a.sum(), k * 64);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: mul_reduce() after fill(2) == 2^N");
  {
    micron::carray<int, 4> a(2);
    require(a.mul_reduce(), 16);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: all(k) iff fill(k) and no modification");
  {
    micron::carray<int, 16> a;
    for ( int k : { 0, 1, -1, 42, 1000 } ) {
      a.fill(k);
      require_true(a.all(k));
      require_false(a.all(k + 1));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: (a + b) - b == a");
  {
    micron::carray<int, 32> a = make_seq<int, 32>(1);
    micron::carray<int, 32> b = make_seq<int, 32>(100);
    auto round_trip = (a + b) - b;
    require_true(arr_eq(round_trip, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: a * ones == a");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(5);
    micron::carray<int, 16> ones(1);
    auto r = a * ones;
    require_true(arr_eq(r, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: a + zero == a");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(1);
    micron::carray<int, 16> zero;
    auto r = a + zero;
    require_true(arr_eq(r, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: a - a == zero");
  {
    micron::carray<int, 16> a = make_seq<int, 16>(1);
    auto r = a - a;
    require_true(all_eq(r, 0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: end() - begin() == N for all sizes");
  {
    micron::carray<int, 1> a1;
    micron::carray<int, 8> a8;
    micron::carray<int, 64> a64;
    micron::carray<int, 512> a512;

    require((int)(a1.end() - a1.begin()), 1);
    require((int)(a8.end() - a8.begin()), 8);
    require((int)(a64.end() - a64.begin()), 64);
    require((int)(a512.end() - a512.begin()), 512);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: (a + b) - b == a – large SIMD sizes");
  {
    micron::carray<int, 256> a = make_seq<int, 256>(0);
    micron::carray<int, 256> b = make_seq<int, 256>(1000);
    auto rt = (a + b) - b;
    require_true(arr_eq(rt, a));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("invariant: float (a + b) - b == a (SIMD path)");
  {
    micron::carray<float, 64> a;
    micron::carray<float, 64> b;
    for ( usize i = 0; i < 64; ++i ) {
      a[i] = static_cast<float>(i);
      b[i] = static_cast<float>(i * 10);
    }
    auto rt = (a + b) - b;
    for ( usize i = 0; i < 64; ++i )
      require(rt[i], static_cast<float>(i));
  }
  end_test_case();

  sb::print("=== ALL CARRAY TESTS PASSED ===");
  return 1;
}
