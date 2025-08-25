//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/spinlock.hpp"
#include "../types.hpp"
#include "../type_traits.hpp"

// TODO: implement

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
