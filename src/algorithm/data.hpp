//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../math/constants.hpp"
#include "../math/generic.hpp"
#include "../math/trig.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

#include "../sort/heap.hpp"

namespace micron
{

template <is_iterable_container T>
  requires is_iterable_container<typename T::value_type>
T
melt(const T &obj)
{
  T out;
  using Row = typename T::value_type;
  for ( umax_t i = 0; i < obj[0].size(); i++ ) {
    for ( umax_t j = 0; j < obj.size(); j++ ) {
      Row r(2);
      r[0] = obj[j][i];
      r[1] = static_cast<typename Row::value_type>(j);
      out.push_back(r);
    }
  }
  return out;
}

template <is_iterable_container T>
  requires micron::is_arithmetic_v<typename T::value_type>
T
cut(const T &obj, const T &bins)
{
  T out;
  out.resize(obj.size());
  for ( umax_t i = 0; i < obj.size(); i++ ) {
    umax_t bin_index = 0;
    while ( bin_index < bins.size() - 1 && obj[i] > bins[bin_index + 1] )
      bin_index++;
    out[i] = bin_index;
  }
  return out;
}

template <typename I, typename J, typename O>
constexpr O
merge(I first1, I last1, J first2, J last2, O d_first)
{
  while ( first1 != last1 && first2 != last2 ) {
    if ( *first2 < *first1 ) {
      *d_first++ = *first2++;
    } else {
      *d_first++ = *first1++;
    }
  }
  while ( first1 != last1 )
    *d_first++ = *first1++;
  while ( first2 != last2 )
    *d_first++ = *first2++;
  return d_first;
}

template <is_iterable_container T>
T
merge(const T &obj1, const T &obj2)
{
  T out = obj1;
  out.insert(out.end(), obj2.begin(), obj2.end());
  return out;
}

template <is_iterable_container T>
T
concat(const T &obj1, const T &obj2)
{
  return merge(obj1, obj2);
}

template <typename I>
void
reverse(I first, I last)
{
  while ( (first != last) && (first != --last) ) {
    auto tmp = *first;
    *first = *last;
    *last = tmp;
    ++first;
  }
}

template <typename I>
I
rotate(I first, I n_first, I last)
{
  if ( first == n_first || n_first == last )
    return last;

  I read = n_first;
  I write = first;

  while ( write != n_first ) {
    auto tmp = *write;
    *write = *read;
    *read = tmp;

    ++write;
    ++read;
    if ( read == last )
      read = n_first;
  }

  I new_first = write;

  while ( read != last ) {
    auto tmp = *write;
    *write = *read;
    *read = tmp;

    ++write;
    ++read;
  }

  return new_first;
}

template <typename I>
I
cycle_rotate(I first, I n_first, I last)
{
  if ( first == n_first || n_first == last )
    return last;

  using diff_t = ptrdiff_t;
  diff_t n = last - first;
  diff_t k = n_first - first;
  diff_t g = math::gcd(n, k);

  for ( diff_t i = 0; i < g; ++i ) {
    auto tmp = *(first + i);
    diff_t j = i;

    while ( true ) {
      diff_t m = j + k;
      if ( m >= n )
        m -= n;
      if ( m == i )
        break;
      *(first + j) = *(first + m);
      j = m;
    }

    *(first + j) = tmp;
  }

  return first + (last - n_first);
}

template <typename T, typename Cmp>
constexpr void
__sift_down(T *first, umax_t start, umax_t end, Cmp comp)
{
  umax_t root = start;
  while ( true ) {
    umax_t child = 2 * root + 1;
    if ( child >= end )
      break;
    if ( child + 1 < end && comp(first[child], first[child + 1]) )
      child++;
    if ( comp(first[root], first[child]) ) {
      micron::swap(first[root], first[child]);
      root = child;
    } else {
      break;
    }
  }
}

template <typename T, typename Cmp>
constexpr void
__sift_up(T *first, umax_t idx, Cmp comp) noexcept
{
  while ( idx > 0 ) {
    umax_t parent = (idx - 1) >> 1;

    if ( !comp(first[parent], first[idx]) )
      return;

    auto tmp = micron::move(first[parent]);
    first[parent] = micron::move(first[idx]);
    first[idx] = micron::move(tmp);

    idx = parent;
  }
}

template <is_iterable_container C>
constexpr void
make_heap(C &c) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };

  umax_t n = c.size();
  if ( n < 2 )
    return;

  for ( umax_t i = (n >> 1); i > 0; ) {
    --i;
    __sift_down(c.begin(), i, n, comp);
  }
}

template <is_iterable_container C, typename Cmp>
constexpr void
make_heap(C &c, Cmp comp) noexcept
{
  umax_t n = c.size();
  if ( n < 2 )
    return;

  for ( umax_t i = (n >> 1); i > 0; ) {
    --i;
    __sift_down(c.begin(), i, n, comp);
  }
}

template <is_iterable_container C>
constexpr void
make_heap(C &c, typename C::umax_type lim) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };

  if ( lim < 2 )
    return;

  for ( umax_t i = (lim >> 1); i > 0; ) {
    --i;
    __sift_down(c.begin(), i, lim, comp);
  }
}

template <is_iterable_container C, typename Cmp>
constexpr void
make_heap(C &c, typename C::umax_type lim, Cmp comp) noexcept
{
  if ( lim < 2 )
    return;

  for ( umax_t i = (lim >> 1); i > 0; ) {
    --i;
    __sift_down(c.begin(), i, lim, comp);
  }
}

template <is_iterable_container C>
constexpr void
push_heap(C &c) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };

  umax_t n = c.size();
  if ( n < 2 )
    return;

  __sift_up(c.begin(), n - 1, comp);
}

