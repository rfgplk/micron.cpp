//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"

#include "../math/generic.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

template <class T, typename Acc>
Acc
accumulate(const T *first, const T *end, Acc init) noexcept
{
  for ( ; first != end; ++first )
    init = micron::move(init) + *first;
  return init;
}

template <class T, typename Acc, typename Fn>
  requires micron::is_invocable_v<Fn, Acc, const T &>
Acc
accumulate(const T *first, const T *end, Acc init, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    init = fn(micron::move(init), *first);
  return init;
}

template <class T, typename Acc>
Acc
accumulate(const T *first, const T *end, Acc init, size_t limit) noexcept
{
  for ( u64 i = 0; first != end and i < limit; ++first, ++i )
    init = micron::move(init) + *first;
  return init;
}

template <class T, typename Acc, typename Fn>
  requires micron::is_invocable_v<Fn, Acc, const T &>
Acc
accumulate(const T *first, const T *end, Acc init, Fn fn, size_t limit) noexcept
{
  for ( u64 i = 0; first != end and i < limit; ++first, ++i )
    init = fn(micron::move(init), *first);
  return init;
}

template <is_iterable_container C, typename Acc = typename C::value_type>
Acc
accumulate(const C &c, Acc init = Acc{}) noexcept
{
  return accumulate(c.begin(), c.end(), micron::move(init));
}

template <is_iterable_container C, typename Acc, typename Fn>
  requires micron::is_invocable_v<Fn, Acc, const typename C::value_type &>
Acc
accumulate(const C &c, Acc init, Fn fn) noexcept
{
  return accumulate(c.begin(), c.end(), micron::move(init), fn);
}

template <is_iterable_container C, typename Acc = typename C::value_type>
Acc
accumulate(const C &c, Acc init, size_t limit) noexcept
{
  return accumulate(c.begin(), c.end(), micron::move(init), limit);
}

template <is_iterable_container C, typename Acc, typename Fn>
  requires micron::is_invocable_v<Fn, Acc, const typename C::value_type &>
Acc
accumulate(const C &c, Acc init, Fn fn, size_t limit) noexcept
{
  return accumulate(c.begin(), c.end(), micron::move(init), fn, limit);
}

template <auto Fn, typename T, typename Acc>
constexpr Acc
accumulate(const T *first, const T *end, Acc init) noexcept
{
  for ( ; first != end; ++first )
    init = Fn(micron::move(init), *first);
  return init;
}

template <auto Fn, typename T, typename Acc>
constexpr Acc
accumulate(const T *first, const T *end, Acc init, size_t limit) noexcept
{
  for ( u64 i = 0; first != end && i < limit; ++first, ++i )
    init = Fn(micron::move(init), *first);
  return init;
}

template <auto Fn, is_iterable_container C, typename Acc = typename C::value_type>
constexpr Acc
accumulate(const C &c, Acc init = Acc{}) noexcept
{
  return accumulate<Fn>(c.begin(), c.end(), micron::move(init));
}

template <auto Fn, is_iterable_container C, typename Acc = typename C::value_type>
constexpr Acc
accumulate(const C &c, Acc init, size_t limit) noexcept
{
  return accumulate<Fn>(c.begin(), c.end(), micron::move(init), limit);
}

};     // namespace micron
