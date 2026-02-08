//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

#include "arena.hpp"
#include "thread.hpp"

// thread pool
namespace micron
{

using standard_arena = arena::__default_arena<micron::thread<>>;

static standard_arena *__global_threadpool = nullptr;

__attribute__((constructor)) void
__make_arena(void)
{
  static standard_arena local_pool;
  __global_threadpool = &local_pool;
};
};
