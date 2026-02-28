//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../memory/actions.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{

template <is_iterable_container C, typename Compare>
constexpr void
__sift_down(C &c, usize start, usize n, Compare comp) noexcept
{
  auto *first = c.begin();
  usize root = start;

  for ( ;; ) {
    usize left = (root << 1) + 1;
    if ( left >= n )
      break;

    usize swap_i = root;
    if ( comp(first[swap_i], first[left]) )
      swap_i = left;

    usize right = left + 1;
    if ( right < n && comp(first[swap_i], first[right]) )
      swap_i = right;

    if ( swap_i == root )
      return;

    auto tmp = micron::move(first[root]);
    first[root] = micron::move(first[swap_i]);
    first[swap_i] = micron::move(tmp);

    root = swap_i;
  }
}

template <is_iterable_container C, typename Compare>
constexpr void
__make_heap(C &c, Compare comp) noexcept
{
  usize n = c.size();
  if ( n < 2 )
    return;

  for ( usize i = n / 2; i-- > 0; )
    __sift_down(c, i, n, comp);
}

template <is_iterable_container C>
constexpr void
make_heap(C &c) noexcept
{
  __make_heap(c, [](const typename C::value_type &a, const typename C::value_type &b) { return a < b; });
}

template <is_iterable_container C, is_valid_comp<C> Cmp>
constexpr void
make_heap(C &c, Cmp comp) noexcept
{
  __make_heap(c, comp);
}

template <is_iterable_container C, typename Compare>
constexpr void
__sort_heap(C &c, Compare comp) noexcept
{
  usize n = c.size();
  if ( n < 2 )
    return;

  while ( n > 1 ) {
    auto tmp = micron::move(c[0]);
    c[0] = micron::move(c[n - 1]);
    c[n - 1] = micron::move(tmp);
    --n;
    __sift_down(c, 0, n, comp);
  }
}

template <is_iterable_container C>
constexpr void
heap(C &c) noexcept
{
  __sort_heap(c, [](const typename C::value_type &a, const typename C::value_type &b) { return a < b; });
}

template <is_iterable_container C, is_valid_comp<C> Cmp>
constexpr void
heap(C &c, Cmp comp) noexcept
{
  __sort_heap(c, comp);
}

template <is_iterable_container C>
constexpr C &
as_heap(C &c) noexcept
{
  make_heap(c);
  heap(c);
  return c;
}

template <is_iterable_container C, is_valid_comp<C> Cmp>
constexpr C &
as_heap(C &c, Cmp comp) noexcept
{
  make_heap(c, comp);
  heap(c, comp);
  return c;
}

template <is_iterable_container C>
constexpr C &
as_heap(C &c, typename C::size_type lim) noexcept
{
  if ( lim < 2 )
    return c;
  __make_heap(c, [](const typename C::value_type &a, const typename C::value_type &b) { return a < b; });
  __sort_heap(c, [](const typename C::value_type &a, const typename C::value_type &b) { return a < b; });
  return c;
}

template <is_iterable_container C, is_valid_comp<C> Cmp>
constexpr C &
as_heap(C &c, typename C::size_type lim, Cmp comp) noexcept
{
  if ( lim < 2 )
    return c;
  __make_heap(c, comp);
  __sort_heap(c, comp);
  return c;
}

};     // namespace sort
};     // namespace micron
