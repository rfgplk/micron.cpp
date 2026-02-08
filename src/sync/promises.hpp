//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"
namespace micron
{
class broken_promise
{
};
class promise
{
  micron::mutex mtx;

public:
  promise(void) {}
  promise(promise &&o) {}
  promise(const promise &) = delete;
  promise &operator=(const promise &) = delete;
  promise &
  operator=(promise &&o)
  {

    return *this;
  }
};

template <typename T, typename F, typename R, typename... Args>
inline bool
expect(const T &t, const F &f, R r, Args &&...args)
{
  if ( t == f ) {
    r(args...);
    return true;
  }
  return false;
}
template <typename T, typename F, typename... Args>
inline bool
expect(const T &t, const F &f)
{
  return (f == t);
}
};
