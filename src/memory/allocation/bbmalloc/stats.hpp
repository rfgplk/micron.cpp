// Copyright (c) 2026 David Lucius Severus
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

#include "config.hpp"

#include "../../../types.hpp"

namespace bb
{

struct bb_stats {
  bool enabled;
  u64 alloc_requests;
  u64 dealloc_requests;
  u64 bytes_requested;
  u64 bytes_granted;
  u64 bytes_freed;
  u64 current_usage;
  u64 sheets_acquired;
  u64 sheets_released;
  u64 fallbacks;
  u64 failures;
};

enum class stat_type : int {
  alloc,
  dealloc,
  requested,
  granted,
  freed,
  sheet_acquired,
  sheet_released,
  fallback,
  failure
};

#if defined(MICRON_BB_STATS)
inline bb_stats __stats = { true, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif

template <stat_type S>
[[gnu::always_inline]] inline void
collect_stats(usize n = 0) noexcept
{
#if defined(MICRON_BB_STATS)
  if constexpr( S == stat_type::alloc )
    __stats.alloc_requests += 1;
  else if constexpr( S == stat_type::dealloc )
    __stats.dealloc_requests += 1;
  else if constexpr( S == stat_type::requested )
    __stats.bytes_requested += n;
  else if constexpr( S == stat_type::granted ) {
    __stats.bytes_granted += n;
    __stats.current_usage += n;
  } else if constexpr( S == stat_type::freed ) {
    __stats.bytes_freed += n;
    __stats.current_usage = n > __stats.current_usage ? 0 : __stats.current_usage - n;
  } else if constexpr( S == stat_type::sheet_acquired )
    __stats.sheets_acquired += 1;
  else if constexpr( S == stat_type::sheet_released )
    __stats.sheets_released += 1;
  else if constexpr( S == stat_type::fallback )
    __stats.fallbacks += 1;
  else if constexpr( S == stat_type::failure )
    __stats.failures += 1;
#else
  (void)n;
#endif
}

[[nodiscard]] inline bb_stats
get_stats() noexcept
{
#if defined(MICRON_BB_STATS)
  return __stats;
#else
  return { false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
}

inline void
reset_stats() noexcept
{
#if defined(MICRON_BB_STATS)
  __stats = { true, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
}

};
