//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

namespace micron
{

struct sched_attr {
  u32 size;
  u32 sched_policy;
  u64 sched_flag;
  i32 sched_nice;         // scheduled
  u32 sched_priority;     // rt
  u64 sched_runtime;      // ns
  u64 sched_deadline;     // ns
  u64 sched_period;       // ns

  u32 sched_util_min;
  u32 sched_util_max;
};

};     // namespace micron
