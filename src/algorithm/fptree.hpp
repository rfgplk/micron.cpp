//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// functional-programming combinators for trees

#include "../concepts.hpp"
#include "../memory/addr.hpp"
#include "../sum.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "../vector/fvector.hpp"

#include "fperrors.hpp"
#include "tree.hpp"

namespace micron
{
namespace fp
{

// keys / values / unzip
template<is_tree_map Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::key_type>
keys(const Tree &t)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::key_type> out;
  t.for_each([&](const auto &k, const auto &) { out.push_back(k); });
  return out;
}

template<is_tree_map Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>
values(const Tree &t)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type> out;
  t.for_each([&](const auto &, const auto &v) { out.push_back(v); });
  return out;
}

template<is_tree_map Tree>
micron::pair<micron::fvector<typename micron::remove_cvref_t<Tree>::key_type>,
             micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>>
unzip(const Tree &t)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::key_type> ks;
  micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type> vs;
  t.for_each([&](const auto &k, const auto &v) {
    ks.push_back(k);
    vs.push_back(v);
  });
  return micron::pair<micron::fvector<typename micron::remove_cvref_t<Tree>::key_type>,
                      micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>>(micron::move(ks), micron::move(vs));
}

// elems
template<is_set_tree Tree>
bool
elem(const Tree &t, const typename micron::remove_cvref_t<Tree>::value_type &x)
{
  bool found = false;
  t.for_each([&](const auto &e) {
    if ( !found && e == x ) found = true;
  });
  return found;
}

template<is_tree_map Tree>
bool
elem(const Tree &t, const typename micron::remove_cvref_t<Tree>::mapped_type &x)
{
  bool found = false;
  t.for_each([&](const auto &, const auto &v) {
    if ( !found && v == x ) found = true;
  });
  return found;
}

// find_first -> option (set: element; map: value)
template<is_set_tree Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::value_type &>
micron::option<typename micron::remove_cvref_t<Tree>::value_type, empty_container_error>
find_first(const Tree &t, Fn fn)
{
  using V = typename micron::remove_cvref_t<Tree>::value_type;
  const V *hit = nullptr;
  t.for_each([&](const auto &e) {
    if ( !hit && fn(e) ) hit = micron::addressof(e);
  });
  if ( hit ) return micron::option<V, empty_container_error>{ *hit };
  return micron::option<V, empty_container_error>{ empty_container_error{} };
}

template<is_tree_map Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::key_type &,
                                  const typename micron::remove_cvref_t<Tree>::mapped_type &>
micron::option<typename micron::remove_cvref_t<Tree>::mapped_type, empty_container_error>
find_first(const Tree &t, Fn fn)
{
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;
  const V *hit = nullptr;
  t.for_each([&](const auto &k, const auto &v) {
    if ( !hit && fn(k, v) ) hit = micron::addressof(v);
  });
  if ( hit ) return micron::option<V, empty_container_error>{ *hit };
  return micron::option<V, empty_container_error>{ empty_container_error{} };
}

// nub -> distinct elements (set) / values (map) as a sequence
template<is_set_tree Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::value_type>
nub(const Tree &t)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::value_type> out;
  t.for_each([&](const auto &e) {
    for ( const auto &x : out )
      if ( x == e ) return;
    out.push_back(e);
  });
  return out;
}

template<is_tree_map Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>
nub(const Tree &t)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type> out;
  t.for_each([&](const auto &, const auto &v) {
    for ( const auto &x : out )
      if ( x == v ) return;
    out.push_back(v);
  });
  return out;
}

// flat_map -> sequence (concat of per-element containers)
template<is_set_tree Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::value_type &>
auto
flat_map(const Tree &t, Fn fn)
{
  using V = typename micron::remove_cvref_t<Tree>::value_type;
  using R = micron::remove_cvref_t<decltype(fn(micron::declval<const V &>()))>;
  using E = typename R::value_type;
  micron::fvector<E> out;
  t.for_each([&](const auto &e) {
    auto sub = fn(e);
    for ( const auto &x : sub ) out.push_back(x);
  });
  return out;
}

template<is_tree_map Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::key_type &,
                                  const typename micron::remove_cvref_t<Tree>::mapped_type &>
auto
flat_map(const Tree &t, Fn fn)
{
  using K = typename micron::remove_cvref_t<Tree>::key_type;
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;
  using R = micron::remove_cvref_t<decltype(fn(micron::declval<const K &>(), micron::declval<const V &>()))>;
  using E = typename R::value_type;
  micron::fvector<E> out;
  t.for_each([&](const auto &k, const auto &v) {
    auto sub = fn(k, v);
    for ( const auto &x : sub ) out.push_back(x);
  });
  return out;
}

// option-guarded aggregates
template<is_set_tree Tree>
micron::option<typename micron::remove_cvref_t<Tree>::value_type, empty_container_error>
safe_max(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::value_type;
  if ( t.size() == 0 ) return micron::option<V, empty_container_error>{ empty_container_error{} };
  return micron::option<V, empty_container_error>{ micron::max(t) };
}

