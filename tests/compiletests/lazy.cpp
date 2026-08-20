//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate for micron::lz on every arch/opt/freestanding cell (verify_compile.duck).
// Not run. The static_asserts below are the load-bearing part: each one pins a property that, if it
// silently flipped, would either break the pipe or quietly hand a pipeline to the EAGER fp:: layer.

#include "../../src/array.hpp"
#include "../../src/doublelist.hpp"
#include "../../src/list.hpp"
#include "../../src/lz.hpp"
#include "../../src/maps/robin.hpp"
#include "../../src/sets/sets.hpp"
#include "../../src/trees/art.hpp"
#include "../../src/trees/rb.hpp"
#include "../../src/vector.hpp"

namespace r = micron::ranges;
namespace lz = micron::lz;
using micron::vector;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// iterator tiers -- every Phase-0 cursor lands where it should
static_assert(micron::contiguous_iterator<vector<int>::iterator>);
static_assert(micron::random_access_iterator<micron::counting_iter<int>>);
static_assert(micron::forward_iterator<micron::list<int>::const_iterator>);
static_assert(!micron::bidirectional_iterator<micron::list<int>::const_iterator>);
static_assert(micron::bidirectional_iterator<micron::double_list<int>::const_iterator>);
static_assert(micron::forward_iterator<micron::rb_tree<int>::const_iterator>);
static_assert(micron::forward_iterator<micron::art<int, int>::const_iterator>);
static_assert(micron::forward_iterator<micron::robin_set<int>::const_iterator>);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// range tiers
static_assert(r::contiguous_range<vector<int>>);
static_assert(r::contiguous_range<int[8]>);
static_assert(r::forward_range<micron::list<int>> && !r::bidirectional_range<micron::list<int>>);
static_assert(r::bidirectional_range<micron::double_list<int>>);
static_assert(r::forward_range<micron::robin_map<int, int>>);
static_assert(r::random_access_range<micron::range<0, 10>>);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the eager/lazy firewall
//
// a view must NEVER satisfy is_iterable_container -- that concept wants data() + operator[] +
// size(), and if a view ever grew all three the eager fp::fmap / micron::filter overloads would
// start accepting pipelines silently. views carry no data() for exactly this reason.
static_assert(!micron::is_iterable_container<lz::ptr_view<int>>);
static_assert(!micron::is_iterable_container<lz::ref_view<vector<int>>>);
static_assert(!micron::is_iterable_container<lz::owning_view<vector<int>>>);

// and a container is a range but not a view; view-ness is opt-in via the tag
static_assert(!r::view<vector<int>>);
static_assert(r::view<lz::ptr_view<int>>);
static_assert(r::viewable_range<vector<int> &>);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pipe hygiene: a view must not be invocable and a closure must not be a range, or micron's very
// greedy operator|(A&&, F&&) requires invocable<F,A> would start matching the wrong pairs
static_assert(!micron::is_invocable_v<lz::ptr_view<int>, int>);
static_assert(!r::range<decltype(lz::collect<vector<int>>())>);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the size-kind axis
static_assert(lz::kind_of<lz::ptr_view<int>> == lz::size_kind::exact);
static_assert(lz::kind_of<lz::ref_view<micron::list<int>>> == lz::size_kind::unknown);      // O(n) size()
static_assert(lz::degrade(lz::size_kind::exact) == lz::size_kind::bounded);
static_assert(lz::degrade(lz::size_kind::unknown) == lz::size_kind::unknown);

// static extent survives the contiguous source only via ref_view
static_assert(lz::static_size_of<lz::as_view_t<micron::array<int, 8> &>> == 8);
static_assert(lz::static_size_of<lz::ptr_view<int>> == lz::no_static_size);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// how each adaptor moves the size kind. these are the axis collect<C>() dispatches on, so a silent
// change here changes which materialisation strategy every pipeline gets.
using __src = lz::as_view_t<vector<int> &>;
static_assert(lz::kind_of<__src> == lz::size_kind::exact);

