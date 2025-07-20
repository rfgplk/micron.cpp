//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
inline u64
fib_32(u32 key)
{
  return (key * 11400714819323198485ull);
}

template <int N>
inline u64
fib_32_n(u32 key)
{
  return (key * 11400714819323198485ull) >> (64 - N);
}
};
