//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// the tests a secretly-EAGER micron::lz would fail.
//
// diffing lz:: against fp:: proves the two agree; it says nothing about whether lz:: is lazy, since
// an eager implementation would agree just as well. this file carries the actual claim:
//   - exact functor invocation counts
//   - endless sources, which an eager implementation cannot terminate on
//   - a tripwire source that aborts if read past its budget
//   - allocation counts against the eager chain
//   - the documented no-cache behaviour of filter's begin()

#include "../../src/lz.hpp"

#include "../../src/algorithm/fp.hpp"
#include "../../src/list.hpp"
#include "../../src/vector.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mock_allocators.hpp"

namespace lz = micron::lz;
using ::sb::print;
using ::sb::require;
using ::sb::require_false;
using ::sb::require_true;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the tripwire source
//
// counting the calls a functor received proves the functor was not over-applied. it does not prove
// the SOURCE was not over-pulled -- a terminal could read one element too many and simply not hand
// it downstream, which is exactly the bug take(n) had (it pulled n + 1, and through a filter that
// means scanning on to the next match nobody asked for). so: an endless source that aborts the
// moment it is dereferenced past its budget. every short-circuiting terminal is run against it.
template<int Tag> struct tripwire_view: public micron::view_interface<tripwire_view<Tag>> {
  static inline usize budget = 0;
  static inline usize pulls = 0;      // dereferences -- the work the source actually did
  static inline usize steps = 0;      // ++ on the source, which for a pure source is free

  static void
  arm(usize __n) noexcept
  {
    budget = __n;
    pulls = 0;
    steps = 0;
  }

  using __lazy_view_tag = void;
  using value_type = int;

  static constexpr micron::lz::size_kind __kind = micron::lz::size_kind::endless;
  static constexpr usize __static_size = micron::lz::no_static_size;
  static constexpr bool __is_materializing = false;

  struct sentinel {
  };

  class iterator
  {
    int __v = 0;

  public:
    using value_type = int;
    using reference = int;
    using difference_type = ssize_t;

    constexpr iterator() = default;

    int
    operator*() const
    {
      // the assertion IS the tripwire: one pull past the budget and the suite stops here
      require_true(pulls < budget);
      ++pulls;
      return __v;
    }

    iterator &
    operator++()
    {
      ++steps;
      ++__v;
      return *this;
    }

    iterator
    operator++(int)
    {
      iterator __t = *this;
      ++*this;
      return __t;
    }

    constexpr bool
    operator==(const sentinel &) const noexcept
    {
      return false;
    }

    constexpr bool
    operator!=(const sentinel &) const noexcept
    {
      return true;
    }
  };

  constexpr iterator
  begin() const
  {
    return iterator{};
  }

  constexpr sentinel
  end() const noexcept
  {
    return {};
  }

  constexpr usize
  reserve_hint() const noexcept
  {
    return 0u;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a functor that counts every call
template<int Tag> struct counted {
  static inline usize calls = 0;

  static void
  reset() noexcept
  {
    calls = 0;
  }

  int
  operator()(int __x) const
  {
    ++calls;
    return __x * 2;
  }
};

template<int Tag> struct counted_pred {
  static inline usize calls = 0;

  static void
  reset() noexcept
  {
    calls = 0;
  }

  bool
  operator()(int __x) const
  {
    ++calls;
    return (__x & 1) == 0;
  }
};

int
main()
{
  print("=== LZ LAZINESS SUITE ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("fmap over a bounded source runs exactly n times");
  {
    micron::vector<int> v(64, 1);
    counted<0>::reset();
    auto out = v | lz::fmap(counted<0>{}) | lz::collect<micron::vector<int>>();
    require(out.size(), usize{ 64 });
    require(counted<0>::calls, usize{ 64 });
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // THE test. eager would call f 1024 times; lazy calls it 3.
  sb::test_case("take(3) after fmap calls the function exactly 3 times, not n");
  {
    micron::vector<int> v(1024, 7);
    counted<1>::reset();
    auto out = v | lz::fmap(counted<1>{}) | lz::take(3) | lz::collect<micron::vector<int>>();
    require(out.size(), usize{ 3 });
    require(counted<1>::calls, usize{ 3 });

    // and the eager layer, for contrast: it maps all 1024 before taking 3
    counted<2>::reset();
    auto eager = micron::fp::take(micron::fp::fmap(counted<2>{}, v), 3);
    require(eager.size(), usize{ 3 });
    require(counted<2>::calls, usize{ 1024 });
    print("  fmap|take(3): lazy called f ", counted<1>::calls, "x, eager called it ", counted<2>::calls, "x");
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("filter stops at the take budget");
  {
    // 0,1,2,... filtered to evens, take 1 -> the predicate sees only element 0
    counted_pred<0>::reset();
    auto out = lz::counting(0) | lz::filter(counted_pred<0>{}) | lz::take(1) | lz::collect<micron::vector<int>>();
    require(out.size(), usize{ 1 });
    require(out[0], 0);
    require(counted_pred<0>::calls, usize{ 1 });
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // an eager implementation cannot terminate on any of these
  sb::test_case("endless sources terminate under take");
  {
    auto a = lz::counting(0) | lz::take(5) | lz::collect<micron::vector<int>>();
    require(a.size(), usize{ 5 });
    require(a[0], 0);
    require(a[4], 4);

    auto b = lz::counting(0) | lz::filter([](int x) { return (x % 7) == 0; }) | lz::take(4) | lz::collect<micron::vector<int>>();
    require(b.size(), usize{ 4 });
    require(b[3], 21);

    auto c = lz::iterate([](int x) { return x * 3; }, 1) | lz::take(5) | lz::collect<micron::vector<int>>();
    require(c.size(), usize{ 5 });
    require(c[4], 81);

    auto d = lz::counting(0) | lz::take_while([](int x) { return x < 6; }) | lz::collect<micron::vector<int>>();
    require(d.size(), usize{ 6 });

    auto e = lz::replicate(3, 9) | lz::collect<micron::vector<int>>();
    require(e.size(), usize{ 3 });
    require(e[2], 9);

    // drop over an endless source is still endless until something bounds it
    auto g = lz::counting(0) | lz::drop(100) | lz::take(2) | lz::collect<micron::vector<int>>();
    require(g[0], 100);
    require(g[1], 101);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("a lazy chain over a scalar terminal allocates nothing");
  {
    using A = mtest::tracking_allocator<77>;
    micron::vector<int, A> src(256, 3);

    A::reset();
    usize n = 0;
    for ( auto x : src | lz::fmap([](int x) { return x + 1; }) | lz::filter([](int x) { return x > 2; }) ) {
      (void)x;
      ++n;
    }
    require(n, usize{ 256 });
    require(A::allocations, usize{ 0 });
    print("  fmap|filter, scalar terminal: ", A::allocations, " allocations");
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("filter's begin() re-scans, by design");
  {
    // the leading rejects are scanned once per begin(); this pins the documented no-cache decision
    // so a later refactor that adds a cache is a deliberate change, not an accident
    micron::vector<int> v(64, 0);
    for ( usize i = 0; i < 64; ++i ) v[i] = static_cast<int>(i);
    counted_pred<1>::reset();
    auto fv = v | lz::filter(counted_pred<1>{});
    (void)fv.begin();
    const usize after_first = counted_pred<1>::calls;
    (void)fv.begin();
    const usize after_second = counted_pred<1>::calls;
    require_true(after_second > after_first);
    print("  begin() twice: ", after_first, " then ", after_second, " predicate calls (no cache)");
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("lz agrees with fp on the same inputs");
  {
    micron::vector<int> v(32, 0);
    for ( usize i = 0; i < 32; ++i ) v[i] = static_cast<int>(i) - 16;

    auto lazy_r = v | lz::fmap([](int x) { return x * 3; }) | lz::collect<micron::vector<int>>();
    auto eager_r = micron::fp::fmap([](int x) { return x * 3; }, v);
    require(lazy_r.size(), eager_r.size());
    for ( usize i = 0; i < lazy_r.size(); ++i ) require(lazy_r[i], eager_r[i]);

    // NOTE the eager filter takes a POINTER predicate only (algorithm/filter.hpp), so the two
    // spellings of the same predicate have to be written out separately
    auto lazy_f = v | lz::filter([](int x) { return x > 0; }) | lz::collect<micron::vector<int>>();
    auto eager_f = micron::filter(v, [](const int *p) { return *p > 0; });
    require(lazy_f.size(), eager_f.size());
    for ( usize i = 0; i < lazy_f.size(); ++i ) require(lazy_f[i], eager_f[i]);

    auto lazy_t = v | lz::take(7) | lz::collect<micron::vector<int>>();
    auto eager_t = micron::fp::take(v, 7);
    require(lazy_t.size(), eager_t.size());
    for ( usize i = 0; i < lazy_t.size(); ++i ) require(lazy_t[i], eager_t[i]);
  }
  sb::end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // exact pull counts for every short-circuiting terminal
  //
  // the answer being right is not the claim here. the claim is that the source was touched exactly
  // as many times as the answer required and not once more.
  // ════════════════════════════════════════════════════════════════════

  sb::test_case("any_of pulls exactly up to and including the first match");
  {
    // counting(0) | any_of(x > 10): 0..11 is twelve elements, the twelfth being the first > 10
    using T = tripwire_view<1>;
    T::arm(12);
    require_true(T{} | lz::any_of([](int x) { return x > 10; }));
    require(T::pulls, usize(12));
    require(T::steps, usize(11));
  }
  sb::end_test_case();

  sb::test_case("all_of and none_of stop on the element that decides them");
  {
    using A = tripwire_view<2>;
    A::arm(1);
    require_false(A{} | lz::all_of([](int x) { return x > 0; }));      // 0 fails immediately
    require(A::pulls, usize(1));

    using B = tripwire_view<3>;
    B::arm(4);
    require_false(B{} | lz::none_of([](int x) { return x == 3; }));
    require(B::pulls, usize(4));

    using C = tripwire_view<4>;
    C::arm(6);
    require_true(C{} | lz::take(6) | lz::all_of([](int x) { return x >= 0; }));
    require(C::pulls, usize(6));      // six, not seven: take decrements its budget BEFORE advancing
    require(C::steps, usize(5));
  }
  sb::end_test_case();

  sb::test_case("find_first / find_index / find_of / elem stop on the hit, reading it ONCE");
  {
    // `if (p(*i)) return *i;` would dereference the hit twice -- free over a vector, a second call
    // to the mapped function over an fmap. these bind it once.
    using A = tripwire_view<5>;
    A::arm(8);
    require((A{} | lz::find_first([](int x) { return x == 7; })).cast<int>(), 7);
    require(A::pulls, usize(8));

    using B = tripwire_view<6>;
    B::arm(5);
    require((B{} | lz::find_index([](int x) { return x == 4; })).cast<usize>(), usize(4));
    require(B::pulls, usize(5));

    using C = tripwire_view<7>;
    C::arm(4);
    require_true(C{} | lz::elem(3));
    require(C::pulls, usize(4));

    using D = tripwire_view<8>;
    D::arm(3);
    require((D{} | lz::find_of(2)).cast<usize>(), usize(2));
    require(D::pulls, usize(3));
  }
  sb::end_test_case();

  sb::test_case("head reads one element; at(n) SKIPS without reading");
  {
    using A = tripwire_view<9>;
    A::arm(1);
    require((A{} | lz::head()).cast<int>(), 0);
    require(A::pulls, usize(1));
    require(A::steps, usize(0));

    // at(n) advances n times but dereferences exactly ONCE -- so over an fmap it calls the mapped
    // function once, not n + 1 times. that is the whole point of skipping lazily.
    using B = tripwire_view<10>;
    B::arm(1);
    require((B{} | lz::at(5)).cast<int>(), 5);
    require(B::pulls, usize(1));
    require(B::steps, usize(5));

    using C = tripwire_view<11>;
    C::arm(1);
    require((C{} | lz::nth(2)).cast<int>(), 2);
    require(C::pulls, usize(1));
    require(C::steps, usize(2));
  }
  sb::end_test_case();

  sb::test_case("traverse stops ON the failing element, not after it");
  {
    using O = micron::option<int, micron::fp::empty_container_error>;
    // traverse ACCUMULATES, so unlike find_first it refuses an endless source outright -- an arrow
    // that never fails would grow the output without bound. bound it with a take and the
    // short-circuit is still the thing under test.
    using A = tripwire_view<12>;
    A::arm(4);
    auto r = A{} | lz::take(50)
             | lz::traverse<micron::vector<int>>([](int x) { return x >= 3 ? O{ micron::fp::empty_container_error{} } : O{ x }; });
    require_true(r.is_second());
    require(A::pulls, usize(4));      // stopped ON the failure, 46 elements of budget untouched
  }
  sb::end_test_case();

  sb::test_case("adaptors do not over-pull on the way to a short-circuiting terminal");
  {
    // filter re-reads the element it accepted: once to test it in __seek, once to hand it on. that
    // is inherent to a pull filter (std::ranges::filter_view does the same) and it is why the header
    // says to put filter BEFORE fmap, not after. six here, not five, and the number is pinned so it
    // cannot quietly become seven.
    using A = tripwire_view<13>;
    A::arm(6);
    require_true(A{} | lz::filter([](int x) { return (x % 5) == 4; }) | lz::any_of([](int) { return true; }));
    require(A::pulls, usize(6));
    require(A::steps, usize(4));

    using B = tripwire_view<14>;
    B::arm(3);
    require((B{} | lz::fmap([](int x) { return x * 2; }) | lz::take(3) | lz::collect<micron::vector<int>>()).size(), usize(3));
    require(B::pulls, usize(3));

    // count does not dereference at all -- there is nothing to compute, only elements to walk past
    using C = tripwire_view<15>;
    C::arm(1);
    require((C{} | lz::step_by(1) | lz::take(4) | lz::count()), usize(4));
    require(C::pulls, usize(0));
    require(C::steps, usize(3));

    using D = tripwire_view<16>;
    D::arm(3);
    require((D{} | lz::scanl(0, [](int a, int x) { return a + x; }) | lz::take(4) | lz::count()), usize(4));
    require(D::pulls, usize(3));      // n + 1 outputs from n pulls -- the seed is free
  }
  sb::end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // allocation counts -- gate 4: a scalar terminal allocates NOTHING
  // ════════════════════════════════════════════════════════════════════

  sb::test_case("scalar terminals over any chain length allocate zero times");
  {
    using A = mtest::tracking_allocator<78>;
    micron::vector<int, A> src(512, 3);
    for ( usize i = 0; i < 512; ++i ) src[i] = static_cast<int>(i) - 200;

    A::reset();
    volatile umax_t sink = 0;
    sink += static_cast<umax_t>(src | lz::count());
    sink += static_cast<umax_t>(src | lz::count_if([](int x) { return x > 0; }));
    sink += static_cast<umax_t>(src | lz::count_of(7));
    sink += static_cast<umax_t>(src | lz::fold(0, [](int a, int x) { return a + x; }));
    sink += (src | lz::sum());
    sink += static_cast<umax_t>(src | lz::min()) + static_cast<umax_t>(src | lz::max());
    sink += static_cast<umax_t>(static_cast<i64>(src | lz::mean<f64>()));
    sink += static_cast<umax_t>(src | lz::any_of([](int x) { return x == 5; }));
    sink += static_cast<umax_t>(src | lz::all_of([](int x) { return x > -1000; }));
    sink += static_cast<umax_t>(src | lz::elem(11));
    sink += static_cast<umax_t>((src | lz::head()).cast<int>());
    sink += static_cast<umax_t>((src | lz::last()).cast<int>());
    sink += static_cast<umax_t>((src | lz::at(300)).cast<int>());
    // and the same through a stack of adaptors, which is where a per-stage buffer would show up
    sink += static_cast<umax_t>(src | lz::fmap([](int x) { return x * 2; }) | lz::filter([](int x) { return x > 0; })
                                | lz::take(100) | lz::count());
    sink += static_cast<umax_t>(src | lz::reverse() | lz::step_by(3) | lz::count());
    sink += static_cast<umax_t>(src | lz::enumerate() | lz::count());
    sink += static_cast<umax_t>(src | lz::unique() | lz::count());
    sink += static_cast<umax_t>(src | lz::intersperse(0) | lz::count());
    sink += static_cast<umax_t>(src | lz::zip_with(src, [](int a, int b) { return a + b; }) | lz::count());
    sink += static_cast<umax_t>(src | lz::chunk(16) | lz::count());
    sink += static_cast<umax_t>(src | lz::sliding(4) | lz::count());
    sink += static_cast<umax_t>(src | lz::group() | lz::count());
    (void)sink;
    require(A::allocations, usize(0));

    // the eager equivalent of just ONE of those, for contrast
    A::reset();
    auto eager = micron::fp::take(micron::fp::fmap([](int x) { return x * 2; }, src), 100);
    require_true(A::allocations > usize(0));
    print("  eager fmap|take allocations: ", A::allocations, " vs lazy: 0");
    (void)eager.size();
  }
  sb::end_test_case();

  sb::test_case("reverse over a contiguous source is the zero-allocation arm; sort is not");
  {
    // this is a type-level property, not a measurement -- which is the point: it cannot regress
    // without someone deleting the assertion
    using V = micron::vector<int>;
    static_assert(!decltype(micron::declval<V &>() | lz::reverse())::__is_materializing);
    static_assert(decltype(micron::declval<micron::list<int> &>() | lz::reverse())::__is_materializing);
    static_assert(decltype(micron::declval<V &>() | lz::sort())::__is_materializing);

    using A = mtest::tracking_allocator<79>;
    micron::vector<int, A> src(64, 1);
    A::reset();
    volatile usize sink = src | lz::reverse() | lz::count();
    (void)sink;
    require(A::allocations, usize(0));
  }
  sb::end_test_case();

  sb::test_case("nub is the one streaming adaptor that allocates -- bounded, and it still terminates");
  {
    // its seen-set goes through the DEFAULT allocator (fp::__impl::seen_set wraps heap_swiss_set or
    // fvector), not the source's, so tracking_allocator cannot see it and there is no honest count
    // to assert. what can be asserted is the shape: nub does not materialize, and it terminates over
    // an endless source -- which it could not do if it buffered the input.
    static_assert(!decltype(micron::declval<micron::vector<int> &>() | lz::nub())::__is_materializing);
    auto ne = lz::counting(0) | lz::fmap([](int x) { return x % 6; }) | lz::nub() | lz::take(6)
              | lz::collect<micron::vector<int>>();
    require(ne.size(), usize(6));
    require(ne[0], 0);
    require(ne[5], 5);
  }
  sb::end_test_case();

  sb::test_case("collect allocates exactly once where the length is known");
  {
    using A = mtest::tracking_allocator<80>;
    micron::vector<int, A> src(256, 2);

    A::reset();
    auto exact = src | lz::fmap([](int x) { return x + 1; }) | lz::collect<micron::vector<int, A>>();
    require(exact.size(), usize(256));
    require(A::allocations, usize(1));      // sized at CONSTRUCTION; a default ctor + resize costs two

    A::reset();
    auto bounded = src | lz::filter([](int x) { return x > 0; }) | lz::collect<micron::vector<int, A>>();
    require(bounded.size(), usize(256));
    require(A::allocations, usize(1));      // bounded: allocate the worst case, fill, shrink in place
  }
  sb::end_test_case();

  print("=== LZ LAZINESS SUITE PASSED ===");
  return 1;
}
