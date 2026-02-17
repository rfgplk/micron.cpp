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
template <typename R, typename... Args> using token_function = R (*)(Args...);

template <typename R = void, typename T = micron::mutex *, typename A = void (micron::mutex::*)()> class token
{
  token_function<R, T, A> fptr;
  micron::mutex mtx;
  void (micron::mutex::*rmtx)();

public:
  ~token() {}

  token() : fptr(nullptr), mtx(), rmtx(nullptr) {}

  token(token_function<R, T, A> f) : fptr(f), mtx(), rmtx(nullptr) {}

  token(const token &) = delete;
  token(token &&) = delete;
  token &operator=(const token &) = delete;
  token &operator=(token &&) = delete;

  bool
  operator()(void)
  {
    if ( !mtx ) {
      rmtx = mtx();
    } else {
      if ( fptr )
        fptr(&mtx, rmtx);
      return false;
    }
    return true;
  }
};
};