template<typename V> using __fm = lz::transform_view<V, int (*)(int)>;
template<typename V> using __fl = lz::filter_view<V, bool (*)(int)>;
template<typename V> using __tw = lz::take_while_view<V, bool (*)(int)>;

static_assert(lz::kind_of<__fm<__src>> == lz::size_kind::exact);            // fmap preserves
static_assert(lz::kind_of<__fl<__src>> == lz::size_kind::bounded);          // filter degrades
static_assert(lz::kind_of<lz::take_view<__src>> == lz::size_kind::exact);
static_assert(lz::kind_of<lz::take_view<__fl<__src>>> == lz::size_kind::bounded);
static_assert(lz::kind_of<lz::drop_view<__src>> == lz::size_kind::exact);   // drop preserves

// a generator is endless, and only take / take_while make it collectable
using __gen = lz::iota_view<int>;
static_assert(lz::kind_of<__gen> == lz::size_kind::endless);
static_assert(lz::kind_of<__fm<__gen>> == lz::size_kind::endless);          // fmap keeps it endless
static_assert(lz::kind_of<__fl<__gen>> == lz::size_kind::endless);          // so does filter
static_assert(lz::kind_of<lz::take_view<__gen>> == lz::size_kind::bounded); // take bounds it
static_assert(lz::kind_of<__tw<__gen>> == lz::size_kind::unknown);          // take_while bounds it

// plain function pointers, so the static_asserts above name a type rather than a closure
inline bool (*const __gt0)(int) = [](int x) { return x > 0; };
inline bool (*const __cmp_lt)(const int &, const int &) = [](const int &a, const int &b) { return a < b; };
inline int (*const __add2)(int, int) = [](int a, int b) { return a + b; };
inline micron::vector<int> (*const __wrap)(int) = [](int x) { return micron::vector<int>{ x }; };
inline micron::option<micron::pair<int, int>, micron::fp::empty_container_error> (*const __step)(const int &)
    = [](const int &s) {
        using O = micron::option<micron::pair<int, int>, micron::fp::empty_container_error>;
        return s < 3 ? O{ micron::pair<int, int>{ s, s + 1 } } : O{ micron::fp::empty_container_error{} };
      };

// flatten needs a range OF ranges
using __nested = decltype(micron::declval<vector<int> &>() | lz::fmap(__wrap) | lz::flatten());

// fmap is the only adaptor that keeps the compile-time extent
static_assert(lz::static_size_of<__fm<lz::as_view_t<micron::array<int, 8> &>>> == 8);
static_assert(lz::static_size_of<__fl<lz::as_view_t<micron::array<int, 8> &>>> == lz::no_static_size);
static_assert(lz::static_size_of<lz::take_view<lz::as_view_t<micron::array<int, 8> &>>> == lz::no_static_size);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// materialization
//
// this is the reverse feature, stated as a type property: over a contiguous source reverse streams
// and allocates nothing; over a forward-only one it has to buffer. sort always buffers.
using __ctg = vector<int> &;
using __fwd = micron::list<int> &;

static_assert(!decltype(micron::declval<__ctg>() | lz::reverse())::__is_materializing);
static_assert(decltype(micron::declval<__fwd>() | lz::reverse())::__is_materializing);
static_assert(decltype(micron::declval<__ctg>() | lz::sort())::__is_materializing);
static_assert(decltype(micron::declval<__ctg>() | lz::sort_by(__cmp_lt))::__is_materializing);

// the windowing adaptors inherit it: a contiguous source is windowed in place, a forward-only one is
// drained ONCE and windowed over that -- one allocation, not one per window
static_assert(!decltype(micron::declval<__ctg>() | lz::chunk(2))::__is_materializing);
static_assert(decltype(micron::declval<__fwd>() | lz::chunk(2))::__is_materializing);
static_assert(!decltype(micron::declval<__ctg>() | lz::sliding(2))::__is_materializing);
static_assert(!decltype(micron::declval<__ctg>() | lz::group())::__is_materializing);

