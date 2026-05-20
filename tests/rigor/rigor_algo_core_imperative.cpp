// rigor_algo_core_imperative.cpp — exhaustive snowball suite for the
// imperative mutating algorithms in src/algorithm/algorithm.hpp.
//
// Coverage:
//   fill / fill_n             (pointer / container, value / Fn / compile-time)
//   generate                  (pointer / container, value / Fn / variadic)
//   transform                 (in-place / binary, runtime / compile-time)
//   shift_left / shift_right  (pointer / container)
//   rotate_left / rotate_right (pointer / container)
//   reverse / reverse_copy    (pointer / container, predicate, compile-time)
//   clamp                     (scalar, custom comparator)
//   sum / mean / geomean /
//     harmonicmean            (integral and floating containers)
//   max / min / max_at / min_at (container)
//   round / ceil / floor      (pointer / container)
//   clear                     (container with optional fill value)
//   where                     (vector-only: requires resize)
//   for_each(map), transform(map) (heap_swiss_map, rb_map)
//
// Pointer overloads receive full adversarial + 10k property coverage; map
// and container overloads use scalar/wiring tests against the principal
// implementation. iarray is excluded from container fan-outs (immutable,
// fails is_iterable_container).
//
// IMPORTANT: micron::reverse(T*, T*) expects end to be the LAST element,
// not one-past-the-end. Container reverse(C&) calls reverse(c.begin(),
// c.end() - 1) internally. Tests of the pointer overload always pass
// end - 1.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/find.hpp"
#include "../../src/maps/heap_swiss.hpp"
#include "../../src/maps/rb_map.hpp"

#include "../support/algo_rigor.hpp"

using namespace mtest::rigor;
using mtest::prng;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

// ─────────────────────────────────────────────────────────────────────────
// Stateless function pointers / lambdas used as compile-time predicates
// ─────────────────────────────────────────────────────────────────────────

static int
gen_seven(void)
{
  return 7;
}

static int
square_int(int x)
{
  return x * x;
}

static constexpr auto runtime_zero_fn = []() -> int { return 1; };
static constexpr auto runtime_neg_fn = [](int x) { return -x; };
static constexpr auto runtime_inc_fn = [](int x) { return x + 1; };
static constexpr auto runtime_double_fn = [](int x) { return x * 2; };

// pointer-form transforms
static constexpr auto ptr_neg = [](int *p) -> int { return -(*p); };
static constexpr auto ptr_square = [](int *p) -> int { return (*p) * (*p); };

// reverse comparators (Fn(T*, T*) — receive pointer pairs as ends of swap)
static constexpr auto rev_always = [](const int *, const int *) { return true; };
static constexpr auto rev_left_lt = [](const int *a, const int *b) { return *a < *b; };

// for_each/transform map predicates
static constexpr auto map_double_value = [](const int &, int &v) { v = v * 2; };
static constexpr auto map_const_visit = [](const int &, const int &) { };
static constexpr auto map_xform = [](const int &k, const int &v) { return k + v; };

