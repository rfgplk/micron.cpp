//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../types.hpp"
#include "types.hpp"

#include "time.hpp"

namespace micron
{
namespace posix
{
constexpr static const i32 rusage_self = 0;
constexpr static const i32 rusage_children = -1;
constexpr static const i32 rusage_both = -2;
constexpr static const i32 rusage_thread = 1;

struct rusage_t {
  timeval_t ru_utime;
  timeval_t ru_stime;
  kernel_long_t ru_maxrss;
  kernel_long_t ru_ixrss;
  kernel_long_t ru_idrss;
  kernel_long_t ru_isrss;
  kernel_long_t ru_minflt;
  kernel_long_t ru_majflt;
  kernel_long_t ru_nswap;
  kernel_long_t ru_inblock;
  kernel_long_t ru_oublock;
  kernel_long_t ru_msgsnd;
  kernel_long_t ru_msgrcv;
  kernel_long_t ru_nsignals;
  kernel_long_t ru_nvcsw;
  kernel_long_t ru_nivcsw;
};

// for arb process look at process/resource

auto
getrusage(i32 who, rusage_t &rusage)
{
  return micron::syscall(SYS_getrusage, who, &rusage);
}

};
};
