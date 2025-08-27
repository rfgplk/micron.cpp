//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

// [

namespace abc
{
void *alloc(size_t size);
void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void *free(void *ptr);
void *aligned_alloc(size_t alignment, size_t size);
// launder()
// query
// inject
// make_at
};
