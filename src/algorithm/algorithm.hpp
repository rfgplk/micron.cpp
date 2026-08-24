//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"

#include "../concepts.hpp"
#include "../math/generic.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "find.hpp"
#include "unroll.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// clamps

template<typename T>
constexpr const T &
clamp(const T &v, const T &lo, const T &hi) noexcept
{
  return (v < lo) ? lo : (hi < v) ? hi : v;
}

template<typename T, typename C>
constexpr const T &
clamp(const T &v, const T &lo, const T &hi, C comp) noexcept
{
  return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

namespace __impl
{
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// when a container may be read as a flat array
template<typename C>
concept __flat_readable = micron::is_contiguous_container<C> && requires(const C &c) {
  { c.begin() } -> micron::convertible_to<const typename C::value_type *>;
};
};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fills

template<auto Fn, typename T>
constexpr void
fill(T *first, T *end) noexcept
{
  for ( ; first != end; ++first ) *first = Fn();
}

template<auto Fn, typename T>
constexpr T *
fill_n(T *first, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i, ++first ) *first = Fn();
  return first;
}

template<auto Fn, is_iterable_container C>
constexpr C &
fill(C &c) noexcept
{
  fill<Fn>(c.begin(), c.end());
  return c;
}

template<auto Fn, is_iterable_container C>
constexpr C &
fill_n(C &c, usize n) noexcept
{
  fill_n<Fn>(c.begin(), n);
  return c;
}

template<typename T, class P>
constexpr void
fill(T *first, T *end, const P &value) noexcept
{
  const usize n = static_cast<usize>(end - first);
  // return early on a null test, avoids UB on nonnull
  if ( n == 0 ) return;
  if constexpr ( !micron::is_class_v<T> ) {
    if ( !__builtin_is_constant_evaluated() ) {
      micron::typeset(first, static_cast<T>(value), n);
      return;
    }
  }
  for ( usize i = 0; i < n; ++i ) first[i] = value;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<T>;
  }
constexpr void
fill(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first ) *first = fn();
}

template<typename T, class P>
constexpr T *
fill_n(T *first, usize n, const P &value) noexcept
{
  for ( usize i = 0; i < n; ++i, ++first ) *first = value;
  return first;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<T>;
  }
constexpr T *
fill_n(T *first, usize n, Fn fn) noexcept
{
  for ( usize i = 0; i < n; ++i, ++first ) *first = fn();
  return first;
}

template<is_iterable_container C, class P>
constexpr C &
fill(C &c, const P &value) noexcept
{
  if constexpr ( __impl::__flat_readable<C> ) {
    fill(c.begin(), c.end(), value);
  } else {
    // no pointer to hand the broadcast, and for a ring no flat range to broadcast over either
    for ( auto &e : c ) e = value;
  }
  return c;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<typename C::value_type>;
  }
constexpr C &
fill(C &c, Fn fn) noexcept
{
  fill(c.begin(), c.end(), fn);
  return c;
}

template<is_iterable_container C, class P>
constexpr C &
fill_n(C &c, usize n, const P &value) noexcept
{
  fill_n(c.begin(), n, value);
  return c;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<typename C::value_type>;
  }
constexpr C &
fill_n(C &c, usize n, Fn fn) noexcept
{
  fill_n(c.begin(), n, fn);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// generates

template<auto Fn, typename T>
constexpr void
generate(T *first, T *end) noexcept
{
  for ( ; first != end; ++first ) *first = Fn();
}

template<auto Fn, typename T, typename... Args>
constexpr void
generate(T *first, T *end, Args &&...args) noexcept
{
  for ( ; first != end; ++first ) *first = Fn(micron::forward<Args>(args)...);
}

template<auto Fn, is_iterable_container C>
constexpr C &
generate(C &c) noexcept
{
  generate<Fn>(c.begin(), c.end());
  return c;
}

template<auto Fn, is_iterable_container C, typename... Args>
constexpr C &
generate(C &c, Args &&...args) noexcept
{
  generate<Fn>(c.begin(), c.end(), micron::forward<Args>(args)...);
  return c;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<T>;
  }
constexpr void
generate(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first ) *first = fn();
}

