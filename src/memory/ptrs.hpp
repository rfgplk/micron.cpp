//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "pointers/constant.hpp"
#include "pointers/free.hpp"
#include "pointers/global.hpp"
//#include "pointers/hazard.hpp"
#include "pointers/node.hpp"
#include "pointers/sentinel.hpp"
#include "pointers/shared.hpp"
#include "pointers/thread.hpp"
#include "pointers/unique.hpp"
#include "pointers/void.hpp"
#include "pointers/weak.hpp"

#include "../concepts.hpp"
#include "../type_traits.hpp"

namespace micron
{
template <is_pointer_class T>
bool
is_alive_ptr(const T &ptr)
{
  return ptr.get() != nullptr;
}
template <typename T>
bool
is_valid_ptr(T &&)
{
  if constexpr ( is_pointer_class<micron::decay_t<T>> ) {
    return true;
  }
  return false;
}
};
