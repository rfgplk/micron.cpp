//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "scheduling.hpp"

#include "../linux/sys/cpu.hpp"

#include "../linux/process/system.hpp"
#include "../linux/sys/resource.hpp"

#include "../../external/cpuid.h"
#include "../io/console.hpp"
#include "../io/filesystem.hpp"
#include "../slice.hpp"
#include "../string/strings.hpp"
#include "../string/unistring.hpp"
#include "../types.hpp"

#include "tasks.hpp"

// the definitive header file including all general CPU handling code (affinity, priority, parking cores, thread maps
// etc)

namespace micron
{
inline unsigned
cpu_count(void)
{
  return (unsigned)maximum_leaf();
}

inline unsigned
which_cpu(void)
{
  return posix::getcpu();
}

inline void
set_priority(int prio, int pid = posix::getpid())
{
  posix::set_priority(posix::prio_process, pid, prio);
  //::setpriority(prio_process, pid, prio);
}

inline unsigned
park_cpu(unsigned c = 0xFFFFFFFF)
{
  if ( c == 0xFFFFFFFF ) {
    c = posix::getcpu();     // use getcpu instead of sched_getcpu
  }
  posix::cpu_set_t set(c);
  posix::sched_setaffinity(0, sizeof(set), set);
  return c;
}

inline unsigned
park_cpu_pid(pid_t pid, unsigned c = 0xFFFFFFFF)
{
  if ( c == 0xFFFFFFFF ) {
    c = posix::getcpu();     // use getcpu instead of sched_getcpu
  }
  posix::cpu_set_t set(c);
  posix::sched_setaffinity(pid, sizeof(set), set);
  return c;
}
inline void
park_cpu(pid_t pid, posix::cpu_set_t &set)
{
  posix::sched_setaffinity(pid, sizeof(set), set);
}

inline void
enable_all_cores(pid_t pid = 0)
{
  posix::cpu_set_t set;
  auto count = cpu_count();
  for ( size_t i = 0; i <= count; i++ )
    set.cpu_set(i);
  posix::sched_setaffinity(pid, sizeof(set), set);
}

template <bool D = false> class cpu_t : public scheduler_t
{
  pid_t pid;
  posix::cpu_set_t procs;
  unsigned count;

public:
  cpu_t(void) : scheduler_t(posix::getpid()), pid(posix::getpid()), procs(), count(cpu_count())
  {
    if constexpr ( D ) {
      for ( size_t i = 0; i <= count; i++ )
        procs.cpu_set(i);
    }     // default to all threads
  }
  bool
  operator[](const size_t n) const
  {
    if ( n >= count )
      throw except::library_error("micron cpu_t::operator[] out of range");
    return procs.cpu_isset(n);
  }
  void
  clear(void)
  {
    procs.cpu_zero();
  }
  auto &
  get()
  {
    return procs;
  }
  bool
  at(const size_t n) const
  {
    if ( n >= count )
      throw except::library_error("micron cpu_t::operator[] out of range");
    return procs.cpu_isset(n);
  }

  void
  set_core(const size_t n)
  {
    if ( n >= count )
      throw except::library_error("micron cpu_t::operator[] out of range");
    procs.cpu_set(n);
  }
  void
  update_cores(void)
  {
    posix::set_affinity(pid, procs);
  }
  void
  update(void)
  {
    enable_all_cores(pid);
    load_scheduler(pid);
    posix::set_affinity(pid, procs);
  }
};

};
