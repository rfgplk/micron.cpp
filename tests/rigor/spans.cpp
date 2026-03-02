// span_tests.cpp
// Comprehensive snowball test suite for micron::span<T, N>

#include "../../src/io/console.hpp"
#include "../../src/span.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_smaller;
using sb::require_true;
using sb::test_case;

// ─────────────────────────────────────────────────────────────────────────────
// Tracked helper — counts ctor/dtor invocations for lifetime tests
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
  sb::print("=== SPAN TESTS ===");

  // ──────────────────────────────────────────────────────────────────────────
  test_case("default construction — zero length, N capacity");
  {
    micron::span<int> s;
    require_true(s.is_empty());
    require(s.size(), size_t(0));
    require(s.max_size(), size_t(64));       // default N
    require_true(s.as_ptr() != nullptr);     // stack storage always valid
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("explicit count construction zero-initialises elements");
  {
    micron::span<int, 16> s(8);
    require(s.size(), size_t(8));
    for ( size_t i = 0; i < 8; ++i )
      require(s[i], 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("count + value construction fills correctly");
  {
    micron::span<int, 32> s(10, 42);
    require(s.size(), size_t(10));
    for ( size_t i = 0; i < 10; ++i )
      require(s[i], 42);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("pointer-range construction copies memory");
  {
    int src[5] = { 10, 20, 30, 40, 50 };
    micron::span<int, 16> s(src, src + 5);
    require(s.size(), size_t(5));
    for ( int i = 0; i < 5; ++i )
      require(s[i], src[i]);
    // mutation of src must not affect s
    src[0] = 999;
    require(s[0], 10);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("raw_slice construction copies contents");
  {
    int arr[] = { 7, 14, 21 };
    micron::raw_slice<int> view(arr, 3);
    micron::span<int, 16> s(view);
    require(s.size(), size_t(3));
    for ( int i = 0; i < 3; ++i )
      require(s[i], arr[i]);
    // mutation of arr must not affect s
    arr[1] = 0;
    require(s[1], 14);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("initializer_list construction");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4 });
    require(s.size(), size_t(4));
    require(s[0], 1);
    require(s[1], 2);
    require(s[2], 3);
    require(s[3], 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("generator construction — zero-arg callable");
  {
    int counter = 0;
    micron::span<int, 16> s(6, [&counter]() -> int { return counter++; });
    for ( int i = 0; i < 6; ++i )
      require(s[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy construction produces independent copy");
  {
    micron::span<int, 8> a(4, 77);
    micron::span<int, 8> b(a);
    require(b.size(), a.size());
    for ( size_t i = 0; i < b.size(); ++i )
      require(b[i], 77);
    // mutate b — a must be unaffected
    b[0] = 0;
    require(a[0], 77);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy construction from different capacity span");
  {
    micron::span<int, 8> small(4, 3);
    micron::span<int, 16> large(small);
    require(large.size(), size_t(4));
    for ( size_t i = 0; i < large.size(); ++i )
      require(large[i], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("move construction transfers data, source becomes empty");
  {
    micron::span<int, 8> a(4, 55);
    micron::span<int, 8> b(micron::move(a));
    require(b.size(), size_t(4));
    for ( size_t i = 0; i < b.size(); ++i )
      require(b[i], 55);
    require(a.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("move construction from different capacity span");
  {
    micron::span<int, 16> big(6, 9);
    micron::span<int, 8> small(micron::move(big));
    // small can hold at most 8; big had 6 — all 6 should transfer
    require(small.size(), size_t(6));
    for ( size_t i = 0; i < small.size(); ++i ) {
      require(small[i], 9);
    }
    require(big.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy assignment produces independent copy");
  {
    micron::span<int, 8> a(3, 10);
    micron::span<int, 8> b;
    b = a;
    require(b.size(), a.size());
    require(b[0], 10);
    b[0] = 99;
    require(a[0], 10);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("move assignment transfers data, source becomes empty");
  {
    micron::span<int, 8> a(5, 3);
    micron::span<int, 8> b;
    b = micron::move(a);
    require(b.size(), size_t(5));
    for ( size_t i = 0; i < b.size(); ++i )
      require(b[i], 3);
    require(a.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("operator[] read and write");
  {
    micron::span<int, 16> s(6, 0);
    for ( size_t i = 0; i < 6; ++i )
      s[i] = static_cast<int>(i * 5);
    for ( size_t i = 0; i < 6; ++i )
      require(s[i], static_cast<int>(i * 5));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("operator[](from, to) returns sub-span");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4, 5 });
    auto sub = s[size_t(2), size_t(5)];
    require(sub.size(), size_t(3));
    require(sub[0], 2);
    require(sub[1], 3);
    require(sub[2], 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("operator[]() returns full copy of active region");
  {
    micron::span<int, 16> s({ 10, 20, 30 });
    auto copy = s[];
    require(copy.size(), size_t(3));
    require(copy[0], 10);
    require(copy[2], 30);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("operator= byte fills memory");
  {
    micron::span<int, 8> s(4, 0xFFFFFFFF);
    s = static_cast<byte>(0x00);
    // all bytes should now be zero
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("operator! returns true when empty");
  {
    micron::span<int, 8> empty;
    require_true(!empty);
    micron::span<int, 8> nonempty(3, 1);
    require_false(!nonempty);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("push_back appends elements and grows length");
  {
    micron::span<int, 16> s;
    for ( int i = 0; i < 5; ++i )
      s.push_back(i * 2);
    require(s.size(), size_t(5));
    for ( int i = 0; i < 5; ++i )
      require(s[i], i * 2);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("move_back appends rvalue and grows length");
  {
    micron::span<Tracked, 16> s;
    reset_tracked();
    s.move_back(Tracked(7));
    require(s.size(), size_t(1));
    require(s[0].v, 7);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("emplace_back constructs in-place");
  {
    micron::span<Tracked, 16> s;
    reset_tracked();
    s.emplace_back(42);
    require(s.size(), size_t(1));
    require(s[0].v, 42);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("pop_back removes last element");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4 });
    s.pop_back();
    require(s.size(), size_t(3));
    require(s[2], 3);
    // pop on empty is safe (no-op)
    micron::span<int, 8> empty;
    empty.pop_back();
    require(empty.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("insert(index) shifts elements right");
  {
    micron::span<int, 16> s({ 1, 2, 4, 5 });
    s.insert(size_t(2), 3);     // insert 3 at index 2
    require(s.size(), size_t(5));
    require(s[0], 1);
    require(s[1], 2);
    require(s[2], 3);
    require(s[3], 4);
    require(s[4], 5);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("insert(iterator) shifts elements right");
  {
    micron::span<int, 16> s({ 10, 30, 40 });
    auto *itr = s.begin() + 1;     // points at 30
    s.insert(itr, 20);
    require(s.size(), size_t(4));
    require(s[0], 10);
    require(s[1], 20);
    require(s[2], 30);
    require(s[3], 40);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("erase(index) removes element and shifts left");
  {
    micron::span<int, 16> s({ 1, 2, 3, 4, 5 });
    s.erase(size_t(2));     // erase 3
    require(s.size(), size_t(4));
    require(s[0], 1);
    require(s[1], 2);
    require(s[2], 4);
    require(s[3], 5);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("clear() destroys all elements and sets length to zero");
  {
    micron::span<int, 16> s(8, 3);
    s.clear();
    require(s.size(), size_t(0));
    require_true(s.is_empty());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("fast_clear() sets length to zero (trivial types skip dtors)");
  {
    micron::span<int, 16> s(6, 99);
    s.fast_clear();
    require(s.size(), size_t(0));
    require_true(s.is_empty());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("empty() / is_empty() / full() / overflowed()");
  {
    micron::span<int, 4> s;
    require_true(s.empty());
    require_true(s.is_empty());

    s.push_back(1);
    s.push_back(2);
    s.push_back(3);
    require_false(s.empty());
    require_false(s.full());

    s.push_back(4);
    require(s.overflowed());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("size() / len() / max_size() consistency");
  {
    micron::span<int, 32> s(12, 0);
    require(s.size(), size_t(12));
    require(s.len(), size_t(12));
    require(s.max_size(), size_t(32));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("mark() adjusts logical length within capacity");
  {
    micron::span<int, 16> s(10, 1);
    s.mark(5);
    require(s.size(), size_t(5));
    // mark beyond N must be silently ignored
    s.mark(9999);
    require(s.size(), size_t(5));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("reset() zeros all N elements and restores length to N");
  {
    micron::span<int, 8> s(4, 7);
    s.mark(2);
    require(s.size(), size_t(2));
    s.reset();
    require(s.size(), size_t(8));
    for ( size_t i = 0; i < 8; ++i )
      require(s[i], 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("set() fills all active elements");
  {
    micron::span<int, 16> s(6, 0);
    s.set(33);
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 33);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("fill() fills all active elements");
  {
    micron::span<int, 16> s(5, 0);
    s.fill(77);
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 77);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("fill_with() generator fills elements");
  {
    micron::span<int, 16> s(6, 0);
    int counter = 0;
    s.fill_with([&counter]() -> int { return counter++; });
    for ( int i = 0; i < 6; ++i )
      require(s[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_filled() alias for fill");
  {
    micron::span<int, 8> s(4, 0);
    s.write_filled(55);
    for ( size_t i = 0; i < s.size(); ++i )
      require(s[i], 55);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_with() index-aware filler");
  {
    micron::span<int, 16> s(5, 0);
    s.write_with([](size_t i) -> int { return static_cast<int>(i * i); });
    for ( size_t i = 0; i < 5; ++i )
      require(s[i], static_cast<int>(i * i));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_iter() copies from external iterator range");
  {
    int src[] = { 5, 4, 3, 2, 1 };
    micron::span<int, 8> s(5, 0);
    s.write_iter(src, src + 5);
    for ( int i = 0; i < 5; ++i )
      require(s[i], src[i]);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy_from_slice() copies raw_slice contents");
  {
    int src[] = { 9, 8, 7, 6, 5 };
    micron::raw_slice<int> view(src, 5);
    micron::span<int, 16> s(5, 0);
    s.copy_from_slice(view);
    for ( int i = 0; i < 5; ++i )
      require(s[i], src[i]);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("write_copy_of_slice() and write_clone_of_slice()");
  {
    int src[] = { 3, 6, 9 };
    micron::raw_slice<int> view(src, 3);

    micron::span<int, 8> sc(3, 0);
    sc.write_copy_of_slice(view);
    for ( int i = 0; i < 3; ++i )
      require(sc[i], src[i]);

    micron::span<int, 8> sk(3, 0);
    sk.write_clone_of_slice(view);
    for ( int i = 0; i < 3; ++i )
      require(sk[i], src[i]);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy_within() non-overlapping regions");
  {
    // [1,2,3,4,5] copy [0..3) → dst=2  ⇒  [1,2,1,2,3]
    micron::span<int, 8> s({ 1, 2, 3, 4, 5 });
    s.copy_within(0, 3, 2);
    require(s[2], 1);
    require(s[3], 2);
    require(s[4], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("copy_within() overlapping regions");
  {
    // [0,1,2,3,4] copy [1..3) → dst=0  ⇒  [1,2,2,3,4]
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
    s.copy_within(1, 3, 0);
    require(s[0], 1);
    require(s[1], 2);
    require(s[2], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_ptr_range returns half-open [begin, end)");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3 });
    auto r = s.as_ptr_range();
    require_true(r.begin != nullptr);
    require_true(r.end != nullptr);
    require(static_cast<size_t>(r.end - r.begin), s.size());
    for ( int i = 0; i < 4; ++i )
      require(r.begin[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_mut_ptr_range returns mutable pointer range");
  {
    micron::span<int, 8> s(4, 0);
    auto r = s.as_mut_ptr_range();
    require_true(r.begin != nullptr);
    require(static_cast<size_t>(r.end - r.begin), s.size());
    r.begin[0] = 42;
    require(s[0], 42);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("data() / addr() / as_ptr() all point to first element");
  {
    micron::span<int, 8> s(3, 5);
    require_true(s.data() == s.addr());
    require_true(s.data() == s.as_ptr());
    require(*s.data(), 5);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("iter() / iter_mut() non-owning views");
  {
    micron::span<int, 8> s({ 2, 4, 6, 8 });
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
    micron::span<int, 16> s({ 0, 1, 2, 3, 4 });
    int sum = 0;
    for ( auto *it = s.begin(); it != s.end(); ++it )
      sum += *it;
    require(sum, 0 + 1 + 2 + 3 + 4);

    int csum = 0;
    const auto &cs = s;
    for ( auto *it = cs.cbegin(); it != cs.cend(); ++it )
      csum += *it;
    require(csum, sum);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("front() / back() return pointers to first and last");
  {
    micron::span<int, 8> s({ 10, 20, 30, 40 });
    require(*s.front(), 10);
    require(*s.back(), 40);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("first() / last() return pointer or nullptr when empty");
  {
    micron::span<int, 8> s({ 7, 8, 9 });
    require_true(s.first() != nullptr);
    require(*s.first(), 7);
    require_true(s.last() != nullptr);
    require(*s.last(), 9);
    require(s.first_mut(), s.first());
    require(s.last_mut(), s.last());

    micron::span<int, 8> empty;
    require_true(empty.first() == nullptr);
    require_true(empty.last() == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("get() bounds-checked access");
  {
    micron::span<int, 8> s({ 0, 11, 22, 33 });
    require_true(s.get(2) != nullptr);
    require(*s.get(2), 22);
    require_true(s.get(99) == nullptr);
    require_true(s.get_mut(0) != nullptr);
    require_true(s.get_mut(99) == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("get_unchecked() and get_unchecked_mut() read/write");
  {
    micron::span<int, 8> s({ 1, 2, 3 });
    require(*s.get_unchecked(1), 2);
    *s.get_unchecked_mut(2) = 99;
    require(s[2], 99);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("get_disjoint_mut returns non-aliasing pointers");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
    auto [a, b] = s.get_disjoint_mut(1, 3);
    require_true(a != nullptr);
    require_true(b != nullptr);
    require_true(a != b);
    require(*a, 1);
    require(*b, 3);
    // aliased indices → invalid
    auto [c, d] = s.get_disjoint_mut(2, 2);
    require_true(c == nullptr);
    require_true(d == nullptr);
    // out-of-range → invalid
    auto [e, f] = s.get_disjoint_mut(0, 99);
    require_true(e == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("get_disjoint_unchecked_mut returns raw pointers without checks");
  {
    micron::span<int, 8> s({ 10, 20, 30 });
    auto [a, b] = s.get_disjoint_unchecked_mut(0, 2);
    require(*a, 10);
    require(*b, 30);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("element_offset reports correct index");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
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
    micron::span<int, 16> s({ 0, 1, 2, 3, 4, 5 });
    int *fc = s.first_chunk<3>();
    require_true(fc != nullptr);
    require(fc[0], 0);
    require(fc[2], 2);

    int *lc = s.last_chunk<3>();
    require_true(lc != nullptr);
    require(lc[0], 3);
    require(lc[2], 5);

    // chunk larger than length → nullptr
    require_true(s.first_chunk<99>() == nullptr);
    require_true(s.last_chunk<99>() == nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split() callback-based iteration");
  {
    // [0,1,0,1,0,1,0] — split on 1
    micron::span<int, 16> s;
    for ( int i = 0; i < 7; ++i )
      s.push_back((i % 2 == 1) ? 1 : 0);

    size_t pieces = 0;
    size_t total_elems = 0;
    s.split([](const int &v) { return v == 1; },
            [&](micron::raw_slice<int> sub) {
              ++pieces;
              total_elems += sub.size();
            });
    require(pieces, size_t(4));          // 3 delimiters → 4 sub-slices
    require(total_elems, size_t(4));     // 4 zeros across all sub-slices
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_inclusive() delimiter included in preceding sub-slice");
  {
    micron::span<int, 16> s({ 1, 2, 3, 1, 2, 3 });
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
    micron::span<int, 16> s({ 0, 1, 0, 1, 0, 1, 0 });
    size_t pieces = 0;
    s.splitn(2, [](const int &v) { return v == 1; }, [&](micron::raw_slice<int>) { ++pieces; });
    require(pieces, size_t(2));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rsplit() iterates right-to-left");
  {
    micron::span<int, 8> s({ 1, 2, 3, 2, 1 });
    size_t count = 0;
    s.rsplit([](const int &v) { return v == 2; }, [&](micron::raw_slice<int>) { ++count; });
    require(count, size_t(3));     // two delimiters → three pieces
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rsplitn() limits right-to-left splits");
  {
    micron::span<int, 16> s({ 0, 1, 0, 1, 0, 1, 0 });
    size_t count = 0;
    s.rsplitn(2, [](const int &v) { return v == 1; }, [&](micron::raw_slice<int>) { ++count; });
    require(count, size_t(2));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_once() first match only");
  {
    micron::span<int, 8> s({ 1, 2, 3, 2, 1 });
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
  test_case("split_once() by predicate");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4, 5 });
    auto [l, r] = s.split_once([](const int &v) { return v == 3; });
    require(l.size(), size_t(2));
    require(r.size(), size_t(2));
    require(l[1], 2);
    require(r[0], 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rsplit_once() last match only");
  {
    micron::span<int, 8> s({ 1, 2, 3, 2, 1 });
    auto [left, right] = s.rsplit_once(2);
    require(left.size(), size_t(3));
    require(right.size(), size_t(1));
    require(right[0], 1);

    auto [bl, br] = s.rsplit_once(99);
    require_false(bl.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rsplit_once() by predicate");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4, 3 });
    auto [l, r] = s.rsplit_once([](const int &v) { return v == 3; });
    require(l.size(), size_t(4));
    require(r.size(), size_t(0));
    require(l[3], 4);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_mut() returns non-owning tail view and truncates");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
    auto tail = s.split_off_mut(2);
    require(s.size(), size_t(2));
    require(tail.size(), size_t(3));
    require(tail[0], 2);
    require(tail[2], 4);

    // mid beyond length → empty view, no change
    micron::span<int, 8> s2({ 1, 2 });
    auto empty_tail = s2.split_off_mut(99);
    require(empty_tail.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_first() pops first element by value");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4 });
    int v = s.split_off_first();
    require(v, 1);
    require(s.size(), size_t(3));
    require(s[0], 2);
    require(s[2], 4);

    // pop on empty returns default-constructed
    micron::span<int, 8> empty;
    require(empty.split_off_first(), 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_first_mut() pops first as raw_slice view");
  {
    micron::span<int, 8> s({ 10, 20, 30 });
    auto first_view = s.split_off_first_mut();
    require(first_view.size(), size_t(1));
    require(first_view[0], 10);
    require(s.size(), size_t(2));
    require(s[0], 20);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_last() pops last element by value");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4 });
    int v = s.split_off_last();
    require(v, 4);
    require(s.size(), size_t(3));
    require(s[2], 3);

    micron::span<int, 8> empty;
    require(empty.split_off_last(), 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("split_off_last_mut() pops last as raw_slice view");
  {
    micron::span<int, 8> s({ 10, 20, 30 });
    auto last_view = s.split_off_last_mut();
    require(last_view.size(), size_t(1));
    require(last_view[0], 30);
    require(s.size(), size_t(2));
    require(s[1], 20);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("contains() by value");
  {
    micron::span<int, 8> s({ 0, 2, 4, 6, 8 });
    require_true(s.contains(4));
    require_false(s.contains(3));
    require_false(s.contains(99));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("contains() by predicate");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
    require_true(s.contains([](const int &v) { return v > 3; }));
    require_false(s.contains([](const int &v) { return v > 10; }));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("starts_with() prefix matching");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4, 5 });

    int arr[] = { 1, 2, 3 };
    micron::raw_slice<int> prefix(arr, 3);
    require_true(s.starts_with(prefix));

    int bad[] = { 2, 3 };
    micron::raw_slice<int> not_prefix(bad, 2);
    require_false(s.starts_with(not_prefix));

    int long_arr[] = { 1, 2, 3, 4, 5, 6 };
    micron::raw_slice<int> too_long(long_arr, 6);
    require_false(s.starts_with(too_long));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ends_with() suffix matching");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4, 5 });

    int suf_arr[] = { 4, 5 };
    micron::raw_slice<int> suffix(suf_arr, 2);
    require_true(s.ends_with(suffix));

    int bad[] = { 3, 5 };
    micron::raw_slice<int> bad_suffix(bad, 2);
    require_false(s.ends_with(bad_suffix));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("strip_prefix() returns remainder after matching prefix");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });

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
  test_case("strip_suffix() returns everything before matching suffix");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });

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
    micron::span<int, 8> s({ 0, 1, 2, 3, 4, 5 });

    int pre[] = { 0, 1 };
    int suf[] = { 4, 5 };
    micron::raw_slice<int> prefix(pre, 2);
    micron::raw_slice<int> suffix(suf, 2);
    auto mid = s.strip_circumfix(prefix, suffix);
    require_true(mid.valid());
    require(mid.size(), size_t(2));
    require(mid[0], 2);
    require(mid[1], 3);

    // suffix mismatch → invalid
    int bad_suf[] = { 99 };
    micron::raw_slice<int> bad_s(bad_suf, 1);
    auto bad_mid = s.strip_circumfix(prefix, bad_s);
    require_false(bad_mid.valid());

    // prefix + suffix longer than span → invalid
    int long_pre[] = { 0, 1, 2, 3, 4 };
    micron::raw_slice<int> lp(long_pre, 5);
    auto too_big = s.strip_circumfix(lp, suffix);
    require_false(too_big.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("trim_prefix() and trim_suffix() alias strip variants");
  {
    micron::span<int, 8> s({ 1, 2, 3, 4 });

    int pre[] = { 1 };
    micron::raw_slice<int> prefix(pre, 1);
    auto r1 = s.trim_prefix(prefix);
    require_true(r1.valid());
    require(r1.size(), size_t(3));
    require(r1[0], 2);

    int suf[] = { 4 };
    micron::raw_slice<int> suffix(suf, 1);
    auto r2 = s.trim_suffix(suffix);
    require_true(r2.valid());
    require(r2.size(), size_t(3));
    require(r2[2], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("swap() exchanges elements, out-of-range is a no-op");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3 });
    s.swap(0, 3);
    require(s[0], 3);
    require(s[3], 0);
    // out-of-range must not crash or corrupt
    s.swap(0, 99);
    require(s[0], 3);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("swap_unchecked() exchanges elements");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3 });
    s.swap_unchecked(1, 2);
    require(s[1], 2);
    require(s[2], 1);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("swap_with_span() pairwise element exchange");
  {
    micron::span<int, 8> a({ 0, 1, 2 });
    micron::span<int, 8> b({ 10, 11, 12 });
    a.swap_with_span(b);
    for ( int i = 0; i < 3; ++i ) {
      require(a[i], i + 10);
      require(b[i], i);
    }
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("reverse() in-place");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
    s.reverse();
    for ( int i = 0; i < 5; ++i )
      require(s[i], 4 - i);

    // idempotent on single element
    micron::span<int, 8> one(1, 7);
    one.reverse();
    require(one[0], 7);

    // empty span is safe
    micron::span<int, 8> empty;
    empty.reverse();
    require(empty.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rotate_left() wraps correctly");
  {
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
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
    micron::span<int, 8> s({ 0, 1, 2, 3, 4 });
    s.rotate_right(2);     // [3,4,0,1,2]
    require(s[0], 3);
    require(s[1], 4);
    require(s[2], 0);
    require(s[4], 2);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("rotate_left and rotate_right are mutual inverses");
  {
    micron::span<int, 16> s;
    for ( int i = 0; i < 7; ++i )
      s.push_back(i);
    s.rotate_left(3);
    s.rotate_right(3);
    for ( int i = 0; i < 7; ++i )
      require(s[i], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_bytes() returns correct byte count and non-null ptr");
  {
    micron::span<int, 8> s({ 0, 1 });
    auto bytes = s.as_bytes();
    require(bytes.size(), sizeof(int) * 2);
    require_true(bytes.ptr != nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_bytes_mut() allows byte-level writes");
  {
    micron::span<int, 8> s(1, 0xFFFFFFFF);
    auto bytes = s.as_bytes_mut();
    require(bytes.size(), sizeof(int));
    for ( size_t i = 0; i < bytes.size(); ++i )
      bytes.ptr[i] = 0;
    require(s[0], 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_flattened<U>() reinterprets memory as a wider element type");
  {
    micron::span<uint8_t, 8> s({ 0x01, 0x02, 0x03, 0x04 });
    auto flat = s.as_flattened<uint16_t>();
    require(flat.size(), size_t(2));
    require_true(flat.ptr != nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("as_flattened_mut<U>() returns mutable wider view");
  {
    micron::span<uint8_t, 8> s({ 0x00, 0x00, 0x00, 0x00 });
    auto flat = s.as_flattened_mut<uint16_t>();
    require(flat.size(), size_t(2));
    flat.ptr[0] = 0xABCD;
    // the underlying bytes should now reflect the written value
    require_true(s[0] != 0 || s[1] != 0);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("align_to<U>() prefix + middle + suffix covers all bytes");
  {
    micron::span<uint8_t, 64> s(32, 1);
    auto [pre, mid, suf] = s.align_to<uint64_t>();
    size_t covered = pre.size() * sizeof(uint8_t) + mid.size() * sizeof(uint64_t) + suf.size() * sizeof(uint8_t);
    require(covered, s.size() * sizeof(uint8_t));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: is_ascii() returns true for pure ASCII content");
  {
    micron::span<char, 64> s;
    for ( char c : { 'H', 'e', 'l', 'l', 'o' } )
      s.push_back(c);
    require_true(s.is_ascii());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: is_ascii() returns false for high bytes");
  {
    micron::span<char, 8> s;
    s.push_back('A');
    s.push_back(static_cast<char>(200));     // non-ASCII byte
    require_false(s.is_ascii());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: to_ascii_uppercase() converts lowercase letters");
  {
    micron::span<char, 16> s({ 'h', 'e', 'l', 'l', 'o' });
    s.to_ascii_uppercase();
    require(s[0], 'H');
    require(s[1], 'E');
    require(s[4], 'O');
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: to_ascii_lowercase() converts uppercase letters");
  {
    micron::span<char, 16> s({ 'W', 'O', 'R', 'L', 'D' });
    s.to_ascii_lowercase();
    require(s[0], 'w');
    require(s[4], 'd');
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: to_ascii_uppercase and lowercase are inverses");
  {
    micron::span<char, 16> s({ 'a', 'b', 'c' });
    s.to_ascii_uppercase();
    require(s[0], 'A');
    s.to_ascii_lowercase();
    require(s[0], 'a');
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: trim_ascii_start() strips leading whitespace");
  {
    micron::span<char, 16> s({ ' ', '\t', 'x', 'y' });
    auto r = s.trim_ascii_start();
    require(r.size(), size_t(2));
    require(r.ptr[0], 'x');
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: trim_ascii_end() strips trailing whitespace");
  {
    micron::span<char, 16> s({ 'a', 'b', ' ', '\n' });
    auto r = s.trim_ascii_end();
    require(r.size(), size_t(2));
    require(r.ptr[1], 'b');
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: trim_ascii() strips both leading and trailing whitespace");
  {
    micron::span<char, 16> s({ ' ', 'h', 'i', '\t' });
    auto r = s.trim_ascii();
    require(r.size(), size_t(2));
    require(r.ptr[0], 'h');
    require(r.ptr[1], 'i');
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: as_ascii() returns valid view for pure ASCII");
  {
    micron::span<char, 16> s({ 'O', 'K' });
    auto r = s.as_ascii();
    require(r.size(), size_t(2));
    require_true(r.ptr != nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: as_ascii() returns empty view when non-ASCII present");
  {
    micron::span<char, 8> s;
    s.push_back('A');
    s.push_back(static_cast<char>(200));
    auto r = s.as_ascii();
    require_false(r.valid());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("ASCII: as_ascii_unchecked() always returns view regardless of content");
  {
    micron::span<char, 8> s;
    s.push_back('Z');
    s.push_back(static_cast<char>(200));
    auto r = s.as_ascii_unchecked();
    require(r.size(), size_t(2));
    require_true(r.ptr != nullptr);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("static type queries: is_pod, is_class_type, is_trivial");
  {
    // trivial / pod type
    require_true(micron::span<int>::is_pod());
    require_false(micron::span<int>::is_class_type());
    require_true(micron::span<int>::is_trivial());

    // class type
    require_false(micron::span<Tracked>::is_pod());
    require_true(micron::span<Tracked>::is_class_type());
    require_false(micron::span<Tracked>::is_trivial());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("tracked object lifetime — push then destroy");
  {
    reset_tracked();
    {
      micron::span<Tracked, 16> s;
      for ( int i = 0; i < 8; ++i )
        s.emplace_back(i);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("tracked object lifetime — clear() destroys elements");
  {
    reset_tracked();
    micron::span<Tracked, 16> s;
    for ( int i = 0; i < 6; ++i )
      s.emplace_back(i);
    size_t before_clear = Tracked::dtor;
    s.clear();
    require(Tracked::dtor - before_clear, size_t(6));
    require(s.size(), size_t(0));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("tracked object lifetime — pop_back() destructs element");
  {
    reset_tracked();
    micron::span<Tracked, 8> s;
    s.emplace_back(1);
    s.emplace_back(2);
    size_t before = Tracked::dtor;
    s.pop_back();
    require(Tracked::dtor - before, size_t(1));
    require(s.size(), size_t(1));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress push_back / pop_back round-trip");
  {
    constexpr int N = 60;
    micron::span<int, 64> s;
    for ( int i = 0; i < N; ++i )
      s.push_back(i);
    require(s.size(), size_t(N));
    for ( int i = N - 1; i >= 0; --i ) {
      require(s[static_cast<size_t>(i)], i);
      s.pop_back();
    }
    require_true(s.is_empty());
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress insert / erase at middle");
  {
    micron::span<int, 64> s;
    for ( int i = 0; i < 10; ++i )
      s.push_back(i);
    // insert 99 at position 5
    s.insert(size_t(5), 99);
    require(s[5], 99);
    require(s.size(), size_t(11));
    // erase it back
    s.erase(size_t(5));
    require(s.size(), size_t(10));
    for ( int i = 0; i < 10; ++i )
      require(s[static_cast<size_t>(i)], i);
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress rotate_left round-trip");
  {
    constexpr size_t N = 60;
    micron::span<int, 64> s;
    for ( size_t i = 0; i < N; ++i )
      s.push_back(static_cast<int>(i));

    for ( size_t r = 1; r < N; ++r ) {
      s.rotate_left(1);
      s.rotate_right(1);
    }
    for ( size_t i = 0; i < N; ++i )
      require(s[i], static_cast<int>(i));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress double reverse is identity");
  {
    constexpr size_t N = 50;
    micron::span<int, 64> s;
    for ( size_t i = 0; i < N; ++i )
      s.push_back(static_cast<int>(i));
    s.reverse();
    s.reverse();
    for ( size_t i = 0; i < N; ++i )
      require(s[i], static_cast<int>(i));
  }
  end_test_case();

  // ──────────────────────────────────────────────────────────────────────────
  test_case("stress split / contains consistency");
  {
    micron::span<int, 64> s;
    for ( int i = 0; i < 32; ++i )
      s.push_back(i % 2 == 0 ? 0 : 1);

    // every odd element is a 1 — contains must agree
    require_true(s.contains(1));
    require_false(s.contains(42));

    size_t total = 0;
    s.split([](const int &v) { return v == 1; }, [&](micron::raw_slice<int> sub) { total += sub.size(); });
    // 16 separators (value 1) → 17 sub-slices of zeros; total zeros == 16
    require(total, size_t(16));
  }
  end_test_case();

  sb::print("=== ALL SPAN TESTS PASSED ===");
  return 1;
}
