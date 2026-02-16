//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

template <typename F> struct __defer_guard {
  F f;
  bool active;

  constexpr explicit __defer_guard(F &&fn) noexcept : f(static_cast<F &&>(fn)), active(true) {}

  constexpr __defer_guard(__defer_guard &&other) noexcept : f(static_cast<F &&>(other.f)), active(other.active) { other.active = false; }

  __defer_guard(const __defer_guard &) = delete;
  __defer_guard &operator=(const __defer_guard &) = delete;
  __defer_guard &operator=(__defer_guard &&) = delete;

  constexpr ~__defer_guard() noexcept(noexcept(f()))
  {
    if ( active )
      f();
  }
};

template <typename F>
constexpr auto
__make_defer(F &&f) noexcept
{
  return __defer_guard<F>(static_cast<F &&>(f));
}

#define __DEFER_CONCAT_IMPL(x, y) x##y
#define __DEFER_CONCAT(x, y) __DEFER_CONCAT_IMPL(x, y)

#define defer(code) auto __DEFER_CONCAT(__defer_obj_, __COUNTER__) = __make_defer([&]() noexcept(noexcept(code)) { code; })
