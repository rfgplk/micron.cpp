//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/allocate_map.hpp"
namespace abc
{

constexpr static const u64 __system_pagesize = micron::page_size;

constexpr static const u64 __class_arena_internal = 65536;
constexpr static const u64 __class_small = 512;
constexpr static const u64 __class_medium = 1024;
constexpr static const u64 __class_large = 4096;
constexpr static const u64 __class_huge = 8192;

constexpr static const u64 __default_max_retries = 10;
constexpr static const u64 __default_arena_page_buf = 32;     // 4MiB for now
constexpr static const u64 __default_page_mul = 8;

};