// streaming adaptors never materialize, nub included -- it allocates, but it is BOUNDED allocation
// (a seen-set), not a copy of the input
static_assert(!decltype(micron::declval<__ctg>() | lz::nub())::__is_materializing);
static_assert(!decltype(micron::declval<__ctg>() | lz::unique())::__is_materializing);
static_assert(!decltype(micron::declval<__ctg>() | lz::scanl(0, __add2))::__is_materializing);
static_assert(!decltype(micron::declval<__ctg>() | lz::enumerate())::__is_materializing);
static_assert(!decltype(micron::declval<__ctg>() | lz::intersperse(0))::__is_materializing);
static_assert(!decltype(micron::declval<__ctg>() | lz::step_by(2))::__is_materializing);
static_assert(!__nested::__is_materializing);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// size-kind rows for the Phase-4 views
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::reverse())> == lz::size_kind::exact);
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::sort())> == lz::size_kind::exact);
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::chunk(2))> == lz::size_kind::exact);
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::sliding(2))> == lz::size_kind::exact);
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::enumerate())> == lz::size_kind::exact);
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::group())> == lz::size_kind::bounded);
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::nub())> == lz::size_kind::bounded);
static_assert(lz::kind_of<decltype(micron::declval<__ctg>() | lz::unique())> == lz::size_kind::bounded);
// flatten cannot know its length without walking every inner range
static_assert(lz::kind_of<__nested> == lz::size_kind::unknown);
// unfold terminates on its own, so unlike counting/iterate it is collectable with no take()
static_assert(lz::kind_of<decltype(lz::unfold(__step, 0))> == lz::size_kind::unknown);
// once/empty carry a compile-time extent
static_assert(lz::kind_of<decltype(lz::once(1))> == lz::size_kind::exact);
static_assert(lz::static_size_of<decltype(lz::once(1))> == 1);
static_assert(lz::static_size_of<decltype(lz::empty<int>())> == lz::no_static_size);
// enumerate is length-preserving, so the extent survives it
static_assert(lz::static_size_of<decltype(micron::declval<micron::array<int, 8> &>() | lz::enumerate())> == 8);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the firewall again, for the Phase-4 views: still no view may look like a container, and no
// terminal closure may look like a range
static_assert(!micron::is_iterable_container<decltype(micron::declval<__ctg>() | lz::reverse())>);
static_assert(!micron::is_iterable_container<decltype(micron::declval<__ctg>() | lz::sort())>);
static_assert(!micron::is_iterable_container<lz::buffer_view<int>>);
static_assert(!r::range<decltype(lz::sum())>);
static_assert(!r::range<decltype(lz::count())>);
static_assert(!r::range<decltype(lz::fold(0, __add2))>);
static_assert(!r::range<decltype(lz::any_of(__gt0))>);
static_assert(!r::range<decltype(lz::partition<vector<int>>(__gt0))>);
static_assert(r::view<decltype(micron::declval<__ctg>() | lz::reverse())>);
static_assert(r::view<decltype(micron::declval<__ctg>() | lz::chunk(2))>);

