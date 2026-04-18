//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../function.hpp"
#include "../sum.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "algorithm.hpp"
#include "data.hpp"
#include "fperrors.hpp"
#include "unroll.hpp"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fp variants of algorithm
// _c denotes curried
namespace micron
{
namespace fp
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fmap  (endomorphism: T -> T)
template <is_iterable_container C, typename Fn>
  requires fn_codomain<Fn, typename C::value_type>
C
fmap(Fn &&fn, C c)
{
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_transform_val(c.begin(), fn, make_index_sequence<C::static_size>{});
  } else {
    micron::transform(c, micron::forward<Fn>(fn));
  }
  return c;
}

template <is_iterable_container C, typename Fn>
  requires fn_arrow<Fn, typename C::value_type *, typename C::value_type> && (!fn_codomain<Fn, typename C::value_type>)
C
fmap(Fn &&fn, C c)
{
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_transform_ptr(c.begin(), fn, make_index_sequence<C::static_size>{});
  } else {
    auto *first = c.begin();
    const auto *last = c.end();
    for ( ; first != last; ++first ) *first = fn(first);
  }
  return c;
}

template <is_iterable_container C, typename Fn>
  requires fn_arrow<Fn, const typename C::value_type *, typename C::value_type>
           && (!fn_arrow<Fn, typename C::value_type *, typename C::value_type>) && (!fn_codomain<Fn, typename C::value_type>)
C
fmap(Fn &&fn, C c)
{
  auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) *first = fn(static_cast<const typename C::value_type *>(first));
  return c;
}

template <is_iterable_container C>
C
fmap(micron::function<typename C::value_type(typename C::value_type)> fn, C c)
{
  auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) *first = fn(*first);
  return c;
}

template <is_iterable_container C>
C
fmap(micron::function<typename C::value_type(typename C::value_type *)> fn, C c)
{
  auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) *first = fn(first);
  return c;
}

template <is_iterable_container C>
C
fmap(micron::function<typename C::value_type(const typename C::value_type &)> fn, C c)
{
  auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) *first = fn(*first);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fmap_into  (heterogeneous: T -> U into different container)
template <is_iterable_container Out, is_iterable_container C, typename Fn>
  requires fn_arrow<Fn, typename C::value_type, typename Out::value_type>
Out
fmap_into(Fn &&fn, const C &c)
{
  Out out;
  out.resize(c.size());
  const auto *first = c.begin();
  const auto *last = c.end();
  auto *dst = out.begin();
  for ( ; first != last; ++first, ++dst ) *dst = fn(*first);
  return out;
}

template <is_iterable_container Out, is_iterable_container C, typename Fn>
  requires fn_arrow<Fn, const typename C::value_type *, typename Out::value_type>
           && (!fn_arrow<Fn, typename C::value_type, typename Out::value_type>)
Out
fmap_into(Fn &&fn, const C &c)
{
  Out out;
  out.resize(c.size());
  const auto *first = c.begin();
  const auto *last = c.end();
  auto *dst = out.begin();
  for ( ; first != last; ++first, ++dst ) *dst = fn(first);
  return out;
}

template <is_iterable_container Out, is_iterable_container C>
Out
fmap_into(micron::function<typename Out::value_type(typename C::value_type)> fn, const C &c)
{
  Out out;
  out.resize(c.size());
  const auto *first = c.begin();
  const auto *last = c.end();
  auto *dst = out.begin();
  for ( ; first != last; ++first, ++dst ) *dst = fn(*first);
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fmap_c (curried fmap)
template <typename Fn>
auto
fmap_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](auto c) mutable { return fp::fmap(fn, micron::move(c)); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scanl  (prefix fold, left)
template <is_iterable_container C, typename R, typename Fn>
  requires fn_fold<Fn, R, typename C::value_type>
C
scanl(const C &c, R init, Fn fn)
{
  C out;
  out.resize(c.size() + 1);
  auto *dst = out.begin();
  *dst++ = static_cast<typename C::value_type>(init);
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) {
    init = fn(micron::move(init), *first);
    *dst++ = static_cast<typename C::value_type>(init);
  }
  return out;
}

template <is_iterable_container C, typename R, typename Fn>
  requires micron::is_invocable_v<Fn, R, const typename C::value_type *>
           && micron::is_same_v<micron::invoke_result_t<Fn, R, const typename C::value_type *>, R>
           && (!fn_fold<Fn, R, typename C::value_type>)
