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

constexpr static const u64 __class_arena_internal = 1024;
constexpr static const u64 __class_precise = (2 << 7);     // 256
constexpr static const u64 __class_small = (2 << 8);       // 512
constexpr static const u64 __class_medium = (2 << 10);     // 2048
constexpr static const u64 __class_large = (2 << 11);      // 4096
constexpr static const u64 __class_huge = (2 << 13);       // 16384

constexpr static const u64 __default_max_retries = 2;
// in pages (each page is 4096)
constexpr static const u64 __default_arena_page_buf = 512;     // 2MiB for now... ~81k rnd allocations
constexpr static const u64 __default_page_mul = 96;            // 393KiB as a baseline
constexpr static const bool __default_init_large_pages = false;
};
