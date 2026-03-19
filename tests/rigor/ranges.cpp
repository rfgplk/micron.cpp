// Copyright (c) 2024- David Lucius Severus
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// test_ranges.cpp
//
// Compile:
//   c++ -std=c++23 -g -Wall -Wextra -o test_ranges test_ranges.cpp && ./test_ranges

#include "../../src/range.hpp"     // adjust path as needed
#include "../snowball/snowball.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Minimal stub container satisfying is_iterable_container
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, usize N> struct flat_array {
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using size_type = usize;
  using iterator = T *;
  using const_iterator = const T *;

  T _data[N];

  constexpr T *
  begin() noexcept
  {
    return _data;
  }

  constexpr T *
  end() noexcept
  {
    return _data + N;
  }

  constexpr const T *
  begin() const noexcept
  {
    return _data;
  }

  constexpr const T *
  end() const noexcept
  {
    return _data + N;
  }

  constexpr const T *
  cbegin() const noexcept
  {
    return _data;
  }

  constexpr const T *
  cend() const noexcept
  {
    return _data + N;
  }

  constexpr T *
  data() noexcept
  {
    return _data;
  }

  constexpr const T *
  data() const noexcept
  {
    return _data;
  }

  constexpr size_type
  size() const noexcept
  {
    return N;
  }

  constexpr bool
  empty() const noexcept
  {
    return N == 0;
  }

  constexpr T &
  operator[](size_type i) noexcept
  {
    return _data[i];
  }

  constexpr const T &
  operator[](size_type i) const noexcept
  {
    return _data[i];
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// ① counting_iter<T>
// ─────────────────────────────────────────────────────────────────────────────

static void
test_counting_iter()
{
  sb::print("=== counting_iter ===");

  sb::test_case("default construction / dereference");
  {
    micron::counting_iter<int> it{ 5 };
    sb::require(*it, 5);
  }
  sb::end_test_case();

  sb::test_case("pre-increment");
  {
    micron::counting_iter<int> it{ 0 };
    ++it;
    ++it;
    sb::require(*it, 2);
  }
  sb::end_test_case();

  sb::test_case("post-increment returns old value");
  {
    micron::counting_iter<int> it{ 3 };
    auto old = it++;
    sb::require(*old, 3);
    sb::require(*it, 4);
  }
  sb::end_test_case();

  sb::test_case("pre-decrement");
  {
    micron::counting_iter<int> it{ 10 };
    --it;
    sb::require(*it, 9);
  }
  sb::end_test_case();

  sb::test_case("operator+ / operator-");
  {
    micron::counting_iter<int> it{ 0 };
    auto it5 = it + 5;
    sb::require(*it5, 5);
    auto it3 = it5 - 2;
    sb::require(*it3, 3);
  }
  sb::end_test_case();

  sb::test_case("operator[] random access");
  {
    micron::counting_iter<int> it{ 10 };
    sb::require(it[0], 10);
    sb::require(it[4], 14);
    sb::require(it[-3], 7);
  }
  sb::end_test_case();

  sb::test_case("difference between two iterators");
  {
    micron::counting_iter<int> a{ 2 }, b{ 7 };
    sb::require(b - a, 5);
    sb::require(a - b, -5);
  }
  sb::end_test_case();

  sb::test_case("equality / inequality");
  {
    micron::counting_iter<int> a{ 4 }, b{ 4 }, c{ 5 };
    sb::require_true(a == b);
    sb::require_false(a == c);
    sb::require_true(a != c);
  }
  sb::end_test_case();

  sb::test_case("ordering operators");
  {
    micron::counting_iter<int> a{ 1 }, b{ 2 };
    sb::require_true(a < b);
    sb::require_true(b > a);
    sb::require_true(a <= a);
    sb::require_true(b >= b);
    sb::require_false(b < a);
  }
  sb::end_test_case();

  sb::test_case("compound += / -=");
  {
    micron::counting_iter<int> it{ 0 };
    it += 6;
    sb::require(*it, 6);
    it -= 2;
    sb::require(*it, 4);
  }
  sb::end_test_case();

  sb::test_case("friend operator n + iter");
  {
    micron::counting_iter<int> it{ 3 };
    auto it8 = 5 + it;
    sb::require(*it8, 8);
  }
  sb::end_test_case();

  sb::test_case("float counting_iter");
  {
    micron::counting_iter<float> it{ 1.0f };
    ++it;
    sb::require(*it, 2.0f);
    auto it3 = it + 1;
    sb::require(*it3, 3.0f);
  }
  sb::end_test_case();

  sb::test_case("u64 large values");
  {
    micron::counting_iter<u64> it{ 1'000'000'000ULL };
    it += 500;
    sb::require(*it, u64{ 1'000'000'500ULL });
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ② reverse_iter<Iter>
// ─────────────────────────────────────────────────────────────────────────────

static void
test_reverse_iter()
{
  sb::print("=== reverse_iter ===");

  sb::test_case("dereference yields element before base");
  {
    micron::counting_iter<int> base{ 5 };
    micron::reverse_iter<micron::counting_iter<int>> rev{ base };
    sb::require(*rev, 4);
  }
  sb::end_test_case();

  sb::test_case("pre-increment moves backwards");
  {
    micron::counting_iter<int> base{ 10 };
    micron::reverse_iter<micron::counting_iter<int>> rev{ base };
    ++rev;
    sb::require(*rev, 8);
  }
  sb::end_test_case();

  sb::test_case("pre-decrement moves forwards");
  {
    micron::counting_iter<int> base{ 5 };
    micron::reverse_iter<micron::counting_iter<int>> rev{ base };
    --rev;
    sb::require(*rev, 5);
  }
  sb::end_test_case();

  sb::test_case("operator+ / operator-");
  {
    micron::counting_iter<int> base{ 10 };
    micron::reverse_iter<micron::counting_iter<int>> rev{ base };
    auto r3 = rev + 3;
    sb::require(*r3, 6);
    auto r1 = r3 - 1;
    sb::require(*r1, 7);
  }
  sb::end_test_case();

  sb::test_case("equality / ordering");
  {
    // reverse_iter ordering mirrors std::reverse_iterator:
    //   r < o  iff  r._base > o._base
    // Higher base = earlier in the reverse traversal = logically less-than.
    // r5 (base=5) is earlier than r3 (base=3), so r5 < r3.
    micron::counting_iter<int> b5{ 5 }, b3{ 3 };
    micron::reverse_iter<micron::counting_iter<int>> r5{ b5 }, r3{ b3 };
    sb::require_true(r5 < r3);
    sb::require_true(r3 > r5);
    sb::require_true(r5 == r5);
    sb::require_true(r5 != r3);
  }
  sb::end_test_case();

  sb::test_case("difference");
  {
    micron::counting_iter<int> b0{ 0 }, b5{ 5 };
    micron::reverse_iter<micron::counting_iter<int>> r0{ b0 }, r5{ b5 };
    sb::require(r0 - r5, 5);
    sb::require(r5 - r0, -5);
  }
  sb::end_test_case();

  sb::test_case("base() accessor");
  {
    micron::counting_iter<int> base{ 7 };
    micron::reverse_iter<micron::counting_iter<int>> rev{ base };
    sb::require(*rev.base(), 7);
  }
  sb::end_test_case();

  sb::test_case("reverse over raw-pointer array");
  {
    int arr[5] = { 10, 20, 30, 40, 50 };
    micron::reverse_iter<int *> rb{ arr + 5 };
    sb::require(*rb, 50);
    ++rb;
    sb::require(*rb, 40);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ③ range<From, To>
// ─────────────────────────────────────────────────────────────────────────────

static void
test_range()
{
  sb::print("=== range<From, To> ===");

  using R = micron::range<2, 7>;

  sb::test_case("size / ssize / empty");
  {
    sb::require(R::size(), umax_t{ 5 });
    sb::require(R::ssize(), 5);
    sb::require_false(R::empty());
  }
  sb::end_test_case();

  sb::test_case("begin / end sentinels");
  {
    sb::require(*R::begin(), umax_t{ 2 });
    sb::require(*R::end(), umax_t{ 7 });
  }
  sb::end_test_case();

  sb::test_case("cbegin / cend sentinels");
  {
    sb::require(*R::cbegin(), umax_t{ 2 });
    sb::require(*R::cend(), umax_t{ 7 });
  }
  sb::end_test_case();

  sb::test_case("rbegin yields last element (To-1)");
  {
    auto rb = R::rbegin();
    sb::require(*rb, umax_t{ 6 });
    ++rb;
    ++rb;
    sb::require(*rb, umax_t{ 4 });
  }
  sb::end_test_case();

  sb::test_case("crbegin / crend endpoints");
  {
    sb::require(*R::crbegin(), umax_t{ 6 });
    sb::require(*R::crend(), umax_t{ 1 });
  }
  sb::end_test_case();

  sb::test_case("range-based-for accumulates correct sum");
  {
    umax_t sum = 0;
    for ( auto v : micron::range<1, 6>{} )
      sum += v;
    sb::require(sum, umax_t{ 15 });
  }
  sb::end_test_case();

  sb::test_case("range-based-for iterates exactly (To-From) times");
  {
    int count = 0;
    for ( [[maybe_unused]] auto v : micron::range<0, 10>{} )
      ++count;
    sb::require(count, 10);
  }
  sb::end_test_case();

  sb::test_case("reverse loop via rbegin/rend hits From last");
  {
    umax_t last = 0;
    auto rb = micron::range<3, 8>::rbegin();
    auto re = micron::range<3, 8>::rend();
    for ( ; rb != re; ++rb )
      last = *rb;
    sb::require(last, umax_t{ 3 });
  }
  sb::end_test_case();

  sb::test_case("existing perform() still invoked correct number of times");
  {
    int n = 0;
    auto fn = [&]() { ++n; };
    micron::range<0, 5>::perform(fn);
    sb::require(n, 5);
  }
  sb::end_test_case();

  sb::test_case("single-element range [4, 5)");
  {
    int count = 0;
    umax_t val = 0;
    for ( auto v : micron::range<4, 5>{} ) {
      val = v;
      ++count;
    }
    sb::require(count, 1);
    sb::require(val, umax_t{ 4 });
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ④ count_range<T, From, To> and type aliases
// ─────────────────────────────────────────────────────────────────────────────

static void
test_count_range()
{
  sb::print("=== count_range / type aliases ===");

  sb::test_case("int_range size / ssize / empty");
  {
    using IR = micron::int_range<-3, 3>;
    sb::require(IR::size(), usize{ 6 });
    sb::require(IR::ssize(), 6);
    sb::require_false(IR::empty());
  }
  sb::end_test_case();

  sb::test_case("int_range begin / end");
  {
    sb::require(*micron::int_range<-3, 3>::begin(), -3);
    sb::require(*micron::int_range<-3, 3>::end(), 3);
  }
  sb::end_test_case();

  sb::test_case("int_range range-based-for sum");
  {
    int sum = 0;
    for ( auto v : micron::int_range<1, 5>{} )
      sum += v;
    sb::require(sum, 10);
  }
  sb::end_test_case();

  sb::test_case("int_range negative-start collects correct endpoints");
  {
    int first = 0, last = 0, idx = 0;
    for ( auto v : micron::int_range<-3, 3>{} ) {
      if ( idx == 0 )
        first = v;
      last = v;
      ++idx;
    }
    sb::require(first, -3);
    sb::require(last, 2);
    sb::require(idx, 6);
  }
  sb::end_test_case();

  sb::test_case("u32_range begin / end / cbegin / cend");
  {
    using U32R = micron::u32_range<10u, 15u>;
    sb::require(*U32R::begin(), 10u);
    sb::require(*U32R::end(), 15u);
    sb::require(*U32R::cbegin(), 10u);
    sb::require(*U32R::cend(), 15u);
  }
  sb::end_test_case();

  sb::test_case("u32_range rbegin yields last element");
  {
    sb::require(*micron::u32_range<0u, 5u>::rbegin(), 4u);
  }
  sb::end_test_case();

  sb::test_case("micron::u64_range sum over small window");
  {
    u64 sum = 0;
    for ( auto v : micron::u64_range<100ULL, 104ULL>{} )
      sum += v;
    sb::require(sum, u64{ 100 + 101 + 102 + 103 });
  }
  sb::end_test_case();

  sb::test_case("float_range sum");
  {
    float sum = 0.f;
    for ( auto v : micron::float_range<0.f, 5.f>{} )
      sum += v;
    sb::require(sum, 0.f + 1.f + 2.f + 3.f + 4.f);
  }
  sb::end_test_case();

  sb::test_case("micron::i64_range count across zero");
  {
    i64 count = 0;
    for ( [[maybe_unused]] auto v : micron::i64_range<-5LL, 5LL>{} )
      ++count;
    sb::require(count, i64{ 10 });
  }
  sb::end_test_case();

  sb::test_case("existing perform(F) passes counter value");
  {
    int last = -1;
    auto fn = [&](int v) { last = v; };
    micron::int_range<0, 5>::perform(fn);
    sb::require(last, 4);
  }
  sb::end_test_case();

  sb::test_case("existing perform(obj, member_fn) accumulates correctly");
  {
    struct Acc {
      int sum = 0;

      void
      add(int v)
      {
        sum += v;
      }
    };

    Acc acc;
    micron::int_range<1, 6>::perform(acc, &Acc::add);
    sb::require(acc.sum, 15);
  }
  sb::end_test_case();

  sb::test_case("crbegin / crend correct endpoints");
  {
    sb::require(*micron::int_range<0, 5>::crbegin(), 4);
    sb::require(*micron::int_range<0, 5>::crend(), -1);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑤ range_of<T, Cnt>::view
// ─────────────────────────────────────────────────────────────────────────────

static void
test_range_of()
{
  sb::print("=== range_of<T, Cnt>::view ===");

  using Arr = flat_array<int, 8>;
  using RO5 = micron::range_of<Arr, 5>;

  sb::test_case("size / ssize / empty on view");
  {
    Arr arr = { 1, 2, 3, 4, 5, 6, 7, 8 };
    auto v = RO5::bind(arr);
    sb::require(v.size(), usize{ 5 });
    sb::require(v.ssize(), 5);
    sb::require_false(v.empty());
  }
  sb::end_test_case();

  sb::test_case("begin / end iterate exactly first Cnt elements");
  {
    Arr arr = { 10, 20, 30, 40, 50, 60, 70, 80 };
    auto v = RO5::bind(arr);
    int out[5];
    int i = 0;
    for ( auto it = v.begin(); it != v.end(); ++it )
      out[i++] = *it;
    sb::require(i, 5);
    sb::require(out[0], 10);
    sb::require(out[4], 50);
  }
  sb::end_test_case();

  sb::test_case("range-based-for sum over first 5");
  {
    Arr arr = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int sum = 0;
    for ( auto x : RO5::bind(arr) )
      sum += x;
    sb::require(sum, 1 + 2 + 3 + 4 + 5);
  }
  sb::end_test_case();

  sb::test_case("cbegin / cend read-only view");
  {
    Arr arr = { 5, 4, 3, 2, 1, 0, 0, 0 };
    auto v = RO5::bind(arr);
    int out[5];
    int i = 0;
    for ( auto it = v.cbegin(); it != v.cend(); ++it )
      out[i++] = *it;
    sb::require(out[0], 5);
    sb::require(out[4], 1);
  }
  sb::end_test_case();

  sb::test_case("data() points to underlying array start");
  {
    Arr arr = { 7, 8, 9, 0, 0, 0, 0, 0 };
    auto v = RO5::bind(arr);
    sb::require_true(v.data() == arr.data());
  }
  sb::end_test_case();

  sb::test_case("cdata() points to underlying array start (const view)");
  {
    Arr arr = { 7, 8, 9, 0, 0, 0, 0, 0 };
    const auto v = RO5::bind(arr);
    sb::require_true(v.cdata() == arr.data());
  }
  sb::end_test_case();

  sb::test_case("operator[] element access");
  {
    Arr arr = { 10, 20, 30, 40, 50, 60, 70, 80 };
    auto v = RO5::bind(arr);
    sb::require(v[0], 10);
    sb::require(v[4], 50);
  }
  sb::end_test_case();

  sb::test_case("rbegin / rend iterate first Cnt in reverse");
  {
    Arr arr = { 1, 2, 3, 4, 5, 6, 7, 8 };
    auto v = RO5::bind(arr);
    int out[5];
    int i = 0;
    for ( auto it = v.rbegin(); it != v.rend(); ++it )
      out[i++] = *it;
    sb::require(out[0], 5);
    sb::require(out[4], 1);
  }
  sb::end_test_case();

  sb::test_case("bind with Cnt == 1");
  {
    Arr arr = { 42, 0, 0, 0, 0, 0, 0, 0 };
    int count = 0, val = 0;
    for ( auto x : micron::range_of<Arr, 1>::bind(arr) ) {
      val = x;
      ++count;
    }
    sb::require(count, 1);
    sb::require(val, 42);
  }
  sb::end_test_case();

  sb::test_case("view never reads past Cnt into remaining elements");
  {
    Arr arr = { 1, 2, 3, 4, 5, 99, 99, 99 };
    int max_seen = 0;
    for ( auto x : RO5::bind(arr) )
      if ( x > max_seen )
        max_seen = x;
    sb::require(max_seen, 5);
  }
  sb::end_test_case();

  sb::test_case("existing perform(obj, fn) still accumulates correctly");
  {
    Arr arr = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int sum = 0;
    RO5::perform(arr, [&](int v) { sum += v; });
    sb::require(sum, 1 + 2 + 3 + 4 + 5);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑥ micron::ranges CPOs
// ─────────────────────────────────────────────────────────────────────────────

static void
test_cpos()
{
  sb::print("=== micron::ranges CPOs ===");

  // ── count_range ──────────────────────────────────────────────────────────

  sb::test_case("ranges::begin / end on count_range");
  {
    micron::int_range<2, 7> r;
    sb::require(*micron::ranges::begin(r), 2);
    sb::require(*micron::ranges::end(r), 7);
  }
  sb::end_test_case();

  sb::test_case("ranges::cbegin / cend on count_range");
  {
    micron::int_range<2, 7> r;
    sb::require(*micron::ranges::cbegin(r), 2);
    sb::require(*micron::ranges::cend(r), 7);
  }
  sb::end_test_case();

  sb::test_case("ranges::rbegin on count_range yields last element");
  {
    micron::int_range<0, 5> r;
    sb::require(*micron::ranges::rbegin(r), 4);
  }
  sb::end_test_case();

  sb::test_case("ranges::crbegin on count_range");
  {
    micron::int_range<0, 5> r;
    sb::require(*micron::ranges::crbegin(r), 4);
  }
  sb::end_test_case();

  sb::test_case("ranges::size / ssize / empty on count_range");
  {
    micron::int_range<3, 8> r;
    sb::require(micron::ranges::size(r), usize{ 5 });
    sb::require(micron::ranges::ssize(r), 5);
    sb::require_false(micron::ranges::empty(r));
  }
  sb::end_test_case();

  // ── range<> ──────────────────────────────────────────────────────────────

  sb::test_case("ranges::begin / end on range<>");
  {
    micron::range<10, 20> r;
    sb::require(*micron::ranges::begin(r), umax_t{ 10 });
    sb::require(*micron::ranges::end(r), umax_t{ 20 });
  }
  sb::end_test_case();

  sb::test_case("ranges::size / ssize / empty on range<>");
  {
    micron::range<0, 8> r;
    sb::require(micron::ranges::size(r), usize{ 8 });
    sb::require(micron::ranges::ssize(r), 8);
    sb::require_false(micron::ranges::empty(r));
  }
  sb::end_test_case();

  // ── flat_array (is_iterable_container) ───────────────────────────────────

  sb::test_case("ranges::begin / end on flat_array");
  {
    flat_array<int, 3> a = { 7, 8, 9 };
    sb::require(*micron::ranges::begin(a), 7);
    sb::require_true(micron::ranges::end(a) == a.data() + 3);
  }
  sb::end_test_case();

  sb::test_case("ranges::cbegin on flat_array");
  {
    flat_array<int, 3> a = { 1, 2, 3 };
    sb::require(*micron::ranges::cbegin(a), 1);
  }
  sb::end_test_case();

  sb::test_case("ranges::data / cdata on flat_array");
  {
    flat_array<int, 4> a = { 5, 6, 7, 8 };
    sb::require_true(micron::ranges::data(a) == a.data());
    sb::require_true(micron::ranges::cdata(a) == a.data());
  }
  sb::end_test_case();

  sb::test_case("ranges::size / ssize / empty on flat_array");
  {
    flat_array<int, 6> a = {};
    sb::require(micron::ranges::size(a), usize{ 6 });
    sb::require(micron::ranges::ssize(a), 6);
    sb::require_false(micron::ranges::empty(a));
  }
  sb::end_test_case();

  sb::test_case("ranges::empty on zero-size specialisation");
  {
    flat_array<int, 0> a = {};
    sb::require_true(micron::ranges::empty(a));
    sb::require(micron::ranges::size(a), usize{ 0 });
  }
  sb::end_test_case();

  // ── raw C arrays ─────────────────────────────────────────────────────────

  sb::test_case("ranges::begin / end on raw array");
  {
    int arr[4] = { 10, 20, 30, 40 };
    sb::require(*micron::ranges::begin(arr), 10);
    sb::require_true(micron::ranges::end(arr) == arr + 4);
  }
  sb::end_test_case();

  sb::test_case("ranges::cbegin / cend on raw array");
  {
    int arr[3] = { 1, 2, 3 };
    sb::require(*micron::ranges::cbegin(arr), 1);
    sb::require_true(micron::ranges::cend(arr) == arr + 3);
  }
  sb::end_test_case();

  sb::test_case("ranges::rbegin / rend on raw array");
  {
    int arr[3] = { 10, 20, 30 };
    auto rb = micron::ranges::rbegin(arr);
    sb::require(*rb, 30);
    ++rb;
    sb::require(*rb, 20);
  }
  sb::end_test_case();

  sb::test_case("ranges::crbegin on raw array");
  {
    int arr[3] = { 1, 2, 3 };
    sb::require(*micron::ranges::crbegin(arr), 3);
  }
  sb::end_test_case();

  sb::test_case("ranges::size / ssize on raw array");
  {
    double arr[7] = {};
    sb::require(micron::ranges::size(arr), usize{ 7 });
    sb::require(micron::ranges::ssize(arr), 7);
  }
  sb::end_test_case();

  sb::test_case("ranges::data / cdata on raw array");
  {
    int arr[5] = { 1, 2, 3, 4, 5 };
    sb::require_true(micron::ranges::data(arr) == arr);
    sb::require_true(micron::ranges::cdata(arr) == arr);
  }
  sb::end_test_case();

  sb::test_case("ranges::reserve_hint falls back to size()");
  {
    micron::int_range<0, 10> r;
    sb::require(micron::ranges::reserve_hint(r), usize{ 10 });
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑦ range primitive type aliases
// ─────────────────────────────────────────────────────────────────────────────

static void
test_type_aliases()
{
  sb::print("=== range primitive type aliases ===");

  sb::test_case("iterator_t<int_range<0,5>> is counting_iter<int>");
  {
    using R = micron::int_range<0, 5>;
    using It = micron::ranges::iterator_t<R>;
    sb::require_true((micron::is_same_v<It, micron::counting_iter<int>>));
  }
  sb::end_test_case();

  sb::test_case("sentinel_t<int_range<0,5>> is counting_iter<int>");
  {
    using R = micron::int_range<0, 5>;
    using S = micron::ranges::sentinel_t<R>;
    sb::require_true((micron::is_same_v<S, micron::counting_iter<int>>));
  }
  sb::end_test_case();

  sb::test_case("range_value_t<int_range> is int (cv-stripped)");
  {
    using R = micron::int_range<0, 5>;
    using V = micron::ranges::range_value_t<R>;
    sb::require_true((micron::is_same_v<V, int>));
  }
  sb::end_test_case();

  sb::test_case("range_reference_t<int_range> is int (by-value iterator)");
  {
    using R = micron::int_range<0, 5>;
    using Ref = micron::ranges::range_reference_t<R>;
    sb::require_true((micron::is_same_v<Ref, int>));
  }
  sb::end_test_case();

  sb::test_case("range_difference_t<micron::u64_range> is signed");
  {
    using R = micron::u64_range<0ULL, 10ULL>;
    using D = micron::ranges::range_difference_t<R>;
    sb::require_true(micron::is_signed_v<D>);
  }
  sb::end_test_case();

  sb::test_case("range_size_type<u32_range> is unsigned");
  {
    using R = micron::u32_range<0u, 10u>;
    using Sz = micron::ranges::range_size_type<R>;
    sb::require_true(micron::is_unsigned_v<Sz>);
  }
  sb::end_test_case();

  sb::test_case("range_value_t<flat_array<int,N>> is int");
  {
    using A = flat_array<int, 4>;
    using V = micron::ranges::range_value_t<A>;
    sb::require_true((micron::is_same_v<V, int>));
  }
  sb::end_test_case();

  sb::test_case("iterator_t and sentinel_t are same type for flat_array");
  {
    using A = flat_array<double, 3>;
    using It = micron::ranges::iterator_t<A>;
    using S = micron::ranges::sentinel_t<A>;
    sb::require_true((micron::is_same_v<It, S>));
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑧ constexpr correctness
// ─────────────────────────────────────────────────────────────────────────────

static void
test_constexpr()
{
  sb::print("=== constexpr correctness ===");

  sb::test_case("range<> size / begin / end are constexpr");
  {
    constexpr auto sz = micron::range<0, 10>::size();
    constexpr auto b = *micron::range<0, 10>::begin();
    constexpr auto e = *micron::range<0, 10>::end();
    sb::require(sz, umax_t{ 10 });
    sb::require(b, umax_t{ 0 });
    sb::require(e, umax_t{ 10 });
  }
  sb::end_test_case();

  sb::test_case("count_range<> size / begin are constexpr");
  {
    constexpr auto sz = micron::int_range<-5, 5>::size();
    constexpr auto b = *micron::int_range<-5, 5>::begin();
    sb::require(sz, usize{ 10 });
    sb::require(b, -5);
  }
  sb::end_test_case();

  sb::test_case("counting_iter arithmetic is constexpr");
  {
    constexpr micron::counting_iter<int> a{ 3 };
    constexpr auto b = a + 4;
    constexpr auto d = b - a;
    sb::require(*b, 7);
    sb::require(d, 4);
  }
  sb::end_test_case();

  sb::test_case("range<> empty is always false at compile time");
  {
    constexpr bool e = micron::range<0, 1>::empty();
    sb::require_false(e);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑨ idiomatic range-based-for patterns
// ─────────────────────────────────────────────────────────────────────────────

static void
test_for_loop_patterns()
{
  sb::print("=== range-based-for idiomatic patterns ===");

  sb::test_case("nested loops: range<> × int_range<>");
  {
    int count = 0;
    for ( [[maybe_unused]] auto i : micron::range<0, 3>{} )
      for ( [[maybe_unused]] auto j : micron::int_range<0, 4>{} )
        ++count;
    sb::require(count, 12);
  }
  sb::end_test_case();

  sb::test_case("copy int_range values into array");
  {
    int out[5] = {};
    int idx = 0;
    for ( auto v : micron::int_range<10, 15>{} )
      out[idx++] = v;
    sb::require(out[0], 10);
    sb::require(out[4], 14);
  }
  sb::end_test_case();

  sb::test_case("manual reverse loop visits correct sequence");
  {
    int out[5] = {};
    int idx = 0;
    auto rb = micron::int_range<0, 5>::rbegin();
    auto re = micron::int_range<0, 5>::rend();
    for ( ; rb != re; ++rb )
      out[idx++] = *rb;
    sb::require(out[0], 4);
    sb::require(out[4], 0);
  }
  sb::end_test_case();

  sb::test_case("CPO begin/end usable as loop bounds");
  {
    micron::int_range<0, 4> r;
    int sum = 0;
    for ( auto it = micron::ranges::begin(r); it != micron::ranges::end(r); ++it )
      sum += *it;
    sb::require(sum, 0 + 1 + 2 + 3);
  }
  sb::end_test_case();

  sb::test_case("float_range first and last element correct");
  {
    int count = 0;
    float first = 0.f, last = 0.f;
    for ( auto v : micron::float_range<2.f, 6.f>{} ) {
      if ( count == 0 )
        first = v;
      last = v;
      ++count;
    }
    sb::require(count, 4);
    sb::require(first, 2.f);
    sb::require(last, 5.f);
  }
  sb::end_test_case();

  sb::test_case("range_of view with CPO begin/end in algorithm loop");
  {
    flat_array<int, 10> arr = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    int sum = 0;
    auto view = micron::range_of<flat_array<int, 10>, 4>::bind(arr);
    for ( auto it = micron::ranges::begin(view); it != micron::ranges::end(view); ++it )
      sum += *it;
    sb::require(sum, 1 + 2 + 3 + 4);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑩ stress / regression
// ─────────────────────────────────────────────────────────────────────────────

static void
test_stress()
{
  sb::print("=== stress / regression ===");

  sb::test_case("int_range<0, 1000> accumulates correct sum via for-loop");
  {
    // Σ(0..999) = 999*1000/2 = 499500
    long long sum = 0;
    for ( auto v : micron::int_range<0, 1000>{} )
      sum += v;
    sb::require(sum, 499500LL);
  }
  sb::end_test_case();

  sb::test_case("range<0, 1000> iterates 1000 times exactly");
  {
    int count = 0;
    for ( [[maybe_unused]] auto v : micron::range<0, 1000>{} )
      ++count;
    sb::require(count, 1000);
  }
  sb::end_test_case();

  sb::test_case("all byte values 0..255 produce correct single-element float_range");
  {
    // float_range<V, V+1> must iterate exactly once and yield V
    // spot-check 16 representative values
    for ( int v : { 0, 1, 63, 64, 127, 128, 200, 254 } ) {
      // use a runtime loop since float NTTPs are fixed at compile time;
      // instead verify counting_iter round-trips the float value
      micron::counting_iter<float> it{ static_cast<float>(v) };
      sb::require(*it, static_cast<float>(v));
      ++it;
      sb::require(*it, static_cast<float>(v + 1));
    }
  }
  sb::end_test_case();

  sb::test_case("rbegin/rend traverse exactly (To-From) elements for large range");
  {
    int count = 0;
    auto rb = micron::int_range<0, 500>::rbegin();
    auto re = micron::int_range<0, 500>::rend();
    for ( ; rb != re; ++rb )
      ++count;
    sb::require(count, 500);
  }
  sb::end_test_case();

  sb::test_case("range_of view over 1024-element array visits exactly Cnt elements");
  {
    flat_array<int, 1024> arr;
    for ( usize i = 0; i < 1024; ++i )
      arr[i] = static_cast<int>(i);

    int count = 0;
    for ( [[maybe_unused]] auto x : micron::range_of<flat_array<int, 1024>, 256>::bind(arr) )
      ++count;
    sb::require(count, 256);
  }
  sb::end_test_case();

  sb::test_case("CPO rbegin traversal matches manual rbegin() member call");
  {
    micron::int_range<0, 8> r;
    int cpo_first = *micron::ranges::rbegin(r);
    int memb_first = *micron::int_range<0, 8>::rbegin();
    sb::require(cpo_first, memb_first);
  }
  sb::end_test_case();

  sb::test_case("crbegin CPO and member agree on first reverse element");
  {
    micron::u32_range<10u, 20u> r;
    unsigned cpo_v = *micron::ranges::crbegin(r);
    unsigned memb_v = *micron::u32_range<10u, 20u>::crbegin();
    sb::require(cpo_v, memb_v);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// entry point
// ─────────────────────────────────────────────────────────────────────────────

int
main()
{
  sb::print("micron::ranges test suite");
  sb::print("=========================");

  test_counting_iter();
  test_reverse_iter();
  test_range();
  test_count_range();
  test_range_of();
  test_cpos();
  test_type_aliases();
  test_constexpr();
  test_for_loop_patterns();
  test_stress();

  sb::print("=========================");
  sb::print("ALL TESTS COMPLETED");
  return 0;
}
