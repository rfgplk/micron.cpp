//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../math/generic.hpp"
#include "../memory/memory.hpp"
#include "../types.hpp"
#include <exception>
#include <type_traits>

namespace micron
{
template <class I, typename T>
T
accumulate(I start, I end, T init)
{
  for ( ; start != end; ++start )
    init = std::move(init) + *start;
  return init;
}
};
