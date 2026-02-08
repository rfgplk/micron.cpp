//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../memory/cmemory.hpp"
#include "../../syscall.hpp"
#include "system.hpp"

#include "../__includes.hpp"

namespace micron
{

namespace posix
{
constexpr i32 nr_open = 1024;

constexpr i32 ngroups_max = 65536;    /* supplemental group ids are available */
constexpr i32 arg_max = 131072;       /* # bytes of args + environ for exec() */
constexpr i32 link_max = 127;         /* # links a file may have */
constexpr i32 max_canon = 255;        /* size of the canonical input queue */
constexpr i32 max_input = 255;        /* size of the type-ahead buffer */
constexpr i32 name_max = 255;         /* # chars in a file name */
constexpr i32 path_max = 4096;        /* # chars in a path name including nul */
constexpr i32 pipe_buf = 4096;        /* # bytes in atomic write to a pipe */
constexpr i32 xattr_name_max = 255;   /* # chars in an extended attribute name */
constexpr i32 xattr_size_max = 65536; /* size of an extended attribute value (64k) */
constexpr i32 xattr_list_max = 65536; /* size of extended attribute namelist (64k) */

constexpr i32 rtsig_max = 32;

constexpr static const int prio_min = -20;
constexpr static const int prio_max = 20;

constexpr static const int prio_process = 0;
constexpr static const int prio_pgrp = 1;
constexpr static const int prio_user = 2;

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

constexpr static const rlim_t rlimit_cpu = 0;
constexpr static const rlim_t rlimit_fsize = 1;
constexpr static const rlim_t rlimit_data = 2;
constexpr static const rlim_t rlimit_stack = 3;
constexpr static const rlim_t rlimit_core = 4;
constexpr static const rlim_t rlimit_rss = 5;
constexpr static const rlim_t rlimit_nproc = 6;
constexpr static const rlim_t rlimit_nofile = 7;
constexpr static const rlim_t rlimit_ofile = 7;
constexpr static const rlim_t rlimit_memlock = 8;
constexpr static const rlim_t rlimit_as = 9;
constexpr static const rlim_t rlimit_locks = 10;
constexpr static const rlim_t rlimit_sigpending = 11;
constexpr static const rlim_t rlimit_msgqueue = 12;
constexpr static const rlim_t rlimit_nice = 13;
constexpr static const rlim_t rlimit_rtprio = 14;
constexpr static const rlim_t rlimit_rttime = 15;

constexpr static const rlim_t rlimit_nlimits = 16;

constexpr static const rlim_t rlim_infinity = 0xFFFFFFFFFFFFFFFFuLL;
constexpr static const rlim_t rlim64_infinity = 0xFFFFFFFFFFFFFFFFuLL;
constexpr static const unsigned long long rlim_saved_max = rlim_infinity;
constexpr static const unsigned long long rlim_saved_cur = rlim_infinity;

struct rlimit_t {
  rlim_t rlim_cur;
  rlim_t rlim_max;
};
auto
getrlimit(limits lm, rlimit_t &rl)
{
  return micron::syscall(SYS_getrlimit, static_cast<i32>(lm), &rl);
}

auto
setrlimit(limits lm, rlimit_t &rl)
{
  return micron::syscall(SYS_setrlimit, static_cast<i32>(lm), &rl);
}
auto
get_limits(limits lm, rlimit_t &rl)
{
  return micron::syscall(SYS_getrlimit, static_cast<i32>(lm), &rl);
}

auto
set_limits(limits lm, rlimit_t &rl)
{
  return micron::syscall(SYS_setrlimit, static_cast<i32>(lm), &rl);
}

auto
get_process_limits(pid_t pid, rlim_t lm, rlimit_t &out)
{
  return micron::syscall(SYS_prlimit64, pid, static_cast<i32>(lm), nullptr, &out);
}

auto
set_process_limits(pid_t pid, rlim_t lm, rlimit_t &in)
{
  return micron::syscall(SYS_prlimit64, pid, static_cast<i32>(lm), &in, nullptr);
}
// with the out
auto
set_process_limits(pid_t pid, rlim_t lm, rlimit_t &in, rlimit_t &out)
{
  return micron::syscall(SYS_prlimit64, pid, static_cast<i32>(lm), &in, &out);
}

auto
prlimit_out(pid_t pid, rlim_t lm, rlimit_t &out)
{
  return micron::syscall(SYS_prlimit64, pid, static_cast<i32>(lm), nullptr, &out);
}

auto
prlimit_in(pid_t pid, rlim_t lm, rlimit_t &in)
{
  return micron::syscall(SYS_prlimit64, pid, static_cast<i32>(lm), &in, nullptr);
}
// with the out
auto
prlimit(pid_t pid, rlim_t lm, rlimit_t &in, rlimit_t &out)
{
  return micron::syscall(SYS_prlimit64, pid, static_cast<i32>(lm), &in, &out);
}

// helper class
struct limits_t {
  rlimit_t lim[rlimit_nlimits];
  ~limits_t() = default;
  limits_t(const pid_t proc = 0)     // for us by default
  {
    for ( rlim_t i = 0; i < rlimit_nlimits; i++ )
      get_process_limits(proc, i, lim[i]);
  }
  limits_t(const limits_t &o) { micron::voidcpy(lim, o.lim, sizeof(rlimit_t) * 16); }
  limits_t(limits_t &&o)
  {
    micron::voidcpy(lim, o.lim, sizeof(rlimit_t) * 16);
    micron::memset(o.lim, 0x0, sizeof(rlimit_t) * 16);
  }
  limits_t &
  operator=(const limits_t &o)
  {
    micron::voidcpy(lim, o.lim, sizeof(rlimit_t) * 16);
    return *this;
  }
  limits_t &
  operator=(limits_t &&o)
  {
    micron::voidcpy(lim, o.lim, sizeof(rlimit_t) * 16);
    micron::memset(o.lim, 0x0, sizeof(rlimit_t) * 16);
    return *this;
  }
};

};
};
;
