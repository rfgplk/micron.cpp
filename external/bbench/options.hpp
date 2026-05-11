
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../src/types.hpp"

namespace bbench
{

struct benchmark_opts {
  u32 detail = 1;                      // -d / -dd / -ddd (1 default, 2, 3)
  u32 delay_ms = 0;                    // -D msec
  u32 timeout_ms = 0;                  // --timeout msec
  bool inherit = true;                 // perf-stat default for spawned commands
  bool scale = true;                   // multiplex-correct; off = print raw values
  bool pinned = false;                 // pin counters; off = let kernel multiplex
  bool excl_kernel = true;             // --all-user implied; --all-kernel flips
  bool excl_user = false;              // --all-kernel sets this true and excl_kernel false
  const char *event_csv = nullptr;     // -e cycles,instructions,…
  const char *pre = nullptr;           // --pre CMD
  const char *post = nullptr;          // --post CMD
};

};     // namespace bbench
