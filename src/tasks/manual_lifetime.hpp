//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../new.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// uninitialized aligned storage

namespace micron
{
namespace coro
{

template<class T> class manual_lifetime
{
  union {
    T __val;
  };

public:
  manual_lifetime() noexcept { }

  ~manual_lifetime() noexcept { }      // trivial: caller controls T's lifetime

  manual_lifetime(const manual_lifetime &) = delete;
  manual_lifetime &operator=(const manual_lifetime &) = delete;

  template<class... A>
  T *
  construct(A &&...__a) noexcept(micron::is_nothrow_constructible_v<T, A...>)
  {
    return ::new (static_cast<void *>(__builtin_addressof(__val))) T(micron::forward<A>(__a)...);
  }

  void
  destroy() noexcept
  {
    __val.~T();
  }

  T &
  get() & noexcept
  {
    return __val;
  }

  const T &
  get() const & noexcept
  {
    return __val;
  }

  T &&
  get() && noexcept
  {
    return micron::move(__val);
  }

  T *
  ptr() noexcept
  {
    return __builtin_addressof(__val);
  }

  T *
  operator->() noexcept
  {
    return __builtin_addressof(__val);
  }
};

template<> class manual_lifetime<void>
{
public:
  manual_lifetime() noexcept { }

  void
  construct() noexcept
  {
  }

  void
  destroy() noexcept
  {
  }

  void
  get() const noexcept
  {
  }
};

};      // namespace coro
};      // namespace micron
