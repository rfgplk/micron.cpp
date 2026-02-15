//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../except.hpp"
#include "../linux/process/sched.hpp"
#include "../linux/process/system.hpp"
#include "../linux/sys/sched.hpp"
#include "../linux/sys/system.hpp"
#include "../memory/actions.hpp"
#include "../syscall.hpp"

namespace micron
{

enum class schedulers : u32 {
  fifo = posix::sched_fifo,
  roundrobin = posix::sched_rr,
  deadline = posix::sched_deadline,
  normal = posix::sched_other,
  idle = posix::sched_idle,
  batch = posix::sched_batch,
  none
};

schedulers
get_scheduler(void)
{
  auto r = posix::get_scheduler(posix::getpid());     //::sched_getscheduler(posix::getpid());
  if ( r == -1 )
    exc<except::system_error>("micron::scheduling unable to get scheduler");
  return static_cast<schedulers>(r);
}
class scheduler_t
{
  void
  __set(const pid_t pid)
  {
    if ( is_scheduled() ) {
      posix::sched_param prio = { .sched_priority = 0 };     // must be zero
      if ( posix::set_scheduler(pid, (int)sched, prio) == -1 )
        exc<except::system_error>("micron::scheduler_t set() failed");
    } else {
      posix::sched_param prio = { .sched_priority = 99 };
      if ( posix::set_scheduler(pid, (int)sched, prio) == -1 )
        exc<except::system_error>("micron::scheduler_t set() failed");
    }
  }

  schedulers sched;     // for convenience
  sched_attr properties;

public:
  ~scheduler_t() = default;
  scheduler_t(pid_t pid)
  {
    if ( posix::get_attrs(pid, properties, 0) == -1 )
      exc<except::system_error>("micron::scheduler_t unable to get scheduling attributes");
    sched = static_cast<schedulers>(properties.sched_policy);
  }
  scheduler_t(const scheduler_t &o) : sched(o.sched), properties(o.properties) {}
  scheduler_t(scheduler_t &&o) : sched(o.sched), properties(micron::move(o.properties)) { o.sched = schedulers::none; }
  scheduler_t &
  operator=(const scheduler_t &o)
  {
    sched = o.sched;
    properties = o.properties;
    return *this;
  }
  scheduler_t &
  operator=(scheduler_t &&o)
  {
    sched = o.sched;
    properties = micron::move(o.properties);
    o.sched = schedulers::none;
    return *this;
  }
  auto
  get_scheduler(void)
  {
    return sched;
  }
  auto
  get_priority(void) -> u64
  {
    if ( is_scheduled() )
      return (u64)properties.sched_nice;
    else if ( is_realtime() )
      return properties.sched_priority;
    return 0;
  }
  void
  set_scheduler(const schedulers s)
  {
    properties.size = sizeof(struct sched_attr);
    properties.sched_policy = static_cast<u32>(s);
    properties.sched_flag = 0x01;
    if ( is_scheduled(s) ) {
      properties.sched_nice = 0;
      properties.sched_priority = 0;
      properties.sched_runtime = 0x0;
      properties.sched_deadline = 0x0;
      properties.sched_period = 0x0;
    }
    if ( is_realtime(s) )
      properties.sched_priority = 99;
    if ( is_deadline(s) ) {
      properties.sched_runtime = 0xDEAD;
      properties.sched_deadline = 0xDEAD;
      properties.sched_period = 0xDEAD;
      // sched_runtime <= sched_deadline <= sched_period
    }
  }
  void
  enable_deadline(u64 time)
  {
    if ( !is_deadline() or time < 1024 )
      return;
    properties.sched_runtime = static_cast<u64>((double)time * 0.5);
    properties.sched_deadline = static_cast<u64>((double)time * 0.75);
    properties.sched_period = time;
  }
  void
  load_scheduler(const pid_t pid)
  {
    __set(pid);
    if ( posix::set_attrs(pid, properties, 0x00) == -1 )
      exc<except::system_error>("micron::scheduling unable to set scheduler");
  }
  static bool
  is_realtime(const schedulers s)
  {
    if ( s == schedulers::fifo or s == schedulers::roundrobin )
      return true;
    return false;
  }
  static bool
  is_scheduled(const schedulers s)
  {
    if ( s == schedulers::normal or s == schedulers::batch or s == schedulers::idle )
      return true;
    return false;
  }
  static bool
  is_deadline(const schedulers s)
  {
    if ( s == schedulers::deadline )
      return true;
    return false;
  }
  bool
  is_realtime(void) const
  {
    if ( sched == schedulers::fifo or sched == schedulers::roundrobin )
      return true;
    return false;
  }
  bool
  is_scheduled(void) const
  {
    if ( sched == schedulers::normal or sched == schedulers::batch or sched == schedulers::idle )
      return true;
    return false;
  }
  bool
  is_deadline(void) const
  {
    if ( sched == schedulers::deadline )
      return true;
    return false;
  }
};

};
