//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// slice_exhaustive.cpp — exhaustive snowball test suite for micron::slice<T>.
//
// Design note: slice<T>(n) returns a slice with size() determined by the
// allocator (rounded up to its granularity), not exactly n. To get a slice
// with logical length == n, use slice<T>(n, value) which sets length = n
// explicitly. This file uses the (n, value) form everywhere it relies on
// exact size, and verifies the rounded-up behaviour separately.

#include "../../src/io/console.hpp"
#include "../../src/slice.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <random>
#include <set>
#include <vector>

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_smaller;
using sb::require_true;
using sb::test_case;

namespace
{

struct Tracked {
  static inline int ctor = 0;
  static inline int dtor = 0;
  int v = 0;

  Tracked() : v(0) { ++ctor; }

  Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
    ++ctor;
  }

  ~Tracked() { ++dtor; }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    return *this;
  }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }

  bool
  operator<(const Tracked &o) const
  {
    return v < o.v;
  }
};

inline void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

};      // namespace

static void
__diag_terminate()
{
  micron::io::print("DIAG: terminate during test_case = [", sb::__global_test_case, "]\n\r");
  __builtin_trap();
}

int
main()
{
  std::set_terminate(__diag_terminate);
  sb::print("=== SLICE EXHAUSTIVE TESTS ===");

  test_case("ctor - default produces non-empty auto-sized slice");
  {
    micron::slice<int> s;
    require_greater(s.size(), size_t(0));
    require(s.size(), s.max_size());
    require_true(s.as_ptr() != nullptr);
  }
  end_test_case();

  test_case("ctor - size-only constructor returns size >= requested (allocator rounds up)");
  {
    micron::slice<int> s(64);
    require_greater(s.size(), size_t(63));
    require(s.size(), s.max_size());
    require_true(s.as_ptr() != nullptr);
  }
  end_test_case();

  test_case("ctor - (n, value) sets logical length to n exactly");
  {
    micron::slice<int> s(size_t(32), 7);
    require(s.size(), size_t(32));
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], 7);
  }
  end_test_case();

  test_case("ctor - pointer-range (T*, T*) copies data, length = b-a");
  {
    int src[6] = { 10, 20, 30, 40, 50, 60 };
    micron::slice<int> s(src, src + 6);
    require(s.size(), size_t(6));
    for ( int i = 0; i < 6; ++i ) require(s[i], src[i]);
    src[0] = -1;
    require(s[0], 10);
  }
  end_test_case();

  test_case("ctor - pointer-range (const T*, const T*) copies data");
  {
    const int src[4] = { 100, 200, 300, 400 };
    micron::slice<int> s(src, src + 4);
    require(s.size(), size_t(4));
    require(s[0], 100);
    require(s[3], 400);
  }
  end_test_case();

  test_case("ctor - nullptr produces empty slice");
  {
    micron::slice<int> s(nullptr);
    require(s.size(), size_t(0));
    require_true(s.is_empty());
  }
  end_test_case();

  test_case("ctor - nullptr, nullptr produces empty slice");
  {
    micron::slice<int> s(nullptr, nullptr);
    require(s.size(), size_t(0));
    require_true(s.is_empty());
  }
  end_test_case();

  test_case("ctor - transform Fn + R copies projected values");
  {
    micron::slice<int> base(size_t(8), 3);
    micron::slice<int> mapped([](const int &x) { return x * x + 1; }, base);
    require(mapped.size(), size_t(8));
    for ( size_t i = 0; i < mapped.size(); ++i ) require(mapped[i], 10);
  }
  end_test_case();

  test_case("ctor - move ctor transfers contents, source empties");
  {
    micron::slice<int> a(size_t(20), 5);
    int *ptr = a.as_ptr();
    size_t sz = a.size();
    micron::slice<int> b(micron::move(a));
    require(b.size(), sz);
    require(b.as_ptr(), ptr);
    require(a.size(), size_t(0));
  }
  end_test_case();

  test_case("move-assign - replaces target, source emptied");
  {
    micron::slice<int> a(size_t(15), 9);
    micron::slice<int> b(size_t(5), 1);
    b = micron::move(a);
    require(b.size(), size_t(15));
    for ( size_t i = 0; i < b.size(); ++i ) require(b[i], 9);
    require(a.size(), size_t(0));
  }
  end_test_case();

  test_case("ctor - size 1 corner case");
  {
    micron::slice<int> s(size_t(1), 42);
    require(s.size(), size_t(1));
    require(s[0], 42);
    require_true(s.first() != nullptr);
    require_true(s.last() != nullptr);
    require(*s.first(), 42);
    require(*s.last(), 42);
  }
  end_test_case();

  test_case("ctor - large size (10000) elements via (n, value)");
  {
    micron::slice<int> s(size_t(10'000), 1);
    require(s.size(), size_t(10'000));
    long long total = 0;
    for ( size_t i = 0; i < s.size(); ++i ) total += s[i];
    require(total, (long long)10'000);
  }
  end_test_case();

  test_case("lifetime - filled slice + temp seed balances ctor/dtor");
  {
    reset_tracked();
    {
      micron::slice<Tracked> s(size_t(8), Tracked(99));
      for ( size_t i = 0; i < s.size(); ++i ) require(s[i].v, 99);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime - move ctor of Tracked slice does not double-count");
  {
    reset_tracked();
    {
      micron::slice<Tracked> a(size_t(6), Tracked(1));
      int a_ctor = Tracked::ctor;
      micron::slice<Tracked> b(micron::move(a));

      require(Tracked::ctor, a_ctor);
      require(b.size(), size_t(6));
      require(b[0].v, 1);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("operator[] - read and write mutable");
  {
    micron::slice<int> s(size_t(8), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = static_cast<int>(i * i);
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], static_cast<int>(i * i));
  }
  end_test_case();

  test_case("operator[] - const overload on const slice");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = 100 + (int)i;
    const auto &cs = s;
    require(cs[0], 100);
    require(cs[3], 103);
  }
  end_test_case();

  test_case("operator[] - signed integral index works");
  {
    micron::slice<int> s(size_t(5), 11);
    int idx = 2;
    require(s[idx], 11);
  }
  end_test_case();

  test_case("operator[](n, m) - subslice [n, m) returns owned copy");
  {
    micron::slice<int> s(size_t(10), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    auto sub = s[2, 7];
    require(sub.size(), size_t(5));
    for ( size_t i = 0; i < sub.size(); ++i ) require(sub[i], (int)(i + 2));
    sub[0] = -1;
    require(s[2], 2);
  }
  end_test_case();

  test_case("operator[]() - whole-slice copy");
  {
    micron::slice<int> s(size_t(6), 9);
    auto whole = s[];
    require(whole.size(), s.size());
    for ( size_t i = 0; i < whole.size(); ++i ) require(whole[i], 9);
  }
  end_test_case();

  test_case("data() / addr() / as_ptr() all reference the first element");
  {
    micron::slice<int> s(size_t(4), 0);
    s[0] = 77;
    require(*s.data(), 77);
    require(*s.addr(), 77);
    require(*s.as_ptr(), 77);
    require(s.data(), s.addr());
    require(s.data(), s.as_ptr());
  }
  end_test_case();

  test_case("as_ptr_range - covers length elements");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    auto r = s.as_ptr_range();
    auto diff = r.end - r.begin;
    require((size_t)diff, s.size());
    require(*r.begin, 0);
    require(*(r.end - 1), 4);
  }
  end_test_case();

  test_case("first() / last() / first_mut() / last_mut() return pointers");
  {
    micron::slice<int> s(size_t(3), 0);
    s[0] = 100;
    s[1] = 200;
    s[2] = 300;
    require(*s.first(), 100);
    require(*s.last(), 300);
    *s.first_mut() = -1;
    require(s[0], -1);
    *s.last_mut() = -2;
    require(s[2], -2);
  }
  end_test_case();

  test_case("last() returns nullptr on zero-length slice");
  {
    micron::slice<int> s(size_t(4), 0);
    s.mark(0);
    require_true(s.last() == nullptr);
  }
  end_test_case();

  test_case("get(i) - in bounds returns pointer; out of bounds returns nullptr");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 10;
    require(*s.get(0), 10);
    require(*s.get(3), 13);
    require_true(s.get(4) == nullptr);
    require_true(s.get(99) == nullptr);
  }
  end_test_case();

  test_case("get_unchecked - returns pointer regardless of bounds");
  {
    micron::slice<int> s(size_t(3), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i * 5;
    require(*s.get_unchecked(2), 10);
  }
  end_test_case();

  test_case("get_disjoint_mut - valid for i != j");
  {
    micron::slice<int> s(size_t(5), 0);
    auto p = s.get_disjoint_mut(0, 4);
    require_true(p.valid());
    *p.a = 11;
    *p.b = 99;
    require(s[0], 11);
    require(s[4], 99);
  }
  end_test_case();

  test_case("get_disjoint_mut - same index returns invalid");
  {
    micron::slice<int> s(size_t(5), 0);
    auto p = s.get_disjoint_mut(2, 2);
    require_false(p.valid());
  }
  end_test_case();

  test_case("element_offset - pointer inside slice returns offset");
  {
    micron::slice<int> s(size_t(8), 0);
    auto off = s.element_offset(s.as_ptr() + 3);
    require(off, size_t(3));
  }
  end_test_case();

  test_case("element_offset - external pointer returns -1");
  {
    micron::slice<int> s(size_t(4), 0);
    int stranger = 0;
    auto off = s.element_offset(&stranger);
    require(off, static_cast<size_t>(-1));
  }
  end_test_case();

  test_case("begin / end - inclusive-end walk covers every element exactly once");
  {
    micron::slice<int> s(size_t(6), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    int sum = 0;
    int count = 0;
    for ( auto p = s.begin();; ++p ) {
      sum += *p;
      ++count;
      if ( p == s.end() ) break;
    }
    require(sum, 21);
    require(count, 6);
  }
  end_test_case();

  test_case("cbegin / cend - const traversal");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    const auto &cs = s;
    int sum = 0;
    for ( auto p = cs.cbegin();; ++p ) {
      sum += *p;
      if ( p == cs.cend() ) break;
    }
    require(sum, 0 + 1 + 2 + 3 + 4);
  }
  end_test_case();

  test_case("iter() - raw_slice covers all elements");
  {
    micron::slice<int> s(size_t(7), 3);
    auto r = s.iter();
    require(r.len, s.size());
    require_true(r.ptr == s.as_ptr());
    long long total = 0;
    for ( size_t i = 0; i < r.len; ++i ) total += r[i];
    require(total, (long long)21);
  }
  end_test_case();

  test_case("iter_mut() - mutation through view reflects in slice");
  {
    micron::slice<int> s(size_t(4), 0);
    auto r = s.iter_mut();
    for ( size_t i = 0; i < r.len; ++i ) r[i] = (int)i * 7;
    require(s[0], 0);
    require(s[3], 21);
  }
  end_test_case();

  test_case("size / len / max_size for (n, value) ctor");
  {
    micron::slice<int> s(size_t(64), 0);
    require(s.size(), size_t(64));
    require(s.len(), size_t(64));
    require_true(s.max_size() >= size_t(64));
  }
  end_test_case();

  test_case("mark - shrink length within capacity");
  {
    micron::slice<int> s(size_t(40), 1);
    s.mark(10);
    require(s.size(), size_t(10));
    require_true(s.max_size() >= size_t(40));
  }
  end_test_case();

  test_case("mark - beyond capacity is no-op");
  {
    micron::slice<int> s(size_t(10), 1);
    s.mark(99999);
    require(s.size(), size_t(10));
  }
  end_test_case();

  test_case("mark - to 0 yields empty");
  {
    micron::slice<int> s(size_t(5), 1);
    s.mark(0);
    require_true(s.is_empty());
    require(s.size(), size_t(0));
  }
  end_test_case();

  test_case("is_empty - true after mark(0), false otherwise");
  {
    micron::slice<int> s(size_t(1), 0);
    require_false(s.is_empty());
    s.mark(0);
    require_true(s.is_empty());
  }
  end_test_case();

  test_case("set - assigns same value to every element");
  {
    micron::slice<int> s(size_t(10), 0);
    s.set(99);
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], 99);
  }
  end_test_case();

  test_case("fill - same as set; chainable return");
  {
    micron::slice<int> s(size_t(5), 0);
    auto &r = s.fill(-3);
    require_true(&r == &s);
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], -3);
  }
  end_test_case();

  test_case("fill_with - generator called once per element");
  {
    micron::slice<int> s(size_t(8), 0);
    int n = 0;
    s.fill_with([&]() { return ++n; });
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], (int)(i + 1));
  }
  end_test_case();

  test_case("write_filled - alias for fill");
  {
    micron::slice<int> s(size_t(6), 0);
    s.write_filled(11);
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], 11);
  }
  end_test_case();

  test_case("write_with - index-aware generator");
  {
    micron::slice<int> s(size_t(6), 0);
    s.write_with([](size_t i) { return (int)i * 3; });
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], (int)i * 3);
  }
  end_test_case();

  test_case("write_iter - copies from external iterator range");
  {
    std::vector<int> src{ 100, 200, 300, 400, 500 };
    micron::slice<int> s(size_t(5), 0);
    s.write_iter(src.begin(), src.end());
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], src[i]);
  }
  end_test_case();

  test_case("write_iter - shorter input does not run past length");
  {
    std::vector<int> src{ 1, 2 };
    micron::slice<int> s(size_t(5), 0);
    s.write_iter(src.begin(), src.end());
    require(s[0], 1);
    require(s[1], 2);
    require(s[2], 0);
  }
  end_test_case();

  test_case("write_copy_of_slice - bytecpy of raw_slice");
  {
    int data[4] = { 9, 8, 7, 6 };
    micron::raw_slice<int> rs{ data, 4 };
    micron::slice<int> s(size_t(4), 0);
    s.write_copy_of_slice(rs);
    for ( size_t i = 0; i < 4; ++i ) require(s[i], data[i]);
  }
  end_test_case();

  test_case("write_clone_of_slice - element-wise copy");
  {
    int data[3] = { 11, 22, 33 };
    micron::raw_slice<int> rs{ data, 3 };
    micron::slice<int> s(size_t(3), 0);
    s.write_clone_of_slice(rs);
    for ( size_t i = 0; i < 3; ++i ) require(s[i], data[i]);
  }
  end_test_case();

  test_case("reset - restores length to capacity");
  {
    micron::slice<int> s(size_t(8), 5);
    s.mark(3);
    require(s.size(), size_t(3));
    s.reset();
    require(s.size(), s.max_size());
  }
  end_test_case();

  test_case("operator=(byte) - memset over slice<char>");
  {
    micron::slice<char> s(size_t(16), '\0');
    s = static_cast<byte>('Z');
    for ( size_t i = 0; i < s.size(); ++i ) require_true(s[i] == 'Z');
  }
  end_test_case();

  test_case("contains(value) - present");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i * 10;
    require_true(s.contains(30));
  }
  end_test_case();

  test_case("contains(value) - absent");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    require_false(s.contains(99));
  }
  end_test_case();

  test_case("contains(predicate) - any-of");
  {
    micron::slice<int> s(size_t(6), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    require_true(s.contains([](const int &x) { return x > 4; }));
    require_false(s.contains([](const int &x) { return x > 100; }));
  }
  end_test_case();

  test_case("starts_with - matching prefix");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    int prefix[2] = { 1, 2 };
    require_true(s.starts_with(micron::raw_slice<int>{ prefix, 2 }));
  }
  end_test_case();

  test_case("starts_with - non-matching prefix");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    int prefix[2] = { 99, 0 };
    require_false(s.starts_with(micron::raw_slice<int>{ prefix, 2 }));
  }
  end_test_case();

  test_case("starts_with - empty needle is always true");
  {
    micron::slice<int> s(size_t(3), 1);
    int dummy = 0;
    require_true(s.starts_with(micron::raw_slice<int>{ &dummy, 0 }));
  }
  end_test_case();

  test_case("starts_with - longer than slice is false");
  {
    micron::slice<int> s(size_t(2), 1);
    int needle[4] = { 1, 1, 1, 1 };
    require_false(s.starts_with(micron::raw_slice<int>{ needle, 4 }));
  }
  end_test_case();

  test_case("ends_with - matching suffix");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    int suffix[2] = { 3, 4 };
    require_true(s.ends_with(micron::raw_slice<int>{ suffix, 2 }));
  }
  end_test_case();

  test_case("ends_with - empty needle is always true");
  {
    micron::slice<int> s(size_t(3), 0);
    int dummy = 0;
    require_true(s.ends_with(micron::raw_slice<int>{ &dummy, 0 }));
  }
  end_test_case();

  test_case("ends_with - non-matching suffix");
  {
    micron::slice<int> s(size_t(3), 0);
    s[0] = 1;
    s[1] = 2;
    s[2] = 3;
    int suffix[2] = { 9, 9 };
    require_false(s.ends_with(micron::raw_slice<int>{ suffix, 2 }));
  }
  end_test_case();

  test_case("strip_prefix - matching prefix removed");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    int p[2] = { 0, 1 };
    auto r = s.strip_prefix(micron::raw_slice<int>{ p, 2 });
    require(r.len, size_t(3));
    require(r[0], 2);
    require(r[2], 4);
  }
  end_test_case();

  test_case("strip_prefix - no match returns empty raw_slice");
  {
    micron::slice<int> s(size_t(4), 5);
    int p[1] = { 99 };
    auto r = s.strip_prefix(micron::raw_slice<int>{ p, 1 });
    require(r.len, size_t(0));
  }
  end_test_case();

  test_case("strip_suffix - matching suffix removed");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    int suf[2] = { 3, 4 };
    auto r = s.strip_suffix(micron::raw_slice<int>{ suf, 2 });
    require(r.len, size_t(3));
    require(r[0], 0);
    require(r[2], 2);
  }
  end_test_case();

  test_case("strip_circumfix - both prefix and suffix match");
  {
    micron::slice<int> s(size_t(6), 0);
    int data[6] = { 1, 2, 7, 7, 9, 8 };
    for ( int i = 0; i < 6; ++i ) s[i] = data[i];
    int p[2] = { 1, 2 };
    int q[2] = { 9, 8 };
    auto r = s.strip_circumfix(micron::raw_slice<int>{ p, 2 }, micron::raw_slice<int>{ q, 2 });
    require(r.len, size_t(2));
    require(r[0], 7);
    require(r[1], 7);
  }
  end_test_case();

  test_case("strip_circumfix - prefix+suffix longer than slice returns empty");
  {
    micron::slice<int> s(size_t(2), 1);
    int p[2] = { 1, 1 };
    int q[2] = { 1, 1 };
    auto r = s.strip_circumfix(micron::raw_slice<int>{ p, 2 }, micron::raw_slice<int>{ q, 2 });
    require(r.len, size_t(0));
  }
  end_test_case();

  test_case("trim_prefix / trim_suffix - aliases for strip_*");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    int p[1] = { 0 };
    int q[1] = { 3 };
    auto rp = s.trim_prefix(micron::raw_slice<int>{ p, 1 });
    auto rs = s.trim_suffix(micron::raw_slice<int>{ q, 1 });
    require(rp.len, size_t(3));
    require(rs.len, size_t(3));
  }
  end_test_case();

  test_case("swap(i, j) - exchanges elements");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    s.swap(0, 3);
    require(s[0], 3);
    require(s[3], 0);
  }
  end_test_case();

  test_case("swap - out-of-bounds is a no-op");
  {
    micron::slice<int> s(size_t(3), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    s.swap(0, 99);
    require(s[0], 0);
  }
  end_test_case();

  test_case("swap - same index is a no-op");
  {
    micron::slice<int> s(size_t(3), 7);
    s.swap(1, 1);
    require(s[1], 7);
  }
  end_test_case();

  test_case("swap_unchecked - operates without bounds check");
  {
    micron::slice<int> s(size_t(3), 0);
    s[0] = 1;
    s[2] = 3;
    s.swap_unchecked(0, 2);
    require(s[0], 3);
    require(s[2], 1);
  }
  end_test_case();

  test_case("swap_with_slice - exchanges min(N, M) elements");
  {
    micron::slice<int> a(size_t(4), 0);
    micron::slice<int> b(size_t(6), 0);
    for ( size_t i = 0; i < a.size(); ++i ) a[i] = (int)i;
    for ( size_t i = 0; i < b.size(); ++i ) b[i] = -(int)i - 1;
    a.swap_with_slice(b);
    for ( size_t i = 0; i < 4; ++i ) {
      require(a[i], -(int)i - 1);
      require(b[i], (int)i);
    }
    require(b[4], -5);
    require(b[5], -6);
  }
  end_test_case();

  test_case("reverse - many elements");
  {
    micron::slice<int> s(size_t(7), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    s.reverse();
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], (int)(6 - i));
  }
  end_test_case();

  test_case("reverse - empty / single-element are no-ops");
  {
    micron::slice<int> e(size_t(1), 0);
    e.mark(0);
    e.reverse();
    require(e.size(), size_t(0));
    micron::slice<int> s(size_t(1), 5);
    s.reverse();
    require(s[0], 5);
  }
  end_test_case();

  test_case("reverse - double reverse is identity");
  {
    micron::slice<int> s(size_t(8), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i * 3;
    s.reverse();
    s.reverse();
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], (int)i * 3);
  }
  end_test_case();

  test_case("rotate_left(k) - basic shift");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    s.rotate_left(2);
    int expected[5] = { 3, 4, 5, 1, 2 };
    for ( size_t i = 0; i < 5; ++i ) require(s[i], expected[i]);
  }
  end_test_case();

  test_case("rotate_left(0) - no change");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    s.rotate_left(0);
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], (int)i);
  }
  end_test_case();

  test_case("rotate_left(length) - identity by modulo");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    s.rotate_left(5);
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], (int)i);
  }
  end_test_case();

  test_case("rotate_right(k) - basic shift");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    s.rotate_right(2);
    int expected[5] = { 4, 5, 1, 2, 3 };
    for ( size_t i = 0; i < 5; ++i ) require(s[i], expected[i]);
  }
  end_test_case();

  test_case("copy_within - non-overlapping forward copy");
  {
    micron::slice<int> s(size_t(8), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    s.copy_within(0, 3, 5);
    require(s[5], 1);
    require(s[6], 2);
    require(s[7], 3);
  }
  end_test_case();

  test_case("copy_within - dst/src out of bounds is no-op");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    s.copy_within(3, 10, 0);
    for ( size_t i = 0; i < s.size(); ++i ) require(s[i], (int)i);
  }
  end_test_case();

  test_case("split_at(mid) - basic split");
  {
    micron::slice<int> s(size_t(6), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    auto pair = s.split_at(3);
    require(pair.first.len, size_t(3));
    require(pair.second.len, size_t(3));
    require(pair.first[0], 0);
    require(pair.second[0], 3);
  }
  end_test_case();

  test_case("split_at(0) - empty first, full second");
  {
    micron::slice<int> s(size_t(4), 9);
    auto pair = s.split_at(0);
    require(pair.first.len, size_t(0));
    require(pair.second.len, size_t(4));
    require(pair.second[0], 9);
  }
  end_test_case();

  test_case("split_at(length) - full first, empty second");
  {
    micron::slice<int> s(size_t(4), 1);
    auto pair = s.split_at(4);
    require(pair.first.len, size_t(4));
    require(pair.second.len, size_t(0));
  }
  end_test_case();

  test_case("split_at(>length) - clamped to length");
  {
    micron::slice<int> s(size_t(3), 0);
    auto pair = s.split_at(99);
    require(pair.first.len, size_t(3));
    require(pair.second.len, size_t(0));
  }
  end_test_case();

  test_case("split_at_checked - out-of-bounds returns invalid pair");
  {
    micron::slice<int> s(size_t(3), 0);
    auto pair = s.split_at_checked(99);
    require(pair.first.len, size_t(0));
    require(pair.second.len, size_t(0));
  }
  end_test_case();

  test_case("split_at_unchecked - trusts the caller");
  {
    micron::slice<int> s(size_t(4), 5);
    auto pair = s.split_at_unchecked(2);
    require(pair.first.len, size_t(2));
    require(pair.second.len, size_t(2));
  }
  end_test_case();

  test_case("split_first - first element + tail");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 10;
    auto r = s.split_first();
    require_true(r.valid());
    require(*r.elem, 10);
    require(r.rest.len, size_t(3));
    require(r.rest[0], 11);
  }
  end_test_case();

  test_case("split_first - empty slice returns invalid");
  {
    micron::slice<int> s(size_t(1), 0);
    s.mark(0);
    auto r = s.split_first();
    require_false(r.valid());
    require_true(r.elem == nullptr);
  }
  end_test_case();

  test_case("split_last - init + last");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 10;
    auto r = s.split_last();
    require_true(r.valid());
    require(*r.elem, 13);
    require(r.init.len, size_t(3));
    require(r.init[2], 12);
  }
  end_test_case();

  test_case("split_first_chunk<N> - length >= N");
  {
    micron::slice<int> s(size_t(6), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    auto r = s.split_first_chunk<3>();
    require_true(r.valid());
    require(r.chunk[0], 0);
    require(r.chunk[2], 2);
    require(r.rest.len, size_t(3));
    require(r.rest[0], 3);
  }
  end_test_case();

  test_case("split_first_chunk<N> - length < N returns invalid");
  {
    micron::slice<int> s(size_t(2), 1);
    auto r = s.split_first_chunk<3>();
    require_false(r.valid());
  }
  end_test_case();

  test_case("split_last_chunk<N> - length >= N");
  {
    micron::slice<int> s(size_t(6), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    auto r = s.split_last_chunk<2>();
    require_true(r.valid());
    require(r.chunk[0], 5);
    require(r.chunk[1], 6);
    require(r.rest.len, size_t(4));
  }
  end_test_case();

  test_case("first_chunk<N> / last_chunk<N> - pointers when long enough");
  {
    micron::slice<int> s(size_t(8), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i;
    require_true(s.first_chunk<3>() != nullptr);
    require(s.first_chunk<3>()[0], 0);
    require(s.last_chunk<2>()[1], 7);
    require_true(s.first_chunk<99>() == nullptr);
    require_true(s.last_chunk<99>() == nullptr);
  }
  end_test_case();

  test_case("split(pred, cb) - delimiters split into chunks");
  {
    micron::slice<int> s(size_t(7), 0);
    int data[7] = { 1, 2, 0, 3, 4, 0, 5 };
    for ( int i = 0; i < 7; ++i ) s[i] = data[i];
    std::vector<std::vector<int>> chunks;
    size_t count = s.split([](const int &x) { return x == 0; },
                           [&](micron::raw_slice<int> r) {
                             std::vector<int> v;
                             for ( size_t i = 0; i < r.len; ++i ) v.push_back(r[i]);
                             chunks.push_back(v);
                           });
    require(count, size_t(3));
    require(chunks.size(), size_t(3));
    require(chunks[0].size(), size_t(2));
    require(chunks[1].size(), size_t(2));
    require(chunks[2].size(), size_t(1));
    require(chunks[2][0], 5);
  }
  end_test_case();

  test_case("split - no delimiter yields one whole chunk");
  {
    micron::slice<int> s(size_t(5), 1);
    int cb_count = 0;
    size_t pieces = s.split([](const int &x) { return x == 99; }, [&](micron::raw_slice<int>) { ++cb_count; });
    require(pieces, size_t(1));
    require(cb_count, 1);
  }
  end_test_case();

  test_case("split_inclusive - delimiter stays in its chunk");
  {
    micron::slice<int> s(size_t(5), 0);
    int data[5] = { 1, 0, 2, 0, 3 };
    for ( int i = 0; i < 5; ++i ) s[i] = data[i];
    std::vector<size_t> sizes;
    s.split_inclusive([](const int &x) { return x == 0; }, [&](micron::raw_slice<int> r) { sizes.push_back(r.len); });
    require(sizes.size(), size_t(3));
    require(sizes[0], size_t(2));
    require(sizes[1], size_t(2));
    require(sizes[2], size_t(1));
  }
  end_test_case();

  test_case("splitn(n, pred, cb) - caps at n chunks");
  {
    micron::slice<int> s(size_t(7), 0);
    int data[7] = { 1, 0, 2, 0, 3, 0, 4 };
    for ( int i = 0; i < 7; ++i ) s[i] = data[i];
    std::vector<size_t> sizes;
    size_t produced = s.splitn(2, [](const int &x) { return x == 0; }, [&](micron::raw_slice<int> r) { sizes.push_back(r.len); });
    require(produced, size_t(2));
    require(sizes.size(), size_t(2));
    require(sizes[0], size_t(1));
    require(sizes[1], size_t(5));
  }
  end_test_case();

  test_case("splitn(0, ...) - returns 0 and skips callback");
  {
    micron::slice<int> s(size_t(3), 0);
    int cb_count = 0;
    size_t r = s.splitn(0, [](const int &) { return true; }, [&](micron::raw_slice<int>) { ++cb_count; });
    require(r, size_t(0));
    require(cb_count, 0);
  }
  end_test_case();

  test_case("rsplit - splits from end");
  {
    micron::slice<int> s(size_t(7), 0);
    int data[7] = { 1, 0, 2, 0, 3, 0, 4 };
    for ( int i = 0; i < 7; ++i ) s[i] = data[i];
    std::vector<size_t> sizes;
    s.rsplit([](const int &x) { return x == 0; }, [&](micron::raw_slice<int> r) { sizes.push_back(r.len); });
    require(sizes.size(), size_t(4));
    require(sizes[0], size_t(1));
    require(sizes[1], size_t(1));
    require(sizes[2], size_t(1));
    require(sizes[3], size_t(1));
  }
  end_test_case();

  test_case("rsplitn - caps from the end");
  {
    micron::slice<int> s(size_t(7), 0);
    int data[7] = { 1, 0, 2, 0, 3, 0, 4 };
    for ( int i = 0; i < 7; ++i ) s[i] = data[i];
    std::vector<size_t> sizes;
    size_t produced = s.rsplitn(2, [](const int &x) { return x == 0; }, [&](micron::raw_slice<int> r) { sizes.push_back(r.len); });
    require(produced, size_t(2));
    require(sizes.size(), size_t(2));
    require(sizes[0], size_t(1));
  }
  end_test_case();

  test_case("split_once(delim) - finds first occurrence");
  {
    micron::slice<int> s(size_t(5), 0);
    int data[5] = { 1, 2, 9, 3, 4 };
    for ( int i = 0; i < 5; ++i ) s[i] = data[i];
    auto pair = s.split_once(9);
    require(pair.first.len, size_t(2));
    require(pair.second.len, size_t(2));
    require(pair.first[0], 1);
    require(pair.second[1], 4);
  }
  end_test_case();

  test_case("split_once - delimiter absent returns empty pair");
  {
    micron::slice<int> s(size_t(3), 1);
    auto pair = s.split_once(99);
    require(pair.first.len, size_t(0));
    require(pair.second.len, size_t(0));
  }
  end_test_case();

  test_case("split_once(predicate)");
  {
    micron::slice<int> s(size_t(5), 0);
    int data[5] = { 1, 2, 4, 3, 5 };
    for ( int i = 0; i < 5; ++i ) s[i] = data[i];
    auto pair = s.split_once([](const int &x) { return x % 2 == 0; });
    require(pair.first.len, size_t(1));
    require(pair.second.len, size_t(3));
    require(pair.first[0], 1);
    require(pair.second[0], 4);
  }
  end_test_case();

  test_case("rsplit_once(delim) - finds last occurrence");
  {
    micron::slice<int> s(size_t(7), 0);
    int data[7] = { 1, 9, 2, 9, 3, 9, 4 };
    for ( int i = 0; i < 7; ++i ) s[i] = data[i];
    auto pair = s.rsplit_once(9);
    require(pair.first.len, size_t(5));
    require(pair.second.len, size_t(1));
    require(pair.second[0], 4);
  }
  end_test_case();

  test_case("split_off(mid) - keeps prefix, returns owning suffix");
  {
    micron::slice<int> s(size_t(6), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    auto tail = s.split_off(2);
    require(s.size(), size_t(2));
    require(tail.size(), size_t(4));
    require(s[0], 1);
    require(s[1], 2);
    require(tail[0], 3);
    require(tail[3], 6);
  }
  end_test_case();

  test_case("split_off_first - returns first element, shifts down");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    int v = s.split_off_first();
    require(v, 1);
    require(s.size(), size_t(3));
    require(s[0], 2);
    require(s[2], 4);
  }
  end_test_case();

  test_case("split_off_last - returns last element, shrinks");
  {
    micron::slice<int> s(size_t(4), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    int v = s.split_off_last();
    require(v, 4);
    require(s.size(), size_t(3));
    require(s[2], 3);
  }
  end_test_case();

  test_case("split_off_last - empty yields T{}");
  {
    micron::slice<int> s(size_t(1), 0);
    s.mark(0);
    int v = s.split_off_last();
    require(v, 0);
  }
  end_test_case();

  test_case("split_off_mut(mid) - returns raw_slice tail, length shrinks");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    auto rest = s.split_off_mut(2);
    require(s.size(), size_t(2));
    require(rest.len, size_t(3));
    require(rest[0], 3);
    require(rest[2], 5);
  }
  end_test_case();

  test_case("to_vec - exact copy of contents");
  {
    micron::slice<int> s(size_t(5), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i * 11;
    auto v = s.to_vec();
    require(v.size(), s.size());
    for ( size_t i = 0; i < v.size(); ++i ) require(v[i], s[i]);
    v[0] = -1;
    require(s[0], 0);
  }
  end_test_case();

  test_case("repeat(1) - first n elements equal source");
  {
    micron::slice<int> s(size_t(3), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    auto r = s.repeat(1);
    require_true(r.size() >= s.size());
    for ( size_t i = 0; i < s.size(); ++i ) require(r[i], s[i]);
  }
  end_test_case();

  test_case("repeat(4) - first 12 elements match repeated pattern");
  {
    micron::slice<int> s(size_t(3), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    auto r = s.repeat(4);
    require_true(r.size() >= size_t(12));
    for ( size_t rep = 0; rep < 4; ++rep )
      for ( size_t i = 0; i < 3; ++i ) require(r[rep * 3 + i], (int)(i + 1));
  }
  end_test_case();

  test_case("concat - first elements are self followed by other");
  {
    micron::slice<int> s(size_t(3), 0);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (int)i + 1;
    int extra[2] = { 10, 11 };
    auto r = s.concat(micron::raw_slice<int>{ extra, 2 });
    require_true(r.size() >= size_t(5));
    require(r[0], 1);
    require(r[2], 3);
    require(r[3], 10);
    require(r[4], 11);
  }
  end_test_case();

  test_case("concat - with empty other still copies self");
  {
    micron::slice<int> s(size_t(3), 9);
    int dummy = 0;
    auto r = s.concat(micron::raw_slice<int>{ &dummy, 0 });
    require_true(r.size() >= s.size());
    for ( size_t i = 0; i < s.size(); ++i ) require(r[i], 9);
  }
  end_test_case();

  test_case("join - first elements are self, sep, other");
  {
    micron::slice<int> s(size_t(2), 0);
    s[0] = 1;
    s[1] = 2;
    int extra[3] = { 4, 5, 6 };
    auto r = s.join(micron::raw_slice<int>{ extra, 3 }, 99);
    require_true(r.size() >= size_t(6));
    require(r[0], 1);
    require(r[1], 2);
    require(r[2], 99);
    require(r[3], 4);
    require(r[5], 6);
  }
  end_test_case();

  test_case("connect - alias for join");
  {
    micron::slice<int> s(size_t(2), 1);
    int extra[2] = { 2, 3 };
    auto r = s.connect(micron::raw_slice<int>{ extra, 2 }, 0);
    require_true(r.size() >= size_t(5));
    require(r[2], 0);
  }
  end_test_case();

  test_case("copy_from_slice - copies first min(N, M) elements");
  {
    int src[6] = { 10, 20, 30, 40, 50, 60 };
    micron::slice<int> dst(size_t(4), 0);
    dst.copy_from_slice(micron::raw_slice<int>{ src, 6 });
    require(dst[0], 10);
    require(dst[3], 40);
  }
  end_test_case();

  test_case("as_bytes - byte view of int slice");
  {
    micron::slice<int> s(size_t(4), 0);
    s[0] = 0x01020304;
    auto bs = s.as_bytes();
    require(bs.len, s.size() * sizeof(int));
    int recon = bs[0] | (bs[1] << 8) | (bs[2] << 16) | (bs[3] << 24);
    require(recon, 0x01020304);
  }
  end_test_case();

  test_case("as_bytes_mut - mutation via byte view affects elements");
  {
    micron::slice<int> s(size_t(2), 0);
    auto bs = s.as_bytes_mut();
    bs[0] = 0x11;
    bs[1] = 0x22;
    bs[2] = 0x33;
    bs[3] = 0x44;
    require(s[0], 0x44332211);
  }
  end_test_case();

  test_case("as_flattened<short> - int slice as short pairs");
  {
    micron::slice<int> s(size_t(3), 0);
    auto fl = s.as_flattened<short>();
    require(fl.len, s.size() * sizeof(int) / sizeof(short));
  }
  end_test_case();

  test_case("align_to<int> - aligned source yields empty prefix/suffix");
  {
    micron::slice<int> s(size_t(8), 0);
    auto a = s.align_to<int>();
    require(a.prefix.len, size_t(0));
    require(a.suffix.len, size_t(0));
    require(a.middle.len, size_t(8));
  }
  end_test_case();

  test_case("is_ascii - all-ASCII text");
  {
    const char *str = "hello, world";
    micron::slice<char> s(str, str + std::strlen(str));
    require_true(s.is_ascii());
  }
  end_test_case();

  test_case("is_ascii - byte >= 128 returns false");
  {
    micron::slice<char> s(size_t(3), '\0');
    s[0] = 'a';
    s[1] = static_cast<char>(0xC3);
    s[2] = 'b';
    require_false(s.is_ascii());
  }
  end_test_case();

  test_case("is_ascii - empty slice is ASCII");
  {
    micron::slice<char> s(size_t(1), 'a');
    s.mark(0);
    require_true(s.is_ascii());
  }
  end_test_case();

  test_case("as_ascii - returns view when valid");
  {
    const char *str = "abcXYZ";
    micron::slice<char> s(str, str + std::strlen(str));
    auto a = s.as_ascii();
    require(a.len, s.size());
  }
  end_test_case();

  test_case("as_ascii - non-ASCII returns empty");
  {
    micron::slice<char> s(size_t(2), '\0');
    s[0] = static_cast<char>(0xFE);
    s[1] = 'a';
    auto a = s.as_ascii();
    require(a.len, size_t(0));
  }
  end_test_case();

  test_case("to_ascii_uppercase - lowercase letters lifted");
  {
    const char *str = "Hello World 123";
    micron::slice<char> s(str, str + std::strlen(str));
    s.to_ascii_uppercase();
    const char *exp = "HELLO WORLD 123";
    for ( size_t i = 0; i < s.size(); ++i ) require_true(s[i] == exp[i]);
  }
  end_test_case();

  test_case("to_ascii_lowercase - uppercase letters dropped");
  {
    const char *str = "Hello World 123";
    micron::slice<char> s(str, str + std::strlen(str));
    s.to_ascii_lowercase();
    const char *exp = "hello world 123";
    for ( size_t i = 0; i < s.size(); ++i ) require_true(s[i] == exp[i]);
  }
  end_test_case();

  test_case("trim_ascii_start - leading whitespace removed");
  {
    const char *str = "   hello";
    micron::slice<char> s(str, str + std::strlen(str));
    auto r = s.trim_ascii_start();
    require(r.len, size_t(5));
    require_true(r[0] == 'h');
  }
  end_test_case();

  test_case("trim_ascii_end - trailing whitespace removed");
  {
    const char *str = "hello\t\n";
    micron::slice<char> s(str, str + std::strlen(str));
    auto r = s.trim_ascii_end();
    require(r.len, size_t(5));
    require_true(r[r.len - 1] == 'o');
  }
  end_test_case();

  test_case("trim_ascii - both sides");
  {
    const char *str = " \t hi \n ";
    micron::slice<char> s(str, str + std::strlen(str));
    auto r = s.trim_ascii();
    require(r.len, size_t(2));
    require_true(r[0] == 'h');
    require_true(r[1] == 'i');
  }
  end_test_case();

  test_case("heavy - fill + reverse + sum on 10k slice");
  {
    micron::slice<int> s(size_t(10'000), 0);
    s.write_with([](size_t i) { return (int)i + 1; });
    s.reverse();
    require(s[0], 10'000);
    require(s[9'999], 1);
    long long total = 0;
    for ( size_t i = 0; i < s.size(); ++i ) total += s[i];
    require(total, (long long)10'000 * 10'001 / 2);
  }
  end_test_case();

  test_case("heavy - random ops vs std::vector ground truth");
  {
    const size_t N = 1000;
    micron::slice<int> s(N, 0);
    std::vector<int> truth(N);
    std::mt19937 rng(0xDECAFC0FFEEull);
    for ( size_t i = 0; i < N; ++i ) {
      int v = std::uniform_int_distribution<int>(-1'000, 1'000)(rng);
      s[i] = v;
      truth[i] = v;
    }
    for ( int op = 0; op < 5'000; ++op ) {
      int c = std::uniform_int_distribution<int>(0, 4)(rng);
      size_t i = std::uniform_int_distribution<size_t>(0, N - 1)(rng);
      size_t j = std::uniform_int_distribution<size_t>(0, N - 1)(rng);
      if ( c == 0 ) {
        int v = std::uniform_int_distribution<int>(-1'000, 1'000)(rng);
        s[i] = v;
        truth[i] = v;
      } else if ( c == 1 ) {
        s.swap(i, j);
        std::swap(truth[i], truth[j]);
      } else if ( c == 2 ) {
        require(s[i], truth[i]);
      } else if ( c == 3 ) {
        require_true(s.contains(truth[i]));
      } else {
        require(*s.get(i), truth[i]);
      }
    }
    for ( size_t i = 0; i < N; ++i ) require(s[i], truth[i]);
  }
  end_test_case();

  test_case("heavy - repeat large pattern");
  {
    micron::slice<int> s(size_t(64), 0);
    s.write_with([](size_t i) { return (int)i; });
    auto r = s.repeat(8);
    require_true(r.size() >= size_t(512));
    for ( size_t rep = 0; rep < 8; ++rep )
      for ( size_t i = 0; i < 64; ++i ) require(r[rep * 64 + i], (int)i);
  }
  end_test_case();

  test_case("heavy - rotate_left preserves multi-set across many strides");
  {
    const size_t N = 200;
    micron::slice<int> s(N, 0);
    for ( size_t i = 0; i < N; ++i ) s[i] = (int)i;
    std::vector<int> snapshot;
    for ( size_t i = 0; i < N; ++i ) snapshot.push_back((int)i);
    for ( size_t k = 1; k < 32; ++k ) {
      s.rotate_left(k);
      std::vector<int> seen;
      for ( size_t i = 0; i < s.size(); ++i ) seen.push_back(s[i]);
      std::sort(seen.begin(), seen.end());
      require_true(seen == snapshot);
    }
  }
  end_test_case();

  test_case("heavy - large split callback count");
  {
    micron::slice<int> s(size_t(1001), 1);
    for ( size_t i = 0; i < s.size(); ++i ) s[i] = (i % 10 == 0) ? 0 : 1;
    int hits = 0;
    size_t produced = s.split([](const int &x) { return x == 0; }, [&](micron::raw_slice<int>) { ++hits; });
    require(produced, size_t(102));
    require(hits, 102);
  }
  end_test_case();

  test_case("Tracked - many slice ops keep ctor and dtor counts balanced after scope");
  {
    reset_tracked();
    {
      micron::slice<Tracked> s(size_t(50), Tracked(7));
      for ( size_t i = 0; i < s.size(); ++i ) require(s[i].v, 7);
      s.reverse();
      s.swap(0, 49);
      s.rotate_left(10);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
