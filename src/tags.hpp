//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron {
//mutability_type


struct mutable_tag {};
struct immutable_tag {};

//category_type
struct theap_tag {};
struct tree_tag {};
struct map_tag {};
struct string_tag {};
struct array_tag {};
struct vector_tag {};
struct list_tag {};
struct buffer_tag {};
struct slice_tag {};
struct bitfield_tag {};

//memory_type
struct heap_tag {};
struct stack_tag {};
struct thread_tag {};


//safety_type
struct safe_tag {};
struct unsafe_tag {};
};
