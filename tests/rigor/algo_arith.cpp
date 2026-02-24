// arithmetic_tests.cpp
// Rigorous snowball test suite for micron arithmetic algorithms
// Covers: pow, add, multiply, divide, subtract — all overload families
// Reference containers: micron::vector<T> and micron::array<T, N>

#include "../../src/algorithm/arith.hpp"
#include "../../src/array/array.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_true;
using sb::test_case;

// ── tolerance helpers ─────────────────────────────────────────────────────────

static bool
near(double a, double b, double eps = 1e-6)
{
  double diff = a - b;
  return (diff < 0 ? -diff : diff) < eps;
}

static bool
near_f(float a, float b, float eps = 1e-4f)
{
  float diff = a - b;
  return (diff < 0 ? -diff : diff) < eps;
}

// ── factory helpers ───────────────────────────────────────────────────────────

static micron::vector<int>
ivec(std::initializer_list<int> lst)
{
  return micron::vector<int>(lst);
}

static micron::vector<double>
dvec(std::initializer_list<double> lst)
{
  return micron::vector<double>(lst);
}

// =============================================================================
int
main()
{
  sb::print("=== ARITHMETIC ALGORITHM TESTS ===");

  // ══════════════════════════════════════════════════════════════════════
  // POW
  // ══════════════════════════════════════════════════════════════════════

  test_case("pow - container overload (integer base, integer exp)");
  {
    auto v = ivec({ 1, 2, 3, 4 });
    micron::pow(v, 2);
    require(v[0], 1);
    require(v[1], 4);
    require(v[2], 9);
    require(v[3], 16);
  }
  end_test_case();

  test_case("pow - container overload (double base, double exp)");
  {
    micron::vector<double> v{ 2.0, 4.0, 8.0 };
    micron::pow(v, 0.5);     // square root
    require_true(near(v[0], 1.41421356));
    require_true(near(v[1], 2.0));
    require_true(near(v[2], 2.82842712));
  }
  end_test_case();

  test_case("pow - container overload (exp = 0 → all ones)");
  {
    auto v = ivec({ 3, 7, 99, 0 });
    micron::pow(v, 0);
    for ( auto it = v.begin(); it != v.end(); ++it )
      require(*it, 1);
  }
  end_test_case();

  test_case("pow - container overload (exp = 1 → identity)");
  {
    auto v = ivec({ 5, 10, 15 });
    micron::pow(v, 1);
    require(v[0], 5);
    require(v[1], 10);
    require(v[2], 15);
  }
  end_test_case();

  test_case("pow - pointer range overload");
  {
    micron::array<int, 4> a{ 2, 3, 4, 5 };
    micron::pow(a.begin(), a.end(), 3);
    require(a[0], 8);
    require(a[1], 27);
    require(a[2], 64);
    require(a[3], 125);
  }
  end_test_case();

  test_case("pow - array container overload");
  {
    micron::array<double, 3> a{ 1.0, 2.0, 3.0 };
    micron::pow(a, 2);
    require_true(near(a[0], 1.0));
    require_true(near(a[1], 4.0));
    require_true(near(a[2], 9.0));
  }
  end_test_case();

  // ══════════════════════════════════════════════════════════════════════
  // ADD — scalar overloads
  // ══════════════════════════════════════════════════════════════════════

  test_case("add - container + scalar (int)");
  {
    auto v = ivec({ 1, 2, 3, 4, 5 });
    micron::add(v, 10);
    for ( int i = 0; i < 5; ++i )
      require(v[i], i + 11);
  }
  end_test_case();

  test_case("add - container + scalar (double)");
  {
    auto v = dvec({ 1.0, 2.0, 3.0 });
    micron::add(v, 0.5);
    require_true(near(v[0], 1.5));
    require_true(near(v[1], 2.5));
    require_true(near(v[2], 3.5));
  }
  end_test_case();

  test_case("add - container + zero is identity");
  {
    auto v = ivec({ 7, 14, 21 });
    micron::add(v, 0);
    require(v[0], 7);
    require(v[1], 14);
    require(v[2], 21);
  }
  end_test_case();

  test_case("add - container + negative scalar");
  {
    auto v = ivec({ 10, 20, 30 });
    micron::add(v, -5);
    require(v[0], 5);
    require(v[1], 15);
    require(v[2], 25);
  }
  end_test_case();

  test_case("add - pointer range + scalar");
  {
    micron::array<int, 5> a{ 0, 1, 2, 3, 4 };
    micron::add(a.begin(), a.end(), 100);
    for ( int i = 0; i < 5; ++i )
      require(a[i], i + 100);
  }
  end_test_case();

  test_case("add - array container + scalar");
  {
    micron::array<int, 3> a{ 5, 10, 15 };
    micron::add(a, 5);
    require(a[0], 10);
    require(a[1], 15);
    require(a[2], 20);
  }
  end_test_case();

  // ── add: element-wise variadic pointer overloads ──────────────────────────

  test_case("add - container + single pointer source (element-wise)");
  {
    auto v = ivec({ 1, 2, 3, 4 });
    int src[] = { 10, 20, 30, 40 };
    micron::add(v, src);
    require(v[0], 11);
    require(v[1], 22);
    require(v[2], 33);
    require(v[3], 44);
  }
  end_test_case();

  test_case("add - container + two pointer sources (element-wise)");
  {
    auto v = ivec({ 0, 0, 0, 0 });
    int a[] = { 1, 2, 3, 4 };
    int b[] = { 10, 20, 30, 40 };
    micron::add(v, a, b);
    require(v[0], 11);
    require(v[1], 22);
    require(v[2], 33);
    require(v[3], 44);
  }
  end_test_case();

  test_case("add - count+pointer + single source (element-wise)");
  {
    int dst[] = { 5, 5, 5, 5 };
    int src[] = { 1, 2, 3, 4 };
    micron::add(4, dst, src);
    require(dst[0], 6);
    require(dst[1], 7);
    require(dst[2], 8);
    require(dst[3], 9);
  }
  end_test_case();

  test_case("add - count+pointer + two sources");
  {
    int dst[] = { 0, 0, 0 };
    int a[] = { 1, 2, 3 };
    int b[] = { 4, 5, 6 };
    micron::add(3, dst, a, b);
    require(dst[0], 5);
    require(dst[1], 7);
    require(dst[2], 9);
  }
  end_test_case();

  // ══════════════════════════════════════════════════════════════════════
  // MULTIPLY — scalar overloads + product accumulator
  // ══════════════════════════════════════════════════════════════════════

  test_case("multiply - container × scalar (int)");
  {
    auto v = ivec({ 1, 2, 3, 4, 5 });
    micron::multiply(v, 3);
    int expected[] = { 3, 6, 9, 12, 15 };
    for ( int i = 0; i < 5; ++i )
      require(v[i], expected[i]);
  }
  end_test_case();

  test_case("multiply - container × 1 is identity");
  {
    auto v = ivec({ 7, 14, 21 });
    micron::multiply(v, 1);
    require(v[0], 7);
    require(v[1], 14);
    require(v[2], 21);
  }
  end_test_case();

  test_case("multiply - container × 0 zeros all elements");
  {
    auto v = ivec({ 5, 10, 15, 20 });
    micron::multiply(v, 0);
    for ( auto it = v.begin(); it != v.end(); ++it )
      require(*it, 0);
  }
  end_test_case();

  test_case("multiply - container × scalar (double)");
  {
    auto v = dvec({ 1.0, 2.0, 4.0 });
    micron::multiply(v, 2.5);
    require_true(near(v[0], 2.5));
    require_true(near(v[1], 5.0));
    require_true(near(v[2], 10.0));
  }
  end_test_case();

  test_case("multiply - pointer range × scalar");
  {
    micron::array<int, 4> a{ 2, 4, 6, 8 };
    micron::multiply(a.begin(), a.end(), 2);
    require(a[0], 4);
    require(a[1], 8);
    require(a[2], 12);
    require(a[3], 16);
  }
  end_test_case();

  test_case("multiply - array container × scalar");
  {
    micron::array<int, 3> a{ 3, 6, 9 };
    micron::multiply(a, 3);
    require(a[0], 9);
    require(a[1], 18);
    require(a[2], 27);
  }
  end_test_case();

  // ── multiply(): product accumulator ──────────────────────────────────────

  test_case("multiply(container) - product of all elements");
  {
    auto v = ivec({ 1, 2, 3, 4, 5 });
    auto p = micron::multiply(v);
    require(p, 120);
  }
  end_test_case();

  test_case("multiply(container) - single element");
  {
    auto v = ivec({ 42 });
    require(micron::multiply(v), 42);
  }
  end_test_case();

  test_case("multiply(container) - contains zero → product is zero");
  {
    auto v = ivec({ 3, 7, 0, 5 });
    require(micron::multiply(v), 0);
  }
  end_test_case();

  test_case("mul alias matches multiply");
  {
    auto v = ivec({ 2, 3, 4 });
    require(micron::mul(v), micron::multiply(v));
  }
  end_test_case();

  test_case("mul(container, y) alias");
  {
    auto v = ivec({ 2, 5, 10 });
    micron::mul(v, 2);
    require(v[0], 4);
    require(v[1], 10);
    require(v[2], 20);
  }
  end_test_case();

  // ── multiply: element-wise variadic pointer overloads ────────────────────

  test_case("multiply - container × single pointer source");
  {
    auto v = ivec({ 1, 2, 3, 4 });
    int src[] = { 10, 10, 10, 10 };
    micron::multiply(v, src);
    require(v[0], 10);
    require(v[1], 20);
    require(v[2], 30);
    require(v[3], 40);
  }
  end_test_case();

  test_case("multiply - count+pointer × single source");
  {
    int dst[] = { 2, 3, 4, 5 };
    int src[] = { 3, 3, 3, 3 };
    micron::multiply(4, dst, src);
    require(dst[0], 6);
    require(dst[1], 9);
    require(dst[2], 12);
    require(dst[3], 15);
  }
  end_test_case();

  // ══════════════════════════════════════════════════════════════════════
  // DIVIDE — scalar overloads
  // ══════════════════════════════════════════════════════════════════════

  test_case("divide - container ÷ scalar (int)");
  {
    auto v = ivec({ 10, 20, 30, 40 });
    micron::divide(v, 10);
    require(v[0], 1);
    require(v[1], 2);
    require(v[2], 3);
    require(v[3], 4);
  }
  end_test_case();

  test_case("divide - container ÷ 1 is identity");
  {
    auto v = ivec({ 7, 13, 99 });
    micron::divide(v, 1);
    require(v[0], 7);
    require(v[1], 13);
    require(v[2], 99);
  }
  end_test_case();

  test_case("divide - container ÷ scalar (double)");
  {
    auto v = dvec({ 1.0, 2.0, 4.0, 8.0 });
    micron::divide(v, 2.0);
    require_true(near(v[0], 0.5));
    require_true(near(v[1], 1.0));
    require_true(near(v[2], 2.0));
    require_true(near(v[3], 4.0));
  }
  end_test_case();

  test_case("divide - pointer range ÷ scalar");
  {
    micron::array<int, 4> a{ 8, 16, 24, 32 };
    micron::divide(a.begin(), a.end(), 8);
    require(a[0], 1);
    require(a[1], 2);
    require(a[2], 3);
    require(a[3], 4);
  }
  end_test_case();

  test_case("divide - array container ÷ scalar");
  {
    micron::array<double, 3> a{ 9.0, 6.0, 3.0 };
    micron::divide(a, 3.0);
    require_true(near(a[0], 3.0));
    require_true(near(a[1], 2.0));
    require_true(near(a[2], 1.0));
  }
  end_test_case();

  // ── divide: element-wise variadic pointer overloads ──────────────────────

  test_case("divide - container ÷ single pointer source (element-wise)");
  {
    auto v = ivec({ 10, 20, 30, 40 });
    int divisors[] = { 2, 4, 5, 8 };
    micron::divide(v, divisors);
    require(v[0], 5);
    require(v[1], 5);
    require(v[2], 6);
    require(v[3], 5);
  }
  end_test_case();

  test_case("divide - count+pointer ÷ single source");
  {
    int dst[] = { 100, 50, 25 };
    int src[] = { 5, 5, 5 };
    micron::divide(3, dst, src);
    require(dst[0], 20);
    require(dst[1], 10);
    require(dst[2], 5);
  }
  end_test_case();

  // ══════════════════════════════════════════════════════════════════════
  // SUBTRACT — scalar overloads
  // ══════════════════════════════════════════════════════════════════════

  test_case("subtract - container − scalar (int)");
  {
    auto v = ivec({ 10, 20, 30, 40, 50 });
    micron::subtract(v, 5);
    for ( int i = 0; i < 5; ++i )
      require(v[i], (i + 1) * 10 - 5);
  }
  end_test_case();

  test_case("subtract - container − 0 is identity");
  {
    auto v = ivec({ 3, 6, 9 });
    micron::subtract(v, 0);
    require(v[0], 3);
    require(v[1], 6);
    require(v[2], 9);
  }
  end_test_case();

  test_case("subtract - container − scalar (double)");
  {
    auto v = dvec({ 5.5, 3.3, 1.1 });
    micron::subtract(v, 0.5);
    require_true(near(v[0], 5.0));
    require_true(near(v[1], 2.8));
    require_true(near(v[2], 0.6));
  }
  end_test_case();

  test_case("subtract - pointer range − scalar");
  {
    micron::array<int, 4> a{ 100, 200, 300, 400 };
    micron::subtract(a.begin(), a.end(), 50);
    require(a[0], 50);
    require(a[1], 150);
    require(a[2], 250);
    require(a[3], 350);
  }
  end_test_case();

  test_case("subtract - array container − scalar");
  {
    micron::array<int, 3> a{ 9, 6, 3 };
    micron::subtract(a, 3);
    require(a[0], 6);
    require(a[1], 3);
    require(a[2], 0);
  }
  end_test_case();

  // ── subtract: element-wise variadic pointer overloads ────────────────────

  test_case("subtract - container − single pointer source (element-wise)");
  {
    auto v = ivec({ 10, 20, 30, 40 });
    int src[] = { 1, 2, 3, 4 };
    micron::subtract(v, src);
    require(v[0], 9);
    require(v[1], 18);
    require(v[2], 27);
    require(v[3], 36);
  }
  end_test_case();

  test_case("subtract - container − two pointer sources (element-wise)");
  {
    auto v = ivec({ 100, 100, 100, 100 });
    int a[] = { 10, 20, 30, 40 };
    int b[] = { 5, 10, 15, 20 };
    micron::subtract(v, a, b);
    require(v[0], 85);
    require(v[1], 70);
    require(v[2], 55);
    require(v[3], 40);
  }
  end_test_case();

  test_case("subtract - count+pointer − single source");
  {
    int dst[] = { 50, 40, 30 };
    int src[] = { 5, 5, 5 };
    micron::subtract(3, dst, src);
    require(dst[0], 45);
    require(dst[1], 35);
    require(dst[2], 25);
  }
  end_test_case();

  test_case("subtract - count+pointer − two sources");
  {
    int dst[] = { 100, 100, 100 };
    int a[] = { 10, 20, 30 };
    int b[] = { 1, 2, 3 };
    micron::subtract(3, dst, a, b);
    require(dst[0], 89);
    require(dst[1], 78);
    require(dst[2], 67);
  }
  end_test_case();

  // ══════════════════════════════════════════════════════════════════════
  // OPERATION INVERSES AND COMPOSITIONS
  // ══════════════════════════════════════════════════════════════════════

  test_case("add then subtract by same scalar → identity");
  {
    auto v = ivec({ 3, 7, 11, 15 });
    micron::add(v, 42);
    micron::subtract(v, 42);
    require(v[0], 3);
    require(v[1], 7);
    require(v[2], 11);
    require(v[3], 15);
  }
  end_test_case();

  test_case("multiply then divide by same scalar → identity (double)");
  {
    auto v = dvec({ 1.0, 2.0, 3.0, 4.0 });
    micron::multiply(v, 7.0);
    micron::divide(v, 7.0);
    for ( int i = 0; i < 4; ++i )
      require_true(near(v[i], static_cast<double>(i + 1)));
  }
  end_test_case();

  test_case("pow then divide recovers original (perfect square roots)");
  {
    auto v = dvec({ 1.0, 4.0, 9.0, 16.0, 25.0 });
    micron::pow(v, 0.5);     // sqrt
    micron::pow(v, 2.0);     // square again
    double expected[] = { 1.0, 4.0, 9.0, 16.0, 25.0 };
    for ( int i = 0; i < 5; ++i )
      require_true(near(v[i], expected[i], 1e-5));
  }
  end_test_case();

  test_case("chained scalar ops on array");
  {
    micron::array<int, 4> a{ 2, 4, 6, 8 };
    micron::add(a, 2);          // { 4,  6,  8, 10 }
    micron::multiply(a, 3);     // {12, 18, 24, 30 }
    micron::subtract(a, 6);     // { 6, 12, 18, 24 }
    micron::divide(a, 6);       // { 1,  2,  3,  4 }
    require(a[0], 1);
    require(a[1], 2);
    require(a[2], 3);
    require(a[3], 4);
  }
  end_test_case();

  test_case("multiply(accumulator) after multiply(scalar) is consistent");
  {
    auto v = ivec({ 1, 2, 3, 4, 5 });
    micron::multiply(v, 2);           // {2,4,6,8,10}
    auto p = micron::multiply(v);     // 2*4*6*8*10 = 3840
    require(p, 3840);
  }
  end_test_case();

  test_case("stress: add + multiply + subtract on large vector");
  {
    micron::vector<int> v(500, 1);
    micron::add(v, 9);           // all 10
    micron::multiply(v, 5);      // all 50
    micron::subtract(v, 25);     // all 25
    micron::divide(v, 5);        // all 5
    for ( auto it = v.begin(); it != v.end(); ++it ) {
      require(*it == 5);
    }
  }
  end_test_case();

  test_case("stress: element-wise ops on large arrays via raw pointers");
  {
    constexpr size_t N = 256;
    micron::array<int, N> dst{};
    micron::array<int, N> src{};

    // fill dst with 1, src with 2
    for ( size_t i = 0; i < N; ++i ) {
      dst[i] = 1;
      src[i] = 2;
    }

    micron::add(N, dst.begin(), src.begin());          // all 3
    micron::multiply(N, dst.begin(), src.begin());     // all 6
    micron::subtract(N, dst.begin(), src.begin());     // all 4
    micron::divide(N, dst.begin(), src.begin());       // all 2

    for ( size_t i = 0; i < N; ++i )
      require(dst[i], 2);
  }
  end_test_case();

  sb::print("=== ALL ARITHMETIC TESTS PASSED ===");
  return 1;
}
