//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/mem.hpp"
#include "../except.hpp"
#include "../types.hpp"
#include <type_traits>

// all functions related to addresses go here

namespace micron
{
template <typename T, typename P>
T
cast(P &&p)
{
  if constexpr ( std::is_pointer_v<T> && std::is_pointer_v<P> )
    return reinterpret_cast<T>(p);
  if constexpr ( (!std::is_pointer_v<T>) && (!std::is_pointer_v<P>) && std::is_convertible_v<T, P> ) {
    if constexpr ( std::is_const_v<T>(p) )
      return const_cast<T>(static_cast<std::remove_const<T>::type>(p));
    else
      return static_cast<T>(p);
  }
  if constexpr ( std::is_const_v<T>(p) )
    return const_cast<T>(p);
}
template <typename T>
size_t
max_cast(T &&t)
{
  return static_cast<size_t>(const_cast<std::remove_const<T>::type>(t));
}
template <typename P>
constexpr P *
addressof(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}
template <typename P>
constexpr P *
addr(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}
template <typename T>
  requires(!std::is_null_pointer_v<T>)
T *
not_null(T *p)
{
  if ( p == nullptr )
    throw except::memory_error("not_null was nullptr");
  return p;
}
};
