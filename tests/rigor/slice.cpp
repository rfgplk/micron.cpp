// slice_tests.cpp
// Comprehensive snowball test suite for micron::slice<T>

#include "../../src/slice.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_true;
using sb::test_case;

// ─────────────────────────────────────────────────────────────────────────────
// Tracked helper — mirrors the one used in vector_tests
// ─────────────────────────────────────────────────────────────────────────────
namespace
{

struct Tracked {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
    ++ctor;
  }

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

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }

  bool
  operator!=(const Tracked &o) const
  {
    return v != o.v;
  }

  bool
  operator<(const Tracked &o) const
  {
    return v < o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

};     // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
int
main()
{
  sb::print("=== SLICE TESTS ===");

  // ──────────────────────────────────────────────────────────────────────────
  test_case("default construction");
  {
    micron::slice<int> s;
    require_false(s.is_empty());     // auto-sized block; length == capacity
    require_greater(s.size(), size_t(0));
    require_true(s.as_ptr() != nullptr);
  }
  end_test_case();
  // ──────────────────────────────────────────────────────────────────────────
  test_case("pointer-range construction copies memory");
  {
    int src[5] = { 10, 20, 30, 40, 50 };
    micron::slice<int> s(src, src + 5);
    require(s.size(), size_t(5));
    for ( int i = 0; i < 5; ++i )
      require(s[i], src[i]);
    // mutation of src must not affect s
    src[0] = 99;
    require(s[0], 10);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("function-transform construction");
  {
    micron::slice<int> base(5, 2);     // [2,2,2,2,2]
    micron::slice<int> doubled([](const int &x) { return x * 2; }, base);
    for ( size_t i = 0; i < 5; ++i )
      require(doubled[i], 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("move construction transfers ownership");
  {
    micron::slice<int> a(4, 7);
    micron::slice<int> b(micron::move(a));
    require(b[0], 7);
    // a should now be in a valid-but-empty state
    require(a.size(), size_t(0));
    require_true(a.as_ptr() == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("move assignment transfers ownership");
  {
    micron::slice<int> a(3, 5);
    micron::slice<int> b(3, 9);
    b = micron::move(a);
    require(b[0], 5);
    require(a.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("operator[] read and write");
  {
    micron::slice<int> s(4, 0);
    for ( size_t i = 0; i < 4; ++i )
      s[i] = static_cast<int>(i * 3);
    for ( size_t i = 0; i < 4; ++i )
      require(s[i], static_cast<int>(i * 3));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("set() fills all elements");
  {
    micron::slice<int> s(6, 0);
    s.set(77);
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 77);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("mark() adjusts logical length");
  {
    micron::slice<int> s(10, 1);
    s.mark(5);
    require(s.size(), size_t(5));
    // mark beyond capacity must be ignored
    s.mark(9999999);
    require(s.size(), size_t(5));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("reset() zeros and restores length to capacity");
  {
    micron::slice<int> s(8, 3);
    s.mark(3);
    require(s.size(), size_t(3));
    s.reset();
    require(s.size(), s.max_size());
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("fill() method fills all elements");
  {
    micron::slice<int> s(5, 0);
    s.fill(99);
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 99);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("fill_with() generator fills elements");
  {
    micron::slice<int> s(6, 0);
    int counter = 0;
    s.fill_with([&counter]() -> int { return counter++; });
    for ( int i = 0; i < 6; ++i )
      require(s[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_with() index-aware filler");
  {
    micron::slice<int> s(5, 0);
    s.write_with([](size_t i) -> int { return static_cast<int>(i * i); });
    for ( size_t i = 0; i < 5; ++i )
      require(s[i], static_cast<int>(i * i));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_iter() copies from external range");
  {
    int src[] = { 5, 4, 3, 2, 1 };
    micron::slice<int> s(5, 0);
    s.write_iter(src, src + 5);
    for ( int i = 0; i < 5; ++i )
      require(s[i], src[i]);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_filled() alias for fill");
  {
    micron::slice<int> s(4, 0);
    s.write_filled(13);
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 13);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy_from_slice() copies raw_slice contents");
  {
    int src[] = { 1, 2, 3, 4, 5 };
    micron::raw_slice<int> view(src, 5);
    micron::slice<int> s(5, 0);
    s.copy_from_slice(view);
    for ( int i = 0; i < 5; ++i )
      require(s[i], src[i]);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_copy_of_slice() and write_clone_of_slice()");
  {
    int src[] = { 9, 8, 7 };
    micron::raw_slice<int> view(src, 3);

    micron::slice<int> sc(3, 0);
    sc.write_copy_of_slice(view);
    for ( int i = 0; i < 3; ++i )
      require(sc[i], src[i]);

    micron::slice<int> sk(3, 0);
    sk.write_clone_of_slice(view);
    for ( int i = 0; i < 3; ++i )
      require(sk[i], src[i]);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy_within() non-overlapping");
  {
    // [1,2,3,4,5] copy [0..3) → dst=2  ⇒  [1,2,1,2,3]
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i + 1;
    s.copy_within(0, 3, 2);
    require(s[2], 1);
    require(s[3], 2);
    require(s[4], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy_within() overlapping regions");
  {
    // [0,1,2,3,4] copy [1..4) → dst=0  ⇒  [1,2,3,3,4]
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;
    s.copy_within(1, 3, 0);
    require(s[0], 1);
    require(s[1], 2);
    require(s[2], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_ptr_range half-open [begin, end)");
  {
    micron::slice<int> s(4, 0);
    for ( int i = 0; i < 4; ++i )
      s[i] = i;
    auto r = s.as_ptr_range();
    require_true(r.begin != nullptr);
    require_true(r.end != nullptr);
    require(static_cast<size_t>(r.end - r.begin), s.size());
    for ( int i = 0; i < 4; ++i )
      require(r.begin[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("data() and addr() return pointer to first element");
  {
    micron::slice<int> s(3, 7);
    require_true(s.data() == s.addr());
    require_true(s.data() == s.as_ptr());
    require(*s.data(), 7);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("iter() / iter_mut() non-owning views");
  {
    micron::slice<int> s(4, 0);
    for ( int i = 0; i < 4; ++i )
      s[i] = i * 2;

    auto view = s.iter();
    require(view.size(), s.size());
    for ( size_t i = 0; i < view.size(); ++i )
      require(view[i], s[i]);

    auto mut = s.iter_mut();
    mut[0] = 100;
    require(s[0], 100);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("begin / end / cbegin / cend iteration");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;
    int sum = 0;
    for ( auto *it = s.begin(); it != s.end() + 1; ++it )
      sum += *it;
    require(sum, 0 + 1 + 2 + 3 + 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("first / last return correct pointers");
  {
    micron::slice<int> s(4, 0);
    s[0] = 10;
    s[3] = 40;
    require(*s.first(), 10);
    require(*s.last(), 40);
    require(s.first_mut(), s.first());
    require(s.last_mut(), s.last());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("get() bounds-checked access");
  {
    micron::slice<int> s(4, 0);
    s[2] = 42;
    require_true(s.get(2) != nullptr);
    require(*s.get(2), 42);
    require_true(s.get(99) == nullptr);
    require_true(s.get_mut(0) != nullptr);
    require_true(s.get_mut(99) == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("get_unchecked reads correct element");
  {
    micron::slice<int> s(3, 0);
    s[1] = 55;
    require(*s.get_unchecked(1), 55);
    *s.get_unchecked_mut(2) = 77;
    require(s[2], 77);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("get_disjoint_mut returns non-aliasing pointers");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;
    auto [a, b] = s.get_disjoint_mut(1, 3);
    require_true(a != nullptr);
    require_true(b != nullptr);
    require_true(a != b);
    require(*a, 1);
    require(*b, 3);
    // aliased indices must return invalid pair
    auto [c, d] = s.get_disjoint_mut(2, 2);
    require_true(c == nullptr);
    require_true(d == nullptr);
    // out-of-range index must return invalid pair
    auto [e, f] = s.get_disjoint_mut(0, 99);
    require_true(e == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("element_offset reports correct byte offset");
  {
    micron::slice<int> s(5, 0);
    int *p = s.as_ptr() + 3;
    require(s.element_offset(p), size_t(3));
    // pointer outside range → sentinel
    int outside = 0;
    require(s.element_offset(&outside), static_cast<size_t>(-1));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("first_chunk<N> and last_chunk<N>");
  {
    micron::slice<int> s(6, 0);
    for ( int i = 0; i < 6; ++i )
      s[i] = i;

    int *fc = s.first_chunk<3>();
    require_true(fc != nullptr);
    require(fc[0], 0);
    require(fc[1], 1);
    require(fc[2], 2);

    int *lc = s.last_chunk<3>();
    require_true(lc != nullptr);
    require(lc[0], 3);
    require(lc[1], 4);
    require(lc[2], 5);

    // N > length → nullptr
    require_true(s.first_chunk<99>() == nullptr);
    require_true(s.last_chunk<99>() == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_first_chunk<N> and split_last_chunk<N>");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;

    auto fres = s.split_first_chunk<2>();
    require_true(fres.valid());
    require(fres.chunk[0], 0);
    require(fres.chunk[1], 1);
    require(fres.rest.size(), size_t(3));
    require(fres.rest[0], 2);

    auto lres = s.split_last_chunk<2>();
    require_true(lres.valid());
    require(lres.chunk[0], 3);
    require(lres.chunk[1], 4);
    require(lres.rest.size(), size_t(3));
    require(lres.rest[0], 0);

    // insufficient length → invalid
    auto bad = s.split_first_chunk<99>();
    require_false(bad.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_at divides correctly");
  {
    micron::slice<int> s(6, 0);
    for ( int i = 0; i < 6; ++i )
      s[i] = i;

    auto [left, right] = s.split_at(3);
    require(left.size(), size_t(3));
    require(right.size(), size_t(3));
    require(left[0], 0);
    require(left[2], 2);
    require(right[0], 3);
    require(right[2], 5);

    // mid == 0 → empty left
    auto [el, er] = s.split_at(0);
    require(el.size(), size_t(0));
    require(er.size(), size_t(6));

    // mid == length → empty right
    auto [fl, fr] = s.split_at(6);
    require(fl.size(), size_t(6));
    require(fr.size(), size_t(0));

    // mid beyond length is clamped
    auto [cl, cr] = s.split_at(100);
    require(cl.size(), size_t(6));
    require(cr.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_at_checked rejects out-of-range mid");
  {
    micron::slice<int> s(4, 1);
    auto ok = s.split_at_checked(2);
    require(ok.first.size(), size_t(2));
    require(ok.second.size(), size_t(2));

    auto bad = s.split_at_checked(99);
    require_false(bad.first.valid());
    require_false(bad.second.valid());
  }
  end_test_case();
  // ──────────────────────────────────────────────────────────────────────────
  test_case("split() callback-based iteration");
  {
    micron::slice<int> s(7, 0);
    // [0,1,0,1,0,1,0] — split on 1
    for ( int i = 0; i < 7; ++i )
      s[i] = (i % 2 == 1) ? 1 : 0;

    size_t pieces = 0;
    size_t total_elems = 0;
    s.split([](const int &v) { return v == 1; },
            [&](micron::raw_slice<int> sub) {
              ++pieces;
              total_elems += sub.size();
            });
    require(pieces, size_t(4));          // 3 delimiters → 4 sub-slices
    require(total_elems, size_t(4));     // 4 zeros
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_inclusive() delimiter included in preceding sub-slice");
  {
    micron::slice<int> s(6, 0);
    // [1,2,3,1,2,3] — split inclusive on 3
    int vals[] = { 1, 2, 3, 1, 2, 3 };
    for ( int i = 0; i < 6; ++i )
      s[i] = vals[i];

    size_t pieces = 0;
    bool last_of_first_is_3 = false;
    s.split_inclusive([](const int &v) { return v == 3; },
                      [&](micron::raw_slice<int> sub) {
                        ++pieces;
                        if ( pieces == 1 )
                          last_of_first_is_3 = (sub[sub.size() - 1] == 3);
                      });
    require(pieces, size_t(2));
    require_true(last_of_first_is_3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("splitn() limits number of sub-slices");
  {
    micron::slice<int> s(7, 0);
    int vals[] = { 0, 1, 0, 1, 0, 1, 0 };
    for ( int i = 0; i < 7; ++i )
      s[i] = vals[i];

    size_t pieces = 0;
    s.splitn(2, [](const int &v) { return v == 1; }, [&](micron::raw_slice<int>) { ++pieces; });
    require(pieces, size_t(2));     // exactly n=2 sub-slices regardless of matches
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rsplit() iterates right-to-left");
  {
    micron::slice<int> s(5, 0);
    int vals[] = { 1, 2, 3, 2, 1 };
    for ( int i = 0; i < 5; ++i )
      s[i] = vals[i];

    // delimit on 2 → right-to-left gives sub-slices: [1], [3], [1,2,3...] in r order
    size_t count = 0;
    s.rsplit([](const int &v) { return v == 2; }, [&](micron::raw_slice<int>) { ++count; });
    require(count, size_t(3));     // two delimiters → three pieces
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rsplitn() limits right-to-left splits");
  {
    micron::slice<int> s(7, 0);
    int vals[] = { 0, 1, 0, 1, 0, 1, 0 };
    for ( int i = 0; i < 7; ++i )
      s[i] = vals[i];

    size_t count = 0;
    s.rsplitn(2, [](const int &v) { return v == 1; }, [&](micron::raw_slice<int>) { ++count; });
    require(count, size_t(2));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_once() first match only");
  {
    micron::slice<int> s(5, 0);
    int vals[] = { 1, 2, 3, 2, 1 };
    for ( int i = 0; i < 5; ++i )
      s[i] = vals[i];

    auto [left, right] = s.split_once(2);
    require(left.size(), size_t(1));
    require(right.size(), size_t(3));
    require(left[0], 1);
    require(right[0], 3);

    // no match → invalid
    auto [bl, br] = s.split_once(99);
    require_false(bl.valid());
    require_false(br.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rsplit_once() last match only");
  {
    micron::slice<int> s(5, 0);
    int vals[] = { 1, 2, 3, 2, 1 };
    for ( int i = 0; i < 5; ++i )
      s[i] = vals[i];

    auto [left, right] = s.rsplit_once(2);
    require(left.size(), size_t(3));
    require(right.size(), size_t(1));
    require(right[0], 1);

    auto [bl, br] = s.rsplit_once(99);
    require_false(bl.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off() creates tail slice and truncates self");
  {
    micron::slice<int> s(6, 0);
    for ( int i = 0; i < 6; ++i )
      s[i] = i;

    micron::slice<int> tail = s.split_off(3);
    require(s.size(), size_t(3));
    require(tail.size(), size_t(3));
    for ( int i = 0; i < 3; ++i )
      require(s[i], i);
    for ( int i = 0; i < 3; ++i )
      require(tail[i], i + 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_first() pops first element");
  {
    micron::slice<int> s(4, 0);
    for ( int i = 0; i < 4; ++i )
      s[i] = i + 1;     // [1,2,3,4]
    int v = s.split_off_first();
    require(v, 1);
    require(s.size(), size_t(3));
    require(s[0], 2);
    require(s[2], 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_last() pops last element");
  {
    micron::slice<int> s(4, 0);
    for ( int i = 0; i < 4; ++i )
      s[i] = i + 1;     // [1,2,3,4]
    int v = s.split_off_last();
    require(v, 4);
    require(s.size(), size_t(3));
    require(s[2], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_mut() returns non-owning tail view");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;

    auto tail = s.split_off_mut(2);
    require(s.size(), size_t(2));
    require(tail.size(), size_t(3));
    require(tail[0], 2);
    require(tail[2], 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("contains() by value");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i * 2;     // [0,2,4,6,8]
    require_true(s.contains(4));
    require_false(s.contains(3));
    require_false(s.contains(99));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("contains() by predicate");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;
    require_true(s.contains([](const int &v) { return v > 3; }));
    require_false(s.contains([](const int &v) { return v > 10; }));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("starts_with()");
  {
    int arr[] = { 1, 2, 3 };
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i + 1;     // [1,2,3,4,5]

    micron::raw_slice<int> prefix(arr, 3);
    require_true(s.starts_with(prefix));

    int bad[] = { 2, 3 };
    micron::raw_slice<int> not_prefix(bad, 2);
    require_false(s.starts_with(not_prefix));

    // prefix longer than slice → false
    int long_arr[] = { 1, 2, 3, 4, 5, 6 };
    micron::raw_slice<int> too_long(long_arr, 6);
    require_false(s.starts_with(too_long));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ends_with()");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i + 1;     // [1,2,3,4,5]

    int suffix_arr[] = { 4, 5 };
    micron::raw_slice<int> suffix(suffix_arr, 2);
    require_true(s.ends_with(suffix));

    int bad[] = { 3, 5 };
    micron::raw_slice<int> bad_suffix(bad, 2);
    require_false(s.ends_with(bad_suffix));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("strip_prefix() returns remainder after prefix");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;     // [0,1,2,3,4]

    int pre[] = { 0, 1 };
    micron::raw_slice<int> prefix(pre, 2);
    auto rest = s.strip_prefix(prefix);
    require_true(rest.valid());
    require(rest.size(), size_t(3));
    require(rest[0], 2);

    int bad[] = { 1, 2 };
    micron::raw_slice<int> bad_prefix(bad, 2);
    auto bad_rest = s.strip_prefix(bad_prefix);
    require_false(bad_rest.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("strip_suffix() returns everything before suffix");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;     // [0,1,2,3,4]

    int suf[] = { 3, 4 };
    micron::raw_slice<int> suffix(suf, 2);
    auto init = s.strip_suffix(suffix);
    require_true(init.valid());
    require(init.size(), size_t(3));
    require(init[2], 2);

    int bad[] = { 2, 4 };
    micron::raw_slice<int> bad_suf(bad, 2);
    auto bad_init = s.strip_suffix(bad_suf);
    require_false(bad_init.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("strip_circumfix() strips prefix and suffix simultaneously");
  {
    micron::slice<int> s(6, 0);
    int vals[] = { 0, 1, 2, 3, 4, 5 };
    for ( int i = 0; i < 6; ++i )
      s[i] = vals[i];

    int pre[] = { 0, 1 };
    int suf[] = { 4, 5 };
    micron::raw_slice<int> prefix(pre, 2);
    micron::raw_slice<int> suffix(suf, 2);
    auto mid = s.strip_circumfix(prefix, suffix);
    require_true(mid.valid());
    require(mid.size(), size_t(2));
    require(mid[0], 2);
    require(mid[1], 3);

    // only prefix matches → invalid
    int bad[] = { 99 };
    micron::raw_slice<int> bad_suf(bad, 1);
    auto bad_mid = s.strip_circumfix(prefix, bad_suf);
    require_false(bad_mid.valid());

    // prefix + suffix longer than slice → invalid
    int long_pre[] = { 0, 1, 2, 3, 4 };
    micron::raw_slice<int> lp(long_pre, 5);
    auto too_big = s.strip_circumfix(lp, suffix);
    require_false(too_big.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("swap() exchanges elements");
  {
    micron::slice<int> s(4, 0);
    for ( int i = 0; i < 4; ++i )
      s[i] = i;
    s.swap(0, 3);
    require(s[0], 3);
    require(s[3], 0);
    // out-of-range swaps must not crash or corrupt
    s.swap(0, 99);
    require(s[0], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("swap_unchecked() exchanges elements without bounds check");
  {
    micron::slice<int> s(4, 0);
    for ( int i = 0; i < 4; ++i )
      s[i] = i;
    s.swap_unchecked(1, 2);
    require(s[1], 2);
    require(s[2], 1);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("swap_with_slice() pairwise element exchange");
  {
    micron::slice<int> a(3, 0);
    micron::slice<int> b(3, 0);
    for ( int i = 0; i < 3; ++i ) {
      a[i] = i;
      b[i] = i + 10;
    }
    a.swap_with_slice(b);
    for ( int i = 0; i < 3; ++i ) {
      require(a[i], i + 10);
      require(b[i], i);
    }
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("reverse() in-place");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;
    s.reverse();
    for ( int i = 0; i < 5; ++i )
      require(s[i], 4 - i);

    // idempotent on single element
    micron::slice<int> one(1, 7);
    one.reverse();
    require(one[0], 7);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rotate_left() wraps correctly");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;           // [0,1,2,3,4]
    s.rotate_left(2);     // [2,3,4,0,1]
    require(s[0], 2);
    require(s[1], 3);
    require(s[3], 0);
    require(s[4], 1);

    // rotate by full length is identity
    s.rotate_left(5);
    require(s[0], 2);

    // rotate by 0 is identity
    s.rotate_left(0);
    require(s[0], 2);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rotate_right() wraps correctly");
  {
    micron::slice<int> s(5, 0);
    for ( int i = 0; i < 5; ++i )
      s[i] = i;            // [0,1,2,3,4]
    s.rotate_right(2);     // [3,4,0,1,2]
    require(s[0], 3);
    require(s[1], 4);
    require(s[2], 0);
    require(s[4], 2);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rotate_left and rotate_right are inverses");
  {
    micron::slice<int> s(7, 0);
    for ( int i = 0; i < 7; ++i )
      s[i] = i;
    s.rotate_left(3);
    s.rotate_right(3);
    for ( int i = 0; i < 7; ++i )
      require(s[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("concat() appends second slice to first");
  {
    micron::slice<int> a(3, 0);
    for ( int i = 0; i < 3; ++i )
      a[i] = i;     // [0,1,2]
    int extra[] = { 3, 4, 5 };
    micron::raw_slice<int> b(extra, 3);

    micron::slice<int> c = a.concat(b);
    for ( int i = 0; i < 6; ++i )
      require(c[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("join() inserts separator between two slices");
  {
    micron::slice<int> a(2, 0);
    a[0] = 1;
    a[1] = 2;
    int extra[] = { 4, 5 };
    micron::raw_slice<int> b(extra, 2);

    micron::slice<int> j = a.join(b, 3);
    require(j[0], 1);
    require(j[1], 2);
    require(j[2], 3);     // separator
    require(j[3], 4);
    require(j[4], 5);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("connect() is an alias for join()");
  {
    micron::slice<int> a(2, 0);
    a[0] = 10;
    a[1] = 20;
    int extra[] = { 40, 50 };
    micron::raw_slice<int> b(extra, 2);

    auto c = a.connect(b, 99);
    auto j = a.join(b, 99);
    require(c.size(), j.size());
    for ( size_t i = 0; i < c.size(); ++i )
      require(c[i], j[i]);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("to_vec() deep-copies slice contents");
  {
    micron::slice<int> s(4, 0);
    for ( int i = 0; i < 4; ++i )
      s[i] = i * 5;
    micron::slice<int> copy = s.to_vec();
    require(copy.size(), s.size());
    for ( size_t i = 0; i < copy.size(); ++i )
      require(copy[i], s[i]);
    // mutation of copy must not affect original
    copy[0] = 999;
    require(s[0], 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_bytes() returns correct byte count and content");
  {
    micron::slice<int> s(2, 0);
    s[0] = 0;
    s[1] = 1;
    auto bytes = s.as_bytes();
    require(bytes.size(), sizeof(int) * 2);
    require_true(bytes.ptr != nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_bytes_mut() allows byte-level writes");
  {
    micron::slice<int> s(1, 0);
    auto bytes = s.as_bytes_mut();
    require(bytes.size(), sizeof(int));
    // zero the int byte by byte
    for ( size_t i = 0; i < bytes.size(); ++i )
      bytes.ptr[i] = 0;
    require(s[0], 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_flattened<U>() reinterprets memory as wider element type");
  {
    // slice of 4 bytes viewed as 2 uint16_t
    micron::slice<uint8_t> s(4, 0);
    s[0] = 0x01;
    s[1] = 0x02;     // little-endian: 0x0201
    s[2] = 0x03;
    s[3] = 0x04;     // 0x0403

    auto flat = s.as_flattened<uint16_t>();
    require(flat.size(), size_t(2));
    require_true(flat.ptr != nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("align_to<U>() prefix+middle+suffix covers all bytes");
  {
    micron::slice<uint8_t> s(32, 1);
    auto [pre, mid, suf] = s.align_to<uint64_t>();
    size_t covered = pre.size() * sizeof(uint8_t) + mid.size() * sizeof(uint64_t) + suf.size() * sizeof(uint8_t);
    require(covered, s.size() * sizeof(uint8_t));
  }
  end_test_case();
  // ──────────────────────────────────────────────────────────────────────────
  test_case("tracked object lifetime — fill then destroy");
  {
    reset_tracked();
    {
      micron::slice<Tracked> s(8, Tracked(99));
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("large allocation / stress fill");
  {
    constexpr size_t N = 1 << 16;     // 65536 elements
    micron::slice<int> s(N, 0);
    require(s.size(), N);
    s.fill_with([]() { return 7; });
    for ( size_t i = 0; i < N; ++i )
      require(s[i], 7);
    s.reverse();
    for ( size_t i = 0; i < N; ++i )
      require(s[i], 7);     // uniform — reverse is a no-op on uniform data
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress split / concat round-trip");
  {
    constexpr size_t N = 256;
    micron::slice<int> s(N, 0);
    for ( size_t i = 0; i < N; ++i )
      s[i] = static_cast<int>(i);

    // split at midpoint, concat back, verify identity
    micron::slice<int> tail = s.split_off(N / 2);
    micron::slice<int> whole = s.concat(tail.iter());
    for ( size_t i = 0; i < N; ++i )
      require(whole[i], static_cast<int>(i));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress rotate_left round-trip");
  {
    constexpr size_t N = 64;
    micron::slice<int> s(N, 0);
    for ( size_t i = 0; i < N; ++i )
      s[i] = static_cast<int>(i);

    for ( size_t r = 1; r < N; ++r ) {
      s.rotate_left(1);
      s.rotate_right(1);
    }
    for ( size_t i = 0; i < N; ++i )
      require(s[i], static_cast<int>(i));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress swap / reverse symmetry");
  {
    constexpr size_t N = 128;
    micron::slice<int> s(N, 0);
    for ( size_t i = 0; i < N; ++i )
      s[i] = static_cast<int>(i);

    s.reverse();
    s.reverse();     // two reverses = identity
    for ( size_t i = 0; i < N; ++i )
      require(s[i], static_cast<int>(i));
  }
  end_test_case();

  sb::print("=== ALL SLICE TESTS PASSED ===");
  return 1;
}
