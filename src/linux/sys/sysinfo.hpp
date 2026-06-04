//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../process/system.hpp"
#include "cpu.hpp"

#include "../../syscall.hpp"
#include "../../types.hpp"

namespace micron
{

struct sysinfo {
  ~sysinfo() = default;

  sysinfo(void)
  {
    long r = micron::syscall(SYS_sysinfo, this);
    (void)r;
  }

  sysinfo(const sysinfo &) = default;
  sysinfo(sysinfo &&) = default;
  sysinfo &operator=(const sysinfo &) = default;
  sysinfo &operator=(sysinfo &&) = default;

  kernel_long_t uptime = 0;              /* Seconds since boot */
  kernel_ulong_t loads[3] = { 0, 0, 0 }; /* 1, 5, and 15 minute load averages */
  kernel_ulong_t totalram = 0;           /* Total usable main memory size */
  kernel_ulong_t freeram = 0;            /* Available memory size */
  kernel_ulong_t sharedram = 0;          /* Amount of shared memory */
  kernel_ulong_t bufferram = 0;          /* Memory used by buffers */
  kernel_ulong_t totalswap = 0;          /* Total swap space size */
  kernel_ulong_t freeswap = 0;           /* swap space still available */
  u16 procs = 0;                         /* Number of current processes */
  u16 pad = 0;                           /* Explicit padding for m68k */
  kernel_ulong_t totalhigh = 0;          /* Total high memory size */
  kernel_ulong_t freehigh = 0;           /* Available high memory size */
  u32 mem_unit = 0;                      /* Memory unit size in bytes */
#if __wordsize == 32
  // WARNING: libc5 ABI tail-padding; kernel sys_sysinfo writes sizeof(struct sysinfo) bytes, which on 32-bit word size is 8 bytes beyond
  // mem_unit omitting these causes the syscall to clobber the stack canary on arm32
  char _f[20 - 2 * sizeof(kernel_ulong_t) - sizeof(u32)];
#endif
};

constexpr const static int __max_cpus = 65535;
constexpr const static int __bits_size = posix::cpu_bits_size(65535);

constexpr int
getpagesize(void)
{
  return __micron_page_size_default;      // arch base page (conservative 64K on arm64)
}

inline usize
getpagesizelive(void) noexcept
{
  static usize cached = 0;
  if ( cached ) return cached;
  usize ps = static_cast<usize>(getpagesize());      // fallback if the probe fails
  long fd = micron::syscall(SYS_openat, -100 /* AT_FDCWD */, "/proc/self/auxv", 0 /* O_RDONLY */, 0);
  if ( !micron::syscall_failed(fd) ) {
    unsigned long e[2];      // each auxv entry is { a_type, a_val }, both unsigned long
    for ( ;; ) {
      long n = micron::syscall(SYS_read, fd, &e[0], sizeof(e));
      if ( n != static_cast<long>(sizeof(e)) ) break;
      if ( e[0] == 6UL ) {      // AT_PAGESZ
        if ( e[1] ) ps = static_cast<usize>(e[1]);
        break;
      }
      if ( e[0] == 0UL ) break;      // AT_NULL terminates the auxv
    }
    micron::syscall(SYS_close, fd);
  }
  cached = ps;
  return ps;
}

long int
get_phys_pages(void)
{
  sysinfo sys;
  unsigned long int _page_size = getpagesizelive();
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
  usize memory;
  usize free_memory;
  usize shared_memory;
  usize buffer_memory;
  usize total_memory;
  usize total_swap;
  usize free_swap;
  usize procs;
  usize mem_unit;

  resources() { __impl_rs(); }

  resources(const resources &) = default;
  resources(resources &&) = default;
  resources &operator=(resources &&) = default;
  resources &operator=(const resources &) = default;

  resources &
  operator()(void)
  {
    __impl_rs();      // update
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
};      // namespace micron
