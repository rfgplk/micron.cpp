// algorithm_tests.cpp
// Rigorous snowball test suite for micron algorithm functions
// Reference containers: micron::vector<T> and micron::array<T, N>

#include "../../src/algorithm/algorithm.hpp"
#include "../../src/array/array.hpp"
#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ── helpers ──────────────────────────────────────────────────────────────────

static micron::vector<int>
make_vec(std::initializer_list<int> lst)
{
  return micron::vector<int>(lst);
}

// =============================================================================
int
main()
{
  sb::print("=== ALGORITHM TESTS ===");

  // ── clamp ──────────────────────────────────────────────────────────────────
  test_case("clamp - scalar");
  {
    require(micron::clamp(5, 0, 10), 5);
    require(micron::clamp(-3, 0, 10), 0);
    require(micron::clamp(15, 0, 10), 10);
    require(micron::clamp(0, 0, 10), 0);
    require(micron::clamp(10, 0, 10), 10);
  }
  end_test_case();

  test_case("clamp - custom comparator");
  {
    auto cmp = [](int a, int b) { return a < b; };
    require(micron::clamp(5, 0, 10, cmp), 5);
    require(micron::clamp(-1, 0, 10, cmp), 0);
    require(micron::clamp(99, 0, 10, cmp), 10);
  }
  end_test_case();

  // ── sum ────────────────────────────────────────────────────────────────────
  test_case("sum - integral vector");
  {
    auto v = make_vec({ 1, 2, 3, 4, 5 });
    require(micron::sum(v), umax_t(15));

    auto empty = make_vec({});
    require(micron::sum(empty), umax_t(0));
  }
  end_test_case();

  test_case("sum - integral array");
  {
    micron::array<int, 5> a{ 10, 20, 30, 40, 50 };
    require(micron::sum(a), umax_t(150));
  }
  end_test_case();

  test_case("sum - floating point vector");
  {
    micron::vector<double> v{ 1.5, 2.5, 3.0 };
    // sum should be 7.0 as f128
    auto s = micron::sum(v);
    require_true(s > 6.99 && s < 7.01);
  }
  end_test_case();

  // ── fill ───────────────────────────────────────────────────────────────────
  test_case("fill - container overload");
  {
    auto v = make_vec({ 0, 0, 0, 0 });
    micron::fill(v, 7);
    for ( auto it = v.begin(); it != v.end(); ++it )
      require(*it, 7);
  }
  end_test_case();

  test_case("fill - pointer range");
  {
    micron::array<int, 6> a;
    micron::fill(a.begin(), a.end(), 42);
    for ( size_t i = 0; i < a.size(); ++i )
      require(a[i], 42);
  }
  end_test_case();

  test_case("fill_n - pointer overload");
  {
    micron::array<int, 8> a{};
    micron::fill_n(a.begin(), 4, 99);
    for ( size_t i = 0; i < 4; ++i )
      require(a[i], 99);
    // remainder untouched (default 0)
    for ( size_t i = 4; i < 8; ++i )
      require(a[i], 0);
  }
  end_test_case();

  test_case("fill_n - container overload");
  {
    micron::vector<int> v(8, 0);
    micron::fill_n(v, 5, 3);
    for ( size_t i = 0; i < 5; ++i )
      require(v[i], 3);
  }
  end_test_case();

  // ── contains ───────────────────────────────────────────────────────────────
  test_case("contains - pointer range");
  {
    micron::array<int, 5> a{ 1, 2, 3, 4, 5 };
    require_true(micron::contains(a.begin(), a.end(), 3));
    require_false(micron::contains(a.begin(), a.end(), 99));
  }
  end_test_case();

  test_case("contains - container overload");
  {
    auto v = make_vec({ 10, 20, 30 });
    require_true(micron::contains(v, 20));
    require_false(micron::contains(v, 5));
  }
  end_test_case();

  // ── clear ──────────────────────────────────────────────────────────────────
  test_case("clear - sets all elements to zero/default");
  {
    auto v = make_vec({ 5, 10, 15, 20 });
    micron::clear(v);
    for ( auto it = v.begin(); it != v.end(); ++it )
      require(*it, 0);
  }
  end_test_case();

  // ── mean / geomean / harmonicmean ──────────────────────────────────────────
  test_case("mean - integer vector");
  {
    auto v = make_vec({ 2, 4, 6, 8, 10 });
    double m = micron::mean(v);
    require_true(m > 5.99 && m < 6.01);
  }
  end_test_case();

  test_case("mean - array");
  {
    micron::array<int, 4> a{ 1, 3, 5, 7 };
    double m = micron::mean(a);
    require_true(m > 3.99 && m < 4.01);
  }
  end_test_case();

  test_case("geomean - integer vector");
  {
    // geomean(1,2,4,8) = (64)^(1/4) = 2.828...
    auto v = make_vec({ 1, 2, 4, 8 });
    auto gm = micron::geomean(v);
    require_true(gm > 2.82 && gm < 2.84);
  }
  end_test_case();

  test_case("harmonicmean - integer vector");
  {
    // harmonic mean of {1, 2, 4} = 3 / (1 + 0.5 + 0.25) = ~1.714
    auto v = make_vec({ 1, 2, 4 });
    auto hm = micron::harmonicmean(v);
    require_true(hm > 1.71 && hm < 1.72);
  }
  end_test_case();

  // ── reverse ────────────────────────────────────────────────────────────────
  test_case("reverse - vector");
  {
    auto v = make_vec({ 1, 2, 3, 4, 5 });
    micron::reverse(v);
    int expected[] = { 5, 4, 3, 2, 1 };
    for ( int i = 0; i < 5; ++i )
      require(v[i], expected[i]);
  }
  end_test_case();

  test_case("reverse - array");
  {
    micron::array<int, 4> a{ 10, 20, 30, 40 };
    micron::reverse(a);
    require(a[0], 40);
    require(a[1], 30);
    require(a[2], 20);
    require(a[3], 10);
  }
  end_test_case();

  test_case("reverse - pointer range");
  {
    micron::array<int, 5> a{ 1, 2, 3, 4, 5 };
    micron::reverse(a.begin(), a.end() - 1);
    require(a[0], 5);
    require(a[4], 1);
  }
  end_test_case();

  test_case("reverse - single element is unchanged");
  {
    micron::array<int, 1> a{ 42 };
    micron::reverse(a);
    require(a[0], 42);
  }
  end_test_case();

  // ── transform ──────────────────────────────────────────────────────────────
  test_case("transform - pointer range");
  {
    micron::array<int, 5> a{ 1, 2, 3, 4, 5 };
    micron::transform(a.begin(), a.end(), [](int x) { return x * x; });
    int expected[] = { 1, 4, 9, 16, 25 };
    for ( int i = 0; i < 5; ++i )
      require(a[i], expected[i]);
  }
  end_test_case();

  test_case("transform - container overload");
  {
    auto v = make_vec({ 1, 2, 3, 4, 5 });
    micron::transform(v, [](int x) { return x + 10; });
    for ( int i = 0; i < 5; ++i )
      require(v[i], i + 11);
  }
  end_test_case();

  test_case("transform - identity leaves values unchanged");
  {
    auto v = make_vec({ 7, 8, 9 });
    micron::transform(v, [](int x) { return x; });
    require(v[0], 7);
    require(v[1], 8);
    require(v[2], 9);
  }
  end_test_case();

  // ── find / find_last ───────────────────────────────────────────────────────
  test_case("find - pointer range hit and miss");
  {
    micron::array<int, 5> a{ 10, 20, 30, 40, 50 };
    auto *p = micron::find(a.begin(), a.end(), 30);
    require_true(p != nullptr);
    require(*p, 30);

    auto *q = micron::find(a.begin(), a.end(), 99);
    require_true(q == nullptr);
  }
  end_test_case();

  test_case("find - container overload");
  {
    auto v = make_vec({ 5, 15, 25, 35 });
    auto *p = micron::find(v, 15);
    require_true(p != nullptr);
    require(*p, 15);
  }
  end_test_case();

  test_case("find_last - returns last occurrence");
  {
    micron::array<int, 6> a{ 1, 2, 3, 2, 5, 2 };
    auto *p = micron::find_last(a.begin(), a.end(), 2);
    require_true(p != nullptr);
    // must point to index 5 (last 2)
    require(static_cast<size_t>(p - a.begin()), size_t(5));
  }
  end_test_case();

  test_case("find_last - returns nullptr when not found");
  {
    auto v = make_vec({ 1, 2, 3 });
    int val = 99;
    auto *p = micron::find_last(v, val);
    require_true(p == nullptr);
  }
  end_test_case();

  // ── generate ───────────────────────────────────────────────────────────────
  test_case("generate - nullary functor");
  {
    micron::array<int, 5> a{};
    int counter = 0;
    micron::generate(a.begin(), a.end(), [&]() { return ++counter; });
    for ( size_t i = 0; i < 5; ++i )
      require(a[i], static_cast<int>(i + 1));
  }
  end_test_case();

  test_case("generate - container overload");
  {
    micron::vector<int> v(6, 0);
    int n = 0;
    micron::generate(v, [&]() { return n += 2; });
    for ( int i = 0; i < 6; ++i )
      require(v[i], (i + 1) * 2);
  }
  end_test_case();

  // ── max / min (value) ──────────────────────────────────────────────────────
  test_case("max - vector");
  {
    auto v = make_vec({ 3, 1, 4, 1, 5, 9, 2, 6 });
    require(micron::max(v), 9);
  }
  end_test_case();

  test_case("max - array");
  {
    micron::array<int, 5> a{ 7, 3, 9, 1, 5 };
    require(micron::max(a), 9);
  }
  end_test_case();

  test_case("min - vector");
  {
    auto v = make_vec({ 8, 3, 7, 1, 5 });
    require(micron::min(v), 1);
  }
  end_test_case();

  test_case("min - array");
  {
    micron::array<int, 4> a{ 10, 4, 7, 2 };
    require(micron::min(a), 2);
  }
  end_test_case();

  test_case("max and min on single element");
  {
    auto v = make_vec({ 42 });
    require(micron::max(v), 42);
    require(micron::min(v), 42);
  }
  end_test_case();

  test_case("max and min on all-equal elements");
  {
    micron::array<int, 4> a{ 7, 7, 7, 7 };
    require(micron::max(a), 7);
    require(micron::min(a), 7);
  }
  end_test_case();

  // ── max_at / min_at (iterator) ─────────────────────────────────────────────
  test_case("max_at - returns iterator to maximum");
  {
    auto v = make_vec({ 3, 9, 1, 7, 5 });
    auto it = micron::max_at(v);
    require(*it, 9);
    require(static_cast<size_t>(it - v.begin()), size_t(1));
  }
  end_test_case();

  test_case("min_at - returns iterator to minimum");
  {
    auto v = make_vec({ 3, 9, 1, 7, 5 });
    auto it = micron::min_at(v);
    require(*it, 1);
    require(static_cast<size_t>(it - v.begin()), size_t(2));
  }
  end_test_case();

  // ── all_of / any_of / none_of ──────────────────────────────────────────────
  test_case("all_of - pointer range");
  {
    micron::array<int, 4> a{ 5, 5, 5, 5 };
    require_true(micron::all_of(a.begin(), a.end(), 5));
    require_false(micron::all_of(a.begin(), a.end(), 3));
  }
  end_test_case();

  test_case("all_of - container overload");
  {
    auto v = make_vec({ 2, 2, 2 });
    require_true(micron::all_of(v, 2));

    auto w = make_vec({ 2, 2, 3 });
    require_false(micron::all_of(w, 2));
  }
  end_test_case();

  test_case("any_of - pointer range");
  {
    micron::array<int, 5> a{ 1, 2, 3, 4, 5 };
    require_true(micron::any_of(a.begin(), a.end(), 3));
    require_false(micron::any_of(a.begin(), a.end(), 9));
  }
  end_test_case();

  test_case("any_of - container overload");
  {
    auto v = make_vec({ 10, 20, 30 });
    require_true(micron::any_of(v, 20));
    require_false(micron::any_of(v, 99));
  }
  end_test_case();

  test_case("none_of - pointer range");
  {
    micron::array<int, 4> a{ 1, 2, 3, 4 };
    require_true(micron::none_of(a.begin(), a.end(), 9));
    require_false(micron::none_of(a.begin(), a.end(), 2));
  }
  end_test_case();

  test_case("none_of - container overload");
  {
    auto v = make_vec({ 5, 10, 15 });
    require_true(micron::none_of(v, 99));
    require_false(micron::none_of(v, 10));
  }
  end_test_case();

  // ── count / count_if ───────────────────────────────────────────────────────
  test_case("count - pointer range");
  {
    micron::array<int, 7> a{ 1, 2, 2, 3, 2, 4, 2 };
    require(micron::count(a.begin(), a.end(), 2), umax_t(4));
    require(micron::count(a.begin(), a.end(), 9), umax_t(0));
  }
  end_test_case();

  test_case("count - container overload");
  {
    auto v = make_vec({ 3, 3, 1, 3, 2 });
    require(micron::count(v, 3), umax_t(3));
    require(micron::count(v, 1), umax_t(1));
  }
  end_test_case();

  test_case("count_if - pointer range");
  {
    micron::array<int, 6> a{ 1, 2, 3, 4, 5, 6 };
    auto evens = micron::count_if(a.begin(), a.end(), [](int x) { return x % 2 == 0; });
    require(evens, umax_t(3));
  }
  end_test_case();

  test_case("count_if - container overload");
  {
    auto v = make_vec({ 10, 15, 20, 25, 30 });
    auto gt20 = micron::count_if(v, [](int x) { return x > 20; });
    require(gt20, umax_t(2));
  }
  end_test_case();

  // ── equal ──────────────────────────────────────────────────────────────────
  test_case("equal - pointer range");
  {
    micron::array<int, 4> a{ 1, 2, 3, 4 };
    micron::array<int, 4> b{ 1, 2, 3, 4 };
    micron::array<int, 4> c{ 1, 2, 3, 5 };
    require_true(micron::equal(a.begin(), a.end(), b.begin()));
    require_false(micron::equal(a.begin(), a.end(), c.begin()));
  }
  end_test_case();

  test_case("equal - container overload");
  {
    auto v = make_vec({ 7, 8, 9 });
    auto w = make_vec({ 7, 8, 9 });
    auto x = make_vec({ 7, 8, 0 });
    require_true(micron::equal(v, w));
    require_false(micron::equal(v, x));
  }
  end_test_case();

  // ── search / search_n ──────────────────────────────────────────────────────
  test_case("search - subsequence found");
  {
    auto haystack = make_vec({ 1, 2, 3, 4, 5 });
    auto needle = make_vec({ 3, 4 });
    auto *p = micron::search(haystack, needle);
    require_true(p != nullptr);
    require(*p, 3);
  }
  end_test_case();

  test_case("search - subsequence not found");
  {
    auto haystack = make_vec({ 1, 2, 3, 4, 5 });
    auto needle = make_vec({ 6, 7 });
    auto *p = micron::search(haystack, needle);
    require_true(p == nullptr);
  }
  end_test_case();

  test_case("search_n - n consecutive values found");
  {
    auto v = make_vec({ 1, 2, 2, 2, 3 });
    auto *p = micron::search_n(v, 3, 2);
    require_true(p != nullptr);
    require(static_cast<size_t>(p - v.begin()), size_t(1));
  }
  end_test_case();

  test_case("search_n - n consecutive values not found");
  {
    auto v = make_vec({ 1, 2, 2, 3, 4 });
    auto *p = micron::search_n(v, 3, 2);
    require_true(p == nullptr);
  }
  end_test_case();

  // ── starts_with / ends_with ────────────────────────────────────────────────
  test_case("starts_with - true and false");
  {
    auto v = make_vec({ 1, 2, 3, 4, 5 });
    auto prefix = make_vec({ 1, 2, 3 });
    auto bad = make_vec({ 2, 3 });
    require_true(micron::starts_with(v, prefix));
    require_false(micron::starts_with(v, bad));
  }
  end_test_case();

  test_case("starts_with - prefix longer than container");
  {
    auto v = make_vec({ 1, 2 });
    auto prefix = make_vec({ 1, 2, 3 });
    require_false(micron::starts_with(v, prefix));
  }
  end_test_case();

  test_case("ends_with - true and false");
  {
    auto v = make_vec({ 1, 2, 3, 4, 5 });
    auto suffix = make_vec({ 3, 4, 5 });
    auto bad = make_vec({ 3, 4 });
    require_true(micron::ends_with(v, suffix));
    require_false(micron::ends_with(v, bad));
  }
  end_test_case();

  test_case("ends_with - suffix longer than container");
  {
    auto v = make_vec({ 4, 5 });
    auto suffix = make_vec({ 3, 4, 5 });
    require_false(micron::ends_with(v, suffix));
  }
  end_test_case();

  // ── round / ceil / floor ───────────────────────────────────────────────────
  test_case("round - pointer range");
  {
    micron::array<float, 4> a{ 1.4f, 1.6f, -1.4f, -1.6f };
    micron::round(a.begin(), a.end());
    require(a[0], 1.0f);
    require(a[1], 2.0f);
    require(a[2], -1.0f);
    require(a[3], -2.0f);
  }
  end_test_case();

  test_case("ceil - container overload");
  {
    micron::vector<float> v{ 1.1f, 2.0f, 3.9f };
    micron::ceil(v);
    require(v[0], 2.0f);
    require(v[1], 2.0f);
    require(v[2], 4.0f);
  }
  end_test_case();

  test_case("floor - pointer range");
  {
    micron::array<float, 3> a{ 1.9f, 2.0f, 3.1f };
    micron::floor(a.begin(), a.end());
    require(a[0], 1.0f);
    require(a[1], 2.0f);
    require(a[2], 3.0f);
  }
  end_test_case();

  // ── algorithm composition / stress ────────────────────────────────────────
  test_case("fill → transform → sum pipeline");
  {
    micron::vector<int> v(10, 0);
    micron::fill(v, 3);
    micron::transform(v, [](int x) { return x * x; });     // 9 × 10 = 90
    require(micron::sum(v), umax_t(90));
  }
  end_test_case();

  test_case("generate → reverse → equal roundtrip");
  {
    micron::array<int, 6> a{};
    int n = 0;
    micron::generate(a.begin(), a.end(), [&]() { return ++n; });
    // a = {1,2,3,4,5,6}
    micron::array<int, 6> b(a);     // copy
    micron::reverse(a);
    micron::reverse(a);     // double reverse = identity
    require_true(micron::equal(a.begin(), a.end(), b.begin()));
  }
  end_test_case();

  test_case("find after transform reflects new values");
  {
    auto v = make_vec({ 1, 2, 3, 4, 5 });
    micron::transform(v, [](int x) { return x * 10; });
    auto *p = micron::find(v, 30);
    require_true(p != nullptr);
    require(*p, 30);
    auto *q = micron::find(v, 3);     // old value gone
    require_true(q == nullptr);
  }
  end_test_case();

  test_case("count after fill is consistent");
  {
    micron::vector<int> v(20, 0);
    micron::fill(v, 5);
    require(micron::count(v, 5), umax_t(20));
    require(micron::count(v, 0), umax_t(0));
    require_true(micron::all_of(v, 5));
    require_false(micron::any_of(v, 0));
    require_true(micron::none_of(v, 0));
  }
  end_test_case();

  test_case("stress: transform + count_if on large vector");
  {
    micron::vector<int> v(1000, 0);
    int counter = 0;
    micron::generate(v, [&]() { return counter++; });      // 0..999
    micron::transform(v, [](int x) { return x % 2; });     // 0 or 1
    auto ones = micron::count_if(v, [](int x) { return x == 1; });
    auto zeros = micron::count_if(v, [](int x) { return x == 0; });
    require(ones, umax_t(500));
    require(zeros, umax_t(500));
    require_true(micron::any_of(v, 0));
    require_true(micron::any_of(v, 1));
    require_false(micron::all_of(v, 0));
    require_false(micron::all_of(v, 1));
  }
  end_test_case();

  sb::print("=== ALL ALGORITHM TESTS PASSED ===");
  return 1;
}
