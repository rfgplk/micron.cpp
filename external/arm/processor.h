// nanoflagsarm (for C99 and onwards)
// https://github.com/rfgplk/nanoflagsarm
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2024 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "features.h"
#include "sysreg.h"
#include "macros.h"

// NOTE: unlike on 8086, with arm you don't have a main cpuid like inst

BEGIN_C_NS

__inline__ int
__nf_strlen(const char *__restrict__ s)
{
  int n = 0;
  while ( s[n] )
    n++;
  return n;
}

#ifdef armFEATURES_CPUSET

#ifdef ARCH_AARCH64
#define __NF_NR_getpid             172
#define __NF_NR_sched_setaffinity  122
#define __NF_NR_sched_getaffinity  123
#else
#define __NF_NR_getpid             20
#define __NF_NR_sched_setaffinity  241
#define __NF_NR_sched_getaffinity  242
#endif

#define __NF_CPU_SETSIZE  1024
#define __NF_NCPUBITS     (8 * sizeof(unsigned long))

typedef struct {
  unsigned long __bits[__NF_CPU_SETSIZE / __NF_NCPUBITS];
} __nf_cpu_set_t;

__inline__ long
__nf_syscall0(long nr)
{
#ifdef ARCH_AARCH64
  register long x8 __asm__("x8") = nr;
  register long x0 __asm__("x0");
  __asm__ volatile("svc #0" : "=r"(x0) : "r"(x8) : "memory");
  return x0;
#else
  register long r7 __asm__("r7") = nr;
  register long r0 __asm__("r0");
  __asm__ volatile("svc #0" : "=r"(r0) : "r"(r7) : "memory");
  return r0;
#endif
}

__inline__ long
__nf_syscall3(long nr, long a0, long a1, long a2)
{
#ifdef ARCH_AARCH64
  register long x8 __asm__("x8") = nr;
  register long x0 __asm__("x0") = a0;
  register long x1 __asm__("x1") = a1;
  register long x2 __asm__("x2") = a2;
  __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
  return x0;
#else
  register long r7 __asm__("r7") = nr;
  register long r0 __asm__("r0") = a0;
  register long r1 __asm__("r1") = a1;
  register long r2 __asm__("r2") = a2;
  __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory");
  return r0;
#endif
}

int
topology_set_cpu(const unsigned long n)
{
  long pid = __nf_syscall0(__NF_NR_getpid);
  __nf_cpu_set_t cpu;

  if ( __nf_syscall3(__NF_NR_sched_getaffinity, pid,
                     (long)sizeof(cpu), (long)&cpu) < 0 )
    return 0;

  for ( unsigned i = 0; i < sizeof(cpu.__bits) / sizeof(cpu.__bits[0]); i++ )
    cpu.__bits[i] = 0;

  cpu.__bits[n / __NF_NCPUBITS] |= (1UL << (n % __NF_NCPUBITS));

  if ( __nf_syscall3(__NF_NR_sched_setaffinity, pid,
                     (long)sizeof(cpu), (long)&cpu) < 0 )
    return 0;

  return 1;
}

#else

__inline__ int
topology_set_cpu(const unsigned long n)
{
  (void)n;
  return 0;
}

#endif

__inline__ void
cache_line_sizes(unsigned int *__restrict__ dcache_line,
                 unsigned int *__restrict__ icache_line)
{
  unsigned long ctr;
  read_ctr(&ctr);
  *icache_line = 4U << rfield(ctr, 3, 0);
  *dcache_line = 4U << rfield(ctr, 19, 16);
}

__inline__ unsigned long
__read_sysfs_ulong(const char *__restrict__ path)
{
  unsigned long val = 0;
  FILE *f = fopen(path, "r");
  if ( f ) {
    char buf[64];
    if ( fgets(buf, sizeof(buf), f) )
      val = strtoul(buf, (void *)0, 10);
    fclose(f);
  }
  return val;
}

__inline__ void
__read_sysfs_str(const char *__restrict__ path,
                 char *__restrict__ buf, int len)
{
  buf[0] = '\0';
  FILE *f = fopen(path, "r");
  if ( f ) {
    if ( fgets(buf, len, f) ) {
      int n = __nf_strlen(buf);
      if ( n > 0 && buf[n - 1] == '\n' )
        buf[n - 1] = '\0';
    }
    fclose(f);
  }
}

__inline__ unsigned long
__parse_size_str(const char *__restrict__ s)
{
  unsigned long val = strtoul(s, (void *)0, 10);
  int n = __nf_strlen(s);
  if ( n > 0 ) {
    char suffix = s[n - 1];
    if ( suffix == 'K' || suffix == 'k' )
      val *= 1024UL;
    else if ( suffix == 'M' || suffix == 'm' )
      val *= 1024UL * 1024UL;
  }
  return val;
}

__inline__ void
cache_from_sysfs(const int cpu, const int index, cache_t *__restrict__ r)
{
  char path[128];
  char tbuf[32];

  r->type = NO_CACHE;
  r->level = 0;
  r->line_size = 0;
  r->associativity = 0;
  r->sets = 0;
  r->size = 0;

  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cache/index%d/type", cpu, index);
  __read_sysfs_str(path, tbuf, sizeof(tbuf));

  if ( tbuf[0] == '\0' )
    return;

  if ( tbuf[0] == 'D' )
    r->type = DATA_CACHE;
  else if ( tbuf[0] == 'I' )
    r->type = INSTRUCTION_CACHE;
  else if ( tbuf[0] == 'U' )
    r->type = UNIFIED_CACHE;
  else
    return;

  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cache/index%d/level", cpu, index);
  r->level = (unsigned short)__read_sysfs_ulong(path);

  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cache/index%d/coherency_line_size",
           cpu, index);
  r->line_size = (unsigned int)__read_sysfs_ulong(path);

  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cache/index%d/ways_of_associativity",
           cpu, index);
  r->associativity = (unsigned int)__read_sysfs_ulong(path);

  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cache/index%d/number_of_sets",
           cpu, index);
  r->sets = (unsigned int)__read_sysfs_ulong(path);

  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cache/index%d/size", cpu, index);
  __read_sysfs_str(path, tbuf, sizeof(tbuf));
  if ( tbuf[0] )
    r->size = __parse_size_str(tbuf);
  else
    r->size = (unsigned long)r->line_size * r->associativity * r->sets;
}

__inline__ void
frequency_user(core_t (*ptr)[256])
{
  unsigned long long start, end;
  start = read_cntvct();
  sleep(1);
  end = read_cntvct();
  unsigned long long freq = end - start;
  for ( int i = 0; i < 256; i++ )
    (*ptr)[i].timer_freq = freq;
}

__inline__ void
cores_all(char *__restrict__ total, core_t (*ptr)[256])
{
  long ncpu = sysconf(_SC_NPROCESSORS_CONF);
  if ( ncpu <= 0 )
    ncpu = 1;
  if ( ncpu > 256 )
    ncpu = 256;
  *total = (char)ncpu;

  for ( int i = 0; i < ncpu; i++ ) {
    (*ptr)[i].type = CORE_TYPE_CORE;
    (*ptr)[i].cluster = 0;

    char path[128];
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
    FILE *f = fopen(path, "r");
    if ( f ) {
      int cl = 0;
      if ( fscanf(f, "%d", &cl) == 1 )
        (*ptr)[i].cluster = (unsigned char)cl;
      fclose(f);
    }
  }

  for ( int i = (int)ncpu; i < 256; i++ )
    (*ptr)[i].type = CORE_TYPE_NONE;
}

END_C_NS
