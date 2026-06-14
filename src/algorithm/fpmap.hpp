//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// functional-programming combinators for maps

#include "../concepts.hpp"
#include "../memory/addr.hpp"
#include "../sum.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "../vector/fvector.hpp"

#include "../bits/__map_build.hpp"
#include "../bits/__visit_kv.hpp"
#include "fperrors.hpp"
#include "map.hpp"

namespace micron
{
namespace fp
{

// keys / values -> sequence (insertion order of the map's traversal)
template<is_map_class M>
micron::fvector<typename micron::remove_cvref_t<M>::key_type>
keys(const M &m)
{
  micron::fvector<typename micron::remove_cvref_t<M>::key_type> out;
  micron::__impl::visit_kv(m, [&](const auto &k, const auto &) { out.push_back(k); });
  return out;
}

template<is_map_class M>
micron::fvector<typename micron::remove_cvref_t<M>::mapped_type>
values(const M &m)
{
  micron::fvector<typename micron::remove_cvref_t<M>::mapped_type> out;
  micron::__impl::visit_kv(m, [&](const auto &, const auto &v) { out.push_back(v); });
  return out;
}

// unzip
template<is_map_class M>
micron::pair<micron::fvector<typename micron::remove_cvref_t<M>::key_type>,
             micron::fvector<typename micron::remove_cvref_t<M>::mapped_type>>
unzip(const M &m)
{
  micron::fvector<typename micron::remove_cvref_t<M>::key_type> ks;
  micron::fvector<typename micron::remove_cvref_t<M>::mapped_type> vs;
  micron::__impl::visit_kv(m, [&](const auto &k, const auto &v) {
    ks.push_back(k);
    vs.push_back(v);
  });
  return micron::pair<micron::fvector<typename micron::remove_cvref_t<M>::key_type>,
                      micron::fvector<typename micron::remove_cvref_t<M>::mapped_type>>(micron::move(ks), micron::move(vs));
}

// elem: value membership
template<is_map_class M>
bool
elem(const M &m, const typename micron::remove_cvref_t<M>::mapped_type &x)
{
  bool found = false;
  micron::__impl::visit_kv(m, [&](const auto &, const auto &v) {
    if ( !found && v == x ) found = true;
  });
  return found;
}

// find_first: first value satisfying fn(k,v), else empty
template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<M>::key_type &,
                                  const typename micron::remove_cvref_t<M>::mapped_type &>
micron::option<typename micron::remove_cvref_t<M>::mapped_type, empty_container_error>
find_first(const M &m, Fn fn)
{
  using V = typename micron::remove_cvref_t<M>::mapped_type;
  const V *hit = nullptr;
  micron::__impl::visit_kv(m, [&](const auto &k, const auto &v) {
    if ( !hit && fn(k, v) ) hit = micron::addressof(v);
  });
  if ( hit ) return micron::option<V, empty_container_error>{ *hit };
  return micron::option<V, empty_container_error>{ empty_container_error{} };
}

// nub: distinct values, in traversal order -> sequence
template<is_map_class M>
micron::fvector<typename micron::remove_cvref_t<M>::mapped_type>
nub(const M &m)
{
  micron::fvector<typename micron::remove_cvref_t<M>::mapped_type> out;
  micron::__impl::visit_kv(m, [&](const auto &, const auto &v) {
    for ( const auto &e : out )
      if ( e == v ) return;
    out.push_back(v);
  });
  return out;
}

// flat_map: (k,v) -> container, concatenated -> sequence
template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<M>::key_type &,
                                  const typename micron::remove_cvref_t<M>::mapped_type &>
auto
flat_map(const M &m, Fn fn)
{
  using K = typename micron::remove_cvref_t<M>::key_type;
  using V = typename micron::remove_cvref_t<M>::mapped_type;
  using R = micron::remove_cvref_t<decltype(fn(micron::declval<const K &>(), micron::declval<const V &>()))>;
  using E = typename R::value_type;
  micron::fvector<E> out;
  micron::__impl::visit_kv(m, [&](const auto &k, const auto &v) {
    auto sub = fn(k, v);
    for ( const auto &e : sub ) out.push_back(e);
  });
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// option-guarded aggregates over values
template<is_map_class M>
micron::option<typename micron::remove_cvref_t<M>::mapped_type, empty_container_error>
safe_max(const M &m)
{
  using V = typename micron::remove_cvref_t<M>::mapped_type;
  if ( m.size() == 0 ) return micron::option<V, empty_container_error>{ empty_container_error{} };
  return micron::option<V, empty_container_error>{ micron::max(m) };
}

template<is_map_class M>
micron::option<typename micron::remove_cvref_t<M>::mapped_type, empty_container_error>
safe_min(const M &m)
{
  using V = typename micron::remove_cvref_t<M>::mapped_type;
  if ( m.size() == 0 ) return micron::option<V, empty_container_error>{ empty_container_error{} };
  return micron::option<V, empty_container_error>{ micron::min(m) };
}

template<is_map_class M>
  requires micron::is_integral_v<typename micron::remove_cvref_t<M>::mapped_type>
micron::option<umax_t, empty_container_error>
safe_sum(const M &m)
{
  if ( m.size() == 0 ) return micron::option<umax_t, empty_container_error>{ empty_container_error{} };
  return micron::option<umax_t, empty_container_error>{ micron::sum(m) };
}

template<is_map_class M>
  requires micron::is_floating_point_v<typename micron::remove_cvref_t<M>::mapped_type>
micron::option<f128, empty_container_error>
safe_sum(const M &m)
{
  if ( m.size() == 0 ) return micron::option<f128, empty_container_error>{ empty_container_error{} };
  return micron::option<f128, empty_container_error>{ micron::sum(m) };
}

template<typename R = f64, is_map_class M>
micron::option<R, empty_container_error>
safe_mean(const M &m)
{
  if ( m.size() == 0 ) return micron::option<R, empty_container_error>{ empty_container_error{} };
  return micron::option<R, empty_container_error>{ micron::mean<R>(m) };
}

// concat: key-union of two maps
template<is_map_class M>
micron::remove_cvref_t<M>
concat(const M &a, const M &b)
{
  using MM = micron::remove_cvref_t<M>;
  MM out = micron::__impl::rebuild_map<MM>(a, [](const auto &, const auto &) { return true; }, micron::__impl::__kv_value);
  if constexpr ( is_persistent_map<MM> ) {
    micron::__impl::visit_kv(b, [&](const auto &k, const auto &v) { out = out.insert(k, v); });
  } else {
    micron::__impl::visit_kv(b, [&](const auto &k, const auto &v) { out.insert(k, v); });
  }
  return out;
}

// traverse : v -> option<U,E>, short-circuit on first error -> option<map, E>
template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename micron::remove_cvref_t<M>::mapped_type &>
auto
traverse(const M &m, Fn fn)
{
  using MM = micron::remove_cvref_t<M>;
  using V = typename MM::mapped_type;
  using Opt = micron::remove_cvref_t<decltype(fn(micron::declval<const V &>()))>;
  using E = typename Opt::second_type;

  bool ok = true;
  E err{};
  MM out = micron::__impl::make_empty_like<MM>(m.size());
  if constexpr ( is_persistent_map<MM> ) out = MM{};

  micron::__impl::visit_kv(m, [&](const auto &k, const auto &v) {
    if ( !ok ) return;
    Opt r = fn(v);
    if ( r.is_first() ) {
      if constexpr ( is_persistent_map<MM> )
        out = out.insert(k, r.template cast<V>());
      else
        out.insert(k, r.template cast<V>());
    } else {
      ok = false;
      err = r.template cast<E>();
    }
  });

  if ( ok ) return micron::option<MM, E>{ micron::move(out) };
  return micron::option<MM, E>{ micron::move(err) };
}

}      // namespace fp
}      // namespace micron
