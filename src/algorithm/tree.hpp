//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../math/generic.hpp"
#include "../memory/addr.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../vector/fvector.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// folds / catamorphisms
template<is_set_tree Tree, typename Acc, typename Fn>
Acc
fold(const Tree &t, Acc init, Fn fn)
{
  t.for_each([&](const auto &e) { init = fn(micron::move(init), e); });
  return init;
}

template<is_tree_map Tree, typename Acc, typename Fn>
Acc
fold(const Tree &t, Acc init, Fn fn)
{
  t.for_each([&](const auto &k, const auto &v) { init = fn(micron::move(init), k, v); });
  return init;
}

template<is_spatial_tree Tree, typename Acc, typename Fn>
Acc
fold(const Tree &t, Acc init, Fn fn)
{
  t.for_each([&](const auto &key, const auto &v) { init = fn(micron::move(init), key, v); });
  return init;
}

template<is_tree Tree, typename Acc, typename Fn>
Acc
fold_left(const Tree &t, Acc init, Fn fn)
{
  return fold(t, micron::move(init), fn);
}

template<is_set_tree Tree, typename Acc>
Acc
accumulate(const Tree &t, Acc init)
{
  t.for_each([&](const auto &e) { init = init + e; });
  return init;
}

template<is_tree_map Tree, typename Acc>
Acc
accumulate(const Tree &t, Acc init)
{
  t.for_each([&](const auto &, const auto &v) { init = init + v; });
  return init;
}

