//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../memory/cmemory.hpp"
#include "../../syscall.hpp"
#include "system.hpp"

namespace micron
{

namespace posix
{

// in order of defines
enum class limits : rlim_t {
  cpu_time = 0,
  file_size = 1,
  data_segment = 2,
  stack_size = 3,
  core = 4,
  resident_set = 5,
  processes = 6,
  files = 7,
  memlocks = 8,
  virtual_memory = 9,
  locks = 10,
  sigpending = 11,
  queue = 12,
  nice = 13,
  realtime_prio = 14,
  realtime_limit = 15,
  none
};

constexpr rlim_t rlim_infinity = 0xFFFFFFFFFFFFFFFFuLL;

struct rlimit_t {
  rlim_t rlim_cur;
  rlim_t rlim_max;
};

auto
get_limits(limits lm, rlimit_t &rl)
{
  return micron::syscall(SYS_getrlimit, static_cast<int>(lm), &rl);
}

auto
set_limits(limits lm, rlimit_t &rl)
{
  return micron::syscall(SYS_setrlimit, static_cast<int>(lm), &rl);
}

struct limits_t {
  rlimit_t lim[16];
  ~limits_t() = default;
  limits_t(void)
  {
    for ( rlim_t i = 0; i < 16; i++ )
      get_limits(static_cast<limits>(i), lim[i]);
  }
  limits_t(const limits_t &o) { micron::bytecpy(lim, o.lim, sizeof(rlimit_t) * 16); }
  limits_t(limits_t &&o)
  {
    micron::bytecpy(lim, o.lim, sizeof(rlimit_t) * 16);
    micron::memset(o.lim, 0x0, sizeof(rlimit_t) * 16);
  }
  limits_t &
  operator=(const limits_t &o)
  {
    micron::bytecpy(lim, o.lim, sizeof(rlimit_t) * 16);
    return *this;
  }
  limits_t &
  operator=(limits_t &&o)
  {
    micron::bytecpy(lim, o.lim, sizeof(rlimit_t) * 16);
    micron::memset(o.lim, 0x0, sizeof(rlimit_t) * 16);

    return *this;
  }
};

};

};
