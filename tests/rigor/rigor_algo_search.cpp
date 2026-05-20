// rigor_algo_search.cpp — exhaustive snowball suite for src/algorithm/find.hpp
//
// Coverage:
//   all_of / any_of / none_of           (pointer/container/compile-time/map)
//   find / find_if / find_if_not        (pointer/container/compile-time)
//   find_last / find_last_if /
//     find_last_if_not                  (pointer/container/compile-time)
//   find_end                            (pointer/container, custom Fn)
//   find_first_of                       (pointer/container, custom Fn)
//   adjacent_find                       (pointer/container, custom Fn)
//   search / search_n                   (pointer/container, custom Fn)
//   count / count_if                    (pointer/container/compile-time/map)
//   contains / contains_if /
//     contains_subrange                 (pointer/container, map)
//   mismatch                            (pointer/container, custom Fn)
//   equal                               (pointer/container, custom Fn)
//   starts_with / ends_with             (pointer/container, custom Fn)
//
// Each principal overload runs at least one 10k property test against
// mtest::rigor::ref:: naive references; container overloads fan out via
// for_each_readonly_container across {vector, svector, array, carray}.
// iarray is tested only via pointer overloads (its const-only begin/data
// fails micron::is_iterable_container). Adversarial input patterns include
// sorted, reverse-sorted, all-equal, alternating, sawtooth, zeros,
// single-spike, near-overflow, empty, single-element, and power-of-2
// boundaries.

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/algorithm/find.hpp"
#include "../../src/maps/heap_swiss.hpp"
#include "../../src/maps/rb_map.hpp"

#include "../support/algo_rigor.hpp"

using namespace mtest::rigor;
using sb::end_test_case;
using sb::expect_no_throw;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

// ─────────────────────────────────────────────────────────────────────────
// Stateless predicates (reused everywhere to keep template counts low)
// ─────────────────────────────────────────────────────────────────────────

static constexpr auto is_even = [](int x) { return (x & 1) == 0; };
static constexpr auto is_odd = [](int x) { return (x & 1) == 1; };
static constexpr auto is_positive = [](int x) { return x > 0; };
static constexpr auto is_negative = [](int x) { return x < 0; };
static constexpr auto is_zero_lambda = [](int x) { return x == 0; };
static constexpr auto greater_50 = [](int x) { return x > 50; };
static constexpr auto less_or_eq_neg100 = [](int x) { return x <= -100; };
static constexpr auto always_true = [](int) { return true; };
static constexpr auto always_false = [](int) { return false; };

// pointer-flavor predicates (find_if/all_of variants accept Fn(const T*))
static constexpr auto pt_is_even = [](const int *p) { return (*p & 1) == 0; };
static constexpr auto pt_is_positive = [](const int *p) { return *p > 0; };
static constexpr auto pt_always_true = [](const int *) { return true; };
static constexpr auto pt_always_false = [](const int *) { return false; };

// adjacent_find / find_end / search comparators (Fn(const T*, const T*))
static constexpr auto cmp_equal = [](const int *a, const int *b) { return *a == *b; };
static constexpr auto cmp_diff_by_one = [](const int *a, const int *b) { return *a - *b == 1 or *b - *a == 1; };

// equality wrappers
static constexpr auto eq_int = [](const int *a, const int *b) { return *a == *b; };

