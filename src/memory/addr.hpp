//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

// all functions related to addresses go here

namespace micron
{

template <class T>
void *
voidify(const T *ptr)
{
  using erase_type = micron::remove_cv_t<decltype(ptr)>;
  if ( ptr != nullptr )
    return const_cast<void *>(static_cast<const void *>(static_cast<erase_type>(ptr)));
  else
    return (void *)nullptr;
};

template <typename T, typename P>
T
cast(P &&p)
{
  if constexpr ( micron::is_pointer_v<T> && micron::is_pointer_v<P> )
    return reinterpret_cast<T>(p);
  if constexpr ( (!micron::is_pointer_v<T>) && (!micron::is_pointer_v<P>) && micron::is_convertible_v<T, P> ) {
    if constexpr ( micron::is_const_v<T>(p) )
      return const_cast<T>(static_cast<micron::remove_const<T>::type>(p));
    else
      return static_cast<T>(p);
  }
  if constexpr ( micron::is_const_v<T>(p) )
    return const_cast<T>(p);
}

template <typename T>
size_t
max_cast(T &&t)
{
  return static_cast<size_t>(const_cast<micron::remove_const<T>::type>(t));
}

template <typename P>
constexpr addr_t *
real_addr(P &p) noexcept
{
  return reinterpret_cast<addr_t *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template <typename O, typename P>
constexpr O *
real_addr_as(P &p) noexcept
{
  return reinterpret_cast<O *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template <typename P>
constexpr P *
addressof(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template <typename P>
constexpr P *
addr_of(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template <typename P>
  requires(micron::is_pointer_v<P>)
constexpr P *
addr(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template <typename P>
  requires(!micron::is_pointer_v<P>)
constexpr P *
addr(P &p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template <typename P>
constexpr P *
addr(P &&p) noexcept
{
  return reinterpret_cast<P *>(&const_cast<byte &>(reinterpret_cast<const volatile byte &>(p)));
}

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
T *
not_null(T *p)
{
  if ( p == nullptr ) {
  }     // exc<except::memory_error>("not_null was nullptr");
  return p;
}
};     // namespace micron
