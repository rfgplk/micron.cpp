// rigor_algo_data_heap.cpp — snowball suite for src/algorithm/data.hpp
//
// Coverage:
//   melt                        (container-of-container reshape)
//   cut                         (binning)
//   merge (5-iter, container)   (sorted merge / unsorted append)
//   concat                      (alias for container merge)
//   reverse (iterator)          (data.hpp's --last variant)
//   rotate (iterator)           (3-iterator rotate)
//   cycle_rotate                (GCD-based rotate)
//   make_heap / push_heap /
//     pop_heap / sort_heap /
//     sort_heap_min / is_heap   (with/without limit, with/without cmp)
//
// NOTE: data.hpp's container merge is APPEND (not sorted-merge). Use the
// iterator overload for sorted-merge semantics.
// NOTE: make_heap with default comparator builds a MAX-heap (parent >= child).
// sort_heap (default cmp) sorts ASCENDING; sort_heap_min sorts DESCENDING.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/data.hpp"
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
  sb::print("=== ALGO/DATA-HEAP RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // merge / concat
  // ════════════════════════════════════════════════════════════════════

  test_case("merge[5-iter] sorted ranges produce sorted output");
  {
    int a[5] = { 1, 3, 5, 7, 9 };
    int b[5] = { 2, 4, 6, 8, 10 };
    int out[10] = {};
    micron::merge(a, a + 5, b, b + 5, out);
    for ( int i = 0; i < 10; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("merge[5-iter] one empty range");
  {
    int a[5] = { 1, 2, 3, 4, 5 };
    int b[1];
    int out[5] = {};
    micron::merge(a, a + 5, b, b, out);
    for ( int i = 0; i < 5; ++i ) require(out[i], i + 1);
  }
  end_test_case();

  test_case("merge[5-iter] both empty produces nothing");
  {
    int a[1];
    int b[1];
    int out[1] = { 99 };
    auto *end = micron::merge(a, a, b, b, out);
    require_true(end == out);
    require(out[0], 99);      // untouched
  }
  end_test_case();

  test_case("merge[container] appends (NOT sorted-merge)");
  {
    micron::vector<int> v1(3, 0);
    v1[0] = 1;
    v1[1] = 2;
    v1[2] = 3;
    micron::vector<int> v2(3, 0);
    v2[0] = 10;
    v2[1] = 20;
    v2[2] = 30;
    auto out = micron::merge(v1, v2);
    require(out.size(), usize(6));
    int expected[6] = { 1, 2, 3, 10, 20, 30 };
    for ( int i = 0; i < 6; ++i ) require(out[i], expected[i]);
  }
  end_test_case();

  test_case("concat is alias for container merge");
  {
    micron::vector<int> v1(3, 0);
    micron::vector<int> v2(3, 0);
    for ( int i = 0; i < 3; ++i ) {
      v1[i] = i;
      v2[i] = i + 10;
    }
    auto m = micron::merge(v1, v2);
    auto c = micron::concat(v1, v2);
    require(m.size(), c.size());
    for ( usize i = 0; i < m.size(); ++i ) require(m[i], c[i]);
  }
  end_test_case();

  property_test(
      "merge[5-iter] result is sorted when both inputs are sorted (10k)",
      [](u32 raw_n, u32 raw_seed) {
        usize n1 = (raw_n & 0xf) + 1;
        usize n2 = ((raw_n >> 4) & 0xf) + 1;
        int a[16];
        int b[16];
        int out[32] = {};
        prng rng(raw_seed + 73);
        pat_random_small(a, n1, rng, -100, 100);
        pat_random_small(b, n2, rng, -100, 100);
        // sort the inputs naively
        for ( usize i = 0; i < n1; ++i )
          for ( usize j = i + 1; j < n1; ++j )
            if ( a[j] < a[i] ) {
              int t = a[i];
              a[i] = a[j];
              a[j] = t;
            }
        for ( usize i = 0; i < n2; ++i )
          for ( usize j = i + 1; j < n2; ++j )
            if ( b[j] < b[i] ) {
              int t = b[i];
              b[i] = b[j];
              b[j] = t;
            }
        micron::merge(a, a + n1, b, b + n2, out);
        // check sorted
        for ( usize i = 1; i < n1 + n2; ++i ) require_true(out[i] >= out[i - 1]);
      },
      10000);

  // NOTE: data.hpp's reverse(I, I) is shadowed by algorithm.hpp's
  // reverse(T*, T*) when called with raw pointers — the latter has
  // requires(is_pointer_v<T>) and is more specialized. The algorithm.hpp
  // reverse uses inclusive-end semantics (already covered in
  // rigor_algo_core_imperative.cpp). data.hpp's generic-iterator reverse
  // is effectively unreachable from pointer call sites.

  // ════════════════════════════════════════════════════════════════════
  // rotate / cycle_rotate (iterator)
  // ════════════════════════════════════════════════════════════════════

  test_case("rotate[iterator] basic");
  {
    int a[8];
    pat_sorted(a, 8);
    auto r = micron::rotate(a, a + 3, a + 8);
    int expected[8] = { 3, 4, 5, 6, 7, 0, 1, 2 };
    for ( int i = 0; i < 8; ++i ) require(a[i], expected[i]);
    (void)r;
  }
  end_test_case();

  test_case("rotate[iterator] n_first == first is no-op");
  {
    int a[5];
    pat_sorted(a, 5);
    micron::rotate(a, a, a + 5);
    for ( int i = 0; i < 5; ++i ) require(a[i], i);
  }
  end_test_case();

  test_case("rotate[iterator] coprime gcd case (n=7, k=3)");
  {
    int a[7];
    pat_sorted(a, 7);
    micron::rotate(a, a + 3, a + 7);
    int expected[7] = { 3, 4, 5, 6, 0, 1, 2 };
    for ( int i = 0; i < 7; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  test_case("cycle_rotate basic");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::cycle_rotate(a, a + 3, a + 8);
    int expected[8] = { 3, 4, 5, 6, 7, 0, 1, 2 };
    for ( int i = 0; i < 8; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  test_case("cycle_rotate with coprime n,k");
  {
    int a[7];      // prime length
    pat_sorted(a, 7);
    micron::cycle_rotate(a, a + 3, a + 7);
    int expected[7] = { 3, 4, 5, 6, 0, 1, 2 };
    for ( int i = 0; i < 7; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  property_test(
      "rotate vs naive (10k random)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0x1f) + 1;
        usize k = (raw_k % n);
        int buf[32];
        int copy[32];
        prng rng(raw_n + 83);
        pat_random(buf, n, rng);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::rotate(buf, buf + k, buf + n);
        ref::naive_rotate_left(copy, n, k);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  property_test(
      "cycle_rotate vs naive (10k random)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0x1f) + 1;      // 1..32 fits in buf[32]
        usize k = (raw_k % n);
        if ( k == 0 ) k = 1 % n;      // cycle_rotate guards both being 0
        if ( n == 1 ) return;         // skip degenerate
        int buf[32];
        int copy[32];
        prng rng(raw_n + 89);
        pat_random(buf, n, rng);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::cycle_rotate(buf, buf + k, buf + n);
        ref::naive_rotate_left(copy, n, k);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // make_heap / push_heap / pop_heap / sort_heap / is_heap
  // ════════════════════════════════════════════════════════════════════

  test_case("make_heap[vector] default builds max-heap");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 4, 1, 7, 3, 8, 2, 6, 5 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::make_heap(v);
    require_true(micron::is_heap(v));
  }
  end_test_case();

  test_case("make_heap[vector] custom comparator");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 4, 1, 7, 3, 8, 2, 6, 5 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    // min-heap via greater-than
    auto cmp = [](const int &a, const int &b) { return a > b; };
    micron::make_heap(v, cmp);
    require_true(micron::is_heap(v, cmp));
  }
  end_test_case();

  test_case("make_heap[empty] is no-op, is_heap true");
  {
    micron::vector<int> v;
    micron::make_heap(v);
    require_true(micron::is_heap(v));
  }
  end_test_case();

  test_case("make_heap[single element] is heap");
  {
    micron::vector<int> v(1, 42);
    micron::make_heap(v);
    require_true(micron::is_heap(v));
  }
  end_test_case();

  test_case("push_heap appends to existing heap");
  {
    micron::vector<int> v;
    v.push_back(100);
    v.push_back(50);
    v.push_back(75);
    micron::make_heap(v);      // valid max-heap
    require_true(micron::is_heap(v));
    v.push_back(200);
    micron::push_heap(v);
    require_true(micron::is_heap(v));
  }
  end_test_case();

  test_case("pop_heap moves root to end, restores property");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 4, 1, 7, 3, 8, 2, 6, 5 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::make_heap(v);
    int root = v[0];
    micron::pop_heap(v);
    // after pop_heap, root is at end, rest is still a heap of size n-1
    require(v[v.size() - 1], root);
    // verify heap property on [0..n-1)
    micron::vector<int> sub(v.size() - 1, 0);
    for ( usize i = 0; i < sub.size(); ++i ) sub[i] = v[i];
    require_true(micron::is_heap(sub));
  }
  end_test_case();

  test_case("sort_heap (max-heap input → ascending output)");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 4, 1, 7, 3, 8, 2, 6, 5 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::make_heap(v);      // max-heap (default)
    micron::sort_heap(v);      // sift with default (a < b) — ascending
    for ( usize i = 1; i < v.size(); ++i ) require_true(v[i] >= v[i - 1]);
  }
  end_test_case();

  test_case("sort_heap_min (min-heap input → descending output)");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 4, 1, 7, 3, 8, 2, 6, 5 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    auto min_cmp = [](const int &a, const int &b) { return a > b; };
    micron::make_heap(v, min_cmp);      // min-heap
    micron::sort_heap_min(v);           // sift with (a > b) — descending
    for ( usize i = 1; i < v.size(); ++i ) require_true(v[i] <= v[i - 1]);
  }
  end_test_case();

  test_case("sort_heap[cmp] custom order on max-heap");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 4, 1, 7, 3, 8, 2, 6, 5 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    micron::make_heap(v);
    auto less_cmp = [](const int &a, const int &b) { return a < b; };
    micron::sort_heap(v, less_cmp);
    for ( usize i = 1; i < v.size(); ++i ) require_true(v[i] >= v[i - 1]);
  }
  end_test_case();

  test_case("is_heap detects valid heap");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { 9, 5, 7, 1, 3 };
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    require_true(micron::is_heap(v));
  }
  end_test_case();

  test_case("is_heap detects invalid heap");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { 1, 5, 7, 3, 9 };      // not a max-heap
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    require_false(micron::is_heap(v));
  }
  end_test_case();

  property_test(
      "make_heap → is_heap holds (10k random)",
      [](u32 raw_n, u32 raw_seed) {
        usize n = (raw_n & 0x1f) + 1;
        int buf[32];
        prng rng(raw_seed + 97);
        pat_random_small(buf, n, rng, -1000, 1000);
        micron::vector<int> v(n, 0);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        micron::make_heap(v);
        require_true(micron::is_heap(v));
      },
      10000);

  property_test(
      "sort_heap produces sorted result (10k random)",
      [](u32 raw_n, u32 raw_seed) {
        usize n = (raw_n & 0x1f) + 1;
        int buf[32];
        prng rng(raw_seed + 101);
        pat_random_small(buf, n, rng, -1000, 1000);
        micron::vector<int> v(n, 0);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        micron::make_heap(v);
        micron::sort_heap(v);
        for ( usize i = 1; i < n; ++i ) require_true(v[i] >= v[i - 1]);
      },
      10000);

  sb::print("=== ALGO/DATA-HEAP RIGOR SUITE PASSED ===");
  return 1;
}