template<is_spatial_tree Tree, typename Acc>
Acc
accumulate(const Tree &t, Acc init)
{
  t.for_each([&](const auto &, const auto &v) { init = init + v; });
  return init;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// predicates
template<is_set_tree Tree, typename Fn>
bool
all_of(const Tree &t, Fn fn)
{
  bool r = true;
  t.for_each([&](const auto &e) {
    if ( r && !fn(e) ) r = false;
  });
  return r;
}

template<is_tree_map Tree, typename Fn>
bool
all_of(const Tree &t, Fn fn)
{
  bool r = true;
  t.for_each([&](const auto &k, const auto &v) {
    if ( r && !fn(k, v) ) r = false;
  });
  return r;
}

template<is_spatial_tree Tree, typename Fn>
bool
all_of(const Tree &t, Fn fn)
{
  bool r = true;
  t.for_each([&](const auto &key, const auto &v) {
    if ( r && !fn(key, v) ) r = false;
  });
  return r;
}

template<is_set_tree Tree, typename Fn>
bool
any_of(const Tree &t, Fn fn)
{
  bool r = false;
  t.for_each([&](const auto &e) {
    if ( !r && fn(e) ) r = true;
  });
  return r;
}

template<is_tree_map Tree, typename Fn>
bool
any_of(const Tree &t, Fn fn)
{
  bool r = false;
  t.for_each([&](const auto &k, const auto &v) {
    if ( !r && fn(k, v) ) r = true;
  });
  return r;
}

template<is_spatial_tree Tree, typename Fn>
bool
any_of(const Tree &t, Fn fn)
{
  bool r = false;
  t.for_each([&](const auto &key, const auto &v) {
    if ( !r && fn(key, v) ) r = true;
  });
  return r;
}

template<is_tree Tree, typename Fn>
bool
none_of(const Tree &t, Fn fn)
{
  return !any_of(t, fn);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// counts
template<is_set_tree Tree, typename Fn>
usize
count_if(const Tree &t, Fn fn)
{
  usize n = 0;
  t.for_each([&](const auto &e) {
    if ( fn(e) ) ++n;
  });
  return n;
}

template<is_tree_map Tree, typename Fn>
usize
count_if(const Tree &t, Fn fn)
{
  usize n = 0;
  t.for_each([&](const auto &k, const auto &v) {
    if ( fn(k, v) ) ++n;
  });
  return n;
}

template<is_spatial_tree Tree, typename Fn>
usize
count_if(const Tree &t, Fn fn)
{
  usize n = 0;
  t.for_each([&](const auto &key, const auto &v) {
    if ( fn(key, v) ) ++n;
  });
  return n;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// finds
template<is_set_tree Tree, typename Fn>
const typename micron::remove_cvref_t<Tree>::value_type *
find_if(const Tree &t, Fn fn)
{
  const typename micron::remove_cvref_t<Tree>::value_type *hit = nullptr;
  t.for_each([&](const auto &e) {
    if ( !hit && fn(e) ) hit = micron::addressof(e);
  });
  return hit;
}

template<is_tree_map Tree, typename Fn>
const typename micron::remove_cvref_t<Tree>::mapped_type *
find_if(const Tree &t, Fn fn)
{
  const typename micron::remove_cvref_t<Tree>::mapped_type *hit = nullptr;
  t.for_each([&](const auto &k, const auto &v) {
    if ( !hit && fn(k, v) ) hit = micron::addressof(v);
  });
  return hit;
}

template<is_spatial_tree Tree, typename Fn>
const typename micron::remove_cvref_t<Tree>::mapped_type *
find_if(const Tree &t, Fn fn)
{
  const typename micron::remove_cvref_t<Tree>::mapped_type *hit = nullptr;
  t.for_each([&](const auto &key, const auto &v) {
    if ( !hit && fn(key, v) ) hit = micron::addressof(v);
  });
  return hit;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// filters

template<is_set_tree Tree, typename Fn>
micron::remove_cvref_t<Tree>
filter(Fn fn, const Tree &t)
{
  micron::remove_cvref_t<Tree> out;
  t.for_each([&](const auto &e) {
    if ( fn(e) ) out.insert(e);
  });
  return out;
}

template<is_tree_map Tree, typename Fn>
micron::remove_cvref_t<Tree>
filter(Fn fn, const Tree &t)
{
  micron::remove_cvref_t<Tree> out;
  t.for_each([&](const auto &k, const auto &v) {
    if ( fn(k, v) ) out.insert(k, v);
  });
  return out;
}

template<is_spatial_tree Tree, typename Fn>
micron::remove_cvref_t<Tree>
filter(Fn fn, const Tree &t)
{
  auto make_out = [&] {
    if constexpr ( requires { t.bounds(); } )
      return micron::remove_cvref_t<Tree>(t.bounds());
    else
      return micron::remove_cvref_t<Tree>();
  };
  micron::remove_cvref_t<Tree> out = make_out();
  t.for_each([&](const auto &key, const auto &v) {
    if ( fn(key, v) ) out.insert(key, v);
  });
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// erases

template<is_set_tree Tree, typename Fn>
usize
erase_if(Tree &t, Fn fn)
{
  using T = typename micron::remove_cvref_t<Tree>::value_type;
  micron::fvector<T> doomed;
  t.for_each([&](const T &e) {
    if ( fn(e) ) doomed.push_back(e);
  });
  for ( const T &k : doomed ) t.erase(k);
  return doomed.size();
}

template<is_tree_map Tree, typename Fn>
usize
erase_if(Tree &t, Fn fn)
{
  using K = typename micron::remove_cvref_t<Tree>::key_type;
  micron::fvector<K> doomed;
  t.for_each([&](const K &k, const auto &v) {
    if ( fn(k, v) ) doomed.push_back(k);
  });
  for ( const K &k : doomed ) t.erase(k);
  return doomed.size();
}

template<is_spatial_tree Tree, typename Fn>
usize
erase_if(Tree &t, Fn fn)
{
  using K = typename micron::remove_cvref_t<Tree>::spatial_key_type;
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;

  struct kv {
    K k;
    V v;
  };

  micron::fvector<kv> doomed;
  t.for_each([&](const auto &key, const auto &val) {
    if ( fn(key, val) ) doomed.push_back(kv{ key, val });
  });
  for ( const kv &d : doomed ) t.erase(d.k, d.v);
  return doomed.size();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fmaps (OCaml like)
template<is_tree Tree, typename Fn>
auto
fmap(Fn fn, const Tree &t)
{
  return t.map(fn);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// find_if_not
template<is_set_tree Tree, typename Fn>
const typename micron::remove_cvref_t<Tree>::value_type *
find_if_not(const Tree &t, Fn fn)
{
  const typename micron::remove_cvref_t<Tree>::value_type *hit = nullptr;
  t.for_each([&](const auto &e) {
    if ( !hit && !fn(e) ) hit = micron::addressof(e);
  });
  return hit;
}

template<is_tree_map Tree, typename Fn>
const typename micron::remove_cvref_t<Tree>::mapped_type *
find_if_not(const Tree &t, Fn fn)
{
  const typename micron::remove_cvref_t<Tree>::mapped_type *hit = nullptr;
  t.for_each([&](const auto &k, const auto &v) {
    if ( !hit && !fn(k, v) ) hit = micron::addressof(v);
  });
  return hit;
}

template<is_spatial_tree Tree, typename Fn>
const typename micron::remove_cvref_t<Tree>::mapped_type *
find_if_not(const Tree &t, Fn fn)
{
  const typename micron::remove_cvref_t<Tree>::mapped_type *hit = nullptr;
  t.for_each([&](const auto &key, const auto &v) {
    if ( !hit && !fn(key, v) ) hit = micron::addressof(v);
  });
  return hit;
}

template<is_tree Tree, typename Fn>
bool
contains_if(const Tree &t, Fn fn)
{
  return any_of(t, fn);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%5
// counts / finds / contains (set + map)
template<is_set_tree Tree>
usize
count(const Tree &t, const typename micron::remove_cvref_t<Tree>::value_type &x)
{
  usize n = 0;
  t.for_each([&](const auto &e) {
    if ( e == x ) ++n;
  });
  return n;
}

template<is_tree_map Tree>
usize
count(const Tree &t, const typename micron::remove_cvref_t<Tree>::mapped_type &x)
{
  usize n = 0;
  t.for_each([&](const auto &, const auto &v) {
    if ( v == x ) ++n;
  });
  return n;
}

template<is_set_tree Tree>
const typename micron::remove_cvref_t<Tree>::value_type *
find(const Tree &t, const typename micron::remove_cvref_t<Tree>::value_type &x)
{
  const typename micron::remove_cvref_t<Tree>::value_type *hit = nullptr;
  t.for_each([&](const auto &e) {
    if ( !hit && e == x ) hit = micron::addressof(e);
  });
  return hit;
}

template<is_tree_map Tree>
const typename micron::remove_cvref_t<Tree>::mapped_type *
find(const Tree &t, const typename micron::remove_cvref_t<Tree>::mapped_type &x)
{
  const typename micron::remove_cvref_t<Tree>::mapped_type *hit = nullptr;
  t.for_each([&](const auto &, const auto &v) {
    if ( !hit && v == x ) hit = micron::addressof(v);
  });
  return hit;
}

template<is_set_tree Tree>
bool
contains(const Tree &t, const typename micron::remove_cvref_t<Tree>::value_type &x)
{
  return find(t, x) != nullptr;
}

template<is_tree_map Tree>
bool
contains(const Tree &t, const typename micron::remove_cvref_t<Tree>::mapped_type &x)
{
  return find(t, x) != nullptr;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// extremas
template<is_set_tree Tree>
typename micron::remove_cvref_t<Tree>::value_type
max(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::value_type best{};
  bool first = true;
  t.for_each([&](const auto &e) {
    if ( first ) {
      best = e;
      first = false;
    } else if ( best < e )
      best = e;
  });
  return best;
}

template<is_set_tree Tree>
typename micron::remove_cvref_t<Tree>::value_type
min(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::value_type best{};
  bool first = true;
  t.for_each([&](const auto &e) {
    if ( first ) {
      best = e;
      first = false;
    } else if ( e < best )
      best = e;
  });
  return best;
}

template<is_tree_map Tree>
typename micron::remove_cvref_t<Tree>::mapped_type
max(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::mapped_type best{};
  bool first = true;
  t.for_each([&](const auto &, const auto &v) {
    if ( first ) {
      best = v;
      first = false;
    } else if ( best < v )
      best = v;
  });
  return best;
}

template<is_tree_map Tree>
typename micron::remove_cvref_t<Tree>::mapped_type
min(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::mapped_type best{};
  bool first = true;
  t.for_each([&](const auto &, const auto &v) {
    if ( first ) {
      best = v;
      first = false;
    } else if ( v < best )
      best = v;
  });
  return best;
}

// argmax / argmin over values -> key (tree_map only)
template<is_tree_map Tree>
typename micron::remove_cvref_t<Tree>::key_type
max_at(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::key_type bestk{};
  typename micron::remove_cvref_t<Tree>::mapped_type bestv{};
  bool first = true;
  t.for_each([&](const auto &k, const auto &v) {
    if ( first ) {
      bestk = k;
      bestv = v;
      first = false;
    } else if ( bestv < v ) {
      bestv = v;
      bestk = k;
    }
  });
  return bestk;
}

template<is_tree_map Tree>
typename micron::remove_cvref_t<Tree>::key_type
min_at(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::key_type bestk{};
  typename micron::remove_cvref_t<Tree>::mapped_type bestv{};
  bool first = true;
  t.for_each([&](const auto &k, const auto &v) {
    if ( first ) {
      bestk = k;
      bestv = v;
      first = false;
    } else if ( v < bestv ) {
      bestv = v;
      bestk = k;
    }
  });
  return bestk;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// numeric aggregates
//
template<is_set_tree Tree>
  requires micron::is_floating_point_v<typename micron::remove_cvref_t<Tree>::value_type>
f128
sum(const Tree &t)
{
  f128 s = 0;
  t.for_each([&](const auto &e) { s += static_cast<f128>(e); });
  return s;
}

template<is_set_tree Tree>
  requires micron::is_integral_v<typename micron::remove_cvref_t<Tree>::value_type>
umax_t
sum(const Tree &t)
{
  umax_t s = 0;
  t.for_each([&](const auto &e) { s += static_cast<umax_t>(e); });
  return s;
}

template<is_tree_map Tree>
  requires micron::is_floating_point_v<typename micron::remove_cvref_t<Tree>::mapped_type>
f128
sum(const Tree &t)
{
  f128 s = 0;
  t.for_each([&](const auto &, const auto &v) { s += static_cast<f128>(v); });
  return s;
}

template<is_tree_map Tree>
  requires micron::is_integral_v<typename micron::remove_cvref_t<Tree>::mapped_type>
umax_t
sum(const Tree &t)
{
  umax_t s = 0;
  t.for_each([&](const auto &, const auto &v) { s += static_cast<umax_t>(v); });
  return s;
}

template<typename R = f64, is_set_tree Tree>
R
mean(const Tree &t)
{
  return static_cast<R>(sum(t)) / static_cast<R>(t.size());
}

template<typename R = f64, is_tree_map Tree>
R
mean(const Tree &t)
{
  return static_cast<R>(sum(t)) / static_cast<R>(t.size());
}

// product fold (accumulate is sum-only)
template<is_set_tree Tree>
typename micron::remove_cvref_t<Tree>::value_type
multiply(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::value_type r = 1;
  t.for_each([&](const auto &e) { r *= e; });
  return r;
}

template<is_tree_map Tree>
typename micron::remove_cvref_t<Tree>::mapped_type
multiply(const Tree &t)
{
  typename micron::remove_cvref_t<Tree>::mapped_type r = 1;
  t.for_each([&](const auto &, const auto &v) { r *= v; });
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rejects + partitions

template<is_set_tree Tree, typename Fn>
micron::remove_cvref_t<Tree>
reject(Fn fn, const Tree &t)
{
  micron::remove_cvref_t<Tree> out;
  t.for_each([&](const auto &e) {
    if ( !fn(e) ) out.insert(e);
  });
  return out;
}

template<is_tree_map Tree, typename Fn>
micron::remove_cvref_t<Tree>
reject(Fn fn, const Tree &t)
{
  micron::remove_cvref_t<Tree> out;
  t.for_each([&](const auto &k, const auto &v) {
    if ( !fn(k, v) ) out.insert(k, v);
  });
  return out;
}

template<is_spatial_tree Tree, typename Fn>
micron::remove_cvref_t<Tree>
reject(Fn fn, const Tree &t)
{
  auto make_out = [&] {
    if constexpr ( requires { t.bounds(); } )
      return micron::remove_cvref_t<Tree>(t.bounds());
    else
      return micron::remove_cvref_t<Tree>();
  };
  micron::remove_cvref_t<Tree> out = make_out();
  t.for_each([&](const auto &key, const auto &v) {
    if ( !fn(key, v) ) out.insert(key, v);
  });
  return out;
}

template<is_set_tree Tree, typename Fn>
micron::pair<micron::remove_cvref_t<Tree>, micron::remove_cvref_t<Tree>>
partition(Fn fn, const Tree &t)
{
  micron::remove_cvref_t<Tree> yes, no;
  t.for_each([&](const auto &e) {
    if ( fn(e) )
      yes.insert(e);
    else
      no.insert(e);
  });
  return micron::pair<micron::remove_cvref_t<Tree>, micron::remove_cvref_t<Tree>>(micron::move(yes), micron::move(no));
}

template<is_tree_map Tree, typename Fn>
micron::pair<micron::remove_cvref_t<Tree>, micron::remove_cvref_t<Tree>>
partition(Fn fn, const Tree &t)
{
  micron::remove_cvref_t<Tree> yes, no;
  t.for_each([&](const auto &k, const auto &v) {
    if ( fn(k, v) )
      yes.insert(k, v);
    else
      no.insert(k, v);
  });
  return micron::pair<micron::remove_cvref_t<Tree>, micron::remove_cvref_t<Tree>>(micron::move(yes), micron::move(no));
}

template<is_spatial_tree Tree, typename Fn>
micron::pair<micron::remove_cvref_t<Tree>, micron::remove_cvref_t<Tree>>
partition(Fn fn, const Tree &t)
{
  auto make_out = [&] {
    if constexpr ( requires { t.bounds(); } )
      return micron::remove_cvref_t<Tree>(t.bounds());
    else
      return micron::remove_cvref_t<Tree>();
  };
  micron::remove_cvref_t<Tree> yes = make_out();
  micron::remove_cvref_t<Tree> no = make_out();
  t.for_each([&](const auto &key, const auto &v) {
    if ( fn(key, v) )
      yes.insert(key, v);
    else
      no.insert(key, v);
  });
  return micron::pair<micron::remove_cvref_t<Tree>, micron::remove_cvref_t<Tree>>(micron::move(yes), micron::move(no));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// catamorphisms

template<is_tree Tree, typename... A>
  requires requires(const Tree &tt) { tt.cata(micron::declval<A>()...); }
auto
cata(const Tree &t, A &&...a)
{
  return t.cata(micron::forward<A>(a)...);
}

template<is_tree Tree, typename... A>
  requires requires(const Tree &tt) { tt.traverse(micron::declval<A>()...); }
auto
traverse(const Tree &t, A &&...a)
{
  return t.traverse(micron::forward<A>(a)...);
}

};      // namespace micron
