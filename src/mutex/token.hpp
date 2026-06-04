//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "mutex.hpp"

namespace micron
{
// tokens are used similar to stop_tokens from the STL (in place of mutex)
// used to signal whether an action should be taken, if it should, execute the callback
template<typename R, typename... Args> using token_function = R (*)(Args...);

template<typename R = void, typename T = micron::mutex *, typename A = void (micron::mutex::*)()> class token
{
  static_assert(micron::is_same_v<T, micron::mutex *>, "token: T must be micron::mutex* (callback dispatch is hardwired)");
  static_assert(micron::is_same_v<A, void (micron::mutex::*)()>,
                "token: A must be micron::mutex's reset PMF (callback dispatch is hardwired)");

  token_function<R, T, A> fptr;
  micron::mutex mtx;      // retained only so the callback receives a (non-null) mutex* + its unlock PMF (legacy ABI)
  atomic_token<bool> fired;

public:
  ~token() { }

  token() : fptr(nullptr), mtx(), fired(false) { }

  token(token_function<R, T, A> f) : fptr(f), mtx(), fired(false) { }

  token(const token &) = delete;
  token(token &&) = delete;
  token &operator=(const token &) = delete;
  token &operator=(token &&) = delete;

  bool
  operator()(void)
  {
    if ( fired.compare_and_swap(false, true) ) return true;      // first caller wins (single atomic op; no TOCTOU, no held lock)
    if ( fptr ) fptr(&mtx, mtx.retrieve());                      // later callers dispatch the callback
    return false;
  }
};
};      // namespace micron
