//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace abc
{

#include "config.hpp"

struct stats_t {
  u64 alloc_requests;
  u64 dealloc_requests;
  u64 total_memory_req;            // how much was requested
  u64 total_memory_throughput;     // how much was actually allocd
  u64 total_memory_freed;
  u64 current_memory_usage;
  u64 current_page_usage;
};

enum class stat_type : int {
  alloc,
  dealloc,
  total_memory_req,
  total_memory_throughput,
  total_memory_freed,
  current_memory_usage,
  current_page_usage,
  __end
};

static stats_t stat = {};
template <stat_type S>
inline __attribute__((always_inline)) void
collect_stats(size_t n = 0)
{
  if constexpr ( __default_collect_stats ) {
    if constexpr ( S == stat_type::alloc ) {
      stat.alloc_requests++;
    } else if constexpr ( S == stat_type::dealloc ) {
      stat.dealloc_requests++;
    } else if constexpr ( S == stat_type::total_memory_req ) {
      stat.total_memory_req += n;
    } else if constexpr ( S == stat_type::total_memory_throughput ) {
      stat.total_memory_throughput += n;
    } else if constexpr ( S == stat_type::total_memory_freed ) {
      stat.total_memory_freed += n;
    }
  }
}
inline __attribute__((always_inline)) auto &
get_stats(void)
{
  return stat;
}
};
