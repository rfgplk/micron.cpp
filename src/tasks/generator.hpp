//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../defs.hpp"
#include "../except.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

template<class T> class [[nodiscard("a dropped micron::generator produces nothing; iterate it")]] generator
{
public:
  struct promise_type {
    micron::remove_reference_t<T> *__value = nullptr;
    const char *__err = nullptr;

    generator
    get_return_object() noexcept
    {
      return generator{ std::coroutine_handle<promise_type>::from_promise(*this) };
    }

    std::suspend_always
    initial_suspend() noexcept
    {
      return {};
    }

    std::suspend_always
    final_suspend() noexcept
    {
      return {};
    }

    std::suspend_always
    yield_value(micron::remove_reference_t<T> &__v) noexcept
    {
      __value = __builtin_addressof(__v);
      return {};
    }

    std::suspend_always
    yield_value(micron::remove_reference_t<T> &&__v) noexcept
    {
      __value = __builtin_addressof(__v);
      return {};
    }

    void
    return_void() noexcept
    {
    }

    void
    unhandled_exception() noexcept
    {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      __err = "micron::generator: unhandled exception";
#else
      __builtin_trap();
#endif
    }
  };

  using __handle = std::coroutine_handle<promise_type>;

  generator() noexcept = default;

  explicit generator(__handle __h) noexcept : __coro(__h) { }

  generator(const generator &) = delete;
  generator &operator=(const generator &) = delete;

  generator(generator &&__o) noexcept : __coro(__o.__coro) { __o.__coro = {}; }

  generator &
  operator=(generator &&__o) noexcept
  {
    if ( this != &__o ) {
      if ( __coro ) __coro.destroy();
      __coro = __o.__coro;
      __o.__coro = {};
    }
    return *this;
  }

  ~generator()
  {
    if ( __coro ) __coro.destroy();
  }

  struct sentinel {
  };

  struct iterator {
    __handle __h{};

    void
    __advance()
    {
      __h.resume();
      if ( __h.done() ) {
        const char *__e = __h.promise().__err;
        if ( __e ) micron::exc<except::library_error>(__e);      // throws under EH, aborts under freestanding
      }
    }

    iterator &
    operator++()
    {
      __advance();
      return *this;
    }

    void
    operator++(int)
    {
      __advance();
    }

    bool
    operator==(sentinel) const noexcept
    {
      return !__h || __h.done();
    }

    bool
    operator!=(sentinel __s) const noexcept
    {
      return !(*this == __s);
    }

    T &
    operator*() const noexcept
    {
      return *__h.promise().__value;
    }

    T *
    operator->() const noexcept
    {
      return __h.promise().__value;
    }
  };

  iterator
  begin()
  {
    iterator __it{ __coro };
    if ( __coro ) __it.__advance();      // resume to the first co_yield
    return __it;
  }

  sentinel
  end() noexcept
  {
    return {};
  }

  bool
  valid() const noexcept
  {
    return static_cast<bool>(__coro);
  }

private:
  __handle __coro{};
};

}      // namespace micron