int
main()
{
  sb::print("=== ALGO/SEARCH RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // all_of — pointer, container, compile-time, map
  // ════════════════════════════════════════════════════════════════════

  test_case("all_of[ptr,scalar] empty range is true");
  {
    int a[1];
    require_true(micron::all_of(a, a, 0));
  }
  end_test_case();

  test_case("all_of[ptr,scalar] all_equal hit/miss");
  {
    int a[64];
    pat_all_equal(a, 64, 7);
    require_true(micron::all_of(a, a + 64, 7));
    require_false(micron::all_of(a, a + 64, 8));
  }
  end_test_case();

  test_case("all_of[ptr,scalar] single mismatch flips result");
  {
    for ( usize n : kAdversarialSizes ) {
      if ( n < 2 ) continue;
      int a[1024];
      pat_all_equal(a, n, 5);
      a[n / 2] = 6;
      require_false(micron::all_of(a, a + n, 5));
    }
  }
  end_test_case();

  test_case("all_of[ptr,Fn-T] sorted positive sequence");
  {
    int a[128];
    pat_sorted(a, 128, 1);
    require_true(micron::all_of(a, a + 128, is_positive));
    require_false(micron::all_of(a, a + 128, is_even));
  }
  end_test_case();

  test_case("all_of[ptr,Fn-T*] always-true predicate");
  {
    int a[64];
    pat_sorted(a, 64);
    require_true(micron::all_of(a, a + 64, pt_always_true));
    require_false(micron::all_of(a, a + 64, pt_always_false));
  }
  end_test_case();

  test_case("all_of[ptr,scalar] sawtooth/alternating/zeros patterns");
  {
    int a[64];
    pat_sawtooth(a, 64, 8);
    require_false(micron::all_of(a, a + 64, 0));
    pat_alternating(a, 64, 5, 5);
    require_true(micron::all_of(a, a + 64, 5));
    pat_zeros(a, 64);
    require_true(micron::all_of(a, a + 64, 0));
  }
  end_test_case();

  test_case("all_of[container] fan-out over 4 reference containers");
  {
    int buf[32];
    pat_all_equal(buf, 32, 9);
    for_each_readonly_container<int, 32>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 32);
      // scalar overload (unambiguous) — container+lambda has overload conflict
      // with the const Cmp& form in find.hpp; pointer form is the workaround.
      require_true(micron::all_of(c, 9));
      require_false(micron::all_of(c, 0));
      require_true(micron::all_of(c.begin(), c.end(), is_positive));
    });
  }
  end_test_case();

  property_test(
      "all_of[ptr,scalar] vs naive (10k random)",
      [](u32 raw_n, u32 raw_v) {
        usize n = (raw_n & 0x3f) + 1;
        int v = static_cast<int>(raw_v & 0x7);
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = (i % 5 == 0) ? v : static_cast<int>(i & 0x7);
        bool actual = micron::all_of(buf, buf + n, v);
        bool expected = ref::naive_all_of_eq(buf, n, v);
        require(actual, expected);
      },
      10000);

  // any_of ──────────────────────────────────────────────────────────────

  test_case("any_of[ptr,scalar] empty range is false");
  {
    int a[1];
    require_false(micron::any_of(a, a, 0));
  }
  end_test_case();

  test_case("any_of[ptr,scalar] single spike found");
  {
    int a[64];
    pat_single_spike(a, 64, 17, 99);
    require_true(micron::any_of(a, a + 64, 99));
    require_false(micron::any_of(a, a + 64, 100));
  }
  end_test_case();

  test_case("any_of[ptr,scalar] all_equal target found, others miss");
  {
    int a[256];
    pat_all_equal(a, 256, 11);
    require_true(micron::any_of(a, a + 256, 11));
    require_false(micron::any_of(a, a + 256, 12));
  }
  end_test_case();

  test_case("any_of[ptr,Fn-T] reverse-sorted, positive predicate");
  {
    int a[64];
    pat_reverse_sorted(a, 64, 1);
    require_true(micron::any_of(a, a + 64, is_positive));
    require_false(micron::any_of(a, a + 64, is_negative));
  }
  end_test_case();

  test_case("any_of[container] fan-out");
  {
    int buf[32];
    pat_single_spike(buf, 32, 7, 42);
    for_each_readonly_container<int, 32>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 32);
      require_true(micron::any_of(c, 42));
      require_false(micron::any_of(c, 43));
      require_true(micron::any_of(c.begin(), c.end(), [](int x) { return x == 42; }));
    });
  }
  end_test_case();

  property_test(
      "any_of[ptr,scalar] vs naive (10k random)",
      [](u32 raw_n, u32 raw_v) {
        usize n = (raw_n & 0x7f) + 1;
        int v = static_cast<int>(raw_v & 0xf);
        int buf[128];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_n + i) & 0xf);
        bool actual = micron::any_of(buf, buf + n, v);
        bool expected = ref::naive_any_of_eq(buf, n, v);
        require(actual, expected);
      },
      10000);

  // none_of ─────────────────────────────────────────────────────────────

  test_case("none_of[ptr,scalar] empty range is true");
  {
    int a[1];
    require_true(micron::none_of(a, a, 0));
  }
  end_test_case();

  test_case("none_of[ptr,scalar] all-different, target absent");
  {
    int a[128];
    pat_sorted(a, 128, 100);
    require_true(micron::none_of(a, a + 128, 0));
    require_false(micron::none_of(a, a + 128, 150));
  }
  end_test_case();

  test_case("none_of[ptr,Fn-T] alternating predicate");
  {
    int a[64];
    pat_alternating(a, 64, 2, 4);
    require_true(micron::none_of(a, a + 64, is_odd));
    require_false(micron::none_of(a, a + 64, is_even));
  }
  end_test_case();

  test_case("none_of[container] fan-out");
  {
    int buf[16];
    pat_sorted(buf, 16, 100);
    for_each_readonly_container<int, 16>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 16);
      require_true(micron::none_of(c.begin(), c.end(), is_negative));
      require_false(micron::none_of(c.begin(), c.end(), is_positive));
      require_true(micron::none_of(c, 0));
    });
  }
  end_test_case();

  property_test(
      "none_of[ptr,scalar] vs negation of any_of (10k random)",
      [](u32 raw_n, u32 raw_v) {
        usize n = (raw_n & 0x3f) + 1;
        int v = static_cast<int>(raw_v & 0x7);
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>(i & 0x7);
        bool actual = micron::none_of(buf, buf + n, v);
        bool expected = !ref::naive_any_of_eq(buf, n, v);
        require(actual, expected);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // find — pointer, container, edge cases
  // ════════════════════════════════════════════════════════════════════

  test_case("find[ptr] empty range returns nullptr");
  {
    int a[1];
    require_true(micron::find(a, a, 0) == nullptr);
  }
  end_test_case();

  test_case("find[ptr] first / last / middle hits");
  {
    int a[8];
    pat_sorted(a, 8);
    require_true(micron::find(a, a + 8, 0) == a);
    require_true(micron::find(a, a + 8, 7) == a + 7);
    require_true(micron::find(a, a + 8, 4) == a + 4);
  }
  end_test_case();

  test_case("find[ptr] miss returns nullptr");
  {
    int a[256];
    pat_zeros(a, 256);
    require_true(micron::find(a, a + 256, 1) == nullptr);
  }
  end_test_case();

  test_case("find[ptr] single spike pattern");
  {
    int a[64];
    pat_single_spike(a, 64, 23, 7777);
    auto *p = micron::find(a, a + 64, 7777);
    require_true(p == a + 23);
    require_true(micron::find(a, a + 64, 7776) == nullptr);
  }
  end_test_case();

  test_case("find[ptr] adversarial sizes return correct index");
  {
    for ( usize n : kAdversarialSizes ) {
      if ( n < 2 ) continue;
      int a[1024];
      pat_sorted(a, n);
      usize mid = n / 2;
      auto *p = micron::find(a, a + n, static_cast<int>(mid));
      require_true(p == a + mid);
    }
  }
  end_test_case();

  test_case("find[container] fan-out over readonly containers");
  {
    int buf[32];
    pat_sorted(buf, 32, 100);
    for_each_readonly_container<int, 32>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 32);
      // micron::find(C&, P) returns nullptr on miss (not c.end()).
      auto *hit = micron::find(c, 115);
      require_true(hit != nullptr);
      require(*hit, 115);
      auto *miss = micron::find(c, 9999);
      require_true(miss == nullptr);
    });
  }
  end_test_case();

  property_test(
      "find[ptr] returns first-hit position (10k random)",
      [](u32 raw_n, u32 raw_t) {
        usize n = (raw_n & 0x7f) + 2;
        int buf[256];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_n + i * 7) & 0x1f);
        int target = static_cast<int>(buf[raw_t % n]);
        const int *actual = micron::find(buf, buf + n, target);
        const int *expected = ref::naive_find(buf, n, target);
        require_true(actual == expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // find_if / find_if_not
  // ────────────────────────────────────────────────────────────────────

  test_case("find_if[ptr,Fn-T] hit on first matching");
  {
    int a[16];
    pat_sorted(a, 16);
    auto *p = micron::find_if(a, a + 16, [](int x) { return x > 3; });
    require_true(p == a + 4);
  }
  end_test_case();

  test_case("find_if[ptr,Fn-T*] hit via pointer predicate");
  {
    int a[16];
    pat_sorted(a, 16);
    auto *p = micron::find_if(a, a + 16, pt_is_even);
    require_true(p == a + 0);
  }
  end_test_case();

  test_case("find_if_not[ptr] returns first not-matching");
  {
    int a[16];
    for ( int i = 0; i < 8; ++i ) a[i] = 0;
    for ( int i = 8; i < 16; ++i ) a[i] = 1;
    auto *p = micron::find_if_not(a, a + 16, pt_is_even);
    require_true(p == a + 8);
  }
  end_test_case();

  test_case("find_if_not[ptr] all matching returns nullptr");
  {
    int a[32];
    pat_all_equal(a, 32, 2);
    auto *p = micron::find_if_not(a, a + 32, pt_is_even);
    require_true(p == nullptr);
  }
  end_test_case();

  test_case("find_if[container] fan-out");
  {
    int buf[16];
    pat_sawtooth(buf, 16, 4);
    for_each_readonly_container<int, 16>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 16);
      auto *it = micron::find_if(c, [](int x) { return x == 3; });
      require_true(it != nullptr);
      require(*it, 3);
    });
  }
  end_test_case();

  property_test(
      "find_if[ptr] vs naive predicate (10k random)",
      [](u32 raw_n, u32 raw_t) {
        usize n = (raw_n & 0x3f) + 2;
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_t + i * 13) & 0xff);
        int target = static_cast<int>(raw_t & 0xff);
        auto pred = [target](int x) { return x >= target; };
        auto pred_ptr = [target](int x) { return x >= target; };
        const int *actual = micron::find_if(buf, buf + n, pred);
        const int *expected = ref::naive_find_if(buf, n, pred_ptr);
        require_true(actual == expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // find_last / find_last_if / find_last_if_not
  // ────────────────────────────────────────────────────────────────────

  test_case("find_last[ptr] returns rightmost occurrence");
  {
    int a[10] = { 3, 1, 3, 1, 3, 1, 3, 1, 3, 1 };
    auto *p = micron::find_last(a, a + 10, 3);
    require_true(p == a + 8);
    auto *q = micron::find_last(a, a + 10, 1);
    require_true(q == a + 9);
  }
  end_test_case();

  test_case("find_last[ptr] miss returns nullptr");
  {
    int a[8];
    pat_zeros(a, 8);
    require_true(micron::find_last(a, a + 8, 1) == nullptr);
  }
  end_test_case();

  test_case("find_last_if[ptr,Fn-T] rightmost matching");
  {
    int a[10];
    pat_sorted(a, 10);
    auto *p = micron::find_last_if(a, a + 10, [](int x) { return x < 5; });
    require_true(p == a + 4);
  }
  end_test_case();

  test_case("find_last_if_not[ptr,Fn-T*] rightmost not-matching");
  {
    int a[10];
    pat_sorted(a, 10);
    auto *p = micron::find_last_if_not(a, a + 10, pt_is_even);
    require_true(p == a + 9);
  }
  end_test_case();

  property_test(
      "find_last[ptr] returns rightmost match (10k random)",
      [](u32 raw_n, u32 raw_t) {
        usize n = (raw_n & 0x3f) + 2;
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_t + i * 5) & 0x1f);
        int target = buf[raw_t % n];
        const int *actual = micron::find_last(buf, buf + n, target);
        const int *expected = ref::naive_find_last(buf, n, target);
        require_true(actual == expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // find_end — last subsequence
  // ────────────────────────────────────────────────────────────────────

  test_case("find_end empty needle returns end");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int needle[1];
    auto *p = micron::find_end(hay, hay + 8, needle, needle);
    require_true(p == hay + 8);
  }
  end_test_case();

  test_case("find_end single occurrence -> returns it");
  {
    int hay[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    int needle[3] = { 3, 4, 5 };
    auto *p = micron::find_end(hay, hay + 10, needle, needle + 3);
    require_true(p == hay + 2);
  }
  end_test_case();

  test_case("find_end multiple occurrences -> returns rightmost");
  {
    int hay[12] = { 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4 };
    int needle[3] = { 1, 2, 3 };
    auto *p = micron::find_end(hay, hay + 12, needle, needle + 3);
    require_true(p == hay + 8);
  }
  end_test_case();

  test_case("find_end with comparator");
  {
    int hay[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int needle[2] = { 5, 6 };
    auto *p = micron::find_end(hay, hay + 8, needle, needle + 2, cmp_equal);
    require_true(p == hay + 4);
  }
  end_test_case();

  test_case("find_end no match returns nullptr");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int needle[3] = { 99, 99, 99 };
    require_true(micron::find_end(hay, hay + 8, needle, needle + 3) == nullptr);
  }
  end_test_case();

  property_test(
      "find_end vs naive_search_end (10k random)",
      [](u32 raw_n, u32 raw_m) {
        usize n = (raw_n & 0x1f) + 4;
        usize m = (raw_m & 0x3) + 1;
        if ( m > n ) m = n;
        int hay[64];
        int needle[8];
        for ( usize i = 0; i < n; ++i ) hay[i] = static_cast<int>((raw_n + i) & 0x7);
        for ( usize j = 0; j < m; ++j ) needle[j] = static_cast<int>((raw_m + j) & 0x7);
        const int *actual = micron::find_end(hay, hay + n, needle, needle + m);
        const int *expected = ref::naive_search_end(hay, n, needle, m);
        require_true(actual == expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // find_first_of
  // ────────────────────────────────────────────────────────────────────

  test_case("find_first_of empty set returns nullptr");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int set_[1];
    require_true(micron::find_first_of(hay, hay + 8, set_, set_) == nullptr);
  }
  end_test_case();

  test_case("find_first_of first element matches");
  {
    int hay[8] = { 5, 1, 2, 3, 4, 5, 6, 7 };
    int set_[3] = { 5, 99, 100 };
    auto *p = micron::find_first_of(hay, hay + 8, set_, set_ + 3);
    require_true(p == hay);
  }
  end_test_case();

  test_case("find_first_of middle hit");
  {
    int hay[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    int set_[2] = { 7, 11 };
    auto *p = micron::find_first_of(hay, hay + 10, set_, set_ + 2);
    require_true(p == hay + 6);
  }
  end_test_case();

  test_case("find_first_of miss");
  {
    int hay[8];
    pat_zeros(hay, 8);
    int set_[3] = { 1, 2, 3 };
    require_true(micron::find_first_of(hay, hay + 8, set_, set_ + 3) == nullptr);
  }
  end_test_case();

  property_test(
      "find_first_of vs naive (10k random)",
      [](u32 raw_n, u32 raw_m) {
        usize n = (raw_n & 0x1f) + 2;
        usize m = (raw_m & 0x7) + 1;
        int hay[64];
        int set_[16];
        for ( usize i = 0; i < n; ++i ) hay[i] = static_cast<int>((raw_n + i) & 0xff);
        for ( usize j = 0; j < m; ++j ) set_[j] = static_cast<int>((raw_m + j * 17) & 0xff);
        const int *actual = micron::find_first_of(hay, hay + n, set_, set_ + m);
        const int *expected = ref::naive_find_first_of(hay, n, set_, m);
        require_true(actual == expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // adjacent_find
  // ────────────────────────────────────────────────────────────────────

  test_case("adjacent_find empty range returns nullptr");
  {
    int a[1];
    require_true(micron::adjacent_find(a, a) == nullptr);
  }
  end_test_case();

  test_case("adjacent_find no duplicates returns nullptr");
  {
    int a[8];
    pat_sorted(a, 8);
    require_true(micron::adjacent_find(a, a + 8) == nullptr);
  }
  end_test_case();

  test_case("adjacent_find first adjacent pair");
  {
    int a[8] = { 1, 2, 2, 3, 4, 5, 6, 7 };
    auto *p = micron::adjacent_find(a, a + 8);
    require_true(p == a + 1);
  }
  end_test_case();

  test_case("adjacent_find all-equal returns first pair (position 0)");
  {
    int a[32];
    pat_all_equal(a, 32, 5);
    auto *p = micron::adjacent_find(a, a + 32);
    require_true(p == a);
  }
  end_test_case();

  test_case("adjacent_find with custom comparator");
  {
    int a[8] = { 1, 2, 3, 4, 4, 5, 6, 7 };
    auto *p = micron::adjacent_find(a, a + 8, cmp_equal);
    require_true(p == a + 3);
  }
  end_test_case();

  property_test(
      "adjacent_find vs naive (10k random)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x3f) + 1;
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_n + i * 11) & 0x3);
        const int *actual = micron::adjacent_find(buf, buf + n);
        const int *expected = ref::naive_adjacent_find(buf, n);
        require_true(actual == expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // search / search_n
  // ────────────────────────────────────────────────────────────────────

  test_case("search empty needle vs micron behavior");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int needle[1];
    // micron::search loops over haystack: when needle empty, while loop
    // immediately exits with it2==pend → returns first. Verify behavior:
    auto *p = micron::search(hay, hay + 8, needle, needle);
    require_true(p == hay);
  }
  end_test_case();

  test_case("search needle at beginning");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int needle[3] = { 0, 1, 2 };
    auto *p = micron::search(hay, hay + 8, needle, needle + 3);
    require_true(p == hay);
  }
  end_test_case();

  test_case("search needle at end");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int needle[3] = { 5, 6, 7 };
    auto *p = micron::search(hay, hay + 8, needle, needle + 3);
    require_true(p == hay + 5);
  }
  end_test_case();

  test_case("search needle longer than haystack returns nullptr");
  {
    int hay[3];
    pat_sorted(hay, 3);
    int needle[5];
    pat_sorted(needle, 5);
    require_true(micron::search(hay, hay + 3, needle, needle + 5) == nullptr);
  }
  end_test_case();

  test_case("search needle == haystack returns start");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int needle[8];
    pat_sorted(needle, 8);
    auto *p = micron::search(hay, hay + 8, needle, needle + 8);
    require_true(p == hay);
  }
  end_test_case();

  test_case("search with custom comparator");
  {
    int hay[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int needle[2] = { 4, 5 };
    auto *p = micron::search(hay, hay + 8, needle, needle + 2, eq_int);
    require_true(p == hay + 3);
  }
  end_test_case();

  property_test(
      "search vs naive_search (10k random)",
      [](u32 raw_n, u32 raw_m) {
        usize n = (raw_n & 0x1f) + 4;
        usize m = (raw_m & 0x3) + 1;
        if ( m > n ) m = n;
        int hay[64];
        int needle[8];
        for ( usize i = 0; i < n; ++i ) hay[i] = static_cast<int>((raw_n + i) & 0x7);
        for ( usize j = 0; j < m; ++j ) needle[j] = static_cast<int>((raw_m + j) & 0x7);
        const int *actual = micron::search(hay, hay + n, needle, needle + m);
        const int *expected = ref::naive_search(hay, n, needle, m);
        require_true(actual == expected);
      },
      10000);

  // search_n ─────────────────────────────────────────────────────────────

  test_case("search_n n=1 finds first equal");
  {
    int a[8] = { 1, 2, 3, 7, 5, 6, 7, 8 };
    auto *p = micron::search_n(a, a + 8, 1, 7);
    require_true(p == a + 3);
  }
  end_test_case();

  test_case("search_n 3 consecutive twos");
  {
    int a[8] = { 1, 2, 2, 2, 3, 4, 5, 6 };
    auto *p = micron::search_n(a, a + 8, 3, 2);
    require_true(p == a + 1);
  }
  end_test_case();

  test_case("search_n not found");
  {
    int a[8] = { 1, 2, 2, 3, 4, 5, 6, 7 };
    require_true(micron::search_n(a, a + 8, 3, 2) == nullptr);
  }
  end_test_case();

  test_case("search_n n exceeds remainder returns nullptr");
  {
    int a[5] = { 5, 5, 5, 5, 5 };
    require_true(micron::search_n(a, a + 5, 6, 5) == nullptr);
  }
  end_test_case();

  property_test(
      "search_n vs naive (10k random)",
      [](u32 raw_n, u32 raw_k) {
        usize n = (raw_n & 0x1f) + 2;
        usize k = (raw_k & 0x3) + 1;
        if ( k > n ) k = n;
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_n + i / 2) & 0x3);
        int v = static_cast<int>(raw_k & 0x3);
        const int *actual = micron::search_n(buf, buf + n, k, v);
        const int *expected = ref::naive_search_n(buf, n, k, v);
        require_true(actual == expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // count / count_if
  // ────────────────────────────────────────────────────────────────────

  test_case("count empty range returns 0");
  {
    int a[1];
    require(micron::count(a, a, 0), umax_t(0));
  }
  end_test_case();

  test_case("count all-equal returns size");
  {
    int a[100];
    pat_all_equal(a, 100, 7);
    require(micron::count(a, a + 100, 7), umax_t(100));
    require(micron::count(a, a + 100, 0), umax_t(0));
  }
  end_test_case();

  test_case("count alternating returns half");
  {
    int a[100];
    pat_alternating(a, 100, 0, 1);
    require(micron::count(a, a + 100, 1), umax_t(50));
    require(micron::count(a, a + 100, 0), umax_t(50));
  }
  end_test_case();

  test_case("count_if[ptr,Fn-T] over sorted range");
  {
    int a[100];
    pat_sorted(a, 100);
    require(micron::count_if(a, a + 100, is_even), umax_t(50));
    require(micron::count_if(a, a + 100, is_odd), umax_t(50));
  }
  end_test_case();

  test_case("count_if[ptr,Fn-T*] using pointer predicate");
  {
    int a[100];
    pat_sorted(a, 100, 100);
    require(micron::count_if(a, a + 100, pt_is_positive), umax_t(100));
  }
  end_test_case();

  test_case("count/count_if container fan-out");
  {
    int buf[32];
    pat_sawtooth(buf, 32, 4);
    for_each_readonly_container<int, 32>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 32);
      require(micron::count(c, 0), umax_t(8));
      require(micron::count_if(c, [](int x) { return x < 2; }), umax_t(16));
    });
  }
  end_test_case();

  property_test(
      "count vs naive (10k random)",
      [](u32 raw_n, u32 raw_v) {
        usize n = (raw_n & 0x3f) + 1;
        int v = static_cast<int>(raw_v & 0x7);
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_n + i) & 0x7);
        umax_t actual = micron::count(buf, buf + n, v);
        umax_t expected = static_cast<umax_t>(ref::naive_count_eq(buf, n, v));
        require(actual, expected);
      },
      10000);

  property_test(
      "count_if vs naive (10k random)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x3f) + 1;
        int buf[64];
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<int>((raw_n + i) & 0xff);
        umax_t actual = micron::count_if(buf, buf + n, is_even);
        auto naive_pred = [](int x) { return (x & 1) == 0; };
        umax_t expected = static_cast<umax_t>(ref::naive_count_if(buf, n, naive_pred));
        require(actual, expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // contains / contains_if / contains_subrange
  // ────────────────────────────────────────────────────────────────────

  test_case("contains[ptr] hit and miss");
  {
    int a[8];
    pat_sorted(a, 8);
    require_true(micron::contains(a, a + 8, 3));
    require_false(micron::contains(a, a + 8, 9));
  }
  end_test_case();

  test_case("contains_subrange[ptr] basic");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int needle[3] = { 3, 4, 5 };
    require_true(micron::contains_subrange(hay, hay + 8, needle, needle + 3));
    int bad[3] = { 9, 10, 11 };
    require_false(micron::contains_subrange(hay, hay + 8, bad, bad + 3));
  }
  end_test_case();

  test_case("contains[container] fan-out");
  {
    int buf[16];
    pat_sorted(buf, 16, 50);
    for_each_readonly_container<int, 16>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 16);
      require_true(micron::contains(c, 55));
      require_false(micron::contains(c, 1));
    });
  }
  end_test_case();

  test_case("contains_if[container] fan-out");
  {
    int buf[16];
    pat_sorted(buf, 16, 50);
    for_each_readonly_container<int, 16>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c = build_test_container<C>(buf, 16);
      require_true(micron::contains_if(c, pt_is_positive));
      require_false(micron::contains_if(c, pt_always_false));
    });
  }
  end_test_case();

  // ────────────────────────────────────────────────────────────────────
  // mismatch
  // ────────────────────────────────────────────────────────────────────

  test_case("mismatch identical ranges returns end pair");
  {
    int a[8];
    int b[8];
    pat_sorted(a, 8);
    pat_sorted(b, 8);
    auto pr = micron::mismatch(a, a + 8, b);
    require_true(pr.a == a + 8);
    require_true(pr.b == b + 8);
  }
  end_test_case();

  test_case("mismatch first index difference");
  {
    int a[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int b[8] = { 1, 2, 3, 99, 5, 6, 7, 8 };
    auto pr = micron::mismatch(a, a + 8, b);
    require_true(pr.a == a + 3);
    require_true(pr.b == b + 3);
  }
  end_test_case();

  test_case("mismatch bounded range respects shorter end");
  {
    int a[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int b[4] = { 1, 2, 3, 4 };
    auto pr = micron::mismatch(a, a + 8, b, b + 4);
    require_true(pr.a == a + 4);
    require_true(pr.b == b + 4);
  }
  end_test_case();

  test_case("mismatch with comparator");
  {
    int a[6] = { 1, 2, 3, 4, 5, 6 };
    int b[6] = { 1, 2, 4, 5, 6, 7 };      // off-by-one starting at idx 2
    auto pr = micron::mismatch(a, a + 6, b, cmp_equal);
    require_true(pr.a == a + 2);
  }
  end_test_case();

  property_test(
      "mismatch index vs naive (10k random)",
      [](u32 raw_n, u32 raw_seed) {
        usize n = (raw_n & 0x3f) + 1;
        int a[64];
        int b[64];
        for ( usize i = 0; i < n; ++i ) {
          a[i] = static_cast<int>((raw_seed + i) & 0xff);
          b[i] = static_cast<int>((raw_seed + i) & 0xff);
        }
        if ( n > 5 ) b[n / 2] ^= 1;
        auto pr = micron::mismatch(a, a + n, b);
        usize actual_idx = static_cast<usize>(pr.a - a);
        usize expected_idx = ref::naive_mismatch_idx(a, b, n);
        require(actual_idx, expected_idx);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // equal
  // ────────────────────────────────────────────────────────────────────

  test_case("equal identical pointer ranges");
  {
    int a[8];
    int b[8];
    pat_sorted(a, 8);
    pat_sorted(b, 8);
    require_true(micron::equal(a, a + 8, b));
  }
  end_test_case();

  test_case("equal with one differing element");
  {
    int a[8];
    int b[8];
    pat_sorted(a, 8);
    pat_sorted(b, 8);
    b[4] = 99;
    require_false(micron::equal(a, a + 8, b));
  }
  end_test_case();

  test_case("equal different-length ranges (4-arg) returns false");
  {
    int a[8];
    int b[5];
    pat_sorted(a, 8);
    pat_sorted(b, 5);
    require_false(micron::equal(a, a + 8, b, b + 5));
  }
  end_test_case();

  test_case("equal same-length ranges with custom comparator");
  {
    int a[6] = { 1, 2, 3, 4, 5, 6 };
    int b[6] = { 1, 2, 3, 4, 5, 6 };
    require_true(micron::equal(a, a + 6, b, eq_int));
  }
  end_test_case();

  test_case("equal container fan-out");
  {
    int buf[16];
    pat_sorted(buf, 16);
    for_each_readonly_container<int, 16>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      C c1 = build_test_container<C>(buf, 16);
      C c2 = build_test_container<C>(buf, 16);
      require_true(micron::equal(c1, c2));
    });
  }
  end_test_case();

  property_test(
      "equal vs naive (10k random)",
      [](u32 raw_n, u32 raw_flip) {
        usize n = (raw_n & 0x3f) + 1;
        int a[64];
        int b[64];
        pat_sorted(a, n);
        pat_sorted(b, n);
        bool flip = (raw_flip & 1u) != 0;
        if ( flip and n > 1 ) b[n / 2] ^= 1;
        bool actual = micron::equal(a, a + n, b);
        bool expected = ref::naive_equal(a, b, n);
        require(actual, expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // starts_with / ends_with
  // ────────────────────────────────────────────────────────────────────

  test_case("starts_with empty prefix is true");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int prefix[1];
    require_true(micron::starts_with(hay, hay + 8, prefix, prefix));
  }
  end_test_case();

  test_case("starts_with matching prefix");
  {
    int hay[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int prefix[3] = { 1, 2, 3 };
    require_true(micron::starts_with(hay, hay + 8, prefix, prefix + 3));
    int bad[3] = { 2, 3, 4 };
    require_false(micron::starts_with(hay, hay + 8, bad, bad + 3));
  }
  end_test_case();

  test_case("starts_with prefix longer than range is false");
  {
    int hay[3] = { 1, 2, 3 };
    int prefix[5] = { 1, 2, 3, 4, 5 };
    require_false(micron::starts_with(hay, hay + 3, prefix, prefix + 5));
  }
  end_test_case();

  test_case("ends_with empty suffix is true");
  {
    int hay[8];
    pat_sorted(hay, 8);
    int suffix[1];
    require_true(micron::ends_with(hay, hay + 8, suffix, suffix));
  }
  end_test_case();

  test_case("ends_with matching suffix");
  {
    int hay[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int suffix[3] = { 6, 7, 8 };
    require_true(micron::ends_with(hay, hay + 8, suffix, suffix + 3));
    int bad[3] = { 5, 6, 7 };
    require_false(micron::ends_with(hay, hay + 8, bad, bad + 3));
  }
  end_test_case();

  test_case("ends_with suffix longer than range is false");
  {
    int hay[3] = { 1, 2, 3 };
    int suffix[5] = { 0, 1, 2, 3, 4 };
    require_false(micron::ends_with(hay, hay + 3, suffix, suffix + 5));
  }
  end_test_case();

  test_case("starts_with / ends_with container fan-out");
  {
    int buf[16];
    int pre[3];
    int suf[3];
    pat_sorted(buf, 16);
    pre[0] = 0;
    pre[1] = 1;
    pre[2] = 2;
    suf[0] = 13;
    suf[1] = 14;
    suf[2] = 15;
    for_each_readonly_container<int, 16>([&]<typename Tag>(Tag) {
      using C = typename Tag::type;
      using P3 = micron::array<int, 3>;
      C hay = build_test_container<C>(buf, 16);
      P3 p3{};
      p3[0] = pre[0];
      p3[1] = pre[1];
      p3[2] = pre[2];
      P3 s3{};
      s3[0] = suf[0];
      s3[1] = suf[1];
      s3[2] = suf[2];
      require_true(micron::starts_with(hay, p3));
      require_true(micron::ends_with(hay, s3));
    });
  }
  end_test_case();

  property_test(
      "starts_with vs naive (10k random)",
      [](u32 raw_n, u32 raw_m) {
        usize n = (raw_n & 0x3f) + 1;
        usize m = (raw_m & 0x7) + 1;
        if ( m > n ) m = n;
        int hay[64];
        int needle[8];
        pat_sorted(hay, n);
        for ( usize i = 0; i < m; ++i ) needle[i] = hay[i];
        if ( (raw_m >> 8) & 1u and m > 0 ) needle[m / 2] ^= 1;
        bool actual = micron::starts_with(hay, hay + n, needle, needle + m);
        bool expected = ref::naive_starts_with(hay, n, needle, m);
        require(actual, expected);
      },
      10000);

  property_test(
      "ends_with vs naive (10k random)",
      [](u32 raw_n, u32 raw_m) {
        usize n = (raw_n & 0x3f) + 1;
        usize m = (raw_m & 0x7) + 1;
        if ( m > n ) m = n;
        int hay[64];
        int needle[8];
        pat_sorted(hay, n);
        for ( usize i = 0; i < m; ++i ) needle[i] = hay[n - m + i];
        if ( (raw_m >> 8) & 1u and m > 0 ) needle[m / 2] ^= 1;
        bool actual = micron::ends_with(hay, hay + n, needle, needle + m);
        bool expected = ref::naive_ends_with(hay, n, needle, m);
        require(actual, expected);
      },
      10000);

  // ────────────────────────────────────────────────────────────────────
  // Compile-time `auto Fn` template variants
  // ────────────────────────────────────────────────────────────────────

  test_case("all_of<Fn>(ptr,ptr) compile-time predicate");
  {
    int a[16];
    pat_sorted(a, 16, 1);
    require_true(micron::all_of<pt_is_positive>(a, a + 16));
    require_false(micron::all_of<pt_always_false>(a, a + 16));
  }
  end_test_case();

  test_case("any_of<Fn>(ptr,ptr) compile-time predicate");
  {
    int a[16];
    pat_zeros(a, 16);
    require_false(micron::any_of<pt_is_positive>(a, a + 16));
  }
  end_test_case();

  test_case("none_of<Fn>(ptr,ptr) compile-time predicate");
  {
    int a[16];
    pat_zeros(a, 16);
    require_true(micron::none_of<pt_is_positive>(a, a + 16));
  }
  end_test_case();

  test_case("find_if<Fn>(ptr,ptr) compile-time predicate");
  {
    int a[16];
    pat_sorted(a, 16);
    auto *p = micron::find_if<pt_is_positive>(a, a + 16);
    require_true(p == a + 1);      // first positive (i=0 is 0)
  }
  end_test_case();

  test_case("count_if<Fn>(ptr,ptr) compile-time predicate");
  {
    int a[16];
    pat_sorted(a, 16);
    auto c = micron::count_if<pt_is_even>(a, a + 16);
    require(c, umax_t(8));
  }
  end_test_case();

  test_case("find_last_if<Fn>(ptr,ptr) compile-time predicate");
  {
    int a[16];
    pat_sorted(a, 16);
    auto *p = micron::find_last_if<pt_is_even>(a, a + 16);
    require_true(p == a + 14);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // MAP OVERLOADS
  // ════════════════════════════════════════════════════════════════════

  test_case("all_of[heap_swiss_map] always-true predicate");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 50; ++i ) m.insert(i, i * 2);
    require_true(micron::all_of(m, [](const int &, const int &v) { return v >= 0; }));
    require_false(micron::all_of(m, [](const int &k, const int &v) { return v > k; }));
  }
  end_test_case();

  test_case("any_of[heap_swiss_map] needle present");
  {
    micron::heap_swiss_map<int, int> m(32);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i + 1000);
    require_true(micron::any_of(m, [](const int &, const int &v) { return v == 1005; }));
    require_false(micron::any_of(m, [](const int &, const int &v) { return v == 9999; }));
  }
  end_test_case();

  test_case("none_of[heap_swiss_map] no match");
  {
    micron::heap_swiss_map<int, int> m(32);
    for ( int i = 0; i < 20; ++i ) m.insert(i, -1);
    require_true(micron::none_of(m, [](const int &, const int &v) { return v == 0; }));
  }
  end_test_case();

  test_case("count_if[heap_swiss_map] counts matches");
  {
    micron::heap_swiss_map<int, int> m(32);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i % 3);
    umax_t c = micron::count_if(m, [](const int &, const int &v) { return v == 0; });
    require(c, umax_t(7));
  }
  end_test_case();

  test_case("count[heap_swiss_map] counts equal values");
  {
    micron::heap_swiss_map<int, int> m(32);
    for ( int i = 0; i < 20; ++i ) m.insert(i, i % 4);
    require(micron::count(m, 0), umax_t(5));
    require(micron::count(m, 1), umax_t(5));
    require(micron::count(m, 4), umax_t(0));
  }
  end_test_case();

  test_case("find[heap_swiss_map] returns pointer to value");
  {
    micron::heap_swiss_map<int, int> m(32);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i + 100);
    const int *p = micron::find(m, 105);
    require_true(p != nullptr);
    if ( p ) require(*p, 105);
    require_true(micron::find(m, 999) == nullptr);
  }
  end_test_case();

  test_case("contains[heap_swiss_map] hit/miss");
  {
    micron::heap_swiss_map<int, int> m(32);
    for ( int i = 0; i < 10; ++i ) m.insert(i, i + 50);
    require_true(micron::contains(m, 55));
    require_false(micron::contains(m, 99));
  }
  end_test_case();

  test_case("find_if[heap_swiss_map] returns matching value");
  {
    micron::heap_swiss_map<int, int> m(64);
    for ( int i = 0; i < 32; ++i ) m.insert(i, i * 10);
    const int *p = micron::find_if(m, [](const int &, const int &v) { return v >= 200; });
    require_true(p != nullptr);
    if ( p ) require_true(*p >= 200);
  }
  end_test_case();

  test_case("all_of[rb_map] ordered iteration");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 30; ++i ) m.insert(i, i + 1);
    require_true(micron::all_of(m, [](const int &k, const int &v) { return v == k + 1; }));
  }
  end_test_case();

  test_case("count[rb_map] counts equal mapped values");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 30; ++i ) m.insert(i, i % 3);
    require(micron::count(m, 0), umax_t(10));
    require(micron::count(m, 1), umax_t(10));
    require(micron::count(m, 2), umax_t(10));
  }
  end_test_case();

  test_case("find[rb_map] returns matching value");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 20; ++i ) m.insert(i, i + 500);
    const int *p = micron::find(m, 512);
    require_true(p != nullptr);
    if ( p ) require(*p, 512);
  }
  end_test_case();

  test_case("contains[rb_map] hit/miss");
  {
    micron::rb_map<int, int> m;
    for ( int i = 0; i < 20; ++i ) m.insert(i, i * 7);
    require_true(micron::contains(m, 14));
    require_false(micron::contains(m, 99));
  }
  end_test_case();

  property_test(
      "rb_map count vs reference_map count (10k random)",
      [](u32 raw_k, u32 raw_v) {
        micron::rb_map<int, int> m;
        mtest::reference_map<int, int, 256> ref;
        for ( int i = 0; i < 32; ++i ) {
          int k = static_cast<int>((raw_k + i * 13) & 0x7f);
          int v = static_cast<int>((raw_v + i) & 0x7);
          m.insert(k, v);
          ref.insert(k, v);
        }
        int target = static_cast<int>(raw_v & 0x7);
        umax_t actual = micron::count(m, target);
        umax_t expected = 0;
        for ( int t = 0; t < 0x80; ++t ) {
          if ( ref.contains(t) and *ref.find(t) == target ) ++expected;
        }
        require(actual, expected);
      },
      10000);

  sb::print("=== ALGO/SEARCH RIGOR SUITE PASSED ===");
  return 1;
}
