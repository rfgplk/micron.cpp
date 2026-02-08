//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../linux/process/system.hpp"
#include "../../linux/sys/cpu.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"

namespace micron
{

struct sysinfo {
  ~sysinfo() = default;
  sysinfo(void) { micron::syscall(SYS_sysinfo, this); }
  sysinfo(const sysinfo &) = default;
  sysinfo(sysinfo &&) = default;
  sysinfo &operator=(const sysinfo &) = default;
  sysinfo &operator=(sysinfo &&) = default;

  kernel_long_t uptime;     /* Seconds since boot */
  kernel_ulong_t loads[3];  /* 1, 5, and 15 minute load averages */
  kernel_ulong_t totalram;  /* Total usable main memory size */
  kernel_ulong_t freeram;   /* Available memory size */
  kernel_ulong_t sharedram; /* Amount of shared memory */
  kernel_ulong_t bufferram; /* Memory used by buffers */
  kernel_ulong_t totalswap; /* Total swap space size */
  kernel_ulong_t freeswap;  /* swap space still available */
  u16 procs;                /* Number of current processes */
  u16 pad;                  /* Explicit padding for m68k */
  kernel_ulong_t totalhigh; /* Total high memory size */
  kernel_ulong_t freehigh;  /* Available high memory size */
  u32 mem_unit;             /* Memory unit size in bytes */
};

constexpr const static int __max_cpus = 65535;
constexpr const static int __bits_size = posix::cpu_bits_size(65535);

constexpr int
getpagesize(void)
{
  return 4096;
}

long int
get_phys_pages(void)
{
  sysinfo sys;
  unsigned long int _page_size = getpagesize();
  while ( sys.mem_unit > 1 && _page_size > 1 ) {
    sys.mem_unit >>= 1;
    _page_size >>= 1;
  }
  sys.totalram *= sys.mem_unit;
  while ( _page_size > 1 ) {
    _page_size >>= 1;
    sys.totalram >>= 1;
  }
  return sys.totalram;
}

// this is a wrapper around Linux spec. fcalls providing necessary system info
// again NO idea why the STL doesn't have this
struct resources {
  size_t memory;
  size_t free_memory;
  size_t shared_memory;
  size_t buffer_memory;
  size_t total_memory;
  size_t total_swap;
  size_t free_swap;
  size_t procs;
  size_t mem_unit;
  resources() { __impl_rs(); }
  resources(const resources &) = default;
  resources(resources &&) = default;
  resources &operator=(resources &&) = default;
  resources &operator=(const resources &) = default;
  resources &
  operator()(void)
  {
    __impl_rs();     // update
    return *this;
  }
  void
  reload(void)
  {
    __impl_rs();
  }

private:
  inline void
  __impl_rs(void)
  {
    sysinfo info;
    memory = info.totalram;
    free_memory = info.freeram;
    shared_memory = info.sharedram;
    buffer_memory = info.bufferram;
    total_memory = info.totalram;
    total_swap = info.totalswap;
    free_swap = info.freeswap;
    procs = info.procs;
    mem_unit = info.mem_unit;
  }
};
};
