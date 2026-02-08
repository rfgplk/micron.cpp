//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/spinlock.hpp"
#include "../types.hpp"
#include "../type_traits.hpp"

// (when) is a framework for enabling asynchronous work. similar to 'async' in the STL and other languages, but with a twist
// the general usage pattern is you select some boolean condition (which must evaluate to true) with a function/workload attached. upon calling when a background thread is launched which periodically checks if the condition has been met, and if it has, launches the task 

namespace micron
{
class when
{

public:
  template <typename... Targs, typename D> when(Targs &&...args, D &result)
  {
    for ( ;; ) {
    }
  }
};

};
