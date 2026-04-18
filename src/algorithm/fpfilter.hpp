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
#include "filter.hpp"
#include "fperrors.hpp"

namespace micron
{
namespace fp
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// filter_c
template <typename Fn>
auto
filter_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](auto c) mutable { return micron::filter(c, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reject
template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
C
reject(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( !fn(*first) ) *dst++ = *first;
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
C
reject(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( !fn(first) ) *dst++ = *first;
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type *> && (!fn_predicate<Fn, const typename C::value_type *>)
           && (!fn_predicate<Fn, typename C::value_type>)
C
reject(const C &c, Fn fn)
{
  return micron::filter(c, [fn = micron::move(fn)](typename C::value_type *p) mutable { return !fn(p); });
}

template <is_iterable_container C>
C
reject(const C &c, micron::function<bool(typename C::value_type)> fn)
{
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( !fn(*first) ) *dst++ = *first;
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

template <typename Fn>
auto
reject_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](auto c) mutable { return fp::reject(c, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// option-lifted filter/reject
template <is_iterable_container C, typename E, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::option<C, E>
filter(micron::option<C, E> opt, Fn fn)
{
  if ( !opt.is_first() ) return opt;
  return micron::option<C, E>{ micron::filter(opt.template cast<C>(), micron::move(fn)) };
}

template <is_iterable_container C, typename E, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::option<C, E>
filter(micron::option<C, E> opt, Fn fn)
{
  if ( !opt.is_first() ) return opt;
  return micron::option<C, E>{ micron::filter(opt.template cast<C>(), micron::move(fn)) };
}

template <is_iterable_container C, typename E, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::option<C, E>
reject(micron::option<C, E> opt, Fn fn)
{
  if ( !opt.is_first() ) return opt;
  return micron::option<C, E>{ fp::reject(opt.template cast<C>(), micron::move(fn)) };
}

template <is_iterable_container C, typename E, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::option<C, E>
reject(micron::option<C, E> opt, Fn fn)
{
  if ( !opt.is_first() ) return opt;
  return micron::option<C, E>{ fp::reject(opt.template cast<C>(), micron::move(fn)) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// partition: returns (elements matching fn, elements not matching fn)
template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::tuple<C, C>
partition(const C &c, Fn fn)
{
  C yes, no;
  yes.resize(c.size());
  no.resize(c.size());
  auto *py = yes.begin();
  auto *pn = no.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) {
    if ( fn(*first) )
      *py++ = *first;
    else
      *pn++ = *first;
  }
  yes.resize(static_cast<usize>(py - yes.begin()));
  no.resize(static_cast<usize>(pn - no.begin()));
  return micron::make_tuple(micron::move(yes), micron::move(no));
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::tuple<C, C>
partition(const C &c, Fn fn)
{
  C yes, no;
  yes.resize(c.size());
  no.resize(c.size());
  auto *py = yes.begin();
  auto *pn = no.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) {
    if ( fn(first) )
      *py++ = *first;
    else
      *pn++ = *first;
  }
  yes.resize(static_cast<usize>(py - yes.begin()));
  no.resize(static_cast<usize>(pn - no.begin()));
  return micron::make_tuple(micron::move(yes), micron::move(no));
}

template <is_iterable_container C>
micron::tuple<C, C>
partition(const C &c, micron::function<bool(typename C::value_type)> fn)
{
  C yes, no;
  yes.resize(c.size());
  no.resize(c.size());
  auto *py = yes.begin();
  auto *pn = no.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first ) {
    if ( fn(*first) )
      *py++ = *first;
    else
      *pn++ = *first;
  }
  yes.resize(static_cast<usize>(py - yes.begin()));
  no.resize(static_cast<usize>(pn - no.begin()));
  return micron::make_tuple(micron::move(yes), micron::move(no));
}

template <typename Fn>
auto
partition_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](auto c) mutable { return fp::partition(c, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// take/drop
template <is_iterable_container C>
C
take(const C &c, usize n) noexcept
{
  const usize sz = c.size() < n ? c.size() : n;
  C out;
  out.resize(sz);
  const auto *first = c.begin();
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) *dst++ = first[i];
  return out;
}

template <is_iterable_container C>
C
drop(const C &c, usize n) noexcept
{
  if ( n >= c.size() ) {
    C empty;
    return empty;
  }
  const usize sz = c.size() - n;
  C out;
  out.resize(sz);
  const auto *first = c.begin() + n;
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) *dst++ = first[i];
  return out;
}

auto
take_c(usize n) noexcept
{
  return [n](auto c) noexcept { return fp::take(c, n); };
}

auto
drop_c(usize n) noexcept
{
  return [n](auto c) noexcept { return fp::drop(c, n); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// take_while / drop_while
template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
C
take_while(const C &c, Fn fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *it = first;
  while ( it != last && fn(*it) ) ++it;
  const usize sz = static_cast<usize>(it - first);
  C out;
  out.resize(sz);
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) dst[i] = first[i];
  return out;
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
C
take_while(const C &c, Fn fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *it = first;
  while ( it != last && fn(it) ) ++it;
  const usize sz = static_cast<usize>(it - first);
  C out;
  out.resize(sz);
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) dst[i] = first[i];
  return out;
}

template <is_iterable_container C>
C
take_while(const C &c, micron::function<bool(typename C::value_type)> fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *it = first;
  while ( it != last && fn(*it) ) ++it;
  const usize sz = static_cast<usize>(it - first);
  C out;
  out.resize(sz);
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) dst[i] = first[i];
  return out;
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
C
drop_while(const C &c, Fn fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *it = first;
  while ( it != last && fn(*it) ) ++it;
  const usize sz = static_cast<usize>(last - it);
  C out;
  out.resize(sz);
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) dst[i] = it[i];
  return out;
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
C
drop_while(const C &c, Fn fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *it = first;
  while ( it != last && fn(it) ) ++it;
  const usize sz = static_cast<usize>(last - it);
  C out;
  out.resize(sz);
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) dst[i] = it[i];
  return out;
}

template <is_iterable_container C>
C
drop_while(const C &c, micron::function<bool(typename C::value_type)> fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *it = first;
  while ( it != last && fn(*it) ) ++it;
  const usize sz = static_cast<usize>(last - it);
  C out;
  out.resize(sz);
  auto *dst = out.begin();
  for ( usize i = 0; i < sz; ++i ) dst[i] = it[i];
  return out;
}

template <typename Fn>
auto
take_while_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](auto c) mutable { return fp::take_while(c, fn); };
}

template <typename Fn>
auto
drop_while_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](auto c) mutable { return fp::drop_while(c, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// span / sbreak
template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::tuple<C, C>
span(const C &c, Fn fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *split = first;
  while ( split != last && fn(*split) ) ++split;
  const usize n_yes = static_cast<usize>(split - first);
  const usize n_no = static_cast<usize>(last - split);
  C yes, no;
  yes.resize(n_yes);
  no.resize(n_no);
  for ( usize i = 0; i < n_yes; ++i ) yes.begin()[i] = first[i];
  for ( usize i = 0; i < n_no; ++i ) no.begin()[i] = split[i];
  return micron::make_tuple(micron::move(yes), micron::move(no));
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::tuple<C, C>
span(const C &c, Fn fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *split = first;
  while ( split != last && fn(split) ) ++split;
  const usize n_yes = static_cast<usize>(split - first);
  const usize n_no = static_cast<usize>(last - split);
  C yes, no;
  yes.resize(n_yes);
  no.resize(n_no);
  for ( usize i = 0; i < n_yes; ++i ) yes.begin()[i] = first[i];
  for ( usize i = 0; i < n_no; ++i ) no.begin()[i] = split[i];
  return micron::make_tuple(micron::move(yes), micron::move(no));
}

template <is_iterable_container C>
micron::tuple<C, C>
span(const C &c, micron::function<bool(typename C::value_type)> fn)
{
  const auto *first = c.begin();
  const auto *last = c.end();
  const auto *split = first;
  while ( split != last && fn(*split) ) ++split;
  const usize n_yes = static_cast<usize>(split - first);
  const usize n_no = static_cast<usize>(last - split);
  C yes, no;
  yes.resize(n_yes);
  no.resize(n_no);
  for ( usize i = 0; i < n_yes; ++i ) yes.begin()[i] = first[i];
  for ( usize i = 0; i < n_no; ++i ) no.begin()[i] = split[i];
  return micron::make_tuple(micron::move(yes), micron::move(no));
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
micron::tuple<C, C>
sbreak(const C &c, Fn fn)
{
  return fp::span(c, [fn = micron::move(fn)](typename C::value_type v) mutable { return !fn(v); });
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
micron::tuple<C, C>
sbreak(const C &c, Fn fn)
{
  return fp::span(c, [fn = micron::move(fn)](const typename C::value_type *p) mutable { return !fn(p); });
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// unique  (remove consecutive duplicates)
template <is_iterable_container C>
C
unique(const C &c)
{
  if ( c.size() == 0 ) return c;
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  *dst++ = *first++;
  for ( ; first != last; ++first )
    if ( !(*first == *(dst - 1)) ) *dst++ = *first;
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

template <is_iterable_container C, typename EqFn>
  requires fn_binary_predicate<EqFn, typename C::value_type>
C
unique(const C &c, EqFn eq)
{
  if ( c.size() == 0 ) return c;
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  *dst++ = *first++;
  for ( ; first != last; ++first )
    if ( !eq(*first, *(dst - 1)) ) *dst++ = *first;
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

template <is_iterable_container C, typename EqFn>
  requires micron::invocable<EqFn, const typename C::value_type *, const typename C::value_type *>
           && micron::is_convertible_v<micron::invoke_result_t<EqFn, const typename C::value_type *, const typename C::value_type *>, bool>
           && (!fn_binary_predicate<EqFn, typename C::value_type>)
C
unique(const C &c, EqFn eq)
{
  if ( c.size() == 0 ) return c;
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  *dst++ = *first++;
  for ( ; first != last; ++first )
    if ( !eq(first, dst - 1) ) *dst++ = *first;
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

// micron::function overload
template <is_iterable_container C>
C
unique(const C &c, micron::function<bool(typename C::value_type, typename C::value_type)> eq)
{
  if ( c.size() == 0 ) return c;
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  *dst++ = *first++;
  for ( ; first != last; ++first )
    if ( !eq(*first, *(dst - 1)) ) *dst++ = *first;
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// nub
template <is_iterable_container C>
C
nub(const C &c)
{
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( const auto *it = first; it != last; ++it ) {
    bool found = false;
    for ( const auto *seen = first; seen != it; ++seen ) {
      if ( *seen == *it ) {
        found = true;
        break;
      }
    }
    if ( !found ) *dst++ = *it;
  }
  out.resize(static_cast<usize>(dst - out.begin()));
  return out;
}

template <is_iterable_container C, typename EqFn>
  requires fn_binary_predicate<EqFn, typename C::value_type>
C
nub_by(const C &c, EqFn eq)
{
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  usize out_count = 0;
  for ( const auto *it = first; it != last; ++it ) {
    bool found = false;
    const auto *ob = out.begin();
    for ( usize k = 0; k < out_count; ++k ) {
      if ( eq(ob[k], *it) ) {
        found = true;
        break;
      }
    }
    if ( !found ) {
      *dst++ = *it;
      ++out_count;
    }
  }
  out.resize(out_count);
  return out;
}

template <is_iterable_container C>
C
nub_by(const C &c, micron::function<bool(typename C::value_type, typename C::value_type)> eq)
{
  C out;
  out.resize(c.size());
  auto *dst = out.begin();
  const auto *first = c.begin();
  const auto *last = c.end();
  usize out_count = 0;
  for ( const auto *it = first; it != last; ++it ) {
    bool found = false;
    const auto *ob = out.begin();
    for ( usize k = 0; k < out_count; ++k ) {
      if ( eq(ob[k], *it) ) {
        found = true;
        break;
      }
    }
    if ( !found ) {
      *dst++ = *it;
      ++out_count;
    }
  }
  out.resize(out_count);
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// count
template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, typename C::value_type>
usize
count(const C &c, Fn fn) noexcept
{
  usize n = 0;
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( fn(*first) ) ++n;
  return n;
}

template <is_iterable_container C, typename Fn>
  requires fn_predicate<Fn, const typename C::value_type *> && (!fn_predicate<Fn, typename C::value_type>)
usize
count(const C &c, Fn fn) noexcept
{
  usize n = 0;
  const auto *first = c.begin();
  const auto *last = c.end();
  for ( ; first != last; ++first )
    if ( fn(first) ) ++n;
  return n;
}

};     // namespace fp
};     // namespace micron
