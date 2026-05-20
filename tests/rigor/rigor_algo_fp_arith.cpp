// rigor_algo_fp_arith.cpp — snowball suite for src/algorithm/fparith.hpp
//
// Coverage:
//   add_c / subtract_c / multiply_c / divide_c / pow_c   (curried)
//   safe_divide / safe_divide_c                          (option-returning)
//   add_zip / subtract_zip / multiply_zip / divide_zip   (element-wise)
//   inner_product                                        (dot product)
//   safe_inner_product
//   negate / abs                                         (element-wise)
//
// All under micron::fp namespace.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/fparith.hpp"

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
  sb::print("=== ALGO/FP-ARITH RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // Curried scalar ops
  // ════════════════════════════════════════════════════════════════════

  test_case("add_c partial application");
  {
    auto plus5 = micron::fp::add_c(5);
    micron::vector<int> v(4, 10);
    auto out = plus5(v);
    for ( int i = 0; i < 4; ++i ) require(out[i], 15);
  }
  end_test_case();

  test_case("subtract_c partial application");
  {
    auto minus3 = micron::fp::subtract_c(3);
    micron::vector<int> v(4, 10);
    auto out = minus3(v);
    for ( int i = 0; i < 4; ++i ) require(out[i], 7);
  }
  end_test_case();

  test_case("multiply_c partial application");
  {
    auto times2 = micron::fp::multiply_c(2);
    micron::vector<int> v(4, 5);
    auto out = times2(v);
    for ( int i = 0; i < 4; ++i ) require(out[i], 10);
  }
  end_test_case();

  test_case("divide_c partial application");
  {
    auto half = micron::fp::divide_c(2);
    micron::vector<int> v(4, 10);
    auto out = half(v);
    for ( int i = 0; i < 4; ++i ) require(out[i], 5);
  }
  end_test_case();

  test_case("pow_c partial application");
  {
    auto sq = micron::fp::pow_c(2);
    micron::vector<int> v(4, 0);
    for ( int i = 0; i < 4; ++i ) v[i] = i + 1;
    auto out = sq(v);
    for ( int i = 0; i < 4; ++i ) require(out[i], (i + 1) * (i + 1));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // safe_divide (returns option on zero)
  // ════════════════════════════════════════════════════════════════════

  test_case("safe_divide with nonzero divisor returns success");
  {
    micron::vector<int> v(4, 20);
    auto opt = micron::fp::safe_divide(v, 4);
    require_true(opt.is_first());
    if ( opt.is_first() ) {
      auto out = opt.template cast<micron::vector<int>>();
      for ( int i = 0; i < 4; ++i ) require(out[i], 5);
    }
  }
  end_test_case();

  test_case("safe_divide with zero divisor returns error");
  {
    micron::vector<int> v(4, 20);
    auto opt = micron::fp::safe_divide(v, 0);
    require_true(opt.is_second());
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Zip-based element-wise
  // ════════════════════════════════════════════════════════════════════

  test_case("add_zip element-wise");
  {
    micron::vector<int> a(4, 0);
    micron::vector<int> b(4, 0);
    for ( int i = 0; i < 4; ++i ) {
      a[i] = i + 1;
      b[i] = 10;
    }
    auto out = micron::fp::add_zip(a, b);
    for ( int i = 0; i < 4; ++i ) require(out[i], i + 11);
  }
  end_test_case();

  test_case("subtract_zip element-wise");
  {
    micron::vector<int> a(4, 0);
    micron::vector<int> b(4, 0);
    for ( int i = 0; i < 4; ++i ) {
      a[i] = (i + 1) * 10;
      b[i] = i + 1;
    }
    auto out = micron::fp::subtract_zip(a, b);
    for ( int i = 0; i < 4; ++i ) require(out[i], (i + 1) * 9);
  }
  end_test_case();

  test_case("multiply_zip element-wise");
  {
    micron::vector<int> a(4, 0);
    micron::vector<int> b(4, 0);
    for ( int i = 0; i < 4; ++i ) {
      a[i] = i + 1;
      b[i] = (i + 1) * 2;
    }
    auto out = micron::fp::multiply_zip(a, b);
    for ( int i = 0; i < 4; ++i ) require(out[i], (i + 1) * (i + 1) * 2);
  }
  end_test_case();

  test_case("divide_zip returns option (no zeros)");
  {
    micron::vector<int> a(4, 0);
    micron::vector<int> b(4, 0);
    for ( int i = 0; i < 4; ++i ) {
      a[i] = (i + 1) * 6;
      b[i] = i + 1;
    }
    auto opt = micron::fp::divide_zip(a, b);
    require_true(opt.is_first());
    if ( opt.is_first() ) {
      auto out = opt.template cast<micron::vector<int>>();
      for ( int i = 0; i < 4; ++i ) require(out[i], 6);
    }
  }
  end_test_case();

  test_case("divide_zip with zero divisor returns error");
  {
    micron::vector<int> a(4, 0);
    micron::vector<int> b(4, 0);
    for ( int i = 0; i < 4; ++i ) {
      a[i] = i + 1;
      b[i] = (i == 2) ? 0 : 1;
    }
    auto opt = micron::fp::divide_zip(a, b);
    require_true(opt.is_second());
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // inner_product
  // ════════════════════════════════════════════════════════════════════

  test_case("inner_product computes dot product");
  {
    micron::vector<int> a(4, 0);
    micron::vector<int> b(4, 0);
    int da[4] = { 1, 2, 3, 4 };
    int db[4] = { 10, 20, 30, 40 };
    for ( int i = 0; i < 4; ++i ) {
      a[i] = da[i];
      b[i] = db[i];
    }
    auto r = micron::fp::inner_product(a, b, 0);
    require(r, 1 * 10 + 2 * 20 + 3 * 30 + 4 * 40);      // 300
  }
  end_test_case();

  test_case("inner_product with init=100");
  {
    micron::vector<int> a(3, 0);
    micron::vector<int> b(3, 0);
    for ( int i = 0; i < 3; ++i ) {
      a[i] = i + 1;
      b[i] = i + 1;
    }
    auto r = micron::fp::inner_product(a, b, 100);
    require(r, 100 + 1 + 4 + 9);      // 114
  }
  end_test_case();

  test_case("inner_product zero vectors == init");
  {
    micron::vector<int> a(5, 0);
    micron::vector<int> b(5, 0);
    auto r = micron::fp::inner_product(a, b, 7);
    require(r, 7);
  }
  end_test_case();

  test_case("inner_product with custom add and mul");
  {
    micron::vector<int> a(3, 0);
    micron::vector<int> b(3, 0);
    for ( int i = 0; i < 3; ++i ) {
      a[i] = i + 1;
      b[i] = i + 1;
    }
    auto r = micron::fp::inner_product(a, b, 0, [](int x, int y) { return x + y; }, [](int x, int y) { return x * y; });
    require(r, 14);      // 1 + 4 + 9
  }
  end_test_case();

  test_case("safe_inner_product on equal lengths returns success");
  {
    micron::vector<int> a(3, 0);
    micron::vector<int> b(3, 0);
    for ( int i = 0; i < 3; ++i ) {
      a[i] = i + 1;
      b[i] = (i + 1) * 2;
    }
    auto opt = micron::fp::safe_inner_product(a, b, 0);
    require_true(opt.is_first());
    if ( opt.is_first() ) require(opt.template cast<int>(), 1 * 2 + 2 * 4 + 3 * 6);
  }
  end_test_case();

  test_case("safe_inner_product on unequal lengths returns error");
  {
    micron::vector<int> a(3, 0);
    micron::vector<int> b(5, 0);
    auto opt = micron::fp::safe_inner_product(a, b, 0);
    require_true(opt.is_second());
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // negate / abs
  // ════════════════════════════════════════════════════════════════════

  test_case("negate flips signs");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { -3, -1, 0, 2, 4 };
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    auto out = micron::fp::negate(v);
    int expected[5] = { 3, 1, 0, -2, -4 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("abs makes positive");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { -3, -1, 0, 2, 4 };
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    auto out = micron::fp::abs(v);
    int expected[5] = { 3, 1, 0, 2, 4 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("negate twice is identity");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i - 4;
    auto orig = v;
    auto a = micron::fp::negate(v);
    auto b = micron::fp::negate(a);
    for ( int i = 0; i < 8; ++i ) require(b[i], orig[i]);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Option-lifted versions
  // ════════════════════════════════════════════════════════════════════

  test_case("negate on option<C> with success");
  {
    micron::vector<int> v(3, 0);
    v[0] = 1;
    v[1] = -2;
    v[2] = 3;
    using OptC = micron::option<micron::vector<int>, micron::fp::empty_container_error>;
    OptC opt{ v };
    auto out = micron::fp::negate(opt);
    require_true(out.is_first());
    if ( out.is_first() ) {
      auto r = out.template cast<micron::vector<int>>();
      require(r[0], -1);
      require(r[1], 2);
      require(r[2], -3);
    }
  }
  end_test_case();

  test_case("negate on option<C> with error short-circuits");
  {
    using OptC = micron::option<micron::vector<int>, micron::fp::empty_container_error>;
    OptC opt{ micron::fp::empty_container_error{} };
    auto out = micron::fp::negate(opt);
    require_true(out.is_second());
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Property tests
  // ════════════════════════════════════════════════════════════════════

  property_test(
      "inner_product matches naive (10k)",
      [](u32 raw_n) {
        usize n = (raw_n & 0xf) + 1;
        micron::vector<int> a(n, 0);
        micron::vector<int> b(n, 0);
        prng rng(raw_n + 131);
        int bufa[16];
        int bufb[16];
        pat_random_small(bufa, n, rng, -100, 100);
        pat_random_small(bufb, n, rng, -100, 100);
        for ( usize i = 0; i < n; ++i ) {
          a[i] = bufa[i];
          b[i] = bufb[i];
        }
        auto actual = micron::fp::inner_product(a, b, 0);
        int expected = 0;
        for ( usize i = 0; i < n; ++i ) expected += bufa[i] * bufb[i];
        require(actual, expected);
      },
      10000);

  property_test(
      "add_c then subtract_c is identity (10k)",
      [](u32 raw_n, u32 raw_y) {
        usize n = (raw_n & 0x1f) + 1;
        int y = static_cast<int>(raw_y & 0xff);
        micron::vector<int> v(n, 0);
        prng rng(raw_n + 137);
        int buf[32];
        pat_random_small(buf, n, rng, -1000, 1000);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        auto add = micron::fp::add_c(y);
        auto sub = micron::fp::subtract_c(y);
        auto out = sub(add(v));
        for ( usize i = 0; i < n; ++i ) require(out[i], buf[i]);
      },
      10000);

  sb::print("=== ALGO/FP-ARITH RIGOR SUITE PASSED ===");
  return 1;
}