template<is_tree_map Tree>
micron::option<typename micron::remove_cvref_t<Tree>::mapped_type, empty_container_error>
safe_max(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;
  if ( t.size() == 0 ) return micron::option<V, empty_container_error>{ empty_container_error{} };
  return micron::option<V, empty_container_error>{ micron::max(t) };
}

template<is_set_tree Tree>
micron::option<typename micron::remove_cvref_t<Tree>::value_type, empty_container_error>
safe_min(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::value_type;
  if ( t.size() == 0 ) return micron::option<V, empty_container_error>{ empty_container_error{} };
  return micron::option<V, empty_container_error>{ micron::min(t) };
}

template<is_tree_map Tree>
micron::option<typename micron::remove_cvref_t<Tree>::mapped_type, empty_container_error>
safe_min(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;
  if ( t.size() == 0 ) return micron::option<V, empty_container_error>{ empty_container_error{} };
  return micron::option<V, empty_container_error>{ micron::min(t) };
}

template<is_set_tree Tree>
  requires micron::is_integral_v<typename micron::remove_cvref_t<Tree>::value_type>
micron::option<umax_t, empty_container_error>
safe_sum(const Tree &t)
{
  if ( t.size() == 0 ) return micron::option<umax_t, empty_container_error>{ empty_container_error{} };
  return micron::option<umax_t, empty_container_error>{ micron::sum(t) };
}

template<is_set_tree Tree>
  requires micron::is_floating_point_v<typename micron::remove_cvref_t<Tree>::value_type>
micron::option<f128, empty_container_error>
safe_sum(const Tree &t)
{
  if ( t.size() == 0 ) return micron::option<f128, empty_container_error>{ empty_container_error{} };
  return micron::option<f128, empty_container_error>{ micron::sum(t) };
}

template<is_tree_map Tree>
  requires micron::is_integral_v<typename micron::remove_cvref_t<Tree>::mapped_type>
micron::option<umax_t, empty_container_error>
safe_sum(const Tree &t)
{
  if ( t.size() == 0 ) return micron::option<umax_t, empty_container_error>{ empty_container_error{} };
  return micron::option<umax_t, empty_container_error>{ micron::sum(t) };
}

template<is_tree_map Tree>
  requires micron::is_floating_point_v<typename micron::remove_cvref_t<Tree>::mapped_type>
micron::option<f128, empty_container_error>
safe_sum(const Tree &t)
{
  if ( t.size() == 0 ) return micron::option<f128, empty_container_error>{ empty_container_error{} };
  return micron::option<f128, empty_container_error>{ micron::sum(t) };
}

template<typename R = f64, is_set_tree Tree>
micron::option<R, empty_container_error>
safe_mean(const Tree &t)
{
  if ( t.size() == 0 ) return micron::option<R, empty_container_error>{ empty_container_error{} };
  return micron::option<R, empty_container_error>{ micron::mean<R>(t) };
}

template<typename R = f64, is_tree_map Tree>
micron::option<R, empty_container_error>
safe_mean(const Tree &t)
{
  if ( t.size() == 0 ) return micron::option<R, empty_container_error>{ empty_container_error{} };
  return micron::option<R, empty_container_error>{ micron::mean<R>(t) };
}

// concat: union of two trees
template<is_set_tree Tree>
micron::remove_cvref_t<Tree>
concat(const Tree &a, const Tree &b)
{
  micron::remove_cvref_t<Tree> out;
  a.for_each([&](const auto &e) { out.insert(e); });
  b.for_each([&](const auto &e) { out.insert(e); });
  return out;
}

template<is_tree_map Tree>
micron::remove_cvref_t<Tree>
concat(const Tree &a, const Tree &b)
{
  micron::remove_cvref_t<Tree> out;
  a.for_each([&](const auto &k, const auto &v) { out.insert(k, v); });
  b.for_each([&](const auto &k, const auto &v) { out.insert(k, v); });
  return out;
}

// traverse (tree_map values) : v -> option<U,E>, short-circuit -> option<tree, E>
template<is_tree_map Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::mapped_type &>
auto
traverse(const Tree &t, Fn fn)
{
  using TT = micron::remove_cvref_t<Tree>;
  using V = typename TT::mapped_type;
  using Opt = micron::remove_cvref_t<decltype(fn(micron::declval<const V &>()))>;
  using E = typename Opt::second_type;

  bool ok = true;
  E err{};
  TT out;
  t.for_each([&](const auto &k, const auto &v) {
    if ( !ok ) return;
    Opt r = fn(v);
    if ( r.is_first() )
      out.insert(k, r.template cast<V>());
    else {
      ok = false;
      err = r.template cast<E>();
    }
  });
  if ( ok ) return micron::option<TT, E>{ micron::move(out) };
  return micron::option<TT, E>{ micron::move(err) };
}

// take: first n
template<is_set_tree Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::value_type>
take(const Tree &t, usize n)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::value_type> out;
  usize i = 0;
  t.for_each([&](const auto &e) {
    if ( i < n ) out.push_back(e);
    ++i;
  });
  return out;
}

