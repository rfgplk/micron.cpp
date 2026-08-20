//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: the eager functional layer (src/algorithm/fp*.hpp) instantiates on every
// arch/opt/freestanding combo (see verify_compile.duck). Not run.
//
// this gate did not exist before the lazy layer went in. the eager layer is the lazy layer's test
// oracle, so an arm64 or -k failure has to be attributable to one or the other -- without this cell
// a Phase-2 break is indistinguishable from a pre-existing one.
//
// each fp:: function carries 3-4 predicate-shape overloads (value / const T* / T* / micron::function)
// and the wrong one is chosen silently, so every family below is instantiated through the value form
// AND the pointer form where both exist.

#include "../../src/algorithm/fp.hpp"
#include "../../src/array.hpp"
#include "../../src/vector.hpp"

int
main()
{
  micron::vector<int> v = { 5, 3, 9, 1, 7, 3, 9 };
  micron::vector<int> w = { 2, 4, 6, 8, 1, 0, 2 };
  int acc = 0;

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // fmap -- note the argument order is (fn, container), inverted from every other sequence op
  auto sq = micron::fmap([](int x) { return x * x; }, v);
  acc += sq[0];
  auto pd = micron::fmap([](int *p) { return *p + 1; }, v);
  acc += pd[0];
  auto as_f = micron::fmap_into<micron::vector<f64>>([](int x) { return static_cast<f64>(x) * 0.5; }, v);
  acc += static_cast<int>(as_f[0]);
  acc += micron::fmap_c([](int x) { return x - 1; })(v)[0];

  // the static-size unroll path (unrollable<C>, C::static_size <= __unroll_max)
  micron::array<int, 8> a;
  micron::fill(a, 2);
  auto a2 = micron::fmap([](int x) { return x * 3; }, a);
  acc += a2[0];

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // scans -- both produce n+1 elements, seed included
  auto sl = micron::scanl(v, 0, [](int s, int x) { return s + x; });
  auto sr = micron::scanr(v, 1, [](int x, int s) { return x * s; });
  acc += sl[0] + sr[0];

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // zip family -- zip_with returns option<C, bad_zip_error>, is_first() is success
  auto z = micron::zip_with(v, w, [](int x, int y) { return x + y; });
  if ( z.is_first() ) acc += z.cast<micron::vector<int>>()[0];
  acc += micron::zip_with_trunc(v, w, [](int x, int y) { return x * y; })[0];
  acc += static_cast<int>(micron::inner_product(v, w, 0));

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // filter family. micron::filter takes a POINTER predicate only (filter.hpp:80); reject/partition
  // take a value predicate. both spellings have to instantiate here.
  acc += micron::filter_c([](const int *p) { return (*p & 1) == 0; })(v).size() ? 1 : 0;
  acc += micron::reject(v, [](int x) { return x > 4; }).size() ? 1 : 0;
  auto pt = micron::partition(v, [](int x) { return x & 1; });
  acc += static_cast<int>(micron::get<0>(pt).size());

  acc += micron::take(v, 3)[0] + micron::drop(v, 3)[0];
  acc += micron::take_while(v, [](int x) { return x > 2; }).size() ? 1 : 0;
  acc += micron::drop_while(v, [](int x) { return x > 2; }).size() ? 1 : 0;
  acc += static_cast<int>(micron::get<0>(micron::sbreak(v, [](int x) { return x < 4; })).size());
  acc += static_cast<int>(micron::nub(v).size() + micron::unique(v).size());

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // data: flatten/chunk/sliding/group/transpose all take an explicit Out container
  using vv = micron::vector<micron::vector<int>>;
  vv nest;
  nest.push_back(v);
  nest.push_back(w);
  acc += micron::flatten(nest)[0];
  acc += static_cast<int>(micron::chunk_into<vv>(v, 3).size() + micron::sliding<vv>(v, 2).size());
  acc += static_cast<int>(micron::group<vv>(v).size());
  acc += static_cast<int>(micron::group_by<vv>(v, [](int x, int y) { return x == y; }).size());
  auto tr = micron::transpose(nest);
  if ( tr.is_first() ) acc += static_cast<int>(tr.cast<vv>().size());
  acc += micron::intersperse(v, 0)[0];
  acc += micron::intercalate<vv>(w, nest)[0];
  acc += micron::flat_map(v, [](int x) { return micron::vector<int>(2, x); })[0];
  acc += micron::concat(v, w)[0] + micron::merge(v, w)[0];

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // element access -- every one of these is option<T, E>, success is is_first()
  auto h = micron::head(v);
  auto lt = micron::last(v);
  auto at = micron::at(v, 2);
  auto ff = micron::find_first(v, [](int x) { return x > 4; });
  auto fl = micron::find_last(v, [](int x) { return x > 4; });
  if ( h.is_first() ) acc += h.cast<int>();
  if ( lt.is_first() ) acc += lt.cast<int>();
  if ( at.is_first() ) acc += at.cast<int>();
  if ( ff.is_first() ) acc += ff.cast<int>();
  if ( fl.is_first() ) acc += fl.cast<int>();
  auto tl = micron::tail(v);
  auto in = micron::init(v);
  if ( tl.is_first() ) acc += tl.cast<micron::vector<int>>()[0];
  if ( in.is_first() ) acc += in.cast<micron::vector<int>>()[0];
  acc += micron::elem(v, 9) ? 1 : 0;

  using ve = micron::vector<micron::tuple<usize, int>>;
  acc += static_cast<int>(micron::enumerate<ve>(v).size());

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // safe aggregates. safe_sum returns umax_t for integral and f128 for floating -- f128 is
  // arch-conditional (types.hpp), so this is the cell that matters on arm32/i386.
  auto mx = micron::safe_max(v);
  auto mn = micron::safe_min(v);
  auto sm = micron::safe_sum(v);
  auto mean = micron::safe_mean(v);
  if ( mx.is_first() ) acc += mx.cast<int>();
  if ( mn.is_first() ) acc += mn.cast<int>();
  if ( sm.is_first() ) acc += static_cast<int>(sm.cast<umax_t>());
  if ( mean.is_first() ) acc += static_cast<int>(mean.cast<f64>());

  micron::vector<f64> fv = { 1.5, 2.5, 3.5 };
  auto fsm = micron::safe_sum(fv);
  if ( fsm.is_first() ) acc += static_cast<int>(fsm.cast<f128>());

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // quantifiers + curried mutators
  acc += micron::all_of_c([](int x) { return x > 0; })(v) ? 1 : 0;
  acc += micron::any_of_c([](int x) { return x > 8; })(v) ? 1 : 0;
  acc += micron::none_of_c([](int x) { return x > 99; })(v) ? 1 : 0;
  acc += micron::transform_c([](int x) { return x + 1; })(v)[0];
  acc += micron::fill_c(4)(v)[0];
  acc += micron::reverse_c()(v)[0];
  acc += micron::sort_c()(v)[0];
  acc += micron::sort_by_c([](int x, int y) { return x > y; })(v)[0];
  acc += micron::clamp_each_c(2, 7)(v)[0];
  acc += micron::replicate<micron::vector<int>>(4, 3)[0];
  acc += micron::on([](int x, int y) { return x + y; }, [](int x) { return x * 2; })(1, 2);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // curried scalars + option-lifted arithmetic
  acc += micron::add_c(2)(v)[0] + micron::subtract_c(1)(v)[0];
  acc += micron::multiply_c(2)(v)[0] + micron::divide_c(2)(v)[0];
  acc += micron::add_zip(v, w)[0] + micron::subtract_zip(v, w)[0] + micron::multiply_zip(v, w)[0];
  acc += micron::negate(v)[0];
  auto sd = micron::safe_divide(v, 2);
  if ( sd.is_first() ) acc += sd.cast<micron::vector<int>>()[0];

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // traverse: Kleisli map, short-circuits on the first error branch
  auto tv = micron::traverse(v, [](int x) { return micron::option<int, micron::empty_container_error>{ x * 2 }; });
  if ( tv.is_first() ) acc += tv.cast<micron::vector<int>>()[0];

  return acc & 0x7f;
}