C
scanl(const C &c, R init, Fn fn)
{
  C out;
  out.resize(c.size() + 1);
  auto *dst = out.begin();
  *dst++ = static_cast<typename C::value_type>(init);
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) {
    init = fn(micron::move(init), first);
    *dst++ = static_cast<typename C::value_type>(init);
  }
  return out;
}

template <is_iterable_container C, typename R>
C
scanl(const C &c, R init, micron::function<R(R, typename C::value_type)> fn)
{
  C out;
  out.resize(c.size() + 1);
  auto *dst = out.begin();
  *dst++ = static_cast<typename C::value_type>(init);
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) {
    init = fn(micron::move(init), *first);
    *dst++ = static_cast<typename C::value_type>(init);
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scanr  (prefix fold, right)
template <is_iterable_container C, typename R, typename Fn>
  requires fn_fold_r<Fn, typename C::value_type, R>
C
scanr(const C &c, R init, Fn fn)
{
  C out;
  const usize n = c.size();
  out.resize(n + 1);
  auto *dst = out.begin();
  dst[n] = static_cast<typename C::value_type>(init);
  const auto *first = c.begin();
  for ( usize i = n; i-- > 0; ) {
    init = fn(first[i], micron::move(init));
    dst[i] = static_cast<typename C::value_type>(init);
  }
  return out;
}

template <is_iterable_container C, typename R, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, R>
           && micron::is_same_v<micron::invoke_result_t<Fn, const typename C::value_type *, R>, R>
           && (!fn_fold_r<Fn, typename C::value_type, R>)
C
scanr(const C &c, R init, Fn fn)
{
  C out;
  const usize n = c.size();
  out.resize(n + 1);
  auto *dst = out.begin();
  dst[n] = static_cast<typename C::value_type>(init);
  const auto *first = c.begin();
  for ( usize i = n; i-- > 0; ) {
    init = fn(&first[i], micron::move(init));
    dst[i] = static_cast<typename C::value_type>(init);
  }
  return out;
}

template <is_iterable_container C, typename R>
C
scanr(const C &c, R init, micron::function<R(typename C::value_type, R)> fn)
{
  C out;
  const usize n = c.size();
  out.resize(n + 1);
  auto *dst = out.begin();
  dst[n] = static_cast<typename C::value_type>(init);
  const auto *first = c.begin();
  for ( usize i = n; i-- > 0; ) {
    init = fn(first[i], micron::move(init));
    dst[i] = static_cast<typename C::value_type>(init);
  }
  return out;
}

template <is_iterable_container C, typename R, typename Fn>
C
scan(const C &c, R init, Fn fn)
{
  return fp::scanl(c, micron::move(init), micron::move(fn));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// zip_with
template <is_iterable_container C, typename Fn>
  requires fn_binary_codomain<Fn, typename C::value_type>
micron::option<C, bad_zip_error>
zip_with(const C &a, const C &b, Fn fn)
{
  if ( a.size() != b.size() ) return micron::option<C, bad_zip_error>{ bad_zip_error{} };
  C out;
  out.resize(a.size());
  const auto *fa = a.begin();
  const auto *fb = b.begin();
  auto *dst = out.begin();
  const auto *end = a.end();
  if constexpr ( has_static_size<C> ) {
    __impl::__unroll_transform_bin(fa, fb, dst, fn, make_index_sequence<C::static_size>{});
  } else {
    for ( ; fa != end; ++fa, ++fb, ++dst ) *dst = fn(*fa, *fb);
  }
  return micron::option<C, bad_zip_error>{ micron::move(out) };
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename C::value_type *>
           && micron::is_same_v<micron::invoke_result_t<Fn, const typename C::value_type *, const typename C::value_type *>,
                                typename C::value_type>
           && (!fn_binary_codomain<Fn, typename C::value_type>)
micron::option<C, bad_zip_error>
zip_with(const C &a, const C &b, Fn fn)
{
  if ( a.size() != b.size() ) return micron::option<C, bad_zip_error>{ bad_zip_error{} };
  C out;
  out.resize(a.size());
  const auto *fa = a.begin();
  const auto *fb = b.begin();
  auto *dst = out.begin();
  const auto *end = a.end();
  for ( ; fa != end; ++fa, ++fb, ++dst ) *dst = fn(fa, fb);
  return micron::option<C, bad_zip_error>{ micron::move(out) };
}

template <is_iterable_container C>
micron::option<C, bad_zip_error>
zip_with(const C &a, const C &b, micron::function<typename C::value_type(typename C::value_type, typename C::value_type)> fn)
{
  if ( a.size() != b.size() ) return micron::option<C, bad_zip_error>{ bad_zip_error{} };
  C out;
  out.resize(a.size());
  const auto *fa = a.begin();
  const auto *fb = b.begin();
  auto *dst = out.begin();
  const auto *end = a.end();
  for ( ; fa != end; ++fa, ++fb, ++dst ) *dst = fn(*fa, *fb);
  return micron::option<C, bad_zip_error>{ micron::move(out) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// zip_with_trunc  (truncates to shorter)
template <is_iterable_container C, typename Fn>
  requires fn_binary_codomain<Fn, typename C::value_type>
C
zip_with_trunc(const C &a, const C &b, Fn fn)
{
  const usize n = a.size() < b.size() ? a.size() : b.size();
  C out;
  out.resize(n);
  const auto *fa = a.begin();
  const auto *fb = b.begin();
  auto *dst = out.begin();
  for ( usize i = 0; i < n; ++i, ++fa, ++fb, ++dst ) *dst = fn(*fa, *fb);
  return out;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename C::value_type *>
           && micron::is_same_v<micron::invoke_result_t<Fn, const typename C::value_type *, const typename C::value_type *>,
                                typename C::value_type>
           && (!fn_binary_codomain<Fn, typename C::value_type>)
C
zip_with_trunc(const C &a, const C &b, Fn fn)
{
  const usize n = a.size() < b.size() ? a.size() : b.size();
  C out;
  out.resize(n);
  const auto *fa = a.begin();
  const auto *fb = b.begin();
  auto *dst = out.begin();
  for ( usize i = 0; i < n; ++i, ++fa, ++fb, ++dst ) *dst = fn(fa, fb);
  return out;
}

template <typename Fn>
auto
zip_with_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](const auto &a, const auto &b) mutable { return fp::zip_with(a, b, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// unzip  (C<tuple<A,B>> -> tuple<C<A>, C<B>>)
template <is_iterable_container C>
  requires requires(typename C::value_type p) {
    micron::get<0>(p);
    micron::get<1>(p);
  }
auto
unzip(const C &c)
{
  // using P = typename C::value_type;
  //  using A = micron::remove_cvref_t<decltype(micron::get<0>(micron::declval<P>()))>;
  //  using B = micron::remove_cvref_t<decltype(micron::get<1>(micron::declval<P>()))>;

  C as, bs;
  as.resize(c.size());
  bs.resize(c.size());
  auto *da = as.begin();
  auto *db = bs.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first, ++da, ++db ) {
    *da = static_cast<typename C::value_type>(micron::get<0>(*first));
    *db = static_cast<typename C::value_type>(micron::get<1>(*first));
  }
  return micron::make_tuple(micron::move(as), micron::move(bs));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// traverse  (f each elem, short-circuit on error)
template <is_iterable_container C, typename Fn, typename T = typename C::value_type>
  requires fn_option_arrow<Fn, T>
auto
traverse(const C &c, Fn fn) -> micron::option<C, typename micron::remove_cvref_t<micron::invoke_result_t<Fn, T>>::second_type>
{
  using Opt = micron::invoke_result_t<Fn, T>;
  using E = typename micron::remove_cvref_t<Opt>::second_type;
  C out;
  out.resize(c.size());
  const auto *first = c.begin();
  const auto *last = c.end();
  auto *dst = out.begin();
  for ( ; first != last; ++first, ++dst ) {
    auto result = fn(*first);
    if ( !result.is_first() ) return micron::option<C, E>{ result.template cast<E>() };
    *dst = result.template cast<T>();
  }
  return micron::option<C, E>{ micron::move(out) };
}

template <is_iterable_container C, typename Fn, typename T = typename C::value_type>
  requires fn_option_arrow<Fn, const T *> && (!fn_option_arrow<Fn, T>)
auto
traverse(const C &c, Fn fn) -> micron::option<C, typename micron::remove_cvref_t<micron::invoke_result_t<Fn, const T *>>::second_type>
{
  using Opt = micron::invoke_result_t<Fn, const T *>;
  using E = typename micron::remove_cvref_t<Opt>::second_type;
  C out;
  out.resize(c.size());
  const auto *first = c.begin();
  const auto *last = c.end();
  auto *dst = out.begin();
  for ( ; first != last; ++first, ++dst ) {
    auto result = fn(first);
    if ( !result.is_first() ) return micron::option<C, E>{ result.template cast<E>() };
    *dst = result.template cast<T>();
  }
  return micron::option<C, E>{ micron::move(out) };
}

template <typename Fn>
auto
traverse_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](const auto &c) mutable { return fp::traverse(c, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sequence  (C<option<T,E>> -> option<C_holds_T, E>)
// NOTE: returns option<bool, E> as lightweight check (true = all first)
template <is_iterable_container C>
  requires micron::is_option<typename C::value_type>
bool
sequence_check(const C &c) noexcept
{
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( !first->is_first() ) return false;
  return true;
}

// full sequence: C<option<T,E>> -> option<C, E>
template <is_iterable_container C>
  requires micron::is_option<typename C::value_type>
auto
sequence(const C &c) -> micron::option<C, typename micron::remove_cvref_t<typename C::value_type>::second_type>
{
  using Opt = typename C::value_type;
  using E = typename micron::remove_cvref_t<Opt>::second_type;

  const auto *first = c.begin();
  const auto *last = c.end();
  for ( const auto *it = first; it != last; ++it )
    if ( !it->is_first() ) return micron::option<C, E>{ it->template cast<E>() };
  // all ok — return copy
  return micron::option<C, E>{ c };
}

// sequence_extract: C<option<T,E>> -> option<In, E>
// where In is a container of T (caller must specify In)
template <is_iterable_container In, is_iterable_container C>
  requires micron::is_option<typename C::value_type>
           && micron::is_same_v<typename In::value_type, typename micron::remove_cvref_t<typename C::value_type>::first_type>
auto
sequence_extract(const C &c) -> micron::option<In, typename micron::remove_cvref_t<typename C::value_type>::second_type>
{
  using Opt = typename C::value_type;
  using T = typename micron::remove_cvref_t<Opt>::first_type;
  using E = typename micron::remove_cvref_t<Opt>::second_type;

  const auto *first = c.begin();
  const auto *last = c.end();

  // check for errors
  for ( const auto *it = first; it != last; ++it )
    if ( !it->is_first() ) return micron::option<In, E>{ it->template cast<E>() };

  // extract
  In out;
  out.resize(c.size());
  auto *dst = out.begin();
  for ( const auto *it = first; it != last; ++it, ++dst ) *dst = it->template cast<T>();

  return micron::option<In, E>{ micron::move(out) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// safe aggregates
template <is_iterable_container C>
micron::option<typename C::value_type, empty_container_error>
safe_max(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
  return micron::option<typename C::value_type, empty_container_error>{ micron::max(c) };
}

template <is_iterable_container C>
micron::option<typename C::value_type, empty_container_error>
safe_min(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
  return micron::option<typename C::value_type, empty_container_error>{ micron::min(c) };
}

template <is_iterable_container C>
  requires micron::is_integral_v<typename C::value_type>
micron::option<umax_t, empty_container_error>
safe_sum(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<umax_t, empty_container_error>{ empty_container_error{} };
  return micron::option<umax_t, empty_container_error>{ micron::sum(c) };
}

template <is_iterable_container C>
  requires micron::is_floating_point_v<typename C::value_type>
micron::option<f128, empty_container_error>
safe_sum(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<f128, empty_container_error>{ empty_container_error{} };
  return micron::option<f128, empty_container_error>{ micron::sum(c) };
}

template <typename R = f64, is_iterable_container C>
micron::option<R, empty_container_error>
safe_mean(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<R, empty_container_error>{ empty_container_error{} };
  return micron::option<R, empty_container_error>{ micron::mean<R>(c) };
}

template <typename R = flong, is_iterable_container C>
micron::option<R, empty_container_error>
safe_geomean(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<R, empty_container_error>{ empty_container_error{} };
  return micron::option<R, empty_container_error>{ micron::geomean<R>(c) };
}

template <typename R = flong, is_iterable_container C>
micron::option<R, empty_container_error>
safe_harmonicmean(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<R, empty_container_error>{ empty_container_error{} };
  return micron::option<R, empty_container_error>{ micron::harmonicmean<R>(c) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// curried quantifiers
template <typename Fn>
auto
all_of_c(Fn &&fn) noexcept
{
  return [fn = micron::forward<Fn>(fn)](const auto &c) mutable noexcept { return all_of(c, fn); };
}

template <typename Fn>
auto
any_of_c(Fn &&fn) noexcept
{
  return [fn = micron::forward<Fn>(fn)](const auto &c) mutable noexcept { return any_of(c, fn); };
}

template <typename Fn>
auto
none_of_c(Fn &&fn) noexcept
{
  return [fn = micron::forward<Fn>(fn)](const auto &c) mutable noexcept { return none_of(c, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// replicate
template <is_iterable_container C>
C
replicate(usize n, const typename C::value_type &v)
{
  C out;
  out.resize(n);
  auto *first = out.begin();
  const auto *last = out.end();
  for ( ; first != last; ++first ) *first = v;
  return out;
}

template <is_iterable_container C>
auto
replicate_c(const typename C::value_type &v)
{
  return [v](usize n) { return fp::replicate<C>(n, v); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ap(ply) container of fns to container of vals
template <is_iterable_container CFns, is_iterable_container CVals>
  requires fn_domain<typename CFns::value_type, typename CVals::value_type>
           && fn_codomain<typename CFns::value_type, typename CVals::value_type>
micron::option<CVals, bad_zip_error>
ap(const CFns &fns, const CVals &vals)
{
  if ( fns.size() != vals.size() ) return micron::option<CVals, bad_zip_error>{ bad_zip_error{} };
  CVals out;
  out.resize(vals.size());
  const auto *ff = fns.begin();
  const auto *fv = vals.begin();
  auto *dst = out.begin();
  const auto *end = fns.end();
  for ( ; ff != end; ++ff, ++fv, ++dst ) *dst = (*ff)(*fv);
  return micron::option<CVals, bad_zip_error>{ micron::move(out) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// on combinator  on(f,g)(x,y) = f(g(x), g(y))
template <typename F, typename G>
auto
on(F &&f, G &&g)
{
  return [f = micron::forward<F>(f), g = micron::forward<G>(g)](auto &&x, auto &&y) mutable {
    return f(g(micron::forward<decltype(x)>(x)), g(micron::forward<decltype(y)>(y)));
  };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// curried combinators
template <typename Fn>
auto
transform_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](auto c) mutable {
    micron::transform(c, fn);
    return c;
  };
}

template <typename V>
auto
fill_c(const V &v) noexcept
{
  return [v](auto c) noexcept {
    micron::fill(c, v);
    return c;
  };
}

inline auto
reverse_c()
{
  return [](auto c) noexcept {
    micron::reverse(c);
    return c;
  };
}

inline auto
sort_c()
{
  return [](auto c) noexcept {
    micron::make_heap(c);
    micron::sort_heap(c);
    return c;
  };
}

template <typename Cmp>
auto
sort_by_c(Cmp &&cmp)
{
  return [cmp = micron::forward<Cmp>(cmp)](auto c) mutable noexcept {
    micron::make_heap(c, cmp);
    micron::sort_heap(c, cmp);
    return c;
  };
}

template <typename T>
auto
clamp_each_c(const T lo, const T hi) noexcept
{
  return [lo, hi](auto c) noexcept {
    if constexpr ( has_static_size<decltype(c)> ) {
      __impl::__unroll_clamp(c.begin(), lo, hi, make_index_sequence<decltype(c)::static_size>{});
    } else {
      auto *first = c.begin();
      const auto *last = c.end();
      for ( ; first != last; ++first ) *first = micron::clamp(*first, lo, hi);
    }
    return c;
  };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// option-lifted fmap  (fmap over option<C,E>)
template <is_iterable_container C, typename E, typename Fn>
  requires fn_codomain<Fn, typename C::value_type>
micron::option<C, E>
fmap(Fn &&fn, micron::option<C, E> opt)
{
  if ( !opt.is_first() ) return opt;
  return micron::option<C, E>{ fp::fmap(micron::forward<Fn>(fn), opt.template cast<C>()) };
}

template <is_iterable_container C, typename E, typename Fn>
  requires fn_arrow<Fn, typename C::value_type *, typename C::value_type> && (!fn_codomain<Fn, typename C::value_type>)
micron::option<C, E>
fmap(Fn &&fn, micron::option<C, E> opt)
{
  if ( !opt.is_first() ) return opt;
  return micron::option<C, E>{ fp::fmap(micron::forward<Fn>(fn), opt.template cast<C>()) };
}

template <is_iterable_container C, typename E, typename Fn>
  requires fn_arrow<Fn, const typename C::value_type *, typename C::value_type>
           && (!fn_arrow<Fn, typename C::value_type *, typename C::value_type>) && (!fn_codomain<Fn, typename C::value_type>)
micron::option<C, E>
fmap(Fn &&fn, micron::option<C, E> opt)
{
  if ( !opt.is_first() ) return opt;
  return micron::option<C, E>{ fp::fmap(micron::forward<Fn>(fn), opt.template cast<C>()) };
}

};     // namespace fp
};     // namespace micron