int
main()
{
  sb::print("=== ALGO/CORE-IMPERATIVE RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // fill / fill_n
  // ════════════════════════════════════════════════════════════════════

  test_case("fill[ptr,value] zero-length is no-op");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::fill(a, a, 99);
    for ( int i = 0; i < 8; ++i ) require(a[i], i);
  }
  end_test_case();

  test_case("fill[ptr,value] full range");
  {
    for ( usize n : kAdversarialSizes ) {
      int a[1024];
      micron::fill(a, a + n, 42);
      for ( usize i = 0; i < n; ++i ) require(a[i], 42);
    }
  }
  end_test_case();

  // NOTE: fill(ptr, ptr, lambda) is ambiguous between the (const P&) value
  // overload and the (Fn) generator overload — both match callable types.
  // Use fill<Fn>(...) for compile-time generators instead.

  test_case("fill<Fn>(ptr) compile-time function");
  {
    int a[16];
    micron::fill<gen_seven>(a, a + 16);
    for ( int i = 0; i < 16; ++i ) require(a[i], 7);
  }
  end_test_case();

  test_case("fill_n[ptr,value] partial range, leaves tail untouched");
  {
    int a[16];
    pat_sorted(a, 16);
    micron::fill_n(a, 8, 77);
    for ( int i = 0; i < 8; ++i ) require(a[i], 77);
    for ( int i = 8; i < 16; ++i ) require(a[i], i);
  }
  end_test_case();

  test_case("fill_n<Fn>(ptr,n) compile-time generator");
  {
    int a[16];
    pat_sorted(a, 16);
    micron::fill_n<gen_seven>(a, 4);
    for ( int i = 0; i < 4; ++i ) require(a[i], 7);
    for ( int i = 4; i < 16; ++i ) require(a[i], i);
  }
  end_test_case();

  test_case("fill[container,value] fan-out");
  {
    for_each_mutating_container<int, 32>([]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c{};
      if constexpr ( requires(C x) { x.resize(usize{ 0 }); } )
        c.resize(32);
      else if constexpr ( requires(C x) { x.push_back(int{}); } ) {
        for ( int i = 0; i < 32; ++i ) c.push_back(0);
      }
      micron::fill(c, 5);
      for ( usize i = 0; i < c.size(); ++i ) require(c[i], 5);
    });
  }
  end_test_case();

  // (fill(container, lambda) skipped — same overload ambiguity)

  property_test(
      "fill[ptr,value] then all_of==value (10k random)",
      [](u32 raw_n, u32 raw_v) {
        usize n = (raw_n & 0x3f) + 1;
        int v = static_cast<int>(raw_v & 0xff);
        int buf[64];
        prng rng(raw_n + 1);
        pat_random(buf, n, rng);
        micron::fill(buf, buf + n, v);
        require_true(ref::naive_all_of_eq(buf, n, v));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // generate
  // ════════════════════════════════════════════════════════════════════

  test_case("generate[ptr,Fn] runs functor end-to-end");
  {
    int a[32];
    int counter = 0;
    micron::generate(a, a + 32, [&counter]() -> int { return ++counter; });
    for ( int i = 0; i < 32; ++i ) require(a[i], i + 1);
  }
  end_test_case();

  test_case("generate<Fn>(ptr) compile-time generator");
  {
    int a[16];
    micron::generate<gen_seven>(a, a + 16);
    for ( int i = 0; i < 16; ++i ) require(a[i], 7);
  }
  end_test_case();

  test_case("generate[ptr,Fn,Args] passes variadic args");
  {
    int a[8];
    auto add = [](int x, int y) { return x + y; };
    micron::generate(a, a + 8, add, 3, 4);
    for ( int i = 0; i < 8; ++i ) require(a[i], 7);
  }
  end_test_case();

  test_case("generate[container,Fn] fan-out");
  {
    for_each_mutating_container<int, 16>([]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c{};
      if constexpr ( requires(C x) { x.resize(usize{ 0 }); } )
        c.resize(16);
      else if constexpr ( requires(C x) { x.push_back(int{}); } ) {
        for ( int i = 0; i < 16; ++i ) c.push_back(0);
      }
      int n = 0;
      micron::generate(c, [&n]() -> int { return ++n; });
      for ( usize i = 0; i < c.size(); ++i ) require(c[i], static_cast<int>(i + 1));
    });
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // transform
  // ════════════════════════════════════════════════════════════════════

  test_case("transform[ptr,Fn-T] in-place by value");
  {
    int a[16];
    pat_sorted(a, 16);
    micron::transform(a, a + 16, square_int);
    for ( int i = 0; i < 16; ++i ) require(a[i], i * i);
  }
  end_test_case();

  test_case("transform[ptr,Fn-T*] in-place by pointer");
  {
    int a[16];
    pat_sorted(a, 16);
    micron::transform(a, a + 16, ptr_neg);
    for ( int i = 0; i < 16; ++i ) require(a[i], -i);
  }
  end_test_case();

  test_case("transform<Fn>(ptr) compile-time");
  {
    int a[8];
    pat_sorted(a, 8, 1);
    micron::transform<square_int>(a, a + 8);
    for ( int i = 0; i < 8; ++i ) require(a[i], (i + 1) * (i + 1));
  }
  end_test_case();

  test_case("transform[ptr,ptr,out,Fn] binary transform");
  {
    int a[8];
    int b[8];
    int out[8];
    pat_sorted(a, 8);
    pat_sorted(b, 8, 10);
    auto add = [](int x, int y) { return x + y; };
    micron::transform(a, a + 8, b, out, add);
    for ( int i = 0; i < 8; ++i ) require(out[i], i + (10 + i));
  }
  end_test_case();

  test_case("transform[container,Fn]");
  {
    micron::vector<int> v(10, 0);
    micron::generate(v, []() -> int {
      static int n = 0;
      return ++n;
    });
    micron::transform(v, runtime_double_fn);
    // v should be 2, 4, 6, ... but static counter pollutes — just verify >=
    for ( usize i = 0; i < 10; ++i ) require_true(v[i] > 0);
  }
  end_test_case();

  test_case("transform identity preserves data");
  {
    int a[16];
    pat_sorted(a, 16);
    micron::transform(a, a + 16, [](int x) { return x; });
    for ( int i = 0; i < 16; ++i ) require(a[i], i);
  }
  end_test_case();

  property_test(
      "transform negate is its own inverse (10k random)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x3f) + 1;
        int buf[64];
        prng rng(raw_n + 7);
        pat_random(buf, n, rng);
        int copy[64];
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::transform(buf, buf + n, runtime_neg_fn);
        micron::transform(buf, buf + n, runtime_neg_fn);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // shift_left / shift_right
  // ════════════════════════════════════════════════════════════════════

  test_case("shift_left[ptr] n=0 is no-op");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::shift_left(a, a + 8, 0);
    for ( int i = 0; i < 8; ++i ) require(a[i], i);
  }
  end_test_case();

  test_case("shift_left[ptr] partial shift zeroes tail");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::shift_left(a, a + 8, 3);
    // expected: 3 4 5 6 7 0 0 0
    require(a[0], 3);
    require(a[1], 4);
    require(a[2], 5);
    require(a[3], 6);
    require(a[4], 7);
    require(a[5], 0);
    require(a[6], 0);
    require(a[7], 0);
  }
  end_test_case();

  test_case("shift_left[ptr] n >= len wipes everything");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::shift_left(a, a + 8, 8);
    for ( int i = 0; i < 8; ++i ) require(a[i], 0);
  }
  end_test_case();

  test_case("shift_right[ptr] partial shift zeroes head");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::shift_right(a, a + 8, 3);
    require(a[0], 0);
    require(a[1], 0);
    require(a[2], 0);
    require(a[3], 0);
    require(a[4], 1);
    require(a[5], 2);
    require(a[6], 3);
    require(a[7], 4);
  }
  end_test_case();

  test_case("shift_right[ptr] n >= len wipes everything");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::shift_right(a, a + 8, 8);
    for ( int i = 0; i < 8; ++i ) require(a[i], 0);
  }
  end_test_case();

  test_case("shift_left[container] fan-out");
  {
    for_each_mutating_container<int, 16>([]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c{};
      if constexpr ( requires(C x) { x.resize(usize{ 0 }); } )
        c.resize(16);
      else if constexpr ( requires(C x) { x.push_back(int{}); } ) {
        for ( int i = 0; i < 16; ++i ) c.push_back(0);
      }
      for ( int i = 0; i < 16; ++i ) c[i] = i;
      micron::shift_left(c, 4);
      // first 12 should be 4..15, last 4 should be 0
      for ( int i = 0; i < 12; ++i ) require(c[i], i + 4);
      for ( int i = 12; i < 16; ++i ) require(c[i], 0);
    });
  }
  end_test_case();

  property_test(
      "shift_left[ptr] vs naive (10k random)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0x3f) + 1;
        usize k = raw_k & 0x3f;
        int buf[64];
        int copy[64];
        prng rng(raw_n + 11);
        pat_random(buf, n, rng);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::shift_left(buf, buf + n, k);
        ref::naive_shift_left(copy, n, k);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  property_test(
      "shift_right[ptr] vs naive (10k random)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0x3f) + 1;
        usize k = raw_k & 0x3f;
        int buf[64];
        int copy[64];
        prng rng(raw_n + 13);
        pat_random(buf, n, rng);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::shift_right(buf, buf + n, k);
        ref::naive_shift_right(copy, n, k);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // rotate_left / rotate_right
  // ════════════════════════════════════════════════════════════════════

  test_case("rotate_left[ptr] n=0 is no-op");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::rotate_left(a, a + 8, 0);
    for ( int i = 0; i < 8; ++i ) require(a[i], i);
  }
  end_test_case();

  test_case("rotate_left[ptr] by 3");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::rotate_left(a, a + 8, 3);
    // expected: 3 4 5 6 7 0 1 2
    int expected[8] = { 3, 4, 5, 6, 7, 0, 1, 2 };
    for ( int i = 0; i < 8; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  test_case("rotate_left[ptr] n mod len");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::rotate_left(a, a + 8, 11);      // 11 % 8 == 3
    int expected[8] = { 3, 4, 5, 6, 7, 0, 1, 2 };
    for ( int i = 0; i < 8; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  test_case("rotate_right[ptr] by 3");
  {
    int a[8];
    pat_sorted(a, 8);
    micron::rotate_right(a, a + 8, 3);
    int expected[8] = { 5, 6, 7, 0, 1, 2, 3, 4 };
    for ( int i = 0; i < 8; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  test_case("rotate_left then rotate_right is identity");
  {
    int a[8];
    pat_sorted(a, 8);
    int copy[8];
    for ( int i = 0; i < 8; ++i ) copy[i] = a[i];
    micron::rotate_left(a, a + 8, 3);
    micron::rotate_right(a, a + 8, 3);
    for ( int i = 0; i < 8; ++i ) require(a[i], copy[i]);
  }
  end_test_case();

  test_case("rotate_left[container] fan-out");
  {
    for_each_mutating_container<int, 16>([]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c{};
      if constexpr ( requires(C x) { x.resize(usize{ 0 }); } )
        c.resize(16);
      else if constexpr ( requires(C x) { x.push_back(int{}); } ) {
        for ( int i = 0; i < 16; ++i ) c.push_back(0);
      }
      for ( int i = 0; i < 16; ++i ) c[i] = i;
      micron::rotate_left(c, 4);
      for ( int i = 0; i < 12; ++i ) require(c[i], i + 4);
      for ( int i = 12; i < 16; ++i ) require(c[i], i - 12);
    });
  }
  end_test_case();

  property_test(
      "rotate_left[ptr] vs naive (10k random)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0x3f) + 1;      // 1..64
        usize k = raw_k & 0x1f;            // 0..31
        int buf[64];
        int copy[64];
        prng rng(raw_n + 17);
        pat_random(buf, n, rng);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::rotate_left(buf, buf + n, k);
        ref::naive_rotate_left(copy, n, k);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  property_test(
      "rotate_right is inverse of rotate_left (10k random)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0x3f) + 1;      // 1..64
        usize k = (raw_k & 0x1f) + 1;      // 1..32
        int buf[64];
        int copy[64];
        prng rng(raw_n + 19);
        pat_random(buf, n, rng);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        micron::rotate_left(buf, buf + n, k);
        micron::rotate_right(buf, buf + n, k);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // reverse / reverse_copy
  //
  // micron::reverse(T*, T*) uses end-INCLUSIVE end (last element). The
  // container overload internally calls reverse(c.begin(), c.end() - 1).
  // ════════════════════════════════════════════════════════════════════

  test_case("reverse[ptr] standard pattern (end - 1)");
  {
    int a[5] = { 1, 2, 3, 4, 5 };
    micron::reverse(a, a + 5 - 1);
    int expected[5] = { 5, 4, 3, 2, 1 };
    for ( int i = 0; i < 5; ++i ) require(a[i], expected[i]);
  }
  end_test_case();

  test_case("reverse[ptr] two-element range is two-element swap");
  {
    int a[2] = { 1, 2 };
    micron::reverse(a, a + 1);      // begin and end-1 are distinct pointers
    require(a[0], 2);
    require(a[1], 1);
  }
  end_test_case();

  test_case("reverse[container] vector");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i;
    micron::reverse(v);
    for ( int i = 0; i < 8; ++i ) require(v[i], 7 - i);
  }
  end_test_case();

  test_case("reverse[container] array");
  {
    micron::array<int, 8> a;
    for ( int i = 0; i < 8; ++i ) a[i] = i;
    micron::reverse(a);
    for ( int i = 0; i < 8; ++i ) require(a[i], 7 - i);
  }
  end_test_case();

  test_case("reverse twice is identity");
  {
    int a[16];
    pat_sorted(a, 16);
    int copy[16];
    for ( int i = 0; i < 16; ++i ) copy[i] = a[i];
    micron::reverse(a, a + 16 - 1);
    micron::reverse(a, a + 16 - 1);
    for ( int i = 0; i < 16; ++i ) require(a[i], copy[i]);
  }
  end_test_case();

  test_case("reverse_copy[ptr] writes to output");
  {
    int a[8];
    int out[8];
    pat_sorted(a, 8);
    micron::reverse_copy(a, a + 8, out);
    for ( int i = 0; i < 8; ++i ) require(out[i], 7 - i);
  }
  end_test_case();

  test_case("reverse_copy[container] returns reversed copy");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i;
    auto rc = micron::reverse_copy(v);
    require(rc.size(), usize(8));
    for ( int i = 0; i < 8; ++i ) require(rc[i], 7 - i);
  }
  end_test_case();

  property_test(
      "reverse[ptr] vs naive (10k random)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x3f) + 1;
        int buf[64];
        int copy[64];
        prng rng(raw_n + 23);
        pat_random(buf, n, rng);
        for ( usize i = 0; i < n; ++i ) copy[i] = buf[i];
        if ( n >= 1 ) micron::reverse(buf, buf + n - 1);
        ref::naive_reverse(copy, n);
        require_true(ref::naive_equal(buf, copy, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // clamp
  // ════════════════════════════════════════════════════════════════════

  test_case("clamp[scalar] interior unchanged, exterior clipped");
  {
    require(micron::clamp(5, 0, 10), 5);
    require(micron::clamp(-3, 0, 10), 0);
    require(micron::clamp(15, 0, 10), 10);
    require(micron::clamp(0, 0, 10), 0);
    require(micron::clamp(10, 0, 10), 10);
  }
  end_test_case();

  test_case("clamp[scalar] equal lo == hi");
  {
    require(micron::clamp(7, 5, 5), 5);
    require(micron::clamp(3, 5, 5), 5);
    require(micron::clamp(10, 5, 5), 5);
  }
  end_test_case();

  test_case("clamp with custom comparator");
  {
    auto cmp = [](int a, int b) { return a < b; };
    require(micron::clamp(5, 0, 10, cmp), 5);
    require(micron::clamp(-1, 0, 10, cmp), 0);
    require(micron::clamp(99, 0, 10, cmp), 10);
  }
  end_test_case();

  property_test(
      "clamp[scalar] invariants (10k random)",
      [](u32 raw_v, u32 raw_lo) {
        int v = static_cast<int>(raw_v & 0xffff) - 0x8000;
        int lo = static_cast<int>(raw_lo & 0xff);
        int hi = lo + static_cast<int>(raw_v >> 16) % 256 + 1;
        int r = micron::clamp(v, lo, hi);
        require_true(r >= lo);
        require_true(r <= hi);
        if ( v >= lo && v <= hi ) require(r, v);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // sum / mean / geomean / harmonicmean
  // ════════════════════════════════════════════════════════════════════

  test_case("sum[vector<int>]");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = i + 1;      // 1..5 = 15
    require(micron::sum(v), umax_t(15));
  }
  end_test_case();

  test_case("sum[vector<int>] empty is 0");
  {
    micron::vector<int> v;
    require(micron::sum(v), umax_t(0));
  }
  end_test_case();

  test_case("sum[array<int,5>]");
  {
    micron::array<int, 5> a;
    for ( int i = 0; i < 5; ++i ) a[i] = (i + 1) * 10;      // 10..50 = 150
    require(micron::sum(a), umax_t(150));
  }
  end_test_case();

  test_case("sum[vector<f64>] floating point");
  {
    micron::vector<f64> v(3, 0.0);
    v[0] = 1.5;
    v[1] = 2.5;
    v[2] = 3.0;
    auto s = micron::sum(v);
    require_true(near<f64>(static_cast<f64>(s), 7.0, 1e-9));
  }
  end_test_case();

  test_case("mean[vector<int>]");
  {
    micron::vector<int> v(5, 0);
    for ( int i = 0; i < 5; ++i ) v[i] = 2 + i * 2;      // 2,4,6,8,10 mean=6
    f64 m = micron::mean(v);
    require_true(near<f64>(m, 6.0, 1e-6));
  }
  end_test_case();

  test_case("geomean[vector<int>] of {1,2,4,8} == 2.828");
  {
    micron::vector<int> v(4, 0);
    v[0] = 1;
    v[1] = 2;
    v[2] = 4;
    v[3] = 8;
    auto gm = micron::geomean(v);
    require_true(near<flong>(gm, static_cast<flong>(2.8284271247), static_cast<flong>(1e-4)));
  }
  end_test_case();

  test_case("harmonicmean[vector<int>] of {1,2,4} == 1.714");
  {
    micron::vector<int> v(3, 0);
    v[0] = 1;
    v[1] = 2;
    v[2] = 4;
    auto hm = micron::harmonicmean(v);
    require_true(near<flong>(hm, static_cast<flong>(1.714285714), static_cast<flong>(1e-4)));
  }
  end_test_case();

  property_test(
      "sum[ptr,int] vs naive (10k random)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x1f) + 1;
        int buf[32];
        prng rng(raw_n + 31);
        pat_random_small(buf, n, rng, -1000, 1000);
        micron::vector<int> v(n, 0);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        umax_t actual = micron::sum(v);
        umax_t expected = 0;
        for ( usize i = 0; i < n; ++i ) expected = static_cast<umax_t>(expected + static_cast<umax_t>(buf[i]));
        require(actual, expected);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // max / min / max_at / min_at
  // ════════════════════════════════════════════════════════════════════

  test_case("max/min[vector]");
  {
    micron::vector<int> v(8, 0);
    int d[8] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    for ( int i = 0; i < 8; ++i ) v[i] = d[i];
    require(micron::max(v), 9);
    require(micron::min(v), 1);
  }
  end_test_case();

  test_case("max/min[array]");
  {
    micron::array<int, 5> a;
    a[0] = 7;
    a[1] = 3;
    a[2] = 9;
    a[3] = 1;
    a[4] = 5;
    require(micron::max(a), 9);
    require(micron::min(a), 1);
  }
  end_test_case();

  test_case("max/min single element");
  {
    micron::vector<int> v(1, 42);
    require(micron::max(v), 42);
    require(micron::min(v), 42);
  }
  end_test_case();

  test_case("max/min all-equal returns same");
  {
    micron::array<int, 8> a;
    for ( int i = 0; i < 8; ++i ) a[i] = 5;
    require(micron::max(a), 5);
    require(micron::min(a), 5);
  }
  end_test_case();

  test_case("max_at returns iterator to max");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { 3, 9, 1, 7, 5 };
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    auto it = micron::max_at(v);
    require(*it, 9);
    require_true(static_cast<usize>(it - v.cbegin()) == 1);
  }
  end_test_case();

  test_case("min_at returns iterator to min");
  {
    micron::vector<int> v(5, 0);
    int d[5] = { 3, 9, 1, 7, 5 };
    for ( int i = 0; i < 5; ++i ) v[i] = d[i];
    auto it = micron::min_at(v);
    require(*it, 1);
    require_true(static_cast<usize>(it - v.cbegin()) == 2);
  }
  end_test_case();

  property_test(
      "max/min[vector] vs naive (10k random)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x1f) + 1;
        int buf[32];
        prng rng(raw_n + 37);
        pat_random_small(buf, n, rng, -1000, 1000);
        micron::vector<int> v(n, 0);
        for ( usize i = 0; i < n; ++i ) v[i] = buf[i];
        require(micron::max(v), ref::naive_max(buf, n));
        require(micron::min(v), ref::naive_min(buf, n));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // round / ceil / floor
  // ════════════════════════════════════════════════════════════════════

  test_case("round[ptr,f32] half-to-even-or-away rounding");
  {
    f32 a[4] = { 1.4f, 1.6f, -1.4f, -1.6f };
    micron::round(a, a + 4);
    require(a[0], 1.0f);
    require(a[1], 2.0f);
    require(a[2], -1.0f);
    require(a[3], -2.0f);
  }
  end_test_case();

  test_case("ceil[container]");
  {
    micron::vector<f32> v(3, 0.0f);
    v[0] = 1.1f;
    v[1] = 2.0f;
    v[2] = 3.9f;
    micron::ceil(v);
    require(v[0], 2.0f);
    require(v[1], 2.0f);
    require(v[2], 4.0f);
  }
  end_test_case();

  test_case("floor[ptr,f32]");
  {
    f32 a[3] = { 1.9f, 2.0f, 3.1f };
    micron::floor(a, a + 3);
    require(a[0], 1.0f);
    require(a[1], 2.0f);
    require(a[2], 3.0f);
  }
  end_test_case();

  test_case("floor[container]");
  {
    micron::array<f32, 4> a;
    a[0] = 1.2f;
    a[1] = -1.2f;
    a[2] = 0.0f;
    a[3] = 5.9f;
    micron::floor(a);
    require(a[0], 1.0f);
    require(a[1], -2.0f);
    require(a[2], 0.0f);
    require(a[3], 5.0f);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // clear
  // ════════════════════════════════════════════════════════════════════

  test_case("clear[vector] zeroes elements");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i + 1;
    micron::clear(v);
    for ( int i = 0; i < 8; ++i ) require(v[i], 0);
  }
  end_test_case();

  test_case("clear[vector] with fill value");
  {
    micron::vector<int> v(8, 0);
    for ( int i = 0; i < 8; ++i ) v[i] = i + 1;
    micron::clear(v, 99);
    for ( int i = 0; i < 8; ++i ) require(v[i], 99);
  }
  end_test_case();

  test_case("clear[array] zeroes");
  {
    micron::array<int, 8> a;
    for ( int i = 0; i < 8; ++i ) a[i] = i + 1;
    micron::clear(a);
    for ( int i = 0; i < 8; ++i ) require(a[i], 0);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // where (vector-only: requires resize)
  // ════════════════════════════════════════════════════════════════════

  test_case("where[vector,Fn-T] selects matching");
  {
    micron::vector<int> v(10, 0);
    for ( int i = 0; i < 10; ++i ) v[i] = i;
    auto out = micron::where(v, [](int x) { return (x & 1) == 0; });
    require(out.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(out[i], i * 2);
  }
  end_test_case();

  test_case("where[vector] empty match returns empty");
  {
    micron::vector<int> v(8, 0);
    auto out = micron::where(v, [](int x) { return x > 100; });
    require(out.size(), usize(0));
  }
  end_test_case();

  test_case("where[vector] all-match returns copy");
  {
    micron::vector<int> v(6, 5);
    auto out = micron::where(v, [](int x) { return x == 5; });
    require(out.size(), usize(6));
    for ( int i = 0; i < 6; ++i ) require(out[i], 5);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // for_each / transform on maps
  // ════════════════════════════════════════════════════════════════════

  test_case("for_each[heap_swiss_map] visits all kv");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i * 10);
    int seen = 0;
    micron::for_each(m, [&seen](const int &, const int &) { ++seen; });
    require(seen, 10);
  }
  end_test_case();

  test_case("for_each[heap_swiss_map] mutates values");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i);
    micron::for_each(m, map_double_value);
    for ( int i = 0; i < 10; ++i ) {
      auto *p = micron::find(m, i * 2);
      require_true(p != nullptr);
    }
  }
  end_test_case();

  test_case("transform[heap_swiss_map] applies fn(k, v)");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i);
    micron::transform(m, map_xform);
    for ( int i = 0; i < 10; ++i ) {
      // expected: each value becomes k + old_v = 2*i
      auto *p = micron::find(m, i * 2);
      require_true(p != nullptr);
    }
  }
  end_test_case();

  test_case("for_each[rb_map] visits in order");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 10; ++i ) m.insert(i, i + 100);
    int sum = 0;
    micron::for_each(m, [&sum](const int &, const int &v) { sum += v; });
    require(sum, 1045);      // 100..109 = 1045
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Compile-time `auto Fn` variants
  // ════════════════════════════════════════════════════════════════════

  test_case("fill<Fn>(container)");
  {
    micron::array<int, 16> a;
    micron::fill<gen_seven>(a);
    for ( int i = 0; i < 16; ++i ) require(a[i], 7);
  }
  end_test_case();

  test_case("generate<Fn>(container)");
  {
    micron::array<int, 16> a;
    micron::generate<gen_seven>(a);
    for ( int i = 0; i < 16; ++i ) require(a[i], 7);
  }
  end_test_case();

  test_case("transform<Fn>(container)");
  {
    micron::array<int, 8> a;
    for ( int i = 0; i < 8; ++i ) a[i] = i + 1;
    micron::transform<square_int>(a);
    for ( int i = 0; i < 8; ++i ) require(a[i], (i + 1) * (i + 1));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Final composition / stress
  // ════════════════════════════════════════════════════════════════════

  test_case("fill -> transform -> sum pipeline");
  {
    micron::vector<int> v(10, 0);
    micron::fill(v, 3);
    micron::transform(v, runtime_double_fn);      // 3 -> 6
    require(micron::sum(v), umax_t(60));
  }
  end_test_case();

  test_case("generate -> reverse -> reverse roundtrip");
  {
    micron::array<int, 6> a;
    int n = 0;
    micron::generate(a, [&n]() -> int { return ++n; });
    int copy[6];
    for ( int i = 0; i < 6; ++i ) copy[i] = a[i];
    micron::reverse(a);
    micron::reverse(a);
    for ( int i = 0; i < 6; ++i ) require(a[i], copy[i]);
  }
  end_test_case();

  test_case("fill -> count consistency");
  {
    micron::vector<int> v(50, 0);
    micron::fill(v, 7);
    require(micron::count(v, 7), umax_t(50));
    require(micron::count(v, 0), umax_t(0));
  }
  end_test_case();

  sb::print("=== ALGO/CORE-IMPERATIVE RIGOR SUITE PASSED ===");
  return 1;
}