int
main()
{
  int acc = 0;

  int arr[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  micron::vector<int> v = { 1, 2, 3, 4, 5 };

  // the three source shapes, and the pipe from a non-micron root (the ADL case)
  acc += static_cast<int>((arr | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((lz::of(v) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((micron::vector<int>{ 9, 9 } | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((lz::owned(micron::vector<int>{ 1 }) | lz::collect<vector<int>>()).size());

  // fixed-extent destination, which only compiles while the extent survives the chain
  micron::array<int, 8> sa;
  micron::fill(sa, 3);
  acc += lz::of(sa) | lz::collect<micron::array<int, 8>>() | [](auto a) { return a[0]; };

  // node sources
  micron::list<int> l;
  l.push_back(4);
  acc += static_cast<int>((l | lz::collect<vector<int>>()).size());

  micron::double_list<int> dl;
  dl.push_back(5);
  acc += static_cast<int>((dl | lz::collect<vector<int>>()).size());

  // view_interface's supplied members
  auto pv = lz::of(v);
  acc += static_cast<int>(pv.size()) + static_cast<int>(!pv.empty()) + pv.front() + pv[1];

  // every adaptor, over a contiguous source and over a node source
  acc += static_cast<int>((v | lz::fmap([](int x) { return x * 2; }) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::filter([](int x) { return x > 2; }) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::reject([](int x) { return x > 2; }) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::take(2) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::drop(2) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::take_while([](int x) { return x < 4; }) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::drop_while([](int x) { return x < 4; }) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((l | lz::fmap([](int x) { return x + 1; }) | lz::filter([](int x) { return x > 0; })
                             | lz::take(1) | lz::collect<vector<int>>())
                              .size());

  // generators, which only terminate because take/take_while bound them
  acc += static_cast<int>((lz::counting(0) | lz::take(4) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((lz::iterate([](int x) { return x + 2; }, 1) | lz::take(3) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((lz::replicate(3, 9) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((lz::counting(0) | lz::take_while([](int x) { return x < 5; }) | lz::collect<vector<int>>()).size());

  // extent survives fmap into a fixed-size destination
  acc += (lz::of(sa) | lz::fmap([](int x) { return x + 1; }) | lz::collect<micron::array<int, 8>>())[0];

  // an adaptor over an OWNED source. owning_view has one begin()/end() pair for a reason: with a
  // non-const pair alongside it, ranges::iterator_t (which probes a non-const V &) and the adaptor's
  // begin() const disagree on the iterator type and none of these compile.
  acc += static_cast<int>((micron::vector<int>{ 1, 2, 3 } | lz::fmap([](int x) { return x; }) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((micron::vector<int>{ 1, 2, 3 } | lz::filter([](int x) { return x > 1; }) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((micron::vector<int>{ 1, 2, 3 } | lz::take(2) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((micron::vector<int>{ 1, 2, 3 } | lz::take_while([](int x) { return x < 3; }) | lz::collect<vector<int>>()).size());

  // an adaptor over a MAP source -- its cursor declares operator*() non-const and yields a prvalue
  // proxy, which is why the lazy cursors hold their upstream `mutable`
  micron::robin_map<int, int> rm;
  rm.insert(1, 10);
  acc += static_cast<int>((rm | lz::keys() | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((rm | lz::values() | lz::filter([](int x) { return x > 0; }) | lz::count()));

  // %%%% terminals
  acc += static_cast<int>(v | lz::count());
  acc += static_cast<int>(v | lz::count_if([](int x) { return x > 1; }));
  acc += static_cast<int>(v | lz::count_of(3));
  acc += static_cast<int>(v | lz::fold(0, [](int a, int x) { return a + x; }));
  acc += static_cast<int>(v | lz::foldl(0, [](int a, int x) { return a + x; }));
  acc += static_cast<int>(v | lz::foldr([](int x, int a) { return x + a; }, 0));
  acc += static_cast<int>(v | lz::sum());
  acc += static_cast<int>((v | lz::safe_sum()).is_first());
  acc += (v | lz::min()) + (v | lz::max());
  acc += static_cast<int>((v | lz::safe_min()).is_first()) + static_cast<int>((v | lz::safe_max()).is_first());
  acc += static_cast<int>(v | lz::mean<f64>());
  acc += static_cast<int>((v | lz::safe_mean<f64>()).is_first());
  acc += static_cast<int>(v | lz::geomean<f64>()) + static_cast<int>(v | lz::harmonicmean<f64>());
  acc += static_cast<int>((v | lz::safe_geomean<f64>()).is_first()) + static_cast<int>((v | lz::safe_harmonicmean<f64>()).is_first());
  acc += static_cast<int>(v | lz::inner_product<f64>(v));
  acc += static_cast<int>((v | lz::safe_inner_product<f64>(v)).is_first());
  v | lz::for_each([&](int x) { acc += x; });

  acc += static_cast<int>(v | lz::all_of([](int x) { return x > 0; }));
  acc += static_cast<int>(v | lz::any_of([](int x) { return x > 3; }));
  acc += static_cast<int>(v | lz::none_of([](int x) { return x > 9; }));
  acc += static_cast<int>(v | lz::all_of_c([](int x) { return x > 0; }));
  acc += static_cast<int>(v | lz::any_of_c([](int x) { return x > 0; }));
  acc += static_cast<int>(v | lz::none_of_c([](int x) { return x > 9; }));
  acc += static_cast<int>((v | lz::find_first([](int x) { return x > 2; })).is_first());
  acc += static_cast<int>((v | lz::find_last([](int x) { return x > 2; })).is_first());
  acc += static_cast<int>((v | lz::find_index([](int x) { return x > 2; })).is_first());
  acc += static_cast<int>((v | lz::find_of(3)).is_first());
  acc += static_cast<int>(v | lz::elem(3));
  acc += static_cast<int>((v | lz::at(1)).is_first()) + static_cast<int>((v | lz::nth(1)).is_first());
  acc += static_cast<int>((v | lz::head()).is_first()) + static_cast<int>((v | lz::last()).is_first());
  acc += static_cast<int>(micron::get<0>(v | lz::span_at<vector<int>>([](int x) { return x < 3; })).size());
  acc += static_cast<int>(micron::get<0>(v | lz::sbreak<vector<int>>([](int x) { return x > 3; })).size());

  acc += static_cast<int>(micron::get<0>(v | lz::partition<vector<int>>([](int x) { return x > 2; })).size());
  acc += static_cast<int>((v | lz::traverse<vector<int>>([](int x) {
                             using O = micron::option<int, micron::fp::empty_container_error>;
                             return O{ x };
                           })).is_first());
  acc += static_cast<int>((v | lz::uncons<vector<int>>()).is_first());
  {
    micron::vector<micron::option<int, micron::fp::empty_container_error>> ov;
    ov.push_back(micron::option<int, micron::fp::empty_container_error>{ 1 });
    acc += static_cast<int>((ov | lz::sequence<vector<int>>()).is_first());
    acc += static_cast<int>(ov | lz::sequence_check());
  }
  {
    micron::vector<micron::tuple<int, int>> tv;
    tv.push_back(micron::make_tuple(1, 2));
    acc += static_cast<int>(micron::get<0>(tv | lz::unzip<vector<int>, vector<int>>()).size());
  }

  // %%%% arithmetic mirror
  acc += static_cast<int>((v | lz::add(1) | lz::subtract(1) | lz::multiply(2) | lz::divide(2) | lz::sum()));
  acc += static_cast<int>((v | lz::pow(2) | lz::sum()));
  acc += static_cast<int>((v | lz::add_c(1) | lz::subtract_c(1) | lz::multiply_c(1) | lz::divide_c(1) | lz::pow_c(1) | lz::count()));
  acc += static_cast<int>((v | lz::negate() | lz::abs() | lz::clamp_each(0, 3) | lz::sum()));
  acc += static_cast<int>((lz::add(v, 1) | lz::count())) + static_cast<int>((lz::subtract(v, 1) | lz::count()));
  acc += static_cast<int>((lz::multiply(v, 1) | lz::count())) + static_cast<int>((lz::divide(v, 1) | lz::count()));
  acc += static_cast<int>((v | lz::add_zip(v) | lz::subtract_zip(v) | lz::multiply_zip(v) | lz::divide_zip(v) | lz::count()));
  acc += static_cast<int>((v | lz::safe_divide<vector<int>>(2)).is_first());
  acc += static_cast<int>((v | lz::safe_divide_zip<vector<int>>(v)).is_first());
  acc += static_cast<int>((v | lz::divide_zip_each<int, int>(v) | lz::sequence<vector<int>>()).is_first());

  // %%%% sources
  acc += static_cast<int>((lz::once(1) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((lz::empty<int>() | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((lz::repeat(3) | lz::take(2) | lz::count()));
  acc += static_cast<int>((lz::unfold(__step, 0) | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::tail() | lz::count())) + static_cast<int>((v | lz::init() | lz::count()));

  // %%%% Phase-4 adaptors
  acc += static_cast<int>((v | lz::reverse() | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((l | lz::reverse() | lz::collect<vector<int>>()).size());      // the buffered arm
  acc += static_cast<int>((v | lz::reverse_c() | lz::count()));
  acc += static_cast<int>((v | lz::sort() | lz::collect<vector<int>>()).size());
  acc += static_cast<int>((v | lz::sort_by(__cmp_lt) | lz::count()));
  acc += static_cast<int>((v | lz::sort_c() | lz::count())) + static_cast<int>((v | lz::sort_by_c(__cmp_lt) | lz::count()));
  acc += static_cast<int>((v | lz::scanl(0, __add2) | lz::count()));
  acc += static_cast<int>((v | lz::scan(0, __add2) | lz::count()));
  acc += static_cast<int>((v | lz::scanr(__add2, 0) | lz::count()));
  acc += static_cast<int>((v | lz::zip_with(v, __add2) | lz::count()));
  acc += static_cast<int>((v | lz::zip_with_trunc(v, __add2) | lz::count()));
  acc += static_cast<int>((v | lz::zip(v) | lz::count()));
  acc += static_cast<int>((v | lz::enumerate() | lz::count()));
  acc += static_cast<int>((v | lz::zip_strict<vector<int>>(v, __add2)).is_first());
  acc += static_cast<int>((v | lz::unique() | lz::count()));
  acc += static_cast<int>((v | lz::unique(__cmp_lt) | lz::count()));
  acc += static_cast<int>((v | lz::nub() | lz::count()));
  acc += static_cast<int>((v | lz::nub_by([](int a, int b) { return a == b; }) | lz::count()));
  acc += static_cast<int>((v | lz::intersperse(0) | lz::count()));
  acc += static_cast<int>((v | lz::intersperse_c(0) | lz::count()));
  acc += static_cast<int>((v | lz::step_by(2) | lz::count()));
  acc += static_cast<int>((v | lz::chunk(2) | lz::count()));
  acc += static_cast<int>((l | lz::chunk(2) | lz::count()));      // the drain-once arm
  acc += static_cast<int>((v | lz::sliding(2) | lz::count()));
  acc += static_cast<int>((v | lz::group() | lz::count()));
  acc += static_cast<int>((v | lz::group_by(__cmp_lt) | lz::count()));
  acc += static_cast<int>((v | lz::chunk_into<micron::vector<micron::vector<int>>>(2)).size());
  acc += static_cast<int>((v | lz::fmap(__wrap) | lz::flatten() | lz::count()));
  acc += static_cast<int>((v | lz::flat_map(__wrap) | lz::count()));
  acc += static_cast<int>((v | lz::concat(v) | lz::count())) + static_cast<int>((v | lz::merge(v) | lz::count()));
  {
    micron::vector<micron::vector<int>> mat;
    mat.push_back(micron::vector<int>{ 1, 2 });
    mat.push_back(micron::vector<int>{ 3, 4 });
    acc += static_cast<int>((mat | lz::transpose<micron::vector<micron::vector<int>>>()).is_first());
    acc += static_cast<int>((mat | lz::transpose_trunc<micron::vector<micron::vector<int>>>()).size());
    acc += static_cast<int>((mat | lz::intercalate(micron::vector<int>{ 0 }) | lz::count()));
  }

  // a long fused chain, terminating in a scalar: the shape the whole layer exists for
  acc += static_cast<int>(lz::counting(1) | lz::fmap([](int x) { return x * 3; }) | lz::filter([](int x) { return (x & 1) == 1; })
                          | lz::take(8) | lz::sum());

  return acc & 0x7f;
}
