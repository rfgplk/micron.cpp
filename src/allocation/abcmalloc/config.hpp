//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/kmemory.hpp"
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
constexpr static const bool __default_oom_enable = false;     // NOTE: costs performance
constexpr static const bool __default_launder_auto = true;
constexpr static const float __default_oom_limit_warn = 0.1f;
constexpr static const float __default_oom_limit_error = 0.2f;

constexpr static const bool __default_tombstone = true; // NOTE: by enabling this the allocator will never return memory that has been freed to a new allocation UNLESS the page on which it resides has been unmapped
constexpr static const bool __default_insert_guard_pages = true;
constexpr static const int __default_guard_page_perms = micron::PROT_NONE;

// NOTE: all of these cost a lot of performance
constexpr static const bool __default_debug_notices = false;
constexpr static const bool __default_zero_on_alloc = false;
constexpr static const bool __default_zero_on_free = false;
constexpr static const bool __default_full_on_free = false;
constexpr static const bool __default_sanitize = false;
constexpr static const byte __default_sanitize_with_on_alloc = 0xcc;

constexpr static const bool __default_collect_stats = false;
};
