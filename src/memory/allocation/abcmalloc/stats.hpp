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

#include "../../../atomic/atomic.hpp"

namespace abc
{

#include "config.hpp"

struct stats_t {
  bool enabled;
  u64 alloc_requests;
  u64 dealloc_requests;
  u64 total_memory_req;             // how much was requested
  u64 total_memory_throughput;      // how much was actually allocd
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

#if defined(MICRON_ABC_STATS)
struct __stats_storage {
  micron::atomic_token<usize> alloc_requests{ 0 };
  micron::atomic_token<usize> dealloc_requests{ 0 };
  micron::atomic_token<usize> total_memory_req{ 0 };
  micron::atomic_token<usize> total_memory_throughput{ 0 };
  micron::atomic_token<usize> total_memory_freed{ 0 };
  micron::atomic_token<usize> current_memory_usage{ 0 };
  micron::atomic_token<usize> current_page_usage{ 0 };
};

inline __stats_storage __stats{};

[[gnu::always_inline]] inline void
__subtract_current_memory(usize n) noexcept
{
  usize observed = __stats.current_memory_usage.get(micron::memory_order_relaxed);
  for ( ;; ) {
    const usize next = n > observed ? 0 : observed - n;
    if ( __stats.current_memory_usage.compare_exchange_weak(observed, next, micron::memory_order_relaxed, micron::memory_order_relaxed) )
      return;
  }
}
#endif

template<stat_type S>
inline __attribute__((always_inline)) void
collect_stats(usize n = 0)
{
#if defined(MICRON_ABC_STATS)
  if constexpr ( S == stat_type::alloc ) {
    __stats.alloc_requests.fetch_add(1, micron::memory_order_relaxed);
  } else if constexpr ( S == stat_type::dealloc ) {
    __stats.dealloc_requests.fetch_add(1, micron::memory_order_relaxed);
  } else if constexpr ( S == stat_type::total_memory_req ) {
    __stats.total_memory_req.fetch_add(n, micron::memory_order_relaxed);
  } else if constexpr ( S == stat_type::total_memory_throughput ) {
    __stats.total_memory_throughput.fetch_add(n, micron::memory_order_relaxed);
    __stats.current_memory_usage.fetch_add(n, micron::memory_order_relaxed);
  } else if constexpr ( S == stat_type::total_memory_freed ) {
    __stats.total_memory_freed.fetch_add(n, micron::memory_order_relaxed);
    __subtract_current_memory(n);
  }
#else
  (void)n;
#endif
}

[[nodiscard]] inline stats_t
get_stats(void)
{
#if defined(MICRON_ABC_STATS)
  return { true,
           __stats.alloc_requests.get(micron::memory_order_relaxed),
           __stats.dealloc_requests.get(micron::memory_order_relaxed),
           __stats.total_memory_req.get(micron::memory_order_relaxed),
           __stats.total_memory_throughput.get(micron::memory_order_relaxed),
           __stats.total_memory_freed.get(micron::memory_order_relaxed),
           __stats.current_memory_usage.get(micron::memory_order_relaxed),
           __stats.current_page_usage.get(micron::memory_order_relaxed) };
#else
  return { false, 0, 0, 0, 0, 0, 0, 0 };
#endif
}

[[nodiscard]] inline stats_t
stats(void)
{
  return get_stats();
}

inline void
reset_stats(void) noexcept
{
#if defined(MICRON_ABC_STATS)
  __stats.alloc_requests.store(0, micron::memory_order_relaxed);
  __stats.dealloc_requests.store(0, micron::memory_order_relaxed);
  __stats.total_memory_req.store(0, micron::memory_order_relaxed);
  __stats.total_memory_throughput.store(0, micron::memory_order_relaxed);
  __stats.total_memory_freed.store(0, micron::memory_order_relaxed);
  __stats.current_memory_usage.store(0, micron::memory_order_relaxed);
  __stats.current_page_usage.store(0, micron::memory_order_relaxed);
#endif
}
};      // namespace abc
