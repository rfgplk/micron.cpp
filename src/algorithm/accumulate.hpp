//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../math/generic.hpp"
#include "../memory/memory.hpp"
#include "../memory/actions.hpp"
#include "../types.hpp"

namespace micron
{
template <class I, typename T>
T
accumulate(I start, I end, T init) noexcept
{
  for ( ; start != end; ++start )
    init = micron::move(init) + *start;
  return init;
}
};
