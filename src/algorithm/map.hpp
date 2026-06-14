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
#include "../types.hpp"
#include "../vector/fvector.hpp"

#include "../bits/__map_build.hpp"
#include "../bits/__visit_kv.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// finds
template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
const typename M::mapped_type *
find_if_not(const M &m, Fn fn) noexcept
{
  const typename M::mapped_type *p = nullptr;
  __impl::visit_kv(m, [&](const auto &k, const auto &v) {
    if ( !p && !fn(k, v) ) p = micron::addressof(v);
  });
  return p;
}

template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
bool
contains_if(const M &m, Fn fn) noexcept
{
  bool r = false;
  __impl::visit_kv(m, [&](const auto &k, const auto &v) {
    if ( !r && fn(k, v) ) r = true;
  });
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// folds
template<is_map_class M, typename Acc, typename Fn>
  requires micron::is_invocable_v<Fn, Acc, const typename M::key_type &, const typename M::mapped_type &>
Acc
fold(const M &m, Acc init, Fn fn) noexcept
{
  __impl::visit_kv(m, [&](const auto &k, const auto &v) { init = fn(micron::move(init), k, v); });
  return init;
}

template<is_map_class M, typename Acc, typename Fn>
  requires micron::is_invocable_v<Fn, Acc, const typename M::mapped_type &>
           && (!micron::is_invocable_v<Fn, Acc, const typename M::key_type &, const typename M::mapped_type &>)
Acc
fold(const M &m, Acc init, Fn fn) noexcept
{
  __impl::visit_kv(m, [&](const auto &, const auto &v) { init = fn(micron::move(init), v); });
  return init;
}

template<is_map_class M, typename Acc, typename Fn>
  requires micron::is_invocable_v<Fn, Acc, const typename M::key_type &, const typename M::mapped_type &>
micron::pair<Acc, usize>
fold_left_counted(const M &m, Acc init, Fn fn) noexcept
{
  usize n = 0;
  __impl::visit_kv(m, [&](const auto &k, const auto &v) {
    init = fn(micron::move(init), k, v);
    ++n;
  });
  return { init, n };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// filters / erases
template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
micron::remove_cvref_t<M>
filter(Fn fn, const M &m)
{
  return __impl::rebuild_map<micron::remove_cvref_t<M>>(m, fn, __impl::__kv_value);
}

template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
micron::remove_cvref_t<M>
reject(Fn fn, const M &m)
{
  return __impl::rebuild_map<micron::remove_cvref_t<M>>(m, [&](const auto &k, const auto &v) { return !fn(k, v); }, __impl::__kv_value);
}

template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
micron::pair<micron::remove_cvref_t<M>, micron::remove_cvref_t<M>>
partition(Fn fn, const M &m)
{
  using MM = micron::remove_cvref_t<M>;
  MM yes = __impl::rebuild_map<MM>(m, [&](const auto &k, const auto &v) { return fn(k, v); }, __impl::__kv_value);
  MM no = __impl::rebuild_map<MM>(m, [&](const auto &k, const auto &v) { return !fn(k, v); }, __impl::__kv_value);
  return micron::pair<MM, MM>(micron::move(yes), micron::move(no));
}

template<is_mutable_map M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
usize
erase_if(M &m, Fn fn)
{
  using K = typename M::key_type;
  micron::fvector<K> doomed;
  __impl::visit_kv(m, [&](const K &k, const auto &v) {
    if ( fn(k, v) ) doomed.push_back(k);
  });
  for ( const K &k : doomed ) m.erase(k);
  return doomed.size();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// extremas
template<is_map_class M>
typename M::mapped_type
max(const M &m) noexcept
{
  typename M::mapped_type best{};
  bool first = true;
  __impl::visit_kv(m, [&](const auto &, const auto &v) {
    if ( first ) {
      best = v;
      first = false;
    } else if ( best < v )
      best = v;
  });
  return best;
}

template<is_map_class M>
typename M::mapped_type
min(const M &m) noexcept
{
  typename M::mapped_type best{};
  bool first = true;
  __impl::visit_kv(m, [&](const auto &, const auto &v) {
    if ( first ) {
      best = v;
      first = false;
    } else if ( v < best )
      best = v;
  });
  return best;
}

// argmax / argmin: the key whose value is the largest / smallest
template<is_map_class M>
typename M::key_type
max_at(const M &m) noexcept
{
  typename M::key_type bestk{};
  typename M::mapped_type bestv{};
  bool first = true;
  __impl::visit_kv(m, [&](const auto &k, const auto &v) {
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

template<is_map_class M>
typename M::key_type
min_at(const M &m) noexcept
{
  typename M::key_type bestk{};
  typename M::mapped_type bestv{};
  bool first = true;
  __impl::visit_kv(m, [&](const auto &k, const auto &v) {
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// numeric aggregates
template<is_map_class M>
  requires micron::is_floating_point_v<typename M::mapped_type>
f128
sum(const M &m) noexcept
{
  f128 s = 0;
  __impl::visit_kv(m, [&](const auto &, const auto &v) { s += static_cast<f128>(v); });
  return s;
}

template<is_map_class M>
  requires micron::is_integral_v<typename M::mapped_type>
umax_t
sum(const M &m) noexcept
{
  umax_t s = 0;
  __impl::visit_kv(m, [&](const auto &, const auto &v) { s += static_cast<umax_t>(v); });
  return s;
}

template<typename R = f64, is_map_class M>
R
mean(const M &m) noexcept
{
  return static_cast<R>(sum(m)) / static_cast<R>(m.size());
}

template<typename R = flong, is_map_class M>
R
geomean(const M &m) noexcept
{
  R prod = 1;
  bool first = true;
  __impl::visit_kv(m, [&](const auto &, const auto &v) {
    if ( first ) {
      prod = static_cast<R>(v);
      first = false;
    } else
      prod *= static_cast<R>(v);
  });
  return math::powerflong(prod, static_cast<R>(R(1) / R(m.size())));
}

template<typename R = flong, is_map_class M>
R
harmonicmean(const M &m) noexcept
{
  R recsum = 0;
  __impl::visit_kv(m, [&](const auto &, const auto &v) { recsum += (R(1) / static_cast<R>(v)); });
  return static_cast<R>(m.size()) / recsum;
}

// %%%%%%%%%%%%%%%%
// fmap
template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::mapped_type &>
           && (!micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>)
micron::remove_cvref_t<M>
fmap(Fn fn, const M &m)
{
  return __impl::rebuild_map<micron::remove_cvref_t<M>>(
      m, [](const auto &, const auto &) { return true; }, [&](const auto &, const auto &v) { return fn(v); });
}

template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
micron::remove_cvref_t<M>
fmap(Fn fn, const M &m)
{
  return __impl::rebuild_map<micron::remove_cvref_t<M>>(
      m, [](const auto &, const auto &) { return true; }, [&](const auto &k, const auto &v) { return fn(k, v); });
}

};      // namespace micron