template <is_iterable_container C, typename Cmp>
constexpr void
push_heap(C &c, typename C::umax_type lim, Cmp comp) noexcept
{
  if ( lim < 2 )
    return;

  __sift_up(c.begin(), lim - 1, comp);
}

template <is_iterable_container C>
constexpr void
push_heap(C &c, typename C::umax_type lim) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };

  if ( lim < 2 )
    return;

  __sift_up(c.begin(), lim - 1, comp);
}

template <is_iterable_container C, typename Cmp>
constexpr void
push_heap(C &c, Cmp comp) noexcept
{
  umax_t n = c.size();
  if ( n < 2 )
    return;

  __sift_up(c.begin(), n - 1, comp);
}

template <is_iterable_container C>
constexpr void
pop_heap(C &c) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };

  umax_t n = c.size();
  if ( n < 2 )
    return;

  auto *first = c.begin();

  auto tmp = micron::move(first[0]);
  first[0] = micron::move(first[n - 1]);
  first[n - 1] = micron::move(tmp);

  __sift_down(first, 0, n - 1, comp);
}

template <is_iterable_container C, typename Cmp>
constexpr void
pop_heap(C &c, Cmp comp) noexcept
{
  umax_t n = c.size();
  if ( n < 2 )
    return;

  auto *first = c.begin();

  auto tmp = micron::move(first[0]);
  first[0] = micron::move(first[n - 1]);
  first[n - 1] = micron::move(tmp);

  __sift_down(first, 0, n - 1, comp);
}

template <is_iterable_container C>
constexpr void
pop_heap(C &c, typename C::umax_type lim) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };

  if ( lim < 2 )
    return;

  auto *first = c.begin();

  auto tmp = micron::move(first[0]);
  first[0] = micron::move(first[lim - 1]);
  first[lim - 1] = micron::move(tmp);

  __sift_down(first, 0, lim - 1, comp);
}

template <is_iterable_container C, typename Cmp>
constexpr void
pop_heap(C &c, typename C::umax_type lim, Cmp comp) noexcept
{
  if ( lim < 2 )
    return;

  auto *first = c.begin();

  auto tmp = micron::move(first[0]);
  first[0] = micron::move(first[lim - 1]);
  first[lim - 1] = micron::move(tmp);

  __sift_down(first, 0, lim - 1, comp);
}

template <is_iterable_container C>
constexpr void
sort_heap(C &c) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a > b; };     // max
  umax_t n = c.size();
  auto *first = c.begin();

  while ( n > 1 ) {
    auto tmp = micron::move(first[0]);
    first[0] = micron::move(first[n - 1]);
    first[n - 1] = micron::move(tmp);

    --n;
    __sift_down(first, 0, n, comp);
  }
}

template <is_iterable_container C>
constexpr void
sort_heap_min(C &c) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };     // min
  umax_t n = c.size();
  auto *first = c.begin();

  while ( n > 1 ) {
    auto tmp = micron::move(first[0]);
    first[0] = micron::move(first[n - 1]);
    first[n - 1] = micron::move(tmp);

    --n;
    __sift_down(first, 0, n, comp);
  }
}

template <is_iterable_container C, typename Cmp>
constexpr void
sort_heap(C &c, Cmp comp) noexcept
{
  umax_t n = c.size();
  auto *first = c.begin();

  while ( n > 1 ) {
    auto tmp = micron::move(first[0]);
    first[0] = micron::move(first[n - 1]);
    first[n - 1] = micron::move(tmp);

    --n;
    __sift_down(first, 0, n, comp);
  }
}

template <is_iterable_container C>
constexpr bool
is_heap(const C &c) noexcept
{
  using T = typename C::value_type;
  auto comp = [](const T &a, const T &b) { return a < b; };

  umax_t n = c.size();
  auto *first = c.begin();

  for ( umax_t i = 1; i < n; ++i ) {
    umax_t parent = (i - 1) >> 1;
    if ( comp(first[parent], first[i]) )
      return false;
  }

  return true;
}

template <is_iterable_container C, typename Cmp>
constexpr bool
is_heap(const C &c, Cmp comp) noexcept
{
  umax_t n = c.size();
  auto *first = c.begin();

  for ( umax_t i = 1; i < n; ++i ) {
    umax_t parent = (i - 1) >> 1;
    if ( comp(first[parent], first[i]) )
      return false;
  }

  return true;
}

}     // namespace micron

// TODO: implement all of this, from pandas

/*
melt(frame[, id_vars, value_vars, var_name, ...])
pivot(data, *, columns[, index, values])
pivot_table(data[, values, index, columns, ...])
crosstab(index, columns[, values, rownames, ...])
cut(x, bins[, right, labels, retbins, ...])
qcut(x, q[, labels, retbins, precision, ...])
merge(left, right[, how, on, left_on, ...])
merge_ordered(left, right[, on, left_on, ...])
merge_asof(left, right[, on, left_on, ...])
concat(objs, *[, axis, join, ignore_index, ...])
get_dummies(data[, prefix, prefix_sep, ...])
from_dummies(data[, sep, default_category])
factorize(values[, sort, use_na_sentinel, ...])
unique(values)
lreshape(data, groups[, dropna])
wide_to_long(df, stubnames, i, j[, sep, suffix])
isna(obj)
isnull(obj)
notna(obj)
notnull(obj)
to_numeric(arg[, errors, downcast, ...])
to_datetime(arg[, errors, dayfirst, ...])
to_timedelta(arg[, unit, errors])
date_range([start, end, periods, freq, tz, ...])
bdate_range([start, end, periods, freq, tz, ...])
period_range([start, end, periods, freq, name])
timedelta_range([start, end, periods, freq, ...])
infer_freq(index)
interval_range([start, end, periods, freq, ...])
col(col_name)
*/
