// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