template<typename T, typename Fn, typename... Args>
  requires micron::invocable<Fn, Args...>
constexpr void
generate(T *first, T *end, Fn fn, Args &&...args) noexcept
{
  for ( ; first != end; ++first ) *first = fn(micron::forward<Args>(args)...);
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<typename C::value_type>;
  }
constexpr C &
generate(C &c, Fn fn) noexcept
{
  generate(c.begin(), c.end(), fn);
  return c;
}

template<is_iterable_container C, typename Fn, typename... Args>
  requires micron::invocable<Fn, Args...>
constexpr C &
generate(C &c, Fn fn, Args &&...args) noexcept
{
  generate(c.begin(), c.end(), fn, micron::forward<Args>(args)...);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// transforms

template<auto Fn, typename T>
constexpr void
transform(T *first, T *end) noexcept
{
  if constexpr ( micron::invocable<decltype(Fn), T *> ) {
    for ( ; first != end; ++first ) *first = Fn(first);
  } else {
    for ( ; first != end; ++first ) *first = Fn(*first);
  }
}

template<auto Fn, typename T, typename O>
constexpr O *
transform(const T *first1, const T *end1, const T *first2, O *out) noexcept
{
  if constexpr ( micron::invocable<decltype(Fn), const T *, const T *> ) {
    for ( ; first1 != end1; ++first1, ++first2, ++out ) *out = Fn(first1, first2);
  } else {
    for ( ; first1 != end1; ++first1, ++first2, ++out ) *out = Fn(*first1, *first2);
  }
  return out;
}

template<auto Fn, is_iterable_container C>
constexpr C &
transform(C &c) noexcept
{
  transform<Fn>(c.begin(), c.end());
  return c;
}

template<auto Fn, is_iterable_container C, is_iterable_container O>
  requires micron::is_same_v<typename C::value_type, typename O::value_type>
constexpr O &
transform(const C &a, const C &b, O &out) noexcept
{
  transform<Fn>(a.begin(), a.end(), b.begin(), out.begin());
  return out;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, T *>
constexpr void
transform(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first ) *first = fn(first);
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, T>
constexpr void
transform(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first ) *first = fn(*first);
}

template<typename T, typename O, typename Fn>
  requires micron::invocable<Fn, T, T>
constexpr O *
transform(const T *first1, const T *end1, const T *first2, O *out, Fn fn) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2, ++out ) *out = fn(*first1, *first2);
  return out;
}

template<typename T, typename O, typename Fn>
  requires micron::invocable<Fn, const T *, const T *>
constexpr O *
transform(const T *first1, const T *end1, const T *first2, O *out, Fn fn) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2, ++out ) *out = fn(first1, first2);
  return out;
}

