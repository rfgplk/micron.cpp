// rigor_algo_arith.cpp — exhaustive snowball suite for src/algorithm/arith.hpp
//
// Coverage:
//   pow            (container/pointer × float/int exponent)
//   add            (container+scalar, pointer+scalar, variadic, count+ptr+variadic)
//   multiply       (container+scalar, pointer+scalar, variadic, count+variadic, product)
//   mul            (alias of multiply)
//   divide         (container+scalar, pointer+scalar, variadic, count+variadic)
//   subtract       (container+scalar, pointer+scalar, variadic, count+variadic)
//
// NOTE on variadic semantics: add/multiply/divide/subtract with variadic
// pointers compute the SUM of the variadic operands first, then apply
// the operation. So add(c, a, b) does c[i] = c[i] + (a[i] + b[i]).

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/arith.hpp"
#include "../../src/algorithm/find.hpp"

#include "../support/algo_rigor.hpp"

using namespace mtest::rigor;
using mtest::prng;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== ALGO/ARITH RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // pow
  // ════════════════════════════════════════════════════════════════════

  test_case("pow[container,int^0] -> all ones");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i + 1;
    micron::pow(v, 0);
    for ( int i = 0; i < 8; ++i ) require(v[i], 1);
  }
  end_test_case();

  test_case("pow[container,int^1] is identity");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::pow(v, 1);
    for ( int i = 0; i < 8; ++i ) require(v[i], d[i]);
  }
  end_test_case();

  test_case("pow[container,int^2] squares");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    micron::pow(v, 2);
    int expected[5] = { 1, 4, 9, 16, 25 };
    for ( int i = 0; i < 5; ++i ) require(v[i], expected[i]);
  }
  end_test_case();

  test_case("pow[container,int^3] cubes");
  {
    micron::array<int, 4> a;
    for ( int i = 0; i < 4; ++i ) a[i] = i + 1;
    micron::pow(a, 3);
    int expected[4] = { 1, 8, 27, 64 };
    for ( int i = 0; i < 4; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  test_case("pow[container,double^0.5] sqrt-like");
  {
    micron::vector<f64> v(3, 0.0);
    v[0] = 4.0;
    v[1] = 9.0;
    v[2] = 16.0;
    micron::pow(v, 0.5);
    require_true(near<f64>(v[0], 2.0, 1e-4));
    require_true(near<f64>(v[1], 3.0, 1e-4));
    require_true(near<f64>(v[2], 4.0, 1e-4));
  }
  end_test_case();

  test_case("pow[ptr,int] adversarial sizes");
  {
    for ( usize n : kAdversarialSizes ) {
      if ( n == 0 ) continue;
      int a[1024];
      pat_all_equal(a, n, 2);
      micron::pow(a, a + n, 4);
      for ( usize i = 0; i < n; ++i ) require(a[i], 16);
    }
  }
  end_test_case();

  test_case("pow[ptr,int] all-equal pattern with exp=2");
  {
    int a[64];
    pat_all_equal(a, 64, 3);
    micron::pow(a, a + 64, 2);
    for ( int i = 0; i < 64; ++i ) require(a[i], 9);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // add (scalar / ptr / variadic / count)
  // ════════════════════════════════════════════════════════════════════

  test_case("add[container,scalar]");
  {
    micron::vector<int> v(5, 10);
    micron::add(v, 5);
    for ( int i = 0; i < 5; ++i ) require(v[i], 15);
  }
  end_test_case();

  test_case("add[container,scalar=0] is identity");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::add(v, 0);
    for ( int i = 0; i < 8; ++i ) require(v[i], d[i]);
  }
  end_test_case();

  test_case("add[ptr,scalar]");
  {
    int a[16];
    pat_sorted(a, 16);
    micron::add(a, a + 16, 100);
    for ( int i = 0; i < 16; ++i ) require(a[i], 100 + i);
  }
  end_test_case();

  test_case("add[container, ptr, ptr] sums variadic operands");
  {
    micron::vector<int> dst(5, 1);
    int a[5] = { 10, 20, 30, 40, 50 };
    int b[5] = { 1, 2, 3, 4, 5 };
    // dst[i] = dst[i] + (a[i] + b[i])
    micron::add(dst, a, b);
    require(dst[0], 1 + 11);
    require(dst[1], 1 + 22);
    require(dst[2], 1 + 33);
    require(dst[3], 1 + 44);
    require(dst[4], 1 + 55);
  }
  end_test_case();

  test_case("add[count, ptr, ptr] pointer-based variadic");
  {
    int dst[4] = { 0, 0, 0, 0 };
    int a[4] = { 1, 2, 3, 4 };
    int b[4] = { 10, 20, 30, 40 };
    micron::add(usize(4), dst, a, b);
    require(dst[0], 11);
    require(dst[1], 22);
    require(dst[2], 33);
    require(dst[3], 44);
  }
  end_test_case();

  property_test(
      "add[ptr,scalar] inverted by subtract (10k random)",
      [](u32 raw_n, u32 raw_y) {
        usize n = (raw_n & 0x1f) + 1;
        int y = static_cast<int>(raw_y & 0xffff);
        int buf[32];
        int copy[32];
        prng rng(raw_n + 41);
        pat_random_small(buf, n, rng, -10000, 10000);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::add(buf, buf + n, y);
        micron::subtract(buf, buf + n, y);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // multiply / mul
  // ════════════════════════════════════════════════════════════════════

  test_case("multiply[container] returns product of all elements");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;      // 1*2*3*4*5 = 120
    auto p = micron::multiply(v);
    require(p, 120);
  }
  end_test_case();

  test_case("multiply[container,scalar]");
  {
    micron::vector<int> v(5, 3);
    micron::multiply(v, 4);
    for ( int i = 0; i < 5; ++i ) require(v[i], 12);
  }
  end_test_case();

  test_case("multiply[container,scalar=0] zeros");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::multiply(v, 0);
    for ( int i = 0; i < 8; ++i ) require(v[i], 0);
  }
  end_test_case();

  test_case("multiply[container,scalar=1] identity");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::multiply(v, 1);
    for ( int i = 0; i < 8; ++i ) require(v[i], d[i]);
  }
  end_test_case();

  test_case("multiply[ptr,scalar]");
  {
    int a[16];
    for ( int i = 0; i < 16; ++i ) a[i] = i + 1;
    micron::multiply(a, a + 16, 3);
    for ( int i = 0; i < 16; ++i ) require(a[i], (i + 1) * 3);
  }
  end_test_case();

  test_case("mul alias matches multiply");
  {
    micron::vector<int> a(5, 0);
    micron::vector<int> b(5, 0);
    for ( int i = 0; i < 5; ++i ) {
      a[i] = i + 1;
      b[i] = i + 1;
    }
    micron::multiply(a, 7);
    micron::mul(b, 7);
    for ( int i = 0; i < 5; ++i ) require(a[i], b[i]);
  }
  end_test_case();

  test_case("multiply[container, ptr, ptr] uses sum-of-operands");
  {
    micron::vector<int> dst(4, 2);
    int a[4] = { 1, 2, 3, 4 };
    int b[4] = { 10, 20, 30, 40 };
    // dst[i] = dst[i] * (a[i] + b[i]) = 2 * (a[i] + b[i])
    micron::multiply(dst, a, b);
    require(dst[0], 2 * 11);
    require(dst[1], 2 * 22);
    require(dst[2], 2 * 33);
    require(dst[3], 2 * 44);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // divide
  // ════════════════════════════════════════════════════════════════════

  test_case("divide[container,scalar=1] identity");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = (i + 1) * 7;
    micron::divide(v, 1);
    for ( int i = 0; i < 5; ++i ) require(v[i], (i + 1) * 7);
  }
  end_test_case();

  test_case("divide[container,scalar=2] halves");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = (i + 1) * 2;
    micron::divide(v, 2);
    for ( int i = 0; i < 5; ++i ) require(v[i], i + 1);
  }
  end_test_case();

  test_case("divide[ptr,scalar] float division");
  {
    f64 a[4] = { 10.0, 20.0, 30.0, 40.0 };
    micron::divide(a, a + 4, 4.0);
    require_true(near<f64>(a[0], 2.5));
    require_true(near<f64>(a[1], 5.0));
    require_true(near<f64>(a[2], 7.5));
    require_true(near<f64>(a[3], 10.0));
  }
  end_test_case();

  test_case("divide[container, ptr, ptr] variadic");
  {
    micron::vector<int> dst(4, 100);
    int a[4] = { 1, 2, 3, 4 };
    int b[4] = { 1, 2, 3, 4 };
    // dst[i] = dst[i] / (a[i] + b[i]) = 100 / (2i)
    micron::divide(dst, a, b);
    require(dst[0], 50);      // 100 / 2
    require(dst[1], 25);      // 100 / 4
    require(dst[2], 16);      // 100 / 6 = 16 int-trunc
    require(dst[3], 12);      // 100 / 8
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // subtract
  // ════════════════════════════════════════════════════════════════════

  test_case("subtract[container,scalar]");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = (i + 1) * 10;      // 10..50
    micron::subtract(v, 5);
    for ( int i = 0; i < 5; ++i ) require(v[i], (i + 1) * 10 - 5);
  }
  end_test_case();

  test_case("subtract[container,scalar=0] identity");
  {
    micron::vector<int> v(8, 7);
    micron::subtract(v, 0);
    for ( int i = 0; i < 8; ++i ) require(v[i], 7);
  }
  end_test_case();

  test_case("subtract[ptr,scalar]");
  {
    int a[16];
    pat_sorted(a, 16, 100);
    micron::subtract(a, a + 16, 50);
    for ( int i = 0; i < 16; ++i ) require(a[i], 50 + i);
  }
  end_test_case();

  test_case("subtract[container, ptr, ptr] variadic");
  {
    micron::vector<int> dst(4, 1000);
    int a[4] = { 10, 20, 30, 40 };
    int b[4] = { 1, 2, 3, 4 };
    // dst[i] = dst[i] - (a[i] + b[i])
    micron::subtract(dst, a, b);
    require(dst[0], 1000 - 11);
    require(dst[1], 1000 - 22);
    require(dst[2], 1000 - 33);
    require(dst[3], 1000 - 44);
  }
  end_test_case();

  property_test(
      "subtract[ptr,scalar] inverted by add (10k random)",
      [](u32 raw_n, u32 raw_y) {
        usize n = (raw_n & 0x1f) + 1;
        int y = static_cast<int>(raw_y & 0xffff);
        int buf[32];
        int copy[32];
        prng rng(raw_n + 43);
        pat_random_small(buf, n, rng, -10000, 10000);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::subtract(buf, buf + n, y);
        micron::add(buf, buf + n, y);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  property_test(
      "multiply[ptr,scalar] then divide by same scalar is identity for divisible inputs (10k)",
      [](u32 raw_n, u32 raw_y) {
        usize n = (raw_n & 0xf) + 1;
        int y = static_cast<int>((raw_y & 0x7) + 1);      // 1..8 to avoid div-by-zero
        int buf[16];
        int copy[16];
        prng rng(raw_n + 47);
        pat_random_small(buf, n, rng, -100, 100);
        for ( usize i = 0; i < n; ++i ) {
          buf[i] = (buf[i] / y) * y;      // make divisible
          copy[i] = buf[i];
        }
        micron::multiply(buf, buf + n, y);
        micron::divide(buf, buf + n, y);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // Combinations / stress
  // ════════════════════════════════════════════════════════════════════

  test_case("add/subtract chain matches naive");
  {
    micron::vector<int> v(20, 5);
    micron::add(v, 10);          // 15
    micron::multiply(v, 2);      // 30
    micron::subtract(v, 5);      // 25
    micron::divide(v, 5);        // 5
    for ( int i = 0; i < 20; ++i ) require(v[i], 5);
  }
  end_test_case();

  test_case("pow with adversarial all-equal patterns");
  {
    int sizes[] = { 1, 8, 64, 256 };
    for ( int s = 0; s < 4; ++s ) {
      int n = sizes[s];
      int a[256];
      pat_all_equal(a, static_cast<usize>(n), 5);
      micron::pow(a, a + n, 2);
      for ( int i = 0; i < n; ++i ) require(a[i], 25);
    }
  }
  end_test_case();

  property_test(
      "add[container,scalar] vs element-wise loop (10k)",
      [](u32 raw_n, u32 raw_y) {
        usize n = (raw_n & 0x1f) + 1;
        int y = static_cast<int>(raw_y & 0xff);
        micron::vector<int> v(n, 0);
        int buf[32];
        prng rng(raw_n + 53);
        pat_random_small(buf, n, rng, -1000, 1000);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        micron::add(v, y);
        for ( usize i = 0; i < n; ++i ) require(v[i], buf[i] + y);
      },
      10000);

  sb::print("=== ALGO/ARITH RIGOR SUITE PASSED ===");
  return 1;
}
