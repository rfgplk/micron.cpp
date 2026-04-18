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
#include "data.hpp"
#include "fperrors.hpp"

namespace micron
{
namespace fp
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// flatten  (C<C<T>> -> C<T>)
template <is_iterable_container O>
  requires is_iterable_container<typename O::value_type>
typename O::value_type
flatten(const O &outer)
{
  using Inner = typename O::value_type;
  usize total = 0;
  const auto *ofirst = outer.begin();
  const auto *olast = outer.end();
  for ( const auto *it = ofirst; it != olast; ++it ) total += it->size();
  Inner out;
  out.resize(total);
  auto *dst = out.begin();
  for ( const auto *it = ofirst; it != olast; ++it ) {
    const auto *ifirst = it->begin();
    const auto *ilast = it->end();
    for ( ; ifirst != ilast; ++ifirst ) *dst++ = *ifirst;
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// flat_map/concat_map  (fmap then flatten)
template <is_iterable_container C, typename Fn>
  requires fn_domain<Fn, typename C::value_type> && is_iterable_container<micron::invoke_result_t<Fn, typename C::value_type>>
C
flat_map(const C &c, Fn fn)
{
  C out;
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( const auto *it = first; it != last; ++it ) {
    auto inner = fn(*it);
    const auto *ifirst = inner.begin();
    const auto *ilast = inner.end();
    for ( ; ifirst != ilast; ++ifirst ) out.push_back(*ifirst);
  }
  return out;
}

template <is_iterable_container C, typename Fn>
  requires fn_domain<Fn, const typename C::value_type *>
           && is_iterable_container<micron::invoke_result_t<Fn, const typename C::value_type *>> && (!fn_domain<Fn, typename C::value_type>)
C
flat_map(const C &c, Fn fn)
{
  C out;
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( const auto *it = first; it != last; ++it ) {
    auto inner = fn(it);
    const auto *ifirst = inner.begin();
    const auto *ilast = inner.end();
    for ( ; ifirst != ilast; ++ifirst ) out.push_back(*ifirst);
  }
  return out;
}

// micron::function overload
template <is_iterable_container C>
C
flat_map(const C &c, micron::function<C(typename C::value_type)> fn)
{
  C out;
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( const auto *it = first; it != last; ++it ) {
    auto inner = fn(*it);
    const auto *ifirst = inner.begin();
    const auto *ilast = inner.end();
    for ( ; ifirst != ilast; ++ifirst ) out.push_back(*ifirst);
  }
  return out;
}

template <typename Fn>
auto
flat_map_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](const auto &c) mutable { return fp::flat_map(c, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// chunk  (split container into consecutive chunks of size n)
template <is_iterable_container C>
micron::option<C, empty_container_error>
chunk(const C &c, usize n)
{
  if ( n == 0 ) return micron::option<C, empty_container_error>{ empty_container_error{} };
  return micron::option<C, empty_container_error>{ c };
}

template <is_iterable_container Inner, is_iterable_container C>
  requires micron::is_same_v<typename Inner::value_type, typename C::value_type>
C
chunk_into(const C &c, usize n)
{
  C out;
  if ( n == 0 || c.size() == 0 ) return out;
  const usize total = c.size();
  const auto *first = c.begin();
  usize i = 0;
  while ( i < total ) {
    Inner chunk_val;
    usize end = (i + n < total) ? i + n : total;
    chunk_val.resize(end - i);
    for ( usize k = 0; k < (end - i); ++k ) chunk_val.begin()[k] = first[i + k];
    out.push_back(micron::move(chunk_val));
    i = end;
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sliding  (sliding window of size n, step 1)
template <is_iterable_container Inner, is_iterable_container C>
  requires micron::is_same_v<typename Inner::value_type, typename C::value_type>
C
sliding(const C &c, usize n)
{
  C out;
  const usize total = c.size();
  if ( n == 0 || total < n ) return out;
  const auto *first = c.begin();
  const usize windows = total - n + 1;
  for ( usize i = 0; i < windows; ++i ) {
    Inner w;
    w.resize(n);
    for ( usize k = 0; k < n; ++k ) w.begin()[k] = first[i + k];
    out.push_back(micron::move(w));
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// intersperse  ([a, b, c] -> [a, sep, b, sep, c])
template <is_iterable_container C>
C
intersperse(const C &c, const typename C::value_type &sep)
{
  if ( c.size() < 2 ) return c;
  C out;
  out.resize(2 * c.size() - 1);
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  *dst++ = *first++;
  for ( ; first != last; ++first ) {
    *dst++ = sep;
    *dst++ = *first;
  }
  return out;
}

auto
intersperse_c(auto sep)
{
  return [sep = micron::move(sep)](auto c) { return fp::intersperse(c, sep); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// intercalate  (flatten(intersperse(outer, sep)))
template <is_iterable_container O>
  requires is_iterable_container<typename O::value_type>
typename O::value_type
intercalate(const typename O::value_type &sep, const O &outer)
{
  using Inner = typename O::value_type;
  if ( outer.size() == 0 ) return Inner{};

  Inner out;
  const auto *ofirst = outer.begin();
  const auto *olast = outer.end();
  bool first_chunk = true;
  for ( const auto *oit = ofirst; oit != olast; ++oit ) {
    if ( !first_chunk ) {
      const auto *sfirst = sep.begin();
      const auto *slast = sep.end();
      for ( ; sfirst != slast; ++sfirst ) out.push_back(*sfirst);
    }
    const auto *ifirst = oit->begin();
    const auto *ilast = oit->end();
    for ( ; ifirst != ilast; ++ifirst ) out.push_back(*ifirst);
    first_chunk = false;
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group_by  (split into maximal runs where adjacent elements match)
template <is_iterable_container Inner, is_iterable_container C, typename EqFn>
  requires fn_binary_predicate<EqFn, typename C::value_type> && micron::is_same_v<typename Inner::value_type, typename C::value_type>
C
group_by(const C &c, EqFn eq)
{
  C out;
  if ( c.size() == 0 ) return out;
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *run_start = first++;
  Inner current;
  current.push_back(*run_start);

  for ( ; first != last; ++first ) {
    if ( eq(*(first - 1), *first) ) {
      current.push_back(*first);
    } else {
      out.push_back(micron::move(current));
      current = Inner{};
      current.push_back(*first);
    }
  }
  out.push_back(micron::move(current));
  return out;
}

template <is_iterable_container Inner, is_iterable_container C, typename EqFn>
  requires micron::invocable<EqFn, const typename C::value_type *, const typename C::value_type *>
           && micron::is_convertible_v<micron::invoke_result_t<EqFn, const typename C::value_type *, const typename C::value_type *>, bool>
           && (!fn_binary_predicate<EqFn, typename C::value_type>) && micron::is_same_v<typename Inner::value_type, typename C::value_type>
C
group_by(const C &c, EqFn eq)
{
  C out;
  if ( c.size() == 0 ) return out;
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *run_start = first++;
  Inner current;
  current.push_back(*run_start);

  for ( ; first != last; ++first ) {
    if ( eq(first - 1, first) ) {
      current.push_back(*first);
    } else {
      out.push_back(micron::move(current));
      current = Inner{};
      current.push_back(*first);
    }
  }
  out.push_back(micron::move(current));
  return out;
}

template <is_iterable_container Inner, is_iterable_container C>
  requires micron::is_same_v<typename Inner::value_type, typename C::value_type>
C
group_by(const C &c, micron::function<bool(typename C::value_type, typename C::value_type)> eq)
{
  C out;
  if ( c.size() == 0 ) return out;
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *run_start = first++;
  Inner current;
  current.push_back(*run_start);

  for ( ; first != last; ++first ) {
    if ( eq(*(first - 1), *first) ) {
      current.push_back(*first);
    } else {
      out.push_back(micron::move(current));
      current = Inner{};
      current.push_back(*first);
    }
  }
  out.push_back(micron::move(current));
  return out;
}

template <is_iterable_container Inner, is_iterable_container C>
  requires micron::is_same_v<typename Inner::value_type, typename C::value_type>
C
group(const C &c)
{
  return group_by<Inner>(c, [](const typename C::value_type &a, const typename C::value_type &b) { return a == b; });
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// transpose  (swap rows and columns of C<C<T>>)
template <is_iterable_container O>
  requires is_iterable_container<typename O::value_type>
micron::option<O, bad_zip_error>
transpose(const O &mat)
{
  using Inner = typename O::value_type;
  const usize rows = mat.size();
  if ( rows == 0 ) return micron::option<O, bad_zip_error>{ O{} };
  const usize cols = mat.begin()->size();
  for ( const auto *row = mat.begin(); row != mat.end(); ++row )
    if ( row->size() != cols ) return micron::option<O, bad_zip_error>{ bad_zip_error{} };

  O out;
  for ( usize c = 0; c < cols; ++c ) {
    Inner row;
    row.resize(rows);
    for ( usize r = 0; r < rows; ++r ) row.begin()[r] = mat.begin()[r].begin()[c];
    out.push_back(micron::move(row));
  }
  return micron::option<O, bad_zip_error>{ micron::move(out) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// merge/concat
template <is_iterable_container C>
auto
merge_c(const C &b)
{
  return [&b](C a) { return micron::merge(micron::move(a), b); };
}

template <is_iterable_container C>
auto
concat_c(const C &b)
{
  return [&b](C a) { return micron::concat(micron::move(a), b); };
}

template <is_iterable_container C, typename E>
micron::option<C, E>
merge(micron::option<C, E> opt_a, const C &b)
{
  if ( !opt_a.is_first() ) return opt_a;
  return micron::option<C, E>{ micron::merge(opt_a.template cast<C>(), b) };
}

template <is_iterable_container C, typename E>
micron::option<C, E>
merge(micron::option<C, E> opt_a, micron::option<C, E> opt_b)
{
  if ( !opt_a.is_first() ) return opt_a;
  if ( !opt_b.is_first() ) return opt_b;
  return micron::option<C, E>{ micron::merge(opt_a.template cast<C>(), opt_b.template cast<C>()) };
}

template <is_iterable_container C, typename E>
micron::option<C, E>
concat(micron::option<C, E> opt_a, const C &b)
{
  if ( !opt_a.is_first() ) return opt_a;
  return micron::option<C, E>{ micron::concat(opt_a.template cast<C>(), b) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// safe element access
template <is_iterable_container C>
micron::option<typename C::value_type, index_out_of_bounds_error>
at(const C &c, usize i) noexcept
{
  if ( i >= c.size() ) return micron::option<typename C::value_type, index_out_of_bounds_error>{ index_out_of_bounds_error{} };
  return micron::option<typename C::value_type, index_out_of_bounds_error>{ c.begin()[i] };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// find_first / find_last
template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::option<typename C::value_type, empty_container_error>
find_first(const C &c, Fn fn) noexcept
{
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( fn(*first) ) return micron::option<typename C::value_type, empty_container_error>{ *first };
  return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::option<typename C::value_type, empty_container_error>
find_first(const C &c, Fn fn) noexcept
{
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( fn(first) ) return micron::option<typename C::value_type, empty_container_error>{ *first };
  return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
}

template <is_iterable_container C>
micron::option<typename C::value_type, empty_container_error>
find_first(const C &c, micron::function<bool(typename C::value_type)> fn) noexcept
{
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( fn(*first) ) return micron::option<typename C::value_type, empty_container_error>{ *first };
  return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::option<typename C::value_type, empty_container_error>
find_last(const C &c, Fn fn) noexcept
{
  if ( c.size() == 0 ) return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
  const auto *first = c.begin();
  const auto *it = c.end();
  while ( it != first ) {
    --it;
    if ( fn(*it) ) return micron::option<typename C::value_type, empty_container_error>{ *it };
  }
  return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::option<typename C::value_type, empty_container_error>
find_last(const C &c, Fn fn) noexcept
{
  if ( c.size() == 0 ) return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
  const auto *first = c.begin();
  const auto *it = c.end();
  while ( it != first ) {
    --it;
    if ( fn(it) ) return micron::option<typename C::value_type, empty_container_error>{ *it };
  }
  return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
}

template <is_iterable_container C>
micron::option<typename C::value_type, empty_container_error>
find_last(const C &c, micron::function<bool(typename C::value_type)> fn) noexcept
{
  if ( c.size() == 0 ) return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
  const auto *first = c.begin();
  const auto *it = c.end();
  while ( it != first ) {
    --it;
    if ( fn(*it) ) return micron::option<typename C::value_type, empty_container_error>{ *it };
  }
  return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// find_index
template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::option<usize, empty_container_error>
find_index(const C &c, Fn fn) noexcept
{
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( usize i = 0; first != last; ++first, ++i )
    if ( fn(*first) ) return micron::option<usize, empty_container_error>{ i };
  return micron::option<usize, empty_container_error>{ empty_container_error{} };
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::option<usize, empty_container_error>
find_index(const C &c, Fn fn) noexcept
{
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( usize i = 0; first != last; ++first, ++i )
    if ( fn(first) ) return micron::option<usize, empty_container_error>{ i };
  return micron::option<usize, empty_container_error>{ empty_container_error{} };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// head/last/tail/init
template <is_iterable_container C>
micron::option<typename C::value_type, empty_container_error>
head(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
  return micron::option<typename C::value_type, empty_container_error>{ *c.begin() };
}

template <is_iterable_container C>
micron::option<typename C::value_type, empty_container_error>
last(const C &c) noexcept
{
  if ( c.size() == 0 ) return micron::option<typename C::value_type, empty_container_error>{ empty_container_error{} };
  return micron::option<typename C::value_type, empty_container_error>{ *(c.end() - 1) };
}

template <is_iterable_container C>
micron::option<C, empty_container_error>
tail(const C &c)
{
  if ( c.size() == 0 ) return micron::option<C, empty_container_error>{ empty_container_error{} };
  C out;
  out.resize(c.size() - 1);
  const auto *first = c.begin() + 1;
  const auto *last_ptr = c.end();
  auto *dst = out.begin();
  for ( ; first != last_ptr; ++first ) *dst++ = *first;
  return micron::option<C, empty_container_error>{ micron::move(out) };
}

template <is_iterable_container C>
micron::option<C, empty_container_error>
init(const C &c)
{
  if ( c.size() == 0 ) return micron::option<C, empty_container_error>{ empty_container_error{} };
  C out;
  out.resize(c.size() - 1);
  const auto *first = c.begin();
  auto *dst = out.begin();
  for ( usize i = 0; i < c.size() - 1; ++i ) dst[i] = first[i];
  return micron::option<C, empty_container_error>{ micron::move(out) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// snoc: append single element
template <is_iterable_container C>
micron::option<C, empty_container_error>
snoc(const C &c, const typename C::value_type &v)
{
  C out;
  out.resize(c.size() + 1);
  const auto *first = c.begin();
  auto *dst = out.begin();
  for ( usize i = 0; i < c.size(); ++i ) dst[i] = first[i];
  dst[c.size()] = v;
  return micron::option<C, empty_container_error>{ micron::move(out) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// uncons
template <is_iterable_container C>
micron::option<micron::tuple<typename C::value_type, C>, empty_container_error>
uncons(const C &c)
{
  using T = typename C::value_type;
  using R = micron::tuple<T, C>;
  if ( c.size() == 0 ) return micron::option<R, empty_container_error>{ empty_container_error{} };
  auto tl = fp::tail(c);
  return micron::option<R, empty_container_error>{ micron::make_tuple(*c.begin(), tl.template cast<C>()) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// elem
template <is_iterable_container C>
bool
elem(const C &c, const typename C::value_type &v) noexcept
{
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( *first == v ) return true;
  return false;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// enumerate
template <is_iterable_container Out, is_iterable_container C>
Out
enumerate(const C &c)
{
  Out out;
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( usize i = 0; first != last; ++first, ++i ) out.push_back(micron::make_tuple(i, *first));
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// iterate: produce container of n applications
// iterate(n, f, x) = [x, f(x), f(f(x)), ...]
template <is_iterable_container C, typename Fn>
  requires fn_codomain<Fn, typename C::value_type>
C
iterate(usize n, Fn fn, typename C::value_type seed)
{
  C out;
  out.resize(n);
  auto *dst = out.begin();
  for ( usize i = 0; i < n; ++i ) {
    dst[i] = seed;
    seed = fn(seed);
  }
  return out;
}

template <is_iterable_container C, typename Fn>
  requires fn_arrow<Fn, const typename C::value_type *, typename C::value_type> && (!fn_codomain<Fn, typename C::value_type>)
C
iterate(usize n, Fn fn, typename C::value_type seed)
{
  C out;
  out.resize(n);
  auto *dst = out.begin();
  for ( usize i = 0; i < n; ++i ) {
    dst[i] = seed;
    seed = fn(&seed);
  }
  return out;
}

template <is_iterable_container C>
C
iterate(usize n, micron::function<typename C::value_type(typename C::value_type)> fn, typename C::value_type seed)
{
  C out;
  out.resize(n);
  auto *dst = out.begin();
  for ( usize i = 0; i < n; ++i ) {
    dst[i] = seed;
    seed = fn(seed);
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// unfold: (B -> option<tuple<A,B>, E>) -> B -> C<A>
// generate elements until the function returns error
template <is_iterable_container C, typename Fn, typename B>
  requires fn_domain<Fn, B> && micron::is_option<micron::invoke_result_t<Fn, B>>
C
unfold(Fn fn, B seed)
{
  C out;
  for ( ;; ) {
    auto result = fn(seed);
    if ( !result.is_first() ) break;
    auto tup = result.template cast<typename micron::remove_cvref_t<micron::invoke_result_t<Fn, B>>::first_type>();
    out.push_back(micron::get<0>(tup));
    seed = micron::get<1>(tup);
  }
  return out;
}

};     // namespace fp
};     // namespace micron
