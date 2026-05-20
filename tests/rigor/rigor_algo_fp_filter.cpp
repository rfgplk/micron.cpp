// rigor_algo_fp_filter.cpp — snowball suite for
//   src/algorithm/filter.hpp + src/algorithm/fpfilter.hpp
//
// Coverage:
//   filter (pointer + container variants)
//   prune
//   reject
//   partition
//   take / drop
//   take_while / drop_while
//   span / sbreak
//   unique / nub
//   filter_c / reject_c / partition_c / take_c / drop_c (curried)
//   take_while_c / drop_while_c
//
// Container-returning algorithms use vector::resize to shrink the output;
// the recent vector::resize patch (project-vector-resize-only-grows) makes
// these tests' .size() assertions meaningful.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/filter.hpp"
#include "../../src/algorithm/find.hpp"
#include "../../src/algorithm/fpfilter.hpp"

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
  sb::print("=== ALGO/FP-FILTER RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // filter (pointer + container)
  // ════════════════════════════════════════════════════════════════════

  test_case("filter[ptr] selects matching to output");
  {
    int src[10];
    pat_sorted(src, 10);
    int out[10] = {};
    auto *last = micron::filter(src, src + 10, [](const int *x) { return ((*x) & 1) == 0; }, out);
    auto cnt = static_cast<usize>(last - out);
    require(cnt, usize(5));
    int expected[5] = { 0, 2, 4, 6, 8 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("filter[ptr,limit] respects bound");
  {
    int src[10];
    pat_sorted(src, 10);
    int out[10] = {};
    auto *last = micron::filter(src, src + 10, [](const int *x) { return *x >= 0; }, out, usize(3));
    auto cnt = static_cast<usize>(last - out);
    require(cnt, usize(3));
  }
  end_test_case();

  test_case("filter[container] returns vector of matches");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    auto out = micron::filter(v, [](const int *x) { return ((*x) & 1) == 0; });
    require(out.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(out[i], i * 2);
  }
  end_test_case();

  test_case("filter_inplace shrinks in place");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    micron::filter_inplace(v, [](const int *x) { return (*x) % 3 == 0; });
    // matches: 0, 3, 6, 9
    require(v.size(), usize(4));
    require(v[0], 0);
    require(v[1], 3);
    require(v[2], 6);
    require(v[3], 9);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // prune  (filter to bounded output, returns iter into output)
  // ════════════════════════════════════════════════════════════════════

  test_case("prune[ptr] returns input position, fills bounded output");
  {
    int src[10];
    pat_sorted(src, 10);
    int out[3] = {};
    // prune returns the position in INPUT where it stopped (after writing
    // `limit` matches). For pat_sorted(0..9) with predicate "x >= 0" and
    // limit=3, we stop after consuming src[0..2] and writing out[0..2].
    auto *input_pos = micron::prune(src, src + 10, [](const int *x) { return *x >= 0; }, out, usize(3));
    auto consumed = static_cast<usize>(input_pos - src);
    require(consumed, usize(3));
    require(out[0], 0);
    require(out[1], 1);
    require(out[2], 2);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // reject (fp namespace)
  // ════════════════════════════════════════════════════════════════════

  test_case("reject is complement of filter");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    auto out = micron::fp::reject(v, [](int x) { return (x & 1) == 0; });
    // rejected (kept the odd): 1, 3, 5, 7, 9
    require(out.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(out[i], 2 * i + 1);
  }
  end_test_case();

  test_case("reject all-false predicate yields full copy");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i + 1;
    auto out = micron::fp::reject(v, [](int) { return false; });
    require(out.size(), v.size());
    for ( usize i = 0; i < v.size(); ++i ) require(out[i], v[i]);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // partition (fp)
  // ════════════════════════════════════════════════════════════════════

  test_case("partition splits into (matching, non-matching)");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    auto pr = micron::fp::partition(v, [](int x) { return (x & 1) == 0; });
    auto &matching = micron::get<0>(pr);
    auto &rest = micron::get<1>(pr);
    require(matching.size(), usize(5));
    require(rest.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(matching[i], i * 2);
    for ( int i = 0; i < 5; ++i ) require(rest[i], 2 * i + 1);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // take / drop
  // ════════════════════════════════════════════════════════════════════

  test_case("take(n) returns first n");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i + 1;
    auto out = micron::fp::take(v, usize(4));
    require(out.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("take(n) where n > size returns all");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto out = micron::fp::take(v, usize(100));
    require(out.size(), usize(5));
  }
  end_test_case();

  test_case("take(0) returns empty");
  {
    micron::vector<int> v(5, 0);
    auto out = micron::fp::take(v, usize(0));
    require(out.size(), usize(0));
  }
  end_test_case();

  test_case("drop(n) drops first n");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i + 1;
    auto out = micron::fp::drop(v, usize(4));
    require(out.size(), usize(6));
    for ( int i = 0; i < 6; ++i ) require(out[i], i + 5);
  }
  end_test_case();

  test_case("drop(n >= size) returns empty");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;
    auto out = micron::fp::drop(v, usize(10));
    require(out.size(), usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // take_while / drop_while
  // ════════════════════════════════════════════════════════════════════

  test_case("take_while takes until predicate fails");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 100, 5, 6, 7 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto out = micron::fp::take_while(v, [](int x) { return x < 10; });
    require(out.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("drop_while drops until predicate fails");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 4, 100, 5, 6, 7 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto out = micron::fp::drop_while(v, [](int x) { return x < 10; });
    require(out.size(), usize(4));
    require(out[0], 100);
    require(out[1], 5);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // span / sbreak  (split on predicate)
  // ════════════════════════════════════════════════════════════════════

  test_case("span splits at first predicate failure");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 100, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto pr = micron::fp::span(v, [](int x) { return x < 10; });
    auto &head = micron::get<0>(pr);
    auto &tail = micron::get<1>(pr);
    require(head.size(), usize(3));
    require(tail.size(), usize(5));
    require(tail[0], 100);
  }
  end_test_case();

  test_case("sbreak is complement of span");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 1, 2, 3, 100, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto pr = micron::fp::sbreak(v, [](int x) { return x >= 10; });
    auto &head = micron::get<0>(pr);
    auto &tail = micron::get<1>(pr);
    require(head.size(), usize(3));
    require(tail.size(), usize(5));
    require(tail[0], 100);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // unique / nub  (remove consecutive duplicates)
  // ════════════════════════════════════════════════════════════════════

  test_case("unique removes consecutive duplicates");
  {
    micron::vector<int> v(10, 0);
    int d[10] = { 1, 1, 2, 2, 2, 3, 1, 1, 4, 4 };
    for ( int i = 0; i < 10; ++i ) v[i] = d[i];
    auto out = micron::fp::unique(v);
    // expected: [1, 2, 3, 1, 4]
    require(out.size(), usize(5));
    int expected[5] = { 1, 2, 3, 1, 4 };
    for ( int i = 0; i < 5; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("nub aliases unique");
  {
    micron::vector<int> v(6, 0);
    int d[6] = { 1, 1, 2, 3, 3, 4 };
    for ( int i = 0; i < 6; ++i ) v[i] = d[i];
    auto a = micron::fp::nub(v);
    auto b = micron::fp::unique(v);
    require(a.size(), b.size());
    for ( usize i = 0; i < a.size(); ++i ) require(a[i], b[i]);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Curried variants
  // ════════════════════════════════════════════════════════════════════

  test_case("filter_c partial application");
  {
    auto evens_c = micron::fp::filter_c([](const int *x) { return ((*x) & 1) == 0; });
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i;
    auto out = evens_c(v);
    require(out.size(), usize(3));
  }
  end_test_case();

  test_case("reject_c partial application");
  {
    auto not_evens_c = micron::fp::reject_c([](int x) { return (x & 1) == 0; });
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i;
    auto out = not_evens_c(v);
    require(out.size(), usize(3));
    require(out[0], 1);
    require(out[1], 3);
    require(out[2], 5);
  }
  end_test_case();

  test_case("take_c partial application");
  {
    auto take_3_c = micron::fp::take_c(usize(3));
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i + 1;
    auto out = take_3_c(v);
    require(out.size(), usize(3));
    for ( int i = 0; i < 3; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("drop_c partial application");
  {
    auto drop_2_c = micron::fp::drop_c(usize(2));
    micron::vector<int> v(6, 0);
    for ( int i = 0; i < 6; ++i ) v[i] = i + 1;
    auto out = drop_2_c(v);
    require(out.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(out[i], i + 3);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Property tests
  // ════════════════════════════════════════════════════════════════════

  property_test(
      "filter then reject of same predicate yields full input length (10k)",
      [](u32 raw_n) {
        usize n = (raw_n & 0xf) + 1;
        micron::vector<int> v(n, 0);
        prng rng(raw_n + 109);
        int buf[16];
        pat_random_small(buf, n, rng, 0, 100);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        auto pred_v = [](int x) { return x < 50; };
        auto pred_p = [](const int *x) { return *x < 50; };
        auto kept = micron::filter(v, pred_p);
        auto rejected = micron::fp::reject(v, pred_v);
        require(kept.size() + rejected.size(), n);
      },
      10000);

  property_test(
      "take(n) + drop(n) concatenate to original (10k)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0xf) + 1;
        usize k = raw_k % (n + 1);
        micron::vector<int> v(n, 0);
        prng rng(raw_n + 113);
        int buf[16];
        pat_random_small(buf, n, rng, -100, 100);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        auto a = micron::fp::take(v, k);
        auto b = micron::fp::drop(v, k);
        require(a.size() + b.size(), n);
        for ( usize i = 0; i < a.size(); ++i ) require(a[i], v[i]);
        for ( usize i = 0; i < b.size(); ++i ) require(b[i], v[k + i]);
      },
      10000);

  sb::print("=== ALGO/FP-FILTER RIGOR SUITE PASSED ===");
  return 1;
}
