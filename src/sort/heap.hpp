//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// heapsort
//   time:  O(n log n) all cases
//   space: O(1) in-place
//   stable: no
//
//   use: hard real-time / memory-tight; serves as the introsort depth fallback

#include "../algorithm/memory.hpp"
#include "../concepts.hpp"
#include "../memory/actions.hpp"
#include "../types.hpp"

namespace micron
{
namespace sort
{

template<is_iterable_container C, typename Compare>
constexpr void
__sift_down(C &c, usize start, usize n, Compare comp) noexcept
{
  auto *first = c.begin();
  usize root = start;

  for ( ;; ) {
    usize left = (root << 1) + 1;
    if ( left >= n ) break;

    usize swap_i = root;
    if ( comp(first[swap_i], first[left]) ) swap_i = left;

    usize right = left + 1;
    if ( right < n && comp(first[swap_i], first[right]) ) swap_i = right;

    if ( swap_i == root ) return;

    micron::swap(first[root], first[swap_i]);
    root = swap_i;
  }
}

template<is_iterable_container C, typename Compare>
constexpr void
__make_heap(C &c, usize n, Compare comp) noexcept
{
  if ( n < 2 ) return;
  for ( usize i = n / 2; i-- > 0; ) __sift_down(c, i, n, comp);
}

template<is_iterable_container C, typename Compare>
constexpr void
__sort_heap(C &c, usize n, Compare comp) noexcept
{
  if ( n < 2 ) return;
  while ( n > 1 ) {
    micron::swap(c[0], c[n - 1]);
    --n;
    __sift_down(c, 0, n, comp);
  }
}

template<is_iterable_container C>
constexpr void
make_heap(C &c) noexcept
{
  __make_heap(c, c.size(), [](const typename C::value_type &a, const typename C::value_type &b) { return a < b; });
}

template<is_iterable_container C, is_valid_comp<C> Cmp>
constexpr void
make_heap(C &c, Cmp comp) noexcept
{
  __make_heap(c, c.size(), comp);
}

template<is_iterable_container C>
constexpr void
heap(C &c) noexcept
{
  auto comp = [](const typename C::value_type &a, const typename C::value_type &b) { return a < b; };
  const usize n = c.size();
  __make_heap(c, n, comp);
  __sort_heap(c, n, comp);
}

template<is_iterable_container C, is_valid_comp<C> Cmp>
constexpr void
heap(C &c, Cmp comp) noexcept
{
  const usize n = c.size();
  __make_heap(c, n, comp);
  __sort_heap(c, n, comp);
}

template<is_iterable_container C>
constexpr C &
as_heap(C &c) noexcept
{
  heap(c);
  return c;
}

template<is_iterable_container C, is_valid_comp<C> Cmp>
constexpr C &
as_heap(C &c, Cmp comp) noexcept
{
  heap(c, comp);
  return c;
}

template<is_iterable_container C>
constexpr C &
as_heap(C &c, typename C::size_type lim) noexcept
{
  const usize n = lim < c.size() ? lim : c.size();
  auto comp = [](const typename C::value_type &a, const typename C::value_type &b) { return a < b; };
  __make_heap(c, n, comp);
  __sort_heap(c, n, comp);
  return c;
}

template<is_iterable_container C, is_valid_comp<C> Cmp>
constexpr C &
as_heap(C &c, typename C::size_type lim, Cmp comp) noexcept
{
  const usize n = lim < c.size() ? lim : c.size();
  __make_heap(c, n, comp);
  __sort_heap(c, n, comp);
  return c;
}

};      // namespace sort
};      // namespace micron