// see micron::unrollable
template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, typename C::value_type *>
constexpr C &
transform(C &c, Fn fn) noexcept
{
  if constexpr ( micron::unrollable<C> ) {
    __impl::__unroll_transform_ptr(c.begin(), fn, make_index_sequence<C::static_size>{});
  } else {
    transform(c.begin(), c.end(), fn);
  }
  return c;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, typename C::value_type>
constexpr C &
transform(C &c, Fn fn) noexcept
{
  if constexpr ( micron::unrollable<C> ) {
    __impl::__unroll_transform_val(c.begin(), fn, make_index_sequence<C::static_size>{});
  } else {
    transform(c.begin(), c.end(), fn);
  }
  return c;
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::is_same_v<typename C::value_type, typename C::value_type>
           && micron::invocable<Fn, typename C::value_type, typename C::value_type>
constexpr O &
transform(const C &a, const C &b, O &out, Fn fn) noexcept
{
  transform(a.begin(), a.end(), b.begin(), out.begin(), fn);
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wheres

template<auto Fn, typename T>
constexpr T *
where(const T *first, const T *end, T *out) noexcept
{
  if constexpr ( micron::invocable<decltype(Fn), const T *> ) {
    for ( ; first != end; ++first )
      if ( Fn(first) ) *out++ = *first;
  } else {
    for ( ; first != end; ++first )
      if ( Fn(*first) ) *out++ = *first;
  }
  return out;
}

template<auto Fn, is_iterable_container C>
C
where(const C &c)
{
  C out;
  out.resize(c.size());
  auto *last = where<Fn>(c.begin(), c.end(), out.begin());
  out.resize(last - out.begin());
  return out;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, T>
constexpr T *
where(const T *first, const T *end, T *out, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(*first) ) *out++ = *first;
  return out;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, const T *>
constexpr T *
where(const T *first, const T *end, T *out, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(first) ) *out++ = *first;
  return out;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, typename C::value_type>
C
where(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = where(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, const typename C::value_type *>
C
where(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = where(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shifts/rotates
template<typename T>
constexpr T *
shift_left(T *first, T *end, usize n) noexcept
{
  if ( n == 0 || first == end ) return end;
  const usize len = static_cast<usize>(end - first);
  if ( n >= len ) {
    for ( T *p = first; p != end; ++p ) *p = T{};
    return first;
  }
  for ( usize i = 0; i + n < len; ++i ) first[i] = micron::move(first[i + n]);
  for ( usize i = len - n; i < len; ++i ) first[i] = T{};
  return first + (len - n);
}

template<typename T>
constexpr T *
shift_right(T *first, T *end, usize n) noexcept
{
  if ( n == 0 || first == end ) return first;
  const usize len = static_cast<usize>(end - first);
  if ( n >= len ) {
    for ( T *p = first; p != end; ++p ) *p = T{};
    return end;
  }
  for ( usize i = len; i-- > n; ) first[i] = micron::move(first[i - n]);
  for ( usize i = 0; i < n; ++i ) first[i] = T{};
  return first + n;
}

template<is_iterable_container C>
constexpr C &
shift_left(C &c, usize n) noexcept
{
  shift_left(c.begin(), c.end(), n);
  return c;
}

template<is_iterable_container C>
constexpr C &
shift_right(C &c, usize n) noexcept
{
  shift_right(c.begin(), c.end(), n);
  return c;
}

template<typename T>
constexpr void
rotate_left(T *first, T *end, usize n) noexcept
{
  const usize len = static_cast<usize>(end - first);
  if ( len == 0 ) return;
  n %= len;
  if ( n == 0 ) return;
  auto rev = [](T *a, T *b) {
    while ( a < b ) {
      auto tmp = *a;
      *a = *b;
      *b = tmp;
      ++a;
      --b;
    }
  };
  rev(first, first + n - 1);
  rev(first + n, end - 1);
  rev(first, end - 1);
}

template<typename T>
constexpr void
rotate_right(T *first, T *end, usize n) noexcept
{
  const usize len = static_cast<usize>(end - first);
  if ( len == 0 ) return;
  n %= len;
  if ( n == 0 ) return;
  rotate_left(first, end, len - n);
}

template<is_iterable_container C>
constexpr C &
rotate_left(C &c, usize n) noexcept
{
  rotate_left(c.begin(), c.end(), n);
  return c;
}

template<is_iterable_container C>
constexpr C &
rotate_right(C &c, usize n) noexcept
{
  rotate_right(c.begin(), c.end(), n);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sums/fills
//
__micron_push_options
__micron_optimize_no_fast_math
namespace __impl
{
[[gnu::always_inline]] constexpr void
__neumaier_add(f64 &acc, f64 &comp, f64 v) noexcept
{
  const f64 t = acc + v;
  // whichever operand is larger in magnitude keeps its bits; the other loses the low end
  comp += (__builtin_fabs(acc) >= __builtin_fabs(v)) ? ((acc - t) + v) : ((v - t) + acc);
  acc = t;
}

[[gnu::always_inline]] constexpr bool
__f64_finite(f64 v) noexcept
{
  return (__builtin_bit_cast(u64, v) & 0x7ff0000000000000ull) != 0x7ff0000000000000ull;
}

template<typename A>
constexpr f64
__sum_lanes(const A &a, usize n) noexcept
{
  f64 s[4] = { 0, 0, 0, 0 };
  f64 c[4] = { 0, 0, 0, 0 };

  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    __neumaier_add(s[0], c[0], static_cast<f64>(a[i + 0]));
    __neumaier_add(s[1], c[1], static_cast<f64>(a[i + 1]));
    __neumaier_add(s[2], c[2], static_cast<f64>(a[i + 2]));
    __neumaier_add(s[3], c[3], static_cast<f64>(a[i + 3]));
  }
  for ( ; i < n; ++i ) __neumaier_add(s[0], c[0], static_cast<f64>(a[i]));

  f64 acc = 0, comp = 0;
  for ( usize k = 0; k < 4; ++k ) {
    __neumaier_add(acc, comp, s[k]);
    comp += c[k];
  }
  return acc + comp;
}

template<typename A>
constexpr f128
__sum_wide(const A &a, usize n) noexcept
{
  f128 sm = 0;
  for ( usize i = 0; i < n; ++i ) sm += static_cast<f128>(a[i]);
  return sm;
}

template<typename A>
constexpr f128
__sum_fp(const A &a, usize n) noexcept
{
  const f64 fast = __sum_lanes(a, n);
  if ( __f64_finite(fast) ) return static_cast<f128>(fast);
  // overflowed an f64 lane
  return __sum_wide(a, n);
}
};      // namespace __impl

template<is_iterable_container T>
  requires micron::is_floating_point_v<typename T::value_type>
constexpr f128
sum(const T &src) noexcept
{
  if constexpr ( __impl::__flat_readable<T> )
    return __impl::__sum_fp(src.begin(), src.size());
  else
    return __impl::__sum_fp(src, src.size());
}

__micron_pop_options
namespace __impl
{
template<typename A>
constexpr umax_t
__sum_int(const A &a, usize n) noexcept
{
  // four accumulators
  umax_t w = 0, x = 0, y = 0, z = 0;
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    w += static_cast<umax_t>(a[i + 0]);
    x += static_cast<umax_t>(a[i + 1]);
    y += static_cast<umax_t>(a[i + 2]);
    z += static_cast<umax_t>(a[i + 3]);
  }
  for ( ; i < n; ++i ) w += static_cast<umax_t>(a[i]);
  return (w + x) + (y + z);
}
};      // namespace __impl

template<is_iterable_container T>
  requires micron::is_integral_v<typename T::value_type>
constexpr umax_t
sum(const T &src) noexcept
{
  if constexpr ( __impl::__flat_readable<T> )
    return __impl::__sum_int(src.begin(), src.size());
  else
    return __impl::__sum_int(src, src.size());
}

template<is_iterable_container T, typename R = typename T::value_type>
constexpr T &
clear(T &src, const R r = 0) noexcept
{
  return fill(src, static_cast<typename T::value_type>(r));
}

template<typename R = f64, typename T>
  requires micron::is_object_v<T> && (!is_map_class<T>) && (!is_tree<T>)
constexpr R
mean(const T &src) noexcept
{
  return static_cast<R>(sum(src)) / static_cast<R>(src.size());
}

template<typename R = flong, typename T>
  requires micron::is_object_v<T> && (!is_map_class<T>) && (!is_tree<T>)
constexpr R
geomean(const T &src) noexcept
{
  R mulsm = static_cast<R>(src[0]);
  for ( usize i = 1; i < src.size(); i++ ) mulsm *= static_cast<R>(src[i]);
  return math::powerflong(mulsm, static_cast<R>(R(1) / R(src.size())));
}

template<typename R = flong, typename T>
  requires micron::is_object_v<T> && (!is_map_class<T>) && (!is_tree<T>)
constexpr R
harmonicmean(const T &src) noexcept
{
  R recsum = 0;
  for ( usize i = 0; i < src.size(); i++ ) recsum += (R(1) / static_cast<R>(src[i]));
  return static_cast<R>(src.size()) / recsum;
}

template<typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
round(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start ) *start = math::round(*start);
}

template<typename T>
constexpr void
round(T &t) noexcept
{
  round(t.begin(), t.end());
}

template<typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
ceil(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start ) *start = math::ceil(*start);
}

template<is_iterable_container T>
constexpr T &
ceil(T &t) noexcept
{
  ceil(t.begin(), t.end());
  return t;
}

template<typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
floor(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start ) *start = math::floor(*start);
}

template<typename T>
constexpr void
floor(T &t) noexcept
{
  floor(t.begin(), t.end());
}

template<typename T>
  requires micron::is_pointer_v<T>
constexpr void
reverse(T __restrict first, T __restrict end) noexcept
{
  while ( first < end ) {
    auto tmp = *first;
    *first = *end;
    *end = tmp;
    ++first;
    --end;
  }
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, const T *, const T *>
constexpr void
reverse(T *first, T *end, Fn fn) noexcept
{
  while ( first < end ) {
    if ( fn(first, end) ) {
      auto tmp = *first;
      *first = *end;
      *end = tmp;
    }
    ++first;
    --end;
  }
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, T, T>
constexpr void
reverse(T *first, T *end, Fn fn) noexcept
{
  while ( first < end ) {
    if ( fn(*first, *end) ) {
      auto tmp = *first;
      *first = *end;
      *end = tmp;
    }
    ++first;
    --end;
  }
}

template<is_iterable_container C>
constexpr C &
reverse(C &c) noexcept
{
  if ( c.size() == 0 ) return c;
  reverse(c.begin(), c.end() - 1);
  return c;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, const typename C::value_type *, const typename C::value_type *>
constexpr C &
reverse(C &c, Fn fn) noexcept
{
  if ( c.size() == 0 ) return c;
  reverse(c.begin(), c.end() - 1, fn);
  return c;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, typename C::value_type, typename C::value_type>
constexpr C &
reverse(C &c, Fn fn) noexcept
{
  if ( c.size() == 0 ) return c;
  reverse(c.begin(), c.end() - 1, fn);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reverses

template<auto Fn, typename T>
constexpr void
reverse(T *first, T *end) noexcept
{
  while ( first < end ) {
    if constexpr ( micron::invocable<decltype(Fn), const T *, const T *> ) {
      if ( Fn(first, end) ) {
        auto tmp = *first;
        *first = *end;
        *end = tmp;
      }
    } else {
      if ( Fn(*first, *end) ) {
        auto tmp = *first;
        *first = *end;
        *end = tmp;
      }
    }
    ++first;
    --end;
  }
}

template<auto Fn, is_iterable_container C>
constexpr C &
reverse(C &c) noexcept
{
  if ( c.size() == 0 ) return c;
  reverse<Fn>(c.begin(), c.end() - 1);
  return c;
}

template<auto Fn, typename T>
constexpr T *
reverse_copy(const T *first, const T *end, T *out) noexcept
{
  const T *it = end;
  while ( it != first ) {
    --it;
    if constexpr ( micron::invocable<decltype(Fn), const T *> ) {
      if ( Fn(it) ) *out++ = *it;
    } else {
      if ( Fn(*it) ) *out++ = *it;
    }
  }
  return out;
}

template<auto Fn, is_iterable_container C>
C
reverse_copy(const C &c)
{
  C out;
  out.resize(c.size());
  auto *last = reverse_copy<Fn>(c.begin(), c.end(), out.begin());
  out.resize(last - out.begin());
  return out;
}

template<typename T>
constexpr T *
reverse_copy(const T *first, const T *end, T *out) noexcept
{
  while ( end != first ) *out++ = *--end;
  return out;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, const T *>
constexpr T *
reverse_copy(const T *first, const T *end, T *out, Fn fn) noexcept
{
  const T *it = end;
  while ( it != first ) {
    --it;
    if ( fn(it) ) *out++ = *it;
  }
  return out;
}

template<typename T, typename Fn>
  requires micron::invocable<Fn, T>
constexpr T *
reverse_copy(const T *first, const T *end, T *out, Fn fn) noexcept
{
  const T *it = end;
  while ( it != first ) {
    --it;
    if ( fn(*it) ) *out++ = *it;
  }
  return out;
}

template<is_iterable_container C>
C
reverse_copy(const C &c)
{
  C out;
  out.resize(c.size());
  reverse_copy(c.begin(), c.end(), out.begin());
  return out;
}

template<is_iterable_container C, is_iterable_container O>
  requires micron::is_same_v<typename C::value_type, typename O::value_type>
O &
reverse_copy(const C &c, O &out)
{
  reverse_copy(c.begin(), c.end(), out.begin());
  return out;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, const typename C::value_type *>
C
reverse_copy(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = reverse_copy(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

template<is_iterable_container C, typename Fn>
  requires micron::invocable<Fn, typename C::value_type>
C
reverse_copy(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = reverse_copy(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

template<typename T>
typename T::const_iterator
max_at(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::const_iterator max_v = it;
  for ( ; it != end; ++it )
    if ( *it > *max_v ) max_v = it;
  return max_v;
}

template<typename T>
typename T::const_iterator
min_at(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::const_iterator min_v = it;
  for ( ; it != end; ++it )
    if ( *it < *min_v ) min_v = it;
  return min_v;
}

template<typename T>
const T *
max_at(const T *first, const T *end) noexcept
{
  if ( first == end ) return end;
  const T *max_v = first;
  for ( ; first != end; ++first )
    if ( *first > *max_v ) max_v = first;
  return max_v;
}

template<typename T>
const T *
min_at(const T *first, const T *end) noexcept
{
  if ( first == end ) return end;
  const T *min_v = first;
  for ( ; first != end; ++first )
    if ( *first < *min_v ) min_v = first;
  return min_v;
}

template<typename T>
typename T::value_type
max(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::value_type max_v = *it++;
  for ( ; it != end; ++it )
    if ( *it > max_v ) max_v = *it;
  return max_v;
}

template<typename T>
typename T::value_type
min(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::value_type min_v = *it++;
  for ( ; it != end; ++it )
    if ( *it < min_v ) min_v = *it;
  return min_v;
}

template<typename T>
T
max(const T *first, const T *end) noexcept
{
  T max_v = *first++;
  for ( ; first != end; ++first )
    if ( *first > max_v ) max_v = *first;
  return max_v;
}

template<typename T>
T
min(const T *first, const T *end) noexcept
{
  T min_v = *first++;
  for ( ; first != end; ++first )
    if ( *first < min_v ) min_v = *first;
  return min_v;
}

template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, typename M::mapped_type &>
constexpr M &
for_each(M &m, Fn fn) noexcept
{
  __impl::visit_kv(m, fn);
  return m;
}

template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
constexpr const M &
for_each(const M &m, Fn fn) noexcept
{
  __impl::visit_kv(m, fn);
  return m;
}

template<is_map_class M, typename Fn>
  requires micron::is_invocable_v<Fn, const typename M::key_type &, const typename M::mapped_type &>
           && micron::convertible_to<micron::invoke_result_t<Fn, const typename M::key_type &, const typename M::mapped_type &>,
                                     typename M::mapped_type>
constexpr M &
transform(M &m, Fn fn) noexcept
{
  __impl::visit_kv(m, [&](const auto &k, auto &v) { v = fn(k, v); });
  return m;
}

};      // namespace micron
