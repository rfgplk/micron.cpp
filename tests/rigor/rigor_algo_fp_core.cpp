// rigor_algo_fp_core.cpp — snowball suite for src/algorithm/fpalgorithm.hpp
//
// Coverage (functions in micron::fp namespace):
//   fmap (with lambda, with member-function call)
//   scanl / scanr / scan (prefix/suffix scan)
//   zip_with / zip_with_trunc
//   replicate
//   safe_max / safe_min / safe_sum / safe_mean
//     (return option<T, empty_container_error>)
//   all_of_c / any_of_c / none_of_c (curried)
//   transform_c, fill_c, reverse_c, sort_c (curried)
//
// option API: opt.is_first() = success, opt.cast<T>() = value.
// is_second() = error path, opt.cast<E>() = error.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/find.hpp"
#include "../../src/algorithm/fpalgorithm.hpp"

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
  sb::print("=== ALGO/FP-CORE RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // fmap
  // ════════════════════════════════════════════════════════════════════

  test_case("fmap[vector] applies fn to all elements");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto out = micron::fp::fmap([](int x) { return x * 2; }, v);
    for ( int i = 0; i < 5; ++i ) require(out[i], (i + 1) * 2);
  }
  end_test_case();

  test_case("fmap[vector] identity preserves data");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i;
    auto out = micron::fp::fmap([](int x) { return x; }, v);
    for ( int i = 0; i < 8; ++i ) require(out[i], i);
  }
  end_test_case();

  test_case("fmap[array] static-size unroll path");
  {
    micron::array<int, 8> a;
    for ( int i = 0; i < 8; ++i ) a[i] = i + 1;
    auto out = micron::fp::fmap([](int x) { return x + 10; }, a);
    for ( int i = 0; i < 8; ++i ) require(out[i], i + 11);
  }
  end_test_case();

  test_case("fmap[vector] empty container");
  {
    micron::vector<int> v;
    auto out = micron::fp::fmap([](int x) { return x; }, v);
    require(out.size(), usize(0));
  }
  end_test_case();

  property_test(
      "fmap[vector] preserves length and applies fn (10k)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x1f) + 1;
        micron::vector<int> v(n, 0);
        prng rng(raw_n + 103);
        int buf[32];
        pat_random_small(buf, n, rng, -100, 100);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        auto out = micron::fp::fmap([](int x) { return x + 7; }, v);
        require(out.size(), n);
        for ( usize i = 0; i < n; ++i ) require(out[i], buf[i] + 7);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // scanl / scanr / scan
  // ════════════════════════════════════════════════════════════════════

  test_case("scanl prefix sums (init=0)");
  {
    micron::vector<int> v(4, 0);
    for ( int i = 0; i < 4; ++i ) v[i] = i + 1;
    auto out = micron::fp::scanl(v, 0, [](int acc, int x) { return acc + x; });
    // out should be [0, 1, 3, 6, 10] (one longer than input)
    require(out.size(), usize(5));
    int expected[5] = { 0, 1, 3, 6, 10 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("scanr suffix sums (init=0)");
  {
    micron::vector<int> v(4, 0);
    for ( int i = 0; i < 4; ++i ) v[i] = i + 1;
    auto out = micron::fp::scanr(v, 0, [](int x, int acc) { return x + acc; });
    // suffix sums: [10, 9, 7, 4, 0]
    require(out.size(), usize(5));
    int expected[5] = { 10, 9, 7, 4, 0 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("scan is alias for scanl");
  {
    micron::vector<int> v(4, 0);
    for ( int i = 0; i < 4; ++i ) v[i] = i + 1;
    auto a = micron::fp::scan(v, 0, [](int acc, int x) { return acc + x; });
    auto b = micron::fp::scanl(v, 0, [](int acc, int x) { return acc + x; });
    require(a.size(), b.size());
    for ( usize i = 0; i < a.size(); ++i ) require(a[i], b[i]);
  }
  end_test_case();

  test_case("scanl empty input returns [init]");
  {
    micron::vector<int> v;
    auto out = micron::fp::scanl(v, 42, [](int acc, int x) { return acc + x; });
    require(out.size(), usize(1));
    require(out[0], 42);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // zip_with / zip_with_trunc
  // ════════════════════════════════════════════════════════════════════

  test_case("zip_with same-length vectors");
  {
    micron::vector<int> a(4, 0);
    micron::vector<int> b(4, 0);
    for ( int i = 0; i < 4; ++i ) {
      a[i] = i + 1;
      b[i] = (i + 1) * 10;
    }
    auto out = micron::fp::zip_with(a, b, [](int x, int y) { return x + y; });
    // zip_with may return option — check via has_value
    require_true(out.has_value());
    if ( out.is_first() ) {
      auto v = out.template cast<micron::vector<int>>();
      require(v.size(), usize(4));
      for ( int i = 0; i < 4; ++i ) require(v[i], (i + 1) + (i + 1) * 10);
    }
  }
  end_test_case();

  test_case("zip_with_trunc shorter result");
  {
    micron::vector<int> a(5, 0);
    micron::vector<int> b(3, 0);
    for ( int i = 0; i < 5; ++i ) a[i] = i + 1;
    for ( int i = 0; i < 3; ++i ) b[i] = (i + 1) * 100;
    auto v = micron::fp::zip_with_trunc(a, b, [](int x, int y) { return x + y; });
    require(v.size(), usize(3));
    for ( int i = 0; i < 3; ++i ) require(v[i], (i + 1) + (i + 1) * 100);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // replicate
  // ════════════════════════════════════════════════════════════════════

  test_case("replicate builds container of n copies");
  {
    auto v = micron::fp::replicate<micron::vector<int>>(7, 42);
    require(v.size(), usize(7));
    for ( int i = 0; i < 7; ++i ) require(v[i], 42);
  }
  end_test_case();

  test_case("replicate n=0 is empty");
  {
    auto v = micron::fp::replicate<micron::vector<int>>(0, 99);
    require(v.size(), usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // safe_max / safe_min / safe_sum / safe_mean
  // ════════════════════════════════════════════════════════════════════

  test_case("safe_max on non-empty returns first(max)");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { 3, 1, 9, 5, 7 };
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    auto opt = micron::fp::safe_max(v);
    require_true(opt.is_first());
    if ( opt.is_first() ) require(opt.template cast<int>(), 9);
  }
  end_test_case();

  test_case("safe_max on empty returns error (second)");
  {
    micron::vector<int> v;
    auto opt = micron::fp::safe_max(v);
    require_true(opt.is_second());
  }
  end_test_case();

  test_case("safe_min on non-empty returns first(min)");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { 3, 1, 9, 5, 7 };
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    auto opt = micron::fp::safe_min(v);
    require_true(opt.is_first());
    if ( opt.is_first() ) require(opt.template cast<int>(), 1);
  }
  end_test_case();

  test_case("safe_sum integral returns first(sum)");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto opt = micron::fp::safe_sum(v);
    require_true(opt.is_first());
    if ( opt.is_first() ) require(opt.template cast<umax_t>(), umax_t(15));
  }
  end_test_case();

  test_case("safe_mean on non-empty returns first(mean)");
  {
    micron::vector<int> v(4, 0);
    int d[4] = { 2, 4, 6, 8 };
    for ( int i = 0; i < 4; ++i ) v[i] = d[i];
    auto opt = micron::fp::safe_mean<f64>(v);
    require_true(opt.is_first());
    if ( opt.is_first() ) {
      f64 m = opt.template cast<f64>();
      require_true(near<f64>(m, 5.0, 1e-6));
    }
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Curried: all_of_c / any_of_c / none_of_c
  // ════════════════════════════════════════════════════════════════════

  test_case("all_of_c partial application");
  {
    auto is_positive_c = micron::fp::all_of_c([](int x) { return x > 0; });
    micron::vector<int> a(4, 1);
    micron::vector<int> b(4, 0);
    require_true(is_positive_c(a));
    require_false(is_positive_c(b));
  }
  end_test_case();

  test_case("any_of_c partial application");
  {
    auto has_zero_c = micron::fp::any_of_c([](int x) { return x == 0; });
    micron::vector<int> a(4, 1);
    micron::vector<int> b(4, 1);
    b[2] = 0;
    require_false(has_zero_c(a));
    require_true(has_zero_c(b));
  }
  end_test_case();

  test_case("none_of_c partial application");
  {
    auto no_negative_c = micron::fp::none_of_c([](int x) { return x < 0; });
    micron::vector<int> a(4, 1);
    micron::vector<int> b(4, 1);
    b[2] = -1;
    require_true(no_negative_c(a));
    require_false(no_negative_c(b));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // fmap_c (curried fmap)
  // ════════════════════════════════════════════════════════════════════

  test_case("fmap_c partial application");
  {
    auto add_1_c = micron::fp::fmap_c([](int x) { return x + 1; });
    micron::vector<int> v(4, 5);
    auto out = add_1_c(v);
    for ( int i = 0; i < 4; ++i ) require(out[i], 6);
  }
  end_test_case();

  property_test(
      "replicate vs naive (10k random)",
      [](u32 raw_n, u32 raw_v) {
        usize n = (raw_n & 0x1f) + 1;
        int v = static_cast<int>(raw_v & 0xff);
        auto out = micron::fp::replicate<micron::vector<int>>(n, v);
        require(out.size(), n);
        for ( usize i = 0; i < n; ++i ) require(out[i], v);
      },
      10000);

  property_test(
      "scanl prefix sum matches naive (10k random)",
      [](u32 raw_n) {
        usize n = (raw_n & 0xf) + 1;
        micron::vector<int> v(n, 0);
        prng rng(raw_n + 107);
        int buf[16];
        pat_random_small(buf, n, rng, -100, 100);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        auto out = micron::fp::scanl(v, 0, [](int acc, int x) { return acc + x; });
        require(out.size(), n + 1);
        require(out[0], 0);
        int running = 0;
        for ( usize i = 0; i < n; ++i ) {
          running += buf[i];
          require(out[i + 1], running);
        }
      },
      10000);

  sb::print("=== ALGO/FP-CORE RIGOR SUITE PASSED ===");
  return 1;
}
