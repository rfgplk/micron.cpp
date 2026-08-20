//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/addr.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

struct __set_empty_v {
  constexpr bool
  operator==(const __set_empty_v &) const noexcept
  {
    return true;
  }

  constexpr bool
  operator!=(const __set_empty_v &) const noexcept
  {
    return false;
  }
};

namespace __impl
{

template<typename E>
constexpr decltype(auto)
__set_key_of(E &&__e) noexcept
{
  if constexpr ( requires { __e.key; } )
    return (__e.key);
  else if constexpr ( requires { __e.a; } )
    return (__e.a);
  else
    return micron::get<0>(__e);
}

};      // namespace __impl

template<typename MIt, typename K> class __set_key_iter
{
  MIt __i;

public:
  using value_type = K;
  using reference = const K &;
  using pointer = const K *;
  using difference_type = ssize_t;

  constexpr __set_key_iter() = default;

  constexpr explicit __set_key_iter(MIt __it) noexcept : __i(__it) { }

  const K &
  operator*() const
  {
    return __impl::__set_key_of(*__i);
  }

  const K *
  operator->() const
  {
    return micron::addressof(__impl::__set_key_of(*__i));
  }

  __set_key_iter &
  operator++()
  {
    ++__i;
    return *this;
  }

  __set_key_iter
  operator++(int)
  {
    __set_key_iter __t = *this;
    ++__i;
    return __t;
  }

  bool
  operator==(const __set_key_iter &__o) const
  {
    return __i == __o.__i;
  }

  bool
  operator!=(const __set_key_iter &__o) const
  {
    return !(__i == __o.__i);
  }

  const MIt &
  base() const noexcept
  {
    return __i;
  }
};

};      // namespace micron