template<is_tree_map Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>
take(const Tree &t, usize n)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type> out;
  usize i = 0;
  t.for_each([&](const auto &, const auto &v) {
    if ( i < n ) out.push_back(v);
    ++i;
  });
  return out;
}

// drop: all but first n
template<is_set_tree Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::value_type>
drop(const Tree &t, usize n)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::value_type> out;
  usize i = 0;
  t.for_each([&](const auto &e) {
    if ( i >= n ) out.push_back(e);
    ++i;
  });
  return out;
}

template<is_tree_map Tree>
micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>
drop(const Tree &t, usize n)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type> out;
  usize i = 0;
  t.for_each([&](const auto &, const auto &v) {
    if ( i >= n ) out.push_back(v);
    ++i;
  });
  return out;
}

// head / last -> option
template<is_set_tree Tree>
micron::option<typename micron::remove_cvref_t<Tree>::value_type, empty_container_error>
head(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::value_type;
  bool got = false;
  V val{};
  t.for_each([&](const auto &e) {
    if ( !got ) {
      val = e;
      got = true;
    }
  });
  if ( got ) return micron::option<V, empty_container_error>{ val };
  return micron::option<V, empty_container_error>{ empty_container_error{} };
}

template<is_tree_map Tree>
micron::option<typename micron::remove_cvref_t<Tree>::mapped_type, empty_container_error>
head(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;
  bool got = false;
  V val{};
  t.for_each([&](const auto &, const auto &v) {
    if ( !got ) {
      val = v;
      got = true;
    }
  });
  if ( got ) return micron::option<V, empty_container_error>{ val };
  return micron::option<V, empty_container_error>{ empty_container_error{} };
}

template<is_set_tree Tree>
micron::option<typename micron::remove_cvref_t<Tree>::value_type, empty_container_error>
last(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::value_type;
  bool got = false;
  V val{};
  t.for_each([&](const auto &e) {
    val = e;
    got = true;
  });
  if ( got ) return micron::option<V, empty_container_error>{ val };
  return micron::option<V, empty_container_error>{ empty_container_error{} };
}

template<is_tree_map Tree>
micron::option<typename micron::remove_cvref_t<Tree>::mapped_type, empty_container_error>
last(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;
  bool got = false;
  V val{};
  t.for_each([&](const auto &, const auto &v) {
    val = v;
    got = true;
  });
  if ( got ) return micron::option<V, empty_container_error>{ val };
  return micron::option<V, empty_container_error>{ empty_container_error{} };
}

// take_while / drop_while (in traversal order)
template<is_set_tree Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::value_type &>
micron::fvector<typename micron::remove_cvref_t<Tree>::value_type>
take_while(const Tree &t, Fn fn)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::value_type> out;
  bool taking = true;
  t.for_each([&](const auto &e) {
    if ( taking ) {
      if ( fn(e) )
        out.push_back(e);
      else
        taking = false;
    }
  });
  return out;
}

template<is_tree_map Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::key_type &,
                                  const typename micron::remove_cvref_t<Tree>::mapped_type &>
micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>
take_while(const Tree &t, Fn fn)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type> out;
  bool taking = true;
  t.for_each([&](const auto &k, const auto &v) {
    if ( taking ) {
      if ( fn(k, v) )
        out.push_back(v);
      else
        taking = false;
    }
  });
  return out;
}

template<is_set_tree Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::value_type &>
micron::fvector<typename micron::remove_cvref_t<Tree>::value_type>
drop_while(const Tree &t, Fn fn)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::value_type> out;
  bool dropping = true;
  t.for_each([&](const auto &e) {
    if ( dropping && fn(e) ) return;
    dropping = false;
    out.push_back(e);
  });
  return out;
}

template<is_tree_map Tree, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<Tree>::key_type &,
                                  const typename micron::remove_cvref_t<Tree>::mapped_type &>
micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type>
drop_while(const Tree &t, Fn fn)
{
  micron::fvector<typename micron::remove_cvref_t<Tree>::mapped_type> out;
  bool dropping = true;
  t.for_each([&](const auto &k, const auto &v) {
    if ( dropping && fn(k, v) ) return;
    dropping = false;
    out.push_back(v);
  });
  return out;
}

// enumerate -> sequence of (index, element/value) in traversal order
template<is_set_tree Tree>
micron::fvector<micron::pair<usize, typename micron::remove_cvref_t<Tree>::value_type>>
enumerate(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::value_type;
  micron::fvector<micron::pair<usize, V>> out;
  usize i = 0;
  t.for_each([&](const auto &e) { out.push_back(micron::pair<usize, V>(i++, e)); });
  return out;
}

template<is_tree_map Tree>
micron::fvector<micron::pair<usize, typename micron::remove_cvref_t<Tree>::mapped_type>>
enumerate(const Tree &t)
{
  using V = typename micron::remove_cvref_t<Tree>::mapped_type;
  micron::fvector<micron::pair<usize, V>> out;
  usize i = 0;
  t.for_each([&](const auto &, const auto &v) { out.push_back(micron::pair<usize, V>(i++, v)); });
  return out;
}

}      // namespace fp
}      // namespace micron
