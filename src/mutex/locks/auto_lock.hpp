//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex.hpp"

#include "../../atomic/atomic.hpp"
#include "../../sync/yield.hpp"

namespace micron
{

template <class M = mutex> class auto_guard
{
  M mtx;
  void (micron::mutex::*rptr)();

public:
  auto_guard() : mtx(), rptr(mtx()) {};

  ~auto_guard() { (mtx.*rptr)(); }

  auto_guard(const auto_guard &) = delete;
  auto_guard(auto_guard &&) = delete;
  auto_guard &operator=(auto_guard &&) = delete;
  auto_guard &operator=(const auto_guard &) = delete;
};

};     // namespace micron
